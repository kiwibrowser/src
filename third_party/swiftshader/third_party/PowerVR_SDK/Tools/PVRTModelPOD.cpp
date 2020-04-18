/******************************************************************************

 @File         PVRTModelPOD.cpp

 @Title        PVRTModelPOD

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Code to load POD files - models exported from MAX.

******************************************************************************/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "PVRTGlobal.h"
#if defined(BUILD_DX11)
#include "PVRTContext.h"
#endif
#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"
#include "PVRTQuaternion.h"
#include "PVRTVertex.h"
#include "PVRTBoneBatch.h"
#include "PVRTModelPOD.h"
#include "PVRTMisc.h"
#include "PVRTResourceFile.h"
#include "PVRTTrans.h"

/****************************************************************************
** Defines
****************************************************************************/
#define PVRTMODELPOD_TAG_MASK			(0x80000000)
#define PVRTMODELPOD_TAG_START			(0x00000000)
#define PVRTMODELPOD_TAG_END			(0x80000000)

#define CFAH		(1024)

/****************************************************************************
** Enumerations
****************************************************************************/
/*!****************************************************************************
 @Struct      EPODFileName
 @Brief       Enum for the binary pod blocks
******************************************************************************/
enum EPODFileName
{
	ePODFileVersion				= 1000,
	ePODFileScene,
	ePODFileExpOpt,
	ePODFileHistory,
	ePODFileEndiannessMisMatch  = -402456576,

	ePODFileColourBackground	= 2000,
	ePODFileColourAmbient,
	ePODFileNumCamera,
	ePODFileNumLight,
	ePODFileNumMesh,
	ePODFileNumNode,
	ePODFileNumMeshNode,
	ePODFileNumTexture,
	ePODFileNumMaterial,
	ePODFileNumFrame,
	ePODFileCamera,		// Will come multiple times
	ePODFileLight,		// Will come multiple times
	ePODFileMesh,		// Will come multiple times
	ePODFileNode,		// Will come multiple times
	ePODFileTexture,	// Will come multiple times
	ePODFileMaterial,	// Will come multiple times
	ePODFileFlags,
	ePODFileFPS,
	ePODFileUserData,
	ePODFileUnits,

	ePODFileMatName				= 3000,
	ePODFileMatIdxTexDiffuse,
	ePODFileMatOpacity,
	ePODFileMatAmbient,
	ePODFileMatDiffuse,
	ePODFileMatSpecular,
	ePODFileMatShininess,
	ePODFileMatEffectFile,
	ePODFileMatEffectName,
	ePODFileMatIdxTexAmbient,
	ePODFileMatIdxTexSpecularColour,
	ePODFileMatIdxTexSpecularLevel,
	ePODFileMatIdxTexBump,
	ePODFileMatIdxTexEmissive,
	ePODFileMatIdxTexGlossiness,
	ePODFileMatIdxTexOpacity,
	ePODFileMatIdxTexReflection,
	ePODFileMatIdxTexRefraction,
	ePODFileMatBlendSrcRGB,
	ePODFileMatBlendSrcA,
	ePODFileMatBlendDstRGB,
	ePODFileMatBlendDstA,
	ePODFileMatBlendOpRGB,
	ePODFileMatBlendOpA,
	ePODFileMatBlendColour,
	ePODFileMatBlendFactor,
	ePODFileMatFlags,
	ePODFileMatUserData,

	ePODFileTexName				= 4000,

	ePODFileNodeIdx				= 5000,
	ePODFileNodeName,
	ePODFileNodeIdxMat,
	ePODFileNodeIdxParent,
	ePODFileNodePos,
	ePODFileNodeRot,
	ePODFileNodeScale,
	ePODFileNodeAnimPos,
	ePODFileNodeAnimRot,
	ePODFileNodeAnimScale,
	ePODFileNodeMatrix,
	ePODFileNodeAnimMatrix,
	ePODFileNodeAnimFlags,
	ePODFileNodeAnimPosIdx,
	ePODFileNodeAnimRotIdx,
	ePODFileNodeAnimScaleIdx,
	ePODFileNodeAnimMatrixIdx,
	ePODFileNodeUserData,

	ePODFileMeshNumVtx			= 6000,
	ePODFileMeshNumFaces,
	ePODFileMeshNumUVW,
	ePODFileMeshFaces,
	ePODFileMeshStripLength,
	ePODFileMeshNumStrips,
	ePODFileMeshVtx,
	ePODFileMeshNor,
	ePODFileMeshTan,
	ePODFileMeshBin,
	ePODFileMeshUVW,			// Will come multiple times
	ePODFileMeshVtxCol,
	ePODFileMeshBoneIdx,
	ePODFileMeshBoneWeight,
	ePODFileMeshInterleaved,
	ePODFileMeshBoneBatches,
	ePODFileMeshBoneBatchBoneCnts,
	ePODFileMeshBoneBatchOffsets,
	ePODFileMeshBoneBatchBoneMax,
	ePODFileMeshBoneBatchCnt,
	ePODFileMeshUnpackMatrix,

	ePODFileLightIdxTgt			= 7000,
	ePODFileLightColour,
	ePODFileLightType,
	ePODFileLightConstantAttenuation,
	ePODFileLightLinearAttenuation,
	ePODFileLightQuadraticAttenuation,
	ePODFileLightFalloffAngle,
	ePODFileLightFalloffExponent,

	ePODFileCamIdxTgt			= 8000,
	ePODFileCamFOV,
	ePODFileCamFar,
	ePODFileCamNear,
	ePODFileCamAnimFOV,

	ePODFileDataType			= 9000,
	ePODFileN,
	ePODFileStride,
	ePODFileData
};

/****************************************************************************
** Structures
****************************************************************************/
struct SPVRTPODImpl
{
	VERTTYPE	fFrame;		/*!< Frame number */
	VERTTYPE	fBlend;		/*!< Frame blend	(AKA fractional part of animation frame number) */
	int			nFrame;		/*!< Frame number (AKA integer part of animation frame number) */

	VERTTYPE	*pfCache;		/*!< Cache indicating the frames at which the matrix cache was filled */
	PVRTMATRIX	*pWmCache;		/*!< Cache of world matrices */
	PVRTMATRIX	*pWmZeroCache;	/*!< Pre-calculated frame 0 matrices */

	bool		bFromMemory;	/*!< Was the mesh data loaded from memory? */

#ifdef _DEBUG
	PVRTint64 nWmTotal, nWmCacheHit, nWmZeroCacheHit;
	float	fHitPerc, fHitPercZero;
#endif
};

/****************************************************************************
** Local code: Memory allocation
****************************************************************************/

/*!***************************************************************************
 @Function			SafeAlloc
 @Input				cnt
 @Output			ptr
 @Return			false if memory allocation failed
 @Description		Allocates a block of memory.
*****************************************************************************/
template <typename T>
bool SafeAlloc(T* &ptr, size_t cnt)
{
	_ASSERT(!ptr);
	if(cnt)
	{
		ptr = (T*)calloc(cnt, sizeof(T));
		_ASSERT(ptr);
		if(!ptr)
			return false;
	}
	return true;
}

/*!***************************************************************************
 @Function			SafeRealloc
 @Modified			ptr
 @Input				cnt
 @Description		Changes the size of a memory allocation.
*****************************************************************************/
template <typename T>
void SafeRealloc(T* &ptr, size_t cnt)
{
	ptr = (T*)realloc(ptr, cnt * sizeof(T));
	_ASSERT(ptr);
}

/****************************************************************************
** Class: CPODData
****************************************************************************/
/*!***************************************************************************
@Function			Reset
@Description		Resets the POD Data to NULL
*****************************************************************************/
void CPODData::Reset()
{
	eType = EPODDataFloat;
	n = 0;
	nStride = 0;
	FREE(pData);
}

// check32BitType and check16BitType are structs where only the specialisations have a standard declaration (complete type)
// if this struct is instantiated with a different type then the compiler will choke on it
// Place a line like: " 		check32BitType<channelType>();	" in a template function
// to ensure it won't be called using a type of the wrong size.
template<class T> struct check32BitType;
template<> struct check32BitType<unsigned int> {};
template<> struct check32BitType<int> {};
template<> struct check32BitType<float> {};
template<class T> struct check16BitType;
template<> struct check16BitType<unsigned short> {};
template<> struct check16BitType<short> {};

/*!***************************************************************************
 Class: CSource
*****************************************************************************/
class CSource
{
public:
	/*!***************************************************************************
	@Function			~CSource
	@Description		Destructor
	*****************************************************************************/
	virtual ~CSource() {};
	virtual bool Read(void* lpBuffer, const unsigned int dwNumberOfBytesToRead) = 0;
	virtual bool Skip(const unsigned int nBytes) = 0;

	template <typename T>
	bool Read(T &n)
	{
		return Read(&n, sizeof(T));
	}

	template <typename T>
	bool Read32(T &n)
	{
		unsigned char ub[4];

		if(Read(&ub, 4))
		{
			unsigned int *pn = (unsigned int*) &n;
			*pn = (unsigned int) ((ub[3] << 24) | (ub[2] << 16) | (ub[1] << 8) | ub[0]);
			return true;
		}

		return false;
	}

	template <typename T>
	bool Read16(T &n)
	{
		unsigned char ub[2];

		if(Read(&ub, 2))
		{
			unsigned short *pn = (unsigned short*) &n;
			*pn = (unsigned short) ((ub[1] << 8) | ub[0]);
			return true;
		}

		return false;
	}

	bool ReadMarker(unsigned int &nName, unsigned int &nLen);

	template <typename T>
	bool ReadAfterAlloc(T* &lpBuffer, const unsigned int dwNumberOfBytesToRead)
	{
		if(!SafeAlloc(lpBuffer, dwNumberOfBytesToRead))
			return false;
		return Read(lpBuffer, dwNumberOfBytesToRead);
	}

	template <typename T>
	bool ReadAfterAlloc32(T* &lpBuffer, const unsigned int dwNumberOfBytesToRead)
	{
		check32BitType<T>();
		if(!SafeAlloc(lpBuffer, dwNumberOfBytesToRead/4))
			return false;
		return ReadArray32((unsigned int*) lpBuffer, dwNumberOfBytesToRead / 4);
	}

	template <typename T>
	bool ReadArray32(T* pn, const unsigned int i32Size)
	{
		check32BitType<T>();
		bool bRet = true;

		for(unsigned int i = 0; i < i32Size; ++i)
			bRet &= Read32(pn[i]);

		return bRet;
	}

	template <typename T>
	bool ReadAfterAlloc16(T* &lpBuffer, const unsigned int dwNumberOfBytesToRead)
	{
		check16BitType<T>();
		if(!SafeAlloc(lpBuffer, dwNumberOfBytesToRead/2 ))
			return false;
		return ReadArray16((unsigned short*) lpBuffer, dwNumberOfBytesToRead / 2);
	}

	bool ReadArray16(unsigned short* pn, unsigned int i32Size)
	{
		bool bRet = true;

		for(unsigned int i = 0; i < i32Size; ++i)
			bRet &= Read16(pn[i]);

		return bRet;
	}
};

bool CSource::ReadMarker(unsigned int &nName, unsigned int &nLen)
{
	if(!Read32(nName))
		return false;
	if(!Read32(nLen))
		return false;
	return true;
}

/*!***************************************************************************
 Class: CSourceStream
*****************************************************************************/
class CSourceStream : public CSource
{
protected:
	CPVRTResourceFile* m_pFile;
	size_t m_BytesReadCount;

public:
	/*!***************************************************************************
	@Function			CSourceStream
	@Description		Constructor
	*****************************************************************************/
	CSourceStream() : m_pFile(0), m_BytesReadCount(0) {}

	/*!***************************************************************************
	@Function			~CSourceStream
	@Description		Destructor
	*****************************************************************************/
	virtual ~CSourceStream();

	bool Init(const char * const pszFileName);
	bool Init(const char * const pData, const size_t i32Size);

	virtual bool Read(void* lpBuffer, const unsigned int dwNumberOfBytesToRead);
	virtual bool Skip(const unsigned int nBytes);
};

/*!***************************************************************************
@Function			~CSourceStream
@Description		Destructor
*****************************************************************************/
CSourceStream::~CSourceStream()
{
	delete m_pFile;
}

/*!***************************************************************************
@Function			Init
@Input				pszFileName		Source file
@Description		Initialises the source stream with a file at the specified
					directory.
*****************************************************************************/
bool CSourceStream::Init(const char * const pszFileName)
{
	m_BytesReadCount = 0;
	if (m_pFile)
	{
		delete m_pFile;
		m_pFile = 0;
	}

	if(!pszFileName)
		return false;

	m_pFile = new CPVRTResourceFile(pszFileName);
	if (!m_pFile->IsOpen())
	{
		delete m_pFile;
		m_pFile = 0;
		return false;
	}
	return true;
}

/*!***************************************************************************
@Function			Init
@Input				pData			Address of the source data
@Input				i32Size			Size of the data (in bytes)
@Description		Initialises the source stream with the data at the specified
					directory.
*****************************************************************************/
bool CSourceStream::Init(const char * pData, size_t i32Size)
{
	m_BytesReadCount = 0;
	if (m_pFile) delete m_pFile;

	m_pFile = new CPVRTResourceFile(pData, i32Size);
	if (!m_pFile->IsOpen())
	{
		delete m_pFile;
		m_pFile = 0;
		return false;
	}
	return true;
}

/*!***************************************************************************
@Function			Read
@Modified			lpBuffer				Buffer to write the data into
@Input				dwNumberOfBytesToRead	Number of bytes to read
@Description		Reads specified number of bytes from the source stream
					into the output buffer.
*****************************************************************************/
bool CSourceStream::Read(void* lpBuffer, const unsigned int dwNumberOfBytesToRead)
{
	_ASSERT(lpBuffer);
	_ASSERT(m_pFile);

	if (m_BytesReadCount + dwNumberOfBytesToRead > m_pFile->Size()) return false;

	memcpy(lpBuffer, &((char*) m_pFile->DataPtr())[m_BytesReadCount], dwNumberOfBytesToRead);

	m_BytesReadCount += dwNumberOfBytesToRead;
	return true;
}

/*!***************************************************************************
@Function			Skip
@Input				nBytes			The number of bytes to skip
@Description		Skips the specified number of bytes of the source stream.
*****************************************************************************/
bool CSourceStream::Skip(const unsigned int nBytes)
{
	if (m_BytesReadCount + nBytes > m_pFile->Size()) return false;
	m_BytesReadCount += nBytes;
	return true;
}

#if defined(_WIN32)
/*!***************************************************************************
 Class: CSourceResource
*****************************************************************************/
class CSourceResource : public CSource
{
protected:
	const unsigned char	*m_pData;
	unsigned int		m_nSize, m_nReadPos;

public:
	bool Init(const TCHAR * const pszName);
	virtual bool Read(void* lpBuffer, const unsigned int dwNumberOfBytesToRead);
	virtual bool Skip(const unsigned int nBytes);
};

/*!***************************************************************************
@Function			Init
@Input				pszName			The file extension of the resource file
@Description		Initialises the source resource from the data at the
					specified file extension.
*****************************************************************************/
bool CSourceResource::Init(const TCHAR * const pszName)
{
	HRSRC	hR;
	HGLOBAL	hG;

	// Find the resource
	hR = FindResource(GetModuleHandle(NULL), pszName, RT_RCDATA);
	if(!hR)
		return false;

	// How big is the resource?
	m_nSize = SizeofResource(NULL, hR);
	if(!m_nSize)
		return false;

	// Get a pointer to the resource data
	hG = LoadResource(NULL, hR);
	if(!hG)
		return false;

	m_pData = (unsigned char*)LockResource(hG);
	if(!m_pData)
		return false;

	m_nReadPos = 0;
	return true;
}

/*!***************************************************************************
@Function			Read
@Modified			lpBuffer				The buffer to write to
@Input				dwNumberOfBytesToRead	The number of bytes to read
@Description		Reads data from the resource to the specified output buffer.
*****************************************************************************/
bool CSourceResource::Read(void* lpBuffer, const unsigned int dwNumberOfBytesToRead)
{
	if(m_nReadPos + dwNumberOfBytesToRead > m_nSize)
		return false;

	_ASSERT(lpBuffer);
	memcpy(lpBuffer, &m_pData[m_nReadPos], dwNumberOfBytesToRead);
	m_nReadPos += dwNumberOfBytesToRead;
	return true;
}

bool CSourceResource::Skip(const unsigned int nBytes)
{
	if(m_nReadPos + nBytes > m_nSize)
		return false;

	m_nReadPos += nBytes;
	return true;
}

#endif /* _WIN32 */

/****************************************************************************
** Local code: File writing
****************************************************************************/

/*!***************************************************************************
 @Function			WriteFileSafe
 @Input				pFile
 @Input				lpBuffer
 @Input				nNumberOfBytesToWrite
 @Return			true if successful
 @Description		Writes data to a file, checking return codes.
*****************************************************************************/
static bool WriteFileSafe(FILE *pFile, const void * const lpBuffer, const unsigned int nNumberOfBytesToWrite)
{
	if(nNumberOfBytesToWrite)
	{
		size_t count = fwrite(lpBuffer, nNumberOfBytesToWrite, 1, pFile);
		return count == 1;
	}
	return true;
}

static bool WriteFileSafe16(FILE *pFile, const unsigned short * const lpBuffer, const unsigned int nSize)
{
	if(nSize)
	{
		unsigned char ub[2];
		bool bRet = true;

		for(unsigned int i = 0; i < nSize; ++i)
		{
			ub[0] = (unsigned char) lpBuffer[i];
			ub[1] = lpBuffer[i] >> 8;

			bRet &= (fwrite(ub, 2, 1, pFile) == 1);
		}

		return bRet;
	}
	return true;
}

static bool WriteFileSafe32(FILE *pFile, const unsigned int * const lpBuffer, const unsigned int nSize)
{
	if(nSize)
	{
		unsigned char ub[4];
		bool bRet = true;

		for(unsigned int i = 0; i < nSize; ++i)
		{
			ub[0] = (unsigned char) (lpBuffer[i]);
			ub[1] = (unsigned char) (lpBuffer[i] >> 8);
			ub[2] = (unsigned char) (lpBuffer[i] >> 16);
			ub[3] = (unsigned char) (lpBuffer[i] >> 24);

			bRet &= (fwrite(ub, 4, 1, pFile) == 1);
		}

		return bRet;
	}
	return true;
}
/*!***************************************************************************
 @Function			WriteMarker
 @Input				pFile
 @Input				nName
 @Input				bEnd
 @Input				nLen
 Return				true if successful
 @Description		Write a marker to a POD file. If bEnd if false, it's a
					beginning marker, otherwise it's an end marker.
*****************************************************************************/
static bool WriteMarker(
	FILE				* const pFile,
	const unsigned int	nName,
	const bool			bEnd,
	const unsigned int	nLen = 0)
{
	unsigned int nMarker;
	bool bRet;

	_ASSERT((nName & ~PVRTMODELPOD_TAG_MASK) == nName);
	nMarker = nName | (bEnd ? PVRTMODELPOD_TAG_END : PVRTMODELPOD_TAG_START);

	bRet  = WriteFileSafe32(pFile, &nMarker, 1);
	bRet &= WriteFileSafe32(pFile, &nLen, 1);

	return bRet;
}

/*!***************************************************************************
 @Function			WriteData
 @Input				pFile
 @Input				nName
 @Input				pData
 @Input				nLen
 @Return			true if successful
 @Description		Write nLen bytes of data from pData, bracketed by an nName
					begin/end markers.
*****************************************************************************/
static bool WriteData(
	FILE				* const pFile,
	const unsigned int	nName,
	const void			* const pData,
	const unsigned int	nLen)
{
	if(pData)
	{
		_ASSERT(nLen);
		if(!WriteMarker(pFile, nName, false, nLen)) return false;
		if(!WriteFileSafe(pFile, pData, nLen)) return false;
		if(!WriteMarker(pFile, nName, true)) return false;
	}
	return true;
}

/*!***************************************************************************
 @Function			WriteData16
 @Input				pFile
 @Input				nName
 @Input				pData
 @Input				i32Size
 @Return			true if successful
 @Description		Write i32Size no. of unsigned shorts from pData, bracketed by
					an nName begin/end markers.
*****************************************************************************/
template <typename T>
static bool WriteData16(
	FILE				* const pFile,
	const unsigned int	nName,
	const T	* const pData,
	int i32Size = 1)
{
	if(pData)
	{
		if(!WriteMarker(pFile, nName, false, 2 * i32Size)) return false;
		if(!WriteFileSafe16(pFile, (unsigned short*) pData, i32Size)) return false;
		if(!WriteMarker(pFile, nName, true)) return false;
	}
	return true;
}

/*!***************************************************************************
 @Function			WriteData32
 @Input				pFile
 @Input				nName
 @Input				pData
 @Input				i32Size
 @Return			true if successful
 @Description		Write i32Size no. of unsigned ints from pData, bracketed by
					an nName begin/end markers.
*****************************************************************************/
template <typename T>
static bool WriteData32(
	FILE				* const pFile,
	const unsigned int	nName,
	const T	* const pData,
	int i32Size = 1)
{
	if(pData)
	{
		if(!WriteMarker(pFile, nName, false, 4 * i32Size)) return false;
		if(!WriteFileSafe32(pFile, (unsigned int*) pData, i32Size)) return false;
		if(!WriteMarker(pFile, nName, true)) return false;
	}
	return true;
}

/*!***************************************************************************
 @Function			WriteData
 @Input				pFile
 @Input				nName
 @Input				n
 @Return			true if successful
 @Description		Write the value n, bracketed by an nName begin/end markers.
*****************************************************************************/
template <typename T>
static bool WriteData(
	FILE				* const pFile,
	const unsigned int	nName,
	const T				&n)
{
	unsigned int nSize = sizeof(T);

	bool bRet = WriteData(pFile, nName, (void*)&n, nSize);

	return bRet;
}

