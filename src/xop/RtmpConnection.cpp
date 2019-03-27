#include "RtmpConnection.h"
#include "RtmpServer.h"
#include <random>

using namespace xop;

RtmpConnection::RtmpConnection(RtmpServer *rtmpServer, TaskScheduler *taskScheduler, SOCKET sockfd)
    : TcpConnection(taskScheduler, sockfd)
    , m_rtmpServer(rtmpServer)
    , m_taskScheduler(taskScheduler)
    , m_channelPtr(new Channel(sockfd))
    
{
    this->setReadCallback([this](std::shared_ptr<TcpConnection> conn, xop::BufferReader& buffer) {
        return this->onRead(buffer);
    });

    this->setCloseCallback([this](std::shared_ptr<TcpConnection> conn) {
        this->onClose();
    });
}

RtmpConnection::~RtmpConnection()
{
    
}

bool RtmpConnection::onRead(BufferReader& buffer)
{   
    if (buffer.readableBytes() <= 0)
    {        
        return false; //close
    }

    bool ret = true;
    if(m_connStatus >= HANDSHAKE_COMPLETE)
    {
        ret = handleChunk(buffer);
    }
    else if(m_connStatus == HANDSHAKE_C0C1 
        || m_connStatus == HANDSHAKE_C2)
    {
        ret = this->handleHandshake(buffer);
        if(m_connStatus==HANDSHAKE_COMPLETE && buffer.readableBytes()>0)
        {
            ret = handleChunk(buffer);
        }
    }
    
    return ret;
}

void RtmpConnection::onClose()
{
    
}

bool RtmpConnection::handleHandshake(BufferReader& buffer)
{
    uint8_t *buf = (uint8_t*)buffer.peek();
    uint32_t bufSize = buffer.readableBytes();
    uint32_t pos = 0;
    std::shared_ptr<char> res;
    uint32_t resSize = 0;
    
    if(m_connStatus == HANDSHAKE_C0C1)
    {
        if(bufSize < 1537) //c0c1
        {           
            return true;
        }
        else
        {
            if(buf[0] != kRtmpVersion)
            {
               return false; 
            }
            
            pos += 1537;
            resSize = 1+ 1536 + 1536;
            res.reset(new char[resSize]); //S0 S1 S2      
            memset(res.get(), 0, 1537);
            res.get()[0] = kRtmpVersion; 
            
            // 填充随机数据
            std::random_device rd;
            char *p = res.get(); p += 9;
            for(int i=0; i<1528; i++)
            {
                *p++ = rd();
            }
            memcpy(p, buf+1, 1536);
            m_connStatus = HANDSHAKE_C2;
        }
    } 
    else if(m_connStatus == HANDSHAKE_C2)
    {
        if(bufSize < 1536) //c0c1
        {           
            return true;
        }
        else 
        {
            pos = 1536;            
            m_connStatus = HANDSHAKE_COMPLETE;
        }
    }
    else 
    {
        return false;
    }
    
    buffer.retrieve(pos);
    if(resSize > 0)
    {
        this->send(res, resSize);
    }
    
    return true;
}

