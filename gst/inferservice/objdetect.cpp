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
 
#include "objdetect.h"
#include <stdlib.h>
#include <string.h>
#include <corecv.hpp>
#include <unistd.h>
#include "normal.h"

#include "loger.h"
#include "amlogic_retinaface_multi_detect_algo.h"

using namespace cv;

int CInferIns::InitService(INFER_SERV_Msg_CB cbMsg,void *user)
{
	return CObjdetectServ::getInstance()->Init(cbMsg,user);
}

int CInferIns::DeinitService(void *user)
{
	return CObjdetectServ::getInstance()->Deinit(user);
}

//动态调用
void *CInferIns::CreateChn(INFER_SERV_RES_CB cbRes,INFER_SERV_Msg_CB cbMsg,void *user)
{
	return (void*)CObjdetectServ::getInstance()->CreateChn(cbRes,cbMsg,user);
}

int CInferIns::DestoryChn(void *handle)
{
	return CObjdetectServ::getInstance()->DestoryChn((TINFER_CHN_HDL *)handle);
}

int CInferIns::SendFrame(void *handle, void *frame)
{
	return CObjdetectServ::getInstance()->SendFrame((TINFER_CHN_HDL *)handle,(FRAME_DATA_S*)frame);
}

int CInferIns::SetChnAttr(void *handle,int nPropID,int nPropType,void *dtAttr)
{
	return CObjdetectServ::getInstance()->SetChnAttr((TINFER_CHN_HDL *)handle,nPropID,nPropType,dtAttr);
}

int CInferIns::GetChnAttr(void *handle,int nPropID,void *dtAttr)
{
	return CObjdetectServ::getInstance()->GetChnAttr((TINFER_CHN_HDL *)handle,nPropID,dtAttr);
}

int CInferIns::InMsg(void *handle,TPluginMsg *data)
{
	return CObjdetectServ::getInstance()->InMsg((TINFER_CHN_HDL *)handle,data);
}

#define CFG_BODY_DETECT "/root/install/config/cfg-body.json"
static void *worker(void *parg);
CObjdetectServ::CObjdetectServ(){
	memset(m_blob_shape,0,4*sizeof(int));
	
	m_nCheckWidth = 50;
	m_nThreshold = 80;
	m_pModelHdl = NULL;
	strcpy(m_tsCfgPath,CFG_BODY_DETECT);
}

CObjdetectServ::~CObjdetectServ(){
}

static int s_vtClr[] = {0x0000FF,0x00FAFA,0xFF0000,0x00FF00,0x962FEB,0x8F00FF};
CObjdetectServ *CObjdetectServ::getInstance()
{
	static CObjdetectServ *obj = NULL;
	if (NULL == obj)
	{
		obj = new CObjdetectServ();	
		//加载模型
		obj->initModel();
	}
	return obj;
}

static void *initModelThrd(void *parg)
{
	CObjdetectServ *pInferService = (CObjdetectServ *)parg;
    LOG_INFO("[initModelThrd]->serv = 0x%x.",parg);
	if(pInferService)
		pInferService->do_initModel();
	return NULL;
}

