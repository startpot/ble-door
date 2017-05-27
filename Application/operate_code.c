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

//����������һϵ�ж�����صı�����һ����ɲ��ˣ���Ҫ����ȫ�ֱ���
uint8_t 	id_num_set;
uint8_t 	key_length_set;
uint8_t 	key_store_set[8];

struct tm 	key_store_time_set;
time_t 		key_use_time_set;
struct key_store_struct	key_store_struct_set;


uint8_t data_array_store[BLE_NUS_MAX_DATA_LEN];//20λ
uint32_t data_send_length = 0;

uint8_t huge_data_store[BLOCK_STORE_SIZE];


void mac_set(void)
{
	uint32_t err_code;
	//����mac��ַ
	
	ble_gap_addr_t addr;
	err_code = sd_ble_gap_address_get(&addr);
	APP_ERROR_CHECK(err_code);
	addr.addr[0] +=1;
	err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,&addr);
	APP_ERROR_CHECK(err_code);
}

/********************************
*��nus servvice���������ݽ��з���
********************************/
void operate_code_check(uint8_t *p_data, uint16_t length)
{
	uint8_t err_code;
	int count;
	static char *reply_code = "OK";
	
	//���ȡ������ʱ����صı���
	struct tm time_set;
	struct tm time_get;
		
	//�Ƿ��ǻ�ȡʱ������õ� 4*7��byte
	if(strncasecmp((char *)p_data, GET_TIME, (sizeof(GET_TIME) - 1)) == 0)
	{
		rtc_time_read(&time_get);
		
		data_array_store[0] = (uint8_t)time_get.tm_sec; //��ȡ��
		data_array_store[1] = (uint8_t)time_get.tm_min; //��ȡ��
		data_array_store[2] = (uint8_t)time_get.tm_hour;//��ȡʱ
		data_array_store[3] = (uint8_t)time_get.tm_mday;//��ȡ��
		data_array_store[4] = (uint8_t)time_get.tm_mon;	//��ȡ��
		data_array_store[5] = (uint8_t)time_get.tm_year;//��ȡ��
		data_array_store[6] = (uint8_t)time_get.tm_wday;//��ȡ��
		data_send_length = 7;
		
		err_code = ble_nus_string_send(&m_nus, data_array_store, data_send_length);
		if(err_code != NRF_ERROR_INVALID_STATE)
		{
			APP_ERROR_CHECK(err_code);
		}
	}			

	//�Ƿ��Ƕ�ʱ����
	if(strncasecmp((char *)p_data, SYNC_TIME, (sizeof(SYNC_TIME) - 1)) == 0)
	{
		//�Ƕ�ʱ����,intδ16λ��charΪ8λ,���͵�����Ϊ2λ��ʾ1λ
		//����ȡ�����[10:16]��7byte����д��tm�ṹ�塣uint8_tǿ��ת��int
		time_set.tm_sec = (int)data_array_store[sizeof(SYNC_TIME) + 1];
		time_set.tm_min = (int)data_array_store[sizeof(SYNC_TIME) + 2];
		time_set.tm_hour = (int)data_array_store[sizeof(SYNC_TIME) + 3];
		time_set.tm_mday = (int)data_array_store[sizeof(SYNC_TIME) + 4];
		time_set.tm_mon = (int)data_array_store[sizeof(SYNC_TIME) + 5];
		time_set.tm_year = (int)data_array_store[sizeof(SYNC_TIME) + 6];
		time_set.tm_wday = (int)data_array_store[sizeof(SYNC_TIME) + 7];			
		//��ʱ��д��RTC
		err_code =  rtc_time_write(&time_set);
		if(err_code ==NRF_SUCCESS)
		{
			//дOK�ַ�������λ��,char ��uint8_tһ��
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, (sizeof(reply_code) - 1));
		}			
	}
		
		//�Ƿ�����������(struct key_store_struct)
		if(strncasecmp((char *)p_data, SET_KEY_STORE, (sizeof(SET_KEY_STORE) - 1)) == 0)
		{
			//ȡID��
			id_num_set = p_data[sizeof(SET_KEY_STORE)];
			//ȡ��������
			key_length_set = length - (sizeof(SET_KEY_STORE) + 1);//�ַ���(����'\0'=:)+id_num
			//ȡ����
			for(int i = 0; i < key_length_set; i++)
			{
				key_store_set[i] = p_data[(sizeof(SET_KEY_STORE) + 1 + i)];
			}
			id_key_store_setted = true;//���ñ�־λ
			//�����뷵�ظ�APP
			ble_nus_string_send(&m_nus, (uint8_t *)key_store_set, key_length_set);
			//дOK�ַ�������λ��,char ��uint8_tһ��
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, sizeof(reply_code) - 1);
		}
	
		//�Ƿ�����������Ĵ洢ʱ��
		if(strncasecmp((char *)p_data, SET_KEY_STORE_TIME, (sizeof(SET_KEY_STORE_TIME) - 1)) == 0)
		{
			//ȡ�洢ʱ��
			key_store_time_set.tm_sec = (int)p_data [sizeof(SET_KEY_STORE_TIME) + 1];
			key_store_time_set.tm_min = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 2];
			key_store_time_set.tm_hour = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 3];
			key_store_time_set.tm_mday = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 4];
			key_store_time_set.tm_mon = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 5];
			key_store_time_set.tm_year = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 6];
			key_store_time_set.tm_wday = (int)p_data[sizeof(SET_KEY_STORE_TIME) + 7];
			
			key_store_time_setted = true;//���ñ�־λ
			//дOK�ַ�������λ��,char ��uint8_tһ��
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, sizeof(reply_code));
		}
		//�Ƿ�������ʹ��ʱ��
		if(strncasecmp((char *)p_data, SET_KEY_USE_TIME, (sizeof(SET_KEY_USE_TIME) - 1)) == 0)
		{
			//ȡʹ��ʱ��,С����ǰ
			key_use_time_set = p_data[sizeof(SET_KEY_USE_TIME) + 1] |\
													p_data[sizeof(SET_KEY_USE_TIME) + 2]<<4 |\
													p_data[sizeof(SET_KEY_USE_TIME) + 3]<<8 |\
													p_data[sizeof(SET_KEY_USE_TIME) + 4]<<12;
			
			key_use_time_setted =true;//���ñ�־λ
			//дOK�ַ�������λ��,char ��uint8_tһ��
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, sizeof(reply_code));
			
		}
		
		//�����������������ɺ󣬻�ȡ����ṹ�壬�洢
		if(id_key_store_setted && key_store_time_setted && key_use_time_setted)
		{
			key_store_struct_set.id_num = id_num_set;
			key_store_struct_set.key_length = key_length_set;
			memcpy(key_store_struct_set.key_store, key_store_set, key_length_set);
			key_store_struct_set.key_store_time = key_store_time_set;
			key_store_struct_set.key_use_time = key_use_time_set;

			//������洢��flash		
			key_store_write(&key_store_struct_set);
			
			id_key_store_setted = false;
			key_store_time_setted = false;
			key_use_time_setted = false;			
		}
		
		//�Ƿ���������
		if(strncasecmp((char *)p_data, SET_DATA, (sizeof(SET_DATA) - 1)) == 0)
		{
			data_send_length = length - sizeof(SET_DATA);
			
			for(int i = 0; i < data_send_length; i++)
			{
				data_array_store[i] = p_data[(sizeof(SET_DATA) + i)];
			}
			inter_flash_write(data_array_store, data_send_length, 0,&block_id_flash_store);

			//дOK�ַ�������λ��,char ��uint8_tһ��
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, (sizeof(reply_code) - 1));
		}
		
		//�Ƿ��ǻ�ȡ����
		if(strncasecmp((char *)p_data, GET_DATA, (sizeof(GET_DATA) - 1)) == 0)
		{
			
			//��ȡ��¼��ֵ
			inter_flash_read(data_array_store, data_send_length, (pstorage_size_t)0, &block_id_flash_store);
				
			ble_nus_string_send(&m_nus, data_array_store, data_send_length);
	
			//дOK�ַ�������λ��,char ��uint8_tһ��
			ble_nus_string_send(&m_nus, (uint8_t *)reply_code, (sizeof(reply_code) - 1));
			data_send_length = 0;
		}
		
		//��ȡ���һ�εĿ�����¼
		if(strncasecmp((char *)p_data, GET_RECD, (sizeof(GET_RECD) - 1)) == 0)
		{
			//��ȡ���һ�ο�����¼��flashλ��
			pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)(KEY_STORE_NUMBER + 1), &block_id_flash_store);
			pstorage_load(&next_block_id_site, &block_id_dest, 4, 0);
			
			if(next_block_id_site ==0)
			{//��¼Ϊ0
				char *no_record = "no record";
				ble_nus_string_send(&m_nus, (uint8_t *)no_record, (sizeof(no_record) - 1));
			}
			else
			{
			memset(&huge_data_store, 0, BLOCK_STORE_SIZE);
			inter_flash_read(huge_data_store, BLOCK_STORE_SIZE, (KEY_STORE_NUMBER + 1 + next_block_id_site), &block_id_flash_store);
			
			//�����ݷֿ鷢��ȥ
			for(count = 0; count < (BLOCK_STORE_SIZE / 20); count++)
			{
				memcpy(data_array_store, &huge_data_store[count*20], 20);
				ble_nus_string_send(&m_nus, data_array_store, 20);
			}
			memcpy(data_array_store, &huge_data_store[(count+1)*20], (BLOCK_STORE_SIZE%20));
			ble_nus_string_send(&m_nus, data_array_store, (BLOCK_STORE_SIZE%20));
			}
		}
		
		//�Ƿ�����������ʱ��
		if(strncasecmp((char *)p_data, SET_LIGHT_TIME, (sizeof(SET_LIGHT_TIME) - 1)) == 0)
		{
			for(int i = 0; i < (length - sizeof(SET_LIGHT_TIME)); i++)
			{
				data_array_store[i] = p_data[(sizeof(SET_LIGHT_TIME) + i)];
			}
			//ȡ��λ
			LED_LIGHT_TIME = data_array_store[length - sizeof(SET_LIGHT_TIME) - 1] - 0x30;
			for(int j = 0; j < (length - sizeof(SET_LIGHT_TIME)); j++)
			{
				LED_LIGHT_TIME = LED_LIGHT_TIME + ((data_array_store[j] -0x30) * (length - sizeof(SET_LIGHT_TIME) - 1 - j) * 10);
			}
		}
		
		//�Ƿ������÷������춯ʱ��
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
		
		//�Ƿ������ÿ���ʱ��
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
		
		//�Ƿ������õ���򿪵�ʱ��
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

	//���� "01234"�Ƿ�Ὺ��
	if(strncasecmp((char *)p_data, "01234", 5) == 0)
	{
		ble_door_open();
	}
}
