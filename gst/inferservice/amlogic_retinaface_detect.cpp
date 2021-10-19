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
 
#include <time.h>
#include <sys/time.h>
#include "amlogic_retinaface_multi_detect_algo.h"
//#include "tengine_c_api.h"
#include <tengine/c_api.h>

#include <algorithm>
#include <math.h>
#include <unistd.h>
#include <iostream>

#define VXDEVICE "TIMVX"

struct box {
    float cx;
    float cy;
    float sx;
    float sy;
};


struct AMLOGIC_MODEL_RETINAFACE_DATA {
    graph_t top_graph;
    tensor_t input_tensor;
    std::vector<tensor_t> output_tensors;

    uint8_t *udata;

    int input_zp{0};
    float input_scale{1.f};

    std::vector<int> output_zps;
    std::vector<float> output_scales;

    int outblob_num;
    std::vector<int> out_strides;
    int anchor_num;
    float score_threshold;
    float nms_threshold;
    std::vector<box> concat_anchor;
    bool concat_feat{true};
    bool with_sigmoid{false};
};

static double __a311d_tic = 0.0;                     // 计时，滴
static double __a311d_toc = 0.0;                     // 计时，答
void A311D_TIC(void)
{
    struct timeval time_v = {0};
    gettimeofday(&time_v, NULL);
    __a311d_tic = (double)(time_v.tv_sec * 1e6 + time_v.tv_usec);
}

void A311D_TOC(const char* func_name)
{
    struct timeval time_v = {0};
    gettimeofday(&time_v, NULL);
    __a311d_toc = (double)(time_v.tv_sec * 1e6 + time_v.tv_usec);
    printf("%s take %.2f ms\n", func_name, (__a311d_toc - __a311d_tic) / 1000.0);
}
bool check_file_exist(const std::string file_name)
{
    FILE* fp = fopen(file_name.c_str(), "r");
    if(!fp)
    {
        std::cerr << "Input file not existed: " << file_name << "\n";
        return false;
    }
    fclose(fp);
    return true;
}
// union
// {
//     uint32_t i;
//     float f;
// }v;

inline std::pair<int,uint8_t> argmax(const uint8_t * begin, int step){
    uint8_t max_val = *begin;
    int max_index = 0;
    for(int i=0; i<step; i++){
        if(*(begin+i) > max_val){
            max_val = *(begin+i);
            max_index = i;
        }
    }
    return std::make_pair(max_index, max_val);
}

inline std::pair<int,float> argmax_quantify(const uint8_t * begin, int step, int quality_zp, float quality_scale){
    float max_val = (*begin - quality_zp) * quality_scale;
    int max_index = 0;
    for(int i=0; i<step; i++){
        float cur_val = (float)(*(begin+i) - quality_zp) * quality_scale;
        if(cur_val > max_val){
            max_val = cur_val;
            max_index = i;
        }
    }
    return std::make_pair(max_index, max_val);
}

void nms(std::vector<abox> &input_boxes, float nms_thresh) {
    std::vector<float> vArea(input_boxes.size());
    for (int i = 0; i < (int) input_boxes.size(); ++i) {
        vArea[i] =
                (input_boxes.at(i).x2 - input_boxes.at(i).x1 + 1) * (input_boxes.at(i).y2 - input_boxes.at(i).y1 + 1);
    }
    for (int i = 0; i < (int) input_boxes.size(); ++i) {
        for (int j = i + 1; j < (int) input_boxes.size();) {
            float xx1 = std::max(input_boxes[i].x1, input_boxes[j].x1);
            float yy1 = std::max(input_boxes[i].y1, input_boxes[j].y1);
            float xx2 = std::min(input_boxes[i].x2, input_boxes[j].x2);
            float yy2 = std::min(input_boxes[i].y2, input_boxes[j].y2);
            float w = std::max(float(0), xx2 - xx1 + 1);
            float h = std::max(float(0), yy2 - yy1 + 1);
            float inter = w * h;
            float ovr = inter / (vArea[i] + vArea[j] - inter);
            if (ovr >= nms_thresh) {
                input_boxes.erase(input_boxes.begin() + j);
                vArea.erase(vArea.begin() + j);
            } else {
                j++;
            }
        }
    }
}

std::vector<size_t> sort_index(const std::vector<float> &v) {
    std::vector<size_t> idx(v.size());
    for (size_t i = 0; i != idx.size(); ++i) {
        idx[i] = i;
    }
    std::sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) { return v[i1] > v[i2]; });
    return idx;
}

