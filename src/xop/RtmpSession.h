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

    void setMetaData(AmfObjects metaData)
    { m_metaData = metaData; }
    
    AmfObjects getMetaData() const 
    { return m_metaData; }   
    
    void addClient(std::shared_ptr<TcpConnection> conn);
    void removeClient(std::shared_ptr<TcpConnection> conn);
    
    void sendMetaData(std::shared_ptr<char> data, uint32_t size);
    
private:        
    std::mutex m_mutex;
    AmfObjects m_metaData;
    std::unordered_map<SOCKET, std::weak_ptr<TcpConnection>> m_players; 
    std::weak_ptr<TcpConnection> m_publisher;
};

}

#endif
