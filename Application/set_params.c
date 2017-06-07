#include <stdint.h>
#include <stdbool.h>

#include "pstorage.h"
#include "app_error.h"
#include "inter_flash.h"

#include "set_params.h"
#include "moto.h"

pstorage_handle_t block_id_params;
uint8_t flash_store_params[8];

uint8_t BEEP_DIDI_NUMBER;
uint8_t LED_LIGHT_TIME;
uint8_t	DOOR_OPEN_HOLD_TIME;
uint8_t	KEY_CHECK_NUMBER;
uint8_t KEY_LENGTH;


void set_default_params(void)
{
	uint32_t err_code;

	//设置动态密码的长度为6位ASCII
	KEY_LENGTH = 6;
	
	//读出设置的参数(判读是否是后期设置的,如果不是，设定参数)
	//初始化参数
	//([0x77(w，如果为w则已经设置参数，如果不是则初始化参数),
	//			25(OPEN_TIME *0.1s),10(DOOR_OPEN_HOLD_TIME *0.1s),
	//			5(BEEP_DIDI_NUMBER 次数)，5(LED_LIGHT_TIME *0.1s),
	//			5(KEY_CHECK_NUMBER) 次数]后面补0)
	err_code = pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)DEFAULT_PARAMS_OFFSET, &block_id_params);
	APP_ERROR_CHECK(err_code);
	pstorage_load(flash_store_params, &block_id_params, 8, 0);
	if(flash_store_params[0] == 0x77)
	{
		OPEN_TIME = flash_store_params[1];//电机转动时间
		DOOR_OPEN_HOLD_TIME = flash_store_params[2];//开门保持时间
		BEEP_DIDI_NUMBER = flash_store_params[3];//蜂鸣器响次数
		LED_LIGHT_TIME = flash_store_params[4];//灯亮起的时间
		KEY_CHECK_NUMBER = flash_store_params[5];//密码测试次数
	}
	else
	{
		OPEN_TIME = 0x19;//电机转动时间
		DOOR_OPEN_HOLD_TIME = 0x0a;//开门保持时间
		BEEP_DIDI_NUMBER = 0x05;//蜂鸣器响次数
		LED_LIGHT_TIME = 0x05;//灯亮起的时间
		KEY_CHECK_NUMBER = 0x05;//密码测试次数
	}
}

