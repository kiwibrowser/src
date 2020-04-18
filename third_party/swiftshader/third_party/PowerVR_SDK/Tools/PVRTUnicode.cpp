/******************************************************************************

 @File         PVRTUnicode.cpp

 @Title        PVRTUnicode

 @Version       @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     All

 @Description  A small collection of functions used to decode Unicode formats to
               individual code points.

******************************************************************************/
#include "PVRTUnicode.h"
#include <string.h>

/****************************************************************************
** Constants
****************************************************************************/
const PVRTuint32 c_u32ReplChar = 0xFFFD;

#define VALID_ASCII 0x80
#define TAIL_MASK 0x3F
#define BYTES_PER_TAIL 6

#define UTF16_SURG_H_MARK 0xD800
#define UTF16_SURG_H_END  0xDBFF
#define UTF16_SURG_L_MARK 0xDC00
#define UTF16_SURG_L_END  0xDFFF

#define UNICODE_NONCHAR_MARK 0xFDD0
#define UNICODE_NONCHAR_END  0xFDEF
#define UNICODE_RESERVED	 0xFFFE
#define UNICODE_MAX			 0x10FFFF

#define MAX_LEN 0x8FFF

/****************************************************************************
** A table which allows quick lookup to determine the number of bytes of a 
** UTF8 code point.
****************************************************************************/
const PVRTuint8 c_u8UTF8Lengths[256] = 
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,
};

/****************************************************************************
** A table which allows quick lookup to determine whether a UTF8 sequence
** is 'overlong'.
****************************************************************************/
const PVRTuint32 c_u32MinVals[4] =
{
	0x00000000,		// 0 tail bytes
	0x00000080,		// 1 tail bytes
	0x00000800,		// 2 tail bytes
	0x00010000,		// 3 tail bytes
};

/*!***************************************************************************
 @Function			CheckGenericUnicode
 @Input				c32			A UTF32 character/Unicode code point
 @Returns			Success or failure. 
 @Description		Checks that the decoded code point is valid.
*****************************************************************************/
static bool CheckGenericUnicode(PVRTuint32 c32)
{
	// Check that this value isn't a UTF16 surrogate mask.
	if(c32 >= UTF16_SURG_H_MARK && c32 <= UTF16_SURG_L_END)
		return false;
	// Check non-char values
	if(c32 >= UNICODE_NONCHAR_MARK && c32 <= UNICODE_NONCHAR_END)
		return false;
	// Check reserved values
	if((c32 & UNICODE_RESERVED) == UNICODE_RESERVED)
		return false;
	// Check max value.
	if(c32 > UNICODE_MAX)
		return false;

	return true;
}

