/*!****************************************************************************

 @file         PVRTMatrix.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Vector and Matrix functions for floating and fixed point math. 
 @details      The general matrix format used is directly compatible with, for
               example, both DirectX and OpenGL.

******************************************************************************/
#ifndef _PVRTMATRIX_H_
#define _PVRTMATRIX_H_

#include "PVRTGlobal.h"
/****************************************************************************
** Defines
****************************************************************************/
#define MAT00 0
#define MAT01 1
#define MAT02 2
#define MAT03 3
#define MAT10 4
#define MAT11 5
#define MAT12 6
#define MAT13 7
#define MAT20 8
#define MAT21 9
#define MAT22 10
#define MAT23 11
#define MAT30 12
#define MAT31 13
#define MAT32 14
#define MAT33 15

/****************************************************************************
** Typedefs
****************************************************************************/
/*!***************************************************************************
 @brief     2D floating point vector
*****************************************************************************/
typedef struct
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
} PVRTVECTOR2f;

/*!***************************************************************************
 @brief     2D fixed point vector
*****************************************************************************/
typedef struct
{
	int x;	/*!< x coordinate */
	int y;	/*!< y coordinate */
} PVRTVECTOR2x;

/*!***************************************************************************
 @brief     3D floating point vector
*****************************************************************************/
typedef struct
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
	float z;	/*!< z coordinate */
} PVRTVECTOR3f;

/*!***************************************************************************
 @brief     3D fixed point vector
*****************************************************************************/
typedef struct
{
	int x;	/*!< x coordinate */
	int y;	/*!< y coordinate */
	int z;	/*!< z coordinate */
} PVRTVECTOR3x;

/*!***************************************************************************
 @brief     4D floating point vector
*****************************************************************************/
typedef struct
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
	float z;	/*!< z coordinate */
	float w;	/*!< w coordinate */
} PVRTVECTOR4f;

/*!***************************************************************************
 @brief     4D fixed point vector
*****************************************************************************/
typedef struct
{
	int x;	/*!< x coordinate */
	int y;	/*!< y coordinate */
	int z;	/*!< z coordinate */
	int w;	/*!< w coordinate */
} PVRTVECTOR4x;

/*!***************************************************************************
 @class     PVRTMATRIXf
 @brief     4x4 floating point matrix
*****************************************************************************/
class PVRTMATRIXf
{
public:
    float* operator [] ( const int Row )
	{
		return &f[Row<<2];
	}
	float f[16];	/*!< Array of float */
};

/*!***************************************************************************
 @class     PVRTMATRIXx
 @brief     4x4 fixed point matrix
*****************************************************************************/
class PVRTMATRIXx
{
public:
    int* operator [] ( const int Row )
	{
		return &f[Row<<2];
	}
	int f[16];
};

/*!***************************************************************************
 @class     PVRTMATRIX3f
 @brief     3x3 floating point matrix
*****************************************************************************/

class PVRTMATRIX3f
{
public:
    float* operator [] ( const int Row )
	{
		return &f[Row*3];
	}
	float f[9];	/*!< Array of float */
};

/*!***************************************************************************
 @class     PVRTMATRIX3x
 @brief     3x3 fixed point matrix
*****************************************************************************/
class PVRTMATRIX3x
{
public:
    int* operator [] ( const int Row )
	{
		return &f[Row*3];
	}
	int f[9];
};


