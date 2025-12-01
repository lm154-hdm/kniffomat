#ifndef HELPERS_H
#define HELPERS_H
#include <stdint.h>

#include "hardware/spi.h"

void clear_bitmap(uint8_t *bm, int cube_size);
void tft_set_window(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void draw_packed_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data,
                        int cube_size, uint16_t color);
void draw_artifacted_bitmap(spi_inst_t *spi_port, uint16_t x0, uint16_t y0, const uint8_t *data,
                            int cube_size, uint16_t color);

#endif  // HELPERS_H
