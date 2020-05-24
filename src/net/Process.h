#ifndef XOP_PROCESS_H
#define XOP_PROCESS_H

#include <string>
#include <cstdio>
#if defined(WIN32) || defined(_WIN32) 
#include <Windows.h>
#endif

namespace xop {

class Process
{
public:
	Process();
	virtual ~Process();

	bool Start(std::string app_path, std::string cmd, std::string log_path ="");
	void Stop();
	bool IsAlive();

private:

	std::string command_;
	int64_t pid_;
#if defined(WIN32) || defined(_WIN32) 
	HANDLE file_ = INVALID_HANDLE_VALUE;
#endif
};

}

#endif