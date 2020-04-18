#ifndef _GLUSHADERUTIL_HPP
#define _GLUSHADERUTIL_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES Utilities
 * ------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Shader utilities.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "deInt32.h"
#include "gluRenderContext.hpp"
#include "tcuVector.hpp"
#include "tcuVector.hpp"
#include "tcuMatrix.hpp"

namespace glu
{

// ShadingLanguageVersion

enum GLSLVersion
{
	GLSL_VERSION_100_ES = 0,	//!< GLSL ES 1.0
	GLSL_VERSION_300_ES,		//!< GLSL ES 3.0
	GLSL_VERSION_310_ES,		//!< GLSL ES 3.1
	GLSL_VERSION_320_ES,		//!< GLSL ES 3.2

	GLSL_VERSION_130,			//!< GLSL 1.3
	GLSL_VERSION_140,			//!< GLSL 1.4
	GLSL_VERSION_150,			//!< GLSL 1.5
	GLSL_VERSION_330,			//!< GLSL 3.0
	GLSL_VERSION_400,			//!< GLSL 4.0
	GLSL_VERSION_410,			//!< GLSL 4.1
	GLSL_VERSION_420,			//!< GLSL 4.2
	GLSL_VERSION_430,			//!< GLSL 4.3
	GLSL_VERSION_440,			//!< GLSL 4.4
	GLSL_VERSION_450,			//!< GLSL 4.5
	GLSL_VERSION_460,			//!< GLSL 4.6

	GLSL_VERSION_LAST
};

const char*		getGLSLVersionName				(GLSLVersion version);
const char*		getGLSLVersionDeclaration		(GLSLVersion version);
bool			glslVersionUsesInOutQualifiers	(GLSLVersion version);
bool			glslVersionIsES					(GLSLVersion version);
bool			isGLSLVersionSupported			(ContextType type, GLSLVersion version);
GLSLVersion		getContextTypeGLSLVersion		(ContextType type);

// ShaderType

enum ShaderType
{
	SHADERTYPE_VERTEX = 0,
	SHADERTYPE_FRAGMENT,
	SHADERTYPE_GEOMETRY,
	SHADERTYPE_TESSELLATION_CONTROL,
	SHADERTYPE_TESSELLATION_EVALUATION,
	SHADERTYPE_COMPUTE,

	SHADERTYPE_LAST
};

const char*	getShaderTypeName (ShaderType shaderType);

// Precision

enum Precision
{
	PRECISION_LOWP = 0,
	PRECISION_MEDIUMP,
	PRECISION_HIGHP,

	PRECISION_LAST
};

const char* getPrecisionName (Precision precision);

// DataType

enum DataType
{
	TYPE_INVALID	= 0,

	TYPE_FLOAT,
	TYPE_FLOAT_VEC2,
	TYPE_FLOAT_VEC3,
	TYPE_FLOAT_VEC4,
	TYPE_FLOAT_MAT2,
	TYPE_FLOAT_MAT2X3,
	TYPE_FLOAT_MAT2X4,
	TYPE_FLOAT_MAT3X2,
	TYPE_FLOAT_MAT3,
	TYPE_FLOAT_MAT3X4,
	TYPE_FLOAT_MAT4X2,
	TYPE_FLOAT_MAT4X3,
	TYPE_FLOAT_MAT4,

	TYPE_DOUBLE,
	TYPE_DOUBLE_VEC2,
	TYPE_DOUBLE_VEC3,
	TYPE_DOUBLE_VEC4,
	TYPE_DOUBLE_MAT2,
	TYPE_DOUBLE_MAT2X3,
	TYPE_DOUBLE_MAT2X4,
	TYPE_DOUBLE_MAT3X2,
	TYPE_DOUBLE_MAT3,
	TYPE_DOUBLE_MAT3X4,
	TYPE_DOUBLE_MAT4X2,
	TYPE_DOUBLE_MAT4X3,
	TYPE_DOUBLE_MAT4,

	TYPE_INT,
	TYPE_INT_VEC2,
	TYPE_INT_VEC3,
	TYPE_INT_VEC4,

	TYPE_UINT,
	TYPE_UINT_VEC2,
	TYPE_UINT_VEC3,
	TYPE_UINT_VEC4,

	TYPE_BOOL,
	TYPE_BOOL_VEC2,
	TYPE_BOOL_VEC3,
	TYPE_BOOL_VEC4,

	TYPE_SAMPLER_1D,
	TYPE_SAMPLER_2D,
	TYPE_SAMPLER_CUBE,
	TYPE_SAMPLER_1D_ARRAY,
	TYPE_SAMPLER_2D_ARRAY,
	TYPE_SAMPLER_3D,
	TYPE_SAMPLER_CUBE_ARRAY,

	TYPE_SAMPLER_1D_SHADOW,
	TYPE_SAMPLER_2D_SHADOW,
	TYPE_SAMPLER_CUBE_SHADOW,
	TYPE_SAMPLER_1D_ARRAY_SHADOW,
	TYPE_SAMPLER_2D_ARRAY_SHADOW,
	TYPE_SAMPLER_CUBE_ARRAY_SHADOW,

