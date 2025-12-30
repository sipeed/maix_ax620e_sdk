/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_TIMER_H
#define _AX_TIMER_H

#include <stddef.h>
#include <time.h>
#include "ax_base_type.h"

typedef enum {
    DATE_TIME_FORMAT_YYMMDD1,        /* XXXX-XX-XX (年月日) */
    DATE_TIME_FORMAT_MMDDYY1,        /* XX-XX-XXXX (月日年) */
    DATE_TIME_FORMAT_DDMMYY1,        /* XX-XX-XXXX (日月年) */
    DATE_TIME_FORMAT_YYMMDD2,        /* XXXX年XX月XX日 */
    DATE_TIME_FORMAT_MMDDYY2,        /* XX月XX日XXXX年 */
    DATE_TIME_FORMAT_DDMMYY2,        /* XX日XX月XXXX年 */
    DATE_TIME_FORMAT_YYMMDD3,        /* XXXX/XX/XX (年月日) */
    DATE_TIME_FORMAT_MMDDYY3,        /* XXXX/XX/XX (年月日) */
    DATE_TIME_FORMAT_DDMMYY3,        /* XXXX/XX/XX (年月日) */
    DATE_TIME_FORMAT_YYMMDDWW1,      /* XXXX-XX-XX XXX (年月日 星期几) */
    DATE_TIME_FORMAT_HHmmSS,         /* XX:XX:XX (时分秒) */
    DATE_TIME_FORMAT_YYMMDDHHmmSS,   /* XXXX-XX-XX XX:XX:XX (年月日 时分秒) */
    DATE_TIME_FORMAT_YYMMDDHHmmSSWW, /* XXXX-XX-XX XX:XX:XX XXX (年月日 时分秒 星期几) */
    DATE_TIME_FORMAT_MAX
} DATE_TIME_FORMAT_E;

#ifdef __cplusplus
extern "C" {
#endif

/* ticked in milliseconds */
AX_U64  OPAL_GetTickCount(AX_VOID);
AX_VOID OPAL_mSleep(AX_U32 ms);
AX_VOID OPAL_uSleep(AX_U32 us);
AX_VOID OPAL_Sleep(AX_U32 sec);
AX_VOID OPAL_Yield(AX_VOID);

/* 2022:03:24 */
const AX_CHAR * OPAL_GetLocalDate(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep);
/* 21:31:43:630 */
const AX_CHAR * OPAL_GetLocalTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep, AX_BOOL bCarryTick);
/* 2022:03:24 21:31:43:630*/
const AX_CHAR * OPAL_GetLocalDateTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep);
wchar_t * OPAL_GetCurrDateStr(wchar_t *szOut, AX_U16 nDateFmt, AX_S32* nOutCharLen);
/* return days of month: [1-31] */
AX_U32 OPAL_GetCurrDay();
time_t OPAL_StringToDatetime(AX_CHAR* strYYYYMMDDHHMMSS);
/* nSeconds: 自1970年1月1日至今描述，转换为整形pair(20230418, 101010) */
AX_VOID OPAL_GetDateTimeIntVal(time_t nSeconds, AX_U32* pDate, AX_U32* pTime);
/* GetDateTimeIntVal的逆运算，将整形日期和时刻（20230418, 101010）转换为time_t */
time_t OPAL_GetTimeTVal(AX_S32 nDate, AX_S32 nTime);

#ifdef __cplusplus
}
#endif

#endif // _AX_TIMER_H