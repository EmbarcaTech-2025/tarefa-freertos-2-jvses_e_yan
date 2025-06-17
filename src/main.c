#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "include/exam.h"

TaskHandle_t THButtons = NULL;
TaskHandle_t THOled = NULL;
TaskHandle_t THLedMatrix = NULL;
TaskHandle_t THJoystick = NULL;
TaskHandle_t THGameProc = NULL;

SemaphoreHandle_t SemJoyCount = NULL;//Semáforo para adiministrar jopystick


static QueueHandle_t xQueue_btn_proc = NULL; //ponte de comunicação task botões-proc
// static QueueHandle_t xQueue_pio = NULL;
static QueueHandle_t xQueue_joystick = NULL;

static QueueHandle_t xQueue_led_matrix = NULL;
static QueueHandle_t xQueue_oled = NULL;

//preparo de tasks

//task para PIO
// task para OLED 
// task para botões
// task para joystick

// task de processamento e adiministração. Ela vai receber as informações das outras e enviar 
// o que deve ser printado no OLED e no PIO (mantém a task de botões só com botões)

typedef struct {
    uint16_t vrx;
	uint16_t vry;
    uint32_t elapsed_ms;
} JoystickData_t;

typedef struct {
	uint8_t direcao;
	bool b_hg;//ampulheta azul(para o final do exame)
	bool y_hg;//ampulheta amarela (para intervalos antes da mudança de direção)
} PioData_t;

typedef struct{
	char str[16];
	uint8_t adrx;//referencia x para escrever
	uint8_t adry;//referencia y para escrever
} OledData_t;



// Função para obter tempo atual em ms (wrapper para ticks do FreeRTOS)
uint32_t get_current_ms() {
    return pdTICKS_TO_MS(xTaskGetTickCount());
}


void task_proc_game(void *params){//trabalha quando o exame começa e adminitra os periféricos

	printf("O exame foi iniciado\n");
	bool recValueA,recValueB;//flags do exame valueA equivale a exam_started
	uint8_t flagRecebida;//recebimento dos botões

	uint8_t sign_placa = rand_sign();//já começa com um valor aleatório

	bool sign_change = false;//variável que indica se tem mudança na palca exibida
	uint8_t dir_atual = {8};//direção percebida. Inicia em 8



	int8_t nivel={1};//nível padrão do jogo iniciado em 1
	uint16_t tempo_de_espera = {2000};//tempo padrão para mudança da placa iniciado em 2s
	int8_t contador_turnos = {0};

	float tempo_medio;
	float tempo_turnos[TURN_LIMIT] = {0};
	uint32_t time_count_start = {0};
	bool novo_tempo = false;	 // avisa se atualizou o contador

	int64_t respose_time = {0}; // tempo de resposta
	absolute_time_t time_start_counting;//tempo para contagem

	PioData_t sendPIO = {sign_placa, false, false};// inicializa a primeira direção para o PIO
	JoystickData_t recJoystick;



	while (1){
		// printf("Comecei o loop do Proc \n");

		if (xQueueReceive(xQueue_btn_proc,&flagRecebida, pdMS_TO_TICKS(10) ) == pdTRUE){// checa se recebeu informação dos botões
			recValueA = (flagRecebida >> 0) & 0x01; //recebe o bit 0
			recValueB = (flagRecebida >> 1) & 0x01; // recebe o bit 1  
			printf("btnA: %d , btnB: %d\n",recValueA,recValueB);
		}

		if(recValueA){//Se o exame foi iniciado

			/*Checa se as tasks necessárias estão ativas ou não*/
			// if (eTaskGetState(THJoystick) == eSuspended){
			// 	vTaskResume(THJoystick);
			// }

			// xQueueSend(xQueue_pio,&sendPIO,0U);//envia informações para o PIO desenhar o que precisa
			

			if(!sign_change){
				/*mexe no PIO para colocar novo valor*/

				/*trata a leitura do joystick*/ //vai ler o joystick enquanto a placa exibida não mudar
				////////////////////////
				if(time_count_start == 0){//se o tempo atual antes de checar o joystick
					time_count_start = get_current_ms();
				}

				xSemaphoreGive(SemJoyCount);//permitiu a execução da task do Joystick

				if(xQueueReceive(xQueue_joystick,&recJoystick,pdMS_TO_TICKS(10)) == pdTRUE ){//fica a espera da atualização do Joystick
					printf("RECEBIDO: VRX:%d |VRY:%d , time:%d\n",recJoystick.vrx,recJoystick.vry,recJoystick.elapsed_ms);
					dir_atual = joy_arrow(recJoystick.vrx,recJoystick.vry);//atualiza a direção do joystick nessa task
				}
				if(dir_atual == sendPIO.direcao && !sign_change){//se está na mesma direção que a MatrizLeds e não houve mudança prévia de direção então acertou
					tempo_turnos[contador_turnos] = (float)( (time_count_start - recJoystick.elapsed_ms)/1000 );//coloca tempo do turno no array
					contador_turnos++;//vai para próximo turno
					time_count_start = 0; //reseta contador de tempo por turno
					sendPIO.direcao=rand_sign();//sorteia novo valor para a direção
					sign_change=true;
					printf("Atingiu a mesma direção da Placa\n");
					printf("Tempo do turno: %.3f\n")
				}
				////////////////////////
			} else{//se houve mudança de sinal




			
			}// vTaskDelay(pdMS_TO_TICKS(100));//espera para visualização no serial


			

		} else{//se o exame não estiver acontecendo então vamos resetar algumas variáveis

		}

		if(recValueB){//altera on nível do jogo
			update_level_rtos(&nivel,&tempo_de_espera);

			/*Enviar novo nível para OLED*/
			printf("Alterado o nível:%d e tempo:%dms \n",nivel,tempo_de_espera);
			recValueB=false;
		}
		// printf("Ainda estou no Proc \n");
		vTaskDelay(pdMS_TO_TICKS(50));//espera de 50ms
		
	}
	

}

