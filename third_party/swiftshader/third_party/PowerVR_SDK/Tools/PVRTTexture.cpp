/******************************************************************************

 @File         PVRTTexture.cpp

 @Title        PVRTTexture

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Texture loading.

******************************************************************************/
#include <string.h>
#include <stdlib.h>

#include "PVRTTexture.h"
#include "PVRTMap.h"

/*****************************************************************************
** Functions
*****************************************************************************/
/*!***************************************************************************
@Function		ReadFromPtr
@Input			pDataCursor		The data to read
@Description	Reads from a pointer of memory in to the meta data block.
*****************************************************************************/
bool MetaDataBlock::ReadFromPtr(const unsigned char** pDataCursor)
{
	memcpy(&DevFOURCC,	 *pDataCursor, sizeof(PVRTuint32));		*pDataCursor += sizeof(PVRTuint32);
	memcpy(&u32Key,		 *pDataCursor, sizeof(PVRTuint32));		*pDataCursor += sizeof(PVRTuint32);
	memcpy(&u32DataSize, *pDataCursor, sizeof(PVRTuint32));		*pDataCursor += sizeof(PVRTuint32);
	if(u32DataSize > 0)
	{
		Data = new PVRTuint8[u32DataSize];
		memcpy(Data, *pDataCursor, u32DataSize);
		*pDataCursor += u32DataSize;
	}

	return true;
}

/*!***************************************************************************
@Function		PVRTTextureLoadTiled
@Modified		pDst			Texture to place the tiled data
@Input			nWidthDst		Width of destination texture
@Input			nHeightDst		Height of destination texture
@Input			pSrc			Texture to tile
@Input			nWidthSrc		Width of source texture
@Input			nHeightSrc		Height of source texture
@Input 			nElementSize	Bytes per pixel
@Input			bTwiddled		True if the data is twiddled
@Description	Needed by PVRTTextureTile() in the various PVRTTextureAPIs
*****************************************************************************/
void PVRTTextureLoadTiled(
	PVRTuint8		* const pDst,
	const unsigned int	nWidthDst,
	const unsigned int	nHeightDst,
	const PVRTuint8	* const pSrc,
	const unsigned int	nWidthSrc,
	const unsigned int	nHeightSrc,
	const unsigned int	nElementSize,
	const bool			bTwiddled)
{
	unsigned int nXs, nYs;
	unsigned int nXd, nYd;
	unsigned int nIdxSrc, nIdxDst;

	for(nIdxDst = 0; nIdxDst < nWidthDst*nHeightDst; ++nIdxDst)
	{
		if(bTwiddled)
		{
			PVRTTextureDeTwiddle(nXd, nYd, nIdxDst);
		}
		else
		{
			nXd = nIdxDst % nWidthDst;
			nYd = nIdxDst / nWidthDst;
		}

		nXs = nXd % nWidthSrc;
		nYs = nYd % nHeightSrc;

		if(bTwiddled)
		{
			PVRTTextureTwiddle(nIdxSrc, nXs, nYs);
		}
		else
		{
			nIdxSrc = nYs * nWidthSrc + nXs;
		}

		memcpy(pDst + nIdxDst*nElementSize, pSrc + nIdxSrc*nElementSize, nElementSize);
	}
}

/*!***************************************************************************
@Function		PVRTTextureCreate
@Input			w			Size of the texture
@Input			h			Size of the texture
@Input			wMin		Minimum size of a texture level
@Input			hMin		Minimum size of a texture level
@Input			nBPP		Bits per pixel of the format
@Input			bMIPMap		Create memory for MIP-map levels also?
@Return			Allocated texture memory (must be free()d)
@Description	Creates a PVRTextureHeaderV3 structure, including room for
				the specified texture, in memory.
*****************************************************************************/
PVRTextureHeaderV3 *PVRTTextureCreate(
	const unsigned int	w,
	const unsigned int	h,
	const unsigned int	wMin,
	const unsigned int	hMin,
	const unsigned int	nBPP,
	const bool			bMIPMap)
{
	size_t			len;
	unsigned char	*p;

	{
		unsigned int	wTmp = w, hTmp = h;

		len = 0;
		do
		{
			len += PVRT_MAX(wTmp, wMin) * PVRT_MAX(hTmp, hMin);
			wTmp >>= 1;
			hTmp >>= 1;
		}
		while(bMIPMap && (wTmp || hTmp));
	}

	len = (len * nBPP) / 8;
	len += PVRTEX3_HEADERSIZE;

	p = (unsigned char*)malloc(len);
	_ASSERT(p);

	if(p)
	{
		PVRTextureHeaderV3 * const psTexHeader = (PVRTextureHeaderV3*)p;

		*psTexHeader=PVRTextureHeaderV3();

		psTexHeader->u32Width=w;
		psTexHeader->u32Height=h;

		return psTexHeader;
	}
	else
	{
		return 0;
	}
}


