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

uint8_t data_array_store[BLE_NUS_MAX_DATA_LEN];//20λ
uint32_t data_send_length = 0;

uint32_t record_length_number;
struct door_open_record door_open_record_get;
struct tm time_record;//������¼��ʱ��
struct tm time_record_compare;//Ҫ�Աȵ�ʱ��
time_t time_record_compare_t;//Ҫ�Աȵ�ʱ���int
time_t time_record_t;//������ʱ���int

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
	
	//���ȡ������ʱ����صı���
	struct tm time_set;
	struct tm time_get;
	//������mac�йصı���
	ble_gap_addr_t addr;
	//���ȡ��¼�����йصı���
	uint8_t tmp[4];
		
	//���� "01234"�Ƿ�Ὺ��
	if(strncasecmp((char *)p_data, "01234", 5) == 0)
	{
		ble_door_open();
	}
	
	switch(p_data[0])
	{
		case DOOR_OPEN_KEY://���ÿ�����Կ
		key_store_write((struct key_store_struct *)&p_data);
		break;
	
		case SYNC_TIME://ͬ��ʱ��
		//�Ƕ�ʱ����,[year0][year1][mon][day][hour][min][sec]
		time_set.tm_sec = (int)p_data[7];
		time_set.tm_min = (int)p_data[6];
		time_set.tm_hour = (int)p_data[5];
		time_set.tm_mday = (int)p_data[4];
		time_set.tm_mon = (int)p_data[3];
		time_set.tm_year = (int)((((int)p_data[1])<<8 | (int)p_data[2]) - 1990);			
		//��ʱ��д��RTC
		err_code =  rtc_time_write(&time_set);
		if(err_code ==NRF_SUCCESS)
		{
			//���������0x40,���ظ�app
			nus_data_array[0] = nus_data_array[0] + 0x40;
			ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
		}			
		break;
		
		case GET_TIME://��ȡʱ��
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
		
		case SET_PARAMS://���ò���
		//���õ��ת��ʱ��
		OPEN_TIME = p_data[1];
		//���ÿ���ʱ��
		DOOR_OPEN_HOLD_TIME = p_data[2];
		//���÷������춯����
		BEEP_DIDI_NUMBER = p_data[3];
		//��������ʱ��
		LED_LIGHT_TIME = p_data[4];
		//���������У�Դ���
		KEY_CHECK_NUMBER = p_data[5];
		
		memset(flash_write_data, 0, 8);
		memcpy(&flash_write_data[1], &p_data[1], 5);
		flash_write_data[0] = 0x77;
		//������д�뵽flash
		inter_flash_write(flash_write_data, 8, DEFAULT_PARAMS_OFFSET, &block_id_flash_store);
		
		//Ӧ���
		//���������0x40,���ظ�app
		nus_data_array[0] = nus_data_array[0] + 0x40;
		ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
		break;
		
		case SET_KEY_SEED://д������
		
		break;
		
		case SET_MAC://����mac
		memset(addr.addr, 0, 6);
		//�������õ�mac
		memcpy(addr.addr, &p_data[1], 6);
		err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,&addr);
		if(err_code == NRF_SUCCESS)
		{
			//���������0x40,���ظ�app
			nus_data_array[0] = nus_data_array[0] + 0x40;
			ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
		}		
		break;
		
/*		case SET_BLE_UUID://����uuid
		
		break;*/
		
		case SET_SUPER_KEY://���ù���Ա����
		memset(flash_write_data, 0, BLOCK_STORE_SIZE*sizeof(uint8_t));
		memcpy(&flash_write_data[1],&p_data[1], SUPER_KEY_LENGTH);
		flash_write_data[0] = 0x77;//'w'
		write_super_key(flash_write_data);
		break;
		
		case GET_USED_KEY://��ѯ��Ч����
		//��ȡ���������
		inter_flash_read(data_array_store, 4, KEY_STORE_OFFSET, &block_id_flash_store);
		//С���ֽڵļ���
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
		case GET_RECORD_NUMBER://��ѯ��¼����
		//��ȡ��¼������
		inter_flash_read(&data_array_store[1], 4, RECORD_OFFSET, &block_id_flash_store);
		memcpy(&data_array_store[1], tmp, 4);
		//����С�ֽڱ����ֽ�
		data_array_store[1] = tmp[3];
		data_array_store[2] = tmp[2];
		data_array_store[3] = tmp[1];
		data_array_store[4] = tmp[0];
		
		data_array_store[0] = p_data[0] + 0x40;
		ble_nus_string_send(&m_nus, data_array_store, 5);
		break;
		
		case GET_RECENT_RECORD://��ѯָ�����ں�ļ�¼
		time_record_compare.tm_sec = p_data[7];
		time_record_compare.tm_min = p_data[6];
		time_record_compare.tm_hour = p_data[5];
		time_record_compare.tm_mday = p_data[4];
		time_record_compare.tm_mon = p_data[3];
		time_record_compare.tm_year = (((int)p_data[1])<<8 | (int)p_data[2]);
		//��������
		time_record_compare_t = mktime(&time_record_compare);
		//��ȡ��¼������
		inter_flash_read(data_array_store, 4, RECORD_OFFSET, &block_id_flash_store);
		//С���ֽڣ����¼������
		record_length_number = ((int)data_array_store[0]) | ((int)data_array_store[1])<<8 |\
						((int)data_array_store[2])<<16 | ((int)data_array_store[3])<<24;
		
		for(int i=0; i<record_length_number; i++)
		{
			//������¼
			inter_flash_read((uint8_t *)&door_open_record_get, 12, (RECORD_OFFSET+1+i), &block_id_flash_store);
			//�Ա�ʱ��
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
