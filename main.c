#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nordic_common.h"
#include "app_error.h"
#include "app_uart.h"
#include "app_trace.h"
#include "ble_db_discovery.h"
#include "app_timer.h"
#include "app_trace.h"
#include "app_util.h"
#include "app_error.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "softdevice_handler.h"
#include "ble_advdata.h"
#include "ble_nus_c.h"
#include "pstorage.h"
#include "SEGGER_RTT.h"

// From the NUS peripheral main module
#include "nrf.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_nus.h"
#include "app_util_platform.h"
// From the NUS peripheral main module

#include "timestamp.h"
#include "custom_board.h"
#include "util.h"
#include "uart_reply.h"
#include "secure_scan.h"
#include "atcmd.h"
#include "pstore.h"
#include "config_hdlr.h"

#define CENTRAL_LINK_COUNT      1                               /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
//#define PERIPHERAL_LINK_COUNT   0                               /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define UART_TX_BUF_SIZE        256                             /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE        256                             /**< UART RX buffer size. */

#define NUS_SERVICE_UUID_TYPE   BLE_UUID_TYPE_VENDOR_BEGIN      /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_TIMER_PRESCALER     32                               /**< Value of the RTC1 PRESCALER register. */
//#define APP_TIMER_OP_QUEUE_SIZE 2                               /**< Size of timer operation queues. */
#define TIMER_INTERVAL          APP_TIMER_TICKS(62, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */


#define APPL_LOG                app_trace_log                   /**< Debug logger macro that will be used in this file to do logging of debug information over UART. */

#define SCAN_INTERVAL           0x00A0                          /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW             0x0050                          /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_ACTIVE             0                               /**< If 1, performe active scanning (scan requests). */
#define SCAN_SELECTIVE          1                               /**< If 1, ignore unknown devices (non whitelisted). */
#define SCAN_TIMEOUT            0x0000                          /**< Timout when scanning. 0x0000 disables timeout. */

#define MIN_CONNECTION_INTERVAL MSEC_TO_UNITS(20, UNIT_1_25_MS) /**< Determines minimum connection interval in millisecond. */
#define MAX_CONNECTION_INTERVAL MSEC_TO_UNITS(75, UNIT_1_25_MS) /**< Determines maximum connection interval in millisecond. */
#define SLAVE_LATENCY           0                               /**< Determines slave latency in counts of connection events. */
#define SUPERVISION_TIMEOUT     MSEC_TO_UNITS(4000, UNIT_10_MS) /**< Determines supervision time-out in units of 10 millisecond. */

#define UUID16_SIZE             2                               /**< Size of 16 bit UUID */
#define UUID32_SIZE             4                               /**< Size of 32 bit UUID */
#define UUID128_SIZE            16                              /**< Size of 128 bit UUID */									
									
static ble_nus_c_t              m_ble_nus_c;                    /**< Instance of NUS service. Must be passed to all NUS_C API calls. */
static ble_db_discovery_t       m_ble_db_discovery;             /**< Instance of database discovery module. Must be passed to all db_discovert API calls */

/**
 * @brief Connection parameters requested for connection.
 */
static const ble_gap_conn_params_t m_connection_param =
  {
    (uint16_t)MIN_CONNECTION_INTERVAL,  // Minimum connection
    (uint16_t)MAX_CONNECTION_INTERVAL,  // Maximum connection
    (uint16_t)SLAVE_LATENCY,            // Slave latency
    (uint16_t)SUPERVISION_TIMEOUT       // Supervision time-out
  };

