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

#include "gst_infer_meta.h"
#include "loger.h"

GType infer_meta_api_get_type(void)
{
	static volatile GType type;
	static const gchar *tags[] = {"infer", "bar", NULL};

	if (g_once_init_enter(&type))
	{
		GType _type = gst_meta_api_type_register("InferMetaApi", tags);
		g_once_init_leave(&type, _type);
	}
	return type;
}

InferMeta *gst_buffer_get_infer_meta(GstBuffer *buffer)
{
	InferMeta *meta;
	g_return_val_if_fail(GST_IS_BUFFER(buffer), NULL);
	meta = (InferMeta *)gst_buffer_get_meta(buffer, INFER_META_API_TYPE);
	return meta;
}

static gboolean infer_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
	InferMeta *infer_meta = (InferMeta *)meta;
	infer_meta->boxes = NULL;
	infer_meta->alarm = NULL;
	infer_meta->extInfo = NULL;
	return TRUE;
}

static gboolean infer_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer, GQuark type, gpointer data)
{
	InferMeta *infer_meta = (InferMeta *)meta;	
	gst_buffer_add_infer_meta(transbuf, (const gchar *)infer_meta->boxes, (const gchar *)infer_meta->alarm, (const gchar *)infer_meta->extInfo);
	return TRUE;
}

static void infer_meta_free(GstMeta *meta, GstBuffer *buffer)
{
	InferMeta *infer_meta = (InferMeta *)meta;

	if (infer_meta->boxes)
	{
		g_free(infer_meta->boxes);
		infer_meta->boxes = NULL;
	}
	if (infer_meta->alarm)
	{
		g_free(infer_meta->alarm);
		infer_meta->alarm = NULL;
	}
	if (infer_meta->extInfo)
	{
		g_free(infer_meta->extInfo);
		infer_meta->extInfo = NULL;
	}
}

const GstMetaInfo *infer_meta_get_info(void)
{
	static const GstMetaInfo *meta_info = NULL;

	if (g_once_init_enter(&meta_info))
	{
		const GstMetaInfo *mi = gst_meta_register(INFER_META_API_TYPE, 
													 "InferMeta", 
													 sizeof(InferMeta), 
													 infer_meta_init, 
													 infer_meta_free, 
													 infer_meta_transform);
		g_once_init_leave(&meta_info, mi);
	}

	return meta_info;
}

InferMeta *gst_buffer_add_infer_meta(GstBuffer *buffer, const gchar *boxes, const gchar *alarm, const gchar *extendInfo)
{
	InferMeta *meta;

	g_return_val_if_fail(GST_IS_BUFFER(buffer), NULL);

	meta = (InferMeta *)gst_buffer_add_meta(buffer, INFER_META_INFO, NULL);
	if (boxes) meta->boxes = g_strdup(boxes);
	if (alarm) meta->alarm = g_strdup(alarm);
	if (extendInfo) meta->extInfo = g_strdup(extendInfo);

	return meta;
}

void gst_buffer_update_infer_meta(InferMeta *infer_meta, const gchar *boxes, const gchar *alarm, const gchar *extendInfo)
{
	if (NULL == infer_meta)
	{
		LOG_WARN("infer_meta is null");
		return;
	}
	if (boxes)
	{
		if (infer_meta->boxes) g_free(infer_meta->boxes);
		infer_meta->boxes = g_strdup(boxes);
	}
	if (alarm)
	{
		if (infer_meta->alarm) g_free(infer_meta->alarm);
		infer_meta->alarm = g_strdup(alarm);
	}
	if (extendInfo)
	{
		if (infer_meta->extInfo) g_free(infer_meta->extInfo);
		infer_meta->extInfo = g_strdup(extendInfo);
	}
}

