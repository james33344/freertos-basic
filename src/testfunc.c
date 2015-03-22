#include <stdio.h>
#include "testfunc.h"
#include "clib.h"
#include "fio.h"
#include "FreeRTOS.h"
#include "timers.h"
extern int fibonacci(int);
static int count = 0;

void fib_timer(xTimerHandle pxTimer){
	count++;
}

int fib(int number){
	if(number>40 || number <0){
		fio_printf(2, "Input error, see man page.\n\r");
		return -1;		
	}

	int result = 0;
/*	xTimerHandle t;
	t = xTimerCreate((const signed char*)"timer",
						1,
						pdTRUE,
						NULL,
						fib_timer
						);	

	xTimerStart(t,0);*/
	result = fibonacci(number);
//	xTimerStop(t,0);
	fio_printf(1,"The fibonacii sequence %d is %d. Exec ticks is %d\n\r",number,result,count);
	
	return 1;
}

