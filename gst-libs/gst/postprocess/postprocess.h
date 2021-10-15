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
 
#ifndef		__POSTPROCESS_H__
#define		__POSTPROCESS_H__

#include "common.h"
typedef	void* POSTPROCESS_HANDLE;
typedef int (*POSTPROCESS_RES_CB)(FRAME_DATA_S *frame, void *userdata);

#ifdef	__cplusplus
extern "C" {
#endif	
int CreatePostprocessService(void);
POSTPROCESS_HANDLE CreatePostprocess(void);
void DestroyPostprocess(POSTPROCESS_HANDLE handle);
void SetPostprocessCallback(POSTPROCESS_HANDLE handle, POSTPROCESS_RES_CB onRecvData, void *userdata);
int PostprocessSendFrame(POSTPROCESS_HANDLE handle, FRAME_DATA_S *frame, char *json);
int PostprocessSendFrameSync(POSTPROCESS_HANDLE handle, FRAME_DATA_S *frame, char *json);
#ifdef	__cplusplus
}
#endif	

#endif

