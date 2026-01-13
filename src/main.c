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

#define NUM_DICE 6

// Lock Buttons
#define PIN_BUTTON_1 7
#define PIN_BUTTON_2 6
#define PIN_BUTTON_3 28
#define PIN_BUTTON_4 27
#define PIN_BUTTON_5 26
#define PIN_BUTTON_6 22
uint dice_buttons[NUM_DICE] = {PIN_BUTTON_1, PIN_BUTTON_2, PIN_BUTTON_3,
                               PIN_BUTTON_4, PIN_BUTTON_5, PIN_BUTTON_6};

// Screensize
#define SCREEN_W 240
#define SCREEN_H 240
#define RADIUS 80

// Dice size
#define DICE_SIZE 60
#define POINT_SIZE 8
#define OFFSET 8

// Dice position
typedef struct {
    uint16_t x;
    uint16_t y;
} DicePosition;
DicePosition dice_positions[NUM_DICE];

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
bool lock_dice[NUM_DICE] = {false, false, false, false, false, false};
#define LONG_PRESS_TIME 600  // ms
uint64_t press_start_time[NUM_DICE] = {0};
bool last_press_state[NUM_DICE] = {0};
typedef enum { NO_PRESS, SHORT_PRESS, LONG_PRESS } PressType;
uint8_t rolled[NUM_DICE];

static const uint8_t *dice_arr[NUM_DICE] = {dice1_bytes, dice2_bytes, dice3_bytes,
                                            dice4_bytes, dice5_bytes, dice6_bytes};

// Accelerometer Connections
#define ACCELEROMETER_BUS_PORT i2c0
static int accelerometer_address = 0x68;
static int accelerometer_sda = 0;
static int accelerometer_scl = 1;

void init_button(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
}

PressType check_long_press(uint pin, int index) {
    bool pressed = gpio_get(pin);

    if (pressed && !last_press_state[index]) {
        // Pressed
        press_start_time[index] = time_us_64() / 1000;
    }

    if (!pressed && last_press_state[index]) {
        // Released
        uint64_t duration = (time_us_64() / 1000) - press_start_time[index];
        last_press_state[index] = pressed;

        if (duration >= LONG_PRESS_TIME)
            return LONG_PRESS;
        else
            return SHORT_PRESS;
    }

    last_press_state[index] = pressed;
    return NO_PRESS;
}

void handle_locking() {
    // Only lock if it was already rolled
    if (!first_roll) return;

    for (int i = 0; i < NUM_DICE; i++) {
        // Check button state
        PressType result = check_long_press(dice_buttons[i], i);

        if (result == SHORT_PRESS) {
            lock_dice[i] = !lock_dice[i];  // toggle state

            uint16_t draw_x = dice_positions[i].x;
            uint16_t draw_y = dice_positions[i].y;

            if (!lock_dice[i]) {
                // Unlocked - Draw standard white number on black
                draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1], DICE_SIZE,
                                   0xFFFF);
            } else {
                // Locked - Draw white number on RED background to highlight selection
                draw_bitmap_with_background(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1],
                                            DICE_SIZE, 0xFFFF, 0xF800);
            }
            printf("Dice %d %s\n", i + 1, lock_dice[i] ? "locked" : "unlocked");

            // Block execution until button is released to prevent rapid toggling
            while (gpio_get(dice_buttons[i])) sleep_ms(10);
        }

        if (result == LONG_PRESS) {
            // Iterate through all dice to unlock and redraw them
            for (int j = 0; j < NUM_DICE; j++) {
                lock_dice[j] = false;
                uint16_t draw_x = dice_positions[j].x;
                uint16_t draw_y = dice_positions[j].y;

                // Redraw black background
                draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[j] - 1], DICE_SIZE,
                                   0xFFFF);
            }
            printf("All dice unlocked\n");

            // Wait for release of the button that triggered the reset
            while (gpio_get(dice_buttons[i])) sleep_ms(10);
        }
    }
}

void roll_dice() {
    // Clear unlocked dice
    for (int i = 0; i < NUM_DICE; i++) {
        if (!lock_dice[i]) {
            // Don't reset the locked buttons
            uint16_t draw_x = dice_positions[i].x;
            uint16_t draw_y = dice_positions[i].y;

            // Draw wipe effect
            draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_clear_bytes, DICE_SIZE, 0xFFFF);
        }
    }
    sleep_ms(300);

    // Animation
    uint8_t anim[NUM_DICE];
    // Flash random dice as animation
    for (int t = 0; t < 2; t++) {
        // Calculate random numbers
        for (int i = 0; i < NUM_DICE; i++) {
            uint32_t seed = (uint32_t)to_us_since_boot(get_absolute_time());
            seed = seed * 1664525 + 17;
            anim[i] = (seed % 6) + 1;  // 1-6
        }

        // Draw the random dice
        for (int i = 0; i < NUM_DICE; i++) {
            if (!lock_dice[i]) {
                uint16_t draw_x = dice_positions[i].x;
                uint16_t draw_y = dice_positions[i].y;

                draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[anim[i] - 1], DICE_SIZE,
                                       0xFFFF);
            }
        }
        sleep_ms(1000);  // hold glitch frame

        for (int i = 0; i < NUM_DICE; i++) {
            if (!lock_dice[i]) {
                uint16_t draw_x = dice_positions[i].x;
                uint16_t draw_y = dice_positions[i].y;
                draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_clear_bytes, DICE_SIZE,
                                       0xFFFF);
            }
        }
        sleep_ms(500);  // pause between blinks
    }

    // Roll 6 dice using random seed
    // Noise from unused pins might be used as rand generator
    // adc_read()
    for (int i = 0; i < NUM_DICE; i++) {
        if (!lock_dice[i]) {
            seed = seed * 1664525 + 1013904223;  // Linear Congruential Generator
            rolled[i] = (seed % 6) + 1;          // Result 1-6
        }
    }

    // Debug output
    printf("Rolled dice: ");
    for (int i = 0; i < NUM_DICE; i++) {
        printf("%d ", rolled[i]);
    }
    printf("\n");

    // Draw rolled dice in concentric radial pattern
    for (int i = 0; i < NUM_DICE; i++) {
        if (!lock_dice[i]) {
            uint16_t draw_x = dice_positions[i].x;
            uint16_t draw_y = dice_positions[i].y;

            draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1], DICE_SIZE,
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
    init_button(PIN_BUTTON_1);
    init_button(PIN_BUTTON_2);
    init_button(PIN_BUTTON_3);
    init_button(PIN_BUTTON_4);
    init_button(PIN_BUTTON_5);
    init_button(PIN_BUTTON_6);

    // Evaluate Dice Positions
    uint16_t cx = SCREEN_W / 2;
    uint16_t cy = SCREEN_H / 2;

    for (int i = 0; i < NUM_DICE; i++) {
        float theta = 2.0f * M_PI * (float)i / (float)NUM_DICE;
        float x = cx + RADIUS * cosf(theta);
        float y = cy + RADIUS * sinf(theta);

        uint16_t draw_x = (uint16_t)(x - DICE_SIZE / 2);
        uint16_t draw_y = (uint16_t)(y - DICE_SIZE / 2);

        dice_positions[i].x = draw_x;
        dice_positions[i].y = draw_y;
    }

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (1) {
        if (was_shaken(accelerometer_address, accelerometer_sda, accelerometer_scl)) {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            roll_dice();
            first_roll = true;
        }

        handle_locking();
        gpio_put(PICO_DEFAULT_LED_PIN, 0);

        sleep_ms(10);
    }
}
