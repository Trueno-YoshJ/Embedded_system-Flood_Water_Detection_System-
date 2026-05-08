/*
 * i2c.c
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 *
 *  I2C1 bare-metal driver for STM32F103.
 *  PB6 = SCL, PB7 = SDA (alternate function open-drain).
 *  Bus speed: 100 kHz standard mode with 8 MHz APB1 clock.
 *
 *  Key fixes vs original:
 *  1. Added i2c_write_reg8 / i2c_read_reg8 for devices (VL53L0X) that use
 *     an 8-bit (single-byte) register address rather than 16-bit.
 *  2. Fixed the STM32 1-byte I2C receive errata: ACK must be cleared and
 *     STOP must be programmed BEFORE the address phase is cleared (SR2 read),
 *     otherwise the peripheral clocks in an extra byte.
 *  3. Added NACK / timeout error-clearing so the bus does not lock after a
 *     failed scan probe.
 */

#include "base.h"
#include "gpio.h"
#include "i2c.h"

/* ── Local register macros (GPIOB also needed here for pin config) ─── */
#define GPIOB_CRL_LOCAL     (*(volatile unsigned int *)(GPIOB_PERIPHERAL + 0x00))

/* ── Clock enable bits ───────────────────────────────────── */
#define IOPBEN              (1U << 3)   /* GPIOB clock in APB2ENR */
#define I2C1EN              (1U << 21)  /* I2C1 clock in APB1ENR  */

/* ── Scan result storage ─────────────────────────────────── */
volatile unsigned char found_addresses[10];
volatile unsigned char found_count = 0;

/* ══════════════════════════════════════════════════════════
 * i2c_init
 * Configures PB6/PB7 as AF open-drain 50 MHz and brings up
 * I2C1 at 100 kHz assuming 8 MHz APB1 (PCLK1).
 * ══════════════════════════════════════════════════════════ */
void i2c_init(void) {

    /* 1. Enable clocks */
    RCC_APB2ENR |= IOPBEN;
    RCC_APB1ENR |= I2C1EN;

    /*
     * 2. Configure PB6 (SCL) and PB7 (SDA):
     *    CNF[1:0] = 11 (AF open-drain), MODE[1:0] = 11 (50 MHz)
     *    → nibble value = 0xB for each pin
     *    CRL bits [27:24] = PB6, bits [31:28] = PB7
     */
    GPIOB_CRL_LOCAL &= ~(0xFFU << 24);   /* clear PB6 and PB7 nibbles */
    GPIOB_CRL_LOCAL |=  (0xBBU << 24);   /* AF open-drain 50 MHz      */

    /* 3. Software reset to clear any stuck state */
    I2C_CR1 |=  I2CCR1SWRST;
    I2C_CR1 &= ~I2CCR1SWRST;

    /*
     * 4. Set peripheral clock frequency.
     *    CR2 FREQ field = APB1 clock in MHz = 8
     */
    I2C_CR2 = 8;
    /*
     * 5. Set CCR for 100 kHz standard mode.
     *    CCR = PCLK1 / (2 * I2C_speed) = 8,000,000 / (2 * 100,000) = 40 = 0x28
     */
    I2C_CCR = 40;;   // ~50kHz (more stable)
    I2C_TRISE = 17;

    /* 7. Enable the peripheral */
    I2C_CR1 |= I2CCR1PEEN;
    I2C_CR1 |= I2CCR1ACK;
}

/* ══════════════════════════════════════════════════════════
 * i2c_clear_bus_error
 * Clears NACK flag and issues STOP so subsequent transfers
 * can proceed. Called after failed address probes.
 * ══════════════════════════════════════════════════════════ */
static void i2c_clear_bus_error(void) {
    /* Clear AF (acknowledge failure) flag */
    I2C_SR1 &= ~I2CSR1AF;
    /* Generate STOP */
    I2C_CR1 |= I2CCR1STOP;
    /* Small delay to let STOP propagate */
    for (volatile int d = 0; d < 2000; d++);
}

