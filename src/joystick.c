#include "include/joystick.h"

volatile uint16_t dma_buffer[2];  // [X, Y]
int dma_chan;
dma_channel_config dma_cfg;

// Função para configurar o joystick (pinos de leitura, ADC e interrupção)
void setup_joystick(){
// Inicializa o ADC e os pinos de entrada analógica
adc_init();         // Inicializa o módulo ADC
adc_gpio_init(VRX); // Configura o pino VRX (eixo X) para entrada ADC
adc_gpio_init(VRY); // Configura o pino VRY (eixo Y) para entrada ADC
// Configura a interrupção do ADC
adc_select_input(ADC_CHANNEL_0); // Seleciona ADC0 inicialmente é necessário selecionar par configurar o round robin
    
// Configura FIFO do ADC
adc_fifo_setup(
    true,    // Habilitar FIFO
    true,    // Habilitar DMA
    1,       // Limiar de DREQ
    false,   // Não saturar
    false     // Modo circular
);
    
adc_set_round_robin((1 << ADC_CHANNEL_0) | (1 << ADC_CHANNEL_1));
adc_set_clkdiv(ADC_CLOCK_DIV);// ativa divisor de clk para não exigir tanto do adc
// adc_set_round_robin(0);

// Configuração DMA
dma_chan = dma_claim_unused_channel(true);
dma_cfg = dma_channel_get_default_config(dma_chan);

channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
channel_config_set_read_increment(&dma_cfg, false);
channel_config_set_write_increment(&dma_cfg, true);
channel_config_set_dreq(&dma_cfg, DREQ_ADC);
channel_config_set_ring(&dma_cfg, false, 2);  // Tamanho do anel



// // Configura interrupção
// dma_channel_set_irq0_enabled(dma_chan, true);
// irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
// irq_set_enabled(DMA_IRQ_0, true);

// Inicia conversões
adc_run(false);

// // Inicializa o pino do botão do joystick
// gpio_init(SW);             // Inicializa o pino do botão
// gpio_set_dir(SW, GPIO_IN); // Configura o pino do botão como entrada
// gpio_pull_up(SW);          // Ativa o pull-up no pino do botão para evitar flutuações
}

// void dma_handler() {
//     // Reconhece a interrupção
//     dma_hw->ints0 = (1u << dma_chan);
//     // Processa dados aqui se necessário
//     dma_channel_start(dma_chan);  // Reinicia transferência
// }


// Função para ler os valores dos eixos do joystick (X e Y)
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value){//volatile uint16_t *vrx_value, volatile uint16_t *vry_value){

  adc_fifo_drain(); // Limpa o FIFO do ADC.
  adc_run(false); // Desliga o ADC (se estiver ligado) para configurar o DMA.

  dma_channel_configure(
      dma_chan,
      &dma_cfg,
      dma_buffer,          // Destino
      &adc_hw->fifo,       // Origem
      2,                   // Transfer count (X e Y)
      false                 // Iniciar imediatamente
  );

  adc_run(true);

  dma_channel_start(dma_chan);

  while (dma_channel_is_busy(dma_chan)){}//trava aqui enquanto o dma estiver ocupado

  adc_run(false);

  *vrx_value = dma_buffer[0];
  *vry_value = dma_buffer[1];

  // // Leitura do valor do eixo X do joystick
  // // printf("Vou ler o X\n");
  // adc_select_input(ADC_CHANNEL_0); // Seleciona o canal ADC para o eixo X
  // // sleep_us(2);                     // Pequeno delay para estabilidade
  // *vrx_value = adc_read();         // Lê o valor do eixo X (0-4095)
  // // printf("Li o X\n");

  // // Leitura do valor do eixo Y do joystick
  // adc_select_input(ADC_CHANNEL_1); // Seleciona o canal ADC para o eixo Y
  // // sleep_us(2);                     // Pequeno delay para estabilidade
  // *vry_value = adc_read();         // Lê o valor do eixo Y (0-4095)
  // // printf("Li o Y\n");
  // printf(" X:%d e Y:%d\n",*vrx_value,*vry_value);
}

uint8_t joy_arrow(uint16_t x_val, uint16_t y_val){
  //menor que 1000, está baixo
  //entre 1000 e 3000 está no meio
  // maior que 3000 está alto
  if(y_val >= THRESHOLD_ADC_LOW && y_val<= THRESHOLD_ADC_HIGH && x_val >= THRESHOLD_ADC_HIGH ){
    return 0;//aponta para norte
  }else if(x_val>=THRESHOLD_ADC_HIGH && y_val>=THRESHOLD_ADC_HIGH){
    return 1; //nordeste
  }else if(y_val>= THRESHOLD_ADC_HIGH && x_val >= THRESHOLD_ADC_LOW && x_val <=THRESHOLD_ADC_HIGH){
    return 2; //Leste
  }else if(x_val<=THRESHOLD_ADC_LOW && y_val>= THRESHOLD_ADC_HIGH){
    return 3; //Suldeste
  }else if(y_val >= THRESHOLD_ADC_LOW && y_val<= THRESHOLD_ADC_HIGH && x_val <= THRESHOLD_ADC_LOW){
    return 4; //Sul
  }else if(x_val<=THRESHOLD_ADC_LOW && y_val<=THRESHOLD_ADC_LOW){
    return 5; //suldoeste
  }else if(y_val<=THRESHOLD_ADC_LOW && x_val >= THRESHOLD_ADC_LOW && x_val<= THRESHOLD_ADC_HIGH){
    return 6; //Oeste
  }else if( x_val>=THRESHOLD_ADC_HIGH && y_val<=THRESHOLD_ADC_LOW){
    return 7; //Noroeste
  }else{ return 8; }

}