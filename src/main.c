#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "include/exam.h"


//preparo de tasks

//task para PIO
// task para OLED
// task para botões
// task para joystick


int main(){
	stdio_init_all();
	exam_setup();

	sleep_ms(2000);

	struct repeating_timer timer;
	add_repeating_timer_ms(10, repeating_reader, NULL, &timer);

	while (true){
		exam_handler();
	}
}