/**
 * @brief Parameters used when scanning.
 */
 static ble_gap_addr_t beacon = {BLE_GAP_ADDR_TYPE_RANDOM_STATIC, {0}};
 static ble_gap_addr_t beacon2 = {BLE_GAP_ADDR_TYPE_RANDOM_STATIC, {0}};
 static ble_gap_addr_t trackr = {BLE_GAP_ADDR_TYPE_RANDOM_STATIC, {0}};
 static ble_gap_addr_t trackr2 = {BLE_GAP_ADDR_TYPE_RANDOM_STATIC, {0}};
 
 static ble_gap_addr_t *whitelisttable[] = {&beacon, &beacon2, &trackr, &trackr2};
 static ble_gap_whitelist_t whitelistcontrol =
 {
	 .pp_addrs = whitelisttable,
	 .addr_count = 4,
	 .pp_irks = NULL,
	 .irk_count = 0
 };
 
//static const ble_gap_scan_params_t m_scan_params = 
static  ble_gap_scan_params_t m_scan_params = 
  {
    .active      = SCAN_ACTIVE,
    .selective   = SCAN_SELECTIVE,
    .p_whitelist = &whitelistcontrol,
    .interval    = 0,
    .window      = 0,
    .timeout     = SCAN_TIMEOUT
  };

/**
 * @brief NUS uuid
 */
static const ble_uuid_t m_nus_uuid = 
  {
    .uuid = BLE_UUID_NUS_SERVICE,
    .type = NUS_SERVICE_UUID_TYPE
  };

static uint8_t m_aes128_key[APP_AES_LENGTH] =
{
	0
};
static uint8_t m_beacon_uuid[APP_AES_LENGTH] =
{
	0
};
static uint8_t m_beacon2_uuid[APP_AES_LENGTH] =
{
	0
};

static uint8_t m_aes128_key2[APP_AES_LENGTH] =
{
	0
};
static char m_config_data_src[PSTORE_MAX_BLOCK];
static uint8_t m_ble_data_src[APP_ATCMD_MAX_DATA_LEN] = {0};
static char m_atcmd_resp_str[PSTORE_MAX_BLOCK + 1];
static uint8_t m_cur_scan_status;

APP_TIMER_DEF(m_app_timer_id);

// From the NUS peripheral main module
#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device. */

//#define CENTRAL_LINK_COUNT              0                                           /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT           1                                           /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define DEVICE_NAME                     "Nordic_UART"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

//#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
//#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
//#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(156, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(937, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */

#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

static ble_nus_t                        m_nus;                                      /**< Structure to identify the Nordic UART Service. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static ble_uuid_t                       m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}};  /**< Universally unique service identifier. */
// From the NUS peripheral main module

// Custom test code for Mike's enforcer test.
static int8_t m_rx_rssi = 0;
// End:Custom test code for Mike's enforcer test.

// Matt: custom handler to send IN/OUT status to NUS client.
static void update_nus_client(char *data_array)
{
    //uint8_t data_array[BLE_NUS_MAX_DATA_LEN] = {0};
    uint8_t index;
    uint32_t err_code;
	
	index = strlen(data_array);
	err_code = ble_nus_string_send(&m_nus, (uint8_t *)data_array, index);
	if (err_code != NRF_ERROR_INVALID_STATE)
	{
		APP_ERROR_CHECK(err_code);
	}
}
// Matt: custom handler to send IN/OUT status to NUS client.
 
/**@brief Function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

/**@brief Function to start scanning.
 */
