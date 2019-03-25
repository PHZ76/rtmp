// PHZ
// 2018-5-15

#ifndef XOP_BUFFER_WRITER_H
#define XOP_BUFFER_WRITER_H

#include <cstdint>
#include <memory>
#include <queue>
#include <string>

namespace xop
{

void writeInt32BE(char* p, int32_t value);
void writeInt32LE(char* p, int32_t value);
void writeInt24BE(char* p, int32_t value);
void writeInt24LE(char* p, int32_t value);
void writeInt16BE(char* p, int16_t value);
void writeInt16LE(char* p, int16_t value);
	
class BufferWriter
{
public:
    BufferWriter(int capacity=kMaxQueueLength);
    ~BufferWriter() {}

    bool append(std::shared_ptr<char> data, uint32_t size, uint32_t index=0);
    bool append(const char* data, uint32_t size, uint32_t index=0);
    int send(int sockfd, int timeout=0); // timeout: ms

    bool isEmpty() const 
    { return _buffer->empty(); }

    bool isFull() const 
    { return ((int)_buffer->size()>=_maxQueueLength?true:false); }

    uint32_t size() const 
    { return _buffer->size(); }
	
private:
    typedef struct 
    {
        std::shared_ptr<char> data;
        uint32_t size;
        uint32_t writeIndex;
    } Packet;

    std::shared_ptr<std::queue<Packet>> _buffer;  		
    int _maxQueueLength = 0;
	 
    static const int kMaxQueueLength = 30;
};

}

#endif