/****************************************************************************
** Float or fixed
****************************************************************************/
#ifdef PVRT_FIXED_POINT_ENABLE
	typedef PVRTVECTOR2x		PVRTVECTOR2;
	typedef PVRTVECTOR3x		PVRTVECTOR3;
	typedef PVRTVECTOR4x		PVRTVECTOR4;
	typedef PVRTMATRIX3x		PVRTMATRIX3;
	typedef PVRTMATRIXx			PVRTMATRIX;
	#define PVRTMatrixIdentity					PVRTMatrixIdentityX
	#define PVRTMatrixMultiply					PVRTMatrixMultiplyX
	#define PVRTMatrixTranslation				PVRTMatrixTranslationX
	#define PVRTMatrixScaling					PVRTMatrixScalingX
	#define PVRTMatrixRotationX					PVRTMatrixRotationXX
	#define PVRTMatrixRotationY					PVRTMatrixRotationYX
	#define PVRTMatrixRotationZ					PVRTMatrixRotationZX
	#define PVRTMatrixTranspose					PVRTMatrixTransposeX
	#define PVRTMatrixInverse					PVRTMatrixInverseX
	#define PVRTMatrixInverseEx					PVRTMatrixInverseExX
	#define PVRTMatrixLookAtLH					PVRTMatrixLookAtLHX
	#define PVRTMatrixLookAtRH					PVRTMatrixLookAtRHX
	#define PVRTMatrixPerspectiveFovLH			PVRTMatrixPerspectiveFovLHX
	#define PVRTMatrixPerspectiveFovRH			PVRTMatrixPerspectiveFovRHX
	#define PVRTMatrixOrthoLH					PVRTMatrixOrthoLHX
	#define PVRTMatrixOrthoRH					PVRTMatrixOrthoRHX
	#define PVRTMatrixVec3Lerp					PVRTMatrixVec3LerpX
	#define PVRTMatrixVec3DotProduct			PVRTMatrixVec3DotProductX
	#define PVRTMatrixVec3CrossProduct			PVRTMatrixVec3CrossProductX
	#define PVRTMatrixVec3Normalize				PVRTMatrixVec3NormalizeX
	#define PVRTMatrixVec3Length				PVRTMatrixVec3LengthX
	#define PVRTMatrixLinearEqSolve				PVRTMatrixLinearEqSolveX
#else
	typedef PVRTVECTOR2f		PVRTVECTOR2;
	typedef PVRTVECTOR3f		PVRTVECTOR3;
	typedef PVRTVECTOR4f		PVRTVECTOR4;
	typedef PVRTMATRIX3f		PVRTMATRIX3;
	typedef PVRTMATRIXf			PVRTMATRIX;
	#define PVRTMatrixIdentity					PVRTMatrixIdentityF
	#define PVRTMatrixMultiply					PVRTMatrixMultiplyF
	#define PVRTMatrixTranslation				PVRTMatrixTranslationF
	#define PVRTMatrixScaling					PVRTMatrixScalingF
	#define PVRTMatrixRotationX					PVRTMatrixRotationXF
	#define PVRTMatrixRotationY					PVRTMatrixRotationYF
	#define PVRTMatrixRotationZ					PVRTMatrixRotationZF
	#define PVRTMatrixTranspose					PVRTMatrixTransposeF
	#define PVRTMatrixInverse					PVRTMatrixInverseF
	#define PVRTMatrixInverseEx					PVRTMatrixInverseExF
	#define PVRTMatrixLookAtLH					PVRTMatrixLookAtLHF
	#define PVRTMatrixLookAtRH					PVRTMatrixLookAtRHF
	#define PVRTMatrixPerspectiveFovLH			PVRTMatrixPerspectiveFovLHF
	#define PVRTMatrixPerspectiveFovRH			PVRTMatrixPerspectiveFovRHF
	#define PVRTMatrixOrthoLH					PVRTMatrixOrthoLHF
	#define PVRTMatrixOrthoRH					PVRTMatrixOrthoRHF
	#define PVRTMatrixVec3Lerp					PVRTMatrixVec3LerpF
	#define PVRTMatrixVec3DotProduct			PVRTMatrixVec3DotProductF
	#define PVRTMatrixVec3CrossProduct			PVRTMatrixVec3CrossProductF
	#define PVRTMatrixVec3Normalize				PVRTMatrixVec3NormalizeF
	#define PVRTMatrixVec3Length				PVRTMatrixVec3LengthF
	#define PVRTMatrixLinearEqSolve				PVRTMatrixLinearEqSolveF
#endif

/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @fn      			PVRTMatrixIdentityF
 @param[out]			mOut	Set to identity
 @brief      		Reset matrix to identity matrix.
*****************************************************************************/
void PVRTMatrixIdentityF(PVRTMATRIXf &mOut);

/*!***************************************************************************
 @fn      			PVRTMatrixIdentityX
 @param[out]			mOut	Set to identity
 @brief      		Reset matrix to identity matrix.
*****************************************************************************/
void PVRTMatrixIdentityX(PVRTMATRIXx &mOut);

