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
 
#include "normal.h"
#include "cJSON.h"
#include "debug.h"
#include "md5.h"

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <sys/syscall.h>
#ifdef AARCH_LINUX
#include <arm_neon.h>
#endif

/* local formate: 2014/11/22 12:24:20 */
char *showtime(char *timestr)
{
	time_t now;	
	struct tm timenow; 

	now = time(NULL);	
	localtime_r(&now, &timenow); 
	sprintf(timestr, "%04d-%02d-%02d %02d:%02d:%02d", timenow.tm_year + 1900, 
											  	   timenow.tm_mon + 1, 
											       timenow.tm_mday, 
											       timenow.tm_hour, 
											       timenow.tm_min, 
											       timenow.tm_sec);
	return timestr;
}

int time_second(void)
{
	time_t now;	
	struct tm timenow; 

	now = time(NULL);

	return (int)now;
}

long long get_timestamp_ext(void)
{
    long long msecond;
    struct timespec current = {0, 0}; 

    clock_gettime(CLOCK_MONOTONIC, &current);
    msecond = current.tv_sec * 1000 + current.tv_nsec / 1000000;

    return msecond;
}

long long get_timestamp(void)
{
	long long timestamp;
	struct timeval timenow; 

	gettimeofday(&timenow, NULL);
	timestamp = (long long)timenow.tv_sec * 1000 * 1000 + (long long)timenow.tv_usec;

	return timestamp;
}

long long get_timestamp_s(void)
{
	long long timestamp;
	struct timeval timenow; 

	gettimeofday(&timenow, NULL);
	timestamp = (long long)timenow.tv_sec;

	return timestamp;
}

int path_exist(const char *path)
{
    if (access(path, F_OK) == 0)
        return 0;
    else
        return -1;
}

int file_size(const char *path)
{
	struct stat buf;

	if (access(path, F_OK) < 0)
	{
		LOG_ERROR("file [%s] not exist", path);
		return -1;
	}
	stat(path, &buf);
	return buf.st_size;
}

int run_daemon(int nochdir, int noclose)
{
	int fd;

	switch (fork()) 
	{
		case -1:
			return (-1);
		case 0:
			break;
		default:
			_exit(0);
	}

	if (setsid() == -1)
		return (-1);

	if (!nochdir)
		chdir("/");

	if (!noclose && (fd = open("/dev/console", O_RDWR, 0)) != -1) 
	{
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > 2)
			close (fd);
	}
	
	return (0);
}

void dump_data(const char *flag, unsigned char *data, int len)
{
    int index = 0;
    
    printf("[%d_%s]dump_data[%s]:\n", __LINE__, __func__, flag);
    while (len--)
    {
        printf("0x%02x ", data[index++]);
        if (index%16 == 0)
            printf("\n");
    }
    printf("\n");	
}

const char bb_hexdigits_upcase[] = "0123456789ABCDEF";
char *bin2hex(char *p, const unsigned char *cp, int count)
{
	while (count) 
	{
		unsigned char c = *cp++;
		/* put lowercase hex digits */
		*p++ = 0x20 | bb_hexdigits_upcase[c >> 4];
		*p++ = 0x20 | bb_hexdigits_upcase[c & 0xf];
		count--;
	}
	return p;
}

