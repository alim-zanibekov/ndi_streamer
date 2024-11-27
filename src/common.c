// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "common.h"

#include <libavutil/avutil.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

void
av_error_fmt(char *out, char *str, int ec)
{
    char error_buf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(error_buf, AV_ERROR_MAX_STRING_SIZE, ec);
    sprintf(out, "%s, (%s)\n", str, error_buf);
}

int64_t
get_current_ts_usec()
{
#ifdef _WIN32
    FILETIME               filetime; /* 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 00:00 UTC */
    ULARGE_INTEGER         x;
    static const ULONGLONG epoch_offset_us = 11644473600000000ULL; /* microseconds betweeen Jan 1,1601 and Jan 1,1970 */

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
    GetSystemTimePreciseAsFileTime(&filetime);
#else
    GetSystemTimeAsFileTime(&filetime);
#endif
    x.LowPart =  filetime.dwLowDateTime;
    x.HighPart = filetime.dwHighDateTime;
    return (int64_t)(x.QuadPart / 10 - epoch_offset_us);
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return (int64_t)now.tv_sec * 1000000 + (int64_t)now.tv_usec;
#endif
}