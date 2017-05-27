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

//����������һϵ�ж�����صı�����һ����ɲ��ˣ���Ҫ����ȫ�ֱ���
extern uint8_t 		id_num_set;

extern uint8_t 		key_store_set[8];
extern uint8_t 		key_length_set;

extern struct tm 	key_store_time_set;
extern time_t 		key_use_time_set;
extern struct key_store_struct 		key_store_struct_set;

extern uint8_t data_array_store[BLE_NUS_MAX_DATA_LEN];//20λ
extern uint32_t 	data_send_length;//�������ݴ洢ʱ�����ȵ�ȫ�ֱ���
extern uint8_t huge_data_store[BLOCK_STORE_SIZE];



#define OPERATE_CODE_LENGHT		8			//����ĳ���
//get time ------(hex)01(��)02(��)03(ʱ)04(��)05(�·� 6��)1B(�� 1990+27=2017��)00(��)
#define GET_TIME				"get time"		//��ȡʱ��
//sync time:(hex)01(��)02(��)03(ʱ)04(��)05(�·� 6��)1B(�� 1990+27=2017��)00(��) ---OK
#define SYNC_TIME				"syn time"		//ͬ��ʱ��

/*********����������һ��ϵ�ж���********/

//set key:1(hex:����ID��)0123456789(������ASCII��ʾ,��Ҫ��Ϊ�������ַ���������'\0',���10λ)---OK
#define SET_KEY_STORE			"set keys"		//��������
//set KStm:(hex)01(��)02(��)03(ʱ)04(��)05(�·� 6��)1B(�� 1990+27=2017��)00(��) ---OK
#define SET_KEY_STORE_TIME		"set kstm"		//��������Ĵ洢ʱ��
//set KUtm:(hex)ffffffff(�ʱ��,С����ǰ,���ֽ�����)--------OK
#define SET_KEY_USE_TIME		"set kutm"		//���������ʹ��ʱ��


//���ò�������
#define SET_LIGHT_TIME			"set litt"		//��������ʱ��n * 100ms
#define	SET_BEEP_TIME			"set beep"		//���÷����������
#define SET_DOOR_HOLD_TIME		"set dhld"		//�����Ŵ򿪵�ʱ�� n*100ms
#define SET_MOTO_OPEN_TIME		"set moto"		//���õ��ת��ʱ�� n*100ms
#define SET_BATT_VOL			"set batb"		//���õ�ص�ѹ����ʱ�� n*100ms



//��������
#define SET_DATA				"set data"//��������
#define	GET_DATA				"get data"//��ȡ����
#define GET_RECD				"get recd"//��ȡ���һ�μ�¼


/**********************************
* ���ݰ��ķ���
***********************************/
#define OPERATE_CODE_BIT		0


void mac_set(void);
void operate_code_check(uint8_t *p_data, uint16_t length);



#endif  //OPERATE_CODE_H__

