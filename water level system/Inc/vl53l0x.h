/*
 * vl53l0x.h
 *
 *  VL53L0X Time-of-Flight distance sensor driver.
 *  Uses 8-bit register addressing over I2C (i2c_write_reg8 / i2c_read_reg8).
 *  Default I2C address: 0x29.
 *  XSHUT pin: PB1 (active-low shutdown, high = operating).
 */

#ifndef VL53L0X_H_
#define VL53L0X_H_

#include "i2c.h"

/* Default I2C address (7-bit) */
#define VL53L0X_ADDR    0x29

/*
 * Initialise the sensor:
 *   - Pulse XSHUT to hardware-reset the device
 *   - Verify model ID (0xEE)
 *   - Load ST reference initialisation sequence
 *   - Configure single-shot ranging mode
 * Sets an internal flag if sensor is not found so that subsequent
 * calls to vl53l0x_read_distance_mm() return 0 safely.
 */
void vl53l0x_init(void);

/*
 * Trigger one single-shot measurement and return the result in mm.
 * Returns 0 if the sensor was not detected at init time,
 * or if the measurement times out.
 * Typical measurement time: 20-30 ms.
 */
unsigned int vl53l0x_read_distance_mm(void);

/*
 * Returns 1 if the sensor was detected and successfully initialised,
 * 0 otherwise.
 */
unsigned char vl53l0x_is_sensor_detected(void);

#endif /* VL53L0X_H_ */
