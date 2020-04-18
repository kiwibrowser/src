/*!****************************************************************************

 @file         PVRTShadowVol.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Declarations of functions relating to shadow volume generation.

******************************************************************************/
#ifndef _PVRTSHADOWVOL_H_
#define _PVRTSHADOWVOL_H_

#include "PVRTContext.h"
#include "PVRTVector.h"

/****************************************************************************
** Defines
****************************************************************************/
#define PVRTSHADOWVOLUME_VISIBLE		0x00000001
#define PVRTSHADOWVOLUME_NEED_CAP_FRONT	0x00000002
#define PVRTSHADOWVOLUME_NEED_CAP_BACK	0x00000004
#define PVRTSHADOWVOLUME_NEED_ZFAIL		0x00000008

/****************************************************************************
** Structures
****************************************************************************/

/*!***********************************************************************
 @brief      	Edge to form part of a shadow volume mesh.
*************************************************************************/
struct PVRTShadowVolMEdge {
	unsigned short	wV0, wV1;		/*!< Indices of the vertices of the edge */
	int				nVis;			/*!< Bit0 = Visible, Bit1 = Hidden, Bit2 = Reverse Winding */
};

/*!***********************************************************************
 @brief      	Triangle to form part of a shadow volume mesh.
*************************************************************************/
struct PVRTShadowVolMTriangle {
	unsigned short	w[3];			/*!< Source indices of the triangle */	
	unsigned int    wE0, wE1, wE2;  /*!< Indices of the edges of the triangle */
	PVRTVECTOR3	vNormal;			/*!< Triangle normal */
	int			nWinding;			/*!< BitN = Correct winding for edge N */
};

/*!***********************************************************************
 @brief      	Shadow volume mesh.
*************************************************************************/
struct PVRTShadowVolShadowMesh {
	PVRTVECTOR3		*pV;	        /*!< Unique vertices in object space */
	PVRTShadowVolMEdge		*pE;    /*!< Unique edges in object space */
	PVRTShadowVolMTriangle	*pT;    /*!< Unique triangles in object space */
	unsigned int	nV;		        /*!< Vertex count */
	unsigned int	nE;		        /*!< Edge count */
	unsigned int	nT;		        /*!< Triangle count */

#ifdef BUILD_DX11
	ID3D11Buffer	*pivb;		/*!< Two copies of the vertices */
#endif
#if defined(BUILD_OGL)
	unsigned int	pivb;	/*!< Two copies of the vertices */
#endif
#if defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	void			*pivb;		/*!< Two copies of the vertices */
#endif
};

/*!***********************************************************************
 @brief      	Renderable shadow-volume information.
*************************************************************************/
struct PVRTShadowVolShadowVol {
#ifdef BUILD_DX11
	ID3D11Buffer	*piib;		/*!< Two copies of the vertices */
#endif
#if defined(BUILD_OGL)
	unsigned int			piib;	
#endif
#if defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
	unsigned short			*piib;		/*!< Indices to render the volume */
#endif
	unsigned int			nIdxCnt;	/*!< Number of indices in piib */

#ifdef _DEBUG
	unsigned int			nIdxCntMax;	/*!< Number of indices which can fit in piib */
#endif
};

/****************************************************************************
** Declarations
****************************************************************************/

/*!***********************************************************************
@fn       	    PVRTShadowVolMeshCreateMesh
@param[in,out]	psMesh		The shadow volume mesh to populate
@param[in]		pVertex		A list of vertices
@param[in]		nNumVertex	The number of vertices
@param[in]		pFaces		A list of faces
@param[in]		nNumFaces	The number of faces
@brief      	Creates a mesh format suitable for generating shadow volumes
*************************************************************************/
void PVRTShadowVolMeshCreateMesh(
	PVRTShadowVolShadowMesh		* const psMesh,
	const float				* const pVertex,
	const unsigned int		nNumVertex,
	const unsigned short	* const pFaces,
	const unsigned int		nNumFaces);

/*!***********************************************************************
@fn       		PVRTShadowVolMeshInitMesh
@param[in]		psMesh	The shadow volume mesh
@param[in]		pContext	A struct for API specific data
@return  		True on success
@brief      	Init the mesh
*************************************************************************/
bool PVRTShadowVolMeshInitMesh(
	PVRTShadowVolShadowMesh		* const psMesh,
	const SPVRTContext		* const pContext);

/*!***********************************************************************
@fn       		PVRTShadowVolMeshInitVol
@param[in,out]	psVol	The shadow volume struct
@param[in]		psMesh	The shadow volume mesh
@param[in]		pContext	A struct for API specific data
@return 		True on success
@brief      	Init the renderable shadow volume information.
*************************************************************************/
bool PVRTShadowVolMeshInitVol(
	PVRTShadowVolShadowVol			* const psVol,
	const PVRTShadowVolShadowMesh	* const psMesh,
	const SPVRTContext		* const pContext);

/*!***********************************************************************
@fn       		PVRTShadowVolMeshDestroyMesh
@param[in]		psMesh	The shadow volume mesh to destroy
@brief      	Destroys all shadow volume mesh data created by PVRTShadowVolMeshCreateMesh
*************************************************************************/
void PVRTShadowVolMeshDestroyMesh(
	PVRTShadowVolShadowMesh		* const psMesh);

