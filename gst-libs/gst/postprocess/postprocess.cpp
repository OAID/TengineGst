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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "postprocess.h"
#include "normal.h"
#include "postprocess_service_cv.h"

class CPostprocess
{
public:
	CPostprocess();
	~CPostprocess();	
	int Init(void);
	void Deinit(void);
	int SendFrame(FRAME_DATA_S *frame, vector<BBOX_S>& boxes);
	int SendFrameSync(FRAME_DATA_S *frame, vector<BBOX_S>& boxes);
	void SetPostprocessResultCb(POSTPROCESS_RES_CB callback, void *userdata);	

	static void OnPostprocessServiceRes(FRAME_DATA_S *frame, void *userdata);	

private:
	CPostprocessService *mpPostprocessService;
	POSTPROCESS_CHN_HANDLE_S *mpPostprocessHandle;
	POSTPROCESS_RES_CB mPostprocessResultCb;
	void *mpUserdata;	
};
static CPostprocessService *gpPostprocessService = NULL; 

CPostprocess::CPostprocess()
{

}

CPostprocess::~CPostprocess()
{

}

int CPostprocess::Init(void)
{
	int ret;

	mpPostprocessService = gpPostprocessService;		/* need modify */
	mpPostprocessHandle = mpPostprocessService->CreateChn();
	if (NULL == mpPostprocessHandle)
	{
		LOG_ERROR("CreateChn fail");
		return -1;
	}
	mpPostprocessService->SetPostprocessServiceResCb(mpPostprocessHandle, CPostprocess::OnPostprocessServiceRes, this);

	return 0;	
}

void CPostprocess::Deinit(void)
{
	mpPostprocessService->DestoryChn(mpPostprocessHandle);
}

void CPostprocess::SetPostprocessResultCb(POSTPROCESS_RES_CB onRecvData, void *userdata)
{
	mPostprocessResultCb = onRecvData;
	mpUserdata = userdata;
}

int CPostprocess::SendFrame(FRAME_DATA_S *frame, vector<BBOX_S>& boxes)
{
	return mpPostprocessService->SendFrame(mpPostprocessHandle, frame, boxes);	
}
int CPostprocess::SendFrameSync(FRAME_DATA_S *frame, vector<BBOX_S>& boxes)
{
	return mpPostprocessService->SendFrameSync(mpPostprocessHandle, frame, boxes);	
}

/* 
1、接收inferService的数据 
2、完成本模块的逻辑处理
3、将处理结果回调给上一层
*/
void CPostprocess::OnPostprocessServiceRes(FRAME_DATA_S *frame, void *userdata)
{
	CPostprocess *pPostprocess = (CPostprocess *)userdata;

	pPostprocess->mPostprocessResultCb(frame, pPostprocess->mpUserdata);			
}

int CreatePostprocessService(void)
{
	gpPostprocessService = CPostprocessService::getInstance();
	if (NULL == gpPostprocessService)
	{
		LOG_ERROR("get infer service instance fail");
		return -1;
	}
	return 0;
}

POSTPROCESS_HANDLE CreatePostprocess(void)
{
	int ret;
	CPostprocess *pPostprocess = NULL;

	pPostprocess = new CPostprocess();
	ret = pPostprocess->Init();
	if (ret < 0)
	{
		LOG_ERROR("postprocess init fail[%d]", ret);
		delete pPostprocess;
		return NULL;
	}
	return pPostprocess;
}

void DestroyPostprocess(POSTPROCESS_HANDLE handle)
{
	CPostprocess *pPostprocess = (CPostprocess *)handle;

	if (NULL == handle)
	{
		LOG_ERROR("postprocess handle is null");
		return;
	}

	pPostprocess->Deinit();
	delete pPostprocess;
	pPostprocess = NULL;
}

void SetPostprocessCallback(POSTPROCESS_HANDLE handle, POSTPROCESS_RES_CB callback, void *userdata)
{
	CPostprocess *pPostprocess = (CPostprocess *)handle;

	if (NULL == handle)
	{
		LOG_ERROR("postprocess handle is null");
		return;
	}

	pPostprocess->SetPostprocessResultCb(callback, userdata);
}

int PostprocessSendFrame(POSTPROCESS_HANDLE handle, FRAME_DATA_S *frame, char *json)
{
	CPostprocess *pPostprocess = (CPostprocess *)handle;
	vector<BBOX_S> boxes;	

	if (NULL == handle)
	{
		LOG_ERROR("postprocess handle is null");
		return -1;
	}
	if(json)
		json_to_bbox(json, boxes);
	return pPostprocess->SendFrame(frame, boxes);
}

int PostprocessSendFrameSync(POSTPROCESS_HANDLE handle, FRAME_DATA_S *frame, char *json)
{
	CPostprocess *pPostprocess = (CPostprocess *)handle;
	vector<BBOX_S> boxes;
	if (NULL == handle)
	{
		LOG_ERROR("postprocess handle is null");
		return -1;
	}
	if(json)
		json_to_bbox(json, boxes);
	return pPostprocess->SendFrameSync(frame, boxes);
}