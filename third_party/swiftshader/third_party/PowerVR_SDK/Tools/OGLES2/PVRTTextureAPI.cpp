/******************************************************************************

 @File         OGLES2/PVRTTextureAPI.cpp

 @Title        OGLES2/PVRTTextureAPI

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  OGLES2 texture loading.

******************************************************************************/

#include <string.h>
#include <stdlib.h>

#include "PVRTContext.h"
#include "PVRTgles2Ext.h"
#include "PVRTTexture.h"
#include "PVRTTextureAPI.h"
#include "PVRTDecompress.h"
#include "PVRTFixedPoint.h"
#include "PVRTMap.h"
#include "PVRTMatrix.h"
#include "PVRTMisc.h"
#include "PVRTResourceFile.h"

/*****************************************************************************
** Functions
****************************************************************************/


/*!***********************************************************************
	@Function:		PVRTGetOGLESTextureFormat
	@Input:			sTextureHeader
	@Modified:		internalformat
	@Modified:		format
	@Modified:		type
	@Description:	Gets the OpenGLES equivalent values of internal format, 
					format and type for this texture header. This will return 
					any supported OpenGLES texture values, it is up to the user 
					to decide if these are valid for their current platform.
*************************************************************************/
static const void PVRTGetOGLES2TextureFormat(const PVRTextureHeaderV3& sTextureHeader, PVRTuint32& internalformat, PVRTuint32& format, PVRTuint32& type)
{
	PVRTuint64 PixelFormat = sTextureHeader.u64PixelFormat;
	EPVRTVariableType ChannelType = (EPVRTVariableType)sTextureHeader.u32ChannelType;
	
	//Initialisation. Any invalid formats will return 0 always.
	format = 0;
	type = 0;
	internalformat=0;

	//Get the last 32 bits of the pixel format.
	PVRTuint64 PixelFormatPartHigh = PixelFormat&PVRTEX_PFHIGHMASK;

	//Check for a compressed format (The first 8 bytes will be 0, so the whole thing will be equal to the last 32 bits).
	if (PixelFormatPartHigh==0)
	{
		//Format and type == 0 for compressed textures.
		switch (PixelFormat)
		{
		case ePVRTPF_PVRTCI_2bpp_RGB:
			{
				internalformat=GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
				return;
			}
		case ePVRTPF_PVRTCI_2bpp_RGBA:
			{
				internalformat=GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
				return;
			}
		case ePVRTPF_PVRTCI_4bpp_RGB:
			{
				internalformat=GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
				return;
			}
		case ePVRTPF_PVRTCI_4bpp_RGBA:
			{
				internalformat=GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
				return;
			}
#ifndef TARGET_OS_IPHONE
		case ePVRTPF_PVRTCII_2bpp:
			{
				internalformat=GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
				return;
			}
		case ePVRTPF_PVRTCII_4bpp:
			{
				internalformat=GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
				return;
			}
		case ePVRTPF_ETC1:
			{
				internalformat=GL_ETC1_RGB8_OES;
				return;
			}
#endif
		default:
			return;
		}
	}
	else
	{
		switch (ChannelType)
		{
		case ePVRTVarTypeFloat:
			{
				switch (PixelFormat)
				{
					//HALF_FLOAT_OES
				case PVRTGENPIXELID4('r','g','b','a',16,16,16,16):
					{
						type=GL_HALF_FLOAT_OES;
						format = GL_RGBA;
						internalformat=GL_RGBA;
						return;
					}
				case PVRTGENPIXELID3('r','g','b',16,16,16):
					{
						type=GL_HALF_FLOAT_OES;
						format = GL_RGB;
						internalformat=GL_RGB;
						return;
					}
				case PVRTGENPIXELID2('l','a',16,16):
					{
						type=GL_HALF_FLOAT_OES;
						format = GL_LUMINANCE_ALPHA;
						internalformat=GL_LUMINANCE_ALPHA;
						return;
					}
				case PVRTGENPIXELID1('l',16):
					{
						type=GL_HALF_FLOAT_OES;
						format = GL_LUMINANCE;
						internalformat=GL_LUMINANCE;
						return;
					}
				case PVRTGENPIXELID1('a',16):
					{
						type=GL_HALF_FLOAT_OES;
						format = GL_ALPHA;
						internalformat=GL_ALPHA;
						return;
					}
					//FLOAT (OES)
				case PVRTGENPIXELID4('r','g','b','a',32,32,32,32):
					{
						type=GL_FLOAT;
						format = GL_RGBA;
						internalformat=GL_RGBA;
						return;
					}
				case PVRTGENPIXELID3('r','g','b',32,32,32):
					{
						type=GL_FLOAT;
						format = GL_RGB;
						internalformat=GL_RGB;
						return;
					}
				case PVRTGENPIXELID2('l','a',32,32):
					{
						type=GL_FLOAT;
						format = GL_LUMINANCE_ALPHA;
						internalformat=GL_LUMINANCE_ALPHA;
						return;
					}
				case PVRTGENPIXELID1('l',32):
					{
						type=GL_FLOAT;
						format = GL_LUMINANCE;
						internalformat=GL_LUMINANCE;
						return;
					}
				case PVRTGENPIXELID1('a',32):
					{
						type=GL_FLOAT;
						format = GL_ALPHA;
						internalformat=GL_ALPHA;
						return;
					}
				}
				break;
			}
		case ePVRTVarTypeUnsignedByteNorm:
			{
				type = GL_UNSIGNED_BYTE;
				switch (PixelFormat)
				{
				case PVRTGENPIXELID4('r','g','b','a',8,8,8,8):
					{
						format = internalformat = GL_RGBA;
						return;
					}
				case PVRTGENPIXELID3('r','g','b',8,8,8):
					{
						format = internalformat = GL_RGB;
						return;
					}
				case PVRTGENPIXELID2('l','a',8,8):
					{
						format = internalformat = GL_LUMINANCE_ALPHA;
						return;
					}
				case PVRTGENPIXELID1('l',8):
					{
						format = internalformat = GL_LUMINANCE;
						return;
					}
				case PVRTGENPIXELID1('a',8):
					{
						format = internalformat = GL_ALPHA;
						return;
					}
				case PVRTGENPIXELID4('b','g','r','a',8,8,8,8):
					{
						format = internalformat = GL_BGRA;
						return;
					}
				}
				break;
			}
		case ePVRTVarTypeUnsignedShortNorm:
			{
				switch (PixelFormat)
				{
				case PVRTGENPIXELID4('r','g','b','a',4,4,4,4):
					{
						type = GL_UNSIGNED_SHORT_4_4_4_4;
						format = internalformat = GL_RGBA;
						return;
					}
				case PVRTGENPIXELID4('r','g','b','a',5,5,5,1):
					{
						type = GL_UNSIGNED_SHORT_5_5_5_1;
						format = internalformat = GL_RGBA;
						return;
					}
				case PVRTGENPIXELID3('r','g','b',5,6,5):
					{
						type = GL_UNSIGNED_SHORT_5_6_5;
						format = internalformat = GL_RGB;
						return;
					}
				}
				break;
			}
		default:
			return;
		}
	}
}

