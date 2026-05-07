#include "i2c.h"
#include "gpio.h"
#include "ssd1306.h"

int main(void) {

    // Initialize once at startup
    gpio_init();
    i2c_init();
    i2c_scan();      // run once for verification only
    ssd1306_init();  // run once at startup

    // Main loop — runs forever
    while(1) {

        // Test 1: Fill all pixels ON
        ssd1306_fill(0xFF);
        for (volatile int i = 0; i < 2000000; i++);

        // Test 2: Clear display
        ssd1306_clear();
        for (volatile int i = 0; i < 2000000; i++);

        // Test 3: Draw horizontal line on page 3
        ssd1306_set_cursor(0, 3);
        for (int i = 0; i < 128; i++) {
            ssd1306_write_data(0xFF);
        }
        for (volatile int i = 0; i < 2000000; i++);
    }

    return 0;
}
