#ifndef XOP_RTMP_CONNECTION_H
#define XOP_RTMP_CONNECTION_H

#include "net/EventLoop.h"
#include "net/TcpConnection.h"
#include "amf.h"
#include <vector>

namespace xop
{

class RtmpServer;

// chunk header: basic_header + rtmp_message_header 
struct RtmpMessageHeader
{
    uint8_t timestamp[3];
    uint8_t length[3];
    uint8_t typeId;
    uint8_t streamId[4]; //小端格式
};

struct RtmpMessage 
{
    uint32_t timestamp = 0;
    uint32_t timestampDelta = 0;
    uint32_t length = 0;
    uint8_t  typeId = 0;
    uint32_t streamId = 0;
    uint32_t extTimestamp = 0;   

    uint8_t  csid = 0;
    uint64_t clock = 0;    
    uint32_t index = 0;
    std::shared_ptr<char> data = nullptr;

    void reset()
    {        
        index = 0;       
    }
};

class RtmpConnection : public TcpConnection
{
public:    
    enum ConnectionStatus
    {
        HANDSHAKE_C0C1, 
        HANDSHAKE_C2,
        HANDSHAKE_COMPLETE,
        START_PLAY,
        START_PUBLISH
    };
    
    enum RtmpMessagType
    {
        RTMP_SET_CHUNK_SIZE     = 0x1 , //设置块大小
        RTMP_AOBRT_MESSAGE      = 0X2 , //终止消息
        RTMP_ACK                = 0x3 , //确认
        RTMP_USER_EVENT         = 0x4 , //用户控制消息
        RTMP_ACK_SIZE           = 0x5 , //窗口大小确认
        RTMP_BANDWIDTH_SIZE     = 0x6 , //设置对端带宽
        RTMP_AUDIO	            = 0x08,
        RTMP_VIDEO              = 0x09,
        RTMP_FLEX_MESSAGE       = 0x11, //amf3
        RTMP_NOTIFY             = 0x12, //
        RTMP_INVOKE             = 0x14, //amf0
        RTMP_FLASH_VIDEO        = 0x16,
    };
    
    enum ChunkSreamId
    {
        CHUNK_CONTROL_ID    = 2, // 控制消息
        CHUNK_INVOKE_ID     = 3, 
        CHUNK_AUDIO_ID      = 4, 
        CHUNK_VIDEO_ID      = 5, 
        CHUNK_DATA_ID       = 6,
    };
    
    RtmpConnection() = delete;
    RtmpConnection(RtmpServer* rtmpServer, TaskScheduler* taskScheduler, SOCKET sockfd);
    ~RtmpConnection();

    std::string getStreamPath() const
    { return m_streamPath; }

    std::string getStreamName() const
    { return m_streamName; }

    std::string getApp() const
    { return m_app; }

    AmfObjects getMetaData() const 
    { return m_metaData; }

    bool isPlayer() const 
    { return m_connStatus==START_PLAY; }

    bool isPublisher() const 
    { return m_connStatus==START_PUBLISH; }        
    
private:
    friend class RtmpSession;

    bool onRead(BufferReader& buffer);
    void onClose();

    bool handleHandshake(BufferReader& buffer);
    bool handleChunk(BufferReader& buffer);
    bool handleMessage(RtmpMessage& rtmpMsg);
    bool handleInvoke(RtmpMessage& rtmpMsg);
    bool handleNotify(RtmpMessage& rtmpMsg);
    bool handleVideo(RtmpMessage rtmpMsg);
    bool handleAudio(RtmpMessage rtmpMsg);

    bool handleConnect();
    bool handleCreateStream();
    bool handlePublish();
    bool handlePlay();
    bool handlePlay2();
    bool handDeleteStream();

    void setPeerBandwidth();
    void sendAcknowledgement();
    void setChunkSize();

    bool sendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payloadSize);
    bool sendNotifyMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payloadSize);   
    bool sendMetaData(AmfObjects& metaData);
    bool sendMediaData(uint8_t type, uint32_t ts, std::shared_ptr<char> payload, uint32_t payloadSize);
    void sendRtmpChunks(uint32_t csid, RtmpMessage& rtmpMsg);
    int createChunkBasicHeader(uint8_t fmt, uint32_t csid, char* buf);
    int createChunkMessageHeader(uint8_t fmt, RtmpMessage& rtmpMsg, char* buf);   

    RtmpServer *m_rtmpServer;
    TaskScheduler *m_taskScheduler;
    std::shared_ptr<xop::Channel> m_channelPtr;

    ConnectionStatus m_connStatus = HANDSHAKE_C0C1;    
    const int kRtmpVersion = 0x03;
    const int kChunkMessageLen[4] = {11, 7, 3, 0};
    uint32_t m_inChunkSize = 128;
    uint32_t m_outChunkSize = 128;
    uint32_t m_streamId = 0;
    bool hasKeyFrame = false; 
    AmfDecoder m_amfDec;
    AmfEncoder m_amfEnc;
    std::string m_app;
    std::string m_streamName;
    std::string m_streamPath;
    AmfObjects m_metaData;
    std::map<int, RtmpMessage> m_rtmpMsgs;  

    const uint32_t kPeerBandwidth       = 5000000;
    const uint32_t kAcknowledgementSize = 5000000;
    const uint32_t kMaxChunkSize        = 60000;
    const uint32_t kStreamId            = 1;
};
      
}

#endif
