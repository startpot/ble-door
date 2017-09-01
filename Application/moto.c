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

#include "moto.h"
#include "set_params.h"

/***********************
*初始化电�
************************/
void moto_init(void)
{
	//配置P.2(fi) P.3(bi)
	nrf_gpio_cfg_output(MOTO_FI);
	nrf_gpio_cfg_output(MOTO_BI);
	
	//设置为stand-by状�
	nrf_gpio_pin_clear(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
#if defined(BLE_DOOR_DEBUG)
	printf("moto stand_by\r\n");
#endif
}

/**************************************************
*moto向前动ms
*in�ms		电机向前转动的时间，单位ms
**************************************************/
static void moto_forward_ms(uint32_t ms)
{
	//HB设置为HL
	nrf_gpio_pin_set(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
	//延迟ms
	nrf_delay_ms(ms);
	nrf_gpio_pin_set(MOTO_BI);
			
	//延迟5ms
	nrf_delay_ms(5);
	//设置为stand-by状�
	nrf_gpio_pin_clear(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
}

/***************************************************
*mto向后动ms
*in�ms	电机向后转动的时间，单位ms
****************************************************/
static void moto_backward_ms(uint32_t ms)
{
	//HB设置为LH
	nrf_gpio_pin_set(MOTO_BI);
	nrf_gpio_pin_clear(MOTO_FI);
	//延迟ms
	nrf_delay_ms(ms);
	nrf_gpio_pin_set(MOTO_FI);
			
	//延迟5ms
	nrf_delay_ms(5);
	//设置为stand-by状�
	nrf_gpio_pin_clear(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
}

/************************************************************
*门打开
*in�	open_time		门打开的时间，单位0.1s
*************************************************************/
void moto_open(uint32_t open_time)
{
	moto_forward_ms(open_time*100);
}

/************************************************************
*门关�
*in�	close_time		门关闭的时间，单�.1s
*************************************************************/
void moto_close(uint32_t close_time)
{
	moto_backward_ms(close_time*100);
}
