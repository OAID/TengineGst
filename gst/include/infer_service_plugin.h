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
 
#ifndef		__INFER_SERVPLUGIN_H__
#define		__INFER_SERVPLUGIN_H__

#include "common.h"
#include <queue>
#include <mutex>

using namespace std;

#ifdef	__cplusplus
extern "C" {
#endif	

/* 
将service层的数据回调出去
1、原始的数据帧
2、坐标信息
*/
typedef void (*INFER_SERV_RES_CB)(void *frame, void* boxes, void *userdata);
typedef int (*INFER_SERV_Msg_CB)(int nCmd,void *bufMsg, void *userdata);

enum
{
	enMMST_Start=0,
	enMMST_End
};

/*
1、通道绑定一个模型实例
2、通道具备相应的回调函数,将推理结果回调到上一层
*/
typedef struct _Frame_Mem
{
	FRAME_DATA_S frame;
	int nCopySt;
}TFrameMem;

typedef struct _INFER_CHN
{
	INFER_SERV_RES_CB callback;
	INFER_SERV_Msg_CB cbMsg;
	void *userdata;
	FRAME_DATA_S arrframe[2];//一个备份，一个推理
	unsigned char data[10 * 1024];
	queue<FRAME_DATA_S> mQueueFrames;
	
	unsigned char *pBufInfer;		//推理库需要的拉伸后图片的内存
	int mCurInfer;			//检测的索引
	int mCopySt;			//拷贝状态，拷贝中，拷贝结束，无数据
	int mIsDel;
}TINFER_CHN_HDL;

//动态调用
/*
void *CreateChn(INFER_SERV_RES_CB cbRes,INFER_SERV_Msg_CB cbMsg,void *user);
int DestoryChn(void *handle);
int SendFrame(void *handle, void *frame);
int SetChnAttr(void *handle,int nPropID,int nPropType,void *dtAttr);
int GetChnAttr(void *handle,int nPropID,void *dtAttr);
*/
#ifdef	__cplusplus
}
#endif

/*
还需要增加消息等属性的回调，比如模型动态更新的状态
*/
#ifdef	__cplusplus
class IForwardServ
{
public:
	//这个可以设置整个service 的消息回调，也可以通过某一路进行通信?，由于多路通道共用，采用引用计数器来析构
	virtual int InitService(INFER_SERV_Msg_CB cbMsg,void *user) = 0;
	//删除掉一个user 的引用，如果user 都没有了，service 就可以整体析构，由于回调函数高度依赖 user，所以还是通道管理消息
	virtual int DeinitService(void *user) = 0;
	virtual void *CreateChn(INFER_SERV_RES_CB cbRes,INFER_SERV_Msg_CB cbMsg,void *user) = 0;
	virtual int DestoryChn(void *handle) = 0;
	virtual int SendFrame(void *handle, void *frame) = 0;
	virtual int SetChnAttr(void *handle,int nPropID,int nPropType,void *dtAttr) = 0;
	virtual int GetChnAttr(void *handle,int nPropID,void *dtAttr) = 0;
	virtual int InMsg(void *handle,TPluginMsg *oMsgData) = 0;
};
#endif

#endif