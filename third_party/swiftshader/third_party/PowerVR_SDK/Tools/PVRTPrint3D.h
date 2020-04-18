/*!****************************************************************************

 @file         PVRTPrint3D.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Code to print text through the 3D interface.

******************************************************************************/
#ifndef _PVRTPRINT3D_H_
#define _PVRTPRINT3D_H_

#include "PVRTGlobal.h"
#include "PVRTError.h"
#include "PVRTMatrix.h"
#include "PVRTVector.h"
#include "PVRTArray.h"

struct MetaDataBlock;
template <typename KeyType, typename DataType>
class CPVRTMap;

/****************************************************************************
** Enums
****************************************************************************/
#define PVRTPRINT3D_MAX_RENDERABLE_LETTERS	(0xFFFF >> 2)

/*!***************************************************************************
 @enum      EPVRTPrint3DLogo
 @brief     Logo flags for DisplayDefaultTitle
*****************************************************************************/
typedef enum {
	ePVRTPrint3DLogoNone	= 0x00,
	ePVRTPrint3DLogoPowerVR = 0x02,
	ePVRTPrint3DLogoIMG		= 0x04,
	ePVRTPrint3DSDKLogo		= ePVRTPrint3DLogoPowerVR
} EPVRTPrint3DLogo;

/****************************************************************************
** Constants
****************************************************************************/
const PVRTuint32 PVRTPRINT3D_HEADER			= 0xFCFC0050;
const PVRTuint32 PVRTPRINT3D_CHARLIST		= 0xFCFC0051;
const PVRTuint32 PVRTPRINT3D_RECTS			= 0xFCFC0052;
const PVRTuint32 PVRTPRINT3D_METRICS		= 0xFCFC0053;
const PVRTuint32 PVRTPRINT3D_YOFFSET		= 0xFCFC0054;
const PVRTuint32 PVRTPRINT3D_KERNING		= 0xFCFC0055;

const PVRTuint32 PVRTPRINT3D_VERSION		= 1;

/****************************************************************************
** Structures
****************************************************************************/
/*!**************************************************************************
 @struct    SPVRTPrint3DHeader
 @brief     A structure for information describing the loaded font.
****************************************************************************/
struct SPVRTPrint3DHeader				// 12 bytes
{
	PVRTuint8	uVersion;				/*!< Version of PVRFont. */
	PVRTuint8	uSpaceWidth;			/*!< The width of the 'Space' character. */
	PVRTint16	wNumCharacters;			/*!< Total number of characters contained in this file. */
	PVRTint16	wNumKerningPairs;		/*!< Number of characters which kern against each other. */
	PVRTint16	wAscent;				/*!< The height of the character, in pixels, from the base line. */
	PVRTint16	wLineSpace;				/*!< The base line to base line dimension, in pixels. */
	PVRTint16	wBorderWidth;			/*!< px Border around each character. */
};
/*!**************************************************************************
 @struct SPVRTPrint3DAPIVertex
 @brief A structure for Print3Ds vertex type
****************************************************************************/
struct SPVRTPrint3DAPIVertex
{
	VERTTYPE		sx, sy, sz, rhw;
	unsigned int	color;
	VERTTYPE		tu, tv;
};

struct PVRTextureHeaderV3;
struct SPVRTPrint3DAPI;
struct SPVRTContext;

/*!***************************************************************************
 @class CPVRTPrint3D
 @brief Display text/logos on the screen
*****************************************************************************/
class CPVRTPrint3D
{
public:
	/*!***************************************************************************
	 @fn           		CPVRTPrint3D
	 @brief     		Init some values.
	*****************************************************************************/
	CPVRTPrint3D();
	/*!***************************************************************************
	 @fn           		~CPVRTPrint3D
	 @brief     		De-allocate the working memory
	*****************************************************************************/
	~CPVRTPrint3D();

