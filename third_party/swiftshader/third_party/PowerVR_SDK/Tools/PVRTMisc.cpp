/******************************************************************************

 @File         PVRTMisc.cpp

 @Title        PVRTMisc

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Miscellaneous functions used in 3D rendering.

******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include "PVRTGlobal.h"
#include "PVRTContext.h"
#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"
#include "PVRTMisc.h"



/*!***************************************************************************
 @Function			PVRTMiscCalculateIntersectionLinePlane
 @Input				pfPlane			Length 4 [A,B,C,D], values for plane
									equation
 @Input				pv0				A point on the line
 @Input				pv1				Another point on the line
 @Output			pvIntersection	The point of intersection
 @Description		Calculates coords of the intersection of a line and an
					infinite plane
*****************************************************************************/
void PVRTMiscCalculateIntersectionLinePlane(
	PVRTVECTOR3			* const pvIntersection,
	const VERTTYPE		pfPlane[4],
	const PVRTVECTOR3	* const pv0,
	const PVRTVECTOR3	* const pv1)
{
	PVRTVECTOR3	vD;
	VERTTYPE fN, fD, fT;

	/* Calculate vector from point0 to point1 */
	vD.x = pv1->x - pv0->x;
	vD.y = pv1->y - pv0->y;
	vD.z = pv1->z - pv0->z;

	/* Denominator */
	fD =
		VERTTYPEMUL(pfPlane[0], vD.x) +
		VERTTYPEMUL(pfPlane[1], vD.y) +
		VERTTYPEMUL(pfPlane[2], vD.z);

	/* Numerator */
	fN =
		VERTTYPEMUL(pfPlane[0], pv0->x) +
		VERTTYPEMUL(pfPlane[1], pv0->y) +
		VERTTYPEMUL(pfPlane[2], pv0->z) +
		pfPlane[3];

	fT = VERTTYPEDIV(-fN, fD);

	/* And for a finale, calculate the intersection coordinate */
	pvIntersection->x = pv0->x + VERTTYPEMUL(fT, vD.x);
	pvIntersection->y = pv0->y + VERTTYPEMUL(fT, vD.y);
	pvIntersection->z = pv0->z + VERTTYPEMUL(fT, vD.z);
}


