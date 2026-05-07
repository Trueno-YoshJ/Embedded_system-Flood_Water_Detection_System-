#include "ssd1306.h"

// ── Frame buffer: 128 x 64 = 1024 bytes (8 pages x 128) ──
static unsigned char ssd1306_buffer[SSD1306_WIDTH * SSD1306_PAGES];

// ── Send single command byte ──────────────────────────────
static void ssd1306_cmd(unsigned char cmd) {
    i2c_write(SSD1306_ADDR, SSD1306_CTRL_CMD, cmd);
}

// ── Send single data byte ────────────────────────────────
static void ssd1306_data(unsigned char data) {
    i2c_write(SSD1306_ADDR, SSD1306_CTRL_DATA, data);
}

void ssd1306_init(void) {

    // Power-on stabilization delay
    // Datasheet section 8.9: wait after VDD stable
    for (volatile int i = 0; i < 500000; i++);

    // ── 1. Display OFF ────────────────────────────────────
    // Datasheet AEh: Display OFF (sleep mode) before config
    ssd1306_cmd(SSD1306_DISPLAY_OFF);              // AEh

    // ── 2. Set oscillator frequency ───────────────────────
    // App note: D5h, 80h
    // D5h A[3:0]=0000 → divide ratio=1
    // D5h A[7:4]=1000 → default oscillator freq
    ssd1306_cmd(SSD1306_SET_CLOCK_DIV);            // D5h
    ssd1306_cmd(0x80);                             // default

    // ── 3. Set MUX ratio ──────────────────────────────────
    // App note: A8h, 3Fh
    // 128x64 → N=63 → 64MUX
    // datasheet: N=A[5:0], MUX=N+1
    ssd1306_cmd(SSD1306_SET_MUX_RATIO);            // A8h
    ssd1306_cmd(0x3F);                             // 64MUX for 128x64

    // ── 4. Set display offset ─────────────────────────────
    // Vertical shift = 0 (no shift)
    ssd1306_cmd(SSD1306_SET_DISPLAY_OFFSET);       // D3h
    ssd1306_cmd(0x00);                             // no offset

    // ── 5. Set display start line ─────────────────────────
    // Start from RAM row 0
    ssd1306_cmd(SSD1306_SET_START_LINE | 0x00);    // 40h

    // ── 6. Enable charge pump ─────────────────────────────
    // App note section 2.1:
    // Your module uses VDD (3.3V) without external VCC
    // so charge pump MUST be enabled
    // Sequence: 8Dh → 14h → AFh (display on last)
    ssd1306_cmd(SSD1306_CHARGE_PUMP);              // 8Dh
    ssd1306_cmd(SSD1306_CHARGE_PUMP_ON);           // 14h enable

    // ── 7. Set memory addressing mode ─────────────────────
    // Horizontal mode: column auto-increments then page
    // Best for writing full screen buffer
    ssd1306_cmd(SSD1306_SET_MEM_MODE);             // 20h
    ssd1306_cmd(SSD1306_MEM_HORIZONTAL);           // 00h

    // ── 8. Set segment remap ──────────────────────────────
    // A1h: col 127 mapped to SEG0 (flip horizontally)
    // matches typical OLED module orientation
    ssd1306_cmd(SSD1306_SET_SEG_REMAP_1);          // A1h

    // ── 9. Set COM output scan direction ─────────────────
    // C8h: scan from COM63 to COM0 (flip vertically)
    // matches typical OLED module orientation
    ssd1306_cmd(SSD1306_SET_COM_SCAN_DEC);         // C8h

    // ── 10. Set COM pins hardware config ─────────────────
    // App note Fig 2: DAh, 02h for 128x64
    // From datasheet Table 10-3:
    // A[4]=0 → sequential COM pin config
    // A[5]=0 → disable COM left/right remap
    // byte = 0b00000010 = 0x02
    ssd1306_cmd(SSD1306_SET_COM_PINS);             // DAh
    ssd1306_cmd(0x02);                             // sequential, no remap

    // ── 11. Set contrast ──────────────────────────────────
    // App note: 81h, 7Fh (mid brightness)
    // Range 00h-FFh, higher = brighter
    ssd1306_cmd(SSD1306_SET_CONTRAST);             // 81h
    ssd1306_cmd(0xCF);                             // bright

    // ── 12. Set precharge period ──────────────────────────
    // App note typical: D9h, F1h
    // A[3:0]=1 phase1, A[7:4]=15 phase2
    ssd1306_cmd(SSD1306_SET_PRECHARGE);            // D9h
    ssd1306_cmd(0xF1);                             // phase1=1, phase2=15

    // ── 13. Set VCOMH deselect level ─────────────────────
    // Datasheet Table DBh:
    // 40h → ~0.77 x VCC (reset value)
    ssd1306_cmd(SSD1306_SET_VCOMH);               // DBh
    ssd1306_cmd(0x40);                             // ~0.77 x VCC

    // ── 14. Resume from RAM display ──────────────────────
    // A4h: output follows RAM content (not all ON)
    ssd1306_cmd(SSD1306_DISPLAY_RAM);              // A4h

    // ── 15. Normal display (not inverted) ────────────────
    // A6h: RAM 1 = pixel ON, RAM 0 = pixel OFF
    ssd1306_cmd(SSD1306_NORMAL_DISPLAY);           // A6h

    // ── 16. Deactivate any scrolling ─────────────────────
    ssd1306_cmd(SSD1306_DEACTIVATE_SCROLL);        // 2Eh

    // ── 17. Clear display buffer and write to RAM ─────────
    ssd1306_clear();

    // ── 18. Display ON ────────────────────────────────────
    // Datasheet section 8.9: send AFh after VCC stable
    // tAF = 100ms for SEG/COM to turn ON
    ssd1306_cmd(SSD1306_DISPLAY_ON);               // AFh

    // Wait 100ms for display to turn on (datasheet tAF)
    for (volatile int i = 0; i < 800000; i++);
}

