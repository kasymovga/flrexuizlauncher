#ifdef _WIN32
#include "process.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>

void* Process::open(FSChar *cmd) {
	HANDLE hChildStd_OUT_Rd = NULL;
	HANDLE hChildStd_OUT_Wr = NULL;
	SECURITY_ATTRIBUTES saAttr;
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOW siStartInfo;
	// Set up members of the PROCESS_INFORMATION structure.
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	// Create a pipe for the child process's STDOUT.
	if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
		goto finish;

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		goto finish;

	// Set up members of the STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = hChildStd_OUT_Wr;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
	// Create the child process.
	if (!CreateProcessW(NULL,
			cmd,          // command line
			NULL,         // process security attributes
			NULL,         // primary thread security attributes
			TRUE,         // handles are inherited
			0,            // creation flags
			NULL,         // use parent's environment
			NULL,         // use parent's current directory
			&siStartInfo, // STARTUPINFO pointer
			&piProcInfo)  // receives PROCESS_INFORMATION
			)
		goto finish;

	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example.
finish:
	if (piProcInfo.hProcess)
		CloseHandle(piProcInfo.hProcess);

	if (piProcInfo.hThread)
		CloseHandle(piProcInfo.hThread);

	if (hChildStd_OUT_Wr)
		CloseHandle(hChildStd_OUT_Wr);

	return (void*)hChildStd_OUT_Rd;
}

void Process::close(void *process) {
	CloseHandle((HANDLE)process);
}

long int Process::fd(void *process) {
	return _open_osfhandle((intptr_t)process, _O_RDONLY);
}

#endif