bool RtmpConnection::handleChunk(BufferReader& buffer)
{
    bool ret = true; 
    do
    {      
        uint8_t *buf = (uint8_t*)buffer.peek();
        uint32_t bufSize = buffer.readableBytes();
        uint32_t bytesUsed = 0;
        if(bufSize == 0)
        {
            break;
        }
    
        uint8_t flags = buf[0];
        bytesUsed += 1;        
        
        uint8_t csid = flags & 0x3f; // chunk stream id
        if(csid == 0) // csid [64, 319]
        {
            if((bufSize-bytesUsed) < 2)
                break;
            csid += buf[bytesUsed] + 64;
            bytesUsed += 1;
        }
        else if(csid == 1) // csid [64, 65599]
        {
            if((bufSize-bytesUsed) < 3)
                break;
            bytesUsed += buf[bytesUsed+1]*255 + buf[bytesUsed] + 64;
            bytesUsed += 2;
        }  
        
        uint8_t fmt = flags >> 6; // message_header_type
        if(fmt >= 4) 
        {
            return false;
        }
     
        uint32_t headerLen = kChunkMessageLen[fmt]; // basic_header + message_header 
        if((bufSize-bytesUsed) < headerLen)
        {
            break;
        }         
        
        RtmpMessageHeader header;  
        memcpy(&header, buf+bytesUsed, headerLen);
        bytesUsed += headerLen;
                
        auto& rtmpMsg = m_rtmpMessages[csid];                
        if (headerLen >= 7) // type 1
        {    
            uint32_t msgLen = header.msgLen[2] | (header.msgLen[1] << 8) | (header.msgLen[0] << 16);          
            if(rtmpMsg.len != msgLen)
            {
                rtmpMsg.len = msgLen;
                rtmpMsg.data.reset(new uint8_t[rtmpMsg.len]);
            }            
            rtmpMsg.index = 0;
			rtmpMsg.csid = csid;
            rtmpMsg.msgType = header.msgType;
		}
             
        if (headerLen >= 3) // type 2
        {
            uint64_t ts = header.timestamp[2] | ((uint32_t) header.timestamp[1] << 8) | \
                          ((uint32_t) header.timestamp[0] << 16);
			uint64_t ext_ts = 0;
            if (ts == 0xffffff) // extended timestamp
            {
				if((bufSize-bytesUsed)  < 4)
                {                    
                    break;
                }
   
                ext_ts = ((uint32_t)buf[bytesUsed+3]) | ((uint32_t)buf[bytesUsed+2] << 8) | \
                         (uint32_t)(buf[bytesUsed+1] << 16) | ((uint32_t) buf[bytesUsed] << 24);
                bytesUsed += 4;         
                rtmpMsg.timestamp = ext_ts;
			}
            else if(ts < 0xffffff)
            {
				ts += rtmpMsg.timestamp;
			}
			rtmpMsg.timestamp = ts;
		}       
                       
        if (headerLen >= 11) // type 0
        {
            rtmpMsg.msgStreamId = header.msgStreamId[0] | ((uint32_t) header.msgStreamId[1] << 8) |  \
                                ((uint32_t) header.msgStreamId[2] << 16) | ((uint32_t) header.msgStreamId[3] << 24);
		}
        
        uint32_t chunkSize = rtmpMsg.len - rtmpMsg.index;
		if (chunkSize > m_inChunkSize)
			chunkSize = m_inChunkSize;     
        if((bufSize-bytesUsed)  < chunkSize)
        {
            break;
        }
        
        if(rtmpMsg.index + chunkSize > rtmpMsg.len)
        {
            return false;
        }
        
        memcpy(rtmpMsg.data.get()+rtmpMsg.index, buf+bytesUsed, chunkSize);
        bytesUsed += chunkSize;
        rtmpMsg.index += chunkSize;        
        if(rtmpMsg.index == rtmpMsg.len)
        {            
            if(!handleMessage(rtmpMsg))
            {              
                return false;
            }            
        }        
        
        buffer.retrieve(bytesUsed);    
    } while(1);

    
    return ret;
}

bool RtmpConnection::handleMessage(RtmpMessage& rtmpMessage)
{
    bool ret = true;  
    switch(rtmpMessage.msgType)
    {        
        case RTMP_INVOKE:
            ret = handleInvoke(rtmpMessage);
            break;
        case RTMP_NOTIFY:
            ret = handleNotify(rtmpMessage);
            break;
        case RTMP_FLEX_MESSAGE:
            throw std::runtime_error("unsupported amf3.");
            break;            
        case RTMP_CHUNK_SIZE:
            m_inChunkSize = readInt32BE((char*)rtmpMessage.data.get());
            break;
        default:
            break;
    }
    
    return ret;
}

bool RtmpConnection::handleInvoke(RtmpMessage& rtmpMessage)
{   
    bool ret  = true;
    m_amfDec.reset();
    int bytesUsed = m_amfDec.decode((const char *)rtmpMessage.data.get(), rtmpMessage.len, 1);
    if(bytesUsed < 0)
    {      
        return false;
    }
    
    std::string method = m_amfDec.getString();
    printf("[Method] %s \n", method.c_str());

    if(rtmpMessage.msgStreamId == 0)
    {
        bytesUsed += m_amfDec.decode((const char *)rtmpMessage.data.get()+bytesUsed, rtmpMessage.len-bytesUsed);
        if(method == "connect")
        {            
            ret = handleConnect();
        }
        else if(method == "FCPublish")
        {
            ret = handleFCPublish();
        }
        else if(method == "createStream")
        {      
            ret = handleCreateStream();
        }
    }
    else if(rtmpMessage.msgStreamId == m_streamId)
    {
        bytesUsed += m_amfDec.decode((const char *)rtmpMessage.data.get()+bytesUsed, rtmpMessage.len-bytesUsed, 1);
        m_streamName = m_amfDec.getString();
        m_streamPath = "/" + m_app + "/" + m_streamName;
        
        if(rtmpMessage.len > bytesUsed)
        {
            bytesUsed += m_amfDec.decode((const char *)rtmpMessage.data.get()+bytesUsed, rtmpMessage.len-bytesUsed);                      
        }
       
        if(method == "publish")
        {            
            ret = handlePublish();
        }
        else if(method == "play")
        {          
            ret = handlePlay();
        }
        else if(method == "play2")
        {         
            ret = handlePlay2();
        }
    }
      
    return ret;
}

