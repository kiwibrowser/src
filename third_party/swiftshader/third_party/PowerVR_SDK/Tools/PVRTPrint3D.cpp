/******************************************************************************

 @file         PVRTPrint3D.cpp
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Displays a text string using 3D polygons. Can be done in two ways:
               using a window defined by the user or writing straight on the
               screen.

******************************************************************************/

/****************************************************************************
** Includes
****************************************************************************/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "PVRTGlobal.h"
#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"
#include "PVRTTexture.h"
#include "PVRTPrint3D.h"
#include "PVRTUnicode.h"
#include "PVRTContext.h"
#include "PVRTMap.h"

/* Print3D texture data */
#include "PVRTPrint3DIMGLogo.h"
#include "PVRTPrint3DHelveticaBold.h"

static inline float PVRTMakeWhole(float f)
{
	return floorf(f + 0.5f);
}


/****************************************************************************
** Defines
****************************************************************************/
#define MAX_LETTERS				(5120)
#define MIN_CACHED_VTX			(0x1000)
#define MAX_CACHED_VTX			(0x00100000)
#define LINES_SPACING			(29.0f)
#define PVRPRINT3DVERSION		(1)

#if defined(_WIN32)
#define vsnprintf _vsnprintf
#endif

const PVRTuint32 PVRFONT_HEADER			= 0xFCFC0050;
const PVRTuint32 PVRFONT_CHARLIST		= 0xFCFC0051;
const PVRTuint32 PVRFONT_RECTS			= 0xFCFC0052;
const PVRTuint32 PVRFONT_METRICS		= 0xFCFC0053;
const PVRTuint32 PVRFONT_YOFFSET		= 0xFCFC0054;
const PVRTuint32 PVRFONT_KERNING		= 0xFCFC0055;

/****************************************************************************
** Constants
****************************************************************************/
static const unsigned int PVRTPRINT3D_INVALID_CHAR = 0xFDFDFDFD;

/****************************************************************************
** Auxiliary functions
****************************************************************************/
/*!***************************************************************************
@fn       		CharacterCompareFunc
@param[in]		pA
@param[in]		pB
@return			PVRTint32	
@brief      	Compares two characters for binary search.
*****************************************************************************/
PVRTint32 CPVRTPrint3D::CharacterCompareFunc(const void* pA, const void* pB)
{
	return (*(PVRTint32*)pA - *(PVRTint32*)pB);
}

/*!***************************************************************************
@fn       		KerningCompareFunc
@param[in]		pA
@param[in]		pB
@return			PVRTint32	
@brief      	Compares two kerning pairs for binary search.
*****************************************************************************/
PVRTint32 CPVRTPrint3D::KerningCompareFunc(const void* pA, const void* pB)
{
	KerningPair* pPairA = (KerningPair*)pA;
	KerningPair* pPairB = (KerningPair*)pB;

	if(pPairA->uiPair > pPairB->uiPair)		return 1;
	if(pPairA->uiPair < pPairB->uiPair)		return -1;

	return 0;
}

/****************************************************************************
** Class: CPVRTPrint3D
****************************************************************************/
/*****************************************************************************
 @fn       		CPVRTPrint3D
 @brief      	Init some values.
*****************************************************************************/
CPVRTPrint3D::CPVRTPrint3D() :	m_pAPI(NULL), m_uLogoToDisplay(ePVRTPrint3DLogoNone), m_pwFacesFont(NULL), m_pPrint3dVtx(NULL), m_bTexturesSet(false), m_pVtxCache(NULL), m_nVtxCache(0),
								m_nVtxCacheMax(0), m_bRotate(false), m_nCachedNumVerts(0), m_pwzPreviousString(NULL), m_pszPreviousString(NULL), m_fPrevScale(0.0f), m_fPrevX(0.0f),
								m_fPrevY(0.0f), m_uiPrevCol(0), m_pUVs(NULL), m_pKerningPairs(NULL), m_pCharMatrics(NULL), m_fTexW(0.0f), m_fTexH(0.0f), m_pRects(NULL), m_pYOffsets(NULL), 
								m_uiNextLineH(0), m_uiSpaceWidth(0), m_uiNumCharacters(0), m_uiNumKerningPairs(0), m_uiAscent(0), m_pszCharacterList(NULL), m_bHasMipmaps(false), 
								m_bUsingProjection(false)
{
	memset(m_fScreenScale, 0, sizeof(m_fScreenScale));
	memset(m_ui32ScreenDim, 0, sizeof(m_ui32ScreenDim));

	PVRTMatrixIdentity(m_mModelView);
	PVRTMatrixIdentity(m_mProj);

	m_pwzPreviousString = new wchar_t[MAX_LETTERS + 1];
	m_pszPreviousString = new char[MAX_LETTERS + 1];
	m_pwzPreviousString[0] = 0;
	m_pszPreviousString[0] = 0;

	m_eFilterMethod[eFilterProc_Min] = eFilter_Default;
	m_eFilterMethod[eFilterProc_Mag] = eFilter_Default;
	m_eFilterMethod[eFilterProc_Mip] = eFilter_MipDefault;
}

