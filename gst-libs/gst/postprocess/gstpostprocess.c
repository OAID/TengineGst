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
 
/**
 * SECTION:element-postprocess
 *
 * FIXME:Describe postprocess here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! postprocess ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "gstpostprocess.h"
#include "postprocess.h"
#include "gst_infer_meta.h"

GST_DEBUG_CATEGORY_STATIC (gst_postprocess_debug);
#define GST_CAT_DEFAULT gst_postprocess_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
};

//static POSTPROCESS_HANDLE gpPostprocessHandle = NULL;

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_postprocess_parent_class parent_class
G_DEFINE_TYPE (Gstpostprocess, gst_postprocess, GST_TYPE_ELEMENT);

static void gst_postprocess_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_postprocess_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_postprocess_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_postprocess_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

static int onRecvPostprocessFrame(FRAME_DATA_S *frame, void *userdata);

/* GObject vmethod implementations */

/* initialize the helmet's class */
static void
gst_postprocess_class_init (GstpostprocessClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_postprocess_set_property;
  gobject_class->get_property = gst_postprocess_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "postprocess",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "econe <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));

  CreatePostprocessService();
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_postprocess_init (Gstpostprocess * filter)
{
  ENTER_FUNCTION();

  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_postprocess_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_postprocess_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = TRUE;
  
  filter->gPostprocessHdl = CreatePostprocess();
  if (NULL == filter->gPostprocessHdl)
  {
	 LOG_ERROR("CreatePostprocess fail");
	 return;
  }
  SetPostprocessCallback(filter->gPostprocessHdl, onRecvPostprocessFrame, filter);
}

static void
gst_postprocess_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstpostprocess *filter = GST_POSTPROCESS (object); 

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_postprocess_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstpostprocess *filter = GST_POSTPROCESS (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_postprocess_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstpostprocess *filter;
  gboolean ret;

  filter = GST_POSTPROCESS (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* 接收到画框之后数据 */
static int onRecvPostprocessFrame(FRAME_DATA_S *frame, void *userdata)
{
	Gstpostprocess *filter = (Gstpostprocess *)userdata;
	GstFlowReturn gRet;
	GstBuffer *buffer;
	GstMapInfo map;

	//printf("[%d] enter %s,frame=%d,pad=0x%x.\n", __LINE__, __func__ , frame->len,filter->srcpad);
	buffer = gst_buffer_new_and_alloc(frame->len);
	gst_buffer_map(buffer, &map, GST_MAP_WRITE);
	memcpy(map.data, frame->data, frame->len);
	map.size = frame->len;
	gst_buffer_unmap(buffer, &map);
	gRet = gst_pad_push(filter->srcpad, buffer);
	if (GST_FLOW_ERROR == gRet)
	{
		printf("gst_pad_push fail\n");
		return -1;
	}
	if (frame->data) free(frame->data);
	if (frame) free(frame);	
	printf("[%d] exit %s\n", __LINE__, __func__);	
	
	return 0;
}
#define SYNC_DRAW
/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_postprocess_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstpostprocess *filter;
  GstMapInfo map;
  InferMeta *infer_meta;
  static int count;

#if 0
  if (count++ % 100 == 0)
  	printf("[%d] enter %s\n", __LINE__, __func__);   
#endif  
  filter = GST_POSTPROCESS (parent);

  if (filter->silent == FALSE)
    g_print ("I'm plugged, therefore I'm in.\n");

  infer_meta = gst_buffer_get_infer_meta(buf);
#ifdef SYNC_DRAW
  if(infer_meta){
    FRAME_DATA_S oframe;  
    buf = gst_buffer_make_writable(buf);
    gst_buffer_map(buf, &map, GST_MAP_WRITE);
    oframe.data = map.data;
    oframe.len = map.size;
    PostprocessSendFrameSync(filter->gPostprocessHdl, &oframe, infer_meta->boxes);
  }
  return gst_pad_push (filter->srcpad, buf);
#else
  FRAME_DATA_S *frame;  
  //if(infer_meta)
  {
    gst_buffer_map(buf, &map, GST_MAP_READ);
    frame = (FRAME_DATA_S *)calloc(1, sizeof(FRAME_DATA_S));
    if (NULL == frame)
    {
    g_print("calloc fail\n");
    return GST_FLOW_ERROR;
    }
    frame->data = (unsigned char *)malloc(map.size);
    if (NULL == frame->data)
    {
      g_print("mallocc fail\n");
      free(frame);
      return GST_FLOW_ERROR;
    }
    memcpy(frame->data, map.data, map.size);
    frame->len = map.size;
    printf("[postprocess]->recv bufSz:%d,inferBox=0x%s,pad=0x%x \n", map.size,infer_meta->boxes,filter->srcpad);
  #if 1
    if(infer_meta)
      PostprocessSendFrame(filter->gPostprocessHdl, frame, infer_meta->boxes);
    else
      PostprocessSendFrame(filter->gPostprocessHdl, frame, NULL);
  #else
    PostprocessSendFrame(filter->gPostprocessHdl, frame, NULL);
  #endif
    gst_buffer_unmap(buf, &map);
    gst_buffer_unref(buf);
  }
  //else{
    //由于处理是异步的，导致有可能两帧冲突，异步 push 竟然崩溃。。。
  //  return gst_pad_push (filter->srcpad, buf);
  //}
#endif
//  printf("[%d] exit %s\n", __LINE__, __func__);    
  /* just push out the incoming buffer without touching it */
  return GST_FLOW_OK;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
postprocess_init (GstPlugin * postprocess)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template postprocess' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_postprocess_debug, "postprocess",
      0, "Template postprocess");

  return gst_element_register (postprocess, "postprocess", GST_RANK_NONE,
      GST_TYPE_POSTPROCESS);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstpostprocess"
#endif

/* gstreamer looks for this structure to register tts
 *
 * exchange the string 'Template postprocess' with your postprocess description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    postprocess,
    "Template postprocess",
    postprocess_init,
    PLUGIN_VERSION, PLUGIN_LICENSE, PACKAGE_NAME, GST_PACKAGE_ORIGIN)

