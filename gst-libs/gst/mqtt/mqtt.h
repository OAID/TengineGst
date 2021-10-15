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
 
#ifndef		__MQTT_H__
#define		__MQTT_H__

#include "mosquitto.h"
#include <pthread.h>

#ifdef	__cplusplus
extern "C" {
#endif	

#ifdef	__cplusplus
class CMqtt
{
public:
	static CMqtt *getInstance()
	{
		static CMqtt *obj = NULL;
		if (NULL == obj)
			obj = new CMqtt();
		return obj;
	} 	
	CMqtt();
	~CMqtt();
	int Init(const char *tsName,const char *tsPwd,const char *broker,int nServPort);
	void Deinit(void);
	int Publish(const char *topic, const char *message);

public:
	int mRunning;
	pthread_t mThreadId;
	struct mosquitto *mpMqtt;
};
#endif

int MQTT_init(const char *tsName,const char *tsPwd,const char *broker,int nServPort);
void MQTT_deinit(void);
int MQTT_publish(const char *topic, const char *message);

#ifdef	__cplusplus
}
#endif	

#endif

