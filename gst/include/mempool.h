/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2021, OPEN AI LAB
 * Author: wlwu@openailab.com
 */
 
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "loger.h"

#ifdef	__cplusplus
extern "C" { 
#endif
 
#define MP_OK			0
#define	MP_MALLOC_FIAL	-1
#define MP_NOT_INIT		-2
 
typedef struct bpn memory_pool_node;

struct bpn{
	unsigned int column;
	unsigned char *data;
	memory_pool_node *next;
};
 
typedef struct{
	unsigned int total_count;
	unsigned int used_count;
	unsigned int block_len;
	memory_pool_node *free_header;
	memory_pool_node *used_header;
}memory_pool_colume;
 
int buffer_pool_init(unsigned int colume_no, unsigned int block_len[], unsigned int block_count[]);
memory_pool_node * buffer_malloc(unsigned int size);
int buffer_free(memory_pool_node * buffer);
int buffer_pool_destory(void);
int buffer_runtime_print(void);
int mempool_init(void);
void mempool_deinit(void);

#ifdef	__cplusplus
}
#endif

#endif

