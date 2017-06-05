#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "bsp.h"
#include "ble_nus.h"
#include "ble_gap.h"
#include "pstorage.h"

#include "operate_code.h"
#include "ble_init.h"
#include "rtc_chip.h"
#include "moto.h"
#include "inter_flash.h"
#include "set_params.h"
#include "led_button.h"


uint32_t key_store_number;
struct key_store_struct	key_store_struct_set;

uint8_t data_array_store[BLE_NUS_MAX_DATA_LEN];//20位
uint32_t data_send_length = 0;

uint32_t record_length_number;
struct door_open_record door_open_record_get;
struct tm time_record;//读出记录的时间
struct tm time_record_compare;//要对比的时间
time_t time_record_compare_t;//要对比的时间的int
time_t time_record_t;//读出的时间的int

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
	
	//与获取和设置时间相关的变量
	struct tm time_set;
	struct tm time_get;
	//与设置mac有关的变量
	ble_gap_addr_t addr;
	//与获取记录数量有关的变量
	uint8_t tmp[4];
		
	//测试 "01234"是否会开锁
	if(strncasecmp((char *)p_data, "01234", 5) == 0)
	{
		ble_door_open();
	}
	
	switch(p_data[0])
	{
		case DOOR_OPEN_KEY://设置开锁秘钥
		key_store_write((struct key_store_struct *)&p_data);
		break;
	
		case SYNC_TIME://同步时间
		//是对时命令,[year0][year1][mon][day][hour][min][sec]
		time_set.tm_sec = (int)p_data[7];
		time_set.tm_min = (int)p_data[6];
		time_set.tm_hour = (int)p_data[5];
		time_set.tm_mday = (int)p_data[4];
		time_set.tm_mon = (int)p_data[3];
		time_set.tm_year = (int)((((int)p_data[1])<<8 | (int)p_data[2]) - 1990);			
		//将时间写入RTC
		err_code =  rtc_time_write(&time_set);
		if(err_code ==NRF_SUCCESS)
		{
			//将命令加上0x40,返回给app
			nus_data_array[0] = nus_data_array[0] + 0x40;
			ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
		}			
		break;
		
		case GET_TIME://获取时间
		err_code = rtc_time_read(&time_get);
		if(err_code == NRF_SUCCESS)
		{
			data_array_store[0] = nus_data_array[0] + 0x40;
			data_array_store[1] = (uint8_t)((time_get.tm_year + 1990)>>8);
			data_array_store[2] = (uint8_t)(time_get.tm_year + 1990);
			data_array_store[3] = (uint8_t)time_get.tm_mon;
			data_array_store[4] = (uint8_t)time_get.tm_mday;
			data_array_store[5] = (uint8_t)time_get.tm_hour;
			data_array_store[6] = (uint8_t)time_get.tm_min;
			data_array_store[7] = (uint8_t)time_get.tm_sec;
			ble_nus_string_send(&m_nus, data_array_store, 8);
		}
		break;
		
		case SET_PARAMS://设置参量
		//设置电机转动时间
		OPEN_TIME = p_data[1];
		//设置开门时间
		DOOR_OPEN_HOLD_TIME = p_data[2];
		//设置蜂鸣器响动次数
		BEEP_DIDI_NUMBER = p_data[3];
		//设置亮灯时间
		LED_LIGHT_TIME = p_data[4];
		//设置密码的校对次数
		KEY_CHECK_NUMBER = p_data[5];
		
		memset(flash_write_data, 0, 8);
		memcpy(&flash_write_data[1], &p_data[1], 5);
		flash_write_data[0] = 0x77;
		//将参数写入到flash
		inter_flash_write(flash_write_data, 8, DEFAULT_PARAMS_OFFSET, &block_id_flash_store);
		
		//应答包
		//将命令加上0x40,返回给app
		nus_data_array[0] = nus_data_array[0] + 0x40;
		ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
		break;
		
		case SET_KEY_SEED://写入种子
		
		break;
		
		case SET_MAC://配置mac
		memset(addr.addr, 0, 6);
		//拷贝设置的mac
		memcpy(addr.addr, &p_data[1], 6);
		err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,&addr);
		if(err_code == NRF_SUCCESS)
		{
			//将命令加上0x40,返回给app
			nus_data_array[0] = nus_data_array[0] + 0x40;
			ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
		}		
		break;
		
