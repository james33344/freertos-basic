#include <stdio.h>
#include "testfunc.h"
#include "clib.h"
#include "fio.h"
#include "FreeRTOS.h"

int fib(int number){
	if(number>40 || number <0){
		fio_printf(2, "Input error, see man page.\n\r");
		return -1;		
	}

	int previous = -1;
	int result = 1;
	int sum = 0;
	int i = 0;

	for(i=0; i<=number; i++){
		sum = result + previous;
		previous = result;
		result = sum;
		fio_printf(1, "Fibonacci %d is %d.\n\r",i , sum);
	}
	return 1;
}

