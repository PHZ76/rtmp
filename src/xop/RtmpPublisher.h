#ifndef XOP_RTMP_PUBLISHER_H
#define XOP_RTMP_PUBLISHER_H

#include <string>
#include <mutex>
#include "RtmpConnection.h"
#include "net/EventLoop.h"

namespace xop
{

class RtmpPublisher : public Rtmp
{
public:
	RtmpPublisher() = delete;
	RtmpPublisher(xop::EventLoop *loop);
	~RtmpPublisher();

	int openUrl(std::string url);
	void close();
	bool isConnected();

private:
	friend class RtmpConnection;

	xop::EventLoop *m_eventLoop;
	std::mutex m_mutex;

	std::shared_ptr<RtmpConnection> m_rtmpConn;
};

}

#endif

