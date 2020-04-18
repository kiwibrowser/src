/******************************************************************************

 @File         PVRTFixedPoint.cpp

 @Title        PVRTFixedPoint

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independant

 @Description  Converts MAX exported meshes to fixed point objects for use with
               opengles lite.

******************************************************************************/
#include <math.h>
#include <string.h>
#include "PVRTContext.h"
#include "PVRTFixedPoint.h"

/********************************************************
** Most of the code only applies to CommonLite profile **
********************************************************/
#ifdef PVRT_FIXED_POINT_ENABLE

/*!***************************************************************************
 @Function		CreateFixedObjectMesh
 @Input			mesh	The mesh to create the fixed point version from
 @Returns		A fixed point version of mesh
 @Description	Converts model floating point data to fixed point
*****************************************************************************/
HeaderStruct_Fixed_Mesh *CreateFixedObjectMesh(HeaderStruct_Mesh *mesh)
{
	HeaderStruct_Fixed_Mesh *new_mesh = new HeaderStruct_Fixed_Mesh;

	new_mesh->fCenter[0] = PVRTF2X(mesh->fCenter[0]);
	new_mesh->fCenter[1] = PVRTF2X(mesh->fCenter[1]);
	new_mesh->fCenter[2] = PVRTF2X(mesh->fCenter[2]);


	new_mesh->nNumVertex = mesh->nNumVertex;
	new_mesh->nNumFaces = mesh->nNumFaces;
	new_mesh->nNumStrips = mesh->nNumStrips;
	new_mesh->nMaterial = mesh->nMaterial;

	if(mesh->nNumVertex)
	{
		new_mesh->pVertex = new VERTTYPE[mesh->nNumVertex*3];
		for(unsigned int i = 0; i < mesh->nNumVertex*3; i++)		// each vertex is 3 floats
			new_mesh->pVertex[i] = PVRTF2X(mesh->pVertex[i]);
	}
	else
	{
		new_mesh->pVertex = 0;
		new_mesh->nNumVertex = 0;
	}

	if(mesh->pUV)
	{
		new_mesh->pUV = new VERTTYPE[mesh->nNumVertex*2];
		for(unsigned int i = 0; i < mesh->nNumVertex*2; i++)		// UVs come in pairs of floats
			new_mesh->pUV[i] = PVRTF2X(mesh->pUV[i]);
	}
	else
		new_mesh->pUV = 0;

	if(mesh->pNormals)
	{
		new_mesh->pNormals = new VERTTYPE[mesh->nNumVertex*3];
		for(unsigned int i = 0; i < mesh->nNumVertex*3; i++)		// each normal is 3 floats
			new_mesh->pNormals[i] = PVRTF2X(mesh->pNormals[i]);
	}
	else
	{
		new_mesh->pNormals = 0;
	}

	/*
	 * Format of packedVerts is
	 *		Position
	 *		Normal / Colour
	 *		UVs
	 */

#define MF_NORMALS 1
#define MF_VERTEXCOLOR 2
#define MF_UV 3

	if(mesh->pPackedVertex)
	{
		unsigned int nPackedVertSize = mesh->nNumVertex * 3 +
					(mesh->nFlags & MF_NORMALS		? mesh->nNumVertex * 3 : 0) +
					(mesh->nFlags & MF_VERTEXCOLOR	? mesh->nNumVertex * 3 : 0) +
					(mesh->nFlags & MF_UV			? mesh->nNumVertex * 2 : 0);

		new_mesh->pPackedVertex = new VERTTYPE[nPackedVertSize];
		for(unsigned int i = 0; i < nPackedVertSize; i++)
			new_mesh->pPackedVertex[i] = PVRTF2X(mesh->pPackedVertex[i]);
	}
	else
		new_mesh->pPackedVertex = 0;

	// simply copy reference to all properties which do not need conversion (indicies)

	new_mesh->pVertexColor				= mesh->pVertexColor;
	new_mesh->pVertexMaterial			= mesh->pVertexMaterial;
	new_mesh->pFaces					= mesh->pFaces;
	new_mesh->pStrips					= mesh->pStrips;
	new_mesh->pStripLength				= mesh->pStripLength;

	// we're leaving the patch stuff alone

	new_mesh->Patch.nType				= mesh->Patch.nType;
	new_mesh->Patch.nNumPatches			= mesh->Patch.nNumPatches;
	new_mesh->Patch.nNumVertices		= mesh->Patch.nNumVertices;
	new_mesh->Patch.nNumSubdivisions	= mesh->Patch.nNumSubdivisions;
	new_mesh->Patch.pControlPoints		= mesh->Patch.pControlPoints;
	new_mesh->Patch.pUVs				= mesh->Patch.pUVs;

	return new_mesh;
}

/*!***************************************************************************
 @Function		FreeFixedObjectMesh
 @Input			mesh	The mesh to delete
 @Description	Release memory allocated in CreateFixedObjectMesh()
*****************************************************************************/
void FreeFixedObjectMesh(HeaderStruct_Fixed_Mesh* mesh)
{

	delete[] mesh->pVertex;
	delete[] mesh->pUV;
	delete[] mesh->pNormals;
	delete[] mesh->pPackedVertex;

	delete mesh;
}

#endif

/*!***************************************************************************
 @Function		PVRTLoadHeaderObject
 @Input			headerObj			Pointer to object structure in the header file
 @Return		directly usable geometry in fixed or float format as appropriate
 @Description	Converts the data exported by MAX to fixed point when used in OpenGL
				ES common-lite profile.
*****************************************************************************/
HeaderStruct_Mesh_Type *PVRTLoadHeaderObject(const void *headerObj)
{
#ifdef PVRT_FIXED_POINT_ENABLE
	return (HeaderStruct_Mesh_Type*) CreateFixedObjectMesh((HeaderStruct_Mesh *) headerObj);
#else
	HeaderStruct_Mesh_Type *new_mesh = new HeaderStruct_Mesh_Type;
	memcpy (new_mesh,headerObj,sizeof(HeaderStruct_Mesh_Type));
	return (HeaderStruct_Mesh_Type*) new_mesh;
#endif
}

/*!***************************************************************************
 @Function		PVRTUnloadHeaderObject
 @Input			headerObj			Pointer returned by LoadHeaderObject
 @Description	Releases memory allocated by LoadHeaderObject when geometry no longer
				needed.
*****************************************************************************/
void PVRTUnloadHeaderObject(HeaderStruct_Mesh_Type* headerObj)
{
#ifdef PVRT_FIXED_POINT_ENABLE
	FreeFixedObjectMesh(headerObj);
#else
	delete headerObj;
#endif
}

/*****************************************************************************
 End of file (PVRTFixedPoint.cpp)
*****************************************************************************/