/*!***************************************************************************
 @Function		PVRTMiscCalculateInfinitePlane
 @Input			nStride			Size of each vertex structure containing pfVtx
 @Input			pvPlane			Length 4 [A,B,C,D], values for plane equation
 @Input			pmViewProjInv	The inverse of the View Projection matrix
 @Input			pFrom			Position of the camera
 @Input			fFar			Far clipping distance
 @Output		pfVtx			Position of the first of 3 floats to receive
								the position of vertex 0; up to 5 vertex positions
								will be written (5 is the maximum number of vertices
								required to draw an infinite polygon clipped to screen
								and far clip plane).
 @Returns		Number of vertices in the polygon fan (Can be 0, 3, 4 or 5)
 @Description	Calculates world-space coords of a screen-filling
				representation of an infinite plane The resulting vertices run
				counter-clockwise around the screen, and can be simply drawn using
				non-indexed TRIANGLEFAN
*****************************************************************************/
int PVRTMiscCalculateInfinitePlane(
	VERTTYPE			* const pfVtx,
	const int			nStride,
	const PVRTVECTOR4	* const pvPlane,
	const PVRTMATRIX 	* const pmViewProjInv,
	const PVRTVECTOR3	* const pFrom,
	const VERTTYPE		fFar)
{
	PVRTVECTOR3		pvWorld[5];
	PVRTVECTOR3		*pvPolyPtr;
	unsigned int	dwCount;
	bool			bClip;
	int				nVert;
	VERTTYPE		fDotProduct;

	/*
		Check whether the plane faces the camera
	*/
	fDotProduct =
		VERTTYPEMUL((pFrom->x + VERTTYPEMUL(pvPlane->x, pvPlane->w)), pvPlane->x) +
		VERTTYPEMUL((pFrom->y + VERTTYPEMUL(pvPlane->y, pvPlane->w)), pvPlane->y) +
		VERTTYPEMUL((pFrom->z + VERTTYPEMUL(pvPlane->z, pvPlane->w)), pvPlane->z);

	if(fDotProduct < 0) {
		/* Camera is behind plane, hence it's not visible */
		return 0;
	}

	/*
		Back transform front clipping plane into world space,
		to give us a point on the line through each corner of the screen
		(from the camera).
	*/

	/*             x = -1.0f;    y = -1.0f;     z = 1.0f;      w = 1.0f */
	pvWorld[0].x = VERTTYPEMUL((-pmViewProjInv->f[ 0] - pmViewProjInv->f[ 4] + pmViewProjInv->f[ 8] + pmViewProjInv->f[12]), fFar);
	pvWorld[0].y = VERTTYPEMUL((-pmViewProjInv->f[ 1] - pmViewProjInv->f[ 5] + pmViewProjInv->f[ 9] + pmViewProjInv->f[13]), fFar);
	pvWorld[0].z = VERTTYPEMUL((-pmViewProjInv->f[ 2] - pmViewProjInv->f[ 6] + pmViewProjInv->f[10] + pmViewProjInv->f[14]), fFar);
	/*             x =  1.0f,    y = -1.0f,     z = 1.0f;      w = 1.0f */
	pvWorld[1].x = VERTTYPEMUL(( pmViewProjInv->f[ 0] - pmViewProjInv->f[ 4] + pmViewProjInv->f[ 8] + pmViewProjInv->f[12]), fFar);
	pvWorld[1].y = VERTTYPEMUL(( pmViewProjInv->f[ 1] - pmViewProjInv->f[ 5] + pmViewProjInv->f[ 9] + pmViewProjInv->f[13]), fFar);
	pvWorld[1].z = VERTTYPEMUL(( pmViewProjInv->f[ 2] - pmViewProjInv->f[ 6] + pmViewProjInv->f[10] + pmViewProjInv->f[14]), fFar);
	/*             x =  1.0f,    y =  1.0f,     z = 1.0f;      w = 1.0f */
	pvWorld[2].x = VERTTYPEMUL(( pmViewProjInv->f[ 0] + pmViewProjInv->f[ 4] + pmViewProjInv->f[ 8] + pmViewProjInv->f[12]), fFar);
	pvWorld[2].y = VERTTYPEMUL(( pmViewProjInv->f[ 1] + pmViewProjInv->f[ 5] + pmViewProjInv->f[ 9] + pmViewProjInv->f[13]), fFar);
	pvWorld[2].z = VERTTYPEMUL(( pmViewProjInv->f[ 2] + pmViewProjInv->f[ 6] + pmViewProjInv->f[10] + pmViewProjInv->f[14]), fFar);
	/*             x = -1.0f,    y =  1.0f,     z = 1.0f;      w = 1.0f */
	pvWorld[3].x = VERTTYPEMUL((-pmViewProjInv->f[ 0] + pmViewProjInv->f[ 4] + pmViewProjInv->f[ 8] + pmViewProjInv->f[12]), fFar);
	pvWorld[3].y = VERTTYPEMUL((-pmViewProjInv->f[ 1] + pmViewProjInv->f[ 5] + pmViewProjInv->f[ 9] + pmViewProjInv->f[13]), fFar);
	pvWorld[3].z = VERTTYPEMUL((-pmViewProjInv->f[ 2] + pmViewProjInv->f[ 6] + pmViewProjInv->f[10] + pmViewProjInv->f[14]), fFar);

	/* We need to do a closed loop of the screen vertices, so copy the first vertex into the last */
	pvWorld[4] = pvWorld[0];

	/*
		Now build a pre-clipped polygon
	*/

	/* Lets get ready to loop */
	dwCount		= 0;
	bClip		= false;
	pvPolyPtr	= (PVRTVECTOR3*)pfVtx;

	nVert = 5;
	while(nVert)
	{
		nVert--;

		/*
			Check which side of the Plane this corner of the far clipping
			plane is on. [A,B,C] of plane equation is the plane normal, D is
			distance from origin; hence [pvPlane->x * -pvPlane->w,
										 pvPlane->y * -pvPlane->w,
										 pvPlane->z * -pvPlane->w]
			is a point on the plane
		*/
		fDotProduct =
			VERTTYPEMUL((pvWorld[nVert].x + VERTTYPEMUL(pvPlane->x, pvPlane->w)), pvPlane->x) +
			VERTTYPEMUL((pvWorld[nVert].y + VERTTYPEMUL(pvPlane->y, pvPlane->w)), pvPlane->y) +
			VERTTYPEMUL((pvWorld[nVert].z + VERTTYPEMUL(pvPlane->z, pvPlane->w)), pvPlane->z);

		if(fDotProduct < 0)
		{
			/*
				Behind plane; Vertex does NOT need clipping
			*/
			if(bClip == true)
			{
				/* Clipping finished */
				bClip = false;

				/*
					We've been clipping, so we need to add an additional
					point on the line to this point, where clipping was
					stopped.
				*/
				PVRTMiscCalculateIntersectionLinePlane(pvPolyPtr, &pvPlane->x, &pvWorld[nVert+1], &pvWorld[nVert]);
				pvPolyPtr = (PVRTVECTOR3*)((char*)pvPolyPtr + nStride);
				dwCount++;
			}

			if(!nVert)
			{
				/* Abort, abort: we've closed the loop with the clipped point */
				break;
			}

			/* Add the current point */
			PVRTMiscCalculateIntersectionLinePlane(pvPolyPtr, &pvPlane->x, pFrom, &pvWorld[nVert]);
			pvPolyPtr = (PVRTVECTOR3*)((char*)pvPolyPtr + nStride);
			dwCount++;
		}
		else
		{
			/*
				Before plane; Vertex DOES need clipping
			*/
			if(bClip == true)
			{
				/* Already in clipping, skip point */
				continue;
			}

			/* Clipping initiated */
			bClip = true;

			/* Don't bother with entry point on first vertex; will take care of it on last vertex (which is a repeat of first vertex) */
			if(nVert != 4)
			{
				/* We need to add an additional point on the line to this point, where clipping was started */
				PVRTMiscCalculateIntersectionLinePlane(pvPolyPtr, &pvPlane->x, &pvWorld[nVert+1], &pvWorld[nVert]);
				pvPolyPtr = (PVRTVECTOR3*)((char*)pvPolyPtr + nStride);
				dwCount++;
			}
		}
	}

	/* Valid vertex counts are 0, 3, 4, 5 */
	_ASSERT(dwCount <= 5);
	_ASSERT(dwCount != 1);
	_ASSERT(dwCount != 2);

	return dwCount;
}


