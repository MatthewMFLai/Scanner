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
#include "nrf_delay.h"
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

#include "gsm_comm.h"
#include "gsm_msg_if.h"
#include "relay_fsm.h"
#include "msg_process.h"
#include "rssi_filter.h"
#include "geofence_hdlr.h"

#include "utmgr.h"
#include "ringbuff_ut.h"

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
	
#define COMMA                   ','
#define LIMITER                 ';'
#define TERM0                   '\0'
	
int main(void)
{	
    uint8_t i, idx = 0;
	char c;
	char inbuf[128];
	char tc_id[8];
	char tc_arg[64];
	char tc_ret[64];
	SEGGER_RTT_Init();
	SEGGER_RTT_printf(0, "Hello World!\n");
	
	utmgr_register(rbf_get_module_desc());
	for (;;)
	{
		c = SEGGER_RTT_WaitKey(); // will block until data is available
		if(c == LIMITER)
		{
			// input buffer should look like
			// rbf001,10,20,test string 1,test string 2
			
			// Look for the first ","
			for (i = 0; i < idx; i++)
				if (inbuf[i] == COMMA)
					break;
				
			if (i == idx)
			{
				// reset idx
				idx = 0;
				continue;
			}
			memcpy(tc_id, inbuf, i);
			tc_id[i] = TERM0;
			i++;
			memcpy(tc_arg, &inbuf[i], idx - i);
			tc_arg[idx - i] = TERM0;
			SEGGER_RTT_printf(0, "tc id: %s arg: %s\n", tc_id, tc_arg);
			utmgr_exec_tc(tc_id, tc_arg, tc_ret);
			SEGGER_RTT_printf(0, "Result: %s\n", tc_ret);
			
			// reset buffer index
			idx = 0;
		}
		else
			inbuf[idx++] = c;
		//power_manage();
	}
}
