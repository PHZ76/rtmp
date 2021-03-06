#ifndef XOP_RTMP_SERVER_H
#define XOP_RTMP_SERVER_H

#include <string>
#include <mutex>
#include "net/TcpServer.h"
#include "rtmp.h"
#include "RtmpSession.h"

namespace xop
{

class RtmpServer : public TcpServer, public Rtmp, public std::enable_shared_from_this<RtmpServer>
{
public:
	using EventCallback = std::function<void(std::string event_type, std::string stream_path)>;

	static std::shared_ptr<RtmpServer> Create(xop::EventLoop* event_loop);
    ~RtmpServer();
       
	void SetEventCallback(EventCallback event_cb);

private:
	friend class RtmpConnection;
	friend class HttpFlvServer;

	RtmpServer(xop::EventLoop *event_loop);
	void AddSession(std::string stream_path);
	void RemoveSession(std::string stream_path);

	RtmpSession::Ptr GetSession(std::string stream_path);
	bool HasSession(std::string stream_path);
	bool HasPublisher(std::string stream_path);

	void NotifyEvent(std::string event_type, std::string stream_path);

    virtual TcpConnection::Ptr OnConnect(SOCKET sockfd);
    
	xop::EventLoop *event_loop_;
    std::mutex mutex_;
    std::unordered_map<std::string, RtmpSession::Ptr> rtmp_sessions_; 
	std::vector<EventCallback> event_callbacks_;
}; 
    
}

#endif