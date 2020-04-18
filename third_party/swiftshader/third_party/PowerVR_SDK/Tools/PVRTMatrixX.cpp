/******************************************************************************

 @File         PVRTMatrixX.cpp

 @Title        PVRTMatrixX

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Set of mathematical functions involving matrices, vectors and
               quaternions.
               The general matrix format used is directly compatible with, for example,
               both DirectX and OpenGL. For the reasons why, read this:
               http://research.microsoft.com/~hollasch/cgindex/math/matrix/column-vec.html

******************************************************************************/

#include "PVRTContext.h"
#include <math.h>
#include <string.h>

#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"



/****************************************************************************
** Constants
****************************************************************************/
static const PVRTMATRIXx	c_mIdentity = {
	{
	PVRTF2X(1.0f), PVRTF2X(0.0f), PVRTF2X(0.0f), PVRTF2X(0.0f),
	PVRTF2X(0.0f), PVRTF2X(1.0f), PVRTF2X(0.0f), PVRTF2X(0.0f),
	PVRTF2X(0.0f), PVRTF2X(0.0f), PVRTF2X(1.0f), PVRTF2X(0.0f),
	PVRTF2X(0.0f), PVRTF2X(0.0f), PVRTF2X(0.0f), PVRTF2X(1.0f)
	}
};


/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @Function			PVRTMatrixIdentityX
 @Output			mOut	Set to identity
 @Description		Reset matrix to identity matrix.
*****************************************************************************/
void PVRTMatrixIdentityX(PVRTMATRIXx &mOut)
{
	mOut.f[ 0]=PVRTF2X(1.0f);	mOut.f[ 4]=PVRTF2X(0.0f);	mOut.f[ 8]=PVRTF2X(0.0f);	mOut.f[12]=PVRTF2X(0.0f);
	mOut.f[ 1]=PVRTF2X(0.0f);	mOut.f[ 5]=PVRTF2X(1.0f);	mOut.f[ 9]=PVRTF2X(0.0f);	mOut.f[13]=PVRTF2X(0.0f);
	mOut.f[ 2]=PVRTF2X(0.0f);	mOut.f[ 6]=PVRTF2X(0.0f);	mOut.f[10]=PVRTF2X(1.0f);	mOut.f[14]=PVRTF2X(0.0f);
	mOut.f[ 3]=PVRTF2X(0.0f);	mOut.f[ 7]=PVRTF2X(0.0f);	mOut.f[11]=PVRTF2X(0.0f);	mOut.f[15]=PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function			PVRTMatrixMultiplyX
 @Output			mOut	Result of mA x mB
 @Input				mA		First operand
 @Input				mB		Second operand
 @Description		Multiply mA by mB and assign the result to mOut
					(mOut = p1 * p2). A copy of the result matrix is done in
					the function because mOut can be a parameter mA or mB.
					The fixed-point shift could be performed after adding
					all four intermediate results together however this might
					cause some overflow issues.
****************************************************************************/
void PVRTMatrixMultiplyX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mA,
	const PVRTMATRIXx	&mB)
{
	PVRTMATRIXx mRet;

	/* Perform calculation on a dummy matrix (mRet) */
	mRet.f[ 0] = PVRTXMUL(mA.f[ 0], mB.f[ 0]) + PVRTXMUL(mA.f[ 1], mB.f[ 4]) + PVRTXMUL(mA.f[ 2], mB.f[ 8]) + PVRTXMUL(mA.f[ 3], mB.f[12]);
	mRet.f[ 1] = PVRTXMUL(mA.f[ 0], mB.f[ 1]) + PVRTXMUL(mA.f[ 1], mB.f[ 5]) + PVRTXMUL(mA.f[ 2], mB.f[ 9]) + PVRTXMUL(mA.f[ 3], mB.f[13]);
	mRet.f[ 2] = PVRTXMUL(mA.f[ 0], mB.f[ 2]) + PVRTXMUL(mA.f[ 1], mB.f[ 6]) + PVRTXMUL(mA.f[ 2], mB.f[10]) + PVRTXMUL(mA.f[ 3], mB.f[14]);
	mRet.f[ 3] = PVRTXMUL(mA.f[ 0], mB.f[ 3]) + PVRTXMUL(mA.f[ 1], mB.f[ 7]) + PVRTXMUL(mA.f[ 2], mB.f[11]) + PVRTXMUL(mA.f[ 3], mB.f[15]);

	mRet.f[ 4] = PVRTXMUL(mA.f[ 4], mB.f[ 0]) + PVRTXMUL(mA.f[ 5], mB.f[ 4]) + PVRTXMUL(mA.f[ 6], mB.f[ 8]) + PVRTXMUL(mA.f[ 7], mB.f[12]);
	mRet.f[ 5] = PVRTXMUL(mA.f[ 4], mB.f[ 1]) + PVRTXMUL(mA.f[ 5], mB.f[ 5]) + PVRTXMUL(mA.f[ 6], mB.f[ 9]) + PVRTXMUL(mA.f[ 7], mB.f[13]);
	mRet.f[ 6] = PVRTXMUL(mA.f[ 4], mB.f[ 2]) + PVRTXMUL(mA.f[ 5], mB.f[ 6]) + PVRTXMUL(mA.f[ 6], mB.f[10]) + PVRTXMUL(mA.f[ 7], mB.f[14]);
	mRet.f[ 7] = PVRTXMUL(mA.f[ 4], mB.f[ 3]) + PVRTXMUL(mA.f[ 5], mB.f[ 7]) + PVRTXMUL(mA.f[ 6], mB.f[11]) + PVRTXMUL(mA.f[ 7], mB.f[15]);

	mRet.f[ 8] = PVRTXMUL(mA.f[ 8], mB.f[ 0]) + PVRTXMUL(mA.f[ 9], mB.f[ 4]) + PVRTXMUL(mA.f[10], mB.f[ 8]) + PVRTXMUL(mA.f[11], mB.f[12]);
	mRet.f[ 9] = PVRTXMUL(mA.f[ 8], mB.f[ 1]) + PVRTXMUL(mA.f[ 9], mB.f[ 5]) + PVRTXMUL(mA.f[10], mB.f[ 9]) + PVRTXMUL(mA.f[11], mB.f[13]);
	mRet.f[10] = PVRTXMUL(mA.f[ 8], mB.f[ 2]) + PVRTXMUL(mA.f[ 9], mB.f[ 6]) + PVRTXMUL(mA.f[10], mB.f[10]) + PVRTXMUL(mA.f[11], mB.f[14]);
	mRet.f[11] = PVRTXMUL(mA.f[ 8], mB.f[ 3]) + PVRTXMUL(mA.f[ 9], mB.f[ 7]) + PVRTXMUL(mA.f[10], mB.f[11]) + PVRTXMUL(mA.f[11], mB.f[15]);

	mRet.f[12] = PVRTXMUL(mA.f[12], mB.f[ 0]) + PVRTXMUL(mA.f[13], mB.f[ 4]) + PVRTXMUL(mA.f[14], mB.f[ 8]) + PVRTXMUL(mA.f[15], mB.f[12]);
	mRet.f[13] = PVRTXMUL(mA.f[12], mB.f[ 1]) + PVRTXMUL(mA.f[13], mB.f[ 5]) + PVRTXMUL(mA.f[14], mB.f[ 9]) + PVRTXMUL(mA.f[15], mB.f[13]);
	mRet.f[14] = PVRTXMUL(mA.f[12], mB.f[ 2]) + PVRTXMUL(mA.f[13], mB.f[ 6]) + PVRTXMUL(mA.f[14], mB.f[10]) + PVRTXMUL(mA.f[15], mB.f[14]);
	mRet.f[15] = PVRTXMUL(mA.f[12], mB.f[ 3]) + PVRTXMUL(mA.f[13], mB.f[ 7]) + PVRTXMUL(mA.f[14], mB.f[11]) + PVRTXMUL(mA.f[15], mB.f[15]);

	/* Copy result in pResultMatrix */
	mOut = mRet;
}

