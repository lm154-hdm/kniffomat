#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// Constants
#define M_PI 3.14159265358979323846

// Pins
#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_DC   14
#define PIN_RST  15

// Screensize
#define SCREEN_W 240
#define SCREEN_H 240
#define RADIUS 80

// Dice size
#define CUBE_SIZE 60
#define POINT_SIZE 8
#define OFFSET 8
#define NUM_CUBES 6

// Bitmap dice data
extern const uint8_t dice1_bytes[480];
extern const uint8_t dice2_bytes[480];
extern const uint8_t dice3_bytes[480];
extern const uint8_t dice4_bytes[480];
extern const uint8_t dice5_bytes[480];
extern const uint8_t dice6_bytes[480];

static const uint8_t *dice_arr[NUM_CUBES] = {
    dice1_bytes, dice2_bytes, dice3_bytes,
    dice4_bytes, dice5_bytes, dice6_bytes
};

spi_inst_t *SPI_PORT = spi1;

// helper: clear bitmap
static void clear_bitmap(uint8_t *bm, int cube_size) {
    for (int y = 0; y < cube_size; y++) {
        for (int x = 0; x < cube_size; x++) {
            bm[y * cube_size + x] = 0;
        }
    }
}

// Set window to draw in
void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];

    // Column Address Set
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0); // Command
    uint8_t cmd = 0x2A;  // Column Address
    spi_write_blocking(SPI_PORT, &cmd, 1);

    gpio_put(PIN_DC, 1); // Daten
    data[0] = x0 >> 8; data[1] = x0 & 0xFF; // High and low byte
    data[2] = x1 >> 8; data[3] = x1 & 0xFF;
    spi_write_blocking(SPI_PORT, data, 4);
    gpio_put(PIN_CS, 1);

    // Page Address Set
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    cmd = 0x2B; // Page Address
    spi_write_blocking(SPI_PORT, &cmd, 1);

    gpio_put(PIN_DC, 1); // Data -> new data in the same data block
    data[0] = y0 >> 8; data[1] = y0 & 0xFF;
    data[2] = y1 >> 8; data[3] = y1 & 0xFF;
    spi_write_blocking(SPI_PORT, data, 4);
    gpio_put(PIN_CS, 1);

    // Memory Write
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    cmd = 0x2C;
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
    // ready to draw
}

// draw_packed_bitmap: for each row, write pixels as spans.
static void draw_packed_bitmap(uint16_t x0, uint16_t y0, const uint8_t *data, int cube_size, uint16_t color) {
    int w = cube_size;
    int h = cube_size;
    int bytes_per_row = (w + 7) / 8;
    uint8_t col_hi = (color >> 8) & 0xFF;
    uint8_t col_lo = color & 0xFF;

    tft_set_window(x0, y0, x0 + w - 1, y0 + h - 1);

    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0); spi_write_blocking(SPI_PORT, (uint8_t[]){0x2C}, 1); // RAMWR
    gpio_put(PIN_DC, 1);

    uint8_t *rowbuf = malloc(2 * cube_size);
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
        spi_write_blocking(SPI_PORT, rowbuf, w * 2);
    }
    gpio_put(PIN_CS, 1);
    free(rowbuf);
}


int main() {
    stdio_init_all();
    spi_init(SPI_PORT, 10 * 1000 * 1000); // 10 MHz
    gpio_set_function(10, GPIO_FUNC_SPI); // SCK
    gpio_set_function(11, GPIO_FUNC_SPI); // MOSI

    // Steuerpins
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);

    // Reset & Display Init
    gpio_put(PIN_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_RST, 1);
    sleep_ms(100);

    // Sleep Out
    gpio_put(PIN_CS, 0); gpio_put(PIN_DC, 0);
    uint8_t cmd = 0x11; spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
    sleep_ms(120);

    // Display ON
    gpio_put(PIN_CS, 0); gpio_put(PIN_DC, 0);
    cmd = 0x29; spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);

    // Cubes
    uint16_t cx = SCREEN_W/2;
    uint16_t cy = SCREEN_H/2;

    // Draw dice in a concentric radial pattern
    for (int i = 0; i < NUM_CUBES; i++) {
        float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
        float x = cx + RADIUS * cosf(theta);
        float y = cy + RADIUS * sinf(theta);

        uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
        uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);

        // draw packed bitmap
        draw_packed_bitmap(draw_x, draw_y, dice_arr[i % NUM_CUBES], CUBE_SIZE, 0xFFFF);
    }

    while (true) { tight_loop_contents(); }
}