/*!****************************************************************************

 @file         PVRTStringHash.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Inherits from PVRTString to include PVRTHash functionality for
               quick string compares.

******************************************************************************/
#ifndef PVRTSTRINGHASH_H
#define PVRTSTRINGHASH_H

#include "PVRTString.h"
#include "PVRTHash.h"

/*!***********************************************************************
 @class        CPVRTStringHash
 @brief        Inherits from PVRTString to include PVRTHash functionality for
               quick string compares.
*************************************************************************/
class CPVRTStringHash
{
public:
	/*!***********************************************************************
	@brief      		Constructor
	@param[in]			_Ptr	A string
	@param[in]			_Count	Length of _Ptr
	************************************************************************/
	explicit CPVRTStringHash(const char* _Ptr, size_t _Count = CPVRTString::npos);

	/*!***********************************************************************
	@brief      		Constructor
	@param[in]			_Right	A string
	************************************************************************/
	explicit CPVRTStringHash(const CPVRTString& _Right);

	/*!***********************************************************************
	@brief      		Constructor
	************************************************************************/
	CPVRTStringHash();

	/*!***********************************************************************
	@brief      		Appends a string
	@param[in]			_Ptr	A string
	@return 			Updated string
	*************************************************************************/
	CPVRTStringHash& append(const char* _Ptr);

	/*!***********************************************************************
	@brief      		Appends a string
	@param[in]			_Str	A string
	@return 			Updated string
	*************************************************************************/
	CPVRTStringHash& append(const CPVRTString& _Str);

	/*!***********************************************************************
	@brief      		Assigns the string to the string _Ptr
	@param[in]			_Ptr A string
	@return 			Updated string
	*************************************************************************/
	CPVRTStringHash& assign(const char* _Ptr);

	/*!***********************************************************************
	@brief      		Assigns the string to the string _Str
	@param[in]			_Str A string
	@return 			Updated string
	*************************************************************************/
	CPVRTStringHash& assign(const CPVRTString& _Str);

	/*!***********************************************************************
	@brief      	== Operator. This function compares the hash values of
					the string.
	@param[in]		_Str 	A hashed string to compare with
	@return 		True if they match
	*************************************************************************/
	bool operator==(const CPVRTStringHash& _Str) const;

	/*!***********************************************************************
	@brief      	== Operator. This function performs a strcmp()
					as it's more efficient to strcmp than to hash the string
					for every comparison.
	@param[in]		_Str 	A string to compare with
	@return 		True if they match
	*************************************************************************/
	bool operator==(const char* _Str) const;

	/*!***********************************************************************
	@brief      	== Operator. This function performs a strcmp()
					as it's more efficient to strcmp than to hash the string
					for every comparison.
	@param[in]		_Str 	A string to compare with
	@return 		True if they match
	*************************************************************************/
	bool operator==(const CPVRTString& _Str) const;

	/*!***********************************************************************
	@brief      	== Operator. This function compares the hash values of
					the string.
	@param[in]		Hash 	A Hash to compare with
	@return 		True if they match
	*************************************************************************/
	bool operator==(const CPVRTHash& Hash) const;

	/*!***********************************************************************
	@brief      	!= Operator
	@param[in]		_Str 	A string to compare with
	@return 		True if they don't match
	*************************************************************************/
	bool operator!=(const CPVRTStringHash& _Str) const;

	/*!***********************************************************************
	@brief      	!= Operator. This function compares the hash values of
					the string.
	@param[in]		Hash 	A Hash to compare with
	@return 		True if they match
	*************************************************************************/
	bool operator!=(const CPVRTHash& Hash) const;

	/*!***********************************************************************
	@fn       		String
	@return 		The original string
	@brief      	Returns the original, base string.
	*************************************************************************/
	const CPVRTString& String() const;

	/*!***********************************************************************
	@brief      	Returns the hash of the base string
	@fn       		Hash
	@return 		The hash
	*************************************************************************/
	const CPVRTHash& Hash() const;

	/*!***************************************************************************
	@fn       		c_str
	@return			The original string.
	@brief      	Returns the base string as a const char*.
	*****************************************************************************/
	const char* c_str() const;

private:
	CPVRTString			m_String;
	CPVRTHash			m_Hash;
};

#endif

