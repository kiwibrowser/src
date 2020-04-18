/******************************************************************************

 @File         PVRTTrans.cpp

 @Title        PVRTTrans

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Set of functions used for 3D transformations and projections.

******************************************************************************/
#include <string.h>

#include "PVRTGlobal.h"
#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"
#include "PVRTTrans.h"

/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @Function			PVRTBoundingBoxCompute
 @Output			pBoundingBox
 @Input				pV
 @Input				nNumberOfVertices
 @Description		Calculate the eight vertices that surround an object.
					This "bounding box" is used later to determine whether
					the object is visible or not.
					This function should only be called once to determine the
					object's bounding box.
*****************************************************************************/
void PVRTBoundingBoxCompute(
	PVRTBOUNDINGBOX		* const pBoundingBox,
	const PVRTVECTOR3	* const pV,
	const int			nNumberOfVertices)
{
	int			i;
	VERTTYPE	MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

	/* Inialise values to first vertex */
	MinX=pV->x;	MaxX=pV->x;
	MinY=pV->y;	MaxY=pV->y;
	MinZ=pV->z;	MaxZ=pV->z;

	/* Loop through all vertices to find extremas */
	for (i=1; i<nNumberOfVertices; i++)
	{
		/* Minimum and Maximum X */
		if (pV[i].x < MinX) MinX = pV[i].x;
		if (pV[i].x > MaxX) MaxX = pV[i].x;

		/* Minimum and Maximum Y */
		if (pV[i].y < MinY) MinY = pV[i].y;
		if (pV[i].y > MaxY) MaxY = pV[i].y;

		/* Minimum and Maximum Z */
		if (pV[i].z < MinZ) MinZ = pV[i].z;
		if (pV[i].z > MaxZ) MaxZ = pV[i].z;
	}

	/* Assign the resulting extremas to the bounding box structure */
	/* Point 0 */
	pBoundingBox->Point[0].x=MinX;
	pBoundingBox->Point[0].y=MinY;
	pBoundingBox->Point[0].z=MinZ;

	/* Point 1 */
	pBoundingBox->Point[1].x=MinX;
	pBoundingBox->Point[1].y=MinY;
	pBoundingBox->Point[1].z=MaxZ;

	/* Point 2 */
	pBoundingBox->Point[2].x=MinX;
	pBoundingBox->Point[2].y=MaxY;
	pBoundingBox->Point[2].z=MinZ;

	/* Point 3 */
	pBoundingBox->Point[3].x=MinX;
	pBoundingBox->Point[3].y=MaxY;
	pBoundingBox->Point[3].z=MaxZ;

	/* Point 4 */
	pBoundingBox->Point[4].x=MaxX;
	pBoundingBox->Point[4].y=MinY;
	pBoundingBox->Point[4].z=MinZ;

	/* Point 5 */
	pBoundingBox->Point[5].x=MaxX;
	pBoundingBox->Point[5].y=MinY;
	pBoundingBox->Point[5].z=MaxZ;

	/* Point 6 */
	pBoundingBox->Point[6].x=MaxX;
	pBoundingBox->Point[6].y=MaxY;
	pBoundingBox->Point[6].z=MinZ;

	/* Point 7 */
	pBoundingBox->Point[7].x=MaxX;
	pBoundingBox->Point[7].y=MaxY;
	pBoundingBox->Point[7].z=MaxZ;
}

