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
 * SECTION:element-streammux
 *
 * FIXME:Describe streammux here.
 * streammux
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! streammux ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
//#include <gst/video/video.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "gststreammux.h"
#include "gst_infer_meta.h"
#include "common.h"
//#include "normal.h"

GST_DEBUG_CATEGORY_STATIC (gst_streammux_debug);
#define GST_CAT_DEFAULT gst_streammux_debug


GType gst_streammux_pad_get_type (void);
#define GST_TYPE_STREAMMUX_PAD \
  (gst_streammux_pad_get_type())
#define GST_STREAMMUX_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_STREAMMUX_PAD, GstStreamMuxPad))
#define GST_STREAMMUX_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_STREAMMUX_PAD, GstStreamMuxPadClass))
#define GST_IS_STREAMMUX_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_STREAMMUX_PAD))
#define GST_IS_STREAMMUX_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_STREAMMUX_PAD))
#define GST_STREAMMUX_PAD_CAST(obj) \
  ((GstStreamMuxPad *)(obj))

typedef struct _GstStreamMuxPad GstStreamMuxPad;
typedef struct _GstStreamMuxPadClass GstStreamMuxPadClass;

struct _GstStreamMuxPad
{
  GstPad parent;

  gboolean got_eos;
};

struct _GstStreamMuxPadClass
{
  GstPadClass parent;
};

G_DEFINE_TYPE (GstStreamMuxPad, gst_streammux_pad, GST_TYPE_PAD);

#define DEFAULT_FORWARD_STICKY_EVENTS	FALSE

enum
{
  PROP_0,
  PROP_FORWARD_STICKY_EVENTS,
  PROP_SILENT
};

static void
gst_streammux_pad_class_init (GstStreamMuxPadClass * klass)
{
}

static void
gst_streammux_pad_init (GstStreamMuxPad * pad)
{
  pad->got_eos = FALSE;
}

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};
/*
enum
{
  PROP_0,
  PROP_SILENT,
};*/

/* add by wuwl */
//static OBJDETECT_HANDLE g_pObjDetectHdl = NULL;

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
#define FORMATS " { AYUV, BGRA, ARGB, RGBA, ABGR, Y444, Y42B, YUY2, UYVY, "\
                "   YVYU, I420, YV12, NV12, NV21, Y41B, RGB, BGR, xRGB, xBGR, "\
                "   RGBx, BGRx } "

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    //GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (FORMATS))
    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY
    //GST_STATIC_CAPS(
    //"video/x-raw, "
    //"format={ (string)NV12, (string)RGBA }, "
    //"width = (int) [128,4096], "
    //"height = (int) [128,2160], "
    //"framerate = (fraction) [ 5, 60 ] ;"
    //)
    //GST_STATIC_CAPS ("ANY")
    //GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (FORMATS))
    );

//#define gst_streammux_parent_class parent_class
//G_DEFINE_TYPE (GstStreamMux, gst_streammux, GST_TYPE_ELEMENT);

#define _do_init \
  GST_DEBUG_CATEGORY_INIT (gst_streammux_debug, "streammux", 0, "streammux element");
#define gst_streammux_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstStreamMux, gst_streammux, GST_TYPE_ELEMENT, _do_init);

static void gst_streammux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_streammux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_streammux_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);

static GstStateChangeReturn gst_streammux_change_state (GstElement *element, GstStateChange transition);

static void gst_streammux_dispose (GObject * object);
/* GObject vmethod implementations */
static GstPad * gst_streammux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps);
static void gst_streammux_release_pad (GstElement * element, GstPad * pad);
static GstFlowReturn
gst_streammux_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer);
static GstFlowReturn
gst_streammux_sink_chain_list (GstPad * pad, GstObject * parent,
    GstBufferList * list);
static gboolean gst_streammux_sink_query (GstPad * pad, GstObject * parent, GstQuery * query);

static int gFrameCount = 0;
static bool s_bQuit = false;
static pthread_t s_FrameCntId = 0;
void *frame_count_process(void *parg)
{
	while (s_bQuit)
	{
		sleep(5);
		g_printf("streammux fps: %d", gFrameCount/5);
		gFrameCount = 0; 
	}
  s_FrameCntId = 0;
  return NULL;
}

