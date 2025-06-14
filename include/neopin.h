#ifndef NEOPIN_LIB
#define NEOPIN_LIB

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include <string.h> // Para memcpy()



#define LED_COUNT 25
#define LED_NEOPIN 7

#define LOW_BRIGHT 0x80 // b10000000 que a máquina entende como b00000001 ou seja valor mínimo de brilho
#define MED_BRIGHT 0xE  // b00001110 -> b0111000 (112)
#define HIG_BRIGHT 0x5F // b01011111 -> b11111010 (250)

// Definição de pixel GRB
struct pixel_t {
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;


void npInit(uint pin);
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);
void npSetArrow(uint8_t direcao);
void npClear();//Limpa o buffer de pixels.
void npWrite();// Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
void npSetWait(uint8_t r, uint8_t g, uint8_t b);//desenha ampulheta
uint8_t rand_sign();
void npDrawArrow(uint8_t placa);
void npDrawAmpulheta(uint8_t r, uint8_t g, uint8_t b);



#endif