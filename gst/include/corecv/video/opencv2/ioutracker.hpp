/******************************************************************************

        版权所有 (C),2018-2025  开放智能机器（上海）有限公司
                        OPEN AI LAB
                        
******************************************************************************
  文 件 名   : ioutracker.hpp
  作     者:  杨荣钊
  生成日期   :  2019年4月1日
  功能描述   :  IOU跟踪器头文件
******************************************************************************/
#ifndef __IOU_TRACKER_HPP__
#define __IOU_TRACKER_HPP__

#include <vector>

#include "core/core.hpp"
#include "tracking.hpp"

#ifdef CV_USE_IVE

#define IOUTRACKER_MAX_ID    30 //最大ID

namespace cv
{

/*----------------------------------------------*
 *                    IOUTracker                *
 *----------------------------------------------*/
class IOUTracker
{
public:
    /********************************
    * 函数名称：IOUTracker
    * 功能描述：构造函数
    * 输入参数   : iou_thresh    判断框框为同一个的IOU阈值
    * 输出参数   : 
    * 返回参数   : 
    ********************************/
    IOUTracker(float iou_thresh = 0.5);

    /********************************
    * 函数名称：~IOUTracker
    * 功能描述：析构函数
    * 输入参数   :  
    * 输出参数   : 
    * 返回参数   : 
    ********************************/
    ~IOUTracker();

    /********************************
    * 函数名称：update
    * 功能描述：更新物体
    * 输入参数   : image        IVE图像
    *           bboxes      新的框框
    * 输出参数   : 
    * 返回参数   : 
    ********************************/
    bool update(IVE_IMAGE_S *image, const std::vector<cv::Rect>& bboxes);

private:
    /********************************
    * 函数名称：compute_quality
    * 功能描述：计算人脸质量，obj中的quality会被更新
    * 输入参数   : image        IVE图像
    *           roi         某个区域
    *           quality     人脸质量
    *           img_roi     扣出来的脸
    * 输出参数   : 
    * 返回参数   : 
    ********************************/
    void compute_quality_and_roi(IVE_IMAGE_S * image, const cv::Rect& roi, float* quality, IVE_IMAGE_S *img_roi);

    /********************************
    * 函数名称：compute_iou
    * 功能描述：计算IOU
    * 输入参数   : rect1     框框1   
    *           rect2    框框2    
    * 输出参数   : 
    * 返回参数   : 交并比，0到1
    ********************************/
    float compute_iou(const cv::Rect& rect1, const cv::Rect& rect2);

    /********************************
    * 函数名称：get_ive_roi
    * 功能描述：从src中获取roi对应的IVE图像到dst
    * 输入参数   : src     源图像  
    *           dst    目标图像   
    *           roi    ROi
    * 输出参数   : 
    * 返回参数   : 
    ********************************/
    void get_ive_roi(IVE_IMAGE_S *src, IVE_IMAGE_S *dst, const cv::Rect& roi);

public:
    // 所有的脸
    std::vector<IVE_TRACK_FACE> objects;
    
private:
    // IOU阈值
    float iou_thresh_;
};

}
#endif // CV_IVE_HI3519A

#endif // __IOU_TRACKER_HPP__