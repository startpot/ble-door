#ifndef	BEEP_H__
#define	BEEP_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


#define	BEEP_OPEN			500		//��������ʱ��(us)
#define	BEEP_CLOSE		500		//�������ر�ʱ��(us)

#define BEEP_DIDI_ONCE_TIME	50	//����������һ������

void beep_init(void);
void beep_didi(uint8_t number);


#endif	//BEEP_H__
