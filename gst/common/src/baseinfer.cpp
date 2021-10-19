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

#include "objdetect.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "normal.h"
#include "loger.h"

using namespace cv;

static void *worker(void *parg);
CBaseInfer::CBaseInfer():
	m_szOriPic(cv::Size(1920,1080)),
    mRunning(0){
	memset(m_blob_shape,0,4*sizeof(int));
	
	m_nCheckWidth = 50;
	m_nThreshold = 80;
	mModelState = 0;
	m_nShapeW = m_nShapeH = 0;
	m_tsCfgPath[0] = '\0';
}

CBaseInfer::~CBaseInfer(){
}

static int s_vtClr[] = {0x0000FF,0x00FAFA,0xFF0000,0x00FF00,0x962FEB,0x8F00FF};

int CBaseInfer::initModel()
{
	return 0;
}

int CBaseInfer::LoadCfgFromFile()
{
	Json::Value jvRoot;
	if(strlen(m_tsCfgPath)>0 && loadcfg2json(m_tsCfgPath,jvRoot))
	{
		int nDetectMinSize = jvRoot["min_size"].asInt();
		if(2 == nDetectMinSize)
		{
			m_nCheckWidth = 100;
		}
		else if(1 == nDetectMinSize)
		{
			m_nCheckWidth = 50;
		}
		else
		{
			m_nCheckWidth = 36;
    	}
		m_nThreshold = jvRoot["threshold"].asInt();
	}
	return 0;
}

int CBaseInfer::SaveCfg2File()
{
	Json::Value jvRoot;
	jvRoot["threshold"] = m_nThreshold;
	if (m_nCheckWidth == 36)
	{
		jvRoot["min_size"] = 0;
	}
	else if (m_nCheckWidth == 50)
	{
		jvRoot["min_size"] = 1;
	}
	else
	{
		jvRoot["min_size"] = 2;
	}
	savejson2cfg(m_tsCfgPath,jvRoot);
	return 0;
}

int CBaseInfer::Init(INFER_SERV_Msg_CB cbMsg,void *user){
	LoadCfgFromFile();
	//if(m_vecClient.size()<=0)
	{
		mRunning = 1;
		pthread_create(&mWorkId, NULL, worker, this);
	}
	//m_vecClient.push_back(user);
	return 0;
}

int CBaseInfer::Deinit(void* user){
   SaveCfg2File();
	//删除模型，清理内存
	//if(m_vecClient.size()<=0)
	{
		mRunning = 0;
		pthread_join(mWorkId, NULL);
		ChildDeinit();
		for (int i = 0; i < (int)m_vecChn.size(); i++)
		{
			TINFER_CHN_HDL *handle = (TINFER_CHN_HDL *)m_vecChn[i];
		
			if(handle->arrframe[0].data)
			free(handle->arrframe[0].data);
			if(handle->arrframe[1].data)
				free(handle->arrframe[1].data);
			if(handle->pBufInfer)
				delete handle->pBufInfer;
			free(handle);
		}
		m_vecChn.clear();
	}
	return 0;
}

TINFER_CHN_HDL* CBaseInfer::find(void *usr)
{
	for (int i = 0; i < (int)m_vecChn.size(); i++)
	{
		TINFER_CHN_HDL *chnIns = (TINFER_CHN_HDL *)m_vecChn[i];
	
		if ((long)chnIns->userdata==(long)usr)
			return chnIns;
	}
	return NULL;
}

