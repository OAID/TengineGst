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
 * SECTION:element-streamdemux
 *
 * FIXME:Describe streamdemux here.
 * streamdemux
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! streamdemux ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/video/video.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "gststreamdemux.h"
#include "gst_infer_meta.h"
#include "common.h"

GST_DEBUG_CATEGORY_STATIC (gst_streamdemux_debug);
#define GST_CAT_DEFAULT gst_streamdemux_debug

GType gst_streamdemux_pad_get_type (void);
#define GST_TYPE_STREAMDEMUX_PAD \
  (gst_streamdemux_pad_get_type())
#define GST_STREAMDEMUX_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_STREAMDEMUX_PAD, GstStreamDemuxPad))
#define GST_STREAMDEMUX_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_STREAMDEMUX_PAD, GstStreamDemuxPadClass))
#define GST_IS_STREAMDEMUX_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_STREAMDEMUX_PAD))
#define GST_IS_STREAMDEMUX_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_STREAMDEMUX_PAD))
#define GST_STREAMDEMUX_PAD_CAST(obj) \
  ((GstStreamDemuxPad *)(obj))

typedef struct _GstStreamDemuxPad GstStreamDemuxPad;
typedef struct _GstStreamDemuxPadClass GstStreamDemuxPadClass;

struct _GstStreamDemuxPad
{
  GstPad parent;

  guint index;
  gboolean pushed;
  GstFlowReturn result;
  gboolean removed;
  gboolean got_eos;
};

struct _GstStreamDemuxPadClass
{
  GstPadClass parent;
};

G_DEFINE_TYPE (GstStreamDemuxPad, gst_streamdemux_pad, GST_TYPE_PAD);

#define DEFAULT_FORWARD_STICKY_EVENTS	TRUE

static void
gst_streamdemux_pad_class_init (GstStreamDemuxPadClass * klass)
{
}

static void
gst_streamdemux_pad_reset (GstStreamDemuxPad * pad)
{
  pad->pushed = FALSE;
  pad->result = GST_FLOW_NOT_LINKED;
  pad->removed = FALSE;
  pad->got_eos = FALSE;
}
static void
gst_streamdemux_pad_init (GstStreamDemuxPad * pad)
{
  gst_streamdemux_pad_reset (pad);
}

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_NUM_SRC_PADS,
  PROP_HAS_CHAIN,
  PROP_SILENT,
  PROP_LAST_MESSAGE,
  PROP_PULL_MODE,
  PROP_ALLOC_PAD,
  PROP_ALLOW_NOT_LINKED,
};

#define DEFAULT_PROP_NUM_SRC_PADS	0
#define DEFAULT_PROP_HAS_CHAIN		TRUE
#define DEFAULT_PROP_SILENT		TRUE
#define DEFAULT_PROP_LAST_MESSAGE	NULL
#define DEFAULT_PULL_MODE		GST_STREAMDEMUX_PULL_MODE_NEVER
#define DEFAULT_PROP_ALLOW_NOT_LINKED	FALSE

#define GST_TYPE_STREAMDEMUX_PULL_MODE (gst_streamdemux_pull_mode_get_type())
static GType
gst_streamdemux_pull_mode_get_type (void)
{
  static GType type = 0;
  static const GEnumValue data[] = {
    {GST_STREAMDEMUX_PULL_MODE_NEVER, "Never activate in pull mode", "never"},
    {GST_STREAMDEMUX_PULL_MODE_SINGLE, "Only one src pad can be active in pull mode",
        "single"},
    {0, NULL, NULL},
  };

  if (!type) {
    type = g_enum_register_static ("GstStreamDemuxPullMode", data);
  }
  return type;
}
static GParamSpec *pspec_last_message = NULL;
static GParamSpec *pspec_alloc_pad = NULL;
/* add by wuwl */
/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
#define FORMATS " { AYUV, BGRA, ARGB, RGBA, ABGR, Y444, Y42B, YUY2, UYVY, "\
                "   YVYU, I420, YV12, NV12, NV21, Y41B, RGB, BGR, xRGB, xBGR, "\
                "   RGBx, BGRx } "

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("ANY")
    //GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (FORMATS))
    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    //GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (FORMATS))
    );

#define gst_streamdemux_parent_class parent_class
G_DEFINE_TYPE (GstStreamDemux, gst_streamdemux, GST_TYPE_ELEMENT);

static void gst_streamdemux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_streamdemux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_streamdemux_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);

static GstStateChangeReturn gst_streamdemux_change_state (GstElement *element, GstStateChange transition);

static void gst_streamdemux_finalize (GObject * object);
static void gst_streamdemux_dispose (GObject * object);
/* GObject vmethod implementations */
static GstPad * gst_streamdemux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps);
static void gst_streamdemux_release_pad (GstElement * element, GstPad * pad);

static gboolean gst_streamdemux_sink_query (GstPad * pad, GstObject * parent, GstQuery * query);

static GstFlowReturn
gst_streamdemux_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer);

static GstFlowReturn
gst_streamdemux_chain_list (GstPad * pad, GstObject * parent, GstBufferList * list);

static gboolean gst_streamdemux_sink_activate_mode (GstPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active);
static gboolean gst_streamdemux_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query);
static gboolean gst_streamdemux_src_activate_mode (GstPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active);
static GstFlowReturn gst_streamdemux_src_get_range (GstPad * pad, GstObject * parent,
    guint64 offset, guint length, GstBuffer ** buf);

