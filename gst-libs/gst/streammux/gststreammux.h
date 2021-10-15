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
 
#ifndef __GST_STREAMMUX_H__
#define __GST_STREAMMUX_H__

#include <gst/gst.h>

G_BEGIN_DECLS 

/* #defines don't like whitespacey bits */
#define GST_TYPE_STREAMMUX \
  (gst_streammux_get_type())
#define GST_STREAMMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_STREAMMUX,GstStreamMux))
#define GST_STREAMMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_STREAMMUX,GstStreamMuxClass))
#define GST_IS_STREAMMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_STREAMMUX))
#define GST_IS_STREAMMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_STREAMMUX))
  
#define GST_STREAMMUX_CAST(obj) ((GstStreamMux *)(obj))

typedef struct _GstStreamMux      GstStreamMux;
typedef struct _GstStreamMuxClass GstStreamMuxClass;

typedef struct _pad_Private
{
  gboolean have_timestamp_offset;
  guint timestamp_offset;

  //GstSegment segment;
  gboolean priority;
  gint nChnID;  //哪一路流
  gboolean got_eos;
} GstSMuxPadPrivate;

struct _GstStreamMux
{
  GstElement element;

  GstPad  *srcpad;//*sinkpad,
  /* sinkpads */
  /* pad counter, used for creating unique request pads */
  gint            padcount;
  //GstCollectPads *collect;
  GstPad *last_sinkpad; /* protected by object lock */
  /* settings */
  gint width, height;
  gint nCurSel;

  gboolean silent;
  gboolean forward_sticky_events;
};

struct _GstStreamMuxClass 
{
  GstElementClass parent_class;
};

GType gst_streammux_get_type (void);

G_END_DECLS

#endif /* __GST_FACE_H__ */