TINFER_CHN_HDL *CBaseInfer::CreateChn(INFER_SERV_RES_CB cbRes,INFER_SERV_Msg_CB cbMsg,void *user){
	if(NULL==find(user))
	{
		TINFER_CHN_HDL *pTmpChn = (TINFER_CHN_HDL*)malloc(sizeof(TINFER_CHN_HDL));
		if(pTmpChn)
		{
			memset((void*)pTmpChn,0,sizeof(TINFER_CHN_HDL));
		}
		pTmpChn->callback = cbRes;
		pTmpChn->cbMsg = cbMsg;
		pTmpChn->userdata = user;
		pTmpChn->mCurInfer = -1;
		if(m_nShapeH>0 && m_nShapeW >0)
		   	pTmpChn->pBufInfer = new uint8_t[m_nShapeH*m_nShapeW*m_nShapeC];
		m_vecChn.push_back(pTmpChn);
		LOG_INFO("chnNum=%d,h:w:c=%d:%d:%d", m_vecChn.size(),m_nShapeH,m_nShapeW,m_nShapeC);
		return pTmpChn;
	}
	
	return NULL;
}

int CBaseInfer::DestoryChn(TINFER_CHN_HDL *handle){
	//vec 里面清理
	if(NULL==handle)
		return -1;
	LOG_INFO("[DestoryChn]->chnNum=%d,st=%d,curInfer=%d,len=%d.", m_vecChn.size(),handle->mCopySt,
		handle->mCurInfer,handle->arrframe[handle->mCurInfer].len);
	handle->mIsDel = 1;
	if(handle->arrframe[handle->mCurInfer].len>0)
		sleep(1);
	handle->mCopySt = enMMST_Start;
	vector<TINFER_CHN_HDL*>::iterator it = m_vecChn.begin();
	while(it!= m_vecChn.end())
	{
		if(*it==handle){
			m_vecChn.erase(it);
			break;
		}
	}
	LOG_INFO("[DestoryChn]->chnNum=%d,st=%d,curInfer=%d,len=%d erase.", m_vecChn.size(),handle->mCopySt,
		handle->mCurInfer,handle->arrframe[handle->mCurInfer].len);
	for(int nIdx=0 ; nIdx<2 ; ++nIdx){
		if(handle->arrframe[nIdx].data)
			free(handle->arrframe[nIdx].data);
	}
	if(handle->pBufInfer)
		delete handle->pBufInfer;
	free(handle);

	return 0;
}

int CBaseInfer::SendFrame(TINFER_CHN_HDL *handle, FRAME_DATA_S *frame){	
	TINFER_CHN_HDL * pChnHdl = find(handle->userdata);
	if(pChnHdl && m_nShapeH>0 && 0==handle->mIsDel)
	{
		pChnHdl->mCopySt = enMMST_Start;
		int nCurSave = (pChnHdl->mCurInfer == -1) ? 0 : !pChnHdl->mCurInfer;
		if(-1 == pChnHdl->mCurInfer)
			pChnHdl->mCurInfer = 0;
		if(pChnHdl->arrframe[nCurSave].width!=frame->width && pChnHdl->arrframe[nCurSave].data){
			free(pChnHdl->arrframe[nCurSave].data);
			pChnHdl->arrframe[nCurSave].data = NULL;
		}
		if(pChnHdl->arrframe[nCurSave].data == NULL){
			pChnHdl->arrframe[nCurSave].data = (unsigned char*)malloc(frame->len);
		}
		memcpy(pChnHdl->arrframe[nCurSave].data, frame->data, frame->len);
		pChnHdl->arrframe[nCurSave].nChnID = frame->nChnID;
		pChnHdl->arrframe[nCurSave].len = frame->len;
		pChnHdl->arrframe[nCurSave].width = frame->width;
		pChnHdl->arrframe[nCurSave].height = frame->height;
		pChnHdl->mCopySt = enMMST_End;
	}
	return 0;
}

int CBaseInfer::SetChnAttr(TINFER_CHN_HDL *handle,int nPropID,int nPropType,void *dtAttr){
	switch(nPropID){
		case enATTR_FPS:
		m_nFps = *(int*)dtAttr;
		break;
		case enATTR_MINI_SIZE:
		m_nCheckWidth = *(int*)dtAttr;
		break;
		case enATTR_BODY_THRESHOLD:
		m_nThreshold = *(int*)dtAttr;
		break;
		case enATTR_MODELURL:
		//比较一下是否更改，更改，就下载，并下载成功后，替换
		break;
	}
	return 0;
}

