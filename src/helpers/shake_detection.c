#include "shake_detection.h"

#include <stdint.h>
#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define ACCELEROMETER_BUS_PORT i2c0

bool was_shaken(int accelerometer_address, int accelerometer_sda, int accelerometer_scl) {
    int i = 0;

    // Store 3 axes accelerometer data, 2 byte per axis
    uint8_t accel_data[6];
    // Starting register for accelerometer Data
    uint8_t accel_data_register = 0x3B;

    // 100 000 iterations take ~8-9s
    while (i <= 100) {
        // Tell sensor we want to read starting from register 3B
        i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, &accel_data_register, 1,
                           true);
        // Read 6 byte of information and store into array
        i2c_read_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, accel_data, 6, false);
        // 0x7FFF = 01111111 1111111. Works with 2G and shaking moderately 0x00FF = 00000000
        // 11111111, 0x8000 = 10000000 00000000

        uint16_t accel_values_x = (accel_data[0] << 8 | accel_data[1]);
        uint16_t accel_values_y = (accel_data[2] << 8 | accel_data[3]);
        // uint16_t accel_values_z = (accel_data[4] << 8 | accel_data[5]);

        // TODO: FIX - negative acceleration not working!
        // If acceleration not close to zero, break loop, return true (2 complement)
        if (accel_values_x > 0x1FFF && accel_values_x < 0x8000 ||
            accel_values_x > 0x8000 && accel_values_x < 0xE000 ||
            accel_values_y > 0x1FFF && accel_values_y < 0x8000 ||
            accel_values_y > 0x8000 && accel_values_y < 0xE000) {
            return true;
        }
        i++;
    }
    return false;
}
