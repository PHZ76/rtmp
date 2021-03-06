#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include "net/Socket.h"
extern "C" {
#include "mongoose/mongoose.h"
}
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>

namespace xop {

class HttpConnection
{
public:
	HttpConnection(mg_connection* mg_conn);
	virtual ~HttpConnection();

	virtual void Poll();

	virtual void Send(const char* data, uint32_t data_size);
	virtual SOCKET GetSocket();

protected:
	void OnSend();

	mg_connection* mg_conn_;

	std::mutex mutex_;

	typedef struct {
		std::shared_ptr<char> data;
		uint32_t size;
		uint32_t write_index;
	} Packet;

	std::queue<Packet> buffer_;
	size_t max_queue_length_ = 0;

	static const size_t kMaxQueueLength = 1024;
	static const size_t kMaxBufferLen = 1024 * 1024;
};

}

#endif