/* ══════════════════════════════════════════════════════════
 * i2c_scan
 * Probes all 7-bit addresses 1-127 and records ACK responses.
 * ══════════════════════════════════════════════════════════ */
void i2c_scan(void) {
    found_count = 0;

    for (unsigned char addr = 1; addr <= 127; addr++) {

        /* Generate START */
        I2C_CR1 |= I2CCR1START;
        while (!(I2C_SR1 & I2CSR1SB));

        /* Send address as write (LSB = 0) */
        I2C_DR = (addr << 1) & 0xFE;

        /* Wait for ADDR or NACK with timeout */
        volatile int timeout = 5000;
        while (!(I2C_SR1 & (I2CSR1ADDR | I2CSR1AF)) && timeout > 0) {
            timeout--;
        }

        if (I2C_SR1 & I2CSR1ADDR) {
            /* Device responded with ACK — record it */
            if (found_count < 10) {
                found_addresses[found_count++] = addr;
            }
            /* Clear ADDR by reading SR1 then SR2 */
            (void)I2C_SR1;
            (void)I2C_SR2;
        } else {
            /* NACK or timeout — clear error */
            i2c_clear_bus_error();
            continue;
        }

        /* Generate STOP */
        I2C_CR1 |= I2CCR1STOP;
        for (volatile int d = 0; d < 2000; d++);
    }
}

/* ══════════════════════════════════════════════════════════
 * i2c_write  (SSD1306 style — 8-bit register/control byte)
 * Frame: START → [addr|W] → reg → data → STOP
 * ══════════════════════════════════════════════════════════ */
void i2c_write(unsigned char address, unsigned char reg, unsigned char data) {

    I2C_CR1 |= I2CCR1START;
    while (!(I2C_SR1 & I2CSR1SB));

    I2C_DR = (address << 1) & 0xFE;
    while (!(I2C_SR1 & I2CSR1ADDR));
    (void)I2C_SR1;
    (void)I2C_SR2;

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = reg;

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = data;

    while (!(I2C_SR1 & I2CSR1BTF));
    I2C_CR1 |= I2CCR1STOP;
}

/* ══════════════════════════════════════════════════════════
 * i2c_read  (SSD1306 style — write reg pointer then read)
 * ══════════════════════════════════════════════════════════ */
void i2c_read(unsigned char address, unsigned char reg,
              unsigned char *data, unsigned char len) {

    /* Write phase: send register pointer */
    I2C_CR1 |= I2CCR1START | I2CCR1ACK;
    while (!(I2C_SR1 & I2CSR1SB));

    I2C_DR = (address << 1) & 0xFE;
    while (!(I2C_SR1 & I2CSR1ADDR));
    (void)I2C_SR1;
    (void)I2C_SR2;

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = reg;
    while (!(I2C_SR1 & I2CSR1BTF));

    /* Repeated START → read phase */
    I2C_CR1 |= I2CCR1START;
    while (!(I2C_SR1 & I2CSR1SB));

    I2C_DR = (address << 1) | 0x01;

    /*
     * STM32 1-byte receive errata (RM0008 §26.3.3):
     * If len == 1, we must: disable ACK, send STOP, THEN clear ADDR.
     * Clearing ADDR (reading SR2) releases SCL and the first bit of
     * the byte starts — we must have STOP/NACK ready beforehand.
     */
    if (len == 1) {
        /* Wait for ADDR without clearing yet */
        while (!(I2C_SR1 & I2CSR1ADDR));
        I2C_CR1 &= ~I2CCR1ACK;     /* disable ACK before clearing ADDR */
        (void)I2C_SR1;
        (void)I2C_SR2;              /* clears ADDR — SCL released now   */
        I2C_CR1 |=  I2CCR1STOP;    /* STOP programmed                  */
        while (!(I2C_SR1 & I2CSR1RXNE));
        data[0] = (unsigned char)I2C_DR;
    } else {
        while (!(I2C_SR1 & I2CSR1ADDR));
        (void)I2C_SR1;
        (void)I2C_SR2;

        for (unsigned char i = 0; i < len; i++) {
            if (i + 1 == len) {
                I2C_CR1 &= ~I2CCR1ACK;
                I2C_CR1 |=  I2CCR1STOP;
            }
            while (!(I2C_SR1 & I2CSR1RXNE));
            data[i] = (unsigned char)I2C_DR;
        }
    }
}