	/*!***************************************************************************
	 @brief     		Initialization and texture upload of default font data. 
						Should be called only once for a Print3D object.
	 @param[in]			pContext		Context
	 @param[in]			dwScreenX		Screen resolution along X
	 @param[in]			dwScreenY		Screen resolution along Y
	 @param[in]			bRotate			Rotate print3D by 90 degrees
	 @param[in]			bMakeCopy		This instance of Print3D creates a copy
										of it's data instead of sharing with previous
										contexts. Set this parameter if you require
										thread safety.										
	 @return			PVR_SUCCESS or PVR_FAIL
	*****************************************************************************/
	EPVRTError SetTextures(
		const SPVRTContext	* const pContext,
		const unsigned int	dwScreenX,
		const unsigned int	dwScreenY,
		const bool bRotate = false,
		const bool bMakeCopy = false);

	/*!***************************************************************************
	 @brief     		Initialization and texture upload of user-provided font 
						data. Should be called only once for a Print3D object.
	 @param[in]			pContext		Context
	 @param[in]			pTexData		User-provided font texture
	 @param[in]			dwScreenX		Screen resolution along X
	 @param[in]			dwScreenY		Screen resolution along Y
	 @param[in]			bRotate			Rotate print3D by 90 degrees
	 @param[in]			bMakeCopy		This instance of Print3D creates a copy
										of it's data instead of sharing with previous
										contexts. Set this parameter if you require
										thread safety.	
	 @return			PVR_SUCCESS or PVR_FAIL
	*****************************************************************************/
	EPVRTError SetTextures(
		const SPVRTContext	* const pContext,
		const void * const pTexData,
		const unsigned int	dwScreenX,
		const unsigned int	dwScreenY,
		const bool bRotate = false,
		const bool bMakeCopy = false);

	/*!***************************************************************************
	 @fn           		SetProjection
	 @param[in]			mProj			Projection matrix
	 @brief     		Sets the projection matrix for the proceeding flush().
	*****************************************************************************/
	void SetProjection(const PVRTMat4& mProj);

	/*!***************************************************************************
	 @fn           		SetModelView
	 @param[in]			mModelView			Model View matrix
	 @brief     		Sets the model view matrix for the proceeding flush().
	*****************************************************************************/
	void SetModelView(const PVRTMat4& mModelView);

	/*!***************************************************************************
	 @fn           		SetFiltering
	 @param[in]			eMin	The method of texture filtering for minification
	 @param[in]			eMag	The method of texture filtering for minification
	 @param[in]			eMip	The method of texture filtering for minification
	 @brief     		Sets the method of texture filtering for the font texture.
						Print3D will attempt to pick the best method by default
						but this method allows the user to override this.
	*****************************************************************************/
	void SetFiltering(ETextureFilter eMin, ETextureFilter eMag, ETextureFilter eMip);

	/*!***************************************************************************
	 @brief     		Display 3D text on screen.
						CPVRTPrint3D::SetTextures(...) must have been called
						beforehand.
						This function accepts formatting in the printf way.
	 @param[in]			fPosX		Position of the text along X
	 @param[in]			fPosY		Position of the text along Y
	 @param[in]			fScale		Scale of the text
	 @param[in]			Colour		Colour of the text
	 @param[in]			pszFormat	Format string for the text
	 @return			PVR_SUCCESS or PVR_FAIL
	*****************************************************************************/
	EPVRTError Print3D(float fPosX, float fPosY, const float fScale, unsigned int Colour, const char * const pszFormat, ...);


	/*!***************************************************************************
	 @brief     		Display wide-char 3D text on screen.
						CPVRTPrint3D::SetTextures(...) must have been called
						beforehand.
						This function accepts formatting in the printf way.
	 @param[in]			fPosX		Position of the text along X
	 @param[in]			fPosY		Position of the text along Y
	 @param[in]			fScale		Scale of the text
	 @param[in]			Colour		Colour of the text
	 @param[in]			pszFormat	Format string for the text
	 @return			PVR_SUCCESS or PVR_FAIL
	*****************************************************************************/
	EPVRTError Print3D(float fPosX, float fPosY, const float fScale, unsigned int Colour, const wchar_t * const pszFormat, ...);