/*!***************************************************************************
 @Function			WriteCPODData
 @Input				pFile
 @Input				nName
 @Input				n
 @Input				nEntries
 @Input				bValidData
 @Return			true if successful
 @Description		Write the value n, bracketed by an nName begin/end markers.
*****************************************************************************/
static bool WriteCPODData(
	FILE				* const pFile,
	const unsigned int	nName,
	const CPODData		&n,
	const unsigned int	nEntries,
	const bool			bValidData)
{
	if(!WriteMarker(pFile, nName, false)) return false;
	if(!WriteData32(pFile, ePODFileDataType, &n.eType)) return false;
	if(!WriteData32(pFile, ePODFileN, &n.n)) return false;
	if(!WriteData32(pFile, ePODFileStride, &n.nStride)) return false;
	if(bValidData)
	{
		switch(PVRTModelPODDataTypeSize(n.eType))
		{
			case 1: if(!WriteData(pFile, ePODFileData, n.pData, nEntries * n.nStride)) return false; break;
			case 2: if(!WriteData16(pFile, ePODFileData, n.pData, nEntries * (n.nStride / 2))) return false; break;
			case 4: if(!WriteData32(pFile, ePODFileData, n.pData, nEntries * (n.nStride / 4))) return false; break;
			default: { _ASSERT(false); }
		};
	}
	else
	{
		unsigned int offset = (unsigned int) (size_t) n.pData;
		if(!WriteData32(pFile, ePODFileData, &offset)) return false;
	}
	if(!WriteMarker(pFile, nName, true)) return false;
	return true;
}

/*!***************************************************************************
 @Function			WriteInterleaved
 @Input				pFile
 @Input				mesh
 @Return			true if successful
 @Description		Write out the interleaved data to file.
*****************************************************************************/
static bool WriteInterleaved(FILE * const pFile, SPODMesh &mesh)
{
	if(!mesh.pInterleaved)
		return true;

	unsigned int i;
	unsigned int ui32CPODDataSize = 0;
	CPODData **pCPODData = new CPODData*[7 + mesh.nNumUVW];

	if(mesh.sVertex.n)		pCPODData[ui32CPODDataSize++] = &mesh.sVertex;
	if(mesh.sNormals.n)		pCPODData[ui32CPODDataSize++] = &mesh.sNormals;
	if(mesh.sTangents.n)	pCPODData[ui32CPODDataSize++] = &mesh.sTangents;
	if(mesh.sBinormals.n)	pCPODData[ui32CPODDataSize++] = &mesh.sBinormals;
	if(mesh.sVtxColours.n)	pCPODData[ui32CPODDataSize++] = &mesh.sVtxColours;
	if(mesh.sBoneIdx.n)		pCPODData[ui32CPODDataSize++] = &mesh.sBoneIdx;
	if(mesh.sBoneWeight.n)	pCPODData[ui32CPODDataSize++] = &mesh.sBoneWeight;

	for(i = 0; i < mesh.nNumUVW; ++i)
		if(mesh.psUVW[i].n) pCPODData[ui32CPODDataSize++] = &mesh.psUVW[i];

	// Bubble sort pCPODData based on the vertex element offsets
	bool bSwap = true;
	unsigned int ui32Size = ui32CPODDataSize;

	while(bSwap)
	{
		bSwap = false;

		for(i = 0; i < ui32Size - 1; ++i)
		{
			if(pCPODData[i]->pData > pCPODData[i + 1]->pData)
			{
				PVRTswap(pCPODData[i], pCPODData[i + 1]);
				bSwap = true;
			}
		}

		--ui32Size;
	}

	// Write out the data
	if(!WriteMarker(pFile, ePODFileMeshInterleaved, false, mesh.nNumVertex * mesh.sVertex.nStride)) return false;

	for(i = 0; i < mesh.nNumVertex; ++i)
	{
		unsigned char* pVtxStart = mesh.pInterleaved + (i * mesh.sVertex.nStride);

		for(unsigned int j = 0; j < ui32CPODDataSize; ++j)
		{
			unsigned char* pData = pVtxStart + (size_t) pCPODData[j]->pData;

			switch(PVRTModelPODDataTypeSize(pCPODData[j]->eType))
			{
				case 1: if(!WriteFileSafe(pFile, pData, pCPODData[j]->n)) return false; break;
				case 2: if(!WriteFileSafe16(pFile, (unsigned short*) pData, pCPODData[j]->n)) return false; break;
				case 4: if(!WriteFileSafe32(pFile, (unsigned int*) pData, pCPODData[j]->n)) return false; break;
				default: { _ASSERT(false); }
			};

			// Write out the padding
			size_t padding;

			if(j != ui32CPODDataSize - 1)
				padding = ((size_t)pCPODData[j + 1]->pData - (size_t)pCPODData[j]->pData) - PVRTModelPODDataStride(*pCPODData[j]);
			else
				padding = (pCPODData[j]->nStride - (size_t)pCPODData[j]->pData) - PVRTModelPODDataStride(*pCPODData[j]);

			fwrite("\0\0\0\0", padding, 1, pFile);
		}
	}

	if(!WriteMarker(pFile, ePODFileMeshInterleaved, true)) return false;

	// Delete our CPOD data array
	delete[] pCPODData;

	return true;
}

/*!***************************************************************************
 @Function			PVRTModelPODGetAnimArraySize
 @Input				pAnimDataIdx
 @Input				ui32Frames
 @Input				ui32Components
 @Return			Size of the animation array
 @Description		Calculates the size of an animation array
*****************************************************************************/
PVRTuint32 PVRTModelPODGetAnimArraySize(PVRTuint32 *pAnimDataIdx, PVRTuint32 ui32Frames, PVRTuint32 ui32Components)
{
	if(pAnimDataIdx)
	{
		// Find the largest index value
		PVRTuint32 ui32Max = 0;
		for(unsigned int i = 0; i < ui32Frames; ++i)
		{
			if(ui32Max < pAnimDataIdx[i])
				ui32Max = pAnimDataIdx[i];
		}

		return ui32Max + ui32Components;
	}

	return ui32Frames * ui32Components;
}

/*!***************************************************************************
 @Function			WritePOD
 @Output			The file referenced by pFile
 @Input				s The POD Scene to write
 @Input				pszExpOpt Exporter options
 @Return			true if successful
 @Description		Write a POD file
*****************************************************************************/
static bool WritePOD(
	FILE			* const pFile,
	const char		* const pszExpOpt,
	const char		* const pszHistory,
	const SPODScene	&s)
{
	unsigned int i, j;

	// Save: file version
	{
		char *pszVersion = (char*)PVRTMODELPOD_VERSION;

		if(!WriteData(pFile, ePODFileVersion, pszVersion, (unsigned int)strlen(pszVersion) + 1)) return false;
	}

	// Save: exporter options
	if(pszExpOpt && *pszExpOpt)
	{
		if(!WriteData(pFile, ePODFileExpOpt, pszExpOpt, (unsigned int)strlen(pszExpOpt) + 1)) return false;
	}

	// Save: .pod file history
	if(pszHistory && *pszHistory)
	{
		if(!WriteData(pFile, ePODFileHistory, pszHistory, (unsigned int)strlen(pszHistory) + 1)) return false;
	}

	// Save: scene descriptor
	if(!WriteMarker(pFile, ePODFileScene, false)) return false;

	{
		if(!WriteData32(pFile, ePODFileUnits, &s.fUnits)) return false;
		if(!WriteData32(pFile, ePODFileColourBackground,	s.pfColourBackground, sizeof(s.pfColourBackground) / sizeof(*s.pfColourBackground))) return false;
		if(!WriteData32(pFile, ePODFileColourAmbient,		s.pfColourAmbient, sizeof(s.pfColourAmbient) / sizeof(*s.pfColourAmbient))) return false;
		if(!WriteData32(pFile, ePODFileNumCamera, &s.nNumCamera)) return false;
		if(!WriteData32(pFile, ePODFileNumLight, &s.nNumLight)) return false;
		if(!WriteData32(pFile, ePODFileNumMesh,	&s.nNumMesh)) return false;
		if(!WriteData32(pFile, ePODFileNumNode,	&s.nNumNode)) return false;
		if(!WriteData32(pFile, ePODFileNumMeshNode,	&s.nNumMeshNode)) return false;
		if(!WriteData32(pFile, ePODFileNumTexture, &s.nNumTexture)) return false;
		if(!WriteData32(pFile, ePODFileNumMaterial,	&s.nNumMaterial)) return false;
		if(!WriteData32(pFile, ePODFileNumFrame, &s.nNumFrame)) return false;

		if(s.nNumFrame)
		{
			if(!WriteData32(pFile, ePODFileFPS, &s.nFPS)) return false;
		}

		if(!WriteData32(pFile, ePODFileFlags, &s.nFlags)) return false;
		if(!WriteData(pFile, ePODFileUserData, s.pUserData, s.nUserDataSize)) return false;

		// Save: cameras
		for(i = 0; i < s.nNumCamera; ++i)
		{
			if(!WriteMarker(pFile, ePODFileCamera, false)) return false;
			if(!WriteData32(pFile, ePODFileCamIdxTgt, &s.pCamera[i].nIdxTarget)) return false;
			if(!WriteData32(pFile, ePODFileCamFOV,	  &s.pCamera[i].fFOV)) return false;
			if(!WriteData32(pFile, ePODFileCamFar,	  &s.pCamera[i].fFar)) return false;
			if(!WriteData32(pFile, ePODFileCamNear,	  &s.pCamera[i].fNear)) return false;
			if(!WriteData32(pFile, ePODFileCamAnimFOV,	s.pCamera[i].pfAnimFOV, s.nNumFrame)) return false;
			if(!WriteMarker(pFile, ePODFileCamera, true)) return false;
		}
		// Save: lights
		for(i = 0; i < s.nNumLight; ++i)
		{
			if(!WriteMarker(pFile, ePODFileLight, false)) return false;
			if(!WriteData32(pFile, ePODFileLightIdxTgt,	&s.pLight[i].nIdxTarget)) return false;
			if(!WriteData32(pFile, ePODFileLightColour,	s.pLight[i].pfColour, sizeof(s.pLight[i].pfColour) / sizeof(*s.pLight[i].pfColour))) return false;
			if(!WriteData32(pFile, ePODFileLightType,	&s.pLight[i].eType)) return false;

			if(s.pLight[i].eType != ePODDirectional)
			{
				if(!WriteData32(pFile, ePODFileLightConstantAttenuation,	&s.pLight[i].fConstantAttenuation))  return false;
				if(!WriteData32(pFile, ePODFileLightLinearAttenuation,		&s.pLight[i].fLinearAttenuation))	  return false;
				if(!WriteData32(pFile, ePODFileLightQuadraticAttenuation,	&s.pLight[i].fQuadraticAttenuation)) return false;
			}

			if(s.pLight[i].eType == ePODSpot)
			{
				if(!WriteData32(pFile, ePODFileLightFalloffAngle,			&s.pLight[i].fFalloffAngle))		  return false;
				if(!WriteData32(pFile, ePODFileLightFalloffExponent,		&s.pLight[i].fFalloffExponent))	  return false;
			}

			if(!WriteMarker(pFile, ePODFileLight, true)) return false;
		}

		// Save: materials
		for(i = 0; i < s.nNumMaterial; ++i)
		{
			if(!WriteMarker(pFile, ePODFileMaterial, false)) return false;

			if(!WriteData32(pFile, ePODFileMatFlags,  &s.pMaterial[i].nFlags)) return false;
			if(!WriteData(pFile,   ePODFileMatName,			s.pMaterial[i].pszName, (unsigned int)strlen(s.pMaterial[i].pszName)+1)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexDiffuse,	&s.pMaterial[i].nIdxTexDiffuse)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexAmbient,	&s.pMaterial[i].nIdxTexAmbient)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexSpecularColour,	&s.pMaterial[i].nIdxTexSpecularColour)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexSpecularLevel,	&s.pMaterial[i].nIdxTexSpecularLevel)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexBump,	&s.pMaterial[i].nIdxTexBump)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexEmissive,	&s.pMaterial[i].nIdxTexEmissive)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexGlossiness,	&s.pMaterial[i].nIdxTexGlossiness)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexOpacity,	&s.pMaterial[i].nIdxTexOpacity)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexReflection,	&s.pMaterial[i].nIdxTexReflection)) return false;
			if(!WriteData32(pFile, ePODFileMatIdxTexRefraction,	&s.pMaterial[i].nIdxTexRefraction)) return false;
			if(!WriteData32(pFile, ePODFileMatOpacity,	&s.pMaterial[i].fMatOpacity)) return false;
			if(!WriteData32(pFile, ePODFileMatAmbient,		s.pMaterial[i].pfMatAmbient, sizeof(s.pMaterial[i].pfMatAmbient) / sizeof(*s.pMaterial[i].pfMatAmbient))) return false;
			if(!WriteData32(pFile, ePODFileMatDiffuse,		s.pMaterial[i].pfMatDiffuse, sizeof(s.pMaterial[i].pfMatDiffuse) / sizeof(*s.pMaterial[i].pfMatDiffuse))) return false;
			if(!WriteData32(pFile, ePODFileMatSpecular,		s.pMaterial[i].pfMatSpecular, sizeof(s.pMaterial[i].pfMatSpecular) / sizeof(*s.pMaterial[i].pfMatSpecular))) return false;
			if(!WriteData32(pFile, ePODFileMatShininess, &s.pMaterial[i].fMatShininess)) return false;
			if(!WriteData(pFile, ePODFileMatEffectFile,		s.pMaterial[i].pszEffectFile, s.pMaterial[i].pszEffectFile ? ((unsigned int)strlen(s.pMaterial[i].pszEffectFile)+1) : 0)) return false;
			if(!WriteData(pFile, ePODFileMatEffectName,		s.pMaterial[i].pszEffectName, s.pMaterial[i].pszEffectName ? ((unsigned int)strlen(s.pMaterial[i].pszEffectName)+1) : 0)) return false;
			if(!WriteData32(pFile, ePODFileMatBlendSrcRGB,  &s.pMaterial[i].eBlendSrcRGB))return false;
			if(!WriteData32(pFile, ePODFileMatBlendSrcA,	&s.pMaterial[i].eBlendSrcA))	return false;
			if(!WriteData32(pFile, ePODFileMatBlendDstRGB,  &s.pMaterial[i].eBlendDstRGB))return false;
			if(!WriteData32(pFile, ePODFileMatBlendDstA,	&s.pMaterial[i].eBlendDstA))	return false;
			if(!WriteData32(pFile, ePODFileMatBlendOpRGB,	&s.pMaterial[i].eBlendOpRGB)) return false;
			if(!WriteData32(pFile, ePODFileMatBlendOpA,		&s.pMaterial[i].eBlendOpA))	return false;
			if(!WriteData32(pFile, ePODFileMatBlendColour, s.pMaterial[i].pfBlendColour, sizeof(s.pMaterial[i].pfBlendColour) / sizeof(*s.pMaterial[i].pfBlendColour))) return false;
			if(!WriteData32(pFile, ePODFileMatBlendFactor, s.pMaterial[i].pfBlendFactor, sizeof(s.pMaterial[i].pfBlendFactor) / sizeof(*s.pMaterial[i].pfBlendFactor))) return false;
			if(!WriteData(pFile,   ePODFileMatUserData, s.pMaterial[i].pUserData, s.pMaterial[i].nUserDataSize)) return false;

			if(!WriteMarker(pFile, ePODFileMaterial, true)) return false;
		}

		// Save: meshes
		for(i = 0; i < s.nNumMesh; ++i)
		{
			if(!WriteMarker(pFile, ePODFileMesh, false)) return false;

			if(!WriteData32(pFile, ePODFileMeshNumVtx,			&s.pMesh[i].nNumVertex)) return false;
			if(!WriteData32(pFile, ePODFileMeshNumFaces,		&s.pMesh[i].nNumFaces)) return false;
			if(!WriteData32(pFile, ePODFileMeshNumUVW,			&s.pMesh[i].nNumUVW)) return false;
			if(!WriteData32(pFile, ePODFileMeshStripLength,		s.pMesh[i].pnStripLength, s.pMesh[i].nNumStrips)) return false;
			if(!WriteData32(pFile, ePODFileMeshNumStrips,		&s.pMesh[i].nNumStrips)) return false;
			if(!WriteInterleaved(pFile, s.pMesh[i])) return false;
			if(!WriteData32(pFile, ePODFileMeshBoneBatchBoneMax,&s.pMesh[i].sBoneBatches.nBatchBoneMax)) return false;
			if(!WriteData32(pFile, ePODFileMeshBoneBatchCnt,	&s.pMesh[i].sBoneBatches.nBatchCnt)) return false;
			if(!WriteData32(pFile, ePODFileMeshBoneBatches,		s.pMesh[i].sBoneBatches.pnBatches, s.pMesh[i].sBoneBatches.nBatchBoneMax * s.pMesh[i].sBoneBatches.nBatchCnt)) return false;
			if(!WriteData32(pFile, ePODFileMeshBoneBatchBoneCnts,	s.pMesh[i].sBoneBatches.pnBatchBoneCnt, s.pMesh[i].sBoneBatches.nBatchCnt)) return false;
			if(!WriteData32(pFile, ePODFileMeshBoneBatchOffsets,	s.pMesh[i].sBoneBatches.pnBatchOffset,s.pMesh[i].sBoneBatches.nBatchCnt)) return false;
			if(!WriteData32(pFile, ePODFileMeshUnpackMatrix,	s.pMesh[i].mUnpackMatrix.f, 16))	return false;

			if(!WriteCPODData(pFile, ePODFileMeshFaces,			s.pMesh[i].sFaces,		PVRTModelPODCountIndices(s.pMesh[i]), true)) return false;
			if(!WriteCPODData(pFile, ePODFileMeshVtx,			s.pMesh[i].sVertex,		s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;
			if(!WriteCPODData(pFile, ePODFileMeshNor,			s.pMesh[i].sNormals,	s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;
			if(!WriteCPODData(pFile, ePODFileMeshTan,			s.pMesh[i].sTangents,	s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;
			if(!WriteCPODData(pFile, ePODFileMeshBin,			 s.pMesh[i].sBinormals,	s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;

			for(j = 0; j < s.pMesh[i].nNumUVW; ++j)
				if(!WriteCPODData(pFile, ePODFileMeshUVW,		s.pMesh[i].psUVW[j],	s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;

			if(!WriteCPODData(pFile, ePODFileMeshVtxCol,		s.pMesh[i].sVtxColours, s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;
			if(!WriteCPODData(pFile, ePODFileMeshBoneIdx,		s.pMesh[i].sBoneIdx,	s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;
			if(!WriteCPODData(pFile, ePODFileMeshBoneWeight,	s.pMesh[i].sBoneWeight,	s.pMesh[i].nNumVertex, s.pMesh[i].pInterleaved == 0)) return false;

			if(!WriteMarker(pFile, ePODFileMesh, true)) return false;
		}

		int iTransformationNo;
		// Save: node
		for(i = 0; i < s.nNumNode; ++i)
		{
			if(!WriteMarker(pFile, ePODFileNode, false)) return false;

			{
				if(!WriteData32(pFile, ePODFileNodeIdx,		&s.pNode[i].nIdx)) return false;
				if(!WriteData(pFile, ePODFileNodeName,		s.pNode[i].pszName, (unsigned int)strlen(s.pNode[i].pszName)+1)) return false;
				if(!WriteData32(pFile, ePODFileNodeIdxMat,	&s.pNode[i].nIdxMaterial)) return false;
				if(!WriteData32(pFile, ePODFileNodeIdxParent, &s.pNode[i].nIdxParent)) return false;
				if(!WriteData32(pFile, ePODFileNodeAnimFlags, &s.pNode[i].nAnimFlags)) return false;

				if(s.pNode[i].pnAnimPositionIdx)
				{
					if(!WriteData32(pFile, ePODFileNodeAnimPosIdx,	s.pNode[i].pnAnimPositionIdx,	s.nNumFrame)) return false;
				}

				iTransformationNo = s.pNode[i].nAnimFlags & ePODHasPositionAni ? PVRTModelPODGetAnimArraySize(s.pNode[i].pnAnimPositionIdx, s.nNumFrame, 3) : 3;
				if(!WriteData32(pFile, ePODFileNodeAnimPos,	s.pNode[i].pfAnimPosition,	iTransformationNo)) return false;

				if(s.pNode[i].pnAnimRotationIdx)
				{
					if(!WriteData32(pFile, ePODFileNodeAnimRotIdx,	s.pNode[i].pnAnimRotationIdx,	s.nNumFrame)) return false;
				}

				iTransformationNo = s.pNode[i].nAnimFlags & ePODHasRotationAni ? PVRTModelPODGetAnimArraySize(s.pNode[i].pnAnimRotationIdx, s.nNumFrame, 4) : 4;
				if(!WriteData32(pFile, ePODFileNodeAnimRot,	s.pNode[i].pfAnimRotation,	iTransformationNo)) return false;

				if(s.pNode[i].pnAnimScaleIdx)
				{
					if(!WriteData32(pFile, ePODFileNodeAnimScaleIdx,	s.pNode[i].pnAnimScaleIdx,	s.nNumFrame)) return false;
				}

				iTransformationNo = s.pNode[i].nAnimFlags & ePODHasScaleAni ? PVRTModelPODGetAnimArraySize(s.pNode[i].pnAnimScaleIdx, s.nNumFrame, 7) : 7;
				if(!WriteData32(pFile, ePODFileNodeAnimScale,	s.pNode[i].pfAnimScale,		iTransformationNo))    return false;

				if(s.pNode[i].pnAnimMatrixIdx)
				{
					if(!WriteData32(pFile, ePODFileNodeAnimMatrixIdx,	s.pNode[i].pnAnimMatrixIdx,	s.nNumFrame)) return false;
				}

				iTransformationNo = s.pNode[i].nAnimFlags & ePODHasMatrixAni ? PVRTModelPODGetAnimArraySize(s.pNode[i].pnAnimMatrixIdx, s.nNumFrame, 16) : 16;
				if(!WriteData32(pFile, ePODFileNodeAnimMatrix,s.pNode[i].pfAnimMatrix,	iTransformationNo))   return false;

				if(!WriteData(pFile, ePODFileNodeUserData, s.pNode[i].pUserData, s.pNode[i].nUserDataSize)) return false;
			}

			if(!WriteMarker(pFile, ePODFileNode, true)) return false;
		}

		// Save: texture
		for(i = 0; i < s.nNumTexture; ++i)
		{
			if(!WriteMarker(pFile, ePODFileTexture, false)) return false;
			if(!WriteData(pFile, ePODFileTexName, s.pTexture[i].pszName, (unsigned int)strlen(s.pTexture[i].pszName)+1)) return false;
			if(!WriteMarker(pFile, ePODFileTexture, true)) return false;
		}
	}
	if(!WriteMarker(pFile, ePODFileScene, true)) return false;

	return true;
}

/****************************************************************************
** Local code: File reading
****************************************************************************/
/*!***************************************************************************
 @Function			ReadCPODData
 @Modified			s The CPODData to read into
 @Input				src CSource object to read data from.
 @Input				nSpec
 @Input				bValidData
 @Return			true if successful
 @Description		Read a CPODData block in  from a pod file
*****************************************************************************/
static bool ReadCPODData(
	CPODData			&s,
	CSource				&src,
	const unsigned int	nSpec,
	const bool			bValidData)
{
	unsigned int nName, nLen, nBuff;

	while(src.ReadMarker(nName, nLen))
	{
		if(nName == (nSpec | PVRTMODELPOD_TAG_END))
			return true;

		switch(nName)
		{
		case ePODFileDataType:	if(!src.Read32(s.eType)) return false;					break;
		case ePODFileN:			if(!src.Read32(s.n)) return false;						break;
		case ePODFileStride:	if(!src.Read32(s.nStride)) return false;					break;
		case ePODFileData:
			if(bValidData)
			{
				switch(PVRTModelPODDataTypeSize(s.eType))
				{
					case 1: if(!src.ReadAfterAlloc(s.pData, nLen)) return false; break;
					case 2:
						{ // reading 16bit data but have 8bit pointer
							PVRTuint16 *p16Pointer=NULL;
							if(!src.ReadAfterAlloc16(p16Pointer, nLen)) return false;
							s.pData = (unsigned char*)p16Pointer;
							break;
						}
					case 4:
						{ // reading 32bit data but have 8bit pointer
							PVRTuint32 *p32Pointer=NULL;
							if(!src.ReadAfterAlloc32(p32Pointer, nLen)) return false;
							s.pData = (unsigned char*)p32Pointer;
							break;
						}
					default:
						{ _ASSERT(false);}
				}
			}
			else
			{
				if(src.Read32(nBuff))
				{
					s.pData = (unsigned char*) (size_t) nBuff;
				}
				else
				{
					return false;
				}
			}
		 break;

		default:
			if(!src.Skip(nLen)) return false;
		}
	}
	return false;
}

/*!***************************************************************************
 @Function			ReadCamera
 @Modified			s The SPODCamera to read into
 @Input				src	CSource object to read data from.
 @Return			true if successful
 @Description		Read a camera block in from a pod file
*****************************************************************************/
static bool ReadCamera(
	SPODCamera	&s,
	CSource		&src)
{
	unsigned int nName, nLen;
	s.pfAnimFOV = 0;

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileCamera | PVRTMODELPOD_TAG_END:			return true;

		case ePODFileCamIdxTgt:		if(!src.Read32(s.nIdxTarget)) return false;					break;
		case ePODFileCamFOV:		if(!src.Read32(s.fFOV)) return false;							break;
		case ePODFileCamFar:		if(!src.Read32(s.fFar)) return false;							break;
		case ePODFileCamNear:		if(!src.Read32(s.fNear)) return false;						break;
		case ePODFileCamAnimFOV:	if(!src.ReadAfterAlloc32(s.pfAnimFOV, nLen)) return false;	break;

		default:
			if(!src.Skip(nLen)) return false;
		}
	}
	return false;
}

/*!***************************************************************************
 @Function			ReadLight
 @Modified			s The SPODLight to read into
 @Input				src	CSource object to read data from.
 @Return			true if successful
 @Description		Read a light block in from a pod file
*****************************************************************************/
static bool ReadLight(
	SPODLight	&s,
	CSource		&src)
{
	unsigned int nName, nLen;

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileLight | PVRTMODELPOD_TAG_END:			return true;

		case ePODFileLightIdxTgt:	if(!src.Read32(s.nIdxTarget)) return false;	break;
		case ePODFileLightColour:	if(!src.ReadArray32(s.pfColour, 3)) return false;		break;
		case ePODFileLightType:		if(!src.Read32(s.eType)) return false;		break;
		case ePODFileLightConstantAttenuation: 		if(!src.Read32(s.fConstantAttenuation))	return false;	break;
		case ePODFileLightLinearAttenuation:		if(!src.Read32(s.fLinearAttenuation))		return false;	break;
		case ePODFileLightQuadraticAttenuation:		if(!src.Read32(s.fQuadraticAttenuation))	return false;	break;
		case ePODFileLightFalloffAngle:				if(!src.Read32(s.fFalloffAngle))			return false;	break;
		case ePODFileLightFalloffExponent:			if(!src.Read32(s.fFalloffExponent))		return false;	break;
		default:
			if(!src.Skip(nLen)) return false;
		}
	}
	return false;
}

/*!***************************************************************************
 @Function			ReadMaterial
 @Modified			s The SPODMaterial to read into
 @Input				src	CSource object to read data from.
 @Return			true if successful
 @Description		Read a material block in from a pod file
*****************************************************************************/
static bool ReadMaterial(
	SPODMaterial	&s,
	CSource			&src)
{
	unsigned int nName, nLen;

	// Set texture IDs to -1
	s.nIdxTexDiffuse = -1;
	s.nIdxTexAmbient = -1;
	s.nIdxTexSpecularColour = -1;
	s.nIdxTexSpecularLevel = -1;
	s.nIdxTexBump = -1;
	s.nIdxTexEmissive = -1;
	s.nIdxTexGlossiness = -1;
	s.nIdxTexOpacity = -1;
	s.nIdxTexReflection = -1;
	s.nIdxTexRefraction = -1;

	// Set defaults for blend modes
	s.eBlendSrcRGB = s.eBlendSrcA = ePODBlendFunc_ONE;
	s.eBlendDstRGB = s.eBlendDstA = ePODBlendFunc_ZERO;
	s.eBlendOpRGB  = s.eBlendOpA  = ePODBlendOp_ADD;

	memset(s.pfBlendColour, 0, sizeof(s.pfBlendColour));
	memset(s.pfBlendFactor, 0, sizeof(s.pfBlendFactor));

	// Set default for material flags
	s.nFlags = 0;

	// Set default for user data
	s.pUserData = 0;
	s.nUserDataSize = 0;

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileMaterial | PVRTMODELPOD_TAG_END:			return true;

		case ePODFileMatFlags:					if(!src.Read32(s.nFlags)) return false;				break;
		case ePODFileMatName:					if(!src.ReadAfterAlloc(s.pszName, nLen)) return false;		break;
		case ePODFileMatIdxTexDiffuse:			if(!src.Read32(s.nIdxTexDiffuse)) return false;				break;
		case ePODFileMatIdxTexAmbient:			if(!src.Read32(s.nIdxTexAmbient)) return false;				break;
		case ePODFileMatIdxTexSpecularColour:	if(!src.Read32(s.nIdxTexSpecularColour)) return false;		break;
		case ePODFileMatIdxTexSpecularLevel:	if(!src.Read32(s.nIdxTexSpecularLevel)) return false;			break;
		case ePODFileMatIdxTexBump:				if(!src.Read32(s.nIdxTexBump)) return false;					break;
		case ePODFileMatIdxTexEmissive:			if(!src.Read32(s.nIdxTexEmissive)) return false;				break;
		case ePODFileMatIdxTexGlossiness:		if(!src.Read32(s.nIdxTexGlossiness)) return false;			break;
		case ePODFileMatIdxTexOpacity:			if(!src.Read32(s.nIdxTexOpacity)) return false;				break;
		case ePODFileMatIdxTexReflection:		if(!src.Read32(s.nIdxTexReflection)) return false;			break;
		case ePODFileMatIdxTexRefraction:		if(!src.Read32(s.nIdxTexRefraction)) return false;			break;
		case ePODFileMatOpacity:		if(!src.Read32(s.fMatOpacity)) return false;						break;
		case ePODFileMatAmbient:		if(!src.ReadArray32(s.pfMatAmbient,  sizeof(s.pfMatAmbient) / sizeof(*s.pfMatAmbient))) return false;		break;
		case ePODFileMatDiffuse:		if(!src.ReadArray32(s.pfMatDiffuse,  sizeof(s.pfMatDiffuse) / sizeof(*s.pfMatDiffuse))) return false;		break;
		case ePODFileMatSpecular:		if(!src.ReadArray32(s.pfMatSpecular, sizeof(s.pfMatSpecular) / sizeof(*s.pfMatSpecular))) return false;		break;
		case ePODFileMatShininess:		if(!src.Read32(s.fMatShininess)) return false;					break;
		case ePODFileMatEffectFile:		if(!src.ReadAfterAlloc(s.pszEffectFile, nLen)) return false;	break;
		case ePODFileMatEffectName:		if(!src.ReadAfterAlloc(s.pszEffectName, nLen)) return false;	break;
		case ePODFileMatBlendSrcRGB:	if(!src.Read32(s.eBlendSrcRGB))	return false;	break;
		case ePODFileMatBlendSrcA:		if(!src.Read32(s.eBlendSrcA))		return false;	break;
		case ePODFileMatBlendDstRGB:	if(!src.Read32(s.eBlendDstRGB))	return false;	break;
		case ePODFileMatBlendDstA:		if(!src.Read32(s.eBlendDstA))		return false;	break;
		case ePODFileMatBlendOpRGB:		if(!src.Read32(s.eBlendOpRGB))	return false;	break;
		case ePODFileMatBlendOpA:		if(!src.Read32(s.eBlendOpA))		return false;	break;
		case ePODFileMatBlendColour:	if(!src.ReadArray32(s.pfBlendColour, sizeof(s.pfBlendColour) / sizeof(*s.pfBlendColour)))	return false;	break;
		case ePODFileMatBlendFactor:	if(!src.ReadArray32(s.pfBlendFactor, sizeof(s.pfBlendFactor) / sizeof(*s.pfBlendFactor)))	return false;	break;

		case ePODFileMatUserData:
			if(!src.ReadAfterAlloc(s.pUserData, nLen))
				return false;
			else
			{
				s.nUserDataSize = nLen;
				break;
			}

		default:
			if(!src.Skip(nLen)) return false;
		}
	}
	return false;
}

/*!***************************************************************************
 @Function			PVRTFixInterleavedEndiannessUsingCPODData
 @Modified			pInterleaved - The interleaved data
 @Input				data - The CPODData.
 @Return			ui32Size - Number of elements in pInterleaved
 @Description		Called multiple times and goes through the interleaved data
					correcting the endianness.
*****************************************************************************/
static void PVRTFixInterleavedEndiannessUsingCPODData(unsigned char* pInterleaved, CPODData &data, unsigned int ui32Size)
{
	if(!data.n)
		return;

	size_t ui32TypeSize = PVRTModelPODDataTypeSize(data.eType);

	unsigned char ub[4];
	unsigned char *pData = pInterleaved + (size_t) data.pData;

	switch(ui32TypeSize)
	{
		case 1: return;
		case 2:
			{
				for(unsigned int i = 0; i < ui32Size; ++i)
				{
					for(unsigned int j = 0; j < data.n; ++j)
					{
						ub[0] = pData[ui32TypeSize * j + 0];
						ub[1] = pData[ui32TypeSize * j + 1];

						((unsigned short*) pData)[j] = (unsigned short) ((ub[1] << 8) | ub[0]);
					}

					pData += data.nStride;
				}
			}
			break;
		case 4:
			{
				for(unsigned int i = 0; i < ui32Size; ++i)
				{
					for(unsigned int j = 0; j < data.n; ++j)
					{
						ub[0] = pData[ui32TypeSize * j + 0];
						ub[1] = pData[ui32TypeSize * j + 1];
						ub[2] = pData[ui32TypeSize * j + 2];
						ub[3] = pData[ui32TypeSize * j + 3];

						((unsigned int*) pData)[j] = (unsigned int) ((ub[3] << 24) | (ub[2] << 16) | (ub[1] << 8) | ub[0]);
					}

					pData += data.nStride;
				}
			}
			break;
		default: { _ASSERT(false); }
	};
}

static void PVRTFixInterleavedEndianness(SPODMesh &s)
{
	if(!s.pInterleaved || PVRTIsLittleEndian())
		return;

	PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.sVertex, s.nNumVertex);
	PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.sNormals, s.nNumVertex);
	PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.sTangents, s.nNumVertex);
	PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.sBinormals, s.nNumVertex);

	for(unsigned int i = 0; i < s.nNumUVW; ++i)
		PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.psUVW[i], s.nNumVertex);

	PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.sVtxColours, s.nNumVertex);
	PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.sBoneIdx, s.nNumVertex);
	PVRTFixInterleavedEndiannessUsingCPODData(s.pInterleaved, s.sBoneWeight, s.nNumVertex);
}

