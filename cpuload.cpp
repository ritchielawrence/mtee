#define _WIN32_WINNT 0x0502
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
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
    if( rc != 0 )
    {
        memcpy( kernelTime, &currentKernelTime, sizeof(ULARGE_INTEGER) );
        memcpy( idleTime,   &currentIdleTime,   sizeof(ULARGE_INTEGER) );
        memcpy( userTime,   &currentUserTime,   sizeof(ULARGE_INTEGER) );
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

    memset( &currentIdleTime, 0x00, sizeof(ULARGE_INTEGER) );
    memset( &currentKernelTime, 0x00, sizeof(ULARGE_INTEGER) );
    memset( &currentUserTime, 0x00, sizeof(ULARGE_INTEGER) );

    memset( &kernelTime, 0x00, sizeof(ULARGE_INTEGER) );
    memset( &idleTime, 0x00, sizeof(ULARGE_INTEGER) );
    memset( &userTime, 0x00, sizeof(ULARGE_INTEGER) );

    float cpuLoad = 0.0f;

    BOOL rc = GetSystemTimesAsUlargeInteger_( &currentIdleTime,
                                                &currentKernelTime,
                                                &currentUserTime);
    if( 0 != rc )
    {
        ULARGE_INTEGER loadTime;
        ULARGE_INTEGER totalTime;

        memset( &loadTime, 0x00, sizeof(ULARGE_INTEGER) );
        memset( &totalTime, 0x00, sizeof(ULARGE_INTEGER) );

        kernelTime.QuadPart = currentKernelTime.QuadPart - lastKernelTime_.QuadPart;
        idleTime.QuadPart = currentIdleTime.QuadPart - lastIdleTime_.QuadPart;
        userTime.QuadPart = currentUserTime.QuadPart - lastUserTime_.QuadPart;

        loadTime.QuadPart = kernelTime.QuadPart + userTime.QuadPart;
        totalTime.QuadPart = kernelTime.QuadPart + userTime.QuadPart + idleTime.QuadPart;

        cpuLoad = (float)((loadTime.QuadPart*100.0)/(totalTime.QuadPart));

        lastIdleTime_.QuadPart = idleTime.QuadPart;
        lastKernelTime_.QuadPart = kernelTime.QuadPart;
        lastUserTime_.QuadPart = userTime.QuadPart;
    }

    #if 0
    printf( "currentCpuLoad = [%5.2f%%]\n", cpuLoad );
    #endif
    return cpuLoad;
}