/*!***************************************************************************
 @fn      			PVRTMatrixMultiplyF
 @param[out]			mOut	Result of mA x mB
 @param[in]				mA		First operand
 @param[in]				mB		Second operand
 @brief      		Multiply mA by mB and assign the result to mOut
					(mOut = p1 * p2). A copy of the result matrix is done in
					the function because mOut can be a parameter mA or mB.
*****************************************************************************/
void PVRTMatrixMultiplyF(
	PVRTMATRIXf			&mOut,
	const PVRTMATRIXf	&mA,
	const PVRTMATRIXf	&mB);
/*!***************************************************************************
 @fn      			PVRTMatrixMultiplyX
 @param[out]			mOut	Result of mA x mB
 @param[in]				mA		First operand
 @param[in]				mB		Second operand
 @brief      		Multiply mA by mB and assign the result to mOut
					(mOut = p1 * p2). A copy of the result matrix is done in
					the function because mOut can be a parameter mA or mB.
					The fixed-point shift could be performed after adding
					all four intermediate results together however this might
					cause some overflow issues.
*****************************************************************************/
void PVRTMatrixMultiplyX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mA,
	const PVRTMATRIXx	&mB);

/*!***************************************************************************
 @fn           		PVRTMatrixTranslationF
 @param[out]			mOut	Translation matrix
 @param[in]				fX		X component of the translation
 @param[in]				fY		Y component of the translation
 @param[in]				fZ		Z component of the translation
 @brief      		Build a transaltion matrix mOut using fX, fY and fZ.
*****************************************************************************/
void PVRTMatrixTranslationF(
	PVRTMATRIXf	&mOut,
	const float	fX,
	const float	fY,
	const float	fZ);
/*!***************************************************************************
 @fn        		PVRTMatrixTranslationX
 @param[out]			mOut	Translation matrix
 @param[in]				fX		X component of the translation
 @param[in]				fY		Y component of the translation
 @param[in]				fZ		Z component of the translation
 @brief      		Build a transaltion matrix mOut using fX, fY and fZ.
*****************************************************************************/
void PVRTMatrixTranslationX(
	PVRTMATRIXx	&mOut,
	const int	fX,
	const int	fY,
	const int	fZ);

/*!***************************************************************************
 @fn           		PVRTMatrixScalingF
 @param[out]			mOut	Scale matrix
 @param[in]				fX		X component of the scaling
 @param[in]				fY		Y component of the scaling
 @param[in]				fZ		Z component of the scaling
 @brief      		Build a scale matrix mOut using fX, fY and fZ.
*****************************************************************************/
void PVRTMatrixScalingF(
	PVRTMATRIXf	&mOut,
	const float fX,
	const float fY,
	const float fZ);

/*!***************************************************************************
 @fn           		PVRTMatrixScalingX
 @param[out]			mOut	Scale matrix
 @param[in]				fX		X component of the scaling
 @param[in]				fY		Y component of the scaling
 @param[in]				fZ		Z component of the scaling
 @brief      		Build a scale matrix mOut using fX, fY and fZ.
*****************************************************************************/
void PVRTMatrixScalingX(
	PVRTMATRIXx	&mOut,
	const int	fX,
	const int	fY,
	const int	fZ);

