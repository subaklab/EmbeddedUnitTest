
#include <Windows.h>

#define OAL_IOCTL 0
#define OAL_FUNC 1
#define OAL_ERROR 0
#define OAL_RTC 2
#define OAL_INFO 3

#define SYSINTR_RTC_ALARM 0
#define MC13783_INT_STAT1_ADDR 1
#define MC13783_RTCRSTI_MASK 2
#define MC13783_RTC_TM_ADDR 5
#define MC13783_RTC_ALM_ADDR 6
#define MC13783_RTC_DAY_ADDR 7
#define MC13783_RTC_DAY_ALM_ADDR 8

#define _T


extern void OALMSG(UINT32 err, char* str);
extern void NKSetLastError(UINT32 err);
extern void OALIntrStaticTranslate(UINT32 err, UINT32 rtc);
extern void ERRORMSG(UINT32 b, char* str);
extern void OALIntrDoneIrqs(UINT32 err, UINT32* irq);