int AMLOGIC_Load_Init_Tengine() {
    // init tengine
    if (init_tengine() < 0) {
        std::cout << " Init tengine failed.\n";
        return 1;
    }

    set_log_level(LOG_WARNING);//LOG_DEBUG);
    /*std::cout << "Tengine version: " << get_tengine_version() << "\n";
    if (request_tengine_version("1.0") < 0) {
        return -1;
    }

    if (load_tengine_plugin(VXDEVICE, "libvxplugin.so", "vx_plugin_init") < 0) {
        std::cout << "Load vx plugin failed.\n";
        return 1;
    }*/

    return 0;
}

int AMLOGIC_Load_Retinaface_Model(char *pcModelName, const AmlogicRetinaParams &params, void **model_handle, int *blob_shape) {
    if (!check_file_exist(pcModelName)) {
        std::cout << "no such model file: " << pcModelName << std::endl;
        return -1;
    }
    AMLOGIC_MODEL_RETINAFACE_DATA *amlogic_model_ptr = new AMLOGIC_MODEL_RETINAFACE_DATA();

    //设置为cpu运行
    // context_t vxContext = nullptr;

    //设置为npu运行
    context_t vxContext = create_context("vx_ctx", 1);
    int nAddRet = add_context_device(vxContext, VXDEVICE);
    printf("[AMLOGIC_Load_Retinaface_Model]->add_context_device %s ret %d.\n",VXDEVICE,nAddRet);

    amlogic_model_ptr->top_graph = create_graph(vxContext, "tengine", pcModelName);
    if (amlogic_model_ptr->top_graph == nullptr) {
        std::cout << "Create top graph failed\n";
        return -1;
    }

#if 0
    // dump_graph(amlogic_model_ptr->top_graph);
    int graph_ret = set_graph_device(amlogic_model_ptr->top_graph, VXDEVICE);
    if (0 != graph_ret) {
        std::cout << "Set graph device failed.\n";
        return -1;
    }
#endif
    int val = 1;
    set_graph_attr(amlogic_model_ptr->top_graph, "low_mem_mode", &val, sizeof(val));

    amlogic_model_ptr->out_strides = params.strides;
    amlogic_model_ptr->anchor_num = params.anchor_num;
    amlogic_model_ptr->with_sigmoid = params.with_sigmoid;
    if(amlogic_model_ptr->with_sigmoid)
        amlogic_model_ptr->score_threshold = params.detectThreshold;
    else
        amlogic_model_ptr->score_threshold = 0-log(1.0/params.detectThreshold-1);
    
    amlogic_model_ptr->nms_threshold = params.nmsThreshold;
    amlogic_model_ptr->concat_feat = params.concat_feat;

    amlogic_model_ptr->input_tensor = get_graph_tensor(amlogic_model_ptr->top_graph, params.inputTensorName.c_str());
    // int dims[] = {1, params.input_shape[0], params.input_shape[1], params.input_shape[2]};
    // int dims[] = {1, 3, 544, 960};
    // set_tensor_shape(amlogic_model_ptr->top_graph, dims, 4);

    int dims_input[4] = {0};
    int input_dims = get_tensor_shape(amlogic_model_ptr->input_tensor, dims_input, 4);
    //assert(input_dims == 4);
    int input_size = 1;
    for (int i = 0; i < input_dims; i++) {
        blob_shape[i] = dims_input[i];
        input_size *= dims_input[i];
    }

    int ret = prerun_graph(amlogic_model_ptr->top_graph);
    if (ret != 0) {
        std::cout << "Prerun top graph failed, errno: " << get_tengine_errno() << "\n";
        return 0;
    }

    //获取输入和输出层的量化参数
    float in_scale = 1;
    int in_zero = 0;
    get_tensor_quant_param(amlogic_model_ptr->input_tensor, &in_scale, &in_zero, 1);
    amlogic_model_ptr->input_zp = in_zero;
    amlogic_model_ptr->input_scale = in_scale;
    amlogic_model_ptr->udata = new uint8_t[input_size];
    int out_tensor_num = params.outputTensorNames.size();
    amlogic_model_ptr->output_tensors.resize(out_tensor_num);
    for (int i = 0; i < out_tensor_num; i++) {
        float out_scale = 1;
        int out_zero = 0;
        amlogic_model_ptr->output_tensors[i] = get_graph_tensor(amlogic_model_ptr->top_graph, params.outputTensorNames[i].c_str());
        get_tensor_quant_param(amlogic_model_ptr->output_tensors[i], &out_scale, &out_zero, 1);
        amlogic_model_ptr->output_zps.push_back(out_zero);
        amlogic_model_ptr->output_scales.push_back(out_scale);
    }

    //计算基础anchor
    int w = dims_input[3];
    int h = dims_input[2];
    amlogic_model_ptr->concat_anchor.clear();
    std::vector<std::vector<int> > feature_map;
    for (size_t i = 0; i < params.strides.size(); ++i) {
        std::vector<int> feat_val;
        feat_val.push_back(ceil(h/params.strides[i]));
        feat_val.push_back(ceil(w/params.strides[i]));
        feature_map.push_back(feat_val);
    }

    for (size_t k = 0; k < feature_map.size(); ++k)
    {
        std::vector<std::vector<float>> min_sizes;
        // std::vector<box> tmp_anchors;

        float s_kx = params.base_sizes[k];
        float s_ky = params.base_sizes[k];
        float box_area = s_kx*s_ky;
        std::vector<float> scales = params.scales[k];
        // for(size_t m=0; m<scales.size(); m++){
        //     for(size_t n=0; n<params.ratios.size(); n++){
        for(size_t n=0; n<params.ratios.size(); n++){
            for(size_t m=0; m<scales.size(); m++){
                float size_ratio = box_area / params.ratios[n];
                float ws = sqrt(size_ratio);
                float hs = ws * params.ratios[n];
                float scale_ws = ws * scales[m];
                float scale_hs = hs * scales[m];
                std::vector<float> min_size;
                min_size.push_back(scale_ws);
                min_size.push_back(scale_hs);
                min_sizes.push_back(min_size);
            }
        }

        for (int i = 0; i < feature_map[k][0]; ++i)
        {
            for (int j = 0; j < feature_map[k][1]; ++j)
            {
                for (size_t l = 0; l < min_sizes.size(); ++l)
                {
                    float s_kx = min_sizes[l][0]*1.0/w;
                    float s_ky = min_sizes[l][1]*1.0/h;
                    float cx = (j + 0.5) * params.strides[k]/w;
                    float cy = (i + 0.5) * params.strides[k]/h;
                    box axil = {cx, cy, s_kx, s_ky};
                    amlogic_model_ptr->concat_anchor.push_back(axil);
                    // tmp_anchors.push_back(axil);
                }
            }
        }
    }
    *model_handle = (void *) (amlogic_model_ptr);

    return 0;
}

