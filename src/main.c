#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "helpers/helpers.h"
#include "pico/stdlib.h"
#include "helpers/gc9a01.h"
#include "helpers/accelerometer_init.h"
#include "helpers/shake_detection.h"

// Constants
#define M_PI 3.14159265358979323846

// Screensize
#define SCREEN_W 240
#define SCREEN_H 240
#define RADIUS 80

// Dice size
#define CUBE_SIZE 60
#define POINT_SIZE 8
#define OFFSET 8
#define NUM_CUBES 6

//Fox Test
extern const uint8_t fox_bytes[];

// Bitmap dice data
extern const uint8_t dice1_bytes[480];
extern const uint8_t dice2_bytes[480];
extern const uint8_t dice3_bytes[480];
extern const uint8_t dice4_bytes[480];
extern const uint8_t dice5_bytes[480];
extern const uint8_t dice6_bytes[480];
extern const uint8_t dice_clear_bytes[480];

static const uint8_t *dice_arr[NUM_CUBES] = {dice1_bytes, dice2_bytes, dice3_bytes,
                                             dice4_bytes, dice5_bytes, dice6_bytes};

// Accelerometer Connecting
#define ACCELEROMETER_BUS_PORT i2c0
static int accelerometer_address = 0x68;
static int accelerometer_sda = 0;
static int accelerometer_scl = 1;

int main() {
    stdio_init_all(); // neccessary for USB serial monitoring
    gc9a01_init();

    // Initialize Accelerometer
    accelerometer_init(accelerometer_address, accelerometer_sda, accelerometer_scl);
    
    // Draw green screen
    uint8_t green[2] = {0x07, 0xE0};
    gc9a01_set_windows(0, 0, 239, 239);
    gpio_put(DC_PIN, 1);
    for (int i = 0; i < 240 * 240; i++) {
        spi_write_blocking(SPI_PORT, green, 2);
    }
    sleep_ms(2000);
    gc9a01_clear();

    // Cubes
    uint16_t cx = SCREEN_W / 2;
    uint16_t cy = SCREEN_H / 2;

    // Animation
    uint32_t seed = (uint32_t)to_us_since_boot(get_absolute_time());
    uint8_t anim[NUM_CUBES];
    for (int t = 0; t < 4; t++) {
        // Generate random dice for animation
        for (int i = 0; i < NUM_CUBES; i++) {
            seed = seed * 1664525 + 17;
            anim[i] = (seed % 6) + 1;  // 1-6
        }
        // Draw the random dice
        for (int i = 0; i < NUM_CUBES; i++) {
            float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
            float x = cx + RADIUS * cosf(theta);
            float y = cy + RADIUS * sinf(theta);

            uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
            uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);

            draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[anim[i] - 1], CUBE_SIZE,
                                   0xFFFF);
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
        rolled[i] = (seed % 6) + 1;  // 1-6
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

    sleep_ms(2000);
    //Draw Fox Test
    draw_packed_bitmap(SPI_PORT, 0, 0, fox_bytes, 240, 0xFFFF);

    while (true) {
        tight_loop_contents();
    }
}