	TYPE_INT_SAMPLER_1D,
	TYPE_INT_SAMPLER_2D,
	TYPE_INT_SAMPLER_CUBE,
	TYPE_INT_SAMPLER_1D_ARRAY,
	TYPE_INT_SAMPLER_2D_ARRAY,
	TYPE_INT_SAMPLER_3D,
	TYPE_INT_SAMPLER_CUBE_ARRAY,

	TYPE_UINT_SAMPLER_1D,
	TYPE_UINT_SAMPLER_2D,
	TYPE_UINT_SAMPLER_CUBE,
	TYPE_UINT_SAMPLER_1D_ARRAY,
	TYPE_UINT_SAMPLER_2D_ARRAY,
	TYPE_UINT_SAMPLER_3D,
	TYPE_UINT_SAMPLER_CUBE_ARRAY,

	TYPE_SAMPLER_2D_MULTISAMPLE,
	TYPE_INT_SAMPLER_2D_MULTISAMPLE,
	TYPE_UINT_SAMPLER_2D_MULTISAMPLE,

	TYPE_IMAGE_2D,
	TYPE_IMAGE_CUBE,
	TYPE_IMAGE_2D_ARRAY,
	TYPE_IMAGE_3D,
	TYPE_IMAGE_CUBE_ARRAY,

	TYPE_INT_IMAGE_2D,
	TYPE_INT_IMAGE_CUBE,
	TYPE_INT_IMAGE_2D_ARRAY,
	TYPE_INT_IMAGE_3D,
	TYPE_INT_IMAGE_CUBE_ARRAY,

	TYPE_UINT_IMAGE_2D,
	TYPE_UINT_IMAGE_CUBE,
	TYPE_UINT_IMAGE_2D_ARRAY,
	TYPE_UINT_IMAGE_3D,
	TYPE_UINT_IMAGE_CUBE_ARRAY,

	TYPE_UINT_ATOMIC_COUNTER,

	TYPE_SAMPLER_BUFFER,
	TYPE_INT_SAMPLER_BUFFER,
	TYPE_UINT_SAMPLER_BUFFER,

	TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY,
	TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
	TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY,

	TYPE_IMAGE_BUFFER,
	TYPE_INT_IMAGE_BUFFER,
	TYPE_UINT_IMAGE_BUFFER,

