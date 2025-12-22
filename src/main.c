#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "helpers/accelerometer_init.h"
#include "helpers/gc9a01.h"
#include "helpers/helpers.h"
#include "helpers/shake_detection.h"
#include "pico/stdlib.h"

// Constants
#define M_PI 3.14159265358979323846

#define NUM_CUBES 6

// Lock Buttons
#define PIN_BUTTON 16
#define PIN_BUTTON_1 7
#define PIN_BUTTON_2 6
#define PIN_BUTTON_3 28
#define PIN_BUTTON_4 27
#define PIN_BUTTON_5 26
#define PIN_BUTTON_6 22
uint dice_buttons[NUM_CUBES] = {PIN_BUTTON_1, PIN_BUTTON_2, PIN_BUTTON_3,
                                PIN_BUTTON_4, PIN_BUTTON_5, PIN_BUTTON_6};

// Screensize
#define SCREEN_W 240
#define SCREEN_H 240
#define RADIUS 80

// Dice size
#define CUBE_SIZE 60
#define POINT_SIZE 8
#define OFFSET 8

// Dice position
typedef struct {
    uint16_t x;
    uint16_t y;
} CubePosition;
CubePosition cube_positions[NUM_CUBES];

// Bitmap dice data
extern const uint8_t dice1_bytes[480];
extern const uint8_t dice2_bytes[480];
extern const uint8_t dice3_bytes[480];
extern const uint8_t dice4_bytes[480];
extern const uint8_t dice5_bytes[480];
extern const uint8_t dice6_bytes[480];
extern const uint8_t dice_clear_bytes[480];

// Locking
bool first_roll = false;
bool lock_dice_1 = false;
bool lock_dice_2 = false;
bool lock_dice_3 = false;
bool lock_dice_4 = false;
bool lock_dice_5 = false;
bool lock_dice_6 = false;
bool lock_dice[NUM_CUBES] = {false, false, false, false, false, false};
#define LONG_PRESS_TIME 600  // ms
uint64_t press_start_time[NUM_CUBES] = {0};
bool last_press_state[NUM_CUBES] = {0};
typedef enum { NO_PRESS, SHORT_PRESS, LONG_PRESS } PressType;
uint8_t rolled[NUM_CUBES];

static const uint8_t *dice_arr[NUM_CUBES] = {dice1_bytes, dice2_bytes, dice3_bytes,
                                             dice4_bytes, dice5_bytes, dice6_bytes};

// Accelerometer Connecting
#define ACCELEROMETER_BUS_PORT i2c0
static int accelerometer_address = 0x68;
static int accelerometer_sda = 0;
static int accelerometer_scl = 1;

void init_button(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
}

bool button_pressed(uint pin) {
    if (gpio_get(pin)) {
        sleep_ms(20);          // wait
        return gpio_get(pin);  // check again
    }
    return false;
}

PressType check_long_press(uint pin, int index) {
    bool pressed_now = gpio_get(pin);

    if (pressed_now && !last_press_state[index]) {
        // pressed just now
        press_start_time[index] = time_us_64() / 1000;
    }

    if (!pressed_now && last_press_state[index]) {
        // released just now
        uint64_t duration = (time_us_64() / 1000) - press_start_time[index];
        last_press_state[index] = pressed_now;

        if (duration >= LONG_PRESS_TIME)
            return LONG_PRESS;
        else
            return SHORT_PRESS;
    }

    last_press_state[index] = pressed_now;
    return NO_PRESS;
}

void handle_locking() {
    // only lock if it was already rolled
    if (!first_roll) return;

    for (int i = 0; i < NUM_CUBES; i++) {
        PressType result = check_long_press(dice_buttons[i], i);

        if (result == SHORT_PRESS) {
            lock_dice[i] = !lock_dice[i];

            uint16_t draw_x = cube_positions[i].x;
            uint16_t draw_y = cube_positions[i].y;

            if (!lock_dice[i]) {
                draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1], CUBE_SIZE,
                                   0xFFFF);
            } else {
                draw_bitmap_with_background(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1],
                                            CUBE_SIZE, 0xFFFF, 0xF800);
            }
            printf("Dice %d %s\n", i + 1, lock_dice[i] ? "locked" : "unlocked");
            while (gpio_get(dice_buttons[i])) sleep_ms(10);
        }

        if (result == LONG_PRESS) {
            for (int j = 0; j < NUM_CUBES; j++) {
                lock_dice[j] = false;
                uint16_t draw_x = cube_positions[j].x;
                uint16_t draw_y = cube_positions[j].y;
                draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[j] - 1], CUBE_SIZE,
                                   0xFFFF);
            }
            printf("All dice unlocked\n");
            while (gpio_get(dice_buttons[i])) sleep_ms(10);
        }
    }
}