int CObjdetectServ::do_initModel()
{
	char tsModelPath[128] = {0};
    AMLOGIC_Load_Init_Tengine();
	AmlogicRetinaParams retina_param;
	//此为王昊训练的人头、安全帽和人体模型的anchor配置
	retina_param.detectThreshold = 0.4;
	retina_param.nmsThreshold = 0.4;
	//保留stride为8，16，32的所有分支
	retina_param.anchor_num = 4;
	retina_param.ratios = std::vector<float>({1.0f,2.5f});
	retina_param.base_sizes = std::vector<float>({10.0f,16.0f,16.0f});
	retina_param.strides = std::vector<int>({8,16,32});
	retina_param.scales = std::vector<std::vector<float>>({{8.0f,4.0f,2.0f,1.0f},
		{8.0f,4.0f,2.0f,1.0f},{32.0f,16.0f,8.0f,4.0f}});
	retina_param.concat_feat = true;
	retina_param.input_shape = std::vector<int>({3, 544, 960});
	retina_param.inputTensorName = "input0";
	retina_param.outputTensorNames = std::vector<std::string>({"classes", "bbox"});
	retina_param.with_sigmoid = false;
	//sprintf(tsModelPath,"%s","/root/install/model/from_onnx_UINT8.tmfile");
	sprintf(tsModelPath,"%s","/home/khadas/from_onnx_UINT8.tmfile");
	AMLOGIC_Load_Retinaface_Model(tsModelPath ,retina_param, &m_pModelHdl, m_blob_shape);
    m_nShapeC = m_blob_shape[1];
    m_nShapeH = m_blob_shape[2];
    m_nShapeW = m_blob_shape[3];
    LOG_INFO("[CObjetectServ]->Model %s input_blob: %d %d %d,model=0x%x",
				tsModelPath,m_nShapeC,m_nShapeH,m_nShapeW,m_pModelHdl);
	mModelState = 1;
	return 0;
}
#define ASYNC_INIT
int CObjdetectServ::initModel()
{
	pthread_t mInitId;
	pthread_create(&mInitId, NULL, initModelThrd, this);
	return 0;
}
int CObjdetectServ::ChildDeinit(){
	mRunning = 0;
	pthread_join(mWorkId, NULL);
	AMLOGIC_Release_Retinaface_Model(m_pModelHdl);
	for (int i = 0; i < (int)m_vecChn.size(); i++)
	{
		TINFER_CHN_HDL *handle = (TINFER_CHN_HDL *)m_vecChn[i];
	
		if(handle->arrframe[0].data)
		free(handle->arrframe[0].data);
		if(handle->arrframe[1].data)
			free(handle->arrframe[1].data);
		if(handle->pBufInfer)
			delete handle->pBufInfer;
		free(handle);
	}
	m_vecChn.clear();
	return 0;
}
int preprocess_img(const cv::Mat& src, uint8_t* dst, int* input_shape){
    int inputH = input_shape[2];
    int inputW = input_shape[3];

    cv::Mat resized_mat(inputH, inputW, src.type());
    cv::resize(src, resized_mat, resized_mat.size(), 0, 0);
    //cv::cvtColor(resized_mat, resized_mat, cv::COLOR_BGR2RGB);
    //package2plannar
    int offset = inputH * inputW;
    uint8_t *out_data0 = reinterpret_cast<uint8_t *>(dst);
    uint8_t *out_data1 = reinterpret_cast<uint8_t *>(dst) + offset;
    uint8_t *out_data2 = reinterpret_cast<uint8_t *>(dst) + offset * 2;
    uint8_t *in_data = resized_mat.ptr<uint8_t>();
    for (int hh = 0; hh < inputH; ++hh) {
        for (int ww = 0; ww < inputW; ++ww) {
            *(out_data0++) = *(in_data++);
            *(out_data1++) = *(in_data++);
            *(out_data2++) = *(in_data++);
        }
    }
    return 0;
}

