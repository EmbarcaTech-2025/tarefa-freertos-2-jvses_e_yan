#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "include/neopin.h"
#include "include/oled_ctrl.h"
#include "include/exam.h"

TaskHandle_t Buttons = NULL;
TaskHandle_t Oled = NULL;
TaskHandle_t LedMatrix = NULL;
TaskHandle_t Joystick = NULL;
TaskHandle_t Debug = NULL;

QueueHandle_t xQueue_led_matrix;
QueueHandle_t xQueue_oled;

typedef struct {
	uint8_t direcao;
	bool b_hg;
	bool y_hg;
} PioData_t;

typedef struct{
	int8_t nivel;
	uint8_t adrx;
	uint8_t start_addry;
	int8_t contador_turnos;
	float tempo_turnos[5];
} OledData_t;

void task_proc_game(){}

void task_joystick(void *params){
	setup_joystick();
	while (1){ }
}

void task_ledMatrix(void *params){
	npInit(LED_NEOPIN);
	PioData_t pio_data;

	while (1){
		if (xQueueReceive(xQueue_led_matrix, &pio_data, pdMS_TO_TICKS(10)) == pdTRUE) {
			npClear();
			if (pio_data.b_hg) {
				npDrawAmpulheta(0, 0, HIG_BRIGHT);
			} else if (pio_data.y_hg) {
				npDrawAmpulheta(HIG_BRIGHT, HIG_BRIGHT, 0);
			} else {
				npDrawArrow(pio_data.direcao);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void task_oled(void *params){
	setup_OLED();
	OledData_t oled_data;

	while (true){
		if (xQueueReceive(xQueue_oled, &oled_data, pdMS_TO_TICKS(10)) == pdTRUE) {
			oled_clear();
			oled_times_print(oled_data.nivel, oled_data.contador_turnos, oled_data.tempo_turnos, oled_data.start_addry);
			oled_render();
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void task_debug(void *params) {
	while (1) {
		PioData_t test_led = {.direcao = 2, .b_hg = false, .y_hg = false};
		xQueueSend(xQueue_led_matrix, &test_led, 0);
		vTaskDelay(pdMS_TO_TICKS(1000));

		test_led.b_hg = true;
		test_led.y_hg = false;
		xQueueSend(xQueue_led_matrix, &test_led, 0);
		vTaskDelay(pdMS_TO_TICKS(1000));

		OledData_t test_oled = {
			.nivel = 1,
			.adrx = 5,
			.start_addry = 8,
			.contador_turnos = 2,
			.tempo_turnos = {1.234f, 2.345f}
		};
		xQueueSend(xQueue_oled, &test_oled, 0);
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void task_buttons(void *params){
	setup_buttons();
	bool lockA = false, lockB = false;

	while (true){
		bool btA = !gpio_get(BUTTON_A);
		bool btB = !gpio_get(BUTTON_B);

		if (!lockA && btA){
			lockA = true;
			vTaskDelay(pdMS_TO_TICKS(200));
		} else{
			lockA = false;
		}
		if (!lockB && btB){
			lockB = true;
			vTaskDelay(pdMS_TO_TICKS(200));
		} else{
			lockB = false;
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
	xTaskCreate(task_debug, "Debug", 1024, NULL, 1, &Debug);

	vTaskStartScheduler();
	while (true) {}
}
