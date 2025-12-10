#ifndef HELPERS_H
#define HELPERS_H
#include <stdint.h>
#include "hardware/spi.h"

void draw_packed_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data, 
                    int cube_size, uint16_t color);
void draw_artifacted_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data, 
                    int cube_size, uint16_t color);
void draw_bitmap_with_background(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data,
                    int cube_size, uint16_t fg_color, uint16_t bg_color);

#endif
