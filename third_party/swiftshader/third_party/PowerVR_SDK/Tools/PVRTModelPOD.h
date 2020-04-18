/*!****************************************************************************

 @file         PVRTModelPOD.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Code to load POD files - models exported from MAX.

******************************************************************************/
#ifndef _PVRTMODELPOD_H_
#define _PVRTMODELPOD_H_

#include "PVRTVector.h"
#include "PVRTError.h"
#include "PVRTVertex.h"
#include "PVRTBoneBatch.h"

/****************************************************************************
** Defines
****************************************************************************/
#define PVRTMODELPOD_VERSION	("AB.POD.2.0") /*!< POD file version string */

// PVRTMODELPOD Scene Flags
#define PVRTMODELPODSF_FIXED	(0x00000001)   /*!< PVRTMODELPOD Fixed-point 16.16 data (otherwise float) flag */

/****************************************************************************
** Enumerations
****************************************************************************/
/*!****************************************************************************
 @struct      EPODLightType
 @brief       Enum for the POD format light types
******************************************************************************/
enum EPODLightType
{
	ePODPoint=0,	 /*!< Point light */
	ePODDirectional, /*!< Directional light */
	ePODSpot,		 /*!< Spot light */
	eNumPODLightTypes
};

/*!****************************************************************************
 @struct      EPODPrimitiveType
 @brief       Enum for the POD format primitive types
******************************************************************************/
enum EPODPrimitiveType
{
	ePODTriangles=0, /*!< Triangles */
	eNumPODPrimitiveTypes
};

/*!****************************************************************************
 @struct      EPODAnimationData
 @brief       Enum for the POD format animation types
******************************************************************************/
enum EPODAnimationData
{
	ePODHasPositionAni	= 0x01,	/*!< Position animation */
	ePODHasRotationAni	= 0x02, /*!< Rotation animation */
	ePODHasScaleAni		= 0x04, /*!< Scale animation */
	ePODHasMatrixAni	= 0x08  /*!< Matrix animation */
};

/*!****************************************************************************
 @struct      EPODMaterialFlags
 @brief       Enum for the material flag options
******************************************************************************/
enum EPODMaterialFlag
{
	ePODEnableBlending	= 0x01	/*!< Enable blending for this material */
};

/*!****************************************************************************
 @struct      EPODBlendFunc
 @brief       Enum for the POD format blend functions
******************************************************************************/
enum EPODBlendFunc
{
	ePODBlendFunc_ZERO=0,
	ePODBlendFunc_ONE,
	ePODBlendFunc_BLEND_FACTOR,
	ePODBlendFunc_ONE_MINUS_BLEND_FACTOR,

	ePODBlendFunc_SRC_COLOR = 0x0300,
	ePODBlendFunc_ONE_MINUS_SRC_COLOR,
	ePODBlendFunc_SRC_ALPHA,
	ePODBlendFunc_ONE_MINUS_SRC_ALPHA,
	ePODBlendFunc_DST_ALPHA,
	ePODBlendFunc_ONE_MINUS_DST_ALPHA,
	ePODBlendFunc_DST_COLOR,
	ePODBlendFunc_ONE_MINUS_DST_COLOR,
	ePODBlendFunc_SRC_ALPHA_SATURATE,

	ePODBlendFunc_CONSTANT_COLOR = 0x8001,
	ePODBlendFunc_ONE_MINUS_CONSTANT_COLOR,
	ePODBlendFunc_CONSTANT_ALPHA,
	ePODBlendFunc_ONE_MINUS_CONSTANT_ALPHA
};

/*!****************************************************************************
 @struct      EPODBlendOp
 @brief       Enum for the POD format blend operation
******************************************************************************/
enum EPODBlendOp
{
	ePODBlendOp_ADD = 0x8006,
	ePODBlendOp_MIN,
	ePODBlendOp_MAX,
	ePODBlendOp_SUBTRACT = 0x800A,
	ePODBlendOp_REVERSE_SUBTRACT
};

/****************************************************************************
** Structures
****************************************************************************/
/*!****************************************************************************
 @class      CPODData
 @brief      A class for representing POD data
******************************************************************************/
class CPODData {
public:
	/*!***************************************************************************
	@fn			Reset
	@brief		Resets the POD Data to NULL
	*****************************************************************************/
	void Reset();

public:
	EPVRTDataType	eType;		/*!< Type of data stored */
	PVRTuint32		n;			/*!< Number of values per vertex */
	PVRTuint32		nStride;	/*!< Distance in bytes from one array entry to the next */
	PVRTuint8		*pData;		/*!< Actual data (array of values); if mesh is interleaved, this is an OFFSET from pInterleaved */
};