/*!***************************************************************************
 @fn           		PVRTMatrixRotationXF
 @param[out]			mOut	Rotation matrix
 @param[in]				fAngle	Angle of the rotation
 @brief      		Create an X rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationXF(
	PVRTMATRIXf	&mOut,
	const float fAngle);

/*!***************************************************************************
 @fn           		PVRTMatrixRotationXX
 @param[out]			mOut	Rotation matrix
 @param[in]				fAngle	Angle of the rotation
 @brief      		Create an X rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationXX(
	PVRTMATRIXx	&mOut,
	const int	fAngle);

/*!***************************************************************************
 @fn           		PVRTMatrixRotationYF
 @param[out]			mOut	Rotation matrix
 @param[in]				fAngle	Angle of the rotation
 @brief      		Create an Y rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationYF(
	PVRTMATRIXf	&mOut,
	const float fAngle);

/*!***************************************************************************
 @fn           		PVRTMatrixRotationYX
 @param[out]			mOut	Rotation matrix
 @param[in]				fAngle	Angle of the rotation
 @brief      		Create an Y rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationYX(
	PVRTMATRIXx	&mOut,
	const int	fAngle);

/*!***************************************************************************
 @fn           		PVRTMatrixRotationZF
 @param[out]			mOut	Rotation matrix
 @param[in]				fAngle	Angle of the rotation
 @brief      		Create an Z rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationZF(
	PVRTMATRIXf	&mOut,
	const float fAngle);
/*!***************************************************************************
 @fn           		PVRTMatrixRotationZX
 @param[out]			mOut	Rotation matrix
 @param[in]				fAngle	Angle of the rotation
 @brief      		Create an Z rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationZX(
	PVRTMATRIXx	&mOut,
	const int	fAngle);

/*!***************************************************************************
 @fn           		PVRTMatrixTransposeF
 @param[out]			mOut	Transposed matrix
 @param[in]				mIn		Original matrix
 @brief      		Compute the transpose matrix of mIn.
*****************************************************************************/
void PVRTMatrixTransposeF(
	PVRTMATRIXf			&mOut,
	const PVRTMATRIXf	&mIn);
/*!***************************************************************************
 @fn           		PVRTMatrixTransposeX
 @param[out]			mOut	Transposed matrix
 @param[in]				mIn		Original matrix
 @brief      		Compute the transpose matrix of mIn.
*****************************************************************************/
void PVRTMatrixTransposeX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mIn);

/*!***************************************************************************
 @fn      			PVRTMatrixInverseF
 @param[out]			mOut	Inversed matrix
 @param[in]				mIn		Original matrix
 @brief      		Compute the inverse matrix of mIn.
					The matrix must be of the form :
					A 0
					C 1
					Where A is a 3x3 matrix and C is a 1x3 matrix.
*****************************************************************************/
void PVRTMatrixInverseF(
	PVRTMATRIXf			&mOut,
	const PVRTMATRIXf	&mIn);
/*!***************************************************************************
 @fn      			PVRTMatrixInverseX
 @param[out]			mOut	Inversed matrix
 @param[in]				mIn		Original matrix
 @brief      		Compute the inverse matrix of mIn.
					The matrix must be of the form :
					A 0
					C 1
					Where A is a 3x3 matrix and C is a 1x3 matrix.
*****************************************************************************/
void PVRTMatrixInverseX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mIn);

/*!***************************************************************************
 @fn      			PVRTMatrixInverseExF
 @param[out]			mOut	Inversed matrix
 @param[in]				mIn		Original matrix
 @brief      		Compute the inverse matrix of mIn.
					Uses a linear equation solver and the knowledge that M.M^-1=I.
					Use this fn to calculate the inverse of matrices that
					PVRTMatrixInverse() cannot.
*****************************************************************************/
void PVRTMatrixInverseExF(
	PVRTMATRIXf			&mOut,
	const PVRTMATRIXf	&mIn);
/*!***************************************************************************
 @fn      			PVRTMatrixInverseExX
 @param[out]			mOut	Inversed matrix
 @param[in]				mIn		Original matrix
 @brief      		Compute the inverse matrix of mIn.
					Uses a linear equation solver and the knowledge that M.M^-1=I.
					Use this fn to calculate the inverse of matrices that
					PVRTMatrixInverse() cannot.
*****************************************************************************/
void PVRTMatrixInverseExX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mIn);

/*!***************************************************************************
 @fn      			PVRTMatrixLookAtLHF
 @param[out]			mOut	Look-at view matrix
 @param[in]				vEye	Position of the camera
 @param[in]				vAt		Point the camera is looking at
 @param[in]				vUp		Up direction for the camera
 @brief      		Create a look-at view matrix.
*****************************************************************************/
void PVRTMatrixLookAtLHF(
	PVRTMATRIXf			&mOut,
	const PVRTVECTOR3f	&vEye,
	const PVRTVECTOR3f	&vAt,
	const PVRTVECTOR3f	&vUp);
