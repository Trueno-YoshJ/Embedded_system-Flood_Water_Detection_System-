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

/* ── Uncomment ONE mode at a time ───────────────────────── */
#define DEBUG_US_ONLY
//#define DEBUG_TOF_ONLY
//#define DEBUG_NORMAL
/* ────────────────────────────────────────────────────────── */

/* ── Tank / threshold config ────────────────────────────── */
#define TANK_HEIGHT_CM          18
#define TANK_HEIGHT_MM          180

/* FIXED: was 90 (9cm!) — crossover is 90cm = 900mm */
#define THRESHOLD_CROSSOVER_MM  900

/* Buzzer fires when water is this close to sensor (tank nearly full) */
#define THRESHOLD_BUZZER_MM     50

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
 * Smoothing filter — FIX: seeds with first real reading
 * ══════════════════════════════════════════════════════════ */
static unsigned int smooth_distance(unsigned int new_val) {
    static unsigned int last        = 0;
    static unsigned char initialized = 0;

    if (!initialized) {
        last        = new_val;
        initialized = 1;
        return last;
    }
    last = (last * 3 + new_val) / 4;
    return last;
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

/* ══════════════════════════════════════════════════════════
 * update_display — shows all info on OLED
 * ══════════════════════════════════════════════════════════ */
static void update_display_normal(unsigned int dist_mm,
                                   unsigned int us_raw_mm,
                                   unsigned int tof_mm,
                                   const char  *active_sensor)
{
    char buf[16];

    /* Row 0: title */
    ssd1306_write_text(0, 0, "WATER LEVEL MON ");

    /* Row 2: final smoothed distance */
    ssd1306_write_text(0, 2, "DIST:       mm  ");
    uint_to_str(dist_mm, buf);
    ssd1306_write_text(36, 2, "      ");   /* clear old */
    ssd1306_write_text(36, 2, buf);

    /* Row 4: active sensor label */
    ssd1306_write_text(0, 4, "SEN:            ");
    ssd1306_write_text(36, 4, active_sensor);

    /* Row 6: raw US mm for reference */
    ssd1306_write_text(0, 6, "US:         mm  ");
    uint_to_str(us_raw_mm, buf);
    ssd1306_write_text(24, 6, "      ");
    ssd1306_write_text(24, 6, buf);
}

/* ── Debug display for ultrasonic calibration ────────────── */
/* ── Debug display for ultrasonic calibration ────────────── */
/* ── Debug display for ultrasonic calibration ────────────── */
static void update_display_us_debug(unsigned int raw_count,
                                     unsigned int cm,
                                     unsigned int mm)
{
    char buf[16];

    /* Clear screen */
    ssd1306_clear();

    /* TITLE */
    ssd1306_write_text(0, 0, "US DEBUG MODE");

//    /* RAW COUNT */
//    ssd1306_write_text(0, 2, "RAW COUNT:");
//
//    uint_to_str(raw_count, buf);
//
//    ssd1306_write_text(0, 3, buf);

    /* DISTANCE CM */
    ssd1306_write_text(0, 5, "DIST CM:");

    uint_to_str(cm, buf);

    ssd1306_write_text(0, 3, buf);
}

/* ── Debug display for TOF calibration ───────────────────── */
static void update_display_tof_debug(unsigned int tof_mm)
{
    char buf[16];

    ssd1306_write_text(0, 0, "TOF DEBUG MODE  ");

    ssd1306_write_text(0, 3, "DIST:           ");
    uint_to_str(tof_mm, buf);
    ssd1306_write_text(36, 3, "      ");
    ssd1306_write_text(36, 3, buf);
    ssd1306_write_text(90, 3, "mm");

    /* Basic sanity indicator */
    if (tof_mm == 0) {
        ssd1306_write_text(0, 5, "SENSOR ERR      ");
    } else if (tof_mm > 2000) {
        ssd1306_write_text(0, 5, "OUT OF RANGE    ");
    } else {
        ssd1306_write_text(0, 5, "OK              ");
    }
}

/* ══════════════════════════════════════════════════════════
 * main
 * ══════════════════════════════════════════════════════════ */
int main(void) {

    /* ── Hardware init ───────────────────────────────────── */
    gpio_init();

    gpio_vl53l0x_xshut_low();
    delay_ms(50);
    gpio_vl53l0x_xshut_high();
    delay_ms(200);

    i2c_init();
    i2c_scan();

    ssd1306_init();
    ssd1306_clear();

    /* ── Show I2C scan result ────────────────────────────── */
    ssd1306_write_text(0, 0, "I2C SCAN:");

    char addr_buf[8];
    if (found_count == 0) {
        ssd1306_write_text(0, 2, "NO DEVICES!");
    } else {
        addr_buf[0] = '0' + found_count;
        addr_buf[1] = ' ';
        addr_buf[2] = 'D';
        addr_buf[3] = 'E';
        addr_buf[4] = 'V';
        addr_buf[5] = 'S';
        addr_buf[6] = '\0';
        ssd1306_write_text(0, 2, addr_buf);

        /* Print each found address */
        char hex[] = "0123456789ABCDEF";
        char txt[6];
        for (int i = 0; i < found_count && i < 4; i++) {
            txt[0] = '0'; txt[1] = 'x';
            txt[2] = hex[(found_addresses[i] >> 4) & 0x0F];
            txt[3] = hex[found_addresses[i] & 0x0F];
            txt[4] = ' '; txt[5] = '\0';
            ssd1306_write_text(i * 36, 4, txt);
        }
    }

    delay_ms(2000);

    /* ── Init TOF sensor ─────────────────────────────────── */
    ssd1306_clear();
    ssd1306_write_text(0, 2, "INIT TOF...");

#if !defined(DEBUG_US_ONLY)
    vl53l0x_init();
#endif

    ssd1306_clear();
    ssd1306_write_text(0, 2, "SYSTEM READY");

#if defined(DEBUG_US_ONLY)
    ssd1306_write_text(0, 4, "MODE: US ONLY");
#elif defined(DEBUG_TOF_ONLY)
    ssd1306_write_text(0, 4, "MODE: TOF ONLY");
#else
    ssd1306_write_text(0, 4, "MODE: NORMAL");
#endif

    delay_ms(2000);
    ssd1306_clear();

    /* ════════════════════════════════════════════════════════
     * Main loop
     * ════════════════════════════════════════════════════════ */
    while (1) {

/* ── MODE 1: Ultrasonic isolation + calibration ───────────── */
#if defined(DEBUG_US_ONLY)

        unsigned int us_cm  = get_us_distance(1);
        unsigned int us_mm  = (us_cm == 999) ? 9999 : us_cm * 10;

        update_display_us_debug(debug_raw_count, us_cm, us_mm);

        /* Buzzer: simple threshold test at <30mm */
        if (us_mm < THRESHOLD_BUZZER_MM)
            gpio_buzzer_on();
        else
            gpio_buzzer_off();

        delay_ms(300);

/* ── MODE 2: TOF isolation ────────────────────────────────── */
#elif defined(DEBUG_TOF_ONLY)

        unsigned int tof_mm = vl53l0x_read_distance_mm();

        /* Clamp garbage readings */
        if (tof_mm > 2000) tof_mm = 9999;

        update_display_tof_debug(tof_mm);

        if (tof_mm < THRESHOLD_BUZZER_MM && tof_mm != 9999)
            gpio_buzzer_on();
        else
            gpio_buzzer_off();

        delay_ms(200);

/* ── MODE 3: Full system — all fixes applied ──────────────── */
#else /* DEBUG_NORMAL */

        unsigned int us_cm     = 0;
        unsigned int us_mm     = 0;
        unsigned int tof_mm    = 0;
        unsigned int final_mm  = 0;
        const char  *active    = "US ";

        /* 1. Read ultrasonic → mm */
        us_cm = get_us_distance(1);
        us_mm = (us_cm == 999) ? 9999 : us_cm * 10;

        /* 2. Sensor selection
         *    FIX: threshold is 900mm (90cm), was incorrectly 90mm (9cm) */
        if (us_mm <= THRESHOLD_CROSSOVER_MM && us_mm != 9999) {

            /* Close range → TOF */
            tof_mm = vl53l0x_read_distance_mm();
            if (tof_mm > 2000) tof_mm = 0;

            final_mm = tof_mm;
            active   = "TOF";

        } else {

            /* Long range → Ultrasonic */
            final_mm = us_mm;
            active   = "US ";
        }

        /* 3. Smooth — FIX: seeded with first real reading */
        final_mm = smooth_distance(final_mm);

        /* 4. Display */
        update_display_normal(final_mm, us_mm, tof_mm, active);

        /* 5. Buzzer: fires when dist < 30mm (tank nearly full) */
        if (final_mm < THRESHOLD_BUZZER_MM)
            gpio_buzzer_on();
        else
            gpio_buzzer_off();

        /* 6. Update rate */
        delay_ms(200);

#endif /* mode select */

    } /* while(1) */

    return 0;
}
