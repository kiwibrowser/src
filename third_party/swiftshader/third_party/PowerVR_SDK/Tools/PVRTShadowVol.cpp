/******************************************************************************

 @File         PVRTShadowVol.cpp

 @Title        PVRTShadowVol

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Declarations of functions relating to shadow volume generation.

******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "PVRTGlobal.h"
#include "PVRTContext.h"
#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"
#include "PVRTTrans.h"
#include "PVRTShadowVol.h"
#include "PVRTError.h"

/****************************************************************************
** Build options
****************************************************************************/

/****************************************************************************
** Defines
****************************************************************************/

/****************************************************************************
** Macros
****************************************************************************/

/****************************************************************************
** Structures
****************************************************************************/
struct SVertexShVol {
	float	x, y, z;
	unsigned int dwExtrude;
#if defined(BUILD_OGLES)
	float fWeight;
#endif
};

/****************************************************************************
** Constants
****************************************************************************/
const static unsigned short c_pwLinesHyperCube[64] = {
	// Cube0
	0, 1,  2, 3,  0, 2,  1, 3,
	4, 5,  6, 7,  4, 6,  5, 7,
	0, 4,  1, 5,  2, 6,  3, 7,
	// Cube1
	8, 9,  10, 11,  8, 10,  9, 11,
	12, 13,  14, 15,  12, 14,  13, 15,
	8, 12,  9, 13,  10, 14,  11, 15,
	// Hyper cube jn
	0, 8,  1, 9,  2, 10,  3, 11,
	4, 12,  5, 13,  6, 14,  7, 15
};
const static PVRTVECTOR3 c_pvRect[4] = {
	{ -1, -1, 1 },
	{ -1,  1, 1 },
	{  1, -1, 1 },
	{  1,  1, 1 }
};

/****************************************************************************
** Shared globals
****************************************************************************/

/****************************************************************************
** Globals
****************************************************************************/

/****************************************************************************
** Declarations
****************************************************************************/

/****************************************************************************
** Code
****************************************************************************/
/****************************************************************************
@Function		FindOrCreateVertex
@Modified		psMesh				The mesh to check against/add to
@Input			pV					The vertex to compare/add
@Return			unsigned short		The array index of the vertex
@Description	Searches through the mesh data to see if the vertex has
				already been used. If it has, the array index of the vertex
				is returned. If the mesh does not already use the vertex,
				it is appended to the vertex array and the array count is incremented.
				The index in the array of the new vertex is then returned.
****************************************************************************/
static unsigned short FindOrCreateVertex(PVRTShadowVolShadowMesh * const psMesh, const PVRTVECTOR3 * const pV) {
	unsigned short	wCurr;

	/*
		First check whether we already have a vertex here
	*/
	for(wCurr = 0; wCurr < psMesh->nV; wCurr++) {
		if(memcmp(&psMesh->pV[wCurr], pV, sizeof(*pV)) == 0) {
			/* Don't do anything more if the vertex already exists */
			return wCurr;
		}
	}

	/*
		Add the vertex then!
	*/
	psMesh->pV[psMesh->nV] = *pV;

	return (unsigned short) psMesh->nV++;
}

/****************************************************************************
@Function		FindOrCreateEdge
@Modified		psMesh				The mesh to check against/add to
@Input			pv0					The first point that defines the edge
@Input			pv1					The second point that defines the edge
@Return			PVRTShadowVolMEdge	The index of the found/created edge in the
									mesh's array
@Description	Searches through the mesh data to see if the edge has
				already been used. If it has, the array index of the edge
				is returned. If the mesh does not already use the edge,
				it is appended to the edge array and the array cound is incremented.
				The index in the array of the new edge is then returned.
****************************************************************************/
static unsigned int FindOrCreateEdge(PVRTShadowVolShadowMesh * const psMesh, const PVRTVECTOR3 * const pv0, const PVRTVECTOR3 * const pv1) {
	unsigned int	nCurr;
	unsigned short			wV0, wV1;
	
	wV0 = FindOrCreateVertex(psMesh, pv0);
	wV1 = FindOrCreateVertex(psMesh, pv1);

	
	/*
		First check whether we already have a edge here
	*/
	for(nCurr = 0; nCurr < psMesh->nE; nCurr++) {
		if(
			(psMesh->pE[nCurr].wV0 == wV0 && psMesh->pE[nCurr].wV1 == wV1) ||
			(psMesh->pE[nCurr].wV0 == wV1 && psMesh->pE[nCurr].wV1 == wV0))
		{
			/* Don't do anything more if the edge already exists */						
			return nCurr;
		}
	}

	/*
		Add the edge then!
	*/
	psMesh->pE[psMesh->nE].wV0	= wV0;
	psMesh->pE[psMesh->nE].wV1	= wV1;
	psMesh->pE[psMesh->nE].nVis	= 0;

	return psMesh->nE++;
}

/****************************************************************************
@Function		CrossProduct
@Output			pvOut			The resultant vector
@Input			pv0				Vector zero
@Input			pv1				Vector one
@Input			pv2				Vector two
@Description	Finds the vector between vector zero and vector one,
				and the vector between vector zero and vector two.
				These two resultant vectors are then multiplied together
				and the result is assigned to the output vector.
****************************************************************************/
static void CrossProduct(
	PVRTVECTOR3 * const pvOut,
	const PVRTVECTOR3 * const pv0,
	const PVRTVECTOR3 * const pv1,
	const PVRTVECTOR3 * const pv2)
{
	PVRTVECTOR3 v0, v1;

	v0.x = pv1->x - pv0->x;
	v0.y = pv1->y - pv0->y;
	v0.z = pv1->z - pv0->z;

	v1.x = pv2->x - pv0->x;
	v1.y = pv2->y - pv0->y;
	v1.z = pv2->z - pv0->z;

	PVRTMatrixVec3CrossProduct(*pvOut, v0, v1);
}

