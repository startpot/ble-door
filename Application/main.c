/** *****************************
*--------------------------------
* bluetooth-door
* ------------------------------- 
*	12 LEDs ; iic(sda,scl,en,int)
*--------------------------------
*	2-wire moto ; 1-wire beer
*--------------------------------
*	ble-softdevice-s130
*--------------------------------
*	nrf51822-P.x	define in
*	custom_board.h
********************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "nordic_common.h"
#include "bsp.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_config.h"

#include "ble_hci.h"
#include "ble_dis.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "pstorage.h"
#include "device_manager.h"
#include "app_trace.h"
#include "app_util_platform.h"

#include "led_button.h"
#include "touch_tsm12.h"
#include "moto.h"
#include "beep.h"
#include "rtc_chip.h"
#include "inter_flash.h"
#include "operate_code.h"
#include "set_params.h"
#include "ble_init.h"


/*****************************
*Application main function.
*****************************/
int main(void)
{
	uint32_t err_code;
	bool erase_bonds;
	
	//初始化UART,printf打印调试信息需要uart，所以先初始化uart
	//uart的速率为115200尽量的快，打印出全部信息
	uart_init();
	
	printf("***ble door controller***");
	printf("\r\n");
	
	//初始化所有参数
	set_default_params();
	
	timers_init();
	//初始化协议栈
	ble_stack_init();
	device_manager_init(erase_bonds);
	gap_params_init();
	services_init();
	advertising_init();
	mac_set();
	conn_params_init();
   
	
	//初始化内部flash
	flash_init();
	//初始化灯，拉高，灭
	leds_init();
	//初始化电机
	moto_init();
	//初始化蜂鸣器
	beep_init();
	//初始化触摸屏
	tsm12_init();
	//初始化RTC
	rtc_init();
	
	//时间RTC测试代码,设定时间为2017.5.1. 23：59：59
	struct tm time_set_test = 
	{
		.tm_sec = 59,
		.tm_min = 59,
		.tm_hour = 23,
		.tm_mday = 1,
		.tm_mon = 4,
		.tm_year = 27,
		.tm_wday = 1,
	};
	rtc_time_write(&time_set_test);
	//初始化触摸屏的中断函数
	iic_int_buttons_init();
	
	//添加配对密码
	char *passcode = "123456";
	ble_opt_t static_option;
	
	static_option.gap_opt.passkey.p_passkey = (uint8_t *)passcode;
	err_code = sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &static_option);
	APP_ERROR_CHECK(err_code);
	printf("ble pair pin set:%s \n",passcode);
	printf("\r\n");
	application_timers_start();
	err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(err_code);
	// Enter main loop.
	while(true)
	{
		power_manage();
		//判断命令
		if(operate_code_setted ==true)
		{
			operate_code_check(nus_data_array, nus_data_array_length);
			operate_code_setted = false;
		}
	}
}
