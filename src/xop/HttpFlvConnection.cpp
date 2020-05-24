#include "HttpFlvConnection.h"
#include "RtmpServer.h"
#include "net/Logger.h"

using namespace xop;

HttpFlvConnection::HttpFlvConnection(mg_connection* mg_conn)
	: HttpConnection(mg_conn)
{

}

HttpFlvConnection::~HttpFlvConnection()
{

}

bool HttpFlvConnection::SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{	 
	if (payload_size == 0) {
		return false;
	}

	is_playing_ = true;

	if (type == RTMP_AVC_SEQUENCE_HEADER) {
		avc_sequence_header_ = payload;
		avc_sequence_header_size_ = payload_size;
		return true;
	}
	else if (type == RTMP_AAC_SEQUENCE_HEADER) {
		aac_sequence_header_ = payload;
		aac_sequence_header_size_ = payload_size;
		return true;
	}

	if (type == RTMP_VIDEO) {
		if (!has_key_frame_) {
			uint8_t frame_type = (payload.get()[0] >> 4) & 0x0f;
			uint8_t codec_id = payload.get()[0] & 0x0f;
			if (frame_type == 1 && codec_id == RTMP_CODEC_ID_H264) {
				has_key_frame_ = true;
			}
			else {
				return true;
			}
		}
		SendVideoData(timestamp, payload, payload_size);
	}
	else if (type == RTMP_AUDIO) {
		if (!has_key_frame_ && avc_sequence_header_size_ > 0) {
			return true;
		}
		SendAudioData(timestamp, payload, payload_size);
	}

	return true;
}

bool HttpFlvConnection::SendVideoData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{
	if (!has_flv_header_) {
		SendFlvHeader();
		SendFlvTag(FLV_TAG_TYPE_VIDEO, 0, avc_sequence_header_, avc_sequence_header_size_);
		SendFlvTag(FLV_TAG_TYPE_AUDIO, 0, aac_sequence_header_, aac_sequence_header_size_);
	}

	SendFlvTag(FLV_TAG_TYPE_VIDEO, timestamp, payload, payload_size);
	return true;
}

bool HttpFlvConnection::SendAudioData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{
	if (!has_flv_header_) {
		SendFlvHeader();
		SendFlvTag(FLV_TAG_TYPE_AUDIO, 0, aac_sequence_header_, aac_sequence_header_size_);
	}

	SendFlvTag(FLV_TAG_TYPE_AUDIO, timestamp, payload, payload_size);
	return true;
}

void HttpFlvConnection::SendFlvHeader()
{
	char flv_header[9] = { 0x46, 0x4c, 0x56, 0x01, 0x00, 0x00, 0x00, 0x00, 0x09 };

	if (avc_sequence_header_size_ > 0) {
		flv_header[4] |= 0x1;
	}

	if (aac_sequence_header_size_ > 0) {
		flv_header[4] |= 0x4;
	}

	this->Send(flv_header, 9);
	char previous_tag_size[4] = { 0x0, 0x0, 0x0, 0x0 };
	this->Send(previous_tag_size, 4);

	has_flv_header_ = true;
}

int HttpFlvConnection::SendFlvTag(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{
	if (payload_size == 0) {
		return -1;
	}

	char tag_header[11] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	char previous_tag_size[4] = { 0x0, 0x0, 0x0, 0x0 };

	tag_header[0] = type;
	WriteUint24BE(tag_header + 1, payload_size);
	tag_header[4] = (timestamp >> 16) & 0xff;
	tag_header[5] = (timestamp >> 8) & 0xff;
	tag_header[6] = timestamp & 0xff;
	tag_header[7] = (timestamp >> 24) & 0xff;

	WriteUint32BE(previous_tag_size, payload_size + 11);

	this->Send(tag_header, 11);
	this->Send(payload.get(), payload_size);
	this->Send(previous_tag_size, 4);
	return 0;
}