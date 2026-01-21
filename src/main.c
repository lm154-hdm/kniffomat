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
#include "startscreen.h"

// Constants
#define M_PI 3.14159265358979323846

#define NUM_DICE 6

// Lock Buttons
#define PIN_BUTTON_1 16
#define PIN_BUTTON_2 17
#define PIN_BUTTON_3 18
#define PIN_BUTTON_4 19
#define PIN_BUTTON_5 20
#define PIN_BUTTON_6 21
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
static const uint8_t *dice_arr[NUM_DICE] = {dice1_bytes, dice2_bytes, dice3_bytes,
                                            dice4_bytes, dice5_bytes, dice6_bytes};

// Long/Short Press Detection
#define DEBOUNCE_US 30000
#define LONG_PRESS_US 600000
typedef enum { NO_PRESS, SHORT_PRESS, LONG_PRESS } PressType;
typedef struct {
    bool last_state;              // logical pressed (true = pressed)
    absolute_time_t press_start;  // when press started
    bool long_fired;              // long press already reported
} button_t;
button_t dice_button_state[NUM_DICE] = {0};

// Blinking Animation
#define BLINK_INTERVAL_MS 300
static absolute_time_t last_blink_time;
static bool blink_state = false;
static bool blink_dirty = false;
static int blink_draw_index = 0;

// Dice Initialization and Locking
bool first_roll = false;
bool lock_dice[NUM_DICE] = {false, false, false, false, false, false};
bool init_dice[NUM_DICE] = {false, false, false, false, false, false};
uint8_t rolled[NUM_DICE];

// Accelerometer Connections
#define ACCELEROMETER_BUS_PORT i2c0
static int accelerometer_address = 0x68;
static int accelerometer_sda = 0;
static int accelerometer_scl = 1;

void init_button(uint pin) {
    gpio_init(pin);
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

PressType button_update(button_t *btn, uint pin) {
    bool pressed = !gpio_get(pin);  // pull-up → invert

    absolute_time_t now = get_absolute_time();

    // Press Start
    if (pressed && !btn->last_state) {
        btn->press_start = now;
        btn->long_fired = false;
        btn->last_state = true;
        return NO_PRESS;
    }

    // Hold
    if (pressed && btn->last_state) {
        if (!btn->long_fired && absolute_time_diff_us(btn->press_start, now) >= LONG_PRESS_US) {
            btn->long_fired = true;
            return LONG_PRESS;
        }
    }

    // Release
    if (!pressed && btn->last_state) {
        btn->last_state = false;

        if (!btn->long_fired) {
            // Short press only if long wasn't triggered
            return SHORT_PRESS;
        }
    }

    return NO_PRESS;
}

// Initialize Dice before first roll
void handle_dice_initialization() {
    for (int i = 0; i < NUM_DICE; i++) {
        PressType ev = button_update(&dice_button_state[i], dice_buttons[i]);
        if (ev == SHORT_PRESS) {
            init_dice[i] = !init_dice[i];  // toggle state
        }
    }
}

// Set blinking state for all dice
void blink_dice_tick(void) {
    absolute_time_t now = get_absolute_time();

    if (absolute_time_diff_us(last_blink_time, now) >= BLINK_INTERVAL_MS * 1000) {
        last_blink_time = now;
        blink_state = !blink_state;

        blink_dirty = true;
        blink_draw_index = 0;
    }
}

// Async render blinking dice
void blink_dice_render(void) {
    if (!blink_dirty) return;

    uint16_t x = dice_positions[blink_draw_index].x;
    uint16_t y = dice_positions[blink_draw_index].y;

    // Make sure initialized dice are shown
    if (init_dice[blink_draw_index]) {
        draw_packed_bitmap(SPI_PORT, x, y, dice_arr[blink_draw_index], DICE_SIZE, 0xFFFF);
        // Show
    } else if (blink_state) {
        draw_packed_bitmap(SPI_PORT, x, y, dice_arr[blink_draw_index], DICE_SIZE, 0xFFFF);
        // Hide
    } else {
        draw_artifacted_bitmap(SPI_PORT, x, y, dice_clear_bytes, DICE_SIZE, 0xFFFF);
    }

    blink_draw_index++;

    if (blink_draw_index >= NUM_DICE) {
        blink_draw_index = 0;
        blink_dirty = false;  // finished rendering
    }
}

void handle_locking() {
    for (int i = 0; i < NUM_DICE; i++) {
        // Don'tl listen to uninitialized dice
        if (!init_dice[i]) {
            continue;
        }

        // Check button state
        PressType result = button_update(&dice_button_state[i], dice_buttons[i]);

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
                                            DICE_SIZE, 0xFFFF, 0x07E0);
            }
        }

        if (result == LONG_PRESS) {
            // Iterate through all initialized dice to unlock and redraw them
            for (int j = 0; j < NUM_DICE; j++) {
                if (init_dice[j]) {
                    lock_dice[j] = false;
                    uint16_t draw_x = dice_positions[j].x;
                    uint16_t draw_y = dice_positions[j].y;

                    // Redraw black background
                    draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[j] - 1], DICE_SIZE,
                                       0xFFFF);
                }
            }
        }
    }
}

