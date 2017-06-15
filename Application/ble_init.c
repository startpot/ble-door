#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "nordic_common.h"
#include "bsp.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_config.h"

#include "ble_hci.h"
#include "ble_dis.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "pstorage.h"
#include "device_manager.h"
#include "app_trace.h"
#include "app_util_platform.h"

#include "ble_init.h"
#include "inter_flash.h"

dm_application_instance_t 				m_app_handle;
dm_handle_t											m_dm_handle;

ble_uuid_t                       	m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}};
uint16_t                         		m_conn_handle = BLE_CONN_HANDLE_INVALID;

ble_nus_t                        	m_nus;//ble 服务注册的nus服务

uint8_t mac[8];//第一位：标志位，第二位：长度
uint8_t device_name[20];//[0]标记位0x77，[1]长度[2...]名字

//自定义的nus服务中data_handle函数中暂存的数据，需要交给check命令
bool    			operate_code_setted = false;
uint8_t			nus_data_array[BLE_NUS_MAX_DATA_LEN];
uint16_t  		nus_data_array_length;

/**********************************************
*nrf回调函数
*in		line_num		错误行数
			p_file_name	错误文件名称
**********************************************/
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**************************************************
*配对超时处理函数
*in：		p_context	超时描述
**************************************************/
static void sec_req_timeout_handler(void * p_context)
{
    uint32_t             err_code;
    dm_security_status_t status;

    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        err_code = dm_security_status_req(&m_dm_handle, &status);
        APP_ERROR_CHECK(err_code);

        // In case the link is secured by the peer during timeout, the request is not sent.
        if (status == NOT_ENCRYPTED)
        {
            err_code = dm_security_setup_req(&m_dm_handle);
            APP_ERROR_CHECK(err_code);
        }
    }
}

/***************************************************
*定时器初始化
****************************************************/
void timers_init(void)
{
	uint32_t err_code;
    // Initialize timer module.
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
    // Create timers.
	// Create Security Request timer.
    err_code = app_timer_create(&m_sec_req_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                sec_req_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/******************************
*定时器使能
******************************/
void application_timers_start(void)
{
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.
    uint32_t err_code;
    err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code); */
}

/**************************************
* GAP初始化
**************************************/
void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
	//如果flash中存的名字有效，则使用flash中名字
	if(device_name[0] == 0x77)
	{
		//有效，使用新名字，device_name[1]为长度
#if defined(BLE_DOOR_DEBUG)
		printf("update device name \r\n");
#endif
		err_code = sd_ble_gap_device_name_set(&sec_mode,\
											(const uint8_t *)device_name +2,\
											device_name[1]);
	}
	else
	{
#if defined(BLE_DOOR_DEBUG)
		printf("default device name \r\n");
#endif
		err_code = sd_ble_gap_device_name_set(&sec_mode,\
                                          (const uint8_t *) DEVICE_NAME,\
                                          strlen(DEVICE_NAME));
	}
	APP_ERROR_CHECK(err_code);
	
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);										 
}

/***************************************************************************
*BLE uart服务处理函数
*in：		*p_nus		uart服务
*				p_data		ble uart服务接受到的数据指针
				length			数据的长度
***************************************************************************/
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
/*
#if defined(BLE_DOOR_DEBUG)
	//将获取的指令，通过UART发送出去，用板子的UART做BLE的nus service的调试口
	for (uint32_t i = 0; i < length; i++)
	{
		while(app_uart_put(p_data[i]) != NRF_SUCCESS);
	}
	while(app_uart_put('\n') != NRF_SUCCESS);
#endif
*/	
	//将获取的数据存到全局变量，供operate_code_check函数用
	for(int i = 0; i <length; i++)
	{
		nus_data_array[i] = p_data[i];
	}
	nus_data_array_length = length;
	operate_code_setted = true;
}

/*******************************************************
* 注册SD服务uart 和device information
 ******************************************************/
void services_init(void)
{
	uint32_t       err_code;
	ble_nus_init_t nus_init;
	ble_dis_init_t dis_init;
	//Initialize Nordic UART Service
	memset(&nus_init, 0, sizeof(nus_init));
		
	nus_init.data_handler = nus_data_handler;
		
	err_code = ble_nus_init(&m_nus, &nus_init);
	APP_ERROR_CHECK(err_code);
	// Initialize Device Information Service.
	memset(&dis_init, 0, sizeof(dis_init));

	ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, MANUFACTURER_NAME);
	ble_srv_ascii_to_utf8(&dis_init.serial_num_str, MODEL_NUMBER);

	ble_dis_sys_id_t system_id;
    system_id.manufacturer_id            = MANUFACTURER_ID;
    system_id.organizationally_unique_id = ORG_UNIQUE_ID;
    dis_init.p_sys_id                    = &system_id;
    
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}

/*****************************************
*连接参数管理处理函数
*in：		*p_evt	连接参数指针
*****************************************/
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;
    
    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**********************************************
*连接参数初始化错误处理函数
***********************************************/
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/***********************************************
* 初始化连接参数
**********************************************/
void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    
    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;
    
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/***********************************************
* 芯片进入低功耗
************************************************/
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
   // err_code = bsp_btn_ble_sleep_mode_prepare();
   // APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/*******************************************
*广播的处理函数
********************************************/
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}

