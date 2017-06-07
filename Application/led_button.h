#ifndef LED_BUTTON_H__
#define LED_BUTTON_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "app_timer.h"


#include "inter_flash.h"

//与动态口令相关的参量
#define SM4_INTERVAL		60
#define SM4_COUNTER			1234


extern char key_express_value;

#define KEY_NUMBER		12
extern char 	key_input[KEY_NUMBER];
extern uint8_t 	key_input_site;

//输入的密码的时间
extern struct tm key_input_time_tm;
extern time_t key_input_time_t;
//种子的数组
extern uint8_t seed[16];

//对比动态密码的变量
extern uint8_t SM4_challenge[4];
extern uint8_t key_store_tmp[6];
extern struct key_store_struct key_store_check;


//存储在flash的密码
extern uint8_t flash_key_store[BLOCK_STORE_SIZE];

extern struct door_open_record		open_record_now;


#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)

void leds_init(void);
void leds_on(uint8_t led_pin, uint32_t ms);

void write_key_expressed(void);
void clear_key_expressed(void);
void ble_door_open(void);
void iic_int_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low);
void iic_int_buttons_init(void);

#endif  //LED_BUTTON_H__
