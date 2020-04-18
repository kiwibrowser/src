/*!****************************************************************************

 @file         PVRTHash.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        A simple hash class which uses TEA to hash a string or given data
               into a 32-bit unsigned int.

******************************************************************************/

#ifndef PVRTHASH_H
#define PVRTHASH_H

#include "PVRTString.h"
#include "PVRTGlobal.h"

/*!****************************************************************************
 @class         CPVRTHash
 @brief         A simple hash class which uses TEA to hash a string or other given 
                data into a 32-bit unsigned int.
******************************************************************************/
class CPVRTHash
{
public:
	/*!***************************************************************************
	@brief      	Constructor
	*****************************************************************************/
	CPVRTHash() : m_uiHash(0) {}

	/*!***************************************************************************
	@brief      	Copy Constructor
	@param[in]		rhs         CPVRTHash to copy.
	*****************************************************************************/
	CPVRTHash(const CPVRTHash& rhs) : m_uiHash(rhs.m_uiHash) {}

	/*!***************************************************************************
	@brief      	Overloaded constructor
	@param[in]		String      CPVRTString to create the CPVRTHash with.
	*****************************************************************************/
	CPVRTHash(const CPVRTString& String) : m_uiHash(0)
	{
		if(String.length() > 0)		// Empty string. Don't set.
		{
			m_uiHash = MakeHash(String);
		}
	}

	/*!***************************************************************************
	@brief      	Overloaded constructor
	@param[in]		c_pszString String to create the CPVRTHash with.
	*****************************************************************************/
	CPVRTHash(const char* c_pszString) : m_uiHash(0)
	{
		_ASSERT(c_pszString);
		if(c_pszString[0] != 0)		// Empty string. Don't set.
		{
			m_uiHash = MakeHash(c_pszString);	
		}
	}

	/*!***************************************************************************
	@brief      	Overloaded constructor
	@param[in]		pData
	@param[in]		dataSize
	@param[in]		dataCount
	*****************************************************************************/
	CPVRTHash(const void* pData, unsigned int dataSize, unsigned int dataCount) : m_uiHash(0)
	{
		_ASSERT(pData);
		_ASSERT(dataSize > 0);

		if(dataCount > 0)
		{
			m_uiHash = MakeHash(pData, dataSize, dataCount);
		}
	}

	/*!***************************************************************************
	@brief      	Overloaded assignment.
	@param[in]		rhs
	@return			CPVRTHash &	
	*****************************************************************************/
	CPVRTHash& operator=(const CPVRTHash& rhs)
	{
		if(this != &rhs)
		{
			m_uiHash = rhs.m_uiHash;
		}

		return *this;
	}

	/*!***************************************************************************
	@brief      	Converts to unsigned int.
	@return			int	
	*****************************************************************************/
	operator unsigned int() const
	{
		return m_uiHash;
	}

	/*!***************************************************************************
	@brief      	Generates a hash from a CPVRTString.
	@param[in]		String
	@return			The hash.
	*****************************************************************************/
	static CPVRTHash MakeHash(const CPVRTString& String)
	{
		if(String.length() > 0)
			return MakeHash(String.c_str(), sizeof(char), (unsigned int) String.length());

		return CPVRTHash();
	}

	/*!***************************************************************************
	@brief      	Generates a hash from a null terminated char array.
	@param[in]		c_pszString
	@return         The hash.
	*****************************************************************************/
	static CPVRTHash MakeHash(const char* c_pszString)
	{
		_ASSERT(c_pszString);

		if(c_pszString[0] == 0)
			return CPVRTHash();

		const char* pCursor = c_pszString;
		while(*pCursor) pCursor++;
		return MakeHash(c_pszString, sizeof(char), (unsigned int) (pCursor - c_pszString));
	}
		
	/*!***************************************************************************
	@brief      	Generates a hash from generic data. This function uses the
					32-bit Fowler/Noll/Vo algorithm which trades efficiency for
					slightly increased risk of collisions. This algorithm is
					public domain. More information can be found at:
					http://www.isthe.com/chongo/tech/comp/fnv/.
	@param[in]		pData
	@param[in]		dataSize
	@param[in]		dataCount
	@return			unsigned int			The hash.
	*****************************************************************************/
	static CPVRTHash MakeHash(const void* pData, unsigned int dataSize, unsigned int dataCount)
	{
		_ASSERT(pData);
		_ASSERT(dataSize > 0);

#define FNV_PRIME		16777619U
#define FNV_OFFSETBIAS	2166136261U

		if(dataCount == 0)
			return CPVRTHash();

		CPVRTHash pvrHash;
		unsigned char* p = (unsigned char*)pData;
		pvrHash.m_uiHash = FNV_OFFSETBIAS;
		for(unsigned int i = 0; i < dataSize * dataCount; ++i)
		{
			pvrHash.m_uiHash = (pvrHash.m_uiHash * FNV_PRIME) ^ p[i];
		}
		
		return pvrHash;
	}

private:
	unsigned int		m_uiHash;		/// The hashed data.
};

#endif

