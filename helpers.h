#ifndef HELPERS_H
#define HELPERS_H
#include <stdint.h>
#include <stddef.h>
#include "hardware/spi.h"

#define GC9A01_WIDTH   240
#define GC9A01_HEIGHT  240   // Works for 240x204 too (unused rows ignored)

void gc9a01_init(spi_inst_t* spi);
void draw_pixel(int x, int y, uint16_t color);
void display_fill(uint16_t color);
void gc9a01_reset(void);
void gc9a01_command(uint8_t cmd);
void gc9a01_data(uint8_t value);
void gc9a01_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

// void draw_packed_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data,
//                     int cube_size, uint16_t color);
// void draw_artifacted_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data,
//                     int cube_size, uint16_t color);

#endif // HELPERS_H
