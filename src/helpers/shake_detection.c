#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "shake_detection.h"

#define ACCELEROMETER_BUS_PORT i2c0

bool was_shaken(int accelerometer_address, int accelerometer_sda, int accelerometer_scl) {
    int i = 0;

    uint8_t accel_data[6]; // store the 3 axes accelerometer data, each 2 byte
    uint8_t accel_data_register = 0x3B; // where the register for accelerometer Data starts
    uint16_t accel_values_x, accel_values_y, accel_values_z;

    // 100 000 iterations -> 8-9s
    while(i <= 100) {
        i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, &accel_data_register, 1, true); // tell sensor we want to read starting from register 3B
        i2c_read_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, accel_data, 6, false); // read 6 byte of information and store into array
        //0x7FFF = 01111111 1111111. Works with 2G and shaking moderately 0x00FF = 00000000 11111111, 0x8000 = 10000000 00000000

        accel_values_x = (accel_data[0] << 8 | accel_data[1]);
        accel_values_y = (accel_data[2] << 8 | accel_data[3]);
        accel_values_z = (accel_data[4] << 8 | accel_data[5]);
        
        // negative acceleration not working! If acceleration not close to zero, break loop, return true (2 complement)
        if(accel_values_x > 0x1FFF && accel_values_x < 0x8000 || accel_values_x > 0x8000 && accel_values_x < 0xE000 ||
            accel_values_y > 0x1FFF && accel_values_y < 0x8000 || accel_values_y > 0x8000 && accel_values_y < 0xE000) {
            return true;
        }
        i++;
    }
    return false;
}