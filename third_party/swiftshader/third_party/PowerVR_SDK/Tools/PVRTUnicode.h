/*!****************************************************************************

 @file         PVRTUnicode.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        A small collection of functions used to decode Unicode formats to
               individual code points.

******************************************************************************/
#ifndef _PVRTUNICODE_H_
#define _PVRTUNICODE_H_

#include "PVRTGlobal.h"
#include "PVRTError.h"
#include "PVRTArray.h"

/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @brief      		Decodes a UTF8-encoded string in to Unicode code points
					(UTF32). If pUTF8 is not null terminated, the results are 
					undefined.
 @param[in]			pUTF8			A UTF8 string, which is null terminated.
 @param[out]		aUTF32			An array of Unicode code points.
 @return 			Success or failure. 
*****************************************************************************/
EPVRTError PVRTUnicodeUTF8ToUTF32(	const PVRTuint8* const pUTF8, CPVRTArray<PVRTuint32>& aUTF32);

/*!***************************************************************************
 @brief      		Decodes a UTF16-encoded string in to Unicode code points
					(UTF32). If pUTF16 is not null terminated, the results are 
					undefined.
 @param[in]			pUTF16			A UTF16 string, which is null terminated.
 @param[out]		aUTF32			An array of Unicode code points.
 @return 			Success or failure. 
*****************************************************************************/
EPVRTError PVRTUnicodeUTF16ToUTF32(const PVRTuint16* const pUTF16, CPVRTArray<PVRTuint32>& aUTF32);

/*!***************************************************************************
 @brief      		Calculates the length of a UTF8 string. If pUTF8 is 
					not null terminated, the results are undefined.
 @param[in]			pUTF8			A UTF8 string, which is null terminated.
 @return 			The length of the string, in Unicode code points.
*****************************************************************************/
unsigned int PVRTUnicodeUTF8Length(const PVRTuint8* const pUTF8);

/*!***************************************************************************
 @brief      		Calculates the length of a UTF16 string.
					If pUTF16 is not null terminated, the results are 
					undefined.
 @param[in]			pUTF16			A UTF16 string, which is null terminated.
 @return 			The length of the string, in Unicode code points.
*****************************************************************************/
unsigned int PVRTUnicodeUTF16Length(const PVRTuint16* const pUTF16);

/*!***************************************************************************
 @brief      		Checks whether the encoding of a UTF8 string is valid.
					If pUTF8 is not null terminated, the results are undefined.
 @param[in]			pUTF8			A UTF8 string, which is null terminated.
 @return 			true or false
*****************************************************************************/
bool PVRTUnicodeValidUTF8(const PVRTuint8* const pUTF8);

#endif /* _PVRTUNICODE_H_ */

/*****************************************************************************
 End of file (PVRTUnicode.h)
*****************************************************************************/

