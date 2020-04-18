/******************************************************************************

 @File         PVRTError.cpp

 @Title        PVRTError

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  

******************************************************************************/

#include "PVRTError.h"
#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#define vsnprintf _vsnprintf
#endif

/*!***************************************************************************
 @Function			PVRTErrorOutputDebug
 @Input				format		printf style format followed by arguments it requires
 @Description		Outputs a string to the standard error.
*****************************************************************************/
void PVRTErrorOutputDebug(char const * const format, ...)
{
	va_list arg;
	char	pszString[1024];

	va_start(arg, format);
	vsnprintf(pszString, 1024, format, arg);
	va_end(arg);


#if defined(UNICODE)
	wchar_t *pswzString = (wchar_t *)malloc((strlen(pszString) + 1) * sizeof(wchar_t));

	int i;
	for(i = 0; pszString[i] != '\0'; i++)
	{
		pswzString[i] = (wchar_t)(pszString[i]);
	}
	pswzString[i] = '\0';

	#if defined(_WIN32)
		OutputDebugString(pswzString);
	#else
		fprintf(stderr, pswzString);
	#endif

	free(pswzString);
#else
	#if defined(_WIN32)
		OutputDebugString(pszString);
	#else
		fprintf(stderr, "%s", pszString);
	#endif
#endif
}

/*****************************************************************************
End of file (PVRTError.cpp)
*****************************************************************************/

