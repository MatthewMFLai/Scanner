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

#include "rssi_filter.h"
#include "ringbuff.h"
#include "SEGGER_RTT.h"

typedef struct
{
	uint32_t ts;
	int8_t   rssi;
} rssi_tuple;

static rssi_tuple node0_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node1_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node2_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node3_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node4_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node5_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node6_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node7_rssi[RSSI_FILTER_MAX_SAMPLES];
static rssi_tuple node8_rssi[RSSI_FILTER_MAX_SAMPLES];

static rssi_tuple tmp_rssi[RSSI_FILTER_MAX_SAMPLES];

static rbuff_t node0_rb = {(void *)node0_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node1_rb = {(void *)node1_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node2_rb = {(void *)node2_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node3_rb = {(void *)node3_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node4_rb = {(void *)node4_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node5_rb = {(void *)node5_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node6_rb = {(void *)node6_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node7_rb = {(void *)node7_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};
static rbuff_t node8_rb = {(void *)node8_rssi, sizeof(rssi_tuple), RSSI_FILTER_MAX_SAMPLES, 0, 0};

static rbuff_t *nodes[RSSI_FILTER_MAX_NODES] = {&node0_rb, &node1_rb, &node2_rb, &node3_rb, &node4_rb,
                                                &node5_rb, &node6_rb, &node7_rb, &node8_rb};

static uint8_t nodes_age[RSSI_FILTER_MAX_NODES];

void rssi_filter_init(void)
{
	uint8_t i, j;
	rbuff_t *p_rb;
	rssi_tuple *p_rssi;
	
    for (i = 0; i < RSSI_FILTER_MAX_NODES; i++)
    {
		p_rb = nodes[i];
		for (j = 0; j < RSSI_FILTER_MAX_SAMPLES; j++)
		{
			p_rssi = (rssi_tuple *)(p_rb->elem_array + j);
		    p_rssi->rssi = RSSI_FILTER_MIN_RSSI_VALUE;
		    p_rssi->ts = 0;
		}
        rbuff_init(p_rb);
        nodes_age[i] = 0;		
	}		
}

void rssi_filter_update(uint16_t node_id, int8_t rssi, uint32_t ts)
{
	rbuff_t *p_rb;
	rssi_tuple new_rssi;

    p_rb = nodes[node_id];
	if (rbuff_is_full(p_rb))
        rbuff_pop(p_rb, (void *)&new_rssi);
		
	new_rssi.ts = ts;
	new_rssi.rssi = rssi;
	rbuff_push(p_rb, (void *)&new_rssi);
	
	// Update all age counters.
    for (uint8_t i = 0; i < RSSI_FILTER_MAX_NODES; i++)
    {
		if (nodes_age[i])
            nodes_age[i] -= 1;
    }
	nodes_age[node_id] = RSSI_FILTER_MAX_SAMPLES;
}

uint16_t rssi_filter_report_best(void)
{
	uint8_t i, j, best_node_id;
	uint32_t len;
	int32_t rssi_max = RSSI_FILTER_MIN_RSSI_VALUE;
	int32_t rssi_val;
	rbuff_t *p_rb;

	best_node_id = 1;
    for (i = 0; i < RSSI_FILTER_MAX_NODES; i++)
    {
		p_rb = nodes[i];
		rbuff_peek_all(p_rb, (void *)tmp_rssi, &len);
		
		if (!len || !nodes_age[i])
			continue;
		
		rssi_val = 0;
		for (j = 0; j < len; j++)
		    rssi_val += (int32_t)tmp_rssi[j].rssi;
		rssi_val /= (int32_t)len;
		
        if (rssi_val > rssi_max)
        {
			rssi_max = rssi_val;
			best_node_id = i;
		}			
	}
    return (best_node_id);	
}