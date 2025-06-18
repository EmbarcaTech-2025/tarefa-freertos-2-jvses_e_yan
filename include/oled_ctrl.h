#ifndef OLED_CONTROL
#define OLED_CONTROL

#include <string.h> // Para memcpy() e memeset
#include <stdio.h>

//bibliotecas para display OLED
#include "pico/binary_info.h"
#include "include/ssd1306.h"
#include "hardware/i2c.h"

//Display OLED
#define I2C_SDA  14
#define I2C_SCL  15


void setup_OLED();
void oled_clear();
void oled_render();
void oled_msg_print_nivel(int8_t nivel);
void oled_msg_inicio(int8_t nivel);
void oled_times_print(int8_t nivel, int8_t contador_turnos,float *tempo_turnos,uint8_t start_addry);
void oled_print_media(float soma);
void oled_print_text_xy(const char *str, uint8_t x, uint8_t y);

#endif