	/*!***************************************************************************
	 @fn           		DisplayDefaultTitle
	 @param[in]			pszTitle			Title to display
	 @param[in]			pszDescription		Description to display
	 @param[in]			uDisplayLogo		1 = Display the logo
	 @return			PVR_SUCCESS or PVR_FAIL
	 @brief     		Creates a default title with predefined position and colours.
						It displays as well company logos when requested:
						0 = No logo
						1 = PowerVR logo
						2 = Img Tech logo
	*****************************************************************************/
	 EPVRTError DisplayDefaultTitle(const char * const pszTitle, const char * const pszDescription, const unsigned int uDisplayLogo);

	 /*!***************************************************************************
	 @brief     		Returns the size of a string in pixels.
	 @param[out]		pfWidth				Width of the string in pixels
	 @param[out]		pfHeight			Height of the string in pixels
	 @param[in]			fScale				A value to scale the font by
	 @param[in]			pszUTF8				UTF8 string to take the size of
	*****************************************************************************/
	void MeasureText(
		float		* const pfWidth,
		float		* const pfHeight,
		float				fScale,
		const char	* const pszUTF8);

	/*!***************************************************************************
	 @brief     		Returns the size of a string in pixels.
	 @param[out]		pfWidth				Width of the string in pixels
	 @param[out]		pfHeight			Height of the string in pixels
	 @param[in]			fScale				A value to scale the font by
	 @param[in]			pszUnicode			Wide character string to take the
											length of.
	*****************************************************************************/
	void MeasureText(
		float		* const pfWidth,
		float		* const pfHeight,
		float				fScale,
		const wchar_t* const pszUnicode);

	/*!***************************************************************************
	@brief     	        Returns the 'ascent' of the font. This is typically the 
                        height from the baseline of the larget glyph in the set.
	@return			    The ascent.
	*****************************************************************************/
	unsigned int GetFontAscent();

	/*!***************************************************************************
	@brief     	    Returns the default line spacing (i.e baseline to baseline) 
					for the font.
	@return			The line spacing.
	*****************************************************************************/
	unsigned int GetFontLineSpacing();

	/*!***************************************************************************
	 @brief     		Returns the current resolution used by Print3D
	 @param[out]		dwScreenX		Screen resolution X
	 @param[out]		dwScreenY		Screen resolution Y
	*****************************************************************************/
	void GetAspectRatio(unsigned int *dwScreenX, unsigned int *dwScreenY);

	/*!***************************************************************************
	 @brief     		Deallocate the memory allocated in SetTextures(...)
	*****************************************************************************/
	void ReleaseTextures();

	/*!***************************************************************************
	 @brief     		Flushes all the print text commands
	*****************************************************************************/
	int Flush();

private:
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
	unsigned int UpdateLine(const float fZPos, float XPos, float YPos, const float fScale, const unsigned int Colour, const CPVRTArray<PVRTuint32>& Text, SPVRTPrint3DAPIVertex * const pVertices);

	/*!***************************************************************************
	 @brief     		Draw a single line of text.
	 @return			true or false
	*****************************************************************************/
	bool DrawLine(SPVRTPrint3DAPIVertex *pVtx, unsigned int nVertices);

	/*!***************************************************************************
	@fn           		LoadFontData
	@param[in]			texHeader
	@param[in]			MetaDataMap
	@return			    bool	true if successful.
	@brief     	        Loads font data bundled with the texture file.
	*****************************************************************************/
	bool LoadFontData(const PVRTextureHeaderV3* texHeader, CPVRTMap<PVRTuint32, CPVRTMap<PVRTuint32, MetaDataBlock> >& MetaDataMap);

	/*!***************************************************************************
	@fn           		ReadMetaBlock
	@param[in]			pDataCursor
	@return			    bool	true if successful.
	@brief     	        Reads a single meta data block from the data file.
	*****************************************************************************/
	bool ReadMetaBlock(const PVRTuint8** pDataCursor);