/******************************
*协议栈处理函数
*******************************/
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t err_code = NRF_SUCCESS;              

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
		case BLE_GAP_EVT_DISCONNECTED:
			m_conn_handle = BLE_CONN_HANDLE_INVALID;
			dm_device_delete_all(&m_app_handle);
			break;
        case BLE_GAP_EVT_AUTH_STATUS:
			//判断配对是否成功，如果不成功断开连接，从而阻止其他人任意连接
			if(p_ble_evt->evt.gap_evt.params.auth_status.auth_status != BLE_GAP_SEC_STATUS_SUCCESS)
			{
				sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			}
			else
			{
#if defined(BLE_DOOR_DEBUG)
				printf("pair success\r\n");
#endif
			}
			break;
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
			//no system attributes have been stored
			err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
        case BLE_GAP_EVT_CONN_SEC_UPDATE:
            break;

        default:
            // No implementation needed.
            break;
    }
}
/******************************************************
*事件处理函数，处理手机发过来的新名字
*******************************************************/
static void device_name_change(ble_evt_t * p_ble_evt)
{
	ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	
	//通过UUID来判断事件是不是写Generic Access服务中的名字属性
	 if((p_evt_write->context.char_uuid.uuid == BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME) && \
					(p_ble_evt->header.evt_id == BLE_GATTS_EVT_WRITE))
	 {
#if defined(BLE_DOOR_DEBUG)
		printf("device name change\r\n");
#endif
		device_name[0] = 0x77;
		device_name[1] = p_evt_write->len;
		memcpy(device_name + 2, p_evt_write->data, p_evt_write->len);
		 
		//将获取的名字存储在flash
		pstorage_block_identifier_get(&block_id_flash_store, (pstorage_size_t)DEVICE_NAME_OFFSET, &block_id_device_name);
		pstorage_clear(&block_id_device_name, DEVICE_NAME_SIZE);
		pstorage_store(&block_id_device_name, device_name, DEVICE_NAME_SIZE, 0);
	 }
}

//因为广播函数是在后面定义的，使用的话，先定义
void advertising_init(void);

/************************************************
*协议栈事件分发函数
************************************************/
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
	//增加接受device_name的事件处理函数
	device_name_change(p_ble_evt);
	
	//在断开连接事件后，初始化广播数据
	if(p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED)
	{
		advertising_init();
	}
	
	dm_ble_evt_handler(p_ble_evt);
	ble_conn_params_on_ble_evt(p_ble_evt);
	on_ble_evt(p_ble_evt);
	ble_advertising_on_ble_evt(p_ble_evt);
	//自己的服务
	ble_nus_on_ble_evt(&m_nus, p_ble_evt);
}

/***************************************************
*系统事件分发函数
**************************************************/
static void sys_evt_dispatch(uint32_t sys_evt)
{
	pstorage_sys_event_handler(sys_evt);
	ble_advertising_on_sys_evt(sys_evt);
}

/****************************
* 初始化BLE,检查APP地址，
* 注册BLE中断函数和SYS中断
*****************************/
void ble_stack_init(void)
{
    uint32_t err_code;
    
    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);
    
    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);
        
    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);
    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
	
	// Register with the SoftDevice handler module for System events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

/*******************************************
*广播初始化函数
********************************************/
void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, &scanrsp, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

/***********************************************
*主函数中，低功耗状态，等待event
***********************************************/
void power_manage(void)
{
	 uint32_t err_code = sd_app_evt_wait();
	 APP_ERROR_CHECK(err_code);	
}

/*****************************************************
*DM处理函数
******************************************************/
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_event,
                                           ret_code_t        event_result)
{
    uint32_t err_code = NRF_SUCCESS;

    m_dm_handle = *p_handle;
    APP_ERROR_CHECK(event_result);
    switch (p_event->event_id)
    {
        case DM_EVT_CONNECTION:
            // Start Security Request timer.
    //        if (m_dm_handle.device_id != DM_INVALID_ID)
            {
                err_code = app_timer_start(m_sec_req_timer_id, SECURITY_REQUEST_DELAY, NULL);
                APP_ERROR_CHECK(err_code);
            }
            break;
		case DM_EVT_DISCONNECTION:
			//dm_device_delete_all(&m_dm_handle);
			break;
		case DM_EVT_SECURITY_SETUP:
			
			break;
		case DM_EVT_SECURITY_SETUP_COMPLETE:
			
			break;
		case DM_EVT_SERVICE_CONTEXT_DELETED:
		//	if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
		//	{
		//		sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		//	}
			break;
		case DM_EVT_LINK_SECURED:
			break;
		
        default:
            break;
    }
	
#ifdef BLE_DFU_APP_SUPPORT
	if(p_event->event_id == DM_EVT_LINK_SECURED)
	{
		app_context_load(p_handle);
	}
#endif	//BLE_DFU_APP_SUPPORT
    return NRF_SUCCESS;
}

/************************************************************************************************
 *DM初始化
 *in：	erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Device Manager.
 ***********************************************************************************************/
void device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.bond         	= SEC_PARAM_BOND;
	register_param.sec_param.mitm		  	= SEC_PARAM_MITM;
	register_param.sec_param.io_caps		= SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          	= SEC_PARAM_OOB;
    register_param.sec_param.min_key_size 	= SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size 	= SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            	= device_manager_evt_handler;
    register_param.service_type           	= DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}

/********************************************************
* BLE芯片的uart口输入数据处理函数
*********************************************************/
static void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;

            if ((data_array[index - 1] == '\n') || (index >= (BLE_NUS_MAX_DATA_LEN)))
            {
                err_code = ble_nus_string_send(&m_nus, data_array, index);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
                
                index = 0;
            }
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}

/**********************************************
*芯片uart口初始化
***********************************************/
void uart_init(void)
{
    uint32_t                     err_code;
    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_ENABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT( &comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOW,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
