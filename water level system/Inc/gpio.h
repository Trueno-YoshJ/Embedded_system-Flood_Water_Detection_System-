/*
 * gpio.h
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "base.h"

#define GPIOA_CRL_OFFSET            (0x00)
#define GPIOA_CRL                   (*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_CRL_OFFSET))

#define GPIOA_ODR_OFFSET            (0x0C)
#define GPIOA_ODR                   (*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_ODR_OFFSET))

#define GPIOA_IDR_OFFSET            (0x08)
#define GPIOA_IDR                   (*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_IDR_OFFSET))

#define GPIOB_CRL_OFFSET            (0x00)
#define GPIOB_CRL                   (*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_CRL_OFFSET))

#define GPIOB_ODR_OFFSET            (0x0C)
#define GPIOB_ODR                   (*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_ODR_OFFSET))

/* FIX: was (0x08h) — invalid C literal */
#define GPIOB_IDR_OFFSET            (0x08)
#define GPIOB_IDR                   (*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_IDR_OFFSET))

void gpio_init(void);
void gpio_trigger_high(unsigned char sensor);
void gpio_trigger_low(unsigned char sensor);
unsigned char gpio_read_echo(unsigned char sensor);

void gpio_buzzer_on(void);
void gpio_buzzer_off(void);
#endif /* GPIO_H_ */
