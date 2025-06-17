#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "include/neopin.h"
#include "include/oled_ctrl.h"
#include "include/exam.h"
extern uint8_t ssd[];

TaskHandle_t Buttons = NULL;
TaskHandle_t Oled = NULL;
TaskHandle_t LedMatrix = NULL;
TaskHandle_t Joystick = NULL;

QueueHandle_t xQueue_led_matrix;
QueueHandle_t xQueue_oled;

typedef struct {
	uint8_t direcao;
	bool b_hg;
	bool y_hg;
} PioData_t;

typedef struct{
	char str[16];
	uint8_t adrx;
	uint8_t adry;
} OledData_t;

//preparo de tasks

//task para PIO
// task para OLED 
// task para botões
// task para joystick

// task de processamento e adiministração. Ela vai receber as informações das outras e enviar 
// o que deve ser printado no OLED e no PIO (mantém a task de botões só com botões)



void task_proc_game(){//trabalha quando o exame começa e adminitra os periféricos

}

void task_joystick(void *params){//quando ativada ela envia a direção que o joystick está apontando e o tempo passado
	setup_joystick();
	
	while (1){
		/* code */
	}
	
}

void task_ledMatrix(void *params){
	npInit(LED_NEOPIN);
	PioData_t pio_data;

	while (1){
		if (xQueueReceive(xQueue_led_matrix, &pio_data, portMAX_DELAY) == pdTRUE) {
			npClear();
			if (pio_data.b_hg) {
				npDrawAmpulheta(0, 0, HIG_BRIGHT);
			} else if (pio_data.y_hg) {
				npDrawAmpulheta(HIG_BRIGHT, HIG_BRIGHT, 0);
			} else {
				npDrawArrow(pio_data.direcao);
			}
		}
	}
}

void task_oled(void *params){
	setup_OLED();
	OledData_t oled_data;

	while (true){
		if (xQueueReceive(xQueue_oled, &oled_data, portMAX_DELAY) == pdTRUE) {
			oled_clear();
			ssd1306_draw_string(ssd, oled_data.adrx, oled_data.adry, oled_data.str);
			oled_render();
		}
	}
}

void task_buttons(void *params){
	//botões iniciados em PULL-UP
	setup_buttons();//readaptados para não ter mais interrupções

	bool lockA={false},lockB={false}; //var auxiliar para softlock

	while (true){
		bool btA=!gpio_get(BUTTON_A);
		bool btB=!gpio_get(BUTTON_B);

		if(!lockA && btA){//se a trava não estiver ativa e A for apertado então faz os eventos
			lockA=true;
			
			/*começa task do exame aqui*/

			vTaskDelay(pdMS_TO_TICKS(200));//debounce de 200ms para o botão
		} else{
			lockA=false;
		}


		if(!lockB && btB){//se a trava não estiver ativa e A for apertado então faz os eventos
			lockB=true;
			
			/*altera nível do exame aqui (Provavelmente de pausar a execução do exame. caso esteja acontecendo)*/

			vTaskDelay(pdMS_TO_TICKS(200));//debounce de 200ms para o botão
		} else{
			lockB=false;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
}


int main(){
	stdio_init_all();

	xQueue_led_matrix = xQueueCreate(5, sizeof(PioData_t));
	xQueue_oled = xQueueCreate(5, sizeof(OledData_t));

	xTaskCreate(task_ledMatrix, "LedMatrix", 1024, NULL, 1, &LedMatrix);
	xTaskCreate(task_oled, "Oled", 1024, NULL, 1, &Oled);

	vTaskStartScheduler();
	// exam_setup();

	// sleep_ms(2000);

	// struct repeating_timer timer;
	// add_repeating_timer_ms(10, repeating_reader, NULL, &timer);

	while (true){
		// exam_handler();
	}
}
