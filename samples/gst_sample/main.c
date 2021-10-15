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
 
#include <gst/gst.h>
#include <glib.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

typedef struct _avc_data
{
	GstElement *rtsp_source;
	GstElement *rtsp_depay;
	GstElement *decode;
	GstElement *app_queue, *app_sink;
	GstElement *pipeline;
} avc_data_t;

// TODO似乎不建议这么使用，但我没有找到更好的方法
struct mutex_wrapper : std ::mutex
{
	mutex_wrapper() = default;
	mutex_wrapper(mutex_wrapper const &) noexcept : std ::mutex() {}
	// bool operator==(mutex_wrapper const &other) noexcept { return this == &other; }
};

typedef struct thread_info_t
{
	int ch_id = 0;
	int type = 0; //解码类型
	int fps = 0;
	char url[64] = {};
	mutex_wrapper imgMutex;
	Mat image;
} thread_info_s;

/************************************************************************************************************/
/*	This example builds the following pipeline:                                                            	*/
/*                                                             											   	*/
/* +-----------+   +------------ +   +------------+   +------------+   +------------+     				   	*/
/* |rtspsrc    |   |rtph264depay |   |v4l2h264dec |   |queue 	   |   |appsink		|            		   	*/
/* |        src|-->|sink      src|-->|sink     src|-->|sink     src|-->|sink        |-->display				*/
/* +-----------+   +-------------+   +------------+   +------------+   +------------+    			       	*/
/*                                                                                                         	*/
/************************************************************************************************************/

/*
软解
gst-launch-1.0 rtspsrc location=rtsp://admin:admin@10.15.4.44/ ! rtph264depay ! capsfilter caps="video/x-h264" ! h264parse ! avdec_h264 ! videoconvert ! ximagesink
播放视频文件
gst-launch-1.0 filesrc location=./video.mp4 ! qtdemux ! avdec_h264 ! videoconvert ! ximagesink
*/

static void avc_pad_added_handler(GstElement *src, GstPad *new_pad, avc_data_t *data)
{
    GstPad *sink_pad = gst_element_get_static_pad(data->rtsp_depay, "sink");
    GstPadLinkReturn ret;

    g_print("Received new add '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* If decoder is already linked, nothing to do here */
    if (gst_pad_is_linked(sink_pad))
    {
        g_print("We are aulready linked. Ignoring.\n");
        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link(new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED(ret))
    {
        g_print("Link failed.\n");
    }
    else
        g_print("Link succeeded.\n");

exit:
    gst_object_unref(sink_pad);

    return;
}

static void avc_cb_message(GstBus *bus, GstMessage *msg, avc_data_t *data)
{
    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError *err;
        gchar *debug_info;

        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);

        gst_element_set_state(data->pipeline, GST_STATE_READY);
        //g_main_loop_quit (data->loop);
        break;
    }
    case GST_MESSAGE_EOS:
        g_print("End-Of_Stream reached.\n");
        gst_element_set_state(data->pipeline, GST_STATE_READY);
        //g_main_loop_quit(data->loop);
        break;

    case GST_MESSAGE_STATE_CHANGED:
        /* We are only interested in state-changed messages from the pipeline */
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline))
        {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            g_print("Pipeline state changed from %s to %s.\n", gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
        }
        break;

    default:
        //g_printerr("Unexpected message received.\n");
        break;
    }

    return;
}