/*!***************************************************************************
 @Function			PVRTBoundingBoxComputeInterleaved
 @Output			pBoundingBox
 @Input				pV
 @Input				nNumberOfVertices
 @Input				i32Offset
 @Input				i32Stride
 @Description		Calculate the eight vertices that surround an object.
					This "bounding box" is used later to determine whether
					the object is visible or not.
					This function should only be called once to determine the
					object's bounding box.
					Takes interleaved data using the first vertex's offset
					and the stride to the next vertex thereafter
*****************************************************************************/
void PVRTBoundingBoxComputeInterleaved(
	PVRTBOUNDINGBOX		* const pBoundingBox,
	const unsigned char			* const pV,
	const int			nNumberOfVertices,
	const int			i32Offset,
	const int			i32Stride)
{
	int			i;
	VERTTYPE	MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

	// point ot first vertex
	PVRTVECTOR3 *pVertex =(PVRTVECTOR3*)(pV+i32Offset);

	/* Inialise values to first vertex */
	MinX=pVertex->x;	MaxX=pVertex->x;
	MinY=pVertex->y;	MaxY=pVertex->y;
	MinZ=pVertex->z;	MaxZ=pVertex->z;

	/* Loop through all vertices to find extremas */
	for (i=1; i<nNumberOfVertices; i++)
	{
		pVertex = (PVRTVECTOR3*)( (unsigned char*)(pVertex)+i32Stride);

		/* Minimum and Maximum X */
		if (pVertex->x < MinX) MinX = pVertex->x;
		if (pVertex->x > MaxX) MaxX = pVertex->x;

		/* Minimum and Maximum Y */
		if (pVertex->y < MinY) MinY = pVertex->y;
		if (pVertex->y > MaxY) MaxY = pVertex->y;

		/* Minimum and Maximum Z */
		if (pVertex->z < MinZ) MinZ = pVertex->z;
		if (pVertex->z > MaxZ) MaxZ = pVertex->z;
	}

	/* Assign the resulting extremas to the bounding box structure */
	/* Point 0 */
	pBoundingBox->Point[0].x=MinX;
	pBoundingBox->Point[0].y=MinY;
	pBoundingBox->Point[0].z=MinZ;

	/* Point 1 */
	pBoundingBox->Point[1].x=MinX;
	pBoundingBox->Point[1].y=MinY;
	pBoundingBox->Point[1].z=MaxZ;

	/* Point 2 */
	pBoundingBox->Point[2].x=MinX;
	pBoundingBox->Point[2].y=MaxY;
	pBoundingBox->Point[2].z=MinZ;

	/* Point 3 */
	pBoundingBox->Point[3].x=MinX;
	pBoundingBox->Point[3].y=MaxY;
	pBoundingBox->Point[3].z=MaxZ;

	/* Point 4 */
	pBoundingBox->Point[4].x=MaxX;
	pBoundingBox->Point[4].y=MinY;
	pBoundingBox->Point[4].z=MinZ;

	/* Point 5 */
	pBoundingBox->Point[5].x=MaxX;
	pBoundingBox->Point[5].y=MinY;
	pBoundingBox->Point[5].z=MaxZ;

	/* Point 6 */
	pBoundingBox->Point[6].x=MaxX;
	pBoundingBox->Point[6].y=MaxY;
	pBoundingBox->Point[6].z=MinZ;

	/* Point 7 */
	pBoundingBox->Point[7].x=MaxX;
	pBoundingBox->Point[7].y=MaxY;
	pBoundingBox->Point[7].z=MaxZ;
}

/*!******************************************************************************
 @Function			PVRTBoundingBoxIsVisible
 @Output			pNeedsZClipping
 @Input				pBoundingBox
 @Input				pMatrix
 @Return			TRUE if the object is visible, FALSE if not.
 @Description		Determine if a bounding box is "visible" or not along the
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
	bool					* const pNeedsZClipping)
{
	VERTTYPE	fX, fY, fZ, fW;
	int			i, nX0, nX1, nY0, nY1, nZ;

	nX0 = 8;
	nX1 = 8;
	nY0 = 8;
	nY1 = 8;
	nZ  = 8;

	/* Transform the eight bounding box vertices */
	i = 8;
	while(i)
	{
		i--;
		fX =	pMatrix->f[ 0]*pBoundingBox->Point[i].x +
				pMatrix->f[ 4]*pBoundingBox->Point[i].y +
				pMatrix->f[ 8]*pBoundingBox->Point[i].z +
				pMatrix->f[12];
		fY =	pMatrix->f[ 1]*pBoundingBox->Point[i].x +
				pMatrix->f[ 5]*pBoundingBox->Point[i].y +
				pMatrix->f[ 9]*pBoundingBox->Point[i].z +
				pMatrix->f[13];
		fZ =	pMatrix->f[ 2]*pBoundingBox->Point[i].x +
				pMatrix->f[ 6]*pBoundingBox->Point[i].y +
				pMatrix->f[10]*pBoundingBox->Point[i].z +
				pMatrix->f[14];
		fW =	pMatrix->f[ 3]*pBoundingBox->Point[i].x +
				pMatrix->f[ 7]*pBoundingBox->Point[i].y +
				pMatrix->f[11]*pBoundingBox->Point[i].z +
				pMatrix->f[15];

		if(fX < -fW)
			nX0--;
		else if(fX > fW)
			nX1--;

		if(fY < -fW)
			nY0--;
		else if(fY > fW)
			nY1--;

		if(fZ < 0)
			nZ--;
	}

	if(nZ)
	{
		if(!(nX0 * nX1 * nY0 * nY1))
		{
			*pNeedsZClipping = false;
			return false;
		}

		if(nZ == 8)
		{
			*pNeedsZClipping = false;
			return true;
		}

		*pNeedsZClipping = true;
		return true;
	}
	else
	{
		*pNeedsZClipping = false;
		return false;
	}
}

