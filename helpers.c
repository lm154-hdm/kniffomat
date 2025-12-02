#include "helpers.h"
#include <stdlib.h>
#include "pico/stdlib.h"

#include "helpers.h"

static spi_inst_t* _spi;

#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_DC   14
#define PIN_RST  15

static inline void cmd(uint8_t c) {
    gpio_put(PIN_DC, 0);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(_spi, &c, 1);
    gpio_put(PIN_CS, 1);
}

static inline void data8(uint8_t d) {
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(_spi, &d, 1);
    gpio_put(PIN_CS, 1);
}

static inline void data16(uint16_t d) {
    uint8_t buf[2] = { d >> 8, d & 0xFF };
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(_spi, buf, 2);
    gpio_put(PIN_CS, 1);
}

// GC9A01 init (basic, common sequence). Adjust MADCTL/COLMOD if needed.
void gc9a01_init(spi_inst_t* spi) {
    _spi = spi;
    // SPI setup: start slow for init
    gpio_init(PIN_DC);  gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_init(PIN_CS);  gpio_set_dir(PIN_CS, GPIO_OUT);

    // Reset sequence
    gpio_put(PIN_CS, 1); 
    gpio_put(PIN_RST, 0); sleep_ms(50);
    gpio_put(PIN_RST, 1); sleep_ms(150);

    cmd(0xEF);
    cmd(0xEB); data8(0x14);

    cmd(0xFE);
    cmd(0xEF);

    cmd(0xEB); data8(0x14);
    cmd(0x84); data8(0x40);
    cmd(0x85); data8(0xFF);
    cmd(0x86); data8(0xFF);
    cmd(0x87); data8(0xFF);
    cmd(0x88); data8(0x0A);
    cmd(0x89); data8(0x21);
    cmd(0x8A); data8(0x00);
    cmd(0x8B); data8(0x80);
    cmd(0x8C); data8(0x01);
    cmd(0x8D); data8(0x01);
    cmd(0x8E); data8(0xFF);
    cmd(0x8F); data8(0xFF);

    cmd(0x3A); data8(0x05);  // 16-bit color
    cmd(0x36); data8(0x00);  // MADCTL

    cmd(0x21); // Inversion on
    cmd(0x11); // Sleep out
    sleep_ms(120);
    cmd(0x29); // Display ON
}

void draw_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= GC9A01_WIDTH || y < 0 || y >= GC9A01_HEIGHT) return;

    cmd(0x2A); data16(x); data16(x);
    cmd(0x2B); data16(y); data16(y);
    cmd(0x2C);

    data16(color);
}

void display_fill(uint16_t color) {
    cmd(0x2A);
    data16(0); data16(GC9A01_WIDTH-1);

    cmd(0x2B);
    data16(0); data16(GC9A01_HEIGHT-1);

    cmd(0x2C);

    gpio_put(13, 1); // dc = 1
    gpio_put(9, 0);

    uint8_t buf[2] = { color>>8, color & 0xFF };
    for (int i = 0; i < GC9A01_WIDTH * GC9A01_HEIGHT; i++)
        spi_write_blocking(_spi, buf, 2);

    gpio_put(9, 1);
}


// void draw_packed_bitmap(uint16_t x0, uint16_t y0, const uint8_t *data, 
//                         int cube_size, uint16_t color) {
//     int w = cube_size;
//     int h = cube_size;
//     int bytes_per_row = (w + 7) / 8;
//     // Use set_window + lcd_send16 for GC9A01
//     uint16_t *rowpix = malloc(sizeof(uint16_t) * w);
//     if (!rowpix) return;
//     // set full window once for the entire bitmap
//     set_window(_spi, x0, y0, x0 + w - 1, y0 + h - 1);
//     for (int ry = 0; ry < h; ry++) {
//         const uint8_t *row_src = data + ry * bytes_per_row;
//         int bx = 0;
//         for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
//             uint8_t b = row_src[byte_i];
//             for (int bit = 7; bit >= 0; bit--) {
//                 if (bx >= w) break;
//                 if (b & (1 << bit)) {
//                     rowpix[bx] = color;
//                 } else {
//                     rowpix[bx] = 0x0000;
//                 }
//                 bx++;
//             }
//         }
//         // push this row of pixels
//         lcd_send16(_spi,rowpix, w);
//     }
//     free(rowpix);
// }

// void draw_artifacted_bitmap(uint16_t x0, uint16_t y0, const uint8_t *data, 
//                         int cube_size, uint16_t color) {
//     int w = cube_size;
//     int h = cube_size;
//     int bytes_per_row = (w + 7) / 8;
//     // Use set_window + lcd_send16 for GC9A01
//     uint16_t *rowpix = malloc(sizeof(uint16_t) * w);
//     if (!rowpix) return;
//     // set full window once for the entire bitmap
//     set_window(_spi, x0, y0, x0 + w - 1, y0 + h - 1);
//     for (int ry = 0; ry < h; ry++) {
//         const uint8_t *row_src = data + ry * bytes_per_row;
//         int bx = 0;
//         for (int byte_i = 0; byte_i < bytes_per_row; byte_i++) {
//             uint8_t b = row_src[byte_i];
//             for (int bit = 7; bit >= 0; bit--) {
//                 if (bx >= w) break;
//                 // original artifact code sent the same byte for both color bytes
//                 // map that to a 16-bit pixel value: high=b, low=b
//                 rowpix[bx] = ((uint16_t)b << 8) | (uint16_t)b;
//                 bx++;
//             }
//         }
//         // push this row of pixels
//         lcd_send16(_spi, rowpix, w);
//     }
//     free(rowpix);
// }