/*!***************************************************************************
 @Function Name		PVRTMatrixTranslationX
 @Output			mOut	Translation matrix
 @Input				fX		X component of the translation
 @Input				fY		Y component of the translation
 @Input				fZ		Z component of the translation
 @Description		Build a transaltion matrix mOut using fX, fY and fZ.
*****************************************************************************/
void PVRTMatrixTranslationX(
	PVRTMATRIXx	&mOut,
	const int	fX,
	const int	fY,
	const int	fZ)
{
	mOut.f[ 0]=PVRTF2X(1.0f);	mOut.f[ 4]=PVRTF2X(0.0f);	mOut.f[ 8]=PVRTF2X(0.0f);	mOut.f[12]=fX;
	mOut.f[ 1]=PVRTF2X(0.0f);	mOut.f[ 5]=PVRTF2X(1.0f);	mOut.f[ 9]=PVRTF2X(0.0f);	mOut.f[13]=fY;
	mOut.f[ 2]=PVRTF2X(0.0f);	mOut.f[ 6]=PVRTF2X(0.0f);	mOut.f[10]=PVRTF2X(1.0f);	mOut.f[14]=fZ;
	mOut.f[ 3]=PVRTF2X(0.0f);	mOut.f[ 7]=PVRTF2X(0.0f);	mOut.f[11]=PVRTF2X(0.0f);	mOut.f[15]=PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function Name		PVRTMatrixScalingX
 @Output			mOut	Scale matrix
 @Input				fX		X component of the scaling
 @Input				fY		Y component of the scaling
 @Input				fZ		Z component of the scaling
 @Description		Build a scale matrix mOut using fX, fY and fZ.
*****************************************************************************/
void PVRTMatrixScalingX(
	PVRTMATRIXx	&mOut,
	const int	fX,
	const int	fY,
	const int	fZ)
{
	mOut.f[ 0]=fX;				mOut.f[ 4]=PVRTF2X(0.0f);	mOut.f[ 8]=PVRTF2X(0.0f);	mOut.f[12]=PVRTF2X(0.0f);
	mOut.f[ 1]=PVRTF2X(0.0f);	mOut.f[ 5]=fY;				mOut.f[ 9]=PVRTF2X(0.0f);	mOut.f[13]=PVRTF2X(0.0f);
	mOut.f[ 2]=PVRTF2X(0.0f);	mOut.f[ 6]=PVRTF2X(0.0f);	mOut.f[10]=fZ;				mOut.f[14]=PVRTF2X(0.0f);
	mOut.f[ 3]=PVRTF2X(0.0f);	mOut.f[ 7]=PVRTF2X(0.0f);	mOut.f[11]=PVRTF2X(0.0f);	mOut.f[15]=PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function Name		PVRTMatrixRotationXX
 @Output			mOut	Rotation matrix
 @Input				fAngle	Angle of the rotation
 @Description		Create an X rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationXX(
	PVRTMATRIXx	&mOut,
	const int	fAngle)
{
	int		fCosine, fSine;

    /* Precompute cos and sin */
#if defined(BUILD_DX11)
	fCosine	= PVRTXCOS(-fAngle);
    fSine	= PVRTXSIN(-fAngle);
#else
	fCosine	= PVRTXCOS(fAngle);
    fSine	= PVRTXSIN(fAngle);
#endif

	/* Create the trigonometric matrix corresponding to X Rotation */
	mOut.f[ 0]=PVRTF2X(1.0f);	mOut.f[ 4]=PVRTF2X(0.0f);		mOut.f[ 8]=PVRTF2X(0.0f);		mOut.f[12]=PVRTF2X(0.0f);
	mOut.f[ 1]=PVRTF2X(0.0f);	mOut.f[ 5]=fCosine;				mOut.f[ 9]=fSine;				mOut.f[13]=PVRTF2X(0.0f);
	mOut.f[ 2]=PVRTF2X(0.0f);	mOut.f[ 6]=-fSine;				mOut.f[10]=fCosine;				mOut.f[14]=PVRTF2X(0.0f);
	mOut.f[ 3]=PVRTF2X(0.0f);	mOut.f[ 7]=PVRTF2X(0.0f);		mOut.f[11]=PVRTF2X(0.0f);		mOut.f[15]=PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function Name		PVRTMatrixRotationYX
 @Output			mOut	Rotation matrix
 @Input				fAngle	Angle of the rotation
 @Description		Create an Y rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationYX(
	PVRTMATRIXx	&mOut,
	const int	fAngle)
{
	int		fCosine, fSine;

	/* Precompute cos and sin */
#if defined(BUILD_DX11)
	fCosine	= PVRTXCOS(-fAngle);
    fSine	= PVRTXSIN(-fAngle);
#else
	fCosine	= PVRTXCOS(fAngle);
    fSine	= PVRTXSIN(fAngle);
#endif

	/* Create the trigonometric matrix corresponding to Y Rotation */
	mOut.f[ 0]=fCosine;				mOut.f[ 4]=PVRTF2X(0.0f);	mOut.f[ 8]=-fSine;				mOut.f[12]=PVRTF2X(0.0f);
	mOut.f[ 1]=PVRTF2X(0.0f);		mOut.f[ 5]=PVRTF2X(1.0f);	mOut.f[ 9]=PVRTF2X(0.0f);		mOut.f[13]=PVRTF2X(0.0f);
	mOut.f[ 2]=fSine;				mOut.f[ 6]=PVRTF2X(0.0f);	mOut.f[10]=fCosine;				mOut.f[14]=PVRTF2X(0.0f);
	mOut.f[ 3]=PVRTF2X(0.0f);		mOut.f[ 7]=PVRTF2X(0.0f);	mOut.f[11]=PVRTF2X(0.0f);		mOut.f[15]=PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function Name		PVRTMatrixRotationZX
 @Output			mOut	Rotation matrix
 @Input				fAngle	Angle of the rotation
 @Description		Create an Z rotation matrix mOut.
*****************************************************************************/
void PVRTMatrixRotationZX(
	PVRTMATRIXx	&mOut,
	const int	fAngle)
{
	int		fCosine, fSine;

	/* Precompute cos and sin */
#if defined(BUILD_DX11)
	fCosine = PVRTXCOS(-fAngle);
    fSine   = PVRTXSIN(-fAngle);
#else
	fCosine = PVRTXCOS(fAngle);
    fSine   = PVRTXSIN(fAngle);
#endif

	/* Create the trigonometric matrix corresponding to Z Rotation */
	mOut.f[ 0]=fCosine;				mOut.f[ 4]=fSine;				mOut.f[ 8]=PVRTF2X(0.0f);	mOut.f[12]=PVRTF2X(0.0f);
	mOut.f[ 1]=-fSine;				mOut.f[ 5]=fCosine;				mOut.f[ 9]=PVRTF2X(0.0f);	mOut.f[13]=PVRTF2X(0.0f);
	mOut.f[ 2]=PVRTF2X(0.0f);		mOut.f[ 6]=PVRTF2X(0.0f);		mOut.f[10]=PVRTF2X(1.0f);	mOut.f[14]=PVRTF2X(0.0f);
	mOut.f[ 3]=PVRTF2X(0.0f);		mOut.f[ 7]=PVRTF2X(0.0f);		mOut.f[11]=PVRTF2X(0.0f);	mOut.f[15]=PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function Name		PVRTMatrixTransposeX
 @Output			mOut	Transposed matrix
 @Input				mIn		Original matrix
 @Description		Compute the transpose matrix of mIn.
*****************************************************************************/
void PVRTMatrixTransposeX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mIn)
{
	PVRTMATRIXx	mTmp;

	mTmp.f[ 0]=mIn.f[ 0];	mTmp.f[ 4]=mIn.f[ 1];	mTmp.f[ 8]=mIn.f[ 2];	mTmp.f[12]=mIn.f[ 3];
	mTmp.f[ 1]=mIn.f[ 4];	mTmp.f[ 5]=mIn.f[ 5];	mTmp.f[ 9]=mIn.f[ 6];	mTmp.f[13]=mIn.f[ 7];
	mTmp.f[ 2]=mIn.f[ 8];	mTmp.f[ 6]=mIn.f[ 9];	mTmp.f[10]=mIn.f[10];	mTmp.f[14]=mIn.f[11];
	mTmp.f[ 3]=mIn.f[12];	mTmp.f[ 7]=mIn.f[13];	mTmp.f[11]=mIn.f[14];	mTmp.f[15]=mIn.f[15];

	mOut = mTmp;
}

/*!***************************************************************************
 @Function			PVRTMatrixInverseX
 @Output			mOut	Inversed matrix
 @Input				mIn		Original matrix
 @Description		Compute the inverse matrix of mIn.
					The matrix must be of the form :
					A 0
					C 1
					Where A is a 3x3 matrix and C is a 1x3 matrix.
*****************************************************************************/
void PVRTMatrixInverseX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mIn)
{
	PVRTMATRIXx	mDummyMatrix;
	int			det_1;
	int			pos, neg, temp;

    /* Calculate the determinant of submatrix A and determine if the
       the matrix is singular as limited by the double precision
       floating-point data representation. */
    pos = neg = 0;
    temp =  PVRTXMUL(PVRTXMUL(mIn.f[ 0], mIn.f[ 5]), mIn.f[10]);
    if (temp >= 0) pos += temp; else neg += temp;
    temp =  PVRTXMUL(PVRTXMUL(mIn.f[ 4], mIn.f[ 9]), mIn.f[ 2]);
    if (temp >= 0) pos += temp; else neg += temp;
    temp =  PVRTXMUL(PVRTXMUL(mIn.f[ 8], mIn.f[ 1]), mIn.f[ 6]);
    if (temp >= 0) pos += temp; else neg += temp;
	temp =  PVRTXMUL(PVRTXMUL(-mIn.f[ 8], mIn.f[ 5]), mIn.f[ 2]);
    if (temp >= 0) pos += temp; else neg += temp;
    temp =  PVRTXMUL(PVRTXMUL(-mIn.f[ 4], mIn.f[ 1]), mIn.f[10]);
    if (temp >= 0) pos += temp; else neg += temp;
    temp =  PVRTXMUL(PVRTXMUL(-mIn.f[ 0], mIn.f[ 9]), mIn.f[ 6]);
    if (temp >= 0) pos += temp; else neg += temp;
    det_1 = pos + neg;

    /* Is the submatrix A singular? */
    if (det_1 == 0)
	{
        /* Matrix M has no inverse */
        _RPT0(_CRT_WARN, "Matrix has no inverse : singular matrix\n");
        return;
    }
    else
	{
        /* Calculate inverse(A) = adj(A) / det(A) */
        //det_1 = 1.0 / det_1;
		det_1 = PVRTXDIV(PVRTF2X(1.0f), det_1);
		mDummyMatrix.f[ 0] =   PVRTXMUL(( PVRTXMUL(mIn.f[ 5], mIn.f[10]) - PVRTXMUL(mIn.f[ 9], mIn.f[ 6]) ), det_1);
		mDummyMatrix.f[ 1] = - PVRTXMUL(( PVRTXMUL(mIn.f[ 1], mIn.f[10]) - PVRTXMUL(mIn.f[ 9], mIn.f[ 2]) ), det_1);
		mDummyMatrix.f[ 2] =   PVRTXMUL(( PVRTXMUL(mIn.f[ 1], mIn.f[ 6]) - PVRTXMUL(mIn.f[ 5], mIn.f[ 2]) ), det_1);
		mDummyMatrix.f[ 4] = - PVRTXMUL(( PVRTXMUL(mIn.f[ 4], mIn.f[10]) - PVRTXMUL(mIn.f[ 8], mIn.f[ 6]) ), det_1);
		mDummyMatrix.f[ 5] =   PVRTXMUL(( PVRTXMUL(mIn.f[ 0], mIn.f[10]) - PVRTXMUL(mIn.f[ 8], mIn.f[ 2]) ), det_1);
		mDummyMatrix.f[ 6] = - PVRTXMUL(( PVRTXMUL(mIn.f[ 0], mIn.f[ 6]) - PVRTXMUL(mIn.f[ 4], mIn.f[ 2]) ), det_1);
		mDummyMatrix.f[ 8] =   PVRTXMUL(( PVRTXMUL(mIn.f[ 4], mIn.f[ 9]) - PVRTXMUL(mIn.f[ 8], mIn.f[ 5]) ), det_1);
		mDummyMatrix.f[ 9] = - PVRTXMUL(( PVRTXMUL(mIn.f[ 0], mIn.f[ 9]) - PVRTXMUL(mIn.f[ 8], mIn.f[ 1]) ), det_1);
		mDummyMatrix.f[10] =   PVRTXMUL(( PVRTXMUL(mIn.f[ 0], mIn.f[ 5]) - PVRTXMUL(mIn.f[ 4], mIn.f[ 1]) ), det_1);

        /* Calculate -C * inverse(A) */
        mDummyMatrix.f[12] = - ( PVRTXMUL(mIn.f[12], mDummyMatrix.f[ 0]) + PVRTXMUL(mIn.f[13], mDummyMatrix.f[ 4]) + PVRTXMUL(mIn.f[14], mDummyMatrix.f[ 8]) );
		mDummyMatrix.f[13] = - ( PVRTXMUL(mIn.f[12], mDummyMatrix.f[ 1]) + PVRTXMUL(mIn.f[13], mDummyMatrix.f[ 5]) + PVRTXMUL(mIn.f[14], mDummyMatrix.f[ 9]) );
		mDummyMatrix.f[14] = - ( PVRTXMUL(mIn.f[12], mDummyMatrix.f[ 2]) + PVRTXMUL(mIn.f[13], mDummyMatrix.f[ 6]) + PVRTXMUL(mIn.f[14], mDummyMatrix.f[10]) );

        /* Fill in last row */
        mDummyMatrix.f[ 3] = PVRTF2X(0.0f);
		mDummyMatrix.f[ 7] = PVRTF2X(0.0f);
		mDummyMatrix.f[11] = PVRTF2X(0.0f);
        mDummyMatrix.f[15] = PVRTF2X(1.0f);
	}

   	/* Copy contents of dummy matrix in pfMatrix */
	mOut = mDummyMatrix;
}

/*!***************************************************************************
 @Function			PVRTMatrixInverseExX
 @Output			mOut	Inversed matrix
 @Input				mIn		Original matrix
 @Description		Compute the inverse matrix of mIn.
					Uses a linear equation solver and the knowledge that M.M^-1=I.
					Use this fn to calculate the inverse of matrices that
					PVRTMatrixInverse() cannot.
*****************************************************************************/
void PVRTMatrixInverseExX(
	PVRTMATRIXx			&mOut,
	const PVRTMATRIXx	&mIn)
{
	PVRTMATRIXx		mTmp;
	int				*ppfRows[4], pfRes[4], pfIn[20];
	int				i, j;

	for (i = 0; i < 4; ++i)
	{
		ppfRows[i] = &pfIn[i * 5];
	}

	/* Solve 4 sets of 4 linear equations */
	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 4; ++j)
		{
			ppfRows[j][0] = c_mIdentity.f[i + 4 * j];
			memcpy(&ppfRows[j][1], &mIn.f[j * 4], 4 * sizeof(float));
		}

		PVRTMatrixLinearEqSolveX(pfRes, (int**)ppfRows, 4);

		for(j = 0; j < 4; ++j)
		{
			mTmp.f[i + 4 * j] = pfRes[j];
		}
	}

	mOut = mTmp;
}

/*!***************************************************************************
 @Function			PVRTMatrixLookAtLHX
 @Output			mOut	Look-at view matrix
 @Input				vEye	Position of the camera
 @Input				vAt		Point the camera is looking at
 @Input				vUp		Up direction for the camera
 @Description		Create a look-at view matrix.
*****************************************************************************/
void PVRTMatrixLookAtLHX(
	PVRTMATRIXx			&mOut,
	const PVRTVECTOR3x	&vEye,
	const PVRTVECTOR3x	&vAt,
	const PVRTVECTOR3x	&vUp)
{
	PVRTVECTOR3x	f, vUpActual, s, u;
	PVRTMATRIXx		t;

	f.x = vEye.x - vAt.x;
	f.y = vEye.y - vAt.y;
	f.z = vEye.z - vAt.z;

	PVRTMatrixVec3NormalizeX(f, f);
	PVRTMatrixVec3NormalizeX(vUpActual, vUp);
	PVRTMatrixVec3CrossProductX(s, f, vUpActual);
	PVRTMatrixVec3CrossProductX(u, s, f);

	mOut.f[ 0] = s.x;
	mOut.f[ 1] = u.x;
	mOut.f[ 2] = -f.x;
	mOut.f[ 3] = PVRTF2X(0.0f);

	mOut.f[ 4] = s.y;
	mOut.f[ 5] = u.y;
	mOut.f[ 6] = -f.y;
	mOut.f[ 7] = PVRTF2X(0.0f);

	mOut.f[ 8] = s.z;
	mOut.f[ 9] = u.z;
	mOut.f[10] = -f.z;
	mOut.f[11] = PVRTF2X(0.0f);

	mOut.f[12] = PVRTF2X(0.0f);
	mOut.f[13] = PVRTF2X(0.0f);
	mOut.f[14] = PVRTF2X(0.0f);
	mOut.f[15] = PVRTF2X(1.0f);

	PVRTMatrixTranslationX(t, -vEye.x, -vEye.y, -vEye.z);
	PVRTMatrixMultiplyX(mOut, t, mOut);
}

/*!***************************************************************************
 @Function			PVRTMatrixLookAtRHX
 @Output			mOut	Look-at view matrix
 @Input				vEye	Position of the camera
 @Input				vAt		Point the camera is looking at
 @Input				vUp		Up direction for the camera
 @Description		Create a look-at view matrix.
*****************************************************************************/
void PVRTMatrixLookAtRHX(
	PVRTMATRIXx			&mOut,
	const PVRTVECTOR3x	&vEye,
	const PVRTVECTOR3x	&vAt,
	const PVRTVECTOR3x	&vUp)
{
	PVRTVECTOR3x	f, vUpActual, s, u;
	PVRTMATRIXx		t;

	f.x = vAt.x - vEye.x;
	f.y = vAt.y - vEye.y;
	f.z = vAt.z - vEye.z;

	PVRTMatrixVec3NormalizeX(f, f);
	PVRTMatrixVec3NormalizeX(vUpActual, vUp);
	PVRTMatrixVec3CrossProductX(s, f, vUpActual);
	PVRTMatrixVec3CrossProductX(u, s, f);

	mOut.f[ 0] = s.x;
	mOut.f[ 1] = u.x;
	mOut.f[ 2] = -f.x;
	mOut.f[ 3] = PVRTF2X(0.0f);

	mOut.f[ 4] = s.y;
	mOut.f[ 5] = u.y;
	mOut.f[ 6] = -f.y;
	mOut.f[ 7] = PVRTF2X(0.0f);

	mOut.f[ 8] = s.z;
	mOut.f[ 9] = u.z;
	mOut.f[10] = -f.z;
	mOut.f[11] = PVRTF2X(0.0f);

	mOut.f[12] = PVRTF2X(0.0f);
	mOut.f[13] = PVRTF2X(0.0f);
	mOut.f[14] = PVRTF2X(0.0f);
	mOut.f[15] = PVRTF2X(1.0f);

	PVRTMatrixTranslationX(t, -vEye.x, -vEye.y, -vEye.z);
	PVRTMatrixMultiplyX(mOut, t, mOut);
}

/*!***************************************************************************
 @Function		PVRTMatrixPerspectiveFovLHX
 @Output		mOut		Perspective matrix
 @Input			fFOVy		Field of view
 @Input			fAspect		Aspect ratio
 @Input			fNear		Near clipping distance
 @Input			fFar		Far clipping distance
 @Input			bRotate		Should we rotate it ? (for upright screens)
 @Description	Create a perspective matrix.
*****************************************************************************/
void PVRTMatrixPerspectiveFovLHX(
	PVRTMATRIXx	&mOut,
	const int	fFOVy,
	const int	fAspect,
	const int	fNear,
	const int	fFar,
	const bool  bRotate)
{
	int		f, fRealAspect;

	if (bRotate)
		fRealAspect = PVRTXDIV(PVRTF2X(1.0f), fAspect);
	else
		fRealAspect = fAspect;

	f = PVRTXDIV(PVRTF2X(1.0f), PVRTXTAN(PVRTXMUL(fFOVy, PVRTF2X(0.5f))));

	mOut.f[ 0] = PVRTXDIV(f, fRealAspect);
	mOut.f[ 1] = PVRTF2X(0.0f);
	mOut.f[ 2] = PVRTF2X(0.0f);
	mOut.f[ 3] = PVRTF2X(0.0f);

	mOut.f[ 4] = PVRTF2X(0.0f);
	mOut.f[ 5] = f;
	mOut.f[ 6] = PVRTF2X(0.0f);
	mOut.f[ 7] = PVRTF2X(0.0f);

	mOut.f[ 8] = PVRTF2X(0.0f);
	mOut.f[ 9] = PVRTF2X(0.0f);
	mOut.f[10] = PVRTXDIV(fFar, fFar - fNear);
	mOut.f[11] = PVRTF2X(1.0f);

	mOut.f[12] = PVRTF2X(0.0f);
	mOut.f[13] = PVRTF2X(0.0f);
	mOut.f[14] = -PVRTXMUL(PVRTXDIV(fFar, fFar - fNear), fNear);
	mOut.f[15] = PVRTF2X(0.0f);

	if (bRotate)
	{
		PVRTMATRIXx mRotation, mTemp = mOut;
		PVRTMatrixRotationZX(mRotation, PVRTF2X(90.0f*PVRT_PIf/180.0f));
		PVRTMatrixMultiplyX(mOut, mTemp, mRotation);
	}
}

/*!***************************************************************************
 @Function		PVRTMatrixPerspectiveFovRHX
 @Output		mOut		Perspective matrix
 @Input			fFOVy		Field of view
 @Input			fAspect		Aspect ratio
 @Input			fNear		Near clipping distance
 @Input			fFar		Far clipping distance
 @Input			bRotate		Should we rotate it ? (for upright screens)
 @Description	Create a perspective matrix.
*****************************************************************************/
void PVRTMatrixPerspectiveFovRHX(
	PVRTMATRIXx	&mOut,
	const int	fFOVy,
	const int	fAspect,
	const int	fNear,
	const int	fFar,
	const bool  bRotate)
{
	int		f;

	int fCorrectAspect = fAspect;
	if (bRotate)
	{
		fCorrectAspect = PVRTXDIV(PVRTF2X(1.0f), fAspect);
	}
	f = PVRTXDIV(PVRTF2X(1.0f), PVRTXTAN(PVRTXMUL(fFOVy, PVRTF2X(0.5f))));

	mOut.f[ 0] = PVRTXDIV(f, fCorrectAspect);
	mOut.f[ 1] = PVRTF2X(0.0f);
	mOut.f[ 2] = PVRTF2X(0.0f);
	mOut.f[ 3] = PVRTF2X(0.0f);

	mOut.f[ 4] = PVRTF2X(0.0f);
	mOut.f[ 5] = f;
	mOut.f[ 6] = PVRTF2X(0.0f);
	mOut.f[ 7] = PVRTF2X(0.0f);

	mOut.f[ 8] = PVRTF2X(0.0f);
	mOut.f[ 9] = PVRTF2X(0.0f);
	mOut.f[10] = PVRTXDIV(fFar + fNear, fNear - fFar);
	mOut.f[11] = PVRTF2X(-1.0f);

	mOut.f[12] = PVRTF2X(0.0f);
	mOut.f[13] = PVRTF2X(0.0f);
	mOut.f[14] = PVRTXMUL(PVRTXDIV(fFar, fNear - fFar), fNear) << 1;	// Cheap 2x
	mOut.f[15] = PVRTF2X(0.0f);

	if (bRotate)
	{
		PVRTMATRIXx mRotation, mTemp = mOut;
		PVRTMatrixRotationZX(mRotation, PVRTF2X(-90.0f*PVRT_PIf/180.0f));
		PVRTMatrixMultiplyX(mOut, mTemp, mRotation);
	}
}

/*!***************************************************************************
 @Function		PVRTMatrixOrthoLHX
 @Output		mOut		Orthographic matrix
 @Input			w			Width of the screen
 @Input			h			Height of the screen
 @Input			zn			Near clipping distance
 @Input			zf			Far clipping distance
 @Input			bRotate		Should we rotate it ? (for upright screens)
 @Description	Create an orthographic matrix.
*****************************************************************************/
void PVRTMatrixOrthoLHX(
	PVRTMATRIXx	&mOut,
	const int	w,
	const int	h,
	const int	zn,
	const int	zf,
	const bool  bRotate)
{
	int fCorrectW = w;
	int fCorrectH = h;
	if (bRotate)
	{
		fCorrectW = h;
		fCorrectH = w;
	}
	mOut.f[ 0] = PVRTXDIV(PVRTF2X(2.0f), fCorrectW);
	mOut.f[ 1] = PVRTF2X(0.0f);
	mOut.f[ 2] = PVRTF2X(0.0f);
	mOut.f[ 3] = PVRTF2X(0.0f);

	mOut.f[ 4] = PVRTF2X(0.0f);
	mOut.f[ 5] = PVRTXDIV(PVRTF2X(2.0f), fCorrectH);
	mOut.f[ 6] = PVRTF2X(0.0f);
	mOut.f[ 7] = PVRTF2X(0.0f);

	mOut.f[ 8] = PVRTF2X(0.0f);
	mOut.f[ 9] = PVRTF2X(0.0f);
	mOut.f[10] = PVRTXDIV(PVRTF2X(1.0f), zf - zn);
	mOut.f[11] = PVRTXDIV(zn, zn - zf);

	mOut.f[12] = PVRTF2X(0.0f);
	mOut.f[13] = PVRTF2X(0.0f);
	mOut.f[14] = PVRTF2X(0.0f);
	mOut.f[15] = PVRTF2X(1.0f);

	if (bRotate)
	{
		PVRTMATRIXx mRotation, mTemp = mOut;
		PVRTMatrixRotationZX(mRotation, PVRTF2X(-90.0f*PVRT_PIf/180.0f));
		PVRTMatrixMultiplyX(mOut, mRotation, mTemp);
	}
}

/*!***************************************************************************
 @Function		PVRTMatrixOrthoRHX
 @Output		mOut		Orthographic matrix
 @Input			w			Width of the screen
 @Input			h			Height of the screen
 @Input			zn			Near clipping distance
 @Input			zf			Far clipping distance
 @Input			bRotate		Should we rotate it ? (for upright screens)
 @Description	Create an orthographic matrix.
*****************************************************************************/
void PVRTMatrixOrthoRHX(
	PVRTMATRIXx	&mOut,
	const int	w,
	const int	h,
	const int	zn,
	const int	zf,
	const bool  bRotate)
{
	int fCorrectW = w;
	int fCorrectH = h;
	if (bRotate)
	{
		fCorrectW = h;
		fCorrectH = w;
	}
	mOut.f[ 0] = PVRTXDIV(PVRTF2X(2.0f), fCorrectW);
	mOut.f[ 1] = PVRTF2X(0.0f);
	mOut.f[ 2] = PVRTF2X(0.0f);
	mOut.f[ 3] = PVRTF2X(0.0f);

	mOut.f[ 4] = PVRTF2X(0.0f);
	mOut.f[ 5] = PVRTXDIV(PVRTF2X(2.0f), fCorrectH);
	mOut.f[ 6] = PVRTF2X(0.0f);
	mOut.f[ 7] = PVRTF2X(0.0f);

	mOut.f[ 8] = PVRTF2X(0.0f);
	mOut.f[ 9] = PVRTF2X(0.0f);
	mOut.f[10] = PVRTXDIV(PVRTF2X(1.0f), zn - zf);
	mOut.f[11] = PVRTXDIV(zn, zn - zf);

	mOut.f[12] = PVRTF2X(0.0f);
	mOut.f[13] = PVRTF2X(0.0f);
	mOut.f[14] = PVRTF2X(0.0f);
	mOut.f[15] = PVRTF2X(1.0f);

	if (bRotate)
	{
		PVRTMATRIXx mRotation, mTemp = mOut;
		PVRTMatrixRotationZX(mRotation, PVRTF2X(-90.0f*PVRT_PIf/180.0f));
		PVRTMatrixMultiplyX(mOut, mRotation, mTemp);
	}
}

/*!***************************************************************************
 @Function			PVRTMatrixVec3LerpX
 @Output			vOut	Result of the interpolation
 @Input				v1		First vector to interpolate from
 @Input				v2		Second vector to interpolate form
 @Input				s		Coefficient of interpolation
 @Description		This function performs the linear interpolation based on
					the following formula: V1 + s(V2-V1).
*****************************************************************************/
void PVRTMatrixVec3LerpX(
	PVRTVECTOR3x		&vOut,
	const PVRTVECTOR3x	&v1,
	const PVRTVECTOR3x	&v2,
	const int			s)
{
	vOut.x = v1.x + PVRTXMUL(s, v2.x - v1.x);
	vOut.y = v1.y + PVRTXMUL(s, v2.y - v1.y);
	vOut.z = v1.z + PVRTXMUL(s, v2.z - v1.z);
}

/*!***************************************************************************
 @Function			PVRTMatrixVec3DotProductX
 @Input				v1		First vector
 @Input				v2		Second vector
 @Return			Dot product of the two vectors.
 @Description		This function performs the dot product of the two
					supplied vectors.
					A single >> 16 shift could be applied to the final accumulated
					result however this runs the risk of overflow between the
					results of the intermediate additions.
*****************************************************************************/
int PVRTMatrixVec3DotProductX(
	const PVRTVECTOR3x	&v1,
	const PVRTVECTOR3x	&v2)
{
	return (PVRTXMUL(v1.x, v2.x) + PVRTXMUL(v1.y, v2.y) + PVRTXMUL(v1.z, v2.z));
}

/*!***************************************************************************
 @Function			PVRTMatrixVec3CrossProductX
 @Output			vOut	Cross product of the two vectors
 @Input				v1		First vector
 @Input				v2		Second vector
 @Description		This function performs the cross product of the two
					supplied vectors.
*****************************************************************************/
void PVRTMatrixVec3CrossProductX(
	PVRTVECTOR3x		&vOut,
	const PVRTVECTOR3x	&v1,
	const PVRTVECTOR3x	&v2)
{
	PVRTVECTOR3x result;

	/* Perform calculation on a dummy VECTOR (result) */
    result.x = PVRTXMUL(v1.y, v2.z) - PVRTXMUL(v1.z, v2.y);
    result.y = PVRTXMUL(v1.z, v2.x) - PVRTXMUL(v1.x, v2.z);
    result.z = PVRTXMUL(v1.x, v2.y) - PVRTXMUL(v1.y, v2.x);

	/* Copy result in pOut */
	vOut = result;
}

/*!***************************************************************************
 @Function			PVRTMatrixVec3NormalizeX
 @Output			vOut	Normalized vector
 @Input				vIn		Vector to normalize
 @Description		Normalizes the supplied vector.
					The square root function is currently still performed
					in floating-point.
					Original vector is scaled down prior to be normalized in
					order to avoid overflow issues.
****************************************************************************/
void PVRTMatrixVec3NormalizeX(
	PVRTVECTOR3x		&vOut,
	const PVRTVECTOR3x	&vIn)
{
	int				f, n;
	PVRTVECTOR3x	vTemp;

	/* Scale vector by uniform value */
	n = PVRTABS(vIn.x) + PVRTABS(vIn.y) + PVRTABS(vIn.z);
	vTemp.x = PVRTXDIV(vIn.x, n);
	vTemp.y = PVRTXDIV(vIn.y, n);
	vTemp.z = PVRTXDIV(vIn.z, n);

	/* Calculate x2+y2+z2/sqrt(x2+y2+z2) */
	f = PVRTMatrixVec3DotProductX(vTemp, vTemp);
	f = PVRTXDIV(PVRTF2X(1.0f), PVRTF2X(sqrt(PVRTX2F(f))));

	/* Multiply vector components by f */
	vOut.x = PVRTXMUL(vTemp.x, f);
	vOut.y = PVRTXMUL(vTemp.y, f);
	vOut.z = PVRTXMUL(vTemp.z, f);
}

/*!***************************************************************************
 @Function			PVRTMatrixVec3LengthX
 @Input				vIn		Vector to get the length of
 @Return			The length of the vector
 @Description		Gets the length of the supplied vector
*****************************************************************************/
int PVRTMatrixVec3LengthX(
	const PVRTVECTOR3x	&vIn)
{
	int temp;

	temp = PVRTXMUL(vIn.x,vIn.x) + PVRTXMUL(vIn.y,vIn.y) + PVRTXMUL(vIn.z,vIn.z);
	return PVRTF2X(sqrt(PVRTX2F(temp)));
}

/*!***************************************************************************
 @Function			PVRTMatrixLinearEqSolveX
 @Input				pSrc	2D array of floats. 4 Eq linear problem is 5x4
							matrix, constants in first column
 @Input				nCnt	Number of equations to solve
 @Output			pRes	Result
 @Description		Solves 'nCnt' simultaneous equations of 'nCnt' variables.
					pRes should be an array large enough to contain the
					results: the values of the 'nCnt' variables.
					This fn recursively uses Gaussian Elimination.
*****************************************************************************/
void PVRTMatrixLinearEqSolveX(
	int			* const pRes,
	int			** const pSrc,
	const int	nCnt)
{
	int		i, j, k;
	int		f;

	if (nCnt == 1)
	{
		_ASSERT(pSrc[0][1] != 0);
		pRes[0] = PVRTXDIV(pSrc[0][0], pSrc[0][1]);
		return;
	}

	// Loop backwards in an attempt avoid the need to swap rows
	i = nCnt;
	while(i)
	{
		--i;

		if(pSrc[i][nCnt] != PVRTF2X(0.0f))
		{
			// Row i can be used to zero the other rows; let's move it to the bottom
			if(i != (nCnt-1))
			{
				for(j = 0; j <= nCnt; ++j)
				{
					// Swap the two values
					f = pSrc[nCnt-1][j];
					pSrc[nCnt-1][j] = pSrc[i][j];
					pSrc[i][j] = f;
				}
			}

			// Now zero the last columns of the top rows
			for(j = 0; j < (nCnt-1); ++j)
			{
				_ASSERT(pSrc[nCnt-1][nCnt] != PVRTF2X(0.0f));
				f = PVRTXDIV(pSrc[j][nCnt], pSrc[nCnt-1][nCnt]);

				// No need to actually calculate a zero for the final column
				for(k = 0; k < nCnt; ++k)
				{
					pSrc[j][k] -= PVRTXMUL(f, pSrc[nCnt-1][k]);
				}
			}

			break;
		}
	}

	// Solve the top-left sub matrix
	PVRTMatrixLinearEqSolveX(pRes, pSrc, nCnt - 1);

	// Now calc the solution for the bottom row
	f = pSrc[nCnt-1][0];
	for(k = 1; k < nCnt; ++k)
	{
		f -= PVRTXMUL(pSrc[nCnt-1][k], pRes[k-1]);
	}
	_ASSERT(pSrc[nCnt-1][nCnt] != PVRTF2X(0));
	f = PVRTXDIV(f, pSrc[nCnt-1][nCnt]);
	pRes[nCnt-1] = f;
}

/*****************************************************************************
 End of file (PVRTMatrixX.cpp)
*****************************************************************************/

