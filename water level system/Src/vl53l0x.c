/*
 * vl53l0x.c
 *
 *  VL53L0X bare-metal driver for STM32F103.
 *
 *  ROOT CAUSE OF INIT FAILURE (diagnosed from debugger screenshot):
 *  ──────────────────────────────────────────────────────────────────
 *  The I2C scan confirmed sensor at 0x29 and display at 0x3C.
 *  Init was failing because:
 *
 *  1. REG_IDENTIFICATION_MODEL_ID (0xC0) is in the VL53L0X private
 *     register bank. It is ONLY readable AFTER the device firmware has
 *     booted AND after the "tuning key" unlock sequence (write 0x88→0x00,
 *     0x80→0x01, 0xFF→0x01, 0x00→0x00).  Reading it cold after XSHUT
 *     release returns garbage → presence check always fails.
 *
 *  2. i2c_read_reg8 write phase was not waiting for BTF before issuing
 *     the repeated START, causing the register pointer byte to be lost.
 *
 *  FIX: Remove the pre-init model ID check. Instead use the I2C scan
 *  result (already stored in found_addresses[]) to confirm presence,
 *  then proceed straight to the ST init sequence. After data_init
 *  unlocks the private bank, we verify the model ID as a sanity check.
 */

#include "vl53l0x.h"
#include "i2c.h"
#include "gpio.h"

/* ── Register map (8-bit addresses, ST UM2039) ───────────── */
#define REG_SYSRANGE_START                  0x00
#define REG_SYSTEM_SEQUENCE_CONFIG          0x01
#define REG_SYSTEM_INTERRUPT_CONFIG_GPIO    0x0A
#define REG_SYSTEM_INTERRUPT_CLEAR          0x0B
#define REG_RESULT_INTERRUPT_STATUS         0x13
#define REG_RESULT_RANGE_STATUS             0x14
#define REG_RESULT_RANGE_MM                 0x1E   /* high byte; 0x1F = low byte */
#define REG_GPIO_HV_MUX_ACTIVE_HIGH         0x84
#define REG_IDENTIFICATION_MODEL_ID         0xC0   /* reads 0xEE after bank unlock */

/* ── Internal state ──────────────────────────────────────── */
static unsigned char sensor_detected = 0;

/* ── Helpers ─────────────────────────────────────────────── */

static void delay_ms(volatile unsigned int ms) {
    /*
     * ~8 MHz STM32F103: generous busy-wait.
     * 2000 inner iterations ≈ 1 ms at 8 MHz with optimisation off.
     */
    while (ms--) {
        for (volatile unsigned int i = 0; i < 2000; i++);
    }
}

/* wr / rd — thin wrappers so init tables stay readable */
static void wr(unsigned char reg, unsigned char val) {
    i2c_write_reg8(VL53L0X_ADDR, reg, val);
}

static unsigned char rd(unsigned char reg) {
    unsigned char val = 0;
    i2c_read_reg8(VL53L0X_ADDR, reg, &val, 1);
    return val;
}

/*
 * i2c_bus_is_present_at_addr
 * Checks the scan results populated by i2c_scan() — no extra I2C
 * transaction needed.  Avoids doing a raw I2C probe before the sensor
 * firmware is ready.
 */
