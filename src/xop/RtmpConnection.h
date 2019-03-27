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
    uint8_t msgLen[3];
    uint8_t msgType;
    uint8_t msgStreamId[4]; //小端格式
};

struct RtmpMessage 
{
    uint32_t csid = 0;
    uint32_t len = 0;
    uint64_t timestamp = 0;
    uint8_t  msgType = 0;
    uint32_t msgStreamId = 0;
    uint32_t index = 0;
    std::shared_ptr<uint8_t> data;
    
    void reset()
    {
        len = 0;
        index = 0;
        data.reset();
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
        START_PULL,
        START_PUBLISH
    };
    
    enum RtmpMessagType
    {
        RTMP_CHUNK_SIZE     = 0x1 , //设置块大小
        RTMP_AOBRT_MESSAGE  = 0X2 , //终止消息
        RTMP_ACK            = 0x3 , //确认
        RTMP_ACK_SIZE       = 0x5 , //窗口大小确认
        RTMP_BANDWIDTH_SIZE = 0x6 , //设置对端带宽
        
        RTMP_FLEX_MESSAGE   = 0x11, //amf3
        RTMP_NOTIFY         = 0x12, //
        RTMP_INVOKE         = 0x14, //amf0
    };
    
    enum ChunkSreamId
    {
        CHUNK_CONTROL_ID = 2, // 控制消息
        CHUNK_RESULT_ID  = 3, 
        CHUNK_STREAM_ID  = 4, 
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
    
private:
    bool onRead(BufferReader& buffer);
    void onClose();
    
    bool handleHandshake(BufferReader& buffer);
    bool handleChunk(BufferReader& buffer);
    bool handleMessage(RtmpMessage& rtmpMessage);
    bool handleInvoke(RtmpMessage& rtmpMessage);
    bool handleNotify(RtmpMessage& rtmpMessage);
    
    bool handleConnect();
    bool handleFCPublish();
    bool handleCreateStream();
    bool handlePublish();
    bool handlePlay();
    bool handlePlay2();
    void setPeerBandwidth(uint32_t size);
    void sendAcknowledgement(uint32_t size);
    void setChunkSize(uint32_t size);
    
    bool sendRtmpMessage(uint8_t* data, uint32_t size, uint32_t csid, uint8_t msgType, uint32_t msgStreamId=0, uint32_t timestamp=0);
    
    RtmpServer *m_rtmpServer;
    TaskScheduler *m_taskScheduler;
    std::shared_ptr<xop::Channel> m_channelPtr;
    
    ConnectionStatus m_connStatus = HANDSHAKE_C0C1;    
    const int kRtmpVersion = 0x03;
    const int kChunkMessageLen[4] = {11, 7, 3, 0};
    uint32_t m_inChunkSize = 128;
    uint32_t m_outChunkSize = 128;
    uint32_t m_streamId = 1;
    
    AmfDecoder m_amfDec;
    AmfEncoder m_amfEnc;
    std::string m_app;
    std::string m_streamName;
    std::string m_streamPath;
    AmfObjects m_metaData;
    std::map<int, RtmpMessage> m_rtmpMessages;  
    
    const uint32_t kPeerBandwidth       = 5000000;
    const uint32_t kAcknowledgementSize = 5000000;
    const uint32_t kMaxChunkSize        = 60000;
};
      
}

#endif
