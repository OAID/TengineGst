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
 
#ifndef		__INFERINS_H__
#define		__INFERINS_H__

#include "baseinfer.h"
class CInferIns:public IForwardServ
{
public:
	CInferIns(){};
	~CInferIns(){};
	virtual int InitService(INFER_SERV_Msg_CB cbMsg,void *user);
	virtual int DeinitService(void *user);
	virtual void *CreateChn(INFER_SERV_RES_CB cbRes,INFER_SERV_Msg_CB cbMsg,void *user);
	virtual int DestoryChn(void *handle) ;
	virtual int SendFrame(void *handle, void *frame) ;
	virtual int SetChnAttr(void *handle,int nPropID,int nPropType,void *dtAttr) ;
	virtual int GetChnAttr(void *handle,int nPropID,void *dtAttr) ;
	virtual int InMsg(void *handle,TPluginMsg *data);
};

#endif