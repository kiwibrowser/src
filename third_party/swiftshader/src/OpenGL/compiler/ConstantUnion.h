// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _CONSTANT_UNION_INCLUDED_
#define _CONSTANT_UNION_INCLUDED_

#ifndef __ANDROID__
#include <assert.h>
#else
#include "../../Common/DebugAndroid.hpp"
#endif

class ConstantUnion {
public:
	POOL_ALLOCATOR_NEW_DELETE();
	ConstantUnion()
	{
		iConst = 0;
		type = EbtVoid;
	}

	bool cast(TBasicType newType, const ConstantUnion &constant)
	{
		switch (newType)
		{
		case EbtFloat:
			switch (constant.type)
			{
			case EbtInt:   setFConst(static_cast<float>(constant.getIConst())); break;
			case EbtUInt:  setFConst(static_cast<float>(constant.getUConst())); break;
			case EbtBool:  setFConst(static_cast<float>(constant.getBConst())); break;
			case EbtFloat: setFConst(static_cast<float>(constant.getFConst())); break;
			default:       return false;
			}
			break;
		case EbtInt:
			switch (constant.type)
			{
			case EbtInt:   setIConst(static_cast<int>(constant.getIConst())); break;
			case EbtUInt:  setIConst(static_cast<int>(constant.getUConst())); break;
			case EbtBool:  setIConst(static_cast<int>(constant.getBConst())); break;
			case EbtFloat: setIConst(static_cast<int>(constant.getFConst())); break;
			default:       return false;
			}
			break;
		case EbtUInt:
			switch (constant.type)
			{
			case EbtInt:   setUConst(static_cast<unsigned int>(constant.getIConst())); break;
			case EbtUInt:  setUConst(static_cast<unsigned int>(constant.getUConst())); break;
			case EbtBool:  setUConst(static_cast<unsigned int>(constant.getBConst())); break;
			case EbtFloat: setUConst(static_cast<unsigned int>(constant.getFConst())); break;
			default:       return false;
			}
			break;
		case EbtBool:
			switch (constant.type)
			{
			case EbtInt:   setBConst(constant.getIConst() != 0);    break;
			case EbtUInt:  setBConst(constant.getUConst() != 0);    break;
			case EbtBool:  setBConst(constant.getBConst());         break;
			case EbtFloat: setBConst(constant.getFConst() != 0.0f); break;
			default:       return false;
			}
			break;
		case EbtStruct:    // Struct fields don't get cast
			switch (constant.type)
			{
			case EbtInt:   setIConst(constant.getIConst()); break;
			case EbtUInt:  setUConst(constant.getUConst()); break;
			case EbtBool:  setBConst(constant.getBConst()); break;
			case EbtFloat: setFConst(constant.getFConst()); break;
			default:       return false;
			}
			break;
		default:
			return false;
		}

		return true;
	}

	void setIConst(int i) {iConst = i; type = EbtInt; }
	void setUConst(unsigned int u) { uConst = u; type = EbtUInt; }
	void setFConst(float f) {fConst = f; type = EbtFloat; }
	void setBConst(bool b) {bConst = b; type = EbtBool; }

	int getIConst() const { return iConst; }
	unsigned int getUConst() const { return uConst; }
	float getFConst() const { return fConst; }
	bool getBConst() const { return bConst; }

	float getAsFloat() const
	{
		const int FFFFFFFFh = 0xFFFFFFFF;

		switch(type)
		{
		case EbtInt:   return reinterpret_cast<const float&>(iConst);
		case EbtUInt:  return reinterpret_cast<const float&>(uConst);
		case EbtFloat: return fConst;
		case EbtBool:  return (bConst == true) ? reinterpret_cast<const float&>(FFFFFFFFh) : 0;
		default:       return 0;
		}
	}

	bool operator==(const int i) const
	{
		return i == iConst;
	}

	bool operator==(const unsigned int u) const
	{
		return u == uConst;
	}

	bool operator==(const float f) const
	{
		return f == fConst;
	}

	bool operator==(const bool b) const
	{
		return b == bConst;
	}

	bool operator==(const ConstantUnion& constant) const
	{
		if (constant.type != type)
			return false;

		switch (type) {
		case EbtInt:
			return constant.iConst == iConst;
		case EbtUInt:
			return constant.uConst == uConst;
		case EbtFloat:
			return constant.fConst == fConst;
		case EbtBool:
			return constant.bConst == bConst;
		default:
			return false;
		}
	}

	bool operator!=(const int i) const
	{
		return !operator==(i);
	}

	bool operator!=(const unsigned int u) const
	{
		return !operator==(u);
	}