/*!***************************************************************************
 @Function			ReadMesh
 @Modified			s The SPODMesh to read into
 @Input				src	CSource object to read data from.
 @Return			true if successful
 @Description		Read a mesh block in from a pod file
*****************************************************************************/
static bool ReadMesh(
	SPODMesh	&s,
	CSource		&src)
{
	unsigned int	nName, nLen;
	unsigned int	nUVWs=0;

	PVRTMatrixIdentity(s.mUnpackMatrix);

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileMesh | PVRTMODELPOD_TAG_END:
			if(nUVWs != s.nNumUVW)
				return false;
			PVRTFixInterleavedEndianness(s);
			return true;

		case ePODFileMeshNumVtx:			if(!src.Read32(s.nNumVertex)) return false;													break;
		case ePODFileMeshNumFaces:			if(!src.Read32(s.nNumFaces)) return false;													break;
		case ePODFileMeshNumUVW:			if(!src.Read32(s.nNumUVW)) return false;	if(!SafeAlloc(s.psUVW, s.nNumUVW)) return false;	break;
		case ePODFileMeshStripLength:		if(!src.ReadAfterAlloc32(s.pnStripLength, nLen)) return false;								break;
		case ePODFileMeshNumStrips:			if(!src.Read32(s.nNumStrips)) return false;													break;
		case ePODFileMeshInterleaved:		if(!src.ReadAfterAlloc(s.pInterleaved, nLen)) return false;									break;
		case ePODFileMeshBoneBatches:		if(!src.ReadAfterAlloc32(s.sBoneBatches.pnBatches, nLen)) return false;						break;
		case ePODFileMeshBoneBatchBoneCnts:	if(!src.ReadAfterAlloc32(s.sBoneBatches.pnBatchBoneCnt, nLen)) return false;					break;
		case ePODFileMeshBoneBatchOffsets:	if(!src.ReadAfterAlloc32(s.sBoneBatches.pnBatchOffset, nLen)) return false;					break;
		case ePODFileMeshBoneBatchBoneMax:	if(!src.Read32(s.sBoneBatches.nBatchBoneMax)) return false;									break;
		case ePODFileMeshBoneBatchCnt:		if(!src.Read32(s.sBoneBatches.nBatchCnt)) return false;										break;
		case ePODFileMeshUnpackMatrix:		if(!src.ReadArray32(&s.mUnpackMatrix.f[0], 16)) return false;										break;

		case ePODFileMeshFaces:			if(!ReadCPODData(s.sFaces, src, ePODFileMeshFaces, true)) return false;							break;
		case ePODFileMeshVtx:			if(!ReadCPODData(s.sVertex, src, ePODFileMeshVtx, s.pInterleaved == 0)) return false;			break;
		case ePODFileMeshNor:			if(!ReadCPODData(s.sNormals, src, ePODFileMeshNor, s.pInterleaved == 0)) return false;			break;
		case ePODFileMeshTan:			if(!ReadCPODData(s.sTangents, src, ePODFileMeshTan, s.pInterleaved == 0)) return false;			break;
		case ePODFileMeshBin:			if(!ReadCPODData(s.sBinormals, src, ePODFileMeshBin, s.pInterleaved == 0)) return false;			break;
		case ePODFileMeshUVW:			if(!ReadCPODData(s.psUVW[nUVWs++], src, ePODFileMeshUVW, s.pInterleaved == 0)) return false;		break;
		case ePODFileMeshVtxCol:		if(!ReadCPODData(s.sVtxColours, src, ePODFileMeshVtxCol, s.pInterleaved == 0)) return false;		break;
		case ePODFileMeshBoneIdx:		if(!ReadCPODData(s.sBoneIdx, src, ePODFileMeshBoneIdx, s.pInterleaved == 0)) return false;		break;
		case ePODFileMeshBoneWeight:	if(!ReadCPODData(s.sBoneWeight, src, ePODFileMeshBoneWeight, s.pInterleaved == 0)) return false;	break;

		default:
			if(!src.Skip(nLen)) return false;
		}
	}
	return false;
}

/*!***************************************************************************
 @Function			ReadNode
 @Modified			s The SPODNode to read into
 @Input				src	CSource object to read data from.
 @Return			true if successful
 @Description		Read a node block in from a pod file
*****************************************************************************/
static bool ReadNode(
	SPODNode	&s,
	CSource		&src)
{
	unsigned int nName, nLen;
	bool bOldNodeFormat = false;
	VERTTYPE fPos[3]   = {0,0,0};
	VERTTYPE fQuat[4]  = {0,0,0,f2vt(1)};
	VERTTYPE fScale[7] = {f2vt(1),f2vt(1),f2vt(1),0,0,0,0};

	// Set default for user data
	s.pUserData = 0;
	s.nUserDataSize = 0;

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileNode | PVRTMODELPOD_TAG_END:
			if(bOldNodeFormat)
			{
				if(s.pfAnimPosition)
					s.nAnimFlags |= ePODHasPositionAni;
				else
				{
					s.pfAnimPosition = (VERTTYPE*) malloc(sizeof(fPos));
					memcpy(s.pfAnimPosition, fPos, sizeof(fPos));
				}

				if(s.pfAnimRotation)
					s.nAnimFlags |= ePODHasRotationAni;
				else
				{
					s.pfAnimRotation = (VERTTYPE*) malloc(sizeof(fQuat));
					memcpy(s.pfAnimRotation, fQuat, sizeof(fQuat));
				}

				if(s.pfAnimScale)
					s.nAnimFlags |= ePODHasScaleAni;
				else
				{
					s.pfAnimScale = (VERTTYPE*) malloc(sizeof(fScale));
					memcpy(s.pfAnimScale, fScale, sizeof(fScale));
				}
			}
			return true;

		case ePODFileNodeIdx:		if(!src.Read32(s.nIdx)) return false;								break;
		case ePODFileNodeName:		if(!src.ReadAfterAlloc(s.pszName, nLen)) return false;			break;
		case ePODFileNodeIdxMat:	if(!src.Read32(s.nIdxMaterial)) return false;						break;
		case ePODFileNodeIdxParent:	if(!src.Read32(s.nIdxParent)) return false;						break;
		case ePODFileNodeAnimFlags:if(!src.Read32(s.nAnimFlags))return false;							break;

		case ePODFileNodeAnimPosIdx:	if(!src.ReadAfterAlloc32(s.pnAnimPositionIdx, nLen)) return false;	break;
		case ePODFileNodeAnimPos:	if(!src.ReadAfterAlloc32(s.pfAnimPosition, nLen)) return false;	break;

		case ePODFileNodeAnimRotIdx:	if(!src.ReadAfterAlloc32(s.pnAnimRotationIdx, nLen)) return false;	break;
		case ePODFileNodeAnimRot:	if(!src.ReadAfterAlloc32(s.pfAnimRotation, nLen)) return false;	break;

		case ePODFileNodeAnimScaleIdx:	if(!src.ReadAfterAlloc32(s.pnAnimScaleIdx, nLen)) return false;	break;
		case ePODFileNodeAnimScale:	if(!src.ReadAfterAlloc32(s.pfAnimScale, nLen)) return false;		break;

		case ePODFileNodeAnimMatrixIdx:	if(!src.ReadAfterAlloc32(s.pnAnimMatrixIdx, nLen)) return false;	break;
		case ePODFileNodeAnimMatrix:if(!src.ReadAfterAlloc32(s.pfAnimMatrix, nLen)) return false;	break;

		case ePODFileNodeUserData:
			if(!src.ReadAfterAlloc(s.pUserData, nLen))
				return false;
			else
			{
				s.nUserDataSize = nLen;
				break;
			}

		// Parameters from the older pod format
		case ePODFileNodePos:		if(!src.ReadArray32(&fPos[0], 3))   return false;		bOldNodeFormat = true;		break;
		case ePODFileNodeRot:		if(!src.ReadArray32(&fQuat[0], 4))  return false;		bOldNodeFormat = true;		break;
		case ePODFileNodeScale:		if(!src.ReadArray32(&fScale[0], 3)) return false;		bOldNodeFormat = true;		break;

		default:
			if(!src.Skip(nLen)) return false;
		}
	}

	return false;
}

/*!***************************************************************************
 @Function			ReadTexture
 @Modified			s The SPODTexture to read into
 @Input				src	CSource object to read data from.
 @Return			true if successful
 @Description		Read a texture block in from a pod file
*****************************************************************************/
static bool ReadTexture(
	SPODTexture	&s,
	CSource		&src)
{
	unsigned int nName, nLen;

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileTexture | PVRTMODELPOD_TAG_END:			return true;

		case ePODFileTexName:		if(!src.ReadAfterAlloc(s.pszName, nLen)) return false;			break;

		default:
			if(!src.Skip(nLen)) return false;
		}
	}
	return false;
}

