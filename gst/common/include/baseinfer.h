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

#ifndef		__BASE_INFER_H__
#define		__BASE_INFER_H__

#include "common.h"
#include "infer_service_plugin.h"
#include <vector>
#include <queue>
#include <pthread.h>
//#include "corecv.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>


#include "inferinf.h"

using namespace std;
enum IMAGE_TYPE_E
{
    IMAGE_TYPE_PACKAGE = 1,    // Package 3-channel
    IMAGE_TYPE_PLANAR  = 2     // Planar 3-channel
};

enum{
	enATTR_FPS,
	enATTR_MINI_SIZE,
	enATTR_BODY_THRESHOLD,
	enATTR_MODELURL=7
};

void TIC(void);
void TOC(const char* func_name);
void Rect_Ajust(BBOX_S *box, int stream_width, int stream_height, int net_width, int net_height);
class CBaseInfer
{
public:
	virtual int Init(INFER_SERV_Msg_CB cbMsg,void *user);
	virtual int Deinit(void*);
	virtual TINFER_CHN_HDL *CreateChn(INFER_SERV_RES_CB cbRes,INFER_SERV_Msg_CB cbMsg,void *user);
	virtual int DestoryChn(TINFER_CHN_HDL *handle);
	virtual int SendFrame(TINFER_CHN_HDL *handle, FRAME_DATA_S *frame);
	virtual int SetChnAttr(TINFER_CHN_HDL *handle,int nPropID,int nPropType,void *dtAttr);
	virtual int GetChnAttr(TINFER_CHN_HDL *handle,int nPropID,void *dtAttr);
	virtual int InMsg(TINFER_CHN_HDL *handle,TPluginMsg *data);
	
	virtual int do_work();
	virtual int RealWork(TINFER_CHN_HDL *pChnHdl)=0;
	virtual int ChildDeinit(){return 0;};
protected:
	virtual int LoadCfgFromFile();
	virtual int SaveCfg2File();
	TINFER_CHN_HDL* find(void *usr);
	virtual int initModel();
    int m_blob_shape[4];
	int m_nShapeC;
	int m_nShapeW;
	int m_nShapeH;
    // 图像格式
    IMAGE_TYPE_E m_img_type;
    // 原图大小
    cv::Size m_szOriPic;
	// 模型要求宽高
    cv::Size m_szModelInput;
	int m_nFps;
	// 检测图片宽高
	int m_nCheckWidth; 
	// 检测阈值
	int m_nThreshold;
	char m_tsCfgPath[128];
	char m_tsModelUrl[1024];
public:
	CBaseInfer();
	~CBaseInfer();
	//添加的通道
	vector<TINFER_CHN_HDL*> m_vecChn;
	//加锁
	vector<void*> m_vecClient;
	INFER_SERV_Msg_CB m_cbMsg;
	
	int mRunning;
	pthread_t mWorkId;
	int mModelState;	//0 准备，1 完成
};

#endif