int write_data_to_file(const char *file, char *data, int len)
{
	int fd, ret;
	
	POINT_CHECK_RETURN_INT(file);
	POINT_CHECK_RETURN_INT(data);

	if ((fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0)	
	{
		printf("open file[%s] error[%d]\n", file, errno);
		return -1;
	}
	ret = write(fd, data, len);
	if (ret != len)
	{
		printf("write data error: %d\n", ret);
		close(fd);
		return -1;
	}
	close(fd);
	
	return 0;
}

int read_data_from_file(const char *file, char *data, int *len)
{
    int fd, ret, index = 0;

	POINT_CHECK_RETURN_INT(file);
	POINT_CHECK_RETURN_INT(data);
	POINT_CHECK_RETURN_INT(len);

	fd = open(file, O_RDONLY, 0666);
	if (fd < 0)	
	{
		printf("open file[%s] error[%d]\n", file, errno);
		return -1;
	} 
    while (1)
    {
        ret = read(fd, data + index, 1024);
        if (ret == 0)    /* 到达文件末尾 */
            break;
		else if (ret < 0)
		{
			LOG_ERROR("read fail: %d_%d_%s", errno, fd, file);
			break;
		}
        index += ret;
    }
    close(fd);
    *len = index;

    return 0;
}

int copy_file(char *src, char *dst)
{
	int src_fd, dst_fd, ret;
	char buf[1024] = {0};
	
	POINT_CHECK_RETURN_INT(src);
	POINT_CHECK_RETURN_INT(dst);	

	LOG_INFO("copy file: %s", src);

	src_fd = open(src, O_RDONLY);
	if (src_fd < 0)
	{
		LOG_ERROR("open[%s] fail:%s", src, strerror(errno));
		return -1;
	}

	LOG_INFO("dst file: %s", dst);
	dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (dst_fd < 0)
	{
		LOG_ERROR("open[%s] fail:%s", dst, strerror(errno));
		return -1;
	}
	while (1)
	{
		ret = read(src_fd, buf, sizeof(buf));
		if (ret == 0)
			break;
		write(dst_fd, buf, ret);
	}
	close(src_fd);
	close(dst_fd);

	return 0;
}

int checksum(char *file)
{
    int fd = -1;
    char read_value[CHECKSUM_LEN + 1] = {0};
    unsigned char real_value_bin[16] = {0};
    char real_value[CHECKSUM_LEN + 1] = {0};   
    struct stat statbuff;

	POINT_CHECK_RETURN_INT(file);

    if (copy_file(file, (char *)CHECKFILE_PATH) != 0)
    {
        LOG_ERROR("copy_file[%s] fail", file);
        return -1;        
    }
    fd = open(CHECKFILE_PATH, O_RDWR);
    if (fd < 0)
    {
		LOG_ERROR("open[%s] fail:%s", CHECKFILE_PATH, strerror(errno));
        return -1;
    }
    lseek(fd, -(CHECKSUM_LEN + 1), SEEK_END);   /* include a enter */
    read(fd, read_value, CHECKSUM_LEN);
    /* get file size */
    stat(CHECKFILE_PATH, &statbuff);
    /* cut of file */
    ftruncate(fd, statbuff.st_size - CHECKSUM_STRING_LEN - 1);   /* include a enter */
    close(fd);

    md5_file(CHECKFILE_PATH, real_value_bin);
    unlink(CHECKFILE_PATH);
    bin2hex(real_value, real_value_bin, sizeof(real_value_bin));   
	if (strcmp(read_value, real_value))
		return -1;
	else
		return 0;
}

int add_checksum(char *file)
{
    int fd;
    unsigned char md5_bin[16] = {0};
    char md5[CHECKSUM_LEN + 1] = {0}, str[128] = {0}; 
    
    md5_file(file, md5_bin);   
    bin2hex(md5, md5_bin, sizeof(md5_bin));
    fd = open(file, O_RDWR | O_APPEND);
    if (fd < 0)
    {
        LOG_ERROR("open[%s] fail[%d]", file, errno);
        return -1;
    }
    sprintf(str, "checksum = %s\n", md5);
    write(fd, str, strlen(str));
    close(fd);

    return 0;
}

void copy_to_backfile(char *file)
{
	char back_file[128] = {0};

	sprintf(back_file, "%s_bak", file);
	copy_file(file, back_file);
}

int json_get_intval(cJSON *jParent,char *jName,int *nVal) {
	cJSON *jVal = NULL;
	jVal = cJSON_GetObjectItem(jParent, jName);
	if(jVal)
		*nVal = jVal->valueint;
	return 0;
}
int json_get_floatval(cJSON *jParent, char *jName, float *fVal) {
	cJSON *jVal = NULL;
	jVal = cJSON_GetObjectItem(jParent, jName);
	if (jVal)
		*fVal = jVal->valuedouble;
	return 0;
}
int json_get_stringval(cJSON* jParent, char* jName, char* strVal){
	cJSON* jVal = NULL;
	jVal = cJSON_GetObjectItem(jParent, jName);
	if(jVal)
	{
		char* tmp = jVal->valuestring;
		memcpy(strVal, tmp, strlen(tmp)+1);
	}
	return 0;
}
/*
{"boxes":[{x1,y1,width1,heigh1},{x2,y2,width2,heigh2}]}
*/
int json_to_bbox(char *json, vector<BBOX_S> &boxes)
{
	int size;
	cJSON *root, *jBoxes, *jBox;
	cJSON *jX, *jY, *jWidth, *jHeigh, *jClr, *jLabel, *jScore;

	POINT_CHECK_RETURN_INT(json);

	root = cJSON_Parse(json);
	if (NULL == root) 
	{
		//LOG_INFO("cJSON_Parse fail: %s\n", json);
		return -1;
	}
	jBoxes = cJSON_GetObjectItem(root, "boxes");
	if (NULL == jBoxes)
	{
		//LOG_INFO("parse boxes fail: %s\n", json);	/* somebug */
		return -1;
	}
	size = cJSON_GetArraySize(jBoxes);
	for (int i = 0; i < size; i++)
	{
		BBOX_S box;
		
		jBox = cJSON_GetArrayItem(jBoxes, i);
		json_get_floatval(jBox, (char *)"x", &box.x);
		json_get_floatval(jBox, (char *)"y", &box.y);
		json_get_floatval(jBox, (char *)"width", &box.width);
		json_get_floatval(jBox, (char *)"heigh", &box.heigh);
		json_get_intval(jBox, (char *)"color", (int*)&box.uColor);
		json_get_intval(jBox, (char *)"label", &box.label);
		json_get_floatval(jBox, (char *)"score", &box.score);
		json_get_stringval(jBox, (char*)"rect_info", box.rect_info);
		boxes.push_back(box);
	}
	cJSON_Delete(root);

	return 0;
}

/*
{"boxes":[{x1,y1,width1,height1},{x2,y2,width2,height2}]}
*/
int bbox_to_json(vector<BBOX_S> boxes, char *json)
{
	cJSON *root, *jBoxes, *jBox;
	char *pbox;

	root = cJSON_CreateObject();
	jBoxes = cJSON_CreateArray();	
	for (int i = 0; i < (int)boxes.size(); i++)
	{
		jBox = cJSON_CreateObject();
		cJSON_AddNumberToObject(jBox, "x", boxes[i].x);
		cJSON_AddNumberToObject(jBox, "y", boxes[i].y);
		cJSON_AddNumberToObject(jBox, "width", boxes[i].width);
		cJSON_AddNumberToObject(jBox, "heigh", boxes[i].heigh);
		cJSON_AddNumberToObject(jBox, "color", boxes[i].uColor);
		cJSON_AddNumberToObject(jBox, "label", boxes[i].label);
		cJSON_AddNumberToObject(jBox, "score", boxes[i].score);
		cJSON_AddStringToObject(jBox, "rect_info", boxes[i].rect_info);
		cJSON_AddItemToArray(jBoxes, jBox);
	}
	cJSON_AddItemToObject(root, "boxes", jBoxes);
	pbox = cJSON_PrintUnformatted(root);
	strcpy(json, pbox);
	free(pbox);				/* 坑 */
	cJSON_Delete(root);

	return 0;
}

int loadcfg2json(const char *tsFilePath, Json::Value &root)
{
    Json::Reader reader;

    ifstream in(tsFilePath, ios::binary);
    if (!in.is_open())
    {
		LOG_INFO("load json file[%s] fail", tsFilePath);
        return -1;
    }
    if (!reader.parse(in, root))
    {
		LOG_INFO("parse json fail");
        return -1;
    }
    return 0;
}

int savejson2cfg(const char *tsFilePath, Json::Value &root)
{
    ofstream osDev;
    Json::StyledWriter sw;	
	std::string jsDevCfg;
	
    osDev.open(tsFilePath);
    jsDevCfg = sw.write(root);
    osDev << jsDevCfg;
    osDev.close();
	
    return 0;
}

int save_image(const char *path, unsigned char *data, int len)
{
	int ret;
	FILE *fp = NULL;

	fp = fopen(path, "w+");
	if (NULL == fp)
	{
		LOG_ERROR("fopen[%s] fail[%d]\n", path, errno);
		return -1;
	}
	ret = fwrite(data, len, 1, fp);
	if (ret < 0)
	{
		LOG_ERROR("fwrite fail[%d]\n", errno);
		return -1;
	}
	fclose(fp);

	return 0;
}

int rm_dir(const char *dir_full_path)
{
    DIR* dirp = NULL;
    struct dirent *dir;
    struct stat st;
	char sub_path[128] = {0};

    dirp = opendir(dir_full_path);    
    if (NULL == dirp)
    {
    	LOG_ERROR("cannot find dir: %s", dir_full_path);
        return -1;
    }
    while((dir = readdir(dirp)) != NULL)
    {
        if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0))
        {
            continue;
        }    
		sprintf(sub_path, "%s/%s", dir_full_path, dir->d_name);
        if (lstat(sub_path, &st) == -1)
        {
    		LOG_WARN("lstat subdir fail: %s", sub_path);			
            continue;
        }    
        if (S_ISDIR(st.st_mode))
        {
            if (rm_dir(sub_path) == -1)		/* 如果是目录,递归删除 */
            {
                closedir(dirp);
                return -1;
            }
            rmdir(sub_path);
        }
        else if (S_ISREG(st.st_mode))
        {
            unlink(sub_path);     	/* 普通文件直接删除 */
        }
    }
    if (rmdir(dir_full_path) == -1)	/* 删除目录本身 */
    {
        closedir(dirp);
        return -1;
    }
    closedir(dirp);
    return 0;
}

#ifdef AARCH_LINUX
void *arm_memcpy(void *dest, const void *src, size_t count)
{        
	int i;        
	unsigned long *s = (unsigned long *)src;        
	unsigned long *d = (unsigned long *)dest;        

	for (i = 0; i < count / 64; i++) 
	{                
		vst1q_u64(&d[0], vld1q_u64(&s[0]));                 
		vst1q_u64(&d[2], vld1q_u64(&s[2]));                 
		vst1q_u64(&d[4], vld1q_u64(&s[4]));                 
		vst1q_u64(&d[6], vld1q_u64(&s[6]));                
		d += 8; s += 8;        
	}        
	return dest;
}
#endif
#ifdef LINUX_GNU
pid_t gettid(void)
{
	//return syscall(__NR_gettid);
	return syscall(SYS_gettid);
}
#endif