/*!***************************************************************************
 @Function		PVRTTextureTwiddle
 @Output		a	Twiddled value
 @Input			u	Coordinate axis 0
 @Input			v	Coordinate axis 1
 @Description	Combine a 2D coordinate into a twiddled value
*****************************************************************************/
void PVRTTextureTwiddle(unsigned int &a, const unsigned int u, const unsigned int v)
{
	_ASSERT(!((u|v) & 0xFFFF0000));
	a = 0;
	for(int i = 0; i < 16; ++i)
	{
		a |= ((u & (1 << i)) << (i+1));
		a |= ((v & (1 << i)) << (i+0));
	}
}

/*!***************************************************************************
 @Function		PVRTTextureDeTwiddle
 @Output		u	Coordinate axis 0
 @Output		v	Coordinate axis 1
 @Input			a	Twiddled value
 @Description	Extract 2D coordinates from a twiddled value.
*****************************************************************************/
void PVRTTextureDeTwiddle(unsigned int &u, unsigned int &v, const unsigned int a)
{
	u = 0;
	v = 0;
	for(int i = 0; i < 16; ++i)
	{
		u |= (a & (1 << ((2*i)+1))) >> (i+1);
		v |= (a & (1 << ((2*i)+0))) >> (i+0);
	}
}

/*!***********************************************************************
 @Function		PVRTGetBitsPerPixel
 @Input			u64PixelFormat			A PVR Pixel Format ID.
 @Return		const PVRTuint32	Number of bits per pixel.
 @Description	Returns the number of bits per pixel in a PVR Pixel Format 
				identifier.
*************************************************************************/
PVRTuint32 PVRTGetBitsPerPixel(PVRTuint64 u64PixelFormat)
{
	if((u64PixelFormat&PVRTEX_PFHIGHMASK)!=0)
	{
		PVRTuint8* PixelFormatChar=(PVRTuint8*)&u64PixelFormat;
		return PixelFormatChar[4]+PixelFormatChar[5]+PixelFormatChar[6]+PixelFormatChar[7];
	}
	else
	{
		switch (u64PixelFormat)
		{
		case ePVRTPF_BW1bpp:
			return 1;
		case ePVRTPF_PVRTCI_2bpp_RGB:
		case ePVRTPF_PVRTCI_2bpp_RGBA:
		case ePVRTPF_PVRTCII_2bpp:
			return 2;
		case ePVRTPF_PVRTCI_4bpp_RGB:
		case ePVRTPF_PVRTCI_4bpp_RGBA:
		case ePVRTPF_PVRTCII_4bpp:
		case ePVRTPF_ETC1:
		case ePVRTPF_EAC_R11:
		case ePVRTPF_ETC2_RGB:	
		case ePVRTPF_ETC2_RGB_A1:
		case ePVRTPF_DXT1:
		case ePVRTPF_BC4:
			return 4;
		case ePVRTPF_DXT2:
		case ePVRTPF_DXT3:
		case ePVRTPF_DXT4:
		case ePVRTPF_DXT5:
		case ePVRTPF_BC5:
		case ePVRTPF_EAC_RG11:
		case ePVRTPF_ETC2_RGBA:
			return 8;
		case ePVRTPF_YUY2:
		case ePVRTPF_UYVY:
		case ePVRTPF_RGBG8888:
		case ePVRTPF_GRGB8888:
			return 16;
		case ePVRTPF_SharedExponentR9G9B9E5:
			return 32;
		case ePVRTPF_NumCompressedPFs:
			return 0;
		}
	}
	return 0;
}

