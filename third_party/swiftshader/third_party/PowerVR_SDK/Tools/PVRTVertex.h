/*!****************************************************************************

 @file         PVRTVertex.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Utility functions which process vertices.

******************************************************************************/
#ifndef _PVRTVERTEX_H_
#define _PVRTVERTEX_H_

#include "PVRTError.h"
#include "PVRTMatrix.h"

/****************************************************************************
** Enumerations
****************************************************************************/
enum EPVRTDataType {
	EPODDataNone,
	EPODDataFloat,
	EPODDataInt,
	EPODDataUnsignedShort,
	EPODDataRGBA,
	EPODDataARGB,
	EPODDataD3DCOLOR,
	EPODDataUBYTE4,
	EPODDataDEC3N,
	EPODDataFixed16_16,
	EPODDataUnsignedByte,
	EPODDataShort,
	EPODDataShortNorm,
	EPODDataByte,
	EPODDataByteNorm,
	EPODDataUnsignedByteNorm,
	EPODDataUnsignedShortNorm,
	EPODDataUnsignedInt,
	EPODDataABGR
};

/*****************************************************************************
** Functions
*****************************************************************************/

/*!***************************************************************************
 @fn       			PVRTVertexRead
 @param[out]			pV
 @param[in]				pData
 @param[in]				eType
 @param[in]				nCnt
 @brief      		Read a vector
*****************************************************************************/
void PVRTVertexRead(
	PVRTVECTOR4f		* const pV,
	const void			* const pData,
	const EPVRTDataType	eType,
	const int			nCnt);

/*!***************************************************************************
 @fn       			PVRTVertexRead
 @param[out]			pV
 @param[in]				pData
 @param[in]				eType
 @brief      		Read an int
*****************************************************************************/
void PVRTVertexRead(
	unsigned int		* const pV,
	const void			* const pData,
	const EPVRTDataType	eType);

/*!***************************************************************************
 @fn       			PVRTVertexWrite
 @param[out]			pOut
 @param[in]				eType
 @param[in]				nCnt
 @param[in]				pV
 @brief      		Write a vector
*****************************************************************************/
void PVRTVertexWrite(
	void				* const pOut,
	const EPVRTDataType	eType,
	const int			nCnt,
	const PVRTVECTOR4f	* const pV);

/*!***************************************************************************
 @fn       			PVRTVertexWrite
 @param[out]			pOut
 @param[in]				eType
 @param[in]				V
 @brief      		Write an int
*****************************************************************************/
void PVRTVertexWrite(
	void				* const pOut,
	const EPVRTDataType	eType,
	const unsigned int	V);

/*!***************************************************************************
 @fn       			PVRTVertexTangentBitangent
 @param[out]			pvTan
 @param[out]			pvBin
 @param[in]				pvNor
 @param[in]				pfPosA
 @param[in]				pfPosB
 @param[in]				pfPosC
 @param[in]				pfTexA
 @param[in]				pfTexB
 @param[in]				pfTexC
 @brief      		Calculates the tangent and bitangent vectors for
					vertex 'A' of the triangle defined by the 3 supplied
					3D position coordinates (pfPosA) and 2D texture
					coordinates (pfTexA).
*****************************************************************************/
void PVRTVertexTangentBitangent(
	PVRTVECTOR3			* const pvTan,
	PVRTVECTOR3			* const pvBin,
	const PVRTVECTOR3	* const pvNor,
	const float			* const pfPosA,
	const float			* const pfPosB,
	const float			* const pfPosC,
	const float			* const pfTexA,
	const float			* const pfTexB,
	const float			* const pfTexC);

/*!***************************************************************************
 @fn       			PVRTVertexGenerateTangentSpace
 @param[out]			pnVtxNumOut			Output vertex count
 @param[out]			pVtxOut				Output vertices (program must free() this)
 @param[in,out]			pui32Idx			input AND output; index array for triangle list
 @param[in]				nVtxNum				Input vertex count
 @param[in]				pVtx				Input vertices
 @param[in]				nStride				Size of a vertex (in bytes)
 @param[in]				nOffsetPos			Offset in bytes to the vertex position
 @param[in]				eTypePos			Data type of the position
 @param[in]				nOffsetNor			Offset in bytes to the vertex normal
 @param[in]				eTypeNor			Data type of the normal
 @param[in]				nOffsetTex			Offset in bytes to the vertex texture coordinate to use
 @param[in]				eTypeTex			Data type of the texture coordinate
 @param[in]				nOffsetTan			Offset in bytes to the vertex tangent
 @param[in]				eTypeTan			Data type of the tangent
 @param[in]				nOffsetBin			Offset in bytes to the vertex bitangent
 @param[in]				eTypeBin			Data type of the bitangent
 @param[in]				nTriNum				Number of triangles
 @param[in]				fSplitDifference	Split a vertex if the DP3 of tangents/bitangents are below this (range -1..1)
 @return			PVR_FAIL if there was a problem.
 @brief      		Calculates the tangent space for all supplied vertices.
					Writes tangent and bitangent vectors to the output
					vertices, copies all other elements from input vertices.
					Will split vertices if necessary - i.e. if two triangles
					sharing a vertex want to assign it different
					tangent-space matrices. The decision whether to split
					uses fSplitDifference - of the DP3 of two desired
					tangents or two desired bitangents is higher than this,
					the vertex will be split.
*****************************************************************************/
EPVRTError PVRTVertexGenerateTangentSpace(
	unsigned int	* const pnVtxNumOut,
	char			** const pVtxOut,
	unsigned int	* const pui32Idx,
	const unsigned int	nVtxNum,
	const char		* const pVtx,
	const unsigned int	nStride,
	const unsigned int	nOffsetPos,
	EPVRTDataType	eTypePos,
	const unsigned int	nOffsetNor,
	EPVRTDataType	eTypeNor,
	const unsigned int	nOffsetTex,
	EPVRTDataType	eTypeTex,
	const unsigned int	nOffsetTan,
	EPVRTDataType	eTypeTan,
	const unsigned int	nOffsetBin,
	EPVRTDataType	eTypeBin,
	const unsigned int	nTriNum,
	const float		fSplitDifference);


#endif /* _PVRTVERTEX_H_ */

/*****************************************************************************
 End of file (PVRTVertex.h)
*****************************************************************************/

