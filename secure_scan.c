
#include <stdint.h>
#include <string.h>
#include "app_timer.h"
#include "softdevice_handler.h"
#include "secure_scan.h"
#include "atcmd.h"
#include "uart_reply.h"

static secure_scan_data_t beacons[APP_MAX_BEACON];
static uint8_t m_cur_state;
static uint32_t m_counter = 0x7c845f92;

/**@brief Function for the AES128 encryption of the 16-byte UUID.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the 16-byte UUID array
 * @param[in] p_key   pointer to a 16-byte key array
 * @param[out] p_out  pointer to the encrypted 16-byte UUID array
 * @param[in] counter new counter value. Should be incrementing...
 */
static void encrypt_128bit_uuid (uint8_t *p_data, uint8_t *p_key, uint8_t *p_out, uint32_t counter)
{
	nrf_ecb_hal_data_t aes_struct;
	uint8_t aes_data[APP_AES_LENGTH];
	
	//Initializing arrays
	memset (&aes_struct, 0, sizeof(aes_struct));
	memset (aes_data, 0, sizeof(aes_data));

	//Initializing key
	for (int i = 0; i < APP_AES_LENGTH; i++)
	{
		aes_struct.key[i] = p_key[i];
	}

	//Initializing nouncence
	memset (aes_struct.cleartext, 0xaa, sizeof(aes_struct.cleartext)); //todo: use more random data
	
	// Add counter
	//aes_struct.cleartext[0] += counter;
	memcpy(&aes_struct.cleartext[0], &counter, sizeof(counter));
	
	//Creating chipertext
	sd_ecb_block_encrypt(&aes_struct);  

	//Decrypt -> XOR chipertext with p_data:
	for (int i = 0; i < APP_AES_LENGTH; i++)
	{  
		aes_data[i] = p_data [i] ^ aes_struct.ciphertext[i];
	}
	
	memcpy (p_out, aes_data, APP_AES_LENGTH);
}

void sscan_init(void)
{
	uint8_t device_idx;
	for (device_idx = 0; device_idx < APP_MAX_BEACON; device_idx++)
		memset(&beacons[device_idx], 0, sizeof(secure_scan_data_t));
	m_cur_state = 0; // Disconnected.
}
void sscan_set_device_id(uint8_t device_idx, uint8_t *p_data)
{
	memcpy(beacons[device_idx].beacon_addr, p_data, APP_DEVICE_ID_LENGTH);
}

void sscan_set_device_uuid(uint8_t device_idx, uint8_t *p_data)
{
	memcpy(beacons[device_idx].beacon_uuid, p_data, APP_AES_LENGTH);
}

void sscan_set_encryption_key(uint8_t device_idx, uint8_t *p_data)
{
	memcpy(beacons[device_idx].aes128_key, p_data, APP_AES_LENGTH);
}

void sscan_set_timeout_window(uint8_t device_idx, uint32_t timeout)
{
	beacons[device_idx].last_adv_timeout = timeout;
}

void sscan_enable_decryption(uint8_t device_idx)
{
	beacons[device_idx].decrypt_enabled = 1;
}

void sscan_disable_decryption(uint8_t device_idx)
{
	beacons[device_idx].decrypt_enabled = 0;
}

void sscan_enable_beacon(uint8_t device_idx)
{
	beacons[device_idx].beacon_enabled = 1;
}

void sscan_disable_beacon(uint8_t device_idx)
{
	beacons[device_idx].beacon_enabled = 0;
}

uint8_t sscan_get_device_index(const uint8_t * p_data)
{
	uint8_t idx;
	for (idx = 0; idx < APP_MAX_BEACON; idx++)
	{
		if (!memcmp(p_data, beacons[idx].beacon_addr, APP_DEVICE_ID_LENGTH))
			break;
	}
	return (idx);
}

/**@brief Function for handling BLE Stack events related to our service and characteristic.
 *
 * @details Handles all events from the BLE stack of interest to Our Service.
 *
 * @param[in]   p_our_service       Our Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
bool sscan_check_last_msg(uint8_t device_idx, uint8_t * p_data)
{
	if (!memcmp(p_data + 9, &(beacons[device_idx].prev_adv_msg), APP_AES_LENGTH))
		return true;	
	else
		return false;
}

/**@brief Function for initializing our new service.
 *
 * @param[in]   p_our_service       Pointer to Our Service structure.
 */
bool sscan_decrypt_match_uuid (uint8_t device_idx, uint8_t *p_data, uint32_t counter_tick)
{	
    uint8_t extracted_uuid[APP_AES_LENGTH];
	
	//for (int i = 0; i < 31; i++)
    //    SEGGER_RTT_printf(0, "data[%d]: 0x%#02x\n", i, p_data[i]); // Print service UUID should match definition BLE_UUID_OUR_SERVICE
	//SEGGER_RTT_printf(0, "counter_tick: 0x%#08x\n", counter_tick);
	encrypt_128bit_uuid(p_data, beacons[device_idx].aes128_key, extracted_uuid, m_counter + counter_tick);
	if (!memcmp(extracted_uuid, beacons[device_idx].beacon_uuid, APP_AES_LENGTH))
	{
		return true;
	}
	return false;	
}
/**@brief Function for updating and sending new characteristic values
 *
 * @details The application calls this function whenever our timer_timeout_handler triggers
 *
 * @param[in]   p_our_service                     Our Service structure.
 * @param[in]   characteristic_value     New characteristic value.
 */
void sscan_set_last_msg(uint8_t device_idx, uint8_t * p_data)
{
	memcpy(beacons[device_idx].prev_adv_msg, p_data, APP_AES_LENGTH);	
}
void sscan_set_last_timestamp(uint8_t device_idx)
{
	app_timer_cnt_get(&beacons[device_idx].last_timestamp);	
}
void sscan_set_connected(uint8_t device_idx)
{
	// Send disconnect message to UART.
	beacons[device_idx].connected = 1;
	if (!m_cur_state)
	{
		m_cur_state = 1;
		uart_reply_string(atcmd_get_in());
		uart_reply_byte('\n');
	}
}
void sscan_check_disconnected(void)
{
	uint8_t device_idx;
	uint32_t cur_timestamp;
	uint32_t time_diff;
	
	app_timer_cnt_get(&cur_timestamp);
	for (device_idx = 0; device_idx < APP_MAX_BEACON; device_idx++)
	{
		app_timer_cnt_diff_compute(cur_timestamp, beacons[device_idx].last_timestamp, &time_diff);
		//SEGGER_RTT_printf(0, "time diff: %d\n", time_diff);
		if (time_diff > beacons[device_idx].last_adv_timeout)
		{
			// Send disconnect message to UART.
			beacons[device_idx].connected = 0;
		}
	}
	
	for (device_idx = 0; device_idx < APP_MAX_BEACON; device_idx++)
	{
		if (beacons[device_idx].beacon_enabled && beacons[device_idx].connected)
		{
			return;
		}
	}
	// Send disconnect message to UART.
	if (m_cur_state)
	{
		m_cur_state = 0;
		uart_reply_string(atcmd_get_out());
		uart_reply_byte('\n');
	}
}

uint8_t sscan_query_connected(void)
{
	return (m_cur_state);
}