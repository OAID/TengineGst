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
 
#ifndef		__OBJ_DETECT_H__
#define		__OBJ_DETECT_H__

#include "common.h"
#include "infer_service_plugin.h"
#include <vector>
#include <queue>
#include <pthread.h>
#include "baseinfer.h"

using namespace std;
class CObjdetectServ:public CBaseInfer
{
public:
	static CObjdetectServ *getInstance();
	virtual int RealWork(TINFER_CHN_HDL *pChnHdl);
	virtual int ChildDeinit();
private:
	virtual int initModel();
    void* m_pModelHdl;
public:
	CObjdetectServ();
	~CObjdetectServ();
	int do_initModel();
};

#endif