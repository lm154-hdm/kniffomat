#include "helpers.h"
#include <stdlib.h>
#include "pico/stdlib.h"

#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_DC   14
#define PIN_RST  15

// helper: send command/data to GC9A01
void lcd_cmd(spi_inst_t *spi_port, uint8_t c) {
    gpio_put(PIN_DC, 0);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi_port, &c, 1);
    gpio_put(PIN_CS, 1);
}
void lcd_data(spi_inst_t *spi_port, const uint8_t *d, size_t len) {
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi_port, d, len);
    gpio_put(PIN_CS, 1);
}

void lcd_send16(spi_inst_t *spi_port,const uint16_t *pixels, size_t count) {
    // pixels are RGB565 (big-endian: high then low byte)
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    // send as bytes, convert on the fly
    for (size_t i = 0; i < count; ++i) {
        uint8_t hi = (pixels[i] >> 8) & 0xFF;
        uint8_t lo = pixels[i] & 0xFF;
        uint8_t b[2] = { hi, lo };
        spi_write_blocking(spi_port, b, 2);
    }
    gpio_put(PIN_CS, 1);
}

void set_window(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t buf[4];
    // Column address
    buf[0] = x0 >> 8; buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8; buf[3] = x1 & 0xFF;
    lcd_cmd(spi_port, 0x2A); lcd_data(spi_port, buf, 4);
    // Row address
    buf[0] = y0 >> 8; buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8; buf[3] = y1 & 0xFF;
    lcd_cmd(spi_port, 0x2B); lcd_data(spi_port, buf, 4);
    // Memory write next
    lcd_cmd(spi_port, 0x2C);
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
    // Use set_window + lcd_send16 for GC9A01
    uint16_t *rowpix = malloc(sizeof(uint16_t) * w);
    if (!rowpix) return;
    // set full window once for the entire bitmap
    set_window(spi_port, x0, y0, x0 + w - 1, y0 + h - 1);
    for (int ry = 0; ry < h; ry++) {
        const uint8_t *row_src = data + ry * bytes_per_row;
        int bx = 0;
        for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
            uint8_t b = row_src[byte_i];
            for (int bit = 7; bit >= 0; bit--) {
                if (bx >= w) break;
                if (b & (1 << bit)) {
                    rowpix[bx] = color;
                } else {
                    rowpix[bx] = 0x0000;
                }
                bx++;
            }
        }
        // push this row of pixels
        lcd_send16(spi_port,rowpix, w);
    }
    free(rowpix);
}

void draw_artifacted_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data, 
                        int cube_size, uint16_t color) {
    int w = cube_size;
    int h = cube_size;
    int bytes_per_row = (w + 7) / 8;
    // Use set_window + lcd_send16 for GC9A01
    uint16_t *rowpix = malloc(sizeof(uint16_t) * w);
    if (!rowpix) return;
    // set full window once for the entire bitmap
    set_window(spi_port, x0, y0, x0 + w - 1, y0 + h - 1);
    for (int ry = 0; ry < h; ry++) {
        const uint8_t *row_src = data + ry * bytes_per_row;
        int bx = 0;
        for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
            uint8_t b = row_src[byte_i];
            for (int bit = 7; bit >= 0; bit--) {
                if (bx >= w) break;
                // original artifact code sent the same byte for both color bytes
                // map that to a 16-bit pixel value: high=b, low=b
                rowpix[bx] = ((uint16_t)b << 8) | (uint16_t)b;
                bx++;
            }
        }
        // push this row of pixels
        lcd_send16(spi_port, rowpix, w);
    }
    free(rowpix);
}
