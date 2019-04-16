#define _WIN32_WINNT 0x0502
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include "cpuload.h"

#define OP_IN_OUT_SHIFT				(16)			// out codes are 16 bits from in codes
#define OP_ANSI_IN					(0x00000001)
#define OP_UNICODE_IN				(0x00000002)
#define OP_ANSI_OUT					(0x00010000)
#define OP_UNICODE_OUT				(0x00020000)
#define OP_HEX_OUT					(0x00040000)

// MTEE Operations
#define OP_ANSI_IN_ANSI_OUT			(OP_ANSI_IN | OP_ANSI_OUT)
#define OP_ANSI_IN_UNICODE_OUT		(OP_ANSI_IN | OP_UNICODE_OUT)
#define OP_UNICODE_IN_ANSI_OUT		(OP_UNICODE_IN | OP_ANSI_OUT)
#define OP_UNICODE_IN_UNICODE_OUT	(OP_UNICODE_IN | OP_UNICODE_OUT)
#define OP_ANSI_IN_HEX_OUT			(OP_ANSI_IN | OP_HEX_OUT)
#define OP_UNICODE_IN_HEX_OUT		(OP_UNICODE_IN | OP_HEX_OUT)


// defines for GetWinVer()
#define VER_PRE_NT4			(0L)
#define VER_NT4				(1L)
#define VER_2000			(2L)
#define VER_XP				(3L)
#define VER_2003			(4L)

#define _MERGE_RDATA_


#if !defined (INVALID_SET_FILE_POINTER)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

//#include <stdio.h>

#define CTRL_CLEAR_EVENT (0xFFFF)

#define MAX_CONSOLE_PID_LIST    (128L)

//
// Stores details of the files to be written to
//
typedef struct _FILEINFO
{
	HANDLE		hFile;			// handle of open file
	PWCHAR		lpFileName;		// pointer to dynamically assigned wide buffer holding filename
	BOOL		bAppend;		// if true, append I/O to file instead of overwrite
	BOOL		bIsConsole;		// true if it is a console device
	_FILEINFO	*fiNext;		// pointer to next FILEINFO structure
} FILEINFO, *PFILEINFO;

//
// Stores the commandline arguments/options
//
typedef struct _ARGS
{
	BOOL		bAddDate;			// if true, prefix each newline with local date
	BOOL		bAddTime;			// if true, prefix each newline with local time
	BOOL		bContinue;			// if true, continue on I/O errors
	BOOL		bHelp;				// if true, show helpscreen
	BOOL		bUnicode;			// if true, output will be unicode (converted if required)
	BOOL        bFwdExitCode;       // if true, exit with exit code of piped process
	BOOL		bAnsi;				// if true, output will be ansi (converted if required)
	BOOL		bIntermediate;		// if true, create intermediate directories if required
	BOOL        bElapsedTime;       // if true, calculate and display elapsed time
	BOOL        bMeasureCPUUsage;   // if true, measure CPU usage during runtime
	FILEINFO	fi;					// first FILEINFO structure in linked list
	DWORD		dwBufSize;			// max size of buffer for read operations
	DWORD		dwPeekTimeout;		// max ms to peek for input
} ARGS, *LPARGS;

// args.cpp
BOOL CheckFileName(PWCHAR lpToken);
BOOL GetCommandLineTokenW(PWCHAR *tokenPtr);	// process ANSI commandline args
BOOL ParseCommandlineW(LPARGS args);
LPWSTR StringAllocW(PWCHAR *dest, LPCWSTR src);
VOID FreeFileInfoStructs(PFILEINFO fi);
PFILEINFO CreateFileInfoStruct(PFILEINFO *fi);


//VOID MsgBox(LPCTSTR);

BOOL WINAPI HandlerRoutine(DWORD);
HWND GetConsoleHandle(VOID);
VOID ShowFileType(HANDLE);
int ShowHelp(VOID);
VOID ShowPipeInfo(HANDLE h);
VOID ConfigStdIn(HANDLE h);
PWSTR CreateFullPathW(PWSTR szPath);

//DWORD GetWinVer(VOID);
DWORD GetFormattedDateTimeA(PCHAR lpBuf, BOOL bDate, BOOL bTime);
DWORD GetFormattedDateTimeW(PWCHAR lpBuf, BOOL bDate, BOOL bTime);
BOOL IsAnOutputConsoleDevice(HANDLE h);
DWORD GetParentProcessId(VOID);
HANDLE GetPipedProcessHandle(VOID);

int FormatElapsedTime( LARGE_INTEGER* elapsedTime, PCHAR outBuf,
                                                        const int outBufSize );

// perr.cpp
DWORD Perror(DWORD dwErrNum);
VOID Verbose(LPCTSTR szMsg);

// output.cpp
BOOL WriteBufferToConsoleAndFilesA(LPARGS args, PCHAR lpBuf, DWORD dwCharsRead, BOOL AddDate, BOOL AddTime);
BOOL WriteBufferToConsoleAndFilesW(LPARGS args, PWCHAR lpBuf, DWORD dwCharsRead, BOOL AddDate, BOOL AddTime);
BOOL AnsiToUnicode(PWCHAR *lpDest, PCHAR lpSrc, LPDWORD lpSize);
BOOL UnicodeToAnsi(PCHAR *lpDest, PWCHAR lpSrc, LPDWORD lpSize);
BOOL WriteBom(PFILEINFO fi, BOOL bContinue);