/* initialize the streamdemux's class */
static void
gst_streamdemux_class_init (GstStreamDemuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  ENTER_FUNCTION();

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_streamdemux_set_property;
  gobject_class->get_property = gst_streamdemux_get_property;
  gobject_class->dispose = gst_streamdemux_dispose;
  gobject_class->finalize = gst_streamdemux_finalize;

  pspec_alloc_pad = g_param_spec_object ("alloc-pad", "Allocation Src Pad",
      "The pad ALLOCATION queries will be proxied to (DEPRECATED, has no effect)",
      GST_TYPE_PAD,
      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));
  g_object_class_install_property (gobject_class, PROP_ALLOC_PAD,
      pspec_alloc_pad);

  g_object_class_install_property (gobject_class, PROP_NUM_SRC_PADS,
      g_param_spec_int ("num-src-pads", "Num Src Pads",
          "The number of source pads", 0, G_MAXINT, DEFAULT_PROP_NUM_SRC_PADS,
          (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_HAS_CHAIN,
      g_param_spec_boolean ("has-chain", "Has Chain",
          "If the element can operate in push mode", DEFAULT_PROP_HAS_CHAIN,
          (GParamFlags)(G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent",
          "Don't produce last_message events", DEFAULT_PROP_SILENT,
          (GParamFlags)(G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  pspec_last_message = g_param_spec_string ("last-message", "Last Message",
      "The message describing current status", DEFAULT_PROP_LAST_MESSAGE,
      (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_LAST_MESSAGE,
      pspec_last_message);
  g_object_class_install_property (gobject_class, PROP_PULL_MODE,
      g_param_spec_enum ("pull-mode", "Pull mode",
          "Behavior of tee in pull mode", GST_TYPE_STREAMDEMUX_PULL_MODE,
          DEFAULT_PULL_MODE,
          (GParamFlags)(G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_details_simple(gstelement_class,
    "streamdemux",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "econe <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
      
  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_streamdemux_request_new_pad);
  gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_streamdemux_release_pad);
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_streamdemux_init (GstStreamDemux * streamdemux)
{
  ENTER_FUNCTION();
  streamdemux->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  streamdemux->sink_mode = GST_PAD_MODE_NONE;

  gst_pad_set_event_function (streamdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_streamdemux_sink_event));
  gst_pad_set_query_function (streamdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_streamdemux_sink_query));
  gst_pad_set_activatemode_function (streamdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_streamdemux_sink_activate_mode));
  gst_pad_set_chain_function (streamdemux->sinkpad, GST_DEBUG_FUNCPTR (gst_streamdemux_chain));
  gst_pad_set_chain_list_function (streamdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_streamdemux_chain_list));

  GST_OBJECT_FLAG_SET (streamdemux->sinkpad, GST_PAD_FLAG_PROXY_ALLOCATION);
  GST_OBJECT_FLAG_SET (streamdemux->sinkpad, GST_PAD_FLAG_PROXY_CAPS);
  gst_element_add_pad (GST_ELEMENT (streamdemux), streamdemux->sinkpad);

  streamdemux->pad_indexes = g_hash_table_new (NULL, NULL);
  streamdemux->last_message = NULL;

  streamdemux->silent = TRUE;
  streamdemux->nCurSel = 0;
  
  EXIT_FUNCTION();
}

static void
gst_streamdemux_notify_alloc_pad (GstStreamDemux * streamdemux)
{
  g_object_notify_by_pspec ((GObject *) streamdemux, pspec_alloc_pad);
}
static void
gst_streamdemux_finalize (GObject * object)
{
  GstStreamDemux *streamdemux;

  streamdemux = GST_STREAMDEMUX (object);
  g_hash_table_unref (streamdemux->pad_indexes);
  g_free (streamdemux->last_message);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void gst_streamdemux_dispose (GObject * object)
{
  GList *item;

restart:
  for (item = GST_ELEMENT_PADS (object); item; item = g_list_next (item)) {
    GstPad *pad = GST_PAD (item->data);
    if (GST_PAD_IS_SRC (pad)) {
      gst_element_release_request_pad (GST_ELEMENT (object), pad);
      goto restart;
    }
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}
static void
gst_streamdemux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstStreamDemux *filter = GST_STREAMDEMUX (object); 

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_HAS_CHAIN:
      filter->has_chain = g_value_get_boolean (value);
      break;
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_PULL_MODE:
      filter->pull_mode = (GstStreamDemuxPullMode) g_value_get_enum (value);
      break;
    case PROP_ALLOC_PAD:
    {
      GstPad *pad = (GstPad *)g_value_get_object (value);
      GST_OBJECT_LOCK (pad);
      if (GST_OBJECT_PARENT (pad) == GST_OBJECT_CAST (object))
        filter->allocpad = pad;
      else
        GST_WARNING_OBJECT (object, "Tried to set alloc pad %s which"
            " is not my pad", GST_OBJECT_NAME (pad));
      GST_OBJECT_UNLOCK (pad);
      break;
    }
    case PROP_ALLOW_NOT_LINKED:
      filter->allow_not_linked = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_streamdemux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstStreamDemux *filter = GST_STREAMDEMUX (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_NUM_SRC_PADS:
      g_value_set_int (value, GST_ELEMENT (filter)->numsrcpads);
      break;
    case PROP_HAS_CHAIN:
      g_value_set_boolean (value, filter->has_chain);
      break;
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_LAST_MESSAGE:
      g_value_set_string (value, filter->last_message);
      break;
    case PROP_PULL_MODE:
      g_value_set_enum (value, filter->pull_mode);
      break;
    case PROP_ALLOC_PAD:
      g_value_set_object (value, filter->allocpad);
      break;
    case PROP_ALLOW_NOT_LINKED:
      g_value_set_boolean (value, filter->allow_not_linked);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static gboolean
forward_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *srcpad = GST_PAD_CAST (user_data);
  GstFlowReturn ret;

  ret = gst_pad_store_sticky_event (srcpad, *event);
  if (ret != GST_FLOW_OK) {
    GST_DEBUG_OBJECT (srcpad, "storing sticky event %p (%s) failed: %s", *event,
        GST_EVENT_TYPE_NAME (*event), gst_flow_get_name (ret));
  }

  return TRUE;
}
static GstPad *gst_streamdemux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{  
  gchar *name;
  GstPad *srcpad;
  GstStreamDemux *streamdemux;
  GstPadMode mode;
  gboolean res;
  guint index = 0;

  printf("===>[streamdemux]->request pad %s.\n",req_name);
  streamdemux = GST_STREAMDEMUX (element);
  GST_DEBUG_OBJECT (streamdemux, "requesting pad");
  GST_OBJECT_LOCK (streamdemux);

  if (req_name && sscanf (req_name, "src_%u", &index) == 1) {
    GST_LOG_OBJECT (element, "name: %s (index %d)", req_name, index);
    if (g_hash_table_contains (streamdemux->pad_indexes, GUINT_TO_POINTER (index))) {
      GST_ERROR_OBJECT (element, "pad name %s is not unique", req_name);
      GST_OBJECT_UNLOCK (streamdemux);
      return NULL;
    }
    if (index >= streamdemux->next_pad_index)
      streamdemux->next_pad_index = index + 1;
  } else {
    index = streamdemux->next_pad_index;

    while (g_hash_table_contains (streamdemux->pad_indexes, GUINT_TO_POINTER (index)))
      index++;

    streamdemux->next_pad_index = index + 1;
  }

  g_hash_table_insert (streamdemux->pad_indexes, GUINT_TO_POINTER (index), NULL);
  name = g_strdup_printf ("src_%u", index);
  srcpad = GST_PAD_CAST (g_object_new (GST_TYPE_STREAMDEMUX_PAD,
          "name", name, "direction", templ->direction, "template", templ,
          NULL));
  GST_STREAMDEMUX_PAD_CAST (srcpad)->index = index;
  g_free (name);

  mode = streamdemux->sink_mode;
  GST_OBJECT_UNLOCK (streamdemux);
  switch (mode) {
    case GST_PAD_MODE_PULL:
      /* we already have a src pad in pull mode, and our pull mode can only be
         SINGLE, so fall through to activate this new pad in push mode */
    case GST_PAD_MODE_PUSH:
      res = gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, TRUE);
      break;
    default:
      res = TRUE;
      break;
  }

  if (!res)
    goto activate_failed;

  gst_pad_set_activatemode_function (srcpad,
      GST_DEBUG_FUNCPTR (gst_streamdemux_src_activate_mode));
  gst_pad_set_query_function (srcpad, GST_DEBUG_FUNCPTR (gst_streamdemux_src_query));
  gst_pad_set_getrange_function (srcpad,
      GST_DEBUG_FUNCPTR (gst_streamdemux_src_get_range));
  GST_OBJECT_FLAG_SET (srcpad, GST_PAD_FLAG_PROXY_CAPS);
  /* Forward sticky events to the new srcpad */
  gst_pad_sticky_events_foreach (streamdemux->sinkpad, forward_sticky_events, srcpad);
  gst_element_add_pad (GST_ELEMENT_CAST (streamdemux), srcpad);

  return srcpad;
  /* ERRORS */
activate_failed:
  {
    gboolean changed = FALSE;

    printf("===>[streamdemux]->request failed pad %s.\n",req_name);
    GST_OBJECT_LOCK (streamdemux);
    GST_DEBUG_OBJECT (streamdemux, "warning failed to activate request pad");
    if (streamdemux->allocpad == srcpad) {
      streamdemux->allocpad = NULL;
      changed = TRUE;
    }
    GST_OBJECT_UNLOCK (streamdemux);
    gst_object_unref (srcpad);
    if (changed) {
      gst_streamdemux_notify_alloc_pad (streamdemux);
    }
    return NULL;
  }
}

static void
gst_streamdemux_release_pad (GstElement * element, GstPad * pad)
{  
  GstStreamDemux *streamdemux;
  gboolean changed = FALSE;
  guint index;

  streamdemux = GST_STREAMDEMUX (element);
  GST_DEBUG_OBJECT (streamdemux, "releasing pad");
  GST_OBJECT_LOCK (streamdemux);
  index = GST_STREAMDEMUX_PAD_CAST (pad)->index;
  printf("===>[streamdemux]->release pad %d.\n",index);
  /* mark the pad as removed so that future pad_alloc fails with NOT_LINKED. */
  GST_STREAMDEMUX_PAD_CAST (pad)->removed = TRUE;
  if (streamdemux->allocpad == pad) {
    streamdemux->allocpad = NULL;
    changed = TRUE;
  }
  GST_OBJECT_UNLOCK (streamdemux);

  gst_object_ref (pad);
  gst_element_remove_pad (GST_ELEMENT_CAST (streamdemux), pad);
  gst_pad_set_active (pad, FALSE);
  gst_object_unref (pad);

  if (changed) {
    gst_streamdemux_notify_alloc_pad (streamdemux);
  }

  GST_OBJECT_LOCK (streamdemux);
  g_hash_table_remove (streamdemux->pad_indexes, GUINT_TO_POINTER (index));
  GST_OBJECT_UNLOCK (streamdemux);
}

static gboolean
gst_streamdemux_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res;
  switch (GST_EVENT_TYPE (event)) {
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

typedef struct _tag_AllocQueryCtx
{
  GstStreamDemux *streamdemux;
  GstQuery *query;
  GstAllocationParams params;
  guint size;
  guint min_buffers;
  gboolean first_query;
  guint num_pads;
}AllocQueryCtx;

/* This function will aggregate some of the allocation query information with
 * the strategy to force upstream allocation. Depending on downstream
 * allocation would otherwise make dynamic pipelines much more complicated as
 * application would need to now drain buffer in certain cases before getting
 * rid of a streamdemux branch. */
static gboolean
gst_streamdemux_query_allocation (const GValue * item, GValue * ret, gpointer user_data)
{
  //struct AllocQueryCtx *ctx = (AllocQueryCtx *)user_data;
  AllocQueryCtx *ctx = (AllocQueryCtx *)user_data;
  GstPad *src_pad = (GstPad *)g_value_get_object (item);
  GstPad *peer_pad;
  GstCaps *caps;
  GstQuery *query;
  guint count, i, size, min;

  GST_DEBUG_OBJECT (ctx->streamdemux, "Aggregating allocation from pad %s:%s",
      GST_DEBUG_PAD_NAME (src_pad));
  printf("!!!====>[streamdemux]->query %s.\n", GST_DEBUG_PAD_NAME (src_pad));

  peer_pad = gst_pad_get_peer (src_pad);
  if (!peer_pad) {
    if (ctx->streamdemux->allow_not_linked) {
      GST_DEBUG_OBJECT (ctx->streamdemux, "Pad %s:%s has no peer, but allowed.",
          GST_DEBUG_PAD_NAME (src_pad));
      printf("[streamdemux]->query Pad %s:%s has no peer, but allowed.",
          GST_DEBUG_PAD_NAME (src_pad));
      return TRUE;
    } else {
      GST_DEBUG_OBJECT (ctx->streamdemux, "Pad %s:%s has no peer, ignoring allocation.",
          GST_DEBUG_PAD_NAME (src_pad));
      printf("[streamdemux]->query Pad %s:%s has no peer, ignoring allocation.",
          GST_DEBUG_PAD_NAME (src_pad));
      g_value_set_boolean (ret, FALSE);
      return FALSE;
    }
  }
  gst_query_parse_allocation (ctx->query, &caps, NULL);
  query = gst_query_new_allocation (caps, FALSE);
  if (!gst_pad_query (peer_pad, query)) {
    GST_DEBUG_OBJECT (ctx->streamdemux,
        "Allocation query failed on pad %s, ignoring allocation",
        GST_PAD_NAME (src_pad));
    g_value_set_boolean (ret, FALSE);
    gst_query_unref (query);
    gst_object_unref (peer_pad);
    return FALSE;
  }
  gst_object_unref (peer_pad);

  /* Allocation Params:
   * store the maximum alignment, prefix and padding, but ignore the
   * allocators and the flags which are tied to downstream allocation */
  count = gst_query_get_n_allocation_params (query);
  for (i = 0; i < count; i++) {
    GstAllocationParams params =  { (GstMemoryFlags)0, 15, 1, 1 };//{ 0, };

    gst_query_parse_nth_allocation_param (query, i, NULL, &params);
    GST_DEBUG_OBJECT (ctx->streamdemux, "Aggregating AllocationParams align=%"
        G_GSIZE_FORMAT " prefix=%" G_GSIZE_FORMAT " padding=%"
        G_GSIZE_FORMAT, params.align, params.prefix, params.padding);

    if (ctx->params.align < params.align)
      ctx->params.align = params.align;

    if (ctx->params.prefix < params.prefix)
      ctx->params.prefix = params.prefix;

    if (ctx->params.padding < params.padding)
      ctx->params.padding = params.padding;
  }

  /* Allocation Pool:
   * We want to keep the biggest size and biggest minimum number of buffers to
   * make sure downstream requirement can be satisfied. We don't really care
   * about the maximum, as this is a parameter of the downstream provided
   * pool. We only read the first allocation pool as the minimum number of
   * buffers is normally constant regardless of the pool being used. */
  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, NULL, &size, &min, NULL);
    GST_DEBUG_OBJECT (ctx->streamdemux,
        "Aggregating allocation pool size=%u min_buffers=%u", size, min);
    if (ctx->size < size)
      ctx->size = size;

    if (ctx->min_buffers < min)
      ctx->min_buffers = min;
  }

  /* Allocation Meta:
   * For allocation meta, we'll need to aggregate the argument using the new
   * GstMetaInfo::agggregate_func */
  count = gst_query_get_n_allocation_metas (query);
  for (i = 0; i < count; i++) {
    guint ctx_index;
    GType api;
    const GstStructure *param;
    api = gst_query_parse_nth_allocation_meta (query, i, &param);

    /* For the first query, copy all metas */
    if (ctx->first_query) {
      gst_query_add_allocation_meta (ctx->query, api, param);
      continue;
    }

    /* Afterward, aggregate the common params */
    if (gst_query_find_allocation_meta (ctx->query, api, &ctx_index)) {
      const GstStructure *ctx_param;
      gst_query_parse_nth_allocation_meta (ctx->query, ctx_index, &ctx_param);

      /* Keep meta which has no params */
      if (ctx_param == NULL && param == NULL)
        continue;

      GST_DEBUG_OBJECT (ctx->streamdemux, "Dropping allocation meta %s",
          g_type_name (api));
      gst_query_remove_nth_allocation_meta (ctx->query, ctx_index);
    }
  }

  /* Finally, cleanup metas from the stored query that aren't support on this
   * pad. */
  count = gst_query_get_n_allocation_metas (ctx->query);
  for (i = 0; i < count;) {
    GType api = gst_query_parse_nth_allocation_meta (ctx->query, i, NULL);

    if (!gst_query_find_allocation_meta (query, api, NULL)) {
      GST_DEBUG_OBJECT (ctx->streamdemux, "Dropping allocation meta %s",
          g_type_name (api));
      gst_query_remove_nth_allocation_meta (ctx->query, i);
      count--;
      continue;
    }
    i++;
  }

  ctx->first_query = FALSE;
  ctx->num_pads++;
  gst_query_unref (query);
  return TRUE;
}

static void
gst_streamdemux_clear_query_allocation_meta (GstQuery * query)
{
  guint count = gst_query_get_n_allocation_metas (query);
  guint i;

  for (i = 1; i <= count; i++)
    gst_query_remove_nth_allocation_meta (query, count - i);
}

static gboolean
gst_streamdemux_sink_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstStreamDemux *streamdemux = GST_STREAMDEMUX (parent);
  gboolean res;
  //43253 GST_QUERY_CAPS <=流程; 再 GST_QUERY_ACCEPT_CAPS 即可。没有执行成功的，查 caps 后没有 accept
  //printf("!!!!====[streamdemux]->sink query mode pad=0x%x %d.\n", pad, GST_QUERY_TYPE (query));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ALLOCATION:
    {
      GstIterator *iter;
      GValue ret = G_VALUE_INIT;
      AllocQueryCtx ctx = { streamdemux, query, };

      g_value_init (&ret, G_TYPE_BOOLEAN);
      g_value_set_boolean (&ret, TRUE);

      ctx.first_query = TRUE;
      gst_allocation_params_init (&ctx.params);

      iter = gst_element_iterate_src_pads (GST_ELEMENT (streamdemux));
      while (GST_ITERATOR_RESYNC ==
          gst_iterator_fold (iter, gst_streamdemux_query_allocation, &ret, &ctx)) {
        gst_iterator_resync (iter);
        ctx.first_query = TRUE;
        gst_allocation_params_init (&ctx.params);
        ctx.size = 0;
        ctx.min_buffers = 0;
        ctx.num_pads = 0;
        gst_streamdemux_clear_query_allocation_meta (query);
      }

      gst_iterator_free (iter);
      res = g_value_get_boolean (&ret);
      g_value_unset (&ret);

      if (res) {
        GST_DEBUG_OBJECT (streamdemux, "Aggregated AllocationParams to align=%"
            G_GSIZE_FORMAT " prefix=%" G_GSIZE_FORMAT " padding=%"
            G_GSIZE_FORMAT, ctx.params.align, ctx.params.prefix,
            ctx.params.padding);

        GST_DEBUG_OBJECT (streamdemux,
            "Aggregated allocation pools size=%u min_buffers=%u", ctx.size,
            ctx.min_buffers);

#ifndef GST_DISABLE_GST_DEBUG
        {
          guint count = gst_query_get_n_allocation_metas (query);
          guint i;

          GST_DEBUG_OBJECT (streamdemux, "Aggregated %u allocation meta:", count);

          for (i = 0; i < count; i++)
            GST_DEBUG_OBJECT (streamdemux, "    + aggregated allocation meta %s",
                g_type_name (gst_query_parse_nth_allocation_meta (ctx.query, i,
                        NULL)));
        }
#endif

        /* Allocate one more buffers when multiplexing so we don't starve the
         * downstream threads. */
        if (ctx.num_pads > 1)
          ctx.min_buffers++;

        /* Check that we actually have parameters besides the defaults. */
        if (ctx.params.align || ctx.params.prefix || ctx.params.padding) {
          gst_query_add_allocation_param (ctx.query, NULL, &ctx.params);
        }
        /* When size == 0, buffers created from this pool would have no memory
         * allocated. */
        if (ctx.size) {
          gst_query_add_allocation_pool (ctx.query, NULL, ctx.size,
              ctx.min_buffers, 0);
        }
      } else {
        gst_streamdemux_clear_query_allocation_meta (query);
      }
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }
  return res;
}

static void
gst_streamdemux_do_message (GstStreamDemux * streamdemux, GstPad * pad, gpointer data, gboolean is_list)
{
  GST_OBJECT_LOCK (streamdemux);
  g_free (streamdemux->last_message);
  if (is_list) {
    streamdemux->last_message =
        g_strdup_printf ("chain-list   ******* (%s:%s)t %p",
        GST_DEBUG_PAD_NAME (pad), data);
  } else {
    streamdemux->last_message =
        g_strdup_printf ("chain        ******* (%s:%s)t (%" G_GSIZE_FORMAT
        " bytes, %" G_GUINT64_FORMAT ") %p", GST_DEBUG_PAD_NAME (pad),
        gst_buffer_get_size ((GstBuffer*)data), GST_BUFFER_TIMESTAMP (data), data);
  }
  GST_OBJECT_UNLOCK (streamdemux);
  printf("!!!!====[streamdemux]->do_message %s.\n", streamdemux->last_message);
  g_object_notify_by_pspec ((GObject *) streamdemux, pspec_last_message);
}

static GstFlowReturn
gst_streamdemux_do_push (GstStreamDemux * streamdemux, GstPad * pad, gpointer data, gboolean is_list)
{
  GstFlowReturn res;

  /* Push */
  if (pad == streamdemux->pull_pad) {
    /* don't push on the pad we're pulling from */
    res = GST_FLOW_OK;
  } else if (is_list) {
    res =
        gst_pad_push_list (pad,
        gst_buffer_list_ref (GST_BUFFER_LIST_CAST (data)));
  } else {
    res = gst_pad_push (pad, gst_buffer_ref (GST_BUFFER_CAST (data)));
  }
  return res;
}

static void
clear_pads (GstPad * pad, GstStreamDemux * streamdemux)
{
  GST_STREAMDEMUX_PAD_CAST (pad)->pushed = FALSE;
  GST_STREAMDEMUX_PAD_CAST (pad)->result = GST_FLOW_NOT_LINKED;
}

static GstFlowReturn
gst_streamdemux_handle_data (GstStreamDemux * streamdemux, gpointer data, gboolean is_list)
{
  GList *pads;
  guint32 cookie;
  GstFlowReturn ret, cret;
  int nIndex = -1;

  if (G_UNLIKELY (!streamdemux->silent))
    gst_streamdemux_do_message (streamdemux, streamdemux->sinkpad, data, is_list);

  GST_OBJECT_LOCK (streamdemux);
  pads = GST_ELEMENT_CAST (streamdemux)->srcpads;

  /* special case for zero pads */
  if (G_UNLIKELY (!pads))
    goto no_pads;

  InferMeta *infer_meta;
  infer_meta = gst_buffer_get_infer_meta(GST_BUFFER_CAST (data));
  if(infer_meta)  {
    //GstMapInfo map;
    //gst_buffer_map(GST_BUFFER_CAST (data), &map, GST_MAP_READ);
    //LOG_INFO("!!!!====[streamdemux]->handle data buf=0x%x,sz=%d meta=%s,padNext=0x%x.\n",
    //  map.data,map.size,infer_meta->extInfo,pads->next);
    nIndex = atoi(infer_meta->extInfo);
    //gst_buffer_unmap(GST_BUFFER_CAST (data), &map);
  }
  /* special case for just one pad that avoids reffing the buffer */
  if (!pads->next) {
    GstPad *pad = GST_PAD_CAST (pads->data);
    /* Keep another ref around, a pad probe
     * might release and destroy the pad */
    gst_object_ref (pad);
    GST_OBJECT_UNLOCK (streamdemux);

    if (pad == streamdemux->pull_pad) {
      ret = GST_FLOW_OK;
    } else if (is_list) {
      ret = gst_pad_push_list (pad, GST_BUFFER_LIST_CAST (data));
    } else {
      ret = gst_pad_push (pad, GST_BUFFER_CAST (data));
    }

    if (GST_STREAMDEMUX_PAD_CAST (pad)->removed)
      ret = GST_FLOW_NOT_LINKED;

    if (ret == GST_FLOW_NOT_LINKED && streamdemux->allow_not_linked) {
      ret = GST_FLOW_OK;
    }
    gst_object_unref (pad);

    return ret;
  }

  /* mark all pads as 'not pushed on yet' */
  g_list_foreach (pads, (GFunc) clear_pads, streamdemux);
restart:
  if (streamdemux->allow_not_linked) {
    cret = GST_FLOW_OK;
  } else {
    cret = GST_FLOW_NOT_LINKED;
  }
  pads = GST_ELEMENT_CAST (streamdemux)->srcpads;
  cookie = GST_ELEMENT_CAST (streamdemux)->pads_cookie;
  while (pads) {
    GstPad *pad;
    pad = GST_PAD_CAST (pads->data);

    if (G_LIKELY (!GST_STREAMDEMUX_PAD_CAST (pad)->pushed)) {
      if(nIndex!=-1){
        if(nIndex!=GST_STREAMDEMUX_PAD_CAST (pad)->index){
          pads = g_list_next (pads);
          continue;
        }
      }
      /* not yet pushed, release lock and start pushing */
      gst_object_ref (pad);
      GST_OBJECT_UNLOCK (streamdemux);
      GST_LOG_OBJECT (pad, "Starting to push %s %p",
          is_list ? "list" : "buffer", data);
      ret = gst_streamdemux_do_push (streamdemux, pad, data, is_list);

      GST_LOG_OBJECT (pad, "Pushing item %p yielded result %s", data,
          gst_flow_get_name (ret));

      GST_OBJECT_LOCK (streamdemux);
      /* keep track of which pad we pushed and the result value */
      GST_STREAMDEMUX_PAD_CAST (pad)->pushed = TRUE;
      GST_STREAMDEMUX_PAD_CAST (pad)->result = ret;
      //LOG_INFO("XXXX====[streamdemux]->handle data 2 index meta=0x%x in=%d : src=%d end.\n",
      //  infer_meta,nIndex,GST_STREAMDEMUX_PAD_CAST (pad)->index);
      gst_object_unref (pad);
      pad = NULL;
    } else {
      /* already pushed, use previous return value */
      ret = GST_STREAMDEMUX_PAD_CAST (pad)->result;
      GST_LOG_OBJECT (pad, "pad already pushed with %s",
          gst_flow_get_name (ret));
    }

    /* before we go combining the return value, check if the pad list is still
     * the same. It could be possible that the pad we just pushed was removed
     * and the return value it not valid anymore */
    if (G_UNLIKELY (GST_ELEMENT_CAST (streamdemux)->pads_cookie != cookie)) {
      GST_LOG_OBJECT (streamdemux, "pad list changed");
      /* the list of pads changed, restart iteration. Pads that we already
       * pushed on and are still in the new list, will not be pushed on
       * again. */
      goto restart;
    }

    /* stop pushing more buffers when we have a fatal error */
    if (G_UNLIKELY (ret != GST_FLOW_OK && ret != GST_FLOW_NOT_LINKED))
      goto error;

    /* keep all other return values, overwriting the previous one. */
    if (G_LIKELY (ret != GST_FLOW_NOT_LINKED)) {
      GST_LOG_OBJECT (streamdemux, "Replacing ret val %d with %d", cret, ret);
      cret = ret;
    }
    pads = g_list_next (pads);
    //只要发送了，就不用循环了 wlwu
    break;
  }
  GST_OBJECT_UNLOCK (streamdemux);

  gst_mini_object_unref (GST_MINI_OBJECT_CAST (data));
  /* no need to unset gvalue */
  return cret;

  /* ERRORS */
no_pads:
  {
    if (streamdemux->allow_not_linked) {
      GST_DEBUG_OBJECT (streamdemux, "there are no pads, dropping %s",
          is_list ? "buffer-list" : "buffer");
      ret = GST_FLOW_OK;
    } else {
      GST_DEBUG_OBJECT (streamdemux, "there are no pads, return not-linked");
      ret = GST_FLOW_NOT_LINKED;
    }
    goto end;
  }
error:
  {
    GST_DEBUG_OBJECT (streamdemux, "received error %s", gst_flow_get_name (ret));
    goto end;
  }
end:
  {
    GST_OBJECT_UNLOCK (streamdemux);
    gst_mini_object_unref (GST_MINI_OBJECT_CAST (data));
    return ret;
  }
}

static GstFlowReturn
gst_streamdemux_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFlowReturn res;
  GstStreamDemux *streamdemux;
  streamdemux = GST_STREAMDEMUX_CAST (parent);
  //printf("!!!!====[streamdemux]->chain data%p.\n", buffer);
  GST_DEBUG_OBJECT (streamdemux, "received buffer %p", buffer);

  res = gst_streamdemux_handle_data (streamdemux, buffer, FALSE);

  GST_DEBUG_OBJECT (streamdemux, "handled buffer %s", gst_flow_get_name (res));

  return res;
}

static GstFlowReturn
gst_streamdemux_chain_list (GstPad * pad, GstObject * parent, GstBufferList * list)
{
  GstFlowReturn res;
  GstStreamDemux *streamdemux;
  streamdemux = GST_STREAMDEMUX_CAST (parent);
  GST_DEBUG_OBJECT (streamdemux, "received list %p", list);

  printf("!!!!====[streamdemux]->received list %p.\n", list);
  res = gst_streamdemux_handle_data (streamdemux, list, TRUE);

  GST_DEBUG_OBJECT (streamdemux, "handled list %s", gst_flow_get_name (res));

  return res;
}

static gboolean
gst_streamdemux_sink_activate_mode (GstPad * pad, GstObject * parent, GstPadMode mode,
    gboolean active)
{
  gboolean res;
  GstStreamDemux *streamdemux;

  streamdemux = GST_STREAMDEMUX (parent);
  printf("!!!!====[streamdemux]->active sink mode %p.\n", streamdemux);

  switch (mode) {
    case GST_PAD_MODE_PUSH:
    {
      GST_OBJECT_LOCK (streamdemux);
      streamdemux->sink_mode = active ? mode : GST_PAD_MODE_NONE;

      if (active && !streamdemux->has_chain)
        goto no_chain;
      GST_OBJECT_UNLOCK (streamdemux);
      res = TRUE;
      break;
    }
    default:
      res = FALSE;
      break;
  }
  return res;

  /* ERRORS */
no_chain:
  {
    GST_OBJECT_UNLOCK (streamdemux);
    GST_INFO_OBJECT (streamdemux,
        "Tee cannot operate in push mode with has-chain==FALSE");
    return FALSE;
  }
}

static gboolean
gst_streamdemux_src_activate_mode (GstPad * pad, GstObject * parent, GstPadMode mode,
    gboolean active)
{
  GstStreamDemux *streamdemux;
  gboolean res;
  GstPad *sinkpad;

  streamdemux = GST_STREAMDEMUX (parent);
  printf("!!!!====[streamdemux]->active src mode %p,mode=%d.\n", streamdemux, mode);
  switch (mode) {
    case GST_PAD_MODE_PULL:
    {
      GST_OBJECT_LOCK (streamdemux);

      if (streamdemux->pull_mode == GST_STREAMDEMUX_PULL_MODE_NEVER)
        goto cannot_pull;

      if (streamdemux->pull_mode == GST_STREAMDEMUX_PULL_MODE_SINGLE && active && streamdemux->pull_pad)
        goto cannot_pull_multiple_srcs;

      sinkpad = (GstPad *)gst_object_ref (streamdemux->sinkpad);

      GST_OBJECT_UNLOCK (streamdemux);

      res = gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PULL, active);
      gst_object_unref (sinkpad);

      if (!res)
        goto sink_activate_failed;

      GST_OBJECT_LOCK (streamdemux);
      if (active) {
        if (streamdemux->pull_mode == GST_STREAMDEMUX_PULL_MODE_SINGLE)
          streamdemux->pull_pad = pad;
      } else {
        if (pad == streamdemux->pull_pad)
          streamdemux->pull_pad = NULL;
      }
      streamdemux->sink_mode = (active ? GST_PAD_MODE_PULL : GST_PAD_MODE_NONE);
      GST_OBJECT_UNLOCK (streamdemux);
      break;
    }
    default:
      res = TRUE;
      break;
  }
  printf("!!!!====[streamdemux]->active src mode %p res=%d.\n", streamdemux,res);
  return res;

  /* ERRORS */
cannot_pull:
  {
    GST_OBJECT_UNLOCK (streamdemux);
    GST_INFO_OBJECT (streamdemux, "Cannot activate in pull mode, pull-mode "
        "set to NEVER");
    return FALSE;
  }
cannot_pull_multiple_srcs:
  {
    GST_OBJECT_UNLOCK (streamdemux);
    GST_INFO_OBJECT (streamdemux, "Cannot activate multiple src pads in pull mode, "
        "pull-mode set to SINGLE");
    return FALSE;
  }
sink_activate_failed:
  {
    GST_INFO_OBJECT (streamdemux, "Failed to %sactivate sink pad in pull mode",
        active ? "" : "de");
    return FALSE;
  }
}

static gboolean
gst_streamdemux_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstStreamDemux *streamdemux;
  gboolean res;
  GstPad *sinkpad;

  streamdemux = GST_STREAMDEMUX (parent);
  //printf("!!!!====[streamdemux]->src query mode %d.\n", GST_QUERY_TYPE (query));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_SCHEDULING:
    {
      gboolean pull_mode;

      GST_OBJECT_LOCK (streamdemux);
      pull_mode = TRUE;
      if (streamdemux->pull_mode == GST_STREAMDEMUX_PULL_MODE_NEVER) {
        GST_INFO_OBJECT (streamdemux, "Cannot activate in pull mode, pull-mode "
            "set to NEVER");
        pull_mode = FALSE;
      } else if (streamdemux->pull_mode == GST_STREAMDEMUX_PULL_MODE_SINGLE && streamdemux->pull_pad) {
        GST_INFO_OBJECT (streamdemux, "Cannot activate multiple src pads in pull mode, "
            "pull-mode set to SINGLE");
        pull_mode = FALSE;
      }

      sinkpad = (GstPad *)gst_object_ref (streamdemux->sinkpad);
      GST_OBJECT_UNLOCK (streamdemux);

      if (pull_mode) {
        /* ask peer if we can operate in pull mode */
        res = gst_pad_peer_query (sinkpad, query);
      } else {
        res = TRUE;
      }
      gst_object_unref (sinkpad);
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static void
gst_streamdemux_push_eos (const GValue * vpad, GstStreamDemux * streamdemux)
{
  GstPad *pad = (GstPad *)g_value_get_object (vpad);

  if (pad != streamdemux->pull_pad)
    gst_pad_push_event (pad, gst_event_new_eos ());
}

static void
gst_streamdemux_pull_eos (GstStreamDemux * streamdemux)
{
  GstIterator *iter;

  iter = gst_element_iterate_src_pads (GST_ELEMENT (streamdemux));
  while (gst_iterator_foreach (iter,
          (GstIteratorForeachFunction) gst_streamdemux_push_eos,
          streamdemux) == GST_ITERATOR_RESYNC)
    gst_iterator_resync (iter);
  gst_iterator_free (iter);
}

static GstFlowReturn
gst_streamdemux_src_get_range (GstPad * pad, GstObject * parent, guint64 offset,
    guint length, GstBuffer ** buf)
{
  GstStreamDemux *streamdemux;
  GstFlowReturn ret;

  streamdemux = GST_STREAMDEMUX (parent);

  ret = gst_pad_pull_range (streamdemux->sinkpad, offset, length, buf);

  if (ret == GST_FLOW_OK)
    ret = gst_streamdemux_handle_data (streamdemux, gst_buffer_ref (*buf), FALSE);
  else if (ret == GST_FLOW_EOS)
    gst_streamdemux_pull_eos (streamdemux);

  return ret;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
streamdemux_init (GstPlugin * streamdemux)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template streamdemux' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_streamdemux_debug, "streamdemux",
      0, "Template streamdemux");

  return gst_element_register (streamdemux, "streamdemux", GST_RANK_NONE,
      GST_TYPE_STREAMDEMUX);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirststreamdemux"
#endif

/* gstreamer looks for this structure to register tts
 *
 * exchange the string 'Template streamdemux' with your streamdemux description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    streamdemux,
    "Template streamdemux",
    streamdemux_init,
    PLUGIN_VERSION, PLUGIN_LICENSE, PACKAGE_NAME, GST_PACKAGE_ORIGIN)
