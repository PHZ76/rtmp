#ifndef XOP_RTMP_SERVER_H
#define XOP_RTMP_SERVER_H

#include <string>
#include <mutex>
#include "RtmpSession.h"
#include "net/TcpServer.h"

namespace xop
{

class RtmpServer : public TcpServer
{
public:
    RtmpServer(xop::EventLoop *loop, std::string ip, uint16_t port = 1935);
    ~RtmpServer();
    
private:
    virtual TcpConnection::Ptr newConnection(SOCKET sockfd);
    
    //std::unordered_map<MediaSessionId, std::shared_ptr<RtmpSession>> _rtmpSessions;
}; 
    
}

#endif