	TYPE_LAST
};

const char*		getDataTypeName				(DataType dataType);
int				getDataTypeScalarSize		(DataType dataType);
DataType		getDataTypeScalarType		(DataType dataType);
DataType		getDataTypeFloatScalars		(DataType dataType);
DataType		getDataTypeDoubleScalars	(DataType dataType);
DataType		getDataTypeVector			(DataType scalarType, int size);
DataType		getDataTypeFloatVec			(int vecSize);
DataType		getDataTypeIntVec			(int vecSize);
DataType		getDataTypeUintVec			(int vecSize);
DataType		getDataTypeBoolVec			(int vecSize);
DataType		getDataTypeMatrix			(int numCols, int numRows);
DataType		getDataTypeFromGLType		(deUint32 glType);

inline bool		isDataTypeFloatOrVec		(DataType dataType)	{ return (dataType >= TYPE_FLOAT)      && (dataType <= TYPE_FLOAT_VEC4);   }
inline bool		isDataTypeDoubleOrDVec		(DataType dataType)	{ return (dataType >= TYPE_DOUBLE)     && (dataType <= TYPE_DOUBLE_VEC4);  }
inline bool		isDataTypeMatrix			(DataType dataType)	{ return ((dataType >= TYPE_FLOAT_MAT2) && (dataType <= TYPE_FLOAT_MAT4)) || ((dataType >= TYPE_DOUBLE_MAT2) && (dataType <= TYPE_DOUBLE_MAT4));  }
inline bool		isDataTypeIntOrIVec			(DataType dataType)	{ return (dataType >= TYPE_INT)        && (dataType <= TYPE_INT_VEC4);     }
inline bool		isDataTypeUintOrUVec		(DataType dataType)	{ return (dataType >= TYPE_UINT)       && (dataType <= TYPE_UINT_VEC4);    }
inline bool		isDataTypeBoolOrBVec		(DataType dataType)	{ return (dataType >= TYPE_BOOL)       && (dataType <= TYPE_BOOL_VEC4);    }
inline bool		isDataTypeScalar			(DataType dataType) { return (dataType == TYPE_FLOAT) || (dataType == TYPE_DOUBLE) ||(dataType == TYPE_INT) || (dataType == TYPE_UINT) || (dataType == TYPE_BOOL); }
inline bool		isDataTypeVector			(DataType dataType) { return deInRange32(dataType, TYPE_FLOAT_VEC2, TYPE_FLOAT_VEC4) || deInRange32(dataType, TYPE_DOUBLE_VEC2, TYPE_DOUBLE_VEC4) || deInRange32(dataType, TYPE_INT_VEC2, TYPE_INT_VEC4) || deInRange32(dataType, TYPE_UINT_VEC2, TYPE_UINT_VEC4) || deInRange32(dataType, TYPE_BOOL_VEC2, TYPE_BOOL_VEC4); }
inline bool		isDataTypeScalarOrVector	(DataType dataType) { return deInRange32(dataType, TYPE_FLOAT, TYPE_FLOAT_VEC4) || deInRange32(dataType, TYPE_DOUBLE, TYPE_DOUBLE_VEC4) || deInRange32(dataType, TYPE_INT, TYPE_INT_VEC4) || deInRange32(dataType, TYPE_UINT, TYPE_UINT_VEC4) || deInRange32(dataType, TYPE_BOOL, TYPE_BOOL_VEC4); }
inline bool		isDataTypeSampler			(DataType dataType)	{ return (dataType >= TYPE_SAMPLER_1D) && (dataType <= TYPE_UINT_SAMPLER_2D_MULTISAMPLE); }
inline bool		isDataTypeImage				(DataType dataType)	{ return (dataType >= TYPE_IMAGE_2D) && (dataType <= TYPE_UINT_IMAGE_3D); }
inline bool		isDataTypeSamplerMultisample(DataType dataType)	{ return (dataType >= TYPE_SAMPLER_2D_MULTISAMPLE) && (dataType <= TYPE_UINT_SAMPLER_2D_MULTISAMPLE); }
inline bool		isDataTypeAtomicCounter		(DataType dataType)	{ return dataType == TYPE_UINT_ATOMIC_COUNTER; }
inline bool		isDataTypeSamplerBuffer		(DataType dataType)	{ return (dataType >= TYPE_SAMPLER_BUFFER) && (dataType <= TYPE_UINT_SAMPLER_BUFFER); }
inline bool		isDataTypeSamplerMSArray	(DataType dataType)	{ return (dataType >= TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY) && (dataType <= TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY); }
inline bool		isDataTypeImageBuffer		(DataType dataType)	{ return (dataType >= TYPE_IMAGE_BUFFER) && (dataType <= TYPE_UINT_IMAGE_BUFFER); }

int				getDataTypeMatrixNumRows	(DataType dataType);
int				getDataTypeMatrixNumColumns	(DataType dataType);

int				getDataTypeNumLocations		(DataType dataType);
int				getDataTypeNumComponents	(DataType dataType);

template <typename T>
struct DataTypeTraits;

template <> struct DataTypeTraits<float>		{ enum { DATATYPE = TYPE_FLOAT			}; };
template <> struct DataTypeTraits<bool>			{ enum { DATATYPE = TYPE_BOOL			}; };
template <> struct DataTypeTraits<int>			{ enum { DATATYPE = TYPE_INT			}; };
template <> struct DataTypeTraits<deUint32>		{ enum { DATATYPE = TYPE_UINT			}; };
template <> struct DataTypeTraits<tcu::Mat2>	{ enum { DATATYPE = TYPE_FLOAT_MAT2		}; };
template <> struct DataTypeTraits<tcu::Mat2x3>	{ enum { DATATYPE = TYPE_FLOAT_MAT2X3	}; };
template <> struct DataTypeTraits<tcu::Mat2x4>	{ enum { DATATYPE = TYPE_FLOAT_MAT2X4	}; };
template <> struct DataTypeTraits<tcu::Mat3x2>	{ enum { DATATYPE = TYPE_FLOAT_MAT3X2	}; };
template <> struct DataTypeTraits<tcu::Mat3>	{ enum { DATATYPE = TYPE_FLOAT_MAT3		}; };
template <> struct DataTypeTraits<tcu::Mat3x4>	{ enum { DATATYPE = TYPE_FLOAT_MAT3X4	}; };
template <> struct DataTypeTraits<tcu::Mat4x2>	{ enum { DATATYPE = TYPE_FLOAT_MAT4X2	}; };
template <> struct DataTypeTraits<tcu::Mat4x3>	{ enum { DATATYPE = TYPE_FLOAT_MAT4X3	}; };
template <> struct DataTypeTraits<tcu::Mat4>	{ enum { DATATYPE = TYPE_FLOAT_MAT4		}; };

template <typename T, int Size>
struct DataTypeTraits<tcu::Vector<T, Size> >
{
	DE_STATIC_ASSERT(TYPE_FLOAT_VEC4 == TYPE_FLOAT + 3);
	DE_STATIC_ASSERT(TYPE_INT_VEC4 == TYPE_INT + 3);
	DE_STATIC_ASSERT(TYPE_UINT_VEC4 == TYPE_UINT + 3);
	DE_STATIC_ASSERT(TYPE_BOOL_VEC4 == TYPE_BOOL + 3);
	enum { DATATYPE = DataTypeTraits<T>::DATATYPE + Size - 1 };
};

template <typename T>
inline DataType	dataTypeOf (void) { return DataType(DataTypeTraits<T>::DATATYPE); }

} // glu

#endif // _GLUSHADERUTIL_HPP