/****************************************************************************
@Function		FindOrCreateTriangle
@Modified		psMesh			The mesh to check against/add to
@Input			pv0				Vertex zero
@Input			pv1				Vertex one
@Input			pv2				Vertex two
@Description	Searches through the mesh data to see if the triangle has
				already been used. If it has, the function returns.
				If the mesh does not already use the triangle,
				it is appended to the triangle array and the array cound is incremented.
****************************************************************************/
static void FindOrCreateTriangle(
	PVRTShadowVolShadowMesh	* const psMesh,
	const PVRTVECTOR3	* const pv0,
	const PVRTVECTOR3	* const pv1,
	const PVRTVECTOR3	* const pv2)
{
	unsigned int	nCurr;
	PVRTShadowVolMEdge	*psE0, *psE1, *psE2;
	unsigned int wE0, wE1, wE2;

	wE0 = FindOrCreateEdge(psMesh, pv0, pv1);
	wE1 = FindOrCreateEdge(psMesh, pv1, pv2);
	wE2 = FindOrCreateEdge(psMesh, pv2, pv0);
	
	if(wE0 == wE1 || wE1 == wE2 || wE2 == wE0) {
		/* Don't add degenerate triangles */
		_RPT0(_CRT_WARN, "FindOrCreateTriangle() Degenerate triangle.\n");
		return;
	}

	/*
		First check whether we already have a triangle here
	*/
	for(nCurr = 0; nCurr < psMesh->nT; nCurr++) {
		if(
			(psMesh->pT[nCurr].wE0 == wE0 || psMesh->pT[nCurr].wE0 == wE1 || psMesh->pT[nCurr].wE0 == wE2) &&
			(psMesh->pT[nCurr].wE1 == wE0 || psMesh->pT[nCurr].wE1 == wE1 || psMesh->pT[nCurr].wE1 == wE2) &&
			(psMesh->pT[nCurr].wE2 == wE0 || psMesh->pT[nCurr].wE2 == wE1 || psMesh->pT[nCurr].wE2 == wE2))
		{
			/* Don't do anything more if the triangle already exists */
			return;
		}
	}

	/*
		Add the triangle then!
	*/
	psMesh->pT[psMesh->nT].wE0 = wE0;
	psMesh->pT[psMesh->nT].wE1 = wE1;
	psMesh->pT[psMesh->nT].wE2 = wE2;

	psE0 = &psMesh->pE[wE0];
	psE1 = &psMesh->pE[wE1];
	psE2 = &psMesh->pE[wE2];

	/*
		Store the triangle indices; these are indices into the shadow mesh, not the source model indices
	*/
	if(psE0->wV0 == psE1->wV0 || psE0->wV0 == psE1->wV1)
		psMesh->pT[psMesh->nT].w[0] = psE0->wV1;
	else
		psMesh->pT[psMesh->nT].w[0] = psE0->wV0;

	if(psE1->wV0 == psE2->wV0 || psE1->wV0 == psE2->wV1)
		psMesh->pT[psMesh->nT].w[1] = psE1->wV1;
	else
		psMesh->pT[psMesh->nT].w[1] = psE1->wV0;

	if(psE2->wV0 == psE0->wV0 || psE2->wV0 == psE0->wV1)
		psMesh->pT[psMesh->nT].w[2] = psE2->wV1;
	else
		psMesh->pT[psMesh->nT].w[2] = psE2->wV0;

	/* Calculate the triangle normal */
	CrossProduct(&psMesh->pT[psMesh->nT].vNormal, pv0, pv1, pv2);

	/* Check which edges have the correct winding order for this triangle */
	psMesh->pT[psMesh->nT].nWinding = 0;
	if(memcmp(&psMesh->pV[psE0->wV0], pv0, sizeof(*pv0)) == 0) psMesh->pT[psMesh->nT].nWinding |= 0x01;
	if(memcmp(&psMesh->pV[psE1->wV0], pv1, sizeof(*pv1)) == 0) psMesh->pT[psMesh->nT].nWinding |= 0x02;
	if(memcmp(&psMesh->pV[psE2->wV0], pv2, sizeof(*pv2)) == 0) psMesh->pT[psMesh->nT].nWinding |= 0x04;

	psMesh->nT++;
}

/*!***********************************************************************
@Function	PVRTShadowVolMeshCreateMesh
@Modified	psMesh		The shadow volume mesh to populate
@Input		pVertex		A list of vertices
@Input		nNumVertex	The number of vertices
@Input		pFaces		A list of faces
@Input		nNumFaces	The number of faces
@Description	Creates a mesh format suitable for generating shadow volumes
*************************************************************************/
void PVRTShadowVolMeshCreateMesh(
	PVRTShadowVolShadowMesh		* const psMesh,
	const float				* const pVertex,
	const unsigned int		nNumVertex,
	const unsigned short	* const pFaces,
	const unsigned int		nNumFaces)
{
	unsigned int	nCurr;

	/*
		Prep the structure to return
	*/
	memset(psMesh, 0, sizeof(*psMesh));

	/*
		Allocate some working space to find the unique vertices
	*/
	psMesh->pV = (PVRTVECTOR3*)malloc(nNumVertex * sizeof(*psMesh->pV));
	psMesh->pE = (PVRTShadowVolMEdge*)malloc(nNumFaces * sizeof(*psMesh->pE) * 3);
	psMesh->pT = (PVRTShadowVolMTriangle*)malloc(nNumFaces * sizeof(*psMesh->pT));
	_ASSERT(psMesh->pV);
	_ASSERT(psMesh->pE);
	_ASSERT(psMesh->pT);

	for(nCurr = 0; nCurr < nNumFaces; nCurr++) {
		FindOrCreateTriangle(psMesh,
			(PVRTVECTOR3*)&pVertex[3 * pFaces[3 * nCurr + 0]],
			(PVRTVECTOR3*)&pVertex[3 * pFaces[3 * nCurr + 1]],
			(PVRTVECTOR3*)&pVertex[3 * pFaces[3 * nCurr + 2]]);
	}

	_ASSERT(psMesh->nV <= nNumVertex);
	_ASSERT(psMesh->nE < nNumFaces * 3);
	_ASSERT(psMesh->nT == nNumFaces);

	_RPT2(_CRT_WARN, "Unique vertices : %d (from %d)\n", psMesh->nV, nNumVertex);
	_RPT2(_CRT_WARN, "Unique edges    : %d (from %d)\n", psMesh->nE, nNumFaces * 3);
	_RPT2(_CRT_WARN, "Unique triangles: %d (from %d)\n", psMesh->nT, nNumFaces);

	/*
		Create the real unique lists
	*/
	psMesh->pV = (PVRTVECTOR3*)realloc(psMesh->pV, psMesh->nV * sizeof(*psMesh->pV));
	psMesh->pE = (PVRTShadowVolMEdge*)realloc(psMesh->pE, psMesh->nE * sizeof(*psMesh->pE));
	psMesh->pT = (PVRTShadowVolMTriangle*)realloc(psMesh->pT, psMesh->nT * sizeof(*psMesh->pT));
	_ASSERT(psMesh->pV);
	_ASSERT(psMesh->pE);
	_ASSERT(psMesh->pT);

#if defined(_DEBUG) && !defined(_UNICODE) && defined(_WIN32)
	/*
		Check we have sensible model data
	*/
	{
		unsigned int nTri, nEdge;
		PVRTERROR_OUTPUT_DEBUG("ShadowMeshCreate() Sanity check...");

		for(nEdge = 0; nEdge < psMesh->nE; nEdge++) {
			nCurr = 0;

			for(nTri = 0; nTri < psMesh->nT; nTri++) {
				if(psMesh->pT[nTri].wE0 == nEdge)
					nCurr++;

				if(psMesh->pT[nTri].wE1 == nEdge)
					nCurr++;

				if(psMesh->pT[nTri].wE2 == nEdge)
					nCurr++;
			}

			/*
				Every edge should be referenced exactly twice. 
				If they aren't then the mesh isn't closed which will cause problems when rendering the shadows.
			*/
			_ASSERTE(nCurr == 2);
		}

		PVRTERROR_OUTPUT_DEBUG("done.\n");
	}
#endif
}

