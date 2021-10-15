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
 
#ifndef __GST_POSTPROCESS_H__
#define __GST_POSTPROCESS_H__

#include <gst/gst.h>

G_BEGIN_DECLS 

/* #defines don't like whitespacey bits */
#define GST_TYPE_POSTPROCESS \
  (gst_postprocess_get_type())
#define GST_POSTPROCESS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_POSTPROCESS,Gstpostprocess))
#define GST_POSTPROCESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_POSTPROCESS,GstpostprocessClass))
#define GST_IS_POSTPROCESS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_POSTPROCESS))
#define GST_IS_POSTPROCESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_POSTPROCESS))

typedef struct _Gstpostprocess      Gstpostprocess;
typedef struct _GstpostprocessClass GstpostprocessClass;

struct _Gstpostprocess
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  gboolean silent;
  void *gPostprocessHdl;
};

struct _GstpostprocessClass 
{
  GstElementClass parent_class;
};

GType gst_postprocess_get_type (void);

G_END_DECLS

#endif /* __GST_POSTPROCESS_H__ */
