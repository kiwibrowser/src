/*!****************************************************************************

 @file         PVRTMisc.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Miscellaneous functions used in 3D rendering.

******************************************************************************/
#ifndef _PVRTMISC_H_
#define _PVRTMISC_H_

#include "PVRTMatrix.h"
#include "PVRTFixedPoint.h"

/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @brief      	Calculates coords of the intersection of a line and an
				infinite plane
 @param[out]	pvIntersection	The point of intersection
 @param[in]		pfPlane			Length 4 [A,B,C,D], values for plane equation
 @param[in]		pv0				A point on the line
 @param[in]		pv1				Another point on the line
*****************************************************************************/
void PVRTMiscCalculateIntersectionLinePlane(
	PVRTVECTOR3			* const pvIntersection,
	const VERTTYPE		pfPlane[4],
	const PVRTVECTOR3	* const pv0,
	const PVRTVECTOR3	* const pv1);

/*!***************************************************************************
 @brief      	Calculates world-space coords of a screen-filling
				representation of an infinite plane The resulting vertices run
				counter-clockwise around the screen, and can be simply drawn using
				non-indexed TRIANGLEFAN
 @param[out]	pfVtx			Position of the first of 3 floats to receive
								the position of vertex 0; up to 5 vertex positions
								will be written (5 is the maximum number of vertices
								required to draw an infinite polygon clipped to screen
								and far clip plane).
 @param[in]		nStride			Size of each vertex structure containing pfVtx
 @param[in]		pvPlane			Length 4 [A,B,C,D], values for plane equation
 @param[in]		pmViewProjInv	The inverse of the View Projection matrix
 @param[in]		pFrom			Position of the camera
 @param[in]		fFar			Far clipping distance
 @return		Number of vertices in the polygon fan (Can be 0, 3, 4 or 5)
*****************************************************************************/
int PVRTMiscCalculateInfinitePlane(
	VERTTYPE			* const pfVtx,
	const int			nStride,
	const PVRTVECTOR4	* const pvPlane,
	const PVRTMATRIX 	* const pmViewProjInv,
	const PVRTVECTOR3	* const pFrom,
	const VERTTYPE		fFar);

/*!***************************************************************************
 @brief      	Creates the vertices and texture coordinates for a skybox
 @param[in]		scale			Scale the skybox
 @param[in]		adjustUV		Adjust or not UVs for PVRT compression
 @param[in]		textureSize		Texture size in pixels
 @param[out]	Vertices		Array of vertices
 @param[out]	UVs				Array of UVs
*****************************************************************************/
void PVRTCreateSkybox(float scale, bool adjustUV, int textureSize, VERTTYPE** Vertices, VERTTYPE** UVs);

/*!***************************************************************************
 @brief      	Destroy the memory allocated for a skybox
 @param[in]		Vertices	    Vertices array to destroy
 @param[in]		UVs			    UVs array to destroy
*****************************************************************************/
void PVRTDestroySkybox(VERTTYPE* Vertices, VERTTYPE* UVs);

/*!***************************************************************************
 @brief      	When iTimesHigher is one, this function will return the closest
				power-of-two value above the base value.
				For every increment beyond one for the iTimesHigher value,
				the next highest power-of-two value will be calculated.
 @param[in]		uiOriginalValue	    Base value
 @param[in]		iTimesHigher		Multiplier
*****************************************************************************/
unsigned int PVRTGetPOTHigher(unsigned int uiOriginalValue, int iTimesHigher);

/*!***************************************************************************
 @brief      	When iTimesLower is one, this function will return the closest
				power-of-two value below the base value.
				For every increment beyond one for the iTimesLower value,
				the next lowest power-of-two value will be calculated. The lowest
				value that can be reached is 1.
 @param[in]		uiOriginalValue	    Base value
 @param[in]		iTimesLower		    Multiplier
*****************************************************************************/
unsigned int PVRTGetPOTLower(unsigned int uiOriginalValue, int iTimesLower);

#endif /* _PVRTMISC_H_ */


/*****************************************************************************
 End of file (PVRTMisc.h)
*****************************************************************************/