static GstFlowReturn avc_new_sample_on_appsink(GstElement *sink, void *parg)
{
    thread_info_s *pThreadInfo = (thread_info_s *)parg;

    GstSample *sample = NULL;
    GstBuffer *buffer = NULL;
    GstMapInfo map;

    /* Retrieve the buffer */
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample)
    {
        buffer = gst_sample_get_buffer(sample);
        if (gst_buffer_map(buffer, &map, GST_MAP_READ))
        {
            pThreadInfo->fps++;

            // 获取图像宽高
            gint width = 0;
            gint height = 0;
            GstCaps *frame_caps = gst_sample_get_caps(sample);
            GstStructure *structure = gst_caps_get_structure(frame_caps, 0);
            gst_structure_get_int(structure, "width", &width);
            gst_structure_get_int(structure, "height", &height);

            if (pThreadInfo->type == 2 || pThreadInfo->type == 3)
            {
                // map.data  YUV420sp_NV12格式
                Mat yuvImg;
                yuvImg.create(height * 3 / 2, width, CV_8UC1);
                memcpy(yuvImg.data, map.data, width * height * 3 / 2 * sizeof(unsigned char));
                Mat bgrImg;
                // cv::cvtColor(yuvImg, bgrImg, CV_YUV2BGR_I420);
                cv::cvtColor(yuvImg, bgrImg, CV_YUV2BGR_NV12);

                // 裁掉最后8行
                if (height > 1080)
                {
                    bgrImg = bgrImg(Rect(0, 0, 1920, 1080));
                }

                resize(bgrImg, bgrImg, cv::Size(width / 2, height / 2));

                pThreadInfo->imgMutex.lock();
                pThreadInfo->image = bgrImg.clone();
                pThreadInfo->imgMutex.unlock();
            }

            gst_buffer_unmap(buffer, &map);
        }
        gst_sample_unref(sample);
    }
    else
    {
        g_print("Sample is null.\n");
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

// 参数包含
int decode(void *parg)
{
    thread_info_s *pThreadInfo = (thread_info_s *)parg;

    avc_data_t avc_data;
    memset(&avc_data, 0, sizeof(avc_data));

    GstBus *avc_bus;
    GstStateChangeReturn ret;

    avc_data.rtsp_source = gst_element_factory_make("rtspsrc", "rtsp_source");
    // g_object_set(avc_data.rtsp_source, "protocols", 4, NULL);

    if (pThreadInfo->type == 0 || pThreadInfo->type == 2)
    {
        avc_data.rtsp_depay = gst_element_factory_make("rtph264depay", "rtsp_depay");
        // avc_data.decode = gst_element_factory_make("v4l2h264dec", "avc_decode");
		avc_data.decode = gst_element_factory_make("amlvdec", "avc_decode");
        // avc_data.decode = gst_element_factory_make("avdec_h264", "avc_decode");
    }
    else
    {
        avc_data.rtsp_depay = gst_element_factory_make("rtph265depay", "rtsp_depay");
        avc_data.decode = gst_element_factory_make("v4l2h265dec", "avc_decode");
        // avc_data.decode = gst_element_factory_make("avdec_h265", "avc_decode");
    }

    avc_data.app_queue = gst_element_factory_make("queue", "avc_app_queue");
    avc_data.app_sink = gst_element_factory_make("appsink", "avc_app_sink");

    /* Create the empty pipeline */
    avc_data.pipeline = gst_pipeline_new("avc-pipeline");

    // 检查是否所有都创建成功
    if (!avc_data.pipeline || !avc_data.rtsp_source ||
        !avc_data.rtsp_depay || !avc_data.decode ||
        !avc_data.app_queue || !avc_data.app_sink)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // 设置app_sink
    g_object_set(avc_data.app_sink, "emit_signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(avc_data.app_sink, "new-sample", G_CALLBACK(avc_new_sample_on_appsink), pThreadInfo);

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(avc_data.pipeline), avc_data.rtsp_source, avc_data.rtsp_depay, avc_data.decode, avc_data.app_queue, avc_data.app_sink, NULL);

    if (!gst_element_link(avc_data.rtsp_depay, avc_data.decode) || !gst_element_link(avc_data.decode, avc_data.app_queue) ||
        !gst_element_link(avc_data.app_queue, avc_data.app_sink))
    {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(avc_data.pipeline);
        return -1;
    }

    /* 设置网络ip */
    g_object_set(avc_data.rtsp_source, "location", pThreadInfo->url, NULL);
    // 将rtsp流和回调函数链接
    g_signal_connect(avc_data.rtsp_source, "pad-added", G_CALLBACK(avc_pad_added_handler), &avc_data);

    // 设置为播放
    ret = gst_element_set_state(avc_data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(avc_data.pipeline);
        return -1;
    }

    /* Listen to the bus */
    avc_bus = gst_element_get_bus(avc_data.pipeline);
    gst_bus_add_signal_watch(avc_bus);
    g_signal_connect(avc_bus, "message", G_CALLBACK(avc_cb_message), &avc_data);

    GstMessage *msg = gst_bus_timed_pop_filtered(avc_bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR);
    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(avc_bus);
    gst_element_set_state(avc_data.pipeline, GST_STATE_NULL);
    gst_object_unref(avc_data.pipeline);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		cout << "Missing input parameter" << endl;
		return -1;
	}
	else
	{
		int type = atoi(argv[1]);
	}
	
	vector<thread_info_s> vThreadInfo;

	for (int i = 2; i < argc; i++)
	{
		thread_info_s temp;

		temp.ch_id = i - 1;
		temp.type = atoi(argv[1]);
		strcpy(temp.url, argv[i]);
		vThreadInfo.push_back(temp);
	}

	/* Initialize GStreamer */
	gst_init(&argc, &argv);

	for (int i = 0; i < vThreadInfo.size(); i++)
	{
		thread t(decode, &vThreadInfo.at(i));
		t.detach();
	}
	
	while (1)
	{
		sleep(1);
	}
	return 0;
}