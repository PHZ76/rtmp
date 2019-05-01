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
    this->handDeleteStream();
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

        uint8_t flags = buf[bytesUsed];
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

        auto& rtmpMsg = m_rtmpMsgs[csid];         
        rtmpMsg.csid = csid;
        
        if (headerLen >= 7) // type 1
        {    
            uint32_t length = readUint24BE((char*)header.length);  
            if(length > 60000)
            {
                return false;
            }    
            
            if(rtmpMsg.length != length)
            {                
                rtmpMsg.length = length;
                rtmpMsg.data.reset(new char[rtmpMsg.length]);   
            }            
            rtmpMsg.index = 0;
            rtmpMsg.typeId = header.typeId;
        }
                          
        if (headerLen >= 11) // type 0
        {
            rtmpMsg.streamId = header.streamId[0] | ((uint32_t) header.streamId[1] << 8) |  \
                                ((uint32_t) header.streamId[2] << 16) | ((uint32_t) header.streamId[3] << 24);
        }
       
        if (headerLen >= 3) // type 2
        {            
            //if(fmt == 0)
            {
                rtmpMsg.timestamp = readUint24BE((char*)header.timestamp);         
            }
/*             else if(fmt == 1)
            {
                rtmpMsg.timestampDelta = readUint24BE((char*)header.timestamp);
            }
            else if(fmt == 2)
            {
                rtmpMsg.timestampDelta = readUint24BE((char*)header.timestamp);
            }     */                  
        }       
               
        /* if (rtmpMsg.timestamp >= 0xffffff) // extended timestamp
        {            
            if((bufSize-bytesUsed) < 4)
            {                    
                break;
            }

            rtmpMsg.extTimestamp = ((uint32_t)buf[bytesUsed+3]) | ((uint32_t)buf[bytesUsed+2] << 8) | \
                                    (uint32_t)(buf[bytesUsed+1] << 16) | ((uint32_t) buf[bytesUsed] << 24);
            bytesUsed += 4;         
        }
        else
        {
            rtmpMsg.extTimestamp = rtmpMsg.timestamp;
        }

        if(fmt == 0)
        {
            rtmpMsg.clock = rtmpMsg.extTimestamp;
        }
        else
        {
            rtmpMsg.clock += rtmpMsg.extTimestamp; //delta
        }            
        */    
        
        uint32_t chunkSize = rtmpMsg.length - rtmpMsg.index;
        if (chunkSize > m_inChunkSize)
            chunkSize = m_inChunkSize;       
        if((bufSize-bytesUsed)  < chunkSize)
        {
            break;
        }

        if(rtmpMsg.index + chunkSize > rtmpMsg.length)
        {
            return false;
        }
                
        memcpy(rtmpMsg.data.get()+rtmpMsg.index, buf+bytesUsed, chunkSize);
        bytesUsed += chunkSize;
        rtmpMsg.index += chunkSize; 
          
        if(fmt == 0)
        {
            rtmpMsg.clock = rtmpMsg.timestamp;                
        }
        else 
        {
            rtmpMsg.clock += rtmpMsg.timestamp;
        }                 

        if(rtmpMsg.index > 0 && rtmpMsg.index == rtmpMsg.length)
        {                           
            if(!handleMessage(rtmpMsg))
            {              
                return false;
            } 
            rtmpMsg.reset();            
        }        

        buffer.retrieve(bytesUsed);  
    } while(1);
    
    return ret;
}

