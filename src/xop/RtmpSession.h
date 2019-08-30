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
    
	void setAvcSequenceHeader(std::shared_ptr<char> avcSequenceHeader, uint32_t avcSequenceHeaderSize)
	{
		m_avcSequenceHeader = avcSequenceHeader;
		m_avcSequenceHeaderSize = avcSequenceHeaderSize;
	}

	void setAacSequenceHeader(std::shared_ptr<char> aacSequenceHeader, uint32_t aacSequenceHeaderSize)
	{
		m_aacSequenceHeader = aacSequenceHeader;
		m_aacSequenceHeaderSize = aacSequenceHeaderSize;
	}

    AmfObjects getMetaData() const 
    { return m_metaData; }   

    void addClient(std::shared_ptr<TcpConnection> conn);
    void removeClient(std::shared_ptr<TcpConnection> conn);
    int  getClients();
        
    void sendMetaData(AmfObjects& metaData);
    void sendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> data, uint32_t size);
    
    bool isPublishing() const 
    { return m_hasPublisher; }
    
	std::shared_ptr<TcpConnection> getPublisher();

private:        
    std::mutex m_mutex;
    AmfObjects m_metaData;
    bool m_hasPublisher = false;
	std::weak_ptr<TcpConnection> m_publisher;
    std::unordered_map<SOCKET, std::weak_ptr<TcpConnection>> m_clients; 

	std::shared_ptr<char> m_avcSequenceHeader;
	std::shared_ptr<char> m_aacSequenceHeader;
	uint32_t m_avcSequenceHeaderSize = 0;
	uint32_t m_aacSequenceHeaderSize = 0;
};

}

#endif
