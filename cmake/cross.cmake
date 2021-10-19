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

SET(CROSS_COMPILE 0)
SET(USE_KHADAS_ENV 0)
		
set(GNU_FLAGS "-fPIC")
set(CMAKE_CXX_FLAGS "${GNU_FLAGS} ")
set(CMAKE_C_FLAGS "${GNU_FLAGS}  ")

IF(CROSS_COMPILE) 	  
	SET(CMAKE_SYSTEM_NAME Linux)
	SET(TOOLCHAIN_DIR "/mnt/d/win-linux/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu")

	set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gnu-g++)
	set(CMAKE_C_COMPILER   ${TOOLCHAIN_DIR}/bin/aarch64-linux-gnu-gcc)
#	set(GNU_FLAGS "-mfpu=vfp -fPIC")

	SET(CMAKE_FIND_ROOT_PATH  ${TOOLCHAIN_DIR}
		 ${TOOLCHAIN_DIR}/aarch64-linux-gnu/include
		  ${TOOLCHAIN_DIR}/aarch64-linux-gnu/lib )
			
  ENDIF(CROSS_COMPILE)

  IF(USE_KHADAS_ENV) 
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gstreamer/include)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gstreamer/lib)

  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glib/include)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glib/lib)

  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/corecv/lib)

  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tengine/include)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tengine/lib)

  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zlog/lib)

  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/algo/include)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/algo/lib)

  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mosquitto/include)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mosquitto/lib)

  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libffi/lib64)	
  link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pcre/lib)	 
  ENDIF(USE_KHADAS_ENV)
