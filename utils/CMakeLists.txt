# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# License); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# Copyright (c) 2021, OPEN AI LAB
# Author: wlwu@openailab.com
#

cmake_minimum_required (VERSION 3.1)

set (TARGET_NAME "utils")

file (GLOB MAIN_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    #av_buffer/*.cpp
    base64/*.c
    cjson/*.c
    json/*.cpp
    hash/*.cpp
    http_client/*.c
    loger/*.cpp
    md5/*.c
    mempool/*.cpp
)

#file (GLOB MAIN_HEADERS
#    av_buffer/*.h
#    base64/*.h
#    cjson/*.h
#    hash/*.h
#    http_client/*.h
#    loger/*.h
#    md5/*.h
#    memcpy/*.h
#    mempool/*.h
#)

add_library(${TARGET_NAME} STATIC ${MAIN_SRC} ) #${MAIN_HEADERS})
set_compile_flags(${TARGET_NAME})

target_include_directories(${TARGET_NAME}
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../gst/include
)

target_link_libraries(${TARGET_NAME}
PUBLIC
#    -Wl,-Bdynamic 
    -pthread 
#PRIVATE
    zlog
)
