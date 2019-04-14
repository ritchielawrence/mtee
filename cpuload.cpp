#define _WIN32_WINNT 0x0502
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "cpuload.h"

static ULARGE_INTEGER lastKernelTime_;
static ULARGE_INTEGER lastIdleTime_;
static ULARGE_INTEGER lastUserTime_;

void cpuLoadInit( void )
{
    memset(&lastKernelTime_, 0x00, sizeof(lastKernelTime_));
    memset(&lastIdleTime_, 0x00, sizeof(lastIdleTime_));
    memset(&lastUserTime_, 0x00, sizeof(lastUserTime_));
}

static BOOL GetSystemTimesAsUlargeInteger_( ULARGE_INTEGER* idleTime,
                                          ULARGE_INTEGER* kernelTime,
                                          ULARGE_INTEGER* userTime )
{
    FILETIME currentKernelTime;
    FILETIME currentIdleTime;
    FILETIME currentUserTime;

    BOOL rc = GetSystemTimes( &currentIdleTime, &currentKernelTime,
                                                            &currentUserTime);

    if( rc )
    {
        memcpy( idleTime, &currentIdleTime, sizeof(ULARGE_INTEGER) );
        memcpy( kernelTime, &currentKernelTime, sizeof(ULARGE_INTEGER) );
        memcpy( userTime, &currentUserTime, sizeof(ULARGE_INTEGER) );
    }

    return rc;
}

float cpuLoadGetCurrentCpuLoad( void )
{
    ULARGE_INTEGER currentKernelTime;
    ULARGE_INTEGER currentIdleTime;
    ULARGE_INTEGER currentUserTime;

    ULARGE_INTEGER kernelTime;
    ULARGE_INTEGER idleTime;
    ULARGE_INTEGER userTime;

    float cpuLoad = 0.0f;

    BOOL rc = GetSystemTimesAsUlargeInteger_( &currentIdleTime,
                                                &currentKernelTime,
                                                &currentUserTime);

    if( rc )
    {
        kernelTime.QuadPart = currentKernelTime.QuadPart - lastKernelTime_.QuadPart;
        idleTime.QuadPart = currentIdleTime.QuadPart - lastIdleTime_.QuadPart;
        userTime.QuadPart = currentUserTime.QuadPart - lastUserTime_.QuadPart;

        cpuLoad = (float)((kernelTime.QuadPart+userTime.QuadPart)/(kernelTime.QuadPart+idleTime.QuadPart+userTime.QuadPart));

        lastIdleTime_.QuadPart = idleTime.QuadPart;
        lastKernelTime_.QuadPart = kernelTime.QuadPart;
        lastUserTime_.QuadPart = userTime.QuadPart;
    }

    return cpuLoad;
}
