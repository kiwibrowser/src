/*!****************************************************************************

 @file         PVRTVector.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Vector and matrix mathematics library

******************************************************************************/
#ifndef __PVRTVECTOR_H__
#define __PVRTVECTOR_H__

#include "assert.h"
#include "PVRTGlobal.h"
#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"
#include <string.h>
#include <math.h>

/*!***************************************************************************
** Forward Declarations for vector and matrix structs
****************************************************************************/
struct PVRTVec4;
struct PVRTVec3;
struct PVRTMat3;
struct PVRTMat4;

/*!***************************************************************************
 @fn       			PVRTLinearEqSolve
 @param[in]			pSrc	2D array of floats. 4 Eq linear problem is 5x4
							matrix, constants in first column
 @param[in]			nCnt	Number of equations to solve
 @param[out]		pRes	Result
 @brief      		Solves 'nCnt' simultaneous equations of 'nCnt' variables.
					pRes should be an array large enough to contain the
					results: the values of the 'nCnt' variables.
					This fn recursively uses Gaussian Elimination.
*****************************************************************************/
void PVRTLinearEqSolve(VERTTYPE * const pRes, VERTTYPE ** const pSrc, const int nCnt);

/*!***************************************************************************
 @struct            PVRTVec2 
 @brief             2 component vector
*****************************************************************************/
struct PVRTVec2
{
	VERTTYPE x, y;
	/*!***************************************************************************
		** Constructors
		****************************************************************************/
	/*!***************************************************************************
		@brief      		Blank constructor.
		*****************************************************************************/
	PVRTVec2() : x(0), y(0) {}
	/*!***************************************************************************
		@brief      		Simple constructor from 2 values.
		@param[in]			fX	X component of vector
		@param[in]			fY	Y component of vector
		*****************************************************************************/
	PVRTVec2(VERTTYPE fX, VERTTYPE fY) : x(fX), y(fY) {}
	/*!***************************************************************************
		@brief      		Constructor from a single value.
		@param[in]			fValue	    A component value
		*****************************************************************************/
	PVRTVec2(VERTTYPE fValue) : x(fValue), y(fValue) {}
	/*!***************************************************************************
		@brief      		Constructor from an array
		@param[in]			pVec	An array
		*****************************************************************************/
	PVRTVec2(const VERTTYPE* pVec) : x(pVec[0]), y(pVec[1]) {}
	/*!***************************************************************************
		@brief      		Constructor from a Vec3
		@param[in]			v3Vec   A Vec3
		*****************************************************************************/
	PVRTVec2(const PVRTVec3& v3Vec);
	/*!***************************************************************************
		** Operators
		****************************************************************************/
	/*!***************************************************************************
		@brief      		componentwise addition operator for two Vec2s
		@param[in]			rhs     Another Vec2
		@return 			result of addition
		*****************************************************************************/
	PVRTVec2 operator+(const PVRTVec2& rhs) const
	{
		PVRTVec2 out(*this);
		return out += rhs;
	}
	/*!***************************************************************************
		@brief      		componentwise subtraction operator for two Vec2s
		@param[in]			rhs    Another vec2
		@return 			result of subtraction
		****************************************************************************/
	PVRTVec2 operator-(const PVRTVec2& rhs) const
	{
		PVRTVec2 out(*this);
		return out -= rhs;
	}