/*!***************************************************************************
 @fn      			PVRTMatrixLookAtLHX
 @param[out]			mOut	Look-at view matrix
 @param[in]				vEye	Position of the camera
 @param[in]				vAt		Point the camera is looking at
 @param[in]				vUp		Up direction for the camera
 @brief      		Create a look-at view matrix.
*****************************************************************************/
void PVRTMatrixLookAtLHX(
	PVRTMATRIXx			&mOut,
	const PVRTVECTOR3x	&vEye,
	const PVRTVECTOR3x	&vAt,
	const PVRTVECTOR3x	&vUp);

/*!***************************************************************************
 @fn      			PVRTMatrixLookAtRHF
 @param[out]			mOut	Look-at view matrix
 @param[in]				vEye	Position of the camera
 @param[in]				vAt		Point the camera is looking at
 @param[in]				vUp		Up direction for the camera
 @brief      		Create a look-at view matrix.
*****************************************************************************/
void PVRTMatrixLookAtRHF(
	PVRTMATRIXf			&mOut,
	const PVRTVECTOR3f	&vEye,
	const PVRTVECTOR3f	&vAt,
	const PVRTVECTOR3f	&vUp);
/*!***************************************************************************
 @fn      			PVRTMatrixLookAtRHX
 @param[out]			mOut	Look-at view matrix
 @param[in]				vEye	Position of the camera
 @param[in]				vAt		Point the camera is looking at
 @param[in]				vUp		Up direction for the camera
 @brief      		Create a look-at view matrix.
*****************************************************************************/
void PVRTMatrixLookAtRHX(
	PVRTMATRIXx			&mOut,
	const PVRTVECTOR3x	&vEye,
	const PVRTVECTOR3x	&vAt,
	const PVRTVECTOR3x	&vUp);

/*!***************************************************************************
 @fn      		PVRTMatrixPerspectiveFovLHF
 @param[out]		mOut		Perspective matrix
 @param[in]			fFOVy		Field of view
 @param[in]			fAspect		Aspect ratio
 @param[in]			fNear		Near clipping distance
 @param[in]			fFar		Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create a perspective matrix.
*****************************************************************************/
void PVRTMatrixPerspectiveFovLHF(
	PVRTMATRIXf	&mOut,
	const float	fFOVy,
	const float	fAspect,
	const float	fNear,
	const float	fFar,
	const bool  bRotate = false);
/*!***************************************************************************
 @fn      		PVRTMatrixPerspectiveFovLHX
 @param[out]		mOut		Perspective matrix
 @param[in]			fFOVy		Field of view
 @param[in]			fAspect		Aspect ratio
 @param[in]			fNear		Near clipping distance
 @param[in]			fFar		Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create a perspective matrix.
*****************************************************************************/
void PVRTMatrixPerspectiveFovLHX(
	PVRTMATRIXx	&mOut,
	const int	fFOVy,
	const int	fAspect,
	const int	fNear,
	const int	fFar,
	const bool  bRotate = false);

/*!***************************************************************************
 @fn      		PVRTMatrixPerspectiveFovRHF
 @param[out]		mOut		Perspective matrix
 @param[in]			fFOVy		Field of view
 @param[in]			fAspect		Aspect ratio
 @param[in]			fNear		Near clipping distance
 @param[in]			fFar		Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create a perspective matrix.
*****************************************************************************/
void PVRTMatrixPerspectiveFovRHF(
	PVRTMATRIXf	&mOut,
	const float	fFOVy,
	const float	fAspect,
	const float	fNear,
	const float	fFar,
	const bool  bRotate = false);
/*!***************************************************************************
 @fn      		PVRTMatrixPerspectiveFovRHX
 @param[out]		mOut		Perspective matrix
 @param[in]			fFOVy		Field of view
 @param[in]			fAspect		Aspect ratio
 @param[in]			fNear		Near clipping distance
 @param[in]			fFar		Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create a perspective matrix.
*****************************************************************************/
void PVRTMatrixPerspectiveFovRHX(
	PVRTMATRIXx	&mOut,
	const int	fFOVy,
	const int	fAspect,
	const int	fNear,
	const int	fFar,
	const bool  bRotate = false);