/*		case SET_BLE_UUID://配置uuid
		
		break;*/
		
		case SET_SUPER_KEY://设置管理员密码
		memset(flash_write_data, 0, BLOCK_STORE_SIZE*sizeof(uint8_t));
		memcpy(&flash_write_data[1],&p_data[1], SUPER_KEY_LENGTH);
		flash_write_data[0] = 0x77;//'w'
		write_super_key(flash_write_data);
		break;
		
		case GET_USED_KEY://查询有效密码
		//获取密码的数量
		inter_flash_read(data_array_store, 4, KEY_STORE_OFFSET, &block_id_flash_store);
		//小端字节的计算
		key_store_number = (((int)data_array_store[0])| ((int)data_array_store[1])<<8 |\
							((int)data_array_store[2])<<16 | ((int)data_array_store[3])<<24);
		data_array_store[0] = p_data[0] + 0x40;
		data_array_store[1] = (uint8_t)key_store_number;
		
		for(int i=0; i<key_store_number; i++)
		{
			data_array_store[2] = (uint8_t)i;
			inter_flash_read(&data_array_store[3], sizeof(struct key_store_struct),\
							 (KEY_STORE_OFFSET+1+i), &block_id_flash_store);
			ble_nus_string_send(&m_nus, data_array_store, sizeof(struct key_store_struct)+3);
		}
		break;
		case GET_RECORD_NUMBER://查询记录数量
		//读取记录的数量
		inter_flash_read(&data_array_store[1], 4, RECORD_OFFSET, &block_id_flash_store);
		memcpy(&data_array_store[1], tmp, 4);
		//进行小字节变大端字节
		data_array_store[1] = tmp[3];
		data_array_store[2] = tmp[2];
		data_array_store[3] = tmp[1];
		data_array_store[4] = tmp[0];
		
		data_array_store[0] = p_data[0] + 0x40;
		ble_nus_string_send(&m_nus, data_array_store, 5);
		break;
		
		case GET_RECENT_RECORD://查询指定日期后的记录
		time_record_compare.tm_sec = p_data[7];
		time_record_compare.tm_min = p_data[6];
		time_record_compare.tm_hour = p_data[5];
		time_record_compare.tm_mday = p_data[4];
		time_record_compare.tm_mon = p_data[3];
		time_record_compare.tm_year = (((int)p_data[1])<<8 | (int)p_data[2]);
		//计算秒数
		time_record_compare_t = mktime(&time_record_compare);
		//获取记录的数量
		inter_flash_read(data_array_store, 4, RECORD_OFFSET, &block_id_flash_store);
		//小端字节，求记录的数量
		record_length_number = ((int)data_array_store[0]) | ((int)data_array_store[1])<<8 |\
						((int)data_array_store[2])<<16 | ((int)data_array_store[3])<<24;
		
		for(int i=0; i<record_length_number; i++)
		{
			//读出记录
			inter_flash_read((uint8_t *)&door_open_record_get, 12, (RECORD_OFFSET+1+i), &block_id_flash_store);
			//对比时间
			if(difftime(door_open_record_get.door_open_time, time_record_compare_t)>0)
			{
				data_array_store[0] = p_data[0] + 0x40;
				data_array_store[1] = i;
				memcpy(&data_array_store[2], &door_open_record_get, 12);
				ble_nus_string_send(&m_nus, data_array_store, 14);
			}
		}
		break;
		default:
		
		break;
	}	
	
}
