/*!****************************************************************************

 @file         PVRTGeometry.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Code to affect triangle mesh geometry.

******************************************************************************/
#ifndef _PVRTGEOMETRY_H_
#define _PVRTGEOMETRY_H_


/****************************************************************************
** Defines
****************************************************************************/
#define PVRTGEOMETRY_IDX	unsigned int

#define PVRTGEOMETRY_SORT_VERTEXCACHE (0x01	/* Sort triangles for optimal vertex cache usage */)
#define PVRTGEOMETRY_SORT_IGNOREVERTS (0x02	/* Do not sort vertices for optimal memory cache usage */)

/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @brief      	    Triangle sorter
 @param[in,out]		pVtxData		Pointer to array of vertices
 @param[in,out]		pwIdx			Pointer to array of indices
 @param[in]			nStride			Size of a vertex (in bytes)
 @param[in]			nVertNum		Number of vertices. Length of pVtxData array
 @param[in]			nTriNum			Number of triangles. Length of pwIdx array is 3* this
 @param[in]			nBufferVtxLimit	Number of vertices that can be stored in a buffer
 @param[in]			nBufferTriLimit	Number of triangles that can be stored in a buffer
 @param[in]			dwFlags			PVRTGEOMETRY_SORT_* flags
*****************************************************************************/
void PVRTGeometrySort(
	void				* const pVtxData,
	PVRTGEOMETRY_IDX	* const pwIdx,
	const int			nStride,
	const int			nVertNum,
	const int			nTriNum,
	const int			nBufferVtxLimit,
	const int			nBufferTriLimit,
	const unsigned int	dwFlags);


#endif /* _PVRTGEOMETRY_H_ */

/*****************************************************************************
 End of file (PVRTGeometry.h)
*****************************************************************************/

