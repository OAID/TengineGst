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

project(Tengine_Streamer_Plugins)

include(cmake/cross.cmake)

if(NOT(UNIX))
    message(FATAL_ERROR "Only UNIX supported")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE is undefined. Set default build type ${CMAKE_BUILD_TYPE}.")
endif()

#/usr/bin/c++
MESSAGE(STATUS "GNUCC ${CMAKE_CXX_COMPILER}") 
MESSAGE(STATUS "Project Directory: ${PROJECT_SOURCE_DIR}")

option(TREAT_WARNING_AS_ERROR "Treat build warnings as errors" OFF)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)

if (NOT DEFINED VERSION_PATCH OR "${VERSION_PATCH}" STREQUAL "")
    set(VERSION_PATCH 0)
    message(WARNING "VERSION_PATCH is undefined. Set default value ${VERSION_PATCH}.")
endif()

if (NOT DEFINED GIT_INFO OR "${GIT_INFO}" STREQUAL "")
    set(GIT_INFO "0")
    message(WARNING "GIT_INFO is undefined. Set default value ${GIT_INFO}.")
endif()

configure_file(cmake/version.txt.in version.txt)

set(PRODUCT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(PLUGIN_VERSION ${PRODUCT_VERSION}.${GIT_INFO})
# Propagate version to plugins
add_definitions(-DPLUGIN_VERSION="${PLUGIN_VERSION}")
add_definitions(-DPACKAGE="gst-video-analyticsin")
add_definitions(-DPACKAGE_NAME="GStreamer Video Analytics elements")
add_definitions(-DGST_PACKAGE_ORIGIN="https://github.com/OAID/TengineStreamer")
add_definitions(-DPLUGIN_LICENSE="MIT/X11")   #Apache-2.0") 

macro(find_IE_package)
    set(IE_MIN_VERSION "2021.10.0")
    find_package(InferenceEngine REQUIRED)
    if (${InferenceEngine_VERSION} VERSION_LESS ${IE_MIN_VERSION})
        message(FATAL_ERROR "InferenceEngine version should be >= ${IE_MIN_VERSION}. Found: ${InferenceEngine_VERSION}")
    endif()
endmacro(find_IE_package)

macro(set_target_lib_version TARGET)
    set_target_properties(
        ${TARGET}
        PROPERTIES
        SOVERSION ${VERSION_MAJOR}
        VERSION ${PRODUCT_VERSION}
    )
endmacro(set_target_lib_version)


# 用于递归添加路径下的头文件路径
function(include_sub_directories_recursively root_dir)
    if (IS_DIRECTORY ${root_dir})               # 当前路径是一个目录吗，是的话就加入到包含目录
        include_directories(${root_dir})
    endif ()

    file(GLOB ALL_SUB RELATIVE ${root_dir} ${root_dir}/*) # 获得当前目录下的所有文件，让如ALL_SUB列表中
    foreach (sub ${ALL_SUB})
        if (IS_DIRECTORY ${root_dir}/${sub})
            include_sub_directories_recursively(${root_dir}/${sub}) # 对子目录递归调用，包含
        endif ()
    endforeach ()
endfunction()

function(link_directories_recursively root_dir)
    if (IS_DIRECTORY ${root_dir})
        link_directories(${root_dir})
    endif()

    file(GLOB ALL_SUB RELATIVE ${root_dir} ${root_dir}/*)
    foreach (sub ${ALL_SUB})
        if (IS_DIRECTORY ${root_dir}/${sub})
            link_directories_recursively(${root_dir}/${sub})
        endif()
    endforeach()
endfunction()

if (NOT(BIN_FOLDER))
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set (ARCH aarch64)
    else()
        set (ARCH aarch32)
    endif()
    set (BIN_FOLDER ${ARCH})
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
set (CMAKE_PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib/gstreamer-1.0)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/bin)
set (CMAKE_SAMPLES_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/samples)

set (TSTREAMER_LIBRARIES_INSTALL_PATH lib)
set (TSTREAMER_PLUGINS_INSTALL_PATH ${TSTREAMER_LIBRARIES_INSTALL_PATH}/gstreamer-1.0)
set (TSTREAMER_HEADERS_INSTALL_PATH include)

# include and lib folders of this project
set (TSTREAMER_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/gst-libs)
set (TSTREAMER_LIBRARIES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

find_package(Threads REQUIRED)

# Common compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -Wuninitialized -Winit-self -Wmaybe-uninitialized -Warray-bounds -fstack-protector-strong -Wno-misleading-indentation -Wno-unused-parameter -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format-extra-args -Wno-sign-compare -Wno-missing-field-initializers -Wno-type-limits -fno-strict-overflow -fno-delete-null-pointer-checks -fwrapv")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wuninitialized -Warray-bounds -fstack-protector-strong -Wno-misleading-indentation -Wno-unused-parameter -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format-extra-args -Wno-implicit-function-declaration -Wno-sign-compare -Wno-missing-field-initializers -Wno-type-limits -fno-strict-overflow -fno-delete-null-pointer-checks -fwrapv")

if(TREAT_WARNING_AS_ERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

# Additional compilation flags aplied to specific targets
set(C_FLAGS -Wall -Wextra)
set(CXX_FLAGS -Wall -Wextra)

function (set_compile_flags TARGET)
    target_compile_options(${TARGET} PRIVATE $<$<COMPILE_LANGUAGE:C>:${C_FLAGS}> $<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>)
endfunction(set_compile_flags)

####################################
## to use C/C++11
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
####################################

add_compile_options(-fPIC)

message(STATUS "c++ ${CMAKE_CXX_FLAGS}")
message(STATUS "c ${CMAKE_C_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "-O2")

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    # TODO: test if it increases x86 performance as well and rm check for aarch64
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ftree-vectorize") # should be used with "-O2"
endif()

option(ENABLE_PAHO_INSTALLATION "Enables paho-mqtt3c installation" OFF)

option(ENABLE_SAMPLES "Parameter to enable samples building" ON)

message("ENABLE_PAHO_INSTALLATION=${ENABLE_PAHO_INSTALLATION}")
if(${ENABLE_PAHO_INSTALLATION})
    find_library(MQTT paho-mqtt3c)
endif()

configure_file(cmake/config.h.in configs/config.h @ONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/configs)

IF(USE_KHADAS_ENV)
    add_subdirectory(thirdparty)
ENDIF(USE_KHADAS_ENV)

add_subdirectory(utils)
add_subdirectory(gst)
add_subdirectory(gst-libs)

if(${ENABLE_SAMPLES})
    add_subdirectory(samples)
endif()
