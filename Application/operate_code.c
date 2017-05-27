#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "bsp.h"
#include "ble_nus.h"
#include "pstorage.h"

#include "operate_code.h"
#include "ble_init.h"
#include "rtc_chip.h"
#include "moto.h"
#include "inter_flash.h"
#include "set_params.h"
#include "led_button.h"

bool	id_key_store_setted = false;
bool	key_store_time_setted = false;
bool	key_use_time_setted	=	false;

//与设置密码一系列动作相关的变量，一步完成不了，需要设置全局变量
uint8_t 	id_num_set;
uint8_t 	key_length_set;
uint8_t 	key_store_set[8];

struct tm 	key_store_time_set;
time_t 		key_use_time_set;
struct key_store_struct	key_store_struct_set;


uint8_t data_array_store[BLE_NUS_MAX_DATA_LEN];//20位
uint32_t data_send_length = 0;

uint8_t huge_data_store[BLOCK_STORE_SIZE];


void mac_set(void)
{
	uint32_t err_code;
	//设置mac地址
	
	ble_gap_addr_t addr;
	err_code = sd_ble_gap_address_get(&addr);
	APP_ERROR_CHECK(err_code);
	addr.addr[0] +=1;
	err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,&addr);
	APP_ERROR_CHECK(err_code);
}

