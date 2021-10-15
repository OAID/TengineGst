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
 
#ifndef __AMLOGIC_RETINAFACE_MULTI_DETECT_ALGO_H__
#define __AMLOGIC_RETINAFACE_MULTI_DETECT_ALGO_H__

#include <string>
#include <vector>
#include <iomanip>
typedef struct abox {
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    int class_id;
} abox;

struct AmlogicRetinaParams
{
    float detectThreshold{0.5};             //检测阈值
    float nmsThreshold{0.4};                //nms合并框时的iou阈值
    bool concat_feat{false};                       //模型中是否合并计算分支
    bool with_sigmoid{false};

    std::string inputTensorName;
    std::vector<std::string> outputTensorNames;
    // std::vector<int> output_zps;
    // std::vector<float> output_scales;
    std::vector<int> input_shape{3, 544, 960};

    int anchor_num;                         //每个点的anchor数量
    std::vector<float> ratios;              //生成anchor的长宽比变换
    std::vector<float> base_sizes;          //生成anchor的basesize
    std::vector<std::vector<float>> scales; //生成anchor的尺度变化
    std::vector<int> strides;               //检测分支的stride
};


/*
    初始化tengine运行环境
*/
int  AMLOGIC_Load_Init_Tengine();

/* 
    加载模型
输入：
    pcModelName：模型路径
    params:     构建网络的参数
输出：
    model_handle：用于调用网络模型的句柄
    blob_shape：  网络输入的形状
*/
int  AMLOGIC_Load_Retinaface_Model(char *pcModelName, const AmlogicRetinaParams& params, void** model_handle, int* blob_shape);

/* 
    运行网络前向获取结果
输入：
    data：        图像数据指针（图像数据为c、h、w排列顺序）
    im_scale_h：      图像高度缩放比例
    im_scale_w：      图像宽度缩放比例
    model_handle：用于调用网络模型的句柄
输出：
    result：     检测得到的结果，得到的结果直接是原图上的位置
*/

int AMLOGIC_Retinaface_Forward_Buffer(void *data, float im_scale_h, float im_scale_w, void* model_handle, std::vector<abox>& result);

// void SVP_NNIE_Retina_Forward_Buffer_split(void *data, float im_scale_h, float im_scale_w, HI_VOID* model_handle, std::vector<abox>& result);


/* 
    设置检测阈值
输入：
    model_handle：用于调用网络模型的句柄
    threshold:    检测算法阈值
*/

void AMLOGIC_Set_Detect_Threshold(void* model_handle, float threshold);

/* 
    释放网络资源
    输入：
        model_handle：  用于调用网络模型的句柄
*/
void AMLOGIC_Release_Retinaface_Model(void* model_handle);

#endif /* __SAMPLE_ALGO_H__ */