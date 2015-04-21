#include <oal.h>

UINT32 g_IRQ_RTC;

void OALMSG(UINT32 err ,char* str)
{
	return ;
}
void NKSetLastError(UINT32 err)
{
	return ;
}
void OALIntrStaticTranslate(UINT32 err, UINT32 rtc)
{
	return ;
}
void ERRORMSG(UINT32 b, char* str)
{
	return ;
}
void OALIntrDoneIrqs(UINT32 err, UINT32* irq)
{
	return ;
}

void SC_Sleep(DWORD s)
{
	return;
}
BOOL OALPmicRead(UINT32 addr, PUINT32 pData)
{
	return 1;
}
BOOL OALPmicWrite(UINT32 addr, UINT32 data)
{
	return 1;
}
BOOL OALPmicWriteMasked(UINT32 addr, UINT32 mask, UINT32 data)
{
	return 1;
}
BOOL OALIoCtlHalUnforceIrq(UINT32 code, VOID *pInpBuffer,
	UINT32 inpSize, VOID *pOutBuffer,
	UINT32 outSize, UINT32 *pOutSize)
{
	return 1;
}