/*!***************************************************************************
 @Function			SetVertex
 @Modified			Vertices
 @Input				index
 @Input				x
 @Input				y
 @Input				z
 @Description		Writes a vertex in a vertex array
*****************************************************************************/
static void SetVertex(VERTTYPE** Vertices, int index, VERTTYPE x, VERTTYPE y, VERTTYPE z)
{
	(*Vertices)[index*3+0] = x;
	(*Vertices)[index*3+1] = y;
	(*Vertices)[index*3+2] = z;
}

/*!***************************************************************************
 @Function			SetUV
 @Modified			UVs
 @Input				index
 @Input				u
 @Input				v
 @Description		Writes a texture coordinate in a texture coordinate array
*****************************************************************************/
static void SetUV(VERTTYPE** UVs, int index, VERTTYPE u, VERTTYPE v)
{
	(*UVs)[index*2+0] = u;
	(*UVs)[index*2+1] = v;
}

/*!***************************************************************************
 @Function		PVRTCreateSkybox
 @Input			scale			Scale the skybox
 @Input			adjustUV		Adjust or not UVs for PVRT compression
 @Input			textureSize		Texture size in pixels
 @Output		Vertices		Array of vertices
 @Output		UVs				Array of UVs
 @Description	Creates the vertices and texture coordinates for a skybox
*****************************************************************************/
void PVRTCreateSkybox(float scale, bool adjustUV, int textureSize, VERTTYPE** Vertices, VERTTYPE** UVs)
{
	*Vertices = new VERTTYPE[24*3];
	*UVs = new VERTTYPE[24*2];

	VERTTYPE unit = f2vt(1);
	VERTTYPE a0 = 0, a1 = unit;

	if (adjustUV)
	{
		VERTTYPE oneover = f2vt(1.0f / textureSize);
		a0 = VERTTYPEMUL(f2vt(4.0f), oneover);
		a1 = unit - a0;
	}

	// Front
	SetVertex(Vertices, 0, -unit, +unit, -unit);
	SetVertex(Vertices, 1, +unit, +unit, -unit);
	SetVertex(Vertices, 2, -unit, -unit, -unit);
	SetVertex(Vertices, 3, +unit, -unit, -unit);
	SetUV(UVs, 0, a0, a1);
	SetUV(UVs, 1, a1, a1);
	SetUV(UVs, 2, a0, a0);
	SetUV(UVs, 3, a1, a0);

	// Right
	SetVertex(Vertices, 4, +unit, +unit, -unit);
	SetVertex(Vertices, 5, +unit, +unit, +unit);
	SetVertex(Vertices, 6, +unit, -unit, -unit);
	SetVertex(Vertices, 7, +unit, -unit, +unit);
	SetUV(UVs, 4, a0, a1);
	SetUV(UVs, 5, a1, a1);
	SetUV(UVs, 6, a0, a0);
	SetUV(UVs, 7, a1, a0);

	// Back
	SetVertex(Vertices, 8 , +unit, +unit, +unit);
	SetVertex(Vertices, 9 , -unit, +unit, +unit);
	SetVertex(Vertices, 10, +unit, -unit, +unit);
	SetVertex(Vertices, 11, -unit, -unit, +unit);
	SetUV(UVs, 8 , a0, a1);
	SetUV(UVs, 9 , a1, a1);
	SetUV(UVs, 10, a0, a0);
	SetUV(UVs, 11, a1, a0);

	// Left
	SetVertex(Vertices, 12, -unit, +unit, +unit);
	SetVertex(Vertices, 13, -unit, +unit, -unit);
	SetVertex(Vertices, 14, -unit, -unit, +unit);
	SetVertex(Vertices, 15, -unit, -unit, -unit);
	SetUV(UVs, 12, a0, a1);
	SetUV(UVs, 13, a1, a1);
	SetUV(UVs, 14, a0, a0);
	SetUV(UVs, 15, a1, a0);

	// Top
	SetVertex(Vertices, 16, -unit, +unit, +unit);
	SetVertex(Vertices, 17, +unit, +unit, +unit);
	SetVertex(Vertices, 18, -unit, +unit, -unit);
	SetVertex(Vertices, 19, +unit, +unit, -unit);
	SetUV(UVs, 16, a0, a1);
	SetUV(UVs, 17, a1, a1);
	SetUV(UVs, 18, a0, a0);
	SetUV(UVs, 19, a1, a0);

	// Bottom
	SetVertex(Vertices, 20, -unit, -unit, -unit);
	SetVertex(Vertices, 21, +unit, -unit, -unit);
	SetVertex(Vertices, 22, -unit, -unit, +unit);
	SetVertex(Vertices, 23, +unit, -unit, +unit);
	SetUV(UVs, 20, a0, a1);
	SetUV(UVs, 21, a1, a1);
	SetUV(UVs, 22, a0, a0);
	SetUV(UVs, 23, a1, a0);

	for (int i=0; i<24*3; i++) (*Vertices)[i] = VERTTYPEMUL((*Vertices)[i], f2vt(scale));
}

