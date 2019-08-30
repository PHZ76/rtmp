#include "HttpFlvConnection.h"

using namespace xop;

HttpFlvConnection::HttpFlvConnection(TaskScheduler* taskScheduler, SOCKET sockfd)
	: TcpConnection(taskScheduler, sockfd)
{

}

HttpFlvConnection::~HttpFlvConnection()
{

}