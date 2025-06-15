#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "include/exam.h"

TaskHandle_t Buttons = NULL;
TaskHandle_t Oled = NULL;
TaskHandle_t LedMatrix = NULL;
TaskHandle_t Joystick = NULL;


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
	// exam_setup();

	// sleep_ms(2000);

	// struct repeating_timer timer;
	// add_repeating_timer_ms(10, repeating_reader, NULL, &timer);

	while (true){
		// exam_handler();
	}
}
