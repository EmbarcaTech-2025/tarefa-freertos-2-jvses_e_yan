#include "include/neopin.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;
/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }
  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
  // Limpa buffer de pixels.
  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

// As direções serão configuradas
// a partir do norte e seguirão
// sentido horário. portanto,
// norte=0,nordeste=1,leste=2 ... nordeste=7

// Nisponibilidade de LEDS na matriz
// 24 23 22 21 20
// 15 16 17 18 19
// 14 13 12 11 10
//  5  6  7  8  9
//  4  3  2  1  0


void npSetArrow(uint8_t direcao){ //desenha a seta de cordo com a direção
  uint8_t leds_addrs[9]={};
  switch (direcao){
  case 0://norte
    uint8_t aux0[]={2,7,10,12,14,16,17,18,22};
    memcpy(leds_addrs, aux0, sizeof(leds_addrs));
    break;
  case 1://nordeste
    uint8_t aux1[]={4,6,10,12,18,19,20,21,22};
    memcpy(leds_addrs, aux1, sizeof(leds_addrs));
    break;
  case 2://leste
    uint8_t aux2[]={2,8,10,11,12,13,14,18,22};
    memcpy(leds_addrs, aux2, sizeof(leds_addrs));
    break;
  case 3://suldeste
    uint8_t aux3[]={0,1,2,8,9,10,12,16,24};
    memcpy(leds_addrs, aux3, sizeof(leds_addrs));
    break;
  case 4://sul
    uint8_t aux4[]={2,6,7,8,10,12,14,17,22};
    memcpy(leds_addrs, aux4, sizeof(leds_addrs));
    break;
  case 5://suldoeste
    uint8_t aux5[]={2,3,4,5,6,14,12,18,20};
    memcpy(leds_addrs, aux5, sizeof(leds_addrs));
    break;
  case 6://oeste
    uint8_t aux6[]={2,6,10,11,12,13,14,16,22};
    memcpy(leds_addrs, aux6, sizeof(leds_addrs));
    break;
  case 7://noroeste
    uint8_t aux7[]={0,8,12,14,15,16,22,23,24};
    memcpy(leds_addrs, aux7, sizeof(leds_addrs));
    break;
  default://centrado ou outros valores
      uint8_t aux_def[]={6,7,8,11,12,13,16,17,18};
      memcpy(leds_addrs, aux_def, sizeof(leds_addrs));
    break;
  }
  for(int i=0; i< 9;i++){
    npSetLED(leds_addrs[i],LOW_BRIGHT,LOW_BRIGHT,LOW_BRIGHT);
  }
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

void npSetWait(uint8_t r, uint8_t g, uint8_t b){//desenha ampulheta
  uint8_t leds_addrs[]={24,23,22,21,20,18,16,12,6,8,4,3,2,1,0};
  for(int i=0; i<15;i++){
    npSetLED(leds_addrs[i],r,g,b);
  }
}


uint8_t rand_sign(){ // função para pegar valor aleatório entre 0 e 7
	uint8_t num = get_rand_32() % 10;
	while (num >= 8){
		num = get_rand_32() % 10;
	}
	return num;
}

void npDrawArrow(uint8_t placa){
	npClear();
	npSetArrow(placa);
	npWrite();
}

void npDrawAmpulheta(uint8_t r, uint8_t g, uint8_t b){
	npClear();
	npSetWait(r, g, b);
	npWrite();
}