/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "app_error.h"
#include "softdevice_handler.h"
#include "pstorage.h"

#include "pstore.h"

static uint8_t             m_pstore_buffer[PSTORE_MAX_BLOCK];
static pstorage_handle_t   m_handle;
static pstorage_handle_t   m_block_0_handle;
static uint8_t             m_wait_flag = 0;
static pstorage_block_t    m_wait_handle = 0;

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

static void pstore_handler(pstorage_handle_t  * handle,
		           uint8_t              op_code,
                           uint32_t             result,
                           uint8_t            * p_data,
                           uint32_t             data_len)
{
    if(handle->block_id == m_wait_handle) { m_wait_flag = 0; }  //If we are waiting for this callback, clear the wait flag.
	switch(op_code)
	{
	    case PSTORAGE_LOAD_OP_CODE:
		if (result == NRF_SUCCESS)
		{
		    printf("pstorage LOAD callback received \r\n");
		}
		else
		{
		    printf("pstorage LOAD ERROR callback received \r\n");
		}
		break;

	    case PSTORAGE_STORE_OP_CODE:
		if (result == NRF_SUCCESS)
		{
		    printf("pstorage STORE callback received \r\n");
		}
		else
		{
		    printf("pstorage STORE ERROR callback received \r\n");
		}
		break;

	    case PSTORAGE_UPDATE_OP_CODE:
		if (result == NRF_SUCCESS)
		{
		    printf("pstorage UPDATE callback received \r\n");
		}
		else
		{
		    printf("pstorage UPDATE ERROR callback received \r\n");
		}
		break;

	    case PSTORAGE_CLEAR_OP_CODE:
		if (result == NRF_SUCCESS)
		{
		    printf("pstorage CLEAR callback received \r\n");
		}
		else
		{
		    printf("pstorage CLEAR ERROR callback received \r\n");
		}
		break;	 
	}			
}

bool pstore_init(void)
{
    uint32_t retval;
    pstorage_module_param_t param;

    retval = pstorage_init();
    if(retval != NRF_SUCCESS)
    {
	return false;	
    }

    param.block_size  = PSTORE_MAX_BLOCK;                   //Select block size of 512 bytes
    param.block_count = 1;                   //Select 1 block, total of 512 bytes
    param.cb          = pstore_handler;   //Set the pstorage callback handler
			
    retval = pstorage_register(&param, &m_handle);
    if (retval != NRF_SUCCESS)
    {
    	return false;	
    }
		
    pstorage_block_identifier_get(&m_handle, 0, &m_block_0_handle);
    return true;
}

uint16_t pstore_get(uint8_t *p_dest)
{
    uint16_t i;
    uint32_t retval;
    retval = pstorage_load(m_pstore_buffer, &m_block_0_handle, PSTORE_MAX_BLOCK, 0); // Read 1 block from flash
    if(retval != NRF_SUCCESS)
    {
	return 0;
    }
    // The ascii file is terminated by 0x00. Search for the very first 0x00.
    for (i = 0; i < PSTORE_MAX_BLOCK; i++)
    {
	if (m_pstore_buffer[i] == 0x00)
	    break;
    }
    if (i == PSTORE_MAX_BLOCK)
	return 0;

    memcpy(p_dest, m_pstore_buffer, i);
    return (i);
}

bool pstore_set(uint8_t *p_src, uint16_t len)
{
    uint32_t retval;

    memset(m_pstore_buffer, 0, PSTORE_MAX_BLOCK);
    memcpy(m_pstore_buffer, p_src, len);
    retval = pstorage_clear(&m_block_0_handle, PSTORE_MAX_BLOCK);                       
    if(retval != NRF_SUCCESS)
    {
	return false;
    }
    //Store data to the block. Wait for the last store operation to finish before reading out the data.
    pstorage_store(&m_block_0_handle, m_pstore_buffer, PSTORE_MAX_BLOCK, 0);
    if(retval != NRF_SUCCESS)
    {
         return false;
    }	
    m_wait_handle = m_block_0_handle.block_id;            //Specify which pstorage handle to wait for
    m_wait_flag = 1;                                    //Set the wait flag. Cleared in the pstore_handler
    while(m_wait_flag) { power_manage(); }              //Sleep until store operation is finished.
    return true;
}
