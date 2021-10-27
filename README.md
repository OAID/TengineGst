# TengineGst

## 简介
TengineGst 是 OPEN AI LAB 基于 GStreamer 多媒体框架的分析推理框架，用于创建各类多媒体处理管道，可以方便的利用各种成熟的插件快速搭建起稳定的应用，并使用 Tengine 优化推理操作，更快更优更专注的开发核心 AI 业务。
完整的解决方案利用了：
- 用于管道管理的开源 GStreamer 框架;
- GStreamer 用于输入和输出的插件，如媒体文件和来自摄像头或网络的实时流媒体;
- GStreamer 各种成熟插件，例如编解码、图形处理等;
- 从主流训练框架 Caffe、TensorFlow、ONNX、Darknet 等转换而来的 Tengine 深度学习模型 tmfile。

TengineGst 中深度学习推理的插件：
- 推理插件利用Tengine 使用深度学习模型进行高性能推理；
- 推理结果的可视化，带有检测对象的边界框和标签，位于视频流之上；
- 推理结果可以通过MQTT等标准协议推送出去。

## 架构
![架构](https://github.com/OAID/TengineGst/blob/main/docs/TengineGst.png)
数据流
![pipeline](https://github.com/OAID/TengineGst/blob/main/docs/TengineGst-Flow.png)

## 插件包括
- streammux：多路流合并成一路由一路算法处理多路；
- streamdemux：一路推理的结果分离出相对应的各路，与 streammux 配合使用；
- videoanalysis：主要的推理插件，提供了标准二次插件接口，可以动态加载推理业务，在大多数不想写插件的时候，只需要实现一个业务动态库，由此插件把推理业务交给推理业务库即可。配合类“inferservice”的业务库使用。如果特别熟悉 GStreamer 插件开发，可以自己写一个插件来直接进行推理业务；
- mqtt：把推理结果泵向 mqtt broker 的功能；
- postprocess：简单的把推理结果叠加到视频流的功能。

## 业务插件
inferservice：调用Tengine 推理框架，加载模型，推理结果，并把结果输出到分析插件按照类 inferservice 库的框架编译的库，也可以作为 videoanalysis 插件的业务库传入，修改 businessdll 属性为业务库地址，即可以支持不同算法业务。

## 需要安装依赖
```
sudo apt update
sudo apt install pkg-config
sudo apt install pkgconf
sudo apt install -y build-essential cmake
sudo apt install gstreamer1.0-tools libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-libav gstreamer1.0-plugins-bad gstreamer1.0-plugins-good
apt install libssl-dev
```

目前在x86/khadas 环境编译，其他设备只需替换不同 Tengine 推理库即可。

### 编译方法
#### 依赖模块进行编译
##### Tengine
```
//参考：https://github.com/OAID/Tengine/tree/tengine-lite/doc/docs_zh/source_compile，编译不同 npu 版本，下面例子编译x86
cd Tengine
mkdir build 
cd build
cmake ..
make
make install
cp -r install/include/ /usr/local/
cp install/lib/* /usr/local/lib/
```

##### zlog：
```
wget https://github.com/HardySimpson/zlog/archive/refs/tags/1.2.15.tar.gz
tar zxvf 1.2.15.tar.gz
cd zlog-1.2.15
make PREFIX=/usr/local
sudo make PREFIX
=/usr/local install
```
##### mosquitto
```
git clone https://github.com/eclipse/mosquitto.git
cd mosquitto && cd lib
make && make install
```

##### turbo-jpeg
```
git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git
cd libjpeg-turbo
mkdir build
cmake ..
make && make install
```
#### opencv
```
wget https://github.com/opencv/opencv/archive/3.4.16.zip
unzip 3.4.16.zip
cd opencv-3.4.16
mkdir build
cd build
cmake ..
make && make install
```

### 编译工程
cmake 目录下 cross.cmake 文件有交叉编译开关。交叉编译环境需调整交叉编译路径。对应的 khadas 支持库已经提供，打开开关即可。

具体编译方法：
```
mkdir build
cd build
cmake ..
make
```

### 示例
khadas 交叉编译，需要 env-run.tar.gz 解压出来，可以减少编译步骤，这时需要把子目录 run 里面的库拷贝到设备里面，模型文件参看源码里面指定的路径，拷贝到相应的目录。
```
tar zxvf env-run.tar.gz
```

编译完成，build 目录会产生 aarc64/Release/lib 目录，里面即是所有的插件。把里面的插件库拷贝到GStreamer 目录下。类似命令：
```bash
// khadas
cp aarch64/Release/lib/libgst* /usr/lib/aarch64-linux-gnu/gstreamer-1.0/
// x86
cp aarch64/Release/lib/libgst* /usr/lib/x86_64-linux-gnu/gstreamer-1.0/
```

执行类似命令检查插件 `gst-inspect-1.0 mqtt` 。因为示例插件，需要把子目录 `run` 里面的库拷贝到设备的 `/usr/lib/aarch64-linux-gnu/` 目录下
模型文件拷贝到：`/home/khadas/`（见插件 inferservice） 即可，这个发布的时候，可以随意指定路径。

### 测试命令
```
gst-launch-1.0 rtspsrc location="rtsp://**" ! rtph264depay ! capsfilter caps="video/x-h264" ! h264parse ! avdec_h264 !  videoanalysis businessdll=/dir/libinferservice.so  ! postprocess ! mqtt username=** userpwd=** servip=** servport=1883 ! fakevideosink
```

当推理得到结果，就会把结果通过 `mqtt` 插件发送到 `mqtt broker`。 `mqtt` 测试工具可以用 `MQTTBox`，订阅主题 `detect_result` 可以查看推理结果。

## 致谢
- [Tengine](https://github.com/OAID/Tengine)
- [GStreamer](https://gstreamer.freedesktop.org/src/)
- [curl](https://github.com/curl/curl.git)
- [zlog](https://github.com/lisongmin/zlog)
- [mosquitto](https://github.com/eclipse/mosquitto)
- [turbojpeg](https://github.com/libjpeg-turbo/libjpeg-turbo)
## License
- [Apache 2.0](https://github.com/OAID/Tengine/blob/tengine-lite/LICENSE)
- LGPL
