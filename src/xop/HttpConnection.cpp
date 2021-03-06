#include "HttpConnection.h"

using namespace xop;

HttpConnection::HttpConnection(mg_connection* mg_conn)
	: mg_conn_(mg_conn)
	, max_queue_length_(kMaxQueueLength)
{

}

HttpConnection::~HttpConnection()
{

}

void HttpConnection::Poll()
{
	mutex_.lock();
	if (!buffer_.empty()) {
		OnSend();
	}	
	mutex_.unlock();
}

SOCKET HttpConnection::GetSocket()
{
	return (SOCKET)mg_conn_->sock;
}

void HttpConnection::Send(const char* data, uint32_t data_size)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (!mg_conn_) {
		return;
	}

	if (buffer_.size() < max_queue_length_) {
		Packet pkt;
		pkt.data.reset(new char[data_size + 512], std::default_delete<char[]>());
		memcpy(pkt.data.get(), data, data_size);
		pkt.size = data_size;
		pkt.write_index = 0;
		buffer_.emplace(std::move(pkt));
	}
}

void HttpConnection::OnSend()
{
	if (!buffer_.empty()) {
		//printf("send_queue_len: %llu, send_buf_len: %llu, send_buf_size: %llu \n", \
		//	buffer_->size(), mg_conn_->send_mbuf.len, mg_conn_->send_mbuf.size);

		if (mg_conn_->send_mbuf.len > kMaxBufferLen) {
			//buffer_->pop();
			return;
		}

		Packet &pkt = buffer_.front();
		mg_send(mg_conn_, pkt.data.get(), pkt.size);
		buffer_.pop();
	}	
}