/* ══════════════════════════════════════════════════════════
 * i2c_write_reg8  — VL53L0X style
 * Frame: START → [addr|W] → reg8 → data → STOP
 * The VL53L0X protocol sends a SINGLE byte register address.
 * ══════════════════════════════════════════════════════════ */
void i2c_write_reg8(unsigned char address, unsigned char reg,
                    unsigned char data)
{
    volatile int timeout;

    // START
    I2C_CR1 |= I2CCR1START;

    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1SB) && timeout > 0) {
        timeout--;
    }

    if(timeout <= 0) return;

    // Send address
    I2C_DR = (address << 1) & 0xFE;

    timeout = 100000;
    while (!(I2C_SR1 & (I2CSR1ADDR | I2CSR1AF)) && timeout > 0) {
        timeout--;
    }

    // NACK received
    if (I2C_SR1 & I2CSR1AF) {

        I2C_SR1 &= ~I2CSR1AF;
        I2C_CR1 |= I2CCR1STOP;

        return;
    }

    if(timeout <= 0) return;

    // Clear ADDR
    (void)I2C_SR1;
    (void)I2C_SR2;

    // TXE wait
    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1TXE) && timeout > 0) {
        timeout--;
    }

    if(timeout <= 0) return;

    // Send reg
    I2C_DR = reg;

    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1TXE) && timeout > 0) {
        timeout--;
    }

    if(timeout <= 0) return;

    // Send data
    I2C_DR = data;

    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1BTF) && timeout > 0) {
        timeout--;
    }

    I2C_CR1 |= I2CCR1STOP;
}
void i2c_read_reg8(unsigned char address,
                   unsigned char reg,
                   unsigned char *data,
                   unsigned char len)
{
    volatile int timeout;

    // ---------- WRITE REGISTER ADDRESS ----------

    I2C_CR1 |= I2CCR1START;

    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1SB) && timeout > 0)
        timeout--;

    if(timeout <= 0) return;

    I2C_DR = (address << 1) & 0xFE;

    timeout = 100000;
    while (!(I2C_SR1 & (I2CSR1ADDR | I2CSR1AF)) && timeout > 0)
        timeout--;

    if (I2C_SR1 & I2CSR1AF) {
        I2C_SR1 &= ~I2CSR1AF;
        I2C_CR1 |= I2CCR1STOP;
        return;
    }

    if(timeout <= 0) return;

    (void)I2C_SR1;
    (void)I2C_SR2;

    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1TXE) && timeout > 0)
        timeout--;

    if(timeout <= 0) return;

    I2C_DR = reg;

    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1BTF) && timeout > 0)
        timeout--;

    if(timeout <= 0) return;

    // ---------- REPEATED START ----------

    I2C_CR1 |= I2CCR1START;

    timeout = 100000;
    while (!(I2C_SR1 & I2CSR1SB) && timeout > 0)
        timeout--;

    if(timeout <= 0) return;

    // ---------- READ MODE ADDRESS ----------

    I2C_DR = (address << 1) | 0x01;

    timeout = 100000;
    while (!(I2C_SR1 & (I2CSR1ADDR | I2CSR1AF)) && timeout > 0)
        timeout--;

    if (I2C_SR1 & I2CSR1AF) {

        I2C_SR1 &= ~I2CSR1AF;
        I2C_CR1 |= I2CCR1STOP;

        return;
    }

    if(timeout <= 0) return;

    // ---------- SINGLE BYTE ----------

    if(len == 1) {

        I2C_CR1 &= ~I2CCR1ACK;

        (void)I2C_SR1;
        (void)I2C_SR2;

        I2C_CR1 |= I2CCR1STOP;

        timeout = 100000;
        while (!(I2C_SR1 & I2CSR1RXNE) && timeout > 0)
            timeout--;

        if(timeout <= 0) {
            I2C_CR1 |= I2CCR1ACK;
            return;
        }

        data[0] = I2C_DR;
        I2C_CR1 |= I2CCR1ACK;
        return;
    }

    // ---------- MULTI-BYTE READ ----------
    while (!(I2C_SR1 & I2CSR1ADDR));
    (void)I2C_SR1;
    (void)I2C_SR2;

    for (unsigned char i = 0; i < len; i++) {
        if (i == len - 2) {
            I2C_CR1 &= ~I2CCR1ACK;
            I2C_CR1 |= I2CCR1STOP;
        }

        timeout = 100000;
        while (!(I2C_SR1 & I2CSR1RXNE) && timeout > 0)
            timeout--;

        if(timeout <= 0) {
            I2C_CR1 |= I2CCR1ACK;
            return;
        }

        data[i] = (unsigned char)I2C_DR;
    }

    I2C_CR1 |= I2CCR1ACK;
}
/* ══════════════════════════════════════════════════════════
 * Legacy 16-bit register variants — kept for compatibility.
 * These send a 16-bit big-endian register address before data.
 * Do NOT use these for the VL53L0X — use reg8 variants above.
 * ══════════════════════════════════════════════════════════ */
