/**********************
*�Ľŷ�����
*
**********************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "custom_board.h"
#include "boards.h"

#include "beep.h"


/**************
*	��ʼ��������
**************/
void beep_init(void)
{
	nrf_gpio_cfg_output(BEEP_IN_PIN);
	//���ø�
	nrf_gpio_pin_clear(BEEP_IN_PIN);
	printf("beep have been set low\r\n");
}

/*********************
* ��������һ��
*********************/
static void	beep_didi_once(void)
{
	for(int i = 0; i < BEEP_DIDI_ONCE_TIME; i++)
	{
	nrf_gpio_pin_clear(BEEP_IN_PIN);
	nrf_delay_us(BEEP_OPEN);
	nrf_gpio_pin_set(BEEP_IN_PIN);
	nrf_delay_us(BEEP_CLOSE);
	}
}

/*****************
*�������켸��
*****************/
void beep_didi(uint8_t number)
{
	//��������
	for(int i = 0; i < number; i++)
	{
		beep_didi_once();
		nrf_delay_ms(50);
	}
	//�ָ�������״̬
	nrf_gpio_pin_set(BEEP_IN_PIN);	
}