/*!***************************************************************************
 @Function Name		PVRTTransformVec3Array
 @Output			pOut				Destination for transformed vectors
 @Input				nOutStride			Stride between vectors in pOut array
 @Input				pV					Input vector array
 @Input				nInStride			Stride between vectors in pV array
 @Input				pMatrix				Matrix to transform the vectors
 @Input				nNumberOfVertices	Number of vectors to transform
 @Description		Transform all vertices [X Y Z 1] in pV by pMatrix and
 					store them in pOut.
*****************************************************************************/
void PVRTTransformVec3Array(
	PVRTVECTOR4			* const pOut,
	const int			nOutStride,
	const PVRTVECTOR3	* const pV,
	const int			nInStride,
	const PVRTMATRIX	* const pMatrix,
	const int			nNumberOfVertices)
{
	const PVRTVECTOR3	*pSrc;
	PVRTVECTOR4			*pDst;
	int					i;

	pSrc = pV;
	pDst = pOut;

	/* Transform all vertices with *pMatrix */
	for (i=0; i<nNumberOfVertices; ++i)
	{
		pDst->x =	VERTTYPEMUL(pMatrix->f[ 0], pSrc->x) +
					VERTTYPEMUL(pMatrix->f[ 4], pSrc->y) +
					VERTTYPEMUL(pMatrix->f[ 8], pSrc->z) +
					pMatrix->f[12];
		pDst->y =	VERTTYPEMUL(pMatrix->f[ 1], pSrc->x) +
					VERTTYPEMUL(pMatrix->f[ 5], pSrc->y) +
					VERTTYPEMUL(pMatrix->f[ 9], pSrc->z) +
					pMatrix->f[13];
		pDst->z =	VERTTYPEMUL(pMatrix->f[ 2], pSrc->x) +
					VERTTYPEMUL(pMatrix->f[ 6], pSrc->y) +
					VERTTYPEMUL(pMatrix->f[10], pSrc->z) +
					pMatrix->f[14];
		pDst->w =	VERTTYPEMUL(pMatrix->f[ 3], pSrc->x) +
					VERTTYPEMUL(pMatrix->f[ 7], pSrc->y) +
					VERTTYPEMUL(pMatrix->f[11], pSrc->z) +
					pMatrix->f[15];

		pDst = (PVRTVECTOR4*)((char*)pDst + nOutStride);
		pSrc = (PVRTVECTOR3*)((char*)pSrc + nInStride);
	}
}

/*!***************************************************************************
 @Function			PVRTTransformArray
 @Output			pTransformedVertex	Destination for transformed vectors
 @Input				pV					Input vector array
 @Input				nNumberOfVertices	Number of vectors to transform
 @Input				pMatrix				Matrix to transform the vectors
 @Input				fW					W coordinate of input vector (e.g. use 1 for position, 0 for normal)
 @Description		Transform all vertices in pVertex by pMatrix and store them in
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
	const VERTTYPE		fW)
{
	int			i;

	/* Transform all vertices with *pMatrix */
	for (i=0; i<nNumberOfVertices; ++i)
	{
		pTransformedVertex[i].x =	VERTTYPEMUL(pMatrix->f[ 0], pV[i].x) +
									VERTTYPEMUL(pMatrix->f[ 4], pV[i].y) +
									VERTTYPEMUL(pMatrix->f[ 8], pV[i].z) +
									VERTTYPEMUL(pMatrix->f[12], fW);
		pTransformedVertex[i].y =	VERTTYPEMUL(pMatrix->f[ 1], pV[i].x) +
									VERTTYPEMUL(pMatrix->f[ 5], pV[i].y) +
									VERTTYPEMUL(pMatrix->f[ 9], pV[i].z) +
									VERTTYPEMUL(pMatrix->f[13], fW);
		pTransformedVertex[i].z =	VERTTYPEMUL(pMatrix->f[ 2], pV[i].x) +
									VERTTYPEMUL(pMatrix->f[ 6], pV[i].y) +
									VERTTYPEMUL(pMatrix->f[10], pV[i].z) +
									VERTTYPEMUL(pMatrix->f[14], fW);
	}
}