	/*!***************************************************************************
		@brief      		Componentwise addition and assignment operator for two Vec2s
		@param[in]			rhs    Another vec2
		@return 			result of addition
		****************************************************************************/
	PVRTVec2& operator+=(const PVRTVec2& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	/*!***************************************************************************
		@brief      		Componentwise subtraction and assignment operator for two Vec2s
		@param[in]			rhs    Another vec2
		@return 			Result of subtraction
		****************************************************************************/
	PVRTVec2& operator-=(const PVRTVec2& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	/*!***************************************************************************
		@brief      		Negation operator for a Vec2
		@param[in]			rhs    Another vec2
		@return 			Result of negation
		****************************************************************************/
	friend PVRTVec2 operator- (const PVRTVec2& rhs) { return PVRTVec2(-rhs.x, -rhs.y); }

	/*!***************************************************************************
		@brief      		Multiplication operator for a Vec2
		@param[in]			lhs     Scalar
		@param[in]			rhs     A Vec2
		@return 			result of multiplication
		****************************************************************************/
	friend PVRTVec2 operator*(const VERTTYPE lhs, const PVRTVec2&  rhs)
	{
		PVRTVec2 out(lhs);
		return out *= rhs;
	}

	/*!***************************************************************************
		@brief      		Division operator for scalar and Vec2
		@param[in]			lhs scalar
		@param[in]			rhs a Vec2
		@return 			Result of division
		****************************************************************************/
	friend PVRTVec2 operator/(const VERTTYPE lhs, const PVRTVec2&  rhs)
	{
		PVRTVec2 out(lhs);
		return out /= rhs;
	}

	/*!**************************************************************************
		@brief      		Componentwise multiplication by scalar for Vec2*
		@param[in]			rhs     A scalar
		@return 			Result of multiplication
		****************************************************************************/
	PVRTVec2 operator*(const VERTTYPE& rhs) const
	{
		PVRTVec2 out(*this);
		return out *= rhs;
	}

	/*!***************************************************************************
		@brief      		Componentwise multiplication and assignment by scalar for Vec2
		@param[in]			rhs     A scalar
		@return 			Result of multiplication and assignment
		****************************************************************************/
	PVRTVec2& operator*=(const VERTTYPE& rhs)
	{
		x = VERTTYPEMUL(x, rhs);
		y = VERTTYPEMUL(y, rhs);
		return *this;
	}

	/*!***************************************************************************
		@brief      		Componentwise multiplication and assignment by Vec2 for Vec2
		@param[in]			rhs     A Vec2
		@return 			Result of multiplication and assignment
		****************************************************************************/
	PVRTVec2& operator*=(const PVRTVec2& rhs)
	{
		x = VERTTYPEMUL(x, rhs.x);
		y = VERTTYPEMUL(y, rhs.y);
		return *this;
	}

	/*!***************************************************************************
		@brief      		componentwise division by scalar for Vec2
		@param[in]			rhs a scalar
		@return 			result of division
		****************************************************************************/
	PVRTVec2 operator/(const VERTTYPE& rhs) const
	{
		PVRTVec2 out(*this);
		return out /= rhs;
	}

	/*!***************************************************************************
		@brief      		componentwise division and assignment by scalar for Vec2
		@param[in]			rhs a scalar
		@return 			result of division and assignment
		****************************************************************************/
	PVRTVec2& operator/=(const VERTTYPE& rhs)
	{
		x = VERTTYPEDIV(x, rhs);
		y = VERTTYPEDIV(y, rhs);
		return *this;
	}

	/*!***************************************************************************
		@brief      		componentwise division and assignment by Vec2 for Vec2
		@param[in]			rhs a Vec2
		@return 			result of division and assignment
		****************************************************************************/
	PVRTVec2& operator/=(const PVRTVec2& rhs)
	{
		x = VERTTYPEDIV(x, rhs.x);
		y = VERTTYPEDIV(y, rhs.y);
		return *this;
	}

	/*!***************************************************************************
        @brief      		PVRTVec2 equality operator
        @param[in]			rhs     A single value
        @return 			true if the two vectors are equal
	****************************************************************************/
	bool operator==(const PVRTVec2& rhs) const
	{
		return ((x == rhs.x) && (y == rhs.y));
	}

	/*!***************************************************************************
        @brief      		PVRTVec2 inequality operator
        @param[in]			rhs     A single value
        @return 			true if the two vectors are not equal
	****************************************************************************/
	bool operator!=(const PVRTVec2& rhs) const
	{
		return ((x != rhs.x) || (y != rhs.y));
	}

	// FUNCTIONS
	/*!***************************************************************************
		@brief      		calculates the square of the magnitude of the vector
		@return 			The square of the magnitude of the vector
		****************************************************************************/
	VERTTYPE lenSqr() const
	{
		return VERTTYPEMUL(x,x)+VERTTYPEMUL(y,y);
	}

	/*!***************************************************************************
		@fn       			length
		@return 			the of the magnitude of the vector
		@brief      		calculates the magnitude of the vector
		****************************************************************************/
	VERTTYPE length() const
	{
		return (VERTTYPE) f2vt(sqrt(vt2f(x)*vt2f(x) + vt2f(y)*vt2f(y)));
	}

	/*!***************************************************************************
		@fn       			normalize
		@return 			the normalized value of the vector
		@brief      		normalizes the vector
		****************************************************************************/
	PVRTVec2 normalize()
	{
		return *this /= length();
	}

	/*!***************************************************************************
		@fn       			normalized
		@return 			returns the normalized value of the vector
		@brief      		returns a normalized vector of the same direction as this
		vector
		****************************************************************************/
	PVRTVec2 normalized() const
	{
		PVRTVec2 out(*this);
		return out.normalize();
	}

	/*!***************************************************************************
		@fn       			rotated90
		@return 			returns the vector rotated 90°
		@brief      		returns the vector rotated 90°
		****************************************************************************/
	PVRTVec2 rotated90() const
	{
		return PVRTVec2(-y, x);
	}

	/*!***************************************************************************
		@fn       			dot
		@param[in]			rhs    A single value
		@return 			scalar product
		@brief      		calculate the scalar product of two Vec3s
		****************************************************************************/
	VERTTYPE dot(const PVRTVec2& rhs) const
	{
		return VERTTYPEMUL(x, rhs.x) + VERTTYPEMUL(y, rhs.y);
	}

	/*!***************************************************************************
		@fn       			ptr
		@return 			pointer
		@brief      		returns a pointer to memory containing the values of the
		Vec3
		****************************************************************************/
	VERTTYPE *ptr() { return (VERTTYPE*)this; }
};

/*!***************************************************************************
 @struct            PVRTVec3 
 @brief             3 component vector
****************************************************************************/
struct PVRTVec3 : public PVRTVECTOR3
{
/*!***************************************************************************
** Constructors
****************************************************************************/
/*!***************************************************************************
 @brief      		Blank constructor.
*****************************************************************************/
	PVRTVec3()
	{
		x = y = z = 0;
	}
/*!***************************************************************************
 @brief      		Simple constructor from 3 values.
 @param[in]			fX	X component of vector
 @param[in]			fY	Y component of vector
 @param[in]			fZ	Z component of vector
*****************************************************************************/
	PVRTVec3(VERTTYPE fX, VERTTYPE fY, VERTTYPE fZ)
	{
		x = fX;	y = fY;	z = fZ;
	}
/*!***************************************************************************
 @brief      		Constructor from a single value.
 @param[in]			fValue	 A component value
*****************************************************************************/
	PVRTVec3(const VERTTYPE fValue)
	{
		x = fValue; y = fValue; z = fValue;
	}
/*!***************************************************************************
 @brief      		Constructor from an array
 @param[in]			pVec	An array
*****************************************************************************/
	PVRTVec3(const VERTTYPE* pVec)
	{
		x = (*pVec++); y = (*pVec++); z = *pVec;
	}
/*!***************************************************************************
 @brief      		Constructor from a PVRTVec4
 @param[in]			v4Vec   A PVRTVec4
*****************************************************************************/
	PVRTVec3(const PVRTVec4& v4Vec);
/*!***************************************************************************
** Operators
****************************************************************************/
/*!***************************************************************************
 @brief      		componentwise addition operator for two PVRTVec3s
 @param[in]			rhs     Another PVRTVec3
 @return 			result of addition
*****************************************************************************/
	PVRTVec3 operator+(const PVRTVec3& rhs) const
	{
		PVRTVec3 out;
		out.x = x+rhs.x;
		out.y = y+rhs.y;
		out.z = z+rhs.z;
		return out;
	}
/*!***************************************************************************
 @brief      		Componentwise subtraction operator for two PVRTVec3s
 @param[in]			rhs    Another PVRTVec3
 @return 			result of subtraction
****************************************************************************/
	PVRTVec3 operator-(const PVRTVec3& rhs) const
	{
		PVRTVec3 out;
		out.x = x-rhs.x;
		out.y = y-rhs.y;
		out.z = z-rhs.z;
		return out;
	}

/*!***************************************************************************
 @brief      		Componentwise addition and assignement operator for two PVRTVec3s
 @param[in]			rhs    Another PVRTVec3
 @return 			Result of addition
****************************************************************************/
	PVRTVec3& operator+=(const PVRTVec3& rhs)
	{
		x +=rhs.x;
		y +=rhs.y;
		z +=rhs.z;
		return *this;
	}

/*!***************************************************************************
 @brief      		Componentwise subtraction and assignement operator for two PVRTVec3s
 @param[in]			rhs    Another PVRTVec3
 @return 			Result of subtraction
****************************************************************************/
	PVRTVec3& operator-=(const PVRTVec3& rhs)
	{
		x -=rhs.x;
		y -=rhs.y;
		z -=rhs.z;
		return *this;
	}

/*!***************************************************************************
 @brief      		Negation operator for a PVRTVec3
 @param[in]			rhs    Another PVRTVec3
 @return 			Result of negation
****************************************************************************/
	friend PVRTVec3 operator - (const PVRTVec3& rhs) { return PVRTVec3(rhs) *= f2vt(-1); }

/*!***************************************************************************
 @brief      		multiplication operator for a PVRTVec3
 @param[in]			lhs     Single value
 @param[in]			rhs     A PVRTVec3
 @return 			Result of multiplication
****************************************************************************/
	friend PVRTVec3 operator*(const VERTTYPE lhs, const PVRTVec3&  rhs)
	{
		PVRTVec3 out;
		out.x = VERTTYPEMUL(lhs,rhs.x);
		out.y = VERTTYPEMUL(lhs,rhs.y);
		out.z = VERTTYPEMUL(lhs,rhs.z);
		return out;
	}

/*!***************************************************************************
 @brief      		Negation operator for a PVRTVec3
 @param[in]			lhs     Single value
 @param[in]			rhs     A PVRTVec3
 @return 			result of negation
****************************************************************************/
	friend PVRTVec3 operator/(const VERTTYPE lhs, const PVRTVec3&  rhs)
	{
		PVRTVec3 out;
		out.x = VERTTYPEDIV(lhs,rhs.x);
		out.y = VERTTYPEDIV(lhs,rhs.y);
		out.z = VERTTYPEDIV(lhs,rhs.z);
		return out;
	}

/*!***************************************************************************
 @brief      		Matrix multiplication operator PVRTVec3 and PVRTMat3
 @param[in]			rhs     A PVRTMat3
 @return 			Result of multiplication
****************************************************************************/
	PVRTVec3 operator*(const PVRTMat3& rhs) const;

/*!***************************************************************************
 @brief      		Matrix multiplication and assignment operator for PVRTVec3 and PVRTMat3
 @param[in]			rhs     A PVRTMat3
 @return 			Result of multiplication and assignment
****************************************************************************/
	PVRTVec3& operator*=(const PVRTMat3& rhs);

/*!***************************************************************************
 @brief      		Componentwise multiplication by single dimension value for PVRTVec3
 @param[in]			rhs     A single value
 @return 			Result of multiplication
****************************************************************************/
	PVRTVec3 operator*(const VERTTYPE& rhs) const
	{
		PVRTVec3 out;
		out.x = VERTTYPEMUL(x,rhs);
		out.y = VERTTYPEMUL(y,rhs);
		out.z = VERTTYPEMUL(z,rhs);
		return out;
	}

/*!***************************************************************************
 @brief      		Componentwise multiplication and assignement by single
					dimension value	for PVRTVec3
 @param[in]			rhs    A single value
 @return 			Result of multiplication and assignment
****************************************************************************/
	PVRTVec3& operator*=(const VERTTYPE& rhs)
	{
		x = VERTTYPEMUL(x,rhs);
		y = VERTTYPEMUL(y,rhs);
		z = VERTTYPEMUL(z,rhs);
		return *this;
	}

/*!***************************************************************************
 @brief      		Componentwise division by single dimension value for PVRTVec3
 @param[in]			rhs    A single value
 @return 			Result of division
****************************************************************************/
	PVRTVec3 operator/(const VERTTYPE& rhs) const
	{
		PVRTVec3 out;
		out.x = VERTTYPEDIV(x,rhs);
		out.y = VERTTYPEDIV(y,rhs);
		out.z = VERTTYPEDIV(z,rhs);
		return out;
	}

/*!***************************************************************************
 @brief      		Componentwise division and assignement by single
					dimension value	for PVRTVec3
 @param[in]			rhs    A single value
 @return 			Result of division and assignment
****************************************************************************/
	PVRTVec3& operator/=(const VERTTYPE& rhs)
	{
		x = VERTTYPEDIV(x,rhs);
		y = VERTTYPEDIV(y,rhs);
		z = VERTTYPEDIV(z,rhs);
		return *this;
	}

/*!***************************************************************************
 @brief      		PVRTVec3 equality operator
 @param[in]			rhs    A single value
 @return 			true if the two vectors are equal
****************************************************************************/
	bool operator==(const PVRTVec3& rhs) const
	{
		return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z));
	}

/*!***************************************************************************
 @brief      		PVRTVec3 inequality operator
 @param[in]			rhs    A single value
 @return 			true if the two vectors are not equal
	****************************************************************************/
	bool operator!=(const PVRTVec3& rhs) const
	{
		return ((x != rhs.x) || (y != rhs.y) || (z != rhs.z));
	}
	// FUNCTIONS
/*!***************************************************************************
 @fn       			lenSqr
 @return 			the square of the magnitude of the vector
 @brief      		calculates the square of the magnitude of the vector
****************************************************************************/
	VERTTYPE lenSqr() const
	{
		return VERTTYPEMUL(x,x)+VERTTYPEMUL(y,y)+VERTTYPEMUL(z,z);
	}

/*!***************************************************************************
 @fn       			length
 @return 			the of the magnitude of the vector
 @brief      		calculates the magnitude of the vector
****************************************************************************/
	VERTTYPE length() const
	{
		return (VERTTYPE) f2vt(sqrt(vt2f(x)*vt2f(x) + vt2f(y)*vt2f(y) + vt2f(z)*vt2f(z)));
	}

/*!***************************************************************************
 @fn       			normalize
 @return 			the normalized value of the vector
 @brief      		normalizes the vector
****************************************************************************/
	PVRTVec3 normalize()
	{
#if defined(PVRT_FIXED_POINT_ENABLE)
		// Scale vector by uniform value
		int n = PVRTABS(x) + PVRTABS(y) + PVRTABS(z);
		x = VERTTYPEDIV(x, n);
		y = VERTTYPEDIV(y, n);
		z = VERTTYPEDIV(z, n);

		// Calculate x2+y2+z2/sqrt(x2+y2+z2)
		int f = dot(*this);
		f = VERTTYPEDIV(PVRTF2X(1.0f), PVRTF2X(sqrt(PVRTX2F(f))));

		// Multiply vector components by f
		x = PVRTXMUL(x, f);
		y = PVRTXMUL(y, f);
		z = PVRTXMUL(z, f);
#else
		VERTTYPE len = length();
		x =VERTTYPEDIV(x,len);
		y =VERTTYPEDIV(y,len);
		z =VERTTYPEDIV(z,len);
#endif
		return *this;
	}

/*!***************************************************************************
 @fn       			normalized
 @return 			returns the normalized value of the vector
 @brief      		returns a normalized vector of the same direction as this
					vector
****************************************************************************/
	PVRTVec3 normalized() const
	{
		PVRTVec3 out;
#if defined(PVRT_FIXED_POINT_ENABLE)
		// Scale vector by uniform value
		int n = PVRTABS(x) + PVRTABS(y) + PVRTABS(z);
		out.x = VERTTYPEDIV(x, n);
		out.y = VERTTYPEDIV(y, n);
		out.z = VERTTYPEDIV(z, n);

		// Calculate x2+y2+z2/sqrt(x2+y2+z2)
		int f = out.dot(out);
		f = VERTTYPEDIV(PVRTF2X(1.0f), PVRTF2X(sqrt(PVRTX2F(f))));

		// Multiply vector components by f
		out.x = PVRTXMUL(out.x, f);
		out.y = PVRTXMUL(out.y, f);
		out.z = PVRTXMUL(out.z, f);
#else
		VERTTYPE len = length();
		out.x =VERTTYPEDIV(x,len);
		out.y =VERTTYPEDIV(y,len);
		out.z =VERTTYPEDIV(z,len);
#endif
		return out;
	}

/*!***************************************************************************
 @fn       			dot
 @param[in]			rhs    A single value
 @return 			scalar product
 @brief      		calculate the scalar product of two PVRTVec3s
****************************************************************************/
	VERTTYPE dot(const PVRTVec3& rhs) const
	{
		return VERTTYPEMUL(x,rhs.x)+VERTTYPEMUL(y,rhs.y)+VERTTYPEMUL(z,rhs.z);
	}

/*!***************************************************************************
 @fn       			cross
 @return 			returns three-dimensional vector
 @brief      		calculate the cross product of two PVRTVec3s
****************************************************************************/
	PVRTVec3 cross(const PVRTVec3& rhs) const
	{
		PVRTVec3 out;
		out.x = VERTTYPEMUL(y,rhs.z)-VERTTYPEMUL(z,rhs.y);
		out.y = VERTTYPEMUL(z,rhs.x)-VERTTYPEMUL(x,rhs.z);
		out.z = VERTTYPEMUL(x,rhs.y)-VERTTYPEMUL(y,rhs.x);
		return out;
	}

/*!***************************************************************************
 @fn       			ptr
 @return 			pointer
 @brief      		returns a pointer to memory containing the values of the
					PVRTVec3
****************************************************************************/
	VERTTYPE *ptr() { return (VERTTYPE*)this; }
};

