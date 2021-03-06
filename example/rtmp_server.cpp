#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "xop/RtmpServer.h"
#include "xop/HttpFlvServer.h"
#include "xop/RtmpPublisher.h"
#include "xop/RtmpClient.h"
#include "xop/HttpFlvServer.h"
#include "xop/H264Parser.h"
#include "net/EventLoop.h"

#define TEST_RTMP_PUSHER  1
#define TEST_RTMP_CLIENT  0
#define TEST_MULTI_THREAD 0
#define RTMP_URL    "rtmp://127.0.0.1:1935/live/01"
#define PUSH_FILE   "./test.h264"
#define HTTP_URL    "http://127.0.0.1:8080/live/01.flv"

int TestRtmpPublisher(xop::EventLoop *event_loop);

int main(int argc, char **argv)
{
	int count = 1; 
#if TEST_MULTI_THREAD
	count = std::thread::hardware_concurrency();
#endif
	xop::EventLoop event_loop(count);

	/* rtmp server example */
	auto rtmp_server = xop::RtmpServer::Create(&event_loop);
	rtmp_server->SetChunkSize(60000);
	//rtmp_server->SetGopCache(); /* enable gop cache */
	rtmp_server->SetEventCallback([](std::string type, std::string stream_path) {
		printf("[Event] %s, stream path: %s\n\n", type.c_str(), stream_path.c_str());
	});
	if (!rtmp_server->Start("0.0.0.0", 1935)) {
		printf("RTMP Server listen on 1935 failed.\n");
	}

	/* http-flv server example */
	xop::HttpFlvServer http_flv_server; 
	http_flv_server.Attach(rtmp_server);
	if (!http_flv_server.Start("0.0.0.0", 8080)) {
		printf("HTTP FLV Server listen on 8080 failed.\n");
	}

#if TEST_RTMP_PUSHER
	/* rtmp pusher example */
	std::thread t([&event_loop] () {
		TestRtmpPublisher(&event_loop);
	});
	t.detach();
#endif 

#if	TEST_RTMP_CLIENT
	auto rtmp_client = xop::RtmpClient::Create(&event_loop);
	rtmp_client->SetFrameCB([](uint8_t* payload, uint32_t length, uint8_t codecId, uint32_t timestamp) {
		printf("recv frame, type:%u, size:%u,\n", codecId, length);
	});

	std::string status;
	if (rtmp_client->OpenUrl(RTMP_URL, 3000, status) != 0) {
		printf("Open url %s failed, status: %s\n", RTMP_URL, status.c_str());
	}
#endif

	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
    
	rtmp_server->Stop();
	//http_flv_server.Stop();
	return 0;
}


class H264File
{
public:
	H264File(int bufSize = 5000000);
	~H264File();

	bool open(const char *path);
	void Close();

	bool isOpened() const
	{
		return (m_file != NULL);
	}

	int readFrame(char *inBuf, int inBufSize, bool *bEndOfFrame);

private:
	FILE *m_file = NULL;
	char *m_buf = NULL;
	int m_bufSize = 0;
	int m_bytesUsed = 0;
	int m_count = 0;
};

H264File::H264File(int bufSize)
	: m_bufSize(bufSize)
{
	m_buf = new char[m_bufSize];
}

H264File::~H264File()
{
	delete m_buf;
}

bool H264File::open(const char *path)
{
	m_file = fopen(path, "rb");
	if (m_file == NULL)
	{
		return false;
	}

	return true;
}

void H264File::Close()
{
	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
		m_count = 0;
		m_bytesUsed = 0;
	}
}

