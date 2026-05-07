/*
 * i2c.h
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 */

#ifndef I2C_H_
#define I2C_H_

#include "base.h"

//#define RCC_APB2ENR_OFFSET		(0x18)
//#define RCC_APB2ENR				(*(volatile unsigned int *)(RCC_PERIPHERAL + RCC_APB2ENR_OFFSET))

//#define RCC_APB1ENR_OFFSET		(0x1C)
//#define RCC_APB1ENR				(*(volatile unsigned int *)(RCC_PERIPHERAL + RCC_APB1ENR_OFFSET))

#define I2C_CR1_OFFSET 			(0x00)
#define I2C_CR1					(*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_CR1_OFFSET))

#define I2C_CR2_OFFSET			(0x04)
#define I2C_CR2					(*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_CR2_OFFSET))

#define I2C_CCR_OFFSET			(0x1C)
#define I2C_CCR					(*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_CCR_OFFSET))

#define I2C_TRISE_OFFSET		(0x20)
#define I2C_TRISE				(*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_TRISE_OFFSET))

#define I2C_SR1_OFFSET 			(0x14)
#define I2C_SR1 				(*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_SR1_OFFSET))

#define I2C_SR2_OFFSET 			(0x18)
#define I2C_SR2 				(*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_SR2_OFFSET))

#define I2C_DR_OFFSET 			(0x10)
#define I2C_DR					(*(volatile unsigned int *)(I2C_PERIPHERAL + I2C_DR_OFFSET))

extern volatile unsigned char found_addresses[10];
extern volatile unsigned char found_count;

void i2c_init(void);
void i2c_scan(void);
void i2c_write(unsigned char address, unsigned char reg, unsigned char data);
void i2c_read(unsigned char address, unsigned char reg, unsigned char *data, unsigned char len);


#endif /* I2C_H_ */