/*!***********************************************************************
@Function		PVRTShadowVolMeshInitMesh
@Input			psMesh	The shadow volume mesh
@Input			pContext	A struct for API specific data
@Returns 		True on success
@Description	Init the mesh
*************************************************************************/
bool PVRTShadowVolMeshInitMesh(
	PVRTShadowVolShadowMesh		* const psMesh,
	const SPVRTContext		* const pContext)
{
	unsigned int	nCurr;
#if defined(BUILD_DX11)
	HRESULT			hRes;
#endif
	SVertexShVol	*pvData;

#if defined(BUILD_OGL)
	_ASSERT(pContext && pContext->pglExt);

	if(!pContext || !pContext->pglExt)
		return false;
#endif

#if defined(BUILD_OGLES2) || defined(BUILD_OGLES) || defined(BUILD_OGLES3)
	PVRT_UNREFERENCED_PARAMETER(pContext);
#endif
	_ASSERT(psMesh);
	_ASSERT(psMesh->nV);
	_ASSERT(psMesh->nE);
	_ASSERT(psMesh->nT);

	/*
		Allocate a vertex buffer for the shadow volumes
	*/
	_ASSERT(psMesh->pivb == NULL);
	_RPT3(_CRT_WARN, "ShadowMeshInitMesh() %5d byte VB (%3dv x 2 x size(%d))\n", psMesh->nV * 2 * sizeof(*pvData), psMesh->nV, sizeof(*pvData));

#if defined(BUILD_DX11)
	D3D11_BUFFER_DESC sVBBufferDesc;
	sVBBufferDesc.ByteWidth		= psMesh->nV * 2 * 3 * sizeof(*pvData);
	sVBBufferDesc.Usage			= D3D11_USAGE_DYNAMIC;
	sVBBufferDesc.BindFlags		= D3D11_BIND_VERTEX_BUFFER;
	sVBBufferDesc.CPUAccessFlags= 0;
	sVBBufferDesc.MiscFlags		= 0;

	hRes = pContext->pDev->CreateBuffer(&sVBBufferDesc, NULL, &psMesh->pivb) != S_OK;

	if(FAILED(hRes)) 
	{
		_ASSERT(false);
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE data;
	ID3D11DeviceContext *pDeviceContext = 0;
	pContext->pDev->GetImmediateContext(&pDeviceContext);
	hRes = pDeviceContext->Map(psMesh->pivb, 0, D3D11_MAP_WRITE_DISCARD, NULL, &data);

	if(FAILED(hRes)) 
	{
		_ASSERT(false);
		return false;
	}

	pvData = (SVertexShVol*) data.pData;
#endif

#if defined(BUILD_OGL)
	_ASSERT(pContext && pContext->pglExt);
	if (!pContext || !pContext->pglExt)
		return false;
	pContext->pglExt->glGenBuffersARB(1, &psMesh->pivb);
	pContext->pglExt->glBindBufferARB(GL_ARRAY_BUFFER_ARB, psMesh->pivb);
	pContext->pglExt->glBufferDataARB(GL_ARRAY_BUFFER_ARB, psMesh->nV * 2 * sizeof(*pvData), NULL, GL_STREAM_DRAW_ARB);
	pvData = (SVertexShVol*)pContext->pglExt->glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
#endif

#if defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	psMesh->pivb = malloc(psMesh->nV * 2 * sizeof(*pvData));
	pvData = (SVertexShVol*)psMesh->pivb;
#endif

	/*
		Fill the vertex buffer with two subtly different copies of the vertices
	*/
	for(nCurr = 0; nCurr < psMesh->nV; ++nCurr) 
	{
		pvData[nCurr].x			= psMesh->pV[nCurr].x;
		pvData[nCurr].y			= psMesh->pV[nCurr].y;
		pvData[nCurr].z			= psMesh->pV[nCurr].z;
		pvData[nCurr].dwExtrude = 0;

#if defined(BUILD_OGLES)
		pvData[nCurr].fWeight = 1;
		pvData[nCurr + psMesh->nV].fWeight = 1;
#endif
		pvData[nCurr + psMesh->nV]				= pvData[nCurr];
		pvData[nCurr + psMesh->nV].dwExtrude	= 0x04030201;		// Order is wzyx
	}

#if defined(BUILD_OGL)
	pContext->pglExt->glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	pContext->pglExt->glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif

#if defined(BUILD_DX11)
	pDeviceContext->Unmap(psMesh->pivb, 0);
#endif
	return true;
}

/*!***********************************************************************
@Function		PVRTShadowVolMeshInitVol
@Modified		psVol	The shadow volume struct
@Input			psMesh	The shadow volume mesh
@Input			pContext	A struct for API specific data
@Returns		True on success
@Description	Init the renderable shadow volume information.
*************************************************************************/
bool PVRTShadowVolMeshInitVol(
	PVRTShadowVolShadowVol			* const psVol,
	const PVRTShadowVolShadowMesh	* const psMesh,
	const SPVRTContext		* const pContext)
{
#if defined(BUILD_DX11)
	HRESULT hRes;
#endif
#if defined(BUILD_OGLES2) || defined(BUILD_OGLES) || defined(BUILD_OGL) || defined(BUILD_OGLES3)
	PVRT_UNREFERENCED_PARAMETER(pContext);
#endif
	_ASSERT(psVol);
	_ASSERT(psMesh);
	_ASSERT(psMesh->nV);
	_ASSERT(psMesh->nE);
	_ASSERT(psMesh->nT);

	_RPT1(_CRT_WARN, "ShadowMeshInitVol() %5lu byte IB\n", psMesh->nT * 2 * 3 * sizeof(unsigned short));

	/*
		Allocate a index buffer for the shadow volumes
	*/
#if defined(_DEBUG)
	psVol->nIdxCntMax = psMesh->nT * 2 * 3;
#endif
#if defined(BUILD_DX11)
	D3D11_BUFFER_DESC sIdxBuferDesc;
	sIdxBuferDesc.ByteWidth		= psMesh->nT * 2 * 3 * sizeof(unsigned short);
	sIdxBuferDesc.Usage			= D3D11_USAGE_DYNAMIC;
	sIdxBuferDesc.BindFlags		= D3D11_BIND_INDEX_BUFFER;
	sIdxBuferDesc.CPUAccessFlags= 0;
	sIdxBuferDesc.MiscFlags		= 0;

	hRes = pContext->pDev->CreateBuffer(&sIdxBuferDesc, NULL, &psVol->piib) != S_OK;

	if(FAILED(hRes)) {
		_ASSERT(false);
		return false;
	}
#endif
#if defined(BUILD_OGL)
	_ASSERT(pContext && pContext->pglExt);
	if (!pContext || !pContext->pglExt)
		return false;
	pContext->pglExt->glGenBuffersARB(1, &psVol->piib);
	pContext->pglExt->glBindBufferARB(GL_ARRAY_BUFFER_ARB, psVol->piib);
	pContext->pglExt->glBufferDataARB(GL_ARRAY_BUFFER_ARB, psMesh->nT * 2 * 3 * sizeof(unsigned short), NULL, GL_STREAM_DRAW_ARB);
#endif

#if defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	psVol->piib = (unsigned short*)malloc(psMesh->nT * 2 * 3 * sizeof(unsigned short));
#endif

	return true;
}

/*!***********************************************************************
@Function		PVRTShadowVolMeshDestroyMesh
@Input			psMesh	The shadow volume mesh to destroy
@Description	Destroys all shadow volume mesh data created by PVRTShadowVolMeshCreateMesh
*************************************************************************/
void PVRTShadowVolMeshDestroyMesh(
	PVRTShadowVolShadowMesh		* const psMesh)
{
	FREE(psMesh->pV);
	FREE(psMesh->pE);
	FREE(psMesh->pT);
}

/*!***********************************************************************
@Function		PVRTShadowVolMeshReleaseMesh
@Input			psMesh	The shadow volume mesh to release
@Description	Releases all shadow volume mesh data created by PVRTShadowVolMeshInitMesh
*************************************************************************/
void PVRTShadowVolMeshReleaseMesh(
	PVRTShadowVolShadowMesh		* const psMesh,
	SPVRTContext				* const psContext)
{
#if defined(BUILD_OGL)
	_ASSERT(psContext && psContext->pglExt);
	if (!psContext || !psContext->pglExt)
		return;
	psContext->pglExt->glDeleteBuffersARB(1, &psMesh->pivb);
#endif
#if defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	PVRT_UNREFERENCED_PARAMETER(psContext);
	FREE(psMesh->pivb);
#endif
}

/*!***********************************************************************
@Function		PVRTShadowVolMeshReleaseVol
@Input			psVol	The shadow volume information to release
@Description	Releases all data create by PVRTShadowVolMeshInitVol
*************************************************************************/
void PVRTShadowVolMeshReleaseVol(
	PVRTShadowVolShadowVol			* const psVol,
	SPVRTContext					* const psContext)
{
#if defined(BUILD_OGL)
	_ASSERT(psContext && psContext->pglExt);
	if (!psContext || !psContext->pglExt)
		return;
	psContext->pglExt->glDeleteBuffersARB(1, &psVol->piib);
#endif

#if defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	PVRT_UNREFERENCED_PARAMETER(psContext);
	FREE(psVol->piib);
#endif
}

/*!***********************************************************************
@Function		PVRTShadowVolSilhouetteProjectedBuild
@Modified		psVol	The shadow volume information
@Input			dwVisFlags	Shadow volume creation flags
@Input			psMesh	The shadow volume mesh
@Input			pvLightModel	The light position/direction
@Input			bPointLight		Is the light a point light
@Input			pContext	A struct for passing in API specific data
@Description	Using the light set up the shadow volume so it can be extruded.
*************************************************************************/
void PVRTShadowVolSilhouetteProjectedBuild(
	PVRTShadowVolShadowVol			* const psVol,
	const unsigned int		dwVisFlags,
	const PVRTShadowVolShadowMesh	* const psMesh,
	const PVRTVec3		* const pvLightModel,
	const bool				bPointLight,
	const SPVRTContext * const pContext)
{
	PVRTShadowVolSilhouetteProjectedBuild(psVol, dwVisFlags,psMesh, (PVRTVECTOR3*) pvLightModel, bPointLight, pContext);
}

/*!***********************************************************************
@Function		PVRTShadowVolSilhouetteProjectedBuild
@Modified		psVol	The shadow volume information
@Input			dwVisFlags	Shadow volume creation flags
@Input			psMesh	The shadow volume mesh
@Input			pvLightModel	The light position/direction
@Input			bPointLight		Is the light a point light
@Input			pContext	A struct for passing in API specific data
@Description	Using the light set up the shadow volume so it can be extruded.
*************************************************************************/
void PVRTShadowVolSilhouetteProjectedBuild(
	PVRTShadowVolShadowVol			* const psVol,
	const unsigned int		dwVisFlags,
	const PVRTShadowVolShadowMesh	* const psMesh,
	const PVRTVECTOR3		* const pvLightModel,
	const bool				bPointLight,
	const SPVRTContext * const pContext)
{
	PVRTVECTOR3		v;
	PVRTShadowVolMTriangle	*psTri;
	PVRTShadowVolMEdge		*psEdge;
	unsigned short	*pwIdx;
#if defined(BUILD_DX11)
	HRESULT			hRes;
#endif
	unsigned int	nCurr;
	float			f;

	/*
		Lock the index buffer; this is where we create the shadow volume
	*/
	_ASSERT(psVol && psVol->piib);
#if defined(BUILD_OGL) || defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	PVRT_UNREFERENCED_PARAMETER(pContext);
#endif
#if defined(BUILD_DX11)
	_ASSERT(pContext);

	if(!pContext)
		return;

	D3D11_MAPPED_SUBRESOURCE data;
	ID3D11DeviceContext *pDeviceContext = 0;
	pContext->pDev->GetImmediateContext(&pDeviceContext);
	hRes = pDeviceContext->Map(psVol->piib, 0, D3D11_MAP_WRITE_DISCARD, NULL, &data);
	pwIdx = (unsigned short*) data.pData;

	_ASSERT(SUCCEEDED(hRes));
#endif
#if defined(BUILD_OGL)
	_ASSERT(pContext && pContext->pglExt);
	if (!pContext || !pContext->pglExt)
		return;

	pContext->pglExt->glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, psVol->piib);
	pwIdx = (unsigned short*)pContext->pglExt->glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
#endif
#if defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	pwIdx = psVol->piib;
#endif

	psVol->nIdxCnt = 0;

	// Run through triangles, testing which face the From point
	for(nCurr = 0; nCurr < psMesh->nT; ++nCurr) 
	{
		PVRTShadowVolMEdge *pE0, *pE1, *pE2;
		psTri = &psMesh->pT[nCurr];
		pE0 = &psMesh->pE[psTri->wE0];
		pE1 = &psMesh->pE[psTri->wE1];
		pE2 = &psMesh->pE[psTri->wE2];

		if(bPointLight) {
			v.x = psMesh->pV[pE0->wV0].x - pvLightModel->x;
			v.y = psMesh->pV[pE0->wV0].y - pvLightModel->y;
			v.z = psMesh->pV[pE0->wV0].z - pvLightModel->z;
			f = PVRTMatrixVec3DotProduct(psTri->vNormal, v);
		} else {
			f = PVRTMatrixVec3DotProduct(psTri->vNormal, *pvLightModel);
		}

		if(f >= 0) {
			/* Triangle is in the light */
			pE0->nVis |= 0x01;
			pE1->nVis |= 0x01;
			pE2->nVis |= 0x01;

			if(dwVisFlags & PVRTSHADOWVOLUME_NEED_CAP_FRONT) 
			{
				// Add the triangle to the volume, unextruded.
				pwIdx[psVol->nIdxCnt+0] = psTri->w[0];
				pwIdx[psVol->nIdxCnt+1] = psTri->w[1];
				pwIdx[psVol->nIdxCnt+2] = psTri->w[2];
				psVol->nIdxCnt += 3;
			}
		} else {
			/* Triangle is in shade; set Bit3 if the winding order needs reversed */
			pE0->nVis |= 0x02 | (psTri->nWinding & 0x01) << 2;
			pE1->nVis |= 0x02 | (psTri->nWinding & 0x02) << 1;
			pE2->nVis |= 0x02 | (psTri->nWinding & 0x04);

			if(dwVisFlags & PVRTSHADOWVOLUME_NEED_CAP_BACK) {
				// Add the triangle to the volume, extruded.
				// psMesh->nV is used as an offst so that the new index refers to the 
				// corresponding position in the second array of vertices (which are extruded)
				pwIdx[psVol->nIdxCnt+0] = (unsigned short) psMesh->nV + psTri->w[0];
				pwIdx[psVol->nIdxCnt+1] = (unsigned short) psMesh->nV + psTri->w[1];
				pwIdx[psVol->nIdxCnt+2] = (unsigned short) psMesh->nV + psTri->w[2];
				psVol->nIdxCnt += 3;
			}
		}
	}

#if defined(_DEBUG)
	_ASSERT(psVol->nIdxCnt <= psVol->nIdxCntMax);
	for(nCurr = 0; nCurr < psVol->nIdxCnt; ++nCurr) {
		_ASSERT(pwIdx[nCurr] < psMesh->nV*2);
	}
#endif

	/*
		Run through edges, testing which are silhouette edges
	*/
	for(nCurr = 0; nCurr < psMesh->nE; nCurr++) {
		psEdge = &psMesh->pE[nCurr];

		if((psEdge->nVis & 0x03) == 0x03) {
			/* 	
				Silhouette edge found! 
				The edge is both visible and hidden, 
				so it is along the silhouette of the model 
				(See header notes for more info)
			*/
			if(psEdge->nVis & 0x04) {
				pwIdx[psVol->nIdxCnt+0] = psEdge->wV0;
				pwIdx[psVol->nIdxCnt+1] = psEdge->wV1;
				pwIdx[psVol->nIdxCnt+2] = psEdge->wV0 + (unsigned short) psMesh->nV;

				pwIdx[psVol->nIdxCnt+3] = psEdge->wV0 + (unsigned short) psMesh->nV;
				pwIdx[psVol->nIdxCnt+4] = psEdge->wV1;
				pwIdx[psVol->nIdxCnt+5] = psEdge->wV1 + (unsigned short) psMesh->nV;
			} else {
				pwIdx[psVol->nIdxCnt+0] = psEdge->wV1;
				pwIdx[psVol->nIdxCnt+1] = psEdge->wV0;
				pwIdx[psVol->nIdxCnt+2] = psEdge->wV1 + (unsigned short) psMesh->nV;

				pwIdx[psVol->nIdxCnt+3] = psEdge->wV1 + (unsigned short) psMesh->nV;
				pwIdx[psVol->nIdxCnt+4] = psEdge->wV0;
				pwIdx[psVol->nIdxCnt+5] = psEdge->wV0 + (unsigned short) psMesh->nV;
			}

			psVol->nIdxCnt += 6;
		}

		/* Zero for next render */
		psEdge->nVis = 0;
	}

#if defined(_DEBUG)
	_ASSERT(psVol->nIdxCnt <= psVol->nIdxCntMax);
	for(nCurr = 0; nCurr < psVol->nIdxCnt; ++nCurr) {
		_ASSERT(pwIdx[nCurr] < psMesh->nV*2);
	}
#endif
#if defined(BUILD_OGL)
	pContext->pglExt->glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	pContext->pglExt->glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
#endif

#if defined(BUILD_DX11)
	pDeviceContext->Unmap(psVol->piib, 0);
#endif
}

