#ifndef XOP_RTMP_SESSION_H
#define XOP_RTMP_SESSION_H

#include <memory>

namespace xop
{
    
class RtmpSession
{
public:
    using Ptr = std::shared_ptr<RtmpSession>;
    
    RtmpSession();
    ~RtmpSession();
    
private:    

};

}

#endif