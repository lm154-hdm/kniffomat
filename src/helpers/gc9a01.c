#include "gc9a01.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

// Send command to display
void gc9a01_command(uint8_t cmd) {
    gpio_put(GC9A01_DC_PIN, 0);  // Command mode (DC low)
    spi_write_blocking(GC9A01_SPI_PORT, &cmd, 1);
}

// Send data to display
void gc9a01_data(uint8_t val) {
    gpio_put(GC9A01_DC_PIN, 1);  // Data mode (DC high)
    spi_write_blocking(GC9A01_SPI_PORT, &val, 1);
}

// Reset the display
void gc9a01_reset(void) {
    gpio_put(GC9A01_RST_PIN, 1);
    sleep_ms(10);
    gpio_put(GC9A01_RST_PIN, 0);
    sleep_ms(10);
    gpio_put(GC9A01_RST_PIN, 1);
    sleep_ms(10);
}

// Initialize module hardware (GPIO, SPI, PWM)
void gc9a01_module_init(void) {
    // Initialize GPIO pins
    gpio_init(GC9A01_RST_PIN);
    gpio_set_dir(GC9A01_RST_PIN, GPIO_OUT);
    
    gpio_init(GC9A01_DC_PIN);
    gpio_set_dir(GC9A01_DC_PIN, GPIO_OUT);
    
    gpio_init(GC9A01_BL_PIN);
    gpio_set_dir(GC9A01_BL_PIN, GPIO_OUT);
    
    // Set up PWM for backlight
    gpio_set_function(GC9A01_BL_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(GC9A01_BL_PIN);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 100);  // 100% brightness
    pwm_set_enabled(slice_num, true);
    
    // Initialize SPI
    spi_init(GC9A01_SPI_PORT, 40000000);  // 40 MHz
    gpio_set_function(GC9A01_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(GC9A01_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(GC9A01_SPI_CS, GPIO_FUNC_SPI);
    
    // Set SPI format
    spi_set_format(GC9A01_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

// Initialize display with exact sequence from Python code
void gc9a01_init(void) {
    gc9a01_module_init();
    gc9a01_reset();
    
    gc9a01_command(0xEF);
    gc9a01_command(0xEB);
    gc9a01_data(0x14);
    
    gc9a01_command(0xFE);
    gc9a01_command(0xEF);
    
    gc9a01_command(0xEB);
    gc9a01_data(0x14);
    
    gc9a01_command(0x84);
    gc9a01_data(0x40);
    
    gc9a01_command(0x85);
    gc9a01_data(0xFF);
    
    gc9a01_command(0x86);
    gc9a01_data(0xFF);
    
    gc9a01_command(0x87);
    gc9a01_data(0xFF);
    
    gc9a01_command(0x88);
    gc9a01_data(0x0A);
    
    gc9a01_command(0x89);
    gc9a01_data(0x21);
    
    gc9a01_command(0x8A);
    gc9a01_data(0x00);
    
    gc9a01_command(0x8B);
    gc9a01_data(0x80);
    
    gc9a01_command(0x8C);
    gc9a01_data(0x01);
    
    gc9a01_command(0x8D);
    gc9a01_data(0x01);
    
    gc9a01_command(0x8E);
    gc9a01_data(0xFF);
    
    gc9a01_command(0x8F);
    gc9a01_data(0xFF);
    
    gc9a01_command(0xB6);
    gc9a01_data(0x00);
    gc9a01_data(0x20);
    
    gc9a01_command(0x36);
    gc9a01_data(0x08);
    
    gc9a01_command(0x3A);
    gc9a01_data(0x05);
    
    gc9a01_command(0x90);
    gc9a01_data(0x08);
    gc9a01_data(0x08);
    gc9a01_data(0x08);
    gc9a01_data(0x08);
    
    gc9a01_command(0xBD);
    gc9a01_data(0x06);
    
    gc9a01_command(0xBC);
    gc9a01_data(0x00);
    
    gc9a01_command(0xFF);
    gc9a01_data(0x60);
    gc9a01_data(0x01);
    gc9a01_data(0x04);
    
    gc9a01_command(0xC3);
    gc9a01_data(0x13);
    
    gc9a01_command(0xC4);
    gc9a01_data(0x13);
    
    gc9a01_command(0xC9);
    gc9a01_data(0x22);
    
    gc9a01_command(0xBE);
    gc9a01_data(0x11);
    
    gc9a01_command(0xE1);
    gc9a01_data(0x10);
    gc9a01_data(0x0E);
    
    gc9a01_command(0xDF);
    gc9a01_data(0x21);
    gc9a01_data(0x0C);
    gc9a01_data(0x02);
    
    gc9a01_command(0xF0);
    gc9a01_data(0x45);
    gc9a01_data(0x09);
    gc9a01_data(0x08);
    gc9a01_data(0x08);
    gc9a01_data(0x26);
    gc9a01_data(0x2A);
    
    gc9a01_command(0xF1);
    gc9a01_data(0x43);
    gc9a01_data(0x70);
    gc9a01_data(0x72);
    gc9a01_data(0x36);
    gc9a01_data(0x37);
    gc9a01_data(0x6F);
    
    gc9a01_command(0xF2);
    gc9a01_data(0x45);
    gc9a01_data(0x09);
    gc9a01_data(0x08);
    gc9a01_data(0x08);
    gc9a01_data(0x26);
    gc9a01_data(0x2A);
    
    gc9a01_command(0xF3);
    gc9a01_data(0x43);
    gc9a01_data(0x70);
    gc9a01_data(0x72);
    gc9a01_data(0x36);
    gc9a01_data(0x37);
    gc9a01_data(0x6F);
    
    gc9a01_command(0xED);
    gc9a01_data(0x1B);
    gc9a01_data(0x0B);
    
    gc9a01_command(0xAE);
    gc9a01_data(0x77);
    
    gc9a01_command(0xCD);
    gc9a01_data(0x63);
    
    gc9a01_command(0x70);
    gc9a01_data(0x07);
    gc9a01_data(0x07);
    gc9a01_data(0x04);
    gc9a01_data(0x0E);
    gc9a01_data(0x0F);
    gc9a01_data(0x09);
    gc9a01_data(0x07);
    gc9a01_data(0x08);
    gc9a01_data(0x03);
    
    gc9a01_command(0xE8);
    gc9a01_data(0x34);
    
    gc9a01_command(0x62);
    gc9a01_data(0x18);
    gc9a01_data(0x0D);
    gc9a01_data(0x71);
    gc9a01_data(0xED);
    gc9a01_data(0x70);
    gc9a01_data(0x70);
    gc9a01_data(0x18);
    gc9a01_data(0x0F);
    gc9a01_data(0x71);
    gc9a01_data(0xEF);
    gc9a01_data(0x70);
    gc9a01_data(0x70);
    
    gc9a01_command(0x63);
    gc9a01_data(0x18);
    gc9a01_data(0x11);
    gc9a01_data(0x71);
    gc9a01_data(0xF1);
    gc9a01_data(0x70);
    gc9a01_data(0x70);
    gc9a01_data(0x18);
    gc9a01_data(0x13);
    gc9a01_data(0x71);
    gc9a01_data(0xF3);
    gc9a01_data(0x70);
    gc9a01_data(0x70);
    
    gc9a01_command(0x64);
    gc9a01_data(0x28);
    gc9a01_data(0x29);
    gc9a01_data(0xF1);
    gc9a01_data(0x01);
    gc9a01_data(0xF1);
    gc9a01_data(0x00);
    gc9a01_data(0x07);
    
    gc9a01_command(0x66);
    gc9a01_data(0x3C);
    gc9a01_data(0x00);
    gc9a01_data(0xCD);
    gc9a01_data(0x67);
    gc9a01_data(0x45);
    gc9a01_data(0x45);
    gc9a01_data(0x10);
    gc9a01_data(0x00);
    gc9a01_data(0x00);
    gc9a01_data(0x00);
    
    gc9a01_command(0x67);
    gc9a01_data(0x00);
    gc9a01_data(0x3C);
    gc9a01_data(0x00);
    gc9a01_data(0x00);
    gc9a01_data(0x00);
    gc9a01_data(0x01);
    gc9a01_data(0x54);
    gc9a01_data(0x10);
    gc9a01_data(0x32);
    gc9a01_data(0x98);
    
    gc9a01_command(0x74);
    gc9a01_data(0x10);
    gc9a01_data(0x85);
    gc9a01_data(0x80);
    gc9a01_data(0x00);
    gc9a01_data(0x00);
    gc9a01_data(0x4E);
    gc9a01_data(0x00);
    
    gc9a01_command(0x98);
    gc9a01_data(0x3E);
    gc9a01_data(0x07);
    
    gc9a01_command(0x35);
    gc9a01_command(0x21);
    
    gc9a01_command(0x11);
    sleep_ms(120);
    gc9a01_command(0x29);
    sleep_ms(20);
}

// Set display window for drawing
void gc9a01_set_windows(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end) {
    // Set the X coordinates
    gc9a01_command(0x2A);
    gc9a01_data(0x00);           // High byte of start X
    gc9a01_data(x_start);        // Low byte of start X
    gc9a01_data(0x00);           // High byte of end X
    gc9a01_data(x_end - 1);      // Low byte of end X
    
    // Set the Y coordinates
    gc9a01_command(0x2B);
    gc9a01_data(0x00);           // High byte of start Y
    gc9a01_data(y_start);        // Low byte of start Y
    gc9a01_data(0x00);           // High byte of end Y
    gc9a01_data(y_end - 1);      // Low byte of end Y
    
    gc9a01_command(0x2C);        // Write to RAM
}

// Clear display to white
void gc9a01_clear(void) {
    gc9a01_set_windows(0, 0, GC9A01_WIDTH, GC9A01_HEIGHT);
    gpio_put(GC9A01_DC_PIN, 1);  // Data mode
    
    uint8_t white_buffer[4096];
    for (int i = 0; i < 4096; i++) {
        white_buffer[i] = 0xFF;
    }
    
    uint32_t total_bytes = GC9A01_WIDTH * GC9A01_HEIGHT * 2;  // 2 bytes per pixel (RGB565)
    
    for (uint32_t i = 0; i < total_bytes; i += 4096) {
        uint32_t chunk_size = (total_bytes - i) < 4096 ? (total_bytes - i) : 4096;
        spi_write_blocking(GC9A01_SPI_PORT, white_buffer, chunk_size);
    }
}

// Clean up and exit
void gc9a01_module_exit(void) {
    // Turn off backlight
    uint slice_num = pwm_gpio_to_slice_num(GC9A01_BL_PIN);
    pwm_set_enabled(slice_num, false);
    
    // Reset pins to safe state
    gpio_put(GC9A01_RST_PIN, 1);
    gpio_put(GC9A01_DC_PIN, 0);
    gpio_put(GC9A01_BL_PIN, 1);
    
    sleep_ms(1);
    
    // Deinitialize SPI
    spi_deinit(GC9A01_SPI_PORT);
}