bool RtmpConnection::handleNotify(RtmpMessage& rtmpMessage)
{
    if(m_streamId != rtmpMessage.msgStreamId)
    {
        return false;
    }
    
    m_amfDec.reset();
    int bytesUsed = m_amfDec.decode((const char *)rtmpMessage.data.get(), rtmpMessage.len, 1);
    if(bytesUsed < 0)
    {
        return false;
    }
        
    if(m_amfDec.getString() == "@setDataFrame")
    {
        m_amfDec.reset();
        bytesUsed = m_amfDec.decode((const char *)rtmpMessage.data.get()+bytesUsed, rtmpMessage.len-bytesUsed, 1);
        if(bytesUsed < 0)
        {           
            return false;
        }
       
        if(m_amfDec.getString() == "onMetaData")
        {
            m_amfDec.decode((const char *)rtmpMessage.data.get()+bytesUsed, rtmpMessage.len-bytesUsed);
            m_metaData = m_amfDec.getObjects();
        }
    }
        
    return true;
}

bool RtmpConnection::handleConnect()
{
    if(!m_amfDec.hasObject("app"))
    {
        return false;
    }
    
    AmfObject amfObj = m_amfDec.getObject("app");
    m_app = amfObj.amf_string;
    if(m_app == "")
    {
        return false;
    }
    
    sendAcknowledgement(kAcknowledgementSize);
    setPeerBandwidth(kPeerBandwidth);   
    setChunkSize(kMaxChunkSize);
   
    AmfObjects objects;    
    m_amfEnc.reset();
    m_amfEnc.encodeString("_result", 7);
    m_amfEnc.encodeNumber(m_amfDec.getNumber());
   
    objects["fmsVer"] = AmfObject(std::string("FMS/4,5,0,297"));
    objects["capabilities"] = AmfObject(255.0);
    objects["mode"] = AmfObject(1.0);
    m_amfEnc.encodeObjects(objects);
    objects.clear();
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetConnection.Connect.Success"));
    objects["description"] = AmfObject(std::string("Connection succeeded."));
    objects["objectEncoding"] = AmfObject(0.0);
    m_amfEnc.encodeObjects(objects);

    //控制消息: msg stream id为0, chunk msg id 为2    
    sendRtmpMessage(m_amfEnc.data().get(), m_amfEnc.size(), CHUNK_RESULT_ID, RTMP_INVOKE, 0);
    
    return true;
}

bool RtmpConnection::handleFCPublish()
{
    std::string path = m_amfDec.getString();

    AmfObjects objects; 
    m_amfEnc.reset();
    m_amfEnc.encodeString("onFCPublish", 11);
    m_amfEnc.encodeNumber(0);
    m_amfEnc.encodeObjects(objects);
    
    objects["code"] = AmfObject(std::string("NetStream.Publish.Start"));
    objects["description"] = AmfObject(path);
    m_amfEnc.encodeObjects(objects); 
    sendRtmpMessage(m_amfEnc.data().get(), m_amfEnc.size(), CHUNK_CONTROL_ID, RTMP_INVOKE, m_streamId);
    
    m_amfEnc.reset();
    objects.clear();
    m_amfEnc.encodeString("_result", 7);
    m_amfEnc.encodeNumber(m_amfDec.getNumber());
    m_amfEnc.encodeObjects(objects);
    m_amfEnc.encodeObjects(objects);
    sendRtmpMessage(m_amfEnc.data().get(), m_amfEnc.size(), CHUNK_RESULT_ID, RTMP_INVOKE, 0);
    
    return true;
}

