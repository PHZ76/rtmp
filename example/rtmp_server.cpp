#include "xop/RtmpServer.h"
#include "net/EventLoop.h"

int main()
{
    xop::EventLoop eventLoop;  
    xop::RtmpServer server(&eventLoop, "0.0.0.0", 1935);        
    eventLoop.loop();
    
    return 0;
}
