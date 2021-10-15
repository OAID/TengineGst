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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mqtt.h"
#include "common.h"

void on_mqtt_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	ENTER_FUNCTION();
}

void on_mqtt_connect(struct mosquitto *mosq, void *userdata, int rc)
{
	ENTER_FUNCTION();
}

void on_mqtt_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
	ENTER_FUNCTION();
}

void on_mqtt_subscribe(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	ENTER_FUNCTION();
}

void *mqtt_process(void *pArg)
{
	CMqtt *pMqtt = (CMqtt *)pArg;

	while (pMqtt->mRunning)
	{
		mosquitto_loop(pMqtt->mpMqtt, -1, 1);
	}
	return NULL;
} 


CMqtt::CMqtt()
{
	mpMqtt = NULL;
}

CMqtt::~CMqtt()
{

}

int CMqtt::Init(const char *tsName,const char *tsPwd,const char *broker,int nServPort)
{
	int ret;
	char mqttid[128] = {"plugin"};

	mosquitto_lib_init();

	/* recv session */
	mpMqtt = mosquitto_new(mqttid, true, NULL);
	if (NULL == mpMqtt)
	{
		LOG_ERROR("mosquitto_new fail");
		return -1;
	}
	mosquitto_connect_callback_set(mpMqtt, on_mqtt_connect);
	mosquitto_disconnect_callback_set(mpMqtt, on_mqtt_disconnect);	
	mosquitto_subscribe_callback_set(mpMqtt, on_mqtt_subscribe);
	mosquitto_message_callback_set(mpMqtt, on_mqtt_message);
	ret = mosquitto_connect(mpMqtt, broker, nServPort, 60);//1883
	if (ret != MOSQ_ERR_SUCCESS)
	{
		LOG_ERROR("mosquitto_connect fail[%d]", ret);
		return -1;
	}
	mRunning = 1;
	pthread_create(&mThreadId, NULL, mqtt_process, this);
	return 0;
}

void CMqtt::Deinit(void)
{
	mRunning = 0;
	pthread_join(mThreadId, NULL);
}

int CMqtt::Publish(const char *topic, const char *message)
{
	if(mpMqtt){
		int ret;

		LOG_INFO("public event");
		
		ret = mosquitto_publish(mpMqtt, NULL, topic, strlen(message), message, 0, 0);
		if (ret < 0)
		{
			LOG_ERROR("mosquitto_publish fail");
			return -1;
		}
	}
	else
		return -2;
	return 0;
}

int MQTT_init(const char *tsName,const char *tsPwd,const char *broker,int nServPort)
{
	return CMqtt::getInstance()->Init(tsName,tsPwd,broker,nServPort);
}

void MQTT_deinit(void)
{
	return CMqtt::getInstance()->Deinit();
}

int MQTT_publish(const char *topic, const char *message)
{
	return CMqtt::getInstance()->Publish(topic, message);
}

