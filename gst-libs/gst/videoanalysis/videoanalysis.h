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
 
#ifndef		__VIDEO_ANALYSIS_H__
#define		__VIDEO_ANALYSIS_H__

#include "common.h"

typedef	void* OBJDETECT_HANDLE;
/* {"boxes":[{x1,y1,width1,height1},{x2,y2,width2,height2},{x3,y3,width3,height3}]} */
typedef int (*OBJDETECT_RESULT_CB)(FRAME_DATA_S *frame, char *boxes, void *userdata);
#ifdef	__cplusplus
extern "C" {
#endif	

OBJDETECT_HANDLE CreateVideoAnalysis(char *config);
void DestroyVideoAnalysis(OBJDETECT_HANDLE handle);
void SetVideoAnalysisCallback(OBJDETECT_HANDLE handle, OBJDETECT_RESULT_CB onRecvData, void *userdata);
int VideoAnalysisSendFrame(OBJDETECT_HANDLE handle, FRAME_DATA_S *frame);
int VideoAnalysisSendMsg(OBJDETECT_HANDLE handle, TPluginMsg *oMsgData);
int VideoAnalysisSetAttr(OBJDETECT_HANDLE handle,int nPropID,int nPropType,void *pAttr);

#ifdef	__cplusplus
}
#endif	


#endif