/*!***************************************************************************
@Function		PVRTTextureTile
@Modified		pOut		The tiled texture in system memory
@Input			pIn			The source texture
@Input			nRepeatCnt	Number of times to repeat the source texture
@Description	Allocates and fills, in system memory, a texture large enough
				to repeat the source texture specified number of times.
*****************************************************************************/
void PVRTTextureTile(
	PVRTextureHeaderV3			**pOut,
	const PVRTextureHeaderV3	* const pIn,
	const int					nRepeatCnt)
{
	unsigned int		nFormat = 0, nType = 0, nBPP, nSize, nElW = 0, nElH = 0, nElD = 0;
	PVRTuint8		*pMmSrc, *pMmDst;
	unsigned int		nLevel;
	PVRTextureHeaderV3	*psTexHeaderNew;

	_ASSERT(pIn->u32Width);
	_ASSERT(pIn->u32Width == pIn->u32Height);
	_ASSERT(nRepeatCnt > 1);

	PVRTGetOGLES2TextureFormat(*pIn,nFormat,nFormat,nType);
	PVRTGetFormatMinDims(pIn->u64PixelFormat,nElW,nElH,nElD);
	
	nBPP = PVRTGetBitsPerPixel(pIn->u64PixelFormat);
	nSize = pIn->u32Width * nRepeatCnt;

	psTexHeaderNew	= PVRTTextureCreate(nSize, nSize, nElW, nElH, nBPP, true);
	*psTexHeaderNew	= *pIn;
	pMmDst	= (PVRTuint8*)psTexHeaderNew + sizeof(*psTexHeaderNew);
	pMmSrc	= (PVRTuint8*)pIn + sizeof(*pIn);

	for(nLevel = 0; ((unsigned int)1 << nLevel) < nSize; ++nLevel)
	{
		int nBlocksDstW = PVRT_MAX((unsigned int)1, (nSize >> nLevel) / nElW);
		int nBlocksDstH = PVRT_MAX((unsigned int)1, (nSize >> nLevel) / nElH);
		int nBlocksSrcW = PVRT_MAX((unsigned int)1, (pIn->u32Width >> nLevel) / nElW);
		int nBlocksSrcH = PVRT_MAX((unsigned int)1, (pIn->u32Height >> nLevel) / nElH);
		int nBlocksS	= nBPP * nElW * nElH / 8;

		PVRTTextureLoadTiled(
			pMmDst,
			nBlocksDstW,
			nBlocksDstH,
			pMmSrc,
			nBlocksSrcW,
			nBlocksSrcH,
			nBlocksS,
			(/*pIn->u64PixelFormat>=ePVRTPF_PVRTCI_2bpp_RGB &&*/ pIn->u64PixelFormat<=ePVRTPF_PVRTCI_4bpp_RGBA) ? true : false);

		pMmDst += nBlocksDstW * nBlocksDstH * nBlocksS;
		pMmSrc += nBlocksSrcW * nBlocksSrcH * nBlocksS;
	}

	psTexHeaderNew->u32Width = nSize;
	psTexHeaderNew->u32Height = nSize;
	psTexHeaderNew->u32MIPMapCount = nLevel+1;
	*pOut = psTexHeaderNew;
}

