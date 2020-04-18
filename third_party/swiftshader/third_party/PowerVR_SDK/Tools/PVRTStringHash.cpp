/******************************************************************************

 @File         PVRTStringHash.cpp

 @Title        String Hash

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     All

 @Description  Inherits from PVRTString to include PVRTHash functionality for
               quick string compares.

******************************************************************************/
#include "PVRTGlobal.h"
#include "PVRTStringHash.h"

/*!***********************************************************************
@Function			CPVRTString
@Input				_Ptr	A string
@Input				_Count	Length of _Ptr
@Description		Constructor
************************************************************************/
CPVRTStringHash::CPVRTStringHash(const char* _Ptr, size_t _Count) : 
	m_String(_Ptr, _Count)
{
	m_Hash = CPVRTHash::MakeHash(m_String);
}

/*!***********************************************************************
@Function			CPVRTString
@Input				_Right	A string
@Description		Constructor
************************************************************************/
CPVRTStringHash::CPVRTStringHash(const CPVRTString& _Right) :
	m_String(_Right)
{
	m_Hash = CPVRTHash::MakeHash(m_String);
}

/*!***********************************************************************
@Function			CPVRTString
@Description		Constructor
************************************************************************/
CPVRTStringHash::CPVRTStringHash()
{
}

/*!***********************************************************************
@Function			append
@Input				_Ptr	A string
@Returns			Updated string
@Description		Appends a string
*************************************************************************/
CPVRTStringHash& CPVRTStringHash::append(const char* _Ptr)
{
	m_String.append(_Ptr);
	m_Hash = CPVRTHash::MakeHash(m_String);
	return *this;
}

/*!***********************************************************************
@Function			append
@Input				_Str	A string
@Returns			Updated string
@Description		Appends a string
*************************************************************************/
CPVRTStringHash& CPVRTStringHash::append(const CPVRTString& _Str)
{
	m_String.append(_Str);
	m_Hash = CPVRTHash::MakeHash(m_String);
	return *this;
}

/*!***********************************************************************
@Function			assign
@Input				_Ptr A string
@Returns			Updated string
@Description		Assigns the string to the string _Ptr
*************************************************************************/
CPVRTStringHash& CPVRTStringHash::assign(const char* _Ptr)
{
	m_String.assign(_Ptr);
	m_Hash = CPVRTHash::MakeHash(m_String);
	return *this;
}

/*!***********************************************************************
@Function			assign
@Input				_Str A string
@Returns			Updated string
@Description		Assigns the string to the string _Str
*************************************************************************/
CPVRTStringHash& CPVRTStringHash::assign(const CPVRTString& _Str)
{
	m_String.assign(_Str);
	m_Hash = CPVRTHash::MakeHash(m_String);
	return *this;
}

/*!***********************************************************************
@Function		==
@Input			_Str 	A string to compare with
@Returns		True if they match
@Description	== Operator
*************************************************************************/
bool CPVRTStringHash::operator==(const CPVRTStringHash& _Str) const
{
	return (m_Hash == _Str.Hash());
}

/*!***********************************************************************
@Function		==
@Input			Hash 	A hash to compare with
@Returns		True if they match
@Description	== Operator
*************************************************************************/
bool CPVRTStringHash::operator==(const CPVRTHash& Hash) const
{
	return (m_Hash == Hash);
}

/*!***********************************************************************
@Function		==
@Input			_Str 	A string to compare with
@Returns		True if they match
@Description	== Operator. This function performs a strcmp()
				as it's more efficient to strcmp than to hash the string
				for every comparison.
*************************************************************************/
bool CPVRTStringHash::operator==(const char* _Str) const
{
	return (m_String.compare(_Str) == 0);
}

/*!***********************************************************************
@Function		==
@Input			_Str 	A string to compare with
@Returns		True if they match
@Description	== Operator. This function performs a strcmp()
				as it's more efficient to strcmp than to hash the string
				for every comparison.
*************************************************************************/
bool CPVRTStringHash::operator==(const CPVRTString& _Str) const
{
	return (m_String.compare(_Str) == 0);
}

/*!***********************************************************************
@Function			!=
@Input				_Str 	A string to compare with
@Returns			True if they don't match
@Description		!= Operator
*************************************************************************/
bool CPVRTStringHash::operator!=(const CPVRTStringHash& _Str) const
{
	return (m_Hash != _Str.Hash());
}

/*!***********************************************************************
@Function		!=
@Input			Hash 	A hash to compare with
@Returns		True if they match
@Description	!= Operator
*************************************************************************/
bool CPVRTStringHash::operator!=(const CPVRTHash& Hash) const
{
	return (m_Hash != Hash);
}

/*!***********************************************************************
@Function			String
@Returns			The original string
@Description		Returns the original, base string.
*************************************************************************/
const CPVRTString& CPVRTStringHash::String() const
{
	return m_String;
}

/*!***********************************************************************
@Function			Hash
@Returns			The hash
@Description		Returns the hash of the base string
*************************************************************************/
const CPVRTHash& CPVRTStringHash::Hash() const
{
	return m_Hash;
}

/*!***************************************************************************
@Function		c_str
@Return			The original string.
@Description	Returns the base string as a const char*.
*****************************************************************************/
const char* CPVRTStringHash::c_str() const
{
	return m_String.c_str();
}

