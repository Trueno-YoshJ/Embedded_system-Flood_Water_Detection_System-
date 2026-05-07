/*
 * i2c.c
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 */

#include "i2c.h"
#include "gpio.h"

#define I2CCR1PEEN      (1U << 0)
#define I2CCR1START     (1U << 8)
#define I2CCR1STOP      (1U << 9)
#define I2CCR1ACK       (1U << 10)
#define I2CCR1SWRST     (1U << 15)

#define I2CSR1SB        (1U << 0)
#define I2CSR1ADDR      (1U << 1)
#define I2CSR1BTF       (1U << 2)
#define I2CSR1RXNE      (1U << 6)
#define I2CSR1TXE       (1U << 7)

#define IOPBEN          (1U << 3)
#define I2C1EN          (1U << 21)

volatile unsigned char found_addresses[10];
volatile unsigned char found_count = 0;

void i2c_init(void) {

    RCC_APB2ENR |= IOPBEN;
    RCC_APB1ENR |= I2C1EN;

    GPIOB_CRL &= ~(0xFFU << 24);
    GPIOB_CRL |=  (0xBBU << 24);

    I2C_CR1 |=  I2CCR1SWRST;
    I2C_CR1 &= ~I2CCR1SWRST;

    I2C_CR2 |= (8U << 0);
    I2C_CCR   = 0x28;
    I2C_TRISE = 9;

    I2C_CR1 |= I2CCR1PEEN;
}
void i2c_scan(void) {
    found_count = 0;  // reset before scan

    for (unsigned char addr = 1; addr <= 127; addr++) {

        I2C_CR1 |= I2CCR1START;
        while (!(I2C_SR1 & I2CSR1SB));

        I2C_DR = (addr << 1) & 0xFE;

        volatile int timeout = 1000;
        while (!(I2C_SR1 & I2CSR1ADDR) && timeout > 0) {
            timeout--;
        }

        if (timeout > 0) {
            found_addresses[found_count] = addr;  // ✅ store
            found_count++;                         // ✅ increment
            (void)I2C_SR1;
            (void)I2C_SR2;
        }

        I2C_CR1 |= I2CCR1STOP;
        for (volatile int d = 0; d < 1000; d++);
    }
    // ← set breakpoint here to check results
}
void i2c_write(unsigned char address, unsigned char reg, unsigned char data) {

    I2C_CR1 |= I2CCR1START | I2CCR1ACK;
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

void i2c_read(unsigned char address, unsigned char reg, unsigned char *data, unsigned char len) {

    I2C_CR1 |= I2CCR1START | I2CCR1ACK;
    while (!(I2C_SR1 & I2CSR1SB));
    I2C_DR = (address << 1) & 0xFE;
    while (!(I2C_SR1 & I2CSR1ADDR));
    (void)I2C_SR1;
    (void)I2C_SR2;

    while (!(I2C_SR1 & I2CSR1TXE));
    I2C_DR = reg;
    while (!(I2C_SR1 & I2CSR1BTF));

    I2C_CR1 |= I2CCR1START;
    while (!(I2C_SR1 & I2CSR1SB));
    I2C_DR = (address << 1) | 0x01;
    while (!(I2C_SR1 & I2CSR1ADDR));
    (void)I2C_SR1;
    (void)I2C_SR2;

    for (unsigned char i = 0; i < len; i++) {
        if (i + 1 == len) {
            I2C_CR1 &= ~I2CCR1ACK;
            I2C_CR1 |=  I2CCR1STOP;
        }
        while (!(I2C_SR1 & I2CSR1RXNE));
        data[i] = I2C_DR;
    }
}
