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
 
#ifndef		__DETECT_INS_H__
#define		__DETECT_INS_H__

#ifdef __cplusplus
extern "C" {
#endif
    /********************************
    * 函数名称：Plugin_Create
    * 功能描述：创建插件对象
    * 输入参数   :  
    * 输出参数   : 对象地址
    * 返回参数   : 
    ********************************/
void *Plugin_Create();

#ifdef __cplusplus
};
#endif

#endif