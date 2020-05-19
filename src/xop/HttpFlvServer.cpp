#include "HttpFlvServer.h"
#include "RtmpServer.h"
#include "net/SocketUtil.h"
#include "net/Logger.h"

using namespace xop;

HttpFlvServer::HttpFlvServer(xop::EventLoop* event_loop)
	: TcpServer(event_loop)
{

}

HttpFlvServer::~HttpFlvServer()
{

}

void HttpFlvServer::Attach(RtmpServer *rtmp_server)
{
	std::lock_guard<std::mutex> locker(mutex_);
	rtmp_server_ = rtmp_server;
}

TcpConnection::Ptr HttpFlvServer::OnConnect(SOCKET sockfd)
{
	return std::make_shared<HttpFlvConnection>(rtmp_server_, event_loop_->GetTaskScheduler().get(), sockfd);
}

