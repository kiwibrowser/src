/*!****************************************************************************

 @file         PVRTResourceFile.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Simple resource file wrapper

******************************************************************************/
#ifndef _PVRTRESOURCEFILE_H_
#define _PVRTRESOURCEFILE_H_

#include <stdlib.h>
#include "PVRTString.h"

typedef void* (*PFNLoadFileFunc)(const char*, char** pData, size_t &size);
typedef bool  (*PFNReleaseFileFunc)(void* handle);

/*!***************************************************************************
 @class CPVRTResourceFile
 @brief Simple resource file wrapper
*****************************************************************************/
class CPVRTResourceFile
{
public:
	/*!***************************************************************************
	@fn       			SetReadPath
	@param[in]			pszReadPath The path where you would like to read from
	@brief      		Sets the read path
	*****************************************************************************/
	static void SetReadPath(const char* pszReadPath);

	/*!***************************************************************************
	@fn       			GetReadPath
	@return 			The currently set read path
	@brief      		Returns the currently set read path
	*****************************************************************************/
	static CPVRTString GetReadPath();

	/*!***************************************************************************
	@fn       			SetLoadReleaseFunctions
	@param[in]			pLoadFileFunc Function to use for opening a file
	@param[in]			pReleaseFileFunc Function to release any data allocated by the load function
	@brief      		This function is used to override the CPVRTResource file loading functions. If
	                    you pass NULL in as the load function CPVRTResource will use the default functions.
	*****************************************************************************/
	static void SetLoadReleaseFunctions(void* pLoadFileFunc, void* pReleaseFileFunc);

	/*!***************************************************************************
	@brief     			CPVRTResourceFile constructor
	@param[in]			pszFilename Name of the file you would like to open
	*****************************************************************************/
	CPVRTResourceFile(const char* pszFilename);

	/*!***************************************************************************
	@brief     			CPVRTResourceFile constructor
	@param[in]			pData A pointer to the data you would like to use
	@param[in]			i32Size The size of the data      		
	*****************************************************************************/
	CPVRTResourceFile(const char* pData, size_t i32Size);

	/*!***************************************************************************
	@fn       			~CPVRTResourceFile
	@brief      		Destructor
	*****************************************************************************/
	virtual ~CPVRTResourceFile();

	/*!***************************************************************************
	@fn       			IsOpen
	@return 			true if the file is open
	@brief      		Is the file open
	*****************************************************************************/
	bool IsOpen() const;

	/*!***************************************************************************
	@fn       			IsMemoryFile
	@return 			true if the file was opened from memory
	@brief      		Was the file opened from memory
	*****************************************************************************/
	bool IsMemoryFile() const;

	/*!***************************************************************************
	@fn       			Size
	@return 			The size of the opened file
	@brief      		Returns the size of the opened file
	*****************************************************************************/
	size_t Size() const;

	/*!***************************************************************************
	@fn       			DataPtr
	@return 			A pointer to the file data
	@brief      		Returns a pointer to the file data. If the data is expected
						to be a string don't assume that it is null-terminated.
	*****************************************************************************/
	const void* DataPtr() const;

	/*!***************************************************************************
	@fn       			Close
	@brief      		Closes the file
	*****************************************************************************/
	void Close();

protected:
	bool m_bOpen;
	bool m_bMemoryFile;
	size_t m_Size;
	const char* m_pData;
	void *m_Handle;

	static CPVRTString s_ReadPath;
	static PFNLoadFileFunc s_pLoadFileFunc;
	static PFNReleaseFileFunc s_pReleaseFileFunc;
};

#endif // _PVRTRESOURCEFILE_H_

/*****************************************************************************
 End of file (PVRTResourceFile.h)
*****************************************************************************/