/********************************
*对nus servvice传来的数据进行分析
********************************/
void operate_code_check(uint8_t *p_data, uint16_t length)
{
	uint8_t err_code;
	int count;
	static char *reply_code = "OK";
	
	//与获取和设置时间相关的变量
	struct tm time_set;
	struct tm time_get;
		
	//是否是获取时间命令，得到 4*7个byte
	if(strncasecmp((char *)p_data, GET_TIME, (sizeof(GET_TIME) - 1)) == 0)
	{
		rtc_time_read(&time_get);
		
		data_array_store[0] = (uint8_t)time_get.tm_sec; //截取秒
		data_array_store[1] = (uint8_t)time_get.tm_min; //截取分
		data_array_store[2] = (uint8_t)time_get.tm_hour;//截取时
		data_array_store[3] = (uint8_t)time_get.tm_mday;//截取天
		data_array_store[4] = (uint8_t)time_get.tm_mon;	//截取月
		data_array_store[5] = (uint8_t)time_get.tm_year;//截取年
		data_array_store[6] = (uint8_t)time_get.tm_wday;//截取周
		data_send_length = 7;
		
		err_code = ble_nus_string_send(&m_nus, data_array_store, data_send_length);
		if(err_code != NRF_ERROR_INVALID_STATE)
		{
			APP_ERROR_CHECK(err_code);
		}
	}			

	//是否是对时命令
	if(strncasecmp((char *)p_data, SYNC_TIME, (sizeof(SYNC_TIME) - 1)) == 0)
	{
		//是对时命令,int未16位，char为8位,发送的命令为2位表示1位
		//将获取的命令，[10:16]共7byte依次写入tm结构体。uint8_t强制转换int
		time_set.tm_sec = (int)data_array_store[sizeof(SYNC_TIME) + 1];
		time_set.tm_min = (int)data_array_store[sizeof(SYNC_TIME) + 2];
		time_set.tm_hour = (int)data_array_store[sizeof(SYNC_TIME) + 3];
		time_set.tm_mday = (int)data_array_store[sizeof(SYNC_TIME) + 4];
		time_set.tm_mon = (int)data_array_store[sizeof(SYNC_TIME) + 5];
		time_set.tm_year = (int)data_array_store[sizeof(SYNC_TIME) + 6];
		time_set.tm_wday = (int)data_array_store[sizeof(SYNC_TIME) + 7];			
		//将时间写入RTC
		err_code =  rtc_time_write(&time_set);
		if(err_code ==NRF_SUCCESS)
		{
			//写OK字符串给上位机,char 和uint8_t一致
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, (sizeof(reply_code) - 1));
		}			
	}
		
		//是否是设置密码(struct key_store_struct)
		if(strncasecmp((char *)p_data, SET_KEY_STORE, (sizeof(SET_KEY_STORE) - 1)) == 0)
		{
			//取ID号
			id_num_set = p_data[sizeof(SET_KEY_STORE)];
			//取密码数量
			key_length_set = length - (sizeof(SET_KEY_STORE) + 1);//字符串(包含'\0'=:)+id_num
			//取密码
			for(int i = 0; i < key_length_set; i++)
			{
				key_store_set[i] = p_data[(sizeof(SET_KEY_STORE) + 1 + i)];
			}
			id_key_store_setted = true;//设置标志位
			//将密码返回给APP
			ble_nus_string_send(&m_nus, (uint8_t *)key_store_set, key_length_set);
			//写OK字符串给上位机,char 和uint8_t一致
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, sizeof(reply_code) - 1);
		}
	
		//是否是设置密码的存储时间
		if(strncasecmp((char *)p_data, SET_KEY_STORE_TIME, (sizeof(SET_KEY_STORE_TIME) - 1)) == 0)
		{
			//取存储时间
			key_store_time_set.tm_sec = (int)p_data [sizeof(SET_KEY_STORE_TIME) + 1];
			key_store_time_set.tm_min = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 2];
			key_store_time_set.tm_hour = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 3];
			key_store_time_set.tm_mday = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 4];
			key_store_time_set.tm_mon = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 5];
			key_store_time_set.tm_year = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 6];
			key_store_time_set.tm_wday = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 7];
			
			key_store_time_setted = true;//设置标志位
			//写OK字符串给上位机,char 和uint8_t一致
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, sizeof(reply_code));
		}
		//是否是设置使用时间
		if(strncasecmp((char *)p_data, SET_KEY_USE_TIME, (sizeof(SET_KEY_USE_TIME) - 1)) == 0)
		{
			//取使用时间,小端在前
			key_use_time_set = p_data[sizeof(SET_KEY_USE_TIME) + 1] |\
													p_data[sizeof(SET_KEY_USE_TIME) + 2]<<4 |\
													p_data[sizeof(SET_KEY_USE_TIME) + 3]<<8 |\
													p_data[sizeof(SET_KEY_USE_TIME) + 4]<<12;
			
			key_use_time_setted =true;//设置标志位
			//写OK字符串给上位机,char 和uint8_t一致
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, sizeof(reply_code));
			
		}
		
		//设置密码的三步都完成后，获取密码结构体，存储
		if(id_key_store_setted && key_store_time_setted && key_use_time_setted)
		{
			key_store_struct_set.id_num = id_num_set;
			key_store_struct_set.key_length = key_length_set;
			memcpy(key_store_struct_set.key_store, key_store_set, key_length_set);
			key_store_struct_set.key_store_time = key_store_time_set;
			key_store_struct_set.key_use_time = key_use_time_set;

			//将密码存储在flash		
			key_store_write(&key_store_struct_set);
			
			id_key_store_setted = false;
			key_store_time_setted = false;
			key_use_time_setted = false;			
		}
		
		//是否设置数据
		if(strncasecmp((char *)p_data, SET_DATA, (sizeof(SET_DATA) - 1)) == 0)
		{
			data_send_length = length - sizeof(SET_DATA);
			
			for(int i = 0; i < data_send_length; i++)
			{
				data_array_store[i] = p_data[(sizeof(SET_DATA) + i)];
			}
			inter_flash_write(data_array_store, data_send_length, 0,&block_id_flash_store);

			//写OK字符串给上位机,char 和uint8_t一致
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, (sizeof(reply_code) - 1));
		}
		
		//是否是获取数据
		if(strncasecmp((char *)p_data, GET_DATA, (sizeof(GET_DATA) - 1)) == 0)
		{
			
			//获取记录的值
			inter_flash_read(data_array_store, data_send_length, (pstorage_size_t)0, &block_id_flash_store);
				
			ble_nus_string_send(&m_nus, data_array_store, data_send_length);
	
			//写OK字符串给上位机,char 和uint8_t一致
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, (sizeof(reply_code) - 1));
			data_send_length = 0;
		}
		
		//获取最近一次的开锁记录
		if(strncasecmp((char *)p_data, GET_RECD, (sizeof(GET_RECD) - 1)) == 0)
		{
			//获取最近一次开锁记录的flash位置
			pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)(KEY_STORE_NUMBER + 1), &block_id_flash_store);
			pstorage_load(&next_block_id_site, &block_id_dest, 4, 0);
			
			if(next_block_id_site ==0)
			{//记录为0
				char *no_record = "no record";
				ble_nus_string_send(&m_nus, (uint8_t *)no_record, (sizeof(no_record) - 1));
			}
			else
			{
			memset(&huge_data_store, 0, BLOCK_STORE_SIZE);
			inter_flash_read(huge_data_store, BLOCK_STORE_SIZE, (KEY_STORE_NUMBER + 1 + next_block_id_site), &block_id_flash_store);
			
			//将数据分块发出去
			for(count = 0; count < (BLOCK_STORE_SIZE / 20); count++)
			{
				memcpy(data_array_store, &huge_data_store[count*20], 20);
				ble_nus_string_send(&m_nus, data_array_store, 20);
			}
			memcpy(data_array_store, &huge_data_store[(count+1)*20], (BLOCK_STORE_SIZE%20));
			ble_nus_string_send(&m_nus, data_array_store, (BLOCK_STORE_SIZE%20));
			}
		}
		
		//是否是设置亮灯时间
		if(strncasecmp((char *)p_data, SET_LIGHT_TIME, (sizeof(SET_LIGHT_TIME) - 1)) == 0)
		{
			for(int i = 0; i < (length - sizeof(SET_LIGHT_TIME)); i++)
			{
				data_array_store[i] = p_data[(sizeof(SET_LIGHT_TIME) + i)];
			}
			//取个位
			LED_LIGHT_TIME = data_array_store[length - sizeof(SET_LIGHT_TIME) - 1] - 0x30;
			for(int j = 0; j < (length - sizeof(SET_LIGHT_TIME)); j++)
			{
				LED_LIGHT_TIME = LED_LIGHT_TIME + ((data_array_store[j] -0x30) * (length - sizeof(SET_LIGHT_TIME) - 1 - j) * 10);
			}
		}
		
		//是否是设置蜂鸣器响动时间
		if(strncasecmp((char *)p_data, SET_BEEP_TIME, (sizeof(SET_BEEP_TIME) - 1)) == 0)
		{
			for(int i = 0; i < (length - sizeof(SET_BEEP_TIME)); i++)
			{
				data_array_store[i] = p_data[(sizeof(SET_BEEP_TIME) + i)];
			}
			BEEP_DIDI_NUMBER = data_array_store[length - sizeof(SET_BEEP_TIME) - 1] - 0x30;;
			for(int j = 0; j < (length - sizeof(SET_BEEP_TIME)); j++)
			{
				BEEP_DIDI_NUMBER = BEEP_DIDI_NUMBER + ((data_array_store[j] -0x30) * (length - sizeof(SET_BEEP_TIME) - 1 - j) * 10);
			}
		}
		
		//是否是设置开门时间
		if(strncasecmp((char *)p_data, SET_DOOR_HOLD_TIME, (sizeof(SET_DOOR_HOLD_TIME) - 1)) == 0)
		{
			for(int i = 0; i < (length - sizeof(SET_DOOR_HOLD_TIME)); i++)
			{
				data_array_store[i] = p_data[(sizeof(SET_DOOR_HOLD_TIME) + i)];
			}
			DOOR_OPEN_HOLD_TIME = data_array_store[length - sizeof(SET_DOOR_HOLD_TIME) - 1] - 0x30;
			for(int j = 0; j < (length - sizeof(SET_DOOR_HOLD_TIME)); j++)
			{
				DOOR_OPEN_HOLD_TIME = DOOR_OPEN_HOLD_TIME + ((data_array_store[j] -0x30) * (length - sizeof(SET_DOOR_HOLD_TIME) - 1 - j) * 10);
			}
		}
		
		//是否是设置电机打开的时间
		if(strncasecmp((char *)p_data, SET_MOTO_OPEN_TIME, (sizeof(SET_MOTO_OPEN_TIME) - 1)) == 0)
		{
			for(int i = 0; i < (length - sizeof(SET_MOTO_OPEN_TIME)); i++)
			{
				data_array_store[i] = p_data[(sizeof(SET_MOTO_OPEN_TIME) + i)];
			}
			OPEN_TIME = data_array_store[length - sizeof(SET_MOTO_OPEN_TIME) - 1] - 0x30;
			for(int j = 0; j < (length - sizeof(SET_MOTO_OPEN_TIME)); j++)
			{
				OPEN_TIME = OPEN_TIME + ((data_array_store[j] -0x30) * (length - sizeof(SET_MOTO_OPEN_TIME) - 1 - j) * 10);
			}
		}

	//测试 "01234"是否会开锁
	if(strncasecmp((char *)p_data, "01234", 5) == 0)
	{
		ble_door_open();
	}
}
