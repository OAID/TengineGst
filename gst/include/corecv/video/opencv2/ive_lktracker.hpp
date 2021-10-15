// 描述：IVE的LK光流法头文件
// 作者：rzyang
// 日期：2019年3月26日

#ifndef _VIDEO_IVE_LKTRACKER_HPP_
#define _VIDEO_IVE_LKTRACKER_HPP_

#include <vector>

#include "core/core.hpp"
#include "imgproc/imgproc.hpp"

#ifdef CV_USE_IVE

// 最大金字塔层数 = 4
#define IVE_LK_MAX_LEVEL_NUM      4
// 最大特征点数目
#define IVE_LK_MAX_FEATURE_NUM    500
// Q7
#define IVE_LK_Q7    128.0f

namespace cv
{
    
// 跟踪物体
typedef struct
{
    int id ;       // 物体ID
    int status;    // 物体状态，保留
    cv::Rect bbox; // 跟踪框
    cv::Point2f center; // 特征点群的中心点
    std::vector<cv::Point2f> feat_pts; // 特征点
} IVE_TRACK_OBJ;

// IVE光流跟踪器
class IVE_LKTracker
{
public:
    // 函数描述：构造函数，初始化跟踪器，若参数非法会自动换成参考值
    // 输入参数：
    // num_levels:      金字塔层数，1到4层，参考取值2
    // min_eig_thresh:  特征值阈值，1到255，参考取值100
    // num_iter:        迭代次数，1到20，参考取值10
    // epsilon:         迭代收敛阈值，dx^2 + dy^2 < epsilon，1到255，参考取值2
    // iou_thresh:      判断框重合的IOU阈值
    IVE_LKTracker(uchar num_levels=2, float min_eig_thresh=1e-4, uchar num_iter=10, float 
    epsilon=0.03, float iou_thresh=0.7);

    // 函数描述：析构函数，释放内存
    ~IVE_LKTracker();

    // 函数名称：init
    // 函数描述：初始化要跟踪的物体，要给出物体的初始框，通常由检测器提供
    // 输入参数：
    // img：               图像，若不是灰度图会被转成灰度，建议在外层转换好
    // init_bboxes：       各个物体的初始框
    // 返回值：true代表成功，false代表失败
    bool init(const cv::Mat& img, const std::vector<cv::Rect>& init_bboxes);

    // 函数名称：update
    // 函数描述：根据新一帧的图片更新正在跟踪的物体的框框
    // 输入参数：
    // img:               新一帧的图像，大小必须和初始化时的图像一致，若不是灰度图会被转成灰度，建议在外层转换好
    // 返回值：true代表成功，false代表失败
    bool update(const cv::Mat& img);

public:
    // 正在跟踪的物体
    std::vector<cv::IVE_TRACK_OBJ> objects;
    // 前一帧所有特征点
    std::vector<cv::Point2f> prev_points;
    // 新一帧所有特征点
    std::vector<cv::Point2f> next_points;

private:
    // 释放内存
    void release();
    // 释放某张图像的内存
    void release_img(IVE_IMAGE_S *ive_img);
    // 初始化图像
    bool init_img(const cv::Mat& img);
    // 添加物体、去重
    void init_objects(const cv::Mat& img, const std::vector<cv::Rect>& init_bboxes);
    // 计算IOU
    float calc_iou(const cv::Rect& rect1, const cv::Rect& rect2);
    // 获取特征点
    bool get_feature_points(const cv::Mat& img, const cv::Rect& roi, std::vector<cv::Point2f>& points);
    // 生成IVE的图像金字塔
    HI_S32 generate_ive_pyramid(IVE_IMAGE_S *ive_images);
    // DMA操作
    HI_S32 lk_dma(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,IVE_DMA_CTRL_S *pstDmaCtrl,HI_BOOL 
    bInstant);
    // 将所有CoreCV特征点塞到IVE的特征点数组
    void copy_points(const std::vector<cv::Point2f>& cv_pts, IVE_MEM_INFO_S *ive_pts, size_t offset=0);
    // 拷贝IVE特征点
    void copy_points(IVE_SRC_MEM_INFO_S *ive_src_pts, std::vector<cv::Point2f>& dst_pts);
    // IVE光流跟踪实现
    bool lk_update();
    // 复制图像金字塔
    bool copy_pyr(IVE_IMAGE_S astPyrSrc[], IVE_IMAGE_S astPyrDst[]);
    // 计算一堆光流点的中心
    cv::Point2f calc_of_center(const std::vector<cv::Point2f> ofs);
    // 计算两个光流点的距离
    float calc_dist(const cv::Point2f& p1, const cv::Point2f& p2);

private:
    // 初始化成功标志位
    bool is_init_;
    // 图像宽度
    int img_width_;
    // 图像高度
    int img_height_;
    // IVE图像，前一帧金字塔
    IVE_IMAGE_S ive_prev_img_[IVE_LK_MAX_LEVEL_NUM];
    // IVE图像，新一帧金字塔
    IVE_IMAGE_S ive_next_img_[IVE_LK_MAX_LEVEL_NUM];
    // IVE图像，计算金字塔时的临时图片
    IVE_IMAGE_S ive_pyramid_temp;
    // IVE LK光流计算控制参数
    IVE_LK_OPTICAL_FLOW_PYR_CTRL_S ive_lk_ctrl_;
    // 跟踪框IOU阈值，超过阈值则认为是同一个框
    float iou_thresh_;
    // 金字塔层数
    int num_levels_;
    // 前一帧特征点的内存
    IVE_MEM_INFO_S ive_prev_pts_;
    // 下一帧特征点的内存
    IVE_MEM_INFO_S ive_next_pts_;
    
    // 跟踪状态，1代表成功，0代表失败
    IVE_MEM_INFO_S ive_track_status_;
    // 跟踪点误差
    IVE_MEM_INFO_S ive_track_err_;
};

} // namespace cv
#endif // _VIDEO_IVE_LKTRACKER_HPP_

#endif // CV_IVE_HI3519A