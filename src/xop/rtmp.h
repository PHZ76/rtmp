#ifndef XOP_RTMP_H
#define XOP_RTMP_H

#define RTMP_VERSION           0X3

#define RTMP_SET_CHUNK_SIZE    0x1 //设置块大小
#define RTMP_AOBRT_MESSAGE     0X2 //终止消息
#define RTMP_ACK               0x3 //确认
#define RTMP_USER_EVENT        0x4 //用户控制消息
#define RTMP_ACK_SIZE          0x5 //窗口大小确认
#define RTMP_BANDWIDTH_SIZE    0x6 //设置对端带宽
#define RTMP_AUDIO             0x08
#define RTMP_VIDEO             0x09
#define RTMP_FLEX_MESSAGE      0x11 //amf3
#define RTMP_NOTIFY            0x12
#define RTMP_INVOKE            0x14 //amf0
#define RTMP_FLASH_VIDEO       0x16

#define RTMP_CHUNK_TYPE_0      0 // 11
#define RTMP_CHUNK_TYPE_1      1 // 7
#define RTMP_CHUNK_TYPE_2      2 // 3
#define RTMP_CHUNK_TYPE_3      3 // 0

#define RTMP_CHUNK_CONTROL_ID  2 
#define RTMP_CHUNK_INVOKE_ID   3
#define RTMP_CHUNK_AUDIO_ID    4
#define RTMP_CHUNK_VIDEO_ID    5
#define RTMP_CHUNK_DATA_ID     6

#define RTMP_CODEC_ID_H264     7
#define RTMP_CODEC_ID_ACC      10
#define RTMP_CODEC_ID_G711A    7
#define RTMP_CODEC_ID_G711U    8

#endif
 