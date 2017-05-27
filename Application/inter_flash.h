#ifndef INTER_FLASH_H__
#define INTER_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "pstorage.h"

//�洢��Կ��
struct key_store_struct
{
	uint8_t		id_num;
	uint8_t 	key_length;
	uint8_t 	key_store[8];//ASCII,���8λ
	struct tm 	key_store_time;
	time_t 		key_use_time;
};
//���ż�¼
struct door_open_record
{
	uint8_t		key_store[8];
	struct tm	door_open_time;//�Ŵ򿪵�ʱ��
};

/********************************************************
*flash�洢�ռ�����һ��block_id
*[0]�洢����+[1]Կ�׸���+Կ�׼�¼(10)+[12]����+���ż�¼[30]
* flash�и����洢����ƫ�Ƶ�ַ�ͳ���
*********************************************************/
#define BLOCK_STORE_SIZE			64


#define	DEFAULT_PARAMS_OFFSET		0
#define DEFAULT_PARAMS_NUMBER		1

#define	KEY_STORE_OFFSET			DEFAULT_PARAMS_OFFSET + DEFAULT_PARAMS_NUMBER
#define KEY_STORE_LENGTH			1
#define	KEY_STORE_NUMBER			10

#define	RECORD_OFFSET				KEY_STORE_OFFSET + KEY_STORE_LENGTH + KEY_STORE_NUMBER
#define RECORD_LENGTH				1
#define	RECORD_NUMBER				30


#define BLOCK_STORE_COUNT			DEFAULT_PARAMS_NUMBER +\
									KEY_STORE_LENGTH + KEY_STORE_NUMBER +\
									RECORD_LENGTH + RECORD_NUMBER


extern pstorage_handle_t	block_id_flash_store;

extern uint8_t				next_block_id_site;
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

void key_store_write(struct key_store_struct *key_input);
void record_write(struct door_open_record *open_record);

#endif //INTER_FLASH_H__
