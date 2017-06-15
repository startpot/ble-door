/********************************
*	初始化LEDs
*	初始化I2C的int_pin和中断处理函数
*********************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

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
#include "sm4_mcu.h"
#include "sm4_dpwd.h"
#include "my_time.h"

#define APP_GPIOTE_MAX_USERS		1

app_gpiote_user_id_t	m_app_gpiote_id;

char key_express_value;

//输入按键值，当作输入密码
char key_input[KEY_NUMBER];
uint8_t key_input_site;

//输入的密码的时间
struct tm key_input_time_tm;
time_t key_input_time_t;

struct key_store_struct key_store_check;

//存储在flash的密码
uint8_t flash_key_store[BLOCK_STORE_SIZE];

///开锁记录全局变量
struct door_open_record		open_record_now;


/***********************************************
 *初始化LED pins
 * 设置LED PINS high(led light when pin is low)
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
	
#if defined(BLE_DOOR_DEBUG)	
	printf("all leds not lit\r\n");
#endif

}

/***********************************************
*LED等亮ms
*in:	led_pin	操作的led引脚
		ms			led亮起的时间，单位0.1s
***********************************************/
void leds_on(uint8_t led_pin, uint32_t ms)
{
	if(((1<<led_pin) & LEDS_MASK) >>led_pin == 1)
	{
		nrf_gpio_pin_clear(led_pin);
		nrf_delay_ms(ms*100);
		nrf_gpio_pin_set(led_pin);
	}
}

/************************************************
*写入键值
************************************************/
static void write_key_expressed(void)
{
	if(key_input_site < KEY_NUMBER)
	{
		key_input[key_input_site] = key_express_value;
		key_input_site ++;
	}
}

/***********************************************
*清除写入的键值
***********************************************/
static void clear_key_expressed(void)
{
	for(int i = 0; i < KEY_NUMBER; i++)
	{
		key_input[i] = 0x0;
	}
	key_input_site = 0x0;
}

/***********************************************
*门打开函数
***********************************************/
void ble_door_open(void)
{
	//亮绿灯
	leds_on(LED_13, LED_LIGHT_TIME);
	//蜂鸣器响几次(BEER_DIDI_NUMBER)
	beep_didi(BEEP_DIDI_NUMBER);
	//开锁
	moto_open(OPEN_TIME);
	nrf_delay_ms(DOOR_OPEN_HOLD_TIME * 100);
	//蜂鸣器响
	beep_didi(BEEP_DIDI_NUMBER);
	//恢复moto状态
	moto_close(OPEN_TIME);
}