/*!***************************************************************************
 @Function			PVRTTransformArrayBack
 @Output			pTransformedVertex
 @Input				pVertex
 @Input				nNumberOfVertices
 @Input				pMatrix
 @Description		Transform all vertices in pVertex by the inverse of pMatrix
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
	const PVRTMATRIX	* const pMatrix)
{
	PVRTMATRIX	mBack;

	PVRTMatrixInverse(mBack, *pMatrix);
	PVRTTransformArray(pTransformedVertex, pVertex, nNumberOfVertices, &mBack);
}

/*!***************************************************************************
 @Function			PVRTTransformBack
 @Output			pOut
 @Input				pV
 @Input				pM
 @Description		Transform vertex pV by the inverse of pMatrix
					and store in pOut.
*****************************************************************************/
void PVRTTransformBack(
	PVRTVECTOR4			* const pOut,
	const PVRTVECTOR4	* const pV,
	const PVRTMATRIX	* const pM)
{
	VERTTYPE *ppfRows[4];
	VERTTYPE pfIn[20];
	int i;
	const PVRTMATRIX	*pMa;

#if defined(BUILD_OGL) || defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	PVRTMATRIX mT;
	PVRTMatrixTranspose(mT, *pM);
	pMa = &mT;
#else
	pMa = pM;
#endif

	for(i = 0; i < 4; ++i)
	{
		/*
			Set up the array of pointers to matrix coefficients
		*/
		ppfRows[i] = &pfIn[i * 5];

		/*
			Copy the 4x4 matrix into RHS of the 5x4 matrix
		*/
		memcpy(&ppfRows[i][1], &pMa->f[i * 4], 4 * sizeof(float));
	}

	/*
		Copy the "result" vector into the first column of the 5x4 matrix
	*/
	ppfRows[0][0] = pV->x;
	ppfRows[1][0] = pV->y;
	ppfRows[2][0] = pV->z;
	ppfRows[3][0] = pV->w;

	/*
		Solve a set of 4 linear equations
	*/
	PVRTMatrixLinearEqSolve(&pOut->x, ppfRows, 4);
}

/*!***************************************************************************
 @Function			PVRTTransform
 @Output			pOut
 @Input				pV
 @Input				pM
 @Description		Transform vertex pV by pMatrix and store in pOut.
*****************************************************************************/
void PVRTTransform(
	PVRTVECTOR4			* const pOut,
	const PVRTVECTOR4	* const pV,
	const PVRTMATRIX	* const pM)
{
	pOut->x = VERTTYPEMUL(pM->f[0], pV->x) + VERTTYPEMUL(pM->f[4], pV->y) + VERTTYPEMUL(pM->f[8],  pV->z) + VERTTYPEMUL(pM->f[12], pV->w);
	pOut->y = VERTTYPEMUL(pM->f[1], pV->x) + VERTTYPEMUL(pM->f[5], pV->y) + VERTTYPEMUL(pM->f[9],  pV->z) + VERTTYPEMUL(pM->f[13], pV->w);
	pOut->z = VERTTYPEMUL(pM->f[2], pV->x) + VERTTYPEMUL(pM->f[6], pV->y) + VERTTYPEMUL(pM->f[10], pV->z) + VERTTYPEMUL(pM->f[14], pV->w);
	pOut->w = VERTTYPEMUL(pM->f[3], pV->x) + VERTTYPEMUL(pM->f[7], pV->y) + VERTTYPEMUL(pM->f[11], pV->z) + VERTTYPEMUL(pM->f[15], pV->w);
}

/*****************************************************************************
 End of file (PVRTTrans.cpp)
*****************************************************************************/

