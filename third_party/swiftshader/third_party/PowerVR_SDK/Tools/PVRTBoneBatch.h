/*!****************************************************************************

 @file         PVRTBoneBatch.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Utility functions which process vertices.

******************************************************************************/
#ifndef _PVRTBONEBATCH_H_
#define _PVRTBONEBATCH_H_

#include "PVRTVertex.h"
#include <stdlib.h>

/*!***************************************************************************
 Handles a batch of bones
*****************************************************************************/
/*!***************************************************************************
 @class CPVRTBoneBatches
 @brief A class for processing vertices into bone batches
*****************************************************************************/
class CPVRTBoneBatches
{
public:
	int	*pnBatches;			/*!< Space for nBatchBoneMax bone indices, per batch */
	int	*pnBatchBoneCnt;	/*!< Actual number of bone indices, per batch */
	int	*pnBatchOffset;		/*!< Offset into triangle array, per batch */
	int nBatchBoneMax;		/*!< Stored value as was passed into Create() */
	int	nBatchCnt;			/*!< Number of batches to render */

	/*!***********************************************************************
	 @brief      	Fills the bone batch structure
	 @param[out]	pnVtxNumOut		vertex count
	 @param[out]	pVtxOut			Output vertices (program must free() this)
	 @param[in,out]	pui32Idx		index array for triangle list
	 @param[in]		nVtxNum			vertex count
	 @param[in]		pVtx			vertices
	 @param[in]		nStride			Size of a vertex (in bytes)
	 @param[in]		nOffsetWeight	Offset in bytes to the vertex bone-weights
	 @param[in]		eTypeWeight		Data type of the vertex bone-weights
	 @param[in]		nOffsetIdx		Offset in bytes to the vertex bone-indices
	 @param[in]		eTypeIdx		Data type of the vertex bone-indices
	 @param[in]		nTriNum			Number of triangles
	 @param[in]		nBatchBoneMax	Number of bones a batch can reference
	 @param[in]		nVertexBones	Number of bones affecting each vertex
	 @return		PVR_SUCCESS if successful
	*************************************************************************/
	EPVRTError Create(
		int					* const pnVtxNumOut,
		char				** const pVtxOut,
		unsigned int		* const pui32Idx,
		const int			nVtxNum,
		const char			* const pVtx,
		const int			nStride,
		const int			nOffsetWeight,
		const EPVRTDataType	eTypeWeight,
		const int			nOffsetIdx,
		const EPVRTDataType	eTypeIdx,
		const int			nTriNum,
		const int			nBatchBoneMax,
		const int			nVertexBones);

	/*!***********************************************************************
	 @brief      	Destroy the bone batch structure
	*************************************************************************/
	void Release()
	{
		FREE(pnBatches);
		FREE(pnBatchBoneCnt);
		FREE(pnBatchOffset);
		nBatchCnt = 0;
	}
};


#endif /* _PVRTBONEBATCH_H_ */

/*****************************************************************************
 End of file (PVRTBoneBatch.h)
*****************************************************************************/

