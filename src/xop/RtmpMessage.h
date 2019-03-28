#ifndef XOP_RTMP_MESSAGE_H
#define XOP_RTMP_MESSAGE_H

#include <cstdint>

namespace xop
{

class RtmpMessage
{
public:
    RtmpMessage();
    ~RtmpMessage();
    
    int parseChunk(const char* data, uint32_t size);
    
    bool getAll() const 
    { return true; }
    
private:
    
    
};
    
}


#endif
