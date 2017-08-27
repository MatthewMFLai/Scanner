/***********************************************************************************
  Copyright (c) Nordic Semiconductor ASA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  3. Neither the name of Nordic Semiconductor ASA nor the names of other
  contributors to this software may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ************************************************************************************/
#include "ringbuff.h"
#include "nrf_error.h"
#include <string.h>
#include "utmgr.h"

/*****************************************************************************
 * Static data
 *****************************************************************************/
 static char module_desc[] = "ring buffer unit test driver";
 /*
 typedef struct
{
	uint16_t	num_of_arg;
	uint8_t 	argtype[UTMGR_MAX_ARGS];
} utmgr_test_argtype_t;
*/
static utmgr_test_argtype_t tc_1_args_desc = {4, {UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_STRING}};
static utmgr_test_argtype_t tc_1_ret_desc = {4, {UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_LWORD}};
 
 /*
 typedef struct
{
	char *p_test_id;
	uint8_t (*p_func_tc_prepare)(void *p_in[], void *p_out[]);
	void (*p_func_tc_run)(void);
	void (*p_func_tc_cleanup)(void);
    utmgr_test_argtype_t *p_in_desc;
	utmgr_test_argtype_t *p_out_desc;
} utmgr_test_desc_t;
*/

/*****************************************************************************
 * Static functions
 *****************************************************************************/
static uint8_t tc_1_prepare(void *p_in[], void *p_out[])
{
	return (0);
}

static void tc_1_run(void)
{
	
}

static void tc_1_cleanup(void)
{
	
}

static utmgr_test_desc_t tc_1_desc = {"rbf001",
                                      tc_1_prepare,
									  tc_1_run,
									  tc_1_cleanup,
									  &tc_1_args_desc,
									  &tc_1_ret_desc};
									  
static utmgr_test_desc_t *tc_array[] = {&tc_1_desc};

static utgmr_module_desc_t module_tc_desc = {module_desc, 1, (utmgr_test_desc_t *)tc_array};
/*****************************************************************************
 * Interface functions
 *****************************************************************************/
