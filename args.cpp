#include "header.h"

BOOL ParseCommandlineW(LPARGS args)
{
	PWCHAR lpToken;
	PFILEINFO fi;
	//
	// init defaults
	//
	args->bHelp			= FALSE; // /?
	args->bAnsi			= FALSE; // /A
	args->bContinue		= FALSE; // /C
	args->bAddDate		= FALSE; // /D
	args->bIntermiate	= FALSE; // /I
	args->bAddTime		= FALSE; // /T
	args->bUnicode		= FALSE; // /U
	args->dwBufSize		= 0x4000;
	args->dwPeekTimeout = 50;
	//
	// point to first fileinfo and init. This will contain the entry for the output file that is the console, unless redirected.
	//
	fi					= &args->fi;
	fi->hFile			= NULL;
	fi->lpFileName		= NULL;
	fi->fiNext			= NULL;
	fi->bAppend			= FALSE;

	while(GetCommandLineTokenW(&lpToken))
	{
		if(!lstrcmpiW(lpToken, L"/?"))		// help
			args->bHelp = TRUE;
		else if(!lstrcmpiW(lpToken, L"/A")) // ansi
			args->bAnsi = TRUE;
		else if(!lstrcmpiW(lpToken, L"/C")) // continue
			args->bContinue = TRUE;
		else if(!lstrcmpiW(lpToken, L"/D")) // add date
			args->bAddDate = TRUE;
		else if(!lstrcmpiW(lpToken, L"/I")) // create intermediate dirs
			args->bIntermiate = TRUE;
		else if(!lstrcmpiW(lpToken, L"/T")) // add time
			args->bAddTime = TRUE;
		else if(!lstrcmpiW(lpToken, L"/U")) // unicode
			args->bUnicode = TRUE;
		else if(!lstrcmpiW(lpToken, L"/+")) // append
		{
			//
			// next token should be a filename, load it and create a new fileinfo struct
			//
			if(!GetCommandLineTokenW(&lpToken))
			{
				Verbose(TEXT("The /+ switch must be followed by a filename.\r\n"));
				return FALSE;
			}
			if(!CreateFileInfoStruct(&fi)) ExitProcess(Perror(NULL));
			fi->bAppend = TRUE;

			if(!CheckFileName(lpToken)) ExitProcess(Perror(ERROR_INVALID_PARAMETER));
			if(!StringAllocW(&fi->lpFileName, lpToken))	ExitProcess(Perror(NULL));
		}
		else
		{
			// unknown token so assume a filename
			if(!CreateFileInfoStruct(&fi)) ExitProcess(Perror(NULL));
			if(!CheckFileName(lpToken)) ExitProcess(Perror(ERROR_INVALID_PARAMETER));
			if(!StringAllocW(&fi->lpFileName, lpToken))	ExitProcess(Perror(NULL));
		}
	}

	//
	// validate the options
	//
	if((args->bAnsi + args->bUnicode) > 1)
	{
		Verbose(TEXT("The /A and /U switches cannot be used together.\r\n"));
		return FALSE;
	}

	return TRUE;

}

BOOL CheckFileName(PWCHAR lpToken)
{
	PWCHAR p = lpToken;
	WCHAR strBadChars[] = L"<>|\"/";
	PWCHAR lpBadChars;
	if(!lstrcmpW(lpToken, L"")) return FALSE;
	while(*p)
	{
		lpBadChars = strBadChars;
		while(*lpBadChars) if(*p == *lpBadChars++) return FALSE;
		p++;
	}
	return TRUE;
}

PFILEINFO CreateFileInfoStruct(PFILEINFO *fi)
{
	
	(*fi)->fiNext = (PFILEINFO) HeapAlloc(GetProcessHeap(), 0, sizeof(FILEINFO));
	if(!(*fi)->fiNext) return NULL;
	//
	// init and point to new struct
	//
	(*fi) = (*fi)->fiNext;
	(*fi)->hFile = NULL;
	(*fi)->lpFileName = NULL;
	(*fi)->bAppend = FALSE;
	(*fi)->fiNext = NULL;
	return *fi;
}

