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
 
#ifndef __GST_MQTT_H__
#define __GST_MQTT_H__

#include <gst/gst.h>

G_BEGIN_DECLS 

/* #defines don't like whitespacey bits */
#define GST_TYPE_MQTT \
  (gst_mqtt_get_type())
#define GST_MQTT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MQTT,Gstmqtt))
#define GST_MQTT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MQTT,GstmqttClass))
#define GST_IS_MQTT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MQTT))
#define GST_IS_MQTT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MQTT))

typedef struct _Gstmqtt      Gstmqtt;
typedef struct _GstmqttClass GstmqttClass;

struct _Gstmqtt
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  gboolean silent;
  gchar *tsChnID;
  gchar tsUsr[64];
  gchar tsUsrPwd[64];
  gchar tsServIP[64];
  int nServPort;
};

struct _GstmqttClass 
{
  GstElementClass parent_class;
};

GType gst_mqtt_get_type (void);

G_END_DECLS

#endif /* __GST_MQTT_H__ */
