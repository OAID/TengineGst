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

#include "videoanalysis.h"
#include "normal.h"
#ifdef WIN32
#else
#include <dirent.h>
#include <dlfcn.h>
#endif

#include "infer_service_plugin.h"
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>

using namespace std;

#ifdef	__cplusplus
class CVideoAnalysis
{
public:
	CVideoAnalysis();
	~CVideoAnalysis();	
	int Init(char *model);
	void Deinit(void);
	int SendFrame(FRAME_DATA_S *frame);
	void SetFaceResultCB(OBJDETECT_RESULT_CB callback, void *userdata);
	int InMsg(TPluginMsg *oMsgData);
	int SetAttr(int nPropID,int nPropType,void *pAttr);

	static void OnInferServiceResult(void *frame, void* boxes, void *userdata);
	static int OnInferServiceMsg(int nCmd,void *bufMsg, void *userdata);
	void OnInferResult_inner(FRAME_DATA_S *frame, vector<BBOX_S>* boxes);
	static void *m_hdlPlugin;

private:
	int32_t Open_plugin(const char* url,void* pHost);
	void* Get_symbol(const std::string& name);
	const std::string m_tsPlugin_CreateFuncName{"Plugin_Create"};
	static IForwardServ *m_libPlugin;

	void *m_pChnHdl;
	OBJDETECT_RESULT_CB mInferResultCb;
	void *mpUserdata;
};
#endif


CVideoAnalysis::CVideoAnalysis()
{
	
}

CVideoAnalysis::~CVideoAnalysis()
{

}

IForwardServ* CVideoAnalysis::m_libPlugin = NULL;
void *CVideoAnalysis::m_hdlPlugin = NULL;

void* CVideoAnalysis::Get_symbol(const std::string& name)
{
	void *func = ::dlsym(CVideoAnalysis::m_hdlPlugin, name.c_str());	
	LOG_INFO("Get_symbol %s 0X%x func=%x,err=%s", name.c_str(), CVideoAnalysis::m_hdlPlugin, func,::dlerror());
	if (func == nullptr) {
        throw std::runtime_error(::dlerror());
    }
    return func;
}

int32_t CVideoAnalysis::Open_plugin(const char *url, void *pHost)
{
    int nRet = -1;	
	if (CVideoAnalysis::m_libPlugin!=NULL){
		return -1;
	}
	
	auto handle = ::dlopen(url, RTLD_NOW|RTLD_GLOBAL);//报错。。。
    void* create = nullptr;
    IForwardServ* p = nullptr;
	if (handle == nullptr){
	    LOG_ERROR("[Open_plugin]->dlopen[%s] err:%s.", url, dlerror());
	    nRet = -1;
        goto loadErr;
	}

	CVideoAnalysis::m_hdlPlugin = handle;
	create = Get_symbol(m_tsPlugin_CreateFuncName);
	if (create == nullptr){
	    dlclose(CVideoAnalysis::m_hdlPlugin);
	    LOG_ERROR("[dlopen] %s 0X%x err %s", m_tsPlugin_CreateFuncName.c_str(), CVideoAnalysis::m_hdlPlugin, dlerror());
	    nRet = -2;
        goto loadErr;
	}
    //传入 manager ，以及 回调函数，用回调函数添加消息，区分异步/同步消息
	p = (reinterpret_cast<IForwardServ* (*)()>(create))();
	if (p == nullptr){
	    dlclose(CVideoAnalysis::m_hdlPlugin);
	    LOG_ERROR("[dlopen] %s", dlerror());
	    nRet = -3;
        goto loadErr;
	}
	CVideoAnalysis::m_libPlugin = p;
	CVideoAnalysis::m_libPlugin->InitService(CVideoAnalysis::OnInferServiceMsg, this);
	nRet = 0;
loadErr:
	LOG_INFO("[dlopen] %s ret=%d %s", url, nRet,dlerror());
    return nRet;
}

int CVideoAnalysis::Init(char *model)
{
	int ret;
	
	Open_plugin(model,this);
	if (CVideoAnalysis::m_libPlugin)
	{
		m_pChnHdl = CVideoAnalysis::m_libPlugin->CreateChn(CVideoAnalysis::OnInferServiceResult,
			CVideoAnalysis::OnInferServiceMsg, this);
	}
	LOG_INFO("CVideoAnalysis::Init");	

	return 0;
}

void CVideoAnalysis::Deinit(void)
{
	if(CVideoAnalysis::m_libPlugin)
		CVideoAnalysis::m_libPlugin->DestoryChn(m_pChnHdl);
}