VOID FreeFileInfoStructs(PFILEINFO fi)
{
	while(fi)
	{
		HeapFree(GetProcessHeap(), 0, fi->lpFileName);
		HeapFree(GetProcessHeap(), 0, fi = fi->fiNext);
	}
}

LPWSTR StringAllocW(PWCHAR *dest, LPCWSTR src)
{
	*dest = (WCHAR *) HeapAlloc(GetProcessHeap(), 0, (lstrlenW(src) * sizeof(WCHAR) + sizeof(WCHAR)));
	if(!*dest) return NULL;
	else return lstrcpyW(*dest, src);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// Function: GetCommandLineToken()
//
// Synopsis: Parses a commandline, returning one token at a time
//
// Arguments: [lpToken] Pointer to a pointer. Points to the found token
//
// Returns: TRUE if a token is found, otherwise FALSE (end of commandline
//			reached
//
// Notes: Each call sets lpToken to point to each token in turn until no
//		  more found
//
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
BOOL GetCommandLineTokenW(PWCHAR *lpToken)
{
	static PWCHAR	p = NULL;			// keeps track of where we've got to on windows cmdline
	static PWCHAR	lpTokenBuf = NULL;	// buffer to hold each token in turn
	PWCHAR			t;					// keeps track of current token
	DWORD			cQuote = 0;			// keeps tracks of quotes
	BOOL			bOutQuotes = TRUE;	// true if outside of quotes
	BOOL			bInToken = FALSE;	// true if inside a token
	
	//
	// if p is NULL, then I'm being called for first time
	//
	if(!p)
	{
		//
		// get a pointer to windows commandline
		//
		p = GetCommandLineW();

		//
		// allocate enough memory to hold any tokens in the commandline
		//
		lpTokenBuf = (PWCHAR) HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (lstrlenW(p) + sizeof(WCHAR)));

		if(!lpTokenBuf) ExitProcess(Perror(NULL));

		//
		// skip over name of exe to get to start arguments
		// windows always removes leading whitespace and adds a trailing space
		// if not already present. [; ; prog/s] --> [prog /s]
		//
		if(*p == L'"')
		{
			//
			// if exe name begins with a quote(s), then skip over all beginning quotes
			//
			while(*p == L'"') p++;
			//
			// now skip to the ending quote(s)
			//
			while(*p != L'"') p++;
			//
			// finally skip any further trailing quotes
			//
			while(*p == L'"') p++;
		}
		//
		// didn't start with a quote, so skip to first whitespace
		//
		else
		{
			while((*p) && (*p != L' ') && (*p != L'\t')) p++;
		}

		//
		// skip over any whitespace to first token or end of commandline
		//
		while(*p && (*p == L' ' || *p == L'\t')) p++;

	}

	//
	// initialise token pointers and token buffer
	//
	*lpToken = t = lstrcpyW(lpTokenBuf, L"");

	//
	// now start to copy characters from windows commandline to token buffer
	//
	if(*p)
	{
		while(*p)
		{
			//
			// skip over whitespace if not inside a quoted token
			//
			if((*p == L' ' || *p == L'\t') && bOutQuotes)
			{
				break;
			}
			else if(*p == L'"')
			{
				bOutQuotes = !bOutQuotes;
				p++;
				
				//
				// three consecutive quotes make one 'real' quote
				//
				if(++cQuote == 3)
				{
					*t++ = L'"';
					cQuote = 0;
					bOutQuotes = !bOutQuotes;
				}
				continue;
			}
			//
			// if forward slash found then reset cQuote and toggle bInToken flag
			//
			else if(*p == L'/')
			{
				cQuote = 0;
				bInToken = !bInToken;

				//
				// if not a token and not in quotes then break and l
				//
				if(!bInToken && bOutQuotes) break;
			}

			cQuote = 0;
			bInToken = TRUE;

			//
			// add the character to the token
			//
			*t++ = *p++;

		} // while(*p)

		
		//
		// terminate the token
		//
		*t = L'\0';

		//
		// set p to point to next argument ready for next call
		//
		while(*p && (*p == L' ' || *p == L'\t')) p++;

		return TRUE;
	}

	//
	// reached the end of windows commandline
	//
	else return FALSE;
}
