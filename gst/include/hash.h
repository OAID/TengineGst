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
 
#ifndef     __HASH_H__
#define     __HASH_H__

#include "linux_list.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct HASH_UNIT_T
{
	void *pData;
    union
    {
        int index;
	    char key_word[32];      
    }own;       /* 关键字或者索引,用来解决冲突 */
	struct list_head list;
}HASH_UNIT;

#ifdef  __cplusplus
class CSimpleHash
{
public:
    CSimpleHash(void);
    ~CSimpleHash(void);
    int Init(int size, int unit_size);
    void Uninit(void);
    unsigned int GetHashPos(const char *key_word);   
    unsigned int GetHashPos(int index);          
    void *AllocRecord(const char *key_word);
    void *AllocRecord(int index);        
    void *GetRecord(const char *key_word);
    void *GetRecord(int index);          

private:
    struct list_head *mListArray;
    int mSize;
    int mUnitSize;
};
#endif

#ifdef		__cplusplus
}
#endif

#endif