/*****************************************************************************
 @fn       		~CPVRTPrint3D
 @brief      	De-allocate the working memory
*****************************************************************************/
CPVRTPrint3D::~CPVRTPrint3D()
{
	delete [] m_pwzPreviousString;
	delete [] m_pszPreviousString;

	delete [] m_pszCharacterList;
	delete [] m_pYOffsets;
	delete [] m_pCharMatrics;
	delete [] m_pKerningPairs;
	delete [] m_pRects;
	delete [] m_pUVs;
}

/*!***************************************************************************
@fn       		ReadMetaBlock
@param[in]		pDataCursor
@return			bool	true if successful.
@brief      	Reads a single meta data block from the data file.
*****************************************************************************/
bool CPVRTPrint3D::ReadMetaBlock(const PVRTuint8** pDataCursor)
{
	SPVRTPrint3DHeader* header;

	unsigned int uiDataSize;

	MetaDataBlock block;
	if(!block.ReadFromPtr(pDataCursor))
	{
		return false;		// Must have been an error.
	}

	switch(block.u32Key)
	{
	case PVRFONT_HEADER:
		header = (SPVRTPrint3DHeader*)block.Data;
		if(header->uVersion != PVRTPRINT3D_VERSION)
		{
			return false;
		}
		// Copy options
		m_uiAscent			= header->wAscent;
		m_uiNextLineH		= header->wLineSpace;
		m_uiSpaceWidth		= header->uSpaceWidth;
		m_uiNumCharacters	= header->wNumCharacters & 0xFFFF;
		m_uiNumKerningPairs = header->wNumKerningPairs & 0xFFFF;	
		break;
	case PVRFONT_CHARLIST:
		uiDataSize = sizeof(PVRTuint32) * m_uiNumCharacters;
		_ASSERT(block.u32DataSize == uiDataSize);
		m_pszCharacterList = new PVRTuint32[m_uiNumCharacters];
		memcpy(m_pszCharacterList, block.Data, uiDataSize);
		break;
	case PVRFONT_YOFFSET:
		uiDataSize = sizeof(PVRTint32) * m_uiNumCharacters;
		_ASSERT(block.u32DataSize == uiDataSize);
		m_pYOffsets	= new PVRTint32[m_uiNumCharacters];
		memcpy(m_pYOffsets, block.Data, uiDataSize);
		break;
	case PVRFONT_METRICS:
		uiDataSize = sizeof(CharMetrics) * m_uiNumCharacters;
		_ASSERT(block.u32DataSize == uiDataSize);
		m_pCharMatrics = new CharMetrics[m_uiNumCharacters];
		memcpy(m_pCharMatrics, block.Data, uiDataSize);
		break;
	case PVRFONT_KERNING:
		uiDataSize = sizeof(KerningPair) * m_uiNumKerningPairs;
		_ASSERT(block.u32DataSize == uiDataSize);
		m_pKerningPairs = new KerningPair[m_uiNumKerningPairs];
		memcpy(m_pKerningPairs, block.Data, uiDataSize);
		break;
	case PVRFONT_RECTS:
		uiDataSize = sizeof(Rectanglei) * m_uiNumCharacters;
		_ASSERT(block.u32DataSize == uiDataSize);

		m_pRects = new Rectanglei[m_uiNumCharacters];
		memcpy(m_pRects, block.Data, uiDataSize);
		break;
	default:
		_ASSERT(!"Unhandled key!");
	}

	return true;
}

