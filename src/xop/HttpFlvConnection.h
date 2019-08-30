#ifndef XOP_HTTP_FLV_CONNECTION_H
#define XOP_HTTP_FLV_CONNECTION_H

#include "net/EventLoop.h"
#include "net/TcpConnection.h"

namespace xop
{

class HttpFlvConnection : public TcpConnection
{
public:
	HttpFlvConnection(TaskScheduler* taskScheduler, SOCKET sockfd);
	~HttpFlvConnection();
};

}


#endif