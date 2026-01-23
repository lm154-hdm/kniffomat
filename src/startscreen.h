#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <stdint.h>

// Image dimensions: 240x240, RGB565 (2 bytes per pixel)
#define START_IMAGE_SIZE (240 * 240 * 2)

extern const uint8_t logo[START_IMAGE_SIZE];

#endif
