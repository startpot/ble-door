/***********************************
*	FI	BI	 fo 		bo			fun
*	H		L			H			L				forward
*	L		H			L			H				backward
*	H		H			L			L				brake
*	L		L			open	open		standby(stop)
************************************/

#include "moto.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "custom_board.h"

uint32_t OPEN_TIME;


/***********************
*初始化moto
************************/
void moto_init(void)
{
	//配置P.2(fi) P.3(bi)
	nrf_gpio_cfg_output(MOTO_FI);
	nrf_gpio_cfg_output(MOTO_BI);
	
	//设置为stand-by状态
	nrf_gpio_pin_clear(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
	printf("moto have been set stand_by\r\n");
}

/*****************************
*moto向前动ms
*****************************/
void moto_forward_ms(uint32_t ms)
{
	//HB设置为HL
	nrf_gpio_pin_set(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
	//延迟ms
	nrf_delay_ms(ms);
	nrf_gpio_pin_set(MOTO_BI);
			
	//延迟5ms
	nrf_delay_ms(5);
	//设置为stand-by状态
	nrf_gpio_pin_clear(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
}

/****************************
*mto向后动ms
****************************/
void moto_backward_ms(uint32_t ms)
{
	//HB设置为LH
	nrf_gpio_pin_set(MOTO_BI);
	nrf_gpio_pin_clear(MOTO_FI);
	//延迟ms
	nrf_delay_ms(ms);
	nrf_gpio_pin_set(MOTO_FI);
			
	//延迟5ms
	nrf_delay_ms(5);
	//设置为stand-by状态
	nrf_gpio_pin_clear(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
}

/***************************
*门打开
****************************/
void moto_open(uint32_t open_time)
{
		moto_forward_ms(open_time*100);
}

/*************************
*门关闭
*************************/
void moto_close(uint32_t close_time)
{
		moto_backward_ms(close_time*100);
}
