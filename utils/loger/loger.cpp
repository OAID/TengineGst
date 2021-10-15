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
 
#include "loger.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

CLoger::CLoger()
{

}

CLoger::~CLoger()
{

}

static char s_DefaultConf[] = "[formats]\
	LogFormat = \"%d(%m-%d %T) %-5V [%c:%F:%-5L] %m%n\".\
	[rules]\
	*.*    >stdout; LogFormat\
	";
	
int CLoger::init(const char *path, const char *category)
{
	int ret;

	ret = zlog_init(path);
	if (ret < 0)
	{
		char confPath[128] = {0};
		sprintf(confPath,"%s/zlog.conf",getenv("HOME"));
		FILE *pFile = fopen(confPath,"wb");
		if(pFile)
		{
			fwrite(s_DefaultConf,strlen(s_DefaultConf),1,pFile);
			fclose(pFile);
		}
		ret = zlog_init(confPath);
		if (ret < 0){
			printf("[%d_%s] zlog_init home=%s fail[%s]\n", __LINE__, __func__, getenv("HOME"), confPath);
			return -1;
		}
	}
	mCategory = zlog_get_category(category);
	if (NULL == mCategory)
	{
		printf("[%d_%s] get category[%s] fail\n",  __LINE__, __func__, category);
		zlog_fini();
		return -1;
	}
	return 0;
}

void CLoger::deinit(void)
{
	zlog_fini();
}

void CLoger::error(const char *file, const char *func, int line, const char *fmt, va_list ap)
{
	char *name = basename((char *)file);
	return vzlog(mCategory, name, strlen(name), func, strlen(func), line, ZLOG_LEVEL_ERROR, fmt, ap);
}

void CLoger::warn(const char *file, const char *func, int line, const char *fmt, va_list ap)
{
	char *name = basename((char *)file);
	return vzlog(mCategory, name, strlen(name), func, strlen(func), line, ZLOG_LEVEL_WARN, fmt, ap);
}

void CLoger::info(const char *file, const char *func, int line, const char *fmt, va_list ap)
{
	char *name = basename((char *)file);
	return vzlog(mCategory, name, strlen(name), func, strlen(func), line, ZLOG_LEVEL_INFO, fmt, ap);
}

void CLoger::debug(const char *file, const char *func, int line, const char *fmt, va_list ap)
{
	char *name = basename((char *)file);
	return vzlog(mCategory, name, strlen(name), func, strlen(func), line, ZLOG_LEVEL_DEBUG, fmt, ap);
}

int loger_init(const char *path, const char *category)
{
	return CLoger::getInstance()->init(path, category);
}

void loger_deinit(void)
{
	return CLoger::getInstance()->deinit();
}

void loger_error(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt); 
	CLoger::getInstance()->error(file, func, line, fmt, ap);
	va_end(ap);
}

void loger_warn(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt); 
	CLoger::getInstance()->warn(file, func, line, fmt, ap);
	va_end(ap); 
}

void loger_info(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt); 
	CLoger::getInstance()->info(file, func, line, fmt, ap);
	va_end(ap); 
}

void loger_debug(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt); 
	CLoger::getInstance()->debug(file, func, line, fmt, ap);
	va_end(ap); 
}