/*!***************************************************************************
 @fn      		PVRTMatrixOrthoLHF
 @param[out]		mOut		Orthographic matrix
 @param[in]			w			Width of the screen
 @param[in]			h			Height of the screen
 @param[in]			zn			Near clipping distance
 @param[in]			zf			Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create an orthographic matrix.
*****************************************************************************/
void PVRTMatrixOrthoLHF(
	PVRTMATRIXf	&mOut,
	const float w,
	const float h,
	const float zn,
	const float zf,
	const bool  bRotate = false);
/*!***************************************************************************
 @fn      		PVRTMatrixOrthoLHX
 @param[out]		mOut		Orthographic matrix
 @param[in]			w			Width of the screen
 @param[in]			h			Height of the screen
 @param[in]			zn			Near clipping distance
 @param[in]			zf			Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create an orthographic matrix.
*****************************************************************************/
void PVRTMatrixOrthoLHX(
	PVRTMATRIXx	&mOut,
	const int	w,
	const int	h,
	const int	zn,
	const int	zf,
	const bool  bRotate = false);

/*!***************************************************************************
 @fn      		PVRTMatrixOrthoRHF
 @param[out]		mOut		Orthographic matrix
 @param[in]			w			Width of the screen
 @param[in]			h			Height of the screen
 @param[in]			zn			Near clipping distance
 @param[in]			zf			Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create an orthographic matrix.
*****************************************************************************/
void PVRTMatrixOrthoRHF(
	PVRTMATRIXf	&mOut,
	const float w,
	const float h,
	const float zn,
	const float zf,
	const bool  bRotate = false);
/*!***************************************************************************
 @fn      		PVRTMatrixOrthoRHX
 @param[out]		mOut		Orthographic matrix
 @param[in]			w			Width of the screen
 @param[in]			h			Height of the screen
 @param[in]			zn			Near clipping distance
 @param[in]			zf			Far clipping distance
 @param[in]			bRotate		Should we rotate it ? (for upright screens)
 @brief      	Create an orthographic matrix.
*****************************************************************************/
void PVRTMatrixOrthoRHX(
	PVRTMATRIXx	&mOut,
	const int	w,
	const int	h,
	const int	zn,
	const int	zf,
	const bool  bRotate = false);

/*!***************************************************************************
 @fn      			PVRTMatrixVec3LerpF
 @param[out]			vOut	Result of the interpolation
 @param[in]				v1		First vector to interpolate from
 @param[in]				v2		Second vector to interpolate form
 @param[in]				s		Coefficient of interpolation
 @brief      		This function performs the linear interpolation based on
					the following formula: V1 + s(V2-V1).
*****************************************************************************/
void PVRTMatrixVec3LerpF(
	PVRTVECTOR3f		&vOut,
	const PVRTVECTOR3f	&v1,
	const PVRTVECTOR3f	&v2,
	const float			s);
/*!***************************************************************************
 @fn      			PVRTMatrixVec3LerpX
 @param[out]			vOut	Result of the interpolation
 @param[in]				v1		First vector to interpolate from
 @param[in]				v2		Second vector to interpolate form
 @param[in]				s		Coefficient of interpolation
 @brief      		This function performs the linear interpolation based on
					the following formula: V1 + s(V2-V1).
*****************************************************************************/
void PVRTMatrixVec3LerpX(
	PVRTVECTOR3x		&vOut,
	const PVRTVECTOR3x	&v1,
	const PVRTVECTOR3x	&v2,
	const int			s);

/*!***************************************************************************
 @fn      			PVRTMatrixVec3DotProductF
 @param[in]				v1		First vector
 @param[in]				v2		Second vector
 @return			Dot product of the two vectors.
 @brief      		This function performs the dot product of the two
					supplied vectors.
*****************************************************************************/
float PVRTMatrixVec3DotProductF(
	const PVRTVECTOR3f	&v1,
	const PVRTVECTOR3f	&v2);
/*!***************************************************************************
 @fn      			PVRTMatrixVec3DotProductX
 @param[in]				v1		First vector
 @param[in]				v2		Second vector
 @return			Dot product of the two vectors.
 @brief      		This function performs the dot product of the two
					supplied vectors.
					A single >> 16 shift could be applied to the final accumulated
					result however this runs the risk of overflow between the
					results of the intermediate additions.
*****************************************************************************/
int PVRTMatrixVec3DotProductX(
	const PVRTVECTOR3x	&v1,
	const PVRTVECTOR3x	&v2);

