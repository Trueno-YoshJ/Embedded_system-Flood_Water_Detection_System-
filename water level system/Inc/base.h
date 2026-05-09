/*
 * base.h
 *
 *  Created on: Apr 25, 2026
 *      Author: Hp
 */

#ifndef BASE_H_ // Prevents multiple inclusion of the same header , Avoids compilation errors
#define BASE_H_   

#define PERIPHERAL_BASE				(0x40000000) 
//This is the starting memory address for all peripherals in STM32.(GPIO, RCC, I2C, timers  all start from here)

//APB1 → low-speed peripherals
#define APB1_PERIPHERAL_OFFSET  	(0x0000)  
#define APB1_PERIPHERAL    			(PERIPHERAL_BASE + APB1_PERIPHERAL_OFFSET)

//APB2 → GPIO, ADC
#define APB2_PERIPHERAL_OFFSET  	(0x00010000)
#define APB2_PERIPHERAL    			(PERIPHERAL_BASE + APB2_PERIPHERAL_OFFSET)
//Used later for GPIO address calculation

//AHB → high-speed core peripherals
#define AHB_PERIPHERAL_OFFSET   	(0x00020000)
#define AHB_PERIPHERAL 				(PERIPHERAL_BASE + AHB_PERIPHERAL_OFFSET) 

#define I2C_PERIPHERAL_OFFSET 		(0x00005400)
#define I2C_PERIPHERAL				(APB1_PERIPHERAL + I2C_PERIPHERAL_OFFSET)

//Defines where GPIO Port A registers are located
#define GPIOA_PERIPHERAL_OFFSET 	(0x00000800)
//Our sensor pins & buzzer pins are connected here
#define GPIOA_PERIPHERAL			(APB2_PERIPHERAL + GPIOA_PERIPHERAL_OFFSET)

#define GPIOB_PERIPHERAL_OFFSET 	(0x00000C00)
#define GPIOB_PERIPHERAL			(APB2_PERIPHERAL + GPIOB_PERIPHERAL_OFFSET)

//RCC controls clock signals to peripherals. Without enabling clock ; GPIO will not work
#define RCC_PERIPHERAL_OFFSET		(0x00001000)
#define RCC_PERIPHERAL 				(AHB_PERIPHERAL + RCC_PERIPHERAL_OFFSET)

//rcc register
//Enables clock for: GPIOA,  GPIOB
#define RCC_APB2ENR_OFFSET			(0x18)
#define RCC_APB2ENR					(*(volatile unsigned int *)(RCC_PERIPHERAL + RCC_APB2ENR_OFFSET))

// volatile → tells compiler value can change anytime , Required for hardware registers
#define RCC_APB1ENR_OFFSET			(0x1C)
#define RCC_APB1ENR					(*(volatile unsigned int *)(RCC_PERIPHERAL + RCC_APB1ENR_OFFSET))

#endif /* BASE_H_ */
