#ifndef XOP_RTMP_CLIENT_H
#define XOP_RTMP_CLIENT_H

#include <string>
#include <mutex>
#include "RtmpConnection.h"
#include "net/EventLoop.h"
#include "net/Timestamp.h"

namespace xop
{

class RtmpClient : public Rtmp
{
public:
	using FrameCallback = std::function<void()>;

	RtmpClient & operator=(const RtmpClient &) = delete;
	RtmpClient(const RtmpClient &) = delete;
	RtmpClient(xop::EventLoop *loop);
	~RtmpClient();

	void setFrameCB(const FrameCallback& cb);
	int openUrl(std::string url, int msec = 0);
	void close();
	bool isConnected();

private:
	friend class RtmpConnection;

	xop::EventLoop *m_eventLoop = nullptr;
	TaskScheduler *m_taskScheduler = nullptr;
	std::mutex m_mutex;
	std::shared_ptr<RtmpConnection> m_rtmpConn;
	FrameCallback m_frameCB;
};

}

#endif