/*!***********************************************************************
@fn       		PVRTShadowVolMeshReleaseMesh
@param[in]		psMesh	The shadow volume mesh to release
@brief      	Releases all shadow volume mesh data created by PVRTShadowVolMeshInitMesh
*************************************************************************/
void PVRTShadowVolMeshReleaseMesh(
	PVRTShadowVolShadowMesh		* const psMesh,
	SPVRTContext				* const psContext=NULL);

/*!***********************************************************************
@fn       		PVRTShadowVolMeshReleaseVol
@param[in]		psVol	The shadow volume information to release
@brief      	Releases all data create by PVRTShadowVolMeshInitVol
*************************************************************************/
void PVRTShadowVolMeshReleaseVol(
	PVRTShadowVolShadowVol			* const psVol,
	SPVRTContext					* const psContext=NULL);

/*!***********************************************************************
@fn       		PVRTShadowVolSilhouetteProjectedBuild
@param[in,out]	psVol	        The shadow volume information
@param[in]		dwVisFlags	    Shadow volume creation flags
@param[in]		psMesh	        The shadow volume mesh
@param[in]		pvLightModel	The light position/direction
@param[in]		bPointLight		Is the light a point light
@param[in]		pContext	    A struct for passing in API specific data	
@brief      	Using the light set up the shadow volume so it can be extruded.
*************************************************************************/
void PVRTShadowVolSilhouetteProjectedBuild(
	PVRTShadowVolShadowVol			* const psVol,
	const unsigned int				dwVisFlags,
	const PVRTShadowVolShadowMesh	* const psMesh,
	const PVRTVECTOR3		* const pvLightModel,
	const bool				bPointLight,
	const SPVRTContext * const pContext = 0);

/*!***********************************************************************
@fn       		PVRTShadowVolSilhouetteProjectedBuild
@param[in,out]	psVol	The shadow volume information
@param[in]		dwVisFlags	Shadow volume creation flags
@param[in]		psMesh	The shadow volume mesh
@param[in]		pvLightModel	The light position/direction
@param[in]		bPointLight		Is the light a point light
@param[in]		pContext	A struct for passing in API specific data	
@brief      	Using the light set up the shadow volume so it can be extruded.
*************************************************************************/
void PVRTShadowVolSilhouetteProjectedBuild(
	PVRTShadowVolShadowVol			* const psVol,
	const unsigned int		dwVisFlags,
	const PVRTShadowVolShadowMesh	* const psMesh,
	const PVRTVec3		* const pvLightModel,
	const bool				bPointLight,
	const SPVRTContext * const pContext = 0);

/*!***********************************************************************
@fn       		PVRTShadowVolBoundingBoxExtrude
@param[in,out]	pvExtrudedCube	8 Vertices to represent the extruded box
@param[in]		pBoundingBox	The bounding box to extrude
@param[in]		pvLightMdl		The light position/direction
@param[in]		bPointLight		Is the light a point light
@param[in]		fVolLength		The length the volume has been extruded by
@brief      	Extrudes the bounding box of the volume
*************************************************************************/
void PVRTShadowVolBoundingBoxExtrude(
	PVRTVECTOR3				* const pvExtrudedCube,
	const PVRTBOUNDINGBOX	* const pBoundingBox,
	const PVRTVECTOR3		* const pvLightMdl,
	const bool				bPointLight,
	const float				fVolLength);

/*!***********************************************************************
@fn       		PVRTShadowVolBoundingBoxIsVisible
@param[in,out]	pdwVisFlags		Visibility flags
@param[in]		bObVisible		Is the object visible? Unused set to true
@param[in]		bNeedsZClipping	Does the object require Z clipping? Unused set to true
@param[in]		pBoundingBox	The volumes bounding box
@param[in]		pmTrans			The projection matrix
@param[in]		pvLightMdl		The light position/direction
@param[in]		bPointLight		Is the light a point light
@param[in]		fCamZProj		The camera's z projection value
@param[in]		fVolLength		The length the volume is extruded by
@brief      	Determines if the volume is visible and if it needs caps
*************************************************************************/
void PVRTShadowVolBoundingBoxIsVisible(
	unsigned int			* const pdwVisFlags,
	const bool				bObVisible,
	const bool				bNeedsZClipping,
	const PVRTBOUNDINGBOX	* const pBoundingBox,
	const PVRTMATRIX		* const pmTrans,
	const PVRTVECTOR3		* const pvLightMdl,
	const bool				bPointLight,
	const float				fCamZProj,
	const float				fVolLength);

/*!***********************************************************************
@fn       		PVRTShadowVolSilhouetteProjectedRender
@param[in]		psMesh		Shadow volume mesh
@param[in]		psVol		Renderable shadow volume information
@param[in]		pContext	A struct for passing in API specific data
@brief      	Draws the shadow volume
*************************************************************************/
int PVRTShadowVolSilhouetteProjectedRender(
	const PVRTShadowVolShadowMesh	* const psMesh,
	const PVRTShadowVolShadowVol	* const psVol,
	const SPVRTContext		* const pContext);


#endif /* _PVRTSHADOWVOL_H_ */

/*****************************************************************************
 End of file (PVRTShadowVol.h)
*****************************************************************************/