bool RtmpConnection::handleMessage(RtmpMessage& rtmpMsg)
{
    uint8_t *dd = (uint8_t*)rtmpMsg.data.get();
    bool ret = true;  
    switch(rtmpMsg.typeId)
    {        
        case RTMP_VIDEO:
            ret = handleVideo(rtmpMsg);
            break;
        case RTMP_AUDIO:
            ret = handleAudio(rtmpMsg);
            break;
        case RTMP_INVOKE:
            ret = handleInvoke(rtmpMsg);
            break;
        case RTMP_NOTIFY:
            ret = handleNotify(rtmpMsg);
            break;
        case RTMP_FLEX_MESSAGE:
            throw std::runtime_error("unsupported amf3.");
            break;            
        case RTMP_SET_CHUNK_SIZE:           
            m_inChunkSize = readUint32BE(rtmpMsg.data.get());
            break;
        case RTMP_FLASH_VIDEO:
            throw std::runtime_error("unsupported flash video.");
            break;    
        case RTMP_ACK:
            break;            
        case RTMP_ACK_SIZE:
            break;
        case RTMP_USER_EVENT:
            break;
        default:
            printf("unkonw message type : %d\n", rtmpMsg.typeId);
            break;
    }

    return ret;
}

bool RtmpConnection::handleInvoke(RtmpMessage& rtmpMsg)
{   
    bool ret  = true;
    m_amfDec.reset();
    int bytesUsed = m_amfDec.decode((const char *)rtmpMsg.data.get(), rtmpMsg.length, 1);
    if(bytesUsed < 0)
    {      
        return false;
    }

    std::string method = m_amfDec.getString();
    printf("[Method] %s\n", method.c_str());

    if(rtmpMsg.streamId == 0)
    {
        bytesUsed += m_amfDec.decode(rtmpMsg.data.get()+bytesUsed, rtmpMsg.length-bytesUsed);
        if(method == "connect")
        {            
            ret = handleConnect();
        }
        else if(method == "createStream")
        {      
            ret = handleCreateStream();
        }
    }
    else if(rtmpMsg.streamId == m_streamId)
    {
        bytesUsed += m_amfDec.decode((const char *)rtmpMsg.data.get()+bytesUsed, rtmpMsg.length-bytesUsed, 3);
        m_streamName = m_amfDec.getString();
        m_streamPath = "/" + m_app + "/" + m_streamName;
        
        if(rtmpMsg.length > bytesUsed)
        {
            bytesUsed += m_amfDec.decode((const char *)rtmpMsg.data.get()+bytesUsed, rtmpMsg.length-bytesUsed);                      
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
        else if(method == "deleteStream")
        {
            ret = handDeleteStream();
        }
    }
      
    return ret;
}

bool RtmpConnection::handleNotify(RtmpMessage& rtmpMsg)
{   
    if(m_streamId != rtmpMsg.streamId)
    {
        return false;
    }

    m_amfDec.reset();
    int bytesUsed = m_amfDec.decode((const char *)rtmpMsg.data.get(), rtmpMsg.length, 1);
    if(bytesUsed < 0)
    {
        return false;
    }
        
    if(m_amfDec.getString() == "@setDataFrame")
    {
        m_amfDec.reset();
        bytesUsed = m_amfDec.decode((const char *)rtmpMsg.data.get()+bytesUsed, rtmpMsg.length-bytesUsed, 1);
        if(bytesUsed < 0)
        {           
            return false;
        }
       
        if(m_amfDec.getString() == "onMetaData")
        {
            m_amfDec.decode((const char *)rtmpMsg.data.get()+bytesUsed, rtmpMsg.length-bytesUsed);
            m_metaData = m_amfDec.getObjects();
            
            auto sessionPtr = m_rtmpServer->getSession(m_streamPath); 
            if(sessionPtr)
            {   
                sessionPtr->setMetaData(m_metaData);                
                sessionPtr->sendMetaData(m_metaData);                
            }
        }
    }

    return true;
}

bool RtmpConnection::handleVideo(RtmpMessage rtmpMsg)
{  
    if(m_streamPath != "")
    {
        auto sessionPtr = m_rtmpServer->getSession(m_streamPath); 
        if(sessionPtr)
        {   
            sessionPtr->sendMediaData(RTMP_VIDEO, rtmpMsg.clock, rtmpMsg.data, rtmpMsg.length);                  
        }  
    } 
    return true;
}

bool RtmpConnection::handleAudio(RtmpMessage rtmpMsg)
{
    if(m_streamPath != "")
    {
        auto sessionPtr = m_rtmpServer->getSession(m_streamPath); 
        if(sessionPtr)
        {   
            sessionPtr->sendMediaData(RTMP_AUDIO, rtmpMsg.clock, rtmpMsg.data, rtmpMsg.length);     
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

    sendAcknowledgement();
    setPeerBandwidth();   
    setChunkSize();

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

    sendInvokeMessage(CHUNK_INVOKE_ID, m_amfEnc.data(), m_amfEnc.size());
    return true;
}

bool RtmpConnection::handleCreateStream()
{ 
    AmfObjects objects; 
    m_amfEnc.reset();
    m_amfEnc.encodeString("_result", 7);
    m_amfEnc.encodeNumber(m_amfDec.getNumber());
    m_amfEnc.encodeObjects(objects);
    m_amfEnc.encodeNumber(kStreamId);   

    sendInvokeMessage(CHUNK_INVOKE_ID, m_amfEnc.data(), m_amfEnc.size());
    m_streamId = kStreamId;
    return true;
}

bool RtmpConnection::handlePublish()
{
    printf("[Publish] stream path: %s\n", m_streamPath.c_str());        

    AmfObjects objects; 
    m_amfEnc.reset();
    m_amfEnc.encodeString("onStatus", 8);
    m_amfEnc.encodeNumber(0);
    m_amfEnc.encodeObjects(objects);

    bool isError = false;
    if(m_rtmpServer->hasPublisher(m_streamPath)) 
    {
        isError = true;
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.BadName"));
        objects["description"] = AmfObject(std::string("Stream already publishing."));
    }
    else if(m_connStatus == START_PUBLISH)
    {
        isError = true;
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.BadConnection"));
        objects["description"] = AmfObject(std::string("Connection already publishing."));
    }
    /* else if(0)
    {
        //认证处理
    } */
    else
    {
        objects["level"] = AmfObject(std::string("status"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.Start"));
        objects["description"] = AmfObject(std::string("Start publising."));
        m_rtmpServer->addSession(m_streamPath);
    }

    m_amfEnc.encodeObjects(objects);     
    sendInvokeMessage(CHUNK_INVOKE_ID, m_amfEnc.data(), m_amfEnc.size());

    if(isError)
    {
        // close ?
    }
    else
    {
        m_connStatus = START_PUBLISH;
    }

    auto sessionPtr = m_rtmpServer->getSession(m_streamPath); 
    if(sessionPtr)
    {   
        sessionPtr->addClient(shared_from_this());             
    }        
    return true;
}

bool RtmpConnection::handlePlay()
{
    printf("[Play] stream path: %s\n", m_streamPath.c_str());

    AmfObjects objects; 
    m_amfEnc.reset(); 
    m_amfEnc.encodeString("onStatus", 8);
    m_amfEnc.encodeNumber(0);
    m_amfEnc.encodeObjects(objects);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Reset"));
    objects["description"] = AmfObject(std::string("Resetting and playing stream."));
    m_amfEnc.encodeObjects(objects);   
    if(!sendInvokeMessage(CHUNK_INVOKE_ID, m_amfEnc.data(), m_amfEnc.size()))
    {
        return false;
    }

    objects.clear(); 
    m_amfEnc.reset(); 
    m_amfEnc.encodeString("onStatus", 8);
    m_amfEnc.encodeNumber(0);    
    m_amfEnc.encodeObjects(objects);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Start"));
    objects["description"] = AmfObject(std::string("Started playing."));   
    m_amfEnc.encodeObjects(objects);
    if(!sendInvokeMessage(CHUNK_INVOKE_ID, m_amfEnc.data(), m_amfEnc.size()))
    {
        return false;
    }

    m_amfEnc.reset(); 
    m_amfEnc.encodeString("|RtmpSampleAccess", 17);
    m_amfEnc.encodeBoolean(true);
    m_amfEnc.encodeBoolean(true);
    if(!sendNotifyMessage(CHUNK_DATA_ID, m_amfEnc.data(), m_amfEnc.size()))
    {
        return false;
    }
             
    m_connStatus = START_PLAY; 
    
    auto sessionPtr = m_rtmpServer->getSession(m_streamPath); 
    if(sessionPtr)
    {   
        sessionPtr->addClient(shared_from_this());
        m_metaData = sessionPtr->getMetaData();
        if(m_metaData.size() > 0)
        {
            this->sendMetaData(m_metaData); 
        }
    }  
    
    return true;
}

bool RtmpConnection::handlePlay2()
{
    printf("[Play2] stream path: %s\n", m_streamPath.c_str());
    return false;
}

bool RtmpConnection::handDeleteStream()
{
    if(m_streamPath != "")
    {
        auto sessionPtr = m_rtmpServer->getSession(m_streamPath); 
        if(sessionPtr)
        {   
            sessionPtr->removeClient(shared_from_this());                 
        }  
        
        if(sessionPtr->getClients() == 0)
        {
            m_rtmpServer->removeSession(m_streamPath);
        }
        
        m_rtmpMsgs.clear();
    }

	return true;
}

bool RtmpConnection::sendMetaData(AmfObjects& metaData)
{
    if(this->isClosed())
    {
        return false;
    }

    m_amfEnc.reset(); 
    m_amfEnc.encodeString("onMetaData", 10);
    m_amfEnc.encodeECMA(metaData);
    if(!sendNotifyMessage(CHUNK_DATA_ID, m_amfEnc.data(), m_amfEnc.size()))
    {
        return false;
    }

    return true;
}

void RtmpConnection::setPeerBandwidth()
{
    std::shared_ptr<char> data(new char[5]);
    writeUint32BE(data.get(), kPeerBandwidth);
    data.get()[4] = 2;
    RtmpMessage rtmpMsg;
    rtmpMsg.typeId = RTMP_BANDWIDTH_SIZE;
    rtmpMsg.data = data;
    rtmpMsg.length = 5;
    sendRtmpChunks(CHUNK_CONTROL_ID, rtmpMsg); 
}

void RtmpConnection::sendAcknowledgement()
{
    std::shared_ptr<char> data(new char[4]);
    writeUint32BE(data.get(), kAcknowledgementSize);    

    RtmpMessage rtmpMsg;
    rtmpMsg.typeId = RTMP_ACK_SIZE;
    rtmpMsg.data = data;
    rtmpMsg.length = 4;
    sendRtmpChunks(CHUNK_CONTROL_ID, rtmpMsg); 
}

void RtmpConnection::setChunkSize()
{
    m_outChunkSize = kMaxChunkSize;
    
    std::shared_ptr<char> data(new char[4]);
    writeUint32BE((char*)data.get(), m_outChunkSize);    

    RtmpMessage rtmpMsg;
    rtmpMsg.typeId = RTMP_SET_CHUNK_SIZE;
    rtmpMsg.data = data;
    rtmpMsg.length = 4;
    sendRtmpChunks(CHUNK_CONTROL_ID, rtmpMsg);  
}

bool RtmpConnection::sendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payloadSize)
{
    if(this->isClosed())
    {
        return false;
    }

    RtmpMessage rtmpMsg;
    rtmpMsg.typeId = RTMP_INVOKE;
    rtmpMsg.timestamp = 0;
    rtmpMsg.streamId = m_streamId;
    rtmpMsg.data = payload;
    rtmpMsg.length = payloadSize; 
    sendRtmpChunks(csid, rtmpMsg);  
    return true;
}

bool RtmpConnection::sendNotifyMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payloadSize)
{
    if(this->isClosed())
    {
        return false;
    }

    RtmpMessage rtmpMsg;
    rtmpMsg.typeId = RTMP_NOTIFY;
    rtmpMsg.timestamp = 0;
    rtmpMsg.streamId = m_streamId;
    rtmpMsg.data = payload;
    rtmpMsg.length = payloadSize; 
    sendRtmpChunks(csid, rtmpMsg);  
    return true;
}

bool RtmpConnection::sendMediaData(uint8_t type, uint32_t ts, std::shared_ptr<char> payload, uint32_t payloadSize)
{
    if(this->isClosed())
    {
        return false;
    }

    if(!hasKeyFrame)
    {
        uint8_t frameType = (payload.get()[0] >> 4) & 0x0f;
        uint8_t codecId = payload.get()[0] & 0x0f;
        if(frameType == 1)
        {
            hasKeyFrame = true;
        }
        else
        {
            return true;
        }
    }

    RtmpMessage rtmpMsg;
    rtmpMsg.typeId = type;
    rtmpMsg.clock = ts;
    rtmpMsg.streamId = m_streamId;
    rtmpMsg.data = payload;
    rtmpMsg.length = payloadSize; 
    
    if(type == RTMP_AUDIO)
    {
        sendRtmpChunks(CHUNK_AUDIO_ID, rtmpMsg); 
    }
    else if(type == RTMP_VIDEO)
    {
        sendRtmpChunks(CHUNK_VIDEO_ID, rtmpMsg); 
    }
     
    return true;
}

void RtmpConnection::sendRtmpChunks(uint32_t csid, RtmpMessage& rtmpMsg)
{    
    uint32_t bufferOffset = 0, payloadOffset = 0;
    uint32_t capacity = rtmpMsg.length + 1024; 
    std::shared_ptr<char> bufferPtr(new char[capacity]);
    char* buffer = bufferPtr.get();

    bufferOffset += this->createChunkBasicHeader(0, csid, buffer + bufferOffset); //first chunk
    bufferOffset += this->createChunkMessageHeader(0, rtmpMsg, buffer + bufferOffset);
    if(rtmpMsg.clock >= 0xffffff)
    {
        writeUint32BE((char*)buffer + bufferOffset, rtmpMsg.clock);
        bufferOffset += 4;
    }

    while(rtmpMsg.length > 0)
    {
        if(rtmpMsg.length > m_outChunkSize)
        {
            memcpy(buffer+bufferOffset, rtmpMsg.data.get()+payloadOffset, m_outChunkSize);         
            payloadOffset += m_outChunkSize;
            bufferOffset += m_outChunkSize;
            rtmpMsg.length -= m_outChunkSize;
            
            bufferOffset += this->createChunkBasicHeader(3, csid, buffer + bufferOffset);
            if(rtmpMsg.clock >= 0xffffff)
            {
                writeUint32BE(buffer + bufferOffset, rtmpMsg.clock);
                bufferOffset += 4;
            }
        }
        else
        {
            memcpy(buffer+bufferOffset, rtmpMsg.data.get()+payloadOffset, rtmpMsg.length);            
            bufferOffset += rtmpMsg.length;
            rtmpMsg.length = 0;            
            break;
        }
    }

    this->send(bufferPtr, bufferOffset);
}

int RtmpConnection::createChunkBasicHeader(uint8_t fmt, uint32_t csid, char* buf)
{
    int len = 0;

    if (csid >= 64 + 255) 
    {
        buf[len++] = (fmt << 6) | 1;
        buf[len++] = (csid - 64) & 0xFF;
        buf[len++] = ((csid - 64) >> 8) & 0xFF;
    } 
    else if (csid >= 64) 
    {
        buf[len++] = (fmt << 6) | 0;
        buf[len++] = (csid - 64) & 0xFF;
    } 
    else 
    {
        buf[len++] = (fmt << 6) | csid;
    }
    return len;
}

int RtmpConnection::createChunkMessageHeader(uint8_t fmt, RtmpMessage& rtmpMsg, char* buf)
{   
    int len = 0;    
    if (fmt <= 2) 
    {
        if(rtmpMsg.clock < 0xffffff)
        {
           writeUint24BE((char*)buf, rtmpMsg.clock);    
        }
        else
        {
            writeUint24BE((char*)buf, 0xffffff);    
        }
        len += 3;
    }

    if (fmt <= 1) 
    {
        writeUint24BE((char*)buf + len, rtmpMsg.length);
        len += 3;
        buf[len++] = rtmpMsg.typeId;
    }

    if (fmt == 0) 
    {
        writeUint32LE((char*)buf + len, rtmpMsg.streamId);    
        len += 4;
    }

    return len;
}

