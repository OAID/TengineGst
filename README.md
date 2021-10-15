# TengineStreamer
# 简介：
Tengine Streamer是由 OPEN AI LAB基于GStreamer多媒体框架的分析推理框架，用于创建各类多媒体处理管道，可以方便的利用各种成熟的插件快速的搭建起稳定的 AI 应用，主要精力放在核心的业务上。
完整的解决方案利用了：

1：用于管道管理的开源GStreamer 框架

2：GStreamer 用于输入和输出的插件，如媒体文件和来自摄像头或网络的实时流媒体

3：从模型商店的培训框架TensorFlow、Caffe等转换而来的Tengine 深度学习模型

Tengine-Streamer中的解决方案的基础插件：

特定于深度学习的插件，这些插件也可在此存储库中获得：

1：推理插件利用Tengine 使用深度学习模型进行高性能推理

2：推理结果的可视化，带有检测对象的边界框和标签，位于视频流之上

3：推理结果可以通过MQTT等标准协议推送出去

## 插件包括
- streammux：多路流合并成一路由一路算法处理多路
- streamdemux：一路推理的结果分离出相对应的各路，与streammux配合使用
- videoanalysis：主要的推理插件，提供了标准二次插件接口，可以动态加载推理业务，在大多数不想写插件的时候，只需要实现一个业务动态库，由此插件把推理业务交给推理业务库即可。配合类“inferservice”的业务库使用。如果特别熟悉GStreamer插件开发，可以自己写一个插件来直接进行推理业务。
- mqtt：提供了把推理结果向 mqtt broker 泵的插件
- postprocess：简单的把推理结果叠加到视频流的功能
## 业务插件
- inferservice：调用Tengine 推理框架，加载模型，推理结果，并把结果输出到分析插件

gst-launch-1.0 rtspsrc location="rtsp://*/*" ! rtph264depay ! capsfilter caps="video/x-h264" ! h264parse ! avdec_h264 !  videoanalysis businessdll=<dir>/libinferservice.so  ! postprocess ! mqtt username=admin userpwd=admin servip=10.11.5.247 servport=1883 ! fakevideosink

按照inferservice框架编译的库，也可以作为videoanalysis 插件的业务库传入，可以支持不同算法业务。
## 需要安装依赖：
- sudo apt install -y build-essential cmake
- sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev

目前在khadas 板子编译，其他只需更改Tengine推理库编译即可
### 编译方法：
cmake 目录下：cross.cmake 里面，有交叉编译开关，如果是交叉编译环境，调整交叉编译路径。真机环境，可以把交叉编译关闭。对应的khadas支持库已经提供。

具体编译方法：

在根目录创建 build 目录，然后进入build 目录，执行 cmake .. ，再执行 make
- mkdir build
- cd build
- cmake ..
- make
### 示例
编译时，可以把环境依赖 env-run.tar.gz 解压出来，可以减少编译难度。

tar zxvf env-run.tar.gz 

在交叉编译环境下，build 目录会产生 aarc64/Release/lib 目录，里面即是所有的插件和依赖库。把里面的插件库拷贝到设备的GStreamer 目录下：/usr/lib/aarch64-linux-gnu/gstreamer-1.0/

执行类似命令检查插件：
gst-inspect-1.0 mqtt

因为示例插件依赖了corecv（opencv扩展）、jpg 等库，需要把子目录 run 里面的库拷贝到设备的 /usr/lib/aarch64-linux-gnu/ 目录下
模型文件拷贝到：/home/khadas/ 即可，这个发布的时候，可以随意指定路径。

# 致谢
- [Tengine](https://github.com/OAID/Tengine)
- [GStreamer](https://gstreamer.freedesktop.org/src/)
- [curl](https://github.com/curl/curl.git)
- [zlog](https://github.com/lisongmin/zlog)
- [mosquitto](https://github.com/eclipse/mosquitto)
- [turbojpeg](https://github.com/libjpeg-turbo/libjpeg-turbo)
# License
- [Apache 2.0](https://github.com/OAID/Tengine/blob/tengine-lite/LICENSE)
