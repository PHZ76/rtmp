#ifndef XOP_HTTP_FLV_SERVER_H
#define XOP_HTTP_FLV_SERVER_H

#include "net/SocketUtil.h"
#include "net/Logger.h"
#include "HttpServer.h"
#include "HttpFlvConnection.h"
#include <mutex>

namespace xop
{
class RtmpServer;

class HttpFlvServer : public HttpServer
{
public:
	HttpFlvServer();
	virtual ~HttpFlvServer();

	void Attach(std::shared_ptr<RtmpServer> rtmp_server);

private:
	virtual void OnConnect(mg_connection* conn) override;
	virtual void OnRequest(mg_connection* conn, void* ev_data) override;
	virtual void OnClose(mg_connection* conn);

	std::mutex mutex_;
	std::weak_ptr<RtmpServer> rtmp_server_;
	std::string stream_path_;
};

}

#endif
