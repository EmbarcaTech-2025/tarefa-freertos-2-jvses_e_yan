#ifndef EXAM_GAME
#define EXAM_GAME

// bibliotecas do SDK
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "pico/rand.h" //gerador de valores aleatórios

#include "include/oled_ctrl.h"
#include "include/neopin.h"
#include "include/joystick.h"

#define TURN_LIMIT 5 // limite de turnos para o exame

// definições de botões A e B
#define BUTTON_A 5
#define BUTTON_B 6


void exam_setup();
bool repeating_reader(struct repeating_timer *t);
void oled_print_info();

void gpio_irq_handler(uint gpio, uint32_t events);
void setup_buttons();
void update_level();
void exam_handler();


#endif