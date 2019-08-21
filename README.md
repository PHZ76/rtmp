# RtmpServer

目前情况
-
* 支持 Windows 和 Linux 平台
* 支持 H.264 和 AAC 转发
* 支持 GOP 缓存

编译运行
-
* make
* ./rtmp_server

测试
-
* ffmpeg.exe -re -i test.h264 -f flv rtmp://127.0.0.1/live/stream
* ffplay.exe rtmp://127.0.0.1/live/stream

后续计划
-
* RTMP 推流器
* 多线程支持
* HEVC 转发支持

感谢
* [Node-Media-Server](https://github.com/illuspas/Node-Media-Server)

联系方式
-
* 2235710879@qq.com