#include "RtmpSession.h"

using namespace xop;

RtmpSession::RtmpSession()
{
    
}

RtmpSession::~RtmpSession()
{
    
}

void RtmpSession::sendMetaData(AmfObjects& metaData)
{ 
    std::lock_guard<std::mutex> lock(m_mutex);    
    if(m_clients.size() == 0)
    {
        return ;
    }
    
    for (auto iter = m_clients.begin(); iter != m_clients.end(); iter++)
    {
        auto conn = iter->second.lock(); 
        if (conn == nullptr) // conn disconect
        {
            m_clients.erase(iter++);
        }
        else
        {	
            RtmpConnection* player = (RtmpConnection*)conn.get();
            if(player->isPlayer())
            {
                player->sendMetaData(metaData);
                iter++;
            }
        }
    }
} 

void RtmpSession::sendMediaData(uint8_t type, uint32_t ts, std::shared_ptr<char> data, uint32_t size)
{
    std::lock_guard<std::mutex> lock(m_mutex);    
    if(m_clients.size() <= 1)
    {
        return ;
    }
    
    for (auto iter = m_clients.begin(); iter != m_clients.end(); iter++)
    {
        auto conn = iter->second.lock(); 
        if (conn == nullptr) // conn disconect
        {
            m_clients.erase(iter++);
        }
        else
        {	
            RtmpConnection* player = (RtmpConnection*)conn.get();
            if(player->isPlayer())
            {               
                player->sendMediaData(type, ts, data, size);
                iter++;
            }
        }
    }
}

void RtmpSession::addClient(std::shared_ptr<TcpConnection> conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);   
    m_clients[conn->fd()] = conn;   
    if(((RtmpConnection*)conn.get())->isPublisher())
    {
        m_hasPublisher = true;
    }
}

void RtmpSession::removeClient(std::shared_ptr<TcpConnection> conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clients.erase(conn->fd());
    if(((RtmpConnection*)conn.get())->isPublisher())
    {
        m_hasPublisher = false;
    }
}

int RtmpSession::getClients()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_clients.size();
}


