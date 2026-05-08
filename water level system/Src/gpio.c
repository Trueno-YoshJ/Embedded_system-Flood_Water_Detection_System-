/*
 * gpio.c
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 */

#include "gpio.h"

#define GPIOAEN     (1U << 2)
#define GPIOBEN     (1U << 3)

void gpio_init(void) {

    // Enable clock for GPIOA
    RCC_APB2ENR |= GPIOAEN;

    // Enable clock for GPIOB
    RCC_APB2ENR |= GPIOBEN;

    // PA0 = output push-pull 50MHz (trig1) → 0011
    GPIOA_CRL |=  (1U << 0);
    GPIOA_CRL |=  (1U << 1);
    GPIOA_CRL &= ~(1U << 2);
    GPIOA_CRL &= ~(1U << 3);

    // PA2 = output push-pull 50MHz (trig2) → 0011
    GPIOA_CRL |=  (1U << 8);
    GPIOA_CRL |=  (1U << 9);
    GPIOA_CRL &= ~(1U << 10);
    GPIOA_CRL &= ~(1U << 11);

    // PA1 = input floating (echo1) → 0100
    GPIOA_CRL &= ~(1U << 4);
    GPIOA_CRL &= ~(1U << 5);
    GPIOA_CRL |=  (1U << 6);
    GPIOA_CRL &= ~(1U << 7);

    // PA3 = input floating (echo2) → 0100
    GPIOA_CRL &= ~(1U << 12);
    GPIOA_CRL &= ~(1U << 13);
    GPIOA_CRL |=  (1U << 14);
    GPIOA_CRL &= ~(1U << 15);

    // PB0 = output push-pull 50MHz (buzzer) → 0011
    GPIOB_CRL |=  (1U << 0);
    GPIOB_CRL |=  (1U << 1);
    GPIOB_CRL &= ~(1U << 2);
    GPIOB_CRL &= ~(1U << 3);

    // PB1 = output push-pull 50MHz (VL53L0X XSHUT) → 0011
    GPIOB_CRL |=  (1U << 4);
    GPIOB_CRL |=  (1U << 5);
    GPIOB_CRL &= ~(1U << 6);
    GPIOB_CRL &= ~(1U << 7);
}

void gpio_trigger_high(unsigned char sensor) {
    if (sensor == 1)
        GPIOA_ODR |= (1U << 0);   // PA0 HIGH → trig1
    else
        GPIOA_ODR |= (1U << 2);   // PA2 HIGH → trig2
}

void gpio_trigger_low(unsigned char sensor) {
    if (sensor == 1)
        GPIOA_ODR &= ~(1U << 0);  // PA0 LOW → trig1
    else
        GPIOA_ODR &= ~(1U << 2);  // PA2 LOW → trig2
}

unsigned char gpio_read_echo(unsigned char sensor) {
    if (sensor == 1)
        return (GPIOA_IDR & (1U << 1)) ? 1 : 0;  // read PA1
    else
        return (GPIOA_IDR & (1U << 3)) ? 1 : 0;  // read PA3
}

void gpio_buzzer_on(void) {
    GPIOB_ODR |= (1U << 0);   // PB0 HIGH
}

void gpio_buzzer_off(void) {
    GPIOB_ODR &= ~(1U << 0);  // PB0 LOW
}

void gpio_vl53l0x_xshut_high(void) {
    GPIOB_ODR |= (1U << 1);   // PB1 HIGH - Enable VL53L0X
}

void gpio_vl53l0x_xshut_low(void) {
    GPIOB_ODR &= ~(1U << 1);  // PB1 LOW - Disable VL53L0X
}
