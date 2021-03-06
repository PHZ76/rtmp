#ifndef RTMP_SINK_H
#define RTMP_SINK_H

#include "amf.h"

#include <cstdint>
#include <memory>

namespace xop {

class RtmpSink
{
public: 
	RtmpSink() {};
	virtual ~RtmpSink() {};

	virtual bool SendMetaData(AmfObjects metaData) { return true; };
	virtual bool SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size) = 0;
	virtual bool SendVideoData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size) = 0;
	virtual bool SendAudioData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size) = 0;

	virtual bool IsPlayer() { return false; }
	virtual bool IsPublisher() { return false; };
	virtual bool IsPlaying() { return false; }
	virtual bool IsPublishing() { return false; };

	virtual uint32_t GetId() = 0;

};

}

#endif