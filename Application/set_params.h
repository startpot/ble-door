#ifndef SET_PARAMS_H__
#define SET_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>

#include "pstorage.h"

extern pstorage_handle_t block_id_params;
extern uint8_t flash_store_params[8];

extern uint8_t 	BEEP_DIDI_NUMBER;
extern uint8_t 	LED_LIGHT_TIME;
extern uint8_t	DOOR_OPEN_HOLD_TIME;
extern uint8_t	KEY_CHECK_NUMBER;
extern uint8_t key_length_set;


void set_default_params(void);

#endif	//SET_PARAMS_H__