int H264File::readFrame(char *inBuf, int inBufSize, bool *bEndOfFrame)
{
	if (m_file == NULL)
	{
		return -1;
	}

	int bytesRead = (int)fread(m_buf, 1, m_bufSize, m_file);
	if (bytesRead == 0)
	{
		fseek(m_file, 0, SEEK_SET);
		m_count = 0;
		m_bytesUsed = 0;
		bytesRead = (int)fread(m_buf, 1, m_bufSize, m_file);
		if (bytesRead == 0)
		{
			this->Close();
			return -1;
		}
	}

	bool bFindStart = false, bFindEnd = false;

	int i = 0, startCode = 3;
	*bEndOfFrame = false;
	for (i = 0; i < bytesRead - 5; i++)
	{
		if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 1)
		{
			startCode = 3;
		}
		else if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 0 && m_buf[i + 3] == 1)
		{
			startCode = 4;
		}
		else
		{
			continue;
		}

		if (((m_buf[i + startCode] & 0x1F) == 0x5 || (m_buf[i + startCode] & 0x1F) == 0x1) &&
			((m_buf[i + startCode + 1] & 0x80) == 0x80))
		{
			bFindStart = true;
			i += 4;
			break;
		}
	}

	for (; i < bytesRead - 5; i++)
	{
		if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 1)
		{
			startCode = 3;
		}
		else if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 0 && m_buf[i + 3] == 1)
		{
			startCode = 4;
		}
		else
		{
			continue;
		}

		if (((m_buf[i + startCode] & 0x1F) == 0x7) || ((m_buf[i + startCode] & 0x1F) == 0x8)
			|| ((m_buf[i + startCode] & 0x1F) == 0x6) || (((m_buf[i + startCode] & 0x1F) == 0x5
				|| (m_buf[i + startCode] & 0x1F) == 0x1) && ((m_buf[i + startCode + 1] & 0x80) == 0x80)))
		{
			bFindEnd = true;
			break;
		}
	}

	bool flag = false;
	if (bFindStart && !bFindEnd && m_count > 0)
	{
		flag = bFindEnd = true;
		i = bytesRead;
		*bEndOfFrame = true;
	}

	if (!bFindStart || !bFindEnd)
	{
		this->Close();
		return -1;
	}

	int size = (i <= inBufSize ? i : inBufSize);
	memcpy(inBuf, m_buf, size);

	if (!flag)
	{
		m_count += 1;
		m_bytesUsed += i;
	}
	else
	{
		m_count = 0;
		m_bytesUsed = 0;
	}

	fseek(m_file, m_bytesUsed, SEEK_SET);
	return size;
}

int TestRtmpPublisher(xop::EventLoop *event_loop)
{
	H264File h264_file;
	if (!h264_file.open(PUSH_FILE)) {
		printf("Open %s failed.\n", PUSH_FILE);
		return -1;
	}

	/* push stream to local rtmp server */
	xop::MediaInfo media_info;
	auto publisher = xop::RtmpPublisher::Create(event_loop);
	publisher->SetChunkSize(60000);

	std::string status;
	if (publisher->OpenUrl(RTMP_URL, 3000, status) < 0) {
		printf("Open url %s failed, status: %s\n", RTMP_URL, status.c_str());
		return -1;
	}

	int buf_size = 500000;
	bool end_of_frame = false;
	bool has_sps_pps = false;
	uint8_t *frame_buf = new uint8_t[buf_size];

	while (publisher->IsConnected()) 
	{
		int frameSize = h264_file.readFrame((char*)frame_buf, buf_size, &end_of_frame);
		if (frameSize > 0) {
			if (!has_sps_pps) {
				if (frame_buf[3] == 0x67 || frame_buf[4] == 0x67) {
					xop::Nal sps = xop::H264Parser::findNal(frame_buf, frameSize);
					if (sps.first != nullptr && sps.second != nullptr && *sps.first == 0x67) {
						media_info.sps_size = (uint32_t)(sps.second - sps.first + 1);
						media_info.sps.reset(new uint8_t[media_info.sps_size], std::default_delete<uint8_t[]>());
						memcpy(media_info.sps.get(), sps.first, media_info.sps_size);

						xop::Nal pps = xop::H264Parser::findNal(sps.second, frameSize - (int)(sps.second - frame_buf));
						if (pps.first != nullptr && pps.second != nullptr && *pps.first == 0x68) {
							media_info.pps_size = (uint32_t)(pps.second - pps.first + 1);
							media_info.pps.reset(new uint8_t[media_info.pps_size], std::default_delete<uint8_t[]>());
							memcpy(media_info.pps.get(), pps.first, media_info.pps_size);

							has_sps_pps = true;
							publisher->SetMediaInfo(media_info); /* set sps pps */							
							printf("Start rtmp pusher, rtmp url: %s , http-flv url: %s \n\n", RTMP_URL, HTTP_URL);
						}
					}
				}
			}
			
			if (has_sps_pps) {
				publisher->PushVideoFrame(frame_buf, frameSize); /* send h.264 frame */
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(40));
	}
	
	delete frame_buf;
	return 0;
}
