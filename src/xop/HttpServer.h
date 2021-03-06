#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "HttpConnection.h"
extern "C" {
#include "mongoose/mongoose.h"
}
#include <cstdint>
#include <thread>
#include <memory>
#include <string>
#include <map>

namespace xop {

class HttpServer
{
public:
	HttpServer();
	virtual ~HttpServer();

	virtual void SetRootDir(std::string pathname);

	virtual bool Start(std::string ip, uint16_t port);
	virtual void Stop();

protected:
	static  void Handler(mg_connection* conn, int ev, void* ev_data);
	virtual void OnConnect(mg_connection* conn);
	virtual void OnClose(mg_connection* conn);
	virtual void OnPoll(mg_connection* conn);
	virtual void OnTimer(mg_connection* conn);
	virtual void OnRequest(mg_connection* conn, void* ev_data);

	struct mg_mgr mgr_;
	struct mg_connection* accepter_;
	struct mg_serve_http_opts http_opts_;

	std::string root_dir_;
	 
	bool is_shutdown_;
	double timer_interval_;
	std::unique_ptr<std::thread> poll_thread_;

	std::map<mg_connection*, std::shared_ptr<HttpConnection>> connections_;
};

}

#endif