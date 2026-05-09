/*
 * main.c
 *
 * Fixed + Debug Version
 * Fixes applied:
 *   1. Crossover threshold: us_mm <= 90  →  us_mm <= 900
 *   2. delay_us calibration exposed via OLED (raw count visible)
 *   3. US_TIMEOUT reduced after calibration
 *   4. Smoothing filter seeded with first real value
 *   5. TOF forced-only mode for isolation testing
 *
 * DEBUG_MODE options (uncomment ONE at a time):
 *   DEBUG_US_ONLY   — shows raw count + cm + mm, ultrasonic only
 *   DEBUG_TOF_ONLY  — shows TOF mm reading only
 *   DEBUG_NORMAL    — full system with corrected logic
 */

#include "i2c.h"
#include "gpio.h"
#include "ssd1306.h"
#include "vl53l0x.h"

/* ── Tank / threshold config ────────────────────────────── */
#define TANK_HEIGHT_CM          18
#define TANK_HEIGHT_MM          180

/* FIXED: was 90 (9cm!) — crossover is 90cm = 900mm */
#define THRESHOLD_CROSSOVER_MM  900

/* Buzzer fires when water is this close to sensor (tank nearly full) */
#define THRESHOLD_BUZZER_MM     50
#define LEVEL_SAFE_MM       100
#define LEVEL_WARNING_MM    50

/* ── Timing ─────────────────────────────────────────────── */
/* US_TIMEOUT: max echo pulse count before we declare out-of-range.
 * After delay_us calibration (Step 2), adjust this value.
 * At 8MHz with inner loop = 8 iters:
 *   - If delay_us(1) ≈ 1µs → keep 38000  (38ms max echo)
 *   - If delay_us(1) ≈ 4µs → use  10000  (40ms max echo)
 * Start with 38000 and reduce if US hangs. */
#define US_TIMEOUT              38000

/* ── Global debug counter (exposed for OLED display) ─────── */
volatile unsigned int debug_raw_count = 0;

/* ══════════════════════════════════════════════════════════
 * Delays
 * ══════════════════════════════════════════════════════════ */
static void delay_ms(volatile unsigned int ms) {
    while (ms--) for (volatile int i = 0; i < 8000; i++);
}

static void delay_us(volatile unsigned int us) {
    while (us--) for (volatile int i = 0; i < 8; i++);
}

/* ══════════════════════════════════════════════════════════
 * uint_to_str  — no stdlib needed
 * ══════════════════════════════════════════════════════════ */
static void uint_to_str(unsigned int v, char *buf) {
    char tmp[6];
    int pos = 0;
    if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (v > 0 && pos < 5) { tmp[pos++] = '0' + (v % 10); v /= 10; }
    int i = 0;
    while (pos > 0) buf[i++] = tmp[--pos];
    buf[i] = '\0';
}

/* ══════════════════════════════════════════════════════════
 * get_us_distance
 *
 * Returns distance in cm.
 * Also writes raw count to debug_raw_count for OLED display.
 *
 * Formula: distance_cm = pulse_duration_us / 58
 * Each loop iteration = delay_us(1) + overhead.
 * If readings are N× wrong, the actual delay_us period is N µs,
 * so fix: cm = count * N / 58  (find N from debug_raw_count).
 * ══════════════════════════════════════════════════════════ */
static unsigned int get_us_distance(int sensor) {

    /* 10 µs trigger pulse */
    gpio_trigger_high(sensor);
    delay_us(10);
    gpio_trigger_low(sensor);

    /* Wait for echo to go HIGH */
    volatile unsigned int timeout = 30000;
    while (!gpio_read_echo(sensor)) {
        if (--timeout == 0) {
            debug_raw_count = 0;
            return 999;
        }
    }

    /* Measure HIGH pulse duration */
    volatile unsigned int count = 0;
    while (gpio_read_echo(sensor)) {
        delay_us(1);
        count++;
        if (count >= US_TIMEOUT) break;
    }

    /* Save raw count so we can display it for calibration */
    debug_raw_count = count;

    /* Distance formula: each count ≈ 1µs (verify with OLED debug) */
    unsigned int cm = count / 3;

    /* Clamp to tank range + small margin */
    if (cm > TANK_HEIGHT_CM + 5) return 999;
    return cm;
}


static void update_display_us_debug(unsigned int raw_count,
                                     unsigned int cm,
                                     unsigned int mm)
{
    char buf[16];

    /* Clear screen */
    ssd1306_clear();

    /* TITLE */
    ssd1306_write_text(0, 0, "WATER LEVEL: ");
    if (mm >= LEVEL_SAFE_MM) {

           ssd1306_write_text(20, 5, "SAFE");

       }
       else if (mm >= LEVEL_WARNING_MM) {

           ssd1306_write_text(20, 5, "WARNING");

       }
       else {

           ssd1306_write_text(20, 5, "CRITICAL !!!");

       }
    /* DISTANCE CM */
    ssd1306_write_text(0, 6, "DIST CM:");

    uint_to_str(cm, buf);

    ssd1306_write_text(20, 3, buf);
}


int main(void) {

    /* ── Hardware init ───────────────────────────────────── */
    gpio_init();

    i2c_init();
    i2c_scan();

    ssd1306_init();
    ssd1306_clear();

    /* ── Show I2C scan result ────────────────────────────── */
    ssd1306_write_text(30, 0, "I2C SCAN");

    char addr_buf[8];
    if (found_count == 0) {
        ssd1306_write_text(0, 6, "NO DEVICES!");
    } else {
        addr_buf[0] = '0' + found_count;
        addr_buf[1] = ' ';
        addr_buf[2] = 'D';
        addr_buf[3] = 'E';
        addr_buf[4] = 'V';
        addr_buf[5] = 'S';
        addr_buf[6] = '\0';
        ssd1306_write_text(20, 3, addr_buf);

        /* Print each found address */
        char hex[] = "0123456789ABCDEF";
        char txt[6];
        for (int i = 0; i < found_count && i < 4; i++) {
            txt[0] = '0'; txt[1] = 'X';
            txt[2] = hex[(found_addresses[i] >> 4) & 0x0F];
            txt[3] = hex[found_addresses[i] & 0x0F];
            txt[4] = ' '; txt[5] = '\0';
            ssd1306_write_text(0,5,"ADDRESSES : ");
            ssd1306_write_text(i * 36, 6, txt);
        }
    }

    delay_ms(2000);

    /* ── Init TOF sensor ─────────────────────────────────── */
    ssd1306_clear();
    ssd1306_write_text(0, 2, "INIT TOF...");

    ssd1306_clear();
    ssd1306_write_text(0, 2, "SYSTEM READY");

    ssd1306_write_text(0, 4, "MODE: HC_SR04");


    delay_ms(2000);
    ssd1306_clear();

    /* ════════════════════════════════════════════════════════
     * Main loop
     * ════════════════════════════════════════════════════════ */
    while (1) {

        unsigned int us_cm  = get_us_distance(1);
        unsigned int us_mm  = (us_cm == 999) ? 9999 : us_cm * 10;

        update_display_us_debug(debug_raw_count, us_cm, us_mm);

        /* Buzzer: simple threshold test at <30mm */
        if (us_mm < THRESHOLD_BUZZER_MM)
            gpio_buzzer_on();
        else
            gpio_buzzer_off();

        delay_ms(300);


    } /* while(1) */

    return 0;
}
