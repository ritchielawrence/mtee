#include "header.h"

#define MAX_MSG_BUF_SIZE (1024)

DWORD Perror(DWORD dwErrNum)
{
	DWORD cMsgLen;
	LPVOID pErrBuf;
	DWORD dwLastErr;
	
	dwErrNum ? dwLastErr = dwErrNum : dwLastErr = GetLastError();

	//
	// get the text description for that error number from the system
	//
	cMsgLen = FormatMessage
	(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwLastErr,
		MAKELANGID(0, SUBLANG_ENGLISH_US),
		(LPTSTR) &pErrBuf,
		MAX_MSG_BUF_SIZE,
		NULL
	);

	if(cMsgLen)
	{
		Verbose((LPCTSTR) pErrBuf);
		LocalFree((HLOCAL) pErrBuf);
	}
	return dwLastErr;
}

VOID Verbose(LPCTSTR szMsg)
{
	DWORD cBytes;

	WriteFile
	(
		GetStdHandle(STD_ERROR_HANDLE),
		szMsg,
		lstrlen(szMsg),
		&cBytes,
		NULL
	);
}






/*
VOID ShowPipeInfo(HANDLE h)
{


	TCHAR z[1024];
	DWORD lpFlags;
	DWORD lpOutBufferSize;
	DWORD lpInBufferSize;
	DWORD lpMaxInstances;
	
	DWORD lpState;
	DWORD lpCurInstances;
	//LPTSTR lpUserName;
	//DWORD nMaxUserNameSize;

	if(!GetNamedPipeInfo(
		h,
		&lpFlags,
		&lpOutBufferSize,
		&lpInBufferSize,
		&lpMaxInstances))
	{
		perr(TEXT("GetNamedPipeInfo()"));
	}

	if(!GetNamedPipeHandleState(
		h,
		&lpState,
		&lpCurInstances,
		NULL,
		NULL,
		NULL,
		NULL))
	{
		perr(TEXT("GetNamedPipeHandleState"));
	}

	wsprintf(
		z,
		TEXT("Handle:\t\t0x%08X\n")
		TEXT("Flags:\t\t0x%08X\n")
		TEXT("OutBufferSize:\t0x%08X\n")
		TEXT("InBufferSize:\t0x%08X\n")
		TEXT("MaxInstances:\t0x%08X\n")
		TEXT("State:\t\t0x%08X\n")
		TEXT("CurInstances:\t0x%08X\n\n"),
		h,
		lpFlags,
		lpOutBufferSize,
		lpInBufferSize,
		lpMaxInstances,
		lpState,
		lpCurInstances);

	DWORD nBytes;

	WriteFile(
		GetStdHandle(STD_ERROR_HANDLE),
		z,
		lstrlen(z) * sizeof(TCHAR),
		&nBytes,
		NULL);
}

VOID ConfigStdIn(HANDLE h)
{
	DWORD lpMode;

	lpMode = (PIPE_READMODE_BYTE | PIPE_NOWAIT);

	if(!SetNamedPipeHandleState(
		h,
		&lpMode,
		NULL,
		NULL))
	{
		perr(TEXT("SetNamedPipeHandleState()"));
	}

}
*/