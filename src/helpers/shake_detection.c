#include "shake_detection.h"

#include <stdint.h>
#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define ACCELEROMETER_BUS_PORT i2c0

// for I2C explanation, see accelerometer_init.c

bool was_shaken(int accelerometer_address, int accelerometer_sda, int accelerometer_scl) {
    int i = 0;

    // Store 3 axes accelerometer data, 2 byte per axis
    uint8_t accel_data[6];
    // Starting register number for accelerometer xyz data
    uint8_t accel_data_register = 0x3B;

    // Check connection, if Chip ID can't be verified -> connection error, return false
    uint8_t who_am_i_reg = 0x75;
    uint8_t chip_id[1];

    // read chip ID
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, &who_am_i_reg, 1, true);
    i2c_read_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, chip_id, 1, false);

    if (chip_id[0] != 0x68) {
        printf("Failed to detect MPU6050");
        return false;
    }

    while (i <= 100) {
        // Call accelerometer values register
        i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, &accel_data_register, 1,
                           true);
        // Read accelerometer xyz values and store into array (values are 2 complements)
        i2c_read_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, accel_data, 6, false);

        // store 2 byte per values into single variable. Use bit shift for upper 8 bits
        uint16_t accel_values_x = (accel_data[0] << 8 | accel_data[1]);
        uint16_t accel_values_y = (accel_data[2] << 8 | accel_data[3]);
        uint16_t accel_values_z = (accel_data[4] << 8 | accel_data[5]);

        // If acceleration exceeds limit, return true
        if (accel_values_x > 0x1FFF && accel_values_x < 0x8000 ||
            accel_values_x > 0x8000 && accel_values_x < 0xE000 ||
            accel_values_y > 0x1FFF && accel_values_y < 0x8000 ||
            accel_values_y > 0x8000 && accel_values_y < 0xE000 ||
            accel_values_z > 0x1FFF && accel_values_z < 0x8000 ||
            accel_values_z > 0x8000 && accel_values_z < 0xE000) {
            return true;
        }
        i++;
    }
    return false;
}