/* initialize the streammux's class */
static void
gst_streammux_class_init (GstStreamMuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  ENTER_FUNCTION();

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_streammux_set_property;
  gobject_class->get_property = gst_streammux_get_property;
  gobject_class->dispose = gst_streammux_dispose;


  g_object_class_install_property (gobject_class, PROP_FORWARD_STICKY_EVENTS,
      g_param_spec_boolean ("forward-sticky-events", "Forward sticky events",
          "Forward sticky events on stream changes",
          DEFAULT_FORWARD_STICKY_EVENTS,
          (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_READY)));
  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "streammux",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "econe <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
  //gstelement_class->change_state = gst_streammux_change_state;	 
  
  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_streammux_request_new_pad);
  gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_streammux_release_pad);
  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_streammux_change_state); 
  if(s_FrameCntId==0){
    s_bQuit = false;
    pthread_create(&s_FrameCntId, NULL, frame_count_process, NULL);
  }
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_streammux_init (GstStreamMux * filter)
{
  ENTER_FUNCTION();
  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = TRUE;
  filter->nCurSel = 0;
  filter->forward_sticky_events = DEFAULT_FORWARD_STICKY_EVENTS;
  
  EXIT_FUNCTION();
}

static void gst_streammux_dispose (GObject * object)
{
  GstStreamMux *filter = GST_STREAMMUX (object);
  GList *item;

  //g_clear_object (&filter->last_sinkpad);
  gst_object_replace ((GstObject **) & filter->last_sinkpad, NULL);

restart:
  for (item = GST_ELEMENT_PADS (object); item; item = g_list_next (item)) {
    GstPad *pad = GST_PAD (item->data);
    if (GST_PAD_IS_SINK (pad)) {
      gst_element_release_request_pad (GST_ELEMENT (object), pad);
      goto restart;
    }
  }

  G_OBJECT_CLASS (gst_streammux_parent_class)->dispose (object);
}
static void
gst_streammux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstStreamMux *filter = GST_STREAMMUX (object); 

  switch (prop_id) {
    case PROP_FORWARD_STICKY_EVENTS:
      filter->forward_sticky_events = g_value_get_boolean (value);
      break;
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_streammux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstStreamMux *filter = GST_STREAMMUX (object);

  switch (prop_id) {
    case PROP_FORWARD_STICKY_EVENTS:
      g_value_set_boolean (value, filter->forward_sticky_events);
      break;
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstPad *gst_streammux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{
  GstPad *sinkpad;
  GST_DEBUG_OBJECT (element, "requesting pad");
  gint padcount;
  GstStreamMux *streammux = GST_STREAMMUX (element);
  
  padcount = g_atomic_int_add (&streammux->padcount, 1);
  sinkpad = GST_PAD_CAST (g_object_new (GST_TYPE_STREAMMUX_PAD,
          "name", req_name, "direction", templ->direction, "template", templ,
          NULL));
  //sinkpad = gst_pad_new_from_template (templ, req_name);
  g_printf("[gst_streammux_request_new_pad]->reqName=%s,pad=%d:0x%x", req_name, padcount, sinkpad);
  
  GstSMuxPadPrivate *padpriv = g_slice_new0 (GstSMuxPadPrivate);
  //获取是哪一路
  padpriv->nChnID = padcount;
  gst_pad_set_element_private (sinkpad, padpriv);

  gst_pad_set_chain_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_streammux_sink_chain));
  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_streammux_sink_chain_list));
  gst_pad_set_event_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_streammux_sink_event));

  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_CAPS);
  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_ALLOCATION);

  gst_pad_set_active (sinkpad, TRUE);
  gst_element_add_pad (element, sinkpad);

  GST_DEBUG_OBJECT (element, "requested pad %s:%s",
      GST_DEBUG_PAD_NAME (sinkpad));

  return sinkpad;
}

