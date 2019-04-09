#ifndef XOP_RTMP_SESSION_H
#define XOP_RTMP_SESSION_H

#include <memory>
#include "RtmpConnection.h"

namespace xop
{
    
class RtmpSession
{
public:
    using Ptr = std::shared_ptr<RtmpSession>;
    
    RtmpSession();
    ~RtmpSession();
    
    void setMetaData(std::string metaData); 
    std::string getMetaData();  
    
    void addClient(std::shared_ptr<RtmpConnection>& conn);
    void removeClient(std::shared_ptr<RtmpConnection>& conn);
    
private:    
    std::string m_metaData;
    std::mutex m_mutex;
    std::unordered_map<SOCKET, std::weak_ptr<RtmpConnection>> _players; 
};

}

#endif