/*!***************************************************************************
 @struct            PVRTVec4 
 @brief             4 component vector
****************************************************************************/
struct PVRTVec4 : public PVRTVECTOR4
{
/*!***************************************************************************
** Constructors
****************************************************************************/
/*!***************************************************************************
 @brief      	Blank constructor.
*****************************************************************************/
	PVRTVec4(){}

/*!***************************************************************************
 @brief      	Blank constructor.
*****************************************************************************/
	PVRTVec4(const VERTTYPE vec)
	{
		x = vec; y = vec; z = vec; w = vec;
	}

/*!***************************************************************************
 @brief      	Constructs a PVRTVec4 from 4 separate values
 @param[in]		fX      Value of x component
 @param[in]		fY      Value of y component
 @param[in]		fZ      Value of z component
 @param[in]		fW      Value of w component
****************************************************************************/
	PVRTVec4(VERTTYPE fX, VERTTYPE fY, VERTTYPE fZ, VERTTYPE fW)
	{
		x = fX;	y = fY;	z = fZ;	w = fW;
	}

/*!***************************************************************************
 @param[in]			vec3 a PVRTVec3
 @param[in]			fW     Value of w component
 @brief      		Constructs a PVRTVec4 from a vec3 and a w component
****************************************************************************/
	PVRTVec4(const PVRTVec3& vec3, VERTTYPE fW)
	{
		x = vec3.x;	y = vec3.y;	z = vec3.z;	w = fW;
	}

/*!***************************************************************************
 @brief      		Constructs a vec4 from a vec3 and a w component
 @param[in]			fX value of x component
 @param[in]			vec3 a PVRTVec3
****************************************************************************/
	PVRTVec4(VERTTYPE fX, const PVRTVec3& vec3)
	{
		x = fX;	y = vec3.x;	z = vec3.y;	w = vec3.z;
	}

/*!***************************************************************************
 @brief      		Constructs a PVRTVec4 from a pointer to an array of four values
 @param[in]			pVec a pointer to an array of four values
****************************************************************************/
	PVRTVec4(const VERTTYPE* pVec)
	{
		x = (*pVec++); y = (*pVec++); z= (*pVec++); w = *pVec;
	}

/*!***************************************************************************
** PVRTVec4 Operators
****************************************************************************/
/*!***************************************************************************
 @brief      		Addition operator for PVRTVec4
 @param[in]			rhs    Another PVRTVec4
 @return 			result of addition
****************************************************************************/
	PVRTVec4 operator+(const PVRTVec4& rhs) const
	{
		PVRTVec4 out;
		out.x = x+rhs.x;
		out.y = y+rhs.y;
		out.z = z+rhs.z;
		out.w = w+rhs.w;
		return out;
	}

/*!***************************************************************************
 @brief      		Subtraction operator for PVRTVec4
 @param[in]			rhs    Another PVRTVec4
 @return 			result of subtraction
****************************************************************************/
	PVRTVec4 operator-(const PVRTVec4& rhs) const
	{
		PVRTVec4 out;
		out.x = x-rhs.x;
		out.y = y-rhs.y;
		out.z = z-rhs.z;
		out.w = w-rhs.w;
		return out;
	}

/*!***************************************************************************
 @brief      		Addition and assignment operator for PVRTVec4
 @param[in]			rhs    Another PVRTVec4
 @return 			result of addition and assignment
****************************************************************************/
	PVRTVec4& operator+=(const PVRTVec4& rhs)
	{
		x +=rhs.x;
		y +=rhs.y;
		z +=rhs.z;
		w +=rhs.w;
		return *this;
	}

/*!***************************************************************************
 @brief      		Subtraction and assignment operator for PVRTVec4
 @param[in]			rhs    Another PVRTVec4
 @return 			result of subtraction and assignment
****************************************************************************/
	PVRTVec4& operator-=(const PVRTVec4& rhs)
	{
		x -=rhs.x;
		y -=rhs.y;
		z -=rhs.z;
		w -=rhs.w;
		return *this;
	}

/*!***************************************************************************
 @brief      		Matrix multiplication for PVRTVec4 and PVRTMat4
 @param[in]			rhs    A PVRTMat4
 @return 			result of multiplication
****************************************************************************/
	PVRTVec4 operator*(const PVRTMat4& rhs) const;

/*!***************************************************************************
 @brief      		Matrix multiplication and assignment for PVRTVec4 and PVRTMat4
 @param[in]			rhs    A PVRTMat4
 @return 			result of multiplication and assignement
****************************************************************************/
	PVRTVec4& operator*=(const PVRTMat4& rhs);

/*!***************************************************************************
 @brief      		Componentwise multiplication of a PVRTVec4 by a single value
 @param[in]			rhs  A single dimension value
 @return 			result of multiplication
****************************************************************************/
	PVRTVec4 operator*(const VERTTYPE& rhs) const
	{
		PVRTVec4 out;
		out.x = VERTTYPEMUL(x,rhs);
		out.y = VERTTYPEMUL(y,rhs);
		out.z = VERTTYPEMUL(z,rhs);
		out.w = VERTTYPEMUL(w,rhs);
		return out;
	}

/*!***************************************************************************
 @brief      		componentwise multiplication and assignment of a PVRTVec4 by
                    a single value
 @param[in]			rhs     A single dimension value
 @return 			result of multiplication and assignment
****************************************************************************/
	PVRTVec4& operator*=(const VERTTYPE& rhs)
	{
		x = VERTTYPEMUL(x,rhs);
		y = VERTTYPEMUL(y,rhs);
		z = VERTTYPEMUL(z,rhs);
		w = VERTTYPEMUL(w,rhs);
		return *this;
	}

/*!***************************************************************************
 @brief      		componentwise division of a PVRTVec4 by a single value
 @param[in]			rhs     A single dimension value
 @return 			result of division
****************************************************************************/
	PVRTVec4 operator/(const VERTTYPE& rhs) const
	{
		PVRTVec4 out;
		out.x = VERTTYPEDIV(x,rhs);
		out.y = VERTTYPEDIV(y,rhs);
		out.z = VERTTYPEDIV(z,rhs);
		out.w = VERTTYPEDIV(w,rhs);
		return out;
	}

/*!***************************************************************************
 @brief      		componentwise division and assignment of a PVRTVec4 by
					a single value
 @param[in]				rhs a single dimension value
 @return 			result of division and assignment
****************************************************************************/
	PVRTVec4& operator/=(const VERTTYPE& rhs)
	{
		x = VERTTYPEDIV(x,rhs);
		y = VERTTYPEDIV(y,rhs);
		z = VERTTYPEDIV(z,rhs);
		w = VERTTYPEDIV(w,rhs);
		return *this;
	}

/*!***************************************************************************
 @brief      		componentwise multiplication of a PVRTVec4 by
					a single value
 @param[in]				lhs a single dimension value
 @param[in]				rhs a PVRTVec4
 @return 			result of muliplication
****************************************************************************/
friend PVRTVec4 operator*(const VERTTYPE lhs, const PVRTVec4&  rhs)
{
	PVRTVec4 out;
	out.x = VERTTYPEMUL(lhs,rhs.x);
	out.y = VERTTYPEMUL(lhs,rhs.y);
	out.z = VERTTYPEMUL(lhs,rhs.z);
	out.w = VERTTYPEMUL(lhs,rhs.w);
	return out;
}

/*!***************************************************************************
 @brief      		PVRTVec4 equality operator
 @param[in]			rhs    A single value
 @return 			true if the two vectors are equal
****************************************************************************/
bool operator==(const PVRTVec4& rhs) const
{
	return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

/*!***************************************************************************
@brief      		PVRTVec4 inequality operator
@param[in]			rhs    A single value
@return 			true if the two vectors are not equal
	****************************************************************************/
bool operator!=(const PVRTVec4& rhs) const
{
	return ((x != rhs.x) || (y != rhs.y) || (z != rhs.z) || (w != rhs.w));
}
/*!***************************************************************************
** Functions
****************************************************************************/
/*!***************************************************************************
 @fn       			lenSqr
 @return 			square of the magnitude of the vector
 @brief      		calculates the square of the magnitude of the vector
****************************************************************************/
	VERTTYPE lenSqr() const
	{
		return VERTTYPEMUL(x,x)+VERTTYPEMUL(y,y)+VERTTYPEMUL(z,z)+VERTTYPEMUL(w,w);
	}

/*!***************************************************************************
 @fn       			length
 @return 			the magnitude of the vector
 @brief      		calculates the magnitude of the vector
****************************************************************************/
	VERTTYPE length() const
	{
		return (VERTTYPE) f2vt(sqrt(vt2f(x)*vt2f(x) + vt2f(y)*vt2f(y) + vt2f(z)*vt2f(z) + vt2f(w)*vt2f(w)));
	}

/*!***************************************************************************
 @fn       			normalize
 @return 			normalized vector
 @brief      		calculates the normalized value of a PVRTVec4
****************************************************************************/
	PVRTVec4 normalize()
	{
		VERTTYPE len = length();
		x =VERTTYPEDIV(x,len);
		y =VERTTYPEDIV(y,len);
		z =VERTTYPEDIV(z,len);
		w =VERTTYPEDIV(w,len);
		return *this;
	}
/*!***************************************************************************
 @fn       			normalized
 @return 			normalized vector
 @brief      		returns a normalized vector of the same direction as this
					vector
****************************************************************************/
	PVRTVec4 normalized() const
	{
		PVRTVec4 out;
		VERTTYPE len = length();
		out.x =VERTTYPEDIV(x,len);
		out.y =VERTTYPEDIV(y,len);
		out.z =VERTTYPEDIV(z,len);
		out.w =VERTTYPEDIV(w,len);
		return out;
	}

/*!***************************************************************************
 @fn       			dot
 @return 			scalar product
 @brief      		returns a normalized vector of the same direction as this
					vector
****************************************************************************/
	VERTTYPE dot(const PVRTVec4& rhs) const
	{
		return VERTTYPEMUL(x,rhs.x)+VERTTYPEMUL(y,rhs.y)+VERTTYPEMUL(z,rhs.z)+VERTTYPEMUL(w,rhs.w);
	}

/*!***************************************************************************
 @fn       			ptr
 @return 			pointer to vector values
 @brief      		returns a pointer to memory containing the values of the
					PVRTVec3
****************************************************************************/
	VERTTYPE *ptr() { return (VERTTYPE*)this; }
};

/*!***************************************************************************
 @struct            PVRTMat3
 @brief             3x3 Matrix
****************************************************************************/
struct PVRTMat3 : public PVRTMATRIX3
{
/*!***************************************************************************
** Constructors
****************************************************************************/
/*!***************************************************************************
 @brief      		Blank constructor.
*****************************************************************************/
	PVRTMat3(){}
/*!***************************************************************************
 @brief      		Constructor from array.
 @param[in]			pMat    An array of values for the matrix
*****************************************************************************/
	PVRTMat3(const VERTTYPE* pMat)
	{
		VERTTYPE* ptr = f;
		for(int i=0;i<9;i++)
		{
			(*ptr++)=(*pMat++);
		}
	}

/*!***************************************************************************
 @brief      	    Constructor from distinct values.
 @param[in]	    	m0	m0 matrix value
 @param[in]	    	m1	m1 matrix value
 @param[in]	    	m2	m2 matrix value
 @param[in]	    	m3	m3 matrix value
 @param[in]	    	m4	m4 matrix value
 @param[in]	    	m5	m5 matrix value
 @param[in]	    	m6	m6 matrix value
 @param[in]	    	m7	m7 matrix value
 @param[in]	    	m8	m8 matrix value
*****************************************************************************/
	PVRTMat3(VERTTYPE m0,VERTTYPE m1,VERTTYPE m2,
		VERTTYPE m3,VERTTYPE m4,VERTTYPE m5,
		VERTTYPE m6,VERTTYPE m7,VERTTYPE m8)
	{
		f[0]=m0;f[1]=m1;f[2]=m2;
		f[3]=m3;f[4]=m4;f[5]=m5;
		f[6]=m6;f[7]=m7;f[8]=m8;
	}

/*!***************************************************************************
 @brief      		Constructor from 4x4 matrix - uses top left values
 @param[in]			mat - a PVRTMat4
*****************************************************************************/
	PVRTMat3(const PVRTMat4& mat);

/****************************************************************************
** PVRTMat3 OPERATORS
****************************************************************************/
/*!***************************************************************************
 @brief      		Returns the value of the element at the specified row and column
					of the PVRTMat3
 @param[in]			row			row of matrix
 @param[in]			column		column of matrix
 @return 			value of element
*****************************************************************************/
	VERTTYPE& operator()(const int row, const int column)
	{
		return f[column*3+row];
	}
/*!***************************************************************************
 @brief      		Returns the value of the element at the specified row and column
					of the PVRTMat3
 @param[in]			row			row of matrix
 @param[in]			column		column of matrix
 @return 			value of element
*****************************************************************************/
	const VERTTYPE& operator()(const int row, const int column) const
	{
		return f[column*3+row];
	}

/*!***************************************************************************
 @brief      		Matrix multiplication of two 3x3 matrices.
 @param[in]			rhs     Another PVRTMat3
 @return 			result of multiplication
*****************************************************************************/
	PVRTMat3 operator*(const PVRTMat3& rhs) const
	{
		PVRTMat3 out;
		// col 1
		out.f[0] =	VERTTYPEMUL(f[0],rhs.f[0])+VERTTYPEMUL(f[3],rhs.f[1])+VERTTYPEMUL(f[6],rhs.f[2]);
		out.f[1] =	VERTTYPEMUL(f[1],rhs.f[0])+VERTTYPEMUL(f[4],rhs.f[1])+VERTTYPEMUL(f[7],rhs.f[2]);
		out.f[2] =	VERTTYPEMUL(f[2],rhs.f[0])+VERTTYPEMUL(f[5],rhs.f[1])+VERTTYPEMUL(f[8],rhs.f[2]);

		// col 2
		out.f[3] =	VERTTYPEMUL(f[0],rhs.f[3])+VERTTYPEMUL(f[3],rhs.f[4])+VERTTYPEMUL(f[6],rhs.f[5]);
		out.f[4] =	VERTTYPEMUL(f[1],rhs.f[3])+VERTTYPEMUL(f[4],rhs.f[4])+VERTTYPEMUL(f[7],rhs.f[5]);
		out.f[5] =	VERTTYPEMUL(f[2],rhs.f[3])+VERTTYPEMUL(f[5],rhs.f[4])+VERTTYPEMUL(f[8],rhs.f[5]);

		// col3
		out.f[6] =	VERTTYPEMUL(f[0],rhs.f[6])+VERTTYPEMUL(f[3],rhs.f[7])+VERTTYPEMUL(f[6],rhs.f[8]);
		out.f[7] =	VERTTYPEMUL(f[1],rhs.f[6])+VERTTYPEMUL(f[4],rhs.f[7])+VERTTYPEMUL(f[7],rhs.f[8]);
		out.f[8] =	VERTTYPEMUL(f[2],rhs.f[6])+VERTTYPEMUL(f[5],rhs.f[7])+VERTTYPEMUL(f[8],rhs.f[8]);
		return out;
	}

/*!***************************************************************************
 @brief      		element by element addition operator.
 @param[in]			rhs     Another PVRTMat3
 @return 			result of addition
*****************************************************************************/
	PVRTMat3 operator+(const PVRTMat3& rhs) const
	{
		PVRTMat3 out;
		VERTTYPE const *lptr = f, *rptr = rhs.f;
		VERTTYPE *outptr = out.f;
		for(int i=0;i<9;i++)
		{
			(*outptr++) = (*lptr++) + (*rptr++);
		}
		return out;
	}

/*!***************************************************************************
 @brief      		element by element subtraction operator.
 @param[in]			rhs     Another PVRTMat3
 @return 			result of subtraction
*****************************************************************************/
	PVRTMat3 operator-(const PVRTMat3& rhs) const
	{
		PVRTMat3 out;
		VERTTYPE const *lptr = f, *rptr = rhs.f;
		VERTTYPE *outptr = out.f;
		for(int i=0;i<9;i++)
		{
			(*outptr++) = (*lptr++) - (*rptr++);
		}
		return out;
	}

/*!***************************************************************************
 @brief      		Element by element addition and assignment operator.
 @param[in]			rhs     Another PVRTMat3
 @return 			Result of addition and assignment
*****************************************************************************/
	PVRTMat3& operator+=(const PVRTMat3& rhs)
	{
		VERTTYPE *lptr = f;
		VERTTYPE const *rptr = rhs.f;
		for(int i=0;i<9;i++)
		{
			(*lptr++) += (*rptr++);
		}
		return *this;
	}

/*!***************************************************************************
 @brief      		element by element subtraction and assignment operator.
 @param[in]			rhs     Another PVRTMat3
 @return 			result of subtraction and assignment
*****************************************************************************/
	PVRTMat3& operator-=(const PVRTMat3& rhs)
	{
		VERTTYPE *lptr = f;
		VERTTYPE const *rptr = rhs.f;
		for(int i=0;i<9;i++)
		{
			(*lptr++) -= (*rptr++);
		}
		return *this;
	}

/*!***************************************************************************
 @brief      		Matrix multiplication and assignment of two 3x3 matrices.
 @param[in]			rhs     Another PVRTMat3
 @return 			result of multiplication and assignment
*****************************************************************************/
	PVRTMat3& operator*=(const PVRTMat3& rhs)
	{
		PVRTMat3 out;
		// col 1
		out.f[0] =	VERTTYPEMUL(f[0],rhs.f[0])+VERTTYPEMUL(f[3],rhs.f[1])+VERTTYPEMUL(f[6],rhs.f[2]);
		out.f[1] =	VERTTYPEMUL(f[1],rhs.f[0])+VERTTYPEMUL(f[4],rhs.f[1])+VERTTYPEMUL(f[7],rhs.f[2]);
		out.f[2] =	VERTTYPEMUL(f[2],rhs.f[0])+VERTTYPEMUL(f[5],rhs.f[1])+VERTTYPEMUL(f[8],rhs.f[2]);

		// col 2
		out.f[3] =	VERTTYPEMUL(f[0],rhs.f[3])+VERTTYPEMUL(f[3],rhs.f[4])+VERTTYPEMUL(f[6],rhs.f[5]);
		out.f[4] =	VERTTYPEMUL(f[1],rhs.f[3])+VERTTYPEMUL(f[4],rhs.f[4])+VERTTYPEMUL(f[7],rhs.f[5]);
		out.f[5] =	VERTTYPEMUL(f[2],rhs.f[3])+VERTTYPEMUL(f[5],rhs.f[4])+VERTTYPEMUL(f[8],rhs.f[5]);

		// col3
		out.f[6] =	VERTTYPEMUL(f[0],rhs.f[6])+VERTTYPEMUL(f[3],rhs.f[7])+VERTTYPEMUL(f[6],rhs.f[8]);
		out.f[7] =	VERTTYPEMUL(f[1],rhs.f[6])+VERTTYPEMUL(f[4],rhs.f[7])+VERTTYPEMUL(f[7],rhs.f[8]);
		out.f[8] =	VERTTYPEMUL(f[2],rhs.f[6])+VERTTYPEMUL(f[5],rhs.f[7])+VERTTYPEMUL(f[8],rhs.f[8]);
		*this = out;
		return *this;
	}

/*!***************************************************************************
 @brief      		Element multiplication by a single value.
 @param[in]			rhs    A single value
 @return 			Result of multiplication and assignment
*****************************************************************************/
	PVRTMat3& operator*(const VERTTYPE rhs)
	{
		for (int i=0; i<9; ++i)
		{
			f[i]*=rhs;
		}
		return *this;
	}
/*!***************************************************************************
 @brief      		Element multiplication and assignment by a single value.
 @param[in]			rhs    A single value
 @return 			result of multiplication and assignment
*****************************************************************************/
	PVRTMat3& operator*=(const VERTTYPE rhs)
	{
		for (int i=0; i<9; ++i)
		{
			f[i]*=rhs;
		}
		return *this;
	}

/*!***************************************************************************
 @brief      		Matrix multiplication of 3x3 matrix and vec3
 @param[in]			rhs    Another PVRTVec3
 @return 			result of multiplication
*****************************************************************************/
	PVRTVec3 operator*(const PVRTVec3& rhs) const
	{
		PVRTVec3 out;
		out.x = VERTTYPEMUL(rhs.x,f[0])+VERTTYPEMUL(rhs.y,f[3])+VERTTYPEMUL(rhs.z,f[6]);
		out.y = VERTTYPEMUL(rhs.x,f[1])+VERTTYPEMUL(rhs.y,f[4])+VERTTYPEMUL(rhs.z,f[7]);
		out.z = VERTTYPEMUL(rhs.x,f[2])+VERTTYPEMUL(rhs.y,f[5])+VERTTYPEMUL(rhs.z,f[8]);

		return out;
	}


