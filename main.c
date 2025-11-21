#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// Pins
#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_DC   14
#define PIN_RST  15

// Screensize
#define SCREEN_W 240
#define SCREEN_H 240
#define RADIUS 80

// Dice size
#define CUBE_SIZE 60
#define POINT_SIZE 8
#define OFFSET 8
#define NUM_CUBES 6

// Dice Bitmaps
uint8_t dice_1[CUBE_SIZE][CUBE_SIZE];
uint8_t dice_2[CUBE_SIZE][CUBE_SIZE];
uint8_t dice_3[CUBE_SIZE][CUBE_SIZE];
uint8_t dice_4[CUBE_SIZE][CUBE_SIZE];
uint8_t dice_5[CUBE_SIZE][CUBE_SIZE];
uint8_t dice_6[CUBE_SIZE][CUBE_SIZE];
uint8_t dice_outline[CUBE_SIZE][CUBE_SIZE];

spi_inst_t *SPI_PORT = spi1;

// Define Dice outline Bitmap -> 1 = white, 0 = empty
void init_dice_outline() { 
  for(int y=0;y<CUBE_SIZE;y++){ 
    for(int x=0;x<CUBE_SIZE;x++){ 
      if(x==0 || x==CUBE_SIZE-1 || y==0 || y==CUBE_SIZE-1) 
        dice_outline[y][x] = 1; // edge 
      else dice_outline[y][x] = 0; // inside empty 
    } 
  } 
}

// Define Dice Number Bitmaps
void init_dice_points() {
    // Set everything to 0
    uint8_t* dice_array[6] = { (uint8_t*)dice_1, (uint8_t*)dice_2, (uint8_t*)dice_3,
                               (uint8_t*)dice_4, (uint8_t*)dice_5, (uint8_t*)dice_6 };

    for(int k=0;k<6;k++){
        for(int y=0;y<CUBE_SIZE;y++)
            for(int x=0;x<CUBE_SIZE;x++)
                dice_array[k][y*CUBE_SIZE+x] = 0;
    }

    // Set rasterized point -> change here to set the points to other positions
    void set_point(uint8_t d[CUBE_SIZE][CUBE_SIZE], int rx, int ry) {
      int px = OFFSET + rx*OFFSET*2;
      int py = OFFSET + ry*OFFSET*2;
      for(int y=0;y<POINT_SIZE;y++)
          for(int x=0;x<POINT_SIZE;x++)
              d[py+y][px+x] = 1;
    }

    // Dice 1
    set_point(dice_1, 1, 1);

    // Dice 2
    set_point(dice_2, 0,0);
    set_point(dice_2, 2,2);

    // Dice 3
    set_point(dice_3,0,0);
    set_point(dice_3,1,1);
    set_point(dice_3,2,2);

    // Dice 4
    set_point(dice_4,0,0);
    set_point(dice_4,2,0);
    set_point(dice_4,0,2);
    set_point(dice_4,2,2);

    // Dice 5
    set_point(dice_5,1,1);
    set_point(dice_5,0,0);
    set_point(dice_5,2,0);
    set_point(dice_5,0,2);
    set_point(dice_5,2,2);

    // Dice 6
    set_point(dice_6,0,0);
    set_point(dice_6,0,1);
    set_point(dice_6,0,2);
    set_point(dice_6,2,0);
    set_point(dice_6,2,1);
    set_point(dice_6,2,2);
}

// Set window to draw in
void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];

    // Column Address Set
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0); // Command
    uint8_t cmd = 0x2A;  // Column Address
    spi_write_blocking(SPI_PORT, &cmd, 1);

    gpio_put(PIN_DC, 1); // Daten
    data[0] = x0 >> 8; data[1] = x0 & 0xFF; // High and low byte
    data[2] = x1 >> 8; data[3] = x1 & 0xFF;
    spi_write_blocking(SPI_PORT, data, 4);
    gpio_put(PIN_CS, 1);

    // Page Address Set
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    cmd = 0x2B; // Page Address
    spi_write_blocking(SPI_PORT, &cmd, 1);

    gpio_put(PIN_DC, 1); // Data -> new data in the same data block
    data[0] = y0 >> 8; data[1] = y0 & 0xFF;
    data[2] = y1 >> 8; data[3] = y1 & 0xFF;
    spi_write_blocking(SPI_PORT, data, 4);
    gpio_put(PIN_CS, 1);

    // Memory Write
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    cmd = 0x2C;
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
    // ready to draw
}


void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    tft_set_window(x, y, x, y);

    uint8_t data[2];
    data[0] = color >> 8;    // High Byte
    data[1] = color & 0xFF;  // Low Byte

    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1); // Data
    spi_write_blocking(SPI_PORT, data, 2);
    gpio_put(PIN_CS, 1);
}

void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    tft_set_window(x, y, x + w - 1, y + h - 1);

    uint16_t i, pixels = w * h;
    uint8_t data[2] = { color >> 8, color & 0xFF };

    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1);

    for (i = 0; i < pixels; i++) {
        spi_write_blocking(SPI_PORT, data, 2);
    }

    gpio_put(PIN_CS, 1);
}

void draw_bitmap(uint16_t x0, uint16_t y0, uint8_t bitmap[CUBE_SIZE][CUBE_SIZE], uint16_t color) {
    for(int y=0;y<CUBE_SIZE;y++){
        for(int x=0;x<CUBE_SIZE;x++){
            if(bitmap[y][x])
                tft_fill_rect(x0+x, y0+y, 1,1,color);
        }
    }
}


int main() {
    stdio_init_all();
    spi_init(SPI_PORT, 10 * 1000 * 1000); // 10 MHz
    gpio_set_function(10, GPIO_FUNC_SPI); // SCK
    gpio_set_function(11, GPIO_FUNC_SPI); // MOSI

    // Steuerpins
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);

    // Reset & Display Init
    gpio_put(PIN_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_RST, 1);
    sleep_ms(100);

    // Sleep Out
    gpio_put(PIN_CS, 0); gpio_put(PIN_DC, 0);
    uint8_t cmd = 0x11; spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
    sleep_ms(120);

    // Display ON
    gpio_put(PIN_CS, 0); gpio_put(PIN_DC, 0);
    cmd = 0x29; spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);



    // Cubes

    // Init Bitmaps
    init_dice_outline();
    init_dice_points();

    uint16_t cx = SCREEN_W/2;
    uint16_t cy = SCREEN_H/2;

    // Draw Cubes with each number
    for(int i=0;i<NUM_CUBES;i++){
      float theta = 2*M_PI * i / NUM_CUBES;
      float x = cx + RADIUS * cosf(theta);
      float y = cy + RADIUS * sinf(theta);

      uint16_t draw_x = (uint16_t)(x - CUBE_SIZE/2); // Center cubes
      uint16_t draw_y = (uint16_t)(y - CUBE_SIZE/2);

      // random number 1-6 (e.g. fixed for test)
      int dice_num = i % 6 + 1;

      // Select bitmap
      uint8_t (*dice_bitmap)[CUBE_SIZE];
      switch(dice_num){
          case 1: dice_bitmap = dice_1; break;
          case 2: dice_bitmap = dice_2; break;
          case 3: dice_bitmap = dice_3; break;
          case 4: dice_bitmap = dice_4; break;
          case 5: dice_bitmap = dice_5; break;
          case 6: dice_bitmap = dice_6; break;
      }

      // Draw outline + points
      draw_bitmap(draw_x, draw_y, dice_outline, 0xFFFF);
      draw_bitmap(draw_x, draw_y, dice_bitmap, 0xFFFF);   
    }

    while (true) { tight_loop_contents(); }
}