/*!***********************************************************************
@Function		IsBoundingBoxVisibleEx
@Input			pBoundingHyperCube	The hypercube to test against
@Input			fCamZ				The camera's position along the z-axis
@Return			bool				Returns true if the bounding box is visible
@Description	This method tests the bounding box's position against
				the camera's position to determine if it is visible. 
				If it is visible, the function returns true.
*************************************************************************/
static bool IsBoundingBoxVisibleEx(
	const PVRTVECTOR4	* const pBoundingHyperCube,
	const float			fCamZ)
{
	PVRTVECTOR3	v, vShift[16];
	unsigned int		dwClipFlags;
	int			i, j;
	unsigned short		w0, w1;

	dwClipFlags = 0;		// Assume all are off-screen

	i = 8;
	while(i)
	{
		i--;

		if(pBoundingHyperCube[i].x <  pBoundingHyperCube[i].w)
			dwClipFlags |= 1 << 0;

		if(pBoundingHyperCube[i].x > -pBoundingHyperCube[i].w)
			dwClipFlags |= 1 << 1;

		if(pBoundingHyperCube[i].y <  pBoundingHyperCube[i].w)
			dwClipFlags |= 1 << 2;

		if(pBoundingHyperCube[i].y > -pBoundingHyperCube[i].w)
			dwClipFlags |= 1 << 3;

		if(pBoundingHyperCube[i].z > 0)
			dwClipFlags |= 1 << 4;
	}

	/*
		Volume is hidden if all the vertices are over a screen edge
	*/
	if(dwClipFlags != 0x1F)
		return false;

	/*
		Well, according to the simple bounding box check, it might be
		visible. Let's now test the view frustrum against the bounding
		cube. (Basically the reverse of the previous test!)

		This catches those cases where a diagonal cube passes near a
		screen edge.
	*/

	// Subtract the camera position from the vertices. I.e. move the camera to 0,0,0
	for(i = 0; i < 8; ++i) {
		vShift[i].x = pBoundingHyperCube[i].x;
		vShift[i].y = pBoundingHyperCube[i].y;
		vShift[i].z = pBoundingHyperCube[i].z - fCamZ;
	}

	i = 12;
	while(i) {
		--i;

		w0 = c_pwLinesHyperCube[2 * i + 0];
		w1 = c_pwLinesHyperCube[2 * i + 1];

		PVRTMatrixVec3CrossProduct(v, vShift[w0], vShift[w1]);
		dwClipFlags = 0;

		j = 4;
		while(j) {
			--j;

			if(PVRTMatrixVec3DotProduct(c_pvRect[j], v) < 0)
				++dwClipFlags;
		}

		// dwClipFlagsA will be 0 or 4 if the screen edges are on the outside of
		// this bounding-box-silhouette-edge.
		if(dwClipFlags % 4)
			continue;

		j = 8;
		while(j) {
			--j;

			if((j != w0) & (j != w1) && (PVRTMatrixVec3DotProduct(vShift[j], v) > 0))
				++dwClipFlags;
		}

		// dwClipFlagsA will be 0 or 18 if this is a silhouette edge of the bounding box
		if(dwClipFlags % 12)
			continue;

		return false;
	}

	return true;
}

