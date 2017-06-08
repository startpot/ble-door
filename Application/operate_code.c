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
#include "inter_flash.h"
#include "set_params.h"
#include "sm4_dpwd.h"

uint32_t key_store_number;
struct key_store_struct	key_store_struct_set;

uint8_t data_array_store[BLE_NUS_MAX_DATA_LEN];//20位
uint32_t data_send_length = 0;

struct door_open_record door_open_record_get;
struct tm 	time_record;//读出记录的时间
time_t 		time_record_t;//读出的时间的int
struct tm 	time_record_compare;//要对比的时间
time_t 		time_record_compare_t;//要对比的时间的int


/************************************************************
*对nus servvice传来的数据进行分析
*in：	*p_data			处理的数据指针
			length				数据长度
***********************************************************/
void operate_code_check(uint8_t *p_data, uint16_t length)
{
	uint8_t err_code;
	uint8_t set_fail[13] = "set mac fail";
	
	//与获取和设置时间相关的变量
	struct tm time_set;
	struct tm time_get;
	time_t time_get_t;
	
	switch(p_data[0])
	{
		case 0x30://设置开锁秘钥
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		if(length ==0x0a)
		{//字节数为10
			//获取收到的时间
			err_code = rtc_time_read(&time_get);
			if(err_code == NRF_SUCCESS)
			{
				time_get_t = mktime(&time_get);
			}
		
			//获取种子
			inter_flash_read(flash_read_data, 32, SEED_OFFSET, &block_id_flash_store);
			memset(seed, 0, 16);
			if(flash_read_data[0] == 0x77)
			{//设置了种子
				//获取种子
				memcpy(seed, &flash_read_data[1], 16);
			}
		
			//对比SET_KEY_CHECK_NUMBER次设置的密码
			for(int i=0; i<SET_KEY_CHECK_NUMBER; i++)
			{
				SM4_DPasswd(seed, time_get_t, SM4_INTERVAL, SM4_COUNTER, SM4_challenge, key_store_tmp);

				if(strncasecmp((char *)p_data, (char *)key_store_tmp, KEY_LENGTH) == 0)
				{//设置的密码相同

#if defined(BLE_DOOR_DEBUG)
					printf("key set success\r\n");
#endif

					//组织密码结构体
					memset(&key_store_struct_set, 0 , sizeof(struct key_store_struct));
					//写密码
					memcpy(&key_store_struct_set.key_store, p_data, 6);
					//写有效时间
					memcpy(&key_store_struct_set.key_use_time, &p_data[6], 2);
					//写控制字
					memcpy(&key_store_struct_set.control_bits, &p_data[8], 1);
					//写版本号
					memcpy(&key_store_struct_set.key_vesion, &p_data[9], 1);
					//写存入时间
					memcpy(&key_store_struct_set.key_store_time, &time_get_t, sizeof(time_t));
				
					//根据控制字，选择是否清除以前的钥匙，0x1则清除，0x0不清除
					if((key_store_struct_set.control_bits &0x1) ==0x1)
					{
						//设置钥匙记录为0
						err_code = pstorage_block_identifier_get(&block_id_flash_store, \
									(pstorage_size_t)KEY_STORE_OFFSET, &block_id_flash_store);
						APP_ERROR_CHECK(err_code);
						key_store_length = 0x00000000;
						err_code = pstorage_clear(&block_id_flash_store,BLOCK_STORE_SIZE);
						APP_ERROR_CHECK(err_code);
						err_code = pstorage_store(&block_id_flash_store, (uint8_t *)&key_store_length, 4, 0);
						APP_ERROR_CHECK(err_code);
#if defined(BLE_DOOR_DEBUG)
						printf("key_store length set %d\r\n", key_store_length);
#endif
						//将钥匙记录到flash
						key_store_write(&key_store_struct_set);
					}
					else
					{
						//直接将钥匙记录到flash
						key_store_write(&key_store_struct_set);
					}
#if defined(BLE_DOOR_DEBUG)
						printf("key set  success:");
						for(int j=0; j<KEY_LENGTH; j++)
						{
							printf("%c",key_store_struct_set.key_store[j]);
						}
						printf("\r\n");
#endif
				}
				else
				{
					time_get_t = time_get_t - 60;
				}
			}

		break;
		}
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
		//设置密码的校对次数(单位 10min)
		KEY_CHECK_NUMBER = p_data[5];
		
		memset(flash_write_data, 0, 8);
		//写入标记'w'
		flash_write_data[0] = 0x77;
		memcpy(&flash_write_data[1], &p_data[1], 5);
		
		//将参数写入到flash
		inter_flash_write(flash_write_data, 8, DEFAULT_PARAMS_OFFSET, &block_id_flash_store);
		
		//应答包
		//将命令加上0x40,返回给app
		nus_data_array[0] = nus_data_array[0] + 0x40;
		ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
		break;
		
		case SET_KEY_SEED://写入种子
		//传输的种子应该是小端字节
		memset(flash_write_data, 0, BLOCK_STORE_SIZE);
		//写入标记'w'
		flash_write_data[0] = 0x77;
		memcpy(&flash_write_data[1],&p_data[1], SEED_LENGTH);
		
		//将种子写入到flash
		inter_flash_write(flash_write_data, BLOCK_STORE_SIZE, SEED_OFFSET, &block_id_flash_store);
		
		//应答包
		//将命令加上0x40,返回给app
		nus_data_array[0] = nus_data_array[0] + 0x40;
		ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);	
		break;
		
		case SET_MAC://配置mac,与显示的mac反向
		if((p_data[6] &0xc0) ==0xc0)
		{//设置的mac最高2位为11，有效
			//存储mac地址
			memset(mac, 0, 8);
			mac[0] = 'w';
			mac[1] = 0x06;
			memcpy(&mac[3], &p_data[1], 6);
			inter_flash_write(mac, 8, MAC_OFFSET, &block_id_flash_store);
			//配置mac
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
		}
		else
		{
			//向手机发送失败信息"set mac fail"
			ble_nus_string_send(&m_nus, set_fail, 13);
		}
		break;
		
		case SET_SUPER_KEY://设置管理员密码
		memset(flash_write_data, 0, BLOCK_STORE_SIZE*sizeof(uint8_t));
		memcpy(&flash_write_data[1],&p_data[1], SUPER_KEY_LENGTH);
		flash_write_data[0] = 0x77;//'w'
		//超级密码就12位，取写入数据前面16位(16>(1+12))
		write_super_key(flash_write_data,16);
		break;
		
		case GET_USED_KEY://查询有效密码
		//获取密码的数量，小端字节
		inter_flash_read((uint8_t *)&key_store_number, 4, KEY_STORE_OFFSET, &block_id_flash_store);

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
		//读取记录的数量,小端字节
		inter_flash_read((uint8_t *)&record_length, 4, RECORD_OFFSET, &block_id_flash_store);
		memcpy(&data_array_store[1], (uint8_t *)&record_length, 4);
		//app也是小端
/*		data_array_store[1] = tmp[3];
		data_array_store[2] = tmp[2];
		data_array_store[3] = tmp[1];
		data_array_store[4] = tmp[0];
	*/	
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
		//获取记录的数量,小端字节
		inter_flash_read((uint8_t *)&record_length, 4, RECORD_OFFSET, &block_id_flash_store);
		
		for(int i=0; i<record_length; i++)
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
