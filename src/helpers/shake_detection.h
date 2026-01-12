#ifndef SHAKE_DETECTION_H
#define SHAKE_DETECTION_H

#include <stdbool.h>
#include <stdint.h>

bool was_shaken(int accelerometer_address, int accelerometer_sda, int accelerometer_scl);
// void accelerometer_read_a(int accelerometer_address, int accelerometer_sda, int
// accelerometer_scl);

#endif
