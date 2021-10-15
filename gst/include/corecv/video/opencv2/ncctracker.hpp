// 描述：IVE的LK光流法头文件
// 作者：rzyang
// 日期：2019年3月26日

#ifndef _VIDEO_NCCTRACKER_HPP_
#define _VIDEO_NCCTRACKER_HPP_

#include <vector>

#include "core/core.hpp"
#include "imgproc/imgproc.hpp"

#ifdef CV_USE_IVE

namespace cv
{

// NCC跟踪器
class NCCTracker
{
public:
    // 函数名称：NCCTracker
    // 函数描述：构造函数
    // 输入参数：
    // grid_size    以init的框框为中心分成多少个格子
    // stride       bbox / stride 作为 框框大小
    NCCTracker(int grid_size=3, int stride=5);

    // 函数名称：~NCCTracker
    // 函数描述：析构函数
    ~NCCTracker();

    // 函数名称：init
    // 函数描述：初始化，记住模板
    // 输入参数：
    // img       灰度图
    // bbox      初始框
    bool init(const cv::Mat& img, const cv::Rect& bbox);

    // 函数名称：update
    // 函数描述：更新框框
    // 输入参数：
    // img       灰度图
    bool update(const cv::Mat& img);

public:
    // 所有框框
    std::vector<cv::Rect> bboxes;

private:
    // 获取合法的候选框
    void get_proposals(const cv::Size& image_size, const cv::Rect& bbox);
    // 归一化互相关系数
    float ncc(const cv::Mat& input, const cv::Mat& templ);
    
private:
    // 格子一行或一列的数目
    int grid_size_;
    // 每个格子的大小
    int stride_;
    // 是否已初始化
    bool is_init_;
    bool is_img_init_;
    // 模板
    cv::Mat template_;
    // 候选框，通过init生成
    std::vector<cv::Rect> proposals_;
    // 候选框的分数
    std::vector<float> scores_;
    // IVE
    IVE_IMAGE_S ive_input_, ive_templ_;
    IVE_MEM_INFO_S ive_mem_;
};

} // namespace cv

#endif // CV_IVE_HI3519A

#endif // _VIDEO_NCCTRACKER_HPP_