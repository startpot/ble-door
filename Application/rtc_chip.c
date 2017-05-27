/***************************
*	RTCоƬ IIC�ӿ�
*--------------------------------------------------------------------------------------
*	tm														RTC							
*	tm_sec(int:0-60)							02H(BCD:0-59:[6:0])
*	tm_min(int:0-59)							03H(BCD:0-59:[6:0])
*	tm_hour(int:0-23)							04H(BCD:0-23:[5:0])
*	tm_mday(int:1-31)							05H(BCD:1-31:[5:0])
*	tm_mon(int:0-11)							07H(BCD:1-12:[4:0],7��ʾ����)
*	tm_year(int:Year-1990)						08H(BCD:0-99)
*	tm_wday(int:0-6)							06H(int:0-6)		sun-sat
*	tm_yday(0-365)
*	tm_isdst(����ʱ)
****************************/

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "bsp.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_twi.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "app_util_platform.h"
#include  "nrf_drv_config.h"

#include "rtc_chip.h"

nrf_drv_twi_t m_twi_master_rtc = NRF_DRV_TWI_INSTANCE(1); //ָ��TWI1

/***********************
*��ʼ��RTCоƬ
************************/
static ret_code_t rtc_iic_init(void)
{
	ret_code_t ret;
  const nrf_drv_twi_config_t config =
    {
       .scl                = RTC_CHIP_IIC_SCL_PIN,
       .sda                = RTC_CHIP_IIC_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    do
    {
        ret = nrf_drv_twi_init(&m_twi_master_rtc, &config, NULL, NULL);
        if(NRF_SUCCESS != ret)
        {
            break;
        }
        nrf_drv_twi_enable(&m_twi_master_rtc);
    }while(0);
    return ret;
}

/*********************
*RTCоƬд��1��byte
*********************/
static ret_code_t rtc_i2c_device_write_byte(uint8_t address, uint8_t data)
{
	ret_code_t ret;
	uint8_t buffer[2] ={address,data};
	ret = nrf_drv_twi_tx(&m_twi_master_rtc, RTC_CHIP_REAL_ADDR, buffer, 2, false);
	return ret;
}

/***********************
*RTCоƬ����length��byte
***********************/
static ret_code_t rtc_i2c_device_read_byte(uint8_t address, uint8_t *p_read_byte, uint8_t length)
{
	ret_code_t ret;
	
	do
	{
	//д��ַ
	uint8_t set_address;
	set_address = address;
	ret = nrf_drv_twi_tx(&m_twi_master_rtc, RTC_CHIP_REAL_ADDR, &set_address, 1, true);
	if(ret !=NRF_SUCCESS)
	{
		break;
	}
	//������
	ret = nrf_drv_twi_rx(&m_twi_master_rtc, RTC_CHIP_REAL_ADDR, p_read_byte, length);
	}while(0);
	return ret;
}

/******************
*	��ʼ��RTCоƬ
******************/
void rtc_init(void)
{
	//��ʼ��RTC��IIC
	rtc_iic_init();
	
	//ʹ��RTCоƬ
	rtc_i2c_device_write_byte(PCF85163_Timer_control_ADDR, 0x83);
	rtc_i2c_device_write_byte(PCF85163_Timer_ADDR, 0xff);
	printf("rtc:pcf85163 init success\r\n");
}

static uint8_t hex_2_bcd(int value)
{
	return (((value/10)<<4) | (value%10));
}

static int bcd_2_hex(int value)
{
	return (((value & 0xf0)>>4)*10 + (value & 0x0f));
}

/******************
*����RTCоƬ��ʱ��
*******************/
uint8_t	rtc_time_write(struct tm *time_write)
{
	
	uint8_t byte_write;
	//ֹͣRTC
	rtc_i2c_device_write_byte(PCF85163_Timer_control_ADDR, 0x03);
	//д��
	byte_write = 0x7f & hex_2_bcd(time_write->tm_sec);
	rtc_i2c_device_write_byte(PCF85163_VL_seconds_ADDR, byte_write);
	//д��
	byte_write = 0x7f & hex_2_bcd(time_write->tm_min);
	rtc_i2c_device_write_byte(PCF85163_Minutes_ADDR, byte_write);
	//дСʱ
	byte_write = 0x3f & hex_2_bcd(time_write->tm_hour);
	rtc_i2c_device_write_byte(PCF85163_Hours_ADDR, byte_write);
	//д��
	byte_write = 0x3f & hex_2_bcd(time_write->tm_mday);
	rtc_i2c_device_write_byte(PCF85163_Days_ADDR, byte_write);
	//д��
	byte_write = 0x07 & hex_2_bcd(time_write->tm_wday);
	rtc_i2c_device_write_byte(PCF85163_Weekdays_ADDR, byte_write);
	//д�� 2xxx��
	byte_write = (0x1f & hex_2_bcd(time_write->tm_mon + 1)) + 0x80;
	rtc_i2c_device_write_byte(PCF85163_Century_months_ADDR, byte_write);
	//д��
	byte_write = 0xff & hex_2_bcd(time_write->tm_year - 10);
	rtc_i2c_device_write_byte(PCF85163_Years_ADDR, byte_write);
	//ʹ��RTC
	rtc_i2c_device_write_byte(PCF85163_Timer_control_ADDR, 0x83);
	printf("rtc time set:%4dyear%2d month%2dday%2d:%2d:%2d\r\n",\
			time_write->tm_year +1990, time_write->tm_mon + 1, \
			time_write->tm_mday, time_write->tm_hour, \
			time_write->tm_min, time_write->tm_sec);
	return 0;
}

/*********************
*����RTCоƬ��ʱ��
*********************/
uint8_t rtc_time_read(struct tm *time_read)
{
	uint8_t byte_read[7];
	//��ʱ��
	rtc_i2c_device_read_byte(PCF85163_VL_seconds_ADDR, byte_read, 7);
	
	//д����
	time_read->tm_sec = bcd_2_hex(0x7f & byte_read[0]);
	//д���
	time_read->tm_min = bcd_2_hex(0x7f & byte_read[1]);
	//д��ʱ
	time_read->tm_hour = bcd_2_hex(0x3f & byte_read[2]);
	//д����
	time_read->tm_mday = bcd_2_hex(0x3f & byte_read[3]);
	//д����
	time_read->tm_wday = bcd_2_hex(0x07 & byte_read[4]);
	//д����
	time_read->tm_mon = bcd_2_hex(0x1f & byte_read[5]) - 1;
	//д����
	time_read->tm_year = bcd_2_hex(0xff & byte_read[6]) + 10;
	printf("rtc time read:%4dyear%2dmonth%2dday%2d:%2d:%2d\r\n",\
			time_read->tm_year +1990, time_read->tm_mon + 1, \
			time_read->tm_mday, time_read->tm_hour, \
			time_read->tm_min, time_read->tm_sec);
	return 0;
}