/*!****************************************************************************
 @struct      SPODCamera
 @brief       Struct for storing POD camera data
******************************************************************************/
struct SPODCamera {
	PVRTint32			nIdxTarget;	/*!< Index of the target object */
	VERTTYPE	fFOV;				/*!< Field of view */
	VERTTYPE	fFar;				/*!< Far clip plane */
	VERTTYPE	fNear;				/*!< Near clip plane */
	VERTTYPE	*pfAnimFOV;			/*!< 1 VERTTYPE per frame of animation. */
};

/*!****************************************************************************
 @struct      SPODLight
 @brief       Struct for storing POD light data
******************************************************************************/
struct SPODLight {
	PVRTint32			nIdxTarget;		/*!< Index of the target object */
	VERTTYPE			pfColour[3];	/*!< Light colour (0.0f -> 1.0f for each channel) */
	EPODLightType		eType;			/*!< Light type (point, directional, spot etc.) */
	PVRTfloat32			fConstantAttenuation;	/*!< Constant attenuation */
	PVRTfloat32			fLinearAttenuation;		/*!< Linear atternuation */
	PVRTfloat32			fQuadraticAttenuation;	/*!< Quadratic attenuation */
	PVRTfloat32			fFalloffAngle;			/*!< Falloff angle (in radians) */
	PVRTfloat32			fFalloffExponent;		/*!< Falloff exponent */
};

/*!****************************************************************************
 @struct      SPODMesh
 @brief       Struct for storing POD mesh data
******************************************************************************/
struct SPODMesh {
	PVRTuint32			nNumVertex;		/*!< Number of vertices in the mesh */
	PVRTuint32			nNumFaces;		/*!< Number of triangles in the mesh */
	PVRTuint32			nNumUVW;		/*!< Number of texture coordinate channels per vertex */
	CPODData			sFaces;			/*!< List of triangle indices */
	PVRTuint32			*pnStripLength;	/*!< If mesh is stripped: number of tris per strip. */
	PVRTuint32			nNumStrips;		/*!< If mesh is stripped: number of strips, length of pnStripLength array. */
	CPODData			sVertex;		/*!< List of vertices (x0, y0, z0, x1, y1, z1, x2, etc...) */
	CPODData			sNormals;		/*!< List of vertex normals (Nx0, Ny0, Nz0, Nx1, Ny1, Nz1, Nx2, etc...) */
	CPODData			sTangents;		/*!< List of vertex tangents (Tx0, Ty0, Tz0, Tx1, Ty1, Tz1, Tx2, etc...) */
	CPODData			sBinormals;		/*!< List of vertex binormals (Bx0, By0, Bz0, Bx1, By1, Bz1, Bx2, etc...) */
	CPODData			*psUVW;			/*!< List of UVW coordinate sets; size of array given by 'nNumUVW' */
	CPODData			sVtxColours;	/*!< A colour per vertex */
	CPODData			sBoneIdx;		/*!< nNumBones*nNumVertex ints (Vtx0Idx0, Vtx0Idx1, ... Vtx1Idx0, Vtx1Idx1, ...) */
	CPODData			sBoneWeight;	/*!< nNumBones*nNumVertex floats (Vtx0Wt0, Vtx0Wt1, ... Vtx1Wt0, Vtx1Wt1, ...) */

	PVRTuint8			*pInterleaved;	/*!< Interleaved vertex data */

	CPVRTBoneBatches	sBoneBatches;	/*!< Bone tables */

	EPODPrimitiveType	ePrimitiveType;	/*!< Primitive type used by this mesh */

	PVRTMATRIX			mUnpackMatrix;	/*!< A matrix used for unscaling scaled vertex data created with PVRTModelPODScaleAndConvertVtxData*/
};

/*!****************************************************************************
 @struct      SPODNode
 @brief       Struct for storing POD node data
******************************************************************************/
struct SPODNode {
	PVRTint32			nIdx;				/*!< Index into mesh, light or camera array, depending on which object list contains this Node */
	PVRTchar8			*pszName;			/*!< Name of object */
	PVRTint32			nIdxMaterial;		/*!< Index of material used on this mesh */

	PVRTint32			nIdxParent;		/*!< Index into MeshInstance array; recursively apply ancestor's transforms after this instance's. */

	PVRTuint32			nAnimFlags;		/*!< Stores which animation arrays the POD Node contains */

