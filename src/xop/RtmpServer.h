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
        
    void addSession(std::string streamPath);
    void removeSession(std::string streamPath);
    
    RtmpSession::Ptr getSession(std::string streamPath);
    bool hasSession(std::string streamPath);
    bool hasPublisher(std::string streamPath);
    
private:
    virtual TcpConnection::Ptr newConnection(SOCKET sockfd);
    
    std::mutex m_mutex;
    std::unordered_map<std::string, RtmpSession::Ptr> m_rtmpSessions; 
}; 
    
}

#endif