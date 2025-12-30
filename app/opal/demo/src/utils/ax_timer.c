/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wchar.h>
#include <errno.h>

AX_U64 OPAL_GetTickCount(AX_VOID) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    AX_U64 result = ts.tv_sec;
    result *= 1000;
    result += (ts.tv_nsec / 1000000);
    return result;
}

AX_VOID OPAL_mSleep(AX_U32 ms) {
    usleep(ms * 1000);
}

AX_VOID OPAL_uSleep(AX_U32 us) {
    usleep(us);
}

AX_VOID OPAL_Sleep(AX_U32 sec) {
    usleep(sec*1000*1000);
}

AX_VOID OPAL_Yield(AX_VOID) {
    // Here nanoseconds can be 0, but for all OS, we set it 1 nanosecond
    // sleep(0) and probably nanosleep with zero provide a mechanism for a thread to surrender the rest of its
    // timeslice. This effectively is a thread yield. So when calling sleep(0) we enter kernel mode and places the
    // thread onto the "runnable" queue. https://stackoverflow.com/questions/58630270/why-does-nanosleep-0-still-execute
    const long nanoseconds = 1;
    struct timespec ts = {0, nanoseconds};
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno))
        ;

}

const AX_CHAR* OPAL_GetLocalDate(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep) {
    struct tm t;
    time_t now = time(NULL);
    localtime_r(&now, &t);

    snprintf(pBuf, nBufSize, "%04d%c%02d%c%02d", t.tm_year + 1900, cSep, t.tm_mon + 1, cSep, t.tm_mday);
    return pBuf;
}

const AX_CHAR* OPAL_GetLocalTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep, AX_BOOL bCarryTick /*= AX_TRUE*/) {
    struct timeval tv;
    struct tm t;
    gettimeofday(&tv, NULL);
    time_t now = time(NULL);
    localtime_r(&now, &t);

    if (bCarryTick) {
        snprintf(pBuf, nBufSize, "%02d%c%02d%c%02d%c%03d", t.tm_hour, cSep, t.tm_min, cSep, t.tm_sec, cSep, (AX_U32)(tv.tv_usec / 1000));
    } else {
        snprintf(pBuf, nBufSize, "%02d%c%02d%c%02d", t.tm_hour, cSep, t.tm_min, cSep, t.tm_sec);
    }

    return pBuf;
}

const AX_CHAR* OPAL_GetLocalDateTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep) {
    struct timeval tv;
    struct tm t;
    gettimeofday(&tv, NULL);
    time_t now = time(NULL);
    localtime_r(&now, &t);

    snprintf(pBuf, nBufSize, "%04d%c%02d%c%02d %02d%c%02d%c%02d%c%03d", t.tm_year + 1900, cSep, t.tm_mon + 1, cSep, t.tm_mday, t.tm_hour,
             cSep, t.tm_min, cSep, t.tm_sec, cSep, (AX_U32)(tv.tv_usec / 1000));
    return pBuf;
}

AX_U32 OPAL_GetCurrDay() {
    struct tm t;
    time_t now = time(NULL);
    localtime_r(&now, &t);
    return t.tm_mday;
}

wchar_t * OPAL_GetCurrDateStr(wchar_t *szOut, AX_U16 nDateFmt, AX_S32 *nOutCharLen) {
    time_t t;
    struct tm tm;

    t = time(NULL);
    localtime_r(&t, &tm);

    AX_U32 nDateLen = 64;
    DATE_TIME_FORMAT_E eDateFmt = (DATE_TIME_FORMAT_E)nDateFmt;
    switch (eDateFmt) {
        case DATE_TIME_FORMAT_YYMMDD1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case DATE_TIME_FORMAT_MMDDYY1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case DATE_TIME_FORMAT_DDMMYY1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case DATE_TIME_FORMAT_YYMMDD2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d年%02d月%02d日", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case DATE_TIME_FORMAT_MMDDYY2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d月%02d日%04d年", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case DATE_TIME_FORMAT_DDMMYY2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d日%02d月%04d年", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case DATE_TIME_FORMAT_YYMMDD3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d/%02d/%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case DATE_TIME_FORMAT_MMDDYY3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case DATE_TIME_FORMAT_DDMMYY3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case DATE_TIME_FORMAT_YYMMDDWW1: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, wday[tm.tm_wday]);
            break;
        }
        case DATE_TIME_FORMAT_HHmmSS: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case DATE_TIME_FORMAT_YYMMDDHHmmSS: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                   tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case DATE_TIME_FORMAT_YYMMDDHHmmSSWW: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %02d:%02d:%02d %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                   tm.tm_hour, tm.tm_min, tm.tm_sec, wday[tm.tm_wday]);
            break;
        }
        default: {
            *nOutCharLen = 0;
            // LOG_M_E(TIME, "Not supported date format: %d.", eDateFmt);
            return NULL;
        }
    }

    return szOut;
}

time_t OPAL_StringToDatetime(AX_CHAR * strYYYYMMDDHHMMSS) {
    struct tm tm_;
    int year, month, day, hour, minute, second;
    sscanf(strYYYYMMDDHHMMSS, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);
    tm_.tm_year = year - 1900;
    tm_.tm_mon = month - 1;
    tm_.tm_mday = day;
    tm_.tm_hour = hour;
    tm_.tm_min = minute;
    tm_.tm_sec = second;
    tm_.tm_isdst = 0;
    time_t t_ = mktime(&tm_);
    return t_;
}

AX_VOID OPAL_GetDateTimeIntVal(time_t nSeconds, AX_U32* pDate, AX_U32* pTime) {
    AX_CHAR szDate[32] = {0};
    AX_CHAR szTime[32] = {0};
    struct tm *p = localtime(&nSeconds);
    sprintf(szDate, "%04d%02d%02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday);
    sprintf(szTime, "%02d%02d%02d", p->tm_hour, p->tm_min, p->tm_sec);
    *pDate = atoi(szDate);
    *pTime = atoi(szTime);
}

time_t OPAL_GetTimeTVal(AX_S32 nDate, AX_S32 nTime) {
    AX_CHAR szDateTime[64] = {0};
    sprintf(szDateTime, "%04d-%02d-%02d %02d:%02d:%02d", nDate / 10000, (nDate % 10000) / 100, nDate % 100, nTime / 10000,
            (nTime % 10000) / 100, nTime % 100);
    return OPAL_StringToDatetime(szDateTime);
}