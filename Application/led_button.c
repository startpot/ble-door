/********************************
*	��ʼ��LEDs
*	��ʼ��I2C��int_pin���жϴ�����
*********************************/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "custom_board.h"
#include "boards.h"
#include "app_gpiote.h"

#include "led_button.h"
#include "touch_tsm12.h"
#include "moto.h"
#include "beep.h"
#include "set_params.h"
#include "inter_flash.h"
#include "rtc_chip.h"

#define APP_GPIOTE_MAX_USERS		1

app_gpiote_user_id_t	m_app_gpiote_id;

char key_express_value;

//���밴��ֵ��������������
char key_input[KEY_NUMBER];
uint8_t key_input_site;

//������¼ȫ�ֱ���
struct door_open_record		open_record_now;



/***********************************************
 *��ʼ��LED pins
 * ����LED PINS high(led light when pin is low)
 **********************************************/
void leds_init(void)
{
	uint32_t led_list[LEDS_NUMBER] = LEDS_LIST;

	//set output set high //led lit when set low
	for(int pin = 0; pin <LEDS_NUMBER; pin++)
	{
		nrf_gpio_cfg_output( led_list[pin] );
		nrf_gpio_pin_set( led_list[pin] );
	}
	printf("all leds set 1 (not lit)\r\n");
}

/*******************************************
*clear_all_key_flag,��������밴���йصı���
*******************************************/
void clear_all_key_flag(void)
{
	key_express_value = 0x0;
}

/********************************
*LED����ms
********************************/
void leds_on(uint8_t led_pin, uint32_t ms)
{
	if(((1<<led_pin) & LEDS_MASK) >>led_pin == 1)
	{
		nrf_gpio_pin_clear(led_pin);
		nrf_delay_ms(ms*100);
		nrf_gpio_pin_set(led_pin);
	}
}


/*****************
*д���ֵ
*****************/
static	void write_key_expressed(void)
{
	if(key_input_site < KEY_NUMBER)
	{
	key_input[key_input_site] = key_express_value;
	key_input_site ++;
	}
}


/********************
*���д��ļ�ֵ
*********************/
static void clear_key_expressed(void)
{
	for(int i = 0; i < KEY_NUMBER; i++)
	{
		key_input[i] = 0x0;
	}
	key_input_site = 0x0;
}

/********************
*ble_door_open
********************/
void ble_door_open(void)
{
	//���̵�
	leds_on(LED_13, LED_LIGHT_TIME);
	//�������켸��(BEER_DIDI_NUMBER)
	beep_didi(BEEP_DIDI_NUMBER);
	//����
	moto_open(OPEN_TIME);
	nrf_delay_ms(DOOR_OPEN_HOLD_TIME * 100);
	//��������
	beep_didi(BEEP_DIDI_NUMBER);
	//�ָ�moto״̬
	moto_close(OPEN_TIME);
}


/*****************************************
*���鰴�µļ�ֵ������¼��
*����ǿ�����(b)��������У��
******************************************/
static void check_key_express(char express_value)
{
	bool door_open = 0;
	switch(express_value)
	{
		//�жϰ���������0.5s
		case '0'://����0��
			leds_on(LED_8, LED_LIGHT_TIME);
			printf("button 0 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '1'://����1��
			leds_on(LED_1, LED_LIGHT_TIME);
			printf("button 1 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '2'://����2��
			leds_on(LED_5, LED_LIGHT_TIME);
			printf("button 2 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '3'://����3��
			leds_on(LED_9, LED_LIGHT_TIME);
			printf("button 3 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '4'://����4��
			leds_on(LED_2, LED_LIGHT_TIME);
			printf("button 4 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '5'://����5��
			leds_on(LED_6, LED_LIGHT_TIME);
			printf("button 5 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '6'://����6��
			leds_on(LED_10, LED_LIGHT_TIME);
			printf("button 6 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '7'://����7��
			leds_on(LED_3, LED_LIGHT_TIME);
			printf("button 7 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '8'://����8��
			leds_on(LED_7, LED_LIGHT_TIME);
			printf("button 8 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case '9'://����9��
			leds_on(LED_11, LED_LIGHT_TIME);
			printf("button 9 pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case 'a'://����*��
			leds_on(LED_4, LED_LIGHT_TIME);
			printf("button * pressed\r\n");
			write_key_expressed();
			clear_all_key_flag();
			break;
		case 'b'://���� #(����) ��
			leds_on(LED_12, LED_LIGHT_TIME);
			printf("button open pressed\r\n");
			clear_all_key_flag();
			//����������������õ�������һ��
			if(key_input_site == key_length_set)
			{
				printf("door open button express,check all numbers expressed\r\n");
				for(int i =0; i<key_length_set; i++)
				{
					if(key_input[i] != key_store_set[i])
					{
						door_open = false;
						goto check_fail;
					}
					else
						door_open = true;
				}
				if(door_open == true)
				{
					ble_door_open();
					printf("door ope\r\n");
					//��¼����
					rtc_time_read(&(open_record_now.door_open_time));
					//��¼����������
					memset(open_record_now.key_store, 0, 10);
					memcpy(open_record_now.key_store, key_input, key_input_site);
					record_write(&open_record_now);
				}
			}
			
check_fail:
			clear_key_expressed();
			printf("clear all express button\r\n");
			break;
		default:
				
			break;
	}
}

/*******************************
*i2c_int_handler
*******************************/
void iic_int_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
{
	
	
	if (event_pins_high_to_low & (1 << TOUCH_IIC_INT_PIN))
	{
		//�ж��ɸ߱��
		key_express_value = (char)tsm12_key_read();
		check_key_express(key_express_value);
	} 
}

/*********************
* ��ʼ��iic_int_pin
********************/
void iic_int_buttons_init(void)
{
	uint32_t err_code;
    
	//��ʼ����ֵ������
	key_express_value = 0x0;
	//��ʼ��������¼,������ֵ����λ��
	clear_key_expressed();
		
	uint32_t   low_to_high_bitmask = 0x00000000;
	uint32_t   high_to_low_bitmask = (1 << TOUCH_IIC_INT_PIN);
    
	// Configure IIC_INT_PIN with SENSE enabled
	nrf_gpio_cfg_sense_input(TOUCH_IIC_INT_PIN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
    
	APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);   
  
	err_code = app_gpiote_user_register(&m_app_gpiote_id, 
                                        low_to_high_bitmask, 
                                        high_to_low_bitmask, 
                                        iic_int_handler);
	APP_ERROR_CHECK(err_code);
    
	err_code = app_gpiote_user_enable(m_app_gpiote_id);
	APP_ERROR_CHECK(err_code);
	printf("touch button interrupte init success\r\n");
}