static void scan_start(void)
{
    uint32_t err_code;
    
	if (m_cur_scan_status)
		return;
	
	m_cur_scan_status = 1;
    err_code = sd_ble_gap_scan_start(&m_scan_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling database discovery events.
 *
 * @details This function is callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function should forward the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    ble_nus_c_on_db_disc_evt(&m_ble_nus_c, p_evt);
}

static void execute_atcmd(uint16_t index, uint8_t *data_array, char *p_resp_str)
{
	uint8_t val_str_len;
	uint16_t param_size;
	char datastr[16] = {0};
	uint16_t config_size;
	uint16_t new_scan_interval;
	uint16_t new_scan_window;
	uint32_t cur_ticks;

	uint8_t rssi_val[4] = {0};
	uint8_t rssi_len;
			
	memset(p_resp_str, 0, PSTORE_MAX_BLOCK + 1);
	// Execute AT command.
	switch (atcmd_parse(index, (char *)data_array)) {
		case APP_ATCMD_ACT_ENABLE_SCAN :
			if (atcmd_scan_enabled())
			{
				config_hdlr_set_byte("sc01", 1);
				scan_start();
			}
			else
			{
				config_hdlr_set_byte("sc01", 0);
				m_cur_scan_status = 0;
				sd_ble_gap_scan_stop();
			}
			memcpy(p_resp_str, atcmd_get_ok(), strlen(atcmd_get_ok()));
			break;
			
		case APP_ATCMD_ACT_MODE_0 :
			memcpy(p_resp_str, atcmd_get_ok(), strlen(atcmd_get_ok()));
			break;
			
		case APP_ATCMD_ACT_SCAN_INT :
			atcmd_get_scan_param(&new_scan_interval, &new_scan_window);
			m_scan_params.interval = new_scan_interval;
			m_scan_params.window = new_scan_window;
			config_hdlr_set_word("sc03", new_scan_interval);
			config_hdlr_set_word("sc04", new_scan_window);
			memcpy(p_resp_str, atcmd_get_ok(), strlen(atcmd_get_ok()));
			break;
			
		case APP_ATCMD_ACT_ENABLE_SCAN_READ :
			*p_resp_str = atcmd_get_enable();
			break;
			
		case APP_ATCMD_ACT_MODE_0_READ :
			*p_resp_str = atcmd_get_mode();
			break;
			
		case APP_ATCMD_ACT_SCAN_INT_READ :
			memcpy(p_resp_str, atcmd_get_interval(), strlen(atcmd_get_interval()));
			p_resp_str += strlen(atcmd_get_interval());
			*p_resp_str = ' ';
			p_resp_str++;
			memcpy(p_resp_str, atcmd_get_window(), strlen(atcmd_get_window()));
			break;

		case APP_ATCMD_ACT_CONFIG_GET :
			memcpy(p_resp_str, atcmd_get_ok(), strlen(atcmd_get_ok()));
			break;
			
		case APP_ATCMD_ACT_CONFIG_SET :
			memcpy(p_resp_str, atcmd_get_ok(), strlen(atcmd_get_ok()));
			break;
			
		case APP_ATCMD_ACT_CONFIG_GET_VER :
			if (!config_hdlr_get_string("vers", &param_size, datastr))
			{
				strcpy(datastr, "NUL");
			}
			memcpy(p_resp_str, datastr, strlen(datastr));
			break;

		case APP_ATCMD_ACT_CONFIG_UPD :
			memset(m_config_data_src, 0, PSTORE_MAX_BLOCK);
			config_size = config_hdlr_build((uint8_t *)m_config_data_src);
			//SEGGER_RTT_printf(0, "config data: rc %d size: %d content: %s\n", config_size, strlen(config_data_src), config_data_src);
			pstore_set((uint8_t *)m_config_data_src, config_size);
			memcpy(p_resp_str, atcmd_get_ok(), strlen(atcmd_get_ok()));
			break;
		
		case APP_ATCMD_ACT_CURRENT_TS :
			app_timer_cnt_get(&cur_ticks);
			val_str_len = longword_to_ascii((uint8_t *)datastr, ts_check_and_set(cur_ticks));
			memcpy(p_resp_str, datastr, val_str_len);
			break;
		
		case APP_ATCMD_ACT_LAST_SENTENCE :
			memcpy(p_resp_str, atcmd_get_lastcmd(), strlen(atcmd_get_lastcmd()));
			break;
			
// Custom test code for Mike's enforcer test.
		case APP_ATCMD_ACT_RSSI_GET :
			rssi_len = signed_byte_to_ascii(rssi_val, m_rx_rssi);
			memcpy(p_resp_str, rssi_val, rssi_len);
			break;
// End:Custom test code for Mike's enforcer test.
			
		default :
			memcpy(p_resp_str, atcmd_get_nack(), strlen(atcmd_get_nack()));
			break;
	}
}

/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to 
 *          a string. The string will be be sent over BLE when the last character received was a 
 *          'new line' i.e '\n' (hex 0x0D) or if the string has reached a length of 
 *          @ref NUS_MAX_DATA_LENGTH.
 */
void uart_event_handle(app_uart_evt_t * p_event)
{
	static uint8_t data_array[APP_ATCMD_MAX_DATA_LEN];
    static uint16_t index = 0;
	
    switch (p_event->evt_type)
    {
        /**@snippet [Handling data from UART] */ 
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;

            if ((data_array[index - 1] == '\r') ||
				(index >= (APP_ATCMD_MAX_DATA_LEN)))
				//(index >= (BLE_NUS_MAX_DATA_LEN)))
            {
                /*while (ble_nus_c_string_send(&m_ble_nus_c, data_array, index) != NRF_SUCCESS)
                {
                    // repeat until sent.
                }*/
				
				// Execute AT command.
				execute_atcmd(index, data_array, m_atcmd_resp_str);
				m_atcmd_resp_str[strlen(m_atcmd_resp_str)] = '\n';
				uart_reply_string(m_atcmd_resp_str);
                index = 0;
            }
            break;
        /**@snippet [Handling data from UART] */ 
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


/**@brief Callback handling NUS Client events.
 *
 * @details This function is called to notify the application of NUS client events.
 *
 * @param[in]   p_ble_nus_c   NUS Client Handle. This identifies the NUS client
 * @param[in]   p_ble_nus_evt Pointer to the NUS Client event.
 */

/**@snippet [Handling events from the ble_nus_c module] */ 
static void ble_nus_c_evt_handler(ble_nus_c_t * p_ble_nus_c, const ble_nus_c_evt_t * p_ble_nus_evt)
{
    uint32_t err_code;
    switch (p_ble_nus_evt->evt_type)
    {
        case BLE_NUS_C_EVT_DISCOVERY_COMPLETE:
            err_code = ble_nus_c_handles_assign(p_ble_nus_c, p_ble_nus_evt->conn_handle, &p_ble_nus_evt->handles);
            APP_ERROR_CHECK(err_code);

            err_code = ble_nus_c_rx_notif_enable(p_ble_nus_c);
            APP_ERROR_CHECK(err_code);
            APPL_LOG("The device has the Nordic UART Service\r\n");
            break;
        
        case BLE_NUS_C_EVT_NUS_RX_EVT:
            for (uint32_t i = 0; i < p_ble_nus_evt->data_len; i++)
            {
                while(app_uart_put( p_ble_nus_evt->p_data[i]) != NRF_SUCCESS);
            }
            break;
        
        case BLE_NUS_C_EVT_DISCONNECTED:
            APPL_LOG("Disconnected\r\n");
            scan_start();
            break;
    }
}
/**@snippet [Handling events from the ble_nus_c module] */

/**@brief Reads an advertising report and checks if a uuid is present in the service list.
 *
 * @details The function is able to search for 16-bit, 32-bit and 128-bit service uuids. 
 *          To see the format of a advertisement packet, see 
 *          https://www.bluetooth.org/Technical/AssignedNumbers/generic_access_profile.htm
 *
 * @param[in]   p_target_uuid The uuid to search fir
 * @param[in]   p_adv_report  Pointer to the advertisement report.
 *
 * @retval      true if the UUID is present in the advertisement report. Otherwise false  
 */
static bool is_uuid_present(const ble_uuid_t *p_target_uuid, 
                            const ble_gap_evt_adv_report_t *p_adv_report)
{
    uint8_t *p_data = (uint8_t *)p_adv_report->data;
	uint32_t counter_tick;
	uint8_t device_idx;
	
	device_idx = sscan_get_device_index(p_adv_report->peer_addr.addr);
	if (device_idx >= APP_MAX_BEACON)
		return false;
	
	if (sscan_check_last_msg(device_idx, p_data + 9))
	{
		sscan_set_last_timestamp(device_idx);
		return true;
	}	

	// Custom test code for Mike's enforcer test.
    return true;
    // End:Custom test code for Mike's enforcer test.

    counter_tick = p_data[25] + (p_data[26] << 8) + (p_data[27] << 16) + (p_data[28] << 24);
	
	//for (int i = 0; i < 31; i++)
    //    SEGGER_RTT_printf(0, "data[%d]: 0x%#02x\n", i, p_data[i]); // Print service UUID should match definition BLE_UUID_OUR_SERVICE
	//for (int i = 0; i < 5; i++)
    //    SEGGER_RTT_printf(0, "data[%d]: 0x%#02x\n", i, p_adv_report->peer_addr.addr[i]); // Print service UUID should match definition BLE_UUID_OUR_SERVICE
	//SEGGER_RTT_printf(0, "counter_tick: 0x%#08x\n", counter_tick);
	if (sscan_decrypt_match_uuid(device_idx, p_data + 9, counter_tick))
	{
		sscan_set_last_msg(device_idx, p_data + 9);
		sscan_set_last_timestamp(device_idx);
		if (sscan_set_connected(device_idx) == RC_SSCAN_FIRST_CONNECT)
		{
			char ts_str[11] = {0};
			char data_array[BLE_NUS_MAX_DATA_LEN] = {0};
			uint32_t cur_ticks;
					
			app_timer_cnt_get(&cur_ticks);
			longword_to_ascii((uint8_t *)ts_str, ts_check_and_set(cur_ticks));
			
			strcpy(data_array, atcmd_get_in());
			data_array[strlen(data_array)] = ' ';
			strcpy(data_array + strlen(data_array), ts_str);

			uart_reply_string(data_array);
			uart_reply_byte('\n');			
			update_nus_client(data_array);
			
			atcmd_set_lastcmd(data_array);
		}
		return true;
	}
	return false;
}

/**@brief Reads an advertising report and checks if a uuid is present in the service list.
 *
 * @details The function is able to search for 16-bit, 32-bit and 128-bit service uuids. 
 *          To see the format of a advertisement packet, see 
 *          https://www.bluetooth.org/Technical/AssignedNumbers/generic_access_profile.htm
 *
 * @param[in]   p_target_uuid The uuid to search fir
 * @param[in]   p_adv_report  Pointer to the advertisement report.
 *
 * @retval      true if the UUID is present in the advertisement report. Otherwise false  
 */
/* 
static bool is_uuid_present(const ble_uuid_t *p_target_uuid, 
                            const ble_gap_evt_adv_report_t *p_adv_report)
{
    uint32_t err_code;
    uint32_t index = 0;
    uint8_t *p_data = (uint8_t *)p_adv_report->data;
    ble_uuid_t extracted_uuid;
	
	return true;
    while (index < p_adv_report->dlen)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type   = p_data[index+1];
                
        if ( (field_type == BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE)
           || (field_type == BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE)
           )
        {
            for (uint32_t u_index = 0; u_index < (field_length/UUID16_SIZE); u_index++)
            {
                err_code = sd_ble_uuid_decode(  UUID16_SIZE, 
                                                &p_data[u_index * UUID16_SIZE + index + 2], 
                                                &extracted_uuid);
                if (err_code == NRF_SUCCESS)
                {
                    if ((extracted_uuid.uuid == p_target_uuid->uuid)
                        && (extracted_uuid.type == p_target_uuid->type))
                    {
                        return true;
                    }
                }
            }
        }

        else if ( (field_type == BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_MORE_AVAILABLE)
                || (field_type == BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_COMPLETE)
                )
        {
            for (uint32_t u_index = 0; u_index < (field_length/UUID32_SIZE); u_index++)
            {
                err_code = sd_ble_uuid_decode(UUID16_SIZE, 
                &p_data[u_index * UUID32_SIZE + index + 2], 
                &extracted_uuid);
                if (err_code == NRF_SUCCESS)
                {
                    if ((extracted_uuid.uuid == p_target_uuid->uuid)
                        && (extracted_uuid.type == p_target_uuid->type))
                    {
                        return true;
                    }
                }
            }
        }
        
        else if ( (field_type == BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE)
                || (field_type == BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE)
                )
        {
            err_code = sd_ble_uuid_decode(UUID128_SIZE, 
                                          &p_data[index + 2], 
                                          &extracted_uuid);
            if (err_code == NRF_SUCCESS)
            {
                if ((extracted_uuid.uuid == p_target_uuid->uuid)
                    && (extracted_uuid.type == p_target_uuid->type))
                {
                    return true;
                }
            }
        }
        index += field_length + 1;
    }
    return false;
}
*/
/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t              err_code;
    const ble_gap_evt_t * p_gap_evt = &p_ble_evt->evt.gap_evt;

	//Matt test end
	
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
        {
            const ble_gap_evt_adv_report_t * p_adv_report = &p_gap_evt->params.adv_report;

            if (is_uuid_present(&m_nus_uuid, p_adv_report))
            {
				//SEGGER_RTT_printf(0, "time diff: %d\n", time_diff);
				//err_code = app_timer_stop(m_app_timer_id);
				//APP_ERROR_CHECK(err_code);
				//err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
				//APP_ERROR_CHECK(err_code);
				
				// Custom test code for Mike's enforcer test.
				m_rx_rssi = p_adv_report->rssi;
                // End:Custom test code for Mike's enforcer test.				
				break;

                err_code = sd_ble_gap_connect(&p_adv_report->peer_addr,
                                              &m_scan_params,
                                              &m_connection_param);

                if (err_code == NRF_SUCCESS)
                {
                    // scan is automatically stopped by the connect
                    APPL_LOG("Connecting to target %02x%02x%02x%02x%02x%02x\r\n",
                             p_adv_report->peer_addr.addr[0],
                             p_adv_report->peer_addr.addr[1],
                             p_adv_report->peer_addr.addr[2],
                             p_adv_report->peer_addr.addr[3],
                             p_adv_report->peer_addr.addr[4],
                             p_adv_report->peer_addr.addr[5]
                             );
                }
            }
            break;
        }

        case BLE_GAP_EVT_CONNECTED:
            APPL_LOG("Connected to target\r\n");

            // start discovery of services. The NUS Client waits for a discovery result
            err_code = ble_db_discovery_start(&m_ble_db_discovery, p_ble_evt->evt.gap_evt.conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)
            {
                APPL_LOG("[APPL]: Scan timed out.\r\n");
                scan_start();
            }
            else if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                APPL_LOG("[APPL]: Connection Request timed out.\r\n");
            }
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
            // Accepting parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                    &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
            break;
    
        default:
            break;
    }
}