/*!***********************************************************************
@Function		IsHyperBoundingBoxVisibleEx
@Input			pBoundingHyperCube	The hypercube to test against
@Input			fCamZ				The camera's position along the z-axis
@Return			bool				Returns true if the bounding box is visible
@Description	This method tests the hypercube bounding box's position against
				the camera's position to determine if it is visible. 
				If it is visible, the function returns true.
*************************************************************************/
static bool IsHyperBoundingBoxVisibleEx(
	const PVRTVECTOR4	* const pBoundingHyperCube,
	const float			fCamZ)
{
	const PVRTVECTOR4	*pv0;
	PVRTVECTOR3	v, vShift[16];
	unsigned int		dwClipFlagsA, dwClipFlagsB;
	int			i, j;
	unsigned short		w0, w1;

	pv0 = &pBoundingHyperCube[8];
	dwClipFlagsA = 0;		// Assume all are off-screen
	dwClipFlagsB = 0;

	i = 8;
	while(i)
	{
		i--;

		// Far
		if(pv0[i].x <  pv0[i].w)
			dwClipFlagsA |= 1 << 0;

		if(pv0[i].x > -pv0[i].w)
			dwClipFlagsA |= 1 << 1;

		if(pv0[i].y <  pv0[i].w)
			dwClipFlagsA |= 1 << 2;

		if(pv0[i].y > -pv0[i].w)
			dwClipFlagsA |= 1 << 3;

		if(pv0[i].z >  0)
			dwClipFlagsA |= 1 << 4;

		// Near
		if(pBoundingHyperCube[i].x <  pBoundingHyperCube[i].w)
			dwClipFlagsB |= 1 << 0;

		if(pBoundingHyperCube[i].x > -pBoundingHyperCube[i].w)
			dwClipFlagsB |= 1 << 1;

		if(pBoundingHyperCube[i].y <  pBoundingHyperCube[i].w)
			dwClipFlagsB |= 1 << 2;

		if(pBoundingHyperCube[i].y > -pBoundingHyperCube[i].w)
			dwClipFlagsB |= 1 << 3;

		if(pBoundingHyperCube[i].z > 0)
			dwClipFlagsB |= 1 << 4;
	}

	/*
		Volume is hidden if all the vertices are over a screen edge
	*/
	if((dwClipFlagsA | dwClipFlagsB) != 0x1F)
		return false;

	/*
		Well, according to the simple bounding box check, it might be
		visible. Let's now test the view frustrum against the bounding
		hyper cube. (Basically the reverse of the previous test!)

		This catches those cases where a diagonal hyper cube passes near a
		screen edge.
	*/

	// Subtract the camera position from the vertices. I.e. move the camera to 0,0,0
	for(i = 0; i < 16; ++i) {
		vShift[i].x = pBoundingHyperCube[i].x;
		vShift[i].y = pBoundingHyperCube[i].y;
		vShift[i].z = pBoundingHyperCube[i].z - fCamZ;
	}

	i = 32;
	while(i) {
		--i;

		w0 = c_pwLinesHyperCube[2 * i + 0];
		w1 = c_pwLinesHyperCube[2 * i + 1];

		PVRTMatrixVec3CrossProduct(v, vShift[w0], vShift[w1]);
		dwClipFlagsA = 0;

		j = 4;
		while(j) {
			--j;

			if(PVRTMatrixVec3DotProduct(c_pvRect[j], v) < 0)
				++dwClipFlagsA;
		}

		// dwClipFlagsA will be 0 or 4 if the screen edges are on the outside of
		// this bounding-box-silhouette-edge.
		if(dwClipFlagsA % 4)
			continue;

		j = 16;
		while(j) {
			--j;

			if((j != w0) & (j != w1) && (PVRTMatrixVec3DotProduct(vShift[j], v) > 0))
				++dwClipFlagsA;
		}

		// dwClipFlagsA will be 0 or 18 if this is a silhouette edge of the bounding box
		if(dwClipFlagsA % 18)
			continue;

		return false;
	}

	return true;
}
/*!***********************************************************************
@Function		IsFrontClipInVolume
@Input			pBoundingHyperCube	The hypercube to test against
@Return			bool				
@Description	Returns true if the hypercube is within the view frustrum.
*************************************************************************/
static bool IsFrontClipInVolume(
	const PVRTVECTOR4	* const pBoundingHyperCube)
{
	const PVRTVECTOR4	*pv0, *pv1;
	unsigned int				dwClipFlags;
	int					i;
	float				fScale, x, y, w;

	/*
		OK. The hyper-bounding-box is in the view frustrum.

		Now decide if we can use Z-pass instead of Z-fail.

		TODO: if we calculate the convex hull of the front-clip intersection
		points, we can use the connecting lines to do a more accurate on-
		screen check (currently it just uses the bounding box of the
		intersection points.)
	*/
	dwClipFlags = 0;

	i = 32;
	while(i) {
		--i;

		pv0 = &pBoundingHyperCube[c_pwLinesHyperCube[2 * i + 0]];
		pv1 = &pBoundingHyperCube[c_pwLinesHyperCube[2 * i + 1]];

		// If both coords are negative, or both coords are positive, it doesn't cross the Z=0 plane
		if(pv0->z * pv1->z > 0)
			continue;

		// TODO: if fScale > 0.5f, do the lerp in the other direction; this is
		// because we want fScale to be close to 0, not 1, to retain accuracy.
		fScale = (0 - pv0->z) / (pv1->z - pv0->z);

		x = fScale * pv1->x + (1.0f - fScale) * pv0->x;
		y = fScale * pv1->y + (1.0f - fScale) * pv0->y;
		w = fScale * pv1->w + (1.0f - fScale) * pv0->w;

		if(x > -w)
			dwClipFlags |= 1 << 0;

		if(x < w)
			dwClipFlags |= 1 << 1;

		if(y > -w)
			dwClipFlags |= 1 << 2;

		if(y < w)
			dwClipFlags |= 1 << 3;
	}

	if(dwClipFlags == 0x0F)
		return true;

	return false;
}

