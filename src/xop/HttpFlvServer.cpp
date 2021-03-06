#include "HttpFlvServer.h"
#include "RtmpServer.h"


using namespace xop;

HttpFlvServer::HttpFlvServer()
{

}

HttpFlvServer::~HttpFlvServer()
{

}

void HttpFlvServer::Attach(std::shared_ptr<RtmpServer> rtmp_server)
{
	std::lock_guard<std::mutex> locker(mutex_);
	rtmp_server_ = rtmp_server;
}

void HttpFlvServer::OnConnect(mg_connection* conn)
{
	auto http_flv_conn = std::make_shared<HttpFlvConnection>(conn);
	auto http_conn = std::dynamic_pointer_cast<HttpConnection>(http_flv_conn);
	connections_.emplace(conn, http_conn);
}

void HttpFlvServer::OnRequest(mg_connection* conn, void* ev_data)
{
	struct http_message *hm = (struct http_message *) ev_data;
	std::string url(hm->uri.p, (int)hm->uri.len);

	auto pos = url.find(".flv");
	if (pos == std::string::npos) {
		mg_printf(conn, "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
		//conn->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	auto rtmp_server = rtmp_server_.lock();
	if (!rtmp_server) {
		mg_printf(conn, "%s","HTTP/1.1 500 Server Error\r\nContent-Length: 0\r\n\r\n");
		//conn->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	std::string stream_path = url.substr(0, pos).c_str();
	//printf("stream path: %s\n", stream_path.c_str());

	if (!rtmp_server->HasPublisher(stream_path)) {
		mg_printf(conn, "%s", "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
		//conn->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	auto session = rtmp_server->GetSession(stream_path);
	auto iter = connections_.find(conn);
	if (session && iter != connections_.end()) {
		mg_printf(conn, "%s", "HTTP/1.1 200 OK\r\nContent-Type: video/x-flv\r\n\r\n");

		auto sink = std::dynamic_pointer_cast<RtmpSink>(iter->second);
		session->AddSink(sink);
		stream_path_ = stream_path;
		rtmp_server->NotifyEvent("http-flv.play", stream_path_ + ".flv");
	}
	else {
		conn->flags |= MG_F_SEND_AND_CLOSE;
	}
}

void HttpFlvServer::OnClose(mg_connection* conn)
{
	if (!stream_path_.empty()) {
		auto rtmp_server = rtmp_server_.lock();
		if (rtmp_server) {
			rtmp_server->NotifyEvent("http-flv.stop", stream_path_ + ".flv");
		}
	}

	connections_.erase(conn);
}
