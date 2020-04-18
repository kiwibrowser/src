/*!****************************************************************************

 @file         PVRTTrans.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Set of functions used for 3D transformations and projections.

******************************************************************************/
#ifndef _PVRTTRANS_H_
#define _PVRTTRANS_H_


/****************************************************************************
** Typedefs
****************************************************************************/
/*!***************************************************************************
 @brief      		PVRTBOUNDINGBOX is a typedef of a PVRTBOUNDINGBOX_TAG struct.
*****************************************************************************/
typedef struct PVRTBOUNDINGBOX_TAG
{
	PVRTVECTOR3	Point[8];       ///< 8 Vertices
} PVRTBOUNDINGBOX, *LPPVRTBOUNDINGBOX;

/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @fn       			PVRTBoundingBoxCompute
 @param[out]		pBoundingBox
 @param[in]			pV
 @param[in]			nNumberOfVertices
 @brief      		Calculate the eight vertices that surround an object.
					This "bounding box" is used later to determine whether
					the object is visible or not.
					This function should only be called once to determine the
					object's bounding box.
*****************************************************************************/
void PVRTBoundingBoxCompute(
	PVRTBOUNDINGBOX		* const pBoundingBox,
	const PVRTVECTOR3	* const pV,
	const int			nNumberOfVertices);

/*!***************************************************************************
 @fn       			PVRTBoundingBoxComputeInterleaved
 @param[out]		pBoundingBox
 @param[in]			pV
 @param[in]			nNumberOfVertices
 @param[in]			i32Offset
 @param[in]			i32Stride
 @brief      		Calculate the eight vertices that surround an object.
					This "bounding box" is used later to determine whether
					the object is visible or not.
					This function should only be called once to determine the
					object's bounding box.
					Takes interleaved data using the first vertex's offset
					and the stride to the next vertex thereafter
*****************************************************************************/
void PVRTBoundingBoxComputeInterleaved(
	PVRTBOUNDINGBOX		* const pBoundingBox,
	const unsigned char	* const pV,
	const int			nNumberOfVertices,
	const int			i32Offset,
	const int			i32Stride);

/*!******************************************************************************
 @fn       			PVRTBoundingBoxIsVisible
 @param[out]		pNeedsZClipping
 @param[in]			pBoundingBox
 @param[in]			pMatrix
 @return			TRUE if the object is visible, FALSE if not.
 @brief      		Determine if a bounding box is "visible" or not along the
					Z axis.
					If the function returns TRUE, the object is visible and should
					be displayed (check bNeedsZClipping to know if Z Clipping needs
					to be done).
					If the function returns FALSE, the object is not visible and thus
					does not require to be displayed.
					bNeedsZClipping indicates whether the object needs Z Clipping
					(i.e. the object is partially visible).
					- *pBoundingBox is a pointer to the bounding box structure.
					- *pMatrix is the World, View & Projection matrices combined.
					- *bNeedsZClipping is TRUE if Z clipping is required.
*****************************************************************************/
bool PVRTBoundingBoxIsVisible(
	const PVRTBOUNDINGBOX	* const pBoundingBox,
	const PVRTMATRIX		* const pMatrix,
	bool					* const pNeedsZClipping);

/*!***************************************************************************
 @fn                PVRTTransformVec3Array
 @param[out]		pOut				Destination for transformed vectors
 @param[in]			nOutStride			Stride between vectors in pOut array
 @param[in]			pV					Input vector array
 @param[in]			nInStride			Stride between vectors in pV array
 @param[in]			pMatrix				Matrix to transform the vectors
 @param[in]			nNumberOfVertices	Number of vectors to transform
 @brief      		Transform all vertices [X Y Z 1] in pV by pMatrix and
 					store them in pOut.
*****************************************************************************/
void PVRTTransformVec3Array(
	PVRTVECTOR4			* const pOut,
	const int			nOutStride,
	const PVRTVECTOR3	* const pV,
	const int			nInStride,
	const PVRTMATRIX	* const pMatrix,
	const int			nNumberOfVertices);

/*!***************************************************************************
 @fn       			PVRTTransformArray
 @param[out]		pTransformedVertex	Destination for transformed vectors
 @param[in]			pV					Input vector array
 @param[in]			nNumberOfVertices	Number of vectors to transform
 @param[in]			pMatrix				Matrix to transform the vectors
 @param[in]			fW					W coordinate of input vector (e.g. use 1 for position, 0 for normal)
 @brief      		Transform all vertices in pVertex by pMatrix and store them in
					pTransformedVertex
					- pTransformedVertex is the pointer that will receive transformed vertices.
					- pVertex is the pointer to untransformed object vertices.
					- nNumberOfVertices is the number of vertices of the object.
					- pMatrix is the matrix used to transform the object.
*****************************************************************************/
void PVRTTransformArray(
	PVRTVECTOR3			* const pTransformedVertex,
	const PVRTVECTOR3	* const pV,
	const int			nNumberOfVertices,
	const PVRTMATRIX	* const pMatrix,
	const VERTTYPE		fW = f2vt(1.0f));

/*!***************************************************************************
 @fn       			PVRTTransformArrayBack
 @param[out]		pTransformedVertex
 @param[in]			pVertex
 @param[in]			nNumberOfVertices
 @param[in]			pMatrix
 @brief      		Transform all vertices in pVertex by the inverse of pMatrix
					and store them in pTransformedVertex.
					- pTransformedVertex is the pointer that will receive transformed vertices.
					- pVertex is the pointer to untransformed object vertices.
					- nNumberOfVertices is the number of vertices of the object.
					- pMatrix is the matrix used to transform the object.
*****************************************************************************/
void PVRTTransformArrayBack(
	PVRTVECTOR3			* const pTransformedVertex,
	const PVRTVECTOR3	* const pVertex,
	const int			nNumberOfVertices,
	const PVRTMATRIX	* const pMatrix);

/*!***************************************************************************
 @fn       			PVRTTransformBack
 @param[out]		pOut
 @param[in]			pV
 @param[in]			pM
 @brief      		Transform vertex pV by the inverse of pMatrix
					and store in pOut.
*****************************************************************************/
void PVRTTransformBack(
	PVRTVECTOR4			* const pOut,
	const PVRTVECTOR4	* const pV,
	const PVRTMATRIX	* const pM);

/*!***************************************************************************
 @fn       			PVRTTransform
 @param[out]		pOut
 @param[in]			pV
 @param[in]			pM
 @brief      		Transform vertex pV by pMatrix and store in pOut.
*****************************************************************************/
void PVRTTransform(
	PVRTVECTOR4			* const pOut,
	const PVRTVECTOR4	* const pV,
	const PVRTMATRIX	* const pM);


#endif /* _PVRTTRANS_H_ */

/*****************************************************************************
 End of file (PVRTTrans.h)
*****************************************************************************/