static gboolean
gst_streammux_all_sinkpads_eos_unlocked (GstStreamMux * streammux, GstPad * pad)
{
  GstElement *element = GST_ELEMENT_CAST (streammux);
  GList *item;
  gboolean all_eos = FALSE;

  if (element->numsinkpads == 0)
    goto done;

  for (item = element->sinkpads; item != NULL; item = g_list_next (item)) {
    GstStreamMuxPad *sinkpad = GST_STREAMMUX_PAD_CAST(item->data);

    if (!sinkpad->got_eos) {
      return FALSE;
    }
  }

  all_eos = TRUE;

done:
  return all_eos;
}
static void
gst_streammux_release_pad (GstElement * element, GstPad * pad)
{  
  GstStreamMux *streammux = GST_STREAMMUX (element);
  GstStreamMuxPad *fpad = GST_STREAMMUX_PAD_CAST (pad);
  gboolean got_eos;
  gboolean send_eos = FALSE;

  GST_DEBUG_OBJECT (streammux, "releasing pad %s:%s", GST_DEBUG_PAD_NAME (pad));
  gst_pad_set_active (pad, FALSE);
  got_eos = fpad->got_eos;
  gst_element_remove_pad (GST_ELEMENT_CAST (streammux), pad);

  GST_OBJECT_LOCK (streammux);
  if (!got_eos && gst_streammux_all_sinkpads_eos_unlocked (streammux, NULL)) {
    GST_DEBUG_OBJECT (streammux, "Pad removed. All others are EOS. Sending EOS");
    send_eos = TRUE;
  }
  GST_OBJECT_UNLOCK (streammux);

  if (send_eos)
    if (!gst_pad_push_event (streammux->srcpad, gst_event_new_eos ()))
      GST_WARNING_OBJECT (streammux, "Failure pushing EOS");
}

static gboolean
forward_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *srcpad = (GstPad *)user_data;

  if (GST_EVENT_TYPE (*event) != GST_EVENT_EOS)
    gst_pad_push_event (srcpad, gst_event_ref (*event));

  return TRUE;
}

static GstFlowReturn
gst_streammux_sink_chain_object (GstPad * pad, GstStreamMux * streammux,
    gboolean is_list, GstMiniObject * obj)
{
  GstFlowReturn res;
  char channel[4] = "0";

  GST_DEBUG_OBJECT (pad, "######[gst_streammux_sink_chain_object]->received %" GST_PTR_FORMAT, obj);

  GST_PAD_STREAM_LOCK (streammux->srcpad);
  if ((streammux->last_sinkpad == NULL) || (streammux->forward_sticky_events
          && (streammux->last_sinkpad != pad))) {
    gst_object_replace ((GstObject **) & streammux->last_sinkpad,
        GST_OBJECT (pad));

    GST_DEBUG_OBJECT (pad, "Forwarding sticky events");
    gst_pad_sticky_events_foreach (pad, forward_events, streammux->srcpad);
  }
  GstSMuxPadPrivate *padpriv;
  padpriv = (GstSMuxPadPrivate *)gst_pad_get_element_private (pad);
  if (padpriv){  
    //添加 meta 信息
    sprintf(channel, "%d", padpriv->nChnID);
  }
  if (is_list){
    res = gst_pad_push_list (streammux->srcpad, GST_BUFFER_LIST_CAST (obj));
  }
  else{
    GstBuffer * buf = GST_BUFFER_CAST (obj);
    buf = gst_buffer_make_writable(buf);
    gst_buffer_add_infer_meta(buf, NULL, NULL, channel);
    res = gst_pad_push(streammux->srcpad, buf);
  }

  GST_PAD_STREAM_UNLOCK (streammux->srcpad);
  GST_LOG_OBJECT (pad, "handled buffer%s %s", (is_list ? "list" : ""),
      gst_flow_get_name (res));

  return res;
}

static GstFlowReturn
gst_streammux_sink_chain_list (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstStreamMux *streammux = GST_STREAMMUX_CAST (parent);

  return gst_streammux_sink_chain_object (pad, streammux, TRUE,
      GST_MINI_OBJECT_CAST (list));
}

static GstFlowReturn
gst_streammux_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstStreamMux *streammux = GST_STREAMMUX_CAST (parent);

  gFrameCount++;

#if 1  
  return gst_streammux_sink_chain_object (pad, streammux, FALSE,
      GST_MINI_OBJECT_CAST (buffer));
#else
  GstFlowReturn gReturn;
  long long mux_pre, mux_next, diff; 

  mux_pre = get_timestamp_ext(); 
  gReturn = gst_streammux_sink_chain_object (pad, streammux, FALSE, GST_MINI_OBJECT_CAST (buffer));
  mux_next = get_timestamp_ext(); 
  diff = mux_next - mux_pre;
//  if (diff > 30)
//	g_printf("chain process diff: %lld_%lld_%lld", mux_next, mux_pre, diff);
  
  return gReturn;
#endif
}