/*!***********************************************************************
 @Function		PVRTGetFormatMinDims
 @Input			u64PixelFormat	A PVR Pixel Format ID.
 @Modified		minX			Returns the minimum width.
 @Modified		minY			Returns the minimum height.
 @Modified		minZ			Returns the minimum depth.
 @Description	Gets the minimum dimensions (x,y,z) for a given pixel format.
*************************************************************************/
void PVRTGetFormatMinDims(PVRTuint64 u64PixelFormat, PVRTuint32 &minX, PVRTuint32 &minY, PVRTuint32 &minZ)
{
	switch(u64PixelFormat)
	{
	case ePVRTPF_DXT1:
	case ePVRTPF_DXT2:
	case ePVRTPF_DXT3:
	case ePVRTPF_DXT4:
	case ePVRTPF_DXT5:
	case ePVRTPF_BC4:
	case ePVRTPF_BC5:
	case ePVRTPF_ETC1:
	case ePVRTPF_ETC2_RGB:
	case ePVRTPF_ETC2_RGBA:
	case ePVRTPF_ETC2_RGB_A1:
	case ePVRTPF_EAC_R11:
	case ePVRTPF_EAC_RG11:
		minX = 4;
		minY = 4;
		minZ = 1;
		break;
	case ePVRTPF_PVRTCI_4bpp_RGB:
	case ePVRTPF_PVRTCI_4bpp_RGBA:
		minX = 8;
		minY = 8;
		minZ = 1;
		break;
	case ePVRTPF_PVRTCI_2bpp_RGB:
	case ePVRTPF_PVRTCI_2bpp_RGBA:
		minX = 16;
		minY = 8;
		minZ = 1;
		break;
	case ePVRTPF_PVRTCII_4bpp:
		minX = 4;
		minY = 4;
		minZ = 1;
		break;
	case ePVRTPF_PVRTCII_2bpp:
		minX = 8;
		minY = 4;
		minZ = 1;
		break;
	case ePVRTPF_UYVY:
	case ePVRTPF_YUY2:
	case ePVRTPF_RGBG8888:
	case ePVRTPF_GRGB8888:
		minX = 2;
		minY = 1;
		minZ = 1;
		break;
	case ePVRTPF_BW1bpp:
		minX = 8;
		minY = 1;
		minZ = 1;
		break;
	default: //Non-compressed formats all return 1.
		minX = 1;
		minY = 1;
		minZ = 1;
		break;
	}
}

/*!***********************************************************************
@Function		PVRTGetTextureDataSize
@Input			iMipLevel	Specifies a mip level to check, 'PVRTEX_ALLMIPLEVELS'
							can be passed to get the size of all MIP levels. 
@Input			bAllSurfs	Size of all surfaces is calculated if true, 
							only a single surface if false.
@Input			bAllFaces	Size of all faces is calculated if true, 
							only a single face if false.
@Return			PVRTuint32		Size in BYTES of the specified texture area.
@Description	Gets the size in BYTES of the texture, given various input 
				parameters.	User can retrieve the size of either all 
				surfaces or a single surface, all faces or a single face and
				all MIP-Maps or a single specified MIP level.
*************************************************************************/
PVRTuint32 PVRTGetTextureDataSize(PVRTextureHeaderV3 sTextureHeader, PVRTint32 iMipLevel, bool bAllSurfaces, bool bAllFaces)
{
	//The smallest divisible sizes for a pixel format
	PVRTuint32 uiSmallestWidth=1;
	PVRTuint32 uiSmallestHeight=1;
	PVRTuint32 uiSmallestDepth=1;

	PVRTuint64 PixelFormatPartHigh = sTextureHeader.u64PixelFormat&PVRTEX_PFHIGHMASK;
	
	//If the pixel format is compressed, get the pixel format's minimum dimensions.
	if (PixelFormatPartHigh==0)
	{
		PVRTGetFormatMinDims((EPVRTPixelFormat)sTextureHeader.u64PixelFormat, uiSmallestWidth, uiSmallestHeight, uiSmallestDepth);
	}

	//Needs to be 64-bit integer to support 16kx16k and higher sizes.
	PVRTuint64 uiDataSize = 0;
	if (iMipLevel==-1)
	{
		for (PVRTuint8 uiCurrentMIP = 0; uiCurrentMIP<sTextureHeader.u32MIPMapCount; ++uiCurrentMIP)
		{
			//Get the dimensions of the current MIP Map level.
			PVRTuint32 uiWidth = PVRT_MAX(1,sTextureHeader.u32Width>>uiCurrentMIP);
			PVRTuint32 uiHeight = PVRT_MAX(1,sTextureHeader.u32Height>>uiCurrentMIP);
			PVRTuint32 uiDepth = PVRT_MAX(1,sTextureHeader.u32Depth>>uiCurrentMIP);

			//If pixel format is compressed, the dimensions need to be padded.
			if (PixelFormatPartHigh==0)
			{
				uiWidth=uiWidth+( (-1*uiWidth)%uiSmallestWidth);
				uiHeight=uiHeight+( (-1*uiHeight)%uiSmallestHeight);
				uiDepth=uiDepth+( (-1*uiDepth)%uiSmallestDepth);
			}

			//Add the current MIP Map's data size to the total.
			uiDataSize+=(PVRTuint64)PVRTGetBitsPerPixel(sTextureHeader.u64PixelFormat)*(PVRTuint64)uiWidth*(PVRTuint64)uiHeight*(PVRTuint64)uiDepth;
		}
	}
	else
	{
		//Get the dimensions of the specified MIP Map level.
		PVRTuint32 uiWidth = PVRT_MAX(1,sTextureHeader.u32Width>>iMipLevel);
		PVRTuint32 uiHeight = PVRT_MAX(1,sTextureHeader.u32Height>>iMipLevel);
		PVRTuint32 uiDepth = PVRT_MAX(1,sTextureHeader.u32Depth>>iMipLevel);

		//If pixel format is compressed, the dimensions need to be padded.
		if (PixelFormatPartHigh==0)
		{
			uiWidth=uiWidth+( (-1*uiWidth)%uiSmallestWidth);
			uiHeight=uiHeight+( (-1*uiHeight)%uiSmallestHeight);
			uiDepth=uiDepth+( (-1*uiDepth)%uiSmallestDepth);
		}

		//Work out the specified MIP Map's data size
		uiDataSize=PVRTGetBitsPerPixel(sTextureHeader.u64PixelFormat)*uiWidth*uiHeight*uiDepth;
	}
	
	//The number of faces/surfaces to register the size of.
	PVRTuint32 numfaces = ((bAllFaces)?(sTextureHeader.u32NumFaces):(1));
	PVRTuint32 numsurfs = ((bAllSurfaces)?(sTextureHeader.u32NumSurfaces):(1));

	//Multiply the data size by number of faces and surfaces specified, and return.
	return (PVRTuint32)(uiDataSize/8)*numsurfs*numfaces;
}

