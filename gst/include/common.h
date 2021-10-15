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
#ifndef     __COMMON_H__
#define     __COMMON_H__

#include <stdio.h>
#include <stdlib.h>

#include "linux_list.h"
#include "mempool.h"
#include "loger.h"

#ifdef	__cplusplus
extern "C" { 
#endif

typedef void* (*TASK_ENTRY)(void *pArg);

#define     BIT(x)                  (0x1 << (x))
#define     ARRAY_SIZE(x)       	(sizeof(x)/sizeof(x[0]))
#define		ROUND_UP(x, unit)	    (((x)/(unit) + !!((x)%(unit))) * (unit))
#define		ROUND_DOWN(x, unit)	    (((x)/(unit)) * (unit))

#define		POINT_CHECK_RETURN_INT(ptr)			\
do {											\
	if (NULL == ptr) {							\
		LOG_ERROR("bad params: %s\n", #ptr);	\
		return -1;								\
	}											\
} while(0)

#define		POINT_CHECK_RETURN_NULL(ptr)		\
do {											\
	if (NULL == ptr) {							\
		LOG_ERROR("bad params: %s\n", #ptr);	\
		return NULL;							\
	}											\
} while(0)

#define		POINT_CHECK_RETURN_VOID(ptr)		\
do {											\
	if (NULL == ptr) {							\
		LOG_ERROR("bad params: %s\n", #ptr);	\
		return;     							\
	}											\
} while(0)

#define		ENTER_FUNCTION()		LOG_INFO("enter %s", __func__)
#define		EXIT_FUNCTION()			LOG_INFO("exit %s", __func__)

#define		MODEL_INTERFACE			/* 仅仅代表一个标志 */
#define		MAP_DIR					("/mnt")				/* /root/docker --> /mnt */
#define		CHECKFILE_PATH			("/tmp/checkfile")
#define     CHECKSUM_STRING_LEN 	(strlen("checksum = ") + CHECKSUM_LEN)	
#define		AUTH_SERVER_HOST		("cloud.openailab.com:80")
#define     HASH_MAP_SIZE           (1024)
#define     CHECKSUM_LEN            (32)
#define     HASH_MAP_SIZE           (1024)
#define     MAX_FRAME_SIZE          (1000 * 1024)
#define		MAX_GRAPH_SIZE			(32)
#define		MAX_STREAM_CH			(32)
#define 	MMC_CID_LENS 			(32)
#define		MAX_LAYOUT_LEN			(5 * 1024 * 1024)
#define		STREAM_WIDTH			(1280)
#define		STREAM_HIGHT			(720)
#define		NOMAL_LAYOUT_LEN		(500 * 1024)
#define		USE_MMEMPOOL			(1)
/* define for mqtt */
#define		MQTT_REQUEST			("mqtt_request")
#define		MQTT_RESPONSE			("mqtt_response")

typedef enum PARAM_TYPE_T
{
	PARAM_TYPE_INT = 0x0,
	PARAM_TYPE_STRING,
	PARAM_TYPE_VOID,
}PARAM_TYPE_E;

typedef enum FUNCTION_TYPE_T
{
	FUNCTION_TYPE_INT = 0x0,
	FUNCTION_TYPE_STRING,
	FUNCTION_TYPE_VOID,
	FUNCTION_TYPE_NORMAL,
}FUNCTION_TYPE_E;

typedef enum FRAME_TYPE_T
{
	FRAME_TYPE_IDR		= (0x1 << 0),
	FRAME_TYPE_P   		= (0x1 << 1),
	FRAME_TYPE_IDR_H265	= (0x1 << 2),
	FRAME_TYPE_P_H265   = (0x1 << 3),	
	FRAME_TYPE_G711		= (0x1 << 30),
	FRAME_TYPE_AAC   	= (0x1 << 31),	
}FRAME_TYPE_E;

typedef enum FRAME_FORMATE_T
{
	FRAME_FORMAT_JPEG = 0, 	 			// JPEG格式
	FRAME_FORMAT_H264 = 1, 	 			// H264格式
	FRAME_FORMAT_H265 = 2, 				// H265格式
	
	FRAME_FORMAT_YVU420SP 		= 100,	// YVU420SP格式
	FRAME_FORMAT_BGR_PACKAGE 	= 101,	// BGR Package 格式
	FRAME_FORMAT_RGB_PACKAGE 	= 102,	// RGB Package 格式
	FRAME_FORMAT_BGR_PLANAR 	= 103, 	// BGR Planar 格式
	FRAME_FORMAT_RGB_PLANAR 	= 104, 	// BGR Planar 格式
	FRAME_FORMAT_BGRA_PLANAR 	= 105,	// BGRA Planar 格式
	FRAME_FORMAT_RGBA_PLANAR 	= 106,	// BGRA Planar 格式
	
	FRAME_FORMAT_PCM = 200,
}FRAME_FORMATE_E;

typedef enum NAL_TYPE_T
{
	TYPE_SLICE  = 0x1,
	TYPE_IDR    = 0x5,			
	TYPE_SEI    = 0x6,		
	TYPE_SPS    = 0x7,
	TYPE_PPS    = 0x8,	
	TYPE_AUD    = 0x9,		
}NAL_TYPE_E;

typedef enum H265_NAL_TYPE_T
{
	H265_NAL_VPS = 32,
	H265_NAL_SPS = 33,
	H265_NAL_PPS = 34,
	H265_NAL_SEI = 39,
	H265_NAL_IDR = 19,
}H265_NAL_TYPE_E;

typedef enum ALGO_VENDOR_T
{
	VENDOR_UNIUBI = 0x1,
}ALGO_VENDOR_E;

/* 用hash表管理 */
typedef struct TASK_INFO_T
{
    int run;
    char pszKey[32];
    pthread_t TaskId;
    TASK_ENTRY task;
    void *pArg;
    void *pModule;
    struct list_head list;
}TASK_INFO_S;

/* 用hash表管理 */
typedef struct FUNCTION_INFO_T
{
    char pszKey[32];					/* 关键字 */
    FUNCTION_TYPE_E type;
    int min_value;             			/* 仅在(INT/INT_STR)类型使用 */ 
    int max_value;						/* 仅在(INT/INT_STR)类型使用 */  
    unsigned int len;			        /* 仅在(STRING/VOID)类型使用 */  
    void *function;
    void *pArg;
	void *pModule;
}FUNCTION_INFO_S;

/* 用hash表管理 */
typedef struct PARAM_INFO_T
{
    void *pModule;
    char key[32];                       /* 关键字 */
	PARAM_TYPE_E type;					/* 标识参数类型 (PARAM_TYPE_INT/PARAM_TYPE_STRING/PARAM_TYPE_CMD) */
	void *pAddr;                      	/* 对应的参数 */
    int min_value;             			/* 仅在(INT/INT_STR)类型使用 */ 
    int max_value;						/* 仅在(INT/INT_STR)类型使用 */ 
    unsigned int len;					/* 仅在(STRING/VOID)类型使用 */
	void *callback;						/* 参数设置的时候调用回调函数 */
}PARAM_INFO_S;

typedef struct FRAME_DATA_T
{
	int nChnID;
	FRAME_FORMATE_E type;
	int width;
	int height;
	int len;
	unsigned char *data;
	memory_pool_node *buffer;
}FRAME_DATA_S;

typedef struct BBOX_T
{
	float x;
	float y;
	float width;
	float heigh;
	unsigned int uColor;
	float score;
	int label;
	char rect_info[10];
}BBOX_S;

typedef struct RTSP_CLIENT_CHN_T
{
	char url[128];
	char username[64];
	char password[64];
	int port;
	int overTcp;
}RTSP_CLIENT_CHN_S;

typedef struct SUBDEV_CHN_T
{
	int used;
	int chnId;
	char name[64];
	char position[64];	
	int proto_type;
	int draw_rect;
	RTSP_CLIENT_CHN_S rtspchn;	
}SUBDEV_CHN_S;

typedef struct SPS_INFO_T
{
    int len;
    unsigned char data[32];
}SPS_INFO;

typedef struct PPS_INFO_T
{
    int len;
    unsigned char data[32];
}PPS_INFO;

typedef struct VPS_INFO_T
{
    int len;
    unsigned char data[32];
}VPS_INFO;

typedef struct AVC_HEADER_T
{
    SPS_INFO sps;
    PPS_INFO pps;
	VPS_INFO vps;
}AVC_HEADER;

typedef struct RTSP_CHANNEL_T
{
	int id;
	int used;
	AVC_HEADER avcHeader;
	void *pAvBuffer;
}RTSP_CHANNEL_S;

typedef struct RTSP_CHN_HANDLE_T
{
	int id;
	int used;
	int h265;
	AVC_HEADER avcHeader;
	void *pAvBuffer;
}RTSP_CHN_HANDLE_S;

typedef struct ALARM_INFO_T
{
	SUBDEV_CHN_S ChnInfo;
	unsigned char *image;
	unsigned long image_len;
	unsigned char *mini_image;//抓拍小图
	unsigned long mini_image_len;
	char path[64];
	char* detect_result;
	char* roi;
	char appType[64];
}ALARM_INFO_S;

typedef struct _Msg_inner{
	int nCmd;
	int nSeq;
	void *pMsgData;
	int nMsgLen;
	void *pMsgOutData;//内部申请
	int nMsgOutLen;
}TPluginMsg;

typedef struct MQTT_REQUEST_T
{
	int seq;
	char app[32];
	char method[64];
	char key[64];
	int datalen;
	union
	{
		int iData;
		float fData;
		char szData[100 * 1024];
		unsigned char data[100 * 1024];		
	};
}MQTT_REQUEST_S;

typedef struct MQTT_RESPONSE_T
{
	int seq;
	int code;
	char message[128];
	int datalen;
	union
	{
		int iData;
		float fData;
		char szData[100 * 1024];
		unsigned char data[100 * 1024];		
	};
}MQTT_RESPONSE_S;

#ifdef	__cplusplus
}
#endif

#endif