/*!***********************************************************************
@Function		PVRTShadowVolBoundingBoxExtrude
@Modified		pvExtrudedCube	8 Vertices to represent the extruded box
@Input			pBoundingBox	The bounding box to extrude
@Input			pvLightMdl		The light position/direction
@Input			bPointLight		Is the light a point light
@Input			fVolLength		The length the volume has been extruded by
@Description	Extrudes the bounding box of the volume
*************************************************************************/
void PVRTShadowVolBoundingBoxExtrude(
	PVRTVECTOR3				* const pvExtrudedCube,
	const PVRTBOUNDINGBOX	* const pBoundingBox,
	const PVRTVECTOR3		* const pvLightMdl,
	const bool				bPointLight,
	const float				fVolLength)
{
	int i;

	if(bPointLight) {
		i = 8;
		while(i)
		{
			i--;

			pvExtrudedCube[i].x = pBoundingBox->Point[i].x + fVolLength * (pBoundingBox->Point[i].x - pvLightMdl->x);
			pvExtrudedCube[i].y = pBoundingBox->Point[i].y + fVolLength * (pBoundingBox->Point[i].y - pvLightMdl->y);
			pvExtrudedCube[i].z = pBoundingBox->Point[i].z + fVolLength * (pBoundingBox->Point[i].z - pvLightMdl->z);
		}
	} else {
		i = 8;
		while(i)
		{
			i--;

			pvExtrudedCube[i].x = pBoundingBox->Point[i].x + fVolLength * pvLightMdl->x;
			pvExtrudedCube[i].y = pBoundingBox->Point[i].y + fVolLength * pvLightMdl->y;
			pvExtrudedCube[i].z = pBoundingBox->Point[i].z + fVolLength * pvLightMdl->z;
		}
	}
}

