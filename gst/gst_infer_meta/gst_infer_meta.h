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
 
#ifndef		__GST_INFER_META_H__
#define		__GST_INFER_META_H__

#include <gst/gst.h>

#ifdef	__cplusplus
extern "C" {
#endif	

typedef struct InferMeta_T
{
	GstMeta meta;
	/* user data */
	gchar *boxes;
	gchar *alarm;
	gchar *extInfo;	//扩展信息，目前保存通道id
}InferMeta;

GType infer_meta_api_get_type(void);
#define	INFER_META_API_TYPE (infer_meta_api_get_type())
InferMeta *gst_buffer_get_infer_meta(GstBuffer *buffer);

const GstMetaInfo *infer_meta_get_info(void);
#define	INFER_META_INFO	(infer_meta_get_info())
InferMeta *gst_buffer_add_infer_meta(GstBuffer *buffer, const gchar *boxes, const gchar *alarm, const gchar *extendInfo);
void gst_buffer_update_infer_meta(InferMeta *infer_meta, const gchar *boxes, const gchar *alarm, const gchar *extendInfo);

#ifdef	__cplusplus
};
#endif	

#endif

