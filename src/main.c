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
	int8_t nivel;
	uint8_t adrx;//referencia x para escrever
	uint8_t start_addry;//referencia y para escrever
	int8_t contador_turnos;
	float tempo_turnos[5];
} OledData_t;



// Função para obter tempo atual em ms (wrapper para ticks do FreeRTOS)
uint32_t get_current_ms() {
    return pdTICKS_TO_MS(xTaskGetTickCount());
}


void task_proc_game(void *params){//trabalha quando o exame começa e adminitra os periféricos

	printf("O exame foi iniciado\n");
	bool recValueA,recValueB;//flags do exame valueA equivale a exam_started
	uint8_t flagRecebida;//recebimento dos botões

	// uint8_t sign_placa = rand_sign();//já começa com um valor aleatório
	bool first_wait_after_start=true;

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

	PioData_t sendPIO = {rand_sign(), false, false};// inicializa a primeira direção para o PIO
	JoystickData_t recJoystick;



	while (1){
		// printf("Comecei o loop do Proc \n");

		if (xQueueReceive(xQueue_btn_proc,&flagRecebida, pdMS_TO_TICKS(10) ) == pdTRUE){// checa se recebeu informação dos botões
			recValueA = (flagRecebida >> 0) & 0x01; //recebe o bit 0
			recValueB = (flagRecebida >> 1) & 0x01; // recebe o bit 1  
			printf("btnA: %d , btnB: %d\n",recValueA,recValueB);
		}

		if(recValueA){//Se o exame foi iniciado
			if(first_wait_after_start){
				sendPIO.b_hg=true;
				xQueueSend(xQueue_led_matrix,&sendPIO,0U);//coloca ampulheta azul na matriz
				vTaskDelay(pdMS_TO_TICKS(tempo_de_espera));
				sendPIO.b_hg=false;
				first_wait_after_start=false;
			}

			

			if(!sign_change){
				/*mexe no PIO para colocar novo valor*/
				xQueueSend(xQueue_led_matrix,&sendPIO,0U);//envia informações para o PIO desenhar o que precisa
				
				/*trata a leitura do joystick*/ //vai ler o joystick enquanto a placa exibida não mudar
				////////////////////////
				if(time_count_start == 0){//se o tempo atual antes de checar o joystick
					time_count_start = get_current_ms();
				}

				xSemaphoreGive(SemJoyCount);//permitiu a execução da task do Joystick

				if(xQueueReceive(xQueue_joystick,&recJoystick,pdMS_TO_TICKS(10)) == pdTRUE ){//fica a espera da atualização do Joystick
					// printf("RECEBIDO: VRX:%d |VRY:%d , time:%d\n",recJoystick.vrx,recJoystick.vry,recJoystick.elapsed_ms);
					dir_atual = joy_arrow(recJoystick.vrx,recJoystick.vry);//atualiza a direção do joystick nessa task
				}
				if(dir_atual == sendPIO.direcao && !sign_change){//se está na mesma direção que a MatrizLeds e não houve mudança prévia de direção então acertou
					printf("Atingiu a mesma direção da Placa\n");
					tempo_turnos[contador_turnos] = (float)(recJoystick.elapsed_ms - time_count_start) / 1000 ;//coloca tempo do turno no array
					printf("Tempo - tempo até acertar(%d - %d) do turno: %.3f\n",time_count_start,recJoystick.elapsed_ms, tempo_turnos[contador_turnos]);
					time_count_start = 0; //reseta contador de tempo por turno

					sign_change=true;
					
				}
				////////////////////////
			} else {//se houve mudança de sinal

				/*atualiza OLED*/

				printf("T[%d] pego\nVamos ao próximo\n",contador_turnos);
				contador_turnos++;//vai para próximo turno
				novo_tempo = true;
				sendPIO.direcao=rand_sign();//sorteia novo valor para a direção
				if (contador_turnos > TURN_LIMIT){
                	contador_turnos = 0;
            	}
				sendPIO.b_hg=true;
				xQueueSend(xQueue_led_matrix,&sendPIO,0U);//coloca ampulheta azul na matriz
				vTaskDelay(pdMS_TO_TICKS(tempo_de_espera));
				sendPIO.b_hg=false;
				xQueueSend(xQueue_led_matrix,&sendPIO,0U);//tira ampulheta azuul da matriz
				sign_change = false;

			
			}// vTaskDelay(pdMS_TO_TICKS(100));//espera para visualização no serial
			if (novo_tempo){//aqui é atualizado o OLED e terminado o exame
        		if (contador_turnos == 5){
					printf("Acabou o exame !!!!\n");
					for(uint8_t i=0; i<5;i++){
						printf("Tempo[%d]= %.3f\n",i,tempo_turnos[i]);
					}
					contador_turnos = 0;

					sign_change = true;
					sendPIO.y_hg=true;

					xQueueSend(xQueue_led_matrix,&sendPIO,0U);
					vTaskDelay(pdMS_TO_TICKS(100));
					// npDrawAmpulheta(LOW_BRIGHT, LOW_BRIGHT, 0);
					// sleep_ms(100);

					recValueA = false;
				}
				novo_tempo = false;
    		}

		} else{//se o exame não estiver acontecendo então vamos resetar algumas variáveis
			// contador_turnos = 0;//se exame acabar então ele reseta o contador de turnos 
			sendPIO.y_hg=false;
			sendPIO.b_hg=false;
			for(uint8_t i=0; i<5;i++){//reseta os tempos do buffer
				tempo_turnos[i]=0;
			}
			sign_change = false;
			first_wait_after_start=true;

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

			// printf("ENVIADO: VRX:%d |VRY:%d , time:%d\n",sendJoyData.vrx,sendJoyData.vry,sendJoyData.elapsed_ms);

			xQueueSend(xQueue_joystick,&sendJoyData,0U);//envia para o queue as informações do joystick
		} else{
			//timeout não faço nada
			// printf("Time out Joystick send\n");
		}
		vTaskDelay(pdMS_TO_TICKS(1));

		/* code */
	}
	
}

