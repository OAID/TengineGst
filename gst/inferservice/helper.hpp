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
 
 
#ifndef __HELPER_HPP_20210122__
#define __HELPER_HPP_20210122__

#include <assert.h>
#include <chrono>

#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>

#include <time.h>
#include <sys/time.h>

// #define AUTHORITY
#ifdef AUTHORITY 
#include "ep_auth.h"
#endif

namespace OAL_ALG_EDGE{
#ifdef AUTHORITY 
  static int do_checkauth(std::string authFile);
#endif

  static double __tic = 0.0;                     
  static double __toc = 0.0;  
  void TIC(void)
  {
      struct timeval time_v = {0};
      gettimeofday(&time_v, NULL);
      __tic = (double)(time_v.tv_sec * 1e6 + time_v.tv_usec);
  }

  void TOC(const char* func_name)
  {
      struct timeval time_v = {0};
      gettimeofday(&time_v, NULL);
      __toc = (double)(time_v.tv_sec * 1e6 + time_v.tv_usec);
      printf("%s take %.2f ms\n", func_name, (__toc - __tic) / 1000.0);
  }

  static void trim(std::string srcs, std::string &dsts){
    dsts = srcs;
    dsts.erase(0, srcs.find_first_not_of(" "));
    dsts.erase(dsts.find_last_not_of(" ") + 1);
  }

  static std::string join_path(std::string dir, std::string file){
    std::string full_file = "";
    
    int nLen = dir.length();
    if(dir[nLen-1] == '/'){
      full_file = dir + file;
    }else{
      full_file = dir + std::string("/") + file;
    }
    return full_file;
  }

  static int full_file_split(std::string full_file, std::string &path, std::string &file){
    int iret = -1;
    // printf("Full file:%s.\n", full_file.c_str());

    int nPos = full_file.find_last_of('/');
    if(nPos < 1){
      return iret;
    }

    int nLen = full_file.length();
    std::string tmpline = full_file.substr(nPos+1, nLen - nPos -1);
    OAL_ALG_EDGE::trim(tmpline, file);
    // printf("Tmpline=%s, File is:%s.\n", tmpline.c_str(), file.c_str());

    tmpline = full_file.substr(0, nPos);
    OAL_ALG_EDGE::trim(tmpline, path);
    // printf("Tmpline=%s, Path is:%s.\n", tmpline.c_str(), path.c_str());
    return 0;
  }


  static int read_labels(std::string label_file, std::vector<std::string> &labels){
    std::ifstream ifs(label_file);
    if(!ifs){
        printf("Open the file failed :%s.\n", label_file.c_str());
    }

    std::string tmpline;
    while(getline(ifs, tmpline)){
        std::stringstream ss(tmpline);
        std::string label;
        ss>>label;
        if(!label.empty()){
            labels.push_back(label);
        }
    }
    return 0;
  }


  static int read_img_files(std::string img_list_file, std::vector<std::string> &img_files);
  static void read_test_file(std::string test_file, std::string img_dir, std::vector<std::string> &files, std::string &path);


  // Implement
  int read_img_files(std::string img_list_file, std::vector<std::string> &img_files){
    std::ifstream ifs(img_list_file);
    if(!ifs){
      printf("Cannot open the image list file:%s.\n", img_list_file.c_str());
      return -1;
    }

    std::string line;
    while(ifs>>line){
      std::string file;
      OAL_ALG_EDGE::trim(line, file);
      if(!file.empty()){
        img_files.push_back(file);
      }
    }
    return 0;
  }
  

  void read_test_file(std::string test_file, std::string img_dir, std::vector<std::string> &files, std::string &path){
    std::ifstream ifs(test_file);
    if(!ifs.is_open()){
      printf("Open test file failed: %s.\n", test_file.c_str());
      return;
    }

    std::string line;
    while(getline(ifs, line)){
      if(!line.empty()){
        if(line[0] == '#'){
          std::string tmp_path;  std::string img_file;
          full_file_split(line, tmp_path, img_file);
          path = tmp_path.substr(2, tmp_path.length()-2);

          std::string full_file = join_path(img_dir, img_file);
          files.push_back(full_file);
          printf("%s.\n", full_file.c_str());
        }
      }
    }
  }

#ifdef AUTHORITY
  static int do_checkauth(std::string authFile){
    char szContractID[64] = {0};
    char szContractPWD[64] = {0};

    FILE *fp = fopen(authFile.c_str(), "r");
    int nRead = fscanf(fp, "%s", szContractID);
    if(nRead < 1){
        printf("Read contract ID failed.\n");
        return -1;
    }
    nRead = fscanf(fp, "%s", szContractPWD);
    if(nRead < 1){
        printf("Read contract pwd failed.\n");
        return -1;
    }
    fclose(fp);

    uint8_t uContractIDArr[16] = {0};
    uint8_t uContractPWDArr[16] = {0};

    for(int i = 0; i < 16; i++){
        char szCharID[2] = {0};
        char szCharPWD[2] = {0};
        memcpy(szCharID,  szContractID + (i * 2), 2);
        memcpy(szCharPWD, szContractPWD + (i * 2), 2);
                
        uContractIDArr[i] = strtol(szCharID, NULL, 16);
        uContractPWDArr[i] = strtol(szCharPWD, NULL, 16);
        printf("AUTH %x %x\n", uContractIDArr[i], uContractPWDArr[i]);
    }

    int status = EaisAuth((char* )uContractIDArr, (char* )uContractPWDArr);//==0?1:0;
    printf("\r\nAuth uContractIDArr:%x, uContractPWDArr:%x, auth=%d.\r\n",\
        uContractIDArr[0], uContractPWDArr[0], status);
    
    return status;
  }
#endif


}
#endif