/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack event has
 *          been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_central_dispatch(ble_evt_t * p_ble_evt)
{
    on_ble_evt(p_ble_evt);
    ble_db_discovery_on_ble_evt(&m_ble_db_discovery, p_ble_evt);
    ble_nus_c_on_ble_evt(&m_ble_nus_c,p_ble_evt);
}

/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    //ble_advertising_on_sys_evt(sys_evt);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 *
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
    
    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
    
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

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
	
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}*/

/**@brief Function for initializing the UART.
 */
static void uart_init(void)
{
    uint32_t err_code;

    const app_uart_comm_params_t comm_params =
      {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud57600
      };

    APP_UART_FIFO_INIT(&comm_params,
                        UART_RX_BUF_SIZE,
                        UART_TX_BUF_SIZE,
                        uart_event_handle,
                        APP_IRQ_PRIORITY_LOW,
                        err_code);

    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the NUS Client.
 */
static void nus_c_init(void)
{
    uint32_t         err_code;
    ble_nus_c_init_t nus_c_init_t;
    
    nus_c_init_t.evt_handler = ble_nus_c_evt_handler;
    
    err_code = ble_nus_c_init(&m_ble_nus_c, &nus_c_init_t);
    APP_ERROR_CHECK(err_code);
}

/** @brief Function for initializing the Database Discovery Module.
 */
static void db_discovery_init(void)
{
    uint32_t err_code = ble_db_discovery_init(db_disc_handler);
    APP_ERROR_CHECK(err_code);
}

/** @brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();

    APP_ERROR_CHECK(err_code);
}

static void timer_timeout_handler (void *p_context)
{
	uint32_t cur_ticks;
	
	if (sscan_check_disconnected() == RC_SSCAN_FIRST_DISCONNECT)
	{
		char ts_str[11] = {0};
		char data_array[BLE_NUS_MAX_DATA_LEN] = {0};
		uint32_t cur_ticks;
				
		app_timer_cnt_get(&cur_ticks);
		longword_to_ascii((uint8_t *)ts_str, ts_check_and_set(cur_ticks));
		
		strcpy(data_array, atcmd_get_out());
		data_array[strlen(data_array)] = ' ';
		strcpy(data_array + strlen(data_array), ts_str);

		uart_reply_string(data_array);
		uart_reply_byte('\n');			
		update_nus_client(data_array);
		
		atcmd_set_lastcmd(data_array);
	}
	app_timer_cnt_get(&cur_ticks);
	ts_check_and_set(cur_ticks);
}


// From the NUS peripheral main module
/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of 
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_nus    Nordic UART Service structure.
 * @param[in] p_data   Data to be send to UART module.
 * @param[in] length   Length of the data.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
	uint32_t err_code;
	uint16_t cur_len = strlen((char *)m_ble_data_src);
	
	// Execute AT command.
	memcpy(m_ble_data_src + cur_len, p_data, length);
	length += cur_len;
	// Need to have the '\r' or ';' as the terminator!
	if (m_ble_data_src[length-1] == ';')
		m_ble_data_src[length-1] = '\r';
	
	if (m_ble_data_src[length-1] != '\r')
		return;
	
	execute_atcmd(length, m_ble_data_src, m_atcmd_resp_str);
	err_code = ble_nus_string_send(&m_nus, (uint8_t *)m_atcmd_resp_str, strlen(m_atcmd_resp_str));
	if (err_code != NRF_ERROR_INVALID_STATE)
	{
		APP_ERROR_CHECK(err_code);
	}
	// Clear the ble command buffer.
	memset(m_ble_data_src, 0, APP_ATCMD_MAX_DATA_LEN);			
}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;
    
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;
    
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;
    
    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
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

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            break;
        case BLE_ADV_EVT_IDLE:
            break;
        default:
            break;
    }
}


/**@brief Function for the application's SoftDevice event handler.
 *
 * @param[in] p_ble_evt SoftDevice event.
 */
static void on_ble_peripheral_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a SoftDevice event to all modules with a SoftDevice 
 *        event handler.
 *
 * @details This function is called from the SoftDevice event interrupt handler after a 
 *          SoftDevice event has been received.
 *
 * @param[in] p_ble_evt  SoftDevice event.
 */
static void ble_evt_peripheral_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
    on_ble_peripheral_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
}