/*!***************************************************************************
@fn       		LoadFontData
@param[in]		texHeader
@param[in]		MetaDataMap
@return			bool	true if successful.
@brief      	Loads font data bundled with the texture file.
*****************************************************************************/
bool CPVRTPrint3D::LoadFontData( const PVRTextureHeaderV3* texHeader, CPVRTMap<PVRTuint32, CPVRTMap<PVRTuint32, MetaDataBlock> >& MetaDataMap )
{
	m_fTexW = (float)texHeader->u32Width;
	m_fTexH = (float)texHeader->u32Height;

	// Mipmap data is stored in the texture header data.
	m_bHasMipmaps = (texHeader->u32MIPMapCount > 1 ? true : false);
	if(m_bHasMipmaps)
	{
		m_eFilterMethod[eFilterProc_Min] = eFilter_Linear;
		m_eFilterMethod[eFilterProc_Mag] = eFilter_Linear;
		m_eFilterMethod[eFilterProc_Mip] = eFilter_Linear;
	}
	else
	{
		m_eFilterMethod[eFilterProc_Min] = eFilter_Linear;
		m_eFilterMethod[eFilterProc_Mag] = eFilter_Linear;
		m_eFilterMethod[eFilterProc_Mip] = eFilter_None;
	}


	// Header
	SPVRTPrint3DHeader* header = (SPVRTPrint3DHeader*)MetaDataMap[PVRTEX3_IDENT][PVRFONT_HEADER].Data;
	if(header->uVersion != PVRTPRINT3D_VERSION)
	{
		return false;
	}
	// Copy options
	m_uiAscent			= header->wAscent;
	m_uiNextLineH		= header->wLineSpace;
	m_uiSpaceWidth		= header->uSpaceWidth;
	m_uiNumCharacters	= header->wNumCharacters & 0xFFFF;
	m_uiNumKerningPairs = header->wNumKerningPairs & 0xFFFF;	

	// Char list
	m_pszCharacterList = new PVRTuint32[m_uiNumCharacters];
	memcpy(m_pszCharacterList, MetaDataMap[PVRTEX3_IDENT][PVRFONT_CHARLIST].Data, MetaDataMap[PVRTEX3_IDENT][PVRFONT_CHARLIST].u32DataSize);
	
	m_pYOffsets	= new PVRTint32[m_uiNumCharacters];
	memcpy(m_pYOffsets, MetaDataMap[PVRTEX3_IDENT][PVRFONT_YOFFSET].Data, MetaDataMap[PVRTEX3_IDENT][PVRFONT_YOFFSET].u32DataSize);

	m_pCharMatrics = new CharMetrics[m_uiNumCharacters];
	memcpy(m_pCharMatrics, MetaDataMap[PVRTEX3_IDENT][PVRFONT_METRICS].Data, MetaDataMap[PVRTEX3_IDENT][PVRFONT_METRICS].u32DataSize);
	
	m_pKerningPairs = new KerningPair[m_uiNumKerningPairs];
	memcpy(m_pKerningPairs, MetaDataMap[PVRTEX3_IDENT][PVRFONT_KERNING].Data, MetaDataMap[PVRTEX3_IDENT][PVRFONT_KERNING].u32DataSize);

	m_pRects = new Rectanglei[m_uiNumCharacters];
	memcpy(m_pRects, MetaDataMap[PVRTEX3_IDENT][PVRFONT_RECTS].Data, MetaDataMap[PVRTEX3_IDENT][PVRFONT_RECTS].u32DataSize);
	

	// Build UVs
	m_pUVs = new CharacterUV[m_uiNumCharacters];
	for(unsigned int uiChar = 0; uiChar < m_uiNumCharacters; uiChar++)
	{
		m_pUVs[uiChar].fUL = m_pRects[uiChar].nX / m_fTexW;
		m_pUVs[uiChar].fUR = m_pUVs[uiChar].fUL + m_pRects[uiChar].nW / m_fTexW;
		m_pUVs[uiChar].fVT = m_pRects[uiChar].nY / m_fTexH;
		m_pUVs[uiChar].fVB = m_pUVs[uiChar].fVT + m_pRects[uiChar].nH / m_fTexH;
	}	

	return true;
}

/*!***************************************************************************
@fn       		FindCharacter
@param[in]		character
@return			The character index, or PVRPRINT3D_INVALID_CHAR if not found.
@brief      	Finds a given character in the binary data and returns it's
				index.
*****************************************************************************/
PVRTuint32 CPVRTPrint3D::FindCharacter(PVRTuint32 character) const
{
	PVRTuint32* pItem = (PVRTuint32*)bsearch(&character, m_pszCharacterList, m_uiNumCharacters, sizeof(PVRTuint32), CharacterCompareFunc);
	if(!pItem)
		return PVRTPRINT3D_INVALID_CHAR;

	PVRTuint32 uiIdx = (PVRTuint32) (pItem - m_pszCharacterList);
	return uiIdx;
}

/*!***************************************************************************
@fn       		ApplyKerning
@param[in]		cA
@param[in]		cB
@param[out]		fOffset
@brief      	Calculates kerning offset.
*****************************************************************************/
void CPVRTPrint3D::ApplyKerning(const PVRTuint32 cA, const PVRTuint32 cB, float& fOffset) const
{	
	PVRTuint64 uiPairToSearch = ((PVRTuint64)cA << 32) | (PVRTuint64)cB;
	KerningPair* pItem = (KerningPair*)bsearch(&uiPairToSearch, m_pKerningPairs, m_uiNumKerningPairs, sizeof(KerningPair), KerningCompareFunc);
	if(pItem)
		fOffset += (float)pItem->iOffset;
}

/*!***************************************************************************
 @fn       			SetTextures
 @param[in]			pContext		Context
 @param[in]			dwScreenX		Screen resolution along X
 @param[in]			dwScreenY		Screen resolution along Y
 @param[in]			bRotate			Rotate print3D by 90 degrees
 @param[in]			bMakeCopy		This instance of Print3D creates a copy
									of it's data instead of sharing with previous
									contexts. Set this parameter if you require
									thread safety.	
 @return			PVR_SUCCESS or PVR_FAIL
 @brief      		Initialization and texture upload. Should be called only once
					for a given context.
*****************************************************************************/
EPVRTError CPVRTPrint3D::SetTextures(
	const SPVRTContext	* const pContext,
	const unsigned int	dwScreenX,
	const unsigned int	dwScreenY,
	const bool bRotate,
	const bool bMakeCopy)
{
	// Determine which set of textures to use depending on the screen resolution.
	const unsigned int uiShortestEdge = PVRT_MIN(dwScreenX, dwScreenY);
	const void* pData = NULL;

	if(uiShortestEdge >= 720)
	{
		pData = (void*)_helvbd_56_pvr;
	}
	else if(uiShortestEdge >= 640)
	{
		pData = (void*)_helvbd_46_pvr;
	}
	else
	{
		pData = (void*)_helvbd_36_pvr;
	}
	
	PVRT_UNREFERENCED_PARAMETER(_helvbd_36_pvr_size);
	PVRT_UNREFERENCED_PARAMETER(_helvbd_46_pvr_size);
	PVRT_UNREFERENCED_PARAMETER(_helvbd_56_pvr_size);

	return SetTextures(pContext, pData, dwScreenX, dwScreenY, bRotate, bMakeCopy);
}

