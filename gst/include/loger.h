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
 
#ifndef     __LOGER_H__
#define     __LOGER_H__

#include "zlog.h"

#define		LOG_CONFIG_FILE			("zlog.conf")
#define		LOG_TAG					("oal")

#ifdef __cplusplus
class CLoger
{
public:
	static CLoger *getInstance()
	{
		static CLoger *obj = NULL;
		if (NULL == obj)
		{
			obj = new CLoger();
			obj->init(LOG_CONFIG_FILE, LOG_TAG);
		}
		return obj;
	}      
    CLoger();
    ~CLoger();  
    int init(const char *path, const char *category);
    void deinit(void);
    void error(const char *file, const char *func, int line, const char *fmt, va_list ap);	
    void warn(const char *file, const char *func, int line, const char *fmt, va_list ap);
    void info(const char *file, const char *func, int line, const char *fmt, va_list ap);
    void debug(const char *file, const char *func, int line, const char *fmt, va_list ap); 

private:
    zlog_category_t *mCategory;
};
#endif

#ifdef	__cplusplus
extern "C" { 
#endif

int loger_init(const char *path, const char *category);
void loger_deinit(void);
void loger_error(const char *file, const char *func, int line, const char *fmt, ...);
void loger_warn(const char *file, const char *func, int line, const char *fmt, ...);
void loger_info(const char *file, const char *func, int line, const char *fmt, ...);
void loger_debug(const char *file, const char *func, int line, const char *fmt, ...);

#define	LOG_DEBUG(fmt, arg...)		loger_debug(__FILE__, __func__, __LINE__, fmt, ##arg)
#define	LOG_INFO(fmt, arg...)		loger_info(__FILE__, __func__, __LINE__, fmt, ##arg)
#define	LOG_WARN(fmt, arg...)		loger_warn(__FILE__, __func__, __LINE__, fmt, ##arg)
#define	LOG_ERROR(fmt, arg...)		loger_error(__FILE__, __func__, __LINE__, fmt, ##arg)
#define	LOG_ERR(fmt, arg...)		loger_error(__FILE__, __func__, __LINE__, fmt, ##arg)

#ifdef	__cplusplus
}
#endif

#endif