void check_dice_initialization() {
    // If no Dice was initialized, initialize all Dice
    bool all_uninitialized = true;
    for (int i = 0; i < NUM_DICE; i++) {
        if (init_dice[i]) {
            all_uninitialized = false;
            break;
        }
    }
    if (all_uninitialized) {
        for (int i = 0; i < NUM_DICE; i++) {
            init_dice[i] = true;
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
    sleep_ms(100);

    // Animation
    uint32_t seed = (uint32_t)to_us_since_boot(get_absolute_time());
    uint8_t anim[NUM_DICE];
    // Flash random dice as animation
    for (int t = 0; t < 3; t++) {
        // Calculate random numbers
        for (int i = 0; i < NUM_DICE; i++) {
            seed = seed * 1664525 + 17;
            anim[i] = (seed % 6) + 1;  // 1-6
        }

        // Draw the random dice -> ignore uninitialized and locked dice
        for (int i = 0; i < NUM_DICE; i++) {
            if (init_dice[i] && !lock_dice[i]) {
                uint16_t draw_x = dice_positions[i].x;
                uint16_t draw_y = dice_positions[i].y;

                draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[anim[i] - 1], DICE_SIZE,
                                       0xFFFF);
            }
        }
        sleep_ms(500);  // hold glitch frame

        // Clear the drawn dice -> ignore uninitialized and locked dice
        for (int i = 0; i < NUM_DICE; i++) {
            if (init_dice[i] && !lock_dice[i]) {
                uint16_t draw_x = dice_positions[i].x;
                uint16_t draw_y = dice_positions[i].y;
                draw_artifacted_bitmap(SPI_PORT, draw_x, draw_y, dice_clear_bytes, DICE_SIZE,
                                       0xFFFF);
            }
        }
        sleep_ms(200);  // pause between blinks
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

    // Draw rolled dice in concentric radial pattern -> ignore uninitialized and locked dice
    for (int i = 0; i < NUM_DICE; i++) {
        if (init_dice[i] && !lock_dice[i]) {
            uint16_t draw_x = dice_positions[i].x;
            uint16_t draw_y = dice_positions[i].y;

            draw_packed_bitmap(SPI_PORT, draw_x, draw_y, dice_arr[rolled[i] - 1], DICE_SIZE,
                               0xFFFF);
        }
    }
}

int main() {
    stdio_init_all();

    // Buttons
    init_button(PIN_BUTTON_1);
    init_button(PIN_BUTTON_2);
    init_button(PIN_BUTTON_3);
    init_button(PIN_BUTTON_4);
    init_button(PIN_BUTTON_5);
    init_button(PIN_BUTTON_6);

    gc9a01_init();
    gc9a01_clear();

    // Initialize Accelerometer
    accelerometer_init(accelerometer_address, accelerometer_sda, accelerometer_scl);

    // Draw start screen (test different colors. Remove two of them later)
    gc9a01_set_windows(0, 0, 239, 239);
    gpio_put(DC_PIN, 1);

    spi_write_blocking(SPI_PORT, image1, START_IMAGE_SIZE);
    sleep_ms(3000);

    spi_write_blocking(SPI_PORT, image2, START_IMAGE_SIZE);
    sleep_ms(3000);

    spi_write_blocking(SPI_PORT, image3, START_IMAGE_SIZE);
    sleep_ms(3000);

    gc9a01_clear();

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

    // Initial blink setup
    last_blink_time = get_absolute_time();
    blink_state = false;

    while (1) {
        // Roll Dice on shake
        if (was_shaken(accelerometer_address, accelerometer_sda, accelerometer_scl)) {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            if (!first_roll) check_dice_initialization();
            roll_dice();  // This locks every input while rolling. This is wanted.
            first_roll = true;

            // Reset button states after roll
            for (int i = 0; i < NUM_DICE; i++) {
                dice_button_state[i].last_state = false;
                dice_button_state[i].long_fired = false;
            }
        }

        // Handle blinking and dice initialization before first roll
        if (!first_roll) {
            handle_dice_initialization();
            blink_dice_tick();
            blink_dice_render();
        }
        // Handle locking after first roll
        else {
            handle_locking();
        }

        gpio_put(PICO_DEFAULT_LED_PIN, 0);

        sleep_ms(1);
    }
}
