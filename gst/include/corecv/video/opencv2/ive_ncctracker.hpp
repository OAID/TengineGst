// 描述：IVE的LK光流法头文件
// 作者：rzyang
// 日期：2019年3月26日

#ifndef _VIDEO_NCCTRACKER_HPP_
#define _VIDEO_NCCTRACKER_HPP_

#include <vector>

#include "core/core.hpp"

namespace cv
{

// NCC跟踪器
class NCCTracker
{
public:
    // 函数名称：IVE_NCCTracker
    // 函数描述：构造函数
    // 输入参数：
    // grid_size    以init的框框为中心分成多少个格子
    // stride       每个格子的大小
    NCCTracker(int grid_size, int stride);

    // 函数名称：~IVE_NCCTracker
    // 函数描述：析构函数
    ~NCCTracker();
}

} // namespace cv

#endif // _VIDEO_NCCTRACKER_HPP_