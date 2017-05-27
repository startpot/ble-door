/**************************************************
*	���������ֲ�
*	T8---1			T9---2			T10--3
*	T7---4			T12--5			T11--6
*	T5---7			T1---8			T2---9
*	T6---*			T4---0			T3---#
*(#���ǿ�����)
**************************************************/

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "bsp.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_twi.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "app_util_platform.h"
#include "nrf_drv_config.h"

#include "touch_tsm12.h"

uint8_t key_value;

nrf_drv_twi_t	m_twi_master_touch	= NRF_DRV_TWI_INSTANCE(1); //ָ��TWI1

/***********************************************
*��ʼ��IIC
************************************************/
ret_code_t touch_iic_init(void)
{
	ret_code_t ret;
	const nrf_drv_twi_config_t config =
		{
			.scl                = TSM12_IIC_SCL_PIN,
			.sda                = TSM12_IIC_SDA_PIN,
			.frequency          = NRF_TWI_FREQ_100K,
			.interrupt_priority = APP_IRQ_PRIORITY_HIGH
		};

    do
    {
        ret = nrf_drv_twi_init(&m_twi_master_touch, &config, NULL, NULL);
        if(NRF_SUCCESS != ret)
        {
            break;
        }
        nrf_drv_twi_enable(&m_twi_master_touch);
    }while(0);
	
    return ret;
}

/**********************
*tsm12_en_start()
*********************/
void tsm12_en_start(void)
{	
	//����ʹ�ܶ�Ϊ���
	nrf_gpio_cfg_output(TSM12_IIC_EN_PIN);
	nrf_gpio_pin_clear(TSM12_IIC_EN_PIN);
}

/*************************
*tsm12_en_stop
*************************/
void tsm12_en_stop(void)
{
	//����ʹ�ܶ�Ϊ���
	nrf_gpio_cfg_output(TSM12_IIC_EN_PIN);	
	nrf_gpio_pin_set(TSM12_IIC_EN_PIN);
}

/**************************************
*i2c_device_write_byte
*дĳ����ַ������
****************************************/
ret_code_t touch_i2c_device_write_byte(uint8_t address, uint8_t data)
{
	ret_code_t ret;
	uint8_t buffer[2] ={address,data};
	ret = nrf_drv_twi_tx(&m_twi_master_touch, TSM12_IIC_REAL_ADDR, buffer, 2, false);
	
	return ret;
}

/************************************
*i2c_device_read_byte
*��ĳ����ַ��ʼ������
*************************************/
ret_code_t touch_i2c_device_read_byte(uint8_t address, uint8_t *p_read_byte, uint8_t length)
{	
	ret_code_t ret;
	
	do
	{
		//д��ַ
		uint8_t set_address;
		set_address = address;
		
		ret = nrf_drv_twi_tx(&m_twi_master_touch, TSM12_IIC_REAL_ADDR, &set_address, 1, true);
		if(ret !=NRF_SUCCESS)
		{
			break;
		}
		//������
		ret = nrf_drv_twi_rx(&m_twi_master_touch, TSM12_IIC_REAL_ADDR, p_read_byte, length);
	}while(0);
		
	return ret;
}

/******************************************
*��ʼ��IICоƬ
*
*******************************************/
void tsm12_init(void)
{
	uint8_t set_data;
	//ʹ��IIC�ܽ�
	tsm12_en_start();
	
	touch_iic_init();
	
	//�����λ��˯��ģʽ��
	set_data = 0x0F;
	touch_i2c_device_write_byte(TSM12_CTRL2, set_data);
	//ʹ�������λ��˯��ģʽ��
	set_data = 0x07;
	touch_i2c_device_write_byte(TSM12_CTRL2, set_data);
	//����ͨ��1-2��������
	set_data = 0xBB;
	touch_i2c_device_write_byte(TSM12_Sensitivity1, set_data);
	//����ͨ��3-4��������
	touch_i2c_device_write_byte(TSM12_Sensitivity2, set_data);
	//����ͨ��5-6��������
	touch_i2c_device_write_byte(TSM12_Sensitivity3, set_data);
	//����ͨ��7-8��������
	touch_i2c_device_write_byte(TSM12_Sensitivity4, set_data);
	//����ͨ��9-10��������
	touch_i2c_device_write_byte(TSM12_Sensitivity5, set_data);
	//����ͨ��11-12��������
	touch_i2c_device_write_byte(TSM12_Sensitivity6, set_data);
	
		//��������
//	set_data = 0x22;
//	i2c_device_write_byte(TSM12_CTRL1, set_data);
	
	
	//����λͨ��1-8�Ĳο�
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ref_rst1, set_data);
	//����λͨ��9-12�Ĳο�
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ref_rst2, set_data);
	
	/*
	//ʹ��ͨ��1-8��λ
	set_data = 0x00;
	i2c_device_write_byte(TSM12_Cal_hold1, set_data);
	//ʹ��ͨ��9-12��λ
	set_data = 0x00;
	i2c_device_write_byte(TSM12_Cal_hold2, set_data);
	*/
	
	//��1-8����ͨ��
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ch_hold1, set_data);
	//��9-12����ͨ��
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ch_hold2, set_data);
	printf("touch button ic:tsm12 init success\r\n");
}

/***************************************************
*key_read(),��ȡ��ֵ
***************************************************/
uint8_t tsm12_key_read(void)
{
	uint8_t temp[3];
	uint8_t *p;
	
	p = temp;
	
	//��ȡͨ��1-4
	touch_i2c_device_read_byte(TSM12_Output1, p, 0x01);
	//��ȡͨ��5-8
	touch_i2c_device_read_byte(TSM12_Output2, (p+1), 0x01);
	//��ȡͨ��9-12
	touch_i2c_device_read_byte(TSM12_Output3, (p+2), 0x01);
	
	if(*p > 2)
	{
		switch(*p)
		{
			case 0x03://00000011
				key_value = '8';
				break;
			case 0x0c://00001100
				key_value = '9';
				break;
			case 0x30://00110000
				key_value = 'b';
				break;
			case 0xc0://11000000
				key_value = '0';
				break;
			default:
				key_value = 0;
				break;
		}
	}
	else if(*(p + 1) > 2)
	{
		switch(*(p + 1))
		{
			case 0x03:
				key_value = '7';
				break;
			case 0x0c:
				key_value = 'a';
				break;
			case 0x30:
				key_value = '4';
				break;
			case 0xc0:
				key_value = '1';
				break;
			default:
				key_value = 0;
				break;
		}
	}
	else
	{
		switch(*(p + 2))
		{
			case 0x03:
				key_value = '2';
				break;
			case 0x0c:
				key_value = '3';
				break;
			case 0x30:
				key_value = '6';
				break;
			case 0xc0:
				key_value = '5';
				break;
			default:
				key_value = 0;
				break;
		}
	}
	return key_value;
}
