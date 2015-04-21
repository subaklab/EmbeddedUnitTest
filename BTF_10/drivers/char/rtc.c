    //   
    // Copyright (c) Microsoft Corporation.  All rights reserved.   
    //   
    //   
    // Use of this source code is subject to the terms of the Microsoft end-user   
    // license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.   
    // If you did not accept the terms of the EULA, you are not authorized to use   
    // this source code. For a copy of the EULA, please see the LICENSE.RTF on your   
    // install media.   
    //   
    //------------------------------------------------------------------------------   
    //   
    //  Copyright (C) 2004, Freescale Semiconductor, Inc. All Rights Reserved   
    //  THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT   
    //  BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF   
    //  FREESCALE SEMICONDUCTOR, INC.   
    //   
    //------------------------------------------------------------------------------   
    //   
    //  Module: rtc.c   
    //   
    
    //  Real-time clock (RTC) routines for the MC13783 PMIC RTC.   
    //   
    //------------------------------------------------------------------------------   
    #include <windows.h>   
    #include <oal.h>   
    #include "nkintr.h"   
    #include "regs.h"   
    #include "regs_rtc.h"   
       
    #include "pmic_ioctl.h"   
    #include "regs_regulator.h"   
    #include "pmic_basic_types.h"   
    #include "pmic_lla.h"   
       
       
    //-----------------------------------------------------------------------------   
    // External Functions   
    extern void SC_Sleep(DWORD);   
    extern BOOL OALPmicRead(UINT32 addr, PUINT32 pData);   
    extern BOOL OALPmicWrite(UINT32 addr, UINT32 data);   
    extern BOOL OALPmicWriteMasked(UINT32 addr, UINT32 mask, UINT32 data);   
    extern BOOL OALIoCtlHalUnforceIrq(UINT32 code, VOID *pInpBuffer,   
                                      UINT32 inpSize, VOID *pOutBuffer,    
                                      UINT32 outSize, UINT32 *pOutSize);   
       
    //-----------------------------------------------------------------------------   
    // Global Variables   
       
    extern UINT32 g_IRQ_RTC;   
       
    //------------------------------------------------------------------------------   
    // Global Variables   
    //These macro define some default information of RTC   
    #define ORIGINYEAR       1980                  // the begin year   
    #define MAXYEAR          (ORIGINYEAR + 100)    // the maxium year   
    #define JAN1WEEK         2                     // Jan 1 1980 is a Tuesday   
    #define GetDayOfWeek(X) (((X-1)+JAN1WEEK)%7)   
       
    #define TYPE_TIME        0   
    #define TYPE_ALRM        1   
       
    static const UINT8 monthtable[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};   
    static const UINT8 monthtable_leap[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};   
       
    BOOL OEMSetRealTime(LPSYSTEMTIME lpst);   
    BOOL OEMGetRealTime(LPSYSTEMTIME lpst);   
       
    //------------------------------------------------------------------------------   
    //   
    //  Function:  OALIoCtlHalInitRTC   
    //   
    //  This function is called by WinCE OS to initialize the time after boot.    
    //  Input buffer contains SYSTEMTIME structure with default time value. If   
    //  hardware has persistent real time clock it will ignore this value   
    //  (or all call).   
    //   
    //------------------------------------------------------------------------------   
    BOOL OALIoCtlHalInitRTC( UINT32 code, VOID *pInpBuffer, UINT32 inpSize,   
                             VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)   
    {   
        BOOL rc = FALSE;   
        UINT32 tempISR1;   
        SYSTEMTIME *pTime = (SYSTEMTIME*)pInpBuffer;   
       
        OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalInitRTC(...)\r\n"));   
       
        // Validate inputs   
        if (pInpBuffer == NULL || inpSize < sizeof(SYSTEMTIME)) {   
            NKSetLastError(ERROR_INVALID_PARAMETER);   
            OALMSG(OAL_ERROR, (   
                L"ERROR: OALIoCtlHalInitRTC: Invalid parameter\r\n"   
            ));   
            goto cleanUp;   
        }   
       
        // Add static mapping for RTC alarm   
        OALIntrStaticTranslate(SYSINTR_RTC_ALARM, g_IRQ_RTC);   
       
        OALPmicRead(MC13783_INT_STAT1_ADDR, &tempISR1);   
           
        // check if we need to reinit timer    
        if ( tempISR1 & MC13783_RTCRSTI_MASK )   
        {   
            // Set time   
            rc = OEMSetRealTime(pTime);   
       
            // clear the status bit   
            OALPmicWriteMasked(MC13783_INT_STAT1_ADDR,MC13783_RTCRSTI_MASK, tempISR1);   
        }   
        else   
            // don't init the timer since the RTC is still valid   
       
    cleanUp:   
        OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OALIoCtlHalInitRTC(rc = %d)\r\n", rc));   
        return rc;   
    }   
       
    //-----------------------------------------------------------------------------   
    // Local Functions   
       
    //------------------------------------------------------------------------------   
    //   
    // Function: IsLeapYear   
    //   
    // Local helper function checks if the year is a leap year   
    //   
    // Parameters:   
    //   
    // Returns:   
    //         
    //   
    //------------------------------------------------------------------------------   
    static int IsLeapYear(int Year)   
    {   
        int Leap;   
       
        Leap = 0;   
        if ((Year % 4) == 0) {   
            Leap = 1;   
            if ((Year % 100) == 0) {   
                Leap = (Year%400) ? 0 : 1;   
            }   
        }   
       
        return (Leap);   
    }   
       
    //------------------------------------------------------------------------------   
    //   
    // Function: CalculateDays   
    //   
    // Local helper function calculate the total number of days in lpTime,   
    // since Jan 1, ORIGINYEAR   
    //   
    // Parameters:   
    //   
    // Returns:   
    //      days   
    //   
    //------------------------------------------------------------------------------   
    UINT32 CalculateDays(SYSTEMTIME* lpTime)   
    {   
        UINT8 *month_tab;   
        int days, year, month;   
        int i;   
       
        days = lpTime->wDay;   
        month = lpTime->wMonth;   
        year = lpTime->wYear;   
       
        // Calculate number of days spent so far from beginning of this year   
        month_tab = (UINT8 *)(IsLeapYear(year) ? monthtable_leap : monthtable);   
       
        for (i = 0; i < month - 1; i++)    
        {   
            days += month_tab[i];   
        }   
       
        // calculate the number of days in the previous years   
        for (i = ORIGINYEAR; i < year; i++)   
        {   
            days += (IsLeapYear(i) ? 366 : 365);   
        }   
       
        return days;   
    }   
       
    //------------------------------------------------------------------------------   
    //   
    // Function: CalculateSeconds   
    //   
    // Local helper function that calculates the number of seconds in lpTime since    
    // the beginning of the day   
    //   
    // Parameters:   
    //   
    // Returns:   
    //      seconds   
    //   
    //------------------------------------------------------------------------------   
    UINT32 CalculateSeconds(SYSTEMTIME* lpTime)   
    {   
        return (lpTime->wHour * 60 * 60 + lpTime->wMinute * 60    
                        + lpTime->wSecond);   
    }   
       
    //------------------------------------------------------------------------------   
    //   
    // Function: ConvertDays   
    //   
    // Local helper function that split total days since Jan 1, ORIGINYEAR into    
    // year, month and day   
    //   
    // Parameters:   
    //   
    // Returns:   
    //      Returns TRUE if successful, otherwise returns FALSE.   
    //   
    //------------------------------------------------------------------------------   
    BOOL ConvertDays(UINT32 days, SYSTEMTIME* lpTime)   
    {   
        int dayofweek, month, year;   
        UINT8 *month_tab;   
       
        //Calculate current day of the week   
        dayofweek = GetDayOfWeek(days);   
       
        year = ORIGINYEAR;   
       
        while (days > 365)   
        {   
            if (IsLeapYear(year))   
            {   
                if (days > 366)   
                {   
                    days -= 366;   
                    year += 1;   
                }   
            }   
            else   
            {   
                days -= 365;   
                year += 1;   
            }   
        }   
       
       
        // Determine whether it is a leap year   
        month_tab = (UINT8 *)((IsLeapYear(year))? monthtable_leap : monthtable);   
       
        for (month=0; month<12; month++)   
        {   
            if (days <= month_tab[month])   
                break;   
            days -= month_tab[month];   
        }   
           
        month += 1;   
           
        lpTime->wDay = days;   
        lpTime->wDayOfWeek = dayofweek;   
        lpTime->wMonth = month;   
        lpTime->wYear = year;   
       
        return TRUE;   
    }   
       
    //------------------------------------------------------------------------------   
    //   
    // Function: ConvertSeconds   
    //   
    // Local helper function that converts time of day in seconds to hour,    
    // minute and seconds   
    //   
    // Parameters:   
    //   
    // Returns:   
    //      Returns TRUE if successful, otherwise returns FALSE.   
    //   
    //------------------------------------------------------------------------------   
    BOOL ConvertSeconds(UINT32 seconds, SYSTEMTIME* lpTime)   
    {   
        int minutes = 0, hours = 0;   
       
        if(seconds < 86400)   
        {   
            if (seconds >= 60)   
            {   
                minutes = (int) seconds / 60;   
                seconds -= (minutes * 60);   
                if (minutes >= 60)   
                {   
                    hours = (int) minutes / 60;   
                    minutes -= (hours * 60);   
                }   
            }   
        }   
        else    
        {   
            ERRORMSG(TRUE, (_T("TOD in sec is wrong(seconds > 86399) %d"), seconds));   
            return FALSE;   
        }   
       
        lpTime->wMilliseconds = 0;   
        lpTime->wHour = hours;   
        lpTime->wMinute = minutes;   
        lpTime->wSecond = seconds;   
       
        return TRUE;   
    }   
       
    //------------------------------------------------------------------------------   
    //   
    // Function: SetTime   
    //   
    // This function sets the given time & day into the register pair indicated by type   
    //   
    // Parameters:   
    //   
    // Returns:   
    //      Returns TRUE if successful, otherwise returns FALSE.   
    //   
    //------------------------------------------------------------------------------   
    BOOL SetTime(UINT32 type, SYSTEMTIME* lpTime)   
    {   
        UINT32 days, seconds, addr;   
       
        // calculate time of day in seconds    
        seconds = CalculateSeconds(lpTime);   
       
        if(seconds > 86399)   
            return FALSE;   
       
        //Set Reg TimeOftheDay TOD -> Hours, Min , Sec : a 17 bit time of day (TOD)   
        addr = (type == TYPE_TIME) ? MC13783_RTC_TM_ADDR : MC13783_RTC_ALM_ADDR;   
        OALPmicWrite(addr, seconds);   
       
        // Calculate days.   
        days = CalculateDays(lpTime);   
       
        //Set Reg Day -> years, months ,  days :the 15 bit DAY counter   
        addr = (type == TYPE_TIME) ? MC13783_RTC_DAY_ADDR : MC13783_RTC_DAY_ALM_ADDR;   
        OALPmicWrite(addr, days);   
       
        return TRUE;   
    }   
       
    //------------------------------------------------------------------------------   
    //   
    // Function: GetTime   
    //   
    // This function gets the time and day from the register pair indicated by type   
    //   
    // Parameters:   
    //   
    // Returns:   
    //      Returns TRUE if successful, otherwise returns FALSE.   
    //   
    //------------------------------------------------------------------------------   
    BOOL GetTime(UINT32 type, SYSTEMTIME* lpTime)   
    {   
        UINT32 seconds, days, addr;   
       
        addr = (type == TYPE_TIME) ? MC13783_RTC_TM_ADDR : MC13783_RTC_ALM_ADDR;   
        OALPmicRead(addr, &seconds);   
           
        addr = (type == TYPE_TIME) ? MC13783_RTC_DAY_ADDR : MC13783_RTC_DAY_ALM_ADDR;   
        OALPmicRead(addr, &days);   
       
        //convert seconds to hours , minutes and seconds of the day   
        if (ConvertSeconds(seconds, lpTime) != TRUE)   
            return FALSE;   
       
        // convert days to year, month and day   
        if (ConvertDays(days, lpTime) != TRUE)   
            return FALSE;   
       
           
        return TRUE;       
    }   
       
    //------------------------------------------------------------------------------   
    //   
    //  Function:  OEMGetRealTime   
    //   
    //  This function is called by the kernel to retrieve the time from   
    //  the real-time clock.   
    //   
    //------------------------------------------------------------------------------   
    BOOL OEMGetRealTime(LPSYSTEMTIME lpst)    
    {   
        return GetTime(TYPE_TIME, lpst);       
    }   
       
    //------------------------------------------------------------------------------   
    //   
    //  Function:  OEMSetRealTime   
    //   
    //  This function is called by the kernel to set the real-time clock.   
    //   
    //------------------------------------------------------------------------------   
    BOOL OEMSetRealTime(LPSYSTEMTIME lpst)    
    {   
        return SetTime(TYPE_TIME, lpst);   
    }   
       
    //------------------------------------------------------------------------------   
     //   
     //  FUNCTION:       CheckRealTime   
     //   
     //  DESCRIPTION:    Helper function is used to check if the input    
     //                  time is valid.    
     //   
     //  PARAMETERS:           
     //                  lpst -   
     //                      Long pointer to the buffer containing    
     //                      the time to be checked in SYSTEMTIME format.    
     //   
     //  RETURNS:           
     //                  TRUE - If time is valid.   
     //   
     //                  FALSE - If time is invalid.   
     //   
     //------------------------------------------------------------------------------   
    BOOL CheckRealTime(LPSYSTEMTIME lpst)   
    {   
        WORD isleap;   
        UINT8 *month_tab;   
       
        isleap = IsLeapYear(lpst->wYear);   
        month_tab = (UINT8 *)(isleap? monthtable_leap : monthtable);   
       
        if ((lpst->wYear < ORIGINYEAR) || (lpst->wYear > MAXYEAR))   
            return FALSE;   
       
        if ((lpst->wMonth < 1) ||(lpst->wMonth > 12))   
            return FALSE;   
       
        if((lpst->wDay < 1) ||(lpst->wDay > month_tab[lpst->wMonth-1]))   
            return FALSE;   
               
        if ((lpst->wHour > 23) ||(lpst->wMinute > 59) ||(lpst->wSecond > 59))   
             return FALSE;   
       
        return TRUE;   
    }   
       
    //------------------------------------------------------------------------------   
    //   
    //  Function:  OEMSetAlarmTime   
    //   
    //  Set the RTC alarm time.   
    //   
    //------------------------------------------------------------------------------   
    BOOL OEMSetAlarmTime(LPSYSTEMTIME pTime)   
    {   
        BOOL rc;   
        UINT32 irq;   
       
        OALMSG(OAL_RTC&&OAL_FUNC, ( L"+OEMSetAlarmTime(%d/%d/%d %d:%d:%d.%03d)\r\n",   
            pTime->wMonth, pTime->wDay, pTime->wYear, pTime->wHour, pTime->wMinute,   
            pTime->wSecond, pTime->wMilliseconds ));   
       
        if (pTime == NULL)    
            goto cleanUp;   
       
        //Set seconds, minutes, hours and day in RTC day alarm register   
        SetTime(TYPE_ALRM, pTime);   
       
        // Enable alarm IRQ   
        OALMSG(OAL_RTC&&OAL_INFO, (TEXT("RTC Alarm Interrupt enabled.\r\n")));   
       
        // Enable/clear RTC interrupt   
        irq = g_IRQ_RTC;   
        OALIoCtlHalUnforceIrq(0, &irq, sizeof(irq), NULL, 0, NULL);   
        OALIntrDoneIrqs(1, &irq);   
       
        // Done   
        rc = TRUE;   
       
    cleanUp:   
        OALMSG(OAL_RTC&&OAL_FUNC, (L"-OEMSetAlarmTime(rc = %d)\r\n", rc));   
        return rc;   
    }   

#ifdef TEST
	bool TDD_IsLeapYear(int Year){
		return IsLeapYear(Year);
	}
#endif