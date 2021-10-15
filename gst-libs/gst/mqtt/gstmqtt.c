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
 * SECTION:element-mqtt
 *
 * FIXME:Describe mqtt here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! mqtt ! fakesink silent=TRUE
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

#include "gstmqtt.h"
#include "mqtt.h"
#include "common.h"
#include "gst_infer_meta.h"

GST_DEBUG_CATEGORY_STATIC (gst_mqtt_debug);
#define GST_CAT_DEFAULT gst_mqtt_debug

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
  PROP_USR_NAME,
  PROP_USR_PWD,
  PROP_SERV_IP,
  PROP_SERV_PORT,
};


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

#define gst_mqtt_parent_class parent_class
G_DEFINE_TYPE (Gstmqtt, gst_mqtt, GST_TYPE_ELEMENT);

static void gst_mqtt_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_mqtt_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_mqtt_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_mqtt_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);


/* GObject vmethod implementations */

/* initialize the mqtt's class */
static void
gst_mqtt_class_init (GstmqttClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  ENTER_FUNCTION();

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_mqtt_set_property;
  gobject_class->get_property = gst_mqtt_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));
  //用户名，密码，服务地址，端口号
  g_object_class_install_property (gobject_class, PROP_USR_NAME,
      g_param_spec_string ("username", "user name",
      "user name", "{}",
      (GParamFlags)(G_PARAM_READWRITE  )));
  g_object_class_install_property (gobject_class, PROP_USR_PWD,
      g_param_spec_string ("userpwd", "user password",
      "user password", "{}",
      (GParamFlags)(G_PARAM_READWRITE  )));
  g_object_class_install_property (gobject_class, PROP_SERV_IP,
      g_param_spec_string ("servip", "server ip",
      "server ip", "{}",
      (GParamFlags)(G_PARAM_READWRITE  )));
  g_object_class_install_property (gobject_class, PROP_SERV_PORT,
      g_param_spec_int ("servport", "server port", "server port",
          0, G_MAXINT, 1883, (GParamFlags)(G_PARAM_READWRITE )));
      

  gst_element_class_set_details_simple(gstelement_class,
    "mqtt",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "econe <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
  //MQTT_init("172.17.0.1");
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_mqtt_init (Gstmqtt * filter)
{
  ENTER_FUNCTION();

  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_mqtt_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_mqtt_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = TRUE;
}

static void
gst_mqtt_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstmqtt *filter = GST_MQTT (object); 

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_USR_NAME:{
      const gchar *vTsVal = g_value_get_string(value);
      if (vTsVal)
        strcpy(filter->tsUsr,vTsVal);
      break;
      }
    case PROP_USR_PWD:{
      const gchar *vTsVal = g_value_get_string(value);
      if (vTsVal)
        strcpy(filter->tsUsrPwd,vTsVal);
      break;
      }
    case PROP_SERV_IP:{
      const gchar *vTsVal = g_value_get_string(value);
      if (vTsVal)
        strcpy(filter->tsServIP,vTsVal);
      break;
      }
    case PROP_SERV_PORT:
      filter->nServPort = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mqtt_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstmqtt *filter = GST_MQTT(object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_USR_NAME:
      g_value_set_string(value,filter->tsUsr);
      break;
    case PROP_USR_PWD:
      g_value_set_string(value,filter->tsUsrPwd);
      break;
    case PROP_SERV_IP:
      g_value_set_string(value,filter->tsServIP);
      break;
    case PROP_SERV_PORT:
      g_value_set_int(value,filter->nServPort);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_mqtt_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstmqtt *filter;
  gboolean ret;

  filter = GST_MQTT (parent);

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


/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_mqtt_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstmqtt *filter;
  InferMeta *infer_meta;
  
  filter = GST_MQTT (parent);

  if (filter->silent == FALSE)
    g_print ("I'm plugged, therefore I'm in.\n");

  infer_meta = gst_buffer_get_infer_meta(buf);
  if (infer_meta && infer_meta->boxes)
  {
    do{
  	if(-2==MQTT_publish("detect_result", infer_meta->boxes)){
      MQTT_init(filter->tsUsr,filter->tsUsrPwd,filter->tsServIP,filter->nServPort);
      continue;
    }
    }while(0);
  }

  /* just push out the incoming buffer without touching it */
  return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
mqtt_init (GstPlugin * mqtt)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template mqtt' with your description
   */

  ENTER_FUNCTION();
  
  GST_DEBUG_CATEGORY_INIT (gst_mqtt_debug, "mqtt",
      0, "Template mqtt");

  return gst_element_register (mqtt, "mqtt", GST_RANK_NONE,
      GST_TYPE_MQTT);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstmqtt"
#endif

/* gstreamer looks for this structure to register tts
 *
 * exchange the string 'Template mqtt' with your mqtt description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    mqtt,
    "Template mqtt",
    mqtt_init,
    PLUGIN_VERSION, PLUGIN_LICENSE, PACKAGE_NAME, GST_PACKAGE_ORIGIN)