	PVRTuint32			*pnAnimPositionIdx;
	VERTTYPE			*pfAnimPosition;	/*!< 3 floats per frame of animation. */

	PVRTuint32			*pnAnimRotationIdx;
	VERTTYPE			*pfAnimRotation;	/*!< 4 floats per frame of animation. */

	PVRTuint32			*pnAnimScaleIdx;
	VERTTYPE			*pfAnimScale;		/*!< 7 floats per frame of animation. */

	PVRTuint32			*pnAnimMatrixIdx;
	VERTTYPE			*pfAnimMatrix;		/*!< 16 floats per frame of animation. */

	PVRTuint32			nUserDataSize;
	PVRTchar8			*pUserData;
};

/*!****************************************************************************
 @struct      SPODTexture
 @brief       Struct for storing POD texture data
******************************************************************************/
struct SPODTexture {
	PVRTchar8	*pszName;			/*!< File-name of texture */
};

/*!****************************************************************************
 @struct      SPODMaterial
 @brief       Struct for storing POD material data
******************************************************************************/
struct SPODMaterial {
	PVRTchar8		*pszName;				/*!< Name of material */
	PVRTint32		nIdxTexDiffuse;			/*!< Idx into pTexture for the diffuse texture */
	PVRTint32		nIdxTexAmbient;			/*!< Idx into pTexture for the ambient texture */
	PVRTint32		nIdxTexSpecularColour;	/*!< Idx into pTexture for the specular colour texture */
	PVRTint32		nIdxTexSpecularLevel;	/*!< Idx into pTexture for the specular level texture */
	PVRTint32		nIdxTexBump;			/*!< Idx into pTexture for the bump map */
	PVRTint32		nIdxTexEmissive;		/*!< Idx into pTexture for the emissive texture */
	PVRTint32		nIdxTexGlossiness;		/*!< Idx into pTexture for the glossiness texture */
	PVRTint32		nIdxTexOpacity;			/*!< Idx into pTexture for the opacity texture */
	PVRTint32		nIdxTexReflection;		/*!< Idx into pTexture for the reflection texture */
	PVRTint32		nIdxTexRefraction;		/*!< Idx into pTexture for the refraction texture */
	VERTTYPE		fMatOpacity;			/*!< Material opacity (used with vertex alpha ?) */
	VERTTYPE		pfMatAmbient[3];		/*!< Ambient RGB value */
	VERTTYPE		pfMatDiffuse[3];		/*!< Diffuse RGB value */
	VERTTYPE		pfMatSpecular[3];		/*!< Specular RGB value */
	VERTTYPE		fMatShininess;			/*!< Material shininess */
	PVRTchar8		*pszEffectFile;			/*!< Name of effect file */
	PVRTchar8		*pszEffectName;			/*!< Name of effect in the effect file */

	EPODBlendFunc	eBlendSrcRGB;		/*!< Blending RGB source value */
	EPODBlendFunc	eBlendSrcA;			/*!< Blending alpha source value */
	EPODBlendFunc	eBlendDstRGB;		/*!< Blending RGB destination value */
	EPODBlendFunc	eBlendDstA;			/*!< Blending alpha destination value */
	EPODBlendOp		eBlendOpRGB;		/*!< Blending RGB operation */
	EPODBlendOp		eBlendOpA;			/*!< Blending alpha operation */
	VERTTYPE		pfBlendColour[4];	/*!< A RGBA colour to be used in blending */
	VERTTYPE		pfBlendFactor[4];	/*!< An array of blend factors, one for each RGBA component */

	PVRTuint32		nFlags;				/*!< Stores information about the material e.g. Enable blending */

	PVRTuint32		nUserDataSize;
	PVRTchar8		*pUserData;
};

/*!****************************************************************************
 @struct      SPODScene
 @brief       Struct for storing POD scene data
******************************************************************************/
struct SPODScene {
	VERTTYPE		fUnits;					/*!< Distance in metres that a single unit of measurement represents */
	VERTTYPE		pfColourBackground[3];	/*!< Background colour */
	VERTTYPE		pfColourAmbient[3];		/*!< Ambient colour */

	PVRTuint32		nNumCamera;				/*!< The length of the array pCamera */
	SPODCamera		*pCamera;				/*!< Camera nodes array */

	PVRTuint32		nNumLight;				/*!< The length of the array pLight */
	SPODLight		*pLight;				/*!< Light nodes array */

	PVRTuint32		nNumMesh;				/*!< The length of the array pMesh */
	SPODMesh		*pMesh;					/*!< Mesh array. Meshes may be instanced several times in a scene; i.e. multiple Nodes may reference any given mesh. */

