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

#ifndef _BASICTYPES_INCLUDED_
#define _BASICTYPES_INCLUDED_

#include "debug.h"

//
// Precision qualifiers
//
enum TPrecision : unsigned char
{
	// These need to be kept sorted
	EbpUndefined,
	EbpLow,
	EbpMedium,
	EbpHigh
};

inline const char *getPrecisionString(TPrecision precision)
{
	switch(precision)
	{
	case EbpHigh:		return "highp";		break;
	case EbpMedium:		return "mediump";	break;
	case EbpLow:		return "lowp";		break;
	default:			return "mediump";   break;   // Safest fallback
	}
}

//
// Basic type.  Arrays, vectors, etc., are orthogonal to this.
//
enum TBasicType : unsigned char
{
	EbtVoid,
	EbtFloat,
	EbtInt,
	EbtUInt,
	EbtBool,
	EbtGVec4,              // non type: represents vec4, ivec4, and uvec4
	EbtGenType,            // non type: represents float, vec2, vec3, and vec4
	EbtGenIType,           // non type: represents int, ivec2, ivec3, and ivec4
	EbtGenUType,           // non type: represents uint, uvec2, uvec3, and uvec4
	EbtGenBType,           // non type: represents bool, bvec2, bvec3, and bvec4
	EbtVec,                // non type: represents vec2, vec3, and vec4
	EbtIVec,               // non type: represents ivec2, ivec3, and ivec4
	EbtUVec,               // non type: represents uvec2, uvec3, and uvec4
	EbtBVec,               // non type: represents bvec2, bvec3, and bvec4
	EbtGuardSamplerBegin,  // non type: see implementation of IsSampler()
	EbtSampler2D,
	EbtSampler3D,
	EbtSamplerCube,
	EbtSampler2DArray,
	EbtSampler2DRect,       // Only valid if ARB_texture_rectangle exists.
	EbtSamplerExternalOES,  // Only valid if OES_EGL_image_external exists.
	EbtISampler2D,
	EbtISampler3D,
	EbtISamplerCube,
	EbtISampler2DArray,
	EbtUSampler2D,
	EbtUSampler3D,
	EbtUSamplerCube,
	EbtUSampler2DArray,
	EbtSampler2DShadow,
	EbtSamplerCubeShadow,
	EbtSampler2DArrayShadow,
	EbtGuardSamplerEnd,    // non type: see implementation of IsSampler()
	EbtGSampler2D,         // non type: represents sampler2D, isampler2D, and usampler2D
	EbtGSampler3D,         // non type: represents sampler3D, isampler3D, and usampler3D
	EbtGSamplerCube,       // non type: represents samplerCube, isamplerCube, and usamplerCube
	EbtGSampler2DArray,    // non type: represents sampler2DArray, isampler2DArray, and usampler2DArray
	EbtStruct,
	EbtInterfaceBlock,
	EbtAddress,            // should be deprecated??
	EbtInvariant           // used as a type when qualifying a previously declared variable as being invariant
};

enum TLayoutMatrixPacking
{
	EmpUnspecified,
	EmpRowMajor,
	EmpColumnMajor
};

enum TLayoutBlockStorage
{
	EbsUnspecified,
	EbsShared,
	EbsPacked,
	EbsStd140
};

inline const char *getBasicString(TBasicType type)
{
	switch(type)
	{
	case EbtVoid:               return "void";
	case EbtFloat:              return "float";
	case EbtInt:                return "int";
	case EbtUInt:               return "uint";
	case EbtBool:               return "bool";
	case EbtSampler2D:          return "sampler2D";
	case EbtSamplerCube:        return "samplerCube";
	case EbtSampler2DRect:      return "sampler2DRect";
	case EbtSamplerExternalOES: return "samplerExternalOES";
	case EbtSampler3D:			return "sampler3D";
	case EbtStruct:             return "structure";
	default: UNREACHABLE(type); return "unknown type";
	}
}

inline const char* getMatrixPackingString(TLayoutMatrixPacking mpq)
{
	switch(mpq)
	{
	case EmpUnspecified:    return "mp_unspecified";
	case EmpRowMajor:       return "row_major";
	case EmpColumnMajor:    return "column_major";
	default: UNREACHABLE(mpq); return "unknown matrix packing";
	}
}

inline const char* getBlockStorageString(TLayoutBlockStorage bsq)
{
	switch(bsq)
	{
	case EbsUnspecified:    return "bs_unspecified";
	case EbsShared:         return "shared";
	case EbsPacked:         return "packed";
	case EbsStd140:         return "std140";
	default: UNREACHABLE(bsq); return "unknown block storage";
	}
}

inline bool IsSampler(TBasicType type)
{
	return type > EbtGuardSamplerBegin && type < EbtGuardSamplerEnd;
}

