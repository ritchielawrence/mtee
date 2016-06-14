#include "header.h"

extern DWORD dwCtrlEvent;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	dwCtrlEvent = dwCtrlType;
	switch(dwCtrlType)
    {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		return TRUE;
	default:
		return FALSE;
    }
}