int CBaseInfer::GetChnAttr(TINFER_CHN_HDL *handle,int nPropID,void *dtAttr){
	return 0;
}

int CBaseInfer::InMsg(TINFER_CHN_HDL *handle,TPluginMsg *pMsgData)
{
	if(pMsgData){
		switch(pMsgData->nCmd){
			/*case EAIS_CMD_GET_BODY_CONFIG:
			{

			}
			break;
			case EAIS_CMD_SET_BODY_CONFIG:
			{
				//设置阈值
				float fDetectThreshold = 80;
				//mini size ，如果检测的框小于多少，就不添加
			}
			break;*/
		}
	}
	return 0;
}

int pre_img(const cv::Mat& src, uint8_t* dst, int* input_shape){
    int inputH = input_shape[2];
    int inputW = input_shape[3];

    cv::Mat resized_mat(inputH, inputW, src.type());
    cv::resize(src, resized_mat, resized_mat.size(), 0, 0);
    //cv::cvtColor(resized_mat, resized_mat, cv::COLOR_BGR2RGB);
    //package2plannar
    int offset = inputH * inputW;
    uint8_t *out_data0 = reinterpret_cast<uint8_t *>(dst);
    uint8_t *out_data1 = reinterpret_cast<uint8_t *>(dst) + offset;
    uint8_t *out_data2 = reinterpret_cast<uint8_t *>(dst) + offset * 2;
    uint8_t *in_data = resized_mat.ptr<uint8_t>();
    for (int hh = 0; hh < inputH; ++hh) {
        for (int ww = 0; ww < inputW; ++ww) {
            *(out_data0++) = *(in_data++);
            *(out_data1++) = *(in_data++);
            *(out_data2++) = *(in_data++);
        }
    }
    return 0;
}

#include <time.h>
#include <sys/time.h>
  static double __tic = 0.0;                     
  static double __toc = 0.0;  
  void TIC(void)
  {
      struct timeval time_v = {0};
      gettimeofday(&time_v, NULL);
      __tic = (double)(time_v.tv_sec * 1e6 + time_v.tv_usec);
  }

  void TOC(const char* func_name)
  {
      struct timeval time_v = {0};
      gettimeofday(&time_v, NULL);
      __toc = (double)(time_v.tv_sec * 1e6 + time_v.tv_usec);
      LOG_INFO("%s take %.2f ms\n", func_name, (__toc - __tic) / 1000.0);
  }

/*
坐标回算
*/
void Rect_Ajust(BBOX_S *box, int stream_width, int stream_height, int net_width, int net_height)
{
	box->x = box->x * stream_width / net_width;
	box->y = box->y * stream_height / net_height;
	box->width = box->width * stream_width / net_width;
	box->heigh = box->heigh * stream_height / net_height;	
}

int CBaseInfer::do_work()
{
	//遍历通道，对通道的当前检测的帧检测
	do{
		int bInfer = 0;
		for (int i = 0; i < (int)m_vecChn.size(); i++)
		{
			TINFER_CHN_HDL *pChnHdl = (TINFER_CHN_HDL *)m_vecChn[i];
			if(pChnHdl)
			{
				if(pChnHdl->mCopySt==enMMST_End &&
				   pChnHdl->mCurInfer!=-1 &&
				   0==pChnHdl->mIsDel)
				{
					if(pChnHdl->arrframe[pChnHdl->mCurInfer].len==0){
						pChnHdl->mCurInfer = !pChnHdl->mCurInfer;
						continue;
					}
					bInfer = RealWork(pChnHdl);
					pChnHdl->arrframe[pChnHdl->mCurInfer].len = 0;
				}
			}
		}
		bInfer==0?usleep(20*1000):usleep(1000);
	}while (1);
}

static void *worker(void *parg)
{
	CBaseInfer *pInferService = (CBaseInfer *)parg;
	if(pInferService)
		pInferService->do_work();
	return NULL;
}

