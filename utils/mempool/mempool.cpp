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
 
#include "mempool.h"
#include "common.h"

pthread_mutex_t glock;
memory_pool_colume *b_p = NULL;
int buffer_pool_no = 0;
 
int buffer_pool_init(int colume_no, unsigned int block_len[], unsigned int block_count[])
{
	pthread_mutex_init(&glock, NULL);

	b_p = (memory_pool_colume *)malloc(sizeof(memory_pool_colume) * colume_no);
	if (b_p == NULL)
		return MP_MALLOC_FIAL;
	memset(b_p, 0, sizeof(memory_pool_colume) * colume_no);
	buffer_pool_no = colume_no;
 
	memory_pool_node * curr_node = NULL;
	memory_pool_node * new_node = NULL;
	for (int i = 0; i < colume_no; i++)
	{
		b_p[i].block_len = block_len[i];
		b_p[i].total_count = block_count[i];
		for (int j = 0; j < block_count[i]; j++)
		{
			new_node =(memory_pool_node *)malloc(sizeof(memory_pool_node));
			new_node->column = i;
			new_node->data =(unsigned char *) malloc(block_len[i]);
			memset(new_node->data, 0, block_len[i]);
			if (new_node == NULL || new_node->data == NULL)
				return MP_MALLOC_FIAL;
			new_node->next = NULL;
			if (j == 0)
			{
				b_p[i].free_header = new_node;
				curr_node = b_p[i].free_header;
			}
			else
			{
				curr_node->next = new_node;
				curr_node = curr_node->next;
			}
		}
	}
	return MP_OK;
}
 
memory_pool_node *buffer_malloc(unsigned int size)
{
	pthread_mutex_lock(&glock);
	memory_pool_node * node = NULL;
	if (size > b_p[buffer_pool_no - 1].block_len)
	{
		LOG_INFO("malloc size[%d] so big,need new from stack", size);
a:		
		node = (memory_pool_node *)malloc(sizeof(memory_pool_node));
		node->column = 9999;
		node->data = (unsigned char *)malloc(size);
		memset(node->data, 0, size);
		if (node == NULL || node->data == NULL)
			return NULL;
		node->next = NULL;		
		pthread_mutex_unlock(&glock);
		return node;
	}
	for (int i = 0; i < buffer_pool_no; i++)
	{
		if (size > b_p[i].block_len)
			continue;
		if (b_p[i].total_count - b_p[i].used_count == 0)
		{
			LOG_WARN("!!!!  size[%d] pool use up  !!!!", b_p[i].block_len);
			continue;
		}
		node = b_p[i].free_header;
		b_p[i].free_header = b_p[i].free_header->next;
		b_p[i].used_count++;
		node->next = b_p[i].used_header;
		b_p[i].used_header = node;
		pthread_mutex_unlock(&glock);
		return node;
	}
	LOG_WARN("!!!!  all of pool used up  !!!!");
	goto a;
}
 
int buffer_free(memory_pool_node * buffer)
{
	pthread_mutex_lock(&glock);
	memory_pool_node * node_cur = b_p[buffer->column].used_header;
	memory_pool_node * node_pre = NULL;
	if (buffer->column == 9999)
	{
		free(buffer->data);
		free(buffer);
		buffer = NULL;
		pthread_mutex_unlock(&glock);
		return MP_OK;
	}
	while(node_cur != NULL)
	{
		if (node_cur != buffer)
		{
			node_pre = node_cur;
			node_cur = node_cur->next;
			continue;
		}
		if (node_pre == NULL)
		{
			b_p[buffer->column].used_header = b_p[buffer->column].used_header->next;
		}
		else
		{
			node_pre->next = node_cur->next;
		}
		b_p[buffer->column].used_count--;
		node_cur->next = b_p[buffer->column].free_header;
		b_p[buffer->column].free_header = node_cur;
		break;
	}
	pthread_mutex_unlock(&glock);
	return MP_OK;
}
 
int buffer_pool_destory(void)
{
	memory_pool_node * node_cur = NULL;
	memory_pool_node * node_del = NULL;
	
	if (b_p == NULL)
		return MP_NOT_INIT;
	for (int i = 0; i < buffer_pool_no; i++)
	{
		node_cur = b_p[i].used_header;
		while (node_cur != NULL)
		{
			node_del = node_cur;
			node_cur = node_cur->next;
			free(node_del->data);
			free(node_del);
		}
		node_cur = b_p[i].free_header;
		while (node_cur != NULL)
		{
			node_del = node_cur;
			node_cur = node_cur->next;
			free(node_del->data);
			free(node_del);
		}
	}
	free(b_p);
	b_p = NULL;
	buffer_pool_no = 0;
	return MP_OK;
}
 
int buffer_runtime_print(void)
{
	if (b_p == NULL)
	{
		LOG_ERROR("buffer pool not init yet");
		return MP_NOT_INIT;
	}
	printf("\n*********************** memory pool runtime report start************************\n");
	for (int i = 0; i < buffer_pool_no; i++)
	{
		printf("pool no[%d] blocksize[%d] blockTotalCount[%d] usedBlock[%d] used percentage[%d%%]\n",
				i, b_p[i].block_len, b_p[i].total_count, b_p[i].used_count, b_p[i].used_count * 100 / b_p[i].total_count);
	}
	printf("*********************** memory pool runtime report end**************************\n");
	return MP_OK;
}

int mempool_init(void)
{
	int colume = 1;
	unsigned int len = 4 * STREAM_WIDTH * STREAM_HIGHT;
	unsigned int block_len[] = {len};
	unsigned int block_size[] = {500};

	buffer_pool_init(colume, block_len, block_size);

	return 0;
}

void mempool_deinit(void)
{
	buffer_pool_destory();
}

