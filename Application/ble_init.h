#ifndef BLE_INIT_H__
#define	BLE_INIT_H__


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "app_timer.h"
#include "ble_nus.h"



#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device. */


//����2�����������SD�����пռ�(app_ram_base.h�ж���)���Ӷ����������ʱ��APP�����пռ�����
#define CENTRAL_LINK_COUNT              0                                           /**<number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT           1                                           /**<number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define DEVICE_NAME                     "Nordic_UART"          	//�����豸���ƣ������㲥�������豸������
#define MANUFACTURER_NAME               "NordicSemiconductor"   //�豸�����̣�Will be passed to Device Information Service             
#define MODEL_NUMBER                   	"nRF51"                 // �ͺ��ַ���. Will be passed to Device Information Service.
#define MANUFACTURER_ID                	0x55AA55AA55           	//�豸������ID(���޸�Ϊ�Լ���). Will be passed to Device Information Service.
#define ORG_UNIQUE_ID                  	0xEEBBEE               	//BLE��֯������Ψһ��ID. Will be passed to Device Information Service

// UUID type for the Nordic UART Service (vendor specific)����Ҫ�ǿ����ùٷ���APP����
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  

#define APP_ADV_INTERVAL                400          //�㲥���(0.625 ms * 400 = 250 ms)���㲥���Խ��Խʡ��
#define APP_ADV_TIMEOUT_IN_SECONDS      180      //�㲥��ʱ����λs

#define APP_TIMER_PRESCALER             0          // Value of the RTC1 PRESCALER register
#define APP_TIMER_OP_QUEUE_SIZE         5          //Size of timer operation queues

#define SECURITY_REQUEST_DELAY         	APP_TIMER_TICKS(4000, APP_TIMER_PRESCALER)


#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  0                                          	/**< Perform not bonding. */
#define SEC_PARAM_MITM                  1                                          	/**< Man In The Middle protection required. */
#define SEC_PARAM_IO_CAPABILITIES      	BLE_GAP_IO_CAPS_DISPLAY_ONLY//BLE_GAP_IO_CAPS_NONE                       	/**< No I/O capabilities. */
#define SEC_PARAM_OOB                  	0                                          	/**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE         	7                                          	/**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE         	16                                         	/**< Maximum encryption key size. */
APP_TIMER_DEF(m_sec_req_timer_id);

#define PASSKEY_TXT                    	"Passkey:"                                  /**< Message to be displayed together with the pass-key. */
#define PASSKEY_TXT_LENGTH             	8                                           /**< Length of message to be displayed together with the pass-key. */
#define PASSKEY_LENGTH                 	6                                           /**< Length of pass-key received by the stack for display. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED      	BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */


extern ble_nus_t                        m_nus; /*Nordic UART Service*/

//����3����������uart service�б����ȫ�ֱ���������operate_code_check����ȥ����
extern bool    							operate_code_setted;
extern uint8_t							nus_data_array[BLE_NUS_MAX_DATA_LEN];
extern uint16_t  						nus_data_array_length;

//�ڱ����ʱ�򣬴��������ӣ���ble�Ļص�����
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name);

//BLE���������ú���
void timers_init(void);
void application_timers_start(void);
void gap_params_init(void);
void services_init(void);
void conn_params_init(void);
void ble_stack_init(void);
void advertising_init(void);
void power_manage(void);
void device_manager_init(bool erase_bonds);
//void buttons_leds_init(bool * p_erase_bonds);

//��BLE�в��ַ��룬���հ���UART0�����ݣ�ͨ���������д����ȥ
//��ʼ��uart�����������ϵ�applicationʹ�ã�(���̵ı���ѡ����NRF_LOG_USES_UART=1 )
void uart_init(void);


#endif		//BLE_INIT_H__


