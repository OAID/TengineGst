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
 
#ifndef		__DEBUG_H__
#define		__DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef		DEBUG_ON
#define		DEBUG_ON        (1)
#endif

#ifndef		TARGET_LEVEL
#define		TARGET_LEVEL	LEVEL_DEBUG
#endif

enum DEBUG_LEVEL
{
	LEVEL_ERROR,
	LEVEL_WARN,
	LEVEL_INFO,
	LEVEL_DEBUG,
	LEVEL_MAX,
};

static char debug_str[LEVEL_MAX][20] =
{
	"error",
	"warn",
	"info",
	"debug",
};

static char *get_filename(const char *dir)
{
	char *pcStr = NULL, *pcRet = NULL;

	pcStr = strrchr((char *)dir, '/') + 1;
	if (pcStr)
		pcRet = strdup(pcStr);
	else
		pcRet = NULL;

	return pcRet;
}

#if DEBUG_ON	
#define		LOG_OUT(level, arg...)										\
				do{														\
					if (level <= TARGET_LEVEL)							\
					{													\
						char *pcStr = get_filename((char *)__FILE__);	\
																		\
						printf("[%s-%s-%d]\t", debug_str[level], pcStr, __LINE__);		\
						printf(arg);									\
						printf("\n");									\
						free(pcStr);									\
					}													\
				}while(0)	
#else
#define		LOG_OUT(level, ...)
#endif

#ifndef		LOG_DEBUG
#define		LOG_DEBUG(arg...)		LOG_OUT(LEVEL_DEBUG, ##arg)
#define		LOG_INFO(arg...)		LOG_OUT(LEVEL_INFO, ##arg)
#define		LOG_WARN(arg...)		LOG_OUT(LEVEL_WARN, ##arg)
#define		LOG_ERROR(arg...)		LOG_OUT(LEVEL_ERROR, ##arg)
#endif

#ifdef	__cplusplus
}
#endif


#endif
