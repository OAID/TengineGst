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
 * SECTION:element-videoanalysis
 *
 * FIXME:Describe videoanalysis here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! videoanalysis ! fakesink silent=TRUE
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

#include "gstvideoanalysis.h"
#include "videoanalysis.h"
#include "gst_infer_meta.h"

GST_DEBUG_CATEGORY_STATIC (gst_videoanalysis_debug);
#define GST_CAT_DEFAULT gst_videoanalysis_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_MODEL_ATTR,
  PROP_BUSINESS_DLL,
  PROP_MODEL_URL
};

/* add by wuwl */
//static OBJDETECT_HANDLE g_pObjDetectHdl = NULL;

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

#define gst_videoanalysis_parent_class parent_class
G_DEFINE_TYPE (Gstvideoanalysis, gst_videoanalysis, GST_TYPE_ELEMENT);

static void gst_videoanalysis_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_videoanalysis_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_videoanalysis_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_videoanalysis_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

static GstStateChangeReturn gst_videoanalysis_change_state (GstElement *element, GstStateChange transition);
/* add by wuwl */
static int onVideoAnalysisRes(FRAME_DATA_S *frame, char *boxes, void *userdata);

/* GObject vmethod implementations */

/* initialize the videoanalysis's class */
static void
gst_videoanalysis_class_init (GstvideoanalysisClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  ENTER_FUNCTION();

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_videoanalysis_set_property;
  gobject_class->get_property = gst_videoanalysis_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  //自定义属性
  g_object_class_install_property (gobject_class, PROP_MODEL_ATTR,
      g_param_spec_string ("modelattr", "model attribute",
      "model download urls", "{}",
      (GParamFlags)(G_PARAM_READWRITE  )));
  g_object_class_install_property (gobject_class, PROP_BUSINESS_DLL,
      g_param_spec_string ("businessdll", "business dll",
      "The message describing current status", "/root/install/lib/libobjdetect_nx.so",
      (GParamFlags)(G_PARAM_READWRITE  )));//G_PARAM_STATIC_STRINGS    
  g_object_class_install_property (gobject_class, PROP_MODEL_URL,
      g_param_spec_string ("modelurl", "business dll",
      "model download urls", "http://oal/down.html",
      (GParamFlags)(G_PARAM_READWRITE  )));

  gst_element_class_set_details_simple(gstelement_class,
    "videoanalysis",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "econe <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
  gstelement_class->change_state = gst_videoanalysis_change_state;	  
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_videoanalysis_init (Gstvideoanalysis * filter)
{
  ENTER_FUNCTION();

  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_videoanalysis_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_videoanalysis_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = TRUE;
  filter->nCurSel = 0;
  filter->tsWork_dll = NULL;
  filter->mInferHdl = NULL;
  memset(filter->arrBoxesMetadata,0,sizeof(char*)*32);
  
  EXIT_FUNCTION();
}

static GstStateChangeReturn
gst_videoanalysis_change_state (GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    Gstvideoanalysis *filter = GST_VIDEOANALYSIS (element);

    switch (transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    default:
        break;
    }

    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE)
        return ret;

    switch (transition)
    {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    break;//要重连就得先调用pause
    case GST_STATE_CHANGE_READY_TO_NULL:
      DestroyVideoAnalysis(filter->mInferHdl);
      filter->mInferHdl = NULL;
      LOG_INFO("====>[gst_videoanalysis_change_state]->2 NULL.\n");
        break;
    default:
        break;
    }
    return ret;
}
static void
gst_videoanalysis_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstvideoanalysis *filter = GST_VIDEOANALYSIS (object); 

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_MODEL_ATTR:{
      const gchar *vTsModelAttr = g_value_get_string(value);
      if(filter->mInferHdl)
        VideoAnalysisSetAttr(filter->mInferHdl, prop_id-PROP_SILENT, 0 ,(void*)vTsModelAttr);
    }
    break;
    case PROP_BUSINESS_DLL:
    {
      //g_value_set_string (value, filter->tsWork_dll);
      const gchar *vTsDll = g_value_get_string(value);
      if(vTsDll&&strlen(vTsDll)>0){
        if(filter->tsWork_dll)
          free(filter->tsWork_dll);
        filter->tsWork_dll = (gchar*)malloc(strlen(vTsDll)+1);
        strcpy(filter->tsWork_dll,vTsDll);
        
        /* add by wuwl */  
        filter->mInferHdl = CreateVideoAnalysis(filter->tsWork_dll);
        if (NULL == filter->mInferHdl)
        {
          LOG_INFO("CreateVideoAnalysis fail");
          return;
        }
        SetVideoAnalysisCallback(filter->mInferHdl, onVideoAnalysisRes, filter);
      }
      break;
    }
    case PROP_MODEL_URL:{
      const gchar *vTsDll = g_value_get_string(value);
      if(filter->tsModel_url)
        free(filter->tsModel_url);
      filter->tsModel_url = (gchar*)malloc(strlen(vTsDll)+1);
      strcpy(filter->tsModel_url,vTsDll);
      if(filter->mInferHdl)
        VideoAnalysisSetAttr(filter->mInferHdl, prop_id-PROP_SILENT, 0 ,(void*)filter->tsModel_url);

      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_videoanalysis_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstvideoanalysis *filter = GST_VIDEOANALYSIS (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_BUSINESS_DLL:
    case PROP_MODEL_ATTR:
    case PROP_MODEL_URL:{
      //g_value_set_int (value, 10);
      g_value_set_string (value, "");
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

static void  read_video_props (GstPad * pad,GstCaps *caps,gint *width, gint *height)
{
	gboolean ret = FALSE;
	const GValue *fps, *par;
	//HRESULT hres;
	GstStructure *s = gst_caps_get_structure (caps, 0);
	Gstvideoanalysis *vdec = (Gstvideoanalysis *) gst_pad_get_parent (pad);
	GstvideoanalysisClass *klass =
	    (GstvideoanalysisClass *) G_OBJECT_GET_CLASS (vdec);

	/* read data */
	if (!gst_structure_get_int (s, "width", &vdec->width) ||
	        !gst_structure_get_int (s, "height", &vdec->height))
	{
		GST_ELEMENT_ERROR (vdec, CORE, NEGOTIATION,
		                   ("error getting video width or height from caps"), (NULL));
		printf("X-X[wuwl]->error getting video width or height from caps.\r\n");
		return ;
	}
	fps = gst_structure_get_value (s, "framerate");
	if (fps)
	{
		vdec->fps_n = gst_value_get_fraction_numerator (fps);
		vdec->fps_d = gst_value_get_fraction_denominator (fps);
	}
	else
	{
		/* Invent a sane default framerate; the timestamps matter
		 * more anyway. */
		vdec->fps_n = 25;
		vdec->fps_d = 1;
	}
	*width = vdec->width;
	*height = vdec->height;
	vdec->nDetectIdx = vdec->detectInterval;
	g_print ("\n****[wuwl]->video capabilities is %dx%d,fps=%d:%d,init alog.\n",  
		vdec->width, vdec->height,vdec->fps_n,vdec->fps_d);
}
/* this function handles sink events */
static gboolean
gst_videoanalysis_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstvideoanalysis *filter;
  gboolean ret;

  filter = GST_VIDEOANALYSIS (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

		  gint width = -1, height = -1;
      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

		  read_video_props(pad,caps,&width,&height);
      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    case GST_EVENT_CUSTOM_BOTH:
    {
      const GstStructure *msgST = gst_event_get_structure(event);
      gint nCmd = 0;
      const gchar * strMsg;
      gst_structure_get_int(msgST,"cmd",&nCmd);
      strMsg = gst_structure_get_string(msgST,"msg");
      g_print ("\n=====****[wuwl]->gst_videoanalysis_sink_event cmd=%d,msg=%s.\n",nCmd,strMsg);
      //解析命令字，然后设置模型，哈哈，不行，这样又麻烦，还是整体打包放进去，头大
		  TPluginMsg msgIn;
      msgIn.nCmd = nCmd;
      msgIn.nSeq = 0;
      msgIn.pMsgData = (char*)strMsg;
      VideoAnalysisSendMsg(filter->mInferHdl, &msgIn);
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* 接收到推理数据 ,metadata 要保存到队列里面，不然直接推下一帧，不晓得有问题没
检测不到，也要把结果空传过来，这样可以清空原先结果，就不会出现闪框现象*/
static int onVideoAnalysisRes(FRAME_DATA_S *frame, char *inferRes, void *userdata)
{
	Gstvideoanalysis *filter = (Gstvideoanalysis *)userdata;
	GstFlowReturn gRet;
	GstBuffer *buffer;
	GstMapInfo map;
	InferMeta *infer_meta;	

  //来的多少都有问题，要管理起来
  if(filter->arrBoxesMetadata[frame->nChnID]){
    g_free(filter->arrBoxesMetadata[frame->nChnID]);
    filter->arrBoxesMetadata[frame->nChnID] = NULL;
  }
  filter->arrBoxesMetadata[frame->nChnID] = g_strdup(inferRes);
  //LOG_INFO("[send] object=%d boxes: %s\n", frame->nChnID,boxes);
  //filter->boxesMeta[filter->nCurSel] = g_strdup(boxes);
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
/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_videoanalysis_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstvideoanalysis *filter;
  GstMapInfo map;    
  FRAME_DATA_S frame = {0};  

//  printf("[%d] enter %s\n", __LINE__, __func__);
  filter = GST_VIDEOANALYSIS (parent);
  buf = gst_buffer_make_writable(buf);

  //TIC();
  char *tsChnID = NULL;
  //char info[24] = {0};
  //if (filter->silent == FALSE)
  //  g_print ("I'm plugged, therefore I'm in.\n");
  if(filter->mInferHdl){
    /* add by wuwl */
    //读取meta，看是哪一路的视频
    InferMeta *infer_meta;
    infer_meta = gst_buffer_get_infer_meta(buf);
    gst_buffer_map(buf, &map, GST_MAP_WRITE);
    frame.data = map.data;
    frame.len = map.size;
    frame.width = filter->width;
    frame.height = filter->height;
    if(infer_meta && infer_meta->extInfo){
      //printf("====>[gst_videoanalysis_chain]->chnID=%s,len=%d,w:h=%d:%d.<====\n",infer_meta->extInfo,map.size,frame.width,frame.height);
      tsChnID = infer_meta->extInfo;
      frame.nChnID = atoi(tsChnID);
    }
    VideoAnalysisSendFrame(filter->mInferHdl, &frame);
    //printf("===>[gst_videoanalysis_chain]->data sz=%d,w:h=%d:%d.\n",
    //  frame.len,frame.width,frame.height);

    gst_buffer_unmap(buf, &map);
    //含有推理结果，就加进去
    if(filter->arrBoxesMetadata[frame.nChnID]){
      gst_buffer_add_infer_meta(buf, filter->arrBoxesMetadata[frame.nChnID], NULL, tsChnID);
      //避免闪烁
      //g_free(filter->tmpBoxesMetadata);
      //filter->tmpBoxesMetadata = NULL;
    }
  }
  else{
    //终止向下发送数据
    gst_buffer_unref(buf);
    return GST_FLOW_OK;
  }
  //sprintf(info,"[gst_videoanalysis_chain]->cp %s ",tsChnID?tsChnID:"0");
  //  TOC(info);
  //gst_buffer_unref(buf);
  return gst_pad_push (filter->srcpad, buf);
  /* just push out the incoming buffer without touching it */
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
videoanalysis_init (GstPlugin * videoanalysis)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template videoanalysis' with your description
   */
  printf("enter [%d_%s]\n", __LINE__, __func__);  
  GST_DEBUG_CATEGORY_INIT (gst_videoanalysis_debug, "videoanalysis",
      0, "Template videoanalysis");

  return gst_element_register (videoanalysis, "videoanalysis", GST_RANK_NONE,
      GST_TYPE_VIDEOANALYSIS);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstvideoanalysis"
#endif

/* gstreamer looks for this structure to register tts
 *
 * exchange the string 'Template videoanalysis' with your videoanalysis description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    videoanalysis,
    "Template videoanalysis",
    videoanalysis_init,
    PLUGIN_VERSION, PLUGIN_LICENSE, PACKAGE_NAME, GST_PACKAGE_ORIGIN)