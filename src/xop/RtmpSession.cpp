#include "RtmpSession.h"

using namespace xop;

RtmpSession::RtmpSession()
{
    
}

RtmpSession::~RtmpSession()
{
    
}

std::string RtmpSession::getMetaData()
{
    std::lock_guard<std::mutex> lock(m_mutex);    
    return m_metaData;
}

void RtmpSession::setMetaData(std::string metaData)
{ 
    std::lock_guard<std::mutex> lock(m_mutex);    
    m_metaData = std::move(metaData); 
    
    for (auto iter = _players.begin(); iter != _players.end();)
    {
        auto conn = iter->second.lock(); 
        if (conn == nullptr) // conn disconect
        {
            _players.erase(iter++);
        }
        else
        {			
            if(conn->isPlayer())
            {
                conn->send(m_metaData.c_str(), m_metaData.size());
                iter++;
            }
        }
    }
}

void RtmpSession::addClient(std::shared_ptr<RtmpConnection>& conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);   
    _players[conn->fd()] = conn;
    
    // send meta data
    conn->send(m_metaData.c_str(), m_metaData.size());
    
    //send gop
}

void RtmpSession::removeClient(std::shared_ptr<RtmpConnection>& conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    _players.erase(conn->fd());
}




