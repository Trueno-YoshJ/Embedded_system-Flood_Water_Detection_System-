/*
 * i2c.h
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 *
 *  The VL53L0X uses 8-bit register addresses.
 *  The SSD1306 uses a control-byte + data model (handled via i2c_write).
 *  Both devices share I2C1 (PB6=SCL, PB7=SDA).
 */

#ifndef I2C_H_
#define I2C_H_

#include "base.h"

/* ── I2C1 Register Map ───────────────────────────────────── */
#define I2C_CR1_OFFSET      (0x00)
#define I2C_CR1             (*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_CR1_OFFSET))

#define I2C_CR2_OFFSET      (0x04)
#define I2C_CR2             (*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_CR2_OFFSET))

#define I2C_CCR_OFFSET      (0x1C)
#define I2C_CCR             (*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_CCR_OFFSET))

#define I2C_TRISE_OFFSET    (0x20)
#define I2C_TRISE           (*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_TRISE_OFFSET))

#define I2C_SR1_OFFSET      (0x14)
#define I2C_SR1             (*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_SR1_OFFSET))

#define I2C_SR2_OFFSET      (0x18)
#define I2C_SR2             (*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_SR2_OFFSET))

#define I2C_DR_OFFSET       (0x10)
#define I2C_DR              (*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_DR_OFFSET))

/* ── CR1 Bit Masks ───────────────────────────────────────── */
#define I2CCR1PEEN          (1U << 0)
#define I2CCR1START         (1U << 8)
#define I2CCR1STOP          (1U << 9)
#define I2CCR1ACK           (1U << 10)
#define I2CCR1SWRST         (1U << 15)

/* ── SR1 Bit Masks ───────────────────────────────────────── */
#define I2CSR1SB            (1U << 0)   /* Start bit generated       */
#define I2CSR1ADDR          (1U << 1)   /* Address sent / matched    */
#define I2CSR1BTF           (1U << 2)   /* Byte transfer finished    */
#define I2CSR1RXNE          (1U << 6)   /* RX register not empty     */
#define I2CSR1TXE           (1U << 7)   /* TX register empty         */
#define I2CSR1AF            (1U << 10)  /* Acknowledge failure       */

/* ── Scan results ────────────────────────────────────────── */
extern volatile unsigned char found_addresses[10];
extern volatile unsigned char found_count;

/* ── Public API ──────────────────────────────────────────── */

/* Initialise I2C1 peripheral (100 kHz, 8 MHz APB1) */
void i2c_init(void);

/* Scan bus and store found addresses in found_addresses[] */
void i2c_scan(void);

/*
 * SSD1306-style write: START → addr_W → reg (control byte) → data → STOP
 * reg  = SSD1306_CTRL_CMD (0x00) or SSD1306_CTRL_DATA (0x40)
 */
void i2c_write(unsigned char address, unsigned char reg, unsigned char data);

/*
 * SSD1306-style read: write reg pointer then repeated-start read.
 * Also used internally.
 */
void i2c_read(unsigned char address, unsigned char reg,
              unsigned char *data, unsigned char len);

/*
 * VL53L0X write: START → addr_W → reg8 → data → STOP
 * The VL53L0X uses single-byte (8-bit) register addresses.
 */
void i2c_write_reg8(unsigned char address, unsigned char reg,
                    unsigned char data);

/*
 * VL53L0X read: write 8-bit reg pointer then repeated-start read N bytes.
 * Handles the STM32 1-byte receive errata correctly.
 */
void i2c_read_reg8(unsigned char address, unsigned char reg,
                   unsigned char *data, unsigned char len);

/* Legacy 16-bit register variants kept for compatibility */
void i2c_write_reg16(unsigned char address, unsigned short reg,
                     unsigned char data);
void i2c_read_reg16(unsigned char address, unsigned short reg,
                    unsigned char *data, unsigned char len);

#endif /* I2C_H_ */