static int s_nInferIdx = 0;
static int s_bSave = 0;
int CObjdetectServ::RealWork(TINFER_CHN_HDL *pChnHdl){
	if(0==mModelState)
		return -1;
	char tsImgPath[128] = {0};
	std::vector<abox> result;
	cv::Mat img;
	cv::Mat frameMat = cv::Mat(pChnHdl->arrframe[pChnHdl->mCurInfer].height * 3 / 2,
		pChnHdl->arrframe[pChnHdl->mCurInfer].width, CV_8UC1, pChnHdl->arrframe[pChnHdl->mCurInfer].data, 0); 	/* bug */
	//cv::cvtColor(frameMat, img, cv::COLOR_YUV2RGB_NV21);
	cv::cvtColor(frameMat, img, cv::COLOR_YUV2RGB_I420);
	//cv::Mat img( pChnHdl->arrframe[pChnHdl->mCurInfer].height,
	//        pChnHdl->arrframe[pChnHdl->mCurInfer].width,
	//       CV_8UC3, pChnHdl->arrframe[pChnHdl->mCurInfer].data);
	sprintf(tsImgPath,"/root/saveYuv_%d.jpg",s_nInferIdx);
	float img_scale_h, img_scale_w;
	img_scale_h = float(m_nShapeH)/ float(img.rows);
	img_scale_w = float(m_nShapeW)/ float(img.cols);

	if(NULL==pChnHdl->pBufInfer)
		pChnHdl->pBufInfer = new uint8_t[m_nShapeH*m_nShapeW*m_nShapeC];
	//resize planar
	preprocess_img(img, pChnHdl->pBufInfer, m_blob_shape);
	//resize packed
	//cv::Mat resized_mat(m_nShapeH, m_nShapeW, img.type());
	//cv::resize(img, resized_mat, resized_mat.size(), 0, 0);
	/*if(0==s_bSave){
		cv::imwrite(tsImgPath, frameMat);
		cv::Mat objPackImg(m_blob_shape[2],m_blob_shape[3],
				CV_8UC3, pChnHdl->pBufInfer);
		sprintf(tsImgPath,"/root/saveResz_%d.jpg",s_nInferIdx);
		cv::imwrite(tsImgPath, objPackImg);
		s_bSave = 1;
	}*/
	//TIC();
	AMLOGIC_Retinaface_Forward_Buffer((void*)pChnHdl->pBufInfer,
		img_scale_h, img_scale_w, m_pModelHdl, result);
	
	//TOC("---[CObjdetectServ::do_work]->Forward_Buffer");
	//把结果回调出去？
	LOG_INFO("\n===>[do_work]->infer cb=0x%x buf=0x%x,count=%d,in dlen=%d w:h=%d:%d,w:h=%d:%d.\n",
		pChnHdl->callback,
		pChnHdl->pBufInfer,result.size(),
			pChnHdl->arrframe[pChnHdl->mCurInfer].len,
		pChnHdl->arrframe[pChnHdl->mCurInfer].width,
		pChnHdl->arrframe[pChnHdl->mCurInfer].height,
		m_blob_shape[2],m_blob_shape[3]);
	//0 行人，1 人头, 2 安全帽
	if(result.size()<50){
		vector<BBOX_S> boxes;
		for (int i = 0; i < (int)result.size(); i++)
		{
			//class id =1 ,head ,pass
			BBOX_S box;

			box.x = result[i].x1;
			box.y = result[i].y1;
			box.width = result[i].x2 - result[i].x1;
			box.heigh = result[i].y2 - result[i].y1;
			box.uColor = s_vtClr[result[i].class_id];
			box.label = result[i].class_id;
			box.score = result[i].score;
			if((1==result[i].class_id) || 
				((0==result[i].class_id)&&(box.width < m_nCheckWidth || box.heigh<m_nCheckWidth)))
				continue;
			/*Rect_Ajust(&box, pChnHdl->arrframe[pChnHdl->mCurInfer].width, 
				pChnHdl->arrframe[pChnHdl->mCurInfer].height, 
				m_nShapeW, m_nShapeH);*/
			boxes.push_back(box);
#if 0
			LOG_INFO("infer res: %lf_%lf_%lf_%lf\n", result[i].x1, result[i].y1, result[i].x2, result[i].y2);
			//printf("infer box: %lf_%lf_%lf_%lf\n", box.x, box.y, box.width, box.heigh); 		
#endif
		}
		if (pChnHdl->callback)
			pChnHdl->callback(&pChnHdl->arrframe[pChnHdl->mCurInfer], (void*)&boxes, pChnHdl->userdata);
	}
	return 1;
}