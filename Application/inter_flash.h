#ifndef INTER_FLASH_H__
#define INTER_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "pstorage.h"

//存储的钥匙
struct key_store_struct
{
	uint8_t		id_num;
	uint8_t 	key_length;
	uint8_t 	key_store[8];//ASCII,最多8位
	struct tm 	key_store_time;
	time_t 		key_use_time;
};
//开门记录
struct door_open_record
{
	uint8_t		key_store[8];
	struct tm	door_open_time;//门打开的时间
};

/********************************************************
*flash存储空间整个一个block_id
*[0]存储参数+[1]钥匙个数+钥匙记录(10)+[12]长度+开门记录[30]
* flash中各个存储量的偏移地址和长度
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