/*!***************************************************************************
 @fn      			PVRTMatrixVec3CrossProductF
 @param[out]			vOut	Cross product of the two vectors
 @param[in]				v1		First vector
 @param[in]				v2		Second vector
 @brief      		This function performs the cross product of the two
					supplied vectors.
*****************************************************************************/
void PVRTMatrixVec3CrossProductF(
	PVRTVECTOR3f		&vOut,
	const PVRTVECTOR3f	&v1,
	const PVRTVECTOR3f	&v2);
/*!***************************************************************************
 @fn      			PVRTMatrixVec3CrossProductX
 @param[out]			vOut	Cross product of the two vectors
 @param[in]				v1		First vector
 @param[in]				v2		Second vector
 @brief      		This function performs the cross product of the two
					supplied vectors.
*****************************************************************************/
void PVRTMatrixVec3CrossProductX(
	PVRTVECTOR3x		&vOut,
	const PVRTVECTOR3x	&v1,
	const PVRTVECTOR3x	&v2);

/*!***************************************************************************
 @fn      			PVRTMatrixVec3NormalizeF
 @param[out]			vOut	Normalized vector
 @param[in]				vIn		Vector to normalize
 @brief      		Normalizes the supplied vector.
*****************************************************************************/
void PVRTMatrixVec3NormalizeF(
	PVRTVECTOR3f		&vOut,
	const PVRTVECTOR3f	&vIn);
/*!***************************************************************************
 @fn      			PVRTMatrixVec3NormalizeX
 @param[out]			vOut	Normalized vector
 @param[in]				vIn		Vector to normalize
 @brief      		Normalizes the supplied vector.
					The square root function is currently still performed
					in floating-point.
					Original vector is scaled down prior to be normalized in
					order to avoid overflow issues.
*****************************************************************************/
void PVRTMatrixVec3NormalizeX(
	PVRTVECTOR3x		&vOut,
	const PVRTVECTOR3x	&vIn);
/*!***************************************************************************
 @fn      			PVRTMatrixVec3LengthF
 @param[in]				vIn		Vector to get the length of
 @return			The length of the vector
  @brief      		Gets the length of the supplied vector.
*****************************************************************************/
float PVRTMatrixVec3LengthF(
	const PVRTVECTOR3f	&vIn);
/*!***************************************************************************
 @fn      			PVRTMatrixVec3LengthX
 @param[in]				vIn		Vector to get the length of
 @return			The length of the vector
 @brief      		Gets the length of the supplied vector
*****************************************************************************/
int PVRTMatrixVec3LengthX(
	const PVRTVECTOR3x	&vIn);
/*!***************************************************************************
 @fn      			PVRTMatrixLinearEqSolveF
 @param[in]				pSrc	2D array of floats. 4 Eq linear problem is 5x4
							matrix, constants in first column
 @param[in]				nCnt	Number of equations to solve
 @param[out]			pRes	Result
 @brief      		Solves 'nCnt' simultaneous equations of 'nCnt' variables.
					pRes should be an array large enough to contain the
					results: the values of the 'nCnt' variables.
					This fn recursively uses Gaussian Elimination.
*****************************************************************************/

void PVRTMatrixLinearEqSolveF(
	float		* const pRes,
	float		** const pSrc,
	const int	nCnt);
/*!***************************************************************************
 @fn      			PVRTMatrixLinearEqSolveX
 @param[in]				pSrc	2D array of floats. 4 Eq linear problem is 5x4
							matrix, constants in first column
 @param[in]				nCnt	Number of equations to solve
 @param[out]			pRes	Result
 @brief      		Solves 'nCnt' simultaneous equations of 'nCnt' variables.
					pRes should be an array large enough to contain the
					results: the values of the 'nCnt' variables.
					This fn recursively uses Gaussian Elimination.
*****************************************************************************/
void PVRTMatrixLinearEqSolveX(
	int			* const pRes,
	int			** const pSrc,
	const int	nCnt);

#endif

/*****************************************************************************
 End of file (PVRTMatrix.h)
*****************************************************************************/

