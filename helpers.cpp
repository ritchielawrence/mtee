#include "header.h"
#include <stdio.h>

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// CreateFullPathW creates the directory structure pointed to by szPath
// It creates everything upto the last backslash. For example:-
// c:\dir1\dir2\ <-- creates dir1 and dir2
// c:\dir1\file.dat <-- creates only dir1. This means you can pass this
// function the path to a file, and it will just create the required
// directories.
// Absolute or relative paths can be used, as can path length of 32,767
// characters, composed of components up to 255 characters in length. To
// specify such a path, use the "\\?\" prefix. For example, "\\?\D:\<path>".
// To specify such a UNC path, use the "\\?\UNC\" prefix. For example,
// "\\?\UNC\<server>\<share>".
//
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

PWSTR CreateFullPathW(PWSTR szPath)
{
	PWCHAR p = szPath;

	while(*p++)
	{
		if(*p == L'\\')
		{
			*p = L'\0';
			CreateDirectoryW(szPath, NULL);
			*p = L'\\';
		}
	}
	return szPath;
}

/*
VOID MsgBox(LPCTSTR msg)
{
	MessageBox
	(
		NULL,
		msg,
		TEXT("MTEE BETA"),
		MB_OK | MB_SETFOREGROUND
	);
}
*/

/*ID ShowFileType(HANDLE h)
{
	DWORD dwHndType;
	dwHndType = GetFileType(h);

	switch(dwHndType)
	{
	case FILE_TYPE_DISK: // stdin/stdout is redirected from/to disk: app.exe<in>out
		MsgBox(TEXT("FILE_TYPE_DISK"));
		break;
	case FILE_TYPE_CHAR: // stdin from keyboard or nul/con/aux/comx, stdout to console or nul
		MsgBox(TEXT("FILE_TYPE_CHAR"));
		break;
	case FILE_TYPE_PIPE: // stdin is from pipe prn/lptx, stdout piped to anything
		MsgBox(TEXT("FILE_TYPE_PIPE"));
		break;
	default:
		MsgBox(TEXT("FILE_TYPE_UNKNOWN"));
		break;
	}
}

*/
/*DWORD GetWinVer(VOID)
{
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	DWORD myVersion;

	if(!GetVersionEx(&os)) return 0;

	if(os.dwPlatformId != VER_PLATFORM_WIN32_NT) myVersion = VER_PRE_NT4;
	else
	{
		switch(os.dwMajorVersion)
		{
			case 3:  myVersion = VER_PRE_NT4; break;
			case 4:  myVersion = VER_NT4;     break;
			default: myVersion = VER_2000;
		}
	}

	if(myVersion == VER_2000)
	{
		switch(os.dwMinorVersion)
		{
			case 0:  myVersion = VER_2000; break;
			case 1:  myVersion = VER_XP;   break;
			default: myVersion = VER_2003;
		}
	}

	return myVersion;
}
*/
/*
         dwPlatFormID  dwMajorVersion  dwMinorVersion  dwBuildNumber  GetWinVer()
95             1              4               0             950            0
95 SP1         1              4               0        >950 && <=1080      0
95 OSR2        1              4             <10           >1080            0
98             1              4              10            1998            0
98 SP1         1              4              10       >1998 && <2183       0
98 SE          1              4              10          >=2183            0
ME             1              4              90            3000            0

NT 3.51        2              3              51                            0
NT 4           2              4               0            1381            1

2000           2              5               0            2195            2
XP             2              5               1            xxxx            3
2003*          2              5               2            xxxx            4

CE             3                                                           0
*/

//----------------------------------------------------------------------------
DWORD GetFormattedDateTimeA(PCHAR lpBuf, BOOL bDate, BOOL bTime)
{
	SYSTEMTIME st;
	DWORD dwSize = 0;

	GetLocalTime(&st);

	if(bDate)
	{
		dwSize += wsprintfA(lpBuf, "%04u-%02u-%02u ", st.wYear, st.wMonth, st.wDay);
	}
	if(bTime)
	{
		dwSize += wsprintfA(lpBuf + dwSize, "%02u:%02u:%02u.%03u ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	}
	return dwSize;
}

//----------------------------------------------------------------------------
DWORD GetFormattedDateTimeW(PWCHAR lpBuf, BOOL bDate, BOOL bTime)
{
	SYSTEMTIME st;
	DWORD dwSize = 0;

	GetLocalTime(&st);

	if(bDate)
	{
		dwSize += wsprintfW(lpBuf, L"%04u-%02u-%02u ", st.wYear, st.wMonth, st.wDay);
	}
	if(bTime)
	{
		dwSize += wsprintfW(lpBuf + dwSize, L"%02u:%02u:%02u.%03u ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	}
	return dwSize; // return characters written, not bytes
}

// determine whether the output is a console
// this is hard. I first tried to use GetConsoleMode but it returns FALSE in case: mtee > con
BOOL IsAnOutputConsoleDevice(HANDLE h)
{
	if (GetFileType(h) == FILE_TYPE_CHAR)
	{
		// CON, NUL, ...
		DWORD dwBytesWritten;
		if (WriteConsoleA(h, "", 0, &dwBytesWritten, NULL))
			return TRUE;
	}
	return FALSE;
}

DWORD GetParentProcessId(VOID)
{
    DWORD ppid = 0, pid = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) Perror((DWORD)NULL);

    PROCESSENTRY32 pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32);

    //
    // find our PID in the process snapshot then lookup parent PID
    //
    if(Process32First(hSnapshot, &pe)) {
    	do {
    		if (pe.th32ProcessID == pid) {
    			ppid = pe.th32ParentProcessID;
    			break;
            }
        } while(Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return ppid;
}

HANDLE GetPipedProcessHandle(VOID)
{
    //
    // returns a handle to the process piped into mtee
    //
    DWORD dwProcCount, lpdwProcessList[MAX_CONSOLE_PID_LIST];
    HANDLE hPipedProcess = NULL;
    //
    // get an array of PIDs attached to this console
    //
    dwProcCount = GetConsoleProcessList(lpdwProcessList, MAX_CONSOLE_PID_LIST);
    for (DWORD dw=0; dw<dwProcCount; dw++) {
    }
    // in tests it __appears__ array element 0 is this PID, element 1 is process
    // piped into mtee, and last element is cmd.exe. if more than one pipe used,
    // element 2 is next process to rhe left:
    // eg A | B | C | mtee /e ==> lpdwProcessList[mtee][A][B][C][cmd]
    //
    // find the first PID that is not this PID and not parent PID.
    //
    DWORD ppid = GetParentProcessId();
    DWORD cpid = GetCurrentProcessId();
    for (DWORD dw = 0; dw < dwProcCount; dw++)
    {
        HANDLE Handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lpdwProcessList[dw]);
        if ((cpid != lpdwProcessList[dw]) && (ppid != lpdwProcessList[dw]))
        {
            hPipedProcess = Handle;
            break;
        }
        CloseHandle(Handle);
    }
    return hPipedProcess;
}

int FormatElapsedTime( LARGE_INTEGER* elapsedTime, PCHAR outBuf,
                                                        const int outBufSize )
{
    int h = 0;
    int m = 0;
    int len = 0;

    float s = float(elapsedTime->QuadPart / 1000000);
    m = (int)(s / 60.0);
    s = s - 60*m;

    h = (int)((float)m/60.0);
    m = m - 60*h;

    len = snprintf( outBuf, outBufSize, "Elapsed time: %02dh%02dm%06.3fs\n", h, m, s);

    return len;
}



