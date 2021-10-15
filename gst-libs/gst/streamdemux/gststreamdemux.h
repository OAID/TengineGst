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
 
#ifndef __GST_STREAMDEMUX_H__
#define __GST_STREAMDEMUX_H__

#include <gst/gst.h>

G_BEGIN_DECLS 

/* #defines don't like whitespacey bits */
#define GST_TYPE_STREAMDEMUX \
  (gst_streamdemux_get_type())
#define GST_STREAMDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_STREAMDEMUX,GstStreamDemux))
#define GST_STREAMDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_STREAMDEMUX,GstStreamDemuxClass))
#define GST_IS_STREAMDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_STREAMDEMUX))
#define GST_IS_STREAMDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_STREAMDEMUX))
#define GST_STREAMDEMUX_CAST(obj) ((GstStreamDemux*) obj)

typedef struct _GstStreamDemux      GstStreamDemux;
typedef struct _GstStreamDemuxClass GstStreamDemuxClass;

typedef enum {
  GST_STREAMDEMUX_PULL_MODE_NEVER,
  GST_STREAMDEMUX_PULL_MODE_SINGLE,
} GstStreamDemuxPullMode;

typedef struct _pad_Private
{
  gboolean have_timestamp_offset;
  guint timestamp_offset;

  //GstSegment segment;
  gboolean priority;
  gint nChnID;  //哪一路流
  gboolean got_eos;
} GstSDemuxPadPrivate;

struct _GstStreamDemux
{
  GstElement element;

  GstPad         *sinkpad;
  GstPad         *allocpad;

  GHashTable     *pad_indexes;
  guint           next_pad_index;

  gboolean        has_chain;
  gboolean        silent;
  gchar          *last_message;

  GstPadMode      sink_mode;
  GstStreamDemuxPullMode  pull_mode;
  GstPad         *pull_pad;

  gboolean        allow_not_linked;
  /* settings */
  gint width, height;
  gint nCurSel;
};

struct _GstStreamDemuxClass 
{
  GstElementClass parent_class;
};

GType gst_streamdemux_get_type (void);

G_END_DECLS

#endif /* __GST_FACE_H__ */
