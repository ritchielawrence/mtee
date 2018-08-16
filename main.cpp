/*
History:
	01-MAR-2013, MTEE 2.1
		Bug: In Windows 8, mtee resulted to "Incorrect function" if output was to pipe.
		Fixed to not not rely on undocumented error codes from WriteConsole.
		At the same time, got rid of separate functions for console and disk files,
		and combined them to WriteBufferToConsoleAndFilesA and WriteBufferToConsoleAndFilesW and separating
		the console case with fi->bIsConsole. Some re-org to the idea of args.fi : the first item
		is always the std output, and the rest of the files are the given output files. The last item
		in the linked list no longer a dummy item.

		Bug: echo x x x x | mtee guessed that the input was Unicode.
		Fixed to use IS_TEXT_UNICODE_NULL_BYTES instead of IS_TEXT_UNICODE_ASCII16 |	IS_TEXT_UNICODE_STATISTICS.

		Bug: echo t013|mtee /u con entered a forever loop.
		Fixed the bug in WriteBufferToDiskW loop.

		Bug: assumed that all files are less than 4 GB.
		Fixed by using also dwFileSizeHigh in GetFileSize.

		Bug: redir to console and con as output file was not supported.
		Fixed by not trying to truncate the result file with SetEndOfFile if it is a console.
		(redir to con may be wanted if std output is already redirected to file)
	27-APR-2016, MTEE 2.2
	   Workaround: Using ExitProcess at end to workaround an issue in Windows 10
		Ref: https://connect.microsoft.com/VisualStudio/feedback/details/2640071/30-s-delay-after-returning-from-main-program
*/
#include "header.h"
#include <stdio.h>

#define PEEK_BUF_SIZE	(0x400) // 1024
#define PEEK_WAIT_INT	(10)

DWORD dwCtrlEvent; // set by ctrl handler

