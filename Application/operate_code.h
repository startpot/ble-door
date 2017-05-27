#ifndef OPERATE_CODE_H__
#define OPERATE_CODE_H__


#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "inter_flash.h"
#include "ble_nus.h"

extern bool			id_key_store_setted;
extern bool			key_store_time_setted;
extern bool			key_use_time_setted;

//与设置密码一系列动作相关的变量，一步完成不了，需要设置全局变量
extern uint8_t 		id_num_set;

extern uint8_t 		key_store_set[8];
extern uint8_t 		key_length_set;

extern struct tm 	key_store_time_set;
extern time_t 		key_use_time_set;
extern struct key_store_struct 		key_store_struct_set;

extern uint8_t data_array_store[BLE_NUS_MAX_DATA_LEN];//20位
extern uint32_t 	data_send_length;//测试数据存储时，长度的全局变量
extern uint8_t huge_data_store[BLOCK_STORE_SIZE];



#define OPERATE_CODE_LENGHT		8			//命令的长度
//get time ------(hex)01(秒)02(分)03(时)04(天)05(月份 6月)1B(年 1990+27=2017年)00(周)
#define GET_TIME				"get time"		//获取时间
//sync time:(hex)01(秒)02(分)03(时)04(天)05(月份 6月)1B(年 1990+27=2017年)00(周) ---OK
#define SYNC_TIME				"syn time"		//同步时间

/*********设置密码是一个系列动作********/

//set key:1(hex:密码ID号)0123456789(数字用ASCII表示,主要是为了区分字符串结束符'\0',最多10位)---OK
#define SET_KEY_STORE			"set keys"		//设置密码
//set KStm:(hex)01(秒)02(分)03(时)04(天)05(月份 6月)1B(年 1990+27=2017年)00(周) ---OK
#define SET_KEY_STORE_TIME		"set kstm"		//设置密码的存储时间
//set KUtm:(hex)ffffffff(最长时间,小端在前,按字节区分)--------OK
#define SET_KEY_USE_TIME		"set kutm"		//设置密码的使用时间


//设置参数命令
#define SET_LIGHT_TIME			"set litt"		//设置亮灯时间n * 100ms
#define	SET_BEEP_TIME			"set beep"		//设置蜂鸣器响次数
#define SET_DOOR_HOLD_TIME		"set dhld"		//设置门打开的时间 n*100ms
#define SET_MOTO_OPEN_TIME		"set moto"		//设置电机转动时间 n*100ms
#define SET_BATT_VOL			"set batb"		//设置电池电压报警时间 n*100ms



//测试命令
#define SET_DATA				"set data"//发送数据
#define	GET_DATA				"get data"//获取数据
#define GET_RECD				"get recd"//获取最近一次记录


/**********************************
* 数据包的分析
***********************************/
#define OPERATE_CODE_BIT		0


void mac_set(void);
void operate_code_check(uint8_t *p_data, uint16_t length);



#endif  //OPERATE_CODE_H__