	// FUNCTIONS
/*!***************************************************************************
** Functions
*****************************************************************************/
/*!***************************************************************************
 @fn       			determinant
 @return 			result of multiplication
 @brief      		Matrix multiplication and assignment of 3x3 matrix and vec3
*****************************************************************************/
	VERTTYPE determinant() const
	{
		return VERTTYPEMUL(f[0],(VERTTYPEMUL(f[4],f[8])-VERTTYPEMUL(f[7],f[5])))
			- VERTTYPEMUL(f[3],(VERTTYPEMUL(f[1],f[8])-VERTTYPEMUL(f[7],f[2])))
			+ VERTTYPEMUL(f[6],(VERTTYPEMUL(f[1],f[5])-VERTTYPEMUL(f[4],f[2])));
	}

/*!***************************************************************************
 @fn       			inverse
 @return 			inverse mat3
 @brief      		Calculates multiplicative inverse of this matrix
*****************************************************************************/
	PVRTMat3 inverse() const
	{
		PVRTMat3 out;


		VERTTYPE recDet = determinant();
		_ASSERT(recDet!=0);
		recDet = VERTTYPEDIV(f2vt(1.0f),recDet);

		//TODO: deal with singular matrices with more than just an assert

		// inverse is 1/det * adjoint of M

		// adjoint is transpose of cofactor matrix

		// do transpose and cofactors in one step

		out.f[0] = VERTTYPEMUL(f[4],f[8]) - VERTTYPEMUL(f[5],f[7]);
		out.f[1] = VERTTYPEMUL(f[2],f[7]) - VERTTYPEMUL(f[1],f[8]);
		out.f[2] = VERTTYPEMUL(f[1],f[5]) - VERTTYPEMUL(f[2],f[4]);

		out.f[3] = VERTTYPEMUL(f[5],f[6]) - VERTTYPEMUL(f[3],f[8]);
		out.f[4] = VERTTYPEMUL(f[0],f[8]) - VERTTYPEMUL(f[2],f[6]);
		out.f[5] = VERTTYPEMUL(f[2],f[3]) - VERTTYPEMUL(f[0],f[5]);

		out.f[6] = VERTTYPEMUL(f[3],f[7]) - VERTTYPEMUL(f[4],f[6]);
		out.f[7] = VERTTYPEMUL(f[1],f[6]) - VERTTYPEMUL(f[0],f[7]);
		out.f[8] = VERTTYPEMUL(f[0],f[4]) - VERTTYPEMUL(f[1],f[3]);

		out *= recDet;
		return out;
	}

/*!***************************************************************************
 @fn       			transpose
 @return 			transpose 3x3 matrix
 @brief      		Calculates the transpose of this matrix
*****************************************************************************/
	PVRTMat3 transpose() const
	{
		PVRTMat3 out;
		out.f[0] = f[0];	out.f[3] = f[1];	out.f[6] = f[2];
		out.f[1] = f[3];	out.f[4] = f[4];	out.f[7] = f[5];
		out.f[2] = f[6];	out.f[5] = f[7];	out.f[8] = f[8];
		return out;
	}

/*!***************************************************************************
 @fn       			ptr
 @return 			pointer to an array of the elements of this PVRTMat3
 @brief      		Calculates transpose of this matrix
*****************************************************************************/
	VERTTYPE *ptr() { return (VERTTYPE*)&f; }

/*!***************************************************************************
** Static factory functions
*****************************************************************************/
/*!***************************************************************************
 @fn       			Identity
 @return 			a PVRTMat3 representation of the 3x3 identity matrix
 @brief      		Generates an identity matrix
*****************************************************************************/
	static PVRTMat3 Identity()
	{
		PVRTMat3 out;
		out.f[0] = 1;out.f[1] = 0;out.f[2] = 0;
		out.f[3] = 0;out.f[4] = 1;out.f[5] = 0;
		out.f[6] = 0;out.f[7] = 0;out.f[8] = 1;
		return out;
	}

/*!***************************************************************************
 @fn       			RotationX
 @return 			a PVRTMat3 corresponding to the requested rotation
 @brief      		Calculates a matrix corresponding to a rotation of angle
					degrees about the X axis
*****************************************************************************/
	static PVRTMat3 RotationX(VERTTYPE angle);

/*!***************************************************************************
 @fn       			RotationY
 @return 			a PVRTMat3 corresponding to the requested rotation
 @brief      		Calculates a matrix corresponding to a rotation of angle
					degrees about the Y axis
*****************************************************************************/
	static PVRTMat3 RotationY(VERTTYPE angle);

/*!***************************************************************************
 @fn       			RotationZ
 @return 			a PVRTMat3 corresponding to the requested rotation
 @brief      		Calculates a matrix corresponding to a rotation of angle
					degrees about the Z axis
*****************************************************************************/
	static PVRTMat3 RotationZ(VERTTYPE angle);

/*!***************************************************************************
 @fn       			Rotation2D
 @return 			a PVRTMat3 corresponding to the requested rotation
 @brief      		Calculates a matrix corresponding to a rotation of angle
					degrees about the Z axis
*****************************************************************************/
	static PVRTMat3 Rotation2D(VERTTYPE angle)
	{
		return RotationZ(angle);
	}

/*!***************************************************************************
 @fn       			Scale
 @return 			a PVRTMat3 corresponding to the requested scaling transformation
 @brief      		Calculates a matrix corresponding to scaling of fx, fy and fz
					times in each axis.
*****************************************************************************/
	static PVRTMat3 Scale(const VERTTYPE fx,const VERTTYPE fy,const VERTTYPE fz)
	{
		return PVRTMat3(fx,0,0,
						0,fy,0,
						0,0,fz);
	}

/*!***************************************************************************
 @fn       			Scale2D
 @return 			a PVRTMat3 corresponding to the requested scaling transformation
 @brief      		Calculates a matrix corresponding to scaling of fx, fy and fz
					times in each axis.
*****************************************************************************/
	static PVRTMat3 Scale2D(const VERTTYPE fx,const VERTTYPE fy)
	{
		return PVRTMat3(fx,0,0,
						0,fy,0,
						0,0,f2vt(1));
	}

/*!***************************************************************************
 @fn       			Translation2D
 @return 			a PVRTMat3 corresponding to the requested translation
 @brief      		Calculates a matrix corresponding to a transformation
					of tx and ty times in each axis.
*****************************************************************************/
	static PVRTMat3 Translation2D(const VERTTYPE tx, const VERTTYPE ty)
	{
		return PVRTMat3( f2vt(1),    0,  0,
						 0,    f2vt(1),  0,
						tx,  		ty,  f2vt(1));
	}

};

/*!***************************************************************************
 @struct            PVRTMat4
 @brief             4x4 Matrix
****************************************************************************/
struct PVRTMat4 : public PVRTMATRIX
{
/*!***************************************************************************
** Constructors
****************************************************************************/
/*!***************************************************************************
 @brief      		Blank constructor.
*****************************************************************************/
	PVRTMat4(){}
/*!***************************************************************************
 @brief      		Constructor from array.
 @param[in]			m0	m0 matrix value
 @param[in]			m1	m1 matrix value
 @param[in]			m2	m2 matrix value
 @param[in]			m3	m3 matrix value
 @param[in]			m4	m4 matrix value
 @param[in]			m5	m5 matrix value
 @param[in]			m6	m6 matrix value
 @param[in]			m7	m7 matrix value
 @param[in]			m8	m8 matrix value
 @param[in]			m9	m9 matrix value
 @param[in]			m10	m10 matrix value
 @param[in]			m11	m11 matrix value
 @param[in]			m12	m12 matrix value
 @param[in]			m13	m13 matrix value
 @param[in]			m14	m14 matrix value
 @param[in]			m15	m15 matrix value
*****************************************************************************/
	PVRTMat4(VERTTYPE m0,VERTTYPE m1,VERTTYPE m2,VERTTYPE m3,
		VERTTYPE m4,VERTTYPE m5,VERTTYPE m6,VERTTYPE m7,
		VERTTYPE m8,VERTTYPE m9,VERTTYPE m10,VERTTYPE m11,
		VERTTYPE m12,VERTTYPE m13,VERTTYPE m14,VERTTYPE m15)
	{
		f[0]=m0;f[1]=m1;f[2]=m2;f[3]=m3;
		f[4]=m4;f[5]=m5;f[6]=m6;f[7]=m7;
		f[8]=m8;f[9]=m9;f[10]=m10;f[11]=m11;
		f[12]=m12;f[13]=m13;f[14]=m14;f[15]=m15;
	}
/*!***************************************************************************
 @brief      		Constructor from distinct values.
 @param[in]			mat     A pointer to an array of 16 VERTTYPEs
*****************************************************************************/
	PVRTMat4(const VERTTYPE* mat)
	{
		VERTTYPE* ptr = f;
		for(int i=0;i<16;i++)
		{
			(*ptr++)=(*mat++);
		}
	}

/****************************************************************************
** PVRTMat4 OPERATORS
****************************************************************************/
/*!***************************************************************************
 @brief      		Returns value of the element at row r and colun c of the
					PVRTMat4
 @param[in]				r - row of matrix
 @param[in]				c - column of matrix
 @return 			value of element
*****************************************************************************/
	VERTTYPE& operator()(const int r, const int c)
	{
		return f[c*4+r];
	}

/*!***************************************************************************
 @brief      		Returns value of the element at row r and colun c of the
					PVRTMat4
 @param[in]				r - row of matrix
 @param[in]				c - column of matrix
 @return 			value of element
*****************************************************************************/
	const VERTTYPE& operator()(const int r, const int c) const
	{
		return f[c*4+r];
	}

/*!***************************************************************************
 @brief      		Matrix multiplication of two 4x4 matrices.
 @param[in]				rhs another PVRTMat4
 @return 			result of multiplication
*****************************************************************************/
	PVRTMat4 operator*(const PVRTMat4& rhs) const;

/*!***************************************************************************
 @brief      		element by element addition operator.
 @param[in]				rhs another PVRTMat4
 @return 			result of addition
*****************************************************************************/
	PVRTMat4 operator+(const PVRTMat4& rhs) const
	{
		PVRTMat4 out;
		VERTTYPE const *lptr = f, *rptr = rhs.f;
		VERTTYPE *outptr = out.f;
		for(int i=0;i<16;i++)
		{
			(*outptr++) = (*lptr++) + (*rptr++);
		}
		return out;
	}

/*!***************************************************************************
 @brief      		element by element subtraction operator.
 @param[in]				rhs another PVRTMat4
 @return 			result of subtraction
*****************************************************************************/
	PVRTMat4 operator-(const PVRTMat4& rhs) const
	{
		PVRTMat4 out;
		for(int i=0;i<16;i++)
		{
			out.f[i] = f[i]-rhs.f[i];
		}
		return out;
	}

/*!***************************************************************************
 @brief      		element by element addition and assignment operator.
 @param[in]				rhs another PVRTMat4
 @return 			result of addition and assignment
*****************************************************************************/
	PVRTMat4& operator+=(const PVRTMat4& rhs)
	{
		VERTTYPE *lptr = f;
		VERTTYPE const *rptr = rhs.f;
		for(int i=0;i<16;i++)
		{
			(*lptr++) += (*rptr++);
		}
		return *this;
	}

/*!***************************************************************************
 @brief      		element by element subtraction and assignment operator.
 @param[in]				rhs another PVRTMat4
 @return 			result of subtraction and assignment
*****************************************************************************/
	PVRTMat4& operator-=(const PVRTMat4& rhs)
	{
		VERTTYPE *lptr = f;
		VERTTYPE const *rptr = rhs.f;
		for(int i=0;i<16;i++)
		{
			(*lptr++) -= (*rptr++);
		}
		return *this;
	}


/*!***************************************************************************
 @brief      		Matrix multiplication and assignment of two 4x4 matrices.
 @param[in]				rhs another PVRTMat4
 @return 			result of multiplication and assignment
*****************************************************************************/
	PVRTMat4& operator*=(const PVRTMat4& rhs)
	{
		PVRTMat4 result;
		// col 0
		result.f[0] =	VERTTYPEMUL(f[0],rhs.f[0])+VERTTYPEMUL(f[4],rhs.f[1])+VERTTYPEMUL(f[8],rhs.f[2])+VERTTYPEMUL(f[12],rhs.f[3]);
		result.f[1] =	VERTTYPEMUL(f[1],rhs.f[0])+VERTTYPEMUL(f[5],rhs.f[1])+VERTTYPEMUL(f[9],rhs.f[2])+VERTTYPEMUL(f[13],rhs.f[3]);
		result.f[2] =	VERTTYPEMUL(f[2],rhs.f[0])+VERTTYPEMUL(f[6],rhs.f[1])+VERTTYPEMUL(f[10],rhs.f[2])+VERTTYPEMUL(f[14],rhs.f[3]);
		result.f[3] =	VERTTYPEMUL(f[3],rhs.f[0])+VERTTYPEMUL(f[7],rhs.f[1])+VERTTYPEMUL(f[11],rhs.f[2])+VERTTYPEMUL(f[15],rhs.f[3]);

		// col 1
		result.f[4] =	VERTTYPEMUL(f[0],rhs.f[4])+VERTTYPEMUL(f[4],rhs.f[5])+VERTTYPEMUL(f[8],rhs.f[6])+VERTTYPEMUL(f[12],rhs.f[7]);
		result.f[5] =	VERTTYPEMUL(f[1],rhs.f[4])+VERTTYPEMUL(f[5],rhs.f[5])+VERTTYPEMUL(f[9],rhs.f[6])+VERTTYPEMUL(f[13],rhs.f[7]);
		result.f[6] =	VERTTYPEMUL(f[2],rhs.f[4])+VERTTYPEMUL(f[6],rhs.f[5])+VERTTYPEMUL(f[10],rhs.f[6])+VERTTYPEMUL(f[14],rhs.f[7]);
		result.f[7] =	VERTTYPEMUL(f[3],rhs.f[4])+VERTTYPEMUL(f[7],rhs.f[5])+VERTTYPEMUL(f[11],rhs.f[6])+VERTTYPEMUL(f[15],rhs.f[7]);

		// col 2
		result.f[8] =	VERTTYPEMUL(f[0],rhs.f[8])+VERTTYPEMUL(f[4],rhs.f[9])+VERTTYPEMUL(f[8],rhs.f[10])+VERTTYPEMUL(f[12],rhs.f[11]);
		result.f[9] =	VERTTYPEMUL(f[1],rhs.f[8])+VERTTYPEMUL(f[5],rhs.f[9])+VERTTYPEMUL(f[9],rhs.f[10])+VERTTYPEMUL(f[13],rhs.f[11]);
		result.f[10] =	VERTTYPEMUL(f[2],rhs.f[8])+VERTTYPEMUL(f[6],rhs.f[9])+VERTTYPEMUL(f[10],rhs.f[10])+VERTTYPEMUL(f[14],rhs.f[11]);
		result.f[11] =	VERTTYPEMUL(f[3],rhs.f[8])+VERTTYPEMUL(f[7],rhs.f[9])+VERTTYPEMUL(f[11],rhs.f[10])+VERTTYPEMUL(f[15],rhs.f[11]);

		// col 3
		result.f[12] =	VERTTYPEMUL(f[0],rhs.f[12])+VERTTYPEMUL(f[4],rhs.f[13])+VERTTYPEMUL(f[8],rhs.f[14])+VERTTYPEMUL(f[12],rhs.f[15]);
		result.f[13] =	VERTTYPEMUL(f[1],rhs.f[12])+VERTTYPEMUL(f[5],rhs.f[13])+VERTTYPEMUL(f[9],rhs.f[14])+VERTTYPEMUL(f[13],rhs.f[15]);
		result.f[14] =	VERTTYPEMUL(f[2],rhs.f[12])+VERTTYPEMUL(f[6],rhs.f[13])+VERTTYPEMUL(f[10],rhs.f[14])+VERTTYPEMUL(f[14],rhs.f[15]);
		result.f[15] =	VERTTYPEMUL(f[3],rhs.f[12])+VERTTYPEMUL(f[7],rhs.f[13])+VERTTYPEMUL(f[11],rhs.f[14])+VERTTYPEMUL(f[15],rhs.f[15]);

		*this = result;
		return *this;
	}

/*!***************************************************************************
 @brief      		element multiplication by a single value.
 @param[in]			rhs    A single value
 @return 			result of multiplication and assignment
*****************************************************************************/
	PVRTMat4& operator*(const VERTTYPE rhs)
	{
		for (int i=0; i<16; ++i)
		{
			f[i]*=rhs;
		}
		return *this;
	}
/*!***************************************************************************
 @brief      		element multiplication and assignment by a single value.
 @param[in]			rhs    A single value
 @return 			result of multiplication and assignment
*****************************************************************************/
	PVRTMat4& operator*=(const VERTTYPE rhs)
	{
		for (int i=0; i<16; ++i)
		{
			f[i]*=rhs;
		}
		return *this;
	}

/*!***************************************************************************
 @brief      		element assignment operator.
 @param[in]				rhs another PVRTMat4
 @return 			result of assignment
*****************************************************************************/
	PVRTMat4& operator=(const PVRTMat4& rhs)
	{
		for (int i=0; i<16; ++i)
		{
			f[i] =rhs.f[i];
		}
		return *this;
	}
/*!***************************************************************************
 @brief      		Matrix multiplication of 4x4 matrix and vec3
 @param[in]				rhs a PVRTVec4
 @return 			result of multiplication
*****************************************************************************/
	PVRTVec4 operator*(const PVRTVec4& rhs) const
	{
		PVRTVec4 out;
		out.x = VERTTYPEMUL(rhs.x,f[0])+VERTTYPEMUL(rhs.y,f[4])+VERTTYPEMUL(rhs.z,f[8])+VERTTYPEMUL(rhs.w,f[12]);
		out.y = VERTTYPEMUL(rhs.x,f[1])+VERTTYPEMUL(rhs.y,f[5])+VERTTYPEMUL(rhs.z,f[9])+VERTTYPEMUL(rhs.w,f[13]);
		out.z = VERTTYPEMUL(rhs.x,f[2])+VERTTYPEMUL(rhs.y,f[6])+VERTTYPEMUL(rhs.z,f[10])+VERTTYPEMUL(rhs.w,f[14]);
		out.w = VERTTYPEMUL(rhs.x,f[3])+VERTTYPEMUL(rhs.y,f[7])+VERTTYPEMUL(rhs.z,f[11])+VERTTYPEMUL(rhs.w,f[15]);

		return out;
	}

/*!***************************************************************************
 @brief      		Matrix multiplication and assignment of 4x4 matrix and vec3
 @param[in]				rhs a PVRTVec4
 @return 			result of multiplication and assignment
*****************************************************************************/
	PVRTVec4 operator*=(const PVRTVec4& rhs) const
	{
		PVRTVec4 out;
		out.x = VERTTYPEMUL(rhs.x,f[0])+VERTTYPEMUL(rhs.y,f[4])+VERTTYPEMUL(rhs.z,f[8])+VERTTYPEMUL(rhs.w,f[12]);
		out.y = VERTTYPEMUL(rhs.x,f[1])+VERTTYPEMUL(rhs.y,f[5])+VERTTYPEMUL(rhs.z,f[9])+VERTTYPEMUL(rhs.w,f[13]);
		out.z = VERTTYPEMUL(rhs.x,f[2])+VERTTYPEMUL(rhs.y,f[6])+VERTTYPEMUL(rhs.z,f[10])+VERTTYPEMUL(rhs.w,f[14]);
		out.w = VERTTYPEMUL(rhs.x,f[3])+VERTTYPEMUL(rhs.y,f[7])+VERTTYPEMUL(rhs.z,f[11])+VERTTYPEMUL(rhs.w,f[15]);

		return out;
	}

/*!***************************************************************************
 @brief      		Calculates multiplicative inverse of this matrix
					The matrix must be of the form :
					A 0
					C 1
					Where A is a 3x3 matrix and C is a 1x3 matrix.
 @return 			inverse mat4
*****************************************************************************/
	PVRTMat4 inverse() const;

/*!***************************************************************************
 @fn       			inverseEx
 @return 			inverse mat4
 @brief      		Calculates multiplicative inverse of this matrix
					Uses a linear equation solver and the knowledge that M.M^-1=I.
					Use this fn to calculate the inverse of matrices that
					inverse() cannot.
*****************************************************************************/
	PVRTMat4 inverseEx() const
	{
		PVRTMat4 out;
		VERTTYPE 		*ppRows[4];
		VERTTYPE 		pRes[4];
		VERTTYPE 		pIn[20];
		int				i, j;

		for(i = 0; i < 4; ++i)
			ppRows[i] = &pIn[i * 5];

		/* Solve 4 sets of 4 linear equations */
		for(i = 0; i < 4; ++i)
		{
			for(j = 0; j < 4; ++j)
			{
				ppRows[j][0] = PVRTMat4::Identity().f[i + 4 * j];
				memcpy(&ppRows[j][1], &f[j * 4], 4 * sizeof(VERTTYPE));
			}

			PVRTLinearEqSolve(pRes, (VERTTYPE**)ppRows, 4);

			for(j = 0; j < 4; ++j)
			{
				out.f[i + 4 * j] = pRes[j];
			}
		}

		return out;
	}

/*!***************************************************************************
 @fn       			transpose
 @return 			transpose mat4
 @brief      		Calculates transpose of this matrix
*****************************************************************************/
	PVRTMat4 transpose() const
	{
		PVRTMat4 out;
		out.f[0] = f[0];		out.f[1] = f[4];		out.f[2] = f[8];		out.f[3] = f[12];
		out.f[4] = f[1];		out.f[5] = f[5];		out.f[6] = f[9];		out.f[7] = f[13];
		out.f[8] = f[2];		out.f[9] = f[6];		out.f[10] = f[10];		out.f[11] = f[14];
		out.f[12] = f[3];		out.f[13] = f[7];		out.f[14] = f[11];		out.f[15] = f[15];
		return out;
	}

/*!***************************************************************************
 @brief      		Alters the translation component of the transformation matrix.
 @param[in]			tx      Distance of translation in x axis
 @param[in]			ty      Distance of translation in y axis
 @param[in]			tz      Distance of translation in z axis
 @return 			Returns this
*****************************************************************************/
	PVRTMat4& postTranslate(VERTTYPE tx, VERTTYPE ty, VERTTYPE tz)
	{
		f[12] += VERTTYPEMUL(tx,f[0])+VERTTYPEMUL(ty,f[4])+VERTTYPEMUL(tz,f[8]);
		f[13] += VERTTYPEMUL(tx,f[1])+VERTTYPEMUL(ty,f[5])+VERTTYPEMUL(tz,f[9]);
		f[14] += VERTTYPEMUL(tx,f[2])+VERTTYPEMUL(ty,f[6])+VERTTYPEMUL(tz,f[10]);
		f[15] += VERTTYPEMUL(tx,f[3])+VERTTYPEMUL(ty,f[7])+VERTTYPEMUL(tz,f[11]);

//			col(3) += tx * col(0) + ty * col(1) + tz * col(2);
		return *this;
	}

/*!***************************************************************************
 @brief      		Alters the translation component of the transformation matrix.
 @param[in]			tvec    Translation vector
 @return 			Returns this
*****************************************************************************/
	PVRTMat4& postTranslate(const PVRTVec3& tvec)
	{
		return postTranslate(tvec.x, tvec.y, tvec.z);
	}

/*!***************************************************************************
 @brief      		Translates the matrix from the passed parameters
 @param[in]			tx      Distance of translation in x axis
 @param[in]			ty      Distance of translation in y axis
 @param[in]			tz      Distance of translation in z axis
 @return 			Returns this
*****************************************************************************/
	PVRTMat4& preTranslate(VERTTYPE tx, VERTTYPE ty, VERTTYPE tz)
	{
		f[0]+=VERTTYPEMUL(f[3],tx);	f[4]+=VERTTYPEMUL(f[7],tx);	f[8]+=VERTTYPEMUL(f[11],tx);	f[12]+=VERTTYPEMUL(f[15],tx);
		f[1]+=VERTTYPEMUL(f[3],ty);	f[5]+=VERTTYPEMUL(f[7],ty);	f[9]+=VERTTYPEMUL(f[11],ty);	f[13]+=VERTTYPEMUL(f[15],ty);
		f[2]+=VERTTYPEMUL(f[3],tz);	f[6]+=VERTTYPEMUL(f[7],tz);	f[10]+=VERTTYPEMUL(f[11],tz);	f[14]+=VERTTYPEMUL(f[15],tz);

//			row(0) += tx * row(3);
//			row(1) += ty * row(3);
//			row(2) += tz * row(3);
		return *this;
	}

/*!***************************************************************************
 @brief      		Translates the matrix from the passed parameters
 @param[in]			tvec    Translation vector
 @return 			Returns the translation defined by the passed parameters
*****************************************************************************/
	PVRTMat4& preTranslate(const PVRTVec3& tvec)
	{
		return preTranslate(tvec.x, tvec.y, tvec.z);
	}
/*!***************************************************************************
 @brief      		Calculates transpose of this matrix
 @return 			pointer to an array of the elements of this PVRTMat4
*****************************************************************************/
	VERTTYPE *ptr() { return (VERTTYPE*)&f; }

/*!***************************************************************************
** Static factory functions
*****************************************************************************/
/*!***************************************************************************
 @brief      		Generates an identity matrix
 @return 			a PVRTMat4 representation of the 4x4 identity matrix
*****************************************************************************/
	static PVRTMat4 Identity()
	{
		PVRTMat4 out;
		out.f[0] = f2vt(1);out.f[1] = 0;out.f[2] = 0;out.f[3] = 0;
		out.f[4] = 0;out.f[5] = f2vt(1);out.f[6] = 0;out.f[7] = 0;
		out.f[8] = 0;out.f[9] = 0;out.f[10] = f2vt(1);out.f[11] = 0;
		out.f[12] = 0;out.f[13] = 0;out.f[14] = 0;out.f[15] = f2vt(1);
		return out;
	}

/*!***************************************************************************
 @fn       			RotationX
 @return 			a PVRTMat3 corresponding to the requested rotation
 @brief      		Calculates a matrix corresponding to a rotation of angle
					degrees about the X axis
*****************************************************************************/
	static PVRTMat4 RotationX(VERTTYPE angle);
/*!***************************************************************************
 @fn       			RotationY
 @return 			a PVRTMat3 corresponding to the requested rotation
 @brief      		Calculates a matrix corresponding to a rotation of angle
					degrees about the Y axis
*****************************************************************************/
	static PVRTMat4 RotationY(VERTTYPE angle);
/*!***************************************************************************
 @fn       			RotationZ
 @return 			a PVRTMat3 corresponding to the requested rotation
 @brief      		Calculates a matrix corresponding to a rotation of angle
					degrees about the Z axis
*****************************************************************************/
	static PVRTMat4 RotationZ(VERTTYPE angle);

/*!***************************************************************************
 @brief      		Calculates a matrix corresponding to scaling of fx, fy and fz
					times in each axis.
 @return 			a PVRTMat3 corresponding to the requested scaling transformation
*****************************************************************************/
	static PVRTMat4 Scale(const VERTTYPE fx,const VERTTYPE fy,const VERTTYPE fz)
	{
		return PVRTMat4(fx,0,0,0,
			0,fy,0,0,
			0,0,fz,0,
			0,0,0,f2vt(1));
	}

/*!***************************************************************************
 @brief      		Calculates a matrix corresponding to scaling of the given vector.
 @return 			a PVRTMat3 corresponding to the requested scaling transformation
*****************************************************************************/
	static PVRTMat4 Scale(const PVRTVec3& vec)
	{
		return Scale(vec.x, vec.y, vec.z);
	}

/*!***************************************************************************
 @brief      		Calculates a 4x4 matrix corresponding to a transformation
					of tx, ty and tz distance in each axis.
 @return 			a PVRTMat4 corresponding to the requested translation
*****************************************************************************/
	static PVRTMat4 Translation(const VERTTYPE tx, const VERTTYPE ty, const VERTTYPE tz)
	{
		return PVRTMat4(f2vt(1),0,0,0,
			0,f2vt(1),0,0,
			0,0,f2vt(1),0,
			tx,ty,tz,f2vt(1));
	}

/*!***************************************************************************
 @brief      		Calculates a 4x4 matrix corresponding to a transformation
					of tx, ty and tz distance in each axis as taken from the
					given vector.
 @return 			a PVRTMat4 corresponding to the requested translation
*****************************************************************************/

