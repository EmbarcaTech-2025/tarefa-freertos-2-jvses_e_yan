#ifndef JOYSTICK_LIB
#define JOYSTICK_LIB

#include "hardware/adc.h" // Biblioteca para manipulação do ADC no RP2040
#include "hardware/dma.h"
#include "hardware/irq.h"
#include <stdio.h>

// Definição dos pinos usados para o joystick e LEDs
#define VRX  26          // Pino de leitura do eixo X do joystick (conectado ao ADC)
#define VRY  27          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
#define ADC_CHANNEL_0  0 // Canal ADC para o eixo X do joystick
#define ADC_CHANNEL_1  1 // Canal ADC para o eixo Y do joystick
#define SW  22           // Pino de leitura do botão do joystick
#define THRESHOLD_ADC_LOW 1000  // Limiar mínimo para Y
#define THRESHOLD_ADC_HIGH 3000 // Limiar máximo para Y
#define ADC_CLOCK_DIV 125000

// Função para configurar o joystick (pinos de leitura, ADC e interrupção)
void setup_joystick();

// Função para ler os valores dos eixos do joystick (X e Y)
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value);

uint8_t joy_arrow(uint16_t x_val, uint16_t y_val);



#endif