/*!***************************************************************************
	@fn       		SetTextures
	@param[in]		pContext		Context
	@param[in]		pTexData		User-provided font texture
	@param[in]		uiDataSize		Size of the data provided
	@param[in]		dwScreenX		Screen resolution along X
	@param[in]		dwScreenY		Screen resolution along Y
	@param[in]		bRotate			Rotate print3D by 90 degrees
	@param[in]		bMakeCopy		This instance of Print3D creates a copy
									of it's data instead of sharing with previous
									contexts. Set this parameter if you require
									thread safety.	
	@return			PVR_SUCCESS or PVR_FAIL
	@brief      	Initialization and texture upload of user-provided font 
					data. Should be called only once for a Print3D object.
*****************************************************************************/
EPVRTError CPVRTPrint3D::SetTextures(
	const SPVRTContext	* const pContext,
	const void * const pTexData,
	const unsigned int	dwScreenX,
	const unsigned int	dwScreenY,
	const bool bRotate,
	const bool bMakeCopy)
{
#if !defined (DISABLE_PRINT3D)

	unsigned short	i;
	bool			bStatus;

	// Set the aspect ratio, so we can change it without updating textures or anything else
	float fX, fY;

	m_bRotate = bRotate;
	m_ui32ScreenDim[0] = bRotate ? dwScreenY : dwScreenX;
	m_ui32ScreenDim[1] = bRotate ? dwScreenX : dwScreenY;

	// Alter the X, Y resolutions if the screen isn't portrait.
	if(dwScreenX > dwScreenY)
	{
		fX = (float) dwScreenX;
		fY = (float) dwScreenY;
	}
	else
	{
		fX = (float) dwScreenY;
		fY = (float) dwScreenX;
	}

	m_fScreenScale[0] = (bRotate ? fY : fX) /640.0f;
	m_fScreenScale[1] = (bRotate ? fX : fY) /480.0f;

	// Check whether textures are already set up just in case
	if (m_bTexturesSet)
		return PVR_SUCCESS;

	// INDEX BUFFERS
	m_pwFacesFont = (unsigned short*)malloc(PVRTPRINT3D_MAX_RENDERABLE_LETTERS*2*3*sizeof(unsigned short));

	if(!m_pwFacesFont)
	{
		return PVR_FAIL;
	}

	// Vertex indices for letters
	for (i=0; i < PVRTPRINT3D_MAX_RENDERABLE_LETTERS; i++)
	{
		m_pwFacesFont[i*6+0] = 0+i*4;
		m_pwFacesFont[i*6+1] = 3+i*4;
		m_pwFacesFont[i*6+2] = 1+i*4;

		m_pwFacesFont[i*6+3] = 3+i*4;
		m_pwFacesFont[i*6+4] = 0+i*4;
		m_pwFacesFont[i*6+5] = 2+i*4;
	}


	if(!APIInit(pContext, bMakeCopy))
	{
		return PVR_FAIL;
	}
	/*
		This is the texture with the fonts.
	*/
	PVRTextureHeaderV3 header;
	CPVRTMap<PVRTuint32, CPVRTMap<PVRTuint32, MetaDataBlock> > MetaDataMap;
	bStatus = APIUpLoadTexture((unsigned char *)pTexData, &header, MetaDataMap);

	if (!bStatus)
	{
		return PVR_FAIL;
	}
	/*
		This is the associated font data with the default font
	*/
	bStatus = LoadFontData(&header, MetaDataMap);
	
	bStatus = APIUpLoadIcons(reinterpret_cast<const PVRTuint8* const>(PVRTPrint3DIMGLogo), reinterpret_cast<const PVRTuint8* const>(PVRTPrint3DPowerVRLogo));

	if (!bStatus) return PVR_FAIL;

	m_nVtxCacheMax = MIN_CACHED_VTX;
	m_pVtxCache = (SPVRTPrint3DAPIVertex*)malloc(m_nVtxCacheMax * sizeof(*m_pVtxCache));
	m_nVtxCache = 0;

	if(!m_pVtxCache)
	{
		return PVR_FAIL;
	}

	// Everything is OK
	m_bTexturesSet = true;

	// Return Success
	return PVR_SUCCESS;

#else
	return PVR_SUCCESS;
#endif
}

