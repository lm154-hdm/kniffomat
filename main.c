#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"


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
static void clear_bitmap(uint8_t bm[CUBE_SIZE][CUBE_SIZE]) {
    for (int y = 0; y < CUBE_SIZE; y++)
        for (int x = 0; x < CUBE_SIZE; x++)
            bm[y][x] = 0;
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

// Fill a horizontal span of `len` pixels at (x,y) with `color` in one SPI transfer.
void tft_fill_color_span(uint16_t x, uint16_t y, uint16_t len, uint16_t color) {
    if (len == 0) return;
    tft_set_window(x, y, x + len - 1, y);

    uint8_t buf[CUBE_SIZE * 2]; // max needed: CUBE_SIZE pixels * 2 bytes per pixel
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (int i = 0; i < len; i++) {
        buf[2 * i] = hi;
        buf[2 * i + 1] = lo;
    }

    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1); // Data
    spi_write_blocking(SPI_PORT, buf, len * 2);
    gpio_put(PIN_CS, 1);
}

// draw_packed_bitmap: for each row, write contiguous runs of '1' pixels as spans.
static void draw_packed_bitmap(uint16_t x0, uint16_t y0, const uint8_t *data, uint16_t color) {
    const int bytes_per_row = (CUBE_SIZE + 7) / 8;
    for (int y = 0; y < CUBE_SIZE; y++) {
        int x = 0;
        while (x < CUBE_SIZE) {
            // skip zeros
            while (x < CUBE_SIZE) {
                int idx = y * bytes_per_row + (x / 8);
                int bit = 7 - (x & 7);
                if ((data[idx] >> bit) & 1) break;
                x++;
            }
            if (x >= CUBE_SIZE) break;
            int start = x;
            // find run of ones
            while (x < CUBE_SIZE) {
                int idx = y * bytes_per_row + (x / 8);
                int bit = 7 - (x & 7);
                if (!((data[idx] >> bit) & 1)) break;
                x++;
            }
            int len = x - start;
            tft_fill_color_span(x0 + start, y0 + y, (uint16_t)len, color);
        }
    }
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

        // draw packed bitmap (use white color)
        draw_packed_bitmap(draw_x, draw_y, dice_arr[i % NUM_CUBES], 0xFFFF);
    }

    while (true) { tight_loop_contents(); }
}