	static PVRTMat4 Translation(const PVRTVec3& vec)
	{
		return Translation(vec.x, vec.y, vec.z);
	}

/*!***************************************************************************
** Clipspace enum
** Determines whether clip space Z ranges from -1 to 1 (OGL) or from 0 to 1 (D3D)
*****************************************************************************/
	enum eClipspace { OGL, D3D };

/*!***************************************************************************
 @brief      		Translates the matrix from the passed parameters
 @param[in]			left        Left view plane
 @param[in]			top         Top view plane
 @param[in]			right       Right view plane
 @param[in]			bottom      Bottom view plane
 @param[in]			nearPlane   The near rendering plane
 @param[in]			farPlane    The far rendering plane
 @param[in]			cs          Which clipspace convention is being used
 @param[in]			bRotate     Is the viewport in portrait or landscape mode
 @return 			Returns the orthogonal projection matrix defined by the passed parameters
*****************************************************************************/
	static PVRTMat4 Ortho(VERTTYPE left, VERTTYPE top, VERTTYPE right,
		VERTTYPE bottom, VERTTYPE nearPlane, VERTTYPE farPlane, const eClipspace cs, bool bRotate = false)
	{
		VERTTYPE rcplmr = VERTTYPEDIV(VERTTYPE(1),(left - right));
		VERTTYPE rcpbmt = VERTTYPEDIV(VERTTYPE(1),(bottom - top));
		VERTTYPE rcpnmf = VERTTYPEDIV(VERTTYPE(1),(nearPlane - farPlane));

		PVRTMat4 result;

		if (bRotate)
		{
			result.f[0]=0;		result.f[4]=VERTTYPEMUL(2,rcplmr); result.f[8]=0; result.f[12]=VERTTYPEMUL(-(right+left),rcplmr);
			result.f[1]=VERTTYPEMUL(-2,rcpbmt);	result.f[5]=0;		result.f[9]=0;	result.f[13]=VERTTYPEMUL((top+bottom),rcpbmt);
		}
		else
		{
			result.f[0]=VERTTYPEMUL(-2,rcplmr);	result.f[4]=0; result.f[8]=0; result.f[12]=VERTTYPEMUL(right+left,rcplmr);
			result.f[1]=0;		result.f[5]=VERTTYPEMUL(-2,rcpbmt);	result.f[9]=0;	result.f[13]=VERTTYPEMUL((top+bottom),rcpbmt);
		}
		if (cs == D3D)
		{
			result.f[2]=0;	result.f[6]=0;	result.f[10]=-rcpnmf;	result.f[14]=VERTTYPEMUL(nearPlane,rcpnmf);
		}
		else
		{
			result.f[2]=0;	result.f[6]=0;	result.f[10]=VERTTYPEMUL(-2,rcpnmf);	result.f[14]=VERTTYPEMUL(nearPlane + farPlane,rcpnmf);
		}
		result.f[3]=0;	result.f[7]=0;	result.f[11]=0;	result.f[15]=1;

		return result;
	}

/*!***************************************************************************
 @fn       			LookAtRH
 @param[in]				vEye position of 'camera'
 @param[in]				vAt target that camera points at
 @param[in]				vUp up vector for camera
 @return 			Returns the view matrix defined by the passed parameters
 @brief      		Create a look-at view matrix for a right hand coordinate
					system
*****************************************************************************/
	static PVRTMat4 LookAtRH(const PVRTVec3& vEye, const PVRTVec3& vAt, const PVRTVec3& vUp)
		{ return LookAt(vEye, vAt, vUp, true); }
/*!***************************************************************************
 @fn       			LookAtLH
 @param[in]				vEye position of 'camera'
 @param[in]				vAt target that camera points at
 @param[in]				vUp up vector for camera
 @return 			Returns the view matrix defined by the passed parameters
 @brief      		Create a look-at view matrix for a left hand coordinate
					system
*****************************************************************************/
	static PVRTMat4 LookAtLH(const PVRTVec3& vEye, const PVRTVec3& vAt, const PVRTVec3& vUp)
		{ return LookAt(vEye, vAt, vUp, false);	}

/*!***************************************************************************
 @brief      	Create a perspective matrix for a right hand coordinate
				system
 @param[in]			width		width of viewplane
 @param[in]			height		height of viewplane
 @param[in]			nearPlane	near clipping distance
 @param[in]			farPlane	far clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveRH(VERTTYPE width, VERTTYPE height, VERTTYPE nearPlane,
		VERTTYPE farPlane, const eClipspace cs, bool bRotate = false)
		{ return Perspective(width, height, nearPlane, farPlane, cs, true, bRotate); }

/*!***************************************************************************
 @brief      	Create a perspective matrix for a left hand coordinate
				system
 @param[in]			width		width of viewplane
 @param[in]			height		height of viewplane
 @param[in]			nearPlane	near clipping distance
 @param[in]			farPlane	far clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveLH(VERTTYPE width, VERTTYPE height, VERTTYPE nearPlane, VERTTYPE farPlane, const eClipspace cs, bool bRotate = false)
		{ return Perspective(width, height, nearPlane, farPlane, cs, false, bRotate); }

/*!***************************************************************************
 @brief      	Create a perspective matrix for a right hand coordinate
				system
 @param[in]			width		width of viewplane
 @param[in]			height		height of viewplane
 @param[in]			nearPlane	near clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFloatDepthRH(VERTTYPE width, VERTTYPE height, VERTTYPE nearPlane, const eClipspace cs, bool bRotate = false)
		{ return PerspectiveFloatDepth(width, height, nearPlane, cs, true, bRotate); }

/*!***************************************************************************
 @brief      	Create a perspective matrix for a left hand coordinate
				system
 @param[in]			width		width of viewplane
 @param[in]			height		height of viewplane
 @param[in]			nearPlane	near clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFloatDepthLH(VERTTYPE width, VERTTYPE height, VERTTYPE nearPlane, const eClipspace cs, bool bRotate = false)
		{ return PerspectiveFloatDepth(width, height, nearPlane, cs, false, bRotate); }

/*!***************************************************************************
 @brief      	Create a perspective matrix for a right hand coordinate
				system
 @param[in]			fovy		angle of view (vertical)
 @param[in]			aspect		aspect ratio of view
 @param[in]			nearPlane	near clipping distance
 @param[in]			farPlane	far clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFovRH(VERTTYPE fovy, VERTTYPE aspect, VERTTYPE nearPlane, VERTTYPE farPlane, const eClipspace cs, bool bRotate = false)
		{ return PerspectiveFov(fovy, aspect, nearPlane, farPlane, cs, true, bRotate); }
/*!***************************************************************************
 @brief      	Create a perspective matrix for a left hand coordinate
				system
 @param[in]			fovy		angle of view (vertical)
 @param[in]			aspect		aspect ratio of view
 @param[in]			nearPlane	near clipping distance
 @param[in]			farPlane	far clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFovLH(VERTTYPE fovy, VERTTYPE aspect, VERTTYPE nearPlane, VERTTYPE farPlane, const eClipspace cs, bool bRotate = false)
		{ return PerspectiveFov(fovy, aspect, nearPlane, farPlane, cs, false, bRotate); }

/*!***************************************************************************
 @brief      	Create a perspective matrix for a right hand coordinate
				system
 @param[in]			fovy		angle of view (vertical)
 @param[in]			aspect		aspect ratio of view
 @param[in]			nearPlane	near clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFovFloatDepthRH(VERTTYPE fovy, VERTTYPE aspect, VERTTYPE nearPlane, const eClipspace cs, bool bRotate = false)
		{ return PerspectiveFovFloatDepth(fovy, aspect, nearPlane, cs, true, bRotate); }
/*!***************************************************************************
 @brief      	Create a perspective matrix for a left hand coordinate
				system
 @param[in]			fovy		angle of view (vertical)
 @param[in]			aspect		aspect ratio of view
 @param[in]			nearPlane	near clipping distance
 @param[in]			cs			cs which clipspace convention is being used
 @param[in]			bRotate is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFovFloatDepthLH(VERTTYPE fovy, VERTTYPE aspect, VERTTYPE nearPlane, const eClipspace cs, bool bRotate = false)
		{ return PerspectiveFovFloatDepth(fovy, aspect, nearPlane, cs, false, bRotate); }

/*!***************************************************************************
 @brief      		Create a look-at view matrix
 @param[in]			vEye            Position of 'camera'
 @param[in]			vAt             Target that camera points at
 @param[in]			vUp             Up vector for camera
 @param[in]			bRightHanded    Handedness of coordinate system
 @return 			Returns the view matrix defined by the passed parameters
*****************************************************************************/
	static PVRTMat4 LookAt(const PVRTVec3& vEye, const PVRTVec3& vAt, const PVRTVec3& vUp, bool bRightHanded)
	{
		PVRTVec3 vForward, vUpNorm, vSide;
		PVRTMat4 result;

		vForward = (bRightHanded) ? vEye - vAt : vAt - vEye;

		vForward.normalize();
		vSide   = vUp.cross( vForward);
		vSide	= vSide.normalized();
		vUpNorm = vForward.cross(vSide);
		vUpNorm = vUpNorm.normalized();

		result.f[0]=vSide.x;	result.f[4]=vSide.y;	result.f[8]=vSide.z;		result.f[12]=0;
		result.f[1]=vUpNorm.x;	result.f[5]=vUpNorm.y;	result.f[9]=vUpNorm.z;		result.f[13]=0;
		result.f[2]=vForward.x; result.f[6]=vForward.y;	result.f[10]=vForward.z;	result.f[14]=0;
		result.f[3]=0;			result.f[7]=0;			result.f[11]=0;				result.f[15]=f2vt(1);


		result.postTranslate(-vEye);
		return result;
	}

/*!***************************************************************************
 @brief      	Create a perspective matrix
 @param[in]		width		Width of viewplane
 @param[in]		height		Height of viewplane
 @param[in]		nearPlane	Near clipping distance
 @param[in]		farPlane	Far clipping distance
 @param[in]		cs			Which clipspace convention is being used
 @param[in]		bRightHanded	Handedness of coordinate system
 @param[in]		bRotate		Is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 Perspective(
		VERTTYPE width, VERTTYPE height,
		VERTTYPE nearPlane, VERTTYPE farPlane,
		const eClipspace cs,
		bool bRightHanded,
		bool bRotate = false)
	{
		VERTTYPE n2 = VERTTYPEMUL(f2vt(2),nearPlane);
		VERTTYPE rcpnmf = VERTTYPEDIV(f2vt(1),(nearPlane - farPlane));

		PVRTMat4 result;
		if (bRotate)
		{
			result.f[0] = 0;	result.f[4]=VERTTYPEDIV(-n2,width);	result.f[8]=0;	result.f[12]=0;
			result.f[1] = VERTTYPEDIV(n2,height);	result.f[5]=0;	result.f[9]=0;	result.f[13]=0;
		}
		else
		{
			result.f[0] = VERTTYPEDIV(n2,width);	result.f[4]=0;	result.f[8]=0;	result.f[12]=0;
			result.f[1] = 0;	result.f[5]=VERTTYPEDIV(n2,height);	result.f[9]=0;	result.f[13]=0;
		}
		if (cs == D3D)
		{
			result.f[2] = 0;	result.f[6]=0;	result.f[10]=VERTTYPEMUL(farPlane,rcpnmf);	result.f[14]=VERTTYPEMUL(VERTTYPEMUL(farPlane,rcpnmf),nearPlane);
		}
		else
		{
			result.f[2] = 0;	result.f[6]=0;	result.f[10]=VERTTYPEMUL(farPlane+nearPlane,rcpnmf);	result.f[14]=VERTTYPEMUL(VERTTYPEMUL(farPlane,rcpnmf),n2);
		}
		result.f[3] = 0;	result.f[7]=0;	result.f[11]=f2vt(-1);	result.f[15]=0;

		if (!bRightHanded)
		{
			result.f[10] = VERTTYPEMUL(result.f[10], f2vt(-1));
			result.f[11] = f2vt(1);
		}

		return result;
	}

/*!***************************************************************************
 @brief      	Perspective calculation where far plane is assumed to be at an infinite distance and the screen
				space Z is inverted
 @param[in]		width		Width of viewplane
 @param[in]		height		Height of viewplane
 @param[in]		nearPlane	Near clipping distance
 @param[in]		cs			Which clipspace convention is being used
 @param[in]		bRightHanded	Handedness of coordinate system
 @param[in]		bRotate		Is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFloatDepth(
		VERTTYPE width, VERTTYPE height,
		VERTTYPE nearPlane,
		const eClipspace cs,
		bool bRightHanded,
		bool bRotate = false)
	{
		VERTTYPE n2 = VERTTYPEMUL(2,nearPlane);

		PVRTMat4 result;
		if (bRotate)
		{
			result.f[0] = 0;	result.f[4]=VERTTYPEDIV(-n2,width);	result.f[8]=0;	result.f[12]=0;
			result.f[1] = VERTTYPEDIV(n2,height);	result.f[5]=0;	result.f[9]=0;	result.f[13]=0;
		}
		else
		{
			result.f[0] = VERTTYPEDIV(n2,width);	result.f[4]=0;	result.f[8]=0;	result.f[12]=0;
			result.f[1] = 0;	result.f[5]=VERTTYPEDIV(n2,height);	result.f[9]=0;	result.f[13]=0;
		}
		if (cs == D3D)
		{
			result.f[2] = 0;	result.f[6]=0;	result.f[10]=0;	result.f[14]=nearPlane;
		}
		else
		{
			result.f[2] = 0;	result.f[6]=0;	result.f[10]=(bRightHanded?(VERTTYPE)1:(VERTTYPE)-1);	result.f[14]=n2;
		}
		result.f[3] = (VERTTYPE)0;	result.f[7]=(VERTTYPE)0;	result.f[11]= (bRightHanded?(VERTTYPE)-1:(VERTTYPE)1);	result.f[15]=(VERTTYPE)0;

		return result;
	}

/*!***************************************************************************
 @brief      	Perspective calculation where field of view is used instead of near plane dimensions
 @param[in]		fovy		Angle of view (vertical)
 @param[in]		aspect		Aspect ratio of view
 @param[in]		nearPlane	Near clipping distance
 @param[in]		farPlane	Far clipping distance
 @param[in]		cs			Which clipspace convention is being used
 @param[in]		bRightHanded	Handedness of coordinate system
 @param[in]		bRotate		Is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFov(
		VERTTYPE fovy, VERTTYPE aspect,
		VERTTYPE nearPlane, VERTTYPE farPlane,
		const eClipspace cs,
		bool bRightHanded,
		bool bRotate = false)
	{
		VERTTYPE height = VERTTYPEMUL(VERTTYPEMUL(f2vt(2.0f),nearPlane),PVRTTAN(VERTTYPEMUL(fovy,f2vt(0.5f))));
		if (bRotate) return Perspective(height, VERTTYPEDIV(height,aspect), nearPlane, farPlane, cs, bRightHanded, bRotate);
		return Perspective(VERTTYPEMUL(height,aspect), height, nearPlane, farPlane, cs, bRightHanded, bRotate);
	}

/*!***************************************************************************
 @brief      	Perspective calculation where field of view is used instead of near plane dimensions
				and far plane is assumed to be at an infinite distance with inverted Z range
 @param[in]		fovy		Angle of view (vertical)
 @param[in]		aspect		Aspect ratio of view
 @param[in]		nearPlane	Near clipping distance
 @param[in]		cs			Which clipspace convention is being used
 @param[in]		bRightHanded	Handedness of coordinate system
 @param[in]		bRotate		Is the viewport in portrait or landscape mode
 @return 		Perspective matrix
*****************************************************************************/
	static PVRTMat4 PerspectiveFovFloatDepth(
		VERTTYPE fovy, VERTTYPE aspect,
		VERTTYPE nearPlane,
		const eClipspace cs,
		bool bRightHanded,
		bool bRotate = false)
	{
		VERTTYPE height = VERTTYPEMUL(VERTTYPEMUL(2,nearPlane), PVRTTAN(VERTTYPEMUL(fovy,0.5)));
		if (bRotate) return PerspectiveFloatDepth(height, VERTTYPEDIV(height,aspect), nearPlane, cs, bRightHanded, bRotate);
		return PerspectiveFloatDepth(VERTTYPEMUL(height,aspect), height, nearPlane, cs, bRightHanded, bRotate);
	}
};

#endif /*__PVRTVECTOR_H__*/

/*****************************************************************************
End of file (PVRTVector.h)
*****************************************************************************/

