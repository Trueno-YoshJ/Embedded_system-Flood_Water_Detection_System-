/*
 * gpio.h
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "base.h"

#define GPIOA_CRL_OFFSET			(0x00)
#define GPIOA_CRL					(*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_CRL_OFFSET))

#define GPIOA_ODR_OFFSET 			(0x0C)
#define GPIOA_ODR					(*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_ODR_OFFSET))
#define GPIOA_IDR_OFFSET 			(0x08)
#define GPIOA_IDR					(*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_IDR_OFFSET))

#define GPIOB_CRL_OFFSET			(0x00)
#define GPIOB_CRL					(*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_CRL_OFFSET))

#define GPIOB_ODR_OFFSET 			(0x0C)
#define GPIOB_ODR					(*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_ODR_OFFSET))

#define GPIOB_IDR_OFFSET 			(0x08h)
#define GPIOB_IDR					(*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_IDR_OFFSET))

//#define RCC_APB2ENR_OFFSET			(0x18)
//#define RCC_APB2ENR					(*(volatile unsigned int *)(RCC_PERIPHERAL + RCC_APB2ENR_OFFSET))


//#define GPIOA_BSRR_OFFSET   (0x10)
//#define GPIOA_BRR_OFFSET    (0x14)
//#define GPIOA_BSRR  (*(volatile unsigned int *)(GPIOA_BASE + GPIOA_BSRR_OFFSET))
//#define GPIOA_BRR   (*(volatile unsigned int *)(GPIOA_BASE + GPIOA_BRR_OFFSET))
//
//#define GPIOB_BSRR_OFFSET   (0x10)
//#define GPIOB_BRR_OFFSET    (0x14)
//#define GPIOB_BSRR  (*(volatile unsigned int *)(GPIOB_BASE + GPIOB_BSRR_OFFSET))
//#define GPIOB_BRR   (*(volatile unsigned int *)(GPIOB_BASE + GPIOB_BRR_OFFSET))


void gpio_init(void);
void gpio_trigger_high(int sensor);
void gpio_trigger_low(int sensor);
int gpio_read_echo(int sensor);
void gpio_buzzer_on(void);
void gpio_buzzer_off(void);


#endif /* GPIO_H_ */