static gboolean
gst_streammux_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstStreamMux *streammux = GST_STREAMMUX_CAST (parent);
  GstStreamMuxPad *fpad = GST_STREAMMUX_PAD_CAST (pad);
  gboolean forward = TRUE;
  gboolean res = TRUE;
  gboolean unlock = FALSE;

  GST_DEBUG_OBJECT (pad, "received event %" GST_PTR_FORMAT, event);

  if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS){
	GstCaps * caps;
	int width, height;
	  
	gst_event_parse_caps (event, &caps);
	/* do something with the caps */
	GstStructure* structure = gst_caps_get_structure(caps, 0);
	gst_structure_get_int(structure, "width", &width);
	gst_structure_get_int(structure, "height", &height);
	g_printf("recv resolution %d x %d", width, height);
  }

  if (GST_EVENT_IS_STICKY (event)) {
    unlock = TRUE;
    GST_PAD_STREAM_LOCK (streammux->srcpad);

    if (GST_EVENT_TYPE (event) == GST_EVENT_EOS) {
      GST_OBJECT_LOCK (streammux);
      fpad->got_eos = TRUE;
      if (!gst_streammux_all_sinkpads_eos_unlocked (streammux, pad)) {
        forward = FALSE;
      } else {
        forward = TRUE;
      }
      GST_OBJECT_UNLOCK (streammux);
    } else if (pad != streammux->last_sinkpad) {
      forward = FALSE;
    }
  } else if (GST_EVENT_TYPE (event) == GST_EVENT_FLUSH_STOP) {
    unlock = TRUE;
    GST_PAD_STREAM_LOCK (streammux->srcpad);
    GST_OBJECT_LOCK (streammux);
    fpad->got_eos = FALSE;
    GST_OBJECT_UNLOCK (streammux);
  }

  if (forward && GST_EVENT_IS_SERIALIZED (event)) {
    /* If no data is coming and we receive serialized event, need to forward all sticky events.
     * Otherwise downstream has an inconsistent set of sticky events when
     * handling the new event. */
    if (!unlock) {
      unlock = TRUE;
      GST_PAD_STREAM_LOCK (streammux->srcpad);
    }

    if ((streammux->last_sinkpad == NULL) || (streammux->last_sinkpad != pad)) {
      gst_object_replace ((GstObject **) & streammux->last_sinkpad,
          GST_OBJECT (pad));
      gst_pad_sticky_events_foreach (pad, forward_events, streammux->srcpad);
    }
  }

  if (forward)
    res = gst_pad_push_event (streammux->srcpad, event);
  else
    gst_event_unref (event);

  if (unlock)
    GST_PAD_STREAM_UNLOCK (streammux->srcpad);

  return res;
}

static void
reset_pad (const GValue * data, gpointer user_data)
{
  GstPad *pad = (GstPad *)g_value_get_object (data);
  GstStreamMuxPad *fpad = GST_STREAMMUX_PAD_CAST (pad);
  GstStreamMux *streammux = (GstStreamMux *)user_data;

  GST_OBJECT_LOCK (streammux);
  fpad->got_eos = FALSE;
  GST_OBJECT_UNLOCK (streammux);
}
static GstStateChangeReturn
gst_streammux_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
      GstIterator *iter = gst_element_iterate_sink_pads (element);
      GstIteratorResult res;

      do {
        res = gst_iterator_foreach (iter, reset_pad, element);
        if (res == GST_ITERATOR_RESYNC)
          gst_iterator_resync (iter);
      } while (res == GST_ITERATOR_RESYNC);
      gst_iterator_free (iter);

      if (res == GST_ITERATOR_ERROR)
        return GST_STATE_CHANGE_FAILURE;

    }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      s_bQuit = true;
      break;
    default:
      break;
  }

  return ret;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
streammux_init (GstPlugin * streammux)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template streammux' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_streammux_debug, "streammux",
      0, "Template streammux");

  return gst_element_register (streammux, "streammux", GST_RANK_NONE,
      GST_TYPE_STREAMMUX);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirststreammux"
#endif

/* gstreamer looks for this structure to register tts
 *
 * exchange the string 'Template streammux' with your streammux description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    streammux,
    "Template streammux",
    streammux_init,
    PLUGIN_VERSION, PLUGIN_LICENSE, PACKAGE_NAME, GST_PACKAGE_ORIGIN)