void CVideoAnalysis::SetFaceResultCB(OBJDETECT_RESULT_CB onRecvData, void *userdata)
{
	mInferResultCb = onRecvData;
	mpUserdata = userdata;
}

int CVideoAnalysis::SendFrame(FRAME_DATA_S *frame)
{
	if(CVideoAnalysis::m_libPlugin)
		return CVideoAnalysis::m_libPlugin->SendFrame(m_pChnHdl, frame);	
	return -1;
}

int CVideoAnalysis::InMsg(TPluginMsg *oMsgData)
{
	if(CVideoAnalysis::m_libPlugin){
		return CVideoAnalysis::m_libPlugin->InMsg(m_pChnHdl,oMsgData);
	}
	return -1;
}

int CVideoAnalysis::SetAttr(int nPropID,int nPropType,void *pAttr)
{
	if(CVideoAnalysis::m_libPlugin){
		return CVideoAnalysis::m_libPlugin->SetChnAttr(m_pChnHdl,nPropID, nPropType, pAttr);
	}
	return -1;
}
/* 
1、接收inferService的数据 
2、完成本模块的逻辑处理
3、将处理结果回调给上一层
*/
void CVideoAnalysis::OnInferResult_inner(FRAME_DATA_S *frame, vector<BBOX_S>* boxes)
{
	char json[10 * 1024] = {0};

	/* 打包给上层 */
	bbox_to_json(*boxes, json);
	mInferResultCb(frame, json, mpUserdata);			
}

int CVideoAnalysis::OnInferServiceMsg(int nCmd,void *bufMsg, void *userdata)
{
	CVideoAnalysis *pInfer = (CVideoAnalysis *)userdata;

	return 0;
}

void CVideoAnalysis::OnInferServiceResult(void *frame, void* boxes, void *userdata)
{
	CVideoAnalysis *pInfer = (CVideoAnalysis *)userdata;
	if (pInfer)
		pInfer->OnInferResult_inner((FRAME_DATA_S *)frame, (vector<BBOX_S>*) boxes);
}

OBJDETECT_HANDLE CreateVideoAnalysis(char *config)
{
	int ret;
	CVideoAnalysis *pObjDetect = NULL;

	pObjDetect = new CVideoAnalysis();
	ret = pObjDetect->Init(config);
	if (ret < 0)
	{
		LOG_ERROR("[CVideoAnalysis] init fail[%d]", ret);
		delete pObjDetect;
		return NULL;
	}
	return pObjDetect;
}

void DestroyVideoAnalysis(OBJDETECT_HANDLE handle)
{
	CVideoAnalysis *pObjDetect = (CVideoAnalysis *)handle;

	if (NULL == handle)
	{
		LOG_ERROR("[CVideoAnalysis] handle is null");
		return;
	}

	pObjDetect->Deinit();
	delete pObjDetect;
	pObjDetect = NULL;
}

void SetVideoAnalysisCallback(OBJDETECT_HANDLE handle, OBJDETECT_RESULT_CB callback, void *userdata)
{
	CVideoAnalysis *pObjDetect = (CVideoAnalysis *)handle;

	if (NULL == handle)
	{
		LOG_ERROR("[face] handle is null");
		return;
	}

	pObjDetect->SetFaceResultCB(callback, userdata);
}

int VideoAnalysisSendFrame(OBJDETECT_HANDLE handle, FRAME_DATA_S *frame)
{
	CVideoAnalysis *pObjDetect = (CVideoAnalysis *)handle;

	if (NULL == handle)
	{
		LOG_ERROR("[VideoAnalysisSendFrame]->sendframe handle is null");
		return -1;
	}
	
	return pObjDetect->SendFrame(frame);
}

int VideoAnalysisSendMsg(OBJDETECT_HANDLE handle, TPluginMsg *oMsgData)
{
	CVideoAnalysis *pObjDetect = (CVideoAnalysis *)handle;

	if (NULL == handle)
	{
		LOG_ERROR("face handle is null");
		return -1;
	}
	
	return pObjDetect->InMsg(oMsgData); 
}

int VideoAnalysisSetAttr(OBJDETECT_HANDLE handle,int nPropID,int nPropType,void *pAttr)
{
	CVideoAnalysis *pObjDetect = (CVideoAnalysis *)handle;

	if (NULL == handle)
	{
		LOG_ERROR("face handle is null");
		return -1;
	}
	
	return pObjDetect->SetAttr(nPropID, nPropType, pAttr); 
}