int main(VOID)
{
	PCHAR		lpBuf			= NULL;	// pointer to main input buffer
	PCHAR		lpAnsiBuf		= NULL; // pointer to buffer for converting unicode to ansi
	PWCHAR		lpUnicodeBuf	= NULL; // pointer to buffer for converting ansi to unicode
	DWORD		dwBytesRead		= 0L;	// bytes read from input
	HANDLE		hOut			= NULL;	// handle to stdout
	HANDLE		hIn				= NULL;	// handle to stdin
	DWORD		dwStdInType		= (DWORD)NULL;	// stdin's filetype (file/pipe/console)
	PFILEINFO	fi				= NULL;	// pointer for indexing FILEINFO records
	BOOL		bBomFound		= FALSE;// true if BOM found
	BOOL		bCtrlHandler	= FALSE;
	ARGS		args;					// holds commandline arguments/options
	DWORD dwPeekBytesRead		= 0L;
	DWORD dwPeekBytesAvailable	= 0L;
	DWORD dwPeekBytesUnavailable= 0L;
	DWORD cPeekTimeout			= 0L;
	BYTE byPeekBuf[PEEK_BUF_SIZE];		// holds peeked input for ansi/unicode test
	DWORD dwInFormat			= OP_ANSI_IN;
	DWORD dwOperation;
	int iFlags;
#ifdef _DEBUG
	MessageBox(0,"start", "mtee", MB_OK);
#endif
/*
	if(!GetWinVer())
	{
		Verbose(TEXT("This program requires Windows NT4, 2000, XP or 2003.\r\n"));
		ExitProcess(1);
	}
*/
	//
	// install ctrl handler to trap ctrl-c and ctrl-break
	//
	dwCtrlEvent = CTRL_CLEAR_EVENT;
	bCtrlHandler = SetConsoleCtrlHandler(HandlerRoutine, TRUE);

	//
	// parse the commandline
	//
	if(!ParseCommandlineW(&args)) ExitProcess(1);

	//
	// did user want to display helpscreen?
	//
	if(args.bHelp) ExitProcess(ShowHelp());

	//
	// get handles to stdout/stdin
	//
	if((hIn = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE) ExitProcess(Perror((DWORD)NULL));
	if((hOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) ExitProcess(Perror((DWORD)NULL));

	args.fi.hFile = hOut;

	//
	// determine whether the output is a console
	//
	args.fi.bIsConsole = IsAnOutputConsoleDevice(hOut);

	//
	// determine the type of input file then peek at content to ascertain if it's unicode
	//
	dwStdInType = GetFileType(hIn);

    //
    // if requested by user, get handle to piped process
    //
    HANDLE hPipedProcess = NULL;
    if (args.bFwdExitCode) hPipedProcess = GetPipedProcessHandle();


	switch(dwStdInType)
	{
	case FILE_TYPE_DISK: // stdin is from a file: mtee.exe < infile
		{
			DWORD dwFileSizeAtLeast;
			DWORD dwFileSizeHigh = 0;

			//
			// try and determine if input file is unicode or ansi. first check the filesize
			// if its zero bytes then don't use readfile as this will block
			//
			dwFileSizeAtLeast = GetFileSize(hIn, &dwFileSizeHigh);
			if (dwFileSizeHigh != 0)
				dwFileSizeAtLeast = 0xFFFFFFFE;
			if(dwFileSizeAtLeast == 0xFFFFFFFF && GetLastError() != NO_ERROR)
			{
				if(!args.bContinue) ExitProcess(Perror((DWORD)NULL));
				else dwFileSizeAtLeast = 0L;
			}
			//
			// only try and peek if there's at least a wchar available otherwise a test for
			// unicode is meaningless
			//
			if(dwFileSizeAtLeast >= sizeof(WCHAR))
			{
				if(!ReadFile(hIn,
					byPeekBuf,
					dwFileSizeAtLeast < sizeof(byPeekBuf) ? dwFileSizeAtLeast : sizeof(byPeekBuf),
					&dwPeekBytesRead,
					NULL))
				{
					//
					// if failed and if i/o errors not being ignored then quit
					//
					if(!args.bContinue) ExitProcess(Perror((DWORD)NULL));
					else break;
				}
				//
				// reset the filepointer to beginning
				//
				if(SetFilePointer(hIn, (LONG)NULL, NULL, FILE_BEGIN) && (!args.bContinue))
					ExitProcess(Perror((DWORD)NULL));
			}
		}
		break;
	case FILE_TYPE_CHAR:
		// stdin from NUL, CON, CONIN$, AUX or COMx
		// if AUX or COMx, then quit without creating any files (not even zero byte files)
		args.dwBufSize = 1;
		{
			DWORD dwInMode;
			if(!GetConsoleMode(hIn, &dwInMode)) // fails (err 6) if NUL, COMx, AUX
			{
				COMMTIMEOUTS CommTimeouts;
				// suceeds if AUX or COMx so quit (allow NUL)
				if(GetCommTimeouts(hIn, &CommTimeouts)) ExitProcess(ERROR_SUCCESS);
			}
		}
		break;
	case FILE_TYPE_PIPE: // stdin is from pipe, prn or lpt1
		while((!dwPeekBytesRead) && (cPeekTimeout < args.dwPeekTimeout) && (dwCtrlEvent == CTRL_CLEAR_EVENT))
		{
			if(!PeekNamedPipe(
				hIn,						// handle to pipe to copy from
				byPeekBuf,					// pointer to data buffer
				PEEK_BUF_SIZE,				// size, in bytes, of data buffer
				&dwPeekBytesRead,           // pointer to number of bytes read
				&dwPeekBytesAvailable,		// pointer to total number of bytes available
				&dwPeekBytesUnavailable))	// pointer to unread bytes in this message
			{
				if(GetLastError() != ERROR_BROKEN_PIPE)	ExitProcess(Perror((DWORD)NULL));
			}
			Sleep(PEEK_WAIT_INT);
			cPeekTimeout += PEEK_WAIT_INT;
		}
		break;
	}

	//
	// open/create all the files after checking stdin, that way if there was an error then
	// zero byte files are not created
	//
	fi = args.fi.fiNext;
	while(fi)
	{
		fi->hFile = CreateFileW
		(
			args.bIntermediate ? CreateFullPathW(fi->lpFileName) : fi->lpFileName,
			GENERIC_WRITE,			// we definitely need write access
			FILE_SHARE_READ,		// allow others to open file for read
			NULL,					// security attr - no thanks
			OPEN_ALWAYS,			// creation disposition - we always want to open or append
			0,						// flags & attributes - gulp! have you seen the documentation?
			NULL					// handle to a template? yer right
		);
		if((fi->hFile == INVALID_HANDLE_VALUE) && !args.bContinue) ExitProcess(Perror((DWORD)NULL));
		//
		// if appending set filepointer to eof
		//
		if(fi->bAppend)
		{
			if((SetFilePointer(fi->hFile, (LONG)NULL, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
				&& !args.bContinue)
			{
				ExitProcess(Perror((DWORD)NULL));
			}
		}

		//
		// Check if it happens to be CON or CONOUT$
		//
		fi->bIsConsole = IsAnOutputConsoleDevice(fi->hFile);

		//
		// Truncate the possibly existing file to zero size
		//
		if(!fi->bIsConsole && !SetEndOfFile(fi->hFile))
		{
			switch(GetLastError())
			{
			case ERROR_INVALID_HANDLE:		// CON, CONOUT$, CONIN$ device, so close the record
				//Yes, this is OK also. fi->hFile = INVALID_HANDLE_VALUE;
				break;
			case ERROR_INVALID_FUNCTION:	// NUL device
			case ERROR_INVALID_PARAMETER:	// PRN device
				break;
			default:
				if(!args.bContinue) ExitProcess(Perror((DWORD)NULL));
			}
		}

		fi = fi->fiNext;
	}


	//
	// if enough bytes read for a meaningful unicode test...
	//
	if(dwPeekBytesRead >= sizeof(WCHAR) * 2)
	{
		//Verbose(TEXT("dwPeekBytesRead >= 4\r\n"));
		//
		// first look for BOM
		// TO DO. if BOM found then do not write it to the console
		// maybe write to files and then advance input pointer two bytes
		//
		if((byPeekBuf[0] == 0xFF) && (byPeekBuf[1] == 0xFE)) // notepad and wordpad's unicode format
		{
			bBomFound = TRUE;
			dwInFormat = OP_UNICODE_IN;
		}
		else
		{
			iFlags = IS_TEXT_UNICODE_NULL_BYTES;
			IsTextUnicode(byPeekBuf, dwPeekBytesRead, &iFlags);
			if(iFlags & IS_TEXT_UNICODE_NULL_BYTES)
			{
				dwInFormat = OP_UNICODE_IN;
			}
		}
	}

//	if(dwInFormat & OP_UNICODE_IN) Verbose("Unicode in...\r\n");
//	else Verbose("ANSI in...\r\n");
 	//
	// allocate the main I/O buffer
	//
	lpBuf = (PCHAR) HeapAlloc(GetProcessHeap(), 0, args.dwBufSize * sizeof(CHAR));
	if(!lpBuf) ExitProcess(Perror((DWORD)NULL));

	if(args.bAnsi) dwOperation = (dwInFormat | OP_ANSI_OUT);
	else if(args.bUnicode) dwOperation = (dwInFormat | OP_UNICODE_OUT);
	else dwOperation = dwInFormat | (dwInFormat << OP_IN_OUT_SHIFT);

	//
	// if input starts with a BOM and output is unicode skip over BOM
	//
	if(bBomFound)
	{
		if(!ReadFile(hIn, lpBuf, sizeof(WCHAR), &dwBytesRead, NULL))
		{
			if(GetLastError() != ERROR_BROKEN_PIPE) ExitProcess(Perror((DWORD)NULL));
		}
	}
	//
	// if output is unicode and user specified unicode conversion, write BOM to files (but not to the std output)
	//
	if((dwOperation & OP_UNICODE_OUT) && args.bUnicode)
	{
		if(!WriteBom(args.fi.fiNext, args.bContinue))
		{
			if(!args.bContinue) ExitProcess(Perror((DWORD)NULL));
		}
	}

    LARGE_INTEGER startTimestamp, endTimestamp, elapsedTime;
    LARGE_INTEGER frequency;

    if( args.bElapsedTime )
    {
        (void)QueryPerformanceFrequency(&frequency);
        (void)QueryPerformanceCounter(&startTimestamp);
    }

	for(;;)
	{
		if(!ReadFile(hIn, lpBuf, args.dwBufSize * sizeof(CHAR), &dwBytesRead, NULL))
		{
			if(GetLastError() != ERROR_BROKEN_PIPE)
			{
				Perror((DWORD)NULL);
				break;
			}
		}
		//
		// if nothing read or user entered EOF then break (ctrl event also causes nothing to be read )
		//
		if( (!dwBytesRead) || ((dwStdInType == FILE_TYPE_CHAR) && (*lpBuf == '\x1A')) ) break;
		if(dwOperation == OP_ANSI_IN_ANSI_OUT)
		{
			if(!WriteBufferToConsoleAndFilesA(&args, lpBuf, dwBytesRead, args.bAddDate, args.bAddTime))
			{
				Perror((DWORD)NULL);
				break;
			}
		}
		else if(dwOperation == OP_UNICODE_IN_UNICODE_OUT)
		{
			if(!WriteBufferToConsoleAndFilesW(&args, (PWCHAR) lpBuf, dwBytesRead / sizeof(WCHAR), args.bAddDate, args.bAddTime))
			{
				Perror((DWORD)NULL);
				break;
			}
		}
		else if(dwOperation == OP_ANSI_IN_UNICODE_OUT)
		{
			AnsiToUnicode(&lpUnicodeBuf, lpBuf, &dwBytesRead);
			if(!WriteBufferToConsoleAndFilesW(&args, (PWCHAR) lpUnicodeBuf, dwBytesRead, args.bAddDate, args.bAddTime))
			{
				Perror((DWORD)NULL);
				break;
			}
		}
		else if(dwOperation == OP_UNICODE_IN_ANSI_OUT)
		{
			UnicodeToAnsi(&lpAnsiBuf, (PWCHAR) lpBuf, &dwBytesRead);
			if(!WriteBufferToConsoleAndFilesA(&args, lpAnsiBuf, dwBytesRead / sizeof(WCHAR), args.bAddDate, args.bAddTime))
			{
				Perror((DWORD)NULL);
				break;
			}
		}
	}

	if( args.bElapsedTime )
    {
        char strElapsedTime[128];
        int strLen = 0;
        memset( strElapsedTime, 0x00, sizeof(strElapsedTime) );

        (void)QueryPerformanceCounter(&endTimestamp);

        elapsedTime.QuadPart = endTimestamp.QuadPart - startTimestamp.QuadPart;
        elapsedTime.QuadPart *= 1000000L;
        elapsedTime.QuadPart /= frequency.QuadPart;

        strLen = FormatElapsedTime( &elapsedTime, strElapsedTime,
                                                    sizeof(strElapsedTime) );
        WriteBufferToConsoleAndFilesA(&args, strElapsedTime, strLen, FALSE,
                                                                        FALSE);
    }

	//
	// close all open files (not the first entry that contains the std output)
	//
	fi = args.fi.fiNext;
	while(fi)
	{
		if(fi->hFile != INVALID_HANDLE_VALUE) CloseHandle(fi->hFile);
		fi = fi->fiNext;
	}
	if(bCtrlHandler) SetConsoleCtrlHandler(HandlerRoutine, FALSE);
	if(dwStdInType == FILE_TYPE_CHAR) FlushFileBuffers(hIn);

    DWORD dwExitCode = 0;
    //
    // if requested by user, get exit code of piped process
    //
    if(args.bFwdExitCode) {
        GetExitCodeProcess(hPipedProcess, &dwExitCode);
        CloseHandle(hPipedProcess);
    }
	//
	// Use ExitProcess (instead of return) to workaround an issue in Windows 10
	//
	ExitProcess(dwExitCode);
}