void task_joystick(void *params){//quando ativada ela envia a direção que o joystick está apontando e o tempo passado
	setup_joystick();
	
	while (1){

		if(xSemaphoreTake(SemJoyCount,pdMS_TO_TICKS(1) ==pdTRUE )){
			JoystickData_t sendJoyData;

			joystick_read_axis(&sendJoyData.vrx, &sendJoyData.vry);
			sendJoyData.elapsed_ms=get_current_ms();

			printf("ENVIADO: VRX:%d |VRY:%d , time:%d\n",sendJoyData.vrx,sendJoyData.vry,sendJoyData.elapsed_ms);

			xQueueSend(xQueue_joystick,&sendJoyData,0U);//envia para o queue as informações do joystick
		} else{
			//timeout não faço nada
			// printf("Time out Joystick send\n");
		}
		vTaskDelay(pdMS_TO_TICKS(50));

		/* code */
	}
	
}

void task_ledMatrix(void *params){//atualiza a matriz de Leds
	npInit(LED_NEOPIN);
	PioData_t pio_data;

	while (1){
		if (xQueueReceive(xQueue_led_matrix, &pio_data, pdMS_TO_TICKS(1)) == pdTRUE) {
			// npClear();
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

void task_oled(void *params){// é provocado pelos botões e pelo Joystick para atualizar a tela
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
	
	
	xQueue_btn_proc = xQueueCreate(1,sizeof(uint8_t));//vou compactar as flags dos botões nos valores do byte


	while (true){
		bool btA=!gpio_get(BUTTON_A);
		bool btB=!gpio_get(BUTTON_B);

		uint8_t flags={0};//falso ao ser cirada. Isso poupa resets para ela

		if(!lockA && btA){//se a trava não estiver ativa e A for apertado então faz os eventos
			// lockA=true;
			flags |= (true << 0);
			// vTaskDelay(pdMS_TO_TICKS(200));//debounce de 200ms para o botão
		}

		if(!lockB && btB){//se a trava não estiver ativa e A for apertado então faz os eventos
			// lockB=true;
			flags |= (true << 1);
			// vTaskDelay(pdMS_TO_TICKS(200));//debounce de 200ms para o botão
		} 

		lockA = btA;
		lockB = btB;

		if (flags != 0 ){//se tiver conteúdo é pra enviar para o task_procGame
			if(xQueueSend(xQueue_btn_proc,&flags,pdMS_TO_TICKS(10)) != pdPASS ){//envia de imediato
				printf("Falha ao enviar botões\n");
			}
		} 
		// printf("estou no loop a botões\n");
		
		vTaskDelay(pdMS_TO_TICKS(200));//debounce.
	}
	
}


int main(){
	stdio_init_all();

	
	xQueue_pio = xQueueCreate(1,sizeof(PioData_t));//
	xQueue_joystick = xQueueCreate(1,sizeof(JoystickData_t));

	SemJoyCount = xSemaphoreCreateBinary();



	xTaskCreate(task_buttons,"T_botões",128,NULL,3,&THButtons);
	xTaskCreate(task_proc_game,"Task_Exame",256,NULL,2,&THGameProc);
	xTaskCreate(task_joystick,"Task_Joystick",256,NULL,1,&THJoystick);

	// vTaskSuspend(THJoystick);//suspende Task que não são necessárias ainda

	vTaskStartScheduler();
	

	while (true){
		// exam_handler();
	}
}