/*!***********************************************************************
@Function		PVRTShadowVolBoundingBoxIsVisible
@Modified		pdwVisFlags		Visibility flags
@Input			bObVisible		Unused set to true
@Input			bNeedsZClipping	Unused set to true
@Input			pBoundingBox	The volumes bounding box
@Input			pmTrans			The projection matrix
@Input			pvLightMdl		The light position/direction
@Input			bPointLight		Is the light a point light
@Input			fCamZProj		The camera's z projection value
@Input			fVolLength		The length the volume is extruded by
@Description	Determines if the volume is visible and if it needs caps
*************************************************************************/
void PVRTShadowVolBoundingBoxIsVisible(
	unsigned int			* const pdwVisFlags,
	const bool				bObVisible,				// Is the object visible?
	const bool				bNeedsZClipping,		// Does the object require Z clipping?
	const PVRTBOUNDINGBOX	* const pBoundingBox,
	const PVRTMATRIX		* const pmTrans,
	const PVRTVECTOR3		* const pvLightMdl,
	const bool				bPointLight,
	const float				fCamZProj,
	const float				fVolLength)
{
	PVRTVECTOR3		pvExtrudedCube[8];
	PVRTVECTOR4		BoundingHyperCubeT[16];
	int				i;
	unsigned int	dwClipFlagsA, dwClipZCnt;
	float			fLightProjZ;

	PVRT_UNREFERENCED_PARAMETER(bObVisible);
	PVRT_UNREFERENCED_PARAMETER(bNeedsZClipping);

	_ASSERT((bObVisible && bNeedsZClipping) || !bNeedsZClipping);

	/*
		Transform the eight bounding box points into projection space
	*/
	PVRTTransformVec3Array(&BoundingHyperCubeT[0], sizeof(*BoundingHyperCubeT), pBoundingBox->Point,	sizeof(*pBoundingBox->Point),	pmTrans, 8);

	/*
		Get the light Z coordinate in projection space
	*/
	fLightProjZ =
		pmTrans->f[ 2] * pvLightMdl->x +
		pmTrans->f[ 6] * pvLightMdl->y +
		pmTrans->f[10] * pvLightMdl->z +
		pmTrans->f[14];

	/*
		Where is the object relative to the near clip plane and light?
	*/
	dwClipZCnt		= 0;
	dwClipFlagsA	= 0;
	i = 8;
	while(i) {
		--i;

		if(BoundingHyperCubeT[i].z <= 0)
			++dwClipZCnt;

		if(BoundingHyperCubeT[i].z <= fLightProjZ)
			++dwClipFlagsA;
	}

	if(dwClipZCnt == 8 && dwClipFlagsA == 8) {
		// hidden
		*pdwVisFlags = 0;
		return;
	}

	/*
		Shadow the bounding box into pvExtrudedCube.
	*/
	PVRTShadowVolBoundingBoxExtrude(pvExtrudedCube, pBoundingBox, pvLightMdl, bPointLight, fVolLength);

	/*
		Transform to projection space
	*/
	PVRTTransformVec3Array(&BoundingHyperCubeT[8], sizeof(*BoundingHyperCubeT), pvExtrudedCube, sizeof(*pvExtrudedCube), pmTrans, 8);

	/*
		Check whether any part of the hyper bounding box is even visible
	*/
	if(!IsHyperBoundingBoxVisibleEx(BoundingHyperCubeT, fCamZProj)) {
		*pdwVisFlags = 0;
		return;
	}

	/*
		It's visible, so choose a render method
	*/
	if(dwClipZCnt == 8) {
		// 1
		if(IsFrontClipInVolume(BoundingHyperCubeT)) {
			*pdwVisFlags = PVRTSHADOWVOLUME_VISIBLE | PVRTSHADOWVOLUME_NEED_ZFAIL;

			if(IsBoundingBoxVisibleEx(&BoundingHyperCubeT[8], fCamZProj))
			{
				*pdwVisFlags |= PVRTSHADOWVOLUME_NEED_CAP_BACK;
			}
		} else {
			*pdwVisFlags = PVRTSHADOWVOLUME_VISIBLE;
		}
	} else {
		if(!(dwClipZCnt | dwClipFlagsA)) {
			// 3
			*pdwVisFlags = PVRTSHADOWVOLUME_VISIBLE;
		} else {
			// 5
			if(IsFrontClipInVolume(BoundingHyperCubeT)) {
				*pdwVisFlags = PVRTSHADOWVOLUME_VISIBLE | PVRTSHADOWVOLUME_NEED_ZFAIL;

				if(IsBoundingBoxVisibleEx(BoundingHyperCubeT, fCamZProj))
				{
					*pdwVisFlags |= PVRTSHADOWVOLUME_NEED_CAP_FRONT;
				}

				if(IsBoundingBoxVisibleEx(&BoundingHyperCubeT[8], fCamZProj))
				{
					*pdwVisFlags |= PVRTSHADOWVOLUME_NEED_CAP_BACK;
				}
			} else {
				*pdwVisFlags = PVRTSHADOWVOLUME_VISIBLE;
			}
		}
	}
}

