#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <stdint.h>

// Image dimensions: 240x240, RGB565 (2 bytes per pixel)
#define START_IMAGE_SIZE (240 * 240 * 2)

extern const uint8_t image1[START_IMAGE_SIZE];
extern const uint8_t image2[START_IMAGE_SIZE];
extern const uint8_t image3[START_IMAGE_SIZE];

#endif