/*!***********************************************************************
 @Function		PVRTConvertOldTextureHeaderToV3
 @Input			LegacyHeader	Legacy header for conversion.
 @Modified		NewHeader		New header to output into.
 @Modified		MetaData		MetaData Map to output into.
 @Description	Converts a legacy texture header (V1 or V2) to a current 
				generation header (V3)
*************************************************************************/
void PVRTConvertOldTextureHeaderToV3(const PVR_Texture_Header* LegacyHeader, PVRTextureHeaderV3& NewHeader, CPVRTMap<PVRTuint32, CPVRTMap<PVRTuint32,MetaDataBlock> >* pMetaData)
{
	//Setup variables
	bool isPreMult;
	PVRTuint64 ptNew;
	EPVRTColourSpace cSpaceNew;
	EPVRTVariableType chanTypeNew;

	//Map the old enum to the new format.
	PVRTMapLegacyTextureEnumToNewFormat((PVRTPixelType)(LegacyHeader->dwpfFlags&0xff),ptNew,cSpaceNew,chanTypeNew,isPreMult);

	//Check if this is a cube map.
	bool isCubeMap = (LegacyHeader->dwpfFlags&PVRTEX_CUBEMAP)!=0;

	//Setup the new header.
	NewHeader.u64PixelFormat=ptNew;
	NewHeader.u32ChannelType=chanTypeNew;
	NewHeader.u32ColourSpace=cSpaceNew;
	NewHeader.u32Depth=1;
	NewHeader.u32Flags=isPreMult?PVRTEX3_PREMULTIPLIED:0;
	NewHeader.u32Height=LegacyHeader->dwHeight;
	NewHeader.u32MetaDataSize=0;
	NewHeader.u32MIPMapCount=(LegacyHeader->dwpfFlags&PVRTEX_MIPMAP?LegacyHeader->dwMipMapCount+1:1); //Legacy headers have a MIP Map count of 0 if there is only the top level. New Headers have a count of 1.
	NewHeader.u32NumFaces=(isCubeMap?6:1);

	//Only compute the number of surfaces if it's a V2 header, else default to 1 surface.
	if (LegacyHeader->dwHeaderSize==sizeof(PVR_Texture_Header))
		NewHeader.u32NumSurfaces=(LegacyHeader->dwNumSurfs/(isCubeMap?6:1));
	else
		NewHeader.u32NumSurfaces=1;

	NewHeader.u32Version=PVRTEX3_IDENT;
	NewHeader.u32Width=LegacyHeader->dwWidth;

	//Clear any currently stored MetaData, or it will be inaccurate.
	if (pMetaData)
	{
		pMetaData->Clear();
	}

	//Check if this is a normal map.
	if (LegacyHeader->dwpfFlags&PVRTEX_BUMPMAP && pMetaData)
	{
		//Get a reference to the correct block.
		MetaDataBlock& mbBumpData=(*pMetaData)[PVRTEX_CURR_IDENT][ePVRTMetaDataBumpData];

		//Set up the block.
		mbBumpData.DevFOURCC=PVRTEX_CURR_IDENT;
		mbBumpData.u32Key=ePVRTMetaDataBumpData;
		mbBumpData.u32DataSize=8;
		mbBumpData.Data=new PVRTuint8[8];

		//Setup the data for the block.
		float bumpScale = 1.0f;
		const char* bumpOrder = "xyz";

		//Copy the bumpScale into the data.
		memcpy(mbBumpData.Data,&bumpScale,4);

		//Clear the string
		memset(mbBumpData.Data+4,0,4);

		//Copy the bumpOrder into the data.
		memcpy(mbBumpData.Data+4, bumpOrder,3);

		//Increment the meta data size.
		NewHeader.u32MetaDataSize+=(12+mbBumpData.u32DataSize);
	}

	//Check if for vertical flip orientation.
	if (LegacyHeader->dwpfFlags&PVRTEX_VERTICAL_FLIP && pMetaData)
	{
		//Get the correct meta data block
		MetaDataBlock& mbTexOrientation=(*pMetaData)[PVRTEX_CURR_IDENT][ePVRTMetaDataTextureOrientation];

		//Set the block up.
		mbTexOrientation.u32DataSize=3;
		mbTexOrientation.Data=new PVRTuint8[3];
		mbTexOrientation.DevFOURCC=PVRTEX_CURR_IDENT;
		mbTexOrientation.u32Key=ePVRTMetaDataTextureOrientation;	

		//Initialise the block to default orientation.
		memset(mbTexOrientation.Data,0,3);

		//Set the block oriented upwards.
		mbTexOrientation.Data[ePVRTAxisY]=ePVRTOrientUp;

		//Increment the meta data size.
		NewHeader.u32MetaDataSize+=(12+mbTexOrientation.u32DataSize);
	}
}