void roll_dice() {
    // Reset screen
    for (int i = 0; i < NUM_CUBES; i++) {
        // Don't reset the locked buttons
        if (!lock_dice[i]) {
            uint16_t draw_x = cube_positions[i].x;
            uint16_t draw_y = cube_positions[i].y;

            draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_clear_bytes, CUBE_SIZE, 0xFFFF);
        }
    }
    sleep_ms(300);

    // Animation
    uint32_t seed = (uint32_t)to_us_since_boot(get_absolute_time());
    uint8_t anim[NUM_CUBES];
    for (int t = 0; t < 2; t++) {
        // Generate random dice for animation
        for (int i = 0; i < NUM_CUBES; i++) {
            seed = seed * 1664525 + 17;
            anim[i] = (seed % 6) + 1;  // 1-6
        }
        // Draw the random dice
        for (int i = 0; i < NUM_CUBES; i++) {
            if (!lock_dice[i]) {
                uint16_t draw_x = cube_positions[i].x;
                uint16_t draw_y = cube_positions[i].y;

                draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[anim[i] - 1], CUBE_SIZE,
                                       0xFFFF);
            }
        }
        sleep_ms(1000);
        for (int i = 0; i < NUM_CUBES; i++) {
            if (!lock_dice[i]) {
                uint16_t draw_x = cube_positions[i].x;
                uint16_t draw_y = cube_positions[i].y;
                draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_clear_bytes, CUBE_SIZE,
                                       0xFFFF);
            }
        }
        sleep_ms(500);
    }

    // Roll 6 dice using seeded randomness
    // true randomness is not possible but can be noise detected from unused pins
    // adc_read()
    for (int i = 0; i < NUM_CUBES; i++) {
        if (!lock_dice[i]) {
            seed = seed * 1664525 + 1013904223;
            rolled[i] = (seed % 6) + 1;  // 1-6
        }
    }
    printf("Rolled dice: ");
    for (int i = 0; i < NUM_CUBES; i++) {
        printf("%d ", rolled[i]);
    }
    printf("\n");

    // Draw the rolled dice in a concentric radial pattern
    for (int i = 0; i < NUM_CUBES; i++) {
        if (!lock_dice[i]) {
            uint16_t draw_x = cube_positions[i].x;
            uint16_t draw_y = cube_positions[i].y;

            draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1], CUBE_SIZE,
                               0xFFFF);
        }
    }
}

int main() {
    stdio_init_all();
    gc9a01_init();
    gc9a01_clear();

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

    // Buttons
    init_button(PIN_BUTTON);
    init_button(PIN_BUTTON_1);
    init_button(PIN_BUTTON_2);
    init_button(PIN_BUTTON_3);
    init_button(PIN_BUTTON_4);
    init_button(PIN_BUTTON_5);
    init_button(PIN_BUTTON_6);

    // Evaluate Dice Positions
    // Cubes
    uint16_t cx = SCREEN_W / 2;
    uint16_t cy = SCREEN_H / 2;

    for (int i = 0; i < NUM_CUBES; i++) {
        float theta = 2.0f * M_PI * (float)i / (float)NUM_CUBES;
        float x = cx + RADIUS * cosf(theta);
        float y = cy + RADIUS * sinf(theta);

        uint16_t draw_x = (uint16_t)(x - CUBE_SIZE / 2);
        uint16_t draw_y = (uint16_t)(y - CUBE_SIZE / 2);

        cube_positions[i].x = draw_x;
        cube_positions[i].y = draw_y;
    }

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (1) {
        if (was_shaken(accelerometer_address, accelerometer_sda, accelerometer_scl)) {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            roll_dice();
            first_roll = true;

            while (gpio_get(PIN_BUTTON)) {
                sleep_ms(10);
            }
        }

        handle_locking();
        gpio_put(PICO_DEFAULT_LED_PIN, 0);

        sleep_ms(10);
    }

    while (true) {
        tight_loop_contents();
    }
}