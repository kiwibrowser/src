/*!****************************************************************************

 @file         PVRTMemoryFileSystem.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Memory file system for resource files.

******************************************************************************/
#ifndef _PVRTMEMORYFILE_H_
#define _PVRTMEMORYFILE_H_

#include "PVRTGlobal.h"
#include <stddef.h>

/*!****************************************************************************
 @class        CPVRTMemoryFileSystem
 @brief        Memory file system for resource files.
******************************************************************************/
class CPVRTMemoryFileSystem
{
public:
	/*!***************************************************************************
     @brief      	Constructor. Creates a CPVRTMemoryFileSystem object based on the parameters supplied.
	 @param[in]		pszFilename		Name of file to register
	 @param[in]		pBuffer			Pointer to file data
	 @param[in]		Size			File size
	 @param[in]		bCopy			Name and data should be copied?
	*****************************************************************************/
    CPVRTMemoryFileSystem(const char* pszFilename, const void* pBuffer, size_t Size, bool bCopy = false);

	/*!***************************************************************************
	 @fn           	RegisterMemoryFile
	 @param[in]		pszFilename		Name of file to register
	 @param[in]		pBuffer			Pointer to file data
	 @param[in]		Size			File size
	 @param[in]		bCopy			Name and data should be copied?
	 @brief      	Registers a block of memory as a file that can be looked up
	                by name.
	*****************************************************************************/
	static void RegisterMemoryFile(const char* pszFilename, const void* pBuffer, size_t Size, bool bCopy = false);

	/*!***************************************************************************
	 @fn           	GetFile
	 @param[in]		pszFilename		Name of file to open
	 @param[out]	ppBuffer		Pointer to file data
	 @param[out]	pSize			File size
	 @return		true if the file was found in memory, false otherwise
	 @brief      	Looks up a file in the memory file system by name. Returns a
	                pointer to the file data as well as its size on success.
	*****************************************************************************/
	static bool GetFile(const char* pszFilename, const void** ppBuffer, size_t* pSize);

	/*!***************************************************************************
	 @fn           	GetNumFiles
	 @return		The number of registered files
	 @brief      	Getter for the number of registered files
	*****************************************************************************/
	static int GetNumFiles();

	/*!***************************************************************************
	 @fn           	GetFilename
	 @param[in]		i32Index		Index of file
	 @return		A pointer to the filename of the requested file
	 @brief      	Looks up a file in the memory file system by name. Returns a
	                pointer to the file data as well as its size on success.
	*****************************************************************************/
	static const char* GetFilename(int i32Index);

protected:
    /*!***************************************************************************
	 @class         CAtExit
	 @brief      	Provides a deconstructor for platforms that don't support the atexit() function.
	*****************************************************************************/
	class CAtExit
	{
	public:
		/*!***************************************************************************
		@brief      Destructor of CAtExit class. Workaround for platforms that
		            don't support the atexit() function. This deletes any memory
					file system data.
		*****************************************************************************/
		~CAtExit();
	};
	static CAtExit s_AtExit;

	friend class CAtExit;

    /*!***************************************************************************
	 @struct        SFileInfo
	 @brief      	Struct which contains information on a single file.
	*****************************************************************************/
	struct SFileInfo
	{
		const char* pszFilename;        ///< File name.
		const void* pBuffer;            ///< Pointer to file data.
		size_t Size;                    ///< File size.
		bool bAllocated;                ///< File was allocated. If true, this file will be deleted on exit.
	};
	static SFileInfo* s_pFileInfo;
	static int s_i32NumFiles;
	static int s_i32Capacity;
};

#endif // _PVRTMEMORYFILE_H_

/*****************************************************************************
 End of file (PVRTMemoryFileSystem.h)
*****************************************************************************/

