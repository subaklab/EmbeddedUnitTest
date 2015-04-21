#include <windows.h>  
#include "CppUTest/TestHarness.h"
#include "CppUTest/SimpleString.h"
#include "CppUTest/PlatformSpecificFunctions.h"

extern "C"{
	extern UINT32 CalculateDays(SYSTEMTIME* lpTime);
}

TEST_GROUP(RTC)
{
};

TEST(RTC, justCall)
{
	SYSTEMTIME t;
	t.wDay = 1;
	t.wMonth = 1;
	t.wYear = 2016;

	//CalculateDays(&t);
	CHECK_EQUAL(366, CalculateDays(&t));
}


