#include "helpers.h"

#include <stdlib.h>

#include "gc9a01.h"
#include "pico/stdlib.h"

// SPI Interface Pins
#define PIN_MISO 12  // RX - Master In Slave Out
#define PIN_CS 13    // Chip Select
#define PIN_SCK 10   // Serial Clock
#define PIN_MOSI 11  // TX - Master Out Slave In

// Display Control Pins
#define PIN_DC 14   // Data/Command - Low = Command/High = Data
#define PIN_RST 15  // Reset

// void clear_bitmap(uint8_t *bm, int cube_size) {
//     for (int y = 0; y < cube_size; y++) {
//         for (int x = 0; x < cube_size; x++) {
//             bm[y * cube_size + x] = 0;
//         }
//     }
// }

void draw_packed_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data,
                        int cube_size, uint16_t color) {
    int w = cube_size;
    int h = cube_size;
    int bytes_per_row = (w + 7) / 8;
    // Split 16 bit color into two 8-bit channels
    uint8_t col_hi = (color >> 8) & 0xFF;
    uint8_t col_lo = color & 0xFF;

    // clear display black
    uint16_t black = 0x0000;  // 16-bit RGB565 black
    gc9a01_set_windows(x0 - 4, y0 - 4, x0 + 63, y0 + 63);
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    spi_write_blocking(spi_port, (uint8_t[]){0x2C}, 1);
    gpio_put(PIN_DC, 1);

    uint8_t red_hi = (black >> 8) & 0xFF;
    uint8_t red_lo = black & 0xFF;
    uint8_t red_row[128];
    for (int i = 0; i < 128; i += 2) {
        red_row[i] = red_hi;
        red_row[i + 1] = red_lo;
    }
    for (int i = 0; i < 73; i++) {
        spi_write_blocking(spi_port, red_row, 128);
    }
    gpio_put(PIN_CS, 1);

    // Display setup
    gc9a01_set_windows(x0, y0, x0 + w - 1, y0 + h - 1);
    gpio_put(PIN_CS, 0);                                 // select display
    gpio_put(PIN_DC, 0);                                 // command mode
    spi_write_blocking(spi_port, (uint8_t[]){0x2C}, 1);  // RAMWR
    gpio_put(PIN_DC, 1);                                 // data mode

    // Allocate memory for one row (2 * w for 16-bit color)
    uint8_t *rowbuf = malloc(2 * w);
    if (!rowbuf) return;

    // Iterate over single data-byte and convert each bit to two bytes of color data
    for (int ry = 0; ry < h; ry++) {
        const uint8_t *row_src = data + ry * bytes_per_row;
        int bx = 0;  // current x-pixel position
        for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
            uint8_t b = row_src[byte_i];
            for (int bit = 7; bit >= 0; bit--) {
                if (bx >= w) break;
                // Draw foreground if pixel is on
                if (b & (1 << bit)) {
                    rowbuf[bx * 2] = col_hi;
                    rowbuf[bx * 2 + 1] = col_lo;
                    // Draw black if pixel is off
                } else {
                    rowbuf[bx * 2] = 0x00;
                    rowbuf[bx * 2 + 1] = 0x00;
                }
                bx++;
            }
        }
        // Send row to display
        spi_write_blocking(spi_port, rowbuf, w * 2);
    }
    free(rowbuf);
    // Deselect display
    gpio_put(PIN_CS, 1);
}

void draw_artifacted_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data,
                            int cube_size, uint16_t color) {
    int w = cube_size;
    int h = cube_size;
    int bytes_per_row = (w + 7) / 8;
    // uint8_t col_hi = (color >> 8) & 0xFF;
    // uint8_t col_lo = color & 0xFF;

    // Display setup
    gc9a01_set_windows(x0, y0, x0 + w - 1, y0 + h - 1);
    gpio_put(PIN_CS, 0);                                 // select display
    gpio_put(PIN_DC, 0);                                 // command mode
    spi_write_blocking(spi_port, (uint8_t[]){0x2C}, 1);  // RAMWR
    gpio_put(PIN_DC, 1);                                 // data mode

    // Allocate memory for one row (2 * w for 16-bit color)
    uint8_t *rowbuf = malloc(2 * w);
    if (!rowbuf) return;

    // Iterate over single data-byte and convert each bit to two bytes of color data
    for (int ry = 0; ry < h; ry++) {
        const uint8_t *row_src = data + ry * bytes_per_row;
        int bx = 0;
        for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
            uint8_t b = row_src[byte_i];
            for (int bit = 7; bit >= 0; bit--) {
                if (bx >= w) break;
                rowbuf[bx * 2] = b;
                rowbuf[bx * 2 + 1] = b;
                bx++;
            }
        }
        // Send row to display
        spi_write_blocking(spi_port, rowbuf, w * 2);
    }
    free(rowbuf);
    // Deselect display
    gpio_put(PIN_CS, 1);
}

void draw_bitmap_with_background(spi_inst_t *spi_port, uint16_t x0, uint16_t y0,
                                 const uint8_t *data, int cube_size,
                                 uint16_t fg_color,  // foreground color
                                 uint16_t bg_color   // background color
) {
    int w = cube_size;
    int h = cube_size;
    int bytes_per_row = (w + 7) / 8;

    uint8_t fg_hi = fg_color & 0xFF;         // high
    uint8_t fg_lo = (fg_color >> 8) & 0xFF;  // low

    uint8_t bg_hi = bg_color & 0xFF;
    uint8_t bg_lo = (bg_color >> 8) & 0xFF;

    // Draw 64x64px red square background
    uint16_t red = 0xF800;  // 16-bit RGB565 red
    gc9a01_set_windows(x0 - 2, y0 - 2, x0 + 60, y0 + 61);
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    spi_write_blocking(spi_port, (uint8_t[]){0x2C}, 1);
    gpio_put(PIN_DC, 1);

    uint8_t red_hi = (red >> 8) & 0xFF;
    uint8_t red_lo = red & 0xFF;
    uint8_t red_row[128];
    for (int i = 0; i < 128; i += 2) {
        red_row[i] = red_hi;
        red_row[i + 1] = red_lo;
    }
    for (int i = 0; i < 72; i++) {
        spi_write_blocking(spi_port, red_row, 128);
    }
    gpio_put(PIN_CS, 1);

    // Reset window for dice rendering
    gc9a01_set_windows(x0, y0, x0 + w - 1, y0 + h - 1);
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    spi_write_blocking(spi_port, (uint8_t[]){0x2C}, 1);
    gpio_put(PIN_DC, 1);

    // Allocate memory for one row (2 * w for 16-bit color)
    uint8_t *rowbuf = malloc(2 * w);
    if (!rowbuf) return;

    // Iterate over single data-byte and convert each bit to two bytes of color data
    for (int ry = 0; ry < h; ry++) {
        const uint8_t *row_src = data + ry * bytes_per_row;
        int bx = 0;
        for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
            uint8_t b = row_src[byte_i];
            for (int bit = 7; bit >= 0; bit--) {
                if (bx >= w) break;
                // draw foreground if pixel is on
                if (b & (1 << bit)) {
                    rowbuf[bx * 2] = fg_hi;
                    rowbuf[bx * 2 + 1] = fg_lo;
                    // draw background if pixel is off
                } else {
                    rowbuf[bx * 2] = bg_hi;
                    rowbuf[bx * 2 + 1] = bg_lo;
                }
                bx++;
            }
        }
        // Send row to display
        spi_write_blocking(spi_port, rowbuf, w * 2);
    }

    free(rowbuf);
    // Deselect display
    gpio_put(PIN_CS, 1);
}