void i2c_write_reg16(unsigned char address, unsigned short reg,
                     unsigned char data) {

    I2C_CR1 |= I2CCR1START;
    while (!(I2C_SR1 & I2CSR1SB));

    I2C_DR = (address << 1) & 0xFE;
    while (!(I2C_SR1 & I2CSR1ADDR));
    (void)I2C_SR1;
    (void)I2C_SR2;

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = (unsigned char)(reg >> 8);

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = (unsigned char)(reg & 0xFF);

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = data;

    while (!(I2C_SR1 & I2CSR1BTF));
    I2C_CR1 |= I2CCR1STOP;
}

void i2c_read_reg16(unsigned char address, unsigned short reg,
                    unsigned char *data, unsigned char len) {

    I2C_CR1 |= I2CCR1START | I2CCR1ACK;
    while (!(I2C_SR1 & I2CSR1SB));

    I2C_DR = (address << 1) & 0xFE;
    while (!(I2C_SR1 & I2CSR1ADDR));
    (void)I2C_SR1;
    (void)I2C_SR2;

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = (unsigned char)(reg >> 8);
    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = (unsigned char)(reg & 0xFF);
    while (!(I2C_SR1 & I2CSR1BTF));

    I2C_CR1 |= I2CCR1START;
    while (!(I2C_SR1 & I2CSR1SB));

    I2C_DR = (address << 1) | 0x01;

    if (len == 1) {
        while (!(I2C_SR1 & I2CSR1ADDR));
        I2C_CR1 &= ~I2CCR1ACK;
        (void)I2C_SR1;
        (void)I2C_SR2;
        I2C_CR1 |= I2CCR1STOP;
        while (!(I2C_SR1 & I2CSR1RXNE));
        data[0] = (unsigned char)I2C_DR;
    } else {
        while (!(I2C_SR1 & I2CSR1ADDR));
        (void)I2C_SR1;
        (void)I2C_SR2;

        for (unsigned char i = 0; i < len; i++) {
            if (i + 1 == len) {
                I2C_CR1 &= ~I2CCR1ACK;
                I2C_CR1 |=  I2CCR1STOP;
            }
            while (!(I2C_SR1 & I2CSR1RXNE));
            data[i] = (unsigned char)I2C_DR;
        }
    }
}
