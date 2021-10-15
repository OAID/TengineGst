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
 
#ifndef     __NORMAL_H__
#define     __NORMAL_H__

#include <stdint.h>
#include <vector>

#include "common.h"
#include "json/json.h"

using namespace std;

#ifdef	__cplusplus
extern "C" { 
#endif

/* local formate: 2014/11/22 12:24:20 */
char *showtime(char *timestr);
int time_second(void);
long long get_timestamp_ext(void);
long long get_timestamp(void);
long long get_timestamp_s(void);
int path_exist(const char *path);
int file_size(const char *path);
int run_daemon(int nochdir, int noclose);
void dump_data(const char *flag, unsigned char *data, int len);
int write_data_to_file(const char *file, char *data, int len);
int read_data_from_file(const char *file, char *data, int *len);
int copy_file(char *src, char *dst);
int checksum(char *file);
int add_checksum(char *file);
void copy_to_backfile(char *file);
int json_to_bbox(char *json, vector<BBOX_S> &boxes);
int bbox_to_json(vector<BBOX_S> boxes, char *json);
int loadcfg2json(const char *tsFilePath, Json::Value &root);
int savejson2cfg(const char *tsFilePath, Json::Value &root);
int save_image(const char *path, unsigned char *data, int len);
int rm_dir(const char *dir_full_path);
#ifdef AARCH_LINUX
void *arm_memcpy(void *dest,const void *src, size_t count);
#endif
#ifdef LINUX_GNU
pid_t gettid(void);
#endif

#ifdef	__cplusplus
} 
#endif

#endif