	PVRTuint32		nNumNode;		/*!< Number of items in the array pNode */
	PVRTuint32		nNumMeshNode;	/*!< Number of items in the array pNode which are objects */
	SPODNode		*pNode;			/*!< Node array. Sorted as such: objects, lights, cameras, Everything Else (bones, helpers etc) */

	PVRTuint32		nNumTexture;	/*!< Number of textures in the array pTexture */
	SPODTexture		*pTexture;		/*!< Texture array */

	PVRTuint32		nNumMaterial;	/*!< Number of materials in the array pMaterial */
	SPODMaterial		*pMaterial;		/*!< Material array */

	PVRTuint32		nNumFrame;		/*!< Number of frames of animation */
	PVRTuint32		nFPS;			/*!< The frames per second the animation should be played at */

	PVRTuint32		nFlags;			/*!< PVRTMODELPODSF_* bit-flags */

	PVRTuint32		nUserDataSize;
	PVRTchar8		*pUserData;
};

struct SPVRTPODImpl;	// Internal implementation data

/*!***************************************************************************
@class CPVRTModelPOD
@brief A class for loading and storing data from POD files/headers
*****************************************************************************/
class CPVRTModelPOD : public SPODScene{
public:
	/*!***************************************************************************
	 @brief     	Constructor for CPVRTModelPOD class
	*****************************************************************************/
	CPVRTModelPOD();

	/*!***************************************************************************
	 @brief     	Destructor for CPVRTModelPOD class
	*****************************************************************************/
	~CPVRTModelPOD();

	/*!***************************************************************************
	@fn       			ReadFromFile
	@param[in]			pszFileName		Filename to load
	@param[out]			pszExpOpt		String in which to place exporter options
	@param[in]			count			Maximum number of characters to store.
	@param[out]			pszHistory		String in which to place the pod file history
	@param[in]			historyCount	Maximum number of characters to store.
	@return			    PVR_SUCCESS if successful, PVR_FAIL if not
	@brief     		    Loads the specified ".POD" file; returns the scene in
						pScene. This structure must later be destroyed with
						PVRTModelPODDestroy() to prevent memory leaks.
						".POD" files are exported using the PVRGeoPOD exporters.
						If pszExpOpt is NULL, the scene is loaded; otherwise the
						scene is not loaded and pszExpOpt is filled in. The same
						is true for pszHistory.
	*****************************************************************************/
	EPVRTError ReadFromFile(
		const char		* const pszFileName,
		char			* const pszExpOpt = NULL,
		const size_t	count = 0,
		char			* const pszHistory = NULL,
		const size_t	historyCount = 0);

	/*!***************************************************************************
	@brief     		    Loads the supplied pod data. This data can be exported
						directly to a header using one of the pod exporters.
						If pszExpOpt is NULL, the scene is loaded; otherwise the
						scene is not loaded and pszExpOpt is filled in. The same
						is true for pszHistory.
	@param[in]			pData			Data to load
	@param[in]			i32Size			Size of data
	@param[out]			pszExpOpt		String in which to place exporter options
	@param[in]			count			Maximum number of characters to store.
	@param[out]			pszHistory		String in which to place the pod file history
	@param[in]			historyCount	Maximum number of characters to store.
	@return	 		    PVR_SUCCESS if successful, PVR_FAIL if not
	*****************************************************************************/
	EPVRTError ReadFromMemory(
		const char		* pData,
		const size_t	i32Size,
		char			* const pszExpOpt = NULL,
		const size_t	count = 0,
		char			* const pszHistory = NULL,
		const size_t	historyCount = 0);

	/*!***************************************************************************
	 @brief     	Sets the scene data from the supplied data structure. Use
					when loading from .H files.
	 @param[in]		scene			Scene data from the header file
	 @return		PVR_SUCCESS if successful, PVR_FAIL if not
	*****************************************************************************/
	EPVRTError ReadFromMemory(
		const SPODScene &scene);

	/*!***************************************************************************
	 @fn       		CopyFromMemory
	 @param[in]			scene			Scene data from the header file
	 @return		PVR_SUCCESS if successful, PVR_FAIL if not
	 @brief     	Copies the scene data from the supplied data structure. Use
					when loading from .H files where you want to modify the data.
	*****************************************************************************/
	EPVRTError CopyFromMemory(
		const SPODScene &scene);

#if defined(_WIN32)
	/*!***************************************************************************
	 @fn       		ReadFromResource
	 @param[in]		pszName			Name of the resource to load from
	 @return		PVR_SUCCESS if successful, PVR_FAIL if not
	 @brief     	Loads the specified ".POD" file; returns the scene in
					pScene. This structure must later be destroyed with
					PVRTModelPODDestroy() to prevent memory leaks.
					".POD" files are exported from 3D Studio MAX using a
					PowerVR plugin.
	*****************************************************************************/
	EPVRTError ReadFromResource(
		const TCHAR * const pszName);
#endif