/**************************************************************
*检验所有按下的键值
**************************************************************/
static void check_keys(void)
{
	//获取按下开锁键的时间
	rtc_time_read(&key_input_time_tm);
	key_input_time_t = my_mktime(&key_input_time_tm);
		
	//如果按键数量和设置超级密码一致
	if(key_input_site == SUPER_KEY_LENGTH)
	{	
		inter_flash_read(flash_read_data, 16, SPUER_KEY_OFFSET, &block_id_flash_store);		
		if(flash_read_data[0] == 0x77)
		{
			memset(super_key, 0, 12);
			memcpy(super_key, &flash_read_data[1],12);
			if(strncasecmp(key_input,super_key, SUPER_KEY_LENGTH) == 0)
			{
				ble_door_open();
#if defined(BLE_DOOR_DEBUG)
				printf("it is spuer key\r\n");
				printf("door open\r\n");
#endif
			}
		}
	}
	//普通密码
	else if(key_input_site == KEY_LENGTH)
	{	//6位密码，首先进行动态密码比对，再进行普通密码比对
		
		//动态密码，获取种子
		inter_flash_read(flash_read_data, 32, SEED_OFFSET, &block_id_flash_store);
		if(flash_read_data[0] == 0x77)
		{//设置了种子
			//获取种子16位，128bit
			memset(seed, 0, 16);
			memcpy(seed, &flash_read_data[1], 16);
			
			//计算KEY_CHECK_NUMBER *10次数
			for(int i = 0; i<(KEY_CHECK_NUMBER * 10);i++)
			{
				SM4_DPasswd(seed, key_input_time_t, SM4_INTERVAL, SM4_COUNTER, SM4_challenge, key_store_tmp);
				if(strncasecmp(key_input, (char *)key_store_tmp, KEY_LENGTH) == 0)
				{//密码相同
					ble_door_open();
#if defined(BLE_DOOR_DEBUG)
					printf("it is a dynamic key auto set\r\n");
					printf("door open\r\n");
#endif
					//记录开门
					memset(&open_record_now, 0, sizeof(struct door_open_record));
					memcpy(&open_record_now.key_store, key_input, 6);
					memcpy(&open_record_now.door_open_time, &key_input_time_t, 4);
					record_write(&open_record_now);
					goto clear_keys_input;
				}
				else
				{
					key_input_time_t = key_input_time_t - 60;
				}						
			}					
		}
		
		//普通密码
		//获取普通密码的个数,小端字节
		inter_flash_read((uint8_t *)key_store_length, 4, KEY_STORE_OFFSET, &block_id_flash_store);
		if(key_store_length >0)
		{
			for(int i=0; i<key_store_length; i++)
			{
				//获取存储的密码
				inter_flash_read((uint8_t *)&key_store_check, sizeof(struct key_store_struct), \
													(KEY_STORE_OFFSET + 1 + i), &block_id_flash_store);
				//对比密码是否一致
				if(strncasecmp(key_input, (char *)&key_store_check.key_store, 6) == 0)
				{//密码相同，看是否在有效时间内
					if((double)(my_difftime(key_input_time_t, key_store_check.key_store_time) <\
								((double)key_store_check.key_use_time * 60)) )
					{
						ble_door_open();
#if defined(BLE_DOOR_DEBUG)
						printf("it is a dynamic key user set\r\n");
						printf("door open\r\n");
#endif
						//记录开门
						memset(&open_record_now, 0, sizeof(struct door_open_record));
						memcpy(&open_record_now.key_store, key_input, 6);
						memcpy(&open_record_now.door_open_time, &key_input_time_t, 4);
						record_write(&open_record_now);
					}
				}					
			}
		}		
	}

clear_keys_input:
	//判断完输入的按键序列后，删除所有按键值
	clear_key_expressed();
#if defined(BLE_DOOR_DEBUG)
	printf("clear all express button\r\n");
#endif
}

/*****************************************
*检验按下的键值，并记录，
*如果是开锁键(b)进行密码校验
*in：	express_value	按下的键值
******************************************/
static void check_key_express(char express_value)
{

	switch(express_value)
	{
		//判断按键，亮点0.5s
		case '0'://按下0键
			leds_on(LED_8, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 0 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '1'://按下1键
			leds_on(LED_1, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 1 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '2'://按下2键
			leds_on(LED_5, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 2 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '3'://按下3键
			leds_on(LED_9, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 3 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '4'://按下4键
			leds_on(LED_2, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 4 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '5'://按下5键
			leds_on(LED_6, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 5 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '6'://按下6键
			leds_on(LED_10, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 6 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '7'://按下7键
			leds_on(LED_3, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 7 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '8'://按下8键
			leds_on(LED_7, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 8 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case '9'://按下9键
			leds_on(LED_11, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button 9 pressed\r\n");
#endif
			write_key_expressed();
			break;
		case 'a'://按下*键
			leds_on(LED_4, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button * pressed\r\n");
#endif
		//	write_key_expressed();
			break;
		case 'b'://按下 #(开锁) 键
			leds_on(LED_12, LED_LIGHT_TIME);
#if defined(BLE_DOOR_DEBUG)
			printf("button open pressed\r\n");
#endif
			check_keys();
			break;
		default:
				
			break;
	}
		
}

/**************************************************************
*触摸屏中断处理函数
*in：	event_pins_low_to_high	状态由低到高的引脚
*			event_pins_high_to_low	状态由高到低的引脚
**************************************************************/
void iic_int_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
{

	if (event_pins_high_to_low & (1 << TOUCH_IIC_INT_PIN))
	{
		//中断由高变低
		key_express_value = (char)tsm12_key_read();
		check_key_express(key_express_value);
	} 
}

/***************************************************************
* 初始化触摸屏中断引脚
*in：	none
***************************************************************/
void iic_int_buttons_init(void)
{
	uint32_t err_code;
    
	//初始化键值，变量
	key_express_value = 0x0;
	//初始化按键记录,及按键值保存位置
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
#if defined(BLE_DOOR_DEBUG)
	printf("touch button int init success\r\n");
#endif
}