/*!***************************************************************************
 @Function			PVRTUnicodeUTF8ToUTF32
 @Input				pUTF8			A UTF8 string, which is null terminated.
 @Output			aUTF32			An array of Unicode code points.
 @Returns			Success or failure. 
 @Description		Decodes a UTF8-encoded string in to Unicode code points
					(UTF32). If pUTF8 is not null terminated, the results are 
					undefined.
*****************************************************************************/
EPVRTError PVRTUnicodeUTF8ToUTF32(const PVRTuint8* const pUTF8, CPVRTArray<PVRTuint32>& aUTF32)						
{
	unsigned int uiTailLen, uiIndex;
	unsigned int uiBytes = (unsigned int) strlen((const char*)pUTF8);
	PVRTuint32 c32;

	const PVRTuint8* pC = pUTF8;
	while(*pC)
	{
		// Quick optimisation for ASCII characters
		while(*pC && *pC < VALID_ASCII)
		{
			aUTF32.Append(*pC++);
		}
		// Done
		if(!*pC)			
			break;

		c32 = *pC++;
		uiTailLen = c_u8UTF8Lengths[c32];

		// Check for invalid tail length. Maximum 4 bytes for each UTF8 character.
		// Also check to make sure the tail length is inside the provided buffer.
		if(uiTailLen == 0 || (pC + uiTailLen > pUTF8 + uiBytes))
			return PVR_OVERFLOW;

		c32 &= (TAIL_MASK >> uiTailLen);	// Get the data out of the first byte. This depends on the length of the tail.

		// Get the data out of each tail byte
		uiIndex = 0;
		while(uiIndex < uiTailLen)
		{
			if((pC[uiIndex] & 0xC0) != 0x80)
				return PVR_FAIL;		// Invalid tail byte!

			c32 = (c32 << BYTES_PER_TAIL) + (pC[uiIndex] & TAIL_MASK);
			uiIndex++;
		}

		pC += uiIndex;

		// Check overlong values.
		if(c32 < c_u32MinVals[uiTailLen])
			return PVR_FAIL;		
		
		if(!CheckGenericUnicode(c32))
			return PVR_FAIL;

		// OK
		aUTF32.Append(c32);
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function			PVRTUnicodeUTF16ToUTF32
 @Input				pUTF16			A UTF16 string, which is null terminated.
 @Output			aUTF32			An array of Unicode code points.
 @Returns			Success or failure. 
 @Description		Decodes a UTF16-encoded string in to Unicode code points
					(UTF32). If pUTF16 is not null terminated, the results are 
					undefined.
*****************************************************************************/
EPVRTError PVRTUnicodeUTF16ToUTF32(const PVRTuint16* const pUTF16, CPVRTArray<PVRTuint32>& aUTF32)
{
	const PVRTuint16* pC = pUTF16;

	// Determine the number of shorts
	while(*++pC && (pC - pUTF16) < MAX_LEN);
	unsigned int uiBufferLen = (unsigned int) (pC - pUTF16);

	if(uiBufferLen == MAX_LEN)
		return PVR_OVERFLOW;		// Probably not NULL terminated.	

	// Reset to start.
	pC = pUTF16;

	PVRTuint32 c32;
	while(*pC)
	{
		// Straight copy. We'll check for surrogate pairs next...
		c32 = *pC++;

		// Check surrogate pair
		if(c32 >= UTF16_SURG_H_MARK && c32 <= UTF16_SURG_H_END)
		{
			// Make sure the next 2 bytes are in range...
			if(pC + 1 > pUTF16 + uiBufferLen || *pC == 0)
				return PVR_OVERFLOW;

			// Check that the next value is in the low surrogate range
			if(*pC < UTF16_SURG_L_MARK || *pC > UTF16_SURG_L_END)
				return PVR_FAIL;

			// Decode
			c32 = ((c32 - UTF16_SURG_H_MARK) << 10) + (*pC - UTF16_SURG_L_MARK) + 0x10000;
			pC++;
		}

		if(!CheckGenericUnicode(c32))
			return PVR_FAIL;

		// OK
		aUTF32.Append(c32);
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function			PVRTUnicodeUTF8Length
 @Input				pUTF8			A UTF8 string, which is null terminated.
 @Returns			The length of the string, in Unicode code points.
 @Description		Calculates the length of a UTF8 string. If pUTF8 is 
					not null terminated, the results are undefined.
*****************************************************************************/
unsigned int PVRTUnicodeUTF8Length(const PVRTuint8* const pUTF8)
{
	const PVRTuint8* pC = pUTF8;

	unsigned int charCount = 0;
	unsigned int mask;
	while(*pC)
	{
		// Quick optimisation for ASCII characters
		const PVRTuint8* pStart = pC;
		while(*pC && *pC < VALID_ASCII)
			pC++;

		charCount += (unsigned int) (pC - pStart);

		// Done
		if(!*pC)	
			break;
		
		mask = *pC & 0xF0;
		switch(mask)
		{
		case 0xF0: pC++;
		case 0xE0: pC++;
		case 0xC0: pC++;
			break;
		default:
			_ASSERT(!"Invalid tail byte!");
			return 0;
		}

		pC++;
		charCount++;
	}

	return charCount;
}

/*!***************************************************************************
 @Function			PVRTUnicodeUTF16Length
 @Input				pUTF16			A UTF16 string, which is null terminated.
 @Returns			The length of the string, in Unicode code points.
 @Description		Calculates the length of a UTF16 string.
					If pUTF16 is not null terminated, the results are 
					undefined.
*****************************************************************************/
unsigned int PVRTUnicodeUTF16Length(const PVRTuint16* const pUTF16)
{
	const PVRTuint16* pC = pUTF16;	
	unsigned int charCount = 0;
	while(*pC && (pC - pUTF16) < MAX_LEN)
	{
		if(	pC[0] >= UTF16_SURG_H_MARK && pC[0] <= UTF16_SURG_H_END
		 && pC[1] >= UTF16_SURG_L_MARK && pC[0] <= UTF16_SURG_L_END)
		{
			pC += 2;
		}
		else
		{
			pC += 1;
		}

		charCount++;
	}

	return charCount;
}

/*!***************************************************************************
 @Function			PVRTUnicodeValidUTF8
 @Input				pUTF8			A UTF8 string, which is null terminated.
 @Returns			true or false
 @Description		Checks whether the encoding of a UTF8 string is valid.
					If pUTF8 is not null terminated, the results are undefined.
*****************************************************************************/
bool PVRTUnicodeValidUTF8(const PVRTuint8* const pUTF8)
{
	unsigned int uiTailLen, uiIndex;
	unsigned int uiBytes = (unsigned int) strlen((const char*)pUTF8);
	const PVRTuint8* pC = pUTF8;
	while(*pC)
	{
		// Quick optimisation for ASCII characters
		while(*pC && *pC < VALID_ASCII)	pC++;
		// Done?
		if(!*pC)			
			break;

		PVRTuint32 c32 = *pC++;
		uiTailLen = c_u8UTF8Lengths[c32];

		// Check for invalid tail length. Maximum 4 bytes for each UTF8 character.
		// Also check to make sure the tail length is inside the provided buffer.
		if(uiTailLen == 0 || (pC + uiTailLen > pUTF8 + uiBytes))
			return false;

		// Get the data out of each tail byte
		uiIndex = 0;
		while(uiIndex < uiTailLen)
		{
			if((pC[uiIndex] & 0xC0) != 0x80)
				return false;		// Invalid tail byte!
			
			c32 = (c32 << BYTES_PER_TAIL) + (pC[uiIndex] & TAIL_MASK);
			uiIndex++;
		}
		
		pC += uiIndex;

		// Check overlong values.
		if(c32 < c_u32MinVals[uiTailLen])
			return false;		
		if(!CheckGenericUnicode(c32))
			return false;
	}

	return true;
}

/*****************************************************************************
 End of file (PVRTUnicode.cpp)
*****************************************************************************/