	/*!***********************************************************************
	 @fn       		InitImpl
	 @brief     	Used by the Read*() fns to initialise implementation
					details. Should also be called by applications which
					manually build data in the POD structures for rendering;
					in this case call it after the data has been created.
					Otherwise, do not call this function.
	*************************************************************************/
	EPVRTError InitImpl();

	/*!***********************************************************************
	 @fn       		DestroyImpl
	 @brief     	Used to free memory allocated by the implementation.
	*************************************************************************/
	void DestroyImpl();

	/*!***********************************************************************
	 @fn       		FlushCache
	 @brief     	Clears the matrix cache; use this if necessary when you
					edit the position or animation of a node.
	*************************************************************************/
	void FlushCache();

	/*!***********************************************************************
	@fn       		IsLoaded
	@brief     	Boolean to check whether a POD file has been loaded.
	*************************************************************************/
	bool IsLoaded();

	/*!***************************************************************************
	 @fn       		Destroy
	 @brief     	Frees the memory allocated to store the scene in pScene.
	*****************************************************************************/
	void Destroy();

	/*!***************************************************************************
	 @fn       		SetFrame
	 @param[in]			fFrame			Frame number
	 @brief     	Set the animation frame for which subsequent Get*() calls
					should return data.
	*****************************************************************************/
	void SetFrame(
		const VERTTYPE fFrame);

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[out]	mOut			Rotation matrix
	 @param[in]		node			Node to get the rotation matrix from
	*****************************************************************************/
	void GetRotationMatrix(
		PVRTMATRIX		&mOut,
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[in]		node			Node to get the rotation matrix from
	 @return		Rotation matrix
	*****************************************************************************/
	PVRTMat4 GetRotationMatrix(
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[out]	mOut			Scaling matrix
	 @param[in]		node			Node to get the rotation matrix from
	*****************************************************************************/
	void GetScalingMatrix(
		PVRTMATRIX		&mOut,
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[in]		node			Node to get the rotation matrix from
	 @return		Scaling matrix
	*****************************************************************************/
	PVRTMat4 GetScalingMatrix(
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the translation vector for the given Mesh
					Instance. Uses animation data.
	 @param[out]	V				Translation vector
	 @param[in]		node			Node to get the translation vector from
	*****************************************************************************/
	void GetTranslation(
		PVRTVECTOR3		&V,
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the translation vector for the given Mesh
					Instance. Uses animation data.
	 @param[in]		node			Node to get the translation vector from
	  @return		Translation vector
	*****************************************************************************/
	PVRTVec3 GetTranslation(
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[out]	mOut			Translation matrix
	 @param[in]		node			Node to get the translation matrix from
	*****************************************************************************/
	void GetTranslationMatrix(
		PVRTMATRIX		&mOut,
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[in]		node			Node to get the translation matrix from
	 @return		Translation matrix
	*****************************************************************************/
	PVRTMat4 GetTranslationMatrix(
		const SPODNode	&node) const;

    /*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[out]	mOut			Transformation matrix
	 @param[in]		node			Node to get the transformation matrix from
	*****************************************************************************/
	void GetTransformationMatrix(PVRTMATRIX &mOut, const SPODNode &node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[out]	mOut			World matrix
	 @param[in]		node			Node to get the world matrix from
	*****************************************************************************/
	void GetWorldMatrixNoCache(
		PVRTMATRIX		&mOut,
		const SPODNode	&node) const;

	/*!***************************************************************************
	@brief     	    Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	@param[in]		node			Node to get the world matrix from
	@return		    World matrix
	*****************************************************************************/
	PVRTMat4 GetWorldMatrixNoCache(
		const SPODNode	&node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	 @param[out]	mOut			World matrix
	 @param[in]		node			Node to get the world matrix from
	*****************************************************************************/
	void GetWorldMatrix(
		PVRTMATRIX		&mOut,
		const SPODNode	&node) const;

	/*!***************************************************************************
	@brief     	    Generates the world matrix for the given Mesh Instance;
					applies the parent's transform too. Uses animation data.
	@param[in]		node			Node to get the world matrix from
	@return		    World matrix
	*****************************************************************************/
	PVRTMat4 GetWorldMatrix(const SPODNode& node) const;

	/*!***************************************************************************
	 @brief     	Generates the world matrix for the given bone.
	 @param[out]	mOut			Bone world matrix
	 @param[in]		NodeMesh		Mesh to take the world matrix from
	 @param[in]		NodeBone		Bone to take the matrix from
	*****************************************************************************/
	void GetBoneWorldMatrix(
		PVRTMATRIX		&mOut,
		const SPODNode	&NodeMesh,
		const SPODNode	&NodeBone);

	/*!***************************************************************************
	@brief     	    Generates the world matrix for the given bone.
	@param[in]		NodeMesh		Mesh to take the world matrix from
	@param[in]		NodeBone		Bone to take the matrix from
	@return		    Bone world matrix
	*****************************************************************************/
	PVRTMat4 GetBoneWorldMatrix(
		const SPODNode	&NodeMesh,
		const SPODNode	&NodeBone);

	/*!***************************************************************************
	 @fn       		GetCamera
	 @param[out]	vFrom			Position of the camera
	 @param[out]	vTo				Target of the camera
	 @param[out]	vUp				Up direction of the camera
	 @param[in]		nIdx			Camera number
	 @return		Camera horizontal FOV
	 @brief     	Calculate the From, To and Up vectors for the given
					camera. Uses animation data.
					Note that even if the camera has a target, *pvTo is not
					the position of that target. *pvTo is a position in the
					correct direction of the target, one unit away from the
					camera.
	*****************************************************************************/
	VERTTYPE GetCamera(
		PVRTVECTOR3			&vFrom,
		PVRTVECTOR3			&vTo,
		PVRTVECTOR3			&vUp,
		const unsigned int	nIdx) const;

	/*!***************************************************************************
	 @fn       		GetCameraPos
	 @param[out]	vFrom			Position of the camera
	 @param[out]	vTo				Target of the camera
	 @param[in]		nIdx			Camera number
	 @return		Camera horizontal FOV
	 @brief     	Calculate the position of the camera and its target. Uses
					animation data.
					If the queried camera does not have a target, *pvTo is
					not changed.
	*****************************************************************************/
	VERTTYPE GetCameraPos(
		PVRTVECTOR3			&vFrom,
		PVRTVECTOR3			&vTo,
		const unsigned int	nIdx) const;

	/*!***************************************************************************
	 @fn       		GetLight
	 @param[out]	vPos			Position of the light
	 @param[out]	vDir			Direction of the light
	 @param[in]		nIdx			Light number
	 @brief     	Calculate the position and direction of the given Light.
					Uses animation data.
	*****************************************************************************/
	void GetLight(
		PVRTVECTOR3			&vPos,
		PVRTVECTOR3			&vDir,
		const unsigned int	nIdx) const;

	/*!***************************************************************************
	 @fn       		GetLightPosition
	 @param[in]		u32Idx			Light number
	 @return		PVRTVec4 position of light with w set correctly
	 @brief     	Calculate the position the given Light. Uses animation data.
	*****************************************************************************/
	PVRTVec4 GetLightPosition(const unsigned int u32Idx) const;

	/*!***************************************************************************
	@fn       		GetLightDirection
	@param[in]		u32Idx			Light number
	@return			PVRTVec4 direction of light with w set correctly
	@brief     	Calculate the direction of the given Light. Uses animation data.
	*****************************************************************************/
	PVRTVec4 GetLightDirection(const unsigned int u32Idx) const;

	/*!***************************************************************************
	 @fn       		CreateSkinIdxWeight
	 @param[out]	pIdx				Four bytes containing matrix indices for vertex (0..255) (D3D: use UBYTE4)
	 @param[out]	pWeight				Four bytes containing blend weights for vertex (0.0 .. 1.0) (D3D: use D3DCOLOR)
	 @param[in]		nVertexBones		Number of bones this vertex uses
	 @param[in]		pnBoneIdx			Pointer to 'nVertexBones' indices
	 @param[in]		pfBoneWeight		Pointer to 'nVertexBones' blend weights
	 @brief     	Creates the matrix indices and blend weights for a boned
					vertex. Call once per vertex of a boned mesh.
	*****************************************************************************/
	EPVRTError CreateSkinIdxWeight(
		char			* const pIdx,
		char			* const pWeight,
		const int		nVertexBones,
		const int		* const pnBoneIdx,
		const VERTTYPE	* const pfBoneWeight);

	/*!***************************************************************************
	 @fn       		SavePOD
	 @param[in]		pszFilename		Filename to save to
	 @param[in]		pszExpOpt		A string containing the options used by the exporter
	 @param[in]		pszHistory		A string containing the history of the exported pod file
	 @brief     	Save a binary POD file (.POD).
	*****************************************************************************/
	EPVRTError SavePOD(const char * const pszFilename, const char * const pszExpOpt = 0, const char * const pszHistory = 0);

private:
	SPVRTPODImpl	*m_pImpl;	/*!< Internal implementation data */
};

/****************************************************************************
** Declarations
****************************************************************************/

/*!***************************************************************************
 @fn       		PVRTModelPODDataTypeSize
 @param[in]		type		Type to get the size of
 @return		Size of the data element
 @brief     	Returns the size of each data element.
*****************************************************************************/
PVRTuint32 PVRTModelPODDataTypeSize(const EPVRTDataType type);

/*!***************************************************************************
 @fn       		PVRTModelPODDataTypeComponentCount
 @param[in]		type		Type to get the number of components from
 @return		number of components in the data element
 @brief     	Returns the number of components in a data element.
*****************************************************************************/
PVRTuint32 PVRTModelPODDataTypeComponentCount(const EPVRTDataType type);

/*!***************************************************************************
 @fn       		PVRTModelPODDataStride
 @param[in]		data		Data elements
 @return		Size of the vector elements
 @brief     	Returns the size of the vector of data elements.
*****************************************************************************/
PVRTuint32 PVRTModelPODDataStride(const CPODData &data);

/*!***************************************************************************
 @fn       			PVRTModelPODGetAnimArraySize
 @param[in]			pAnimDataIdx
 @param[in]			ui32Frames
 @param[in]			ui32Components
 @return			Size of the animation array
 @brief     		Calculates the size of an animation array
*****************************************************************************/
PVRTuint32 PVRTModelPODGetAnimArraySize(PVRTuint32 *pAnimDataIdx, PVRTuint32 ui32Frames, PVRTuint32 ui32Components);

/*!***************************************************************************
 @fn       		PVRTModelPODScaleAndConvertVtxData
 @Modified		mesh		POD mesh to scale and convert the mesh data
 @param[in]		eNewType	The data type to scale and convert the vertex data to
 @return		PVR_SUCCESS on success and PVR_FAIL on failure.
 @brief     	Scales the vertex data to fit within the range of the requested
				data type and then converts the data to that type. This function
				isn't currently compiled in for fixed point builds of the tools.
*****************************************************************************/
#if !defined(PVRT_FIXED_POINT_ENABLE)
EPVRTError PVRTModelPODScaleAndConvertVtxData(SPODMesh &mesh, const EPVRTDataType eNewType);
#endif
/*!***************************************************************************
 @fn       		PVRTModelPODDataConvert
 @Modified		data		Data elements to convert
 @param[in]		eNewType	New type of elements
 @param[in]		nCnt		Number of elements
 @brief     	Convert the format of the array of vectors.
*****************************************************************************/
void PVRTModelPODDataConvert(CPODData &data, const unsigned int nCnt, const EPVRTDataType eNewType);

/*!***************************************************************************
 @fn       			PVRTModelPODDataShred
 @Modified			data		Data elements to modify
 @param[in]			nCnt		Number of elements
 @param[in]			pChannels	A list of the wanted channels, e.g. {'x', 'y', 0}
 @brief     		Reduce the number of dimensions in 'data' using the requested
					channel array. The array should have a maximum length of 4
					or be null terminated if less channels are wanted. Supported
					elements are 'x','y','z' and 'w'. They must be defined in lower
					case. It is also possible to negate an element, e.g. {'x','y', -'z'}.
*****************************************************************************/
void PVRTModelPODDataShred(CPODData &data, const unsigned int nCnt, const int *pChannels);

/*!***************************************************************************
 @fn       			PVRTModelPODReorderFaces
 @Modified			mesh		The mesh to re-order the faces of
 @param[in]			i32El1		The first index to be written out
 @param[in]			i32El2		The second index to be written out
 @param[in]			i32El3		The third index to be written out
 @brief     		Reorders the face indices of a mesh.
*****************************************************************************/
void PVRTModelPODReorderFaces(SPODMesh &mesh, const int i32El1, const int i32El2, const int i32El3);

/*!***************************************************************************
 @fn       		PVRTModelPODToggleInterleaved
 @Modified		mesh		Mesh to modify
 @param[in]		ui32AlignToNBytes Align the interleaved data to this no. of bytes.
 @brief     	Switches the supplied mesh to or from interleaved data format.
*****************************************************************************/
void PVRTModelPODToggleInterleaved(SPODMesh &mesh, unsigned int ui32AlignToNBytes = 1);

/*!***************************************************************************
 @fn       		PVRTModelPODDeIndex
 @Modified		mesh		Mesh to modify
 @brief     	De-indexes the supplied mesh. The mesh must be
				Interleaved before calling this function.
*****************************************************************************/
void PVRTModelPODDeIndex(SPODMesh &mesh);

/*!***************************************************************************
 @fn       		PVRTModelPODToggleStrips
 @Modified		mesh		Mesh to modify
 @brief     	Converts the supplied mesh to or from strips.
*****************************************************************************/
void PVRTModelPODToggleStrips(SPODMesh &mesh);

/*!***************************************************************************
 @fn       		PVRTModelPODCountIndices
 @param[in]		mesh		Mesh
 @return		Number of indices used by mesh
 @brief     	Counts the number of indices of a mesh
*****************************************************************************/
unsigned int PVRTModelPODCountIndices(const SPODMesh &mesh);

/*!***************************************************************************
 @fn       			PVRTModelPODCopyCPODData
 @param[in]			in
 @param[out]		out
 @param[in]			ui32No
 @param[in]			bInterleaved
 @brief     		Used to copy a CPODData of a mesh
*****************************************************************************/
void PVRTModelPODCopyCPODData(const CPODData &in, CPODData &out, unsigned int ui32No, bool bInterleaved);

/*!***************************************************************************
 @fn       			PVRTModelPODCopyNode
 @param[in]			in
 @param[out]		out
 @param[in]			nNumFrames The number of animation frames
 @brief     		Used to copy a pod node
*****************************************************************************/
void PVRTModelPODCopyNode(const SPODNode &in, SPODNode &out, int nNumFrames);

/*!***************************************************************************
 @fn       			PVRTModelPODCopyMesh
 @param[in]			in
 @param[out]		out
 @brief     		Used to copy a pod mesh
*****************************************************************************/
void PVRTModelPODCopyMesh(const SPODMesh &in, SPODMesh &out);

/*!***************************************************************************
 @fn       			PVRTModelPODCopyTexture
 @param[in]			in
 @param[out]		out
 @brief     		Used to copy a pod texture
*****************************************************************************/
void PVRTModelPODCopyTexture(const SPODTexture &in, SPODTexture &out);

/*!***************************************************************************
 @fn       			PVRTModelPODCopyMaterial
 @param[in]			in
 @param[out]		out
 @brief     		Used to copy a pod material
*****************************************************************************/
void PVRTModelPODCopyMaterial(const SPODMaterial &in, SPODMaterial &out);

/*!***************************************************************************
 @fn       			PVRTModelPODCopyCamera
 @param[in]			in
 @param[out]		out
 @param[in]			nNumFrames The number of animation frames
 @brief     		Used to copy a pod camera
*****************************************************************************/
void PVRTModelPODCopyCamera(const SPODCamera &in, SPODCamera &out, int nNumFrames);

/*!***************************************************************************
 @fn       			PVRTModelPODCopyLight
 @param[in]			in
 @param[out]		out
 @brief     		Used to copy a pod light
*****************************************************************************/
void PVRTModelPODCopyLight(const SPODLight &in, SPODLight &out);

/*!***************************************************************************
 @fn       			PVRTModelPODFlattenToWorldSpace
 @param[in]			in - Source scene. All meshes must not be interleaved.
 @param[out]		out
 @brief     		Used to flatten a pod scene to world space. All animation
					and skinning information will be removed. The returned
					position, normal, binormals and tangent data if present
					will be returned as floats regardless of the input data
					type.
*****************************************************************************/
EPVRTError PVRTModelPODFlattenToWorldSpace(CPVRTModelPOD &in, CPVRTModelPOD &out);


/*!***************************************************************************
 @fn       			PVRTModelPODMergeMaterials
 @param[in]			src - Source scene
 @param[out]		dst - Destination scene
 @brief     		This function takes two scenes and merges the textures,
					PFX effects and blending parameters from the src materials
					into the dst materials if they have the same material name.
*****************************************************************************/
EPVRTError PVRTModelPODMergeMaterials(const CPVRTModelPOD &src, CPVRTModelPOD &dst);

#endif /* _PVRTMODELPOD_H_ */

/*****************************************************************************
 End of file (PVRTModelPOD.h)
*****************************************************************************/

