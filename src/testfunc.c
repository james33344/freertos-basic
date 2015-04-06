#include <stdio.h>
#include "testfunc.h"
#include "clib.h"
#include "fio.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "host.h"
extern int fibonacci(int);
static int count = 0;
 unsigned int get_time()
 {
     static unsigned int const *reload = (void *) 0xE000E014;
     static unsigned int const *current = (void *) 0xE000E018;
     static const unsigned int scale = 1000000 / configTICK_RATE_HZ;
                     /* microsecond */

     return xTaskGetTickCount() * scale +
            (*reload - *current) / (*reload / scale);
}
void fib_timer(xTimerHandle pxTimer){
	count++;
}

int fib(int number){
	if(number>40 || number <0){
		fio_printf(2, "Input error, see man page.\n\r");
		return -1;		
	}
	int t;
	char buf_echo[128] = "echo fibonacci ";
	char* buf;
//	int result = 0;
/*	xTimerHandle t;
	t = xTimerCreate((const signed char*)"timer",
						1,
						pdTRUE,
						NULL,
						fib_timer
						);	
	xTimerStart(t,0);*/
	t = get_time();
/*	result =*/ fibonacci(number);
	t = get_time() - t;
	buf = itoa("0123456789",number,10);
	strcat(buf_echo,buf);
	strcat(buf_echo," takes time is: ");
	buf = itoa("0123456789",t,10);
	strcat(buf_echo,buf);

//	xTimerStop(t,0);
	host_action(SYS_SYSTEM, buf_echo);
//	fio_printf(1,"The fibonacii sequence %d is %d. Exec ticks is %d\n\r",number,result,t);
	
	return 1;
}

