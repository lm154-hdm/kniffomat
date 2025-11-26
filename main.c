#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "helpers.h"

// Constants
#define M_PI 3.14159265358979323846

// Pins
#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_DC   14
#define PIN_RST  15

spi_inst_t *SPI_PORT = spi1;

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
extern const uint8_t dice_clear_bytes[480];

static const uint8_t *dice_arr[NUM_CUBES] = {
    dice1_bytes, dice2_bytes, dice3_bytes,
    dice4_bytes, dice5_bytes, dice6_bytes
};

// GC9A01 init (basic, common sequence). Adjust MADCTL/COLMOD if needed.
static void init_gc9a01() {
    // SPI setup: start slow for init
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    spi_set_baudrate(SPI_PORT, 2000000); // 2 MHz for init

    // Reset sequence
    gpio_put(PIN_RST, 0); sleep_ms(50);
    gpio_put(PIN_RST, 1); sleep_ms(150);

    lcd_cmd(0x01); sleep_ms(150); // SWRESET
    lcd_cmd(0x11); sleep_ms(120); // SLPOUT

    // Interface pixel format: 16-bit/pixel (RGB565)
    uint8_t colmod = 0x55;
    lcd_cmd(0x3A); lcd_data(&colmod, 1);

    // Memory access control: orientation and RGB/BGR order
    // 0x00 is a common default; if colors mirror swap BGR/RGB via MADCTL bit
    uint8_t madctl = 0x00;
    lcd_cmd(0x36); lcd_data(&madctl, 1);

    // Set column/row full range for 240x240
    uint8_t coladdr[] = { 0x00, 0x00, 0x00, 0xEF }; // 0..239
    uint8_t rowaddr[] = { 0x00, 0x00, 0x00, 0xEF }; // 0..239
    lcd_cmd(0x2A); lcd_data(coladdr, sizeof(coladdr));
    lcd_cmd(0x2B); lcd_data(rowaddr, sizeof(rowaddr));

    // Optional: inversion/gamma settings are omitted here; add from module datasheet if needed

    lcd_cmd(0x29); sleep_ms(20); // DISPON

    // Increase SPI speed for framebuffer transfers
    spi_set_baudrate(SPI_PORT, 16000000); // try 16 MHz (reduce if artifacts)
}

int main() {
    stdio_init_all();
    spi_init(SPI_PORT, 10 * 1000 * 1000); // 10 MHz
    gpio_set_function(10, GPIO_FUNC_SPI); // SCK
    gpio_set_function(11, GPIO_FUNC_SPI); // MOSI

    // Pins
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    init_gc9a01();

    // Cubes
    uint16_t cx = SCREEN_W/2;
    uint16_t cy = SCREEN_H/2;

    // Animation
    uint32_t seed = (uint32_t) to_us_since_boot(get_absolute_time());
    uint8_t anim[NUM_CUBES];
    for (int t = 0; t < 4; t++) {
        // Generate random dice for animation
        for (int i = 0; i < NUM_CUBES; i++) {
            seed = seed * 1664525 + 17;
            anim[i] = (seed % 6) + 1; // 1-6
        }
        // Draw the random dice
        for (int i = 0; i < NUM_CUBES; i++) {
            float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
            float x = cx + RADIUS * cosf(theta);
            float y = cy + RADIUS * sinf(theta);

            uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
            uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);

            draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[anim[i] - 1], CUBE_SIZE, 0xFFFF);
        }
        sleep_ms(1000);
        for (int i = 0; i < NUM_CUBES; i++) {
            float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
            float x = cx + RADIUS * cosf(theta);
            float y = cy + RADIUS * sinf(theta);

            uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
            uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);
            draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_clear_bytes, CUBE_SIZE, 0xFFFF);
        }
        sleep_ms(500);
    }

    // Roll 6 dice using seeded randomness
    // true randomness is not possible but can be noise detected from unused pins
    // adc_read()
    uint8_t rolled[NUM_CUBES];
    for (int i = 0; i < NUM_CUBES; i++) {
        seed = seed * 1664525 + 1013904223;
        rolled[i] = (seed % 6) + 1; // 1-6
    }
    printf("Rolled dice: ");
    for (int i = 0; i < NUM_CUBES; i++) {
        printf("%d ", rolled[i]);
    }

    // Draw the rolled dice in a concentric radial pattern
    for (int i = 0; i < NUM_CUBES; i++) {
        float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
        float x = cx + RADIUS * cosf(theta);
        float y = cy + RADIUS * sinf(theta);

        uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
        uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);

        draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1], CUBE_SIZE, 0xFFFF);
    }

    while (true) { tight_loop_contents(); }
}