	/*!***************************************************************************
	@fn           		FindCharacter
	@param[in]			character
	@return			    The character index, or PVRPRINT3D_INVALID_CHAR if not found.
	@brief     	        Finds a given character in the binary data and returns its
                        index.
	*****************************************************************************/
	PVRTuint32 FindCharacter(PVRTuint32 character) const;

	/*!***************************************************************************
	@fn           		CharacterCompareFunc
	@param[in]			pA
	@param[in]			pB
	@return			    PVRTint32	
	@brief     	        Compares two characters for binary search.
	*****************************************************************************/
	static PVRTint32 CharacterCompareFunc(const void* pA, const void* pB);
	
	/*!***************************************************************************
	@fn           		KerningCompareFunc
	@param[in]			pA
	@param[in]			pB
	@return			    VRTint32	
	@brief     	        Compares two kerning pairs for binary search.
	*****************************************************************************/
	static PVRTint32 KerningCompareFunc(const void* pA, const void* pB);

	/*!***************************************************************************
	@fn           		ApplyKerning
	@param[in]			cA
	@param[in]			cB
	@param[out]			fOffset
	@brief     	        Calculates kerning offset.
	*****************************************************************************/
	void ApplyKerning(const PVRTuint32 cA, const PVRTuint32 cB, float& fOffset) const;

	/*!***************************************************************************
	 @brief     		Returns the size of a string in pixels.
	 @param[out]		pfWidth				Width of the string in pixels
	 @param[out]		pfHeight			Height of the string in pixels
	 @param[in]			fScale				Font size
	 @param[in]			utf32				UTF32 string to take the size of.
	*****************************************************************************/
	void MeasureText(
		float		* const pfWidth,
		float		* const pfHeight,
		float				fScale,
		const CPVRTArray<PVRTuint32>& utf32);
	
	/*!***************************************************************************
	@brief     	        Takes an array of UTF32 characters and generates the required mesh.
	@param[in]			fPosX		X Position
	@param[in]			fPosY		Y Position
	@param[in]			fScale		Text scale
	@param[in]			Colour		ARGB colour
	@param[in]			UTF32		Array of UTF32 characters
	@param[in]			bUpdate		Whether to update the vertices
	@return			    EPVRTError	Success of failure
	*****************************************************************************/
	EPVRTError Print3D(float fPosX, float fPosY, const float fScale, unsigned int Colour, const CPVRTArray<PVRTuint32>& UTF32, bool bUpdate);

//***************************************************************************
// Structures and enums for font data
// The following structures are used to provide layout information for associated fonts.
//*****************************************************************************/
private:
	struct CharacterUV
	{
		PVRTfloat32 fUL;
		PVRTfloat32 fVT;
		PVRTfloat32 fUR;
		PVRTfloat32 fVB;
	};

	struct Rectanglei
	{
		PVRTint32 nX;
		PVRTint32 nY;
		PVRTint32 nW;
		PVRTint32 nH;
	};

#pragma pack(push, 4)		// Force 4byte alignment.
	struct KerningPair
	{
		PVRTuint64 uiPair;			/*!< OR'd pair for 32bit characters */
		PVRTint32  iOffset;			/*!< Kerning offset (in pixels) */
	};
#pragma pack(pop)

	struct CharMetrics
	{
		PVRTint16  nXOff;			/*!< Prefix offset */
		PVRTuint16 nAdv;			/*!< Character width */
	};

	enum
	{
		eFilterProc_Min,
		eFilterProc_Mag,
		eFilterProc_Mip,

		eFilterProc_Size
	};

