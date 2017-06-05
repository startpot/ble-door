#ifndef INTER_FLASH_H__
#define INTER_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "pstorage.h"

//�洢��Կ��
struct key_store_struct
{
	uint32_t 	key_store;
	uint16_t 	key_use_time;
	uint8_t		control_bits;
	uint8_t		key_vesion;
};



#define SUPER_KEY_LENGTH		12
//��������Ա��Կ
extern uint8_t	super_key[SUPER_KEY_LENGTH];

//���ż�¼
struct door_open_record
{
	uint32_t	key_store_record;
	time_t		door_open_time;//�Ŵ򿪵�ʱ��s
};

/********************************************************
*flash�洢�ռ�����һ��block_id
*[0]�洢����+[1]Կ�׸���+Կ�׼�¼(10)+[12]����+���ż�¼[30]
* flash�и����洢����ƫ�Ƶ�ַ�ͳ���
*********************************************************/
#define BLOCK_STORE_SIZE			16

/*********************************************************
*Ĭ�ϵĲ���:(1byte)
*			���ת��ʱ��(OPEN_TIME)
*			���ź�ȴ�ʱ��(DOOR_OPEN_HOLD_TIME)
*			�������춯����(BEEP_DIDI_NUMBER)
*			����ʱ��(LED_LIGHT_TIME)
*			����У�Դ���(KEY_CHECK_NUMBER)
**********************************************************/
#define	DEFAULT_PARAMS_OFFSET		0
#define DEFAULT_PARAMS_NUMBER		1

#define SPUER_KEY_OFFSET			DEFAULT_PARAMS_OFFSET + DEFAULT_PARAMS_NUMBER
#define SUPER_KEY_NUMBER			1

#define	KEY_STORE_OFFSET			SPUER_KEY_OFFSET + SUPER_KEY_NUMBER
#define KEY_STORE_LENGTH			1
#define	KEY_STORE_NUMBER			10

#define	RECORD_OFFSET				KEY_STORE_OFFSET + KEY_STORE_LENGTH + KEY_STORE_NUMBER
#define RECORD_LENGTH				1
#define	RECORD_NUMBER				30


#define BLOCK_STORE_COUNT			DEFAULT_PARAMS_NUMBER + SUPER_KEY_NUMBER +\
									KEY_STORE_LENGTH + KEY_STORE_NUMBER +\
									RECORD_LENGTH + RECORD_NUMBER


//��flash�ж���������
extern uint8_t	flash_write_data[BLOCK_STORE_SIZE];
extern uint8_t	flash_read_data[BLOCK_STORE_SIZE];


extern pstorage_handle_t	block_id_flash_store;
extern pstorage_handle_t	block_id_key_store;
extern pstorage_handle_t	block_id_record;

extern pstorage_handle_t	block_id_dest;


extern uint32_t				key_store_length;
extern uint32_t				record_length;




extern bool key_store_full;
extern bool record_full;


void flash_init(void);
void inter_flash_write(uint8_t *p_data, uint32_t data_len, \
					   pstorage_size_t block_id_offset, pstorage_handle_t *block_id_write);
void inter_flash_read(uint8_t *p_data, uint32_t data_len, \
					 pstorage_size_t block_id_offset, pstorage_handle_t *block_id_read);

void write_super_key(uint8_t *p_data);
void key_store_write(struct key_store_struct *key_input);
void record_write(struct door_open_record *open_record);

#endif //INTER_FLASH_H__
