#include <stdio.h>
#include "testfunc.h"
#include "clib.h"
#include "fio.h"
#include "FreeRTOS.h"
#include "stm32f4xx.h"
#include "timers.h"
#include "host.h"
extern int fibonacci(int);
unsigned int get_systick()
{
     static unsigned int const *reload = (void *) 0xE000E014;
     static unsigned int const *current = (void *) 0xE000E018;
//     static const unsigned int scale = 1000000 / configTICK_RATE_HZ;
                     /* microsecond */

     return xTaskGetTickCount() * (*reload) +
            (*reload - *current);
}

float get_time(){
#if 0
    static unsigned int const *reload = (void *) 0xE000E014;
    static unsigned int const *current = (void *) 0xE000E018;
//	float tt = (float)(8.0f * 18750.f / (SystemCoreClock)) * 1000.0f;
//    static const unsigned int scale = 1000 / configTICK_RATE_HZ;
    return (float)xTaskGetTickCount()  + 
            ((float)(*reload - *current)/ (float)(*reload));
#endif
	return	(float)get_systick();// * (float)(8.0f*18750.0f/(SystemCoreClock)) * 1000.0f;
}

char* ftoa(float f){
		static char buf[64];
		char* bufa;
		int dig = 0;
		int a = (int)f;
		bufa = itoa("0123456789",a,10);
		strcpy(buf,bufa);
		f = (f - (float)a) * 1000000;
		for(int i=f;i;i/=10)dig++;
		
		a = (int)f;
		strcat(buf,".");
		for(;dig<6;dig++){
			strcat(buf,"0");
		}
		bufa = itoa("0123456789",a,10);
		strcat(buf,bufa);
		return buf;
}

int fib(int number){
	if(number>40 || number <0){
		fio_printf(2, "Input error, see man page.\n\r");
		return -1;		
	}
	float t;
	char buf_echo[128] = "echo fibonacci ";
	char buf_echo_w[512] = "echo \"";
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

//	host_action(SYS_SYSTEM, "echo start.");
	buf = itoa("0123456789",number,10);
	strcat(buf_echo,buf);
	strcat(buf_echo_w,buf);
	strcat(buf_echo_w," ");

	strcat(buf_echo," takes time is: ");
//	buf = itoa("0123456789",t,10);
	buf = ftoa(t);
	strcat(buf_echo,buf);
	strcat(buf_echo_w,buf);
#ifdef FIB_r
	strcat(buf_echo_w,"\" >> plot_fib_r");
#elif FIB_r2
	strcat(buf_echo_w,"\" >> plot_fib_r2");
#else
	strcat(buf_echo_w,"\" >> plot_fib");
#endif

//	xTimerStop(t,0);
	host_action(SYS_SYSTEM, buf_echo);
	host_action(SYS_SYSTEM, buf_echo_w);
//	fio_printf(1,"The fibonacii sequence %d is %d. Exec ticks is %d\n\r",number,result,t);
	
//	host_action(SYS_SYSTEM, "echo end.");
	return 1;
}

