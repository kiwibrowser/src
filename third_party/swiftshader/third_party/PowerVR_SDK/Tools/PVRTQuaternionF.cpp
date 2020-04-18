/******************************************************************************

 @File         PVRTQuaternionF.cpp

 @Title        PVRTQuaternionF

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Set of mathematical functions for quaternions.

******************************************************************************/
#include "PVRTGlobal.h"
#include <math.h>
#include <string.h>
#include "PVRTFixedPoint.h"		// Only needed for trig function float lookups
#include "PVRTQuaternion.h"


/****************************************************************************
** Functions
****************************************************************************/

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionIdentityF
 @Output			qOut	Identity quaternion
 @Description		Sets the quaternion to (0, 0, 0, 1), the identity quaternion.
*****************************************************************************/
void PVRTMatrixQuaternionIdentityF(PVRTQUATERNIONf &qOut)
{
	qOut.x = 0;
	qOut.y = 0;
	qOut.z = 0;
	qOut.w = 1;
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionRotationAxisF
 @Output			qOut	Rotation quaternion
 @Input				vAxis	Axis to rotate around
 @Input				fAngle	Angle to rotate
 @Description		Create quaternion corresponding to a rotation of fAngle
					radians around submitted vector.
*****************************************************************************/
void PVRTMatrixQuaternionRotationAxisF(
	PVRTQUATERNIONf		&qOut,
	const PVRTVECTOR3f	&vAxis,
	const float			fAngle)
{
	float	fSin, fCos;

	fSin = (float)PVRTFSIN(fAngle * 0.5f);
	fCos = (float)PVRTFCOS(fAngle * 0.5f);

	/* Create quaternion */
	qOut.x = vAxis.x * fSin;
	qOut.y = vAxis.y * fSin;
	qOut.z = vAxis.z * fSin;
	qOut.w = fCos;

	/* Normalise it */
	PVRTMatrixQuaternionNormalizeF(qOut);
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionToAxisAngleF
 @Input				qIn		Quaternion to transform
 @Output			vAxis	Axis of rotation
 @Output			fAngle	Angle of rotation
 @Description		Convert a quaternion to an axis and angle. Expects a unit
					quaternion.
*****************************************************************************/
void PVRTMatrixQuaternionToAxisAngleF(
	const PVRTQUATERNIONf	&qIn,
	PVRTVECTOR3f			&vAxis,
	float					&fAngle)
{
	float	fCosAngle, fSinAngle;
	double	temp;

	/* Compute some values */
	fCosAngle	= qIn.w;
	temp		= 1.0f - fCosAngle*fCosAngle;
	fAngle		= (float)PVRTFACOS(fCosAngle)*2.0f;
	fSinAngle	= (float)sqrt(temp);

	/* This is to avoid a division by zero */
	if ((float)fabs(fSinAngle)<0.0005f)
		fSinAngle = 1.0f;

	/* Get axis vector */
	vAxis.x = qIn.x / fSinAngle;
	vAxis.y = qIn.y / fSinAngle;
	vAxis.z = qIn.z / fSinAngle;
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionSlerpF
 @Output			qOut	Result of the interpolation
 @Input				qA		First quaternion to interpolate from
 @Input				qB		Second quaternion to interpolate from
 @Input				t		Coefficient of interpolation
 @Description		Perform a Spherical Linear intERPolation between quaternion A
					and quaternion B at time t. t must be between 0.0f and 1.0f
*****************************************************************************/
void PVRTMatrixQuaternionSlerpF(
	PVRTQUATERNIONf			&qOut,
	const PVRTQUATERNIONf	&qA,
	const PVRTQUATERNIONf	&qB,
	const float				t)
{
	float		fCosine, fAngle, A, B;

	/* Parameter checking */
	if (t<0.0f || t>1.0f)
	{
		_RPT0(_CRT_WARN, "PVRTMatrixQuaternionSlerp : Bad parameters\n");
		qOut.x = 0;
		qOut.y = 0;
		qOut.z = 0;
		qOut.w = 1;
		return;
	}

	/* Find sine of Angle between Quaternion A and B (dot product between quaternion A and B) */
	fCosine = qA.w*qB.w + qA.x*qB.x + qA.y*qB.y + qA.z*qB.z;

	if (fCosine < 0)
	{
		PVRTQUATERNIONf qi;

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

		PVRTMatrixQuaternionSlerpF(qOut, qA, qi, t);
		return;
	}

	fCosine = PVRT_MIN(fCosine, 1.0f);
	fAngle = (float)PVRTFACOS(fCosine);

	/* Avoid a division by zero */
	if (fAngle==0.0f)
	{
		qOut = qA;
		return;
	}

	/* Precompute some values */
	A = (float)(PVRTFSIN((1.0f-t)*fAngle) / PVRTFSIN(fAngle));
	B = (float)(PVRTFSIN(t*fAngle) / PVRTFSIN(fAngle));

	/* Compute resulting quaternion */
	qOut.x = A * qA.x + B * qB.x;
	qOut.y = A * qA.y + B * qB.y;
	qOut.z = A * qA.z + B * qB.z;
	qOut.w = A * qA.w + B * qB.w;

	/* Normalise result */
	PVRTMatrixQuaternionNormalizeF(qOut);
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionNormalizeF
 @Modified			quat	Vector to normalize
 @Description		Normalize quaternion.
*****************************************************************************/
void PVRTMatrixQuaternionNormalizeF(PVRTQUATERNIONf &quat)
{
	float	fMagnitude;
	double	temp;

	/* Compute quaternion magnitude */
	temp = quat.w*quat.w + quat.x*quat.x + quat.y*quat.y + quat.z*quat.z;
	fMagnitude = (float)sqrt(temp);

	/* Divide each quaternion component by this magnitude */
	if (fMagnitude!=0.0f)
	{
		fMagnitude = 1.0f / fMagnitude;
		quat.x *= fMagnitude;
		quat.y *= fMagnitude;
		quat.z *= fMagnitude;
		quat.w *= fMagnitude;
	}
}

/*!***************************************************************************
 @Function			PVRTMatrixRotationQuaternionF
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
void PVRTMatrixRotationQuaternionF(
	PVRTMATRIXf				&mOut,
	const PVRTQUATERNIONf	&quat)
{
	const PVRTQUATERNIONf *pQ;

#if defined(BUILD_DX11)
	PVRTQUATERNIONf qInv;

	qInv.x = -quat.x;
	qInv.y = -quat.y;
	qInv.z = -quat.z;
	qInv.w = quat.w;

	pQ = &qInv;
#else
	pQ = &quat;
#endif

    /* Fill matrix members */
	mOut.f[0] = 1.0f - 2.0f*pQ->y*pQ->y - 2.0f*pQ->z*pQ->z;
	mOut.f[1] = 2.0f*pQ->x*pQ->y - 2.0f*pQ->z*pQ->w;
	mOut.f[2] = 2.0f*pQ->x*pQ->z + 2.0f*pQ->y*pQ->w;
	mOut.f[3] = 0.0f;

	mOut.f[4] = 2.0f*pQ->x*pQ->y + 2.0f*pQ->z*pQ->w;
	mOut.f[5] = 1.0f - 2.0f*pQ->x*pQ->x - 2.0f*pQ->z*pQ->z;
	mOut.f[6] = 2.0f*pQ->y*pQ->z - 2.0f*pQ->x*pQ->w;
	mOut.f[7] = 0.0f;

	mOut.f[8] = 2.0f*pQ->x*pQ->z - 2*pQ->y*pQ->w;
	mOut.f[9] = 2.0f*pQ->y*pQ->z + 2.0f*pQ->x*pQ->w;
	mOut.f[10] = 1.0f - 2.0f*pQ->x*pQ->x - 2*pQ->y*pQ->y;
	mOut.f[11] = 0.0f;

	mOut.f[12] = 0.0f;
	mOut.f[13] = 0.0f;
	mOut.f[14] = 0.0f;
	mOut.f[15] = 1.0f;
}

/*!***************************************************************************
 @Function			PVRTMatrixQuaternionMultiplyF
 @Output			qOut	Resulting quaternion
 @Input				qA		First quaternion to multiply
 @Input				qB		Second quaternion to multiply
 @Description		Multiply quaternion A with quaternion B and return the
					result in qOut.
*****************************************************************************/
void PVRTMatrixQuaternionMultiplyF(
	PVRTQUATERNIONf			&qOut,
	const PVRTQUATERNIONf	&qA,
	const PVRTQUATERNIONf	&qB)
{
	PVRTVECTOR3f	CrossProduct;
	PVRTQUATERNIONf qRet;

	/* Compute scalar component */
	qRet.w = (qA.w*qB.w) - (qA.x*qB.x + qA.y*qB.y + qA.z*qB.z);

	/* Compute cross product */
	CrossProduct.x = qA.y*qB.z - qA.z*qB.y;
	CrossProduct.y = qA.z*qB.x - qA.x*qB.z;
	CrossProduct.z = qA.x*qB.y - qA.y*qB.x;

	/* Compute result vector */
	qRet.x = (qA.w * qB.x) + (qB.w * qA.x) + CrossProduct.x;
	qRet.y = (qA.w * qB.y) + (qB.w * qA.y) + CrossProduct.y;
	qRet.z = (qA.w * qB.z) + (qB.w * qA.z) + CrossProduct.z;

	/* Normalize resulting quaternion */
	PVRTMatrixQuaternionNormalizeF(qRet);

	/* Copy result to mOut */
	qOut = qRet;
}

/*****************************************************************************
 End of file (PVRTQuaternionF.cpp)
*****************************************************************************/