/*!***************************************************************************
 @Function		PVRTTextureLoadFromPointer
 @Input			pointer				Pointer to header-texture's structure
 @Modified		texName				the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader		Pointer to a PVRTextureHeaderV3 struct. Modified to
									contain the header data of the returned texture Ignored if NULL.
 @Input			bAllowDecompress	Allow decompression if PVRTC is not supported in hardware.
 @Input			nLoadFromLevel		Which mip map level to start loading from (0=all)
 @Input			texPtr				If null, texture follows header, else texture is here.
 @Modified		pMetaData			If a valid map is supplied, this will return any and all 
									MetaDataBlocks stored in the texture, organised by DevFourCC
									then identifier. Supplying NULL will ignore all MetaData.
 @Return		PVR_SUCCESS on success
 @Description	Allows textures to be stored in C header files and loaded in. Can load parts of a
				mip mapped texture (i.e. skipping the highest detailed levels). In OpenGL Cube Map, each
				texture's up direction is defined as next (view direction, up direction),
				(+x,-y)(-x,-y)(+y,+z)(-y,-z)(+z,-y)(-z,-y).
				Sets the texture MIN/MAG filter to GL_LINEAR_MIPMAP_NEAREST/GL_LINEAR
				if mip maps are present, GL_LINEAR/GL_LINEAR otherwise.
*****************************************************************************/
EPVRTError PVRTTextureLoadFromPointer(	const void* pointer,
										GLuint *const texName,
										const void *psTextureHeader,
										bool bAllowDecompress,
										const unsigned int nLoadFromLevel,
										const void * const texPtr,
										CPVRTMap<unsigned int, CPVRTMap<unsigned int, MetaDataBlock> > *pMetaData)
{
	//Compression bools
	bool bIsCompressedFormatSupported=false;
	bool bIsCompressedFormat=false;
	bool bIsLegacyPVR=false;
	bool bUsesTexImage3D = false;

	CPVRTgles2Ext extensions;
	extensions.LoadExtensions();

	//Texture setup
	PVRTextureHeaderV3 sTextureHeader;
	PVRTuint8* pTextureData=NULL;

	//Just in case header and pointer for decompression.
	PVRTextureHeaderV3 sTextureHeaderDecomp;
	void* pDecompressedData=NULL;

	//Check if it's an old header format
	if((*(PVRTuint32*)pointer)!=PVRTEX3_IDENT)
	{
		//Convert the texture header to the new format.
		PVRTConvertOldTextureHeaderToV3((PVR_Texture_Header*)pointer,sTextureHeader,pMetaData);

		//Get the texture data.
		pTextureData = texPtr? (PVRTuint8*)texPtr:(PVRTuint8*)pointer+*(PVRTuint32*)pointer;

		bIsLegacyPVR=true;
	}
	else
	{
		//Get the header from the main pointer.
		sTextureHeader=*(PVRTextureHeaderV3*)pointer;

		//Get the texture data.
		pTextureData = texPtr? (PVRTuint8*)texPtr:(PVRTuint8*)pointer+PVRTEX3_HEADERSIZE+sTextureHeader.u32MetaDataSize;

		if (pMetaData)
		{
			//Read in all the meta data.
			PVRTuint32 metaDataSize=0;
			while (metaDataSize<sTextureHeader.u32MetaDataSize)
			{
				//Read the DevFourCC and advance the pointer offset.
				PVRTuint32 DevFourCC=*(PVRTuint32*)((PVRTuint8*)pointer+PVRTEX3_HEADERSIZE+metaDataSize);
				metaDataSize+=sizeof(DevFourCC);

				//Read the Key and advance the pointer offset.
				PVRTuint32 u32Key=*(PVRTuint32*)((PVRTuint8*)pointer+PVRTEX3_HEADERSIZE+metaDataSize);
				metaDataSize+=sizeof(u32Key);

				//Read the DataSize and advance the pointer offset.
				PVRTuint32 u32DataSize = *(PVRTuint32*)((PVRTuint8*)pointer+PVRTEX3_HEADERSIZE+metaDataSize);
				metaDataSize+=sizeof(u32DataSize);

				//Get the current meta data.
				MetaDataBlock& currentMetaData = (*pMetaData)[DevFourCC][u32Key];

				//Assign the values to the meta data.
				currentMetaData.DevFOURCC=DevFourCC;
				currentMetaData.u32Key=u32Key;
				currentMetaData.u32DataSize=u32DataSize;
				
				//Check for data, if there is any, read it into the meta data.
				if(u32DataSize > 0)
				{
					//Allocate memory.
					currentMetaData.Data = new PVRTuint8[u32DataSize];

					//Copy the data.
					memcpy(currentMetaData.Data, ((PVRTuint8*)pointer+PVRTEX3_HEADERSIZE+metaDataSize), u32DataSize);

					//Advance the meta data size.
					metaDataSize+=u32DataSize;
				}
			}
		}
	}

	//Return the PVRTextureHeader.
	if (psTextureHeader)
	{
		*(PVRTextureHeaderV3*)psTextureHeader=sTextureHeader;
	}
	
	//Setup GL Texture format values.
	GLenum eTextureFormat = 0;
	GLenum eTextureInternalFormat = 0;	// often this is the same as textureFormat, but not for BGRA8888 on iOS, for instance
	GLenum eTextureType = 0;

	//Get the OGLES format values.
	PVRTGetOGLES2TextureFormat(sTextureHeader,eTextureInternalFormat,eTextureFormat,eTextureType);

	//Check supported texture formats.
	bool bIsPVRTCSupported = CPVRTgles2Ext::IsGLExtensionSupported("GL_IMG_texture_compression_pvrtc");
	bool bIsPVRTC2Supported = CPVRTgles2Ext::IsGLExtensionSupported("GL_IMG_texture_compression_pvrtc2");
#ifndef TARGET_OS_IPHONE
	bool bIsBGRA8888Supported  = CPVRTgles2Ext::IsGLExtensionSupported("GL_IMG_texture_format_BGRA8888");
#else
	bool bIsBGRA8888Supported  = CPVRTgles2Ext::IsGLExtensionSupported("GL_APPLE_texture_format_BGRA8888");
#endif
	bool bIsFloat16Supported = CPVRTgles2Ext::IsGLExtensionSupported("GL_OES_texture_half_float");
	bool bIsFloat32Supported = CPVRTgles2Ext::IsGLExtensionSupported("GL_OES_texture_float");
	bool bIsTexture3DSupported = CPVRTgles2Ext::IsGLExtensionSupported("GL_OES_texture_3D");
#ifndef TARGET_OS_IPHONE
	bool bIsETCSupported = CPVRTgles2Ext::IsGLExtensionSupported("GL_OES_compressed_ETC1_RGB8_texture");
#endif
		
	//Check for compressed formats
	if (eTextureFormat==0 && eTextureType==0 && eTextureInternalFormat!=0)
	{
		if (eTextureInternalFormat>=GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG && eTextureInternalFormat<=GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
		{
			//Check for PVRTCI support.
			if(bIsPVRTCSupported)
			{
				bIsCompressedFormatSupported = bIsCompressedFormat = true;
			}
			else
			{
				//Try to decompress the texture.
				if(bAllowDecompress)
				{
					//Output a warning.
					PVRTErrorOutputDebug("PVRTTextureLoadFromPointer warning: PVRTC not supported. Converting to RGBA8888 instead.\n");

					//Modify boolean values.
					bIsCompressedFormatSupported = false;
					bIsCompressedFormat = true;

					//Check if it's 2bpp.
					bool bIs2bppPVRTC = (eTextureInternalFormat==GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG || eTextureInternalFormat==GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG);

					//Change texture format.
					eTextureFormat = eTextureInternalFormat = GL_RGBA;
					eTextureType = GL_UNSIGNED_BYTE;

					//Create a near-identical texture header for the decompressed header.
					sTextureHeaderDecomp = sTextureHeader;
					sTextureHeaderDecomp.u32ChannelType=ePVRTVarTypeUnsignedByteNorm;
					sTextureHeaderDecomp.u32ColourSpace=ePVRTCSpacelRGB;
					sTextureHeaderDecomp.u64PixelFormat=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);

					//Allocate enough memory for the decompressed data. OGLES2, so only decompress one surface, but all faces.
					pDecompressedData = malloc(PVRTGetTextureDataSize(sTextureHeaderDecomp, PVRTEX_ALLMIPLEVELS, false, true) );

					//Check the malloc.
					if (!pDecompressedData)
					{
						PVRTErrorOutputDebug("PVRTTextureLoadFromPointer error: Unable to allocate memory to decompress texture.\n");
						return PVR_FAIL;
					}

					//Get the dimensions for the current MIP level.
					PVRTuint32 uiMIPWidth = sTextureHeaderDecomp.u32Width>>nLoadFromLevel;
					PVRTuint32 uiMIPHeight = sTextureHeaderDecomp.u32Height>>nLoadFromLevel;

					//Setup temporary variables.
					PVRTuint8* pTempDecompData = (PVRTuint8*)pDecompressedData;
					PVRTuint8* pTempCompData = (PVRTuint8*)pTextureData;

					if (bIsLegacyPVR)
					{
						//Decompress all the MIP levels.
						for (PVRTuint32 uiFace=0;uiFace<sTextureHeader.u32NumFaces;++uiFace)
						{

							for (PVRTuint32 uiMIPMap=nLoadFromLevel;uiMIPMap<sTextureHeader.u32MIPMapCount;++uiMIPMap)
							{
								//Get the face offset. Varies per MIP level.
								PVRTuint32 decompressedFaceOffset = PVRTGetTextureDataSize(sTextureHeaderDecomp, uiMIPMap, false, false);
								PVRTuint32 compressedFaceOffset = PVRTGetTextureDataSize(sTextureHeader, uiMIPMap, false, false);

								//Decompress the texture data.
								PVRTDecompressPVRTC(pTempCompData,bIs2bppPVRTC?1:0,uiMIPWidth,uiMIPHeight,pTempDecompData);

								//Move forward through the pointers.
								pTempDecompData+=decompressedFaceOffset;
								pTempCompData+=compressedFaceOffset;

								//Work out the current MIP dimensions.
								uiMIPWidth=PVRT_MAX(1,uiMIPWidth>>1);
								uiMIPHeight=PVRT_MAX(1,uiMIPHeight>>1);
							}

							//Reset the dims.
							uiMIPWidth=sTextureHeader.u32Width;
							uiMIPHeight=sTextureHeader.u32Height;
						}
					}
					else
					{
						//Decompress all the MIP levels.
						for (PVRTuint32 uiMIPMap=nLoadFromLevel;uiMIPMap<sTextureHeader.u32MIPMapCount;++uiMIPMap)
						{
							//Get the face offset. Varies per MIP level.
							PVRTuint32 decompressedFaceOffset = PVRTGetTextureDataSize(sTextureHeaderDecomp, uiMIPMap, false, false);
							PVRTuint32 compressedFaceOffset = PVRTGetTextureDataSize(sTextureHeader, uiMIPMap, false, false);

							for (PVRTuint32 uiFace=0;uiFace<sTextureHeader.u32NumFaces;++uiFace)
							{
								//Decompress the texture data.
								PVRTDecompressPVRTC(pTempCompData,bIs2bppPVRTC?1:0,uiMIPWidth,uiMIPHeight,pTempDecompData);

								//Move forward through the pointers.
								pTempDecompData+=decompressedFaceOffset;
								pTempCompData+=compressedFaceOffset;
							}

							//Work out the current MIP dimensions.
							uiMIPWidth=PVRT_MAX(1,uiMIPWidth>>1);
							uiMIPHeight=PVRT_MAX(1,uiMIPHeight>>1);
						}
					}
				}
				else
				{
					PVRTErrorOutputDebug("PVRTTextureLoadFromPointer error: PVRTC not supported.\n");
					return PVR_FAIL;
				}
			}
		}
#ifndef TARGET_OS_IPHONE //TODO
		else if (eTextureInternalFormat==GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG || eTextureInternalFormat==GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG)
		{
			//Check for PVRTCI support.
			if(bIsPVRTC2Supported)
			{
				bIsCompressedFormatSupported = bIsCompressedFormat = true;
			}
			else
			{				
				PVRTErrorOutputDebug("PVRTTextureLoadFromPointer error: PVRTC not supported.\n");
				return PVR_FAIL;
			}
		}
		else if (eTextureInternalFormat==GL_ETC1_RGB8_OES)
		{
			if(bIsETCSupported)
			{
				bIsCompressedFormatSupported = bIsCompressedFormat = true;
			}
			else
			{
				if(bAllowDecompress)
				{
					//Output a warning.
					PVRTErrorOutputDebug("PVRTTextureLoadFromPointer warning: ETC not supported. Converting to RGBA8888 instead.\n");

					//Modify boolean values.
					bIsCompressedFormatSupported = false;
					bIsCompressedFormat = true;

					//Change texture format.
					eTextureFormat = eTextureInternalFormat = GL_RGBA;
					eTextureType = GL_UNSIGNED_BYTE;

					//Create a near-identical texture header for the decompressed header.
					sTextureHeaderDecomp = sTextureHeader;
					sTextureHeaderDecomp.u32ChannelType=ePVRTVarTypeUnsignedByteNorm;
					sTextureHeaderDecomp.u32ColourSpace=ePVRTCSpacelRGB;
					sTextureHeaderDecomp.u64PixelFormat=PVRTGENPIXELID4('r','g','b','a',8,8,8,8);

					//Allocate enough memory for the decompressed data. OGLES1, so only decompress one surface/face.
					pDecompressedData = malloc(PVRTGetTextureDataSize(sTextureHeaderDecomp, PVRTEX_ALLMIPLEVELS, false, true) );

					//Check the malloc.
					if (!pDecompressedData)
					{
						PVRTErrorOutputDebug("PVRTTextureLoadFromPointer error: Unable to allocate memory to decompress texture.\n");
						return PVR_FAIL;
					}

					//Get the dimensions for the current MIP level.
					PVRTuint32 uiMIPWidth = sTextureHeaderDecomp.u32Width>>nLoadFromLevel;
					PVRTuint32 uiMIPHeight = sTextureHeaderDecomp.u32Height>>nLoadFromLevel;

					//Setup temporary variables.
					PVRTuint8* pTempDecompData = (PVRTuint8*)pDecompressedData;
					PVRTuint8* pTempCompData = (PVRTuint8*)pTextureData;

					if (bIsLegacyPVR)
					{
						//Decompress all the MIP levels.
						for (PVRTuint32 uiFace=0;uiFace<sTextureHeader.u32NumFaces;++uiFace)
						{

							for (PVRTuint32 uiMIPMap=nLoadFromLevel;uiMIPMap<sTextureHeader.u32MIPMapCount;++uiMIPMap)
							{
								//Get the face offset. Varies per MIP level.
								PVRTuint32 decompressedFaceOffset = PVRTGetTextureDataSize(sTextureHeaderDecomp, uiMIPMap, false, false);
								PVRTuint32 compressedFaceOffset = PVRTGetTextureDataSize(sTextureHeader, uiMIPMap, false, false);

								//Decompress the texture data.
								PVRTDecompressETC(pTempCompData,uiMIPWidth,uiMIPHeight,pTempDecompData,0);

								//Move forward through the pointers.
								pTempDecompData+=decompressedFaceOffset;
								pTempCompData+=compressedFaceOffset;

								//Work out the current MIP dimensions.
								uiMIPWidth=PVRT_MAX(1,uiMIPWidth>>1);
								uiMIPHeight=PVRT_MAX(1,uiMIPHeight>>1);
							}

							//Reset the dims.
							uiMIPWidth=sTextureHeader.u32Width;
							uiMIPHeight=sTextureHeader.u32Height;
						}
					}
					else
					{
						//Decompress all the MIP levels.
						for (PVRTuint32 uiMIPMap=nLoadFromLevel;uiMIPMap<sTextureHeader.u32MIPMapCount;++uiMIPMap)
						{
							//Get the face offset. Varies per MIP level.
							PVRTuint32 decompressedFaceOffset = PVRTGetTextureDataSize(sTextureHeaderDecomp, uiMIPMap, false, false);
							PVRTuint32 compressedFaceOffset = PVRTGetTextureDataSize(sTextureHeader, uiMIPMap, false, false);

							for (PVRTuint32 uiFace=0;uiFace<sTextureHeader.u32NumFaces;++uiFace)
							{
								//Decompress the texture data.
								PVRTDecompressETC(pTempCompData,uiMIPWidth,uiMIPHeight,pTempDecompData,0);

								//Move forward through the pointers.
								pTempDecompData+=decompressedFaceOffset;
								pTempCompData+=compressedFaceOffset;
							}

							//Work out the current MIP dimensions.
							uiMIPWidth=PVRT_MAX(1,uiMIPWidth>>1);
							uiMIPHeight=PVRT_MAX(1,uiMIPHeight>>1);
						}
					}
				}
				else
				{
					PVRTErrorOutputDebug("PVRTTextureLoadFromPointer error: ETC not supported.\n");
					return PVR_FAIL;
				}
			}
		}
#endif
	}

	//Check for BGRA support.	
	if(eTextureFormat==GL_BGRA)
	{
#ifdef TARGET_OS_IPHONE
		eTextureInternalFormat = GL_RGBA;
#endif
		if(!bIsBGRA8888Supported)
		{
#ifdef TARGET_OS_IPHONE
			PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: Unable to load GL_BGRA texture as extension GL_APPLE_texture_format_BGRA8888 is unsupported.\n");
#else
			PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: Unable to load GL_BGRA texture as extension GL_IMG_texture_format_BGRA8888 is unsupported.\n");
#endif
			return PVR_FAIL;
		}
	}

	//Check for floating point textures
	if (eTextureType==GL_HALF_FLOAT_OES)
	{
		if(!bIsFloat16Supported)
		{
			PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: Unable to load GL_HALF_FLOAT_OES texture as extension GL_OES_texture_half_float is unsupported.\n");
		}
	}
	if (eTextureType==GL_FLOAT)
	{
		if(!bIsFloat32Supported)
		{
			PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: Unable to load GL_FLOAT texture as extension GL_OES_texture_float is unsupported.\n");
		}
	}

	//Deal with unsupported texture formats
	if (eTextureInternalFormat==0)
	{
		PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: pixel type not supported.\n");
		return PVR_FAIL;
	}

	//PVR files are never row aligned.
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	//Generate a texture
	glGenTextures(1, texName);

	//Initialise a texture target.
	GLint eTarget=GL_TEXTURE_2D;
	
	//A mix of arrays/cubes/depths are not permitted in OpenGL ES. Check.
	if(sTextureHeader.u32NumFaces>1 || sTextureHeader.u32NumSurfaces>1 || sTextureHeader.u32Depth>1)
	{
		if((sTextureHeader.u32NumFaces>1) && (sTextureHeader.u32NumSurfaces>1))
		{
			PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: Arrays of cubemaps are not supported by OpenGL ES 3.0\n");
			return PVR_FAIL;
		}
		else if((sTextureHeader.u32NumFaces>1) && (sTextureHeader.u32Depth>1))
		{
			PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: 3D Cubemap textures are not supported by OpenGL ES 3.0\n");
			return PVR_FAIL;
		}
		else if((sTextureHeader.u32NumSurfaces>1) && (sTextureHeader.u32Depth>1))
		{
			PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: Arrays of 3D textures are not supported by OpenGL ES 3.0\n");
			return PVR_FAIL;
		}

		if(sTextureHeader.u32NumSurfaces>1)
		{
			// GL_TEXTURE_2D_ARRAY;
			return PVR_FAIL;
		}
		else if(sTextureHeader.u32NumFaces>1)
		{
			eTarget = GL_TEXTURE_CUBE_MAP;
		}
		else if(sTextureHeader.u32Depth>1)
		{
			eTarget = GL_TEXTURE_3D_OES;
			bUsesTexImage3D = true;
		}
	}

	if(bUsesTexImage3D && !bIsTexture3DSupported)
	{
		return PVR_FAIL;
	}

	//Bind the texture
	glBindTexture(eTarget, *texName);

	if(glGetError())
	{
		PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: glBindTexture() failed.\n");
		return PVR_FAIL;
	}

	//Initialise the current MIP size.
	PVRTuint32 uiCurrentMIPSize=0;

	//Loop through the faces
	//Check if this is a cube map.
	if(sTextureHeader.u32NumFaces>1)
	{
		eTarget=GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}

	//Initialise the width/height
	PVRTuint32 u32MIPWidth = sTextureHeader.u32Width;
	PVRTuint32 u32MIPHeight = sTextureHeader.u32Height;

	//Temporary data to save on if statements within the load loops.
	PVRTuint8* pTempData=NULL;
	PVRTextureHeaderV3 *psTempHeader=NULL;
	if (bIsCompressedFormat && !bIsCompressedFormatSupported)
	{
		pTempData=(PVRTuint8*)pDecompressedData;
		psTempHeader=&sTextureHeaderDecomp;
	}
	else
	{
		pTempData=pTextureData;
		psTempHeader=&sTextureHeader;
	}

	PVRTuint32 u32MIPDepth = 1;
	if(bUsesTexImage3D)
	{
		if(psTempHeader->u32Depth>1)
		{
			u32MIPDepth = psTempHeader->u32Depth; //3d texture.
		}
		else
		{
			return PVR_FAIL; //2d arrays.
		}
	}

	//Loop through all MIP levels.
	if (bIsLegacyPVR)
	{
		//Temporary texture target.
		GLint eTextureTarget=eTarget;

		//Loop through all the faces.
		for (PVRTuint32 uiFace=0; uiFace<psTempHeader->u32NumFaces; ++uiFace)
		{
			//Loop through all the mip levels.
			for (PVRTuint32 uiMIPLevel=0; uiMIPLevel<psTempHeader->u32MIPMapCount; ++uiMIPLevel)
			{
				//Get the current MIP size.
				uiCurrentMIPSize=PVRTGetTextureDataSize(*psTempHeader,uiMIPLevel,false,false);

				if (uiMIPLevel>=nLoadFromLevel)
				{
					//Upload the texture
					if(bUsesTexImage3D)
					{
						if(bIsCompressedFormat && bIsCompressedFormatSupported)
						{
							extensions.glCompressedTexImage3DOES(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, u32MIPDepth, 0, uiCurrentMIPSize, pTempData);
						}
						else
						{
							extensions.glTexImage3DOES(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, u32MIPDepth, 0, eTextureFormat, eTextureType, pTempData);
						}
					}
					else
					{
						if(bIsCompressedFormat && bIsCompressedFormatSupported)
						{
							glCompressedTexImage2D(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, 0, uiCurrentMIPSize, pTempData);
						}
						else
						{
							glTexImage2D(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, 0, eTextureFormat, eTextureType, pTempData);
						}
					}
				}
				pTempData+=uiCurrentMIPSize;

				//Reduce the MIP Size.
				u32MIPWidth=PVRT_MAX(1,u32MIPWidth>>1);
				u32MIPHeight=PVRT_MAX(1,u32MIPHeight>>1);
			}

			//Increase the texture target.
			eTextureTarget++;

			//Reset the current MIP dimensions.
			u32MIPWidth=psTempHeader->u32Width;
			u32MIPHeight=psTempHeader->u32Height;

			//Error check
			if(glGetError())
			{
				FREE(pDecompressedData);
				PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: glTexImage2D() failed.\n");
				return PVR_FAIL;
			}
		}
	}
	else
	{
		for (PVRTuint32 uiMIPLevel=0; uiMIPLevel<psTempHeader->u32MIPMapCount; ++uiMIPLevel)
		{
			//Get the current MIP size.
			uiCurrentMIPSize=PVRTGetTextureDataSize(*psTempHeader,uiMIPLevel,false,false);

			GLint eTextureTarget=eTarget;

			for (PVRTuint32 uiFace=0; uiFace<psTempHeader->u32NumFaces; ++uiFace)
			{
				if (uiMIPLevel>=nLoadFromLevel)
				{
					//Upload the texture
					if(bUsesTexImage3D)
					{
						//Upload the texture
						if(bIsCompressedFormat && bIsCompressedFormatSupported)
						{
							extensions.glCompressedTexImage3DOES(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, u32MIPDepth, 0, uiCurrentMIPSize, pTempData);
						}
						else
						{
							extensions.glTexImage3DOES(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, u32MIPDepth, 0, eTextureFormat, eTextureType, pTempData);
						}
					}
					else
					{
						//Upload the texture
						if(bIsCompressedFormat && bIsCompressedFormatSupported)
						{
							glCompressedTexImage2D(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, 0, uiCurrentMIPSize, pTempData);
						}
						else
						{
							glTexImage2D(eTextureTarget, uiMIPLevel - nLoadFromLevel, eTextureInternalFormat, u32MIPWidth, u32MIPHeight, 0, eTextureFormat, eTextureType, pTempData);
						}
					}
				}
				pTempData+=uiCurrentMIPSize;
				eTextureTarget++;
			}

			//Reduce the MIP Size.
			u32MIPWidth=PVRT_MAX(1,u32MIPWidth>>1);
			u32MIPHeight=PVRT_MAX(1,u32MIPHeight>>1);

			//Error check
			if(glGetError())
			{
				FREE(pDecompressedData);
				PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: glTexImage2D() failed.\n");
				return PVR_FAIL;
			}
		}
	}

	FREE(pDecompressedData);

	if (eTarget!=GL_TEXTURE_2D)
	{
		eTarget=GL_TEXTURE_CUBE_MAP;
	}

	//Error check
	if(glGetError())
	{
		PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: glTexImage2D() failed.\n");
		return PVR_FAIL;
	}
	
	//Set Minification and Magnification filters according to whether MIP maps are present.
	if(eTextureType==GL_FLOAT || eTextureType==GL_HALF_FLOAT_OES)
	{
		if(sTextureHeader.u32MIPMapCount==1)
		{	// Texture filter modes are limited to these for float textures
			glTexParameteri(eTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(eTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
			glTexParameteri(eTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(eTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}
	else
	{
		if(sTextureHeader.u32MIPMapCount==1)
		{
			glTexParameteri(eTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(eTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri(eTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameteri(eTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}

	if(	(sTextureHeader.u32Width & (sTextureHeader.u32Width - 1)) | (sTextureHeader.u32Height & (sTextureHeader.u32Height - 1)))
	{
		/*
			NPOT textures requires the wrap mode to be set explicitly to
			GL_CLAMP_TO_EDGE or the texture will be inconsistent.
		*/
		glTexParameteri(eTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(eTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameteri(eTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(eTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	//Error check
	if(glGetError())
	{
		PVRTErrorOutputDebug("PVRTTextureLoadFromPointer failed: glTexParameter() failed.\n");
		return PVR_FAIL;
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function		PVRTTextureLoadFromPVR
 @Input			filename			Filename of the .PVR file to load the texture from
 @Modified		texName				the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader		Pointer to a PVRTextureHeaderV3 struct. Modified to
									contain the header data of the returned texture Ignored if NULL.
 @Input			bAllowDecompress	Allow decompression if PVRTC is not supported in hardware.
 @Input			nLoadFromLevel		Which mip map level to start loading from (0=all)
 @Modified		pMetaData			If a valid map is supplied, this will return any and all 
									MetaDataBlocks stored in the texture, organised by DevFourCC
									then identifier. Supplying NULL will ignore all MetaData.
 @Return		PVR_SUCCESS on success
 @Description	Allows textures to be stored in binary PVR files and loaded in. Can load parts of a
				mip mapped texture (i.e. skipping the highest detailed levels).
				Sets the texture MIN/MAG filter to GL_LINEAR_MIPMAP_NEAREST/GL_LINEAR
				if mip maps are present, GL_LINEAR/GL_LINEAR otherwise.
*****************************************************************************/
EPVRTError PVRTTextureLoadFromPVR(	const char * const filename,
									GLuint * const texName,
									const void *psTextureHeader,
									bool bAllowDecompress,
									const unsigned int nLoadFromLevel,
									CPVRTMap<unsigned int, CPVRTMap<unsigned int, MetaDataBlock> > *pMetaData)
{
	//Attempt to open file.
	CPVRTResourceFile TexFile(filename);

	//Check file opened successfully.
	if (!TexFile.IsOpen()) 
	{
		return PVR_FAIL;
	}

	//Header size.
	PVRTuint32 u32HeaderSize=0;

	//Boolean whether to byte swap the texture data or not.
	bool bSwapDataEndianness=false;

	//Texture header to check against.
	PVRTextureHeaderV3 sTextureHeader;

	//The channel type for endian swapping.
	EPVRTVariableType u32CurrentChannelType=ePVRTVarTypeUnsignedByte;

	//Check the first word of the file and see if it's equal to the current identifier (or reverse identifier)
	if(*(PVRTuint32*)TexFile.DataPtr()!=PVRTEX_CURR_IDENT && *(PVRTuint32*)TexFile.DataPtr()!=PVRTEX_CURR_IDENT_REV)
	{
		//Swap the header bytes if necessary.
		if(!PVRTIsLittleEndian())
		{
			bSwapDataEndianness=true;
			PVRTuint32 u32HeaderSize=PVRTByteSwap32(*(PVRTuint32*)TexFile.DataPtr());

			for (PVRTuint32 i=0; i<u32HeaderSize; ++i)
			{
				PVRTByteSwap( (PVRTuint8*)( ( (PVRTuint32*)TexFile.DataPtr() )+i),sizeof(PVRTuint32) );
			}
		}

		//Get a pointer to the header.
		PVR_Texture_Header* sLegacyTextureHeader=(PVR_Texture_Header*)TexFile.DataPtr();

		//Set the header size.
		u32HeaderSize=sLegacyTextureHeader->dwHeaderSize;

		//We only really need the channel type.
		PVRTuint64 tempFormat;
		EPVRTColourSpace tempColourSpace;
		bool tempIsPreMult;

		//Map the enum to get the channel type.
		PVRTMapLegacyTextureEnumToNewFormat( (PVRTPixelType)( sLegacyTextureHeader->dwpfFlags&0xff),tempFormat,tempColourSpace, u32CurrentChannelType, tempIsPreMult);
	}
	// If the header file has a reverse identifier, then we need to swap endianness
	else if(*(PVRTuint32*)TexFile.DataPtr()==PVRTEX_CURR_IDENT_REV)
	{
		//Setup the texture header
		sTextureHeader=*(PVRTextureHeaderV3*)TexFile.DataPtr();

		bSwapDataEndianness=true;
		PVRTextureHeaderV3* pTextureHeader=(PVRTextureHeaderV3*)TexFile.DataPtr();

		pTextureHeader->u32ChannelType=PVRTByteSwap32(pTextureHeader->u32ChannelType);
		pTextureHeader->u32ColourSpace=PVRTByteSwap32(pTextureHeader->u32ColourSpace);
		pTextureHeader->u32Depth=PVRTByteSwap32(pTextureHeader->u32Depth);
		pTextureHeader->u32Flags=PVRTByteSwap32(pTextureHeader->u32Flags);
		pTextureHeader->u32Height=PVRTByteSwap32(pTextureHeader->u32Height);
		pTextureHeader->u32MetaDataSize=PVRTByteSwap32(pTextureHeader->u32MetaDataSize);
		pTextureHeader->u32MIPMapCount=PVRTByteSwap32(pTextureHeader->u32MIPMapCount);
		pTextureHeader->u32NumFaces=PVRTByteSwap32(pTextureHeader->u32NumFaces);
		pTextureHeader->u32NumSurfaces=PVRTByteSwap32(pTextureHeader->u32NumSurfaces);
		pTextureHeader->u32Version=PVRTByteSwap32(pTextureHeader->u32Version);
		pTextureHeader->u32Width=PVRTByteSwap32(pTextureHeader->u32Width);
		PVRTByteSwap((PVRTuint8*)&pTextureHeader->u64PixelFormat,sizeof(PVRTuint64));

		//Channel type.
		u32CurrentChannelType=(EPVRTVariableType)pTextureHeader->u32ChannelType;

		//Header size.
		u32HeaderSize=PVRTEX3_HEADERSIZE+sTextureHeader.u32MetaDataSize;
	}
	else
	{
		//Header size.
		u32HeaderSize=PVRTEX3_HEADERSIZE+sTextureHeader.u32MetaDataSize;
	}

	// Convert the data if needed
	if(bSwapDataEndianness)
	{
		//Get the size of the variables types.
		PVRTuint32 ui32VariableSize=0;
		switch(u32CurrentChannelType)
		{
		case ePVRTVarTypeFloat:
		case ePVRTVarTypeUnsignedInteger:
		case ePVRTVarTypeUnsignedIntegerNorm:
		case ePVRTVarTypeSignedInteger:
		case ePVRTVarTypeSignedIntegerNorm:
			{
				ui32VariableSize=4;
				break;
			}
		case ePVRTVarTypeUnsignedShort:
		case ePVRTVarTypeUnsignedShortNorm:
		case ePVRTVarTypeSignedShort:
		case ePVRTVarTypeSignedShortNorm:
			{
				ui32VariableSize=2;
				break;
			}
		case ePVRTVarTypeUnsignedByte:
		case ePVRTVarTypeUnsignedByteNorm:
		case ePVRTVarTypeSignedByte:
		case ePVRTVarTypeSignedByteNorm:
			{
				ui32VariableSize=1;
				break;
			}
		default:
			break;
		}
		
		//If the size of the variable type is greater than 1, then we need to byte swap.
		if (ui32VariableSize>1)
		{
			//Get the texture data.
			PVRTuint8* pu8OrigData = ( (PVRTuint8*)TexFile.DataPtr() + u32HeaderSize);

			//Get the size of the texture data.
			PVRTuint32 ui32TextureDataSize = PVRTGetTextureDataSize(sTextureHeader);
				
			//Loop through and byte swap all the data. It's swapped in place so no need to do anything special.
			for(PVRTuint32 i = 0; i < ui32TextureDataSize; i+=ui32VariableSize)
			{
				PVRTByteSwap(pu8OrigData+i,ui32VariableSize);
			}
		}
	}
	
	return PVRTTextureLoadFromPointer(TexFile.DataPtr(), texName, psTextureHeader, bAllowDecompress, nLoadFromLevel,NULL,pMetaData);
}

/*!***************************************************************************
 @Function			PVRTTextureFormatGetBPP
 @Input				nFormat
 @Input				nType
 @Description		Returns the bits per pixel (BPP) of the format.
*****************************************************************************/
unsigned int PVRTTextureFormatGetBPP(const GLuint nFormat, const GLuint nType)
{
	switch(nFormat)
	{
	case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
	case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
		return 2;
	case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
	case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
		return 4;
	case GL_UNSIGNED_BYTE:
		switch(nType)
		{
		case GL_RGBA:
		case GL_BGRA:
			return 32;
		}
	case GL_UNSIGNED_SHORT_5_5_5_1:
		switch(nType)
		{
		case GL_RGBA:
			return 16;
		}
	}

	return 0xFFFFFFFF;
}

/*****************************************************************************
 End of file (PVRTTextureAPI.cpp)
*****************************************************************************/

