#ifndef ACCELEROMETER_INIT_H
#define ACCELEROMETER_INIT_H

#include <stdint.h>

#include "hardware/spi.h"

void accelerometer_init(int accelerometer_adress, int accelerometer_sda, int accelerometer_scl);

#endif