#ifndef XOP_HTTP_FLV_CONNECTION_H
#define XOP_HTTP_FLV_CONNECTION_H

#include "HttpConnection.h"
#include "RtmpSink.h"

namespace xop
{

class HttpFlvConnection : public HttpConnection, public RtmpSink
{
public:
	HttpFlvConnection(mg_connection* mg_conn);
	virtual ~HttpFlvConnection();
	
	virtual bool IsPlaying() override { return is_playing_; }
	virtual bool IsPlayer() override { return true; }

	virtual bool SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size) override;
	virtual bool SendVideoData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size) override;
	virtual bool SendAudioData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size) override;

	virtual uint32_t GetId() override
	{ return (uint32_t)this->GetSocket(); }

private:
	friend class RtmpSession;

	bool HasFlvHeader() const { return has_flv_header_; }
	void SendFlvHeader();
	int  SendFlvTag(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size);

	std::string stream_path_;

	std::shared_ptr<char> avc_sequence_header_;
	std::shared_ptr<char> aac_sequence_header_;
	uint32_t avc_sequence_header_size_ = 0;
	uint32_t aac_sequence_header_size_ = 0;
	bool has_key_frame_ = false;
	bool has_flv_header_ = false;
	bool is_playing_ = false;

	const uint8_t FLV_TAG_TYPE_AUDIO = 0x8;
	const uint8_t FLV_TAG_TYPE_VIDEO = 0x9;
};

};

#endif
