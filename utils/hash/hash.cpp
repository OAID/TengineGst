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
 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "linux_list.h"
#include "hash.h"
#include "common.h"
#include "loger.h"

#undef		TARGET_LEVEL
#define		TARGET_LEVEL	LEVEL_INFO

CSimpleHash::CSimpleHash(void)
{

}

CSimpleHash::~CSimpleHash(void)
{

}

unsigned int HashString(const char *str)		/* hash算法 */
{   
#if 0
	unsigned long hash = 0x71e2c3d8;
	
    while (*str != 0)    
    {        
        hash <<= 1;       
        hash += *str++;      
    } 
	
    return hash;
#else
	unsigned int h = 0;

	for (; *str; str++)
	{		
		h = *str + h * 27;
	}
	
	return h;
#endif	
}

unsigned int CSimpleHash::GetHashPos(const char *key_word)
{
    return (HashString(key_word) % mSize);
}

unsigned int CSimpleHash::GetHashPos(int index)
{
    return (index % mSize);
}

int CSimpleHash::Init(int size, int unit_size)
{
	int i;

	mSize = size;
	mUnitSize = unit_size;

	mListArray = (struct list_head *)calloc(size, sizeof(struct list_head));
	if (NULL == mListArray)
	{
		LOG_ERROR("calloc fail\n");
		return -1;
	}

	for (i = 0; i < size; i++)
		INIT_LIST_HEAD(&mListArray[i]);

	return 0;
}

void *CSimpleHash::AllocRecord(const char *key_word)		/* 用于插入表项 */
{
	int pos = GetHashPos(key_word);
	HASH_UNIT *pNewRecord = NULL;

	pNewRecord = (HASH_UNIT *)calloc(1, sizeof(HASH_UNIT));
	if (NULL == pNewRecord)
	{
		LOG_WARN("calloc fail");
		return NULL;
	}
	pNewRecord->pData = calloc(1, mUnitSize);
	if (NULL == pNewRecord)
	{
		LOG_WARN("calloc fail");
		free(pNewRecord);
		return NULL;
	}		
	strcpy(pNewRecord->own.key_word, key_word);	
	list_add_tail(&pNewRecord->list, &mListArray[pos]);
	
	return pNewRecord->pData;		
}

void *CSimpleHash::AllocRecord(int index)
{
	int pos = GetHashPos(index);
	HASH_UNIT *pNewRecord = NULL;

	pNewRecord = (HASH_UNIT *)calloc(1, sizeof(HASH_UNIT));
	if (NULL == pNewRecord)
	{
		LOG_WARN("calloc fail");
		return NULL;
	}
	pNewRecord->pData = calloc(1, mUnitSize);
	if (NULL == pNewRecord)
	{
		LOG_WARN("calloc fail");
		free(pNewRecord);
		return NULL;
	}		
	pNewRecord->own.index = index;	
	list_add_tail(&pNewRecord->list, &mListArray[pos]);
	
	return pNewRecord->pData;		
}

void *CSimpleHash::GetRecord(const char *key_word)		/* 用于获取表项 */
{
	int pos = GetHashPos(key_word);
	HASH_UNIT *pRecord = NULL;
		
	list_for_each_entry(pRecord, &mListArray[pos], list)
	{
		if (!strcmp(pRecord->own.key_word, key_word))
			break;
	}
		
	if (&pRecord->list == &mListArray[pos])
	{
		LOG_DEBUG("bad key_word[%s]", key_word);	
		return NULL;		
	}

	return pRecord->pData;
}

void *CSimpleHash::GetRecord(int index)		/* 用于获取表项 */
{
	int pos = GetHashPos(index);
	HASH_UNIT *pRecord = NULL;
		
	list_for_each_entry(pRecord, &mListArray[pos], list)
	{
		if (pRecord->own.index == index)
			break;
	}
		
	if (&pRecord->list == &mListArray[pos])
	{
		LOG_INFO("bad index[0x%x]", index);	
		return NULL;		
	}

	return pRecord->pData;
}

void CSimpleHash::Uninit(void)
{
	int i;
	HASH_UNIT *n_pRecord = NULL, *pRecord = NULL;

	for (i = 0; i < mSize; i++)
	{
		list_for_each_entry_safe(pRecord, n_pRecord, &mListArray[i], list)
		{
			list_del(&pRecord->list);
			free(pRecord);
		}
	}
	free(mListArray);
}

