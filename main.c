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
#define PIN_CLK  10
#define PIN_DIN  11
#define PIN_CS   13
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

int main() {
    stdio_init_all();
    spi_init(SPI_PORT, 2 * 1000 * 1000); // 2 MHz
    gpio_set_function(PIN_CLK, GPIO_FUNC_SPI); // SCK
    gpio_set_function(PIN_DIN, GPIO_FUNC_SPI); // Data

    gc9a01_init(SPI_PORT, PIN_CS, PIN_CLK, PIN_DIN, PIN_DC, PIN_RST);
    
    for (int i = 0; i < 100; ++i) {
        display_fill(0xFFFF);
        sleep_ms(1000);
        display_fill(0x0000);
        sleep_ms(1000);
    }

    // // Cubes
    // uint16_t cx = SCREEN_W/2;
    // uint16_t cy = SCREEN_H/2;

    // // Animation
    // uint32_t seed = (uint32_t) to_us_since_boot(get_absolute_time());
    // uint8_t anim[NUM_CUBES];
    // for (int t = 0; t < 4; t++) {
    //     // Generate random dice for animation
    //     for (int i = 0; i < NUM_CUBES; i++) {
    //         seed = seed * 1664525 + 17;
    //         anim[i] = (seed % 6) + 1; // 1-6
    //     }
    //     // Draw the random dice
    //     for (int i = 0; i < NUM_CUBES; i++) {
    //         float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
    //         float x = cx + RADIUS * cosf(theta);
    //         float y = cy + RADIUS * sinf(theta);

    //         uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
    //         uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);

    //         draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[anim[i] - 1], CUBE_SIZE, 0xFFFF);
    //     }
    //     sleep_ms(1000);
    //     for (int i = 0; i < NUM_CUBES; i++) {
    //         float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
    //         float x = cx + RADIUS * cosf(theta);
    //         float y = cy + RADIUS * sinf(theta);

    //         uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
    //         uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);
    //         draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_clear_bytes, CUBE_SIZE, 0xFFFF);
    //     }
    //     sleep_ms(500);
    // }

    // // Roll 6 dice using seeded randomness
    // // true randomness is not possible but can be noise detected from unused pins
    // // adc_read()
    // uint8_t rolled[NUM_CUBES];
    // for (int i = 0; i < NUM_CUBES; i++) {
    //     seed = seed * 1664525 + 1013904223;
    //     rolled[i] = (seed % 6) + 1; // 1-6
    // }
    // printf("Rolled dice: ");
    // for (int i = 0; i < NUM_CUBES; i++) {
    //     printf("%d ", rolled[i]);
    // }

    // // Draw the rolled dice in a concentric radial pattern
    // for (int i = 0; i < NUM_CUBES; i++) {
    //     float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
    //     float x = cx + RADIUS * cosf(theta);
    //     float y = cy + RADIUS * sinf(theta);

    //     uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
    //     uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);

    //     draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1], CUBE_SIZE, 0xFFFF);
    // }

    while (true) { tight_loop_contents(); }
}