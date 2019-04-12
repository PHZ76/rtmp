#include "RtmpSession.h"

using namespace xop;

RtmpSession::RtmpSession()
{
    
}

RtmpSession::~RtmpSession()
{
    
}

void RtmpSession::sendMetaData(std::shared_ptr<char> data, uint32_t size)
{ 
    std::lock_guard<std::mutex> lock(m_mutex);    
    
    for (auto iter = m_players.begin(); iter != m_players.end();)
    {
        auto conn = iter->second.lock(); 
        if (conn == nullptr) // conn disconect
        {
            m_players.erase(iter++);
        }
        else
        {	
            RtmpConnection* player = (RtmpConnection*)conn.get();
            if(player->isPlayer())
            {
                player->sendMetaData(data, size);
                iter++;
            }
        }
    }
} 

void RtmpSession::addClient(std::shared_ptr<TcpConnection> conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);   
    m_players[conn->fd()] = conn;
}

void RtmpSession::removeClient(std::shared_ptr<TcpConnection> conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_players.erase(conn->fd());
}




