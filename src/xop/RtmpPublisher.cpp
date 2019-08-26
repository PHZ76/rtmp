#include "RtmpPublisher.h"
#include "net/Logger.h"
#include "net/log.h"

using namespace xop;

RtmpPublisher::RtmpPublisher(xop::EventLoop *loop)
	: m_eventLoop(loop)
{

}

RtmpPublisher::~RtmpPublisher()
{

}

int RtmpPublisher::openUrl(std::string url)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (this->parseRtmpUrl(url) != 0)
	{
		LOG_INFO("[RtmpPublisher] rtmp url:%s was illegal.\n", url.c_str());
		return -1;
	}

	//LOG_INFO("[RtmpPublisher] ip:%s, port:%hu, stream path:%s\n", m_ip.c_str(), m_port, m_streamPath.c_str());

	SOCKET sockfd = 0;

	if (m_rtmpConn != nullptr)
	{		
		std::shared_ptr<RtmpConnection> rtmpConn = m_rtmpConn;
		sockfd = rtmpConn->fd();
		m_eventLoop->addTriggerEvent([sockfd, rtmpConn]() {
			rtmpConn->close();
		});
	}

	TcpSocket tcpSocket;
	tcpSocket.create();
	if (!tcpSocket.connect(m_ip, m_port, 3000)) // 3s timeout
	{
		tcpSocket.close();
		return -1;
	}

	
	sockfd = tcpSocket.fd();
	//m_rtmpConn.reset(new RtmpConnection(this, m_eventLoop->getTaskScheduler().get(), sockfd));
	m_eventLoop->addTriggerEvent([sockfd, this]() {
		//rtmpConn->sendOptions(RtspConnection::RTSP_PUSHER);
	});

	return 0;

	return 0;
}