inline bool IsIntegerSampler(TBasicType type)
{
	switch(type)
	{
	case EbtISampler2D:
	case EbtISampler3D:
	case EbtISamplerCube:
	case EbtISampler2DArray:
	case EbtUSampler2D:
	case EbtUSampler3D:
	case EbtUSamplerCube:
	case EbtUSampler2DArray:
		return true;
	case EbtSampler2D:
	case EbtSampler3D:
	case EbtSamplerCube:
	case EbtSampler2DRect:
	case EbtSamplerExternalOES:
	case EbtSampler2DArray:
	case EbtSampler2DShadow:
	case EbtSamplerCubeShadow:
	case EbtSampler2DArrayShadow:
		return false;
	default:
		assert(!IsSampler(type));
	}

	return false;
}

inline bool IsSampler2D(TBasicType type)
{
	switch(type)
	{
	case EbtSampler2D:
	case EbtISampler2D:
	case EbtUSampler2D:
	case EbtSampler2DArray:
	case EbtISampler2DArray:
	case EbtUSampler2DArray:
	case EbtSampler2DRect:
	case EbtSamplerExternalOES:
	case EbtSampler2DShadow:
	case EbtSampler2DArrayShadow:
		return true;
	case EbtSampler3D:
	case EbtISampler3D:
	case EbtUSampler3D:
	case EbtISamplerCube:
	case EbtUSamplerCube:
	case EbtSamplerCube:
	case EbtSamplerCubeShadow:
		return false;
	default:
		assert(!IsSampler(type));
	}

	return false;
}

inline bool IsSamplerCube(TBasicType type)
{
	switch(type)
	{
	case EbtSamplerCube:
	case EbtISamplerCube:
	case EbtUSamplerCube:
	case EbtSamplerCubeShadow:
		return true;
	case EbtSampler2D:
	case EbtSampler3D:
	case EbtSampler2DRect:
	case EbtSamplerExternalOES:
	case EbtSampler2DArray:
	case EbtISampler2D:
	case EbtISampler3D:
	case EbtISampler2DArray:
	case EbtUSampler2D:
	case EbtUSampler3D:
	case EbtUSampler2DArray:
	case EbtSampler2DShadow:
	case EbtSampler2DArrayShadow:
		return false;
	default:
		assert(!IsSampler(type));
	}

	return false;
}

inline bool IsSampler3D(TBasicType type)
{
	switch(type)
	{
	case EbtSampler3D:
	case EbtISampler3D:
	case EbtUSampler3D:
		return true;
	case EbtSampler2D:
	case EbtSamplerCube:
	case EbtSampler2DRect:
	case EbtSamplerExternalOES:
	case EbtSampler2DArray:
	case EbtISampler2D:
	case EbtISamplerCube:
	case EbtISampler2DArray:
	case EbtUSampler2D:
	case EbtUSamplerCube:
	case EbtUSampler2DArray:
	case EbtSampler2DShadow:
	case EbtSamplerCubeShadow:
	case EbtSampler2DArrayShadow:
		return false;
	default:
		assert(!IsSampler(type));
	}

	return false;
}

inline bool IsSamplerArray(TBasicType type)
{
	switch(type)
	{
	case EbtSampler2DArray:
	case EbtISampler2DArray:
	case EbtUSampler2DArray:
	case EbtSampler2DArrayShadow:
		return true;
	case EbtSampler2D:
	case EbtISampler2D:
	case EbtUSampler2D:
	case EbtSampler2DRect:
	case EbtSamplerExternalOES:
	case EbtSampler3D:
	case EbtISampler3D:
	case EbtUSampler3D:
	case EbtISamplerCube:
	case EbtUSamplerCube:
	case EbtSamplerCube:
	case EbtSampler2DShadow:
	case EbtSamplerCubeShadow:
		return false;
	default:
		assert(!IsSampler(type));
	}

	return false;
}

inline bool IsShadowSampler(TBasicType type)
{
	switch(type)
	{
	case EbtSampler2DShadow:
	case EbtSamplerCubeShadow:
	case EbtSampler2DArrayShadow:
		return true;
	case EbtISampler2D:
	case EbtISampler3D:
	case EbtISamplerCube:
	case EbtISampler2DArray:
	case EbtUSampler2D:
	case EbtUSampler3D:
	case EbtUSamplerCube:
	case EbtUSampler2DArray:
	case EbtSampler2D:
	case EbtSampler3D:
	case EbtSamplerCube:
	case EbtSampler2DRect:
	case EbtSamplerExternalOES:
	case EbtSampler2DArray:
		return false;
	default:
		assert(!IsSampler(type));
	}

	return false;
}

inline bool IsInteger(TBasicType type)
{
	return type == EbtInt || type == EbtUInt;
}

inline bool SupportsPrecision(TBasicType type)
{
	return type == EbtFloat || type == EbtInt || type == EbtUInt || IsSampler(type);
}