/*!***************************************************************************
 @Function		PVRTDestroySkybox
 @Input			Vertices	Vertices array to destroy
 @Input			UVs			UVs array to destroy
 @Description	Destroy the memory allocated for a skybox
*****************************************************************************/
void PVRTDestroySkybox(VERTTYPE* Vertices, VERTTYPE* UVs)
{
	delete [] Vertices;
	delete [] UVs;
}

/*!***************************************************************************
 @Function		PVRTGetPOTHigher
 @Input			uiOriginalValue	Base value
 @Input			iTimesHigher		Multiplier
 @Description	When iTimesHigher is one, this function will return the closest
				power-of-two value above the base value.
				For every increment beyond one for the iTimesHigher value,
				the next highest power-of-two value will be calculated.
*****************************************************************************/
unsigned int PVRTGetPOTHigher(unsigned int uiOriginalValue, int iTimesHigher)
{
	if(uiOriginalValue == 0 || iTimesHigher < 0)
	{
		return 0;
	}

	unsigned int uiSize = 1;
	while (uiSize < uiOriginalValue) uiSize *= 2;

	// Keep increasing the POT value until the iTimesHigher value has been met
	for(int i = 1 ; i < iTimesHigher; ++i)
	{
		uiSize *= 2;
	}

	return uiSize;
}

/*!***************************************************************************
 @Function		PVRTGetPOTLower
 @Input			uiOriginalValue	Base value
 @Input			iTimesLower		Multiplier
 @Description	When iTimesLower is one, this function will return the closest
				power-of-two value below the base value.
				For every increment beyond one for the iTimesLower value,
				the next lowest power-of-two value will be calculated. The lowest
				value that can be reached is 1.
*****************************************************************************/
// NOTE: This function should be optimised
unsigned int PVRTGetPOTLower(unsigned int uiOriginalValue, int iTimesLower)
{
	if(uiOriginalValue == 0 || iTimesLower < 0)
	{
		return 0;
	}
	unsigned int uiSize = PVRTGetPOTHigher(uiOriginalValue,1);
	uiSize >>= 1;//uiSize /=2;

	for(int i = 1; i < iTimesLower; ++i)
	{
		uiSize >>= 1;//uiSize /=2;
		if(uiSize == 1)
		{
			// Lowest possible value has been reached, so break
			break;
		}
	}
	return uiSize;
}



/*****************************************************************************
 End of file (PVRTMisc.cpp)
*****************************************************************************/

