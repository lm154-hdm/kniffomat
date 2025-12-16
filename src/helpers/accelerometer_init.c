#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "accelerometer_init.h"

#define ACCELEROMETER_BUS_PORT i2c0


// Initialize I2C for accelerometer
void accelerometer_init(int accelerometer_adress, int accelerometer_sda, int accelerometer_scl) {
    // Initialize Pico's onboard LED
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    // Configure I2C Bus for accelerometer
    i2c_init(ACCELEROMETER_BUS_PORT, 100 * 10000);
    gpio_set_function(accelerometer_sda, GPIO_FUNC_I2C); // Set GPIO to I2C function (SDA)
    gpio_set_function(accelerometer_scl, GPIO_FUNC_I2C); // Set GPIO to I2C function (SCL)
    gpio_pull_up(accelerometer_sda); // Enable pull-up on SDA
    gpio_pull_up(accelerometer_scl); // Enable pull-up on SCL

    // wait for sensor to boot up
    sleep_ms(100); 

    // check connection: Read the WHO_AM_I register (0x75) which should return 0x68
    uint8_t who_am_i_reg = 0x75; // WHO_AM_I register address: 0x75
    uint8_t chip_id[1]; // array to store the chip ID
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_adress, &who_am_i_reg, 1, true); // write register address, (first i2c connection, address, pointer to data, length of data, keep control of bus)
    i2c_read_blocking(ACCELEROMETER_BUS_PORT, accelerometer_adress, chip_id, 1, false); // read 1 byte from the register (first i2c connection, address, data, length of data, release bus)

    if(chip_id[0] == 0x68) {
        // Visual clue for established connection: print line over serial, turn on onboard LED, wait for ... ms
        printf("MPU6050 connected");
        gpio_put(25, 1);
        sleep_ms(500);
    } else {
        // blink 2x to signalize failed connection
        printf("Failed to detect MPU6050");
        gpio_put(25, 0);
        gpio_put(25, 1);
        sleep_ms(300);
        gpio_put(25, 0);
        sleep_ms(300);
        gpio_put(25, 1);
        sleep_ms(300);
        gpio_put(25, 0);
    }

    // define array for setting up registers, first value is register adress and second is configuration
    uint8_t data[2]; 

    // setup power management register 107: 
    // no reset, disable sleep, set no cycle mode, reserved, disable temperature sensor, 3 set oscillator type: internal is not that accurate, gyroscope has better oscillator that is recommended to use. Not clear which axes, try X first.
    data[0] = 0x6B;
    data[1] = 0x28; // 10000001 // set cycle bit to true: devices cycles between sleep and waking up to take single accelerometer data. set temp_dis bit to true = disable temperature sensor
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_adress, data, 2, true);

    // setup power management register 108:
    // 2 wake up frequency 40Hz, rest bits: use standby accelerometer xyz / gyroscope xyz
    data[0] = 0x6C;
    data[1] = 0xC7; //11000111 // set cycle bit to true: devices cycles between sleep and waking up to take single accelerometer data. set temp_dis bit to true = disable temperature sensor
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_adress, data, 2, true);

    // register 28: 3 self test, 2 accelerometer configuration (fast hand movements should not exceed 5m/s^2 ~ 0.5G)
    data[0] = 0x1C;
    data[1] = 0x0; //00001000 = 0x8 //no self test, set scale range to 4G, 00000000 = 2G
    i2c_write_blocking(ACCELEROMETER_BUS_PORT, accelerometer_adress, data, 2, true);

    gpio_put(25, 0);
}