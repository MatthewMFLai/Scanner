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
#ifndef _RBUFF_H_
#define _RBUFF_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
  void* elem_array;
  uint32_t elem_size;
  uint32_t array_len;
  uint32_t head;
  uint32_t tail;
} rbuff_t;

void rbuff_init(rbuff_t* p_rbuff);
uint32_t rbuff_push(rbuff_t* p_rbuff, const void* p_elem);
uint32_t rbuff_pop(rbuff_t* p_rbuff, void* p_elem);
uint32_t rbuff_peek_at(rbuff_t* p_rbuff, void* p_elem, uint32_t elem);
uint32_t rbuff_peek(rbuff_t* p_rbuff, void* p_elem);
uint32_t rbuff_peek_all(rbuff_t* p_rbuff, void* p_elem, uint32_t* p_length);
void rbuff_flush(rbuff_t* p_rbuff);
uint32_t rbuff_get_len(rbuff_t* p_rbuff);
bool rbuff_is_full(rbuff_t* p_rbuff);
bool rbuff_is_empty(rbuff_t* p_rbuff);

#endif /* _RBUFF_H_ */