	enum ELogoPos
	{
		eBottom = 0x01,
		eTop = 0x02,
		eLeft = 0x04,
		eRight = 0x08
	};

private:
	// Mesh parameters
	SPVRTPrint3DAPI			*m_pAPI;
	unsigned int			m_uLogoToDisplay;
	unsigned short			*m_pwFacesFont;
	SPVRTPrint3DAPIVertex	*m_pPrint3dVtx;
	float					m_fScreenScale[2];
	unsigned int			m_ui32ScreenDim[2];
	bool					m_bTexturesSet;
	SPVRTPrint3DAPIVertex	*m_pVtxCache;
	int						m_nVtxCache;
	int						m_nVtxCacheMax;
	bool					m_bRotate;

	// Cached memory
	CPVRTArray<PVRTuint32>	m_CachedUTF32;
	int						m_nCachedNumVerts;
	wchar_t*				m_pwzPreviousString;
	char*					m_pszPreviousString;
	float					m_fPrevScale;
	float					m_fPrevX;
	float					m_fPrevY;
	unsigned int			m_uiPrevCol;

	// Font parameters
	CharacterUV*			m_pUVs;
	KerningPair*			m_pKerningPairs;
	CharMetrics*			m_pCharMatrics;
	
	float					m_fTexW;
	float					m_fTexH;

	Rectanglei*				m_pRects;
	int*					m_pYOffsets;
	int						m_uiNextLineH;

	unsigned int			m_uiSpaceWidth;	
	unsigned int			m_uiNumCharacters;
	unsigned int			m_uiNumKerningPairs;
	unsigned int			m_uiAscent;
	PVRTuint32*				m_pszCharacterList;
	bool					m_bHasMipmaps;
	
	// View parameters
	PVRTMat4				m_mProj;
	PVRTMat4				m_mModelView;
	bool					m_bUsingProjection;
	ETextureFilter			m_eFilterMethod[eFilterProc_Size];

//***************************************************************************
//	API specific code
//  The following functions are API specific. Their implementation
//	can be found in the directory *CurrentAPI*\PVRTPrint3DAPI
//*****************************************************************************/
private:
	/*!***************************************************************************
	 @fn           		APIInit
	 @param[in]			pContext
	 @param[in]			bMakeCopy
	 @return			true or false
	 @brief     		Initialization and texture upload. Should be called only once
						for a given context.
	*****************************************************************************/
	bool APIInit(const SPVRTContext	* const pContext, bool bMakeCopy);

	/*!***************************************************************************
	 @fn           		APIRelease
	 @brief     		Deinitialization.
	*****************************************************************************/
	void APIRelease();

	/*!***************************************************************************
	 @fn           		APIUpLoadIcons
	 @param[in]			pIMG
	 @return			true or false
	 @brief     		Initialization and texture upload. Should be called only once
						for a given context.
	*****************************************************************************/
	bool APIUpLoadIcons(const PVRTuint8 * const pIMG, const PVRTuint8 * const pPowerVR);

	/*!***************************************************************************
	 @fn           		APIUpLoadTexture
	 @param[in]			pSource
	 @param[in]			header
	 @param[in]			MetaDataMap
	 @return			true if successful, false otherwise.
	 @brief     		Reads texture data from *.dat and loads it in
						video memory.
	*****************************************************************************/
	bool APIUpLoadTexture(const PVRTuint8* pSource, const PVRTextureHeaderV3* header, CPVRTMap<PVRTuint32, CPVRTMap<PVRTuint32, MetaDataBlock> >& MetaDataMap);


	/*!***************************************************************************
	 @fn           		APIRenderStates
	 @param[in]			nAction
	 @brief     		Stores, writes and restores Render States
	*****************************************************************************/
	void APIRenderStates(int nAction);

	/*!***************************************************************************
	 @fn           		APIDrawLogo
	 @param[in]			uLogoToDisplay
	 @param[in]			nPod
	 @brief     		nPos = -1 to the left
						nPos = +1 to the right
	*****************************************************************************/
	void APIDrawLogo(const EPVRTPrint3DLogo uLogoToDisplay, const int ePos);
};


#endif /* _PVRTPRINT3D_H_ */

/*****************************************************************************
 End of file (PVRTPrint3D.h)
*****************************************************************************/
