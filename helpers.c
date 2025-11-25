#include "helpers.h"
#include <stdio.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"

#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_DC   14
#define PIN_RST  15

void clear_bitmap(uint8_t *bm, int cube_size) {
    for (int y = 0; y < cube_size; y++) {
        for (int x = 0; x < cube_size; x++) {
            bm[y * cube_size + x] = 0;
        }
    }
}

void tft_set_window(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];
    // Column Address Set
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0); // Command
    uint8_t cmd = 0x2A;  // Column Address
    spi_write_blocking(spi_port, &cmd, 1);
    gpio_put(PIN_DC, 1); // Data
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;
    spi_write_blocking(spi_port, data, 4);
    gpio_put(PIN_CS, 1);
    // Row Address Set
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    cmd = 0x2B;
    spi_write_blocking(spi_port, &cmd, 1);
    gpio_put(PIN_DC, 1);
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    spi_write_blocking(spi_port, data, 4);
    gpio_put(PIN_CS, 1);
}

void draw_packed_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data, 
                        int cube_size, uint16_t color) {
    int w = cube_size;
    int h = cube_size;
    int bytes_per_row = (w + 7) / 8;
    uint8_t col_hi = (color >> 8) & 0xFF;
    uint8_t col_lo = color & 0xFF;
    tft_set_window(spi_port, x0, y0, x0 + w - 1, y0 + h - 1);
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0); spi_write_blocking(spi_port, (uint8_t[]){0x2C}, 1); // RAMWR
    gpio_put(PIN_DC, 1);
    uint8_t *rowbuf = malloc(2 * w);
    if (!rowbuf) return;
    for (int ry = 0; ry < h; ry++) {
        const uint8_t *row_src = data + ry * bytes_per_row;
        int bx = 0;
        for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
            uint8_t b = row_src[byte_i];
            for (int bit = 7; bit >= 0; bit--) {
                if (bx >= w) break;
                if (b & (1 << bit)) {
                    rowbuf[bx * 2]     = col_hi;
                    rowbuf[bx * 2 + 1] = col_lo;
                } else {
                    rowbuf[bx * 2]     = 0x00;
                    rowbuf[bx * 2 + 1] = 0x00;
                }
                bx++;
            }
        }
        spi_write_blocking(spi_port, rowbuf, w * 2);
    }
    free(rowbuf);
    gpio_put(PIN_CS, 1);
}
