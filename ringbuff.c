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

/*****************************************************************************
 * Static functions
 *****************************************************************************/
#define RBUFF_ELEM_AT(p_rbuff, index) ((uint8_t*) ((uint8_t*) p_rbuff->elem_array) + (p_rbuff->elem_size) * (index))
#define RBUFF_IS_FULL(p_rbuff) (p_rbuff->tail + p_rbuff->array_len == p_rbuff->head)
#define RBUFF_IS_EMPTY(p_rbuff) (p_rbuff->tail == p_rbuff->head)
/*****************************************************************************
 * Interface functions
 *****************************************************************************/
void rbuff_init(rbuff_t* p_rbuff)
{
    /* round off to nearest(lower) size that can use & operator instead of modulo */
    uint32_t i = 32;
    while (!((p_rbuff->array_len >> --i) & 0x01));
    p_rbuff->array_len = (1 << i);

    p_rbuff->head = 0;
    p_rbuff->tail = 0;
}

uint32_t rbuff_push(rbuff_t* p_rbuff, const void* p_elem)
{
    if (p_elem == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if (RBUFF_IS_FULL(p_rbuff))
    {
        return NRF_ERROR_NO_MEM;
    }

    void* p_dest = RBUFF_ELEM_AT(p_rbuff, p_rbuff->head & (p_rbuff->array_len - 1));
    memcpy(p_dest, p_elem, p_rbuff->elem_size);

    ++p_rbuff->head;
    return NRF_SUCCESS;
}

uint32_t rbuff_pop(rbuff_t* p_rbuff, void* p_elem)
{
    if (RBUFF_IS_EMPTY(p_rbuff))
    {
        return NRF_ERROR_NULL;
    }

    if (p_elem != NULL)
    {
        void* p_src = RBUFF_ELEM_AT(p_rbuff, p_rbuff->tail & (p_rbuff->array_len - 1));
        memcpy(p_elem, p_src, p_rbuff->elem_size);
    }

    ++p_rbuff->tail;
    return NRF_SUCCESS;
}

uint32_t rbuff_peek_at(rbuff_t* p_rbuff, void* p_elem, uint32_t elem)
{
    if (p_elem == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if (rbuff_get_len(p_rbuff) <= elem)
    {
        return NRF_ERROR_NULL;
    }

    void* p_src = RBUFF_ELEM_AT(p_rbuff, (p_rbuff->tail + elem) & (p_rbuff->array_len - 1));
    memcpy(p_elem, p_src, p_rbuff->elem_size);
	
    return NRF_SUCCESS;
}

uint32_t rbuff_peek(rbuff_t* p_rbuff, void* p_elem)
{
    return rbuff_peek_at(p_rbuff, p_elem, 0);
}

uint32_t rbuff_peek_all(rbuff_t* p_rbuff, void* p_elem, uint32_t* p_length)
{
	uint32_t len = rbuff_get_len(p_rbuff);
	uint32_t i;
	uint32_t rc;
	
	for (i = 0; i < len; i++)
	{
        rc = rbuff_peek_at(p_rbuff, p_elem + (i * p_rbuff->elem_size), i);
		if (rc != NRF_SUCCESS)
			return (rc);
	}
	*p_length = len;
	return NRF_SUCCESS;
}

void rbuff_flush(rbuff_t* p_rbuff)
{
    p_rbuff->tail = p_rbuff->head;
}

uint32_t rbuff_get_len(rbuff_t* p_rbuff)
{
    return (p_rbuff->head - p_rbuff->tail);
}

inline bool rbuff_is_full(rbuff_t* p_rbuff)
{
    return RBUFF_IS_FULL(p_rbuff);
}

bool rbuff_is_empty(rbuff_t* p_rbuff)
{
    return RBUFF_IS_EMPTY(p_rbuff);
}

#ifdef UT_SUPPORT
#include "ringbuff_ut.c"
#endif