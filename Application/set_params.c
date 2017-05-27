

#include <stdint.h>
#include <stdbool.h>

#include "set_params.h"
#include "moto.h"


uint32_t 	BEEP_DIDI_NUMBER;
uint32_t 	LED_LIGHT_TIME;
uint32_t	DOOR_OPEN_HOLD_TIME;





void set_default_params(void)
{
	BEEP_DIDI_NUMBER = 5; //5��
	LED_LIGHT_TIME = 5;//ÿ��5*100ms
	DOOR_OPEN_HOLD_TIME = 10;//ÿ��0.5��
	OPEN_TIME = 25;
}