/**@brief Function for dispatching a SoftDevice event to all modules with a SoftDevice 
 *        event handler.
 *
 * @details This function is called from the SoftDevice event interrupt handler after a 
 *          SoftDevice event has been received.
 *
 * @param[in] p_ble_evt  SoftDevice event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_evt_peripheral_dispatch(p_ble_evt);
    ble_evt_central_dispatch(p_ble_evt);
}

/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
    
    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
    
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
	
    // Register with the SoftDevice handler module for sys events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
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

// From the NUS peripheral main module

int main(void)
{
	uint8_t config_data_raw[PSTORE_MAX_BLOCK] = {0};
	uint16_t config_size;
	uint16_t param_size;
	uint8_t bytedata;
	uint32_t longdata;
	char verstr[10] = {0};
	uint32_t err_code;
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);
	
    uart_init();
    db_discovery_init();
    ble_stack_init();
    nus_c_init();

// From the NUS peripheral main module
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
// From the NUS peripheral main module

	// Matt: our code
	// Get config data from internal flash.
	sscan_init();
	config_hdlr_init();
	pstore_init();

	config_size = pstore_get(config_data_raw);
	config_hdlr_parse(config_size, config_data_raw);

	// Change to small endian format for the device ids first.
    if (config_hdlr_get_bcd("b101", &param_size, (char *)beacon.addr))
		big_to_small_endian(beacon.addr, APP_DEVICE_ID_LENGTH);
    if (config_hdlr_get_bcd("b201", &param_size, (char *)beacon2.addr))
		big_to_small_endian(beacon2.addr, APP_DEVICE_ID_LENGTH);
    if (config_hdlr_get_bcd("b301", &param_size, (char *)trackr.addr))
		big_to_small_endian(trackr.addr, APP_DEVICE_ID_LENGTH);
    if (config_hdlr_get_bcd("b401", &param_size, (char *)trackr2.addr))
		big_to_small_endian(trackr2.addr, APP_DEVICE_ID_LENGTH);
	
	// Set scan parameters
	config_hdlr_get_word("sc03", &m_scan_params.interval);
	config_hdlr_get_word("sc04", &m_scan_params.window);
	
	sscan_set_device_id(0, beacon.addr);
	if (config_hdlr_get_bcd("b102", &param_size, (char *)m_beacon_uuid))
		sscan_set_device_uuid(0, m_beacon_uuid);
	if (config_hdlr_get_bcd("b105", &param_size, (char *)m_aes128_key))
		sscan_set_encryption_key(0, m_aes128_key);
	if (config_hdlr_get_longword("b108", &longdata))
		sscan_set_timeout_window(0, longdata);
	sscan_enable_decryption(0);
	sscan_enable_beacon(0);
	
	sscan_set_device_id(1, beacon2.addr);
	if (config_hdlr_get_bcd("b202", &param_size, (char *)m_beacon2_uuid))
		sscan_set_device_uuid(1, m_beacon2_uuid);
	if (config_hdlr_get_bcd("b205", &param_size, (char *)m_aes128_key2))
		sscan_set_encryption_key(1, m_aes128_key2);
	if (config_hdlr_get_longword("b208", &longdata))
		sscan_set_timeout_window(1, longdata);
	sscan_enable_decryption(1);
	sscan_enable_beacon(1);
	
	sscan_set_device_id(2, trackr.addr);
	sscan_set_device_uuid(2, m_beacon_uuid);
	sscan_set_encryption_key(2, m_aes128_key);
	if (config_hdlr_get_longword("b308", &longdata))
		sscan_set_timeout_window(2, longdata);
	sscan_disable_decryption(2);
	sscan_enable_beacon(2);
	
	sscan_set_device_id(3, trackr2.addr);
	sscan_set_device_uuid(3, m_beacon_uuid);
	sscan_set_encryption_key(3, m_aes128_key);
	if (config_hdlr_get_longword("b408", &longdata))
		sscan_set_timeout_window(3, longdata);
	sscan_disable_decryption(3);
	sscan_enable_beacon(3);
	
    //err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_SINGLE_SHOT, timer_timeout_handler);
	err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
	// Init current scan status.
	m_cur_scan_status = 0;
	
    // Start scanning for peripherals and initiate connection
    // with devices that advertise NUS UUID.
	// Activate scan with data from pStorage.
	
// Custom test code for Mike's enforcer test.
	if (config_hdlr_get_byte("sc01", &bytedata))
		if (bytedata)
			scan_start();
// End:Custom test code for Mike's enforcer test.

	if (!config_hdlr_get_string("vers", &param_size, verstr))
	{
		strcpy(verstr, "NUL");
	}
	uart_reply_string(verstr);
	uart_reply_byte('\n');
	
    err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
	
    for (;;)
    {
        power_manage();
    }
}
