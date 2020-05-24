#include "Process.h"
#if defined(WIN32) || defined(_WIN32) 
#include <tlhelp32.h>
#endif

using namespace xop;

Process::Process()
	: pid_(0)
{
	
}

Process::~Process()
{

}

bool Process::Start(std::string app_path, std::string cmd, std::string log_path)
{
	if (pid_ > 0) {
		return false;
	}

#if defined(WIN32) || defined(_WIN32) 
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL,TRUE };
	si.wShowWindow = SW_HIDE;

	if (!log_path.empty()) {
		file_ = CreateFile(log_path.c_str(), GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			&sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file_ != INVALID_HANDLE_VALUE) {
			si.dwFlags = STARTF_USESTDHANDLES;
			si.hStdOutput = file_;
			si.hStdError = file_;
		}
	}

	if (CreateProcess(app_path.c_str(), (LPTSTR)cmd.c_str(), NULL, NULL, true, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		pid_ = static_cast<int64_t>(pi.dwProcessId);
	}
	else {
		return false;
	}

#elif defined(__linux) || defined(__linux__) 



#endif
	return false;
}

void Process::Stop()
{
	if (pid_ <= 0) {
		return ;
	}
#if defined(WIN32) || defined(_WIN32) 
	HANDLE hProcess = NULL;
	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid_);
	if (hProcess != NULL) {
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}

	if (file_ != INVALID_HANDLE_VALUE) {
		CloseHandle(file_);
		file_ = INVALID_HANDLE_VALUE;
	}
#elif defined(__linux) || defined(__linux__) 

#endif
}

bool Process::IsAlive()
{
	if (pid_ <= 0) {
		return false;
	}
#if defined(WIN32) || defined(_WIN32) 
	HANDLE hProcess = NULL;
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)pid_);
	//hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, (DWORD)pid_);
	if (hProcess != NULL) {
		CloseHandle(hProcess);
		return true;
	}
#elif defined(__linux) || defined(__linux__) 

#endif
	return false;
}