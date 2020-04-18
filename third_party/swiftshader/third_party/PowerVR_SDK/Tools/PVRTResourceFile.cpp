/******************************************************************************

 @File         PVRTResourceFile.cpp

 @Title        PVRTResourceFile.cpp

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Simple resource file wrapper

******************************************************************************/

#include "PVRTResourceFile.h"
#include <stdio.h>
#include <string.h>

#include "PVRTResourceFile.h"
#include "PVRTString.h"
#include "PVRTMemoryFileSystem.h"

CPVRTString CPVRTResourceFile::s_ReadPath;

static void* LoadFileFunc(const char* pFilename, char** pData, size_t &size)
{
	size = 0;
	
	FILE* pFile = fopen(pFilename, "rb");

	if (pFile)
	{
		// Get the file size
		fseek(pFile, 0, SEEK_END);
		size = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		// read the data
		char* pTmp = new char[size];
		size_t BytesRead = fread(pTmp, 1, size, pFile);

		if (BytesRead != size)
		{
			delete [] pTmp;
			pTmp = NULL;
			size = 0;
		}
		else
			*pData = pTmp;

		fclose(pFile);
		return pTmp;
	}

	return 0;
}

static bool ReleaseFileFunc(void* handle)
{
	if(handle)
	{
		delete[] (char*) handle;
		return true;
	}

	return false;
}

PFNLoadFileFunc CPVRTResourceFile::s_pLoadFileFunc = &LoadFileFunc;
PFNReleaseFileFunc CPVRTResourceFile::s_pReleaseFileFunc = &ReleaseFileFunc;

/*!***************************************************************************
@Function			SetReadPath
@Input				pszReadPath The path where you would like to read from
@Description		Sets the read path
*****************************************************************************/
void CPVRTResourceFile::SetReadPath(const char* const pszReadPath)
{
	s_ReadPath = (pszReadPath) ? pszReadPath : "";
}

/*!***************************************************************************
@Function			GetReadPath
@Returns			The currently set read path
@Description		Returns the currently set read path
*****************************************************************************/
CPVRTString CPVRTResourceFile::GetReadPath()
{
	return CPVRTString(s_ReadPath);
}

/*!***************************************************************************
@Function			SetLoadReleaseFunctions
@Input				pLoadFileFunc Function to use for opening a file
@Input				pReleaseFileFunc Function to release any data allocated by the load function
@Description		This function is used to override the CPVRTResource file loading functions. If
                    you pass NULL in as the load function CPVRTResource will use the default functions.
*****************************************************************************/
void CPVRTResourceFile::SetLoadReleaseFunctions(void* pLoadFileFunc, void* pReleaseFileFunc)
{
	if(pLoadFileFunc)
	{
		s_pLoadFileFunc = (PFNLoadFileFunc) pLoadFileFunc;
		s_pReleaseFileFunc = (PFNReleaseFileFunc) pReleaseFileFunc;
	}
	else
	{
		s_pLoadFileFunc = &LoadFileFunc;
		s_pReleaseFileFunc = &ReleaseFileFunc;
	}
}

/*!***************************************************************************
@Function			CPVRTResourceFile
@Input				pszFilename Name of the file you would like to open
@Description		Constructor
*****************************************************************************/
CPVRTResourceFile::CPVRTResourceFile(const char* const pszFilename) :
	m_bOpen(false),
	m_bMemoryFile(false),
	m_Size(0),
	m_pData(0),
	m_Handle(0)
{
	CPVRTString Path(s_ReadPath);
	Path += pszFilename;

	m_Handle = s_pLoadFileFunc(Path.c_str(), (char**) &m_pData, m_Size);
	m_bOpen = (m_pData && m_Size) != 0;

	if (!m_bOpen)
	{
		m_bOpen = m_bMemoryFile = CPVRTMemoryFileSystem::GetFile(pszFilename, (const void**)(&m_pData), &m_Size);
	}
}

/*!***************************************************************************
@Function			CPVRTResourceFile
@Input				pData A pointer to the data you would like to use
@Input				i32Size The size of the data
@Description		Constructor
*****************************************************************************/
CPVRTResourceFile::CPVRTResourceFile(const char* pData, size_t i32Size) :
	m_bOpen(true),
	m_bMemoryFile(true),
	m_Size(i32Size),
	m_pData(pData),
	m_Handle(0)
{
}

/*!***************************************************************************
@Function			~CPVRTResourceFile
@Description		Destructor
*****************************************************************************/
CPVRTResourceFile::~CPVRTResourceFile()
{
	Close();
}

/*!***************************************************************************
@Function			IsOpen
@Returns			true if the file is open
@Description		Is the file open
*****************************************************************************/
bool CPVRTResourceFile::IsOpen() const
{
	return m_bOpen;
}

/*!***************************************************************************
@Function			IsMemoryFile
@Returns			true if the file was opened from memory
@Description		Was the file opened from memory
*****************************************************************************/
bool CPVRTResourceFile::IsMemoryFile() const
{
	return m_bMemoryFile;
}

/*!***************************************************************************
@Function			Size
@Returns			The size of the opened file
@Description		Returns the size of the opened file
*****************************************************************************/
size_t CPVRTResourceFile::Size() const
{
	return m_Size;
}

/*!***************************************************************************
@Function			DataPtr
@Returns			A pointer to the file data
@Description		Returns a pointer to the file data
*****************************************************************************/
const void* CPVRTResourceFile::DataPtr() const
{
	return m_pData;
}

