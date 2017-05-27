#ifndef MOTO_H__
#define MOTO_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

extern uint32_t OPEN_TIME;


void moto_init(void);
void moto_forward_ms(uint32_t ms);
void moto_backward_ms(uint32_t ms);

void moto_open(uint32_t open_time);
void moto_close(uint32_t close_time);

#endif //MOTO_H__