void task_ledMatrix(void *params){//atualiza a matriz de Leds
	npInit(LED_NEOPIN);
	npClear();
	npWrite();
	PioData_t pio_data;

	while (1){
		if (xQueueReceive(xQueue_led_matrix, &pio_data, pdMS_TO_TICKS(1)) == pdTRUE) {
			// npClear();
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

void task_oled(void *params){// é provocado pelos botões e pelo Joystick para atualizar a tela
	setup_OLED();
	OledData_t oled_data;

	while (true){
		if (xQueueReceive(xQueue_oled, &oled_data, portMAX_DELAY) == pdTRUE) {
			oled_clear();
			// ssd1306_draw_string(ssd, oled_data.adrx, oled_data.adry, oled_data.str);
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

	
	// xQueue_pio = xQueueCreate(1,sizeof(PioData_t));//
	xQueue_joystick = xQueueCreate(1,sizeof(JoystickData_t));

	xQueue_led_matrix = xQueueCreate(2, sizeof(PioData_t));

	xQueue_oled = xQueueCreate(5, sizeof(OledData_t));

	SemJoyCount = xSemaphoreCreateBinary();



	xTaskCreate(task_buttons,"T_botões",128,NULL,3,&THButtons);
	xTaskCreate(task_proc_game,"Task_Exame",256,NULL,2,&THGameProc);
	xTaskCreate(task_joystick,"Task_Joystick",256,NULL,1,&THJoystick);
	xTaskCreate(task_ledMatrix,"Task Neopixel",128,NULL,1,&THLedMatrix);

	// vTaskSuspend(THJoystick);//suspende Task que não são necessárias ainda

	vTaskStartScheduler();
	

	while (true){
		// exam_handler();
	}
}
