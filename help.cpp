#include "header.h"

int ShowHelp(VOID)
{
	DWORD cBytes;
	PCHAR lpHelpMsg = (PCHAR)

	("\r\nMTEE v2.4 Win32 Commandline Standard Stream Splitter for Windows XP .. 10.\r\n"
	"Copyright (c) 2001-2016 Ritchie Lawrence, http://www.commandline.co.uk.\r\n\r\n"
//   ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8
	"  MTEE [/A | /U] [/C] [/D] [/T] [/E] [[/+] file] [...]\r\n\r\n"
//   ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8
	"  /A    Convert output to ANSI. Default output is same as input.\r\n"
	"  /C    Continue if errors occur opening/writing to file (advanced users only).\r\n"
	"  /D    Prefix each line of output with local date in YYYY-MM-DD format.\r\n"
//	"  /I    Create intermediate directories if required (advanced users only).\r\n"
	"  /T    Prefix each line of output with local time in HH:MM:SS.MSS format.\r\n"
	"  /U    Convert output to UNICODE. Default output is same as input.\r\n"
    "  /E    Exit with exit code of piped process.\r\n"
    "  /ET   Calculate and display elapsed time.\r\n"
	"  /+    Append to existing file. If omitted, existing file is overwritten.\r\n"
	"  file  File to receive the output. File is overwritten if /+ not specified.\r\n"
	"  ...   Any number of additional files. Use /+ before each file to append.\r\n\r\n"
//   ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8
	"  Example usage:-\r\n\r\n"
//   ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8
	"  1) script.cmd | mtee result.txt\r\n"
	"  2) ftp -n -s:ftp.scr | mtee local.log /+ \\\\server\\logs$\\remote.log\r\n"
	"  3) update.cmd 2>&1 | mtee/d/t/+ log.txt\r\n\r\n"
//   ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8
	"  1) Sends the output of script.cmd to the console and to result.log. If\r\n"
	"     result.txt already exists, it will be overwritten.\r\n"
	"  2) Sends output of automated ftp session to the console and two log files,\r\n"
	"     local.log is overwritten if it already exists, remote.log is appended to.\r\n"
	"  3) Redirects stdout and stderr from update.cmd to console and appends to\r\n"
	"     log.txt. Each line is prefixed with local date and time.\r\n");

	WriteFile(
		GetStdHandle(STD_OUTPUT_HANDLE),
		lpHelpMsg,
		lstrlenA(lpHelpMsg),
		&cBytes,
		NULL
	);

	return 0;
}

