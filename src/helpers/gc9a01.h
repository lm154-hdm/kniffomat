#ifndef GC9A01_H
#define GC9A01_H

#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"

// Display dimensions
#define GC9A01_WIDTH 240
#define GC9A01_HEIGHT 240

// Pin definitions
#define DC_PIN 14
#define RST_PIN 15
#define BL_PIN 12

// SPI configuration
#define SPI_PORT spi1
#define SPI_SCK 10
#define SPI_MOSI 11
#define SPI_CS 13

// Function declarations
void gc9a01_command(uint8_t cmd);
void gc9a01_data(uint8_t val);
void gc9a01_reset(void);
void gc9a01_module_init(void);
void gc9a01_init(void);
void gc9a01_set_windows(uint16_t x_start, uint16_t y_start, uint16_t x_end,
                        uint16_t y_end);  // Changed from uint8_t
void gc9a01_clear(void);
void gc9a01_module_exit(void);

#endif  // GC9A01_H
