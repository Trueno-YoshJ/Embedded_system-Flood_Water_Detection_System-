/* File information comment: describes the file name, date created, and author */
 /*
  * gpio.h
  *
  * Created on: Apr 25, 2026
  * Author: Hp
  */

/* Header guard start: prevents this file from being included multiple times */
#ifndef GPIO_H_
#define GPIO_H_

/* Include base.h to get peripheral base addresses and RCC definitions */
#include "base.h"

/* Offset address of GPIOA Configuration Low Register (CRL) */
#define GPIOA_CRL_OFFSET            (0x00)

/* Macro to directly access GPIOA CRL register using memory-mapped I/O */
#define GPIOA_CRL                   (*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_CRL_OFFSET))

/* Offset address of GPIOA Output Data Register (ODR) */
#define GPIOA_ODR_OFFSET            (0x0C)

/* Macro to access GPIOA ODR register to set output pins HIGH or LOW */
#define GPIOA_ODR                   (*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_ODR_OFFSET))

/* Offset address of GPIOA Input Data Register (IDR) */
#define GPIOA_IDR_OFFSET            (0x08)

/* Macro to read GPIOA input pin values from IDR register */
#define GPIOA_IDR                   (*(volatile unsigned int *)(GPIOA_PERIPHERAL + GPIOA_IDR_OFFSET))

/* Offset address of GPIOB Configuration Low Register (CRL) */
#define GPIOB_CRL_OFFSET            (0x00)

/* Macro to access GPIOB CRL register for pin configuration */
#define GPIOB_CRL                   (*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_CRL_OFFSET))

/* Offset address of GPIOB Output Data Register (ODR) */
#define GPIOB_ODR_OFFSET            (0x0C)

/* Macro to access GPIOB ODR register to control output pins */
#define GPIOB_ODR                   (*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_ODR_OFFSET))

/* Comment explaining a previous bug fix: 0x08h is invalid in C */
 /* FIX: was (0x08h) — invalid C literal */

/* Offset address of GPIOB Input Data Register (IDR) */
#define GPIOB_IDR_OFFSET            (0x08)

/* Macro to read input values from GPIOB IDR register */
#define GPIOB_IDR                   (*(volatile unsigned int *)(GPIOB_PERIPHERAL + GPIOB_IDR_OFFSET))

/* Function declaration to initialize GPIO pins (input/output configuration) */
void gpio_init(void);

/* Function to set sensor trigger pin HIGH */
void gpio_trigger_high(unsigned char sensor);

/* Function to set sensor trigger pin LOW */
void gpio_trigger_low(unsigned char sensor);

/* Function to read echo signal from a given sensor */
unsigned char gpio_read_echo(unsigned char sensor);

/* Function to turn ON the buzzer */
void gpio_buzzer_on(void);

/* Function to turn OFF the buzzer */
void gpio_buzzer_off(void);

/* End of header guard */
#endif /* GPIO_H_ */
