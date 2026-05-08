#ifndef SSD1306_H_
#define SSD1306_H_

#include "i2c.h"

// ── Display Properties ────────────────────────────────────
#define SSD1306_ADDR            0x3C    // confirmed by your scan
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64      // 128x64 per your spec
#define SSD1306_PAGES           8       // 64/8 = 8 pages

// ── Control Bytes (from datasheet Fig 8-7) ────────────────
// Co=0, D/C#=0 → command byte follows
// Co=0, D/C#=1 → data byte follows
#define SSD1306_CTRL_CMD        0x00
#define SSD1306_CTRL_DATA       0x40

// ── Fundamental Commands (Table 9-1) ─────────────────────
#define SSD1306_SET_CONTRAST        0x81    // double byte
#define SSD1306_DISPLAY_RAM         0xA4    // output follows RAM
#define SSD1306_DISPLAY_ALL_ON      0xA5    // ignore RAM
#define SSD1306_NORMAL_DISPLAY      0xA6    // 1=ON pixel
#define SSD1306_INVERSE_DISPLAY     0xA7    // 0=ON pixel
#define SSD1306_DISPLAY_OFF         0xAE
#define SSD1306_DISPLAY_ON          0xAF

// ── Addressing Commands (Table 9-1) ──────────────────────
#define SSD1306_SET_MEM_MODE        0x20    // double byte
#define SSD1306_MEM_HORIZONTAL      0x00    // horizontal mode
#define SSD1306_MEM_VERTICAL        0x01
#define SSD1306_MEM_PAGE            0x02    // default
#define SSD1306_SET_COL_ADDR        0x21    // triple byte
#define SSD1306_SET_PAGE_ADDR       0x22    // triple byte

// ── Hardware Config Commands (Table 9-1) ─────────────────
#define SSD1306_SET_START_LINE      0x40    // OR with line 0-63
#define SSD1306_SET_SEG_REMAP_0     0xA0    // col 0 → SEG0
#define SSD1306_SET_SEG_REMAP_1     0xA1    // col 127 → SEG0
#define SSD1306_SET_MUX_RATIO       0xA8    // double byte
#define SSD1306_SET_COM_SCAN_INC    0xC0    // COM0 to COM[N-1]
#define SSD1306_SET_COM_SCAN_DEC    0xC8    // COM[N-1] to COM0
#define SSD1306_SET_DISPLAY_OFFSET  0xD3    // double byte
#define SSD1306_SET_COM_PINS        0xDA    // double byte

// ── Timing Commands (Table 9-1) ──────────────────────────
#define SSD1306_SET_CLOCK_DIV       0xD5    // double byte
#define SSD1306_SET_PRECHARGE       0xD9    // double byte
#define SSD1306_SET_VCOMH           0xDB    // double byte
#define SSD1306_NOP                 0xE3

// ── Charge Pump (App Note page 3) ────────────────────────
#define SSD1306_CHARGE_PUMP         0x8D    // double byte
#define SSD1306_CHARGE_PUMP_ON      0x14    // enable
#define SSD1306_CHARGE_PUMP_OFF     0x10    // disable

// ── Scroll Commands ───────────────────────────────────────
#define SSD1306_DEACTIVATE_SCROLL   0x2E

// ── Function Declarations ─────────────────────────────────
void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_set_cursor(unsigned char col, unsigned char page);
void ssd1306_write_data(unsigned char data);
void ssd1306_fill(unsigned char pattern);
void ssd1306_write_text(unsigned char col, unsigned char page, const char *text);

#endif /* SSD1306_H_ */