bool RtmpConnection::handleCreateStream()
{
    AmfObjects objects; 
    m_amfEnc.reset();
    m_amfEnc.encodeString("_result", 7);
    m_amfEnc.encodeNumber(m_amfDec.getNumber());
    m_amfEnc.encodeObjects(objects);
    m_amfEnc.encodeNumber(m_streamId);   
    sendRtmpMessage(m_amfEnc.data().get(), m_amfEnc.size(), CHUNK_RESULT_ID, RTMP_INVOKE, 0);
    
    return true;
}

bool RtmpConnection::handlePublish()
{
    printf("[Publish] stream path: %s\n", m_streamPath.c_str());
    
    m_connStatus = START_PUBLISH;
    
    AmfObjects objects; 
    m_amfEnc.reset();
    m_amfEnc.encodeString("onStatus", 8);
    m_amfEnc.encodeNumber(0);
    m_amfEnc.encodeObjects(objects);
    
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Publish.Start"));
    objects["description"] = AmfObject(std::string("Start publising."));
    m_amfEnc.encodeObjects(objects); 
    sendRtmpMessage(m_amfEnc.data().get(), m_amfEnc.size(), CHUNK_STREAM_ID, RTMP_INVOKE, m_streamId);
    
    m_amfEnc.reset();
    objects.clear();
    m_amfEnc.encodeString("_result", 7);
    m_amfEnc.encodeNumber(m_amfDec.getNumber());
    m_amfEnc.encodeObjects(objects);
    m_amfEnc.encodeObjects(objects);
    sendRtmpMessage(m_amfEnc.data().get(), m_amfEnc.size(), CHUNK_RESULT_ID, RTMP_INVOKE, 0);
    
    return true;
}

bool RtmpConnection::handlePlay()
{
    printf("[Play] stream path: %s\n", m_streamPath.c_str());
    return true;
}

bool RtmpConnection::handlePlay2()
{
    printf("[Play] stream path: %s\n", m_streamPath.c_str());
    return true;
}

void RtmpConnection::setPeerBandwidth(uint32_t size)
{
    uint8_t data[5];
    writeInt32BE((char*)data, size);
    data[4] = 2; //dynamic
    sendRtmpMessage(data, 5, CHUNK_CONTROL_ID, RTMP_BANDWIDTH_SIZE, 0);
}

void RtmpConnection::sendAcknowledgement(uint32_t size)
{
    uint8_t data[4];
    writeInt32BE((char*)data, size);
    sendRtmpMessage(data, 4, CHUNK_CONTROL_ID, RTMP_ACK_SIZE, 0);
}

void RtmpConnection::setChunkSize(uint32_t size)
{
    uint8_t data[4];
    writeInt32BE((char*)data, size);
    m_outChunkSize = size;
    sendRtmpMessage(data, 4, CHUNK_CONTROL_ID, RTMP_CHUNK_SIZE, 0);
}

bool RtmpConnection::sendRtmpMessage(uint8_t* data, uint32_t size, uint32_t csid, uint8_t msgType, uint32_t msgStreamId, uint32_t timestamp)
{
    if(this->isClosed())
    {
        return false;
    }
        
    RtmpMessageHeader rtmpMsgHdr;
    memset(&rtmpMsgHdr, 0, sizeof(RtmpMessageHeader));
    
    rtmpMsgHdr.msgType = msgType;
    writeInt24BE((char*)rtmpMsgHdr.timestamp, timestamp);
    writeInt24BE((char*)rtmpMsgHdr.msgLen, size);
    writeInt32LE((char*)rtmpMsgHdr.msgStreamId, msgStreamId);

    uint8_t chunkType = 0;  // first chunk type -- 0
    uint8_t chunkBasicHeader = (csid & 0x3f) | (chunkType << 6); 
    int bytesUsed = 0;
    while(size > bytesUsed)
    {             
        if(this->isClosed())
        {
            return false;
        }
        
        if(bytesUsed == 0) //发送第一个块
        {           
            this->send((char *)&chunkBasicHeader, 1);
            this->send((char *)&rtmpMsgHdr, sizeof(rtmpMsgHdr));
        }
        else if(bytesUsed > 0)
        {
            chunkType = 3;    
            chunkBasicHeader = (csid & 0x3f) | (chunkType << 6); 
            this->send((char *)&chunkBasicHeader, 1);    
        }
        
        uint32_t chunkSize = m_outChunkSize;
        if(size - bytesUsed < m_outChunkSize)
        {
            chunkSize = size - bytesUsed;
        }        
        this->send((char *)data+bytesUsed, chunkSize);
        bytesUsed += chunkSize;
    }
    return true;
}