/*!***************************************************************************
@fn       		Print3D
@param[in]		fPosX		X Position
@param[in]		fPosY		Y Position
@param[in]		fScale		Text scale
@param[in]		Colour		ARGB colour
@param[in]		UTF32		Array of UTF32 characters
@param[in]		bUpdate		Whether to update the vertices
@return			EPVRTError	Success of failure
@brief      	Takes an array of UTF32 characters and generates the required mesh.
*****************************************************************************/
EPVRTError CPVRTPrint3D::Print3D(float fPosX, float fPosY, const float fScale, unsigned int Colour, const CPVRTArray<PVRTuint32>& UTF32, bool bUpdate)
{
	// No textures! so... no window
	if (!m_bTexturesSet)
	{
		PVRTErrorOutputDebug("DisplayWindow : You must call CPVRTPrint3D::SetTextures(...) before using this function.\n");
		return PVR_FAIL;
	}

	// nothing to be drawn
	if(UTF32.GetSize() == 0)
		return PVR_FAIL;

	// Adjust input parameters
	if(!m_bUsingProjection)
	{
		fPosX =  (float)((int)(fPosX * (640.0f/100.0f)));
		fPosY = -(float)((int)(fPosY * (480.0f/100.0f)));
	}

	// Create Vertex Buffer (only if it doesn't exist)
	if(m_pPrint3dVtx == 0)
	{
		m_pPrint3dVtx = (SPVRTPrint3DAPIVertex*)malloc(MAX_LETTERS*4*sizeof(SPVRTPrint3DAPIVertex));

		if(!m_pPrint3dVtx)
			return PVR_FAIL;
	}

	// Fill up our buffer
	if(bUpdate)
		m_nCachedNumVerts = UpdateLine(0.0f, fPosX, fPosY, fScale, Colour, UTF32, m_pPrint3dVtx);

	// Draw the text
	if(!DrawLine(m_pPrint3dVtx, m_nCachedNumVerts))
		return PVR_FAIL;

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @fn       			Print3D
 @param[in]			fPosX		Position of the text along X
 @param[in]			fPosY		Position of the text along Y
 @param[in]			fScale		Scale of the text
 @param[in]			Colour		Colour of the text
 @param[in]			pszFormat	Format string for the text
 @return			PVR_SUCCESS or PVR_FAIL
 @brief      		Display wide-char 3D text on screen.
					CPVRTPrint3D::SetTextures(...) must have been called
					beforehand.
					This function accepts formatting in the printf way.
*****************************************************************************/
EPVRTError CPVRTPrint3D::Print3D(float fPosX, float fPosY, const float fScale, unsigned int Colour, const wchar_t * const pszFormat, ...)
{
#ifdef DISABLE_PRINT3D
	return PVR_SUCCESS;
#endif

	static wchar_t s_Text[MAX_LETTERS+1] = {0};

	/*
		Unfortunately only Windows seems to properly support non-ASCII characters formatted in
		vswprintf.
	*/
#if defined(_WIN32) && !defined(UNDER_CE)
	va_list		args;
	// Reading the arguments to create our Text string
	va_start(args, pszFormat);
	vswprintf(s_Text, MAX_LETTERS+1, pszFormat, args);
	va_end(args);
#else
	wcscpy(s_Text, pszFormat);
#endif

	bool bUpdate = false;

	// Optimisation to check that the strings are actually different.
	if(wcscmp(s_Text, m_pwzPreviousString) != 0 || m_fPrevX != fPosX || m_fPrevY != fPosY || m_fPrevScale != fScale || m_uiPrevCol != Colour)
	{
		// Copy strings
		wcscpy(m_pwzPreviousString, s_Text);
		m_fPrevX = fPosX;
		m_fPrevY = fPosY;
		m_fPrevScale = fScale;
		m_uiPrevCol  = Colour;
		
		m_CachedUTF32.Clear();
#if PVRTSIZEOFWCHAR == 2			// 2 byte wchar.
		PVRTUnicodeUTF16ToUTF32((PVRTuint16*)s_Text, m_CachedUTF32);
#elif PVRTSIZEOFWCHAR == 4			// 4 byte wchar (POSIX)
		unsigned int uiC = 0;
		PVRTuint32* pUTF32 = (PVRTuint32*)s_Text;
		while(*pUTF32 && uiC < MAX_LETTERS)
		{
			m_CachedUTF32.Append(*pUTF32++);
			uiC++;
		}
#else
		return PVR_FAIL;
#endif

		bUpdate = true;
	}

	// Print
	return Print3D(fPosX, fPosY, fScale, Colour, m_CachedUTF32, bUpdate);
}

/*!***************************************************************************
 @fn       			PVRTPrint3D
 @param[in]			fPosX		Position of the text along X
 @param[in]			fPosY		Position of the text along Y
 @param[in]			fScale		Scale of the text
 @param[in]			Colour		Colour of the text
 @param[in]			pszFormat	Format string for the text
 @return			PVR_SUCCESS or PVR_FAIL
 @brief      		Display 3D text on screen.
					No window needs to be allocated to use this function.
					However, CPVRTPrint3D::SetTextures(...) must have been called
					beforehand.
					This function accepts formatting in the printf way.
*****************************************************************************/
EPVRTError CPVRTPrint3D::Print3D(float fPosX, float fPosY, const float fScale, unsigned int Colour, const char * const pszFormat, ...)
{
#ifdef DISABLE_PRINT3D
	return PVR_SUCCESS;
#endif

	va_list		args;
	static char	s_Text[MAX_LETTERS+1] = {0};
	
	// Reading the arguments to create our Text string
	va_start(args, pszFormat);
	vsnprintf(s_Text, MAX_LETTERS+1, pszFormat, args);
	va_end(args);

	bool bUpdate = false;

	// Optimisation to check that the strings are actually different.
	if(strcmp(s_Text, m_pszPreviousString) != 0 || m_fPrevX != fPosX || m_fPrevY != fPosY || m_fPrevScale != fScale || m_uiPrevCol != Colour)
	{
		// Copy strings
		strcpy (m_pszPreviousString, s_Text);
		m_fPrevX = fPosX;
		m_fPrevY = fPosY;
		m_fPrevScale = fScale;
		m_uiPrevCol  = Colour;

		// Convert from UTF8 to UTF32
		m_CachedUTF32.Clear();
		PVRTUnicodeUTF8ToUTF32((const PVRTuint8*)s_Text, m_CachedUTF32);

		bUpdate = true;
	}

	// Print
	return Print3D(fPosX, fPosY, fScale, Colour, m_CachedUTF32, bUpdate);
}

/*!***************************************************************************
 @fn       			DisplayDefaultTitle
 @param[in]			sTitle				Title to display
 @param[in]			sDescription		Description to display
 @param[in]			uDisplayLogo		1 = Display the logo
 @return			PVR_SUCCESS or PVR_FAIL
 @brief      		Creates a default title with predefined position and colours.
					It displays as well company logos when requested:
					0 = No logo
					1 = PowerVR logo
					2 = Img Tech logo
*****************************************************************************/
EPVRTError CPVRTPrint3D::DisplayDefaultTitle(const char * const pszTitle, const char * const pszDescription, const unsigned int uDisplayLogo)
{
	EPVRTError eRet = PVR_SUCCESS;

#if !defined (DISABLE_PRINT3D)

	// Display Title
	if(pszTitle)
	{
		if(Print3D(0.0f, -1.0f, 1.0f,  PVRTRGBA(255, 255, 255, 255), pszTitle) != PVR_SUCCESS)
			eRet = PVR_FAIL;
	}
	
	float fYVal;
	if(m_bRotate)
		fYVal = m_fScreenScale[0] * 480.0f;
	else
		fYVal = m_fScreenScale[1] * 480.0f;

	// Display Description
	if(pszDescription)
	{
        float fY;
		float a = 320.0f/fYVal;
		fY = m_uiNextLineH / (480.0f/100.0f) * a;
		
		if(Print3D(0.0f, fY, 0.8f,  PVRTRGBA(255, 255, 255, 255), pszDescription) != PVR_SUCCESS)
			eRet = PVR_FAIL;
	}

	m_uLogoToDisplay = uDisplayLogo;

#endif

	return eRet;
}

/*!***************************************************************************
 @fn       			MeasureText
 @param[out]		pfWidth				Width of the string in pixels
 @param[out]		pfHeight			Height of the string in pixels
 @param[in]			fFontSize			Font size
 @param[in]			sString				String to take the size of
 @brief      		Returns the size of a string in pixels.
*****************************************************************************/
void CPVRTPrint3D::MeasureText(
	float		* const pfWidth,
	float		* const pfHeight,
	float				fScale,
	const CPVRTArray<PVRTuint32>& utf32)
{
#if !defined (DISABLE_PRINT3D)
	if(utf32.GetSize() == 0) {
		if(pfWidth)
			*pfWidth = 0;
		if(pfHeight)
			*pfHeight = 0;
		return;
	}

	float fLength			= 0;
	float fMaxLength		= -1.0f;
	float fMaxHeight		= (float)m_uiNextLineH;
	PVRTuint32 txNextChar	= 0;
	PVRTuint32 uiIdx;
	for(PVRTuint32 uiIndex = 0; uiIndex < utf32.GetSize(); uiIndex++)
	{
		if(utf32[uiIndex] == 0x0D || utf32[uiIndex] == 0x0A)
		{
			if(fLength > fMaxLength)
				fMaxLength = fLength;

			fLength = 0;
			fMaxHeight += (float)m_uiNextLineH;
		}
		uiIdx = FindCharacter(utf32[uiIndex]);
		if(uiIdx == PVRTPRINT3D_INVALID_CHAR)		// No character found. Add a space.
		{
			fLength += m_uiSpaceWidth;
			continue;
		}

		txNextChar = utf32[uiIndex + 1];
		float fKernOffset = 0;
		ApplyKerning(utf32[uiIndex], txNextChar, fKernOffset);

		fLength += m_pCharMatrics[uiIdx].nAdv + fKernOffset;		// Add on this characters width
	}

	if(fMaxLength < 0.0f)		// Obviously no new line.
		fMaxLength = fLength;

	if(pfWidth)
		*pfWidth = fMaxLength * fScale;
	if(pfHeight)
		*pfHeight = fMaxHeight * fScale;
#endif
}

/*!***************************************************************************
 @fn       			GetSize
 @param[out]		pfWidth				Width of the string in pixels
 @param[out]		pfHeight			Height of the string in pixels
 @param[in]			pszUTF8				UTF8 string to take the size of
 @brief      		Returns the size of a string in pixels.
*****************************************************************************/
void CPVRTPrint3D::MeasureText(
	float		* const pfWidth,
	float		* const pfHeight,
	float				fScale,
	const char	* const pszUTF8)
{
	m_CachedUTF32.Clear();
	PVRTUnicodeUTF8ToUTF32((PVRTuint8*)pszUTF8, m_CachedUTF32);
	MeasureText(pfWidth,pfHeight,fScale,m_CachedUTF32);
}

/*!***************************************************************************
 @fn       			MeasureText
 @param[out]		pfWidth		Width of the string in pixels
 @param[out]		pfHeight	Height of the string in pixels
 @param[in]			pszUnicode	Wide character string to take the length of.
 @brief      		Returns the size of a string in pixels.
*****************************************************************************/
void CPVRTPrint3D::MeasureText(
	float		* const pfWidth,
	float		* const pfHeight,
	float				fScale,
	const wchar_t* const pszUnicode)
{
	_ASSERT(pszUnicode);
	m_CachedUTF32.Clear();

#if PVRTSIZEOFWCHAR == 2			// 2 byte wchar.
	PVRTUnicodeUTF16ToUTF32((PVRTuint16*)pszUnicode, m_CachedUTF32);
#else								// 4 byte wchar (POSIX)
	unsigned int uiC = 0;
	PVRTuint32* pUTF32 = (PVRTuint32*)pszUnicode;
	while(*pUTF32 && uiC < MAX_LETTERS)
	{
		m_CachedUTF32.Append(*pUTF32++);
		uiC++;
	}
#endif
	
	MeasureText(pfWidth,pfHeight,fScale,m_CachedUTF32);
}

/*!***************************************************************************
 @fn       			GetAspectRatio
 @param[out]		dwScreenX		Screen resolution X
 @param[out]		dwScreenY		Screen resolution Y
 @brief      		Returns the current resolution used by Print3D
*****************************************************************************/
void CPVRTPrint3D::GetAspectRatio(unsigned int *dwScreenX, unsigned int *dwScreenY)
{
#if !defined (DISABLE_PRINT3D)

	*dwScreenX = (int)(640.0f * m_fScreenScale[0]);
	*dwScreenY = (int)(480.0f * m_fScreenScale[1]);
#endif
}

/*************************************************************
*					 PRIVATE FUNCTIONS						 *
**************************************************************/

/*!***************************************************************************
 @brief             Update a single line
 @param[in]			fZPos
 @param[in]			XPos
 @param[in]			YPos
 @param[in]			fScale
 @param[in]			Colour
 @param[in]			Text
 @param[in]			pVertices
 @return            Number of vertices affected
*****************************************************************************/
unsigned int CPVRTPrint3D::UpdateLine(const float fZPos, float XPos, float YPos, const float fScale, const unsigned int Colour, const CPVRTArray<PVRTuint32>& Text, SPVRTPrint3DAPIVertex * const pVertices)
{
	/* Nothing to update */
	if (Text.GetSize() == 0) 
		return 0;

	if(!m_bUsingProjection)
	{
		XPos *= ((float)m_ui32ScreenDim[0] / 640.0f);
		YPos *= ((float)m_ui32ScreenDim[1] / 480.0f);
	}

	YPos -= m_uiAscent * fScale;
	
	YPos = PVRTMakeWhole(YPos);

	float fPreXPos	= XPos;		// The original offset (after screen scale modification) of the X coordinate.

	float		fKernOffset;
	float		fAOff;
	float		fYOffset;
	unsigned int VertexCount = 0;
	PVRTint32 NextChar;

	unsigned int uiNumCharsInString = Text.GetSize();
	for(unsigned int uiIndex = 0; uiIndex < uiNumCharsInString; uiIndex++)
	{
		if(uiIndex > MAX_LETTERS) 
			break;

		// Newline
		if(Text[uiIndex] == 0x0A)
		{
			XPos = fPreXPos;
			YPos -= PVRTMakeWhole(m_uiNextLineH * fScale);
			continue;
		}

		// Get the character
		PVRTuint32 uiIdx = FindCharacter(Text[uiIndex]);

		// Not found. Add a space.
		if(uiIdx == PVRTPRINT3D_INVALID_CHAR)		// No character found. Add a space.
		{
			XPos += PVRTMakeWhole(m_uiSpaceWidth * fScale);
			continue;
		}

		fKernOffset = 0;
		fYOffset	= m_pYOffsets[uiIdx] * fScale;
		fAOff		= PVRTMakeWhole(m_pCharMatrics[uiIdx].nXOff * fScale);					// The A offset. Could include overhang or underhang.
		if(uiIndex < uiNumCharsInString - 1)
		{
			NextChar = Text[uiIndex + 1];
			ApplyKerning(Text[uiIndex], NextChar, fKernOffset);
		}

		/* Filling vertex data */
		pVertices[VertexCount+0].sx		= f2vt(XPos + fAOff);
		pVertices[VertexCount+0].sy		= f2vt(YPos + fYOffset);
		pVertices[VertexCount+0].sz		= f2vt(fZPos);
		pVertices[VertexCount+0].rhw	= f2vt(1.0f);
		pVertices[VertexCount+0].tu		= f2vt(m_pUVs[uiIdx].fUL);
		pVertices[VertexCount+0].tv		= f2vt(m_pUVs[uiIdx].fVT);

		pVertices[VertexCount+1].sx		= f2vt(XPos + fAOff + PVRTMakeWhole(m_pRects[uiIdx].nW * fScale));
		pVertices[VertexCount+1].sy		= f2vt(YPos + fYOffset);
		pVertices[VertexCount+1].sz		= f2vt(fZPos);
		pVertices[VertexCount+1].rhw	= f2vt(1.0f);
		pVertices[VertexCount+1].tu		= f2vt(m_pUVs[uiIdx].fUR);
		pVertices[VertexCount+1].tv		= f2vt(m_pUVs[uiIdx].fVT);

		pVertices[VertexCount+2].sx		= f2vt(XPos + fAOff);
		pVertices[VertexCount+2].sy		= f2vt(YPos + fYOffset - PVRTMakeWhole(m_pRects[uiIdx].nH * fScale));
		pVertices[VertexCount+2].sz		= f2vt(fZPos);
		pVertices[VertexCount+2].rhw	= f2vt(1.0f);
		pVertices[VertexCount+2].tu		= f2vt(m_pUVs[uiIdx].fUL);
		pVertices[VertexCount+2].tv		= f2vt(m_pUVs[uiIdx].fVB);

		pVertices[VertexCount+3].sx		= f2vt(XPos + fAOff + PVRTMakeWhole(m_pRects[uiIdx].nW * fScale));
		pVertices[VertexCount+3].sy		= f2vt(YPos + fYOffset - PVRTMakeWhole(m_pRects[uiIdx].nH * fScale));
		pVertices[VertexCount+3].sz		= f2vt(fZPos);
		pVertices[VertexCount+3].rhw	= f2vt(1.0f);
		pVertices[VertexCount+3].tu		= f2vt(m_pUVs[uiIdx].fUR);
		pVertices[VertexCount+3].tv		= f2vt(m_pUVs[uiIdx].fVB);

		pVertices[VertexCount+0].color	= Colour;
		pVertices[VertexCount+1].color	= Colour;
		pVertices[VertexCount+2].color	= Colour;
		pVertices[VertexCount+3].color	= Colour;

		XPos = XPos + PVRTMakeWhole((m_pCharMatrics[uiIdx].nAdv + fKernOffset) * fScale);		// Add on this characters width
		VertexCount += 4;
	}

	return VertexCount;
}

/*!***************************************************************************
 @fn       			DrawLineUP
 @return			true or false
 @brief      		Draw a single line of text.
*****************************************************************************/
bool CPVRTPrint3D::DrawLine(SPVRTPrint3DAPIVertex *pVtx, unsigned int nVertices)
{
	if(!nVertices)
		return true;

	_ASSERT((nVertices % 4) == 0);
	_ASSERT((nVertices/4) < MAX_LETTERS);

	while(m_nVtxCache + (int)nVertices > m_nVtxCacheMax) {
		if(m_nVtxCache + nVertices > MAX_CACHED_VTX) {
			_RPT1(_CRT_WARN, "Print3D: Out of space to cache text! (More than %d vertices!)\n", MAX_CACHED_VTX);
			return false;
		}

		m_nVtxCacheMax	= PVRT_MIN(m_nVtxCacheMax * 2, MAX_CACHED_VTX);
		SPVRTPrint3DAPIVertex* pTmp = (SPVRTPrint3DAPIVertex*)realloc(m_pVtxCache, m_nVtxCacheMax * sizeof(*m_pVtxCache));

		_ASSERT(pTmp);
		if(!pTmp)
		{
			free(m_pVtxCache);
			m_pVtxCache = 0;
			return false; // Failed to re-allocate data
		}

		m_pVtxCache = pTmp;
		
		_RPT1(_CRT_WARN, "Print3D: TextCache increased to %d vertices.\n", m_nVtxCacheMax);
	}

	memcpy(&m_pVtxCache[m_nVtxCache], pVtx, nVertices * sizeof(*pVtx));
	m_nVtxCache += nVertices;
	return true;
}

/*!***************************************************************************
 @fn       			SetProjection
 @brief      		Sets projection matrix.
*****************************************************************************/
void CPVRTPrint3D::SetProjection(const PVRTMat4& mProj)
{
	m_mProj				= mProj;
	m_bUsingProjection	= true;
}

/*!***************************************************************************
 @fn       			SetModelView
 @brief      		Sets model view matrix.
*****************************************************************************/
void CPVRTPrint3D::SetModelView(const PVRTMat4& mModelView)
{
	m_mModelView = mModelView;
}

/*!***************************************************************************
 @fn       		SetFiltering
 @param[in]		eFilter				The method of texture filtering
 @brief      	Sets the method of texture filtering for the font texture.
					Print3D will attempt to pick the best method by default
					but this method allows the user to override this.
*****************************************************************************/
void CPVRTPrint3D::SetFiltering(ETextureFilter eMin, ETextureFilter eMag, ETextureFilter eMip)
{
	if(eMin == eFilter_None) eMin = eFilter_Default;		// Illegal value
	if(eMag == eFilter_None) eMag = eFilter_Default;		// Illegal value

	m_eFilterMethod[eFilterProc_Min] = eMin;
	m_eFilterMethod[eFilterProc_Mag] = eMag;
	m_eFilterMethod[eFilterProc_Mip] = eMip;
}

/*!***************************************************************************
 @fn       		GetFontAscent
 @return		unsigned int	The ascent.
 @brief      	Returns the 'ascent' of the font. This is typically the 
				height from the baseline of the larget glyph in the set.
*****************************************************************************/
unsigned int CPVRTPrint3D::GetFontAscent()
{
	return m_uiAscent;
}

/*!***************************************************************************
 @fn       		GetFontLineSpacing
 @return		unsigned int	The line spacing.
 @brief      	Returns the default line spacing (i.e baseline to baseline) 
				for the font.
*****************************************************************************/
unsigned int CPVRTPrint3D::GetFontLineSpacing()
{
	return m_uiNextLineH;
}

/****************************************************************************
** Local code
****************************************************************************/

/*****************************************************************************
 End of file (PVRTPrint3D.cpp)
*****************************************************************************/