void ssd1306_clear(void) {

    // Clear local buffer
    for (int i = 0; i < SSD1306_WIDTH * SSD1306_PAGES; i++) {
        ssd1306_buffer[i] = 0x00;
    }

    // Set full screen address window
    // Column 0 to 127
    ssd1306_cmd(SSD1306_SET_COL_ADDR);             // 21h
    ssd1306_cmd(0);                                // start col = 0
    ssd1306_cmd(127);                              // end col = 127

    // Page 0 to 7 (128x64 = 8 pages)
    ssd1306_cmd(SSD1306_SET_PAGE_ADDR);            // 22h
    ssd1306_cmd(0);                                // start page = 0
    ssd1306_cmd(7);                                // end page = 7

    // Write 1024 zero bytes to clear display RAM
    for (int i = 0; i < SSD1306_WIDTH * SSD1306_PAGES; i++) {
        ssd1306_data(0x00);
    }
}

void ssd1306_fill(unsigned char pattern) {

    // Set full screen address window
    ssd1306_cmd(SSD1306_SET_COL_ADDR);
    ssd1306_cmd(0);
    ssd1306_cmd(127);

    ssd1306_cmd(SSD1306_SET_PAGE_ADDR);
    ssd1306_cmd(0);
    ssd1306_cmd(7);

    // Fill entire display with pattern
    for (int i = 0; i < SSD1306_WIDTH * SSD1306_PAGES; i++) {
        ssd1306_data(pattern);
    }
}

void ssd1306_set_cursor(unsigned char col, unsigned char page) {

    // Set column window from col to end of screen
    ssd1306_cmd(SSD1306_SET_COL_ADDR);
    ssd1306_cmd(col);
    ssd1306_cmd(127);

    // Set page window from page to end of screen
    ssd1306_cmd(SSD1306_SET_PAGE_ADDR);
    ssd1306_cmd(page);
    ssd1306_cmd(7);
}

void ssd1306_write_data(unsigned char data) {
    ssd1306_data(data);
}
