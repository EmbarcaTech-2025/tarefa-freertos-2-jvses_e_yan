#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "include/oled_ctrl.h"
#include "include/exam.h"

TaskHandle_t THButtons = NULL;
TaskHandle_t THOled = NULL;
TaskHandle_t THLedMatrix = NULL;
TaskHandle_t THJoystick = NULL;
TaskHandle_t THGameProc = NULL;

SemaphoreHandle_t SemJoyCount = NULL;
static QueueHandle_t xQueue_btn_proc = NULL;
static QueueHandle_t xQueue_joystick = NULL;
static QueueHandle_t xQueue_led_matrix = NULL;
static QueueHandle_t xQueue_oled = NULL;

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

typedef struct {
    uint16_t vrx;
	uint16_t vry;
    uint32_t elapsed_ms;
} JoystickData_t;

uint32_t get_current_ms() {
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

void task_proc_game(void *params){
	bool recValueA, recValueB;
	uint8_t flagRecebida;
	bool first_wait_after_start = true;
	bool sign_change = false;
	uint8_t dir_atual = 8;
	int8_t nivel = 1;
	uint16_t tempo_de_espera = 2000;
	int8_t contador_turnos = 0;
	float tempo_turnos[TURN_LIMIT] = {0};
	uint32_t time_count_start = 0;
	bool novo_tempo = false;
	JoystickData_t recJoystick;
	PioData_t sendPIO = {rand_sign(), false, false};

	OledData_t start_msg = {.nivel = nivel, .adrx = 3, .start_addry = 0, .contador_turnos = 0};
	xQueueSend(xQueue_oled, &start_msg, 0);
	vTaskDelay(pdMS_TO_TICKS(2000));

	while (1){
		if (xQueueReceive(xQueue_btn_proc, &flagRecebida, pdMS_TO_TICKS(10)) == pdTRUE){
			recValueA = (flagRecebida >> 0) & 0x01;
			recValueB = (flagRecebida >> 1) & 0x01;
		}

		if(recValueA){
			if(first_wait_after_start){
				sendPIO.b_hg = true;
				xQueueSend(xQueue_led_matrix, &sendPIO, 0U);
				vTaskDelay(pdMS_TO_TICKS(tempo_de_espera));
				sendPIO.b_hg = false;
				first_wait_after_start = false;
			}

			if(!sign_change){
				xQueueSend(xQueue_led_matrix, &sendPIO, 0U);
				if(time_count_start == 0){
					time_count_start = get_current_ms();
				}
				xSemaphoreGive(SemJoyCount);

				if(xQueueReceive(xQueue_joystick, &recJoystick, pdMS_TO_TICKS(10)) == pdTRUE){
					dir_atual = joy_arrow(recJoystick.vrx, recJoystick.vry);
				}

				if(dir_atual == sendPIO.direcao && !sign_change){
					tempo_turnos[contador_turnos] = (float)(recJoystick.elapsed_ms - time_count_start) / 1000;
					time_count_start = 0;
					sign_change = true;
				}
			} else {
				contador_turnos++;
				novo_tempo = true;
				sendPIO.direcao = rand_sign();
				if (contador_turnos > TURN_LIMIT) contador_turnos = 0;
				sendPIO.b_hg = true;
				xQueueSend(xQueue_led_matrix, &sendPIO, 0U);
				vTaskDelay(pdMS_TO_TICKS(tempo_de_espera));
				sendPIO.b_hg = false;
				xQueueSend(xQueue_led_matrix, &sendPIO, 0U);
				sign_change = false;
			}

			if (novo_tempo){
				if (contador_turnos == 5){
					OledData_t msg;
					msg.nivel = nivel;
					msg.adrx = 5;
					msg.start_addry = 8;
					msg.contador_turnos = 5;
					for (int i = 0; i < 5; i++) msg.tempo_turnos[i] = tempo_turnos[i];
					xQueueSend(xQueue_oled, &msg, 0);
					contador_turnos = 0;
					sign_change = true;
					sendPIO.y_hg = true;
					xQueueSend(xQueue_led_matrix, &sendPIO, 0);
					vTaskDelay(pdMS_TO_TICKS(100));
					recValueA = false;
				}
				novo_tempo = false;
			}
		} else {
			sendPIO.y_hg = false;
			sendPIO.b_hg = false;
			for (uint8_t i = 0; i < 5; i++) tempo_turnos[i] = 0;
			sign_change = false;
			first_wait_after_start = true;
		}

		if(recValueB){
			update_level_rtos(&nivel, &tempo_de_espera);
			OledData_t lvl_msg = {.nivel = nivel, .adrx = 3, .start_addry = 0, .contador_turnos = 0};
			xQueueSend(xQueue_oled, &lvl_msg, 0);
			recValueB = false;
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

void task_joystick(void *params){
	setup_joystick();
	while (1){
		if(xSemaphoreTake(SemJoyCount, pdMS_TO_TICKS(1)) == pdTRUE){
			JoystickData_t sendJoyData;
			joystick_read_axis(&sendJoyData.vrx, &sendJoyData.vry);
			sendJoyData.elapsed_ms = get_current_ms();
			xQueueSend(xQueue_joystick, &sendJoyData, 0U);
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void task_ledMatrix(void *params){
	npInit(LED_NEOPIN);
	npClear();
	npWrite();
	PioData_t pio_data;
	while (1){
		if (xQueueReceive(xQueue_led_matrix, &pio_data, pdMS_TO_TICKS(1)) == pdTRUE) {
			if (pio_data.y_hg) {
				npDrawAmpulheta(LOW_BRIGHT, LOW_BRIGHT, 0);
			} else if (pio_data.b_hg) {
				npDrawAmpulheta(0, 0, LOW_BRIGHT);
			} else {
				npDrawArrow(pio_data.direcao);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void task_oled(void *params){
	setup_OLED();
	OledData_t oled_data;
	while (true){
		if (xQueueReceive(xQueue_oled, &oled_data, pdMS_TO_TICKS(10)) == pdTRUE) {
			oled_clear();
			if (oled_data.contador_turnos > 0) {
				oled_times_print(oled_data.nivel, oled_data.contador_turnos, oled_data.tempo_turnos, oled_data.start_addry);
			} else {
				oled_msg_print_nivel(oled_data.nivel);
			}
			oled_render();
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void task_buttons(void *params){
	setup_buttons();
	bool lockA = false, lockB = false;
	while (true){
		bool btA = !gpio_get(BUTTON_A);
		bool btB = !gpio_get(BUTTON_B);
		uint8_t flags = 0;
		if(!lockA && btA) flags |= (true << 0);
		if(!lockB && btB) flags |= (true << 1);
		lockA = btA;
		lockB = btB;
		if (flags != 0){
			if(xQueueSend(xQueue_btn_proc, &flags, pdMS_TO_TICKS(10)) != pdPASS){
				printf("Falha ao enviar botões\n");
			}
		}
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

int main(){
	stdio_init_all();
	xQueue_btn_proc = xQueueCreate(2, sizeof(uint8_t));
	xQueue_joystick = xQueueCreate(1, sizeof(JoystickData_t));
	xQueue_led_matrix = xQueueCreate(2, sizeof(PioData_t));
	xQueue_oled = xQueueCreate(5, sizeof(OledData_t));
	SemJoyCount = xSemaphoreCreateBinary();

	xTaskCreate(task_buttons, "T_botões", 128, NULL, 3, &THButtons);
	xTaskCreate(task_proc_game, "Task_Exame", 256, NULL, 2, &THGameProc);
	xTaskCreate(task_joystick, "Task_Joystick", 256, NULL, 1, &THJoystick);
	xTaskCreate(task_ledMatrix, "Task Neopixel", 128, NULL, 1, &THLedMatrix);
	xTaskCreate(task_oled, "Task_OLED", 256, NULL, 1, &THOled);

	vTaskStartScheduler();
	while (true) {}
}