/*!***********************************************************************
 @Function		PVRTMapLegacyTextureEnumToNewFormat
 @Input			OldFormat		Legacy Enumeration Value
 @Modified		newType			New PixelType identifier.
 @Modified		newCSpace		New ColourSpace
 @Modified		newChanType		New Channel Type
 @Modified		isPreMult		Whether format is pre-multiplied
 @Description	Maps a legacy enumeration value to the new PVR3 style format.
*************************************************************************/
void PVRTMapLegacyTextureEnumToNewFormat(PVRTPixelType OldFormat, PVRTuint64& newType, EPVRTColourSpace& newCSpace, EPVRTVariableType& newChanType, bool& isPreMult)
{
	//Default value.
	isPreMult=false;

	switch (OldFormat)
	{
	case MGLPT_ARGB_4444: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',4,4,4,4);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case MGLPT_ARGB_1555: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',1,5,5,5);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case MGLPT_RGB_565: 
		{
			newType=PVRTGENPIXELID3('r','g','b',5,6,5);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case MGLPT_RGB_555: 
		{
			newType=PVRTGENPIXELID4('x','r','g','b',1,5,5,5);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case MGLPT_RGB_888: 
		{
			newType=PVRTGENPIXELID3('r','g','b',8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case MGLPT_ARGB_8888: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case MGLPT_ARGB_8332: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',8,3,3,2);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case MGLPT_I_8: 
		{
			newType=PVRTGENPIXELID1('i',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case MGLPT_AI_88: 
		{
			newType=PVRTGENPIXELID2('a','i',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case MGLPT_1_BPP: 
		{
			newType=ePVRTPF_BW1bpp;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case MGLPT_VY1UY0: 
		{
			newType=ePVRTPF_YUY2;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case MGLPT_Y1VY0U: 
		{
			newType=ePVRTPF_UYVY;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case MGLPT_PVRTC2: 
		{
			newType=ePVRTPF_PVRTCI_2bpp_RGBA;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case MGLPT_PVRTC4: 
		{
			newType=ePVRTPF_PVRTCI_4bpp_RGBA;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_RGBA_4444: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',4,4,4,4);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case OGL_RGBA_5551: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',5,5,5,1);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case OGL_RGBA_8888: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_RGB_565: 
		{
			newType=PVRTGENPIXELID3('r','g','b',5,6,5);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case OGL_RGB_555: 
		{
			newType=PVRTGENPIXELID4('r','g','b','x',5,5,5,1);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case OGL_RGB_888: 
		{
			newType=PVRTGENPIXELID3('r','g','b',8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_I_8: 
		{
			newType=PVRTGENPIXELID1('l',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_AI_88: 
		{
			newType=PVRTGENPIXELID2('l','a',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_PVRTC2: 
		{
			newType=ePVRTPF_PVRTCI_2bpp_RGBA;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_PVRTC4: 
		{
			newType=ePVRTPF_PVRTCI_4bpp_RGBA;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_BGRA_8888: 
		{
			newType=PVRTGENPIXELID4('b','g','r','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_A_8: 
		{
			newType=PVRTGENPIXELID1('a',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_PVRTCII4: 
		{
			newType=ePVRTPF_PVRTCII_4bpp;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case OGL_PVRTCII2: 
		{
			newType=ePVRTPF_PVRTCII_2bpp;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

#ifdef _WIN32
	case D3D_DXT1: 
		{
			newType=ePVRTPF_DXT1;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case D3D_DXT2: 
		{
			newType=ePVRTPF_DXT2;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case D3D_DXT3: 
		{
			newType=ePVRTPF_DXT3;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case D3D_DXT4: 
		{
			newType=ePVRTPF_DXT4;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case D3D_DXT5: 
		{
			newType=ePVRTPF_DXT5;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

#endif
	case D3D_RGB_332: 
		{
			newType=PVRTGENPIXELID3('r','g','b',3,3,2);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_AL_44: 
		{
			newType=PVRTGENPIXELID2('a','l',4,4);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_LVU_655: 
		{
			newType=PVRTGENPIXELID3('l','g','r',6,5,5);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedIntegerNorm;
			break;
		}

	case D3D_XLVU_8888: 
		{
			newType=PVRTGENPIXELID4('x','l','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedIntegerNorm;
			break;
		}

	case D3D_QWVU_8888: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedIntegerNorm;
			break;
		}

	case D3D_ABGR_2101010: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',2,10,10,10);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_ARGB_2101010: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',2,10,10,10);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_AWVU_2101010: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',2,10,10,10);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_GR_1616: 
		{
			newType=PVRTGENPIXELID2('g','r',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_VU_1616: 
		{
			newType=PVRTGENPIXELID2('g','r',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedIntegerNorm;
			break;
		}

	case D3D_ABGR_16161616: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',16,16,16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_R16F: 
		{
			newType=PVRTGENPIXELID1('r',16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case D3D_GR_1616F: 
		{
			newType=PVRTGENPIXELID2('g','r',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case D3D_ABGR_16161616F: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',16,16,16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case D3D_R32F: 
		{
			newType=PVRTGENPIXELID1('r',32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case D3D_GR_3232F: 
		{
			newType=PVRTGENPIXELID2('g','r',32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case D3D_ABGR_32323232F: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',32,32,32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case ETC_RGB_4BPP: 
		{
			newType=ePVRTPF_ETC1;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case D3D_A8: 
		{
			newType=PVRTGENPIXELID1('a',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_V8U8: 
		{
			newType=PVRTGENPIXELID2('g','r',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedIntegerNorm;
			break;
		}

	case D3D_L16: 
		{
			newType=PVRTGENPIXELID1('l',16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_L8: 
		{
			newType=PVRTGENPIXELID1('l',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_AL_88: 
		{
			newType=PVRTGENPIXELID2('a','l',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case D3D_UYVY: 
		{
			newType=ePVRTPF_UYVY;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case D3D_YUY2: 
		{
			newType=ePVRTPF_YUY2;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_R32G32B32A32_FLOAT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',32,32,32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R32G32B32A32_UINT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',32,32,32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedInteger;
			break;
		}

	case DX10_R32G32B32A32_SINT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',32,32,32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedInteger;
			break;
		}

	case DX10_R32G32B32_FLOAT: 
		{
			newType=PVRTGENPIXELID3('r','g','b',32,32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R32G32B32_UINT: 
		{
			newType=PVRTGENPIXELID3('r','g','b',32,32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedInteger;
			break;
		}

	case DX10_R32G32B32_SINT: 
		{
			newType=PVRTGENPIXELID3('r','g','b',32,32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedInteger;
			break;
		}

	case DX10_R16G16B16A16_FLOAT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',16,16,16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R16G16B16A16_UNORM: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',16,16,16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case DX10_R16G16B16A16_UINT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',16,16,16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShort;
			break;
		}

	case DX10_R16G16B16A16_SNORM: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',16,16,16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedShortNorm;
			break;
		}

	case DX10_R16G16B16A16_SINT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',16,16,16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedShort;
			break;
		}

	case DX10_R32G32_FLOAT: 
		{
			newType=PVRTGENPIXELID2('r','g',32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R32G32_UINT: 
		{
			newType=PVRTGENPIXELID2('r','g',32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedInteger;
			break;
		}

	case DX10_R32G32_SINT: 
		{
			newType=PVRTGENPIXELID2('r','g',32,32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedInteger;
			break;
		}

	case DX10_R10G10B10A2_UNORM: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',10,10,10,2);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_R10G10B10A2_UINT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',10,10,10,2);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedInteger;
			break;
		}

	case DX10_R11G11B10_FLOAT: 
		{
			newType=PVRTGENPIXELID3('r','g','b',11,11,10);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R8G8B8A8_UNORM: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_R8G8B8A8_UNORM_SRGB: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_R8G8B8A8_UINT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByte;
			break;
		}

	case DX10_R8G8B8A8_SNORM: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedByteNorm;
			break;
		}

	case DX10_R8G8B8A8_SINT: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedByte;
			break;
		}

	case DX10_R16G16_FLOAT: 
		{
			newType=PVRTGENPIXELID2('r','g',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R16G16_UNORM: 
		{
			newType=PVRTGENPIXELID2('r','g',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case DX10_R16G16_UINT: 
		{
			newType=PVRTGENPIXELID2('r','g',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShort;
			break;
		}

	case DX10_R16G16_SNORM: 
		{
			newType=PVRTGENPIXELID2('r','g',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedShortNorm;
			break;
		}

	case DX10_R16G16_SINT: 
		{
			newType=PVRTGENPIXELID2('r','g',16,16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedShort;
			break;
		}

	case DX10_R32_FLOAT: 
		{
			newType=PVRTGENPIXELID1('r',32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R32_UINT: 
		{
			newType=PVRTGENPIXELID1('r',32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedInteger;
			break;
		}

	case DX10_R32_SINT: 
		{
			newType=PVRTGENPIXELID1('r',32);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedInteger;
			break;
		}

	case DX10_R8G8_UNORM: 
		{
			newType=PVRTGENPIXELID2('r','g',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_R8G8_UINT: 
		{
			newType=PVRTGENPIXELID2('r','g',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByte;
			break;
		}

	case DX10_R8G8_SNORM: 
		{
			newType=PVRTGENPIXELID2('r','g',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedByteNorm;
			break;
		}

	case DX10_R8G8_SINT: 
		{
			newType=PVRTGENPIXELID2('r','g',8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedByte;
			break;
		}

	case DX10_R16_FLOAT: 
		{
			newType=PVRTGENPIXELID1('r',16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R16_UNORM: 
		{
			newType=PVRTGENPIXELID1('r',16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case DX10_R16_UINT: 
		{
			newType=PVRTGENPIXELID1('r',16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedShort;
			break;
		}

	case DX10_R16_SNORM: 
		{
			newType=PVRTGENPIXELID1('r',16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedShortNorm;
			break;
		}

	case DX10_R16_SINT: 
		{
			newType=PVRTGENPIXELID1('r',16);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedShort;
			break;
		}

	case DX10_R8_UNORM: 
		{
			newType=PVRTGENPIXELID1('r',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_R8_UINT: 
		{
			newType=PVRTGENPIXELID1('r',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByte;
			break;
		}

	case DX10_R8_SNORM: 
		{
			newType=PVRTGENPIXELID1('r',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedByteNorm;
			break;
		}

	case DX10_R8_SINT: 
		{
			newType=PVRTGENPIXELID1('r',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedByte;
			break;
		}

	case DX10_A8_UNORM: 
		{
			newType=PVRTGENPIXELID1('r',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_R1_UNORM: 
		{
			newType=ePVRTPF_BW1bpp;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_R9G9B9E5_SHAREDEXP: 
		{
			newType=ePVRTPF_SharedExponentR9G9B9E5;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedFloat;
			break;
		}

	case DX10_R8G8_B8G8_UNORM: 
		{
			newType=ePVRTPF_RGBG8888;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case DX10_G8R8_G8B8_UNORM: 
		{
			newType=ePVRTPF_GRGB8888;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

#ifdef _WIN32
	case DX10_BC1_UNORM: 
		{
			newType=ePVRTPF_DXT1;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC1_UNORM_SRGB: 
		{
			newType=ePVRTPF_DXT1;
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC2_UNORM: 
		{
			newType=ePVRTPF_DXT3;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC2_UNORM_SRGB: 
		{
			newType=ePVRTPF_DXT3;
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC3_UNORM: 
		{
			newType=ePVRTPF_DXT5;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC3_UNORM_SRGB: 
		{
			newType=ePVRTPF_DXT5;
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC4_UNORM: 
		{
			newType=ePVRTPF_BC4;
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC4_SNORM: 
		{
			newType=ePVRTPF_BC4;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedIntegerNorm;
			break;
		}

	case DX10_BC5_UNORM: 
		{
			newType=ePVRTPF_BC5;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedIntegerNorm;
			break;
		}

	case DX10_BC5_SNORM: 
		{
			newType=ePVRTPF_BC5;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeSignedIntegerNorm;
			break;
		}

#endif
	case ePT_VG_sRGBX_8888: 
		{
			newType=PVRTGENPIXELID4('r','g','b','x',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sRGBA_8888: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sRGBA_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case ePT_VG_sRGB_565: 
		{
			newType=PVRTGENPIXELID3('r','g','b',5,6,5);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_sRGBA_5551: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',5,5,5,1);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_sRGBA_4444: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',4,4,4,4);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_sL_8: 
		{
			newType=PVRTGENPIXELID1('l',8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lRGBX_8888: 
		{
			newType=PVRTGENPIXELID4('r','g','b','x',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lRGBA_8888: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lRGBA_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case ePT_VG_lL_8: 
		{
			newType=PVRTGENPIXELID1('l',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_A_8: 
		{
			newType=PVRTGENPIXELID1('a',8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_BW_1: 
		{
			newType=ePVRTPF_BW1bpp;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sXRGB_8888: 
		{
			newType=PVRTGENPIXELID4('x','r','g','b',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sARGB_8888: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sARGB_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case ePT_VG_sARGB_1555: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',1,5,5,5);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_sARGB_4444: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',4,4,4,4);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_lXRGB_8888: 
		{
			newType=PVRTGENPIXELID4('x','r','g','b',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lARGB_8888: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lARGB_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('a','r','g','b',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case ePT_VG_sBGRX_8888: 
		{
			newType=PVRTGENPIXELID4('b','g','r','x',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sBGRA_8888: 
		{
			newType=PVRTGENPIXELID4('b','g','r','a',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sBGRA_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('b','g','r','a',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case ePT_VG_sBGR_565: 
		{
			newType=PVRTGENPIXELID3('b','g','r',5,6,5);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_sBGRA_5551: 
		{
			newType=PVRTGENPIXELID4('b','g','r','a',5,5,5,1);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_sBGRA_4444: 
		{
			newType=PVRTGENPIXELID4('b','g','r','x',4,4,4,4);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_lBGRX_8888: 
		{
			newType=PVRTGENPIXELID4('b','g','r','x',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lBGRA_8888: 
		{
			newType=PVRTGENPIXELID4('b','g','r','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lBGRA_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('b','g','r','a',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case ePT_VG_sXBGR_8888: 
		{
			newType=PVRTGENPIXELID4('x','b','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sABGR_8888: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_sABGR_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}

	case ePT_VG_sABGR_1555: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',1,5,5,5);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_sABGR_4444: 
		{
			newType=PVRTGENPIXELID4('x','b','g','r',4,4,4,4);
			newCSpace=ePVRTCSpacesRGB;
			newChanType=ePVRTVarTypeUnsignedShortNorm;
			break;
		}

	case ePT_VG_lXBGR_8888: 
		{
			newType=PVRTGENPIXELID4('x','b','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lABGR_8888: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			break;
		}

	case ePT_VG_lABGR_8888_PRE: 
		{
			newType=PVRTGENPIXELID4('a','b','g','r',8,8,8,8);
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeUnsignedByteNorm;
			isPreMult=true;
			break;
		}
	default:
		{
			newType=ePVRTPF_NumCompressedPFs;
			newCSpace=ePVRTCSpacelRGB;
			newChanType=ePVRTVarTypeNumVarTypes;
			break;
		}
	}
}

/*****************************************************************************
 End of file (PVRTTexture.cpp)
*****************************************************************************/
