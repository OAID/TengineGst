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
 
#ifndef __GST_VIDEOANALYSIS_H__
#define __GST_VIDEOANALYSIS_H__

#include <gst/gst.h>

G_BEGIN_DECLS 

/* #defines don't like whitespacey bits */
#define GST_TYPE_VIDEOANALYSIS \
  (gst_videoanalysis_get_type())
#define GST_VIDEOANALYSIS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEOANALYSIS,Gstvideoanalysis))
#define GST_VIDEOANALYSIS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEOANALYSIS,GstvideoanalysisClass))
#define GST_IS_VIDEOANALYSIS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEOANALYSIS))
#define GST_IS_VIDEOANALYSIS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEOANALYSIS))

typedef struct _Gstvideoanalysis      Gstvideoanalysis;
typedef struct _GstvideoanalysisClass GstvideoanalysisClass;

struct _Gstvideoanalysis
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  gboolean silent;
  /* settings */
  gint width, height;
  gint fps_n, fps_d;
  gint par_n, par_d;
  gint detectInterval;
  gint nDetectIdx;
  //meta data
  void *mInferHdl;
  gchar *boxesMeta[2];
  //gchar *tmpBoxesMetadata;
  gchar *arrBoxesMetadata[32];//32路足够了
  int nCurSel;
  gchar          *tsWork_dll;
  gchar          *tsModel_url;
};

struct _GstvideoanalysisClass 
{
  GstElementClass parent_class;
};

GType gst_videoanalysis_get_type (void);

G_END_DECLS

#endif /* __GST_FACE_H__ */