/*!***********************************************************************
@Function		PVRTShadowVolSilhouetteProjectedRender
@Input			psMesh		Shadow volume mesh
@Input			psVol		Renderable shadow volume information
@Input			pContext	A struct for passing in API specific data
@Description	Draws the shadow volume
*************************************************************************/
int PVRTShadowVolSilhouetteProjectedRender(
	const PVRTShadowVolShadowMesh	* const psMesh,
	const PVRTShadowVolShadowVol	* const psVol,
	const SPVRTContext				* const pContext)
{
#if defined(BUILD_DX11)
	return 0; // Not implemented yet
#endif

#if defined(BUILD_OGL) || defined(BUILD_OGLES2) || defined(BUILD_OGLES) || defined(BUILD_OGLES3)
	_ASSERT(psMesh->pivb);

#if defined(_DEBUG) // To fix error in Linux
	_ASSERT(psVol->nIdxCnt <= psVol->nIdxCntMax);
	_ASSERT(psVol->nIdxCnt % 3 == 0);
	_ASSERT(psVol->nIdxCnt / 3 <= 0xFFFF);
#endif

#if defined(BUILD_OGL)
	_ASSERT(pContext && pContext->pglExt);

	//Bind the buffers
	pContext->pglExt->glBindBufferARB(GL_ARRAY_BUFFER_ARB, psMesh->pivb);
	pContext->pglExt->glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, psVol->piib);
	
	pContext->pglExt->glEnableVertexAttribArrayARB(0);
	pContext->pglExt->glEnableVertexAttribArrayARB(1);
	
	pContext->pglExt->glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(SVertexShVol), (void*)0);
	pContext->pglExt->glVertexAttribPointerARB(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(SVertexShVol), (void*)12);

	glDrawElements(GL_TRIANGLES, psVol->nIdxCnt, GL_UNSIGNED_SHORT, NULL);

	pContext->pglExt->glDisableVertexAttribArrayARB(0);
	pContext->pglExt->glDisableVertexAttribArrayARB(1);

	pContext->pglExt->glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	pContext->pglExt->glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

	return psVol->nIdxCnt / 3;
#elif defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	PVRT_UNREFERENCED_PARAMETER(pContext);
	GLint i32CurrentProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &i32CurrentProgram);

	_ASSERT(i32CurrentProgram); //no program currently set

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SVertexShVol), &((SVertexShVol*)psMesh->pivb)[0].x);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(SVertexShVol), &((SVertexShVol*)psMesh->pivb)[0].dwExtrude);
	glEnableVertexAttribArray(1);

	glDrawElements(GL_TRIANGLES, psVol->nIdxCnt, GL_UNSIGNED_SHORT, psVol->piib);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	return psVol->nIdxCnt / 3;

#elif defined(BUILD_OGLES)
	_ASSERT(pContext && pContext->pglesExt);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_MATRIX_INDEX_ARRAY_OES);
	glEnableClientState(GL_WEIGHT_ARRAY_OES);

	glVertexPointer(3, GL_FLOAT, sizeof(SVertexShVol), &((SVertexShVol*)psMesh->pivb)[0].x);
	pContext->pglesExt->glMatrixIndexPointerOES(1, GL_UNSIGNED_BYTE, sizeof(SVertexShVol), &((SVertexShVol*)psMesh->pivb)[0].dwExtrude);
	pContext->pglesExt->glWeightPointerOES(1, GL_FLOAT, sizeof(SVertexShVol), &((SVertexShVol*)psMesh->pivb)[0].fWeight);

	glDrawElements(GL_TRIANGLES, psVol->nIdxCnt, GL_UNSIGNED_SHORT, psVol->piib);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_MATRIX_INDEX_ARRAY_OES);
	glDisableClientState(GL_WEIGHT_ARRAY_OES);

	return psVol->nIdxCnt / 3;
#endif

#endif
}

/*****************************************************************************
 End of file (PVRTShadowVol.cpp)
*****************************************************************************/

