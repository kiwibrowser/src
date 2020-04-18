/*!****************************************************************************

 @file         OGLES2/PVRTTextureAPI.h
 @ingroup      API_OGLES2
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        OGLES2 texture loading.

******************************************************************************/
#ifndef _PVRTTEXTUREAPI_H_
#define _PVRTTEXTUREAPI_H_

/*!
 @addtogroup API_OGLES2
 @{
*/

#include "../PVRTError.h"

/****************************************************************************
** Functions
****************************************************************************/
template <typename KeyType, typename DataType>
class CPVRTMap;

/*!***************************************************************************
 @brief      	Allows textures to be stored in C header files and loaded in. Can load parts of a
				mipmaped texture (ie skipping the highest detailed levels).
				Sets the texture MIN/MAG filter to GL_LINEAR_MIPMAP_NEAREST/GL_LINEAR
				if mipmaps are present, GL_LINEAR/GL_LINEAR otherwise.
 @param[in]		pointer				Pointer to header-texture's structure
 @param[in,out]	texName				the OpenGL ES texture name as returned by glBindTexture
 @param[in,out]	psTextureHeader		Pointer to a PVRTextureHeaderV3 struct. Modified to
									contain the header data of the returned texture Ignored if NULL.
 @param[in]		bAllowDecompress	Allow decompression if PVRTC is not supported in hardware.
 @param[in]		nLoadFromLevel		Which mipmap level to start loading from (0=all)
 @param[in]		texPtr				If null, texture follows header, else texture is here.
 @param[in,out]	pMetaData			If a valid map is supplied, this will return any and all 
									MetaDataBlocks stored in the texture, organised by DevFourCC
									then identifier. Supplying NULL will ignore all MetaData.
 @return		PVR_SUCCESS on success
*****************************************************************************/
EPVRTError PVRTTextureLoadFromPointer(	const void* pointer,
										GLuint *const texName,
										const void *psTextureHeader=NULL,
										bool bAllowDecompress = true,
										const unsigned int nLoadFromLevel=0,
										const void * const texPtr=0,
										CPVRTMap<unsigned int, CPVRTMap<unsigned int, struct MetaDataBlock> > *pMetaData=NULL);

/*!***************************************************************************
 @brief      	Allows textures to be stored in binary PVR files and loaded in. Can load parts of a
				mipmaped texture (ie skipping the highest detailed levels).
				Sets the texture MIN/MAG filter to GL_LINEAR_MIPMAP_NEAREST/GL_LINEAR
				if mipmaps are present, GL_LINEAR/GL_LINEAR otherwise.
 @param[in]		filename			Filename of the .PVR file to load the texture from
 @param[in,out]	texName				the OpenGL ES texture name as returned by glBindTexture
 @param[in,out]	psTextureHeader		Pointer to a PVRTextureHeaderV3 struct. Modified to
									contain the header data of the returned texture Ignored if NULL.
 @param[in]		bAllowDecompress	Allow decompression if PVRTC is not supported in hardware.
 @param[in]		nLoadFromLevel		Which mipmap level to start loading from (0=all)
 @param[in,out]	pMetaData			If a valid map is supplied, this will return any and all 
									MetaDataBlocks stored in the texture, organised by DevFourCC
									then identifier. Supplying NULL will ignore all MetaData.
 @return		PVR_SUCCESS on success
*****************************************************************************/
EPVRTError PVRTTextureLoadFromPVR(	const char * const filename,
									GLuint * const texName,
									const void *psTextureHeader=NULL,
									bool bAllowDecompress = true,
									const unsigned int nLoadFromLevel=0,
									CPVRTMap<unsigned int, CPVRTMap<unsigned int, struct MetaDataBlock> > *pMetaData=NULL);

/*!***************************************************************************
 @brief      		Returns the bits per pixel (BPP) of the format.
 @param[in]			nFormat
 @param[in]			nType
 @return            Unsigned integer representing the bits per pixel of the format
*****************************************************************************/
unsigned int PVRTTextureFormatGetBPP(const GLuint nFormat, const GLuint nType);

/*! @} */

#endif /* _PVRTTEXTUREAPI_H_ */

/*****************************************************************************
 End of file (PVRTTextureAPI.h)
*****************************************************************************/