/*!***************************************************************************
 @Function			ReadScene
 @Modified			s The SPODScene to read into
 @Input				src	CSource object to read data from.
 @Return			true if successful
 @Description		Read a scene block in from a pod file
*****************************************************************************/
static bool ReadScene(
	SPODScene	&s,
	CSource		&src)
{
	unsigned int nName, nLen;
	unsigned int nCameras=0, nLights=0, nMaterials=0, nMeshes=0, nTextures=0, nNodes=0;
	s.nFPS = 30;
	s.fUnits = 1.0f;

	// Set default for user data
	s.pUserData = 0;
	s.nUserDataSize = 0;

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileScene | PVRTMODELPOD_TAG_END:
			if(nCameras		!= s.nNumCamera) return false;
			if(nLights		!= s.nNumLight) return false;
			if(nMaterials	!= s.nNumMaterial) return false;
			if(nMeshes		!= s.nNumMesh) return false;
			if(nTextures	!= s.nNumTexture) return false;
			if(nNodes		!= s.nNumNode) return false;
			return true;
			
		case ePODFileUnits:				if(!src.Read32(s.fUnits))	return false;				break;
		case ePODFileColourBackground:	if(!src.ReadArray32(&s.pfColourBackground[0], sizeof(s.pfColourBackground) / sizeof(*s.pfColourBackground))) return false;	break;
		case ePODFileColourAmbient:		if(!src.ReadArray32(&s.pfColourAmbient[0], sizeof(s.pfColourAmbient) / sizeof(*s.pfColourAmbient))) return false;		break;
		case ePODFileNumCamera:			if(!src.Read32(s.nNumCamera)) return false;			if(!SafeAlloc(s.pCamera, s.nNumCamera)) return false;		break;
		case ePODFileNumLight:			if(!src.Read32(s.nNumLight)) return false;			if(!SafeAlloc(s.pLight, s.nNumLight)) return false;			break;
		case ePODFileNumMesh:			if(!src.Read32(s.nNumMesh)) return false;				if(!SafeAlloc(s.pMesh, s.nNumMesh)) return false;			break;
		case ePODFileNumNode:			if(!src.Read32(s.nNumNode)) return false;				if(!SafeAlloc(s.pNode, s.nNumNode)) return false;			break;
		case ePODFileNumMeshNode:		if(!src.Read32(s.nNumMeshNode)) return false;			break;
		case ePODFileNumTexture:		if(!src.Read32(s.nNumTexture)) return false;			if(!SafeAlloc(s.pTexture, s.nNumTexture)) return false;		break;
		case ePODFileNumMaterial:		if(!src.Read32(s.nNumMaterial)) return false;			if(!SafeAlloc(s.pMaterial, s.nNumMaterial)) return false;	break;
		case ePODFileNumFrame:			if(!src.Read32(s.nNumFrame)) return false;			break;
		case ePODFileFPS:				if(!src.Read32(s.nFPS))	return false;				break;
		case ePODFileFlags:				if(!src.Read32(s.nFlags)) return false;				break;

		case ePODFileCamera:	if(!ReadCamera(s.pCamera[nCameras++], src)) return false;		break;
		case ePODFileLight:		if(!ReadLight(s.pLight[nLights++], src)) return false;			break;
		case ePODFileMaterial:	if(!ReadMaterial(s.pMaterial[nMaterials++], src)) return false;	break;
		case ePODFileMesh:		if(!ReadMesh(s.pMesh[nMeshes++], src)) return false;			break;
		case ePODFileNode:		if(!ReadNode(s.pNode[nNodes++], src)) return false;				break;
		case ePODFileTexture:	if(!ReadTexture(s.pTexture[nTextures++], src)) return false;	break;

		case ePODFileUserData:
			if(!src.ReadAfterAlloc(s.pUserData, nLen))
				return false;
			else
			{
				s.nUserDataSize = nLen;
				break;
			}

		default:
			if(!src.Skip(nLen)) return false;
		}
	}
	return false;
}

