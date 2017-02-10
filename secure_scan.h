
#ifndef SECURE_SCAN_H__
#define SECURE_SCAN_H__
										
#define APP_AES_LENGTH          0x10                              /**< Total length for AES encryption. */
#define APP_NO_ADV_GAP_TICKS    250000
#define APP_MAX_BEACON    		4
#define APP_DEVICE_ID_LENGTH    6

#define RC_SSCAN_FIRST_CONNECT	  0
#define RC_SSCAN_CONNECTED		  1
#define RC_SSCAN_FIRST_DISCONNECT 2
#define RC_SSCAN_DISCONNECTED	  3

// This structure contains various status information for our service. 
// The name is based on the naming convention used in Nordics SDKs. 
// 'ble’ indicates that it is a Bluetooth Low Energy relevant structure and 
// ‘os’ is short for Our Service). 
typedef struct
{
	uint32_t     	last_timestamp; /* last received adv message timestamp */
    uint16_t     	connected;    /* 0 = disconnected, 1 = connected */
	uint8_t      	prev_adv_msg[APP_AES_LENGTH];
	uint8_t      	beacon_uuid[APP_AES_LENGTH];
	uint8_t			aes128_key[APP_AES_LENGTH];
	uint32_t		last_adv_timeout;
	uint8_t 	    beacon_addr[APP_DEVICE_ID_LENGTH];
	uint8_t			decrypt_enabled;
	uint8_t			beacon_enabled;
} secure_scan_data_t;

void sscan_init(void);
void sscan_set_device_id(uint8_t device_idx, uint8_t *p_data);

void sscan_set_device_uuid(uint8_t device_idx, uint8_t *p_data);

void sscan_set_encryption_key(uint8_t device_idx, uint8_t *p_data);

void sscan_set_timeout_window(uint8_t device_idx, uint32_t timeout);

void sscan_enable_decryption(uint8_t device_idx);

void sscan_disable_decryption(uint8_t device_idx);

void sscan_enable_beacon(uint8_t device_idx);

void sscan_disable_beacon(uint8_t device_idx);

uint8_t sscan_get_device_index(const uint8_t * p_data);

/**@brief Function for handling BLE Stack events related to our service and characteristic.
 *
 * @details Handles all events from the BLE stack of interest to Our Service.
 *
 * @param[in]   p_our_service       Our Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
bool sscan_check_last_msg(uint8_t device_idx, uint8_t * p_data);

/**@brief Function for initializing our new service.
 *
 * @param[in]   p_our_service       Pointer to Our Service structure.
 */
bool sscan_decrypt_match_uuid (uint8_t device_idx, uint8_t *p_data, uint32_t counter_tick);

/**@brief Function for updating and sending new characteristic values
 *
 * @details The application calls this function whenever our timer_timeout_handler triggers
 *
 * @param[in]   p_our_service                     Our Service structure.
 * @param[in]   characteristic_value     New characteristic value.
 */
void sscan_set_last_msg(uint8_t device_idx, uint8_t * p_data);

void sscan_set_last_timestamp(uint8_t device_idx);

uint8_t sscan_set_connected(uint8_t device_idx);

uint8_t sscan_check_disconnected(void);

uint8_t sscan_query_connected(void);

#endif  /* _ SECURE_SCAN_H__ */
