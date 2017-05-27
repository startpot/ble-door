#include <stdint.h>
#include "bsp.h"
#include "pstorage.h"
#include "app_error.h"

#include "inter_flash.h"

pstorage_handle_t	block_id_flash_store;

uint8_t				next_block_id_site;
pstorage_handle_t	block_id_dest;

uint32_t				key_store_length;
uint32_t				record_length;


bool key_store_full;
bool record_full;


static void my_cb(pstorage_block_t *handle, uint8_t op_code, uint32_t result, uint8_t *p_data, uint32_t data_len)
{
	switch(op_code)
	{
		case PSTORAGE_LOAD_OP_CODE:
			if (result != NRF_SUCCESS)
			{
				printf("pstorage LOAD ERROR callback received \r\n");
			}
			break;
		case PSTORAGE_STORE_OP_CODE:
			if (result != NRF_SUCCESS)
			{
				printf("pstorage STORE ERROR callback received \r\n");
			}
			break;				 
		case PSTORAGE_UPDATE_OP_CODE:
			if (result != NRF_SUCCESS)
			{
				printf("pstorage UPDATE ERROR callback received \r\n");
			}
			break;
		case PSTORAGE_CLEAR_OP_CODE:
			if (result != NRF_SUCCESS)
			{
				printf("pstorage CLEAR ERROR callback received \r\n");
			}
			break;
		}			
}

/******************************
*初始化内部flash空间
******************************/
void flash_init(void)
{
	uint32_t err_code;
	
	pstorage_init(); //初始化flash操作
	
	//初始化key_store的空间
	pstorage_module_param_t module_param_key_store;
	module_param_key_store.block_count = BLOCK_STORE_COUNT;//申请11个块
	module_param_key_store.block_size = BLOCK_STORE_SIZE; //每块大小64byte 最长的钥匙结构体为60byte
	module_param_key_store.cb = (pstorage_ntf_cb_t)my_cb;
	
	err_code = pstorage_register(&module_param_key_store, &block_id_flash_store);
	APP_ERROR_CHECK(err_code);
	printf("pstorage register: name:block_id_flash_store.\r\n" );
	printf("it has %i blocks and block size is %i \r\n",\
			module_param_key_store.block_count, module_param_key_store.block_size);
	//写钥匙记录条数为0
	err_code = pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)KEY_STORE_OFFSET, &block_id_dest);
	APP_ERROR_CHECK(err_code);
	key_store_length = 0x00000000;
	err_code = pstorage_clear(&block_id_dest,64);
	APP_ERROR_CHECK(err_code);
	err_code = pstorage_store(&block_id_dest, (uint8_t *)&key_store_length, 4, 0);
	APP_ERROR_CHECK(err_code);
	printf("flash_init, key_store length set %d\r\n", key_store_length);
	//写开门记录条数为0
	pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)RECORD_OFFSET, &block_id_dest);
	record_length = 0x0;
	pstorage_clear(&block_id_dest,64);
	pstorage_store(&block_id_dest, (uint8_t *)&record_length, 4, 0);
	printf("flash init, record length set %d\r\n", record_length);
	
	printf("flash init success \r\n");
}

/*******************************************
*存储到flash
********************************************/
void inter_flash_write(uint8_t *p_data, uint32_t data_len,\
					   pstorage_size_t block_id_offset, pstorage_handle_t *block_id_write)
{	
	//获取需要存储的位置
	pstorage_block_identifier_get(block_id_write, block_id_offset, &block_id_dest);
	//清除当前存储区域
	pstorage_clear(&block_id_dest, 64);
	pstorage_store(&block_id_dest, p_data, (pstorage_size_t)data_len, 0);
	printf("%2d byte data have been store in flash offset:%i\r\n", data_len, block_id_offset);
}

/**********************************
*将flash中的数据读出来
***********************************/
void inter_flash_read(uint8_t *p_data, uint32_t data_len, \
					 pstorage_size_t block_id_offset, pstorage_handle_t *block_id_read)
{
	pstorage_block_identifier_get(block_id_read, (pstorage_size_t)block_id_offset, &block_id_dest);
	pstorage_load(p_data, &block_id_dest, (pstorage_size_t)data_len, 0);
	printf("%2d byte data have been read in flash offset:%i\r\n", data_len, block_id_offset);
}

/********************************
*将钥匙存储在flash中
********************************/
void key_store_write(struct key_store_struct *key_input)
{
	//写记录条数
	pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)KEY_STORE_OFFSET, &block_id_dest);
	pstorage_load((uint8_t *)&key_store_length, &block_id_dest, 4, 0);
	if(key_store_length > KEY_STORE_NUMBER)
	{//达到记录上限
		key_store_length = 0x1;
		pstorage_clear(&block_id_dest, 64);
		pstorage_store(&block_id_dest, (uint8_t *)&key_store_length, 4, 0);
		key_store_full = true;
	}
	else
	{//未达到记录上限
		key_store_length++;
		pstorage_clear(&block_id_dest, 64);
		pstorage_store(&block_id_dest, (uint8_t *)&key_store_length, 4, 0);
	}
	inter_flash_write((uint8_t *)key_input, sizeof(struct key_store_struct), \
						(pstorage_size_t)(KEY_STORE_OFFSET + key_store_length), &block_id_flash_store);
}

/****************************
*将记录存储在flash中
****************************/
void record_write(struct door_open_record *open_record)
{
	//写记录条数
	pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)RECORD_OFFSET, &block_id_dest);
	pstorage_load((uint8_t *)&record_length, &block_id_dest, 4, 0);
	if(record_length > RECORD_NUMBER)
	{//达到记录上限
		record_length = 0x1;
		pstorage_clear(&block_id_dest, 64);
		pstorage_store(&block_id_dest, (uint8_t *)&record_length, 4, 0);	
		record_full = true;
	}
	else
	{//未达到记录上限
		record_length++;
		pstorage_clear(&block_id_dest, 64);
		pstorage_store(&block_id_dest, (uint8_t *)&record_length, 4, 0);
	}
	inter_flash_write((uint8_t *)open_record, sizeof(struct door_open_record), \
						(pstorage_size_t)(RECORD_OFFSET + record_length), &block_id_flash_store);
}
