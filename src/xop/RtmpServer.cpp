#include "RtmpServer.h"
#include "RtmpConnection.h"
#include "net/SocketUtil.h"
#include "net/Logger.h"

using namespace xop;

RtmpServer::RtmpServer(EventLoop* loop, std::string ip, uint16_t port)
	: TcpServer(loop, ip, port)
{
    if (this->start() != 0)
    {
        LOG_INFO("RTSP Server listening on %d failed.", port);
    }
}

RtmpServer::~RtmpServer()
{
    
}

TcpConnection::Ptr RtmpServer::newConnection(SOCKET sockfd)
{
    return std::make_shared<RtmpConnection>(this, _eventLoop->getTaskScheduler().get(), sockfd);
}

