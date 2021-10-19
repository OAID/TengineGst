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

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

#include "postprocess_service_cv.h"
/* GetRValue */
#define COLOR_GET_RED(rgb)    ((unsigned char)((rgb) & 0xff)) 
/* GetGValue */
#define COLOR_GET_GREEN(rgb)  ((unsigned char)(((rgb) >> 8) & 0xff)) 
/* GetBValue */
#define COLOR_GET_BLUE(rgb)   ((unsigned char)(((rgb)>>16) & 0xff)) 

int draw_rect(unsigned char *yuv, BBOX_S &rect)
{
	cv::Mat frame(1080 * 1.5, 1920, CV_8UC1, (unsigned char *)yuv, 0);

	//cv::rectangle(frame, cv::Rect(rect.x, rect.y, rect.width, rect.heigh), cv::Scalar(255, 0, 0), 2, 1, 0);
	//cv::rectangle(frame, cv::Rect(rect.x, rect.y, rect.width, rect.heigh), 
	//	cv::Scalar(COLOR_GET_BLUE(rect.uColor),COLOR_GET_GREEN(rect.uColor),COLOR_GET_RED(rect.uColor)),
	//	2, 1, 0);
	cv::rectangle(frame, cv::Point(rect.x, rect.y), cv::Point(rect.x + rect.width, rect.y + rect.heigh), 
		cv::Scalar(COLOR_GET_BLUE(rect.uColor),COLOR_GET_GREEN(rect.uColor),COLOR_GET_RED(rect.uColor)),
		2, 1, 0);
#if 0	
	printf("[%d_%s] draw box: %lf_%lf_%lf_%lf,clr=%d:%d:%d.\n",
		__LINE__, __func__, rect.x, rect.y, rect.width, rect.heigh,
		COLOR_GET_RED(rect.uColor),COLOR_GET_GREEN(rect.uColor),COLOR_GET_BLUE(rect.uColor));
#endif 
	return 0;
}

int draw_rects(unsigned char *yuv, vector<BBOX_S> &rects)
{
	cv::Mat frame(1080 * 1.5, 1920, CV_8UC1, (unsigned char *)yuv, 0);

	for (int i = 0; i < (int)rects.size(); i++){
		//cv::rectangle(frame, cv::Rect(rects[i].x, rects[i].y, rects[i].width, rects[i].heigh), 
		//	cv::Scalar(COLOR_GET_BLUE(rects[i].uColor),COLOR_GET_GREEN(rects[i].uColor),COLOR_GET_RED(rects[i].uColor)),
		//	2, 1, 0);
		cv::rectangle(frame, cv::Point(rects[i].x, rects[i].y), cv::Point(rects[i].x + rects[i].width, rects[i].y + rects[i].heigh), 
			cv::Scalar(COLOR_GET_BLUE(rects[i].uColor),COLOR_GET_GREEN(rects[i].uColor),COLOR_GET_RED(rects[i].uColor)),
			2, 1, 0);
	}
	
	return 0;
}

void *worker_post(void *parg)
{
	CPostprocessService *pPostProcessService = (CPostprocessService *)parg;
	POSTPROCESS_CHN_HANDLE_S *pChHandle = NULL;
	POSTPROCESS_SERVICE_FRAME_S frame;
	vector<BBOX_S> boxes;

	POINT_CHECK_RETURN_NULL(parg);	
	int nRetPos = 0;
	int nFrameSz = 0;

	while (pPostProcessService->mRunning)
	{
		if (pPostProcessService->mFrames.empty())
		{
			usleep(10 * 1000);
			continue;
		}
		nRetPos = 0;
		pPostProcessService->m_mtxVecFrame.lock();
		frame = pPostProcessService->mFrames.front();
		pPostProcessService->mFrames.pop();
		nFrameSz = pPostProcessService->mFrames.size();
		pPostProcessService->m_mtxVecFrame.unlock();
		if (nFrameSz > 30)
		{
			LOG_WARN("!!!!!! postprocess service queue is full,memory full");
			goto clear_mem;
		}
		pChHandle = (POSTPROCESS_CHN_HANDLE_S *)frame.userdata;
		if (NULL == pChHandle)
		{
			LOG_ERROR("infer ch handle is null");
			usleep(10 * 1000);
			goto clear_mem;
		}
		if(frame.boxes.size()>0)
			draw_rects(frame.frame->data, frame.boxes);
		if (pChHandle->callback)
			pChHandle->callback(frame.frame, pChHandle->userdata);
		nRetPos = 1;
clear_mem:
		if(nRetPos<1){
			free(frame.frame->data);
			free(frame.frame);
		}
	}

	return NULL;
}

CPostprocessService::CPostprocessService()
{
	mChCount = 0;	
	mRunning = 0;
}

CPostprocessService::~CPostprocessService()
{

}

int CPostprocessService::Init(void)
{
	mRunning = 1;
	pthread_create(&mWorkId, NULL, worker_post, this);

	return 0;
}

void CPostprocessService::Deinit(void)
{
	mRunning = 0;
	pthread_join(mWorkId, NULL);
}

POSTPROCESS_CHN_HANDLE_S *CPostprocessService::CreateChn(void)
{
	POSTPROCESS_CHN_HANDLE_S *postprocess = NULL;

	postprocess = (POSTPROCESS_CHN_HANDLE_S *)calloc(1, sizeof(POSTPROCESS_CHN_HANDLE_S));
	if (NULL == postprocess)
	{
		LOG_ERROR("malloc fail[%zd]", sizeof(POSTPROCESS_CHN_HANDLE_S));
		return NULL;
	}
	mChCount++;

	return postprocess;
}

void CPostprocessService::DestoryChn(POSTPROCESS_CHN_HANDLE_S *handle)
{
	if (NULL == handle)
	{
		LOG_ERROR("handle is null");
		return;
	}
	free(handle);
}

void CPostprocessService::SetPostprocessServiceResCb(POSTPROCESS_CHN_HANDLE_S *handle, POSTPROCESS_SERVICE_RES_CB callback, void *userdata)
{
	handle->callback = callback;
	handle->userdata = userdata;
}

/* buger:帧乱序 */
int CPostprocessService::SendFrame(POSTPROCESS_CHN_HANDLE_S *handle, FRAME_DATA_S *frame, vector<BBOX_S>& boxes)
{
	POSTPROCESS_SERVICE_FRAME_S postprocess_service_frame;
	postprocess_service_frame.frame = frame;
	postprocess_service_frame.boxes = boxes;
	postprocess_service_frame.userdata = handle;
	m_mtxVecFrame.lock();
	mFrames.push(postprocess_service_frame);
	m_mtxVecFrame.unlock();

	return 0;
}

int CPostprocessService::SendFrameSync(POSTPROCESS_CHN_HANDLE_S *handle, FRAME_DATA_S *frame, vector<BBOX_S>& boxes)
{	
	if(boxes.size()>0)
		draw_rects(frame->data, boxes);
	return 0;
}

