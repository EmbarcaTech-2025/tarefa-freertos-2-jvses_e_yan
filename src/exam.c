#include "include/exam.h"

volatile uint16_t vrx_value, vry_value, sw_value; // Variáveis para armazenar os valores do joystick (eixos X e Y) e botão
volatile uint8_t aux = 8;				 // Variável auxiliar que alinha seu valor de acordo para onde o joystick está apontado.
volatile uint8_t sign_placa = {0};
volatile bool sign_change = false;
volatile bool exam_started = false;

volatile uint16_t tempo_de_espera = {2000};

volatile int8_t contador_turnos = {0};
volatile float tempo_medio;
volatile float tempo_turnos[TURN_LIMIT] = {0};

volatile bool novo_tempo = false;	 // avisa se atualizou o contador
volatile int64_t respose_time = {0}; // tempo de resposta
volatile int8_t nivel = {1};

volatile absolute_time_t time_start_counting;
volatile absolute_time_t last_time_presed_a_or_b; //timer para controle de debounce;



void exam_setup(){
    setup_OLED();
	setup_buttons();
	setup_joystick();
	// Inicializa matriz de LEDs NeoPixel.
	npInit(LED_NEOPIN);

    sign_placa = rand_sign(); // pega um valor aleatório para o sinal ne início
	last_time_presed_a_or_b = get_absolute_time();//pega tempo para usar no debounce

    // //imprime informações(nível e tempos) no OLED uma primeira vez
	oled_clear();
	oled_msg_inicio(nivel);
	oled_render();
    npDrawAmpulheta(LOW_BRIGHT, 0, LOW_BRIGHT); // ampulheta
}

bool repeating_reader(struct repeating_timer *t){
	// printf("Vou ler os valores do ADC\n");
	joystick_read_axis(&vrx_value, &vry_value);
	aux = joy_arrow(vrx_value, vry_value);

	// printf("Valores lidos são X:%d e Y:%d\n",vrx_value,vry_value);
	if (sign_placa == aux && sign_change == false){ // vê se o paciente colocou na direção certa

		absolute_time_t now = get_absolute_time();
		respose_time = absolute_time_diff_us(time_start_counting, now);//atualiza o tempo de resposta
		// printf("Acabei de ler os valores são X:%d e Y:%d\n",vrx_value,vry_value);

		sign_placa = rand_sign(); // atualiza novo sinal para placa
		sign_change = true;
	}

	return true;//garante que não vai travar
}

void oled_print_info(){
	// if (novo_tempo && exam_started)	{
		uint8_t y = 8;

		oled_clear();
		// oled_msg_print_nivel(nivel);
		oled_times_print(nivel,contador_turnos, tempo_turnos, 8);

		if (contador_turnos == 5){

			// se for o caso, vai agora exibir a média
			float soma = 0.0f; // Inicializa a soma como 0.0
			char buffer_media[10], msg_med[18] = "tm i ";

			// Soma todos os elementos do array
			for (uint8_t i = 0; i < TURN_LIMIT; i++){
				soma += tempo_turnos[i];
			}

			soma /= 5;
			oled_print_media(soma);
			oled_render();

			sign_change = true;;
			npDrawAmpulheta(LOW_BRIGHT, LOW_BRIGHT, 0);

			sleep_ms(100);
			exam_started = false;
		}
		novo_tempo = false;
	// }
}


void gpio_irq_handler(uint gpio, uint32_t events){ // inicia exame e/ou reinicia
    absolute_time_t now = get_absolute_time();
	// printf("Passei por uma interrupção\n");
	if(absolute_time_diff_us(last_time_presed_a_or_b,now)>100000){//debounce de 100ms para ambos botões
		if (gpio == BUTTON_A){
			if (exam_started){
				contador_turnos = 0;
				exam_started = false;
			}
			else{
				exam_started = true;
				time_start_counting = get_absolute_time();
			}
		}
		else if (gpio == BUTTON_B){
			update_level();

		}
		last_time_presed_a_or_b=now;
		oled_clear();
		oled_msg_inicio(nivel); // atualiza a tela
		oled_render();
	}
}

void setup_buttons(){
	gpio_init(BUTTON_A);
	gpio_set_dir(BUTTON_A, GPIO_IN);
	gpio_pull_up(BUTTON_A); // Habilita o resistor pull-up interno para evitar leituras incorretas.
	gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_RISE, true);
	gpio_init(BUTTON_B);
	gpio_set_dir(BUTTON_B, GPIO_IN);
	gpio_pull_up(BUTTON_B); // Habilita o resistor pull-up interno para evitar leituras incorretas.
	gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_RISE, true);

	// Configura o handler global de interrupções
	gpio_set_irq_callback(gpio_irq_handler);
	irq_set_enabled(IO_IRQ_BANK0, true);
}

void update_level(){
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
}

void exam_handler(){
    if (exam_started){//se exame teve início

        if (!sign_change){ // se o sinal não tiver mudado(se manteve em false)
            // npClear();
            // npSetArrow(sign_placa);
            // npWrite();
			npDrawArrow(sign_placa);
            sleep_us(800); // pausa para estabilização da matriz(sem isso ela pisca sem parar)
        }
        else{ // se ele for verdadeiro

            oled_times_print(nivel,contador_turnos,tempo_turnos,8);
            oled_render();
            tempo_turnos[contador_turnos] = ((float)respose_time / 1000);
            // checagem de contador de turnos
            contador_turnos++;
            novo_tempo = true;
            if (contador_turnos > TURN_LIMIT){
                contador_turnos = 0;
            }

            npDrawAmpulheta(LOW_BRIGHT, 0, LOW_BRIGHT); // ampulheta
            sleep_ms(tempo_de_espera);

            npDrawArrow(sign_placa);

            sign_change = false;
            time_start_counting = get_absolute_time();
        }
    }
    
    if (novo_tempo && exam_started){
        oled_print_info();
    }
}