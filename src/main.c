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
static QueueHandle_t xQueue_pio = NULL;
static QueueHandle_t xQueue_joystick = NULL;

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
	bool recValueA,recValueB;//flags do exame
	uint8_t flagRecebida;//recebimento dos botões

	uint8_t sign_placa = rand_sign();//já começa com um valor aleatório

	bool sign_change = false;
	uint8_t dir_atual = {8};//direção percebida. Inicia em 8



	uint8_t nivel={1};//nível padrão do jogo iniciado em 1
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

		if(recValueA){//inicia exame

			/*Checa se as tasks necessárias estão ativas ou não*/
			// if (eTaskGetState(THJoystick) == eSuspended){
			// 	vTaskResume(THJoystick);
			// }

			// xQueueSend(xQueue_pio,&sendPIO,0U);//envia informações do PIO
			

			
			
			
			
			if(time_count_start == 0){//se for a primeira interação ele pega o tempo atual
				time_count_start = get_current_ms();
			}

			xSemaphoreGive(SemJoyCount);//permitiu a execução da task do Joystick

			if(xQueueReceive(xQueue_joystick,&recJoystick,pdMS_TO_TICKS(10)) == pdTRUE ){//fica a espera da atualização do Joystick
				printf("RECEBIDO: VRX:%d |VRY:%d , time:%d\n",recJoystick.vrx,recJoystick.vry,recJoystick.elapsed_ms);
				dir_atual = joy_arrow(recJoystick.vrx,recJoystick.vry);//atualiza a direção do joystick nessa task
			}
			if(dir_atual == sendPIO.direcao){//se está na mesma direção que a MatrizLeds então acertou
				tempo_turnos[contador_turnos] = (float)( (time_count_start - recJoystick.elapsed_ms)/1000 );//coloca tempo do turno no array
				contador_turnos++;//vai para próximo turno
				time_count_start = 0; //reseta contador de tempo por turno
				sendPIO.direcao=rand_sign();//sorteia novo valor para a direção
			}

			// vTaskDelay(pdMS_TO_TICKS(100));//espera para visualização no serial


			

		} else{//se o exame não estiver acontecendo

		}

		if(recValueB){//altera on nível do jogo
			switch (nivel){
				case 1:
					nivel = 2;
					tempo_de_espera = 1200; // agora é nível 2
					break;
				case 2:
					nivel = 3;
					tempo_de_espera = 800;
					break;
				case 3:
					nivel = 4;
					tempo_de_espera = 400;
					break;
				case 4:
					nivel = 1;
					tempo_de_espera = 2000;
					break;
				default:
					nivel = 1;
					tempo_de_espera = 2000;
					break;
			}
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

	while (1){//espera mudanças para desenhar na matriz de led
		/* code */
	}
	
}

void task_oled(void *params){// é provocado pelos botões e pelo Joystick para atualizar a tela
	setup_OLED();

	while (true){//deve ficar em standby esperando informações novas para serem escritas
		/* code */
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
	xTaskCreate(task_joystick,"Task_Joystick",128,NULL,1,&THJoystick);

	// vTaskSuspend(THJoystick);//suspende Task que não são necessárias ainda

	vTaskStartScheduler();
	

	while (true){
		// exam_handler();
	}
}
