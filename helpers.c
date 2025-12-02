#include "helpers.h"
#include <stdlib.h>
#include "pico/stdlib.h"

#include "helpers.h"

static spi_inst_t* _spi;
static uint8_t _cs; 
static uint8_t _sck;
static uint8_t _mosi;
static uint8_t _dc;
static uint8_t _rst;

static inline void cmd(uint8_t cmd) {
    gpio_put(_dc, 0);
    gpio_put(_cs, 0);
    spi_write_blocking(_spi, &cmd, 1);
    gpio_put(_cs, 1);
}

static inline void data8(uint8_t d) {
    gpio_put(_dc, 1);
    gpio_put(_cs, 0);
    spi_write_blocking(_spi, &d, 1);
    gpio_put(_cs, 1);
}

static inline void data16(uint16_t d) {
    uint8_t buf[2] = { d >> 8, d & 0xFF };
    gpio_put(_dc, 1);
    gpio_put(_cs, 0);
    spi_write_blocking(_spi, buf, 2);
    gpio_put(_cs, 1);
}

void gc9a01_reset(void) {
    gpio_put(_rst, 1);
    sleep_ms(10);
    gpio_put(_rst, 0);
    sleep_ms(10);
    gpio_put(_rst, 1);
    sleep_ms(10);
}

void gc9a01_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address set
    cmd(0x2A);
    data8(0x00);
    data8(x0);
    data8(0x00);
    data8(x1 - 1);
    // Row address set
    cmd(0x2B);
    data8(0x00);
    data8(y0);
    data8(0x00);
    data8(y1 - 1);

    // Memory write
    cmd(0x2C);
}


// GC9A01 init (basic, common sequence). Adjust MADCTL/COLMOD if needed.
void gc9a01_init(spi_inst_t* spi,
                     uint8_t pin_cs, uint8_t pin_sck,
                     uint8_t pin_mosi, uint8_t pin_dc, uint8_t pin_rst) {
        _spi = spi;
        _cs   = pin_cs;
        _sck  = pin_sck;
        _mosi = pin_mosi;
        _dc   = pin_dc;
        _rst  = pin_rst;
    // SPI setup: start slow for init
    gpio_init(_dc);  gpio_set_dir(_dc, GPIO_OUT);
    gpio_init(_rst); gpio_set_dir(_rst, GPIO_OUT);
    gpio_init(_cs);  gpio_set_dir(_cs, GPIO_OUT);
    // Reset sequence
    gpio_put(_cs, 1); 
    gpio_put(_rst, 0); sleep_ms(50);
    gpio_put(_rst, 1); sleep_ms(150);
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

    cmd(0xB6);
    data8(0x00);
    data8(0x20);

    cmd(0x36);
    data8(0x08);

    cmd(0x3A);
    data8(0x05);   // 16-bit color

    cmd(0x90);
    data8(0x08);
    data8(0x08);
    data8(0x08);
    data8(0x08);

    cmd(0xBD);
    data8(0x06);

    cmd(0xBC);
    data8(0x00);

    cmd(0xFF);
    data8(0x60);
    data8(0x01);
    data8(0x04);

    cmd(0xC3);
    data8(0x13);

    cmd(0xC4);
    data8(0x13);

    cmd(0xC9);
    data8(0x22);

    cmd(0xBE);
    data8(0x11);

    cmd(0xE1);
    data8(0x10);
    data8(0x0E);

    cmd(0xDF);
    data8(0x21);
    data8(0x0C);
    data8(0x02);

    cmd(0xF0);
    data8(0x45);
    data8(0x09);
    data8(0x08);
    data8(0x08);
    data8(0x26);
    data8(0x2A);

    cmd(0xF1);
    data8(0x43);
    data8(0x70);
    data8(0x72);
    data8(0x36);
    data8(0x37);
    data8(0x6F);

    cmd(0xF2);
    data8(0x45);
    data8(0x09);
    data8(0x08);
    data8(0x08);
    data8(0x26);
    data8(0x2A);

    cmd(0xF3);
    data8(0x43);
    data8(0x70);
    data8(0x72);
    data8(0x36);
    data8(0x37);
    data8(0x6F);

    cmd(0xED);
    data8(0x1B);
    data8(0x0B);

    cmd(0xAE);
    data8(0x77);

    cmd(0xCD);
    data8(0x63);

    cmd(0x70);
    data8(0x07);
    data8(0x07);
    data8(0x04);
    data8(0x0E);
    data8(0x0F);
    data8(0x09);
    data8(0x07);
    data8(0x08);
    data8(0x03);

    cmd(0xE8);
    data8(0x34);

    cmd(0x62);
    data8(0x18);
    data8(0x0D);
    data8(0x71);
    data8(0xED);
    data8(0x70);
    data8(0x70);
    data8(0x18);
    data8(0x0F);
    data8(0x71);
    data8(0xEF);
    data8(0x70);
    data8(0x70);

    cmd(0x63);
    data8(0x18);
    data8(0x11);
    data8(0x71);
    data8(0xF1);
    data8(0x70);
    data8(0x70);
    data8(0x18);
    data8(0x13);
    data8(0x71);
    data8(0xF3);
    data8(0x70);
    data8(0x70);

    cmd(0x64);
    data8(0x28);
    data8(0x29);
    data8(0xF1);
    data8(0x01);
    data8(0xF1);
    data8(0x00);
    data8(0x07);

    cmd(0x66);
    data8(0x3C);
    data8(0x00);
    data8(0xCD);
    data8(0x67);
    data8(0x45);
    data8(0x45);
    data8(0x10);
    data8(0x00);
    data8(0x00);
    data8(0x00);

    cmd(0x67);
    data8(0x00);
    data8(0x3C);
    data8(0x00);
    data8(0x00);
    data8(0x00);
    data8(0x01);
    data8(0x54);
    data8(0x10);
    data8(0x32);
    data8(0x98);

    cmd(0x74);
    data8(0x10);
    data8(0x85);
    data8(0x80);
    data8(0x00);
    data8(0x00);
    data8(0x4E);
    data8(0x00);

    cmd(0x98);
    data8(0x3E);
    data8(0x07);

    cmd(0x35);  // TE on
    cmd(0x21);  // Display inversion on

    cmd(0x11);  // Sleep out
    sleep_ms(120);

    cmd(0x29);  // Display on
    sleep_ms(20);

    // ---- End of init sequence ---- //
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