/*!***************************************************************************
@Function			Close
@Description		Closes the file
*****************************************************************************/
void CPVRTResourceFile::Close()
{
	if (m_bOpen)
	{
		if (!m_bMemoryFile && s_pReleaseFileFunc)
		{
			s_pReleaseFileFunc(m_Handle);
		}

		m_bMemoryFile = false;
		m_bOpen = false;
		m_pData = 0;
		m_Size = 0;
	}
}

/****************************************************************************
** class CPVRTMemoryFileSystem
****************************************************************************/
CPVRTMemoryFileSystem::CAtExit CPVRTMemoryFileSystem::s_AtExit;
CPVRTMemoryFileSystem::SFileInfo* CPVRTMemoryFileSystem::s_pFileInfo = 0;
int CPVRTMemoryFileSystem::s_i32Capacity = 0;
int CPVRTMemoryFileSystem::s_i32NumFiles = 0;

/*!***************************************************************************
@Function		Destructor
@Description	Destructor of CAtExit class. Workaround for platforms that
		        don't support the atexit() function. This deletes any memory
				file system data.
*****************************************************************************/
CPVRTMemoryFileSystem::CAtExit::~CAtExit()
{
	for (int i = 0; i < CPVRTMemoryFileSystem::s_i32NumFiles; ++i)
	{
		if (CPVRTMemoryFileSystem::s_pFileInfo[i].bAllocated)
		{
			delete [] (char*)CPVRTMemoryFileSystem::s_pFileInfo[i].pszFilename;
			delete [] (char*)CPVRTMemoryFileSystem::s_pFileInfo[i].pBuffer;
		}
	}
	delete [] CPVRTMemoryFileSystem::s_pFileInfo;
}

CPVRTMemoryFileSystem::CPVRTMemoryFileSystem(const char* pszFilename, const void* pBuffer, size_t Size, bool bCopy)
{
	RegisterMemoryFile(pszFilename, pBuffer, Size, bCopy);
}

/*!***************************************************************************
@Function		RegisterMemoryFile
@Input			pszFilename		Name of file to register
@Input			pBuffer			Pointer to file data
@Input			Size			File size
@Input			bCopy			Name and data should be copied?
@Description	Registers a block of memory as a file that can be looked up
				by name.
*****************************************************************************/
void CPVRTMemoryFileSystem::RegisterMemoryFile(const char* pszFilename, const void* pBuffer, size_t Size, bool bCopy)
{
	if (s_i32NumFiles == s_i32Capacity)
	{
		SFileInfo* pFileInfo = new SFileInfo[s_i32Capacity + 10];
		memcpy(pFileInfo, s_pFileInfo, sizeof(SFileInfo) * s_i32Capacity);
		delete [] s_pFileInfo;
		s_pFileInfo = pFileInfo;
		s_i32Capacity += 10;
	}

	s_pFileInfo[s_i32NumFiles].pszFilename = pszFilename;
	s_pFileInfo[s_i32NumFiles].pBuffer = pBuffer;
	if (bCopy)
	{
		char* pszNewFilename = new char[strlen(pszFilename) + 1];
		strcpy(pszNewFilename, pszFilename);
		s_pFileInfo[s_i32NumFiles].pszFilename = pszNewFilename;

		void* pszNewBuffer = new char[Size];
		memcpy(pszNewBuffer, pBuffer, Size);
		s_pFileInfo[s_i32NumFiles].pBuffer = pszNewBuffer;
	}
	s_pFileInfo[s_i32NumFiles].Size = Size;
	s_pFileInfo[s_i32NumFiles].bAllocated = bCopy;
	++s_i32NumFiles;
}

/*!***************************************************************************
@Function		GetFile
@Input			pszFilename		Name of file to open
@Output			ppBuffer		Pointer to file data
@Output			pSize			File size
@Return			true if the file was found in memory, false otherwise
@Description	Looks up a file in the memory file system by name. Returns a
				pointer to the file data as well as its size on success.
*****************************************************************************/
bool CPVRTMemoryFileSystem::GetFile(const char* pszFilename, const void** ppBuffer, size_t* pSize)
{
	for (int i = 0; i < s_i32NumFiles; ++i)
	{
		if (strcmp(s_pFileInfo[i].pszFilename, pszFilename) == 0)
		{
			if (ppBuffer) *ppBuffer = s_pFileInfo[i].pBuffer;
			if (pSize) *pSize = s_pFileInfo[i].Size;
			return true;
		}
	}
	return false;
}

/*!***************************************************************************
@Function		GetNumFiles
@Return			The number of registered files
@Description	Getter for the number of registered files
*****************************************************************************/
int CPVRTMemoryFileSystem::GetNumFiles()
{
	return s_i32NumFiles;
}

/*!***************************************************************************
@Function		GetFilename
@Input			i32Index		Index of file
@Return			A pointer to the filename of the requested file
@Description	Looks up a file in the memory file system by name. Returns a
				pointer to the file data as well as its size on success.
*****************************************************************************/
const char* CPVRTMemoryFileSystem::GetFilename(int i32Index)
{
	if (i32Index < 0 || i32Index > s_i32NumFiles) return 0;

	return s_pFileInfo[i32Index].pszFilename;
}


/*****************************************************************************
 End of file (PVRTResourceFile.cpp)
*****************************************************************************/
