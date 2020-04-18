/******************************************************************************

 @File         PVRTQuaternionX.cpp

 @Title        PVRTQuaternionX

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Set of mathematical functions for quaternions.

******************************************************************************/
#include "PVRTContext.h"
#include <math.h>
#include <string.h>

#include "PVRTFixedPoint.h"
#include "PVRTQuaternion.h"


/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionIdentityX
 @Output			qOut	Identity quaternion
 @Description		Sets the quaternion to (0, 0, 0, 1), the identity quaternion.
*****************************************************************************/
void PVRTMatrixQuaternionIdentityX(PVRTQUATERNIONx		&qOut)
{
	qOut.x = PVRTF2X(0.0f);
	qOut.y = PVRTF2X(0.0f);
	qOut.z = PVRTF2X(0.0f);
	qOut.w = PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionRotationAxisX
 @Output			qOut	Rotation quaternion
 @Input				vAxis	Axis to rotate around
 @Input				fAngle	Angle to rotate
 @Description		Create quaternion corresponding to a rotation of fAngle
					radians around submitted vector.
*****************************************************************************/
void PVRTMatrixQuaternionRotationAxisX(
	PVRTQUATERNIONx		&qOut,
	const PVRTVECTOR3x	&vAxis,
	const int			fAngle)
{
	int	fSin, fCos;

	fSin = PVRTXSIN(fAngle>>1);
	fCos = PVRTXCOS(fAngle>>1);

	/* Create quaternion */
	qOut.x = PVRTXMUL(vAxis.x, fSin);
	qOut.y = PVRTXMUL(vAxis.y, fSin);
	qOut.z = PVRTXMUL(vAxis.z, fSin);
	qOut.w = fCos;

	/* Normalise it */
	PVRTMatrixQuaternionNormalizeX(qOut);
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionToAxisAngleX
 @Input				qIn		Quaternion to transform
 @Output			vAxis	Axis of rotation
 @Output			fAngle	Angle of rotation
 @Description		Convert a quaternion to an axis and angle. Expects a unit
					quaternion.
*****************************************************************************/
void PVRTMatrixQuaternionToAxisAngleX(
	const PVRTQUATERNIONx	&qIn,
	PVRTVECTOR3x			&vAxis,
	int						&fAngle)
{
	int		fCosAngle, fSinAngle;
	int		temp;

	/* Compute some values */
	fCosAngle	= qIn.w;
	temp		= PVRTF2X(1.0f) - PVRTXMUL(fCosAngle, fCosAngle);
	fAngle		= PVRTXMUL(PVRTXACOS(fCosAngle), PVRTF2X(2.0f));
	fSinAngle	= PVRTF2X(((float)sqrt(PVRTX2F(temp))));

	/* This is to avoid a division by zero */
	if (PVRTABS(fSinAngle)<PVRTF2X(0.0005f))
	{
		fSinAngle = PVRTF2X(1.0f);
	}

	/* Get axis vector */
	vAxis.x = PVRTXDIV(qIn.x, fSinAngle);
	vAxis.y = PVRTXDIV(qIn.y, fSinAngle);
	vAxis.z = PVRTXDIV(qIn.z, fSinAngle);
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionSlerpX
 @Output			qOut	Result of the interpolation
 @Input				qA		First quaternion to interpolate from
 @Input				qB		Second quaternion to interpolate from
 @Input				t		Coefficient of interpolation
 @Description		Perform a Spherical Linear intERPolation between quaternion A
					and quaternion B at time t. t must be between 0.0f and 1.0f
					Requires input quaternions to be normalized
*****************************************************************************/
void PVRTMatrixQuaternionSlerpX(
	PVRTQUATERNIONx			&qOut,
	const PVRTQUATERNIONx	&qA,
	const PVRTQUATERNIONx	&qB,
	const int				t)
{
	int		fCosine, fAngle, A, B;

	/* Parameter checking */
	if (t<PVRTF2X(0.0f) || t>PVRTF2X(1.0f))
	{
		_RPT0(_CRT_WARN, "PVRTMatrixQuaternionSlerp : Bad parameters\n");
		qOut.x = PVRTF2X(0.0f);
		qOut.y = PVRTF2X(0.0f);
		qOut.z = PVRTF2X(0.0f);
		qOut.w = PVRTF2X(1.0f);
		return;
	}

	/* Find sine of Angle between Quaternion A and B (dot product between quaternion A and B) */
	fCosine = PVRTXMUL(qA.w, qB.w) +
		PVRTXMUL(qA.x, qB.x) + PVRTXMUL(qA.y, qB.y) + PVRTXMUL(qA.z, qB.z);

	if(fCosine < PVRTF2X(0.0f))
	{
		PVRTQUATERNIONx qi;

		/*
			<http://www.magic-software.com/Documentation/Quaternions.pdf>

			"It is important to note that the quaternions q and -q represent
			the same rotation... while either quaternion will do, the
			interpolation methods require choosing one over the other.

			"Although q1 and -q1 represent the same rotation, the values of
			Slerp(t; q0, q1) and Slerp(t; q0,-q1) are not the same. It is
			customary to choose the sign... on q1 so that... the angle
			between q0 and q1 is acute. This choice avoids extra
			spinning caused by the interpolated rotations."
		*/
		qi.x = -qB.x;
		qi.y = -qB.y;
		qi.z = -qB.z;
		qi.w = -qB.w;

		PVRTMatrixQuaternionSlerpX(qOut, qA, qi, t);
		return;
	}

	fCosine = PVRT_MIN(fCosine, PVRTF2X(1.0f));
	fAngle = PVRTXACOS(fCosine);

	/* Avoid a division by zero */
	if (fAngle==PVRTF2X(0.0f))
	{
		qOut = qA;
		return;
	}

	/* Precompute some values */
	A = PVRTXDIV(PVRTXSIN(PVRTXMUL((PVRTF2X(1.0f)-t), fAngle)), PVRTXSIN(fAngle));
	B = PVRTXDIV(PVRTXSIN(PVRTXMUL(t, fAngle)), PVRTXSIN(fAngle));

	/* Compute resulting quaternion */
	qOut.x = PVRTXMUL(A, qA.x) + PVRTXMUL(B, qB.x);
	qOut.y = PVRTXMUL(A, qA.y) + PVRTXMUL(B, qB.y);
	qOut.z = PVRTXMUL(A, qA.z) + PVRTXMUL(B, qB.z);
	qOut.w = PVRTXMUL(A, qA.w) + PVRTXMUL(B, qB.w);

	/* Normalise result */
	PVRTMatrixQuaternionNormalizeX(qOut);
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionNormalizeX
 @Modified			quat	Vector to normalize
 @Description		Normalize quaternion.
					Original quaternion is scaled down prior to be normalized in
					order to avoid overflow issues.
*****************************************************************************/
void PVRTMatrixQuaternionNormalizeX(PVRTQUATERNIONx &quat)
{
	PVRTQUATERNIONx	qTemp;
	int				f, n;

	/* Scale vector by uniform value */
	n = PVRTABS(quat.w) + PVRTABS(quat.x) + PVRTABS(quat.y) + PVRTABS(quat.z);
	qTemp.w = PVRTXDIV(quat.w, n);
	qTemp.x = PVRTXDIV(quat.x, n);
	qTemp.y = PVRTXDIV(quat.y, n);
	qTemp.z = PVRTXDIV(quat.z, n);

	/* Compute quaternion magnitude */
	f = PVRTXMUL(qTemp.w, qTemp.w) + PVRTXMUL(qTemp.x, qTemp.x) + PVRTXMUL(qTemp.y, qTemp.y) + PVRTXMUL(qTemp.z, qTemp.z);
	f = PVRTXDIV(PVRTF2X(1.0f), PVRTF2X(sqrt(PVRTX2F(f))));

	/* Multiply vector components by f */
	quat.x = PVRTXMUL(qTemp.x, f);
	quat.y = PVRTXMUL(qTemp.y, f);
	quat.z = PVRTXMUL(qTemp.z, f);
	quat.w = PVRTXMUL(qTemp.w, f);
}

/*!***************************************************************************
 @Function			PVRTMatrixRotationQuaternionX
 @Output			mOut	Resulting rotation matrix
 @Input				quat	Quaternion to transform
 @Description		Create rotation matrix from submitted quaternion.
					Assuming the quaternion is of the form [X Y Z W]:

						|       2     2									|
						| 1 - 2Y  - 2Z    2XY - 2ZW      2XZ + 2YW		 0	|
						|													|
						|                       2     2					|
					M = | 2XY + 2ZW       1 - 2X  - 2Z   2YZ - 2XW		 0	|
						|													|
						|                                      2     2		|
						| 2XZ - 2YW       2YZ + 2XW      1 - 2X  - 2Y	 0	|
						|													|
						|     0			   0			  0          1  |
*****************************************************************************/
void PVRTMatrixRotationQuaternionX(
	PVRTMATRIXx				&mOut,
	const PVRTQUATERNIONx	&quat)
{
	const PVRTQUATERNIONx *pQ;

#if defined(BUILD_DX11)
	PVRTQUATERNIONx qInv;

	qInv.x = -quat.x;
	qInv.y = -quat.y;
	qInv.z = -quat.z;
	qInv.w =  quat.w;

	pQ = &qInv;
#else
	pQ = &quat;
#endif

    /* Fill matrix members */
	mOut.f[0] = PVRTF2X(1.0f) - (PVRTXMUL(pQ->y, pQ->y)<<1) - (PVRTXMUL(pQ->z, pQ->z)<<1);
	mOut.f[1] = (PVRTXMUL(pQ->x, pQ->y)<<1) - (PVRTXMUL(pQ->z, pQ->w)<<1);
	mOut.f[2] = (PVRTXMUL(pQ->x, pQ->z)<<1) + (PVRTXMUL(pQ->y, pQ->w)<<1);
	mOut.f[3] = PVRTF2X(0.0f);

	mOut.f[4] = (PVRTXMUL(pQ->x, pQ->y)<<1) + (PVRTXMUL(pQ->z, pQ->w)<<1);
	mOut.f[5] = PVRTF2X(1.0f) - (PVRTXMUL(pQ->x, pQ->x)<<1) - (PVRTXMUL(pQ->z, pQ->z)<<1);
	mOut.f[6] = (PVRTXMUL(pQ->y, pQ->z)<<1) - (PVRTXMUL(pQ->x, pQ->w)<<1);
	mOut.f[7] = PVRTF2X(0.0f);

	mOut.f[8] = (PVRTXMUL(pQ->x, pQ->z)<<1) - (PVRTXMUL(pQ->y, pQ->w)<<1);
	mOut.f[9] = (PVRTXMUL(pQ->y, pQ->z)<<1) + (PVRTXMUL(pQ->x, pQ->w)<<1);
	mOut.f[10] = PVRTF2X(1.0f) - (PVRTXMUL(pQ->x, pQ->x)<<1) - (PVRTXMUL(pQ->y, pQ->y)<<1);
	mOut.f[11] = PVRTF2X(0.0f);

	mOut.f[12] = PVRTF2X(0.0f);
	mOut.f[13] = PVRTF2X(0.0f);
	mOut.f[14] = PVRTF2X(0.0f);
	mOut.f[15] = PVRTF2X(1.0f);
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionMultiplyX
 @Output			qOut	Resulting quaternion
 @Input				qA		First quaternion to multiply
 @Input				qB		Second quaternion to multiply
 @Description		Multiply quaternion A with quaternion B and return the
					result in qOut.
					Input quaternions must be normalized.
*****************************************************************************/
void PVRTMatrixQuaternionMultiplyX(
	PVRTQUATERNIONx			&qOut,
	const PVRTQUATERNIONx	&qA,
	const PVRTQUATERNIONx	&qB)
{
	PVRTVECTOR3x	CrossProduct;

	/* Compute scalar component */
	qOut.w = PVRTXMUL(qA.w, qB.w) -
				   (PVRTXMUL(qA.x, qB.x) + PVRTXMUL(qA.y, qB.y) + PVRTXMUL(qA.z, qB.z));

	/* Compute cross product */
	CrossProduct.x = PVRTXMUL(qA.y, qB.z) - PVRTXMUL(qA.z, qB.y);
	CrossProduct.y = PVRTXMUL(qA.z, qB.x) - PVRTXMUL(qA.x, qB.z);
	CrossProduct.z = PVRTXMUL(qA.x, qB.y) - PVRTXMUL(qA.y, qB.x);

	/* Compute result vector */
	qOut.x = PVRTXMUL(qA.w, qB.x) + PVRTXMUL(qB.w, qA.x) + CrossProduct.x;
	qOut.y = PVRTXMUL(qA.w, qB.y) + PVRTXMUL(qB.w, qA.y) + CrossProduct.y;
	qOut.z = PVRTXMUL(qA.w, qB.z) + PVRTXMUL(qB.w, qA.z) + CrossProduct.z;

	/* Normalize resulting quaternion */
	PVRTMatrixQuaternionNormalizeX(qOut);
}

/*****************************************************************************
 End of file (PVRTQuaternionX.cpp)
*****************************************************************************/