	bool operator!=(const float f) const
	{
		return !operator==(f);
	}

	bool operator!=(const bool b) const
	{
		return !operator==(b);
	}

	bool operator!=(const ConstantUnion& constant) const
	{
		return !operator==(constant);
	}

	bool operator>(const ConstantUnion& constant) const
	{
		assert(type == constant.type);
		switch (type) {
		case EbtInt:
			return iConst > constant.iConst;
		case EbtUInt:
			return uConst > constant.uConst;
		case EbtFloat:
			return fConst > constant.fConst;
		default:
			return false;   // Invalid operation, handled at semantic analysis
		}
	}

	bool operator<(const ConstantUnion& constant) const
	{
		assert(type == constant.type);
		switch (type) {
		case EbtInt:
			return iConst < constant.iConst;
		case EbtUInt:
			return uConst < constant.uConst;
		case EbtFloat:
			return fConst < constant.fConst;
		default:
			return false;   // Invalid operation, handled at semantic analysis
		}
	}

	bool operator<=(const ConstantUnion& constant) const
	{
		assert(type == constant.type);
		switch (type) {
		case EbtInt:
			return iConst <= constant.iConst;
		case EbtUInt:
			return uConst <= constant.uConst;
		case EbtFloat:
			return fConst <= constant.fConst;
		default:
			return false;   // Invalid operation, handled at semantic analysis
		}
	}

	bool operator>=(const ConstantUnion& constant) const
	{
		assert(type == constant.type);
		switch (type) {
		case EbtInt:
			return iConst >= constant.iConst;
		case EbtUInt:
			return uConst >= constant.uConst;
		case EbtFloat:
			return fConst >= constant.fConst;
		default:
			return false;   // Invalid operation, handled at semantic analysis
		}
	}

	ConstantUnion operator+(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtInt: returnValue.setIConst(iConst + constant.iConst); break;
		case EbtUInt: returnValue.setUConst(uConst + constant.uConst); break;
		case EbtFloat: returnValue.setFConst(fConst + constant.fConst); break;
		default: assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator-(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtInt: returnValue.setIConst(iConst - constant.iConst); break;
		case EbtUInt: returnValue.setUConst(uConst - constant.uConst); break;
		case EbtFloat: returnValue.setFConst(fConst - constant.fConst); break;
		default: assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator*(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtInt: returnValue.setIConst(iConst * constant.iConst); break;
		case EbtUInt: returnValue.setUConst(uConst * constant.uConst); break;
		case EbtFloat: returnValue.setFConst(fConst * constant.fConst); break;
		default: assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator%(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtInt: returnValue.setIConst(iConst % constant.iConst); break;
		case EbtUInt: returnValue.setUConst(uConst % constant.uConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator>>(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtInt: returnValue.setIConst(iConst >> constant.iConst); break;
		case EbtUInt: returnValue.setUConst(uConst >> constant.uConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator<<(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		// The signedness of the second parameter might be different, but we
		// don't care, since the result is undefined if the second parameter is
		// negative, and aliasing should not be a problem with unions.
		assert(constant.type == EbtInt || constant.type == EbtUInt);
		switch (type) {
		case EbtInt: returnValue.setIConst(iConst << constant.iConst); break;
		case EbtUInt: returnValue.setUConst(uConst << constant.uConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator&(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(constant.type == EbtInt || constant.type == EbtUInt);
		switch (type) {
		case EbtInt:  returnValue.setIConst(iConst & constant.iConst); break;
		case EbtUInt:  returnValue.setUConst(uConst & constant.uConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator|(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtInt:  returnValue.setIConst(iConst | constant.iConst); break;
		case EbtUInt:  returnValue.setUConst(uConst | constant.uConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator^(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtInt:  returnValue.setIConst(iConst ^ constant.iConst); break;
		case EbtUInt:  returnValue.setUConst(uConst ^ constant.uConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator&&(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtBool: returnValue.setBConst(bConst && constant.bConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	ConstantUnion operator||(const ConstantUnion& constant) const
	{
		ConstantUnion returnValue;
		assert(type == constant.type);
		switch (type) {
		case EbtBool: returnValue.setBConst(bConst || constant.bConst); break;
		default:     assert(false && "Default missing");
		}

		return returnValue;
	}

	TBasicType getType() const { return type; }
private:

	union  {
		int iConst;  // used for ivec, scalar ints
		unsigned int uConst; // used for uvec, scalar uints
		bool bConst; // used for bvec, scalar bools
		float fConst;   // used for vec, mat, scalar floats
	} ;

	TBasicType type;
};

#endif // _CONSTANT_UNION_INCLUDED_
