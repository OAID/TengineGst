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
 
#ifndef		__POSTPROCESS_SERVER_CV_H__
#define		__POSTPROCESS_SERVER_CV_H__

#include "common.h"
#include <vector>
#include <queue>
#include <pthread.h>
#include <mutex>

using namespace std;

/* 
将service层的数据回调出去
1、已经叠加矩形框的数据帧
*/
typedef void (*POSTPROCESS_SERVICE_RES_CB)(FRAME_DATA_S *frame, void *userdata);

typedef struct POSTPROCESS_CHN_HANDLE_T
{
	POSTPROCESS_SERVICE_RES_CB callback;
	void *userdata;
}POSTPROCESS_CHN_HANDLE_S;

typedef struct POSTPROCESS_SERVICE_FRAME_T
{
	FRAME_DATA_S *frame;
	vector<BBOX_S> boxes;
	void *userdata;
}POSTPROCESS_SERVICE_FRAME_S;

#ifdef	__cplusplus
extern "C" {
#endif	

class CPostprocessService
{
public:
	static CPostprocessService *getInstance()
	{
		static CPostprocessService *obj = NULL;
		if (NULL == obj)
		{
			obj = new CPostprocessService();	
            obj->Init();
		}
		return obj;
	}  	
	CPostprocessService();
	~CPostprocessService();	
	int Init(void);
	void Deinit(void);
	POSTPROCESS_CHN_HANDLE_S *CreateChn(void);
	void DestoryChn(POSTPROCESS_CHN_HANDLE_S *handle);	
	void SetPostprocessServiceResCb(POSTPROCESS_CHN_HANDLE_S *handle, POSTPROCESS_SERVICE_RES_CB callback, void *userdata);
	int SendFrame(POSTPROCESS_CHN_HANDLE_S *handle, FRAME_DATA_S *frame, vector<BBOX_S>& boxes);
	int SendFrameSync(POSTPROCESS_CHN_HANDLE_S *handle, FRAME_DATA_S *frame, vector<BBOX_S>& boxes);
	std::mutex m_mtxVecFrame;
public:
	queue<POSTPROCESS_SERVICE_FRAME_S> mFrames;
	int mChCount;
	int mRunning;
	pthread_t mWorkId;
};

#ifdef	__cplusplus
}
#endif	

#endif