int AMLOGIC_Retinaface_Forward_Buffer(void *data, float im_scale_h, float im_scale_w, 
    void *model_handle, std::vector<abox> &result) {
    result.clear();
    AMLOGIC_MODEL_RETINAFACE_DATA *amlogic_model_ptr = (AMLOGIC_MODEL_RETINAFACE_DATA *) model_handle;

    // A311D_TIC();
    int dims_input[4] = {0};
    int input_dims = get_tensor_shape(amlogic_model_ptr->input_tensor, dims_input, 4);
    //assert(input_dims == 4);
    int input_h = dims_input[2];
    int input_w = dims_input[3];
    int input_size = input_h*input_w;
    //输入数据  减去均值，再量化
    int mean[3]= {104,117,123};
    uint8_t *u_ori_data = (uint8_t *) data;
    float in_scale = amlogic_model_ptr->input_scale;
    int in_zero = amlogic_model_ptr->input_zp;
    
    for (int i = 0; i < dims_input[1]; i++) {
        for(int j=0; j< input_size; j++){
            int t = (uint8_t) (round(((int)u_ori_data[i*input_size+j]-mean[i]) / in_scale) + in_zero);
            if (t > 255)
                t = 255;
            else if (t < 0)
                t = 0;

            amlogic_model_ptr->udata[i*input_size+j] = t;
        }
    }

    set_tensor_buffer(amlogic_model_ptr->input_tensor, amlogic_model_ptr->udata, input_w * input_h * 3);
    // A311D_TOC("set_buffer");
    //运行模型前向
    // A311D_TIC();
    int ret = run_graph(amlogic_model_ptr->top_graph, 1);
    if (ret != 0) {
        std::cout << "Run top graph failed, errno: " << get_tengine_errno() << "\n";
        return -1;
    }
    
    std::vector<abox> proposals_list;
    std::vector<float> score_list;    
    std::vector<int> quality_zps = amlogic_model_ptr->output_zps;
    std::vector<float> quality_scales = amlogic_model_ptr->output_scales;

    uint8_t *score_data_uint8 = (uint8_t *) get_tensor_buffer(amlogic_model_ptr->output_tensors[0]);
    uint8_t *box_data_uint8 = (uint8_t *) get_tensor_buffer(amlogic_model_ptr->output_tensors[1]);
    int dims_score_delta[4] = {0};
    int score_delta_dims = get_tensor_shape(amlogic_model_ptr->output_tensors[0], dims_score_delta, 4);

    // A311D_TOC("run_graph");
    // A311D_TIC();
    int class_num = dims_score_delta[2];
    for (size_t i = 0; i < amlogic_model_ptr->concat_anchor.size(); ++i)
    {
        // std::pair<int, float> max_result = argmax_quantify(score_data_uint8+1, class_num-1, quality_zps[0], quality_scales[0]);
        // float score_val = max_result.second;
        std::pair<int, float> max_result = argmax(score_data_uint8+1, class_num-1);
        float score_val = (max_result.second - quality_zps[0])* quality_scales[0];

        if ( score_val > amlogic_model_ptr->score_threshold){
            if(!(amlogic_model_ptr->with_sigmoid))
                score_val = 1/(1+exp(0-score_val));

            box tmp0 = amlogic_model_ptr->concat_anchor[i];
            box tmp1;
            abox tmp_bbox;

            // loc and conf
            tmp1.cx = tmp0.cx + ((*box_data_uint8)-quality_zps[1]) * quality_scales[1] * 0.1 * tmp0.sx;
            tmp1.cy = (tmp0.cy + (*(box_data_uint8+1)-quality_zps[1]) * quality_scales[1] * 0.1 * tmp0.sy);
            tmp1.sx = tmp0.sx * exp((*(box_data_uint8+2)-quality_zps[1]) * quality_scales[1] * 0.2);
            tmp1.sy = tmp0.sy * exp((*(box_data_uint8+3)-quality_zps[1]) * quality_scales[1] * 0.2);

            tmp_bbox.x1 = (tmp1.cx - tmp1.sx/2) * input_w;
            if (tmp_bbox.x1<0)
                tmp_bbox.x1 = 0;
            tmp_bbox.y1 = (tmp1.cy - tmp1.sy/2) * input_h;
            if (tmp_bbox.y1<0)
                tmp_bbox.y1 = 0;
            tmp_bbox.x2 = (tmp1.cx + tmp1.sx/2) * input_w;
            if (tmp_bbox.x2 > input_w)
                tmp_bbox.x2 = input_w;
            tmp_bbox.y2 = (tmp1.cy + tmp1.sy/2)* input_h;
            if (tmp_bbox.y2 > input_h)
                tmp_bbox.y2 = input_h;

            tmp_bbox.x1 = tmp_bbox.x1 / im_scale_w;
            tmp_bbox.y1 = tmp_bbox.y1 / im_scale_h;
            tmp_bbox.x2 = tmp_bbox.x2 / im_scale_w;
            tmp_bbox.y2 = tmp_bbox.y2 / im_scale_h;
            tmp_bbox.class_id = max_result.first;
            tmp_bbox.score = score_val;

            score_list.push_back(score_val);
            proposals_list.push_back(tmp_bbox);
        }
        
        score_data_uint8 += class_num;
        box_data_uint8 += 4;
    }

    std::vector<size_t> sorted_idx;
    sorted_idx = sort_index(score_list);
    std::vector<abox> proposals_box;
    for (size_t i = 0; i < sorted_idx.size(); i++) {
        proposals_box.push_back(proposals_list[sorted_idx[i]]);
    }

    for (size_t i = 0; i < proposals_list.size(); i++) {
        proposals_box[i].score = score_list[sorted_idx[i]];
    };

    // ====== Filtering the target boxes =======
    nms(proposals_box, amlogic_model_ptr->nms_threshold);
    for (size_t k = 0; k < proposals_list.size(); k++) {
        if (proposals_box[k].score < amlogic_model_ptr->score_threshold) {
            proposals_box.erase(proposals_box.begin() + k);
        } else
            k++;
    }
    if (proposals_box.size() > 0) {
        for (int b = 0; b < (int) proposals_box.size(); b++) {
            result.push_back(proposals_box[b]);
        }
    }
    // A311D_TOC("post");
    return 0;
}

void AMLOGIC_Set_Detect_Threshold(void *model_handle, float threshold) {
    AMLOGIC_MODEL_RETINAFACE_DATA *amlogic_model_ptr = (AMLOGIC_MODEL_RETINAFACE_DATA *) model_handle;

    if(amlogic_model_ptr->with_sigmoid)
        amlogic_model_ptr->score_threshold = threshold;
    else
        amlogic_model_ptr->score_threshold = 0-log(1.0/threshold-1);
}

void AMLOGIC_Release_Retinaface_Model(void *model_handle) {
    AMLOGIC_MODEL_RETINAFACE_DATA *amlogic_model_ptr = (AMLOGIC_MODEL_RETINAFACE_DATA *) model_handle;
    int ret = postrun_graph(amlogic_model_ptr->top_graph);
    if (ret != 0) {
        std::cout << "Postrun top graph failed, errno: " << get_tengine_errno() << "\n";
        return;
    }

    delete amlogic_model_ptr->udata;

    destroy_graph(amlogic_model_ptr->top_graph);

    release_tengine();
}