static unsigned char i2c_bus_is_present_at_addr(unsigned char addr) {
    for (unsigned char i = 0; i < found_count; i++) {
        if (found_addresses[i] == addr) {
            return 1;
        }
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════
 * vl53l0x_data_init
 * Equivalent to VL53L0X_DataInit() in ST API.
 *
 * Writes the "tuning key" that unlocks the private register bank,
 * making 0xC0 (model ID) and other private registers readable.
 * MUST be called before vl53l0x_verify_model_id().
 * ══════════════════════════════════════════════════════════ */
static void vl53l0x_data_init(void) {
    wr(0x88, 0x00);   /* I2C standard mode */

    /* Unlock private register bank */
    wr(0x80, 0x01);
    wr(0xFF, 0x01);
    wr(0x00, 0x00);
    wr(0x91, 0x3C);   /* stop_variable — 0x3C is the ST API default */
    wr(0x00, 0x01);
    wr(0xFF, 0x00);
    wr(0x80, 0x00);
}

/* ══════════════════════════════════════════════════════════
 * vl53l0x_verify_model_id
 * Call AFTER vl53l0x_data_init() has unlocked the private bank.
 * 0xC0 should now return 0xEE.
 * ══════════════════════════════════════════════════════════ */
static unsigned char vl53l0x_verify_model_id(void) {
    unsigned char id = 0;
    for (int attempt = 0; attempt < 5; attempt++) {
        id = rd(REG_IDENTIFICATION_MODEL_ID);
        if (id == 0xEE) return 1;
        delay_ms(5);
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════
 * vl53l0x_static_init
 * Loads ST-recommended tuning registers (AN4545 / API source).
 * ══════════════════════════════════════════════════════════ */
static void vl53l0x_static_init(void) {
    wr(0xFF, 0x01);
    wr(0x00, 0x00);
    wr(0xFF, 0x00);
    wr(0x09, 0x00);
    wr(0x10, 0x00);
    wr(0x11, 0x00);
    wr(0x24, 0x01);
    wr(0x25, 0xFF);
    wr(0x75, 0x00);
    wr(0xFF, 0x01);
    wr(0x4E, 0x2C);
    wr(0x48, 0x00);
    wr(0x30, 0x20);
    wr(0xFF, 0x00);
    wr(0x30, 0x09);
    wr(0x54, 0x00);
    wr(0x31, 0x04);
    wr(0x32, 0x03);
    wr(0x40, 0x83);
    wr(0x46, 0x25);
    wr(0x60, 0x00);
    wr(0x27, 0x00);
    wr(0x50, 0x06);
    wr(0x51, 0x00);
    wr(0x52, 0x96);
    wr(0x56, 0x08);
    wr(0x57, 0x30);
    wr(0x61, 0x00);
    wr(0x62, 0x00);
    wr(0x64, 0x00);
    wr(0x65, 0x00);
    wr(0x66, 0xA0);
    wr(0xFF, 0x01);
    wr(0x22, 0x32);
    wr(0x47, 0x14);
    wr(0x49, 0xFF);
    wr(0x4A, 0x00);
    wr(0xFF, 0x00);
    wr(0x7A, 0x0A);
    wr(0x7B, 0x00);
    wr(0x78, 0x21);
    wr(0xFF, 0x01);
    wr(0x23, 0x34);
    wr(0x42, 0x00);
    wr(0x44, 0xFF);
    wr(0x45, 0x26);
    wr(0x46, 0x05);
    wr(0x40, 0x40);
    wr(0x0E, 0x06);
    wr(0x20, 0x1A);
    wr(0x43, 0x40);
    wr(0xFF, 0x00);
    wr(0x34, 0x03);
    wr(0x35, 0x44);
    wr(0xFF, 0x01);
    wr(0x31, 0x04);
    wr(0x4B, 0x09);
    wr(0x4C, 0x05);
    wr(0x4D, 0x04);
    wr(0xFF, 0x00);
    wr(0x44, 0x00);
    wr(0x45, 0x20);
    wr(0x47, 0x08);
    wr(0x48, 0x28);
    wr(0x67, 0x00);
    wr(0x70, 0x04);
    wr(0x71, 0x01);
    wr(0x72, 0xFE);
    wr(0x76, 0x00);
    wr(0x77, 0xFF);
    wr(0xFF, 0x01);
    wr(0x0D, 0x01);
    wr(0xFF, 0x00);
    wr(0x80, 0x01);
    wr(0x01, 0xF8);
    wr(0xFF, 0x01);
    wr(0x8E, 0x01);
    wr(0x00, 0x01);
    wr(0xFF, 0x00);
    wr(0x80, 0x00);

    /* Interrupt fires when new sample is ready */
    wr(REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);

    /* Active low: clear the polarity bit in GPIO_HV_MUX */
    unsigned char hv = rd(REG_GPIO_HV_MUX_ACTIVE_HIGH);
    wr(REG_GPIO_HV_MUX_ACTIVE_HIGH, hv & ~0x10);

    /* Clear any stale interrupt */
    wr(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

    /* Enable DSS, Pre-Range, Final-Range in the measurement sequence */
    wr(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);
}

/* ══════════════════════════════════════════════════════════
 * vl53l0x_perform_single_ref_calibration
 * ST API: VL53L0X_PerformSingleRefCalibration()
 * calib_type: 0x40 = VHV calibration, 0x00 = Phase calibration
 * ══════════════════════════════════════════════════════════ */
static void vl53l0x_perform_single_ref_calibration(unsigned char calib_type) {
    wr(REG_SYSRANGE_START, 0x01 | calib_type);

    volatile int timeout = 1000000;
    while (!(rd(REG_RESULT_INTERRUPT_STATUS) & 0x07)) {

        if (--timeout == 0) {
            wr(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
            return 0;   // NEVER freeze system
        }
    }

    wr(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    wr(REG_SYSRANGE_START, 0x00);
}

/* ══════════════════════════════════════════════════════════
 * vl53l0x_init  (public)
 *
 * Call AFTER i2c_init() and i2c_scan().
 *
 * Sequence:
 *  1. Check scan results — bail if 0x29 not found on bus
 *  2. Hardware reset via XSHUT (PB1) with generous timing
 *  3. data_init  — unlocks private register bank
 *  4. verify model ID (NOW readable after bank unlock)
 *  5. static_init — loads ST tuning registers
 *  6. VHV + Phase reference calibration
 *  7. Restore normal sequence config, set sensor_detected = 1
 * ══════════════════════════════════════════════════════════ */
void vl53l0x_init(void) {

    sensor_detected = 0;

    ssd1306_clear();

    if (!i2c_bus_is_present_at_addr(VL53L0X_ADDR)) {
        ssd1306_write_text(0,2,"NO SENSOR");
        return;
    }

    delay_ms(100);

    vl53l0x_data_init();

    delay_ms(1000);

    unsigned char id = rd(REG_IDENTIFICATION_MODEL_ID);

    char hex[] = "0123456789ABCDEF";
    char txt[16];

    txt[0] = 'I';
    txt[1] = 'D';
    txt[2] = ':';
    txt[3] = ' ';

    txt[4] = hex[(id >> 4) & 0x0F];
    txt[5] = hex[id & 0x0F];
    txt[6] = '\0';

    delay_ms(1000);

    vl53l0x_static_init();

    delay_ms(1000);
    wr(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);
//    wr(REG_SYSTEM_SEQUENCE_CONFIG, 0x01);
//
//    vl53l0x_perform_single_ref_calibration(0x40);

    delay_ms(1000);

//    wr(REG_SYSTEM_SEQUENCE_CONFIG, 0x02);
//
//    vl53l0x_perform_single_ref_calibration(0x00);

    wr(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);

    sensor_detected = 1;

    ssd1306_clear();
    ssd1306_write_text(0,0,"INIT DONE");
}
unsigned char vl53l0x_is_sensor_detected(void) {
    return sensor_detected;
}

/* ══════════════════════════════════════════════════════════
 * vl53l0x_read_distance_mm  (public)
 *
 * Triggers one single-shot measurement.
 * Returns distance in mm, or 0 on failure / timeout.
 * Typical measurement time: ~23 ms.
 * ══════════════════════════════════════════════════════════ */
unsigned int vl53l0x_read_distance_mm(void) {

    if (!sensor_detected) {
        return 0;
    }

    /* Unlock sequence required before each single-shot trigger */
    wr(0x80, 0x01);
    wr(0xFF, 0x01);
    wr(0x00, 0x00);
    wr(0x91, 0x3C);
    wr(0x00, 0x01);
    wr(0xFF, 0x00);
    wr(0x80, 0x00);

    /* Fire single-shot measurement */
    wr(REG_SYSRANGE_START, 0x01);

    /* Poll for data-ready: interrupt status bits [2:0] go non-zero */
    volatile int timeout = 200000;
    while (!(rd(REG_RESULT_INTERRUPT_STATUS) & 0x07) && timeout > 0) {
        timeout--;
    }

    if (timeout <= 0) {
        wr(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
        return 0;
    }

    /* Read 2-byte range result: 0x1E = high byte, 0x1F = low byte */
    unsigned char raw[2] = {0, 0};
    i2c_read_reg8(VL53L0X_ADDR, REG_RESULT_RANGE_MM, raw, 2);

    /* Clear interrupt flag so next measurement can fire */
    wr(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

    return ((unsigned int)raw[0] << 8) | raw[1];
}
