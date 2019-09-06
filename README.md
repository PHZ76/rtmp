# rtmp

目前情况
-
* 支持 Windows 和 Linux 平台
* 支持 RTMP, HTTP-FLV 协议
* 支持 H.264 和 AAC 转发
* 支持 GOP 缓存
* 支持 RTMP 推流

编译运行
-
* make
* ./rtmp_server

推流器测试
-
* [DesktopSharing](https://github.com/PHZ76/DesktopSharing)

服务器测试
-
* ffmpeg.exe -re -i test.h264 -f flv rtmp://127.0.0.1/live/stream
* ffplay.exe rtmp://127.0.0.1:1935/live/stream
* ffplay.exe http://127.0.0.1:8000/live/stream.flv

后续计划
-
* HLS支持
* 性能优化

联系方式
-
* penghaoze76@qq.com