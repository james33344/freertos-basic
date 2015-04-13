#define USE_STDPERIPH_DRIVER
#define MOUNTPOINT "romfs"
//#include "stm32f10x.h"
#include "stm32f4xx.h"
//#include "stm32_p103.h"
//#include "system_stm32f10x.h"
#include "stm32f4xx_rcc.h"
#include "system_stm32f4xx.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"
#include "romfs.h"

#include "clib.h"
#include "shell.h"
#include "host.h"

#include "testfunc.h"
/* _sromfs symbol can be found in main.ld linker script
 * it contains file system structure of test_romfs directory
 */
extern const unsigned char _sromfs;

//static void setup_hardware();

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
/* Add for serial input */
volatile xQueueHandle serial_rx_queue = NULL;

enum sysclk_freq {
    SYSCLK_42_MHZ=0,
    SYSCLK_84_MHZ,
    SYSCLK_168_MHZ,
    SYSCLK_200_MHZ,
    SYSCLK_240_MHZ,
};
 
void rcc_set_frequency(enum sysclk_freq freq)
{
    int freqs[]   = {42, 84, 168, 200, 240};
 
    /* USB freqs: 42MHz, 42Mhz, 48MHz, 50MHz, 48MHz */
    int pll_div[] = {2, 4, 7, 10, 10}; 
 
    /* PLL_VCO = (HSE_VALUE / PLL_M) * PLL_N */
    /* SYSCLK = PLL_VCO / PLL_P */
    /* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
    uint32_t PLL_P = 2;
    uint32_t PLL_N = freqs[freq] * 2;
    uint32_t PLL_M = (HSE_VALUE/1000000)*30;
    uint32_t PLL_Q = pll_div[freq];
 
    RCC_DeInit();
 
    /* Enable HSE osscilator */
    RCC_HSEConfig(RCC_HSE_ON);
 
    if (RCC_WaitForHSEStartUp() == ERROR) {
        return;
    }
 
    /* Configure PLL clock M, N, P, and Q dividers */
    RCC_PLLConfig(RCC_PLLSource_HSE, PLL_M, PLL_N, PLL_P, PLL_Q);
    /* Enable PLL clock */
    RCC_PLLCmd(ENABLE);
 
    /* Wait until PLL clock is stable */
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);
 
    /* Set PLL_CLK as system clock source SYSCLK */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
 
    /* Set AHB clock divider */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
 
    /* Set APBx clock dividers */
    switch (freq) {
        /* Max freq APB1: 42MHz APB2: 84MHz */
        case SYSCLK_42_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div1); /* 42MHz */
            RCC_PCLK2Config(RCC_HCLK_Div1); /* 42MHz */
            break;
        case SYSCLK_84_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div2); /* 42MHz */
            RCC_PCLK2Config(RCC_HCLK_Div1); /* 84MHz */
            break;
        case SYSCLK_168_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div4); /* 42MHz */
            RCC_PCLK2Config(RCC_HCLK_Div2); /* 84MHz */
            break;
        case SYSCLK_200_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div4); /* 50MHz */
            RCC_PCLK2Config(RCC_HCLK_Div2); /* 100MHz */
            break;
        case SYSCLK_240_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div4); /* 60MHz */
            RCC_PCLK2Config(RCC_HCLK_Div2); /* 120MHz */
            break;
    }
 
    /* Update SystemCoreClock variable */
    SystemCoreClockUpdate();
}


/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART2_IRQHandler()
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;

	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}else if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET){
		char msg = USART_ReceiveData(USART2);

		/* If there is an error when queueing the received byte, freeze! */
		while(!xQueueSendToBackFromISR(serial_rx_queue, &msg, &xHigherPriorityTaskWoken));
			
	}
	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}

void send_byte(char ch)
{
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART2, ch);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

char recv_byte()
{
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	char msg;
	while(!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));
//fio_printf(2,"DD\n\r");
	return msg;
}
void command_prompt(void *pvParameters)
{
	char buf[128];
	char *argv[20];
    char hint[] = USER_NAME "@" USER_NAME "-STM32:~$ ";
	fio_printf(1, "\rWelcome to FreeRTOS Shell\r\n");
	while(1){
                fio_printf(1, "%s", hint);
		fio_read(0, buf, 127);
	
		int n=parse_command(buf, argv);

		/* will return pointer to the command function */
		cmdfunc *fptr=do_command(argv[0]);
		if(fptr!=NULL)
			fptr(n, argv);
		else
			fio_printf(2, "\r\n\"%s\" command not found.\r\n", argv[0]);
	}

}

void system_logger(void *pvParameters)
{
    signed char buf[128];
    char output[512] = {0};
    char *tag = "\nName          State   Priority  Stack  Num\n*******************************************\n";
    int handle, error;
    const portTickType xDelay = 100000 / 100;

    handle = host_action(SYS_OPEN, "output/syslog", 4);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n");
        return;
    }

    while(1) {
        memcpy(output, tag, strlen(tag));
        error = host_action(SYS_WRITE, handle, (void *)output, strlen(output));
        if(error != 0) {
            fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
            host_action(SYS_CLOSE, handle);
            return;
        }
        vTaskList(buf);

        memcpy(output, (char *)(buf + 2), strlen((char *)buf) - 2);

        error = host_action(SYS_WRITE, handle, (void *)buf, strlen((char *)buf));
        if(error != 0) {
            fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
            host_action(SYS_CLOSE, handle);
            return;
        }

        vTaskDelay(xDelay);
    }
    
    host_action(SYS_CLOSE, handle);
}

void test_fib(){
	host_action(SYS_SYSTEM, "echo > plot_fib_r");
	int i=2;
	for(;i<44;i++)
	fib(i);

}


int main()
{
//rcc_set_frequency(SYSCLK_42_MHZ);
//	init_rs232();
//	enable_rs232_interrupts();
//	enable_rs232();
	
	fs_init();
	fio_init();
	
//	SysTick_Config(72E6);

	register_romfs(MOUNTPOINT, &_sromfs);
//	printf("romfs success\n\r");	

	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	vSemaphoreCreateBinary(serial_tx_wait_sem);
	/* Add for serial input 
	 * Reference: www.freertos.org/a00116.html */
	serial_rx_queue = xQueueCreate(1, sizeof(char));

    register_devfs();
	/* Create a task to output text read from romfs. */
	xTaskCreate(command_prompt,
	            (signed portCHAR *) "CLI",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 2 , NULL);

	xTaskCreate(test_fib,
	            (signed portCHAR *) "test_fib",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 10 , NULL);
#if 0
	/* Create a task to record system log. */
	xTaskCreate(system_logger,
	            (signed portCHAR *) "Logger",
	            1024 /* stack size */, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif

	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