/*!***************************************************************************
 @Function			Read
 @Output			pS				SPODScene data. May be NULL.
 @Input				src				CSource object to read data from.
 @Output			pszExpOpt		Export options.
 @Input				count			Data size.
 @Output			pszHistory		Export history.
 @Input				historyCount	History data size.
 @Description		Loads the specified ".POD" file; returns the scene in
					pScene. This structure must later be destroyed with
					PVRTModelPODDestroy() to prevent memory leaks.
					".POD" files are exported from 3D Studio MAX using a
					PowerVR plugin. pS may be NULL if only the export options
					are required.
*****************************************************************************/
static bool Read(
	SPODScene		* const pS,
	CSource			&src,
	char			* const pszExpOpt,
	const size_t	count,
	char			* const pszHistory,
	const size_t	historyCount)
{
	unsigned int	nName, nLen;
	bool			bVersionOK = false, bDone = false;
	bool			bNeedOptions = pszExpOpt != 0;
	bool			bNeedHistory = pszHistory != 0;
	bool			bLoadingOptionsOrHistory = bNeedOptions || bNeedHistory;

	while(src.ReadMarker(nName, nLen))
	{
		switch(nName)
		{
		case ePODFileVersion:
			{
				char *pszVersion = NULL;
				if(nLen != strlen(PVRTMODELPOD_VERSION)+1) return false;
				if(!SafeAlloc(pszVersion, nLen)) return false;
				if(!src.Read(pszVersion, nLen)) return false;
				if(strcmp(pszVersion, PVRTMODELPOD_VERSION) != 0) return false;
				bVersionOK = true;
				FREE(pszVersion);
			}
			continue;

		case ePODFileScene:
			if(pS)
			{
				if(!ReadScene(*pS, src))
					return false;
				bDone = true;
			}
			continue;

		case ePODFileExpOpt:
			if(bNeedOptions)
			{
				if(!src.Read(pszExpOpt, PVRT_MIN(nLen, (unsigned int) count)))
					return false;

				bNeedOptions = false;

				if(count < nLen)
					nLen -= (unsigned int) count ; // Adjust nLen as the read has moved our position
				else
					nLen = 0;
			}
			break;

		case ePODFileHistory:
			if(bNeedHistory)
			{
				if(!src.Read(pszHistory, PVRT_MIN(nLen, (unsigned int) historyCount)))
					return false;

				bNeedHistory = false;

				if(count < nLen)
					nLen -= (unsigned int) historyCount; // Adjust nLen as the read has moved our position
				else
					nLen = 0;
			}
			break;

		case ePODFileScene | PVRTMODELPOD_TAG_END:
			return bVersionOK == true && bDone == true;

		case (unsigned int) ePODFileEndiannessMisMatch:
			PVRTErrorOutputDebug("Error: Endianness mismatch between the .pod file and the platform.\n");
			return false;

		}

		if(bLoadingOptionsOrHistory && !bNeedOptions && !bNeedHistory)
			return true; // The options and/or history has been loaded

		// Unhandled data, skip it
		if(!src.Skip(nLen))
			return false;
	}

	if(bLoadingOptionsOrHistory)
		return true;

	if(!pS)
		return false;
    
	/*
		Convert data to fixed or float point as this build desires
	*/
#ifdef PVRT_FIXED_POINT_ENABLE
	if(!(pS->nFlags & PVRTMODELPODSF_FIXED))
	{
		PVRTErrorOutputDebug("Error: The tools have been compiled with fixed point enabled but the POD file isn't in fixed point format.\n");
#else
	if(pS->nFlags & PVRTMODELPODSF_FIXED)
	{
		PVRTErrorOutputDebug("Error: The POD file is in fixed point format but the tools haven't been compiled with fixed point enabled.\n");
#endif
		return false;
	}


	return bVersionOK == true && bDone == true;
}

/*!***************************************************************************
 @Function			ReadFromSourceStream
 @Output			pS				CPVRTModelPOD data. May not be NULL.
 @Input				src				CSource object to read data from.
 @Output			pszExpOpt		Export options.
 @Input				count			Data size.
 @Output			pszHistory		Export history.
 @Input				historyCount	History data size.
 @Description		Loads the ".POD" data from the source stream; returns the scene
					in pS.
*****************************************************************************/
static EPVRTError ReadFromSourceStream(
	CPVRTModelPOD	* const pS,
	CSourceStream &src,
	char			* const pszExpOpt,
	const size_t	count,
	char			* const pszHistory,
	const size_t	historyCount)
{
	memset(pS, 0, sizeof(*pS));
	if(!Read(pszExpOpt || pszHistory ? NULL : pS, src, pszExpOpt, count, pszHistory, historyCount))
		return PVR_FAIL;

	if(pS->InitImpl() != PVR_SUCCESS)
		return PVR_FAIL;

	return PVR_SUCCESS;
}

/****************************************************************************
** Class: CPVRTModelPOD
****************************************************************************/

/*!***************************************************************************
 @Function			ReadFromFile
 @Input				pszFileName		Filename to load
 @Output			pszExpOpt		String in which to place exporter options
 @Input				count			Maximum number of characters to store.
 @Output			pszHistory		String in which to place the pod file history
 @Input				historyCount	Maximum number of characters to store.
 @Return			PVR_SUCCESS if successful, PVR_FAIL if not
 @Description		Loads the specified ".POD" file; returns the scene in
					pScene. This structure must later be destroyed with
					PVRTModelPODDestroy() to prevent memory leaks.
					".POD" files are exported using the PVRGeoPOD exporters.
					If pszExpOpt is NULL, the scene is loaded; otherwise the
					scene is not loaded and pszExpOpt is filled in. The same
					is true for pszHistory.
*****************************************************************************/
EPVRTError CPVRTModelPOD::ReadFromFile(
	const char		* const pszFileName,
	char			* const pszExpOpt,
	const size_t	count,
	char			* const pszHistory,
	const size_t	historyCount)
{
	CSourceStream src;

	if(!src.Init(pszFileName))
		return PVR_FAIL;

	return ReadFromSourceStream(this, src, pszExpOpt, count, pszHistory, historyCount);
}

/*!***************************************************************************
 @Function			ReadFromMemory
 @Input				pData			Data to load
 @Input				i32Size			Size of data
 @Output			pszExpOpt		String in which to place exporter options
 @Input				count			Maximum number of characters to store.
 @Output			pszHistory		String in which to place the pod file history
 @Input				historyCount	Maximum number of characters to store.
 @Return			PVR_SUCCESS if successful, PVR_FAIL if not
 @Description		Loads the supplied pod data. This data can be exported
					directly to a header using one of the pod exporters.
					If pszExpOpt is NULL, the scene is loaded; otherwise the
					scene is not loaded and pszExpOpt is filled in. The same
					is true for pszHistory.
*****************************************************************************/
EPVRTError CPVRTModelPOD::ReadFromMemory(
	const char		* pData,
	const size_t	i32Size,
	char			* const pszExpOpt,
	const size_t	count,
	char			* const pszHistory,
	const size_t	historyCount)
{
	CSourceStream src;

	if(!src.Init(pData, i32Size))
		return PVR_FAIL;

	return ReadFromSourceStream(this, src, pszExpOpt, count, pszHistory, historyCount);
}

/*!***************************************************************************
 @Function			ReadFromMemory
 @Input				scene			Scene data from the header file
 @Return			PVR_SUCCESS if successful, PVR_FAIL if not
 @Description		Sets the scene data from the supplied data structure. Use
					when loading from .H files.
*****************************************************************************/
EPVRTError CPVRTModelPOD::ReadFromMemory(
	const SPODScene &scene)
{
	Destroy();

	memset(this, 0, sizeof(*this));

	*(SPODScene*)this = scene;

	if(InitImpl() != PVR_SUCCESS)
		return PVR_FAIL;

	m_pImpl->bFromMemory = true;

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function			CopyFromMemory
 @Input				scene			Scene data
 @Return			PVR_SUCCESS if successful, PVR_FAIL if not
 @Description		Sets the scene data from the supplied data structure.
*****************************************************************************/
EPVRTError CPVRTModelPOD::CopyFromMemory(const SPODScene &scene)
{
	Destroy();

	unsigned int i;

	// SPODScene
	nNumFrame	= scene.nNumFrame;
	nFPS		= scene.nFPS;
	nFlags		= scene.nFlags;
	fUnits		= scene.fUnits;

	for(i = 0; i < 3; ++i)
	{
		pfColourBackground[i] = scene.pfColourBackground[i];
		pfColourAmbient[i]	  = scene.pfColourAmbient[i];
	}

	// Nodes
	if(scene.nNumNode && SafeAlloc(pNode, scene.nNumNode))
	{
		nNumNode     = scene.nNumNode;
		nNumMeshNode = scene.nNumMeshNode;

		for(i = 0; i < nNumNode; ++i)
			PVRTModelPODCopyNode(scene.pNode[i], pNode[i], scene.nNumFrame);
	}

	// Meshes
	if(scene.nNumMesh && SafeAlloc(pMesh, scene.nNumMesh))
	{
		nNumMesh = scene.nNumMesh;

		for(i = 0; i < nNumMesh; ++i)
			PVRTModelPODCopyMesh(scene.pMesh[i], pMesh[i]);
	}

	// Cameras
	if(scene.nNumCamera && SafeAlloc(pCamera, scene.nNumCamera))
	{
		nNumCamera = scene.nNumCamera;

		for(i = 0; i < nNumCamera; ++i)
			PVRTModelPODCopyCamera(scene.pCamera[i], pCamera[i], scene.nNumFrame);
	}

	// Lights
	if(scene.nNumLight && SafeAlloc(pLight, scene.nNumLight))
	{
		nNumLight = scene.nNumLight;

		for(i = 0; i < nNumLight; ++i)
			PVRTModelPODCopyLight(scene.pLight[i], pLight[i]);
	}

	// Textures
	if(scene.nNumTexture && SafeAlloc(pTexture, scene.nNumTexture))
	{
		nNumTexture = scene.nNumTexture;

		for(i = 0; i < nNumTexture; ++i)
			PVRTModelPODCopyTexture(scene.pTexture[i], pTexture[i]);
	}

	// Materials
	if(scene.nNumMaterial && SafeAlloc(pMaterial, scene.nNumMaterial))
	{
		nNumMaterial = scene.nNumMaterial;

		for(i = 0; i < nNumMaterial; ++i)
			PVRTModelPODCopyMaterial(scene.pMaterial[i], pMaterial[i]);
	}

	if(scene.pUserData && SafeAlloc(pUserData, scene.nUserDataSize))
	{
		memcpy(pUserData, scene.pUserData, nUserDataSize);
		nUserDataSize = scene.nUserDataSize;
	}

	if(InitImpl() != PVR_SUCCESS)
		return PVR_FAIL;

	return PVR_SUCCESS;
}

#if defined(_WIN32)
/*!***************************************************************************
 @Function			ReadFromResource
 @Input				pszName			Name of the resource to load from
 @Return			PVR_SUCCESS if successful, PVR_FAIL if not
 @Description		Loads the specified ".POD" file; returns the scene in
					pScene. This structure must later be destroyed with
					PVRTModelPODDestroy() to prevent memory leaks.
					".POD" files are exported from 3D Studio MAX using a
					PowerVR plugin.
*****************************************************************************/
EPVRTError CPVRTModelPOD::ReadFromResource(
	const TCHAR * const pszName)
{
	CSourceResource src;

	if(!src.Init(pszName))
		return PVR_FAIL;

	memset(this, 0, sizeof(*this));
	if(!Read(this, src, NULL, 0, NULL, 0))
		return PVR_FAIL;
	if(InitImpl() != PVR_SUCCESS)
		return PVR_FAIL;
	return PVR_SUCCESS;
}
#endif /* WIN32 */

/*!***********************************************************************
 @Function		InitImpl
 @Description	Used by the Read*() fns to initialise implementation
				details. Should also be called by applications which
				manually build data in the POD structures for rendering;
				in this case call it after the data has been created.
				Otherwise, do not call this function.
*************************************************************************/
EPVRTError CPVRTModelPOD::InitImpl()
{
	// Allocate space for implementation data
	delete m_pImpl;
	m_pImpl = new SPVRTPODImpl;
	if(!m_pImpl)
		return PVR_FAIL;

	// Zero implementation data
	memset(m_pImpl, 0, sizeof(*m_pImpl));

#ifdef _DEBUG
	m_pImpl->nWmTotal = 0;
#endif

	// Allocate world-matrix cache
	m_pImpl->pfCache		= new VERTTYPE[nNumNode];
	m_pImpl->pWmCache		= new PVRTMATRIX[nNumNode];
	m_pImpl->pWmZeroCache	= new PVRTMATRIX[nNumNode];
	FlushCache();

	return PVR_SUCCESS;
}

/*!***********************************************************************
 @Function		DestroyImpl
 @Description	Used to free memory allocated by the implementation.
*************************************************************************/
void CPVRTModelPOD::DestroyImpl()
{
	if(m_pImpl)
	{
		if(m_pImpl->pfCache)		delete [] m_pImpl->pfCache;
		if(m_pImpl->pWmCache)		delete [] m_pImpl->pWmCache;
		if(m_pImpl->pWmZeroCache)	delete [] m_pImpl->pWmZeroCache;

		delete m_pImpl;
		m_pImpl = 0;
	}
}

/*!***********************************************************************
 @Function		FlushCache
 @Description	Clears the matrix cache; use this if necessary when you
				edit the position or animation of a node.
*************************************************************************/
void CPVRTModelPOD::FlushCache()
{
	// Pre-calc frame zero matrices
	SetFrame(0);
	for(unsigned int i = 0; i < nNumNode; ++i)
		GetWorldMatrixNoCache(m_pImpl->pWmZeroCache[i], pNode[i]);

	// Load cache with frame-zero data
	memcpy(m_pImpl->pWmCache, m_pImpl->pWmZeroCache, nNumNode * sizeof(*m_pImpl->pWmCache));
	memset(m_pImpl->pfCache, 0, nNumNode * sizeof(*m_pImpl->pfCache));
}

/*!***********************************************************************
 @Function		IsLoaded
 @Description	Boolean to check whether a POD file has been loaded.
*************************************************************************/
bool CPVRTModelPOD::IsLoaded()
{
	return (m_pImpl!=NULL);
}

/*!***************************************************************************
 @Function			Constructor
 @Description		Initializes the pointer to scene data to NULL
*****************************************************************************/
CPVRTModelPOD::CPVRTModelPOD() : m_pImpl(NULL)
{}

/*!***************************************************************************
 @Function			Destructor
 @Description		Frees the memory allocated to store the scene in pScene.
*****************************************************************************/
CPVRTModelPOD::~CPVRTModelPOD()
{
	Destroy();
}

/*!***************************************************************************
 @Function			Destroy
 @Description		Frees the memory allocated to store the scene in pScene.
*****************************************************************************/
void CPVRTModelPOD::Destroy()
{
	unsigned int	i;

	if(m_pImpl != NULL)
	{
		/*
			Only attempt to free this memory if it was actually allocated at
			run-time, as opposed to compiled into the app.
		*/
		if(!m_pImpl->bFromMemory)
		{

			for(i = 0; i < nNumCamera; ++i)
				FREE(pCamera[i].pfAnimFOV);
			FREE(pCamera);

			FREE(pLight);

			for(i = 0; i < nNumMaterial; ++i)
			{
				FREE(pMaterial[i].pszName);
				FREE(pMaterial[i].pszEffectFile);
				FREE(pMaterial[i].pszEffectName);
				FREE(pMaterial[i].pUserData);
			}
			FREE(pMaterial);

			for(i = 0; i < nNumMesh; ++i) {
				FREE(pMesh[i].sFaces.pData);
				FREE(pMesh[i].pnStripLength);
				if(pMesh[i].pInterleaved)
				{
					FREE(pMesh[i].pInterleaved);
				}
				else
				{
					FREE(pMesh[i].sVertex.pData);
					FREE(pMesh[i].sNormals.pData);
					FREE(pMesh[i].sTangents.pData);
					FREE(pMesh[i].sBinormals.pData);
					for(unsigned int j = 0; j < pMesh[i].nNumUVW; ++j)
						FREE(pMesh[i].psUVW[j].pData);
					FREE(pMesh[i].sVtxColours.pData);
					FREE(pMesh[i].sBoneIdx.pData);
					FREE(pMesh[i].sBoneWeight.pData);
				}
				FREE(pMesh[i].psUVW);
				pMesh[i].sBoneBatches.Release();
			}
			FREE(pMesh);

			for(i = 0; i < nNumNode; ++i) {
				FREE(pNode[i].pszName);
				FREE(pNode[i].pfAnimPosition);
				FREE(pNode[i].pnAnimPositionIdx);
				FREE(pNode[i].pfAnimRotation);
				FREE(pNode[i].pnAnimRotationIdx);
				FREE(pNode[i].pfAnimScale);
				FREE(pNode[i].pnAnimScaleIdx);
				FREE(pNode[i].pfAnimMatrix);
				FREE(pNode[i].pnAnimMatrixIdx);
				FREE(pNode[i].pUserData);
				pNode[i].nAnimFlags = 0;
			}

			FREE(pNode);

			for(i = 0; i < nNumTexture; ++i)
				FREE(pTexture[i].pszName);
			FREE(pTexture);

			FREE(pUserData);
		}

		// Free the working space used by the implementation
		DestroyImpl();
	}

	memset(this, 0, sizeof(*this));
}

/*!***************************************************************************
 @Function			SetFrame
 @Input				fFrame			Frame number
 @Description		Set the animation frame for which subsequent Get*() calls
					should return data.
*****************************************************************************/
void CPVRTModelPOD::SetFrame(const VERTTYPE fFrame)
{
	if(nNumFrame) {
		/*
			Limit animation frames.

			Example: If there are 100 frames of animation, the highest frame
			number allowed is 98, since that will blend between frames 98 and
			99. (99 being of course the 100th frame.)
		*/
		_ASSERT(fFrame <= f2vt((float)(nNumFrame-1)));
		m_pImpl->nFrame = (int)vt2f(fFrame);
		m_pImpl->fBlend = fFrame - f2vt(m_pImpl->nFrame);
	}
	else
	{
		m_pImpl->fBlend = 0;
		m_pImpl->nFrame = 0;
	}

	m_pImpl->fFrame = fFrame;
}

/*!***************************************************************************
 @Function			GetRotationMatrix
 @Output			mOut			Rotation matrix
 @Input				node			Node to get the rotation matrix from
 @Description		Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetRotationMatrix(
	PVRTMATRIX		&mOut,
	const SPODNode	&node) const
{
	PVRTQUATERNION	q;

	if(node.pfAnimRotation)
	{
		if(node.nAnimFlags & ePODHasRotationAni)
		{
			if(node.pnAnimRotationIdx)
			{
				PVRTMatrixQuaternionSlerp(
					q,
					(PVRTQUATERNION&)node.pfAnimRotation[node.pnAnimRotationIdx[m_pImpl->nFrame]],
					(PVRTQUATERNION&)node.pfAnimRotation[node.pnAnimRotationIdx[m_pImpl->nFrame+1]], m_pImpl->fBlend);
			}
			else
			{
				PVRTMatrixQuaternionSlerp(
					q,
					(PVRTQUATERNION&)node.pfAnimRotation[4*m_pImpl->nFrame],
					(PVRTQUATERNION&)node.pfAnimRotation[4*(m_pImpl->nFrame+1)], m_pImpl->fBlend);
			}

			PVRTMatrixRotationQuaternion(mOut, q);
		}
		else
		{
			PVRTMatrixRotationQuaternion(mOut, *(PVRTQUATERNION*)node.pfAnimRotation);
		}
	}
	else
	{
		PVRTMatrixIdentity(mOut);
	}
}

/*!***************************************************************************
 @Function		GetRotationMatrix
 @Input			node			Node to get the rotation matrix from
 @Returns		Rotation matrix
 @Description	Generates the world matrix for the given Mesh Instance;
				applies the parent's transform too. Uses animation data.
*****************************************************************************/
PVRTMat4 CPVRTModelPOD::GetRotationMatrix(const SPODNode &node) const
{
	PVRTMat4 mOut;
	GetRotationMatrix(mOut,node);
	return mOut;
}

/*!***************************************************************************
 @Function			GetScalingMatrix
 @Output			mOut			Scaling matrix
 @Input				node			Node to get the rotation matrix from
 @Description		Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetScalingMatrix(
	PVRTMATRIX		&mOut,
	const SPODNode	&node) const
{
	PVRTVECTOR3 v;

	if(node.pfAnimScale)
	{
		if(node.nAnimFlags & ePODHasScaleAni)
		{
			if(node.pnAnimScaleIdx)
			{
				PVRTMatrixVec3Lerp(
					v,
					(PVRTVECTOR3&)node.pfAnimScale[node.pnAnimScaleIdx[m_pImpl->nFrame+0]],
					(PVRTVECTOR3&)node.pfAnimScale[node.pnAnimScaleIdx[m_pImpl->nFrame+1]], m_pImpl->fBlend);
			}
			else
			{
				PVRTMatrixVec3Lerp(
					v,
					(PVRTVECTOR3&)node.pfAnimScale[7*(m_pImpl->nFrame+0)],
					(PVRTVECTOR3&)node.pfAnimScale[7*(m_pImpl->nFrame+1)], m_pImpl->fBlend);
			}

			PVRTMatrixScaling(mOut, v.x, v.y, v.z);
		}
		else
		{
			PVRTMatrixScaling(mOut, node.pfAnimScale[0], node.pfAnimScale[1], node.pfAnimScale[2]);
		}
	}
	else
	{
		PVRTMatrixIdentity(mOut);
	}
}

/*!***************************************************************************
 @Function		GetScalingMatrix
 @Input			node			Node to get the rotation matrix from
 @Returns		Scaling matrix
 @Description	Generates the world matrix for the given Mesh Instance;
				applies the parent's transform too. Uses animation data.
*****************************************************************************/
PVRTMat4 CPVRTModelPOD::GetScalingMatrix(const SPODNode &node) const
{
	PVRTMat4 mOut;
	GetScalingMatrix(mOut, node);
	return mOut;
}

/*!***************************************************************************
 @Function			GetTranslation
 @Output			V				Translation vector
 @Input				node			Node to get the translation vector from
 @Description		Generates the translation vector for the given Mesh
					Instance. Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetTranslation(
	PVRTVECTOR3		&V,
	const SPODNode	&node) const
{
	if(node.pfAnimPosition)
	{
		if(node.nAnimFlags & ePODHasPositionAni)
		{
			if(node.pnAnimPositionIdx)
			{
				PVRTMatrixVec3Lerp(V,
					(PVRTVECTOR3&)node.pfAnimPosition[node.pnAnimPositionIdx[m_pImpl->nFrame+0]],
					(PVRTVECTOR3&)node.pfAnimPosition[node.pnAnimPositionIdx[m_pImpl->nFrame+1]], m_pImpl->fBlend);
			}
			else
			{
				PVRTMatrixVec3Lerp(V,
					(PVRTVECTOR3&)node.pfAnimPosition[3 * (m_pImpl->nFrame+0)],
					(PVRTVECTOR3&)node.pfAnimPosition[3 * (m_pImpl->nFrame+1)], m_pImpl->fBlend);
			}
		}
		else
		{
			V = *(PVRTVECTOR3*) node.pfAnimPosition;
		}
	}
	else
	{
		_ASSERT(false);
	}
}

/*!***************************************************************************
 @Function		GetTranslation
 @Input			node			Node to get the translation vector from
 @Returns		Translation vector
 @Description	Generates the translation vector for the given Mesh
				Instance. Uses animation data.
*****************************************************************************/
PVRTVec3 CPVRTModelPOD::GetTranslation(const SPODNode &node) const
{
	PVRTVec3 vOut;
	GetTranslation(vOut, node);
	return vOut;
}

/*!***************************************************************************
 @Function			GetTranslationMatrix
 @Output			mOut			Translation matrix
 @Input				node			Node to get the translation matrix from
 @Description		Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetTranslationMatrix(
	PVRTMATRIX		&mOut,
	const SPODNode	&node) const
{
	PVRTVECTOR3 v;

	if(node.pfAnimPosition)
	{
		if(node.nAnimFlags & ePODHasPositionAni)
		{
			if(node.pnAnimPositionIdx)
			{
				PVRTMatrixVec3Lerp(v,
					(PVRTVECTOR3&)node.pfAnimPosition[node.pnAnimPositionIdx[m_pImpl->nFrame+0]],
					(PVRTVECTOR3&)node.pfAnimPosition[node.pnAnimPositionIdx[m_pImpl->nFrame+1]], m_pImpl->fBlend);
			}
			else
			{
				PVRTMatrixVec3Lerp(v,
					(PVRTVECTOR3&)node.pfAnimPosition[3*(m_pImpl->nFrame+0)],
					(PVRTVECTOR3&)node.pfAnimPosition[3*(m_pImpl->nFrame+1)], m_pImpl->fBlend);
			}

			PVRTMatrixTranslation(mOut, v.x, v.y, v.z);
		}
		else
		{
			PVRTMatrixTranslation(mOut, node.pfAnimPosition[0], node.pfAnimPosition[1], node.pfAnimPosition[2]);
		}
	}
	else
	{
		PVRTMatrixIdentity(mOut);
	}
}

/*!***************************************************************************
 @Function		GetTranslationMatrix
 @Input			node			Node to get the translation matrix from
 @Returns		Translation matrix
 @Description	Generates the world matrix for the given Mesh Instance;
				applies the parent's transform too. Uses animation data.
*****************************************************************************/
PVRTMat4 CPVRTModelPOD::GetTranslationMatrix(const SPODNode &node) const
{
	PVRTMat4 mOut;
	GetTranslationMatrix(mOut, node);
	return mOut;
}

/*!***************************************************************************
 @Function		GetTransformationMatrix
 @Output		mOut			Transformation matrix
 @Input			node			Node to get the transformation matrix from
 @Description	Generates the world matrix for the given Mesh Instance;
				applies the parent's transform too. Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetTransformationMatrix(PVRTMATRIX &mOut, const SPODNode &node) const
{
	if(node.pfAnimMatrix)
	{
		if(node.nAnimFlags & ePODHasMatrixAni)
		{
			if(node.pnAnimMatrixIdx)
				mOut = *((PVRTMATRIX*) &node.pfAnimMatrix[node.pnAnimMatrixIdx[m_pImpl->nFrame]]);
			else
				mOut = *((PVRTMATRIX*) &node.pfAnimMatrix[16*m_pImpl->nFrame]);
		}
		else
		{
			mOut = *((PVRTMATRIX*) node.pfAnimMatrix);
		}
	}
	else
	{
		PVRTMatrixIdentity(mOut);
	}
}
/*!***************************************************************************
 @Function			GetWorldMatrixNoCache
 @Output			mOut			World matrix
 @Input				node			Node to get the world matrix from
 @Description		Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetWorldMatrixNoCache(
	PVRTMATRIX		&mOut,
	const SPODNode	&node) const
{
	PVRTMATRIX mTmp;

    if(node.pfAnimMatrix) // The transformations are stored as matrices
		GetTransformationMatrix(mOut, node);
	else
	{
		// Scale
		GetScalingMatrix(mOut, node);

		// Rotation
		GetRotationMatrix(mTmp, node);
		PVRTMatrixMultiply(mOut, mOut, mTmp);

		// Translation
		GetTranslationMatrix(mTmp, node);
		PVRTMatrixMultiply(mOut, mOut, mTmp);
	}

 	// Do we have to worry about a parent?
	if(node.nIdxParent < 0)
		return;

	// Apply parent's transform too.
	GetWorldMatrixNoCache(mTmp, pNode[node.nIdxParent]);
	PVRTMatrixMultiply(mOut, mOut, mTmp);
}

/*!***************************************************************************
 @Function		GetWorldMatrixNoCache
 @Input			node			Node to get the world matrix from
 @Returns		World matrix
 @Description	Generates the world matrix for the given Mesh Instance;
				applies the parent's transform too. Uses animation data.
*****************************************************************************/
PVRTMat4 CPVRTModelPOD::GetWorldMatrixNoCache(const SPODNode& node) const
{
	PVRTMat4 mWorld;
	GetWorldMatrixNoCache(mWorld,node);
	return mWorld;
}

/*!***************************************************************************
 @Function			GetWorldMatrix
 @Output			mOut			World matrix
 @Input				node			Node to get the world matrix from
 @Description		Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetWorldMatrix(
	PVRTMATRIX		&mOut,
	const SPODNode	&node) const
{
	unsigned int nIdx;

#ifdef _DEBUG
	++m_pImpl->nWmTotal;
	m_pImpl->fHitPerc = (float)m_pImpl->nWmCacheHit / (float)m_pImpl->nWmTotal;
	m_pImpl->fHitPercZero = (float)m_pImpl->nWmZeroCacheHit / (float)m_pImpl->nWmTotal;
#endif

	// Calculate a node index
	nIdx = (unsigned int)(&node - pNode);

	// There is a dedicated cache for frame 0 data
	if(m_pImpl->fFrame == 0)
	{
		mOut = m_pImpl->pWmZeroCache[nIdx];
#ifdef _DEBUG
		++m_pImpl->nWmZeroCacheHit;
#endif
		return;
	}

	// Has this matrix been calculated & cached?
	if(m_pImpl->fFrame == m_pImpl->pfCache[nIdx])
	{
		mOut = m_pImpl->pWmCache[nIdx];
#ifdef _DEBUG
		++m_pImpl->nWmCacheHit;
#endif
		return;
	}

	GetWorldMatrixNoCache(mOut, node);

	// Cache the matrix
	m_pImpl->pfCache[nIdx]	= m_pImpl->fFrame;
	m_pImpl->pWmCache[nIdx]	= mOut;
}

/*!***************************************************************************
 @Function		GetWorldMatrix
 @Input			node			Node to get the world matrix from
 @Returns		World matrix
 @Description	Generates the world matrix for the given Mesh Instance;
				applies the parent's transform too. Uses animation data.
*****************************************************************************/
PVRTMat4 CPVRTModelPOD::GetWorldMatrix(const SPODNode& node) const
{
	PVRTMat4 mWorld;
	GetWorldMatrix(mWorld,node);
	return mWorld;
}

/*!***************************************************************************
 @Function			GetBoneWorldMatrix
 @Output			mOut			Bone world matrix
 @Input				NodeMesh		Mesh to take the bone matrix from
 @Input				NodeBone		Bone to take the matrix from
 @Description		Generates the world matrix for the given bone.
*****************************************************************************/
void CPVRTModelPOD::GetBoneWorldMatrix(
	PVRTMATRIX		&mOut,
	const SPODNode	&NodeMesh,
	const SPODNode	&NodeBone)
{
	PVRTMATRIX	mTmp;
	VERTTYPE	fFrame;

	fFrame = m_pImpl->fFrame;

	SetFrame(0);

	// Transform by object matrix
	GetWorldMatrix(mOut, NodeMesh);

	// Back transform bone from frame 0 position
	GetWorldMatrix(mTmp, NodeBone);
	PVRTMatrixInverse(mTmp, mTmp);
	PVRTMatrixMultiply(mOut, mOut, mTmp);

	// The bone origin should now be at the origin

	SetFrame(fFrame);

	// Transform bone into frame fFrame position
	GetWorldMatrix(mTmp, NodeBone);
	PVRTMatrixMultiply(mOut, mOut, mTmp);
}

/*!***************************************************************************
 @Function		GetBoneWorldMatrix
 @Input			NodeMesh		Mesh to take the bone matrix from
 @Input			NodeBone		Bone to take the matrix from
 @Returns		Bone world matrix
 @Description	Generates the world matrix for the given bone.
*****************************************************************************/
PVRTMat4 CPVRTModelPOD::GetBoneWorldMatrix(
	const SPODNode	&NodeMesh,
	const SPODNode	&NodeBone)
{
	PVRTMat4 mOut;
	GetBoneWorldMatrix(mOut,NodeMesh,NodeBone);
	return mOut;
}

/*!***************************************************************************
 @Function			GetCamera
 @Output			vFrom			Position of the camera
 @Output			vTo				Target of the camera
 @Output			vUp				Up direction of the camera
 @Input				nIdx			Camera number
 @Return			Camera horizontal FOV
 @Description		Calculate the From, To and Up vectors for the given
					camera. Uses animation data.
					Note that even if the camera has a target, *pvTo is not
					the position of that target. *pvTo is a position in the
					correct direction of the target, one unit away from the
					camera.
*****************************************************************************/
VERTTYPE CPVRTModelPOD::GetCamera(
	PVRTVECTOR3			&vFrom,
	PVRTVECTOR3			&vTo,
	PVRTVECTOR3			&vUp,
	const unsigned int	nIdx) const
{
	PVRTMATRIX		mTmp;
	VERTTYPE		*pfData;
	SPODCamera		*pCam;
	const SPODNode	*pNd;

	_ASSERT(nIdx < nNumCamera);

	// Camera nodes are after the mesh and light nodes in the array
	pNd = &pNode[nNumMeshNode + nNumLight + nIdx];

	pCam = &pCamera[pNd->nIdx];

	GetWorldMatrix(mTmp, *pNd);

	// View position is 0,0,0,1 transformed by world matrix
	vFrom.x = mTmp.f[12];
	vFrom.y = mTmp.f[13];
	vFrom.z = mTmp.f[14];

	// View direction is 0,-1,0,1 transformed by world matrix
	vTo.x = -mTmp.f[4] + mTmp.f[12];
	vTo.y = -mTmp.f[5] + mTmp.f[13];
	vTo.z = -mTmp.f[6] + mTmp.f[14];

#if defined(BUILD_DX11)
	/*
		When you rotate the camera from "straight forward" to "straight down", in
		D3D the UP vector will be [0, 0, 1]
	*/
	vUp.x = mTmp.f[ 8];
	vUp.y = mTmp.f[ 9];
	vUp.z = mTmp.f[10];
#endif

#if defined(BUILD_OGL) || defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	/*
		When you rotate the camera from "straight forward" to "straight down", in
		OpenGL the UP vector will be [0, 0, -1]
	*/
	vUp.x = -mTmp.f[ 8];
	vUp.y = -mTmp.f[ 9];
	vUp.z = -mTmp.f[10];
#endif

	/*
		Find & calculate FOV value
	*/
	if(pCam->pfAnimFOV) {
		pfData = &pCam->pfAnimFOV[m_pImpl->nFrame];

		return pfData[0] + m_pImpl->fBlend * (pfData[1] - pfData[0]);
	} else {
		return pCam->fFOV;
	}
}

/*!***************************************************************************
 @Function			GetCameraPos
 @Output			vFrom			Position of the camera
 @Output			vTo				Target of the camera
 @Input				nIdx			Camera number
 @Return			Camera horizontal FOV
 @Description		Calculate the position of the camera and its target. Uses
					animation data.
					If the queried camera does not have a target, *pvTo is
					not changed.
*****************************************************************************/
VERTTYPE CPVRTModelPOD::GetCameraPos(
	PVRTVECTOR3			&vFrom,
	PVRTVECTOR3			&vTo,
	const unsigned int	nIdx) const
{
	PVRTMATRIX		mTmp;
	VERTTYPE		*pfData;
	SPODCamera		*pCam;
	const SPODNode	*pNd;

	_ASSERT(nIdx < nNumCamera);

	// Camera nodes are after the mesh and light nodes in the array
	pNd = &pNode[nNumMeshNode + nNumLight + nIdx];

	// View position is 0,0,0,1 transformed by world matrix
	GetWorldMatrix(mTmp, *pNd);
	vFrom.x = mTmp.f[12];
	vFrom.y = mTmp.f[13];
	vFrom.z = mTmp.f[14];

	pCam = &pCamera[pNd->nIdx];
	if(pCam->nIdxTarget >= 0)
	{
		// View position is 0,0,0,1 transformed by world matrix
		GetWorldMatrix(mTmp, pNode[pCam->nIdxTarget]);
		vTo.x = mTmp.f[12];
		vTo.y = mTmp.f[13];
		vTo.z = mTmp.f[14];
	}

	/*
		Find & calculate FOV value
	*/
	if(pCam->pfAnimFOV) {
		pfData = &pCam->pfAnimFOV[m_pImpl->nFrame];

		return pfData[0] + m_pImpl->fBlend * (pfData[1] - pfData[0]);
	} else {
		return pCam->fFOV;
	}
}

/*!***************************************************************************
 @Function			GetLight
 @Output			vPos			Position of the light
 @Output			vDir			Direction of the light
 @Input				nIdx			Light number
 @Description		Calculate the position and direction of the given Light.
					Uses animation data.
*****************************************************************************/
void CPVRTModelPOD::GetLight(
	PVRTVECTOR3			&vPos,
	PVRTVECTOR3			&vDir,
	const unsigned int	nIdx) const
{
	PVRTMATRIX		mTmp;
	const SPODNode	*pNd;

	_ASSERT(nIdx < nNumLight);

	// Light nodes are after the mesh nodes in the array
	pNd = &pNode[nNumMeshNode + nIdx];

	GetWorldMatrix(mTmp, *pNd);

	// View position is 0,0,0,1 transformed by world matrix
	vPos.x = mTmp.f[12];
	vPos.y = mTmp.f[13];
	vPos.z = mTmp.f[14];

	// View direction is 0,-1,0,0 transformed by world matrix
	vDir.x = -mTmp.f[4];
	vDir.y = -mTmp.f[5];
	vDir.z = -mTmp.f[6];
}

/*!***************************************************************************
 @Function		GetLightPositon
 @Input			u32Idx			Light number
 @Return		PVRTVec4 position of light with w set correctly
 @Description	Calculates the position of the given light. Uses animation data
*****************************************************************************/
PVRTVec4 CPVRTModelPOD::GetLightPosition(const unsigned int u32Idx) const
{	// TODO: make this a real function instead of just wrapping GetLight()
	PVRTVec3 vPos, vDir;
	GetLight(vPos,vDir,u32Idx);

	_ASSERT(u32Idx < nNumLight);
	_ASSERT(pLight[u32Idx].eType!=ePODDirectional);
	return PVRTVec4(vPos,1);
}

/*!***************************************************************************
 @Function		GetLightDirection
 @Input			u32Idx			Light number
 @Return		PVRTVec4 direction of light with w set correctly
 @Description	Calculate the direction of the given Light. Uses animation data.
*****************************************************************************/
PVRTVec4 CPVRTModelPOD::GetLightDirection(const unsigned int u32Idx) const
{	// TODO: make this a real function instead of just wrapping GetLight()
	PVRTVec3 vPos, vDir;
	GetLight(vPos,vDir,u32Idx);

	_ASSERT(u32Idx < nNumLight);
	_ASSERT(pLight[u32Idx].eType!=ePODPoint);
	return PVRTVec4(vDir,0);
}

/*!***************************************************************************
 @Function			CreateSkinIdxWeight
 @Output			pIdx				Four bytes containing matrix indices for vertex (0..255) (D3D: use UBYTE4)
 @Output			pWeight				Four bytes containing blend weights for vertex (0.0 .. 1.0) (D3D: use D3DCOLOR)
 @Input				nVertexBones		Number of bones this vertex uses
 @Input				pnBoneIdx			Pointer to 'nVertexBones' indices
 @Input				pfBoneWeight		Pointer to 'nVertexBones' blend weights
 @Description		Creates the matrix indices and blend weights for a boned
					vertex. Call once per vertex of a boned mesh.
*****************************************************************************/
EPVRTError CPVRTModelPOD::CreateSkinIdxWeight(
	char			* const pIdx,			// Four bytes containing matrix indices for vertex (0..255) (D3D: use UBYTE4)
	char			* const pWeight,		// Four bytes containing blend weights for vertex (0.0 .. 1.0) (D3D: use D3DCOLOR)
	const int		nVertexBones,			// Number of bones this vertex uses
	const int		* const pnBoneIdx,		// Pointer to 'nVertexBones' indices
	const VERTTYPE	* const pfBoneWeight)	// Pointer to 'nVertexBones' blend weights
{
	int i, nSum;
	int nIdx[4];
	int nWeight[4];

	for(i = 0; i < nVertexBones; ++i)
	{
		nIdx[i]		= pnBoneIdx[i];
		nWeight[i]	= (int)vt2f((VERTTYPEMUL(f2vt(255.0f), pfBoneWeight[i])));

		if(nIdx[i] > 255)
		{
			PVRTErrorOutputDebug("Too many bones (highest index is 255).\n");
			return PVR_FAIL;
		}

		nWeight[i]	= PVRT_MAX(nWeight[i], 0);
		nWeight[i]	= PVRT_MIN(nWeight[i], 255);
	}

	for(; i < 4; ++i)
	{
		nIdx[i]		= 0;
		nWeight[i]	= 0;
	}

	if(nVertexBones)
	{
		// It's important the weights sum to 1
		nSum = 0;
		for(i = 0; i < 4; ++i)
			nSum += nWeight[i];

		if(!nSum)
			return PVR_FAIL;

		_ASSERT(nSum <= 255);

		i = 0;
		while(nSum < 255)
		{
			if(nWeight[i]) {
				++nWeight[i];
				++nSum;
			}

			if(++i > 3)
				i = 0;
		}

		_ASSERT(nSum == 255);
	}

#if defined(BUILD_DX11)
	*(unsigned int*)pIdx = ((unsigned int)(((nIdx[3]&0xff)<<24)|((nIdx[2]&0xff)<<16)|((nIdx[1]&0xff)<<8)|(nIdx[0]&0xff)));					// UBYTE4 is WZYX
	*(unsigned int*)pWeight = ((unsigned int)(((nWeight[3]&0xff)<<24)|((nWeight[0]&0xff)<<16)|((nWeight[1]&0xff)<<8)|(nWeight[2]&0xff)));	// D3DCOLORs are WXYZ
#endif

#if defined(BUILD_OGL) || defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	// Return indices and weights as bytes
	for(i = 0; i < 4; ++i)
	{
		pIdx[i]		= (char) nIdx[i];
		pWeight[i]	= (char) nWeight[i];
	}
#endif

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function			SavePOD
 @Input				pszFilename		Filename to save to
 @Input				pszExpOpt		A string containing the options used by the exporter
 @Description		Save a binary POD file (.POD).
*****************************************************************************/
EPVRTError CPVRTModelPOD::SavePOD(const char * const pszFilename, const char * const pszExpOpt, const char * const pszHistory)
{
	FILE	*pFile;
	bool	bRet;

	pFile = fopen(pszFilename, "wb+");
	if(!pFile)
		return PVR_FAIL;

	bRet = WritePOD(pFile, pszExpOpt, pszHistory, *this);

	// Done
	fclose(pFile);
	return bRet ? PVR_SUCCESS : PVR_FAIL;
}


/*!***************************************************************************
 @Function			PVRTModelPODDataTypeSize
 @Input				type		Type to get the size of
 @Return			Size of the data element
 @Description		Returns the size of each data element.
*****************************************************************************/
PVRTuint32 PVRTModelPODDataTypeSize(const EPVRTDataType type)
{
	switch(type)
	{
	default:
		_ASSERT(false);
		return 0;
	case EPODDataFloat:
		return static_cast<PVRTuint32>(sizeof(float));
	case EPODDataInt:
	case EPODDataUnsignedInt:
		return static_cast<PVRTuint32>(sizeof(int));
	case EPODDataShort:
	case EPODDataShortNorm:
	case EPODDataUnsignedShort:
	case EPODDataUnsignedShortNorm:
		return static_cast<PVRTuint32>(sizeof(unsigned short));
	case EPODDataRGBA:
		return static_cast<PVRTuint32>(sizeof(unsigned int));
	case EPODDataABGR:
		return static_cast<PVRTuint32>(sizeof(unsigned int));
	case EPODDataARGB:
		return static_cast<PVRTuint32>(sizeof(unsigned int));
	case EPODDataD3DCOLOR:
		return static_cast<PVRTuint32>(sizeof(unsigned int));
	case EPODDataUBYTE4:
		return static_cast<PVRTuint32>(sizeof(unsigned int));
	case EPODDataDEC3N:
		return static_cast<PVRTuint32>(sizeof(unsigned int));
	case EPODDataFixed16_16:
		return static_cast<PVRTuint32>(sizeof(unsigned int));
	case EPODDataUnsignedByte:
	case EPODDataUnsignedByteNorm:
	case EPODDataByte:
	case EPODDataByteNorm:
		return static_cast<PVRTuint32>(sizeof(unsigned char));
	}
}

/*!***************************************************************************
@Function			PVRTModelPODDataTypeComponentCount
@Input				type		Type to get the number of components from
@Return				number of components in the data element
@Description		Returns the number of components in a data element.
*****************************************************************************/
PVRTuint32 PVRTModelPODDataTypeComponentCount(const EPVRTDataType type)
{
	switch(type)
	{
	default:
		_ASSERT(false);
		return 0;

	case EPODDataFloat:
	case EPODDataInt:
	case EPODDataUnsignedInt:
	case EPODDataShort:
	case EPODDataShortNorm:
	case EPODDataUnsignedShort:
	case EPODDataUnsignedShortNorm:
	case EPODDataFixed16_16:
	case EPODDataByte:
	case EPODDataByteNorm:
	case EPODDataUnsignedByte:
	case EPODDataUnsignedByteNorm:
		return 1;

	case EPODDataDEC3N:
		return 3;

	case EPODDataRGBA:
	case EPODDataABGR:
	case EPODDataARGB:
	case EPODDataD3DCOLOR:
	case EPODDataUBYTE4:
		return 4;
	}
}

/*!***************************************************************************
 @Function			PVRTModelPODDataStride
 @Input				data		Data elements
 @Return			Size of the vector elements
 @Description		Returns the size of the vector of data elements.
*****************************************************************************/
PVRTuint32 PVRTModelPODDataStride(const CPODData &data)
{
	return PVRTModelPODDataTypeSize(data.eType) * data.n;
}

/*!***************************************************************************
 @Function			PVRTModelPODDataConvert
 @Modified			data		Data elements to convert
 @Input				eNewType	New type of elements
 @Input				nCnt		Number of elements
 @Description		Convert the format of the array of vectors.
*****************************************************************************/
void PVRTModelPODDataConvert(CPODData &data, const unsigned int nCnt, const EPVRTDataType eNewType)
{
	PVRTVECTOR4f	v;
	unsigned int	i;
	CPODData		old;

	if(!data.pData || data.eType == eNewType)
		return;

	old = data;

	switch(eNewType)
	{
	case EPODDataFloat:
	case EPODDataInt:
	case EPODDataUnsignedInt:
	case EPODDataUnsignedShort:
	case EPODDataUnsignedShortNorm:
	case EPODDataFixed16_16:
	case EPODDataUnsignedByte:
	case EPODDataUnsignedByteNorm:
	case EPODDataShort:
	case EPODDataShortNorm:
	case EPODDataByte:
	case EPODDataByteNorm:
		data.n = (PVRTuint32) (old.n * PVRTModelPODDataTypeComponentCount(old.eType));
		break;
	case EPODDataRGBA:
	case EPODDataABGR:
	case EPODDataARGB:
	case EPODDataD3DCOLOR:
	case EPODDataUBYTE4:
	case EPODDataDEC3N:
		data.n = 1;
		break;
	default:
		_ASSERT(false); // unrecognised type
		break;
	}

	data.eType = eNewType;
	data.nStride = (unsigned int)PVRTModelPODDataStride(data);

	// If the old & new strides are identical, we can convert it in place
	if(old.nStride != data.nStride)
	{
		data.pData = (unsigned char*)malloc(data.nStride * nCnt);
	}

	for(i = 0; i < nCnt; ++i)
	{
		PVRTVertexRead(&v, old.pData + i * old.nStride, old.eType, old.n);
		PVRTVertexWrite(data.pData + i * data.nStride, eNewType, (int) (data.n * PVRTModelPODDataTypeComponentCount(data.eType)), &v);
	}

	if(old.nStride != data.nStride)
	{
		FREE(old.pData);
	}
}

/*!***************************************************************************
 @Function		PVRTModelPODScaleAndConvertVtxData
 @Modified		mesh		POD mesh to scale and convert the mesh data
 @Input			eNewType	The data type to scale and convert the vertex data to
 @Return		PVR_SUCCESS on success and PVR_FAIL on failure.
 @Description	Scales the vertex data to fit within the range of the requested
				data type and then converts the data to that type. This function
				isn't currently compiled in for fixed point builds of the tools.
*****************************************************************************/
#if !defined(PVRT_FIXED_POINT_ENABLE)
EPVRTError PVRTModelPODScaleAndConvertVtxData(SPODMesh &mesh, const EPVRTDataType eNewType)
{
	// Initialise the matrix to identity
	PVRTMatrixIdentity(mesh.mUnpackMatrix);

	// No vertices to process
	if(!mesh.nNumVertex)
		return PVR_SUCCESS;

	// This function expects the data to be floats and not interleaved
	if(mesh.sVertex.eType != EPODDataFloat && mesh.pInterleaved != 0)
		return PVR_FAIL;

	if(eNewType == EPODDataFloat) // Nothing to do
		return PVR_FAIL;

	// A few variables
	float fLower = 0.0f, fUpper = 0.0f;
	PVRTBOUNDINGBOX BoundingBox;
	PVRTMATRIX	mOffset, mScale;
	PVRTVECTOR4 v,o;

	// Set the w component of o as it is needed for later
	o.w = 1.0f;

	// Calc bounding box
	PVRTBoundingBoxComputeInterleaved(&BoundingBox, mesh.sVertex.pData,  mesh.nNumVertex, 0,  mesh.sVertex.nStride);

	// Get new type data range that we wish to scale the data to

	// Due to a hardware bug in early MBXs in some cases we clamp the data to the minimum possible value +1
	switch(eNewType)
	{
	case EPODDataInt:
		fUpper = 1 << 30;
		fLower = -fUpper;
	break;
	case EPODDataUnsignedInt:
		fUpper = 1 << 30;
	break;
	case EPODDataShort:
	case EPODDataFixed16_16:
		fUpper =  32767.0f;
		fLower = -fUpper;
	break;
	case EPODDataUnsignedShort:
		fUpper = 0x0ffff;
	break;
	case EPODDataRGBA:
	case EPODDataABGR:
	case EPODDataARGB:
	case EPODDataD3DCOLOR:
		fUpper = 1.0f;
	break;
	case EPODDataUBYTE4:
	case EPODDataUnsignedByte:
		fUpper = 0x0ff;
	break;
	case EPODDataShortNorm:
	case EPODDataUnsignedShortNorm:
	case EPODDataByteNorm:
	case EPODDataUnsignedByteNorm:
		fUpper =  1.0f;
		fLower = -fUpper;
	break;
	case EPODDataDEC3N:
		fUpper =  511.0f;
		fLower = -fUpper;
	break;
	case EPODDataByte:
		fUpper =  127.0f;
		fLower = -fUpper;
	break;
	default:
		_ASSERT(false);
		return PVR_FAIL; // Unsupported format specified
	}

	PVRTVECTOR3f vScale, vOffset;

	float fRange = fUpper - fLower;
	vScale.x = fRange / (BoundingBox.Point[7].x - BoundingBox.Point[0].x);
	vScale.y = fRange / (BoundingBox.Point[7].y - BoundingBox.Point[0].y);
	vScale.z = fRange / (BoundingBox.Point[7].z - BoundingBox.Point[0].z);

	vOffset.x = -BoundingBox.Point[0].x;
	vOffset.y = -BoundingBox.Point[0].y;
	vOffset.z = -BoundingBox.Point[0].z;

	PVRTMatrixTranslation(mOffset, -fLower, -fLower, -fLower);
	PVRTMatrixScaling(mScale, 1.0f / vScale.x, 1.0f / vScale.y, 1.0f / vScale.z);
	PVRTMatrixMultiply(mesh.mUnpackMatrix, mOffset, mScale);

	PVRTMatrixTranslation(mOffset, -vOffset.x, -vOffset.y, -vOffset.z);
	PVRTMatrixMultiply(mesh.mUnpackMatrix, mesh.mUnpackMatrix, mOffset);

	// Transform vertex data
	for(unsigned int i = 0; i < mesh.nNumVertex; ++i)
	{
		PVRTVertexRead(&v,  mesh.sVertex.pData + i *  mesh.sVertex.nStride,  mesh.sVertex.eType,  mesh.sVertex.n);

		o.x = (v.x + vOffset.x) * vScale.x + fLower;
		o.y = (v.y + vOffset.y) * vScale.y + fLower;
		o.z = (v.z + vOffset.z) * vScale.z + fLower;

		_ASSERT((o.x >= fLower && o.x <= fUpper) || fabs(1.0f - o.x / fLower) < 0.01f || fabs(1.0f - o.x / fUpper) < 0.01f);
		_ASSERT((o.y >= fLower && o.y <= fUpper) || fabs(1.0f - o.y / fLower) < 0.01f || fabs(1.0f - o.y / fUpper) < 0.01f);
		_ASSERT((o.z >= fLower && o.z <= fUpper) || fabs(1.0f - o.z / fLower) < 0.01f || fabs(1.0f - o.z / fUpper) < 0.01f);

#if defined(_DEBUG)
		PVRTVECTOR4 res;
		PVRTTransform(&res, &o, &mesh.mUnpackMatrix);

		_ASSERT(fabs(res.x - v.x) <= 0.02);
		_ASSERT(fabs(res.y - v.y) <= 0.02);
		_ASSERT(fabs(res.z - v.z) <= 0.02);
		_ASSERT(fabs(res.w - 1.0) <= 0.02);
#endif

		PVRTVertexWrite(mesh.sVertex.pData + i * mesh.sVertex.nStride, mesh.sVertex.eType, (int) (mesh.sVertex.n * PVRTModelPODDataTypeComponentCount(mesh.sVertex.eType)), &o);
	}

	// Convert the data to the chosen format
	PVRTModelPODDataConvert(mesh.sVertex, mesh.nNumVertex, eNewType);

	return PVR_SUCCESS;
}
#endif
/*!***************************************************************************
 @Function			PVRTModelPODDataShred
 @Modified			data		Data elements to modify
 @Input				nCnt		Number of elements
 @Input				pChannels	A list of the wanted channels, e.g. {'x', 'y', 0}
 @Description		Reduce the number of dimensions in 'data' using the requested
					channel array. The array should have a maximum length of 4
					or be null terminated if less channels are wanted. It is also
					possible to negate an element, e.g. {'x','y', -'z'}.
*****************************************************************************/
void PVRTModelPODDataShred(CPODData &data, const unsigned int nCnt, const int * pChannels)
{
	CPODData		old;
	PVRTVECTOR4f	v,o;
	float * const pv = &v.x;
	float * const po = &o.x;
	unsigned int	i, nCh;
	int  i32Map[4];
	bool bNegate[4];

	if(!data.pData || !pChannels)
		return;

	old = data;

	// Count the number of output channels while setting up cMap and bNegate
	for(data.n = 0; data.n < 4 && pChannels[data.n]; ++data.n)
	{
		i32Map[data.n]	= abs(pChannels[data.n]) == 'w' ? 3 : abs(pChannels[data.n]) - 'x';
		bNegate[data.n] = pChannels[data.n] < 0;
	}

	if(data.n > old.n)
		data.n = old.n;

	// Allocate output memory
	data.nStride = (unsigned int)PVRTModelPODDataStride(data);

	if(data.nStride == 0)
	{
		FREE(data.pData);
		return;
	}

	data.pData = (unsigned char*)malloc(data.nStride * nCnt);

	for(i = 0; i < nCnt; ++i)
	{
		// Read the vector
		PVRTVertexRead(&v, old.pData + i * old.nStride, old.eType, old.n);

		// Shred the vector
		for(nCh = 0; nCh < 4 && pChannels[nCh]; ++nCh)
			po[nCh] = bNegate[nCh] ? -pv[i32Map[nCh]] : pv[i32Map[nCh]];

		for(; nCh < 4; ++nCh)
			po[nCh] = 0;

		// Write the vector
		PVRTVertexWrite((char*)data.pData + i * data.nStride, data.eType, (int) (data.n * PVRTModelPODDataTypeComponentCount(data.eType)), &o);
	}

	FREE(old.pData);
}

/*!***************************************************************************
 @Function			PVRTModelPODReorderFaces
 @Modified			mesh		The mesh to re-order the faces of
 @Input				i32El1		The first index to be written out
 @Input				i32El2		The second index to be written out
 @Input				i32El3		The third index to be written out
 @Description		Reorders the face indices of a mesh.
*****************************************************************************/
void PVRTModelPODReorderFaces(SPODMesh &mesh, const int i32El1, const int i32El2, const int i32El3)
{
	if(!mesh.sFaces.pData)
		return;

	unsigned int ui32V[3];

	for(unsigned int i = 0; i < mesh.nNumFaces * 3; i += 3)
	{
		unsigned char *pData = mesh.sFaces.pData + i * mesh.sFaces.nStride;

		// Read
		PVRTVertexRead(&ui32V[0], pData, mesh.sFaces.eType);
		PVRTVertexRead(&ui32V[1], pData + mesh.sFaces.nStride, mesh.sFaces.eType);
		PVRTVertexRead(&ui32V[2], pData + 2 * mesh.sFaces.nStride, mesh.sFaces.eType);

		// Write in place the new order
		PVRTVertexWrite(pData, mesh.sFaces.eType, ui32V[i32El1]);
		PVRTVertexWrite(pData + mesh.sFaces.nStride, mesh.sFaces.eType, ui32V[i32El2]);
		PVRTVertexWrite(pData + 2 * mesh.sFaces.nStride, mesh.sFaces.eType, ui32V[i32El3]);
	}
}

/*!***************************************************************************
 @Function			InterleaveArray
 @Modified			pInterleaved
 @Modified			data
 @Input				nNumVertex
 @Input				nStride
 @Input				nPadding
 @Input				nOffset
 @Description		Interleaves the pod data
*****************************************************************************/
static void InterleaveArray(
	char			* const pInterleaved,
	CPODData		&data,
	const PVRTuint32 nNumVertex,
	const PVRTuint32 nStride,
	const PVRTuint32 nPadding,
	PVRTuint32		&nOffset)
{
	if(!data.nStride)
		return;

	for(PVRTuint32 i = 0; i < nNumVertex; ++i)
		memcpy(pInterleaved + i * nStride + nOffset, (char*)data.pData + i * data.nStride, data.nStride);

	FREE(data.pData);
	data.pData		= (unsigned char*)nOffset;
	data.nStride	= nStride;
	nOffset			+= PVRTModelPODDataStride(data) + nPadding;
}

/*!***************************************************************************
 @Function			DeinterleaveArray
 @Input				data
 @Input				pInter
 @Input				nNumVertex
 @Description		DeInterleaves the pod data
*****************************************************************************/
static void DeinterleaveArray(
	CPODData			&data,
	const void			* const pInter,
	const PVRTuint32	nNumVertex,
	const PVRTuint32	nAlignToNBytes)
{
	const PVRTuint32 nSrcStride	= data.nStride;
	const PVRTuint32 nDestStride= PVRTModelPODDataStride(data);
	const PVRTuint32 nAlignedStride = nDestStride + ((nAlignToNBytes - nDestStride % nAlignToNBytes) % nAlignToNBytes);
	const char		*pSrc		= (char*)pInter + (size_t)data.pData;

	if(!nSrcStride)
		return;

	data.pData = 0;
	SafeAlloc(data.pData, nAlignedStride * nNumVertex);
	data.nStride = nAlignedStride;

	for(PVRTuint32 i = 0; i < nNumVertex; ++i)
		memcpy((char*)data.pData + i * nAlignedStride, pSrc + i * nSrcStride, nDestStride);
}

/*!***************************************************************************
 @Function		PVRTModelPODToggleInterleaved
 @Modified		mesh		Mesh to modify
 @Input			ui32AlignToNBytes Align the interleaved data to this no. of bytes.
 @Description	Switches the supplied mesh to or from interleaved data format.
*****************************************************************************/
void PVRTModelPODToggleInterleaved(SPODMesh &mesh, const PVRTuint32 ui32AlignToNBytes)
{
	unsigned int i;

	if(!mesh.nNumVertex)
		return;

	if(mesh.pInterleaved)
	{
		/*
			De-interleave
		*/
		DeinterleaveArray(mesh.sVertex, mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);
		DeinterleaveArray(mesh.sNormals, mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);
		DeinterleaveArray(mesh.sTangents, mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);
		DeinterleaveArray(mesh.sBinormals, mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);

		for(i = 0; i < mesh.nNumUVW; ++i)
			DeinterleaveArray(mesh.psUVW[i], mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);

		DeinterleaveArray(mesh.sVtxColours, mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);
		DeinterleaveArray(mesh.sBoneIdx, mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);
		DeinterleaveArray(mesh.sBoneWeight, mesh.pInterleaved, mesh.nNumVertex, ui32AlignToNBytes);
		FREE(mesh.pInterleaved);
	}
	else
	{
		PVRTuint32 nStride, nOffset, nBytes;

#define NEEDED_PADDING(x) ((x && ui32AlignToNBytes) ? (ui32AlignToNBytes - x % ui32AlignToNBytes) % ui32AlignToNBytes : 0) 

		// Interleave

		PVRTuint32 nVertexStride, nNormalStride, nTangentStride, nBinormalStride, nVtxColourStride, nBoneIdxStride, nBoneWeightStride;
		PVRTuint32 nUVWStride[8];
		PVRTuint32 nVertexPadding, nNormalPadding, nTangentPadding, nBinormalPadding, nVtxColourPadding, nBoneIdxPadding, nBoneWeightPadding;
		PVRTuint32 nUVWPadding[8];

		_ASSERT(mesh.nNumUVW < 8);

		nStride  = nVertexStride = PVRTModelPODDataStride(mesh.sVertex);
		nStride += nVertexPadding = NEEDED_PADDING(nVertexStride);

		nStride += nNormalStride = PVRTModelPODDataStride(mesh.sNormals);
		nStride += nNormalPadding = NEEDED_PADDING(nNormalStride);

		nStride += nTangentStride = PVRTModelPODDataStride(mesh.sTangents);
		nStride += nTangentPadding = NEEDED_PADDING(nTangentStride);

		nStride += nBinormalStride = PVRTModelPODDataStride(mesh.sBinormals);
		nStride += nBinormalPadding = NEEDED_PADDING(nBinormalStride);

		for(i = 0; i < mesh.nNumUVW; ++i)
		{
			nStride += nUVWStride[i] = PVRTModelPODDataStride(mesh.psUVW[i]);
			nStride += nUVWPadding[i] = NEEDED_PADDING(nUVWStride[i]);
		}

		nStride += nVtxColourStride = PVRTModelPODDataStride(mesh.sVtxColours);
		nStride += nVtxColourPadding = NEEDED_PADDING(nVtxColourStride);

		nStride += nBoneIdxStride = PVRTModelPODDataStride(mesh.sBoneIdx);
		nStride += nBoneIdxPadding = NEEDED_PADDING(nBoneIdxStride);

		nStride += nBoneWeightStride = PVRTModelPODDataStride(mesh.sBoneWeight);
		nStride += nBoneWeightPadding = NEEDED_PADDING(nBoneWeightStride);

#undef NEEDED_PADDING
		// Allocate interleaved array
		SafeAlloc(mesh.pInterleaved, mesh.nNumVertex * nStride);

		// Interleave the data
		nOffset = 0;

		for(nBytes = 4; nBytes > 0; nBytes >>= 1)
		{
			if(PVRTModelPODDataTypeSize(mesh.sVertex.eType) == nBytes)
				InterleaveArray((char*)mesh.pInterleaved, mesh.sVertex, mesh.nNumVertex, nStride, nVertexPadding, nOffset);

			if(PVRTModelPODDataTypeSize(mesh.sNormals.eType) == nBytes)
				InterleaveArray((char*)mesh.pInterleaved, mesh.sNormals, mesh.nNumVertex, nStride, nNormalPadding, nOffset);

			if(PVRTModelPODDataTypeSize(mesh.sTangents.eType) == nBytes)
				InterleaveArray((char*)mesh.pInterleaved, mesh.sTangents, mesh.nNumVertex, nStride, nTangentPadding, nOffset);

			if(PVRTModelPODDataTypeSize(mesh.sBinormals.eType) == nBytes)
				InterleaveArray((char*)mesh.pInterleaved, mesh.sBinormals, mesh.nNumVertex, nStride, nBinormalPadding, nOffset);

			if(PVRTModelPODDataTypeSize(mesh.sVtxColours.eType) == nBytes)
				InterleaveArray((char*)mesh.pInterleaved, mesh.sVtxColours, mesh.nNumVertex, nStride, nVtxColourPadding, nOffset);

			for(i = 0; i < mesh.nNumUVW; ++i)
			{
				if(PVRTModelPODDataTypeSize(mesh.psUVW[i].eType) == nBytes)
					InterleaveArray((char*)mesh.pInterleaved, mesh.psUVW[i], mesh.nNumVertex, nStride, nUVWPadding[i], nOffset);
			}

			if(PVRTModelPODDataTypeSize(mesh.sBoneIdx.eType) == nBytes)
				InterleaveArray((char*)mesh.pInterleaved, mesh.sBoneIdx, mesh.nNumVertex, nStride, nBoneIdxPadding, nOffset);

			if(PVRTModelPODDataTypeSize(mesh.sBoneWeight.eType) == nBytes)
				InterleaveArray((char*)mesh.pInterleaved, mesh.sBoneWeight, mesh.nNumVertex, nStride, nBoneWeightPadding, nOffset);
		}
	}
}

/*!***************************************************************************
 @Function			PVRTModelPODDeIndex
 @Modified			mesh		Mesh to modify
 @Description		De-indexes the supplied mesh. The mesh must be
					Interleaved before calling this function.
*****************************************************************************/
void PVRTModelPODDeIndex(SPODMesh &mesh)
{
	unsigned char *pNew = 0;

	if(!mesh.pInterleaved || !mesh.nNumVertex)
		return;

	_ASSERT(mesh.nNumVertex && mesh.nNumFaces);

	// Create a new vertex list
	mesh.nNumVertex = PVRTModelPODCountIndices(mesh);
	SafeAlloc(pNew, mesh.sVertex.nStride * mesh.nNumVertex);

	// Deindex the vertices
	if(mesh.sFaces.eType == EPODDataUnsignedShort)
	{
		for(unsigned int i = 0; i < mesh.nNumVertex; ++i)
			memcpy(pNew + i * mesh.sVertex.nStride, (char*)mesh.pInterleaved + ((unsigned short*)mesh.sFaces.pData)[i] * mesh.sVertex.nStride, mesh.sVertex.nStride);
	}
	else
	{
		_ASSERT(mesh.sFaces.eType == EPODDataUnsignedInt);

		for(unsigned int i = 0; i < mesh.nNumVertex; ++i)
			memcpy(pNew + i * mesh.sVertex.nStride, (char*)mesh.pInterleaved + ((unsigned int*)mesh.sFaces.pData)[i] * mesh.sVertex.nStride, mesh.sVertex.nStride);
	}

	// Replace the old vertex list
	FREE(mesh.pInterleaved);
	mesh.pInterleaved = pNew;

	// Get rid of the index list
	FREE(mesh.sFaces.pData);
	mesh.sFaces.n		= 0;
	mesh.sFaces.nStride	= 0;
}

/*!***************************************************************************
 @Function			PVRTModelPODToggleStrips
 @Modified			mesh		Mesh to modify
 @Description		Converts the supplied mesh to or from strips.
*****************************************************************************/
void PVRTModelPODToggleStrips(SPODMesh &mesh)
{
	CPODData	old;
	size_t	nIdxSize, nTriStride;

	if(!mesh.nNumFaces)
		return;

	_ASSERT(mesh.sFaces.n == 1);
	nIdxSize	= PVRTModelPODDataTypeSize(mesh.sFaces.eType);
	nTriStride	= PVRTModelPODDataStride(mesh.sFaces) * 3;

	old					= mesh.sFaces;
	mesh.sFaces.pData	= 0;
	SafeAlloc(mesh.sFaces.pData, nTriStride * mesh.nNumFaces);

	if(mesh.nNumStrips)
	{
		unsigned int nListIdxCnt, nStripIdxCnt;

		//	Convert to list
		nListIdxCnt		= 0;
		nStripIdxCnt	= 0;

		for(unsigned int i = 0; i < mesh.nNumStrips; ++i)
		{
			for(unsigned int j = 0; j < mesh.pnStripLength[i]; ++j)
			{
				if(j)
				{
					_ASSERT(j == 1); // Because this will surely break with any other number

					memcpy(
						(char*)mesh.sFaces.pData	+ nIdxSize * nListIdxCnt,
						(char*)old.pData			+ nIdxSize * (nStripIdxCnt - 1),
						nIdxSize);
					nListIdxCnt += 1;

					memcpy(
						(char*)mesh.sFaces.pData	+ nIdxSize * nListIdxCnt,
						(char*)old.pData			+ nIdxSize * (nStripIdxCnt - 2),
						nIdxSize);
					nListIdxCnt += 1;

					memcpy(
						(char*)mesh.sFaces.pData	+ nIdxSize * nListIdxCnt,
						(char*)old.pData			+ nIdxSize * nStripIdxCnt,
						nIdxSize);
					nListIdxCnt += 1;

					nStripIdxCnt += 1;
				}
				else
				{
					memcpy(
						(char*)mesh.sFaces.pData	+ nIdxSize * nListIdxCnt,
						(char*)old.pData			+ nIdxSize * nStripIdxCnt,
						nTriStride);

					nStripIdxCnt += 3;
					nListIdxCnt += 3;
				}
			}
		}

		_ASSERT(nListIdxCnt == mesh.nNumFaces*3);
		FREE(mesh.pnStripLength);
		mesh.nNumStrips = 0;
	}
	else
	{
		int		nIdxCnt;
		int		nBatchCnt;
		unsigned int n0, n1, n2;
		unsigned int p0, p1, p2, nFaces;
		unsigned char* pFaces;

		//	Convert to strips
		mesh.pnStripLength	= (unsigned int*)calloc(mesh.nNumFaces, sizeof(*mesh.pnStripLength));
		mesh.nNumStrips		= 0;
		nIdxCnt				= 0;
		nBatchCnt			= mesh.sBoneBatches.nBatchCnt ? mesh.sBoneBatches.nBatchCnt : 1;

		for(int h = 0; h < nBatchCnt; ++h)
		{
			n0 = 0;
			n1 = 0;
			n2 = 0;

			if(!mesh.sBoneBatches.nBatchCnt)
			{
				nFaces = mesh.nNumFaces;
				pFaces = old.pData;
			}
			else
			{
				if(h + 1 < mesh.sBoneBatches.nBatchCnt)
					nFaces = mesh.sBoneBatches.pnBatchOffset[h+1] - mesh.sBoneBatches.pnBatchOffset[h];
				else
					nFaces = mesh.nNumFaces - mesh.sBoneBatches.pnBatchOffset[h];

				pFaces = &old.pData[3 * mesh.sBoneBatches.pnBatchOffset[h] * old.nStride];
			}

			for(unsigned int i = 0; i < nFaces; ++i)
			{
				p0 = n0;
				p1 = n1;
				p2 = n2;

				PVRTVertexRead(&n0, (char*)pFaces + (3 * i + 0) * old.nStride, old.eType);
				PVRTVertexRead(&n1, (char*)pFaces + (3 * i + 1) * old.nStride, old.eType);
				PVRTVertexRead(&n2, (char*)pFaces + (3 * i + 2) * old.nStride, old.eType);

				if(mesh.pnStripLength[mesh.nNumStrips])
				{
					if(mesh.pnStripLength[mesh.nNumStrips] & 0x01)
					{
						if(p1 == n1 && p2 == n0)
						{
							PVRTVertexWrite((char*)mesh.sFaces.pData + nIdxCnt * mesh.sFaces.nStride, mesh.sFaces.eType, n2);
							++nIdxCnt;
							mesh.pnStripLength[mesh.nNumStrips] += 1;
							continue;
						}
					}
					else
					{
						if(p2 == n1 && p0 == n0)
						{
							PVRTVertexWrite((char*)mesh.sFaces.pData + nIdxCnt * mesh.sFaces.nStride, mesh.sFaces.eType, n2);
							++nIdxCnt;
							mesh.pnStripLength[mesh.nNumStrips] += 1;
							continue;
						}
					}

					++mesh.nNumStrips;
				}

				//	Start of strip, copy entire triangle
				PVRTVertexWrite((char*)mesh.sFaces.pData + nIdxCnt * mesh.sFaces.nStride, mesh.sFaces.eType, n0);
				++nIdxCnt;
				PVRTVertexWrite((char*)mesh.sFaces.pData + nIdxCnt * mesh.sFaces.nStride, mesh.sFaces.eType, n1);
				++nIdxCnt;
				PVRTVertexWrite((char*)mesh.sFaces.pData + nIdxCnt * mesh.sFaces.nStride, mesh.sFaces.eType, n2);
				++nIdxCnt;

				mesh.pnStripLength[mesh.nNumStrips] += 1;
			}
		}

		if(mesh.pnStripLength[mesh.nNumStrips])
			++mesh.nNumStrips;

		SafeRealloc(mesh.sFaces.pData, nIdxCnt * nIdxSize);
		mesh.pnStripLength	= (unsigned int*)realloc(mesh.pnStripLength, sizeof(*mesh.pnStripLength) * mesh.nNumStrips);
	}

	FREE(old.pData);
}

/*!***************************************************************************
 @Function		PVRTModelPODCountIndices
 @Input			mesh		Mesh
 @Return		Number of indices used by mesh
 @Description	Counts the number of indices of a mesh
*****************************************************************************/
unsigned int PVRTModelPODCountIndices(const SPODMesh &mesh)
{
	return mesh.nNumStrips ? mesh.nNumFaces + (mesh.nNumStrips * 2) : mesh.nNumFaces * 3;
}

/*!***************************************************************************
 @Function			PVRTModelPODCopyCPODData
 @Input				in
 @Output			out
 @Input				ui32No
 @Input				bInterleaved
 @Description		Used to copy a CPODData of a mesh
*****************************************************************************/
void PVRTModelPODCopyCPODData(const CPODData &in, CPODData &out, unsigned int ui32No, bool bInterleaved)
{
	FREE(out.pData);

	out.eType	= in.eType;
	out.n		= in.n;
	out.nStride = in.nStride;

	if(bInterleaved)
	{
		out.pData = in.pData;
	}
	else if(in.pData)
	{
		size_t ui32Size = PVRTModelPODDataStride(out) * ui32No;

		if(SafeAlloc(out.pData, ui32Size))
			memcpy(out.pData, in.pData, ui32Size);
	}
}

/*!***************************************************************************
 @Function			PVRTModelPODCopyNode
 @Input				in
 @Output			out
 @Input				nNumFrames
 @Description		Used to copy a pod node
*****************************************************************************/
void PVRTModelPODCopyNode(const SPODNode &in, SPODNode &out, int nNumFrames)
{
	out.nIdx = in.nIdx;
	out.nIdxMaterial = in.nIdxMaterial;
	out.nIdxParent = in.nIdxParent;
	out.nAnimFlags = in.nAnimFlags;
	out.pUserData = 0;
	out.nUserDataSize = 0;

	if(in.pszName && SafeAlloc(out.pszName, strlen(in.pszName) + 1))
		memcpy(out.pszName, in.pszName, strlen(in.pszName) + 1);

	int i32Size;

	// Position
	i32Size = in.nAnimFlags & ePODHasPositionAni ? PVRTModelPODGetAnimArraySize(in.pnAnimPositionIdx, nNumFrames, 3) : 3;

	if(in.pnAnimPositionIdx && SafeAlloc(out.pnAnimPositionIdx, nNumFrames))
		memcpy(out.pnAnimPositionIdx, in.pnAnimPositionIdx, sizeof(*out.pnAnimPositionIdx) * nNumFrames);

	if(in.pfAnimPosition && SafeAlloc(out.pfAnimPosition, i32Size))
		memcpy(out.pfAnimPosition, in.pfAnimPosition, sizeof(*out.pfAnimPosition) * i32Size);

	// Rotation
	i32Size = in.nAnimFlags & ePODHasRotationAni ? PVRTModelPODGetAnimArraySize(in.pnAnimRotationIdx, nNumFrames, 4) : 4;

	if(in.pnAnimRotationIdx && SafeAlloc(out.pnAnimRotationIdx, nNumFrames))
		memcpy(out.pnAnimRotationIdx, in.pnAnimRotationIdx, sizeof(*out.pnAnimRotationIdx) * nNumFrames);

	if(in.pfAnimRotation && SafeAlloc(out.pfAnimRotation, i32Size))
		memcpy(out.pfAnimRotation, in.pfAnimRotation, sizeof(*out.pfAnimRotation) * i32Size);

	// Scale
	i32Size = in.nAnimFlags & ePODHasScaleAni ? PVRTModelPODGetAnimArraySize(in.pnAnimScaleIdx, nNumFrames, 7) : 7;

	if(in.pnAnimScaleIdx && SafeAlloc(out.pnAnimScaleIdx, nNumFrames))
		memcpy(out.pnAnimScaleIdx, in.pnAnimScaleIdx, sizeof(*out.pnAnimScaleIdx) * nNumFrames);

	if(in.pfAnimScale && SafeAlloc(out.pfAnimScale, i32Size))
		memcpy(out.pfAnimScale, in.pfAnimScale, sizeof(*out.pfAnimScale) * i32Size);

	// Matrix
	i32Size = in.nAnimFlags & ePODHasMatrixAni ? PVRTModelPODGetAnimArraySize(in.pnAnimMatrixIdx, nNumFrames, 16) : 16;

	if(in.pnAnimMatrixIdx && SafeAlloc(out.pnAnimMatrixIdx, nNumFrames))
		memcpy(out.pnAnimMatrixIdx, in.pnAnimMatrixIdx, sizeof(*out.pnAnimMatrixIdx) * nNumFrames);

	if(in.pfAnimMatrix && SafeAlloc(out.pfAnimMatrix, i32Size))
		memcpy(out.pfAnimMatrix, in.pfAnimMatrix, sizeof(*out.pfAnimMatrix) * i32Size);

	if(in.pUserData && SafeAlloc(out.pUserData, in.nUserDataSize))
	{
		memcpy(out.pUserData, in.pUserData, in.nUserDataSize);
		out.nUserDataSize = in.nUserDataSize;
	}
}

/*!***************************************************************************
 @Function			PVRTModelPODCopyMesh
 @Input				in
 @Output			out
 @Description		Used to copy a pod mesh
*****************************************************************************/
void PVRTModelPODCopyMesh(const SPODMesh &in, SPODMesh &out)
{
	unsigned int i;
	bool bInterleaved = in.pInterleaved != 0;
	out.nNumVertex = in.nNumVertex;
	out.nNumFaces  = in.nNumFaces;

	// Face data
	PVRTModelPODCopyCPODData(in.sFaces	 , out.sFaces	 , out.nNumFaces * 3, false);

	// Vertex data
	PVRTModelPODCopyCPODData(in.sVertex	 , out.sVertex	 , out.nNumVertex, bInterleaved);
	PVRTModelPODCopyCPODData(in.sNormals	 , out.sNormals	 , out.nNumVertex, bInterleaved);
	PVRTModelPODCopyCPODData(in.sTangents	 , out.sTangents	 , out.nNumVertex, bInterleaved);
	PVRTModelPODCopyCPODData(in.sBinormals , out.sBinormals , out.nNumVertex, bInterleaved);
	PVRTModelPODCopyCPODData(in.sVtxColours, out.sVtxColours, out.nNumVertex, bInterleaved);
	PVRTModelPODCopyCPODData(in.sBoneIdx	 , out.sBoneIdx	 , out.nNumVertex, bInterleaved);
	PVRTModelPODCopyCPODData(in.sBoneWeight, out.sBoneWeight, out.nNumVertex, bInterleaved);

	if(in.nNumUVW && SafeAlloc(out.psUVW, in.nNumUVW))
	{
		out.nNumUVW = in.nNumUVW;

		for(i = 0; i < out.nNumUVW; ++i)
		{
			PVRTModelPODCopyCPODData(in.psUVW[i], out.psUVW[i], out.nNumVertex, bInterleaved);
		}
	}

	// Allocate and copy interleaved array
	if(bInterleaved && SafeAlloc(out.pInterleaved, out.nNumVertex * in.sVertex.nStride))
		memcpy(out.pInterleaved, in.pInterleaved, out.nNumVertex * in.sVertex.nStride);

	if(in.pnStripLength && SafeAlloc(out.pnStripLength, out.nNumFaces))
	{
		memcpy(out.pnStripLength, in.pnStripLength, sizeof(*out.pnStripLength) * out.nNumFaces);
		out.nNumStrips = in.nNumStrips;
	}

	if(in.sBoneBatches.nBatchCnt)
	{
		out.sBoneBatches.Release();

		out.sBoneBatches.nBatchBoneMax = in.sBoneBatches.nBatchBoneMax;
		out.sBoneBatches.nBatchCnt     = in.sBoneBatches.nBatchCnt;

		if(in.sBoneBatches.pnBatches)
		{
			out.sBoneBatches.pnBatches = (int*) malloc(out.sBoneBatches.nBatchCnt * out.sBoneBatches.nBatchBoneMax * sizeof(*out.sBoneBatches.pnBatches));

			if(out.sBoneBatches.pnBatches)
				memcpy(out.sBoneBatches.pnBatches, in.sBoneBatches.pnBatches, out.sBoneBatches.nBatchCnt * out.sBoneBatches.nBatchBoneMax * sizeof(*out.sBoneBatches.pnBatches));
		}

		if(in.sBoneBatches.pnBatchBoneCnt)
		{
			out.sBoneBatches.pnBatchBoneCnt = (int*) malloc(out.sBoneBatches.nBatchCnt * sizeof(*out.sBoneBatches.pnBatchBoneCnt));

			if(out.sBoneBatches.pnBatchBoneCnt)
				memcpy(out.sBoneBatches.pnBatchBoneCnt, in.sBoneBatches.pnBatchBoneCnt, out.sBoneBatches.nBatchCnt * sizeof(*out.sBoneBatches.pnBatchBoneCnt));
		}

		if(in.sBoneBatches.pnBatchOffset)
		{
			out.sBoneBatches.pnBatchOffset = (int*) malloc(out.sBoneBatches.nBatchCnt * sizeof(out.sBoneBatches.pnBatchOffset));

			if(out.sBoneBatches.pnBatchOffset)
				memcpy(out.sBoneBatches.pnBatchOffset, in.sBoneBatches.pnBatchOffset, out.sBoneBatches.nBatchCnt * sizeof(*out.sBoneBatches.pnBatchOffset));
		}
	}

	memcpy(out.mUnpackMatrix.f, in.mUnpackMatrix.f, sizeof(in.mUnpackMatrix.f[0]) * 16);

	out.ePrimitiveType = in.ePrimitiveType;
}

/*!***************************************************************************
 @Function			PVRTModelPODCopyTexture
 @Input				in
 @Output			out
 @Description		Used to copy a pod texture
*****************************************************************************/
void PVRTModelPODCopyTexture(const SPODTexture &in, SPODTexture &out)
{
	if(in.pszName && SafeAlloc(out.pszName, strlen(in.pszName) + 1))
		memcpy(out.pszName, in.pszName, strlen(in.pszName) + 1);
}

/*!***************************************************************************
 @Function			PVRTModelPODCopyMaterial
 @Input				in
 @Output			out
 @Description		Used to copy a pod material
*****************************************************************************/
void PVRTModelPODCopyMaterial(const SPODMaterial &in, SPODMaterial &out)
{
	memcpy(&out, &in, sizeof(SPODMaterial));

	out.pszName = 0;
	out.pszEffectFile = 0;
	out.pszEffectName = 0;
	out.pUserData = 0;
	out.nUserDataSize = 0;

	if(in.pszName && SafeAlloc(out.pszName, strlen(in.pszName) + 1))
		memcpy(out.pszName, in.pszName, strlen(in.pszName) + 1);

	if(in.pszEffectFile && SafeAlloc(out.pszEffectFile, strlen(in.pszEffectFile) + 1))
		memcpy(out.pszEffectFile, in.pszEffectFile, strlen(in.pszEffectFile) + 1);

	if(in.pszEffectName && SafeAlloc(out.pszEffectName, strlen(in.pszEffectName) + 1))
		memcpy(out.pszEffectName, in.pszEffectName, strlen(in.pszEffectName) + 1);

	if(in.pUserData && SafeAlloc(out.pUserData, in.nUserDataSize))
	{
		memcpy(out.pUserData, in.pUserData, in.nUserDataSize);
		out.nUserDataSize = in.nUserDataSize;
	}
}

/*!***************************************************************************
 @Function			PVRTModelPODCopyCamera
 @Input				in
 @Output			out
 @Input				nNumFrames The number of animation frames
 @Description		Used to copy a pod camera
*****************************************************************************/
void PVRTModelPODCopyCamera(const SPODCamera &in, SPODCamera &out, int nNumFrames)
{
	memcpy(&out, &in, sizeof(SPODCamera));

	out.pfAnimFOV = 0;

	if(in.pfAnimFOV && SafeAlloc(out.pfAnimFOV, nNumFrames))
		memcpy(out.pfAnimFOV, in.pfAnimFOV, sizeof(*out.pfAnimFOV) * nNumFrames);
}

/*!***************************************************************************
 @Function			PVRTModelPODCopyLight
 @Input				in
 @Output			out
 @Description		Used to copy a pod light
*****************************************************************************/
void PVRTModelPODCopyLight(const SPODLight &in, SPODLight &out)
{
	memcpy(&out, &in, sizeof(SPODLight));
}

/*!***************************************************************************
 @Function			TransformCPODData
 @Input				in
 @Output			out
 @Input				idx Value to transform
 @Input				pPalette Palette of matrices to transform with
 @Input				pBoneIdx Array of indices into pPalette
 @Input				pBoneWeight Array of weights to weight the influence of the matrices of pPalette with
 @Input				i32BoneCnt Size of pBoneIdx and pBoneWeight
 @Description		Used to transform a particular value in a CPODData
*****************************************************************************/
inline void TransformCPODData(CPODData &in, CPODData &out, int idx, PVRTMATRIX *pPalette, float *pBoneIdx, float *pBoneW, int i32BoneCnt, bool bNormalise)
{
	PVRTVECTOR4f fResult, fOrig, fTmp;

	if(in.n)
	{

		PVRTVertexRead(&fOrig, in.pData + (idx * in.nStride), in.eType, in.n);

		memset(&fResult.x, 0, sizeof(fResult));

		if(i32BoneCnt)
		{
			for(int i = 0; i < i32BoneCnt; ++i)
			{
				int i32BoneIdx = (int) pBoneIdx[i];
				fTmp.x = vt2f(pPalette[i32BoneIdx].f[0]) * fOrig.x + vt2f(pPalette[i32BoneIdx].f[4]) * fOrig.y + vt2f(pPalette[i32BoneIdx].f[8]) * fOrig.z + vt2f(pPalette[i32BoneIdx].f[12]) * fOrig.w;
				fTmp.y = vt2f(pPalette[i32BoneIdx].f[1]) * fOrig.x + vt2f(pPalette[i32BoneIdx].f[5]) * fOrig.y + vt2f(pPalette[i32BoneIdx].f[9]) * fOrig.z + vt2f(pPalette[i32BoneIdx].f[13]) * fOrig.w;
				fTmp.z = vt2f(pPalette[i32BoneIdx].f[2]) * fOrig.x + vt2f(pPalette[i32BoneIdx].f[6]) * fOrig.y + vt2f(pPalette[i32BoneIdx].f[10])* fOrig.z + vt2f(pPalette[i32BoneIdx].f[14]) * fOrig.w;
				fTmp.w = vt2f(pPalette[i32BoneIdx].f[3]) * fOrig.x + vt2f(pPalette[i32BoneIdx].f[7]) * fOrig.y + vt2f(pPalette[i32BoneIdx].f[11])* fOrig.z + vt2f(pPalette[i32BoneIdx].f[15]) * fOrig.w;

				fResult.x += fTmp.x * pBoneW[i];
				fResult.y += fTmp.y * pBoneW[i];
				fResult.z += fTmp.z * pBoneW[i];
				fResult.w += fTmp.w * pBoneW[i];
			}
		}
		else
		{
			fResult.x = vt2f(pPalette[0].f[0]) * fOrig.x + vt2f(pPalette[0].f[4]) * fOrig.y + vt2f(pPalette[0].f[8]) * fOrig.z + vt2f(pPalette[0].f[12]) * fOrig.w;
			fResult.y = vt2f(pPalette[0].f[1]) * fOrig.x + vt2f(pPalette[0].f[5]) * fOrig.y + vt2f(pPalette[0].f[9]) * fOrig.z + vt2f(pPalette[0].f[13]) * fOrig.w;
			fResult.z = vt2f(pPalette[0].f[2]) * fOrig.x + vt2f(pPalette[0].f[6]) * fOrig.y + vt2f(pPalette[0].f[10])* fOrig.z + vt2f(pPalette[0].f[14]) * fOrig.w;
			fResult.w = vt2f(pPalette[0].f[3]) * fOrig.x + vt2f(pPalette[0].f[7]) * fOrig.y + vt2f(pPalette[0].f[11])* fOrig.z + vt2f(pPalette[0].f[15]) * fOrig.w;
		}

		if(bNormalise)
		{
			double temp = (double)(fResult.x * fResult.x + fResult.y * fResult.y + fResult.z * fResult.z);
			temp = 1.0 / sqrt(temp);
			float f = (float)temp;

			fResult.x = fResult.x * f;
			fResult.y = fResult.y * f;
			fResult.z = fResult.z * f;
		}

		PVRTVertexWrite(out.pData + (idx * out.nStride), out.eType, in.n, &fResult);
	}
}
/*!***************************************************************************
 @Function			PVRTModelPODFlattenToWorldSpace
 @Input				in - Source scene. All meshes must not be interleaved.
 @Output			out
 @Description		Used to flatten a pod scene to world space. All animation
					and skinning information will be removed. The returned
					position, normal, binormals and tangent data if present
					will be returned as floats regardless of the input data
					type.
*****************************************************************************/
EPVRTError PVRTModelPODFlattenToWorldSpace(CPVRTModelPOD &in, CPVRTModelPOD &out)
{
	unsigned int i, j, k, l;
	PVRTMATRIX mWorld;

	// Destroy the out pod scene to make sure it is clean
	out.Destroy();

	// Init mesh and node arrays
	SafeAlloc(out.pNode, in.nNumNode);
	SafeAlloc(out.pMesh, in.nNumMeshNode);

	out.nNumNode = in.nNumNode;
	out.nNumMesh = out.nNumMeshNode = in.nNumMeshNode;

	// Init scene values
	out.nNumFrame = 0;
	out.nFlags = in.nFlags;
	out.fUnits = in.fUnits;

	for(i = 0; i < 3; ++i)
	{
		out.pfColourBackground[i] = in.pfColourBackground[i];
		out.pfColourAmbient[i]	  = in.pfColourAmbient[i];
	}

	// flatten meshes to world space
	for(i = 0; i < in.nNumMeshNode; ++i)
	{


		SPODNode& inNode  = in.pNode[i];
		SPODNode& outNode = out.pNode[i];

		// Get the meshes
		SPODMesh& inMesh  = in.pMesh[inNode.nIdx];
		SPODMesh& outMesh = out.pMesh[i];

		if(inMesh.pInterleaved != 0) // This function requires all the meshes to be de-interleaved
		{
			_ASSERT(inMesh.pInterleaved == 0);
			out.Destroy(); // Destroy the out pod scene
			return PVR_FAIL;
		}

		// Copy the node
		PVRTModelPODCopyNode(inNode, outNode, in.nNumFrame);

		// Strip out animation and parenting
		outNode.nIdxParent = -1;

		outNode.nAnimFlags = 0;
		FREE(outNode.pfAnimMatrix);
		FREE(outNode.pfAnimPosition);
		FREE(outNode.pfAnimRotation);
		FREE(outNode.pfAnimScale);

		// Update the mesh ID. The rest of the IDs should remain correct
		outNode.nIdx = i;

		// Copy the mesh
		PVRTModelPODCopyMesh(inMesh, outMesh);

		// Strip out skinning information as that is no longer needed
		outMesh.sBoneBatches.Release();
		outMesh.sBoneIdx.Reset();
		outMesh.sBoneWeight.Reset();

		// Set the data type to float and resize the arrays as this function outputs transformed data as float only
		if(inMesh.sVertex.n)
		{
			outMesh.sVertex.eType = EPODDataFloat;
			outMesh.sVertex.pData = (unsigned char*) realloc(outMesh.sVertex.pData, PVRTModelPODDataStride(outMesh.sVertex) * inMesh.nNumVertex);
		}

		if(inMesh.sNormals.n)
		{
			outMesh.sNormals.eType = EPODDataFloat;
			outMesh.sNormals.pData = (unsigned char*) realloc(outMesh.sNormals.pData, PVRTModelPODDataStride(outMesh.sNormals) * inMesh.nNumVertex);
		}

		if(inMesh.sTangents.n)
		{
			outMesh.sTangents.eType = EPODDataFloat;
			outMesh.sTangents.pData = (unsigned char*) realloc(outMesh.sTangents.pData, PVRTModelPODDataStride(outMesh.sTangents) * inMesh.nNumVertex);
		}

		if(inMesh.sBinormals.n)
		{
			outMesh.sBinormals.eType = EPODDataFloat;
			outMesh.sBinormals.pData = (unsigned char*) realloc(outMesh.sBinormals.pData, PVRTModelPODDataStride(outMesh.sBinormals) * inMesh.nNumVertex);
		}

		if(inMesh.sBoneBatches.nBatchCnt)
		{
			unsigned int ui32BatchPaletteSize   = 0;
			PVRTMATRIX *pPalette = 0;
			PVRTMATRIX *pPaletteInvTrans = 0;
			unsigned int ui32Offset = 0, ui32Strip = 0;
			bool *pbTransformed = 0;

			SafeAlloc(pPalette, inMesh.sBoneBatches.nBatchBoneMax);
			SafeAlloc(pPaletteInvTrans, inMesh.sBoneBatches.nBatchBoneMax);
			SafeAlloc(pbTransformed, inMesh.nNumVertex);

			for(j = 0; j < (unsigned int) inMesh.sBoneBatches.nBatchCnt; ++j)
			{
				ui32BatchPaletteSize = (unsigned int) inMesh.sBoneBatches.pnBatchBoneCnt[j];

				for(k = 0; k < ui32BatchPaletteSize; ++k)
				{
					// Get the Node of the bone
					int i32NodeID = inMesh.sBoneBatches.pnBatches[j * inMesh.sBoneBatches.nBatchBoneMax + k];

					// Get the World transformation matrix for this bone
					in.GetBoneWorldMatrix(pPalette[k], inNode, in.pNode[i32NodeID]);

					// Get the inverse transpose of the 3x3
					if(inMesh.sNormals.n || inMesh.sTangents.n || inMesh.sBinormals.n)
					{
						pPaletteInvTrans[k] = pPalette[k];
						pPaletteInvTrans[k].f[3]  = pPaletteInvTrans[k].f[7]  = pPaletteInvTrans[k].f[11] = 0;
						pPaletteInvTrans[k].f[12] = pPaletteInvTrans[k].f[13] = pPaletteInvTrans[k].f[14] = 0;
						PVRTMatrixInverse(pPaletteInvTrans[k], pPaletteInvTrans[k]);
						PVRTMatrixTranspose(pPaletteInvTrans[k], pPaletteInvTrans[k]);
					}
				}
				// Calculate the number of triangles in the current batch
				unsigned int ui32Tris;

				if(j + 1 < (unsigned int) inMesh.sBoneBatches.nBatchCnt)
					ui32Tris = inMesh.sBoneBatches.pnBatchOffset[j + 1] - inMesh.sBoneBatches.pnBatchOffset[j];
				else
					ui32Tris = inMesh.nNumFaces - inMesh.sBoneBatches.pnBatchOffset[j];

				unsigned int idx;
				float fBoneIdx[4], fBoneWeights[4];

				if(inMesh.nNumStrips == 0)
				{
					ui32Offset = 3 * inMesh.sBoneBatches.pnBatchOffset[j];

					for(l = ui32Offset; l < ui32Offset + (ui32Tris * 3); ++l)
					{
						if(inMesh.sFaces.pData) // Indexed Triangle Lists
							PVRTVertexRead(&idx, inMesh.sFaces.pData + (l * inMesh.sFaces.nStride), inMesh.sFaces.eType);
						else // Indexed Triangle Lists
							idx = l;

						if(!pbTransformed[idx])
						{
							PVRTVertexRead((PVRTVECTOR4f*) &fBoneIdx[0], inMesh.sBoneIdx.pData + (idx * inMesh.sBoneIdx.nStride), inMesh.sBoneIdx.eType, inMesh.sBoneIdx.n);
							PVRTVertexRead((PVRTVECTOR4f*) &fBoneWeights[0], inMesh.sBoneWeight.pData + (idx * inMesh.sBoneWeight.nStride), inMesh.sBoneWeight.eType, inMesh.sBoneWeight.n);

							TransformCPODData(inMesh.sVertex, outMesh.sVertex, idx, pPalette, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, false);
							TransformCPODData(inMesh.sNormals, outMesh.sNormals, idx, pPaletteInvTrans, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, true);
							TransformCPODData(inMesh.sTangents, outMesh.sTangents, idx, pPaletteInvTrans, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, true);
							TransformCPODData(inMesh.sBinormals, outMesh.sBinormals, idx, pPaletteInvTrans, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, true);
							pbTransformed[idx] = true;
						}
					}
				}
				else
				{
					unsigned int ui32TrisDrawn = 0;

					while(ui32TrisDrawn < ui32Tris)
					{
						for(l = ui32Offset; l < ui32Offset + (inMesh.pnStripLength[ui32Strip]+2); ++l)
						{
							if(inMesh.sFaces.pData) // Indexed Triangle Strips
								PVRTVertexRead(&idx, inMesh.sFaces.pData + (l * inMesh.sFaces.nStride), inMesh.sFaces.eType);
							else // Triangle Strips
								idx = l;

							if(!pbTransformed[idx])
							{
								PVRTVertexRead((PVRTVECTOR4f*) &fBoneIdx[0], inMesh.sBoneIdx.pData + (idx * inMesh.sBoneIdx.nStride), inMesh.sBoneIdx.eType, inMesh.sBoneIdx.n);
								PVRTVertexRead((PVRTVECTOR4f*) &fBoneWeights[0], inMesh.sBoneWeight.pData + (idx * inMesh.sBoneWeight.nStride), inMesh.sBoneWeight.eType, inMesh.sBoneWeight.n);

								TransformCPODData(inMesh.sVertex, outMesh.sVertex, idx, pPalette, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, false);
								TransformCPODData(inMesh.sNormals, outMesh.sNormals, idx, pPaletteInvTrans, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, true);
								TransformCPODData(inMesh.sTangents, outMesh.sTangents, idx, pPaletteInvTrans, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, true);
								TransformCPODData(inMesh.sBinormals, outMesh.sBinormals, idx, pPaletteInvTrans, &fBoneIdx[0], &fBoneWeights[0], inMesh.sBoneIdx.n, true);
								pbTransformed[idx] = true;
							}
						}

						ui32Offset	  += inMesh.pnStripLength[ui32Strip] + 2;
						ui32TrisDrawn += inMesh.pnStripLength[ui32Strip];

						++ui32Strip;
					}
				}
			}

			FREE(pPalette);
			FREE(pPaletteInvTrans);
			FREE(pbTransformed);
		}
		else
		{
			// Get transformation matrix
			in.GetWorldMatrix(mWorld, inNode);
			PVRTMATRIX mWorldInvTrans;

			// Get the inverse transpose of the 3x3
			if(inMesh.sNormals.n || inMesh.sTangents.n || inMesh.sBinormals.n)
			{
				mWorldInvTrans = mWorld;
				mWorldInvTrans.f[3]  = mWorldInvTrans.f[7]  = mWorldInvTrans.f[11] = 0;
				mWorldInvTrans.f[12] = mWorldInvTrans.f[13] = mWorldInvTrans.f[14] = 0;
				PVRTMatrixInverse(mWorldInvTrans, mWorldInvTrans);
				PVRTMatrixTranspose(mWorldInvTrans, mWorldInvTrans);
			}

			// Transform the vertices
			for(j = 0; j < inMesh.nNumVertex; ++j)
			{
				TransformCPODData(inMesh.sVertex, outMesh.sVertex, j, &mWorld, 0, 0, 0, false);
				TransformCPODData(inMesh.sNormals, outMesh.sNormals, j, &mWorldInvTrans, 0, 0, 0, true);
				TransformCPODData(inMesh.sTangents, outMesh.sTangents, j, &mWorldInvTrans, 0, 0, 0, true);
				TransformCPODData(inMesh.sBinormals, outMesh.sBinormals, j, &mWorldInvTrans, 0, 0, 0, true);
			}
		}
	}

	// Copy the rest of the nodes
	for(i = in.nNumMeshNode; i < in.nNumNode; ++i)
	{
		PVRTModelPODCopyNode(in.pNode[i], out.pNode[i], in.nNumFrame);

		// Strip out animation and parenting
		out.pNode[i].nIdxParent = -1;

		out.pNode[i].nAnimFlags = 0;
		FREE(out.pNode[i].pfAnimMatrix);
		FREE(out.pNode[i].pnAnimMatrixIdx);

		FREE(out.pNode[i].pfAnimPosition);
		FREE(out.pNode[i].pnAnimPositionIdx);

		FREE(out.pNode[i].pfAnimRotation);
		FREE(out.pNode[i].pnAnimRotationIdx);

		FREE(out.pNode[i].pfAnimScale);
		FREE(out.pNode[i].pnAnimScaleIdx);

		// Get world transformation matrix....
		in.GetWorldMatrix(mWorld, in.pNode[i]);

		// ...set the out node transformation matrix
		if(SafeAlloc(out.pNode[i].pfAnimMatrix, 16))
			memcpy(out.pNode[i].pfAnimMatrix, mWorld.f, sizeof(PVRTMATRIX));
	}

	// Copy camera, lights
	if(in.nNumCamera && SafeAlloc(out.pCamera, in.nNumCamera))
	{
		out.nNumCamera = in.nNumCamera;

		for(i = 0; i < in.nNumCamera; ++i)
			PVRTModelPODCopyCamera(in.pCamera[i], out.pCamera[i], in.nNumFrame);
	}

	if(in.nNumLight && SafeAlloc(out.pLight, in.nNumLight))
	{
		out.nNumLight = in.nNumLight;

		for(i = 0; i < out.nNumLight; ++i)
			PVRTModelPODCopyLight(in.pLight[i], out.pLight[i]);
	}

	// Copy textures
	if(in.nNumTexture && SafeAlloc(out.pTexture, in.nNumTexture))
	{
		out.nNumTexture = in.nNumTexture;

		for(i = 0; i < out.nNumTexture; ++i)
			PVRTModelPODCopyTexture(in.pTexture[i], out.pTexture[i]);
	}

	// Copy materials
	if(in.nNumMaterial && SafeAlloc(out.pMaterial, in.nNumMaterial))
	{
		out.nNumMaterial = in.nNumMaterial;

		for(i = 0; i < in.nNumMaterial; ++i)
			PVRTModelPODCopyMaterial(in.pMaterial[i], out.pMaterial[i]);
	}

	out.InitImpl();

	return PVR_SUCCESS;
}

static bool MergeTexture(const CPVRTModelPOD &src, CPVRTModelPOD &dst, const int &srcTexID, int &dstTexID)
{
	if(srcTexID != -1 && srcTexID < (int) src.nNumTexture)
	{
		if(dstTexID == -1)
		{
			// Resize our texture array to add our texture
			dst.pTexture = (SPODTexture*) realloc(dst.pTexture, (dst.nNumTexture + 1) * sizeof(SPODTexture));

			if(!dst.pTexture)
				return false;

			dstTexID = dst.nNumTexture;
			++dst.nNumTexture;

			dst.pTexture[dstTexID].pszName = (char*) malloc(strlen(src.pTexture[srcTexID].pszName) + 1);
			strcpy(dst.pTexture[dstTexID].pszName, src.pTexture[srcTexID].pszName);
			return true;
		}

		// See if our texture names match
		if(strcmp(src.pTexture[srcTexID].pszName, dst.pTexture[dstTexID].pszName) == 0)
			return true; // Nothing to do

		// See if our texture filenames match
		char * srcName = src.pTexture[srcTexID].pszName;
		char * dstName = dst.pTexture[dstTexID].pszName;
		bool bFoundPossibleEndOfFilename = false;
		bool bStrMatch = true, bFilenameMatch = true;

		while(*srcName != '\0' && *dstName != '\0')
		{
			if(*srcName != *dstName)
			{
				if(!bFoundPossibleEndOfFilename)
					return true; // They don't match

				bStrMatch = false;
			}

			if(*srcName == '.')
			{
				if(!bStrMatch)
					return true; // They don't match

				bFoundPossibleEndOfFilename = true;
				bFilenameMatch = bStrMatch;
			}

			++srcName;
			++dstName;
		}

		if(bFilenameMatch)
		{
			// Our filenames match but our extensions don't so merge our textures
			FREE(dst.pTexture[dstTexID].pszName);
			dst.pTexture[dstTexID].pszName = (char*) malloc(strlen(src.pTexture[srcTexID].pszName) + 1);
			strcpy(dst.pTexture[dstTexID].pszName, src.pTexture[srcTexID].pszName);
			return true;
		}

		// Our texture names aren't the same so don't try and merge
	}

	return true;
}

/*!***************************************************************************
 @Function			PVRTModelPODMergeMaterials
 @Input				src - Source scene
 @Output			dst - Destination scene
 @Description		This function takes two scenes and merges the textures,
					PFX effects and blending parameters from the src materials
					into the dst materials if they have the same material name.
*****************************************************************************/
EPVRTError PVRTModelPODMergeMaterials(const CPVRTModelPOD &src, CPVRTModelPOD &dst)
{
	if(!src.nNumMaterial || !dst.nNumMaterial)
		return PVR_SUCCESS;

	bool *bMatched = (bool*) calloc(dst.nNumMaterial, sizeof(bool));

	if(!bMatched)
		return PVR_FAIL;

	for(unsigned int i = 0; i < src.nNumMaterial; ++i)
	{
		const SPODMaterial &srcMaterial = src.pMaterial[i];

		// Match our current material with one in the dst
		for(unsigned int j = 0; j < dst.nNumMaterial; ++j)
		{
			if(bMatched[j])
				continue; // We have already matched this material with another

			SPODMaterial &dstMaterial = dst.pMaterial[j];

			// We've found a material with the same name
			if(strcmp(srcMaterial.pszName, dstMaterial.pszName) == 0)
			{
				bMatched[j] = true;

				// Merge the textures
				if(!MergeTexture(src, dst, srcMaterial.nIdxTexDiffuse, dstMaterial.nIdxTexDiffuse))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexAmbient, dstMaterial.nIdxTexAmbient))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexSpecularColour, dstMaterial.nIdxTexSpecularColour))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexSpecularLevel, dstMaterial.nIdxTexSpecularLevel))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexBump, dstMaterial.nIdxTexBump))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexEmissive, dstMaterial.nIdxTexEmissive))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexGlossiness, dstMaterial.nIdxTexGlossiness))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexOpacity, dstMaterial.nIdxTexOpacity))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexReflection, dstMaterial.nIdxTexReflection))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				if(!MergeTexture(src, dst, srcMaterial.nIdxTexRefraction, dstMaterial.nIdxTexRefraction))
				{
					FREE(bMatched);
					return PVR_FAIL;
				}

				dstMaterial.eBlendSrcRGB = srcMaterial.eBlendSrcRGB;
				dstMaterial.eBlendSrcA = srcMaterial.eBlendSrcA;
				dstMaterial.eBlendDstRGB = srcMaterial.eBlendDstRGB;
				dstMaterial.eBlendDstA = srcMaterial.eBlendDstA;
				dstMaterial.eBlendOpRGB = srcMaterial.eBlendOpRGB;
				dstMaterial.eBlendOpA = srcMaterial.eBlendOpA;
				memcpy(dstMaterial.pfBlendColour, srcMaterial.pfBlendColour, 4 * sizeof(VERTTYPE));
				memcpy(dstMaterial.pfBlendFactor, srcMaterial.pfBlendFactor, 4 * sizeof(VERTTYPE));
				dstMaterial.nFlags = srcMaterial.nFlags;

				// Merge effect names
				if(srcMaterial.pszEffectFile)
				{
					FREE(dstMaterial.pszEffectFile);
					dstMaterial.pszEffectFile = (char*) malloc(strlen(srcMaterial.pszEffectFile) + 1);
					strcpy(dstMaterial.pszEffectFile, srcMaterial.pszEffectFile);
				}

				if(srcMaterial.pszEffectName)
				{
					FREE(dstMaterial.pszEffectName);
					dstMaterial.pszEffectName = (char*) malloc(strlen(srcMaterial.pszEffectName) + 1);
					strcpy(dstMaterial.pszEffectName, srcMaterial.pszEffectName);
				}

				break;
			}
		}
	}

	FREE(bMatched);
	return PVR_SUCCESS;
}

/*****************************************************************************
 End of file (PVRTModelPOD.cpp)
*****************************************************************************/

