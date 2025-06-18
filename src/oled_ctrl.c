#include "include/oled_ctrl.h"


//declaração global do OLED para usar em funções fora da main
uint8_t ssd[ssd1306_buffer_length];
  // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
  struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
  };


void setup_OLED(){
  // Inicialização do i2c
  i2c_init(i2c1, ssd1306_i2c_clock * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  // Processo de inicialização completo do OLED SSD1306
  ssd1306_init();

  calculate_render_area_buffer_length(&frame_area);
}

void oled_clear(){//limpa o buffer do OLED
    memset(ssd, 0, ssd1306_buffer_length);
}

void oled_render(){//rederiza buffer atual do oled na tela
    render_on_display(ssd, &frame_area);
}

void oled_msg_print_nivel(int8_t nivel){
  char buffer[16];
  sprintf(buffer,"Nivel:%d",nivel);
  memset(ssd, 0, ssd1306_buffer_length);//limpa o buffer
  ssd1306_draw_string(ssd,3,0,buffer);//escreve o nível
}

void oled_msg_inicio(int8_t nivel){
  //imprime informações(nível e tempos) no OLED uma primeira vez
  oled_msg_print_nivel(nivel);
  ssd1306_draw_string(ssd,3,8,"aperte o btn A");//Aviso de iníco
//   render_on_display(ssd, &frame_area);
}

void oled_times_print(int8_t nivel,int8_t contador_turnos,float *tempo_turnos,uint8_t start_addry){
  
    oled_msg_print_nivel(nivel);
    uint8_t y=start_addry;

    for(uint8_t i=0; i< contador_turnos ;i++){//for para escrever todos os tempos pegos
        char buffer[16];
        sprintf(buffer,"T%d %.3f",i,tempo_turnos[i]);
        ssd1306_draw_string(ssd,5,y,buffer);
        y+=8;
    }

    // render_on_display(ssd, &frame_area);
}

void oled_print_media(float soma){
    char buffer[16];
    sprintf(buffer,"tm %.3f",soma);
    ssd1306_draw_string(ssd,5,48,buffer);
    // render_on_display(ssd, &frame_area);
}

void oled_print_text_xy(const char *str, uint8_t x, uint8_t y) {
	oled_clear();
	ssd1306_draw_string(ssd, x, y, (char *)str);
	oled_render();
}
