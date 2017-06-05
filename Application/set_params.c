

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
uint8_t key_length_set;


void set_default_params(void)
{
	uint32_t err_code;

	//���ö�̬����ĳ���Ϊ6λASCII
	key_length_set = 6;
	
	//�������õĲ���(�ж��Ƿ��Ǻ������õ�,������ǣ��趨����)
	//��ʼ������
	//([0x77(w�����Ϊw���Ѿ����ò���������������ʼ������),
	//			25(OPEN_TIME *0.1s),10(DOOR_OPEN_HOLD_TIME *0.1s),
	//			5(BEEP_DIDI_NUMBER ����)��5(LED_LIGHT_TIME *0.1s),
	//			5(KEY_CHECK_NUMBER) ����]���油0)
	err_code = pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)DEFAULT_PARAMS_OFFSET, &block_id_params);
	APP_ERROR_CHECK(err_code);
	pstorage_load(flash_store_params, &block_id_params, 8, 0);
	if(flash_store_params[0] == 0x77)
	{
		OPEN_TIME = flash_store_params[1];//���ת��ʱ��
		DOOR_OPEN_HOLD_TIME = flash_store_params[2];//���ű���ʱ��
		BEEP_DIDI_NUMBER = flash_store_params[3];//�����������
		LED_LIGHT_TIME = flash_store_params[4];//�������ʱ��
		KEY_CHECK_NUMBER = flash_store_params[5];//������Դ���
	}
	else
	{
		OPEN_TIME = 0x19;//���ת��ʱ��
		DOOR_OPEN_HOLD_TIME = 0x0a;//���ű���ʱ��
		BEEP_DIDI_NUMBER = 0x05;//�����������
		LED_LIGHT_TIME = 0x05;//�������ʱ��
		KEY_CHECK_NUMBER = 0x05;//������Դ���
	}
}

