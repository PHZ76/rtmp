#include "HttpServer.h"

using namespace xop;

HttpServer::HttpServer()
	: accepter_(nullptr)
	, poll_thread_(nullptr)
	, is_shutdown_(false)
{
	timer_interval_ = 1.0; 
	memset(&http_opts_, 0, sizeof(mg_serve_http_opts));
}

HttpServer::~HttpServer()
{

}

void HttpServer::SetRootDir(std::string pathname)
{
	root_dir_ = pathname;
}

bool HttpServer::Start(std::string ip, uint16_t port)
{
	if (accepter_ != nullptr) {
		Stop();
	}

	mg_mgr_init(&mgr_, NULL);
	mgr_.user_data = this;
	accepter_ = mg_bind(&mgr_, std::to_string(port).c_str(), Handler);
	if (accepter_ == nullptr) {
		return false;
	}

	mg_set_timer(accepter_, mg_time() + timer_interval_);
	mg_set_protocol_http_websocket(accepter_);

	is_shutdown_ = false;
	poll_thread_.reset(new std::thread([this] {
		while (!is_shutdown_) {
			int msec = 200;
			if (connections_.size() > 0) {
				msec = 1;
			}
			mg_mgr_poll(&mgr_, msec);
		}
	}));
	return true;
}

void HttpServer::Stop()
{
	if (accepter_ != nullptr) {
		is_shutdown_ = true;
		poll_thread_->join();
		poll_thread_ = nullptr;

		mg_mgr_free(&mgr_);
		accepter_ = nullptr;
		connections_.clear();
	}
}

void HttpServer::Handler(struct mg_connection* conn, int ev, void* ev_data)
{
	HttpServer* server = (HttpServer*)conn->mgr->user_data;

	switch (ev)
	{
	case MG_EV_HTTP_REQUEST:
		server->OnRequest(conn, ev_data);
		break;
	case MG_EV_TIMER:
		server->OnTimer(conn);
		break;
	//case MG_EV_RECV:
	//case MG_EV_SEND:
	case MG_EV_POLL:
		server->OnPoll(conn);
		break;
	case MG_EV_ACCEPT:
		server->OnConnect(conn);
		break;
	case MG_EV_CLOSE:
		server->OnClose(conn);
		break;
	default:
		break;
	}
}

void HttpServer::OnConnect(mg_connection* conn)
{
	auto http_conn = std::make_shared<HttpConnection>(conn);
	connections_.emplace(conn, http_conn);
}

void HttpServer::OnClose(mg_connection* conn)
{
	connections_.erase(conn);
}

void HttpServer::OnPoll(mg_connection* conn)
{
	auto iter = connections_.find(conn);
	if (iter != connections_.end()) {
		iter->second->Poll();
	}
}

void HttpServer::OnTimer(mg_connection* conn)
{
	HttpServer* server = (HttpServer*)conn->mgr->user_data;
	server->OnPoll(conn);
	mg_set_timer(conn, mg_time() + timer_interval_);
}

void HttpServer::OnRequest(mg_connection* conn, void* ev_data)
{
	HttpServer* server = (HttpServer*)conn->mgr->user_data;
	if (!root_dir_.empty()) {
		server->http_opts_.document_root = server->root_dir_.c_str();
		server->http_opts_.enable_directory_listing = "no";
		mg_serve_http(conn, (struct http_message *) ev_data, server->http_opts_);
	}
	else {
		struct http_message *hm = (struct http_message *) ev_data;
		//mg_send_head(conn, 200, hm->message.len, "Content-Type: text/plain");
		//mg_printf(conn, "%.*s", (int)hm->message.len, hm->message.p);
		//printf("\n%.*s\n", (int)hm->message.len, hm->message.p);
		printf("\n%.*s\n", (int)hm->uri.len, hm->uri.p);

		mg_printf(conn, "%s", "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
		conn->flags |= MG_F_SEND_AND_CLOSE;
	}
}