#include "accelerometer_init.h"

#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define ACCELEROMETER_BUS_PORT i2c0

// Initialize accelerometer, including I2C explanation
void accelerometer_init(int accelerometer_address, int accelerometer_sda, int accelerometer_scl) {
    // Initialize Pico's onboard LED
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    // Configure I2C Bus for accelerometer
    i2c_init(ACCELEROMETER_BUS_PORT, 100 * 10000);
    gpio_set_function(accelerometer_sda, GPIO_FUNC_I2C);  // set gpio pin to SDA
    gpio_set_function(accelerometer_scl, GPIO_FUNC_I2C);  // set gpio pin to SCL
    gpio_pull_up(accelerometer_sda);                      // enable pull-up for SDA
    gpio_pull_up(accelerometer_scl);                      // enable pull-up for SCL

    // Wait for sensor to boot up
    sleep_ms(100);

    // Check connection: Read the WHO_AM_I register (chip ID) [75], should return 0x68
    uint8_t who_am_i_reg = 0x75;
    uint8_t chip_id[1];

    // Example explanation for I2C bus communication for the following 2 lines:
    // Write register address, arguments:
    //(first i2c connection, address, pointer to data, length of data, keep control of bus)
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, &who_am_i_reg, 1, true);

    // Read from register (first i2c connection, address, data, length of data, release bus)
    i2c_read_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, chip_id, 1, false);

    if (chip_id[0] == 0x68) {
        // If returned Chip ID matches, turn on LED, print line over serial
        printf("MPU6050 connected");
        gpio_put(25, 1);
        sleep_ms(50);
    } else {
        // Blink to signalize failed connection
        gpio_put(25, 0);
        printf("Failed to detect MPU6050");
        int i = 0;
        while (i < 6) {
            gpio_put(25, 1);
            sleep_ms(300);
            gpio_put(25, 0);
            sleep_ms(300);
            i++;
        }
    }

    // Configure accelerometer
    // Use 2 bytes for setting register bits, 1st byte register adress, 2nd bits for config
    // Comments explain register number, what register bits do, what is set true/false (and why)
    uint8_t data[2];

    // Power management, register 107 [6B], bits function:
    // 7 reset, 6 sleep mode, 5 cycle mode, 4 reserved, 3 disable temp sensor, 2...0 clock source
    // Reset. disable sleep, cycle & temp sensor. Use x axes gyro for clock source
    data[0] = 0x6B;
    data[1] = 0x28;  // 10001001 /aktuell 0x28 = 40 = 00101000
    // data[1] = 0b10001001;  // testen
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, data, 2, true);

    // Power management, register 108 [6C], bits function:
    // 7...6 low power mode wake control, 5...3 standby accelerometer xyz, 2...0 standby gyro xyz
    // low power mode was disabled in register 107. Put gyro axes in standby, gyro not needed.
    data[0] = 0x6C;
    data[1] = 0xC7;  // 11000111
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, data, 2, true);

    // Accelerometer configuration, Register 28 [1C], bits function:
    // 7...5 self test x/y/z axes, 4...3 scale range 0/1/2/3 -> 2G/4G/8G/16G
    // No self test, set scale range to 8G
    data[0] = 0x1C;
    data[1] = 0x16;  // 00001000 = 0x8
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_address, data, 2, true);

    gpio_put(25, 0);
}