//
// Qualifiers and built-ins.  These are mainly used to see what can be read
// or written, and by the machine dependent translator to know which registers
// to allocate variables in.  Since built-ins tend to go to different registers
// than varying or uniform, it makes sense they are peers, not sub-classes.
//
enum TQualifier : unsigned char
{
	EvqTemporary,     // For temporaries (within a function), read/write
	EvqGlobal,        // For globals read/write
	EvqConstExpr,     // User defined constants
	EvqAttribute,     // Readonly
	EvqVaryingIn,     // readonly, fragment shaders only
	EvqVaryingOut,    // vertex shaders only  read/write
	EvqInvariantVaryingIn,     // readonly, fragment shaders only
	EvqInvariantVaryingOut,    // vertex shaders only  read/write
	EvqUniform,       // Readonly, vertex and fragment

	EvqVertexIn,      // Vertex shader input
	EvqFragmentOut,   // Fragment shader output
	EvqVertexOut,     // Vertex shader output
	EvqFragmentIn,    // Fragment shader input

	// pack/unpack input and output
	EvqInput,
	EvqOutput,

	// parameters
	EvqIn,
	EvqOut,
	EvqInOut,
	EvqConstReadOnly,

	// built-ins written by vertex shader
	EvqPosition,
	EvqPointSize,
	EvqInstanceID,
	EvqVertexID,

	// built-ins read by fragment shader
	EvqFragCoord,
	EvqFrontFacing,
	EvqPointCoord,

	// built-ins written by fragment shader
	EvqFragColor,
	EvqFragData,
	EvqFragDepth,

	// GLSL ES 3.0 vertex output and fragment input
	EvqSmooth,        // Incomplete qualifier, smooth is the default
	EvqFlat,          // Incomplete qualifier
	EvqSmoothOut = EvqSmooth,
	EvqFlatOut = EvqFlat,
	EvqCentroidOut,   // Implies smooth
	EvqSmoothIn,
	EvqFlatIn,
	EvqCentroidIn,    // Implies smooth

	// end of list
	EvqLast
};

struct TLayoutQualifier
{
	static TLayoutQualifier create()
	{
		TLayoutQualifier layoutQualifier;

		layoutQualifier.location = -1;
		layoutQualifier.matrixPacking = EmpUnspecified;
		layoutQualifier.blockStorage = EbsUnspecified;

		return layoutQualifier;
	}

	bool isEmpty() const
	{
		return location == -1 && matrixPacking == EmpUnspecified && blockStorage == EbsUnspecified;
	}

	int location;
	TLayoutMatrixPacking matrixPacking;
	TLayoutBlockStorage blockStorage;
};

//
// This is just for debug print out, carried along with the definitions above.
//
inline const char *getQualifierString(TQualifier qualifier)
{
	switch(qualifier)
	{
	case EvqTemporary:      return "Temporary";      break;
	case EvqGlobal:         return "Global";         break;
	case EvqConstExpr:      return "const";          break;
	case EvqConstReadOnly:  return "const";          break;
	case EvqAttribute:      return "attribute";      break;
	case EvqVaryingIn:      return "varying";        break;
	case EvqVaryingOut:     return "varying";        break;
	case EvqInvariantVaryingIn: return "invariant varying";	break;
	case EvqInvariantVaryingOut:return "invariant varying";	break;
	case EvqUniform:        return "uniform";        break;
	case EvqVertexIn:       return "in";             break;
	case EvqFragmentOut:    return "out";            break;
	case EvqVertexOut:      return "out";            break;
	case EvqFragmentIn:     return "in";             break;
	case EvqIn:             return "in";             break;
	case EvqOut:            return "out";            break;
	case EvqInOut:          return "inout";          break;
	case EvqInput:          return "input";          break;
	case EvqOutput:         return "output";         break;
	case EvqPosition:       return "Position";       break;
	case EvqPointSize:      return "PointSize";      break;
	case EvqInstanceID:     return "InstanceID";     break;
	case EvqVertexID:       return "VertexID";       break;
	case EvqFragCoord:      return "FragCoord";      break;
	case EvqFrontFacing:    return "FrontFacing";    break;
	case EvqFragColor:      return "FragColor";      break;
	case EvqFragData:       return "FragData";       break;
	case EvqFragDepth:      return "FragDepth";      break;
	case EvqSmooth:         return "Smooth";         break;
	case EvqFlat:           return "Flat";           break;
	case EvqCentroidOut:    return "CentroidOut";    break;
	case EvqSmoothIn:       return "SmoothIn";       break;
	case EvqFlatIn:         return "FlatIn";         break;
	case EvqCentroidIn:     return "CentroidIn";     break;
	default: UNREACHABLE(qualifier); return "unknown qualifier";
	}
}

#endif // _BASICTYPES_INCLUDED_
