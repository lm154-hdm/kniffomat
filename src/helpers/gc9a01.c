#include "gc9a01.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

// Send command to display
void gc9a01_command(uint8_t cmd) {
    gpio_put(DC_PIN, 0);  // Command mode (DC low)
    spi_write_blocking(SPI_PORT, &cmd, 1);
}

// Send data to display
void gc9a01_data(uint8_t val) {
    gpio_put(DC_PIN, 1);  // Data mode (DC high)
    spi_write_blocking(SPI_PORT, &val, 1);
}

// Reset the display
void gc9a01_reset(void) {
    gpio_put(RST_PIN, 0);
    sleep_ms(10);
    gpio_put(RST_PIN, 1);
    sleep_ms(10);
}

// Initialize module hardware (GPIO, SPI, PWM)
void gc9a01_module_init(void) {
    gpio_init(DC_PIN);
    gpio_set_dir(DC_PIN, GPIO_OUT);

    gpio_init(RST_PIN);
    gpio_set_dir(RST_PIN, GPIO_OUT);
    
    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1);
    
    // Initialize SPI
    spi_init(SPI_PORT, 40000000);  // 40 MHz
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_CS, GPIO_FUNC_SPI);
    
    // Set SPI format
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

// Initialize display
// 1. Active Low Signal to RST pin
// 2. Sleep Out Mode (0x11) (wait 5ms before sending next command)
// 3. Enable Registers send FEh, then EFh
// 4. Vcore voltage A7h using vdd_ad[3:0]
// 5. Power Control 4 C9h using vrh[5:0]
// 6. Power Control 2 C3h using vreg1_vbp_d[6:0]
// 7. Power Control 3 C4h using vreg1_vbp_d[6:0]
// 8. Power Control 1 C1h 
// 9. Gamma Correction F0h, F1h, F2h, F3h
// 10. Frame Rate Control E8h (DINV[3:0]) and frame rate related parameters (RTN1[3:0] and RTN2[7:0])
// 11. Memory Access Control 36h (display orientation)
// 12. Interface Pixel Format 3Ah (8bits/pixel)
// 13. Adress Window Setup 2Ah, 2Bh (column and row address set)
// for a full 240x240 display under default settings 
// (MADCTL B5=0): SC=0000h, EC=013Fh (320 columns, !!!!
// although only 240 are active on the panel), 
// SP=0000h, and EP=00EFh (240 rows)
// Display on 29h
// Write display data 2Ch
// Brightness control 51h 53h
void gc9a01_init(void) {
    gc9a01_module_init();
    gc9a01_reset();

    sleep_ms(120);
    
    gc9a01_command(0xEF);
    gc9a01_command(0xEB);
    gc9a01_data(0x14);
    
    gc9a01_command(0xFE);   // Enable Registers
    gc9a01_command(0xEF);
    
    gc9a01_command(0xEB);
    gc9a01_data(0x14);
    
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
    
    gc9a01_command(0x36);   // Memory Access Control
    gc9a01_data(0x08);      // MX, MY, MV, ML, BGR, MH
    
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
    gc9a01_data(0x40);
    gc9a01_data(0x8A);
    
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
    
    gc9a01_command(0x35);   // Tearing effect line on
    gc9a01_command(0x21);   // Display inversion on
    
    gc9a01_command(0x11);   // Exit Sleep mode
    sleep_ms(120);
    gc9a01_command(0x29);   // Turn Display on
    sleep_ms(20);
}

// Set display window for drawing
void gc9a01_set_windows(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    gc9a01_command(0x2A);
    gc9a01_data(0x00);              // High byte = 0x00 
    gc9a01_data(x_start & 0xFF);    // Low byte only
    gc9a01_data(0x00);              // High byte = 0x00 
    gc9a01_data(x_end & 0xFF);      // Low byte 
    
    gc9a01_command(0x2B);
    gc9a01_data(0x00);              // High byte = 0x00 
    gc9a01_data(y_start & 0xFF);    // Low byte only
    gc9a01_data(0x00);              // High byte = 0x00 
    gc9a01_data(y_end & 0xFF);      // Low byte 
    
    gc9a01_command(0x2C);  // Memory Write
}

// Clear display to black
void gc9a01_clear(void) {
    gc9a01_set_windows(0, 0, GC9A01_WIDTH, GC9A01_HEIGHT);
    gpio_put(DC_PIN, 1);
    
    uint8_t black_buffer[4096];
    for (int i = 0; i < 4096; i++) {
        black_buffer[i] = 0x00;
    }
    
    uint32_t total_bytes = GC9A01_WIDTH * GC9A01_HEIGHT * 2;
    
    for (uint32_t i = 0; i < total_bytes; i += 4096) {
        uint32_t chunk_size = (total_bytes - i) < 4096 ? (total_bytes - i) : 4096;
        spi_write_blocking(SPI_PORT, black_buffer, chunk_size);
    }
}

// uncomment unused function
//void gc9a01_module_exit(void) {
//    // Turn off backlight
//    uint slice_num = pwm_gpio_to_slice_num(BL_PIN);
//    pwm_set_enabled(slice_num, false);
//    
//    // Reset pins to safe state
//    gpio_put(RST_PIN, 1);
//    gpio_put(DC_PIN, 0);
//    gpio_put(BL_PIN, 1);
//    
//    sleep_ms(1);
//    
//    // Deinitialize SPI
//    spi_deinit(SPI_PORT);
//}
