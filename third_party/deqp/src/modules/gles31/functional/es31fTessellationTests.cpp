/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
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
 * \brief Tessellation Tests.
 *//*--------------------------------------------------------------------*/

#include "es31fTessellationTests.hpp"
#include "glsTextureTestUtil.hpp"
#include "glsShaderLibrary.hpp"
#include "glsStateQueryUtil.hpp"
#include "gluShaderProgram.hpp"
#include "gluRenderContext.hpp"
#include "gluPixelTransfer.hpp"
#include "gluDrawUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluVarType.hpp"
#include "gluVarTypeUtil.hpp"
#include "gluCallLogWrapper.hpp"
#include "tcuTestLog.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuImageIO.hpp"
#include "tcuResource.hpp"
#include "tcuImageCompare.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deUniquePtr.hpp"
#include "deString.h"
#include "deMath.h"

#include "glwEnums.hpp"
#include "glwDefs.hpp"
#include "glwFunctions.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <set>
#include <limits>

using glu::ShaderProgram;
using glu::RenderContext;
using tcu::RenderTarget;
using tcu::TestLog;
using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using de::Random;
using de::SharedPtr;

using std::vector;
using std::string;

using namespace glw; // For GL types.

namespace deqp
{

using gls::TextureTestUtil::RandomViewport;

namespace gles31
{
namespace Functional
{

using namespace gls::StateQueryUtil;

enum
{
	MINIMUM_MAX_TESS_GEN_LEVEL = 64 //!< GL-defined minimum for GL_MAX_TESS_GEN_LEVEL.
};

static inline bool vec3XLessThan (const Vec3& a, const Vec3& b) { return a.x() < b.x(); }

template <typename IterT>
static string elemsStr (const IterT& begin, const IterT& end, int wrapLengthParam = 0, int numIndentationSpaces = 0)
{
	const string	baseIndentation	= string(numIndentationSpaces, ' ');
	const string	deepIndentation	= baseIndentation + string(4, ' ');
	const int		wrapLength		= wrapLengthParam > 0 ? wrapLengthParam : std::numeric_limits<int>::max();
	const int		length			= (int)std::distance(begin, end);
	string			result;

	if (length > wrapLength)
		result += "(amount: " + de::toString(length) + ") ";
	result += string() + "{" + (length > wrapLength ? "\n"+deepIndentation : " ");

	{
		int index = 0;
		for (IterT it = begin; it != end; ++it)
		{
			if (it != begin)
				result += string() + ", " + (index % wrapLength == 0 ? "\n"+deepIndentation : "");
			result += de::toString(*it);
			index++;
		}

		result += length > wrapLength ? "\n"+baseIndentation : " ";
	}

	result += "}";
	return result;
}

template <typename ContainerT>
static string containerStr (const ContainerT& c, int wrapLengthParam = 0, int numIndentationSpaces = 0)
{
	return elemsStr(c.begin(), c.end(), wrapLengthParam, numIndentationSpaces);
}

template <typename T, int N>
static string arrayStr (const T (&arr)[N], int wrapLengthParam = 0, int numIndentationSpaces = 0)
{
	return elemsStr(DE_ARRAY_BEGIN(arr), DE_ARRAY_END(arr), wrapLengthParam, numIndentationSpaces);
}

template <typename T, int N>
static T arrayMax (const T (&arr)[N])
{
	return *std::max_element(DE_ARRAY_BEGIN(arr), DE_ARRAY_END(arr));
}

template <typename T, typename MembT>
static vector<MembT> members (const vector<T>& objs, MembT T::* membP)
{
	vector<MembT> result(objs.size());
	for (int i = 0; i < (int)objs.size(); i++)
		result[i] = objs[i].*membP;
	return result;
}

template <typename T, int N>
static vector<T> arrayToVector (const T (&arr)[N])
{
	return vector<T>(DE_ARRAY_BEGIN(arr), DE_ARRAY_END(arr));
}

template <typename ContainerT, typename T>
static inline bool contains (const ContainerT& c, const T& key)
{
	return c.find(key) != c.end();
}

template <int Size>
static inline tcu::Vector<bool, Size> singleTrueMask (int index)
{
	DE_ASSERT(de::inBounds(index, 0, Size));
	tcu::Vector<bool, Size> result;
	result[index] = true;
	return result;
}

static int intPow (int base, int exp)
{
	DE_ASSERT(exp >= 0);
	if (exp == 0)
		return 1;
	else
	{
		const int sub = intPow(base, exp/2);
		if (exp % 2 == 0)
			return sub*sub;
		else
			return sub*sub*base;
	}
}

tcu::Surface getPixels (const glu::RenderContext& rCtx, int x, int y, int width, int height)
{
	tcu::Surface result(width, height);
	glu::readPixels(rCtx, x, y, result.getAccess());
	return result;
}

tcu::Surface getPixels (const glu::RenderContext& rCtx, const RandomViewport& vp)
{
	return getPixels(rCtx, vp.x, vp.y, vp.width, vp.height);
}

static inline void checkRenderTargetSize (const RenderTarget& renderTarget, int minSize)
{
	if (renderTarget.getWidth() < minSize || renderTarget.getHeight() < minSize)
		throw tcu::NotSupportedError("Render target width and height must be at least " + de::toString(minSize));
}

tcu::TextureLevel getPNG (const tcu::Archive& archive, const string& filename)
{
	tcu::TextureLevel result;
	tcu::ImageIO::loadPNG(result, archive, filename.c_str());
	return result;
}

static int numBasicSubobjects (const glu::VarType& type)
{
	if (type.isBasicType())
		return 1;
	else if (type.isArrayType())
		return type.getArraySize()*numBasicSubobjects(type.getElementType());
	else if (type.isStructType())
	{
		const glu::StructType&	structType	= *type.getStructPtr();
		int						result		= 0;
		for (int i = 0; i < structType.getNumMembers(); i++)
			result += numBasicSubobjects(structType.getMember(i).getType());
		return result;
	}
	else
	{
		DE_ASSERT(false);
		return -1;
	}
}

static inline int numVerticesPerPrimitive (deUint32 primitiveTypeGL)
{
	switch (primitiveTypeGL)
	{
		case GL_POINTS:		return 1;
		case GL_TRIANGLES:	return 3;
		case GL_LINES:		return 2;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

static inline void setViewport (const glw::Functions& gl, const RandomViewport& vp)
{
	gl.viewport(vp.x, vp.y, vp.width, vp.height);
}

static inline deUint32 getQueryResult (const glw::Functions& gl, deUint32 queryObject)
{
	deUint32 result = (deUint32)-1;
	gl.getQueryObjectuiv(queryObject, GL_QUERY_RESULT, &result);
	TCU_CHECK(result != (deUint32)-1);
	return result;
}

template <typename T>
static void readDataMapped (const glw::Functions& gl, deUint32 bufferTarget, int numElems, T* dst)
{
	const int							numBytes	= numElems*(int)sizeof(T);
	const T* const						mappedData	= (const T*)gl.mapBufferRange(bufferTarget, 0, numBytes, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), (string() + "glMapBufferRange(" + glu::getBufferTargetName((int)bufferTarget) + ", 0, " + de::toString(numBytes) + ", GL_MAP_READ_BIT)").c_str());
	TCU_CHECK(mappedData != DE_NULL);

	for (int i = 0; i < numElems; i++)
		dst[i] = mappedData[i];

	gl.unmapBuffer(bufferTarget);
}

template <typename T>
static vector<T> readDataMapped (const glw::Functions& gl, deUint32 bufferTarget, int numElems)
{
	vector<T> result(numElems);
	readDataMapped(gl, bufferTarget, numElems, &result[0]);
	return result;
}

namespace
{

template <typename ArgT, bool res>
struct ConstantUnaryPredicate
{
	bool operator() (const ArgT&) const { return res; }
};

//! Helper for handling simple, one-varying transform feedbacks.
template <typename VaryingT>
class TransformFeedbackHandler
{
public:
	struct Result
	{
		int					numPrimitives;
		vector<VaryingT>	varying;

		Result (void)								: numPrimitives(-1) {}
		Result (int n, const vector<VaryingT>& v)	: numPrimitives(n), varying(v) {}
	};

									TransformFeedbackHandler	(const glu::RenderContext& renderCtx, int maxNumVertices);

	Result							renderAndGetPrimitives		(deUint32 programGL, deUint32 tfPrimTypeGL, int numBindings, const glu::VertexArrayBinding* bindings, int numVertices) const;

private:
	const glu::RenderContext&		m_renderCtx;
	const glu::TransformFeedback	m_tf;
	const glu::Buffer				m_tfBuffer;
	const glu::Query				m_tfPrimQuery;
};

template <typename AttribType>
TransformFeedbackHandler<AttribType>::TransformFeedbackHandler (const glu::RenderContext& renderCtx, int maxNumVertices)
	: m_renderCtx		(renderCtx)
	, m_tf				(renderCtx)
	, m_tfBuffer		(renderCtx)
	, m_tfPrimQuery		(renderCtx)
{
	const glw::Functions&	gl			= m_renderCtx.getFunctions();
	// \note Room for 1 extra triangle, to detect if GL returns too many primitives.
	const int				bufferSize	= (maxNumVertices + 3) * (int)sizeof(AttribType);

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, *m_tfBuffer);
	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bufferSize, DE_NULL, GL_DYNAMIC_READ);
}

template <typename AttribType>
typename TransformFeedbackHandler<AttribType>::Result TransformFeedbackHandler<AttribType>::renderAndGetPrimitives (deUint32 programGL, deUint32 tfPrimTypeGL, int numBindings, const glu::VertexArrayBinding* bindings, int numVertices) const
{
	DE_ASSERT(tfPrimTypeGL == GL_POINTS || tfPrimTypeGL == GL_LINES || tfPrimTypeGL == GL_TRIANGLES);

	const glw::Functions& gl = m_renderCtx.getFunctions();

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, *m_tf);
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, *m_tfBuffer);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, *m_tfBuffer);

	gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, *m_tfPrimQuery);
	gl.beginTransformFeedback(tfPrimTypeGL);

	glu::draw(m_renderCtx, programGL, numBindings, bindings, glu::pr::Patches(numVertices));
	GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");

	gl.endTransformFeedback();
	gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	{
		const int numPrimsWritten = (int)getQueryResult(gl, *m_tfPrimQuery);
		return Result(numPrimsWritten, readDataMapped<AttribType>(gl, GL_TRANSFORM_FEEDBACK_BUFFER, numPrimsWritten * numVerticesPerPrimitive(tfPrimTypeGL)));
	}
}

template <typename T>
class SizeLessThan
{
public:
	bool operator() (const T& a, const T& b) const { return a.size() < b.size(); }
};

//! Predicate functor for comparing structs by their members.
template <typename Pred, typename T, typename MembT>
class MemberPred
{
public:
				MemberPred	(MembT T::* membP) : m_membP(membP), m_pred(Pred()) {}
	bool		operator()	(const T& a, const T& b) const { return m_pred(a.*m_membP, b.*m_membP); }

private:
	MembT T::*	m_membP;
	Pred		m_pred;
};

//! Convenience wrapper for MemberPred, because class template arguments aren't deduced based on constructor arguments.
template <template <typename> class Pred, typename T, typename MembT>
static MemberPred<Pred<MembT>, T, MembT> memberPred (MembT T::* membP) { return MemberPred<Pred<MembT>, T, MembT>(membP); }

template <typename SeqT, int Size, typename Pred>
class LexCompare
{
public:
	LexCompare (void) : m_pred(Pred()) {}

	bool operator() (const SeqT& a, const SeqT& b) const
	{
		for (int i = 0; i < Size; i++)
		{
			if (m_pred(a[i], b[i]))
				return true;
			if (m_pred(b[i], a[i]))
				return false;
		}
		return false;
	}

private:
	Pred m_pred;
};

template <int Size>
class VecLexLessThan : public LexCompare<tcu::Vector<float, Size>, Size, std::less<float> >
{
};

enum TessPrimitiveType
{
	TESSPRIMITIVETYPE_TRIANGLES = 0,
	TESSPRIMITIVETYPE_QUADS,
	TESSPRIMITIVETYPE_ISOLINES,

	TESSPRIMITIVETYPE_LAST
};

enum SpacingMode
{
	SPACINGMODE_EQUAL,
	SPACINGMODE_FRACTIONAL_ODD,
	SPACINGMODE_FRACTIONAL_EVEN,

	SPACINGMODE_LAST
};

enum Winding
{
	WINDING_CCW = 0,
	WINDING_CW,

	WINDING_LAST
};

static inline const char* getTessPrimitiveTypeShaderName (TessPrimitiveType type)
{
	switch (type)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:	return "triangles";
		case TESSPRIMITIVETYPE_QUADS:		return "quads";
		case TESSPRIMITIVETYPE_ISOLINES:	return "isolines";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static inline const char* getSpacingModeShaderName (SpacingMode mode)
{
	switch (mode)
	{
		case SPACINGMODE_EQUAL:				return "equal_spacing";
		case SPACINGMODE_FRACTIONAL_ODD:	return "fractional_odd_spacing";
		case SPACINGMODE_FRACTIONAL_EVEN:	return "fractional_even_spacing";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static inline const char* getWindingShaderName (Winding winding)
{
	switch (winding)
	{
		case WINDING_CCW:	return "ccw";
		case WINDING_CW:	return "cw";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static inline string getTessellationEvaluationInLayoutString (TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode=false)
{
	return string() + "layout (" + getTessPrimitiveTypeShaderName(primType)
								 + ", " + getSpacingModeShaderName(spacing)
								 + ", " + getWindingShaderName(winding)
								 + (usePointMode ? ", point_mode" : "")
								 + ") in;\n";
}

static inline string getTessellationEvaluationInLayoutString (TessPrimitiveType primType, SpacingMode spacing, bool usePointMode=false)
{
	return string() + "layout (" + getTessPrimitiveTypeShaderName(primType)
								 + ", " + getSpacingModeShaderName(spacing)
								 + (usePointMode ? ", point_mode" : "")
								 + ") in;\n";
}

static inline string getTessellationEvaluationInLayoutString (TessPrimitiveType primType, Winding winding, bool usePointMode=false)
{
	return string() + "layout (" + getTessPrimitiveTypeShaderName(primType)
								 + ", " + getWindingShaderName(winding)
								 + (usePointMode ? ", point_mode" : "")
								 + ") in;\n";
}

static inline string getTessellationEvaluationInLayoutString (TessPrimitiveType primType, bool usePointMode=false)
{
	return string() + "layout (" + getTessPrimitiveTypeShaderName(primType)
								 + (usePointMode ? ", point_mode" : "")
								 + ") in;\n";
}

static inline deUint32 outputPrimitiveTypeGL (TessPrimitiveType tessPrimType, bool usePointMode)
{
	if (usePointMode)
		return GL_POINTS;
	else
	{
		switch (tessPrimType)
		{
			case TESSPRIMITIVETYPE_TRIANGLES:	return GL_TRIANGLES;
			case TESSPRIMITIVETYPE_QUADS:		return GL_TRIANGLES;
			case TESSPRIMITIVETYPE_ISOLINES:	return GL_LINES;
			default:
				DE_ASSERT(false);
				return (deUint32)-1;
		}
	}
}

static inline int numInnerTessellationLevels (TessPrimitiveType primType)
{
	switch (primType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:	return 1;
		case TESSPRIMITIVETYPE_QUADS:		return 2;
		case TESSPRIMITIVETYPE_ISOLINES:	return 0;
		default: DE_ASSERT(false); return -1;
	}
}

static inline int numOuterTessellationLevels (TessPrimitiveType primType)
{
	switch (primType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:	return 3;
		case TESSPRIMITIVETYPE_QUADS:		return 4;
		case TESSPRIMITIVETYPE_ISOLINES:	return 2;
		default: DE_ASSERT(false); return -1;
	}
}

static string tessellationLevelsString (const float* inner, int numInner, const float* outer, int numOuter)
{
	DE_ASSERT(numInner >= 0 && numOuter >= 0);
	return "inner: " + elemsStr(inner, inner+numInner) + ", outer: " + elemsStr(outer, outer+numOuter);
}

static string tessellationLevelsString (const float* inner, const float* outer, TessPrimitiveType primType)
{
	return tessellationLevelsString(inner, numInnerTessellationLevels(primType), outer, numOuterTessellationLevels(primType));
}

static string tessellationLevelsString (const float* inner, const float* outer)
{
	return tessellationLevelsString(inner, 2, outer, 4);
}

static inline float getClampedTessLevel (SpacingMode mode, float tessLevel)
{
	switch (mode)
	{
		case SPACINGMODE_EQUAL:				return de::max(1.0f, tessLevel);
		case SPACINGMODE_FRACTIONAL_ODD:	return de::max(1.0f, tessLevel);
		case SPACINGMODE_FRACTIONAL_EVEN:	return de::max(2.0f, tessLevel);
		default:
			DE_ASSERT(false);
			return -1.0f;
	}
}

static inline int getRoundedTessLevel (SpacingMode mode, float clampedTessLevel)
{
	int result = (int)deFloatCeil(clampedTessLevel);

	switch (mode)
	{
		case SPACINGMODE_EQUAL:											break;
		case SPACINGMODE_FRACTIONAL_ODD:	result += 1 - result % 2;	break;
		case SPACINGMODE_FRACTIONAL_EVEN:	result += result % 2;		break;
		default:
			DE_ASSERT(false);
	}
	DE_ASSERT(de::inRange<int>(result, 1, MINIMUM_MAX_TESS_GEN_LEVEL));

	return result;
}

static int getClampedRoundedTessLevel (SpacingMode mode, float tessLevel)
{
	return getRoundedTessLevel(mode, getClampedTessLevel(mode, tessLevel));
}

//! A description of an outer edge of a triangle, quad or isolines.
//! An outer edge can be described by the index of a u/v/w coordinate
//! and the coordinate's value along that edge.
struct OuterEdgeDescription
{
	int		constantCoordinateIndex;
	float	constantCoordinateValueChoices[2];
	int		numConstantCoordinateValueChoices;

	OuterEdgeDescription (int i, float c0)			: constantCoordinateIndex(i), numConstantCoordinateValueChoices(1) { constantCoordinateValueChoices[0] = c0; }
	OuterEdgeDescription (int i, float c0, float c1)	: constantCoordinateIndex(i), numConstantCoordinateValueChoices(2) { constantCoordinateValueChoices[0] = c0; constantCoordinateValueChoices[1] = c1; }

	string description (void) const
	{
		static const char* const	coordinateNames[] = { "u", "v", "w" };
		string						result;
		for (int i = 0; i < numConstantCoordinateValueChoices; i++)
			result += string() + (i > 0 ? " or " : "") + coordinateNames[constantCoordinateIndex] + "=" + de::toString(constantCoordinateValueChoices[i]);
		return result;
	}

	bool contains (const Vec3& v) const
	{
		for (int i = 0; i < numConstantCoordinateValueChoices; i++)
			if (v[constantCoordinateIndex] == constantCoordinateValueChoices[i])
				return true;
		return false;
	}
};

static vector<OuterEdgeDescription> outerEdgeDescriptions (TessPrimitiveType primType)
{
	static const OuterEdgeDescription triangleOuterEdgeDescriptions[3] =
	{
		OuterEdgeDescription(0, 0.0f),
		OuterEdgeDescription(1, 0.0f),
		OuterEdgeDescription(2, 0.0f)
	};

	static const OuterEdgeDescription quadOuterEdgeDescriptions[4] =
	{
		OuterEdgeDescription(0, 0.0f),
		OuterEdgeDescription(1, 0.0f),
		OuterEdgeDescription(0, 1.0f),
		OuterEdgeDescription(1, 1.0f)
	};

	static const OuterEdgeDescription isolinesOuterEdgeDescriptions[1] =
	{
		OuterEdgeDescription(0, 0.0f, 1.0f),
	};

	switch (primType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:	return arrayToVector(triangleOuterEdgeDescriptions);
		case TESSPRIMITIVETYPE_QUADS:		return arrayToVector(quadOuterEdgeDescriptions);
		case TESSPRIMITIVETYPE_ISOLINES:	return arrayToVector(isolinesOuterEdgeDescriptions);
		default: DE_ASSERT(false); return vector<OuterEdgeDescription>();
	}
}

// \note The tessellation coordinates generated by this function could break some of the rules given in the spec (e.g. it may not exactly hold that u+v+w == 1.0f, or [uvw] + (1.0f-[uvw]) == 1.0f).
static vector<Vec3> generateReferenceTriangleTessCoords (SpacingMode spacingMode, int inner, int outer0, int outer1, int outer2)
{
	vector<Vec3> tessCoords;

	if (inner == 1)
	{
		if (outer0 == 1 && outer1 == 1 && outer2 == 1)
		{
			tessCoords.push_back(Vec3(1.0f, 0.0f, 0.0f));
			tessCoords.push_back(Vec3(0.0f, 1.0f, 0.0f));
			tessCoords.push_back(Vec3(0.0f, 0.0f, 1.0f));
			return tessCoords;
		}
		else
			return generateReferenceTriangleTessCoords(spacingMode, spacingMode == SPACINGMODE_FRACTIONAL_ODD ? 3 : 2,
																	outer0, outer1, outer2);
	}
	else
	{
		for (int i = 0; i < outer0; i++) { const float v = (float)i / (float)outer0; tessCoords.push_back(Vec3(	   0.0f,		   v,	1.0f - v)); }
		for (int i = 0; i < outer1; i++) { const float v = (float)i / (float)outer1; tessCoords.push_back(Vec3(1.0f - v,		0.0f,		   v)); }
		for (int i = 0; i < outer2; i++) { const float v = (float)i / (float)outer2; tessCoords.push_back(Vec3(		  v,	1.0f - v,		0.0f)); }

		const int numInnerTriangles = inner/2;
		for (int innerTriangleNdx = 0; innerTriangleNdx < numInnerTriangles; innerTriangleNdx++)
		{
			const int curInnerTriangleLevel = inner - 2*(innerTriangleNdx+1);

			if (curInnerTriangleLevel == 0)
				tessCoords.push_back(Vec3(1.0f/3.0f));
			else
			{
				const float		minUVW		= (float)(2 * (innerTriangleNdx + 1)) / (float)(3 * inner);
				const float		maxUVW		= 1.0f - 2.0f*minUVW;
				const Vec3		corners[3]	=
				{
					Vec3(maxUVW, minUVW, minUVW),
					Vec3(minUVW, maxUVW, minUVW),
					Vec3(minUVW, minUVW, maxUVW)
				};

				for (int i = 0; i < curInnerTriangleLevel; i++)
				{
					const float f = (float)i / (float)curInnerTriangleLevel;
					for (int j = 0; j < 3; j++)
						tessCoords.push_back((1.0f - f)*corners[j] + f*corners[(j+1)%3]);
				}
			}
		}

		return tessCoords;
	}
}

static int referenceTriangleNonPointModePrimitiveCount (SpacingMode spacingMode, int inner, int outer0, int outer1, int outer2)
{
	if (inner == 1)
	{
		if (outer0 == 1 && outer1 == 1 && outer2 == 1)
			return 1;
		else
			return referenceTriangleNonPointModePrimitiveCount(spacingMode, spacingMode == SPACINGMODE_FRACTIONAL_ODD ? 3 : 2,
																			outer0, outer1, outer2);
	}
	else
	{
		int result = outer0 + outer1 + outer2;

		const int numInnerTriangles = inner/2;
		for (int innerTriangleNdx = 0; innerTriangleNdx < numInnerTriangles; innerTriangleNdx++)
		{
			const int curInnerTriangleLevel = inner - 2*(innerTriangleNdx+1);

			if (curInnerTriangleLevel == 1)
				result += 4;
			else
				result += 2*3*curInnerTriangleLevel;
		}

		return result;
	}
}

// \note The tessellation coordinates generated by this function could break some of the rules given in the spec (e.g. it may not exactly hold that [uv] + (1.0f-[uv]) == 1.0f).
static vector<Vec3> generateReferenceQuadTessCoords (SpacingMode spacingMode, int inner0, int inner1, int outer0, int outer1, int outer2, int outer3)
{
	vector<Vec3> tessCoords;

	if (inner0 == 1 || inner1 == 1)
	{
		if (inner0 == 1 && inner1 == 1 && outer0 == 1 && outer1 == 1 && outer2 == 1 && outer3 == 1)
		{
			tessCoords.push_back(Vec3(0.0f, 0.0f, 0.0f));
			tessCoords.push_back(Vec3(1.0f, 0.0f, 0.0f));
			tessCoords.push_back(Vec3(0.0f, 1.0f, 0.0f));
			tessCoords.push_back(Vec3(1.0f, 1.0f, 0.0f));
			return tessCoords;
		}
		else
			return generateReferenceQuadTessCoords(spacingMode, inner0 > 1 ? inner0 : spacingMode == SPACINGMODE_FRACTIONAL_ODD ? 3 : 2,
																inner1 > 1 ? inner1 : spacingMode == SPACINGMODE_FRACTIONAL_ODD ? 3 : 2,
																outer0, outer1, outer2, outer3);
	}
	else
	{
		for (int i = 0; i < outer0; i++) { const float v = (float)i / (float)outer0; tessCoords.push_back(Vec3(0.0f,	v,			0.0f)); }
		for (int i = 0; i < outer1; i++) { const float v = (float)i / (float)outer1; tessCoords.push_back(Vec3(1.0f-v,	0.0f,		0.0f)); }
		for (int i = 0; i < outer2; i++) { const float v = (float)i / (float)outer2; tessCoords.push_back(Vec3(1.0f,	1.0f-v,		0.0f)); }
		for (int i = 0; i < outer3; i++) { const float v = (float)i / (float)outer3; tessCoords.push_back(Vec3(v,		1.0f,		0.0f)); }

		for (int innerVtxY = 0; innerVtxY < inner1-1; innerVtxY++)
		for (int innerVtxX = 0; innerVtxX < inner0-1; innerVtxX++)
			tessCoords.push_back(Vec3((float)(innerVtxX + 1) / (float)inner0,
									  (float)(innerVtxY + 1) / (float)inner1,
									  0.0f));

		return tessCoords;
	}
}

static int referenceQuadNonPointModePrimitiveCount (SpacingMode spacingMode, int inner0, int inner1, int outer0, int outer1, int outer2, int outer3)
{
	vector<Vec3> tessCoords;

	if (inner0 == 1 || inner1 == 1)
	{
		if (inner0 == 1 && inner1 == 1 && outer0 == 1 && outer1 == 1 && outer2 == 1 && outer3 == 1)
			return 2;
		else
			return referenceQuadNonPointModePrimitiveCount(spacingMode, inner0 > 1 ? inner0 : spacingMode == SPACINGMODE_FRACTIONAL_ODD ? 3 : 2,
																		inner1 > 1 ? inner1 : spacingMode == SPACINGMODE_FRACTIONAL_ODD ? 3 : 2,
																		outer0, outer1, outer2, outer3);
	}
	else
		return 2*(inner0-2)*(inner1-2) + 2*(inner0-2) + 2*(inner1-2) + outer0+outer1+outer2+outer3;
}

// \note The tessellation coordinates generated by this function could break some of the rules given in the spec (e.g. it may not exactly hold that [uv] + (1.0f-[uv]) == 1.0f).
static vector<Vec3> generateReferenceIsolineTessCoords (int outer0, int outer1)
{
	vector<Vec3> tessCoords;

	for (int y = 0; y < outer0;		y++)
	for (int x = 0; x < outer1+1;	x++)
		tessCoords.push_back(Vec3((float)x / (float)outer1,
												  (float)y / (float)outer0,
												  0.0f));

	return tessCoords;
}

static int referenceIsolineNonPointModePrimitiveCount (int outer0, int outer1)
{
	return outer0*outer1;
}

static void getClampedRoundedTriangleTessLevels (SpacingMode spacingMode, const float* innerSrc, const float* outerSrc, int* innerDst, int *outerDst)
{
	innerDst[0] = getClampedRoundedTessLevel(spacingMode, innerSrc[0]);
	for (int i = 0; i < 3; i++)
		outerDst[i] = getClampedRoundedTessLevel(spacingMode, outerSrc[i]);
}

static void getClampedRoundedQuadTessLevels (SpacingMode spacingMode, const float* innerSrc, const float* outerSrc, int* innerDst, int *outerDst)
{
	for (int i = 0; i < 2; i++)
		innerDst[i] = getClampedRoundedTessLevel(spacingMode, innerSrc[i]);
	for (int i = 0; i < 4; i++)
		outerDst[i] = getClampedRoundedTessLevel(spacingMode, outerSrc[i]);
}

static void getClampedRoundedIsolineTessLevels (SpacingMode spacingMode, const float* outerSrc, int* outerDst)
{
	outerDst[0] = getClampedRoundedTessLevel(SPACINGMODE_EQUAL,	outerSrc[0]);
	outerDst[1] = getClampedRoundedTessLevel(spacingMode,		outerSrc[1]);
}

static inline bool isPatchDiscarded (TessPrimitiveType primitiveType, const float* outerLevels)
{
	const int numOuterLevels = numOuterTessellationLevels(primitiveType);
	for (int i = 0; i < numOuterLevels; i++)
		if (outerLevels[i] <= 0.0f)
			return true;
	return false;
}

static vector<Vec3> generateReferenceTessCoords (TessPrimitiveType primitiveType, SpacingMode spacingMode, const float* innerLevels, const float* outerLevels)
{
	if (isPatchDiscarded(primitiveType, outerLevels))
		return vector<Vec3>();

	switch (primitiveType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:
		{
			int inner;
			int outer[3];
			getClampedRoundedTriangleTessLevels(spacingMode, innerLevels, outerLevels, &inner, &outer[0]);

			if (spacingMode != SPACINGMODE_EQUAL)
			{
				// \note For fractional spacing modes, exact results are implementation-defined except in special cases.
				DE_ASSERT(de::abs(innerLevels[0] - (float)inner) < 0.001f);
				for (int i = 0; i < 3; i++)
					DE_ASSERT(de::abs(outerLevels[i] - (float)outer[i]) < 0.001f);
				DE_ASSERT(inner > 1 || (outer[0] == 1 && outer[1] == 1 && outer[2] == 1));
			}

			return generateReferenceTriangleTessCoords(spacingMode, inner, outer[0], outer[1], outer[2]);
		}

		case TESSPRIMITIVETYPE_QUADS:
		{
			int inner[2];
			int outer[4];
			getClampedRoundedQuadTessLevels(spacingMode, innerLevels, outerLevels, &inner[0], &outer[0]);

			if (spacingMode != SPACINGMODE_EQUAL)
			{
				// \note For fractional spacing modes, exact results are implementation-defined except in special cases.
				for (int i = 0; i < 2; i++)
					DE_ASSERT(de::abs(innerLevels[i] - (float)inner[i]) < 0.001f);
				for (int i = 0; i < 4; i++)
					DE_ASSERT(de::abs(outerLevels[i] - (float)outer[i]) < 0.001f);

				DE_ASSERT((inner[0] > 1 && inner[1] > 1) || (inner[0] == 1 && inner[1] == 1 && outer[0] == 1 && outer[1] == 1 && outer[2] == 1 && outer[3] == 1));
			}

			return generateReferenceQuadTessCoords(spacingMode, inner[0], inner[1], outer[0], outer[1], outer[2], outer[3]);
		}

		case TESSPRIMITIVETYPE_ISOLINES:
		{
			int outer[2];
			getClampedRoundedIsolineTessLevels(spacingMode, &outerLevels[0], &outer[0]);

			if (spacingMode != SPACINGMODE_EQUAL)
			{
				// \note For fractional spacing modes, exact results are implementation-defined except in special cases.
				DE_ASSERT(de::abs(outerLevels[1] - (float)outer[1]) < 0.001f);
			}

			return generateReferenceIsolineTessCoords(outer[0], outer[1]);
		}

		default:
			DE_ASSERT(false);
			return vector<Vec3>();
	}
}

static int referencePointModePrimitiveCount (TessPrimitiveType primitiveType, SpacingMode spacingMode, const float* innerLevels, const float* outerLevels)
{
	if (isPatchDiscarded(primitiveType, outerLevels))
		return 0;

	switch (primitiveType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:
		{
			int inner;
			int outer[3];
			getClampedRoundedTriangleTessLevels(spacingMode, innerLevels, outerLevels, &inner, &outer[0]);
			return (int)generateReferenceTriangleTessCoords(spacingMode, inner, outer[0], outer[1], outer[2]).size();
		}

		case TESSPRIMITIVETYPE_QUADS:
		{
			int inner[2];
			int outer[4];
			getClampedRoundedQuadTessLevels(spacingMode, innerLevels, outerLevels, &inner[0], &outer[0]);
			return (int)generateReferenceQuadTessCoords(spacingMode, inner[0], inner[1], outer[0], outer[1], outer[2], outer[3]).size();
		}

		case TESSPRIMITIVETYPE_ISOLINES:
		{
			int outer[2];
			getClampedRoundedIsolineTessLevels(spacingMode, &outerLevels[0], &outer[0]);
			return (int)generateReferenceIsolineTessCoords(outer[0], outer[1]).size();
		}

		default:
			DE_ASSERT(false);
			return -1;
	}
}

static int referenceNonPointModePrimitiveCount (TessPrimitiveType primitiveType, SpacingMode spacingMode, const float* innerLevels, const float* outerLevels)
{
	if (isPatchDiscarded(primitiveType, outerLevels))
		return 0;

	switch (primitiveType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:
		{
			int inner;
			int outer[3];
			getClampedRoundedTriangleTessLevels(spacingMode, innerLevels, outerLevels, &inner, &outer[0]);
			return referenceTriangleNonPointModePrimitiveCount(spacingMode, inner, outer[0], outer[1], outer[2]);
		}

		case TESSPRIMITIVETYPE_QUADS:
		{
			int inner[2];
			int outer[4];
			getClampedRoundedQuadTessLevels(spacingMode, innerLevels, outerLevels, &inner[0], &outer[0]);
			return referenceQuadNonPointModePrimitiveCount(spacingMode, inner[0], inner[1], outer[0], outer[1], outer[2], outer[3]);
		}

		case TESSPRIMITIVETYPE_ISOLINES:
		{
			int outer[2];
			getClampedRoundedIsolineTessLevels(spacingMode, &outerLevels[0], &outer[0]);
			return referenceIsolineNonPointModePrimitiveCount(outer[0], outer[1]);
		}

		default:
			DE_ASSERT(false);
			return -1;
	}
}

static int referencePrimitiveCount (TessPrimitiveType primitiveType, SpacingMode spacingMode, bool usePointMode, const float* innerLevels, const float* outerLevels)
{
	return usePointMode ? referencePointModePrimitiveCount		(primitiveType, spacingMode, innerLevels, outerLevels)
						: referenceNonPointModePrimitiveCount	(primitiveType, spacingMode, innerLevels, outerLevels);
}

static int referenceVertexCount (TessPrimitiveType primitiveType, SpacingMode spacingMode, bool usePointMode, const float* innerLevels, const float* outerLevels)
{
	return referencePrimitiveCount(primitiveType, spacingMode, usePointMode, innerLevels, outerLevels)
		   * numVerticesPerPrimitive(outputPrimitiveTypeGL(primitiveType, usePointMode));
}

//! Helper for calling referenceVertexCount multiple times with different tessellation levels.
//! \note Levels contains inner and outer levels, per patch, in order IIOOOO. The full 6 levels must always be present, irrespective of primitiveType.
static int multiplePatchReferenceVertexCount (TessPrimitiveType primitiveType, SpacingMode spacingMode, bool usePointMode, const float* levels, int numPatches)
{
	int result = 0;
	for (int patchNdx = 0; patchNdx < numPatches; patchNdx++)
		result += referenceVertexCount(primitiveType, spacingMode, usePointMode, &levels[6*patchNdx + 0], &levels[6*patchNdx + 2]);
	return result;
}

vector<float> generateRandomPatchTessLevels (int numPatches, int constantOuterLevelIndex, float constantOuterLevel, de::Random& rnd)
{
	vector<float> tessLevels(numPatches*6);

	for (int patchNdx = 0; patchNdx < numPatches; patchNdx++)
	{
		float* const inner = &tessLevels[patchNdx*6 + 0];
		float* const outer = &tessLevels[patchNdx*6 + 2];

		for (int j = 0; j < 2; j++)
			inner[j] = rnd.getFloat(1.0f, 62.0f);
		for (int j = 0; j < 4; j++)
			outer[j] = j == constantOuterLevelIndex ? constantOuterLevel : rnd.getFloat(1.0f, 62.0f);
	}

	return tessLevels;
}

static inline void drawPoint (tcu::Surface& dst, int centerX, int centerY, const tcu::RGBA& color, int size)
{
	const int width		= dst.getWidth();
	const int height	= dst.getHeight();
	DE_ASSERT(de::inBounds(centerX, 0, width) && de::inBounds(centerY, 0, height));
	DE_ASSERT(size > 0);

	for (int yOff = -((size-1)/2); yOff <= size/2; yOff++)
	for (int xOff = -((size-1)/2); xOff <= size/2; xOff++)
	{
		const int pixX = centerX + xOff;
		const int pixY = centerY + yOff;
		if (de::inBounds(pixX, 0, width) && de::inBounds(pixY, 0, height))
			dst.setPixel(pixX, pixY, color);
	}
}

static void drawTessCoordPoint (tcu::Surface& dst, TessPrimitiveType primitiveType, const Vec3& pt, const tcu::RGBA& color, int size)
{
	// \note These coordinates should match the description in the log message in TessCoordCase::iterate.

	static const Vec2 triangleCorners[3] =
	{
		Vec2(0.95f, 0.95f),
		Vec2(0.5f,  0.95f - 0.9f*deFloatSqrt(3.0f/4.0f)),
		Vec2(0.05f, 0.95f)
	};

	static const float quadIsolineLDRU[4] =
	{
		0.1f, 0.9f, 0.9f, 0.1f
	};

	const Vec2 dstPos = primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? pt.x()*triangleCorners[0]
																	 + pt.y()*triangleCorners[1]
																	 + pt.z()*triangleCorners[2]

					  : primitiveType == TESSPRIMITIVETYPE_QUADS ||
						primitiveType == TESSPRIMITIVETYPE_ISOLINES ? Vec2((1.0f - pt.x())*quadIsolineLDRU[0] + pt.x()*quadIsolineLDRU[2],
																		   (1.0f - pt.y())*quadIsolineLDRU[1] + pt.y()*quadIsolineLDRU[3])

					  : Vec2(-1.0f);

	drawPoint(dst, (int)(dstPos.x() * (float)dst.getWidth()), (int)(dstPos.y() * (float)dst.getHeight()), color, size);
}

static void drawTessCoordVisualization (tcu::Surface& dst, TessPrimitiveType primitiveType, const vector<Vec3>& coords)
{
	const int		imageWidth		= 256;
	const int		imageHeight		= 256;
	dst.setSize(imageWidth, imageHeight);

	tcu::clear(dst.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	for (int i = 0; i < (int)coords.size(); i++)
		drawTessCoordPoint(dst, primitiveType, coords[i], tcu::RGBA::white(), 2);
}

static int binarySearchFirstVec3WithXAtLeast (const vector<Vec3>& sorted, float x)
{
	const Vec3 ref(x, 0.0f, 0.0f);
	const vector<Vec3>::const_iterator first = std::lower_bound(sorted.begin(), sorted.end(), ref, vec3XLessThan);
	if (first == sorted.end())
		return -1;
	return (int)std::distance(sorted.begin(), first);
}

template <typename T, typename P>
static vector<T> sorted (const vector<T>& unsorted, P pred)
{
	vector<T> result = unsorted;
	std::sort(result.begin(), result.end(), pred);
	return result;
}

template <typename T>
static vector<T> sorted (const vector<T>& unsorted)
{
	vector<T> result = unsorted;
	std::sort(result.begin(), result.end());
	return result;
}

// Check that all points in subset are (approximately) present also in superset.
static bool oneWayComparePointSets (TestLog&				log,
									tcu::Surface&			errorDst,
									TessPrimitiveType		primitiveType,
									const vector<Vec3>&		subset,
									const vector<Vec3>&		superset,
									const char*				subsetName,
									const char*				supersetName,
									const tcu::RGBA&		errorColor)
{
	const vector<Vec3>	supersetSorted			= sorted(superset, vec3XLessThan);
	const float			epsilon					= 0.01f;
	const int			maxNumFailurePrints		= 5;
	int					numFailuresDetected		= 0;

	for (int subNdx = 0; subNdx < (int)subset.size(); subNdx++)
	{
		const Vec3& subPt = subset[subNdx];

		bool matchFound = false;

		{
			// Binary search the index of the first point in supersetSorted with x in the [subPt.x() - epsilon, subPt.x() + epsilon] range.
			const Vec3	matchMin			= subPt - epsilon;
			const Vec3	matchMax			= subPt + epsilon;
			int			firstCandidateNdx	= binarySearchFirstVec3WithXAtLeast(supersetSorted, matchMin.x());

			if (firstCandidateNdx >= 0)
			{
				// Compare subPt to all points in supersetSorted with x in the [subPt.x() - epsilon, subPt.x() + epsilon] range.
				for (int superNdx = firstCandidateNdx; superNdx < (int)supersetSorted.size() && supersetSorted[superNdx].x() <= matchMax.x(); superNdx++)
				{
					const Vec3& superPt = supersetSorted[superNdx];

					if (tcu::boolAll(tcu::greaterThanEqual	(superPt, matchMin)) &&
						tcu::boolAll(tcu::lessThanEqual		(superPt, matchMax)))
					{
						matchFound = true;
						break;
					}
				}
			}
		}

		if (!matchFound)
		{
			numFailuresDetected++;
			if (numFailuresDetected < maxNumFailurePrints)
				log << TestLog::Message << "Failure: no matching " << supersetName << " point found for " << subsetName << " point " << subPt << TestLog::EndMessage;
			else if (numFailuresDetected == maxNumFailurePrints)
				log << TestLog::Message << "Note: More errors follow" << TestLog::EndMessage;

			drawTessCoordPoint(errorDst, primitiveType, subPt, errorColor, 4);
		}
	}

	return numFailuresDetected == 0;
}

static bool compareTessCoords (TestLog& log, TessPrimitiveType primitiveType, const vector<Vec3>& refCoords, const vector<Vec3>& resCoords)
{
	tcu::Surface	refVisual;
	tcu::Surface	resVisual;
	bool			success = true;

	drawTessCoordVisualization(refVisual, primitiveType, refCoords);
	drawTessCoordVisualization(resVisual, primitiveType, resCoords);

	// Check that all points in reference also exist in result.
	success = oneWayComparePointSets(log, refVisual, primitiveType, refCoords, resCoords, "reference", "result", tcu::RGBA::blue()) && success;
	// Check that all points in result also exist in reference.
	success = oneWayComparePointSets(log, resVisual, primitiveType, resCoords, refCoords, "result", "reference", tcu::RGBA::red()) && success;

	if (!success)
	{
		log << TestLog::Message << "Note: in the following reference visualization, points that are missing in result point set are blue (if any)" << TestLog::EndMessage
			<< TestLog::Image("RefTessCoordVisualization", "Reference tessCoord visualization", refVisual)
			<< TestLog::Message << "Note: in the following result visualization, points that are missing in reference point set are red (if any)" << TestLog::EndMessage;
	}

	log << TestLog::Image("ResTessCoordVisualization", "Result tessCoord visualization", resVisual);

	return success;
}

namespace VerifyFractionalSpacingSingleInternal
{

struct Segment
{
	int		index; //!< Index of left coordinate in sortedXCoords.
	float	length;
	Segment (void)						: index(-1),		length(-1.0f)	{}
	Segment (int index_, float length_)	: index(index_),	length(length_)	{}

	static vector<float> lengths (const vector<Segment>& segments) { return members(segments, &Segment::length); }
};

}

/*--------------------------------------------------------------------*//*!
 * \brief Verify fractional spacing conditions for a single line
 *
 * Verify that the splitting of an edge (resulting from e.g. an isoline
 * with outer levels { 1.0, tessLevel }) with a given fractional spacing
 * mode fulfills certain conditions given in the spec.
 *
 * Note that some conditions can't be checked from just one line
 * (specifically, that the additional segment decreases monotonically
 * length and the requirement that the additional segments be placed
 * identically for identical values of clamped level).
 *
 * Therefore, the function stores some values to additionalSegmentLengthDst
 * and additionalSegmentLocationDst that can later be given to
 * verifyFractionalSpacingMultiple(). A negative value in length means that
 * no additional segments are present, i.e. there's just one segment.
 * A negative value in location means that the value wasn't determinable,
 * i.e. all segments had same length.
 * The values are not stored if false is returned.
 *//*--------------------------------------------------------------------*/
static bool verifyFractionalSpacingSingle (TestLog& log, SpacingMode spacingMode, float tessLevel, const vector<float>& coords, float& additionalSegmentLengthDst, int& additionalSegmentLocationDst)
{
	using namespace VerifyFractionalSpacingSingleInternal;

	DE_ASSERT(spacingMode == SPACINGMODE_FRACTIONAL_ODD || spacingMode == SPACINGMODE_FRACTIONAL_EVEN);

	const float				clampedLevel	= getClampedTessLevel(spacingMode, tessLevel);
	const int				finalLevel		= getRoundedTessLevel(spacingMode, clampedLevel);
	const vector<float>		sortedCoords	= sorted(coords);
	string					failNote		= "Note: tessellation level is " + de::toString(tessLevel) + "\nNote: sorted coordinates are:\n    " + containerStr(sortedCoords);

	if ((int)coords.size() != finalLevel + 1)
	{
		log << TestLog::Message << "Failure: number of vertices is " << coords.size() << "; expected " << finalLevel + 1
			<< " (clamped tessellation level is " << clampedLevel << ")"
			<< "; final level (clamped level rounded up to " << (spacingMode == SPACINGMODE_FRACTIONAL_EVEN ? "even" : "odd") << ") is " << finalLevel
			<< " and should equal the number of segments, i.e. number of vertices minus 1" << TestLog::EndMessage
			<< TestLog::Message << failNote << TestLog::EndMessage;
		return false;
	}

	if (sortedCoords[0] != 0.0f || sortedCoords.back() != 1.0f)
	{
		log << TestLog::Message << "Failure: smallest coordinate should be 0.0 and biggest should be 1.0" << TestLog::EndMessage
			<< TestLog::Message << failNote << TestLog::EndMessage;
		return false;
	}

	{
		vector<Segment> segments(finalLevel);
		for (int i = 0; i < finalLevel; i++)
			segments[i] = Segment(i, sortedCoords[i+1] - sortedCoords[i]);

		failNote += "\nNote: segment lengths are, from left to right:\n    " + containerStr(Segment::lengths(segments));

		{
			// Divide segments to two different groups based on length.

			vector<Segment> segmentsA;
			vector<Segment> segmentsB;
			segmentsA.push_back(segments[0]);

			for (int segNdx = 1; segNdx < (int)segments.size(); segNdx++)
			{
				const float		epsilon		= 0.001f;
				const Segment&	seg			= segments[segNdx];

				if (de::abs(seg.length - segmentsA[0].length) < epsilon)
					segmentsA.push_back(seg);
				else if (segmentsB.empty() || de::abs(seg.length - segmentsB[0].length) < epsilon)
					segmentsB.push_back(seg);
				else
				{
					log << TestLog::Message << "Failure: couldn't divide segments to 2 groups by length; "
											<< "e.g. segment of length " << seg.length << " isn't approximately equal to either "
											<< segmentsA[0].length << " or " << segmentsB[0].length << TestLog::EndMessage
											<< TestLog::Message << failNote << TestLog::EndMessage;
					return false;
				}
			}

			if (clampedLevel == (float)finalLevel)
			{
				// All segments should be of equal length.
				if (!segmentsA.empty() && !segmentsB.empty())
				{
					log << TestLog::Message << "Failure: clamped and final tessellation level are equal, but not all segments are of equal length." << TestLog::EndMessage
						<< TestLog::Message << failNote << TestLog::EndMessage;
					return false;
				}
			}

			if (segmentsA.empty() || segmentsB.empty()) // All segments have same length. This is ok.
			{
				additionalSegmentLengthDst		= segments.size() == 1 ? -1.0f : segments[0].length;
				additionalSegmentLocationDst	= -1;
				return true;
			}

			if (segmentsA.size() != 2 && segmentsB.size() != 2)
			{
				log << TestLog::Message << "Failure: when dividing the segments to 2 groups by length, neither of the two groups has exactly 2 or 0 segments in it" << TestLog::EndMessage
					<< TestLog::Message << failNote << TestLog::EndMessage;
				return false;
			}

			// For convenience, arrange so that the 2-segment group is segmentsB.
			if (segmentsB.size() != 2)
				std::swap(segmentsA, segmentsB);

			// \note For 4-segment lines both segmentsA and segmentsB have 2 segments each.
			//		 Thus, we can't be sure which ones were meant as the additional segments.
			//		 We give the benefit of the doubt by assuming that they're the shorter
			//		 ones (as they should).

			if (segmentsA.size() != 2)
			{
				if (segmentsB[0].length > segmentsA[0].length + 0.001f)
				{
					log << TestLog::Message << "Failure: the two additional segments are longer than the other segments" << TestLog::EndMessage
						<< TestLog::Message << failNote << TestLog::EndMessage;
					return false;
				}
			}
			else
			{
				// We have 2 segmentsA and 2 segmentsB, ensure segmentsB has the shorter lengths
				if (segmentsB[0].length > segmentsA[0].length)
					std::swap(segmentsA, segmentsB);
			}

			// Check that the additional segments are placed symmetrically.
			if (segmentsB[0].index + segmentsB[1].index + 1 != (int)segments.size())
			{
				log << TestLog::Message << "Failure: the two additional segments aren't placed symmetrically; "
										<< "one is at index " << segmentsB[0].index << " and other is at index " << segmentsB[1].index
										<< " (note: the two indexes should sum to " << (int)segments.size()-1 << ", i.e. numberOfSegments-1)" << TestLog::EndMessage
					<< TestLog::Message << failNote << TestLog::EndMessage;
				return false;
			}

			additionalSegmentLengthDst = segmentsB[0].length;
			if (segmentsA.size() != 2)
				additionalSegmentLocationDst = de::min(segmentsB[0].index, segmentsB[1].index);
			else
				additionalSegmentLocationDst = segmentsB[0].length < segmentsA[0].length - 0.001f ? de::min(segmentsB[0].index, segmentsB[1].index)
											 : -1; // \note -1 when can't reliably decide which ones are the additional segments, a or b.

			return true;
		}
	}
}

namespace VerifyFractionalSpacingMultipleInternal
{

struct LineData
{
	float	tessLevel;
	float	additionalSegmentLength;
	int		additionalSegmentLocation;
	LineData (float lev, float len, int loc) : tessLevel(lev), additionalSegmentLength(len), additionalSegmentLocation(loc) {}
};

}

/*--------------------------------------------------------------------*//*!
 * \brief Verify fractional spacing conditions between multiple lines
 *
 * Verify the fractional spacing conditions that are not checked in
 * verifyFractionalSpacingSingle(). Uses values given by said function
 * as parameters, in addition to the spacing mode and tessellation level.
 *//*--------------------------------------------------------------------*/
static bool verifyFractionalSpacingMultiple (TestLog& log, SpacingMode spacingMode, const vector<float>& tessLevels, const vector<float>& additionalSegmentLengths, const vector<int>& additionalSegmentLocations)
{
	using namespace VerifyFractionalSpacingMultipleInternal;

	DE_ASSERT(spacingMode == SPACINGMODE_FRACTIONAL_ODD || spacingMode == SPACINGMODE_FRACTIONAL_EVEN);
	DE_ASSERT(tessLevels.size() == additionalSegmentLengths.size() &&
			  tessLevels.size() == additionalSegmentLocations.size());

	vector<LineData> lineDatas;

	for (int i = 0; i < (int)tessLevels.size(); i++)
		lineDatas.push_back(LineData(tessLevels[i], additionalSegmentLengths[i], additionalSegmentLocations[i]));

	{
		const vector<LineData> lineDatasSortedByLevel = sorted(lineDatas, memberPred<std::less>(&LineData::tessLevel));

		// Check that lines with identical clamped tessellation levels have identical additionalSegmentLocation.

		for (int lineNdx = 1; lineNdx < (int)lineDatasSortedByLevel.size(); lineNdx++)
		{
			const LineData& curData		= lineDatasSortedByLevel[lineNdx];
			const LineData& prevData	= lineDatasSortedByLevel[lineNdx-1];

			if (curData.additionalSegmentLocation < 0 || prevData.additionalSegmentLocation < 0)
				continue; // Unknown locations, skip.

			if (getClampedTessLevel(spacingMode, curData.tessLevel) == getClampedTessLevel(spacingMode, prevData.tessLevel) &&
				curData.additionalSegmentLocation != prevData.additionalSegmentLocation)
			{
				log << TestLog::Message << "Failure: additional segments not located identically for two edges with identical clamped tessellation levels" << TestLog::EndMessage
					<< TestLog::Message << "Note: tessellation levels are " << curData.tessLevel << " and " << prevData.tessLevel
										<< " (clamped level " << getClampedTessLevel(spacingMode, curData.tessLevel) << ")"
										<< "; but first additional segments located at indices "
										<< curData.additionalSegmentLocation << " and " << prevData.additionalSegmentLocation << ", respectively" << TestLog::EndMessage;
				return false;
			}
		}

		// Check that, among lines with same clamped rounded tessellation level, additionalSegmentLength is monotonically decreasing with "clampedRoundedTessLevel - clampedTessLevel" (the "fraction").

		for (int lineNdx = 1; lineNdx < (int)lineDatasSortedByLevel.size(); lineNdx++)
		{
			const LineData&		curData				= lineDatasSortedByLevel[lineNdx];
			const LineData&		prevData			= lineDatasSortedByLevel[lineNdx-1];

			if (curData.additionalSegmentLength < 0.0f || prevData.additionalSegmentLength < 0.0f)
				continue; // Unknown segment lengths, skip.

			const float			curClampedLevel		= getClampedTessLevel(spacingMode, curData.tessLevel);
			const float			prevClampedLevel	= getClampedTessLevel(spacingMode, prevData.tessLevel);
			const int			curFinalLevel		= getRoundedTessLevel(spacingMode, curClampedLevel);
			const int			prevFinalLevel		= getRoundedTessLevel(spacingMode, prevClampedLevel);

			if (curFinalLevel != prevFinalLevel)
				continue;

			const float			curFraction		= (float)curFinalLevel - curClampedLevel;
			const float			prevFraction	= (float)prevFinalLevel - prevClampedLevel;

			if (curData.additionalSegmentLength < prevData.additionalSegmentLength ||
				(curClampedLevel == prevClampedLevel && curData.additionalSegmentLength != prevData.additionalSegmentLength))
			{
				log << TestLog::Message << "Failure: additional segment length isn't monotonically decreasing with the fraction <n> - <f>, among edges with same final tessellation level" << TestLog::EndMessage
					<< TestLog::Message << "Note: <f> stands for the clamped tessellation level and <n> for the final (rounded and clamped) tessellation level" << TestLog::EndMessage
					<< TestLog::Message << "Note: two edges have tessellation levels " << prevData.tessLevel << " and " << curData.tessLevel << " respectively"
										<< ", clamped " << prevClampedLevel << " and " << curClampedLevel << ", final " << prevFinalLevel << " and " << curFinalLevel
										<< "; fractions are " << prevFraction << " and " << curFraction
										<< ", but resulted in segment lengths " << prevData.additionalSegmentLength << " and " << curData.additionalSegmentLength << TestLog::EndMessage;
				return false;
			}
		}
	}

	return true;
}

//! Compare triangle sets, ignoring triangle order and vertex order within triangle, and possibly exclude some triangles too.
template <typename IsTriangleRelevantT>
static bool compareTriangleSets (const vector<Vec3>&			coordsA,
								 const vector<Vec3>&			coordsB,
								 TestLog&						log,
								 const IsTriangleRelevantT&		isTriangleRelevant,
								 const char*					ignoredTriangleDescription = DE_NULL)
{
	typedef tcu::Vector<Vec3, 3>							Triangle;
	typedef LexCompare<Triangle, 3, VecLexLessThan<3> >		TriangleLexLessThan;
	typedef std::set<Triangle, TriangleLexLessThan>			TriangleSet;

	DE_ASSERT(coordsA.size() % 3 == 0 && coordsB.size() % 3 == 0);

	const int		numTrianglesA = (int)coordsA.size()/3;
	const int		numTrianglesB = (int)coordsB.size()/3;
	TriangleSet		trianglesA;
	TriangleSet		trianglesB;

	for (int aOrB = 0; aOrB < 2; aOrB++)
	{
		const vector<Vec3>&		coords			= aOrB == 0 ? coordsA			: coordsB;
		const int				numTriangles	= aOrB == 0 ? numTrianglesA		: numTrianglesB;
		TriangleSet&			triangles		= aOrB == 0 ? trianglesA		: trianglesB;

		for (int triNdx = 0; triNdx < numTriangles; triNdx++)
		{
			Triangle triangle(coords[3*triNdx + 0],
							  coords[3*triNdx + 1],
							  coords[3*triNdx + 2]);

			if (isTriangleRelevant(triangle.getPtr()))
			{
				std::sort(triangle.getPtr(), triangle.getPtr()+3, VecLexLessThan<3>());
				triangles.insert(triangle);
			}
		}
	}

	{
		TriangleSet::const_iterator aIt = trianglesA.begin();
		TriangleSet::const_iterator bIt = trianglesB.begin();

		while (aIt != trianglesA.end() || bIt != trianglesB.end())
		{
			const bool aEnd = aIt == trianglesA.end();
			const bool bEnd = bIt == trianglesB.end();

			if (aEnd || bEnd || *aIt != *bIt)
			{
				log << TestLog::Message << "Failure: triangle sets in two cases are not equal (when ignoring triangle and vertex order"
					<< (ignoredTriangleDescription == DE_NULL ? "" : string() + ", and " + ignoredTriangleDescription) << ")" << TestLog::EndMessage;

				if (!aEnd && (bEnd || TriangleLexLessThan()(*aIt, *bIt)))
					log << TestLog::Message << "Note: e.g. triangle " << *aIt << " exists for first case but not for second" << TestLog::EndMessage;
				else
					log << TestLog::Message << "Note: e.g. triangle " << *bIt << " exists for second case but not for first" << TestLog::EndMessage;

				return false;
			}

			++aIt;
			++bIt;
		}

		return true;
	}
}

static bool compareTriangleSets (const vector<Vec3>& coordsA, const vector<Vec3>& coordsB, TestLog& log)
{
	return compareTriangleSets(coordsA, coordsB, log, ConstantUnaryPredicate<const Vec3*, true>());
}

static void checkGPUShader5Support (Context& context)
{
	const bool supportsES32 = glu::contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || context.getContextInfo().isExtensionSupported("GL_EXT_gpu_shader5"), "GL_EXT_gpu_shader5 is not supported");
}

static void checkTessellationSupport (Context& context)
{
	const bool supportsES32 = glu::contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"), "GL_EXT_tessellation_shader is not supported");
}

static std::string specializeShader(Context& context, const char* code)
{
	const glu::ContextType				contextType		= context.getRenderContext().getType();
	const glu::GLSLVersion				glslVersion		= glu::getContextTypeGLSLVersion(contextType);
	bool								supportsES32	= glu::contextSupports(contextType, glu::ApiType::es(3, 2));

	std::map<std::string, std::string>	specializationMap;

	specializationMap["GLSL_VERSION_DECL"]				= glu::getGLSLVersionDeclaration(glslVersion);
	specializationMap["GPU_SHADER5_REQUIRE"]			= supportsES32 ? "" : "#extension GL_EXT_gpu_shader5 : require";
	specializationMap["TESSELLATION_SHADER_REQUIRE"]	= supportsES32 ? "" : "#extension GL_EXT_tessellation_shader : require";

	return tcu::StringTemplate(code).specialize(specializationMap);
}

// Draw primitives with shared edges and check that no cracks are visible at the shared edges.
class CommonEdgeCase : public TestCase
{
public:
	enum CaseType
	{
		CASETYPE_BASIC = 0,		//!< Order patch vertices such that when two patches share a vertex, it's at the same index for both.
		CASETYPE_PRECISE,		//!< Vertex indices don't match like for CASETYPE_BASIC, but other measures are taken, using the 'precise' qualifier.

		CASETYPE_LAST
	};

	CommonEdgeCase (Context& context, const char* name, const char* description, TessPrimitiveType primitiveType, SpacingMode spacing, CaseType caseType)
		: TestCase			(context, name, description)
		, m_primitiveType	(primitiveType)
		, m_spacing			(spacing)
		, m_caseType		(caseType)
	{
		DE_ASSERT(m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES || m_primitiveType == TESSPRIMITIVETYPE_QUADS);
	}

	void							init		(void);
	void							deinit		(void);
	IterateResult					iterate		(void);

private:
	static const int				RENDER_SIZE = 256;

	const TessPrimitiveType			m_primitiveType;
	const SpacingMode				m_spacing;
	const CaseType					m_caseType;

	SharedPtr<const ShaderProgram>	m_program;
};

void CommonEdgeCase::init (void)
{
	checkTessellationSupport(m_context);

	if (m_caseType == CASETYPE_PRECISE)
		checkGPUShader5Support(m_context);

	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "in highp vec2 in_v_position;\n"
													 "in highp float in_v_tessParam;\n"
													 "\n"
													 "out highp vec2 in_tc_position;\n"
													 "out highp float in_tc_tessParam;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	in_tc_position = in_v_position;\n"
													 "	in_tc_tessParam = in_v_tessParam;\n"
												 "}\n");

	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 + string(m_caseType == CASETYPE_PRECISE ? "${GPU_SHADER5_REQUIRE}\n" : "") +
													 "\n"
													 "layout (vertices = " + string(m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? "3" : m_primitiveType == TESSPRIMITIVETYPE_QUADS ? "4" : DE_NULL) + ") out;\n"
													 "\n"
													 "in highp vec2 in_tc_position[];\n"
													 "in highp float in_tc_tessParam[];\n"
													 "\n"
													 "out highp vec2 in_te_position[];\n"
													 "\n"
													 + (m_caseType == CASETYPE_PRECISE ? "precise gl_TessLevelOuter;\n\n" : "") +
													 "void main (void)\n"
													 "{\n"
													 "	in_te_position[gl_InvocationID] = in_tc_position[gl_InvocationID];\n"
													 "\n"
													 "	gl_TessLevelInner[0] = 5.0;\n"
													 "	gl_TessLevelInner[1] = 5.0;\n"
													 "\n"
													 + (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
														"	gl_TessLevelOuter[0] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[1] + in_tc_tessParam[2]);\n"
														"	gl_TessLevelOuter[1] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[2] + in_tc_tessParam[0]);\n"
														"	gl_TessLevelOuter[2] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[0] + in_tc_tessParam[1]);\n"
													  : m_primitiveType == TESSPRIMITIVETYPE_QUADS ?
														"	gl_TessLevelOuter[0] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[0] + in_tc_tessParam[2]);\n"
														"	gl_TessLevelOuter[1] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[1] + in_tc_tessParam[0]);\n"
														"	gl_TessLevelOuter[2] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[3] + in_tc_tessParam[1]);\n"
														"	gl_TessLevelOuter[3] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[2] + in_tc_tessParam[3]);\n"
													  : DE_NULL) +
												 "}\n");

	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 + string(m_caseType == CASETYPE_PRECISE ? "${GPU_SHADER5_REQUIRE}\n" : "") +
													 "\n"
													 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing) +
													 "\n"
													 "in highp vec2 in_te_position[];\n"
													 "\n"
													 "out mediump vec4 in_f_color;\n"
													 "\n"
													 + (m_caseType == CASETYPE_PRECISE ? "precise gl_Position;\n\n" : "") +
													 "void main (void)\n"
													 "{\n"
													 + (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
														"	highp vec2 pos = gl_TessCoord.x*in_te_position[0] + gl_TessCoord.y*in_te_position[1] + gl_TessCoord.z*in_te_position[2];\n"
														"\n"
														"	highp float f = sqrt(3.0 * min(gl_TessCoord.x, min(gl_TessCoord.y, gl_TessCoord.z))) * 0.5 + 0.5;\n"
														"	in_f_color = vec4(gl_TessCoord*f, 1.0);\n"
													  : m_primitiveType == TESSPRIMITIVETYPE_QUADS ?
													    string()
														+ (m_caseType == CASETYPE_BASIC ?
															"	highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[0]\n"
															"	               + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[1]\n"
															"	               + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[2]\n"
															"	               + (    gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[3];\n"
														 : m_caseType == CASETYPE_PRECISE ?
															"	highp vec2 a = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[0];\n"
															"	highp vec2 b = (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[1];\n"
															"	highp vec2 c = (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[2];\n"
															"	highp vec2 d = (    gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[3];\n"
															"	highp vec2 pos = a+b+c+d;\n"
														 : DE_NULL) +
														"\n"
														"	highp float f = sqrt(1.0 - 2.0 * max(abs(gl_TessCoord.x - 0.5), abs(gl_TessCoord.y - 0.5)))*0.5 + 0.5;\n"
														"	in_f_color = vec4(0.1, gl_TessCoord.xy*f, 1.0);\n"
													  : DE_NULL) +
													 "\n"
													 "	// Offset the position slightly, based on the parity of the bits in the float representation.\n"
													 "	// This is done to detect possible small differences in edge vertex positions between patches.\n"
													 "	uvec2 bits = floatBitsToUint(pos);\n"
													 "	uint numBits = 0u;\n"
													 "	for (uint i = 0u; i < 32u; i++)\n"
													 "		numBits += ((bits[0] >> i) & 1u) + ((bits[1] >> i) & 1u);\n"
													 "	pos += float(numBits&1u)*0.04;\n"
													 "\n"
													 "	gl_Position = vec4(pos, 0.0, 1.0);\n"
												 "}\n");

	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "in mediump vec4 in_f_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void CommonEdgeCase::deinit (void)
{
	m_program.clear();
}

CommonEdgeCase::IterateResult CommonEdgeCase::iterate (void)
{
	TestLog&					log						= m_testCtx.getLog();
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	const RandomViewport		viewport				(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32				programGL				= m_program->getProgram();
	const glw::Functions&		gl						= renderCtx.getFunctions();

	const int					gridWidth				= 4;
	const int					gridHeight				= 4;
	const int					numVertices				= (gridWidth+1)*(gridHeight+1);
	const int					numIndices				= gridWidth*gridHeight * (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3*2 : m_primitiveType == TESSPRIMITIVETYPE_QUADS ? 4 : -1);
	const int					numPosCompsPerVertex	= 2;
	const int					totalNumPosComps		= numPosCompsPerVertex*numVertices;
	vector<float>				gridPosComps;
	vector<float>				gridTessParams;
	vector<deUint16>			gridIndices;

	gridPosComps.reserve(totalNumPosComps);
	gridTessParams.reserve(numVertices);
	gridIndices.reserve(numIndices);

	{
		for (int i = 0; i < gridHeight+1; i++)
		for (int j = 0; j < gridWidth+1; j++)
		{
			gridPosComps.push_back(-1.0f + 2.0f * ((float)j + 0.5f) / (float)(gridWidth+1));
			gridPosComps.push_back(-1.0f + 2.0f * ((float)i + 0.5f) / (float)(gridHeight+1));
			gridTessParams.push_back((float)(i*(gridWidth+1) + j) / (float)(numVertices-1));
		}
	}

	// Generate patch vertex indices.
	// \note If CASETYPE_BASIC, the vertices are ordered such that when multiple
	//		 triangles/quads share a vertex, it's at the same index for everyone.

	if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
	{
		for (int i = 0; i < gridHeight; i++)
		for (int j = 0; j < gridWidth; j++)
		{
			const deUint16 corners[4] =
			{
				(deUint16)((i+0)*(gridWidth+1) + j+0),
				(deUint16)((i+0)*(gridWidth+1) + j+1),
				(deUint16)((i+1)*(gridWidth+1) + j+0),
				(deUint16)((i+1)*(gridWidth+1) + j+1)
			};

			const int secondTriangleVertexIndexOffset = m_caseType == CASETYPE_BASIC	? 0
													  : m_caseType == CASETYPE_PRECISE	? 1
													  : -1;
			DE_ASSERT(secondTriangleVertexIndexOffset != -1);

			for (int k = 0; k < 3; k++)
				gridIndices.push_back(corners[(k+0 + i + (2-j%3)) % 3]);
			for (int k = 0; k < 3; k++)
				gridIndices.push_back(corners[(k+2 + i + (2-j%3) + secondTriangleVertexIndexOffset) % 3 + 1]);
		}
	}
	else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS)
	{
		for (int i = 0; i < gridHeight; i++)
		for (int j = 0; j < gridWidth; j++)
		{
			// \note The vertices are ordered such that when multiple quads
			//		 share a vertices, it's at the same index for everyone.
			for (int m = 0; m < 2; m++)
			for (int n = 0; n < 2; n++)
				gridIndices.push_back((deUint16)((i+(i+m)%2)*(gridWidth+1) + j+(j+n)%2));

			if(m_caseType == CASETYPE_PRECISE && (i+j) % 2 == 0)
				std::reverse(gridIndices.begin() + (gridIndices.size() - 4),
							 gridIndices.begin() + gridIndices.size());
		}
	}
	else
		DE_ASSERT(false);

	DE_ASSERT((int)gridPosComps.size() == totalNumPosComps);
	DE_ASSERT((int)gridTessParams.size() == numVertices);
	DE_ASSERT((int)gridIndices.size() == numIndices);

	setViewport(gl, viewport);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.useProgram(programGL);

	{
		gl.patchParameteri(GL_PATCH_VERTICES, m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : m_primitiveType == TESSPRIMITIVETYPE_QUADS ? 4 : -1);
		gl.clear(GL_COLOR_BUFFER_BIT);

		const glu::VertexArrayBinding attrBindings[] =
		{
			glu::va::Float("in_v_position", numPosCompsPerVertex, numVertices, 0, &gridPosComps[0]),
			glu::va::Float("in_v_tessParam", 1, numVertices, 0, &gridTessParams[0])
		};

		glu::draw(renderCtx, programGL, DE_LENGTH_OF_ARRAY(attrBindings), &attrBindings[0],
			glu::pr::Patches((int)gridIndices.size(), &gridIndices[0]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");
	}

	{
		const tcu::Surface rendered = getPixels(renderCtx, viewport);

		log << TestLog::Image("RenderedImage", "Rendered Image", rendered)
			<< TestLog::Message << "Note: coloring is done to clarify the positioning and orientation of the "
								<< (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? "triangles" : m_primitiveType == TESSPRIMITIVETYPE_QUADS ? "quads" : DE_NULL)
								<< "; the color of a vertex corresponds to the index of that vertex in the patch"
								<< TestLog::EndMessage;

		if (m_caseType == CASETYPE_BASIC)
			log << TestLog::Message << "Note: each shared vertex has the same index among the primitives it belongs to" << TestLog::EndMessage;
		else if (m_caseType == CASETYPE_PRECISE)
			log << TestLog::Message << "Note: the 'precise' qualifier is used to avoid cracks between primitives" << TestLog::EndMessage;
		else
			DE_ASSERT(false);

		// Ad-hoc result verification - check that a certain rectangle in the image contains no black pixels.

		const int startX	= (int)(0.15f * (float)rendered.getWidth());
		const int endX		= (int)(0.85f * (float)rendered.getWidth());
		const int startY	= (int)(0.15f * (float)rendered.getHeight());
		const int endY		= (int)(0.85f * (float)rendered.getHeight());

		for (int y = startY; y < endY; y++)
		for (int x = startX; x < endX; x++)
		{
			const tcu::RGBA pixel = rendered.getPixel(x, y);

			if (pixel.getRed() == 0 && pixel.getGreen() == 0 && pixel.getBlue() == 0)
			{
				log << TestLog::Message << "Failure: there seem to be cracks in the rendered result" << TestLog::EndMessage
					<< TestLog::Message << "Note: pixel with zero r, g and b channels found at " << tcu::IVec2(x, y) << TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
				return STOP;
			}
		}

		log << TestLog::Message << "Success: there seem to be no cracks in the rendered result" << TestLog::EndMessage;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

// Check tessellation coordinates (read with transform feedback).
class TessCoordCase : public TestCase
{
public:
					TessCoordCase (Context& context, const char* name, const char* description, TessPrimitiveType primitiveType, SpacingMode spacing)
						: TestCase			(context, name, description)
						, m_primitiveType	(primitiveType)
						, m_spacing			(spacing)
					{
					}

	void			init		(void);
	void			deinit		(void);
	IterateResult	iterate		(void);

private:
	struct TessLevels
	{
		float inner[2];
		float outer[4];
	};

	static const int				RENDER_SIZE = 16;

	vector<TessLevels>				genTessLevelCases (void) const;

	const TessPrimitiveType			m_primitiveType;
	const SpacingMode				m_spacing;

	SharedPtr<const ShaderProgram>	m_program;
};

void TessCoordCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
												 "}\n");

	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 "layout (vertices = 1) out;\n"
													 "\n"
													 "uniform mediump float u_tessLevelInner0;\n"
													 "uniform mediump float u_tessLevelInner1;\n"
													 "\n"
													 "uniform mediump float u_tessLevelOuter0;\n"
													 "uniform mediump float u_tessLevelOuter1;\n"
													 "uniform mediump float u_tessLevelOuter2;\n"
													 "uniform mediump float u_tessLevelOuter3;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	gl_TessLevelInner[0] = u_tessLevelInner0;\n"
													 "	gl_TessLevelInner[1] = u_tessLevelInner1;\n"
													 "\n"
													 "	gl_TessLevelOuter[0] = u_tessLevelOuter0;\n"
													 "	gl_TessLevelOuter[1] = u_tessLevelOuter1;\n"
													 "	gl_TessLevelOuter[2] = u_tessLevelOuter2;\n"
													 "	gl_TessLevelOuter[3] = u_tessLevelOuter3;\n"
												 "}\n");

	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing, true) +
													 "\n"
													 "out highp vec3 out_te_tessCoord;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	out_te_tessCoord = gl_TessCoord;\n"
													 "	gl_Position = vec4(gl_TessCoord.xy*1.6 - 0.8, 0.0, 1.0);\n"
												 "}\n");

	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = vec4(1.0);\n"
												 "}\n");

       m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
			<< glu::TransformFeedbackVarying		("out_te_tessCoord")
			<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void TessCoordCase::deinit (void)
{
	m_program.clear();
}

vector<TessCoordCase::TessLevels> TessCoordCase::genTessLevelCases (void) const
{
	static const TessLevels rawTessLevelCases[] =
	{
		{ { 1.0f,	1.0f	},	{ 1.0f,		1.0f,	1.0f,	1.0f	} },
		{ { 63.0f,	24.0f	},	{ 15.0f,	42.0f,	10.0f,	12.0f	} },
		{ { 3.0f,	2.0f	},	{ 6.0f,		8.0f,	7.0f,	9.0f	} },
		{ { 4.0f,	6.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 2.0f,	2.0f	},	{ 6.0f,		8.0f,	7.0f,	9.0f	} },
		{ { 5.0f,	6.0f	},	{ 1.0f,		1.0f,	1.0f,	1.0f	} },
		{ { 1.0f,	6.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 5.0f,	1.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 5.2f,	1.6f	},	{ 2.9f,		3.4f,	1.5f,	4.1f	} }
	};

	if (m_spacing == SPACINGMODE_EQUAL)
		return vector<TessLevels>(DE_ARRAY_BEGIN(rawTessLevelCases), DE_ARRAY_END(rawTessLevelCases));
	else
	{
		vector<TessLevels> result;
		result.reserve(DE_LENGTH_OF_ARRAY(rawTessLevelCases));

		for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < DE_LENGTH_OF_ARRAY(rawTessLevelCases); tessLevelCaseNdx++)
		{
			TessLevels curTessLevelCase = rawTessLevelCases[tessLevelCaseNdx];

			float* const inner = &curTessLevelCase.inner[0];
			float* const outer = &curTessLevelCase.outer[0];

			for (int j = 0; j < 2; j++) inner[j] = (float)getClampedRoundedTessLevel(m_spacing, inner[j]);
			for (int j = 0; j < 4; j++) outer[j] = (float)getClampedRoundedTessLevel(m_spacing, outer[j]);

			if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			{
				if (outer[0] > 1.0f || outer[1] > 1.0f || outer[2] > 1.0f)
				{
					if (inner[0] == 1.0f)
						inner[0] = (float)getClampedRoundedTessLevel(m_spacing, inner[0] + 0.1f);
				}
			}
			else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS)
			{
				if (outer[0] > 1.0f || outer[1] > 1.0f || outer[2] > 1.0f || outer[3] > 1.0f)
				{
					if (inner[0] == 1.0f) inner[0] = (float)getClampedRoundedTessLevel(m_spacing, inner[0] + 0.1f);
					if (inner[1] == 1.0f) inner[1] = (float)getClampedRoundedTessLevel(m_spacing, inner[1] + 0.1f);
				}
			}

			result.push_back(curTessLevelCase);
		}

		DE_ASSERT((int)result.size() == DE_LENGTH_OF_ARRAY(rawTessLevelCases));
		return result;
	}
}

TessCoordCase::IterateResult TessCoordCase::iterate (void)
{
	typedef TransformFeedbackHandler<Vec3> TFHandler;

	TestLog&						log							= m_testCtx.getLog();
	const RenderContext&			renderCtx					= m_context.getRenderContext();
	const RandomViewport			viewport					(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32					programGL					= m_program->getProgram();
	const glw::Functions&			gl							= renderCtx.getFunctions();

	const int						tessLevelInner0Loc			= gl.getUniformLocation(programGL, "u_tessLevelInner0");
	const int						tessLevelInner1Loc			= gl.getUniformLocation(programGL, "u_tessLevelInner1");
	const int						tessLevelOuter0Loc			= gl.getUniformLocation(programGL, "u_tessLevelOuter0");
	const int						tessLevelOuter1Loc			= gl.getUniformLocation(programGL, "u_tessLevelOuter1");
	const int						tessLevelOuter2Loc			= gl.getUniformLocation(programGL, "u_tessLevelOuter2");
	const int						tessLevelOuter3Loc			= gl.getUniformLocation(programGL, "u_tessLevelOuter3");

	const vector<TessLevels>		tessLevelCases				= genTessLevelCases();
	vector<vector<Vec3> >			caseReferences				(tessLevelCases.size());

	for (int i = 0; i < (int)tessLevelCases.size(); i++)
		caseReferences[i] = generateReferenceTessCoords(m_primitiveType, m_spacing, &tessLevelCases[i].inner[0], &tessLevelCases[i].outer[0]);

	const int						maxNumVertices				= (int)std::max_element(caseReferences.begin(), caseReferences.end(), SizeLessThan<vector<Vec3> >())->size();
	const TFHandler					tfHandler					(m_context.getRenderContext(), maxNumVertices);

	bool							success						= true;

	setViewport(gl, viewport);
	gl.useProgram(programGL);

	gl.patchParameteri(GL_PATCH_VERTICES, 1);

	for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < (int)tessLevelCases.size(); tessLevelCaseNdx++)
	{
		const float* const innerLevels = &tessLevelCases[tessLevelCaseNdx].inner[0];
		const float* const outerLevels = &tessLevelCases[tessLevelCaseNdx].outer[0];

		log << TestLog::Message << "Tessellation levels: " << tessellationLevelsString(innerLevels, outerLevels, m_primitiveType) << TestLog::EndMessage;

		gl.uniform1f(tessLevelInner0Loc, innerLevels[0]);
		gl.uniform1f(tessLevelInner1Loc, innerLevels[1]);
		gl.uniform1f(tessLevelOuter0Loc, outerLevels[0]);
		gl.uniform1f(tessLevelOuter1Loc, outerLevels[1]);
		gl.uniform1f(tessLevelOuter2Loc, outerLevels[2]);
		gl.uniform1f(tessLevelOuter3Loc, outerLevels[3]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Setup failed");

		{
			const vector<Vec3>&			tessCoordsRef	= caseReferences[tessLevelCaseNdx];
			const TFHandler::Result		tfResult		= tfHandler.renderAndGetPrimitives(programGL, GL_POINTS, 0, DE_NULL, 1);

			if (tfResult.numPrimitives != (int)tessCoordsRef.size())
			{
				log << TestLog::Message << "Failure: GL reported GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN to be "
										<< tfResult.numPrimitives << ", reference value is " << tessCoordsRef.size()
										<< " (logging further info anyway)" << TestLog::EndMessage;
				success = false;
			}
			else
				log << TestLog::Message << "Note: GL reported GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN to be " << tfResult.numPrimitives << TestLog::EndMessage;

			if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
				log << TestLog::Message << "Note: in the following visualization(s), the u=1, v=1, w=1 corners are at the right, top, and left corners, respectively" << TestLog::EndMessage;
			else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS || m_primitiveType == TESSPRIMITIVETYPE_ISOLINES)
				log << TestLog::Message << "Note: in the following visualization(s), u and v coordinate go left-to-right and bottom-to-top, respectively" << TestLog::EndMessage;
			else
				DE_ASSERT(false);

			success = compareTessCoords(log, m_primitiveType, tessCoordsRef, tfResult.varying) && success;
		}

		if (!success)
			break;
		else
			log << TestLog::Message << "All OK" << TestLog::EndMessage;
	}

	m_testCtx.setTestResult(success ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, success ? "Pass" : "Invalid tessellation coordinates");
	return STOP;
}

// Check validity of fractional spacing modes. Draws a single isoline, reads tesscoords with transform feedback.
class FractionalSpacingModeCase : public TestCase
{
public:
					FractionalSpacingModeCase (Context& context, const char* name, const char* description, SpacingMode spacing)
						: TestCase	(context, name, description)
						, m_spacing	(spacing)
					{
						DE_ASSERT(m_spacing == SPACINGMODE_FRACTIONAL_EVEN || m_spacing == SPACINGMODE_FRACTIONAL_ODD);
					}

	void			init		(void);
	void			deinit		(void);
	IterateResult	iterate		(void);

private:
	static const int				RENDER_SIZE = 16;

	static vector<float>			genTessLevelCases (void);

	const SpacingMode				m_spacing;

	SharedPtr<const ShaderProgram>	m_program;
};

void FractionalSpacingModeCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 "layout (vertices = 1) out;\n"
													 "\n"
													 "uniform mediump float u_tessLevelOuter1;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	gl_TessLevelOuter[0] = 1.0;\n"
													 "	gl_TessLevelOuter[1] = u_tessLevelOuter1;\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(TESSPRIMITIVETYPE_ISOLINES, m_spacing, true) +
													 "\n"
													 "out highp float out_te_tessCoord;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	out_te_tessCoord = gl_TessCoord.x;\n"
													 "	gl_Position = vec4(gl_TessCoord.xy*1.6 - 0.8, 0.0, 1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = vec4(1.0);\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
			<< glu::TransformFeedbackVarying		("out_te_tessCoord")
			<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void FractionalSpacingModeCase::deinit (void)
{
	m_program.clear();
}

vector<float> FractionalSpacingModeCase::genTessLevelCases (void)
{
	vector<float> result;

	// Ranges [7.0 .. 8.0), [8.0 .. 9.0) and [9.0 .. 10.0)
	{
		static const float	rangeStarts[]		= { 7.0f, 8.0f, 9.0f };
		const int			numSamplesPerRange	= 10;

		for (int rangeNdx = 0; rangeNdx < DE_LENGTH_OF_ARRAY(rangeStarts); rangeNdx++)
			for (int i = 0; i < numSamplesPerRange; i++)
				result.push_back(rangeStarts[rangeNdx] + (float)i/(float)numSamplesPerRange);
	}

	// 0.3, 1.3, 2.3,  ... , 62.3
	for (int i = 0; i <= 62; i++)
		result.push_back((float)i + 0.3f);

	return result;
}

FractionalSpacingModeCase::IterateResult FractionalSpacingModeCase::iterate (void)
{
	typedef TransformFeedbackHandler<float> TFHandler;

	TestLog&						log							= m_testCtx.getLog();
	const RenderContext&			renderCtx					= m_context.getRenderContext();
	const RandomViewport			viewport					(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32					programGL					= m_program->getProgram();
	const glw::Functions&			gl							= renderCtx.getFunctions();

	const int						tessLevelOuter1Loc			= gl.getUniformLocation(programGL, "u_tessLevelOuter1");

	// Second outer tessellation levels.
	const vector<float>				tessLevelCases				= genTessLevelCases();
	const int						maxNumVertices				= 1 + getClampedRoundedTessLevel(m_spacing, *std::max_element(tessLevelCases.begin(), tessLevelCases.end()));
	vector<float>					additionalSegmentLengths;
	vector<int>						additionalSegmentLocations;

	const TFHandler					tfHandler					(m_context.getRenderContext(), maxNumVertices);

	bool							success						= true;

	setViewport(gl, viewport);
	gl.useProgram(programGL);

	gl.patchParameteri(GL_PATCH_VERTICES, 1);

	for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < (int)tessLevelCases.size(); tessLevelCaseNdx++)
	{
		const float outerLevel1 = tessLevelCases[tessLevelCaseNdx];

		gl.uniform1f(tessLevelOuter1Loc, outerLevel1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Setup failed");

		{
			const TFHandler::Result		tfResult = tfHandler.renderAndGetPrimitives(programGL, GL_POINTS, 0, DE_NULL, 1);
			float						additionalSegmentLength;
			int							additionalSegmentLocation;

			success = verifyFractionalSpacingSingle(log, m_spacing, outerLevel1, tfResult.varying,
													additionalSegmentLength, additionalSegmentLocation);

			if (!success)
				break;

			additionalSegmentLengths.push_back(additionalSegmentLength);
			additionalSegmentLocations.push_back(additionalSegmentLocation);
		}
	}

	if (success)
		success = verifyFractionalSpacingMultiple(log, m_spacing, tessLevelCases, additionalSegmentLengths, additionalSegmentLocations);

	m_testCtx.setTestResult(success ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, success ? "Pass" : "Invalid tessellation coordinates");
	return STOP;
}

// Base class for a case with one input attribute (in_v_position) and optionally a TCS; tests with a couple of different sets of tessellation levels.
class BasicVariousTessLevelsPosAttrCase : public TestCase
{
public:
					BasicVariousTessLevelsPosAttrCase (Context&				context,
													   const char*			name,
													   const char*			description,
													   TessPrimitiveType	primitiveType,
													   SpacingMode			spacing,
													   const char*			referenceImagePathPrefix)
						: TestCase						(context, name, description)
						, m_primitiveType				(primitiveType)
						, m_spacing						(spacing)
						, m_referenceImagePathPrefix	(referenceImagePathPrefix)
					{
					}

	void			init			(void);
	void			deinit			(void);
	IterateResult	iterate			(void);

protected:
	virtual const glu::ProgramSources	makeSources (TessPrimitiveType, SpacingMode, const char* vtxOutPosAttrName) const = DE_NULL;

private:
	static const int					RENDER_SIZE = 256;

	const TessPrimitiveType				m_primitiveType;
	const SpacingMode					m_spacing;
	const string						m_referenceImagePathPrefix;

	SharedPtr<const ShaderProgram>		m_program;
};

void BasicVariousTessLevelsPosAttrCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	{
		glu::ProgramSources sources = makeSources(m_primitiveType, m_spacing, "in_tc_position");
		DE_ASSERT(sources.sources[glu::SHADERTYPE_TESSELLATION_CONTROL].empty());

		std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
													 "${TESSELLATION_SHADER_REQUIRE}\n"
													"\n"
													"layout (vertices = " + string(m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? "3" : "4") + ") out;\n"
													"\n"
													"in highp vec2 in_tc_position[];\n"
													"\n"
													"out highp vec2 in_te_position[];\n"
													"\n"
													"uniform mediump float u_tessLevelInner0;\n"
													"uniform mediump float u_tessLevelInner1;\n"
													"uniform mediump float u_tessLevelOuter0;\n"
													"uniform mediump float u_tessLevelOuter1;\n"
													"uniform mediump float u_tessLevelOuter2;\n"
													"uniform mediump float u_tessLevelOuter3;\n"
													"\n"
													"void main (void)\n"
													"{\n"
													"	in_te_position[gl_InvocationID] = in_tc_position[gl_InvocationID];\n"
													"\n"
													"	gl_TessLevelInner[0] = u_tessLevelInner0;\n"
													"	gl_TessLevelInner[1] = u_tessLevelInner1;\n"
													"\n"
													"	gl_TessLevelOuter[0] = u_tessLevelOuter0;\n"
													"	gl_TessLevelOuter[1] = u_tessLevelOuter1;\n"
													"	gl_TessLevelOuter[2] = u_tessLevelOuter2;\n"
													"	gl_TessLevelOuter[3] = u_tessLevelOuter3;\n"
													"}\n");

		sources << glu::TessellationControlSource(specializeShader(m_context, tessellationControlTemplate.c_str()));

		m_program = SharedPtr<const ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(), sources));
	}

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void BasicVariousTessLevelsPosAttrCase::deinit (void)
{
	m_program.clear();
}

BasicVariousTessLevelsPosAttrCase::IterateResult BasicVariousTessLevelsPosAttrCase::iterate (void)
{
	static const struct
	{
		float inner[2];
		float outer[4];
	} tessLevelCases[] =
	{
		{ { 9.0f,	9.0f	},	{ 9.0f,		9.0f,	9.0f,	9.0f	} },
		{ { 8.0f,	11.0f	},	{ 13.0f,	15.0f,	18.0f,	21.0f	} },
		{ { 17.0f,	14.0f	},	{ 3.0f,		6.0f,	9.0f,	12.0f	} }
	};

	TestLog&				log					= m_testCtx.getLog();
	const RenderContext&	renderCtx			= m_context.getRenderContext();
	const RandomViewport	viewport			(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32			programGL			= m_program->getProgram();
	const glw::Functions&	gl					= renderCtx.getFunctions();
	const int				patchSize			= m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES	? 3
												: m_primitiveType == TESSPRIMITIVETYPE_QUADS		? 4
												: m_primitiveType == TESSPRIMITIVETYPE_ISOLINES		? 4
												: -1;

	setViewport(gl, viewport);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.useProgram(programGL);

	gl.patchParameteri(GL_PATCH_VERTICES, patchSize);

	for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < DE_LENGTH_OF_ARRAY(tessLevelCases); tessLevelCaseNdx++)
	{
		float innerLevels[2];
		float outerLevels[4];

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(innerLevels); i++)
			innerLevels[i] = (float)getClampedRoundedTessLevel(m_spacing, tessLevelCases[tessLevelCaseNdx].inner[i]);

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(outerLevels); i++)
			outerLevels[i] = (float)getClampedRoundedTessLevel(m_spacing, tessLevelCases[tessLevelCaseNdx].outer[i]);

		log << TestLog::Message << "Tessellation levels: " << tessellationLevelsString(&innerLevels[0], &outerLevels[0], m_primitiveType) << TestLog::EndMessage;

		gl.uniform1f(gl.getUniformLocation(programGL, "u_tessLevelInner0"), innerLevels[0]);
		gl.uniform1f(gl.getUniformLocation(programGL, "u_tessLevelInner1"), innerLevels[1]);
		gl.uniform1f(gl.getUniformLocation(programGL, "u_tessLevelOuter0"), outerLevels[0]);
		gl.uniform1f(gl.getUniformLocation(programGL, "u_tessLevelOuter1"), outerLevels[1]);
		gl.uniform1f(gl.getUniformLocation(programGL, "u_tessLevelOuter2"), outerLevels[2]);
		gl.uniform1f(gl.getUniformLocation(programGL, "u_tessLevelOuter3"), outerLevels[3]);

		gl.clear(GL_COLOR_BUFFER_BIT);

		{
			vector<Vec2> positions;
			positions.reserve(4);

			if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			{
				positions.push_back(Vec2( 0.8f,    0.6f));
				positions.push_back(Vec2( 0.0f, -0.786f));
				positions.push_back(Vec2(-0.8f,    0.6f));
			}
			else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS || m_primitiveType == TESSPRIMITIVETYPE_ISOLINES)
			{
				positions.push_back(Vec2(-0.8f, -0.8f));
				positions.push_back(Vec2( 0.8f, -0.8f));
				positions.push_back(Vec2(-0.8f,  0.8f));
				positions.push_back(Vec2( 0.8f,  0.8f));
			}
			else
				DE_ASSERT(false);

			DE_ASSERT((int)positions.size() == patchSize);

			const glu::VertexArrayBinding attrBindings[] =
			{
				glu::va::Float("in_v_position", 2, (int)positions.size(), 0, &positions[0].x())
			};

			glu::draw(m_context.getRenderContext(), programGL, DE_LENGTH_OF_ARRAY(attrBindings), &attrBindings[0],
				glu::pr::Patches(patchSize));
			GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");
		}

		{
			const tcu::Surface			rendered	= getPixels(renderCtx, viewport);
			const tcu::TextureLevel		reference	= getPNG(m_testCtx.getArchive(), m_referenceImagePathPrefix + "_" + de::toString(tessLevelCaseNdx) + ".png");
			const bool					success		= tcu::fuzzyCompare(log, "ImageComparison", "Image Comparison", reference.getAccess(), rendered.getAccess(), 0.002f, tcu::COMPARE_LOG_RESULT);

			if (!success)
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

// Test that there are no obvious gaps in the triangulation of a tessellated triangle or quad.
class BasicTriangleFillCoverCase : public BasicVariousTessLevelsPosAttrCase
{
public:
	BasicTriangleFillCoverCase (Context& context, const char* name, const char* description, TessPrimitiveType primitiveType, SpacingMode spacing, const char* referenceImagePathPrefix)
		: BasicVariousTessLevelsPosAttrCase (context, name, description, primitiveType, spacing, referenceImagePathPrefix)
	{
		DE_ASSERT(primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS);
	}

protected:
	void init (void)
	{
		checkGPUShader5Support(m_context);
		BasicVariousTessLevelsPosAttrCase::init();
	}

	const glu::ProgramSources makeSources (TessPrimitiveType primitiveType, SpacingMode spacing, const char* vtxOutPosAttrName) const
	{
		std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "in highp vec2 in_v_position;\n"
													 "\n"
													 "out highp vec2 " + string(vtxOutPosAttrName) + ";\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	" + vtxOutPosAttrName + " = in_v_position;\n"
													 "}\n");
		std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
													 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "${GPU_SHADER5_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(primitiveType, spacing) +
													 "\n"
													 "in highp vec2 in_te_position[];\n"
													 "\n"
													 "precise gl_Position;\n"
													 "void main (void)\n"
													 "{\n"
													 + (primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
														"\n"
														"	highp float d = 3.0 * min(gl_TessCoord.x, min(gl_TessCoord.y, gl_TessCoord.z));\n"
														"	highp vec2 corner0 = in_te_position[0];\n"
														"	highp vec2 corner1 = in_te_position[1];\n"
														"	highp vec2 corner2 = in_te_position[2];\n"
														"	highp vec2 pos =  corner0*gl_TessCoord.x + corner1*gl_TessCoord.y + corner2*gl_TessCoord.z;\n"
														"	highp vec2 fromCenter = pos - (corner0 + corner1 + corner2) / 3.0;\n"
														"	highp float f = (1.0 - length(fromCenter)) * (1.5 - d);\n"
														"	pos += 0.75 * f * fromCenter / (length(fromCenter) + 0.3);\n"
														"	gl_Position = vec4(pos, 0.0, 1.0);\n"
													  : primitiveType == TESSPRIMITIVETYPE_QUADS ?
														"	highp vec2 corner0 = in_te_position[0];\n"
														"	highp vec2 corner1 = in_te_position[1];\n"
														"	highp vec2 corner2 = in_te_position[2];\n"
														"	highp vec2 corner3 = in_te_position[3];\n"
														"	highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner0\n"
														"	               + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner1\n"
														"	               + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*corner2\n"
														"	               + (    gl_TessCoord.x)*(    gl_TessCoord.y)*corner3;\n"
														"	highp float d = 2.0 * min(abs(gl_TessCoord.x-0.5), abs(gl_TessCoord.y-0.5));\n"
														"	highp vec2 fromCenter = pos - (corner0 + corner1 + corner2 + corner3) / 4.0;\n"
														"	highp float f = (1.0 - length(fromCenter)) * sqrt(1.7 - d);\n"
														"	pos += 0.75 * f * fromCenter / (length(fromCenter) + 0.3);\n"
														"	gl_Position = vec4(pos, 0.0, 1.0);\n"
													  : DE_NULL) +
													 "}\n");
		std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = vec4(1.0);\n"
													 "}\n");

		return glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()));
	}
};

// Check that there are no obvious overlaps in the triangulation of a tessellated triangle or quad.
class BasicTriangleFillNonOverlapCase : public BasicVariousTessLevelsPosAttrCase
{
public:
	BasicTriangleFillNonOverlapCase (Context& context, const char* name, const char* description, TessPrimitiveType primitiveType, SpacingMode spacing, const char* referenceImagePathPrefix)
		: BasicVariousTessLevelsPosAttrCase (context, name, description, primitiveType, spacing, referenceImagePathPrefix)
	{
		DE_ASSERT(primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS);
	}

protected:
	const glu::ProgramSources makeSources (TessPrimitiveType primitiveType, SpacingMode spacing, const char* vtxOutPosAttrName) const
	{
		std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "in highp vec2 in_v_position;\n"
													 "\n"
													 "out highp vec2 " + string(vtxOutPosAttrName) + ";\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	" + vtxOutPosAttrName + " = in_v_position;\n"
													 "}\n");
		std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
													 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(primitiveType, spacing) +
													 "\n"
													 "in highp vec2 in_te_position[];\n"
													 "\n"
													 "out mediump vec4 in_f_color;\n"
													 "\n"
													 "uniform mediump float u_tessLevelInner0;\n"
													 "uniform mediump float u_tessLevelInner1;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 + (primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
														"\n"
														"	highp vec2 corner0 = in_te_position[0];\n"
														"	highp vec2 corner1 = in_te_position[1];\n"
														"	highp vec2 corner2 = in_te_position[2];\n"
														"	highp vec2 pos =  corner0*gl_TessCoord.x + corner1*gl_TessCoord.y + corner2*gl_TessCoord.z;\n"
														"	gl_Position = vec4(pos, 0.0, 1.0);\n"
														"	highp int numConcentricTriangles = int(round(u_tessLevelInner0)) / 2 + 1;\n"
														"	highp float d = 3.0 * min(gl_TessCoord.x, min(gl_TessCoord.y, gl_TessCoord.z));\n"
														"	highp int phase = int(d*float(numConcentricTriangles)) % 3;\n"
														"	in_f_color = phase == 0 ? vec4(1.0, 0.0, 0.0, 1.0)\n"
														"	           : phase == 1 ? vec4(0.0, 1.0, 0.0, 1.0)\n"
														"	           :              vec4(0.0, 0.0, 1.0, 1.0);\n"
													  : primitiveType == TESSPRIMITIVETYPE_QUADS ?
														"	highp vec2 corner0 = in_te_position[0];\n"
														"	highp vec2 corner1 = in_te_position[1];\n"
														"	highp vec2 corner2 = in_te_position[2];\n"
														"	highp vec2 corner3 = in_te_position[3];\n"
														"	highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner0\n"
														"	               + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner1\n"
														"	               + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*corner2\n"
														"	               + (    gl_TessCoord.x)*(    gl_TessCoord.y)*corner3;\n"
														"	gl_Position = vec4(pos, 0.0, 1.0);\n"
														"	highp int phaseX = int(round((0.5 - abs(gl_TessCoord.x-0.5)) * u_tessLevelInner0));\n"
														"	highp int phaseY = int(round((0.5 - abs(gl_TessCoord.y-0.5)) * u_tessLevelInner1));\n"
														"	highp int phase = min(phaseX, phaseY) % 3;\n"
														"	in_f_color = phase == 0 ? vec4(1.0, 0.0, 0.0, 1.0)\n"
														"	           : phase == 1 ? vec4(0.0, 1.0, 0.0, 1.0)\n"
														"	           :              vec4(0.0, 0.0, 1.0, 1.0);\n"
													  : DE_NULL) +
													 "}\n");
		std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "in mediump vec4 in_f_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = in_f_color;\n"
													 "}\n");

		return glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()));
	}
};

// Basic isolines rendering case.
class IsolinesRenderCase : public BasicVariousTessLevelsPosAttrCase
{
public:
	IsolinesRenderCase (Context& context, const char* name, const char* description, SpacingMode spacing, const char* referenceImagePathPrefix)
		: BasicVariousTessLevelsPosAttrCase (context, name, description, TESSPRIMITIVETYPE_ISOLINES, spacing, referenceImagePathPrefix)
	{
	}

protected:
	const glu::ProgramSources makeSources (TessPrimitiveType primitiveType, SpacingMode spacing, const char* vtxOutPosAttrName) const
	{
		DE_ASSERT(primitiveType == TESSPRIMITIVETYPE_ISOLINES);
		DE_UNREF(primitiveType);

		std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "in highp vec2 in_v_position;\n"
													 "\n"
													 "out highp vec2 " + string(vtxOutPosAttrName) + ";\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	" + vtxOutPosAttrName + " = in_v_position;\n"
													 "}\n");
		std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
													 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(TESSPRIMITIVETYPE_ISOLINES, spacing) +
													 "\n"
													 "in highp vec2 in_te_position[];\n"
													 "\n"
													 "out mediump vec4 in_f_color;\n"
													 "\n"
													 "uniform mediump float u_tessLevelOuter0;\n"
													 "uniform mediump float u_tessLevelOuter1;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	highp vec2 corner0 = in_te_position[0];\n"
													 "	highp vec2 corner1 = in_te_position[1];\n"
													 "	highp vec2 corner2 = in_te_position[2];\n"
													 "	highp vec2 corner3 = in_te_position[3];\n"
													 "	highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner0\n"
													 "	               + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner1\n"
													 "	               + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*corner2\n"
													 "	               + (    gl_TessCoord.x)*(    gl_TessCoord.y)*corner3;\n"
													 "	pos.y += 0.15*sin(gl_TessCoord.x*10.0);\n"
													 "	gl_Position = vec4(pos, 0.0, 1.0);\n"
													 "	highp int phaseX = int(round(gl_TessCoord.x*u_tessLevelOuter1));\n"
													 "	highp int phaseY = int(round(gl_TessCoord.y*u_tessLevelOuter0));\n"
													 "	highp int phase = (phaseX + phaseY) % 3;\n"
													 "	in_f_color = phase == 0 ? vec4(1.0, 0.0, 0.0, 1.0)\n"
													 "	           : phase == 1 ? vec4(0.0, 1.0, 0.0, 1.0)\n"
													 "	           :              vec4(0.0, 0.0, 1.0, 1.0);\n"
													 "}\n");
		std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "in mediump vec4 in_f_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = in_f_color;\n"
													 "}\n");

		return glu::ProgramSources()
				<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
				<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
				<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()));
	}
};

// Test the "cw" and "ccw" TES input layout qualifiers.
class WindingCase : public TestCase
{
public:
					WindingCase (Context& context, const char* name, const char* description, TessPrimitiveType primitiveType, Winding winding)
						: TestCase			(context, name, description)
						, m_primitiveType	(primitiveType)
						, m_winding			(winding)
					{
						DE_ASSERT(primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS);
					}

	void			init		(void);
	void			deinit		(void);
	IterateResult	iterate		(void);

private:
	static const int				RENDER_SIZE = 64;

	const TessPrimitiveType			m_primitiveType;
	const Winding					m_winding;

	SharedPtr<const ShaderProgram>	m_program;
};

void WindingCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 "layout (vertices = 1) out;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	gl_TessLevelInner[0] = 5.0;\n"
													 "	gl_TessLevelInner[1] = 5.0;\n"
													 "\n"
													 "	gl_TessLevelOuter[0] = 5.0;\n"
													 "	gl_TessLevelOuter[1] = 5.0;\n"
													 "	gl_TessLevelOuter[2] = 5.0;\n"
													 "	gl_TessLevelOuter[3] = 5.0;\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(m_primitiveType, m_winding) +
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	gl_Position = vec4(gl_TessCoord.xy*2.0 - 1.0, 0.0, 1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = vec4(1.0);\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void WindingCase::deinit (void)
{
	m_program.clear();
}

WindingCase::IterateResult WindingCase::iterate (void)
{
	TestLog&						log							= m_testCtx.getLog();
	const RenderContext&			renderCtx					= m_context.getRenderContext();
	const RandomViewport			viewport					(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32					programGL					= m_program->getProgram();
	const glw::Functions&			gl							= renderCtx.getFunctions();
	const glu::VertexArray			vao							(renderCtx);

	bool							success						= true;

	setViewport(gl, viewport);
	gl.clearColor(1.0f, 0.0f, 0.0f, 1.0f);
	gl.useProgram(programGL);

	gl.patchParameteri(GL_PATCH_VERTICES, 1);

	gl.enable(GL_CULL_FACE);

	gl.bindVertexArray(*vao);

	log << TestLog::Message << "Face culling enabled" << TestLog::EndMessage;

	for (int frontFaceWinding = 0; frontFaceWinding < WINDING_LAST; frontFaceWinding++)
	{
		log << TestLog::Message << "Setting glFrontFace(" << (frontFaceWinding == WINDING_CW ? "GL_CW" : "GL_CCW") << ")" << TestLog::EndMessage;

		gl.frontFace(frontFaceWinding == WINDING_CW ? GL_CW : GL_CCW);

		gl.clear(GL_COLOR_BUFFER_BIT);
		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");

		{
			const tcu::Surface rendered = getPixels(renderCtx, viewport);
			log << TestLog::Image("RenderedImage", "Rendered Image", rendered);

			{
				const int totalNumPixels		= rendered.getWidth()*rendered.getHeight();
				const int badPixelTolerance		= m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 5*de::max(rendered.getWidth(), rendered.getHeight()) : 0;

				int numWhitePixels	= 0;
				int numRedPixels	= 0;
				for (int y = 0; y < rendered.getHeight();	y++)
				for (int x = 0; x < rendered.getWidth();	x++)
				{
					numWhitePixels	+= rendered.getPixel(x, y) == tcu::RGBA::white()	? 1 : 0;
					numRedPixels	+= rendered.getPixel(x, y) == tcu::RGBA::red()	? 1 : 0;
				}

				DE_ASSERT(numWhitePixels + numRedPixels <= totalNumPixels);

				log << TestLog::Message << "Note: got " << numWhitePixels << " white and " << numRedPixels << " red pixels" << TestLog::EndMessage;

				if (totalNumPixels - numWhitePixels - numRedPixels > badPixelTolerance)
				{
					log << TestLog::Message << "Failure: Got " << totalNumPixels - numWhitePixels - numRedPixels << " other than white or red pixels (maximum tolerance " << badPixelTolerance << ")" << TestLog::EndMessage;
					success = false;
					break;
				}

				if ((Winding)frontFaceWinding == m_winding)
				{
					if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
					{
						if (de::abs(numWhitePixels - totalNumPixels/2) > badPixelTolerance)
						{
							log << TestLog::Message << "Failure: wrong number of white pixels; expected approximately " << totalNumPixels/2 << TestLog::EndMessage;
							success = false;
							break;
						}
					}
					else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS)
					{
						if (numWhitePixels != totalNumPixels)
						{
							log << TestLog::Message << "Failure: expected only white pixels (full-viewport quad)" << TestLog::EndMessage;
							success = false;
							break;
						}
					}
					else
						DE_ASSERT(false);
				}
				else
				{
					if (numWhitePixels != 0)
					{
						log << TestLog::Message << "Failure: expected only red pixels (everything culled)" << TestLog::EndMessage;
						success = false;
						break;
					}
				}
			}
		}
	}

	m_testCtx.setTestResult(success ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, success ? "Pass" : "Image verification failed");
	return STOP;
}

// Test potentially differing input and output patch sizes.
class PatchVertexCountCase : public TestCase
{
public:
	PatchVertexCountCase (Context& context, const char* name, const char* description, int inputPatchSize, int outputPatchSize, const char* referenceImagePath)
		: TestCase				(context, name, description)
		, m_inputPatchSize		(inputPatchSize)
		, m_outputPatchSize		(outputPatchSize)
		, m_referenceImagePath	(referenceImagePath)
	{
	}

	void							init				(void);
	void							deinit				(void);
	IterateResult					iterate				(void);

private:
	static const int				RENDER_SIZE = 256;

	const int						m_inputPatchSize;
	const int						m_outputPatchSize;

	const string					m_referenceImagePath;

	SharedPtr<const ShaderProgram>	m_program;
};

void PatchVertexCountCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	const string inSizeStr		= de::toString(m_inputPatchSize);
	const string outSizeStr		= de::toString(m_outputPatchSize);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "in highp float in_v_attr;\n"
													 "\n"
													 "out highp float in_tc_attr;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 "layout (vertices = " + outSizeStr + ") out;\n"
													 "\n"
													 "in highp float in_tc_attr[];\n"
													 "\n"
													 "out highp float in_te_attr[];\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	in_te_attr[gl_InvocationID] = in_tc_attr[gl_InvocationID*" + inSizeStr + "/" + outSizeStr + "];\n"
													 "\n"
													 "	gl_TessLevelInner[0] = 5.0;\n"
													 "	gl_TessLevelInner[1] = 5.0;\n"
													 "\n"
													"	gl_TessLevelOuter[0] = 5.0;\n"
													"	gl_TessLevelOuter[1] = 5.0;\n"
													"	gl_TessLevelOuter[2] = 5.0;\n"
													"	gl_TessLevelOuter[3] = 5.0;\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(TESSPRIMITIVETYPE_QUADS) +
													 "\n"
													 "in highp float in_te_attr[];\n"
													 "\n"
													 "out mediump vec4 in_f_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	highp float x = gl_TessCoord.x*2.0 - 1.0;\n"
													 "	highp float y = gl_TessCoord.y - in_te_attr[int(round(gl_TessCoord.x*float(" + outSizeStr + "-1)))];\n"
													 "	gl_Position = vec4(x, y, 0.0, 1.0);\n"
													 "	in_f_color = vec4(1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "in mediump vec4 in_f_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void PatchVertexCountCase::deinit (void)
{
	m_program.clear();
}

PatchVertexCountCase::IterateResult PatchVertexCountCase::iterate (void)
{
	TestLog&					log						= m_testCtx.getLog();
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	const RandomViewport		viewport				(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32				programGL				= m_program->getProgram();
	const glw::Functions&		gl						= renderCtx.getFunctions();

	setViewport(gl, viewport);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.useProgram(programGL);

	log << TestLog::Message << "Note: input patch size is " << m_inputPatchSize << ", output patch size is " << m_outputPatchSize << TestLog::EndMessage;

	{
		vector<float> attributeData;
		attributeData.reserve(m_inputPatchSize);

		for (int i = 0; i < m_inputPatchSize; i++)
		{
			const float f = (float)i / (float)(m_inputPatchSize-1);
			attributeData.push_back(f*f);
		}

		gl.patchParameteri(GL_PATCH_VERTICES, m_inputPatchSize);
		gl.clear(GL_COLOR_BUFFER_BIT);

		const glu::VertexArrayBinding attrBindings[] =
		{
			glu::va::Float("in_v_attr", 1, (int)attributeData.size(), 0, &attributeData[0])
		};

		glu::draw(m_context.getRenderContext(), programGL, DE_LENGTH_OF_ARRAY(attrBindings), &attrBindings[0],
			glu::pr::Patches(m_inputPatchSize));
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");
	}

	{
		const tcu::Surface			rendered	= getPixels(renderCtx, viewport);
		const tcu::TextureLevel		reference	= getPNG(m_testCtx.getArchive(), m_referenceImagePath);
		const bool					success		= tcu::fuzzyCompare(log, "ImageComparison", "Image Comparison", reference.getAccess(), rendered.getAccess(), 0.02f, tcu::COMPARE_LOG_RESULT);

		m_testCtx.setTestResult(success ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, success ? "Pass" : "Image comparison failed");
		return STOP;
	}
}

// Test per-patch inputs/outputs.
class PerPatchDataCase : public TestCase
{
public:
	enum CaseType
	{
		CASETYPE_PRIMITIVE_ID_TCS = 0,
		CASETYPE_PRIMITIVE_ID_TES,
		CASETYPE_PATCH_VERTICES_IN_TCS,
		CASETYPE_PATCH_VERTICES_IN_TES,
		CASETYPE_TESS_LEVEL_INNER0_TES,
		CASETYPE_TESS_LEVEL_INNER1_TES,
		CASETYPE_TESS_LEVEL_OUTER0_TES,
		CASETYPE_TESS_LEVEL_OUTER1_TES,
		CASETYPE_TESS_LEVEL_OUTER2_TES,
		CASETYPE_TESS_LEVEL_OUTER3_TES,

		CASETYPE_LAST
	};

	PerPatchDataCase (Context& context, const char* name, const char* description, CaseType caseType, const char* referenceImagePath)
		: TestCase				(context, name, description)
		, m_caseType			(caseType)
		, m_referenceImagePath	(caseTypeUsesRefImageFromFile(caseType) ? referenceImagePath : "")
	{
		DE_ASSERT(caseTypeUsesRefImageFromFile(caseType) == (referenceImagePath != DE_NULL));
	}

	void							init							(void);
	void							deinit							(void);
	IterateResult					iterate							(void);

	static const char*				getCaseTypeName					(CaseType);
	static const char*				getCaseTypeDescription			(CaseType);
	static bool						caseTypeUsesRefImageFromFile	(CaseType);

private:
	static const int				RENDER_SIZE = 256;
	static const int				INPUT_PATCH_SIZE;
	static const int				OUTPUT_PATCH_SIZE;

	const CaseType					m_caseType;
	const string					m_referenceImagePath;

	SharedPtr<const ShaderProgram>	m_program;
};

const int PerPatchDataCase::INPUT_PATCH_SIZE	= 10;
const int PerPatchDataCase::OUTPUT_PATCH_SIZE	= 5;

const char* PerPatchDataCase::getCaseTypeName (CaseType type)
{
	switch (type)
	{
		case CASETYPE_PRIMITIVE_ID_TCS:			return "primitive_id_tcs";
		case CASETYPE_PRIMITIVE_ID_TES:			return "primitive_id_tes";
		case CASETYPE_PATCH_VERTICES_IN_TCS:	return "patch_vertices_in_tcs";
		case CASETYPE_PATCH_VERTICES_IN_TES:	return "patch_vertices_in_tes";
		case CASETYPE_TESS_LEVEL_INNER0_TES:	return "tess_level_inner_0_tes";
		case CASETYPE_TESS_LEVEL_INNER1_TES:	return "tess_level_inner_1_tes";
		case CASETYPE_TESS_LEVEL_OUTER0_TES:	return "tess_level_outer_0_tes";
		case CASETYPE_TESS_LEVEL_OUTER1_TES:	return "tess_level_outer_1_tes";
		case CASETYPE_TESS_LEVEL_OUTER2_TES:	return "tess_level_outer_2_tes";
		case CASETYPE_TESS_LEVEL_OUTER3_TES:	return "tess_level_outer_3_tes";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

const char* PerPatchDataCase::getCaseTypeDescription (CaseType type)
{
	switch (type)
	{
		case CASETYPE_PRIMITIVE_ID_TCS:			return "Read gl_PrimitiveID in TCS and pass it as patch output to TES";
		case CASETYPE_PRIMITIVE_ID_TES:			return "Read gl_PrimitiveID in TES";
		case CASETYPE_PATCH_VERTICES_IN_TCS:	return "Read gl_PatchVerticesIn in TCS and pass it as patch output to TES";
		case CASETYPE_PATCH_VERTICES_IN_TES:	return "Read gl_PatchVerticesIn in TES";
		case CASETYPE_TESS_LEVEL_INNER0_TES:	return "Read gl_TessLevelInner[0] in TES";
		case CASETYPE_TESS_LEVEL_INNER1_TES:	return "Read gl_TessLevelInner[1] in TES";
		case CASETYPE_TESS_LEVEL_OUTER0_TES:	return "Read gl_TessLevelOuter[0] in TES";
		case CASETYPE_TESS_LEVEL_OUTER1_TES:	return "Read gl_TessLevelOuter[1] in TES";
		case CASETYPE_TESS_LEVEL_OUTER2_TES:	return "Read gl_TessLevelOuter[2] in TES";
		case CASETYPE_TESS_LEVEL_OUTER3_TES:	return "Read gl_TessLevelOuter[3] in TES";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

bool PerPatchDataCase::caseTypeUsesRefImageFromFile (CaseType type)
{
	switch (type)
	{
		case CASETYPE_PRIMITIVE_ID_TCS:
		case CASETYPE_PRIMITIVE_ID_TES:
			return true;

		default:
			return false;
	}
}

void PerPatchDataCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	DE_ASSERT(OUTPUT_PATCH_SIZE < INPUT_PATCH_SIZE);

	const string inSizeStr		= de::toString(INPUT_PATCH_SIZE);
	const string outSizeStr		= de::toString(OUTPUT_PATCH_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "in highp float in_v_attr;\n"
													 "\n"
													 "out highp float in_tc_attr;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 "layout (vertices = " + outSizeStr + ") out;\n"
													 "\n"
													 "in highp float in_tc_attr[];\n"
													 "\n"
													 "out highp float in_te_attr[];\n"
													 + (m_caseType == CASETYPE_PRIMITIVE_ID_TCS			? "patch out mediump int in_te_primitiveIDFromTCS;\n"
													  : m_caseType == CASETYPE_PATCH_VERTICES_IN_TCS	? "patch out mediump int in_te_patchVerticesInFromTCS;\n"
													  : "") +
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	in_te_attr[gl_InvocationID] = in_tc_attr[gl_InvocationID];\n"
													 + (m_caseType == CASETYPE_PRIMITIVE_ID_TCS			? "\tin_te_primitiveIDFromTCS = gl_PrimitiveID;\n"
													  : m_caseType == CASETYPE_PATCH_VERTICES_IN_TCS	? "\tin_te_patchVerticesInFromTCS = gl_PatchVerticesIn;\n"
													  : "") +
													 "\n"
													 "	gl_TessLevelInner[0] = 9.0;\n"
													 "	gl_TessLevelInner[1] = 8.0;\n"
													 "\n"
													"	gl_TessLevelOuter[0] = 7.0;\n"
													"	gl_TessLevelOuter[1] = 6.0;\n"
													"	gl_TessLevelOuter[2] = 5.0;\n"
													"	gl_TessLevelOuter[3] = 4.0;\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(TESSPRIMITIVETYPE_QUADS) +
													 "\n"
													 "in highp float in_te_attr[];\n"
													 + (m_caseType == CASETYPE_PRIMITIVE_ID_TCS			? "patch in mediump int in_te_primitiveIDFromTCS;\n"
													  : m_caseType == CASETYPE_PATCH_VERTICES_IN_TCS	? "patch in mediump int in_te_patchVerticesInFromTCS;\n"
													  : string()) +
													 "\n"
													 "out mediump vec4 in_f_color;\n"
													 "\n"
													 "uniform highp float u_xScale;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	highp float x = (gl_TessCoord.x*u_xScale + in_te_attr[0]) * 2.0 - 1.0;\n"
													 "	highp float y = gl_TessCoord.y*2.0 - 1.0;\n"
													 "	gl_Position = vec4(x, y, 0.0, 1.0);\n"
													 + (m_caseType == CASETYPE_PRIMITIVE_ID_TCS			? "\tbool ok = in_te_primitiveIDFromTCS == 3;\n"
													  : m_caseType == CASETYPE_PRIMITIVE_ID_TES			? "\tbool ok = gl_PrimitiveID == 3;\n"
													  : m_caseType == CASETYPE_PATCH_VERTICES_IN_TCS	? "\tbool ok = in_te_patchVerticesInFromTCS == " + inSizeStr + ";\n"
													  : m_caseType == CASETYPE_PATCH_VERTICES_IN_TES	? "\tbool ok = gl_PatchVerticesIn == " + outSizeStr + ";\n"
													  : m_caseType == CASETYPE_TESS_LEVEL_INNER0_TES	? "\tbool ok = abs(gl_TessLevelInner[0] - 9.0) < 0.1f;\n"
													  : m_caseType == CASETYPE_TESS_LEVEL_INNER1_TES	? "\tbool ok = abs(gl_TessLevelInner[1] - 8.0) < 0.1f;\n"
													  : m_caseType == CASETYPE_TESS_LEVEL_OUTER0_TES	? "\tbool ok = abs(gl_TessLevelOuter[0] - 7.0) < 0.1f;\n"
													  : m_caseType == CASETYPE_TESS_LEVEL_OUTER1_TES	? "\tbool ok = abs(gl_TessLevelOuter[1] - 6.0) < 0.1f;\n"
													  : m_caseType == CASETYPE_TESS_LEVEL_OUTER2_TES	? "\tbool ok = abs(gl_TessLevelOuter[2] - 5.0) < 0.1f;\n"
													  : m_caseType == CASETYPE_TESS_LEVEL_OUTER3_TES	? "\tbool ok = abs(gl_TessLevelOuter[3] - 4.0) < 0.1f;\n"
													  : DE_NULL) +
													  "	in_f_color = ok ? vec4(1.0) : vec4(vec3(0.0), 1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "in mediump vec4 in_f_color;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void PerPatchDataCase::deinit (void)
{
	m_program.clear();
}

PerPatchDataCase::IterateResult PerPatchDataCase::iterate (void)
{
	TestLog&					log						= m_testCtx.getLog();
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	const RandomViewport		viewport				(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32				programGL				= m_program->getProgram();
	const glw::Functions&		gl						= renderCtx.getFunctions();

	setViewport(gl, viewport);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.useProgram(programGL);

	log << TestLog::Message << "Note: input patch size is " << INPUT_PATCH_SIZE << ", output patch size is " << OUTPUT_PATCH_SIZE << TestLog::EndMessage;

	{
		const int numPrimitives = m_caseType == CASETYPE_PRIMITIVE_ID_TCS || m_caseType == CASETYPE_PRIMITIVE_ID_TES ? 8 : 1;

		vector<float> attributeData;
		attributeData.reserve(numPrimitives*INPUT_PATCH_SIZE);

		for (int i = 0; i < numPrimitives; i++)
		{
			attributeData.push_back((float)i / (float)numPrimitives);
			for (int j = 0; j < INPUT_PATCH_SIZE-1; j++)
				attributeData.push_back(0.0f);
		}

		gl.patchParameteri(GL_PATCH_VERTICES, INPUT_PATCH_SIZE);
		gl.clear(GL_COLOR_BUFFER_BIT);

		gl.uniform1f(gl.getUniformLocation(programGL, "u_xScale"), 1.0f / (float)numPrimitives);

		const glu::VertexArrayBinding attrBindings[] =
		{
			glu::va::Float("in_v_attr", 1, (int)attributeData.size(), 0, &attributeData[0])
		};

		glu::draw(m_context.getRenderContext(), programGL, DE_LENGTH_OF_ARRAY(attrBindings), &attrBindings[0],
			glu::pr::Patches(numPrimitives*INPUT_PATCH_SIZE));
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");
	}

	{
		const tcu::Surface rendered = getPixels(renderCtx, viewport);

		if (m_caseType == CASETYPE_PRIMITIVE_ID_TCS || m_caseType == CASETYPE_PRIMITIVE_ID_TES)
		{
			DE_ASSERT(caseTypeUsesRefImageFromFile(m_caseType));

			const tcu::TextureLevel		reference	= getPNG(m_testCtx.getArchive(), m_referenceImagePath);
			const bool					success		= tcu::fuzzyCompare(log, "ImageComparison", "Image Comparison", reference.getAccess(), rendered.getAccess(), 0.02f, tcu::COMPARE_LOG_RESULT);

			m_testCtx.setTestResult(success ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, success ? "Pass" : "Image comparison failed");
			return STOP;
		}
		else
		{
			DE_ASSERT(!caseTypeUsesRefImageFromFile(m_caseType));

			log << TestLog::Image("RenderedImage", "Rendered Image", rendered);

			for (int y = 0; y < rendered.getHeight();	y++)
			for (int x = 0; x < rendered.getWidth();	x++)
			{
				if (rendered.getPixel(x, y) != tcu::RGBA::white())
				{
					log << TestLog::Message << "Failure: expected all white pixels" << TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
					return STOP;
				}
			}

			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
			return STOP;
		}
	}
}

// Basic barrier() usage in TCS.
class BarrierCase : public TestCase
{
public:
	BarrierCase (Context& context, const char* name, const char* description, const char* referenceImagePath)
		: TestCase				(context, name, description)
		, m_referenceImagePath	(referenceImagePath)
	{
	}

	void							init		(void);
	void							deinit		(void);
	IterateResult					iterate		(void);

private:
	static const int				RENDER_SIZE = 256;
	static const int				NUM_VERTICES;

	const string					m_referenceImagePath;

	SharedPtr<const ShaderProgram>	m_program;
};

const int BarrierCase::NUM_VERTICES = 32;

void BarrierCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	const string numVertsStr = de::toString(NUM_VERTICES);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "in highp float in_v_attr;\n"
													 "\n"
													 "out highp float in_tc_attr;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 "layout (vertices = " + numVertsStr + ") out;\n"
													 "\n"
													 "in highp float in_tc_attr[];\n"
													 "\n"
													 "out highp float in_te_attr[];\n"
													 "patch out highp float in_te_patchAttr;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	in_te_attr[gl_InvocationID] = in_tc_attr[gl_InvocationID];\n"
													 "	in_te_patchAttr = 0.0f;\n"
													 "	barrier();\n"
													 "	if (gl_InvocationID == 5)\n"
													 "		in_te_patchAttr = float(gl_InvocationID)*0.1;\n"
													 "	barrier();\n"
													 "	highp float temp = in_te_patchAttr + in_te_attr[gl_InvocationID];\n"
													 "	barrier();\n"
													 "	if (gl_InvocationID == " + numVertsStr + "-1)\n"
													 "		in_te_patchAttr = float(gl_InvocationID);\n"
													 "	barrier();\n"
													 "	in_te_attr[gl_InvocationID] = temp;\n"
													 "	barrier();\n"
													 "	temp = temp + in_te_attr[(gl_InvocationID+1) % " + numVertsStr + "];\n"
													 "	barrier();\n"
													 "	in_te_attr[gl_InvocationID] = 0.25*temp;\n"
													 "\n"
													 "	gl_TessLevelInner[0] = 32.0;\n"
													 "	gl_TessLevelInner[1] = 32.0;\n"
													 "\n"
													 "	gl_TessLevelOuter[0] = 32.0;\n"
													 "	gl_TessLevelOuter[1] = 32.0;\n"
													 "	gl_TessLevelOuter[2] = 32.0;\n"
													 "	gl_TessLevelOuter[3] = 32.0;\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
													 "\n"
													 + getTessellationEvaluationInLayoutString(TESSPRIMITIVETYPE_QUADS) +
													 "\n"
													 "in highp float in_te_attr[];\n"
													 "patch in highp float in_te_patchAttr;\n"
													 "\n"
													 "out highp float in_f_blue;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	highp float x = gl_TessCoord.x*2.0 - 1.0;\n"
													 "	highp float y = gl_TessCoord.y - in_te_attr[int(round(gl_TessCoord.x*float(" + numVertsStr + "-1)))];\n"
													 "	gl_Position = vec4(x, y, 0.0, 1.0);\n"
													 "	in_f_blue = abs(in_te_patchAttr - float(" + numVertsStr + "-1));\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
													 "\n"
													 "layout (location = 0) out mediump vec4 o_color;\n"
													 "\n"
													 "in highp float in_f_blue;\n"
													 "\n"
													 "void main (void)\n"
													 "{\n"
													 "	o_color = vec4(1.0, 0.0, in_f_blue, 1.0);\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
			<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
			<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
			<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void BarrierCase::deinit (void)
{
	m_program.clear();
}

BarrierCase::IterateResult BarrierCase::iterate (void)
{
	TestLog&					log						= m_testCtx.getLog();
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	const RandomViewport		viewport				(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const deUint32				programGL				= m_program->getProgram();
	const glw::Functions&		gl						= renderCtx.getFunctions();

	setViewport(gl, viewport);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.useProgram(programGL);

	{
		vector<float> attributeData(NUM_VERTICES);

		for (int i = 0; i < NUM_VERTICES; i++)
			attributeData[i] = (float)i / (float)(NUM_VERTICES-1);

		gl.patchParameteri(GL_PATCH_VERTICES, NUM_VERTICES);
		gl.clear(GL_COLOR_BUFFER_BIT);

		const glu::VertexArrayBinding attrBindings[] =
		{
			glu::va::Float("in_v_attr", 1, (int)attributeData.size(), 0, &attributeData[0])
		};

		glu::draw(m_context.getRenderContext(), programGL, DE_LENGTH_OF_ARRAY(attrBindings), &attrBindings[0],
			glu::pr::Patches(NUM_VERTICES));
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");
	}

	{
		const tcu::Surface			rendered	= getPixels(renderCtx, viewport);
		const tcu::TextureLevel		reference	= getPNG(m_testCtx.getArchive(), m_referenceImagePath);
		const bool					success		= tcu::fuzzyCompare(log, "ImageComparison", "Image Comparison", reference.getAccess(), rendered.getAccess(), 0.02f, tcu::COMPARE_LOG_RESULT);

		m_testCtx.setTestResult(success ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, success ? "Pass" : "Image comparison failed");
		return STOP;
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Base class for testing invariance of entire primitive set
 *
 * Draws two patches with identical tessellation levels and compares the
 * results. Repeats the same with other programs that are only different
 * in irrelevant ways; compares the results between these two programs.
 * Also potentially compares to results produced by different tessellation
 * levels (see e.g. invariance rule #6).
 * Furthermore, repeats the above with multiple different tessellation
 * value sets.
 *
 * The manner of primitive set comparison is defined by subclass. E.g.
 * case for invariance rule #1 tests that same vertices come out, in same
 * order; rule #5 only requires that the same triangles are output, but
 * not necessarily in the same order.
 *//*--------------------------------------------------------------------*/
class PrimitiveSetInvarianceCase : public TestCase
{
public:
	enum WindingUsage
	{
		WINDINGUSAGE_CCW = 0,
		WINDINGUSAGE_CW,
		WINDINGUSAGE_VARY,

		WINDINGUSAGE_LAST
	};

	PrimitiveSetInvarianceCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, bool usePointMode, WindingUsage windingUsage)
		: TestCase			(context, name, description)
		, m_primitiveType	(primType)
		, m_spacing			(spacing)
		, m_usePointMode	(usePointMode)
		, m_windingUsage	(windingUsage)
	{
	}

	void									init				(void);
	void									deinit				(void);
	IterateResult							iterate				(void);

protected:
	struct TessLevels
	{
		float inner[2];
		float outer[4];
		string description (void) const { return tessellationLevelsString(&inner[0], &outer[0]); }
	};
	struct LevelCase
	{
		vector<TessLevels>	levels;
		int					mem; //!< Subclass-defined arbitrary piece of data, for type of the levelcase, if needed. Passed to compare().
		LevelCase (const TessLevels& lev) : levels(vector<TessLevels>(1, lev)), mem(0) {}
		LevelCase (void) : mem(0) {}
	};

	virtual vector<LevelCase>	genTessLevelCases	(void) const;
	virtual bool				compare				(const vector<Vec3>& coordsA, const vector<Vec3>& coordsB, int levelCaseMem) const = 0;

	const TessPrimitiveType		m_primitiveType;

private:
	struct Program
	{
		Winding							winding;
		SharedPtr<const ShaderProgram>	program;

				Program			(Winding w, const SharedPtr<const ShaderProgram>& prog) : winding(w), program(prog) {}

		string	description		(void) const { return string() + "winding mode " + getWindingShaderName(winding); };
	};

	static const int			RENDER_SIZE = 16;

	const SpacingMode			m_spacing;
	const bool					m_usePointMode;
	const WindingUsage			m_windingUsage;

	vector<Program>				m_programs;
};

vector<PrimitiveSetInvarianceCase::LevelCase> PrimitiveSetInvarianceCase::genTessLevelCases (void) const
{
	static const TessLevels basicTessLevelCases[] =
	{
		{ { 1.0f,	1.0f	},	{ 1.0f,		1.0f,	1.0f,	1.0f	} },
		{ { 63.0f,	24.0f	},	{ 15.0f,	42.0f,	10.0f,	12.0f	} },
		{ { 3.0f,	2.0f	},	{ 6.0f,		8.0f,	7.0f,	9.0f	} },
		{ { 4.0f,	6.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 2.0f,	2.0f	},	{ 6.0f,		8.0f,	7.0f,	9.0f	} },
		{ { 5.0f,	6.0f	},	{ 1.0f,		1.0f,	1.0f,	1.0f	} },
		{ { 1.0f,	6.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 5.0f,	1.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 5.2f,	1.6f	},	{ 2.9f,		3.4f,	1.5f,	4.1f	} }
	};

	vector<LevelCase> result;
	for (int i = 0; i < DE_LENGTH_OF_ARRAY(basicTessLevelCases); i++)
		result.push_back(LevelCase(basicTessLevelCases[i]));

	{
		de::Random rnd(123);
		for (int i = 0; i < 10; i++)
		{
			TessLevels levels;
			for (int j = 0; j < DE_LENGTH_OF_ARRAY(levels.inner); j++)
				levels.inner[j] = rnd.getFloat(1.0f, 16.0f);
			for (int j = 0; j < DE_LENGTH_OF_ARRAY(levels.outer); j++)
				levels.outer[j] = rnd.getFloat(1.0f, 16.0f);
			result.push_back(LevelCase(levels));
		}
	}

	return result;
}

void PrimitiveSetInvarianceCase::init (void)
{
	const int			numDifferentConstantExprCases = 2;
	vector<Winding>		windings;
	switch (m_windingUsage)
	{
		case WINDINGUSAGE_CCW:		windings.push_back(WINDING_CCW); break;
		case WINDINGUSAGE_CW:		windings.push_back(WINDING_CW); break;
		case WINDINGUSAGE_VARY:		windings.push_back(WINDING_CCW);
									windings.push_back(WINDING_CW); break;
		default: DE_ASSERT(false);
	}

	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	for (int constantExprCaseNdx = 0; constantExprCaseNdx < numDifferentConstantExprCases; constantExprCaseNdx++)
	{
		for (int windingCaseNdx = 0; windingCaseNdx < (int)windings.size(); windingCaseNdx++)
		{
			const string	floatLit01 = de::floatToString(10.0f / (float)(constantExprCaseNdx + 10), 2);
			const int		programNdx = (int)m_programs.size();

			std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
															 "\n"
															 "in highp float in_v_attr;\n"
															 "out highp float in_tc_attr;\n"
															 "\n"
															 "void main (void)\n"
															 "{\n"
															 "	in_tc_attr = in_v_attr;\n"
														 "}\n");
			std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
														 "${TESSELLATION_SHADER_REQUIRE}\n"
															 "\n"
															 "layout (vertices = " + de::toString(constantExprCaseNdx+1) + ") out;\n"
															 "\n"
															 "in highp float in_tc_attr[];\n"
															 "\n"
															 "patch out highp float in_te_positionOffset;\n"
															 "\n"
															 "void main (void)\n"
															 "{\n"
															 "	in_te_positionOffset = in_tc_attr[6];\n"
															 "\n"
															 "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
															 "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
															 "\n"
															 "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
															 "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
															 "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
															 "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
														 "}\n");
			std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
														 "${TESSELLATION_SHADER_REQUIRE}\n"
															 "\n"
															 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing, windings[windingCaseNdx], m_usePointMode) +
															 "\n"
															 "patch in highp float in_te_positionOffset;\n"
															 "\n"
															 "out highp vec4 in_f_color;\n"
															 "invariant out highp vec3 out_te_tessCoord;\n"
															 "\n"
															 "void main (void)\n"
															 "{\n"
															 "	gl_Position = vec4(gl_TessCoord.xy*" + floatLit01 + " - in_te_positionOffset + float(gl_PrimitiveID)*0.1, 0.0, 1.0);\n"
															 "	in_f_color = vec4(" + floatLit01 + ");\n"
															 "	out_te_tessCoord = gl_TessCoord;\n"
														 "}\n");
			std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
															 "\n"
															 "layout (location = 0) out mediump vec4 o_color;\n"
															 "\n"
															 "in highp vec4 in_f_color;\n"
															 "\n"
															 "void main (void)\n"
															 "{\n"
															 "	o_color = in_f_color;\n"
														 "}\n");

			m_programs.push_back(Program(windings[windingCaseNdx],
										 SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
					<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
					<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
					<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
					<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
					<< glu::TransformFeedbackVarying		("out_te_tessCoord")
					<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)))));

			{
				const tcu::ScopedLogSection section(m_testCtx.getLog(), "Program" + de::toString(programNdx), "Program " + de::toString(programNdx));

				if (programNdx == 0 || !m_programs.back().program->isOk())
					m_testCtx.getLog() << *m_programs.back().program;

				if (!m_programs.back().program->isOk())
					TCU_FAIL("Program compilation failed");

				if (programNdx > 0)
					m_testCtx.getLog() << TestLog::Message << "Note: program " << programNdx << " is similar to above, except some constants are different, and: " << m_programs.back().description() << TestLog::EndMessage;
			}
		}
	}
}

void PrimitiveSetInvarianceCase::deinit (void)
{
	m_programs.clear();
}

PrimitiveSetInvarianceCase::IterateResult PrimitiveSetInvarianceCase::iterate (void)
{
	typedef TransformFeedbackHandler<Vec3> TFHandler;

	TestLog&					log					= m_testCtx.getLog();
	const RenderContext&		renderCtx			= m_context.getRenderContext();
	const RandomViewport		viewport			(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&		gl					= renderCtx.getFunctions();
	const vector<LevelCase>		tessLevelCases		= genTessLevelCases();
	vector<vector<int> >		primitiveCounts;
	int							maxNumPrimitives	= -1;

	for (int caseNdx = 0; caseNdx < (int)tessLevelCases.size(); caseNdx++)
	{
		primitiveCounts.push_back(vector<int>());
		for (int i = 0; i < (int)tessLevelCases[caseNdx].levels.size(); i++)
		{
			const int primCount = referencePrimitiveCount(m_primitiveType, m_spacing, m_usePointMode,
														  &tessLevelCases[caseNdx].levels[i].inner[0], &tessLevelCases[caseNdx].levels[i].outer[0]);
			primitiveCounts.back().push_back(primCount);
			maxNumPrimitives = de::max(maxNumPrimitives, primCount);
		}
	}

	const deUint32				primitiveTypeGL		= outputPrimitiveTypeGL(m_primitiveType, m_usePointMode);
	const TFHandler				transformFeedback	(m_context.getRenderContext(), 2*maxNumPrimitives*numVerticesPerPrimitive(primitiveTypeGL));

	setViewport(gl, viewport);
	gl.patchParameteri(GL_PATCH_VERTICES, 7);

	for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < (int)tessLevelCases.size(); tessLevelCaseNdx++)
	{
		const LevelCase&	levelCase = tessLevelCases[tessLevelCaseNdx];
		vector<Vec3>		firstPrimVertices;

		{
			string tessLevelsStr;
			for (int i = 0; i < (int)levelCase.levels.size(); i++)
				tessLevelsStr += (levelCase.levels.size() > 1 ? "\n" : "") + levelCase.levels[i].description();
			log << TestLog::Message << "Tessellation level sets: " << tessLevelsStr << TestLog::EndMessage;
		}

		for (int subTessLevelCaseNdx = 0; subTessLevelCaseNdx < (int)levelCase.levels.size(); subTessLevelCaseNdx++)
		{
			const TessLevels&				tessLevels		= levelCase.levels[subTessLevelCaseNdx];
			const float						(&inner)[2]		= tessLevels.inner;
			const float						(&outer)[4]		= tessLevels.outer;
			const float						attribute[2*7]	= { inner[0], inner[1], outer[0], outer[1], outer[2], outer[3], 0.0f,
																inner[0], inner[1], outer[0], outer[1], outer[2], outer[3], 0.5f };
			const glu::VertexArrayBinding	bindings[]		= { glu::va::Float("in_v_attr", 1, DE_LENGTH_OF_ARRAY(attribute), 0, &attribute[0]) };

			for (int programNdx = 0; programNdx < (int)m_programs.size(); programNdx++)
			{
				const deUint32				programGL	= m_programs[programNdx].program->getProgram();
				gl.useProgram(programGL);
				const TFHandler::Result		tfResult	= transformFeedback.renderAndGetPrimitives(programGL, primitiveTypeGL, DE_LENGTH_OF_ARRAY(bindings), &bindings[0], DE_LENGTH_OF_ARRAY(attribute));

				if (tfResult.numPrimitives != 2*primitiveCounts[tessLevelCaseNdx][subTessLevelCaseNdx])
				{
					log << TestLog::Message << "Failure: GL reported GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN to be "
											<< tfResult.numPrimitives << ", reference value is " << 2*primitiveCounts[tessLevelCaseNdx][subTessLevelCaseNdx]
											<< TestLog::EndMessage;

					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of primitives");
					return STOP;
				}

				{
					const int			half			= (int)tfResult.varying.size()/2;
					const vector<Vec3>	prim0Vertices	= vector<Vec3>(tfResult.varying.begin(), tfResult.varying.begin() + half);
					const Vec3* const	prim1Vertices	= &tfResult.varying[half];

					for (int vtxNdx = 0; vtxNdx < (int)prim0Vertices.size(); vtxNdx++)
					{
						if (prim0Vertices[vtxNdx] != prim1Vertices[vtxNdx])
						{
							log << TestLog::Message << "Failure: tessellation coordinate at index " << vtxNdx << " differs between two primitives drawn in one draw call" << TestLog::EndMessage
								<< TestLog::Message << "Note: the coordinate is " << prim0Vertices[vtxNdx] << " for the first primitive and " << prim1Vertices[vtxNdx] << " for the second" << TestLog::EndMessage
								<< TestLog::Message << "Note: tessellation levels for both primitives were: " << tessellationLevelsString(&inner[0], &outer[0]) << TestLog::EndMessage;
							m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of primitives");
							return STOP;
						}
					}

					if (programNdx == 0 && subTessLevelCaseNdx == 0)
						firstPrimVertices = prim0Vertices;
					else
					{
						const bool compareOk = compare(firstPrimVertices, prim0Vertices, levelCase.mem);
						if (!compareOk)
						{
							log << TestLog::Message << "Note: comparison of tessellation coordinates failed; comparison was made between following cases:\n"
													<< "  - case A: program 0, tessellation levels: " << tessLevelCases[tessLevelCaseNdx].levels[0].description() << "\n"
													<< "  - case B: program " << programNdx << ", tessellation levels: " << tessLevels.description() << TestLog::EndMessage;
							m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of primitives");
							return STOP;
						}
					}
				}
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #1
 *
 * Test that the sequence of primitives input to the TES only depends on
 * the tessellation levels, tessellation mode, spacing mode, winding, and
 * point mode.
 *//*--------------------------------------------------------------------*/
class InvariantPrimitiveSetCase : public PrimitiveSetInvarianceCase
{
public:
	InvariantPrimitiveSetCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode)
		: PrimitiveSetInvarianceCase(context, name, description, primType, spacing, usePointMode, winding == WINDING_CCW	? WINDINGUSAGE_CCW
																								: winding == WINDING_CW		? WINDINGUSAGE_CW
																								: WINDINGUSAGE_LAST)
	{
	}

protected:
	virtual bool compare (const vector<Vec3>& coordsA, const vector<Vec3>& coordsB, int) const
	{
		for (int vtxNdx = 0; vtxNdx < (int)coordsA.size(); vtxNdx++)
		{
			if (coordsA[vtxNdx] != coordsB[vtxNdx])
			{
				m_testCtx.getLog() << TestLog::Message << "Failure: tessellation coordinate at index " << vtxNdx << " differs between two programs" << TestLog::EndMessage
								   << TestLog::Message << "Note: the coordinate is " << coordsA[vtxNdx] << " for the first program and " << coordsB[vtxNdx] << " for the other" << TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #2
 *
 * Test that the set of vertices along an outer edge of a quad or triangle
 * only depends on that edge's tessellation level, and spacing.
 *
 * For each (outer) edge in the quad or triangle, draw multiple patches
 * with identical tessellation levels for that outer edge but with
 * different values for the other outer edges; compare, among the
 * primitives, the vertices generated for that outer edge. Repeat with
 * different programs, using different winding etc. settings. Compare
 * the edge's vertices between different programs.
 *//*--------------------------------------------------------------------*/
class InvariantOuterEdgeCase : public TestCase
{
public:
	InvariantOuterEdgeCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing)
		: TestCase			(context, name, description)
		, m_primitiveType	(primType)
		, m_spacing			(spacing)
	{
		DE_ASSERT(primType == TESSPRIMITIVETYPE_TRIANGLES || primType == TESSPRIMITIVETYPE_QUADS);
	}

	void						init		(void);
	void						deinit		(void);
	IterateResult				iterate		(void);

private:
	struct Program
	{
		Winding							winding;
		bool							usePointMode;
		SharedPtr<const ShaderProgram>	program;

				Program			(Winding w, bool point, const SharedPtr<const ShaderProgram>& prog) : winding(w), usePointMode(point), program(prog) {}

		string	description		(void) const { return string() + "winding mode " + getWindingShaderName(winding) + ", " + (usePointMode ? "" : "don't ") + "use point mode"; };
	};

	static vector<float>		generatePatchTessLevels (int numPatches, int constantOuterLevelIndex, float constantOuterLevel);

	static const int			RENDER_SIZE = 16;

	const TessPrimitiveType		m_primitiveType;
	const SpacingMode			m_spacing;

	vector<Program>				m_programs;
};

vector<float> InvariantOuterEdgeCase::generatePatchTessLevels (int numPatches, int constantOuterLevelIndex, float constantOuterLevel)
{
	de::Random rnd(123);
	return generateRandomPatchTessLevels(numPatches, constantOuterLevelIndex, constantOuterLevel, rnd);
}

void InvariantOuterEdgeCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	for (int windingI = 0; windingI < WINDING_LAST; windingI++)
	{
		const Winding winding = (Winding)windingI;

		for (int usePointModeI = 0; usePointModeI <= 1; usePointModeI++)
		{
			const bool		usePointMode	= usePointModeI != 0;
			const int		programNdx		= (int)m_programs.size();
			const string	floatLit01		= de::floatToString(10.0f / (float)(programNdx + 10), 2);

			std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
														 "\n"
														 "in highp float in_v_attr;\n"
														 "out highp float in_tc_attr;\n"
														 "\n"
														 "void main (void)\n"
														 "{\n"
														 "	in_tc_attr = in_v_attr;\n"
														 "}\n");
			std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
														 "${TESSELLATION_SHADER_REQUIRE}\n"
														 "\n"
														 "layout (vertices = " + de::toString(programNdx+1) + ") out;\n"
														 "\n"
														 "in highp float in_tc_attr[];\n"
														 "\n"
														 "void main (void)\n"
														 "{\n"
														 "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
														 "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
														 "\n"
														 "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
														 "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
														 "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
														 "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
														 "}\n");
			std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
														 "${TESSELLATION_SHADER_REQUIRE}\n"
														 "\n"
														 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing, winding, usePointMode) +
														 "\n"
														 "out highp vec4 in_f_color;\n"
														 "invariant out highp vec3 out_te_tessCoord;\n"
														 "\n"
														 "void main (void)\n"
														 "{\n"
														 "	gl_Position = vec4(gl_TessCoord.xy*" + floatLit01 + " - float(gl_PrimitiveID)*0.05, 0.0, 1.0);\n"
														 "	in_f_color = vec4(" + floatLit01 + ");\n"
														 "	out_te_tessCoord = gl_TessCoord;\n"
														 "}\n");
			std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
														 "\n"
														 "layout (location = 0) out mediump vec4 o_color;\n"
														 "\n"
														 "in highp vec4 in_f_color;\n"
														 "\n"
														 "void main (void)\n"
														 "{\n"
														 "	o_color = in_f_color;\n"
														 "}\n");

			m_programs.push_back(Program(winding, usePointMode,
										 SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
				<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
				<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
				<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
				<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
				<< glu::TransformFeedbackVarying		("out_te_tessCoord")
				<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)))));

			{
				const tcu::ScopedLogSection section(m_testCtx.getLog(), "Program" + de::toString(programNdx), "Program " + de::toString(programNdx));

				if (programNdx == 0 || !m_programs.back().program->isOk())
					m_testCtx.getLog() << *m_programs.back().program;

				if (!m_programs.back().program->isOk())
					TCU_FAIL("Program compilation failed");

				if (programNdx > 0)
					m_testCtx.getLog() << TestLog::Message << "Note: program " << programNdx << " is similar to above, except some constants are different, and: " << m_programs.back().description() << TestLog::EndMessage;
			}
		}
	}
}

void InvariantOuterEdgeCase::deinit (void)
{
	m_programs.clear();
}

InvariantOuterEdgeCase::IterateResult InvariantOuterEdgeCase::iterate (void)
{
	typedef TransformFeedbackHandler<Vec3> TFHandler;

	TestLog&							log							= m_testCtx.getLog();
	const RenderContext&				renderCtx					= m_context.getRenderContext();
	const RandomViewport				viewport					(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&				gl							= renderCtx.getFunctions();

	static const float					singleOuterEdgeLevels[]		= { 1.0f, 1.2f, 1.9f, 2.3f, 2.8f, 3.3f, 3.8f, 10.2f, 1.6f, 24.4f, 24.7f, 63.0f };
	const int							numPatchesPerDrawCall		= 10;
	const vector<OuterEdgeDescription>	edgeDescriptions			= outerEdgeDescriptions(m_primitiveType);

	{
		// Compute the number vertices in the largest draw call, so we can allocate the TF buffer just once.
		int maxNumVerticesInDrawCall = 0;
		{
			const vector<float> patchTessLevels = generatePatchTessLevels(numPatchesPerDrawCall, 0 /* outer-edge index doesn't affect vertex count */, arrayMax(singleOuterEdgeLevels));

			for (int usePointModeI = 0; usePointModeI <= 1; usePointModeI++)
				maxNumVerticesInDrawCall = de::max(maxNumVerticesInDrawCall,
												   multiplePatchReferenceVertexCount(m_primitiveType, m_spacing, usePointModeI != 0, &patchTessLevels[0], numPatchesPerDrawCall));
		}

		{
			const TFHandler tfHandler(m_context.getRenderContext(), maxNumVerticesInDrawCall);

			setViewport(gl, viewport);
			gl.patchParameteri(GL_PATCH_VERTICES, 6);

			for (int outerEdgeIndex = 0; outerEdgeIndex < (int)edgeDescriptions.size(); outerEdgeIndex++)
			{
				const OuterEdgeDescription& edgeDesc = edgeDescriptions[outerEdgeIndex];

				for (int outerEdgeLevelCaseNdx = 0; outerEdgeLevelCaseNdx < DE_LENGTH_OF_ARRAY(singleOuterEdgeLevels); outerEdgeLevelCaseNdx++)
				{
					typedef std::set<Vec3, VecLexLessThan<3> > Vec3Set;

					const vector<float>				patchTessLevels		= generatePatchTessLevels(numPatchesPerDrawCall, outerEdgeIndex, singleOuterEdgeLevels[outerEdgeLevelCaseNdx]);
					const glu::VertexArrayBinding	bindings[]			= { glu::va::Float("in_v_attr", 1, (int)patchTessLevels.size(), 0, &patchTessLevels[0]) };
					Vec3Set							firstOuterEdgeVertices; // Vertices of the outer edge of the first patch of the first program's draw call; used for comparison with other patches.

					log << TestLog::Message << "Testing with outer tessellation level " << singleOuterEdgeLevels[outerEdgeLevelCaseNdx]
											<< " for the " << edgeDesc.description() << " edge, and with various levels for other edges, and with all programs" << TestLog::EndMessage;

					for (int programNdx = 0; programNdx < (int)m_programs.size(); programNdx++)
					{
						const Program& program		= m_programs[programNdx];
						const deUint32 programGL	= program.program->getProgram();

						gl.useProgram(programGL);

						{
							const TFHandler::Result		tfResult			= tfHandler.renderAndGetPrimitives(programGL, outputPrimitiveTypeGL(m_primitiveType, program.usePointMode),
																											   DE_LENGTH_OF_ARRAY(bindings), &bindings[0], (int)patchTessLevels.size());
							const int					refNumVertices		= multiplePatchReferenceVertexCount(m_primitiveType, m_spacing, program.usePointMode, &patchTessLevels[0], numPatchesPerDrawCall);
							int							numVerticesRead		= 0;

							if ((int)tfResult.varying.size() != refNumVertices)
							{
								log << TestLog::Message << "Failure: the number of vertices returned by transform feedback is "
														<< tfResult.varying.size() << ", expected " << refNumVertices << TestLog::EndMessage
									<< TestLog::Message << "Note: rendered " << numPatchesPerDrawCall
														<< " patches in one draw call; tessellation levels for each patch are (in order [inner0, inner1, outer0, outer1, outer2, outer3]):\n"
														<< containerStr(patchTessLevels, 6) << TestLog::EndMessage;

								m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
								return STOP;
							}

							// Check the vertices of each patch.

							for (int patchNdx = 0; patchNdx < numPatchesPerDrawCall; patchNdx++)
							{
								const float* const	innerLevels			= &patchTessLevels[6*patchNdx + 0];
								const float* const	outerLevels			= &patchTessLevels[6*patchNdx + 2];
								const int			patchNumVertices	= referenceVertexCount(m_primitiveType, m_spacing, program.usePointMode, innerLevels, outerLevels);
								Vec3Set				outerEdgeVertices;

								// We're interested in just the vertices on the current outer edge.
								for(int vtxNdx = numVerticesRead; vtxNdx < numVerticesRead + patchNumVertices; vtxNdx++)
								{
									const Vec3& vtx = tfResult.varying[vtxNdx];
									if (edgeDesc.contains(vtx))
										outerEdgeVertices.insert(tfResult.varying[vtxNdx]);
								}

								// Check that the outer edge contains an appropriate number of vertices.
								{
									const int refNumVerticesOnOuterEdge = 1 + getClampedRoundedTessLevel(m_spacing, singleOuterEdgeLevels[outerEdgeLevelCaseNdx]);

									if ((int)outerEdgeVertices.size() != refNumVerticesOnOuterEdge)
									{
										log << TestLog::Message << "Failure: the number of vertices on outer edge is " << outerEdgeVertices.size()
																<< ", expected " << refNumVerticesOnOuterEdge << TestLog::EndMessage
											<< TestLog::Message << "Note: vertices on the outer edge are:\n" << containerStr(outerEdgeVertices, 5, 0) << TestLog::EndMessage
											<< TestLog::Message << "Note: the following parameters were used: " << program.description()
																<< ", tessellation levels: " << tessellationLevelsString(innerLevels, outerLevels) << TestLog::EndMessage;
										m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
										return STOP;
									}
								}

								// Compare the vertices to those of the first patch (unless this is the first patch).

								if (programNdx == 0 && patchNdx == 0)
									firstOuterEdgeVertices = outerEdgeVertices;
								else
								{
									if (firstOuterEdgeVertices != outerEdgeVertices)
									{
										log << TestLog::Message << "Failure: vertices generated for the edge differ between the following cases:\n"
																<< "  - case A: " << m_programs[0].description() << ", tessellation levels: "
																				  << tessellationLevelsString(&patchTessLevels[0], &patchTessLevels[2]) << "\n"
																<< "  - case B: " << program.description() << ", tessellation levels: "
																				  << tessellationLevelsString(innerLevels, outerLevels) << TestLog::EndMessage;

										log << TestLog::Message << "Note: resulting vertices for the edge for the cases were:\n"
																<< "  - case A: " << containerStr(firstOuterEdgeVertices, 5, 14) << "\n"
																<< "  - case B: " << containerStr(outerEdgeVertices, 5, 14) << TestLog::EndMessage;

										m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
										return STOP;
									}
								}

								numVerticesRead += patchNumVertices;
							}

							DE_ASSERT(numVerticesRead == (int)tfResult.varying.size());
						}
					}
				}
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #3
 *
 * Test that the vertices along an outer edge are placed symmetrically.
 *
 * Draw multiple patches with different tessellation levels and different
 * point_mode, winding etc. Before outputting tesscoords with TF, mirror
 * the vertices in the TES such that every vertex on an outer edge -
 * except the possible middle vertex - should be duplicated in the output.
 * Check that appropriate duplicates exist.
 *//*--------------------------------------------------------------------*/
class SymmetricOuterEdgeCase : public TestCase
{
public:
	SymmetricOuterEdgeCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode)
		: TestCase			(context, name, description)
		, m_primitiveType	(primType)
		, m_spacing			(spacing)
		, m_winding			(winding)
		, m_usePointMode	(usePointMode)
	{
	}

	void									init		(void);
	void									deinit		(void);
	IterateResult							iterate		(void);

private:
	static vector<float>					generatePatchTessLevels (int numPatches, int constantOuterLevelIndex, float constantOuterLevel);

	static const int						RENDER_SIZE = 16;

	const TessPrimitiveType					m_primitiveType;
	const SpacingMode						m_spacing;
	const Winding							m_winding;
	const bool								m_usePointMode;

	SharedPtr<const glu::ShaderProgram>		m_program;
};

vector<float> SymmetricOuterEdgeCase::generatePatchTessLevels (int numPatches, int constantOuterLevelIndex, float constantOuterLevel)
{
	de::Random rnd(123);
	return generateRandomPatchTessLevels(numPatches, constantOuterLevelIndex, constantOuterLevel, rnd);
}

void SymmetricOuterEdgeCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "in highp float in_v_attr;\n"
												 "out highp float in_tc_attr;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 "layout (vertices = 1) out;\n"
												 "\n"
												 "in highp float in_tc_attr[];\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
												 "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
												 "\n"
												 "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
												 "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
												 "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
												 "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing, m_winding, m_usePointMode) +
												 "\n"
												 "out highp vec4 in_f_color;\n"
												 "out highp vec4 out_te_tessCoord_isMirrored;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 + (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
													"	float x = gl_TessCoord.x;\n"
													"	float y = gl_TessCoord.y;\n"
													"	float z = gl_TessCoord.z;\n"
													"	// Mirror one half of each outer edge onto the other half, except the endpoints (because they belong to two edges)\n"
													"	out_te_tessCoord_isMirrored = z == 0.0 && x > 0.5 && x != 1.0 ? vec4(1.0-x,  1.0-y,    0.0, 1.0)\n"
													"	                            : y == 0.0 && z > 0.5 && z != 1.0 ? vec4(1.0-x,    0.0,  1.0-z, 1.0)\n"
													"	                            : x == 0.0 && y > 0.5 && y != 1.0 ? vec4(  0.0,  1.0-y,  1.0-z, 1.0)\n"
													"	                            : vec4(x, y, z, 0.0);\n"
												  : m_primitiveType == TESSPRIMITIVETYPE_QUADS ?
													"	float x = gl_TessCoord.x;\n"
													"	float y = gl_TessCoord.y;\n"
													"	// Mirror one half of each outer edge onto the other half, except the endpoints (because they belong to two edges)\n"
													"	out_te_tessCoord_isMirrored = (x == 0.0 || x == 1.0) && y > 0.5 && y != 1.0 ? vec4(    x, 1.0-y, 0.0, 1.0)\n"
													"	                            : (y == 0.0 || y == 1.0) && x > 0.5 && x != 1.0 ? vec4(1.0-x,     y, 0.0, 1.0)\n"
													"	                            : vec4(x, y, 0.0, 0.0);\n"
												  : m_primitiveType == TESSPRIMITIVETYPE_ISOLINES ?
													"	float x = gl_TessCoord.x;\n"
													"	float y = gl_TessCoord.y;\n"
													"	// Mirror one half of each outer edge onto the other half\n"
													"	out_te_tessCoord_isMirrored = (x == 0.0 || x == 1.0) && y > 0.5 ? vec4(x, 1.0-y, 0.0, 1.0)\n"
													"	                            : vec4(x, y, 0.0, 0.0f);\n"
												  : DE_NULL) +
												 "\n"
												 "	gl_Position = vec4(gl_TessCoord.xy, 0.0, 1.0);\n"
												 "	in_f_color = vec4(1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "layout (location = 0) out mediump vec4 o_color;\n"
												 "\n"
												 "in highp vec4 in_f_color;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
		<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
		<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
		<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
		<< glu::TransformFeedbackVarying		("out_te_tessCoord_isMirrored")
		<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void SymmetricOuterEdgeCase::deinit (void)
{
	m_program.clear();
}

SymmetricOuterEdgeCase::IterateResult SymmetricOuterEdgeCase::iterate (void)
{
	typedef TransformFeedbackHandler<Vec4> TFHandler;

	TestLog&							log							= m_testCtx.getLog();
	const RenderContext&				renderCtx					= m_context.getRenderContext();
	const RandomViewport				viewport					(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&				gl							= renderCtx.getFunctions();

	static const float					singleOuterEdgeLevels[]		= { 1.0f, 1.2f, 1.9f, 2.3f, 2.8f, 3.3f, 3.8f, 10.2f, 1.6f, 24.4f, 24.7f, 63.0f };
	const vector<OuterEdgeDescription>	edgeDescriptions			= outerEdgeDescriptions(m_primitiveType);

	{
		// Compute the number vertices in the largest draw call, so we can allocate the TF buffer just once.
		int maxNumVerticesInDrawCall;
		{
			const vector<float> patchTessLevels = generatePatchTessLevels(1, 0 /* outer-edge index doesn't affect vertex count */, arrayMax(singleOuterEdgeLevels));
			maxNumVerticesInDrawCall = referenceVertexCount(m_primitiveType, m_spacing, m_usePointMode, &patchTessLevels[0], &patchTessLevels[2]);
		}

		{
			const TFHandler tfHandler(m_context.getRenderContext(), maxNumVerticesInDrawCall);

			setViewport(gl, viewport);
			gl.patchParameteri(GL_PATCH_VERTICES, 6);

			for (int outerEdgeIndex = 0; outerEdgeIndex < (int)edgeDescriptions.size(); outerEdgeIndex++)
			{
				const OuterEdgeDescription& edgeDesc = edgeDescriptions[outerEdgeIndex];

				for (int outerEdgeLevelCaseNdx = 0; outerEdgeLevelCaseNdx < DE_LENGTH_OF_ARRAY(singleOuterEdgeLevels); outerEdgeLevelCaseNdx++)
				{
					typedef std::set<Vec3, VecLexLessThan<3> > Vec3Set;

					const vector<float>				patchTessLevels		= generatePatchTessLevels(1, outerEdgeIndex, singleOuterEdgeLevels[outerEdgeLevelCaseNdx]);
					const glu::VertexArrayBinding	bindings[]			= { glu::va::Float("in_v_attr", 1, (int)patchTessLevels.size(), 0, &patchTessLevels[0]) };

					log << TestLog::Message << "Testing with outer tessellation level " << singleOuterEdgeLevels[outerEdgeLevelCaseNdx]
											<< " for the " << edgeDesc.description() << " edge, and with various levels for other edges" << TestLog::EndMessage;

					{
						const deUint32 programGL = m_program->getProgram();

						gl.useProgram(programGL);

						{
							const TFHandler::Result		tfResult		= tfHandler.renderAndGetPrimitives(programGL, outputPrimitiveTypeGL(m_primitiveType, m_usePointMode),
																										   DE_LENGTH_OF_ARRAY(bindings), &bindings[0], (int)patchTessLevels.size());
							const int					refNumVertices	= referenceVertexCount(m_primitiveType, m_spacing, m_usePointMode, &patchTessLevels[0], &patchTessLevels[2]);

							if ((int)tfResult.varying.size() != refNumVertices)
							{
								log << TestLog::Message << "Failure: the number of vertices returned by transform feedback is "
														<< tfResult.varying.size() << ", expected " << refNumVertices << TestLog::EndMessage
									<< TestLog::Message << "Note: rendered 1 patch, tessellation levels are (in order [inner0, inner1, outer0, outer1, outer2, outer3]):\n"
														<< containerStr(patchTessLevels, 6) << TestLog::EndMessage;

								m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
								return STOP;
							}

							// Check the vertices.

							{
								Vec3Set nonMirroredEdgeVertices;
								Vec3Set mirroredEdgeVertices;

								// We're interested in just the vertices on the current outer edge.
								for(int vtxNdx = 0; vtxNdx < refNumVertices; vtxNdx++)
								{
									const Vec3& vtx = tfResult.varying[vtxNdx].swizzle(0,1,2);
									if (edgeDesc.contains(vtx))
									{
										// Ignore the middle vertex of the outer edge, as it's exactly at the mirroring point;
										// for isolines, also ignore (0, 0) and (1, 0) because there's no mirrored counterpart for them.
										if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES && vtx == tcu::select(Vec3(0.0f), Vec3(0.5f), singleTrueMask<3>(edgeDesc.constantCoordinateIndex)))
											continue;
										if (m_primitiveType == TESSPRIMITIVETYPE_QUADS && vtx.swizzle(0,1) == tcu::select(Vec2(edgeDesc.constantCoordinateValueChoices[0]),
																															   Vec2(0.5f),
																															   singleTrueMask<2>(edgeDesc.constantCoordinateIndex)))
											continue;
										if (m_primitiveType == TESSPRIMITIVETYPE_ISOLINES && (vtx == Vec3(0.0f, 0.5f, 0.0f) || vtx == Vec3(1.0f, 0.5f, 0.0f) ||
																							  vtx == Vec3(0.0f, 0.0f, 0.0f) || vtx == Vec3(1.0f, 0.0f, 0.0f)))
											continue;

										const bool isMirrored = tfResult.varying[vtxNdx].w() > 0.5f;
										if (isMirrored)
											mirroredEdgeVertices.insert(vtx);
										else
											nonMirroredEdgeVertices.insert(vtx);
									}
								}

								if (m_primitiveType != TESSPRIMITIVETYPE_ISOLINES)
								{
									// Check that both endpoints are present. Note that endpoints aren't mirrored by the shader, since they belong to more than one edge.

									Vec3 endpointA;
									Vec3 endpointB;

									if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
									{
										endpointA = tcu::select(Vec3(1.0f), Vec3(0.0f), singleTrueMask<3>((edgeDesc.constantCoordinateIndex + 1) % 3));
										endpointB = tcu::select(Vec3(1.0f), Vec3(0.0f), singleTrueMask<3>((edgeDesc.constantCoordinateIndex + 2) % 3));
									}
									else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS)
									{
										endpointA.xy() = tcu::select(Vec2(edgeDesc.constantCoordinateValueChoices[0]), Vec2(0.0f), singleTrueMask<2>(edgeDesc.constantCoordinateIndex));
										endpointB.xy() = tcu::select(Vec2(edgeDesc.constantCoordinateValueChoices[0]), Vec2(1.0f), singleTrueMask<2>(edgeDesc.constantCoordinateIndex));
									}
									else
										DE_ASSERT(false);

									if (!contains(nonMirroredEdgeVertices, endpointA) ||
										!contains(nonMirroredEdgeVertices, endpointB))
									{
										log << TestLog::Message << "Failure: edge doesn't contain both endpoints, " << endpointA << " and " << endpointB << TestLog::EndMessage
											<< TestLog::Message << "Note: non-mirrored vertices:\n" << containerStr(nonMirroredEdgeVertices, 5)
																<< "\nmirrored vertices:\n" << containerStr(mirroredEdgeVertices, 5) << TestLog::EndMessage;
										m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
										return STOP;
									}
									nonMirroredEdgeVertices.erase(endpointA);
									nonMirroredEdgeVertices.erase(endpointB);
								}

								if (nonMirroredEdgeVertices != mirroredEdgeVertices)
								{
									log << TestLog::Message << "Failure: the set of mirrored edges isn't equal to the set of non-mirrored edges (ignoring endpoints and possible middle)" << TestLog::EndMessage
										<< TestLog::Message << "Note: non-mirrored vertices:\n" << containerStr(nonMirroredEdgeVertices, 5)
																<< "\nmirrored vertices:\n" << containerStr(mirroredEdgeVertices, 5) << TestLog::EndMessage;
									m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
									return STOP;
								}
							}
						}
					}
				}
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #4
 *
 * Test that the vertices on an outer edge don't depend on which of the
 * edges it is, other than with respect to component order.
 *//*--------------------------------------------------------------------*/
class OuterEdgeVertexSetIndexIndependenceCase : public TestCase
{
public:
	OuterEdgeVertexSetIndexIndependenceCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode)
		: TestCase			(context, name, description)
		, m_primitiveType	(primType)
		, m_spacing			(spacing)
		, m_winding			(winding)
		, m_usePointMode	(usePointMode)
	{
		DE_ASSERT(primType == TESSPRIMITIVETYPE_TRIANGLES || primType == TESSPRIMITIVETYPE_QUADS);
	}

	void									init		(void);
	void									deinit		(void);
	IterateResult							iterate		(void);

private:
	static vector<float>					generatePatchTessLevels (int numPatches, int constantOuterLevelIndex, float constantOuterLevel);

	static const int						RENDER_SIZE = 16;

	const TessPrimitiveType					m_primitiveType;
	const SpacingMode						m_spacing;
	const Winding							m_winding;
	const bool								m_usePointMode;

	SharedPtr<const glu::ShaderProgram>		m_program;
};

vector<float> OuterEdgeVertexSetIndexIndependenceCase::generatePatchTessLevels (int numPatches, int constantOuterLevelIndex, float constantOuterLevel)
{
	de::Random rnd(123);
	return generateRandomPatchTessLevels(numPatches, constantOuterLevelIndex, constantOuterLevel, rnd);
}

void OuterEdgeVertexSetIndexIndependenceCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "in highp float in_v_attr;\n"
												 "out highp float in_tc_attr;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 "layout (vertices = 1) out;\n"
												 "\n"
												 "in highp float in_tc_attr[];\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
												 "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
												 "\n"
												 "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
												 "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
												 "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
												 "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing, m_winding, m_usePointMode) +
												 "\n"
												 "out highp vec4 in_f_color;\n"
												 "out highp vec3 out_te_tessCoord;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	out_te_tessCoord = gl_TessCoord;"
												 "	gl_Position = vec4(gl_TessCoord.xy, 0.0, 1.0);\n"
												 "	in_f_color = vec4(1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "layout (location = 0) out mediump vec4 o_color;\n"
												 "\n"
												 "in highp vec4 in_f_color;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
		<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
		<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
		<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
		<< glu::TransformFeedbackVarying		("out_te_tessCoord")
		<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void OuterEdgeVertexSetIndexIndependenceCase::deinit (void)
{
	m_program.clear();
}

OuterEdgeVertexSetIndexIndependenceCase::IterateResult OuterEdgeVertexSetIndexIndependenceCase::iterate (void)
{
	typedef TransformFeedbackHandler<Vec3> TFHandler;

	TestLog&							log							= m_testCtx.getLog();
	const RenderContext&				renderCtx					= m_context.getRenderContext();
	const RandomViewport				viewport					(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&				gl							= renderCtx.getFunctions();
	const deUint32						programGL					= m_program->getProgram();

	static const float					singleOuterEdgeLevels[]		= { 1.0f, 1.2f, 1.9f, 2.3f, 2.8f, 3.3f, 3.8f, 10.2f, 1.6f, 24.4f, 24.7f, 63.0f };
	const vector<OuterEdgeDescription>	edgeDescriptions			= outerEdgeDescriptions(m_primitiveType);

	gl.useProgram(programGL);
	setViewport(gl, viewport);
	gl.patchParameteri(GL_PATCH_VERTICES, 6);

	{
		// Compute the number vertices in the largest draw call, so we can allocate the TF buffer just once.
		int maxNumVerticesInDrawCall = 0;
		{
			const vector<float> patchTessLevels = generatePatchTessLevels(1, 0 /* outer-edge index doesn't affect vertex count */, arrayMax(singleOuterEdgeLevels));
			maxNumVerticesInDrawCall = referenceVertexCount(m_primitiveType, m_spacing, m_usePointMode, &patchTessLevels[0], &patchTessLevels[2]);
		}

		{
			const TFHandler tfHandler(m_context.getRenderContext(), maxNumVerticesInDrawCall);

			for (int outerEdgeLevelCaseNdx = 0; outerEdgeLevelCaseNdx < DE_LENGTH_OF_ARRAY(singleOuterEdgeLevels); outerEdgeLevelCaseNdx++)
			{
				typedef std::set<Vec3, VecLexLessThan<3> > Vec3Set;

				Vec3Set firstEdgeVertices;

				for (int outerEdgeIndex = 0; outerEdgeIndex < (int)edgeDescriptions.size(); outerEdgeIndex++)
				{
					const OuterEdgeDescription&		edgeDesc			= edgeDescriptions[outerEdgeIndex];
					const vector<float>				patchTessLevels		= generatePatchTessLevels(1, outerEdgeIndex, singleOuterEdgeLevels[outerEdgeLevelCaseNdx]);
					const glu::VertexArrayBinding	bindings[]			= { glu::va::Float("in_v_attr", 1, (int)patchTessLevels.size(), 0, &patchTessLevels[0]) };

					log << TestLog::Message << "Testing with outer tessellation level " << singleOuterEdgeLevels[outerEdgeLevelCaseNdx]
											<< " for the " << edgeDesc.description() << " edge, and with various levels for other edges" << TestLog::EndMessage;

					{
						const TFHandler::Result		tfResult		= tfHandler.renderAndGetPrimitives(programGL, outputPrimitiveTypeGL(m_primitiveType, m_usePointMode),
																										DE_LENGTH_OF_ARRAY(bindings), &bindings[0], (int)patchTessLevels.size());
						const int					refNumVertices	= referenceVertexCount(m_primitiveType, m_spacing, m_usePointMode, &patchTessLevels[0], &patchTessLevels[2]);

						if ((int)tfResult.varying.size() != refNumVertices)
						{
							log << TestLog::Message << "Failure: the number of vertices returned by transform feedback is "
													<< tfResult.varying.size() << ", expected " << refNumVertices << TestLog::EndMessage
								<< TestLog::Message << "Note: rendered 1 patch, tessellation levels are (in order [inner0, inner1, outer0, outer1, outer2, outer3]):\n"
													<< containerStr(patchTessLevels, 6) << TestLog::EndMessage;

							m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
							return STOP;
						}

						{
							Vec3Set currentEdgeVertices;

							// Get the vertices on the current outer edge.
							for(int vtxNdx = 0; vtxNdx < refNumVertices; vtxNdx++)
							{
								const Vec3& vtx = tfResult.varying[vtxNdx];
								if (edgeDesc.contains(vtx))
								{
									// Swizzle components to match the order of the first edge.
									if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
									{
										currentEdgeVertices.insert(outerEdgeIndex == 0 ? vtx
																	: outerEdgeIndex == 1 ? vtx.swizzle(1, 0, 2)
																	: outerEdgeIndex == 2 ? vtx.swizzle(2, 1, 0)
																	: Vec3(-1.0f));
									}
									else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS)
									{
										currentEdgeVertices.insert(Vec3(outerEdgeIndex == 0 ? vtx.y()
																		: outerEdgeIndex == 1 ? vtx.x()
																		: outerEdgeIndex == 2 ? vtx.y()
																		: outerEdgeIndex == 3 ? vtx.x()
																		: -1.0f,
																		0.0f, 0.0f));
									}
									else
										DE_ASSERT(false);
								}
							}

							if (outerEdgeIndex == 0)
								firstEdgeVertices = currentEdgeVertices;
							else
							{
								// Compare vertices of this edge to those of the first edge.

								if (currentEdgeVertices != firstEdgeVertices)
								{
									const char* const swizzleDesc = m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? (outerEdgeIndex == 1 ? "(y, x, z)"
																													: outerEdgeIndex == 2 ? "(z, y, x)"
																													: DE_NULL)
																	: m_primitiveType == TESSPRIMITIVETYPE_QUADS ? (outerEdgeIndex == 1 ? "(x, 0)"
																												: outerEdgeIndex == 2 ? "(y, 0)"
																												: outerEdgeIndex == 3 ? "(x, 0)"
																												: DE_NULL)
																	: DE_NULL;

									log << TestLog::Message << "Failure: the set of vertices on the " << edgeDesc.description() << " edge"
															<< " doesn't match the set of vertices on the " << edgeDescriptions[0].description() << " edge" << TestLog::EndMessage
										<< TestLog::Message << "Note: set of vertices on " << edgeDesc.description() << " edge, components swizzled like " << swizzleDesc
															<< " to match component order on first edge:\n" << containerStr(currentEdgeVertices, 5)
															<< "\non " << edgeDescriptions[0].description() << " edge:\n" << containerStr(firstEdgeVertices, 5) << TestLog::EndMessage;
									m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid set of vertices");
									return STOP;
								}
							}
						}
					}
				}
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #5
 *
 * Test that the set of triangles input to the TES only depends on the
 * tessellation levels, tessellation mode and spacing mode. Specifically,
 * winding doesn't change the set of triangles, though it can change the
 * order in which they are input to TES, and can (and will) change the
 * vertex order within a triangle.
 *//*--------------------------------------------------------------------*/
class InvariantTriangleSetCase : public PrimitiveSetInvarianceCase
{
public:
	InvariantTriangleSetCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing)
		: PrimitiveSetInvarianceCase(context, name, description, primType, spacing, false, WINDINGUSAGE_VARY)
	{
		DE_ASSERT(primType == TESSPRIMITIVETYPE_TRIANGLES || primType == TESSPRIMITIVETYPE_QUADS);
	}

protected:
	virtual bool compare (const vector<Vec3>& coordsA, const vector<Vec3>& coordsB, int) const
	{
		return compareTriangleSets(coordsA, coordsB, m_testCtx.getLog());
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #6
 *
 * Test that the set of inner triangles input to the TES only depends on
 * the inner tessellation levels, tessellation mode and spacing mode.
 *//*--------------------------------------------------------------------*/
class InvariantInnerTriangleSetCase : public PrimitiveSetInvarianceCase
{
public:
	InvariantInnerTriangleSetCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing)
		: PrimitiveSetInvarianceCase(context, name, description, primType, spacing, false, WINDINGUSAGE_VARY)
	{
		DE_ASSERT(primType == TESSPRIMITIVETYPE_TRIANGLES || primType == TESSPRIMITIVETYPE_QUADS);
	}

protected:
	virtual vector<LevelCase> genTessLevelCases (void) const
	{
		const int					numSubCases		= 4;
		const vector<LevelCase>		baseResults		= PrimitiveSetInvarianceCase::genTessLevelCases();
		vector<LevelCase>			result;
		de::Random					rnd				(123);

		// Generate variants with different values for irrelevant levels.
		for (int baseNdx = 0; baseNdx < (int)baseResults.size(); baseNdx++)
		{
			const TessLevels&	base	= baseResults[baseNdx].levels[0];
			TessLevels			levels	= base;
			LevelCase			levelCase;

			for (int subNdx = 0; subNdx < numSubCases; subNdx++)
			{
				levelCase.levels.push_back(levels);

				for (int i = 0; i < DE_LENGTH_OF_ARRAY(levels.outer); i++)
					levels.outer[i] = rnd.getFloat(2.0f, 16.0f);
				if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
					levels.inner[1] = rnd.getFloat(2.0f, 16.0f);
			}

			result.push_back(levelCase);
		}

		return result;
	}

	struct IsInnerTriangleTriangle
	{
		bool operator() (const Vec3* vertices) const
		{
			for (int v = 0; v < 3; v++)
				for (int c = 0; c < 3; c++)
					if (vertices[v][c] == 0.0f)
						return false;
			return true;
		}
	};

	struct IsInnerQuadTriangle
	{
		bool operator() (const Vec3* vertices) const
		{
			for (int v = 0; v < 3; v++)
				for (int c = 0; c < 2; c++)
					if (vertices[v][c] == 0.0f || vertices[v][c] == 1.0f)
						return false;
			return true;
		}
	};

	virtual bool compare (const vector<Vec3>& coordsA, const vector<Vec3>& coordsB, int) const
	{
		if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			return compareTriangleSets(coordsA, coordsB, m_testCtx.getLog(), IsInnerTriangleTriangle(), "outer triangles");
		else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS)
			return compareTriangleSets(coordsA, coordsB, m_testCtx.getLog(), IsInnerQuadTriangle(), "outer triangles");
		else
		{
			DE_ASSERT(false);
			return false;
		}
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #7
 *
 * Test that the set of outer triangles input to the TES only depends on
 * tessellation mode, spacing mode and the inner and outer tessellation
 * levels corresponding to the inner and outer edges relevant to that
 * triangle.
 *//*--------------------------------------------------------------------*/
class InvariantOuterTriangleSetCase : public PrimitiveSetInvarianceCase
{
public:
	InvariantOuterTriangleSetCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing)
		: PrimitiveSetInvarianceCase(context, name, description, primType, spacing, false, WINDINGUSAGE_VARY)
	{
		DE_ASSERT(primType == TESSPRIMITIVETYPE_TRIANGLES || primType == TESSPRIMITIVETYPE_QUADS);
	}

protected:
	virtual vector<LevelCase> genTessLevelCases (void) const
	{
		const int					numSubCasesPerEdge	= 4;
		const int					numEdges			= m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES	? 3
														: m_primitiveType == TESSPRIMITIVETYPE_QUADS		? 4
														: -1;
		const vector<LevelCase>		baseResult			= PrimitiveSetInvarianceCase::genTessLevelCases();
		vector<LevelCase>			result;
		de::Random					rnd					(123);

		// Generate variants with different values for irrelevant levels.
		for (int baseNdx = 0; baseNdx < (int)baseResult.size(); baseNdx++)
		{
			const TessLevels& base = baseResult[baseNdx].levels[0];
			if (base.inner[0] == 1.0f || (m_primitiveType == TESSPRIMITIVETYPE_QUADS && base.inner[1] == 1.0f))
				continue;

			for (int edgeNdx = 0; edgeNdx < numEdges; edgeNdx++)
			{
				TessLevels	levels = base;
				LevelCase	levelCase;
				levelCase.mem = edgeNdx;

				for (int subCaseNdx = 0; subCaseNdx < numSubCasesPerEdge; subCaseNdx++)
				{
					levelCase.levels.push_back(levels);

					for (int i = 0; i < DE_LENGTH_OF_ARRAY(levels.outer); i++)
					{
						if (i != edgeNdx)
							levels.outer[i] = rnd.getFloat(2.0f, 16.0f);
					}

					if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
						levels.inner[1] = rnd.getFloat(2.0f, 16.0f);
				}

				result.push_back(levelCase);
			}
		}

		return result;
	}

	class IsTriangleTriangleOnOuterEdge
	{
	public:
		IsTriangleTriangleOnOuterEdge (int edgeNdx) : m_edgeNdx(edgeNdx) {}
		bool operator() (const Vec3* vertices) const
		{
			bool touchesAppropriateEdge = false;
			for (int v = 0; v < 3; v++)
				if (vertices[v][m_edgeNdx] == 0.0f)
					touchesAppropriateEdge = true;

			if (touchesAppropriateEdge)
			{
				const Vec3 avg = (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
				return avg[m_edgeNdx] < avg[(m_edgeNdx+1)%3] &&
					   avg[m_edgeNdx] < avg[(m_edgeNdx+2)%3];
			}
			return false;
		}

	private:
		int m_edgeNdx;
	};

	class IsQuadTriangleOnOuterEdge
	{
	public:
		IsQuadTriangleOnOuterEdge (int edgeNdx) : m_edgeNdx(edgeNdx) {}

		bool onEdge (const Vec3& v) const
		{
			return v[m_edgeNdx%2] == (m_edgeNdx <= 1 ? 0.0f : 1.0f);
		}

		static inline bool onAnyEdge (const Vec3& v)
		{
			return v[0] == 0.0f || v[0] == 1.0f || v[1] == 0.0f || v[1] == 1.0f;
		}

		bool operator() (const Vec3* vertices) const
		{
			for (int v = 0; v < 3; v++)
			{
				const Vec3& a = vertices[v];
				const Vec3& b = vertices[(v+1)%3];
				const Vec3& c = vertices[(v+2)%3];
				if (onEdge(a) && onEdge(b))
					return true;
				if (onEdge(c) && !onAnyEdge(a) && !onAnyEdge(b) && a[m_edgeNdx%2] == b[m_edgeNdx%2])
					return true;
			}

			return false;
		}

	private:
		int m_edgeNdx;
	};

	virtual bool compare (const vector<Vec3>& coordsA, const vector<Vec3>& coordsB, int outerEdgeNdx) const
	{
		if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
		{
			return compareTriangleSets(coordsA, coordsB, m_testCtx.getLog(),
									   IsTriangleTriangleOnOuterEdge(outerEdgeNdx),
									   ("inner triangles, and outer triangles corresponding to other edge than edge "
										+ outerEdgeDescriptions(m_primitiveType)[outerEdgeNdx].description()).c_str());
		}
		else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS)
		{
			return compareTriangleSets(coordsA, coordsB, m_testCtx.getLog(),
									   IsQuadTriangleOnOuterEdge(outerEdgeNdx),
									   ("inner triangles, and outer triangles corresponding to other edge than edge "
										+ outerEdgeDescriptions(m_primitiveType)[outerEdgeNdx].description()).c_str());
		}
		else
			DE_ASSERT(false);

		return true;
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Base class for testing individual components of tess coords
 *
 * Useful for testing parts of invariance rule #8.
 *//*--------------------------------------------------------------------*/
class TessCoordComponentInvarianceCase : public TestCase
{
public:
	TessCoordComponentInvarianceCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode)
		: TestCase			(context, name, description)
		, m_primitiveType	(primType)
		, m_spacing			(spacing)
		, m_winding			(winding)
		, m_usePointMode	(usePointMode)
	{
	}

	void									init		(void);
	void									deinit		(void);
	IterateResult							iterate		(void);

protected:
	virtual string							tessEvalOutputComponentStatements	(const char* tessCoordComponentName, const char* outputComponentName) const = 0;
	virtual bool							checkTessCoordComponent				(float component) const = 0;

private:
	static vector<float>					genTessLevelCases (int numCases);

	static const int						RENDER_SIZE = 16;

	const TessPrimitiveType					m_primitiveType;
	const SpacingMode						m_spacing;
	const Winding							m_winding;
	const bool								m_usePointMode;

	SharedPtr<const glu::ShaderProgram>		m_program;
};

void TessCoordComponentInvarianceCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "in highp float in_v_attr;\n"
												 "out highp float in_tc_attr;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 "layout (vertices = 1) out;\n"
												 "\n"
												 "in highp float in_tc_attr[];\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
												 "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
												 "\n"
												 "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
												 "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
												 "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
												 "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing, m_winding, m_usePointMode) +
												 "\n"
												 "out highp vec4 in_f_color;\n"
												 "out highp vec3 out_te_output;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 + tessEvalOutputComponentStatements("gl_TessCoord.x", "out_te_output.x")
												 + tessEvalOutputComponentStatements("gl_TessCoord.y", "out_te_output.y")

												 + (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
													tessEvalOutputComponentStatements("gl_TessCoord.z", "out_te_output.z") :
													"	out_te_output.z = 0.0f;\n") +
												 "	gl_Position = vec4(gl_TessCoord.xy, 0.0, 1.0);\n"
												 "	in_f_color = vec4(1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "layout (location = 0) out mediump vec4 o_color;\n"
												 "\n"
												 "in highp vec4 in_f_color;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
		<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
		<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
		<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
		<< glu::TransformFeedbackVarying		("out_te_output")
		<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void TessCoordComponentInvarianceCase::deinit (void)
{
	m_program.clear();
}

vector<float> TessCoordComponentInvarianceCase::genTessLevelCases (int numCases)
{
	de::Random		rnd(123);
	vector<float>	result;

	for (int i = 0; i < numCases; i++)
		for (int j = 0; j < 6; j++)
			result.push_back(rnd.getFloat(1.0f, 63.0f));

	return result;
}

TessCoordComponentInvarianceCase::IterateResult TessCoordComponentInvarianceCase::iterate (void)
{
	typedef TransformFeedbackHandler<Vec3> TFHandler;

	TestLog&				log					= m_testCtx.getLog();
	const RenderContext&	renderCtx			= m_context.getRenderContext();
	const RandomViewport	viewport			(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&	gl					= renderCtx.getFunctions();
	const int				numTessLevelCases	= 32;
	const vector<float>		tessLevels			= genTessLevelCases(numTessLevelCases);
	const deUint32			programGL			= m_program->getProgram();

	gl.useProgram(programGL);
	setViewport(gl, viewport);
	gl.patchParameteri(GL_PATCH_VERTICES, 6);

	{
		// Compute the number vertices in the largest draw call, so we can allocate the TF buffer just once.
		int maxNumVerticesInDrawCall = 0;
		for (int i = 0; i < numTessLevelCases; i++)
			maxNumVerticesInDrawCall = de::max(maxNumVerticesInDrawCall, referenceVertexCount(m_primitiveType, m_spacing, m_usePointMode, &tessLevels[6*i+0], &tessLevels[6*i+2]));

		{
			const TFHandler tfHandler(m_context.getRenderContext(), maxNumVerticesInDrawCall);

			for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < numTessLevelCases; tessLevelCaseNdx++)
			{
				log << TestLog::Message << "Testing with tessellation levels: " << tessellationLevelsString(&tessLevels[6*tessLevelCaseNdx+0], &tessLevels[6*tessLevelCaseNdx+2]) << TestLog::EndMessage;

				const glu::VertexArrayBinding bindings[] = { glu::va::Float("in_v_attr", 1, (int)6, 0, &tessLevels[6*tessLevelCaseNdx]) };
				const TFHandler::Result tfResult = tfHandler.renderAndGetPrimitives(programGL, outputPrimitiveTypeGL(m_primitiveType, m_usePointMode),
																					DE_LENGTH_OF_ARRAY(bindings), &bindings[0], 6);

				for (int vtxNdx = 0; vtxNdx < (int)tfResult.varying.size(); vtxNdx++)
				{
					const Vec3&		vec			= tfResult.varying[vtxNdx];
					const int		numComps	= m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 2;

					for (int compNdx = 0; compNdx < numComps; compNdx++)
					{
						if (!checkTessCoordComponent(vec[compNdx]))
						{
							log << TestLog::Message << "Note: output value at index " << vtxNdx << " is "
													<< (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? de::toString(vec) : de::toString(vec.swizzle(0,1)))
													<< TestLog::EndMessage;
							m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid tessellation coordinate component");
							return STOP;
						}
					}
				}
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Test first part of invariance rule #8
 *
 * Test that all (relevant) components of tess coord are in [0,1].
 *//*--------------------------------------------------------------------*/
class TessCoordComponentRangeCase : public TessCoordComponentInvarianceCase
{
public:
	TessCoordComponentRangeCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode)
		: TessCoordComponentInvarianceCase(context, name, description, primType, spacing, winding, usePointMode)
	{
	}

protected:
	virtual string tessEvalOutputComponentStatements (const char* tessCoordComponentName, const char* outputComponentName) const
	{
		return string() + "\t" + outputComponentName + " = " + tessCoordComponentName + ";\n";
	}

	virtual bool checkTessCoordComponent (float component) const
	{
		if (!de::inRange(component, 0.0f, 1.0f))
		{
			m_testCtx.getLog() << TestLog::Message << "Failure: tess coord component isn't in range [0,1]" << TestLog::EndMessage;
			return false;
		}
		return true;
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test second part of invariance rule #8
 *
 * Test that all (relevant) components of tess coord are in [0,1] and
 * 1.0-c is exact for every such component c.
 *//*--------------------------------------------------------------------*/
class OneMinusTessCoordComponentCase : public TessCoordComponentInvarianceCase
{
public:
	OneMinusTessCoordComponentCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode)
		: TessCoordComponentInvarianceCase(context, name, description, primType, spacing, winding, usePointMode)
	{
	}

protected:
	virtual string tessEvalOutputComponentStatements (const char* tessCoordComponentName, const char* outputComponentName) const
	{
		return string() + "	{\n"
						  "		float oneMinusComp = 1.0 - " + tessCoordComponentName + ";\n"
						  "		" + outputComponentName + " = " + tessCoordComponentName + " + oneMinusComp;\n"
						  "	}\n";
	}

	virtual bool checkTessCoordComponent (float component) const
	{
		if (component != 1.0f)
		{
			m_testCtx.getLog() << TestLog::Message << "Failure: comp + (1.0-comp) doesn't equal 1.0 for some component of tessellation coordinate" << TestLog::EndMessage;
			return false;
		}
		return true;
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test that patch is discarded if relevant outer level <= 0.0
 *
 * Draws patches with different combinations of tessellation levels,
 * varying which levels are negative. Verifies by checking that colored
 * pixels exist inside the area of valid primitives, and only black pixels
 * exist inside the area of discarded primitives. An additional sanity
 * test is done, checking that the number of primitives written by TF is
 * correct.
 *//*--------------------------------------------------------------------*/
class PrimitiveDiscardCase : public TestCase
{
public:
	PrimitiveDiscardCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, SpacingMode spacing, Winding winding, bool usePointMode)
		: TestCase			(context, name, description)
		, m_primitiveType	(primType)
		, m_spacing			(spacing)
		, m_winding			(winding)
		, m_usePointMode	(usePointMode)
	{
	}

	void									init		(void);
	void									deinit		(void);
	IterateResult							iterate		(void);

private:
	static vector<float>					genAttributes (void);

	static const int						RENDER_SIZE = 256;

	const TessPrimitiveType					m_primitiveType;
	const SpacingMode						m_spacing;
	const Winding							m_winding;
	const bool								m_usePointMode;

	SharedPtr<const glu::ShaderProgram>		m_program;
};

void PrimitiveDiscardCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "in highp float in_v_attr;\n"
												 "out highp float in_tc_attr;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 "layout (vertices = 1) out;\n"
												 "\n"
												 "in highp float in_tc_attr[];\n"
												 "\n"
												 "patch out highp vec2 in_te_positionScale;\n"
												 "patch out highp vec2 in_te_positionOffset;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	in_te_positionScale  = vec2(in_tc_attr[6], in_tc_attr[7]);\n"
												 "	in_te_positionOffset = vec2(in_tc_attr[8], in_tc_attr[9]);\n"
												 "\n"
												 "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
												 "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
												 "\n"
												 "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
												 "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
												 "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
												 "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 + getTessellationEvaluationInLayoutString(m_primitiveType, m_spacing, m_winding, m_usePointMode) +
												 "\n"
												 "patch in highp vec2 in_te_positionScale;\n"
												 "patch in highp vec2 in_te_positionOffset;\n"
												 "\n"
												 "out highp vec3 out_te_tessCoord;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	out_te_tessCoord = gl_TessCoord;\n"
												 "	gl_Position = vec4(gl_TessCoord.xy*in_te_positionScale + in_te_positionOffset, 0.0, 1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "layout (location = 0) out mediump vec4 o_color;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	o_color = vec4(1.0);\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
		<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
		<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
		<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
		<< glu::TransformFeedbackVarying		("out_te_tessCoord")
		<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void PrimitiveDiscardCase::deinit (void)
{
	m_program.clear();
}

vector<float> PrimitiveDiscardCase::genAttributes (void)
{
	// Generate input attributes (tessellation levels, and position scale and
	// offset) for a number of primitives. Each primitive has a different
	// combination of tessellatio levels; each level is either a valid
	// value or an "invalid" value (negative or zero, chosen from
	// invalidTessLevelChoices).

	// \note The attributes are generated in such an order that all of the
	//		 valid attribute tuples come before the first invalid one both
	//		 in the result vector, and when scanning the resulting 2d grid
	//		 of primitives is scanned in y-major order. This makes
	//		 verification somewhat simpler.

	static const float	baseTessLevels[6]			= { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
	static const float	invalidTessLevelChoices[]	= { -0.42f, 0.0f };
	const int			numChoices					= 1 + DE_LENGTH_OF_ARRAY(invalidTessLevelChoices);
	float				choices[6][numChoices];
	vector<float>		result;

	for (int levelNdx = 0; levelNdx < 6; levelNdx++)
		for (int choiceNdx = 0; choiceNdx < numChoices; choiceNdx++)
			choices[levelNdx][choiceNdx] = choiceNdx == 0 ? baseTessLevels[levelNdx] : invalidTessLevelChoices[choiceNdx-1];

	{
		const int	numCols			= intPow(numChoices, 6/2); // sqrt(numChoices**6) == sqrt(number of primitives)
		const int	numRows			= numCols;
		int			index			= 0;
		int			i[6];
		// We could do this with some generic combination-generation function, but meh, it's not that bad.
		for (i[2] = 0; i[2] < numChoices; i[2]++) // First  outer
		for (i[3] = 0; i[3] < numChoices; i[3]++) // Second outer
		for (i[4] = 0; i[4] < numChoices; i[4]++) // Third  outer
		for (i[5] = 0; i[5] < numChoices; i[5]++) // Fourth outer
		for (i[0] = 0; i[0] < numChoices; i[0]++) // First  inner
		for (i[1] = 0; i[1] < numChoices; i[1]++) // Second inner
		{
			for (int j = 0; j < 6; j++)
				result.push_back(choices[j][i[j]]);

			{
				const int col = index % numCols;
				const int row = index / numCols;
				// Position scale.
				result.push_back((float)2.0f / (float)numCols);
				result.push_back((float)2.0f / (float)numRows);
				// Position offset.
				result.push_back((float)col / (float)numCols * 2.0f - 1.0f);
				result.push_back((float)row / (float)numRows * 2.0f - 1.0f);
			}

			index++;
		}
	}

	return result;
}

PrimitiveDiscardCase::IterateResult PrimitiveDiscardCase::iterate (void)
{
	typedef TransformFeedbackHandler<Vec3> TFHandler;

	TestLog&				log						= m_testCtx.getLog();
	const RenderContext&	renderCtx				= m_context.getRenderContext();
	const RandomViewport	viewport				(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&	gl						= renderCtx.getFunctions();
	const vector<float>		attributes				= genAttributes();
	const int				numAttribsPerPrimitive	= 6+2+2; // Tess levels, scale, offset.
	const int				numPrimitives			= (int)attributes.size() / numAttribsPerPrimitive;
	const deUint32			programGL				= m_program->getProgram();

	gl.useProgram(programGL);
	setViewport(gl, viewport);
	gl.patchParameteri(GL_PATCH_VERTICES, numAttribsPerPrimitive);

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	// Check the convenience assertion that all discarded patches come after the last non-discarded patch.
	{
		bool discardedPatchEncountered = false;
		for (int patchNdx = 0; patchNdx < numPrimitives; patchNdx++)
		{
			const bool discard = isPatchDiscarded(m_primitiveType, &attributes[numAttribsPerPrimitive*patchNdx + 2]);
			DE_ASSERT(discard || !discardedPatchEncountered);
			discardedPatchEncountered = discard;
		}
		DE_UNREF(discardedPatchEncountered);
	}

	{
		int numVerticesInDrawCall = 0;
		for (int patchNdx = 0; patchNdx < numPrimitives; patchNdx++)
			numVerticesInDrawCall += referenceVertexCount(m_primitiveType, m_spacing, m_usePointMode, &attributes[numAttribsPerPrimitive*patchNdx+0], &attributes[numAttribsPerPrimitive*patchNdx+2]);

		log << TestLog::Message << "Note: rendering " << numPrimitives << " patches; first patches have valid relevant outer levels, "
								<< "but later patches have one or more invalid (i.e. less than or equal to 0.0) relevant outer levels" << TestLog::EndMessage;

		{
			const TFHandler					tfHandler	(m_context.getRenderContext(), numVerticesInDrawCall);
			const glu::VertexArrayBinding	bindings[]	= { glu::va::Float("in_v_attr", 1, (int)attributes.size(), 0, &attributes[0]) };
			const TFHandler::Result			tfResult	= tfHandler.renderAndGetPrimitives(programGL, outputPrimitiveTypeGL(m_primitiveType, m_usePointMode),
																						   DE_LENGTH_OF_ARRAY(bindings), &bindings[0], (int)attributes.size());
			const tcu::Surface				pixels		= getPixels(renderCtx, viewport);

			log << TestLog::Image("RenderedImage", "Rendered image", pixels);

			if ((int)tfResult.varying.size() != numVerticesInDrawCall)
			{
				log << TestLog::Message << "Failure: expected " << numVerticesInDrawCall << " vertices from transform feedback, got " << tfResult.varying.size() << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Wrong number of tessellation coordinates");
				return STOP;
			}

			// Check that white pixels are found around every non-discarded
			// patch, and that only black pixels are found after the last
			// non-discarded patch.
			{
				int lastWhitePixelRow									= 0;
				int secondToLastWhitePixelRow							= 0;
				int	lastWhitePixelColumnOnSecondToLastWhitePixelRow		= 0;

				for (int patchNdx = 0; patchNdx < numPrimitives; patchNdx++)
				{
					const float* const	attr			= &attributes[numAttribsPerPrimitive*patchNdx];
					const bool			validLevels		= !isPatchDiscarded(m_primitiveType, &attr[2]);

					if (validLevels)
					{
						// Not a discarded patch; check that at least one white pixel is found in its area.

						const float* const	scale		= &attr[6];
						const float* const	offset		= &attr[8];
						const int			x0			= (int)((			offset[0] + 1.0f)*0.5f*(float)pixels.getWidth()) - 1;
						const int			x1			= (int)((scale[0] + offset[0] + 1.0f)*0.5f*(float)pixels.getWidth()) + 1;
						const int			y0			= (int)((			offset[1] + 1.0f)*0.5f*(float)pixels.getHeight()) - 1;
						const int			y1			= (int)((scale[1] + offset[1] + 1.0f)*0.5f*(float)pixels.getHeight()) + 1;
						const bool			isMSAA		= renderCtx.getRenderTarget().getNumSamples() > 1;
						bool				pixelOk		= false;

						if (y1 > lastWhitePixelRow)
						{
							secondToLastWhitePixelRow	= lastWhitePixelRow;
							lastWhitePixelRow			= y1;
						}
						lastWhitePixelColumnOnSecondToLastWhitePixelRow = x1;

						for (int y = y0; y <= y1 && !pixelOk; y++)
						for (int x = x0; x <= x1 && !pixelOk; x++)
						{
							if (!de::inBounds(x, 0, pixels.getWidth()) || !de::inBounds(y, 0, pixels.getHeight()))
								continue;

							if (isMSAA)
							{
								if (pixels.getPixel(x, y) != tcu::RGBA::black())
									pixelOk = true;
							}
							else
							{
								if (pixels.getPixel(x, y) == tcu::RGBA::white())
									pixelOk = true;
							}
						}

						if (!pixelOk)
						{
							log << TestLog::Message << "Failure: expected at least one " << (isMSAA ? "non-black" : "white") << " pixel in the rectangle "
													<< "[x0=" << x0 << ", y0=" << y0 << ", x1=" << x1 << ", y1=" << y1 << "]" << TestLog::EndMessage
								<< TestLog::Message << "Note: the rectangle approximately corresponds to the patch with these tessellation levels: "
													<< tessellationLevelsString(&attr[0], &attr[1]) << TestLog::EndMessage;
							m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
							return STOP;
						}
					}
					else
					{
						// First discarded primitive patch; the remaining are guaranteed to be discarded ones as well.

						for (int y = 0; y < pixels.getHeight(); y++)
						for (int x = 0; x < pixels.getWidth(); x++)
						{
							if (y > lastWhitePixelRow || (y > secondToLastWhitePixelRow && x > lastWhitePixelColumnOnSecondToLastWhitePixelRow))
							{
								if (pixels.getPixel(x, y) != tcu::RGBA::black())
								{
									log << TestLog::Message << "Failure: expected all pixels to be black in the area "
															<< (lastWhitePixelColumnOnSecondToLastWhitePixelRow < pixels.getWidth()-1
																	? string() + "y > " + de::toString(lastWhitePixelRow) + " || (y > " + de::toString(secondToLastWhitePixelRow)
																			   + " && x > " + de::toString(lastWhitePixelColumnOnSecondToLastWhitePixelRow) + ")"
																	: string() + "y > " + de::toString(lastWhitePixelRow))
															<< " (they all correspond to patches that should be discarded)" << TestLog::EndMessage
										<< TestLog::Message << "Note: pixel " << tcu::IVec2(x, y) << " isn't black" << TestLog::EndMessage;
									m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
									return STOP;
								}
							}
						}

						break;
					}
				}
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Case testing user-defined IO between TCS and TES
 *
 * TCS outputs various values to TES, including aggregates. The outputs
 * can be per-patch or per-vertex, and if per-vertex, they can also be in
 * an IO block. Per-vertex input array size can be left implicit (i.e.
 * inputArray[]) or explicit either by gl_MaxPatchVertices or an integer
 * literal whose value is queried from GL.
 *
 * The values output are generated in TCS and verified in TES against
 * similarly generated values. In case a verification of a value fails, the
 * index of the invalid value is output with TF.
 * As a sanity check, also the rendering result is verified (against pre-
 * rendered reference).
 *//*--------------------------------------------------------------------*/
class UserDefinedIOCase : public TestCase
{
public:
	enum IOType
	{
		IO_TYPE_PER_PATCH = 0,
		IO_TYPE_PER_PATCH_ARRAY,
		IO_TYPE_PER_PATCH_BLOCK,
		IO_TYPE_PER_PATCH_BLOCK_ARRAY,
		IO_TYPE_PER_VERTEX,
		IO_TYPE_PER_VERTEX_BLOCK,

		IO_TYPE_LAST
	};

	enum VertexIOArraySize
	{
		VERTEX_IO_ARRAY_SIZE_IMPLICIT = 0,
		VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN,		//!< Use gl_MaxPatchVertices as size for per-vertex input array.
		VERTEX_IO_ARRAY_SIZE_EXPLICIT_QUERY,				//!< Query GL_MAX_PATCH_VERTICES, and use that as size for per-vertex input array.

		VERTEX_IO_ARRAY_SIZE_LAST
	};

	enum TessControlOutArraySize
	{
		TESS_CONTROL_OUT_ARRAY_SIZE_IMPLICIT = 0,
		TESS_CONTROL_OUT_ARRAY_SIZE_LAYOUT,
		TESS_CONTROL_OUT_ARRAY_SIZE_QUERY,
		TESS_CONTROL_OUT_ARRAY_SIZE_SHADER_BUILTIN
	};

	UserDefinedIOCase (Context& context, const char* name, const char* description, TessPrimitiveType primType, IOType ioType, VertexIOArraySize vertexIOArraySize, TessControlOutArraySize tessControlOutArraySize, const char* referenceImagePath)
		: TestCase					(context, name, description)
		, m_primitiveType			(primType)
		, m_ioType					(ioType)
		, m_vertexIOArraySize		(vertexIOArraySize)
		, m_tessControlOutArraySize	(tessControlOutArraySize)
		, m_referenceImagePath		(referenceImagePath)
	{
	}

	void									init		(void);
	void									deinit		(void);
	IterateResult							iterate		(void);

private:
	typedef string (*BasicTypeVisitFunc)(const string& name, glu::DataType type, int indentationDepth); //!< See glslTraverseBasicTypes below.

	class TopLevelObject
	{
	public:
		virtual			~TopLevelObject					(void) {}

		virtual string	name							(void) const = 0;
		virtual string	declare							(void) const = 0;
		virtual string	declareArray					(const string& arraySizeExpr) const = 0;
		virtual string	glslTraverseBasicTypeArray		(int numArrayElements, //!< If negative, traverse just array[gl_InvocationID], not all indices.
														 int indentationDepth,
														 BasicTypeVisitFunc) const = 0;
		virtual string	glslTraverseBasicType			(int indentationDepth,
														 BasicTypeVisitFunc) const = 0;
		virtual int		numBasicSubobjectsInElementType	(void) const = 0;
		virtual string	basicSubobjectAtIndex			(int index, int arraySize) const = 0;
	};

	class Variable : public TopLevelObject
	{
	public:
		Variable (const string& name_, const glu::VarType& type, bool isArray)
			: m_name		(name_)
			, m_type		(type)
			, m_isArray		(isArray)
		{
			DE_ASSERT(!type.isArrayType());
		}

		string	name								(void) const { return m_name; }
		string	declare								(void) const;
		string	declareArray						(const string& arraySizeExpr) const;
		string	glslTraverseBasicTypeArray			(int numArrayElements, int indentationDepth, BasicTypeVisitFunc) const;
		string	glslTraverseBasicType				(int indentationDepth, BasicTypeVisitFunc) const;
		int		numBasicSubobjectsInElementType		(void) const;
		string	basicSubobjectAtIndex				(int index, int arraySize) const;

	private:
		string			m_name;
		glu::VarType	m_type; //!< If this Variable is an array element, m_type is the element type; otherwise just the variable type.
		const bool		m_isArray;
	};

	class IOBlock : public TopLevelObject
	{
	public:
		struct Member
		{
			string			name;
			glu::VarType	type;
			Member (const string& n, const glu::VarType& t) : name(n), type(t) {}
		};

		IOBlock (const string& blockName, const string& interfaceName, const vector<Member>& members)
			: m_blockName		(blockName)
			, m_interfaceName	(interfaceName)
			, m_members			(members)
		{
		}

		string	name								(void) const { return m_interfaceName; }
		string	declare								(void) const;
		string	declareArray						(const string& arraySizeExpr) const;
		string	glslTraverseBasicTypeArray			(int numArrayElements, int indentationDepth, BasicTypeVisitFunc) const;
		string	glslTraverseBasicType				(int indentationDepth, BasicTypeVisitFunc) const;
		int		numBasicSubobjectsInElementType		(void) const;
		string	basicSubobjectAtIndex				(int index, int arraySize) const;

	private:
		string			m_blockName;
		string			m_interfaceName;
		vector<Member>	m_members;
	};

	static string							glslTraverseBasicTypes				(const string&			rootName,
																				 const glu::VarType&	rootType,
																				 int					arrayNestingDepth,
																				 int					indentationDepth,
																				 BasicTypeVisitFunc		visit);

	static string							glslAssignBasicTypeObject			(const string& name, glu::DataType, int indentationDepth);
	static string							glslCheckBasicTypeObject			(const string& name, glu::DataType, int indentationDepth);
	static int								numBasicSubobjectsInElementType		(const vector<SharedPtr<TopLevelObject> >&);
	static string							basicSubobjectAtIndex				(int index, const vector<SharedPtr<TopLevelObject> >&, int topLevelArraySizes);

	enum
	{
		RENDER_SIZE = 256
	};
	enum
	{
		NUM_OUTPUT_VERTICES = 5
	};
	enum
	{
		NUM_PER_PATCH_ARRAY_ELEMS = 3
	};
	enum
	{
		NUM_PER_PATCH_BLOCKS = 2
	};

	const TessPrimitiveType					m_primitiveType;
	const IOType							m_ioType;
	const VertexIOArraySize					m_vertexIOArraySize;
	const TessControlOutArraySize			m_tessControlOutArraySize;
	const string							m_referenceImagePath;

	vector<glu::StructType>					m_structTypes;
	vector<SharedPtr<TopLevelObject> >		m_tcsOutputs;
	vector<SharedPtr<TopLevelObject> >		m_tesInputs;

	SharedPtr<const glu::ShaderProgram>		m_program;
};

/*--------------------------------------------------------------------*//*!
 * \brief Generate GLSL code to traverse (possibly aggregate) object
 *
 * Generates a string that represents GLSL code that traverses the
 * basic-type subobjects in a rootType-typed object named rootName. Arrays
 * are traversed with loops and struct members are each traversed
 * separately. The code for each basic-type subobject is generated with
 * the function given as the 'visit' argument.
 *//*--------------------------------------------------------------------*/
string UserDefinedIOCase::glslTraverseBasicTypes (const string&			rootName,
												  const glu::VarType&	rootType,
												  int					arrayNestingDepth,
												  int					indentationDepth,
												  BasicTypeVisitFunc	visit)
{
	if (rootType.isBasicType())
		return visit(rootName, rootType.getBasicType(), indentationDepth);
	else if (rootType.isArrayType())
	{
		const string indentation	= string(indentationDepth, '\t');
		const string loopIndexName	= "i" + de::toString(arrayNestingDepth);
		const string arrayLength	= de::toString(rootType.getArraySize());
		return indentation + "for (int " + loopIndexName + " = 0; " + loopIndexName + " < " + de::toString(rootType.getArraySize()) + "; " + loopIndexName + "++)\n" +
			   indentation + "{\n" +
			   glslTraverseBasicTypes(rootName + "[" + loopIndexName + "]", rootType.getElementType(), arrayNestingDepth+1, indentationDepth+1, visit) +
			   indentation + "}\n";
	}
	else if (rootType.isStructType())
	{
		const glu::StructType&	structType = *rootType.getStructPtr();
		const int				numMembers = structType.getNumMembers();
		string					result;

		for (int membNdx = 0; membNdx < numMembers; membNdx++)
		{
			const glu::StructMember& member = structType.getMember(membNdx);
			result += glslTraverseBasicTypes(rootName + "." + member.getName(), member.getType(), arrayNestingDepth, indentationDepth, visit);
		}

		return result;
	}
	else
	{
		DE_ASSERT(false);
		return DE_NULL;
	}
}

string UserDefinedIOCase::Variable::declare (void) const
{
	DE_ASSERT(!m_isArray);
	return de::toString(glu::declare(m_type, m_name)) + ";\n";
}

string UserDefinedIOCase::Variable::declareArray (const string& sizeExpr) const
{
	DE_ASSERT(m_isArray);
	return de::toString(glu::declare(m_type, m_name)) + "[" + sizeExpr + "];\n";
}

string UserDefinedIOCase::IOBlock::declare (void) const
{
	std::ostringstream buf;

	buf << m_blockName << "\n"
		<< "{\n";

	for (int i = 0; i < (int)m_members.size(); i++)
		buf << "\t" << glu::declare(m_members[i].type, m_members[i].name) << ";\n";

	buf << "} " << m_interfaceName << ";\n";
	return buf.str();
}

string UserDefinedIOCase::IOBlock::declareArray (const string& sizeExpr) const
{
	std::ostringstream buf;

	buf << m_blockName << "\n"
		<< "{\n";

	for (int i = 0; i < (int)m_members.size(); i++)
		buf << "\t" << glu::declare(m_members[i].type, m_members[i].name) << ";\n";

	buf << "} " << m_interfaceName << "[" << sizeExpr << "];\n";
	return buf.str();
}

string UserDefinedIOCase::Variable::glslTraverseBasicTypeArray (int numArrayElements, int indentationDepth, BasicTypeVisitFunc visit) const
{
	DE_ASSERT(m_isArray);

	const bool				traverseAsArray		= numArrayElements >= 0;
	const string			traversedName		= m_name + (!traverseAsArray ? "[gl_InvocationID]" : "");
	const glu::VarType		type				= traverseAsArray ? glu::VarType(m_type, numArrayElements) : m_type;

	return UserDefinedIOCase::glslTraverseBasicTypes(traversedName, type, 0, indentationDepth, visit);
}

string UserDefinedIOCase::Variable::glslTraverseBasicType (int indentationDepth, BasicTypeVisitFunc visit) const
{
	DE_ASSERT(!m_isArray);

	return UserDefinedIOCase::glslTraverseBasicTypes(m_name, m_type, 0, indentationDepth, visit);
}

string UserDefinedIOCase::IOBlock::glslTraverseBasicTypeArray (int numArrayElements, int indentationDepth, BasicTypeVisitFunc visit) const
{
	if (numArrayElements >= 0)
	{
		const string	indentation			= string(indentationDepth, '\t');
		string			result				= indentation + "for (int i0 = 0; i0 < " + de::toString(numArrayElements) + "; i0++)\n" +
											  indentation + "{\n";
		for (int i = 0; i < (int)m_members.size(); i++)
			result += UserDefinedIOCase::glslTraverseBasicTypes(m_interfaceName + "[i0]." + m_members[i].name, m_members[i].type, 1, indentationDepth+1, visit);
		result += indentation + "}\n";
		return result;
	}
	else
	{
		string result;
		for (int i = 0; i < (int)m_members.size(); i++)
			result += UserDefinedIOCase::glslTraverseBasicTypes(m_interfaceName + "[gl_InvocationID]." + m_members[i].name, m_members[i].type, 0, indentationDepth, visit);
		return result;
	}
}


string UserDefinedIOCase::IOBlock::glslTraverseBasicType (int indentationDepth, BasicTypeVisitFunc visit) const
{
	string result;
	for (int i = 0; i < (int)m_members.size(); i++)
		result += UserDefinedIOCase::glslTraverseBasicTypes(m_interfaceName + "." + m_members[i].name, m_members[i].type, 0, indentationDepth, visit);
	return result;
}

int UserDefinedIOCase::Variable::numBasicSubobjectsInElementType (void) const
{
	return numBasicSubobjects(m_type);
}

int UserDefinedIOCase::IOBlock::numBasicSubobjectsInElementType (void) const
{
	int result = 0;
	for (int i = 0; i < (int)m_members.size(); i++)
		result += numBasicSubobjects(m_members[i].type);
	return result;
}

string UserDefinedIOCase::Variable::basicSubobjectAtIndex (int subobjectIndex, int arraySize) const
{
	const glu::VarType	type			= m_isArray ? glu::VarType(m_type, arraySize) : m_type;
	int					currentIndex	= 0;

	for (glu::BasicTypeIterator basicIt = glu::BasicTypeIterator::begin(&type);
		 basicIt != glu::BasicTypeIterator::end(&type);
		 ++basicIt)
	{
		if (currentIndex == subobjectIndex)
			return m_name + de::toString(glu::TypeAccessFormat(type, basicIt.getPath()));
		currentIndex++;
	}
	DE_ASSERT(false);
	return DE_NULL;
}

string UserDefinedIOCase::IOBlock::basicSubobjectAtIndex (int subobjectIndex, int arraySize) const
{
	int currentIndex = 0;
	for (int arrayNdx = 0; arrayNdx < arraySize; arrayNdx++)
	{
		for (int memberNdx = 0; memberNdx < (int)m_members.size(); memberNdx++)
		{
			const glu::VarType& membType = m_members[memberNdx].type;
			for (glu::BasicTypeIterator basicIt = glu::BasicTypeIterator::begin(&membType);
				 basicIt != glu::BasicTypeIterator::end(&membType);
				 ++basicIt)
			{
				if (currentIndex == subobjectIndex)
					return m_interfaceName + "[" + de::toString(arrayNdx) + "]." + m_members[memberNdx].name + de::toString(glu::TypeAccessFormat(membType, basicIt.getPath()));
				currentIndex++;
			}
		}
	}
	DE_ASSERT(false);
	return DE_NULL;
}

// Used as the 'visit' argument for glslTraverseBasicTypes.
string UserDefinedIOCase::glslAssignBasicTypeObject (const string& name, glu::DataType type, int indentationDepth)
{
	const int		scalarSize		= glu::getDataTypeScalarSize(type);
	const string	indentation		= string(indentationDepth, '\t');
	string			result;

	result += indentation + name + " = ";

	if (type != glu::TYPE_FLOAT)
		result += string() + glu::getDataTypeName(type) + "(";
	for (int i = 0; i < scalarSize; i++)
		result += (i > 0 ? ", v+" + de::floatToString(0.8f*(float)i, 1)
						 : "v");
	if (type != glu::TYPE_FLOAT)
		result += ")";
	result += ";\n" +
			  indentation + "v += 0.4;\n";
	return result;
}

// Used as the 'visit' argument for glslTraverseBasicTypes.
string UserDefinedIOCase::glslCheckBasicTypeObject (const string& name, glu::DataType type, int indentationDepth)
{
	const int		scalarSize		= glu::getDataTypeScalarSize(type);
	const string	indentation		= string(indentationDepth, '\t');
	string			result;

	result += indentation + "allOk = allOk && compare_" + glu::getDataTypeName(type) + "(" + name + ", ";

	if (type != glu::TYPE_FLOAT)
		result += string() + glu::getDataTypeName(type) + "(";
	for (int i = 0; i < scalarSize; i++)
		result += (i > 0 ? ", v+" + de::floatToString(0.8f*(float)i, 1)
						 : "v");
	if (type != glu::TYPE_FLOAT)
		result += ")";
	result += ");\n" +
			  indentation + "v += 0.4;\n" +
			  indentation + "if (allOk) firstFailedInputIndex++;\n";

	return result;
}

int UserDefinedIOCase::numBasicSubobjectsInElementType (const vector<SharedPtr<TopLevelObject> >& objects)
{
	int result = 0;
	for (int i = 0; i < (int)objects.size(); i++)
		result += objects[i]->numBasicSubobjectsInElementType();
	return result;
}

string UserDefinedIOCase::basicSubobjectAtIndex (int subobjectIndex, const vector<SharedPtr<TopLevelObject> >& objects, int topLevelArraySize)
{
	int currentIndex	= 0;
	int objectIndex		= 0;
	for (; currentIndex < subobjectIndex; objectIndex++)
		currentIndex += objects[objectIndex]->numBasicSubobjectsInElementType() * topLevelArraySize;
	if (currentIndex > subobjectIndex)
	{
		objectIndex--;
		currentIndex -= objects[objectIndex]->numBasicSubobjectsInElementType() * topLevelArraySize;
	}

	return objects[objectIndex]->basicSubobjectAtIndex(subobjectIndex - currentIndex, topLevelArraySize);
}

void UserDefinedIOCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	const bool			isPerPatchIO				= m_ioType == IO_TYPE_PER_PATCH				||
													  m_ioType == IO_TYPE_PER_PATCH_ARRAY		||
													  m_ioType == IO_TYPE_PER_PATCH_BLOCK		||
													  m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY;

	const bool			isExplicitVertexArraySize	= m_vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN ||
													  m_vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_QUERY;

	const string		vertexAttrArrayInputSize	= m_vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_IMPLICIT					? ""
													: m_vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN	? "gl_MaxPatchVertices"
													: m_vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_QUERY			? de::toString(m_context.getContextInfo().getInt(GL_MAX_PATCH_VERTICES))
													: DE_NULL;

	const char* const	maybePatch					= isPerPatchIO ? "patch " : "";
	const string		outMaybePatch				= string() + maybePatch + "out ";
	const string		inMaybePatch				= string() + maybePatch + "in ";
	const bool			useBlock					= m_ioType == IO_TYPE_PER_VERTEX_BLOCK		||
													  m_ioType == IO_TYPE_PER_PATCH_BLOCK		||
													  m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY;

	string				tcsDeclarations;
	string				tcsStatements;

	string				tesDeclarations;
	string				tesStatements;

	{
		m_structTypes.push_back(glu::StructType("S"));

		const glu::VarType	highpFloat		(glu::TYPE_FLOAT, glu::PRECISION_HIGHP);
		glu::StructType&	structType		= m_structTypes.back();
		const glu::VarType	structVarType	(&structType);
		bool				usedStruct		= false;

		structType.addMember("x", glu::VarType(glu::TYPE_INT, glu::PRECISION_HIGHP));
		structType.addMember("y", glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP));

		if (useBlock)
		{
			// It is illegal to have a structure containing an array as an output variable
			structType.addMember("z", glu::VarType(highpFloat, 2));
		}

		if (useBlock)
		{
			const bool				useLightweightBlock = (m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY); // use leaner block to make sure it is not larger than allowed (per-patch storage is very limited)
			vector<IOBlock::Member>	blockMembers;

			if (!useLightweightBlock)
				blockMembers.push_back(IOBlock::Member("blockS",	structVarType));

			blockMembers.push_back(IOBlock::Member("blockFa",	glu::VarType(highpFloat, 3)));
			blockMembers.push_back(IOBlock::Member("blockSa",	glu::VarType(structVarType, 2)));
			blockMembers.push_back(IOBlock::Member("blockF",	highpFloat));

			m_tcsOutputs.push_back	(SharedPtr<TopLevelObject>(new IOBlock("TheBlock", "tcBlock", blockMembers)));
			m_tesInputs.push_back	(SharedPtr<TopLevelObject>(new IOBlock("TheBlock", "teBlock", blockMembers)));

			usedStruct = true;
		}
		else
		{
			const Variable var0("in_te_s", structVarType,	m_ioType != IO_TYPE_PER_PATCH);
			const Variable var1("in_te_f", highpFloat,		m_ioType != IO_TYPE_PER_PATCH);

			if (m_ioType != IO_TYPE_PER_PATCH_ARRAY)
			{
				// Arrays of structures are disallowed, add struct cases only if not arrayed variable
				m_tcsOutputs.push_back	(SharedPtr<TopLevelObject>(new Variable(var0)));
				m_tesInputs.push_back	(SharedPtr<TopLevelObject>(new Variable(var0)));

				usedStruct = true;
			}

			m_tcsOutputs.push_back	(SharedPtr<TopLevelObject>(new Variable(var1)));
			m_tesInputs.push_back	(SharedPtr<TopLevelObject>(new Variable(var1)));
		}

		tcsDeclarations += "in " + Variable("in_tc_attr", highpFloat, true).declareArray(vertexAttrArrayInputSize);

		if (usedStruct)
			tcsDeclarations += de::toString(glu::declare(structType)) + ";\n";

		tcsStatements += "\t{\n"
						 "\t\thighp float v = 1.3;\n";

		for (int tcsOutputNdx = 0; tcsOutputNdx < (int)m_tcsOutputs.size(); tcsOutputNdx++)
		{
			const TopLevelObject&	output		= *m_tcsOutputs[tcsOutputNdx];
			const int				numElements	= !isPerPatchIO								? -1	//!< \note -1 means indexing with gl_InstanceID
												: m_ioType == IO_TYPE_PER_PATCH				? 1
												: m_ioType == IO_TYPE_PER_PATCH_ARRAY		? NUM_PER_PATCH_ARRAY_ELEMS
												: m_ioType == IO_TYPE_PER_PATCH_BLOCK		? 1
												: m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? NUM_PER_PATCH_BLOCKS
												: -2;
			const bool				isArray		= (numElements != 1);

			DE_ASSERT(numElements != -2);

			if (isArray)
			{
				tcsDeclarations += outMaybePatch + output.declareArray(m_ioType == IO_TYPE_PER_PATCH_ARRAY											? de::toString(int(NUM_PER_PATCH_ARRAY_ELEMS))
																	   : m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY									? de::toString(int(NUM_PER_PATCH_BLOCKS))
																	   : m_tessControlOutArraySize == TESS_CONTROL_OUT_ARRAY_SIZE_LAYOUT			? de::toString(int(NUM_OUTPUT_VERTICES))
																	   : m_tessControlOutArraySize == TESS_CONTROL_OUT_ARRAY_SIZE_QUERY				? de::toString(m_context.getContextInfo().getInt(GL_MAX_PATCH_VERTICES))
																	   : m_tessControlOutArraySize == TESS_CONTROL_OUT_ARRAY_SIZE_SHADER_BUILTIN	? "gl_MaxPatchVertices"
																	   : "");
			}
			else
				tcsDeclarations += outMaybePatch + output.declare();

			if (!isPerPatchIO)
				tcsStatements += "\t\tv += float(gl_InvocationID)*" + de::floatToString(0.4f * (float)output.numBasicSubobjectsInElementType(), 1) + ";\n";

			tcsStatements += "\n\t\t// Assign values to output " + output.name() + "\n";
			if (isArray)
				tcsStatements += output.glslTraverseBasicTypeArray(numElements, 2, glslAssignBasicTypeObject);
			else
				tcsStatements += output.glslTraverseBasicType(2, glslAssignBasicTypeObject);

			if (!isPerPatchIO)
				tcsStatements += "\t\tv += float(" + de::toString(int(NUM_OUTPUT_VERTICES)) + "-gl_InvocationID-1)*" + de::floatToString(0.4f * (float)output.numBasicSubobjectsInElementType(), 1) + ";\n";
		}
		tcsStatements += "\t}\n";

		if (usedStruct)
			tesDeclarations += de::toString(glu::declare(structType)) + ";\n";

		tesStatements += "\tbool allOk = true;\n"
						 "\thighp uint firstFailedInputIndex = 0u;\n"
						 "\t{\n"
						 "\t\thighp float v = 1.3;\n";
		for (int tesInputNdx = 0; tesInputNdx < (int)m_tesInputs.size(); tesInputNdx++)
		{
			const TopLevelObject&	input		= *m_tesInputs[tesInputNdx];
			const int				numElements	= !isPerPatchIO								? (int)NUM_OUTPUT_VERTICES
												: m_ioType == IO_TYPE_PER_PATCH				? 1
												: m_ioType == IO_TYPE_PER_PATCH_BLOCK		? 1
												: m_ioType == IO_TYPE_PER_PATCH_ARRAY		? NUM_PER_PATCH_ARRAY_ELEMS
												: m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? NUM_PER_PATCH_BLOCKS
												: -2;
			const bool				isArray		= (numElements != 1);

			DE_ASSERT(numElements != -2);

			if (isArray)
				tesDeclarations += inMaybePatch + input.declareArray(m_ioType == IO_TYPE_PER_PATCH_ARRAY			? de::toString(int(NUM_PER_PATCH_ARRAY_ELEMS))
																	 : m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? de::toString(int(NUM_PER_PATCH_BLOCKS))
																	 : isExplicitVertexArraySize					? de::toString(vertexAttrArrayInputSize)
																	 : "");
			else
				tesDeclarations += inMaybePatch + input.declare();

			tesStatements += "\n\t\t// Check values in input " + input.name() + "\n";
			if (isArray)
				tesStatements += input.glslTraverseBasicTypeArray(numElements, 2, glslCheckBasicTypeObject);
			else
				tesStatements += input.glslTraverseBasicType(2, glslCheckBasicTypeObject);
		}
		tesStatements += "\t}\n";
	}

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "in highp float in_v_attr;\n"
												 "out highp float in_tc_attr;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	in_tc_attr = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 "layout (vertices = " + de::toString(int(NUM_OUTPUT_VERTICES)) + ") out;\n"
												 "\n"
												 + tcsDeclarations +
												 "\n"
												 "patch out highp vec2 in_te_positionScale;\n"
												 "patch out highp vec2 in_te_positionOffset;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 + tcsStatements +
												 "\n"
												 "	in_te_positionScale  = vec2(in_tc_attr[6], in_tc_attr[7]);\n"
												 "	in_te_positionOffset = vec2(in_tc_attr[8], in_tc_attr[9]);\n"
												 "\n"
												 "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
												 "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
												 "\n"
												 "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
												 "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
												 "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
												 "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 + getTessellationEvaluationInLayoutString(m_primitiveType) +
												 "\n"
												 + tesDeclarations +
												 "\n"
												 "patch in highp vec2 in_te_positionScale;\n"
												 "patch in highp vec2 in_te_positionOffset;\n"
												 "\n"
												 "out highp vec4 in_f_color;\n"
												 "// Will contain the index of the first incorrect input,\n"
												 "// or the number of inputs if all are correct\n"
												 "flat out highp uint out_te_firstFailedInputIndex;\n"
												 "\n"
												 "bool compare_int   (int   a, int   b) { return a == b; }\n"
												 "bool compare_float (float a, float b) { return abs(a - b) < 0.01f; }\n"
												 "bool compare_vec4  (vec4  a, vec4  b) { return all(lessThan(abs(a - b), vec4(0.01f))); }\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 + tesStatements +
												 "\n"
												 "	gl_Position = vec4(gl_TessCoord.xy*in_te_positionScale + in_te_positionOffset, 0.0, 1.0);\n"
												 "	in_f_color = allOk ? vec4(0.0, 1.0, 0.0, 1.0)\n"
												 "	                   : vec4(1.0, 0.0, 0.0, 1.0);\n"
												 "	out_te_firstFailedInputIndex = firstFailedInputIndex;\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "layout (location = 0) out mediump vec4 o_color;\n"
												 "\n"
												 "in highp vec4 in_f_color;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
		<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
		<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
		<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))
		<< glu::TransformFeedbackVarying		("out_te_firstFailedInputIndex")
		<< glu::TransformFeedbackMode			(GL_INTERLEAVED_ATTRIBS)));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void UserDefinedIOCase::deinit (void)
{
	m_program.clear();
}

UserDefinedIOCase::IterateResult UserDefinedIOCase::iterate (void)
{
	typedef TransformFeedbackHandler<deUint32> TFHandler;

	TestLog&				log						= m_testCtx.getLog();
	const RenderContext&	renderCtx				= m_context.getRenderContext();
	const RandomViewport	viewport				(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&	gl						= renderCtx.getFunctions();
	static const float		attributes[6+2+2]		= { /* inner */ 3.0f, 4.0f, /* outer */ 5.0f, 6.0f, 7.0f, 8.0f, /* pos. scale */ 1.2f, 1.3f, /* pos. offset */ -0.3f, -0.4f };
	const deUint32			programGL				= m_program->getProgram();
	const int				numVertices				= referenceVertexCount(m_primitiveType, SPACINGMODE_EQUAL, false, &attributes[0], &attributes[2]);
	const TFHandler			tfHandler				(renderCtx, numVertices);
	tcu::ResultCollector	result;

	gl.useProgram(programGL);
	setViewport(gl, viewport);
	gl.patchParameteri(GL_PATCH_VERTICES, DE_LENGTH_OF_ARRAY(attributes));

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	{
		const glu::VertexArrayBinding	bindings[]	= { glu::va::Float("in_v_attr", 1, DE_LENGTH_OF_ARRAY(attributes), 0, &attributes[0]) };
		const TFHandler::Result			tfResult	= tfHandler.renderAndGetPrimitives(programGL, outputPrimitiveTypeGL(m_primitiveType, false),
																					   DE_LENGTH_OF_ARRAY(bindings), &bindings[0], DE_LENGTH_OF_ARRAY(attributes));

		{
			const tcu::Surface			pixels		= getPixels(renderCtx, viewport);
			const tcu::TextureLevel		reference	= getPNG(m_testCtx.getArchive(), m_referenceImagePath.c_str());
			const bool					success		= tcu::fuzzyCompare(log, "ImageComparison", "Image Comparison", reference.getAccess(), pixels.getAccess(), 0.02f, tcu::COMPARE_LOG_RESULT);

			if (!success)
				result.fail("Image comparison failed");
		}

		if ((int)tfResult.varying.size() != numVertices)
		{
			log << TestLog::Message << "Failure: transform feedback returned " << tfResult.varying.size() << " vertices; expected " << numVertices << TestLog::EndMessage;
			result.fail("Wrong number of vertices");
		}
		else
		{
			const int topLevelArraySize		= (m_ioType == IO_TYPE_PER_PATCH				? 1
											 : m_ioType == IO_TYPE_PER_PATCH_ARRAY			? NUM_PER_PATCH_ARRAY_ELEMS
											 : m_ioType == IO_TYPE_PER_PATCH_BLOCK			? 1
											 : m_ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? NUM_PER_PATCH_BLOCKS
											 : (int)NUM_OUTPUT_VERTICES);
			const int numTEInputs			= numBasicSubobjectsInElementType(m_tesInputs) * topLevelArraySize;

			for (int vertexNdx = 0; vertexNdx < (int)numVertices; vertexNdx++)
			{
				if (tfResult.varying[vertexNdx] > (deUint32)numTEInputs)
				{
					log << TestLog::Message << "Failure: out_te_firstFailedInputIndex has value " << tfResult.varying[vertexNdx]
											<< ", should be in range [0, " << numTEInputs << "]" << TestLog::EndMessage;
					result.fail("Invalid transform feedback output");
				}
				else if (tfResult.varying[vertexNdx] != (deUint32)numTEInputs)
				{
					log << TestLog::Message << "Failure: in tessellation evaluation shader, check for input "
											<< basicSubobjectAtIndex(tfResult.varying[vertexNdx], m_tesInputs, topLevelArraySize) << " failed" << TestLog::EndMessage;
					result.fail("Invalid input value in tessellation evaluation shader");
				}
			}
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Pass gl_Position between VS and TCS, or between TCS and TES.
 *
 * In TCS gl_Position is in the gl_out[] block and in TES in the gl_in[]
 * block, and has no special semantics in those. Arbitrary vec4 data can
 * thus be passed there.
 *//*--------------------------------------------------------------------*/
class GLPositionCase : public TestCase
{
public:
	enum CaseType
	{
		CASETYPE_VS_TO_TCS = 0,
		CASETYPE_TCS_TO_TES,
		CASETYPE_VS_TO_TCS_TO_TES,

		CASETYPE_LAST
	};

	GLPositionCase (Context& context, const char* name, const char* description, CaseType caseType, const char* referenceImagePath)
		: TestCase				(context, name, description)
		, m_caseType			(caseType)
		, m_referenceImagePath	(referenceImagePath)
	{
	}

	void									init				(void);
	void									deinit				(void);
	IterateResult							iterate				(void);

	static const char*						getCaseTypeName		(CaseType type);

private:
	static const int						RENDER_SIZE = 256;

	const CaseType							m_caseType;
	const string							m_referenceImagePath;

	SharedPtr<const glu::ShaderProgram>		m_program;
};

const char* GLPositionCase::getCaseTypeName (CaseType type)
{
	switch (type)
	{
		case CASETYPE_VS_TO_TCS:			return "gl_position_vs_to_tcs";
		case CASETYPE_TCS_TO_TES:			return "gl_position_tcs_to_tes";
		case CASETYPE_VS_TO_TCS_TO_TES:		return "gl_position_vs_to_tcs_to_tes";
		default:
			DE_ASSERT(false); return DE_NULL;
	}
}

void GLPositionCase::init (void)
{
	checkTessellationSupport(m_context);
	checkRenderTargetSize(m_context.getRenderTarget(), RENDER_SIZE);

	const bool		vsToTCS		= m_caseType == CASETYPE_VS_TO_TCS		|| m_caseType == CASETYPE_VS_TO_TCS_TO_TES;
	const bool		tcsToTES	= m_caseType == CASETYPE_TCS_TO_TES		|| m_caseType == CASETYPE_VS_TO_TCS_TO_TES;

	const string	tesIn0		= tcsToTES ? "gl_in[0].gl_Position" : "in_te_attr[0]";
	const string	tesIn1		= tcsToTES ? "gl_in[1].gl_Position" : "in_te_attr[1]";
	const string	tesIn2		= tcsToTES ? "gl_in[2].gl_Position" : "in_te_attr[2]";

	std::string vertexShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "in highp vec4 in_v_attr;\n"
												 + string(!vsToTCS ? "out highp vec4 in_tc_attr;\n" : "") +
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	" + (vsToTCS ? "gl_Position" : "in_tc_attr") + " = in_v_attr;\n"
												 "}\n");
	std::string tessellationControlTemplate		("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 "layout (vertices = 3) out;\n"
												 "\n"
												 + string(!vsToTCS ? "in highp vec4 in_tc_attr[];\n" : "") +
												 "\n"
												 + (!tcsToTES ? "out highp vec4 in_te_attr[];\n" : "") +
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	" + (tcsToTES ? "gl_out[gl_InvocationID].gl_Position" : "in_te_attr[gl_InvocationID]") + " = "
													  + (vsToTCS ? "gl_in[gl_InvocationID].gl_Position" : "in_tc_attr[gl_InvocationID]") + ";\n"
												 "\n"
												 "	gl_TessLevelInner[0] = 2.0;\n"
												 "	gl_TessLevelInner[1] = 3.0;\n"
												 "\n"
												 "	gl_TessLevelOuter[0] = 4.0;\n"
												 "	gl_TessLevelOuter[1] = 5.0;\n"
												 "	gl_TessLevelOuter[2] = 6.0;\n"
												 "	gl_TessLevelOuter[3] = 7.0;\n"
												 "}\n");
	std::string tessellationEvaluationTemplate	("${GLSL_VERSION_DECL}\n"
												 "${TESSELLATION_SHADER_REQUIRE}\n"
												 "\n"
												 + getTessellationEvaluationInLayoutString(TESSPRIMITIVETYPE_TRIANGLES) +
												 "\n"
												 + (!tcsToTES ? "in highp vec4 in_te_attr[];\n" : "") +
												 "\n"
												 "out highp vec4 in_f_color;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	highp vec2 xy = gl_TessCoord.x * " + tesIn0 + ".xy\n"
												 "	              + gl_TessCoord.y * " + tesIn1 + ".xy\n"
												 "	              + gl_TessCoord.z * " + tesIn2 + ".xy;\n"
												 "	gl_Position = vec4(xy, 0.0, 1.0);\n"
												 "	in_f_color = vec4(" + tesIn0 + ".z + " + tesIn1 + ".w,\n"
												 "	                  " + tesIn2 + ".z + " + tesIn0 + ".w,\n"
												 "	                  " + tesIn1 + ".z + " + tesIn2 + ".w,\n"
												 "	                  1.0);\n"
												 "}\n");
	std::string fragmentShaderTemplate			("${GLSL_VERSION_DECL}\n"
												 "\n"
												 "layout (location = 0) out mediump vec4 o_color;\n"
												 "\n"
												 "in highp vec4 in_f_color;\n"
												 "\n"
												 "void main (void)\n"
												 "{\n"
												 "	o_color = in_f_color;\n"
												 "}\n");

	m_program = SharedPtr<const ShaderProgram>(new ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource					(specializeShader(m_context, vertexShaderTemplate.c_str()))
		<< glu::TessellationControlSource		(specializeShader(m_context, tessellationControlTemplate.c_str()))
		<< glu::TessellationEvaluationSource	(specializeShader(m_context, tessellationEvaluationTemplate.c_str()))
		<< glu::FragmentSource					(specializeShader(m_context, fragmentShaderTemplate.c_str()))));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void GLPositionCase::deinit (void)
{
	m_program.clear();
}

GLPositionCase::IterateResult GLPositionCase::iterate (void)
{
	TestLog&				log						= m_testCtx.getLog();
	const RenderContext&	renderCtx				= m_context.getRenderContext();
	const RandomViewport	viewport				(renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()));
	const glw::Functions&	gl						= renderCtx.getFunctions();
	const deUint32			programGL				= m_program->getProgram();

	static const float attributes[3*4] =
	{
		-0.8f, -0.7f, 0.1f, 0.7f,
		-0.5f,  0.4f, 0.2f, 0.5f,
		 0.3f,  0.2f, 0.3f, 0.45f
	};

	gl.useProgram(programGL);
	setViewport(gl, viewport);
	gl.patchParameteri(GL_PATCH_VERTICES, 3);

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	log << TestLog::Message << "Note: input data for in_v_attr:\n" << arrayStr(attributes, 4) << TestLog::EndMessage;

	{
		const glu::VertexArrayBinding bindings[] = { glu::va::Float("in_v_attr", 4, 3, 0, &attributes[0]) };
		glu::draw(renderCtx, programGL, DE_LENGTH_OF_ARRAY(bindings), &bindings[0], glu::pr::Patches(3));

		{
			const tcu::Surface			pixels		= getPixels(renderCtx, viewport);
			const tcu::TextureLevel		reference	= getPNG(m_testCtx.getArchive(), m_referenceImagePath.c_str());
			const bool					success		= tcu::fuzzyCompare(log, "ImageComparison", "Image Comparison", reference.getAccess(), pixels.getAccess(), 0.02f, tcu::COMPARE_LOG_RESULT);

			if (!success)
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

class LimitQueryCase : public TestCase
{
public:
						LimitQueryCase	(Context& context, const char* name, const char* desc, glw::GLenum target, int minValue);
private:
	IterateResult		iterate			(void);

	const glw::GLenum	m_target;
	const int			m_minValue;
};

LimitQueryCase::LimitQueryCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minValue)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_minValue		(minValue)
{
}

LimitQueryCase::IterateResult LimitQueryCase::iterate (void)
{
	checkTessellationSupport(m_context);

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_INTEGER);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Types", "Alternative queries");
		verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_BOOLEAN);
		verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_INTEGER64);
		verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_FLOAT);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class CombinedUniformLimitCase : public TestCase
{
public:
						CombinedUniformLimitCase	(Context& context, const char* name, const char* desc, glw::GLenum combined, glw::GLenum numBlocks, glw::GLenum defaultComponents);
private:
	IterateResult		iterate						(void);

	const glw::GLenum	m_combined;
	const glw::GLenum	m_numBlocks;
	const glw::GLenum	m_defaultComponents;
};

CombinedUniformLimitCase::CombinedUniformLimitCase (Context& context, const char* name, const char* desc, glw::GLenum combined, glw::GLenum numBlocks, glw::GLenum defaultComponents)
	: TestCase				(context, name, desc)
	, m_combined			(combined)
	, m_numBlocks			(numBlocks)
	, m_defaultComponents	(defaultComponents)
{
}

CombinedUniformLimitCase::IterateResult CombinedUniformLimitCase::iterate (void)
{
	checkTessellationSupport(m_context);

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "The minimum value of " << glu::getGettableStateStr(m_combined)
						<< " is " << glu::getGettableStateStr(m_numBlocks)
						<< " x MAX_UNIFORM_BLOCK_SIZE / 4 + "
						<< glu::getGettableStateStr(m_defaultComponents)
						<< tcu::TestLog::EndMessage;

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformBlocks;
	gl.glGetIntegerv(m_numBlocks, &maxUniformBlocks);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformBlockSize;
	gl.glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformComponents;
	gl.glGetIntegerv(m_defaultComponents, &maxUniformComponents);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	if (maxUniformBlocks.verifyValidity(result) && maxUniformBlockSize.verifyValidity(result) && maxUniformComponents.verifyValidity(result))
	{
		const int limit = ((int)maxUniformBlocks) * ((int)maxUniformBlockSize) / 4 + (int)maxUniformComponents;
		verifyStateIntegerMin(result, gl, m_combined, limit, QUERY_INTEGER);

		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Types", "Alternative queries");
			verifyStateIntegerMin(result, gl, m_combined, limit, QUERY_BOOLEAN);
			verifyStateIntegerMin(result, gl, m_combined, limit, QUERY_INTEGER64);
			verifyStateIntegerMin(result, gl, m_combined, limit, QUERY_FLOAT);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class PatchVerticesStateCase : public TestCase
{
public:
						PatchVerticesStateCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate					(void);
};

PatchVerticesStateCase::PatchVerticesStateCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

PatchVerticesStateCase::IterateResult PatchVerticesStateCase::iterate (void)
{
	checkTessellationSupport(m_context);

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial value");

		verifyStateInteger(result, gl, GL_PATCH_VERTICES, 3, QUERY_INTEGER);
	}

	// bind
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "set", "After set");

		gl.glPatchParameteri(GL_PATCH_VERTICES, 22);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glPatchParameteri");

		verifyStateInteger(result, gl, GL_PATCH_VERTICES, 22, QUERY_INTEGER);
		{
			const tcu::ScopedLogSection	subsection(m_testCtx.getLog(), "Types", "Alternative queries");
			verifyStateIntegerMin(result, gl, GL_PATCH_VERTICES, 22, QUERY_BOOLEAN);
			verifyStateIntegerMin(result, gl, GL_PATCH_VERTICES, 22, QUERY_INTEGER64);
			verifyStateIntegerMin(result, gl, GL_PATCH_VERTICES, 22, QUERY_FLOAT);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class PrimitiveRestartForPatchesSupportedCase : public TestCase
{
public:
						PrimitiveRestartForPatchesSupportedCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate									(void);
};

PrimitiveRestartForPatchesSupportedCase::PrimitiveRestartForPatchesSupportedCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

PrimitiveRestartForPatchesSupportedCase::IterateResult PrimitiveRestartForPatchesSupportedCase::iterate (void)
{
	checkTessellationSupport(m_context);

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	QueriedState			state;

	gl.enableLogging(true);

	queryState(result, gl, QUERY_BOOLEAN, GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED, state);

	if (!state.isUndefined())
	{
		const tcu::ScopedLogSection	subsection(m_testCtx.getLog(), "Types", "Alternative types");
		verifyStateBoolean(result, gl, GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED, state.getBoolAccess(), QUERY_INTEGER);
		verifyStateBoolean(result, gl, GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED, state.getBoolAccess(), QUERY_INTEGER64);
		verifyStateBoolean(result, gl, GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED, state.getBoolAccess(), QUERY_FLOAT);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TessProgramQueryCase : public TestCase
{
public:
						TessProgramQueryCase	(Context& context, const char* name, const char* desc);

	std::string			getVertexSource			(void) const;
	std::string			getFragmentSource		(void) const;
	std::string			getTessCtrlSource		(const char* globalLayouts) const;
	std::string			getTessEvalSource		(const char* globalLayouts) const;
};

TessProgramQueryCase::TessProgramQueryCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

std::string TessProgramQueryCase::getVertexSource (void) const
{
	return	"${GLSL_VERSION_DECL}\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = vec4(float(gl_VertexID), float(gl_VertexID / 2), 0.0, 1.0);\n"
			"}\n";
}

std::string TessProgramQueryCase::getFragmentSource (void) const
{
	return	"${GLSL_VERSION_DECL}\n"
			"layout (location = 0) out mediump vec4 o_color;\n"
			"void main (void)\n"
			"{\n"
			"	o_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"}\n";
}

std::string TessProgramQueryCase::getTessCtrlSource (const char* globalLayouts) const
{
	return	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			+ std::string(globalLayouts) + ";\n"
			"void main (void)\n"
			"{\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			"	gl_TessLevelInner[0] = 2.8;\n"
			"	gl_TessLevelInner[1] = 2.8;\n"
			"	gl_TessLevelOuter[0] = 2.8;\n"
			"	gl_TessLevelOuter[1] = 2.8;\n"
			"	gl_TessLevelOuter[2] = 2.8;\n"
			"	gl_TessLevelOuter[3] = 2.8;\n"
			"}\n";
}

std::string TessProgramQueryCase::getTessEvalSource (const char* globalLayouts) const
{
	return	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			+ std::string(globalLayouts) + ";\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = gl_TessCoord.x * gl_in[0].gl_Position\n"
			"	            + gl_TessCoord.y * gl_in[1].gl_Position\n"
			"	            + gl_TessCoord.y * gl_in[2].gl_Position\n"
			"	            + gl_TessCoord.z * gl_in[3].gl_Position;\n"
			"}\n";
}

class TessControlOutputVerticesCase : public TessProgramQueryCase
{
public:
						TessControlOutputVerticesCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate							(void);
};

TessControlOutputVerticesCase::TessControlOutputVerticesCase (Context& context, const char* name, const char* desc)
	: TessProgramQueryCase(context, name, desc)
{
}

TessControlOutputVerticesCase::IterateResult TessControlOutputVerticesCase::iterate (void)
{
	checkTessellationSupport(m_context);

	glu::ShaderProgram program (m_context.getRenderContext(), glu::ProgramSources()
																<< glu::VertexSource(specializeShader(m_context, getVertexSource().c_str()))
																<< glu::FragmentSource(specializeShader(m_context, getFragmentSource().c_str()))
																<< glu::TessellationControlSource(specializeShader(m_context, getTessCtrlSource("layout(vertices=4) out").c_str()))
																<< glu::TessellationEvaluationSource(specializeShader(m_context, getTessEvalSource("layout(triangles) in").c_str())));

	m_testCtx.getLog() << program;
	if (!program.isOk())
		throw tcu::TestError("failed to build program");

	{
		glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
		tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

		gl.enableLogging(true);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_CONTROL_OUTPUT_VERTICES, 4, QUERY_PROGRAM_INTEGER);

		result.setTestContextResult(m_testCtx);
	}
	return STOP;
}

class TessGenModeQueryCase : public TessProgramQueryCase
{
public:
						TessGenModeQueryCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate					(void);
};

TessGenModeQueryCase::TessGenModeQueryCase (Context& context, const char* name, const char* desc)
	: TessProgramQueryCase(context, name, desc)
{
}

TessGenModeQueryCase::IterateResult TessGenModeQueryCase::iterate (void)
{
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	static const struct
	{
		const char* description;
		const char* layout;
		glw::GLenum mode;
	} s_modes[] =
	{
		{ "Triangles",	"layout(triangles) in",	GL_TRIANGLES	},
		{ "Isolines",	"layout(isolines) in",	GL_ISOLINES		},
		{ "Quads",		"layout(quads) in",		GL_QUADS		},
	};

	checkTessellationSupport(m_context);
	gl.enableLogging(true);

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_modes); ++ndx)
	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Type", s_modes[ndx].description);

		glu::ShaderProgram program (m_context.getRenderContext(), glu::ProgramSources()
																	<< glu::VertexSource(specializeShader(m_context, getVertexSource().c_str()))
																	<< glu::FragmentSource(specializeShader(m_context, getFragmentSource().c_str()))
																	<< glu::TessellationControlSource(specializeShader(m_context, getTessCtrlSource("layout(vertices=6) out").c_str()))
																	<< glu::TessellationEvaluationSource(specializeShader(m_context, getTessEvalSource(s_modes[ndx].layout).c_str())));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			result.fail("failed to build program");
		else
			verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_MODE, s_modes[ndx].mode, QUERY_PROGRAM_INTEGER);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TessGenSpacingQueryCase : public TessProgramQueryCase
{
public:
						TessGenSpacingQueryCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate					(void);
};

TessGenSpacingQueryCase::TessGenSpacingQueryCase (Context& context, const char* name, const char* desc)
	: TessProgramQueryCase(context, name, desc)
{
}

TessGenSpacingQueryCase::IterateResult TessGenSpacingQueryCase::iterate (void)
{
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	static const struct
	{
		const char* description;
		const char* layout;
		glw::GLenum spacing;
	} s_modes[] =
	{
		{ "Default spacing",			"layout(triangles) in",								GL_EQUAL			},
		{ "Equal spacing",				"layout(triangles, equal_spacing) in",				GL_EQUAL			},
		{ "Fractional even spacing",	"layout(triangles, fractional_even_spacing) in",	GL_FRACTIONAL_EVEN	},
		{ "Fractional odd spacing",		"layout(triangles, fractional_odd_spacing) in",		GL_FRACTIONAL_ODD	},
	};

	checkTessellationSupport(m_context);
	gl.enableLogging(true);

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_modes); ++ndx)
	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Type", s_modes[ndx].description);

		glu::ShaderProgram program (m_context.getRenderContext(), glu::ProgramSources()
																	<< glu::VertexSource(specializeShader(m_context, getVertexSource().c_str()))
																	<< glu::FragmentSource(specializeShader(m_context, getFragmentSource().c_str()))
																	<< glu::TessellationControlSource(specializeShader(m_context, getTessCtrlSource("layout(vertices=6) out").c_str()))
																	<< glu::TessellationEvaluationSource(specializeShader(m_context, getTessEvalSource(s_modes[ndx].layout).c_str())));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			result.fail("failed to build program");
		else
			verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_SPACING, s_modes[ndx].spacing, QUERY_PROGRAM_INTEGER);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TessGenVertexOrderQueryCase : public TessProgramQueryCase
{
public:
						TessGenVertexOrderQueryCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate						(void);
};

TessGenVertexOrderQueryCase::TessGenVertexOrderQueryCase (Context& context, const char* name, const char* desc)
	: TessProgramQueryCase(context, name, desc)
{
}

TessGenVertexOrderQueryCase::IterateResult TessGenVertexOrderQueryCase::iterate (void)
{
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	static const struct
	{
		const char* description;
		const char* layout;
		glw::GLenum order;
	} s_modes[] =
	{
		{ "Default order",	"layout(triangles) in",			GL_CCW	},
		{ "CW order",		"layout(triangles, cw) in",		GL_CW	},
		{ "CCW order",		"layout(triangles, ccw) in",	GL_CCW	},
	};

	checkTessellationSupport(m_context);
	gl.enableLogging(true);

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_modes); ++ndx)
	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Type", s_modes[ndx].description);

		glu::ShaderProgram program (m_context.getRenderContext(), glu::ProgramSources()
																	<< glu::VertexSource(specializeShader(m_context, getVertexSource().c_str()))
																	<< glu::FragmentSource(specializeShader(m_context, getFragmentSource().c_str()))
																	<< glu::TessellationControlSource(specializeShader(m_context, getTessCtrlSource("layout(vertices=6) out").c_str()))
																	<< glu::TessellationEvaluationSource(specializeShader(m_context, getTessEvalSource(s_modes[ndx].layout).c_str())));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			result.fail("failed to build program");
		else
			verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_VERTEX_ORDER, s_modes[ndx].order, QUERY_PROGRAM_INTEGER);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TessGenPointModeQueryCase : public TessProgramQueryCase
{
public:
						TessGenPointModeQueryCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate						(void);
};

TessGenPointModeQueryCase::TessGenPointModeQueryCase (Context& context, const char* name, const char* desc)
	: TessProgramQueryCase(context, name, desc)
{
}

TessGenPointModeQueryCase::IterateResult TessGenPointModeQueryCase::iterate (void)
{
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	static const struct
	{
		const char* description;
		const char* layout;
		glw::GLenum mode;
	} s_modes[] =
	{
		{ "Default mode",	"layout(triangles) in",			GL_FALSE	},
		{ "Point mode",		"layout(triangles, point_mode) in",		GL_TRUE		},
	};

	checkTessellationSupport(m_context);
	gl.enableLogging(true);

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_modes); ++ndx)
	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Type", s_modes[ndx].description);

		glu::ShaderProgram program (m_context.getRenderContext(), glu::ProgramSources()
																	<< glu::VertexSource(specializeShader(m_context, getVertexSource().c_str()))
																	<< glu::FragmentSource(specializeShader(m_context, getFragmentSource().c_str()))
																	<< glu::TessellationControlSource(specializeShader(m_context, getTessCtrlSource("layout(vertices=6) out").c_str()))
																	<< glu::TessellationEvaluationSource(specializeShader(m_context, getTessEvalSource(s_modes[ndx].layout).c_str())));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			result.fail("failed to build program");
		else
			verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_POINT_MODE, s_modes[ndx].mode, QUERY_PROGRAM_INTEGER);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ReferencedByTessellationQueryCase : public TestCase
{
public:
					ReferencedByTessellationQueryCase	(Context& context, const char* name, const char* desc, bool isCtrlCase);
private:
	void			init								(void);
	IterateResult	iterate								(void);

	std::string		getVertexSource						(void) const;
	std::string		getFragmentSource					(void) const;
	std::string		getTessCtrlSource					(void) const;
	std::string		getTessEvalSource					(void) const;

	const bool		m_isCtrlCase;
};

ReferencedByTessellationQueryCase::ReferencedByTessellationQueryCase (Context& context, const char* name, const char* desc, bool isCtrlCase)
	: TestCase		(context, name, desc)
	, m_isCtrlCase	(isCtrlCase)
{
}

void ReferencedByTessellationQueryCase::init (void)
{
	checkTessellationSupport(m_context);
}

ReferencedByTessellationQueryCase::IterateResult ReferencedByTessellationQueryCase::iterate (void)
{
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::ShaderProgram		program	(m_context.getRenderContext(), glu::ProgramSources()
																	<< glu::VertexSource(specializeShader(m_context, getVertexSource().c_str()))
																	<< glu::FragmentSource(specializeShader(m_context, getFragmentSource().c_str()))
																	<< glu::TessellationControlSource(specializeShader(m_context, getTessCtrlSource().c_str()))
																	<< glu::TessellationEvaluationSource(specializeShader(m_context, getTessEvalSource().c_str())));

	gl.enableLogging(true);

	m_testCtx.getLog() << program;
	if (!program.isOk())
		result.fail("failed to build program");
	else
	{
		const deUint32 props[1] = { (deUint32)((m_isCtrlCase) ? (GL_REFERENCED_BY_TESS_CONTROL_SHADER) : (GL_REFERENCED_BY_TESS_EVALUATION_SHADER)) };

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "UnreferencedUniform", "Unreferenced uniform u_unreferenced");
			deUint32					resourcePos;
			glw::GLsizei				length		= 0;
			glw::GLint					referenced	= 0;

			resourcePos = gl.glGetProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_unreferenced");
			m_testCtx.getLog() << tcu::TestLog::Message << "u_unreferenced resource index: " << resourcePos << tcu::TestLog::EndMessage;

			if (resourcePos == GL_INVALID_INDEX)
				result.fail("resourcePos was GL_INVALID_INDEX");
			else
			{
				gl.glGetProgramResourceiv(program.getProgram(), GL_UNIFORM, resourcePos, 1, props, 1, &length, &referenced);
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Query " << glu::getProgramResourcePropertyStr(props[0])
					<< ", got " << length << " value(s), value[0] = " << glu::getBooleanStr(referenced)
					<< tcu::TestLog::EndMessage;

				GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "query resource");

				if (length == 0 || referenced != GL_FALSE)
					result.fail("expected GL_FALSE");
			}
		}

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "ReferencedUniform", "Referenced uniform u_referenced");
			deUint32					resourcePos;
			glw::GLsizei				length		= 0;
			glw::GLint					referenced	= 0;

			resourcePos = gl.glGetProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_referenced");
			m_testCtx.getLog() << tcu::TestLog::Message << "u_referenced resource index: " << resourcePos << tcu::TestLog::EndMessage;

			if (resourcePos == GL_INVALID_INDEX)
				result.fail("resourcePos was GL_INVALID_INDEX");
			else
			{
				gl.glGetProgramResourceiv(program.getProgram(), GL_UNIFORM, resourcePos, 1, props, 1, &length, &referenced);
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Query " << glu::getProgramResourcePropertyStr(props[0])
					<< ", got " << length << " value(s), value[0] = " << glu::getBooleanStr(referenced)
					<< tcu::TestLog::EndMessage;

				GLU_EXPECT_NO_ERROR(gl.glGetError(), "query resource");

				if (length == 0 || referenced != GL_TRUE)
					result.fail("expected GL_TRUE");
			}
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

std::string ReferencedByTessellationQueryCase::getVertexSource (void) const
{
	return	"${GLSL_VERSION_DECL}\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = vec4(float(gl_VertexID), float(gl_VertexID / 2), 0.0, 1.0);\n"
			"}\n";
}

std::string ReferencedByTessellationQueryCase::getFragmentSource (void) const
{
	return	"${GLSL_VERSION_DECL}\n"
			"layout (location = 0) out mediump vec4 o_color;\n"
			"void main (void)\n"
			"{\n"
			"	o_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"}\n";
}

std::string ReferencedByTessellationQueryCase::getTessCtrlSource (void) const
{
	std::ostringstream buf;
	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"layout(vertices = 3) out;\n"
			"uniform highp vec4 " << ((m_isCtrlCase) ? ("u_referenced") : ("u_unreferenced")) << ";\n"
			"void main (void)\n"
			"{\n"
			"	vec4 offset = " << ((m_isCtrlCase) ? ("u_referenced") : ("u_unreferenced")) << ";\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position + offset;\n"
			"	gl_TessLevelInner[0] = 2.8;\n"
			"	gl_TessLevelInner[1] = 2.8;\n"
			"	gl_TessLevelOuter[0] = 2.8;\n"
			"	gl_TessLevelOuter[1] = 2.8;\n"
			"	gl_TessLevelOuter[2] = 2.8;\n"
			"	gl_TessLevelOuter[3] = 2.8;\n"
			"}\n";
	return buf.str();
}

std::string ReferencedByTessellationQueryCase::getTessEvalSource (void) const
{
	std::ostringstream buf;
	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"layout(triangles) in;\n"
			"uniform highp vec4 " << ((m_isCtrlCase) ? ("u_unreferenced") : ("u_referenced")) << ";\n"
			"void main (void)\n"
			"{\n"
			"	vec4 offset = " << ((m_isCtrlCase) ? ("u_unreferenced") : ("u_referenced")) << ";\n"
			"	gl_Position = gl_TessCoord.x * gl_in[0].gl_Position\n"
			"	            + gl_TessCoord.y * gl_in[1].gl_Position\n"
			"	            + gl_TessCoord.z * gl_in[2].gl_Position\n"
			"	            + offset;\n"
			"}\n";

	return buf.str();
}

class IsPerPatchQueryCase : public TestCase
{
public:
					IsPerPatchQueryCase		(Context& context, const char* name, const char* desc);
private:
	void			init					(void);
	IterateResult	iterate					(void);
};

IsPerPatchQueryCase::IsPerPatchQueryCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

void IsPerPatchQueryCase::init (void)
{
	checkTessellationSupport(m_context);
}

IsPerPatchQueryCase::IterateResult IsPerPatchQueryCase::iterate (void)
{
	static const char* const s_controlSource =	"${GLSL_VERSION_DECL}\n"
												"${TESSELLATION_SHADER_REQUIRE}\n"
												"layout(vertices = 3) out;\n"
												"patch out highp vec4 v_perPatch;\n"
												"out highp vec4 v_perVertex[];\n"
												"void main (void)\n"
												"{\n"
												"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
												"	v_perPatch = gl_in[0].gl_Position;\n"
												"	v_perVertex[gl_InvocationID] = -gl_in[gl_InvocationID].gl_Position;\n"
												"	gl_TessLevelInner[0] = 2.8;\n"
												"	gl_TessLevelInner[1] = 2.8;\n"
												"	gl_TessLevelOuter[0] = 2.8;\n"
												"	gl_TessLevelOuter[1] = 2.8;\n"
												"	gl_TessLevelOuter[2] = 2.8;\n"
												"	gl_TessLevelOuter[3] = 2.8;\n"
												"}\n";
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::ShaderProgram		program	(m_context.getRenderContext(), glu::ProgramSources()
																	<< glu::TessellationControlSource(specializeShader(m_context, s_controlSource))
																	<< glu::ProgramSeparable(true));

	gl.enableLogging(true);

	m_testCtx.getLog() << program;
	if (!program.isOk())
		result.fail("failed to build program");
	else
	{
		const deUint32 props[1] = { GL_IS_PER_PATCH };

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "PerPatchOutput", "Per patch v_perPatch");
			deUint32					resourcePos;
			glw::GLsizei				length		= 0;
			glw::GLint					referenced	= 0;

			resourcePos = gl.glGetProgramResourceIndex(program.getProgram(), GL_PROGRAM_OUTPUT, "v_perPatch");
			m_testCtx.getLog() << tcu::TestLog::Message << "v_perPatch resource index: " << resourcePos << tcu::TestLog::EndMessage;

			if (resourcePos == GL_INVALID_INDEX)
				result.fail("resourcePos was GL_INVALID_INDEX");
			else
			{
				gl.glGetProgramResourceiv(program.getProgram(), GL_PROGRAM_OUTPUT, resourcePos, 1, props, 1, &length, &referenced);
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Query " << glu::getProgramResourcePropertyStr(props[0])
					<< ", got " << length << " value(s), value[0] = " << glu::getBooleanStr(referenced)
					<< tcu::TestLog::EndMessage;

				GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "query resource");

				if (length == 0 || referenced != GL_TRUE)
					result.fail("expected GL_TRUE");
			}
		}

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "PerVertexhOutput", "Per vertex v_perVertex");
			deUint32					resourcePos;
			glw::GLsizei				length		= 0;
			glw::GLint					referenced	= 0;

			resourcePos = gl.glGetProgramResourceIndex(program.getProgram(), GL_PROGRAM_OUTPUT, "v_perVertex");
			m_testCtx.getLog() << tcu::TestLog::Message << "v_perVertex resource index: " << resourcePos << tcu::TestLog::EndMessage;

			if (resourcePos == GL_INVALID_INDEX)
				result.fail("resourcePos was GL_INVALID_INDEX");
			else
			{
				gl.glGetProgramResourceiv(program.getProgram(), GL_PROGRAM_OUTPUT, resourcePos, 1, props, 1, &length, &referenced);
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Query " << glu::getProgramResourcePropertyStr(props[0])
					<< ", got " << length << " value(s), value[0] = " << glu::getBooleanStr(referenced)
					<< tcu::TestLog::EndMessage;

				GLU_EXPECT_NO_ERROR(gl.glGetError(), "query resource");

				if (length == 0 || referenced != GL_FALSE)
					result.fail("expected GL_FALSE");
			}
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

TessellationTests::TessellationTests (Context& context)
	: TestCaseGroup(context, "tessellation", "Tessellation Tests")
{
}

TessellationTests::~TessellationTests (void)
{
}

void TessellationTests::init (void)
{
	{
		tcu::TestCaseGroup* const queryGroup = new tcu::TestCaseGroup(m_testCtx, "state_query", "Query tests");
		addChild(queryGroup);

		// new limits
		queryGroup->addChild(new LimitQueryCase(m_context, "max_patch_vertices",								"Test MAX_PATCH_VERTICES",								GL_MAX_PATCH_VERTICES,							32));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_gen_level",								"Test MAX_TESS_GEN_LEVEL",								GL_MAX_TESS_GEN_LEVEL,							64));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_uniform_components",				"Test MAX_TESS_CONTROL_UNIFORM_COMPONENTS",				GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS,			1024));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_uniform_components",			"Test MAX_TESS_EVALUATION_UNIFORM_COMPONENTS",			GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS,		1024));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_texture_image_units",				"Test MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS",			GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,		16));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_texture_image_units",			"Test MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS",			GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,		16));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_output_components",				"Test MAX_TESS_CONTROL_OUTPUT_COMPONENTS",				GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS,			64));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_patch_components",							"Test MAX_TESS_PATCH_COMPONENTS",						GL_MAX_TESS_PATCH_COMPONENTS,					120));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_total_output_components",			"Test MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS",		GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS,	2048));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_output_components",				"Test MAX_TESS_EVALUATION_OUTPUT_COMPONENTS",			GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS,		64));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_uniform_blocks",					"Test MAX_TESS_CONTROL_UNIFORM_BLOCKS",					GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,				12));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_uniform_blocks",				"Test MAX_TESS_EVALUATION_UNIFORM_BLOCKS",				GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,			12));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_input_components",					"Test MAX_TESS_CONTROL_INPUT_COMPONENTS",				GL_MAX_TESS_CONTROL_INPUT_COMPONENTS,			64));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_input_components",				"Test MAX_TESS_EVALUATION_INPUT_COMPONENTS",			GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS,		64));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_atomic_counter_buffers",			"Test MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS",			GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS,		0));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_atomic_counter_buffers",		"Test MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS",		GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS,	0));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_atomic_counters",					"Test MAX_TESS_CONTROL_ATOMIC_COUNTERS",				GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS,			0));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_atomic_counters",				"Test MAX_TESS_EVALUATION_ATOMIC_COUNTERS",				GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS,			0));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_image_uniforms",					"Test MAX_TESS_CONTROL_IMAGE_UNIFORMS",					GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,				0));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_image_uniforms",				"Test MAX_TESS_EVALUATION_IMAGE_UNIFORMS",				GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS,			0));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_control_shader_storage_blocks",			"Test MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS",			GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,		0));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_tess_evaluation_shader_storage_blocks",			"Test MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS",		GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,	0));

		// modified limits
		queryGroup->addChild(new LimitQueryCase(m_context, "max_uniform_buffer_bindings",						"Test MAX_UNIFORM_BUFFER_BINDINGS",						GL_MAX_UNIFORM_BUFFER_BINDINGS,					72));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_combined_uniform_blocks",						"Test MAX_COMBINED_UNIFORM_BLOCKS",						GL_MAX_COMBINED_UNIFORM_BLOCKS,					60));
		queryGroup->addChild(new LimitQueryCase(m_context, "max_combined_texture_image_units",					"Test MAX_COMBINED_TEXTURE_IMAGE_UNITS",				GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,			96));

		// combined limits
		queryGroup->addChild(new CombinedUniformLimitCase(m_context, "max_combined_tess_control_uniform_components",		"Test MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS",	GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS,		GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,		GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS));
		queryGroup->addChild(new CombinedUniformLimitCase(m_context, "max_combined_tess_evaluation_uniform_components",		"Test MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS",	GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS,		GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,	GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS));

		// features
		queryGroup->addChild(new PrimitiveRestartForPatchesSupportedCase(m_context, "primitive_restart_for_patches_supported", "Test PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED"));

		// states
		queryGroup->addChild(new PatchVerticesStateCase(m_context, "patch_vertices", "Test PATCH_VERTICES"));

		// program states
		queryGroup->addChild(new TessControlOutputVerticesCase	(m_context, "tess_control_output_vertices",	"Test TESS_CONTROL_OUTPUT_VERTICES"));
		queryGroup->addChild(new TessGenModeQueryCase			(m_context, "tess_gen_mode",				"Test TESS_GEN_MODE"));
		queryGroup->addChild(new TessGenSpacingQueryCase		(m_context, "tess_gen_spacing",				"Test TESS_GEN_SPACING"));
		queryGroup->addChild(new TessGenVertexOrderQueryCase	(m_context, "tess_gen_vertex_order",		"Test TESS_GEN_VERTEX_ORDER"));
		queryGroup->addChild(new TessGenPointModeQueryCase		(m_context, "tess_gen_point_mode",			"Test TESS_GEN_POINT_MODE"));

		// resource queries
		queryGroup->addChild(new ReferencedByTessellationQueryCase	(m_context, "referenced_by_tess_control_shader",	"Test REFERENCED_BY_TESS_CONTROL_SHADER",		true));
		queryGroup->addChild(new ReferencedByTessellationQueryCase	(m_context, "referenced_by_tess_evaluation_shader",	"Test REFERENCED_BY_TESS_EVALUATION_SHADER",	false));
		queryGroup->addChild(new IsPerPatchQueryCase				(m_context, "is_per_patch",							"Test IS_PER_PATCH"));
	}

	{
		TestCaseGroup* const tessCoordGroup = new TestCaseGroup(m_context, "tesscoord", "Get tessellation coordinates with transform feedback and validate them");
		addChild(tessCoordGroup);

		for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
		{
			const TessPrimitiveType primitiveType = (TessPrimitiveType)primitiveTypeI;

			for (int spacingI = 0; spacingI < SPACINGMODE_LAST; spacingI++)
				tessCoordGroup->addChild(new TessCoordCase(m_context,
														   (string() + getTessPrimitiveTypeShaderName(primitiveType) + "_" + getSpacingModeShaderName((SpacingMode)spacingI)).c_str(), "",
														   primitiveType, (SpacingMode)spacingI));
		}
	}

	{
		TestCaseGroup* const windingGroup = new TestCaseGroup(m_context, "winding", "Test the cw and ccw input layout qualifiers");
		addChild(windingGroup);

		for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
		{
			const TessPrimitiveType primitiveType = (TessPrimitiveType)primitiveTypeI;
			if (primitiveType == TESSPRIMITIVETYPE_ISOLINES)
				continue;

			for (int windingI = 0; windingI < WINDING_LAST; windingI++)
			{
				const Winding winding = (Winding)windingI;
				windingGroup->addChild(new WindingCase(m_context, (string() + getTessPrimitiveTypeShaderName(primitiveType) + "_" + getWindingShaderName(winding)).c_str(), "", primitiveType, winding));
			}
		}
	}

	{
		TestCaseGroup* const shaderInputOutputGroup = new TestCaseGroup(m_context, "shader_input_output", "Test tessellation control and evaluation shader inputs and outputs");
		addChild(shaderInputOutputGroup);

		{
			static const struct
			{
				int inPatchSize;
				int outPatchSize;
			} patchVertexCountCases[] =
			{
				{  5, 10 },
				{ 10,  5 }
			};

			for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(patchVertexCountCases); caseNdx++)
			{
				const int inSize	= patchVertexCountCases[caseNdx].inPatchSize;
				const int outSize	= patchVertexCountCases[caseNdx].outPatchSize;

				const string caseName = "patch_vertices_" + de::toString(inSize) + "_in_" + de::toString(outSize) + "_out";

				shaderInputOutputGroup->addChild(new PatchVertexCountCase(m_context, caseName.c_str(), "Test input and output patch vertex counts", inSize, outSize,
																		  ("data/tessellation/" + caseName + "_ref.png").c_str()));
			}
		}

		for (int caseTypeI = 0; caseTypeI < PerPatchDataCase::CASETYPE_LAST; caseTypeI++)
		{
			const PerPatchDataCase::CaseType	caseType	= (PerPatchDataCase::CaseType)caseTypeI;
			const char* const					caseName	= PerPatchDataCase::getCaseTypeName(caseType);

			shaderInputOutputGroup->addChild(new PerPatchDataCase(m_context, caseName, PerPatchDataCase::getCaseTypeDescription(caseType), caseType,
																  PerPatchDataCase::caseTypeUsesRefImageFromFile(caseType) ? (string() + "data/tessellation/" + caseName + "_ref.png").c_str() : DE_NULL));
		}

		for (int caseTypeI = 0; caseTypeI < GLPositionCase::CASETYPE_LAST; caseTypeI++)
		{
			const GLPositionCase::CaseType	caseType	= (GLPositionCase::CaseType)caseTypeI;
			const char* const				caseName	= GLPositionCase::getCaseTypeName(caseType);

			shaderInputOutputGroup->addChild(new GLPositionCase(m_context, caseName, "", caseType, "data/tessellation/gl_position_ref.png"));
		}

		shaderInputOutputGroup->addChild(new BarrierCase(m_context, "barrier", "Basic barrier usage", "data/tessellation/barrier_ref.png"));
	}

	{
		TestCaseGroup* const miscDrawGroup = new TestCaseGroup(m_context, "misc_draw", "Miscellaneous draw-result-verifying cases");
		addChild(miscDrawGroup);

		for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
		{
			const TessPrimitiveType primitiveType = (TessPrimitiveType)primitiveTypeI;
			if (primitiveType == TESSPRIMITIVETYPE_ISOLINES)
				continue;

			const char* const primTypeName = getTessPrimitiveTypeShaderName(primitiveType);

			for (int spacingI = 0; spacingI < SPACINGMODE_LAST; spacingI++)
			{
				const string caseName = string() + "fill_cover_" + primTypeName + "_" + getSpacingModeShaderName((SpacingMode)spacingI);

				miscDrawGroup->addChild(new BasicTriangleFillCoverCase(m_context,
																	   caseName.c_str(), "Check that there are no obvious gaps in the triangle-filled area of a tessellated shape",
																	   primitiveType, (SpacingMode)spacingI,
																	   ("data/tessellation/" + caseName + "_ref").c_str()));
			}
		}

		for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
		{
			const TessPrimitiveType primitiveType = (TessPrimitiveType)primitiveTypeI;
			if (primitiveType == TESSPRIMITIVETYPE_ISOLINES)
				continue;

			const char* const primTypeName = getTessPrimitiveTypeShaderName(primitiveType);

			for (int spacingI = 0; spacingI < SPACINGMODE_LAST; spacingI++)
			{
				const string caseName = string() + "fill_overlap_" + primTypeName + "_" + getSpacingModeShaderName((SpacingMode)spacingI);

				miscDrawGroup->addChild(new BasicTriangleFillNonOverlapCase(m_context,
																			caseName.c_str(), "Check that there are no obvious triangle overlaps in the triangle-filled area of a tessellated shape",
																			primitiveType, (SpacingMode)spacingI,
																			("data/tessellation/" + caseName + "_ref").c_str()));
			}
		}

		for (int spacingI = 0; spacingI < SPACINGMODE_LAST; spacingI++)
		{
			const string caseName = string() + "isolines_" + getSpacingModeShaderName((SpacingMode)spacingI);

			miscDrawGroup->addChild(new IsolinesRenderCase(m_context,
														   caseName.c_str(), "Basic isolines render test",
														   (SpacingMode)spacingI,
														   ("data/tessellation/" + caseName + "_ref").c_str()));
		}
	}

	{
		TestCaseGroup* const commonEdgeGroup = new TestCaseGroup(m_context, "common_edge", "Draw multiple adjacent shapes and check that no cracks appear between them");
		addChild(commonEdgeGroup);

		for (int caseTypeI = 0; caseTypeI < CommonEdgeCase::CASETYPE_LAST; caseTypeI++)
		{
			for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
			{
				const CommonEdgeCase::CaseType	caseType		= (CommonEdgeCase::CaseType)caseTypeI;
				const TessPrimitiveType			primitiveType	= (TessPrimitiveType)primitiveTypeI;
				if (primitiveType == TESSPRIMITIVETYPE_ISOLINES)
						continue;

				for (int spacingI = 0; spacingI < SPACINGMODE_LAST; spacingI++)
				{
					const SpacingMode	spacing		= (SpacingMode)spacingI;
					const string		caseName	= (string() + getTessPrimitiveTypeShaderName(primitiveType)
																+ "_" + getSpacingModeShaderName(spacing)
																+ (caseType == CommonEdgeCase::CASETYPE_BASIC		? ""
																 : caseType == CommonEdgeCase::CASETYPE_PRECISE		? "_precise"
																 : DE_NULL));

					commonEdgeGroup->addChild(new CommonEdgeCase(m_context, caseName.c_str(), "", primitiveType, spacing, caseType));
				}
			}
		}
	}

	{
		TestCaseGroup* const fractionalSpacingModeGroup = new TestCaseGroup(m_context, "fractional_spacing", "Test fractional spacing modes");
		addChild(fractionalSpacingModeGroup);

		fractionalSpacingModeGroup->addChild(new FractionalSpacingModeCase(m_context, "odd",	"", SPACINGMODE_FRACTIONAL_ODD));
		fractionalSpacingModeGroup->addChild(new FractionalSpacingModeCase(m_context, "even",	"", SPACINGMODE_FRACTIONAL_EVEN));
	}

	{
		TestCaseGroup* const primitiveDiscardGroup = new TestCaseGroup(m_context, "primitive_discard", "Test primitive discard with relevant outer tessellation level <= 0.0");
		addChild(primitiveDiscardGroup);

		for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
		{
			for (int spacingI = 0; spacingI < SPACINGMODE_LAST; spacingI++)
			{
				for (int windingI = 0; windingI < WINDING_LAST; windingI++)
				{
					for (int usePointModeI = 0; usePointModeI <= 1; usePointModeI++)
					{
						const TessPrimitiveType		primitiveType	= (TessPrimitiveType)primitiveTypeI;
						const SpacingMode			spacing			= (SpacingMode)spacingI;
						const Winding				winding			= (Winding)windingI;
						const bool					usePointMode	= usePointModeI != 0;

						primitiveDiscardGroup->addChild(new PrimitiveDiscardCase(m_context, (string() + getTessPrimitiveTypeShaderName(primitiveType)
																									  + "_" + getSpacingModeShaderName(spacing)
																									  + "_" + getWindingShaderName(winding)
																									  + (usePointMode ? "_point_mode" : "")).c_str(), "",
																				 primitiveType, spacing, winding, usePointMode));
					}
				}
			}
		}
	}

	{
		TestCaseGroup* const invarianceGroup							= new TestCaseGroup(m_context, "invariance",						"Test tessellation invariance rules");

		TestCaseGroup* const invariantPrimitiveSetGroup					= new TestCaseGroup(m_context, "primitive_set",						"Test invariance rule #1");
		TestCaseGroup* const invariantOuterEdgeGroup					= new TestCaseGroup(m_context, "outer_edge_division",				"Test invariance rule #2");
		TestCaseGroup* const symmetricOuterEdgeGroup					= new TestCaseGroup(m_context, "outer_edge_symmetry",				"Test invariance rule #3");
		TestCaseGroup* const outerEdgeVertexSetIndexIndependenceGroup	= new TestCaseGroup(m_context, "outer_edge_index_independence",		"Test invariance rule #4");
		TestCaseGroup* const invariantTriangleSetGroup					= new TestCaseGroup(m_context, "triangle_set",						"Test invariance rule #5");
		TestCaseGroup* const invariantInnerTriangleSetGroup				= new TestCaseGroup(m_context, "inner_triangle_set",				"Test invariance rule #6");
		TestCaseGroup* const invariantOuterTriangleSetGroup				= new TestCaseGroup(m_context, "outer_triangle_set",				"Test invariance rule #7");
		TestCaseGroup* const tessCoordComponentRangeGroup				= new TestCaseGroup(m_context, "tess_coord_component_range",		"Test invariance rule #8, first part");
		TestCaseGroup* const oneMinusTessCoordComponentGroup			= new TestCaseGroup(m_context, "one_minus_tess_coord_component",	"Test invariance rule #8, second part");

		addChild(invarianceGroup);
		invarianceGroup->addChild(invariantPrimitiveSetGroup);
		invarianceGroup->addChild(invariantOuterEdgeGroup);
		invarianceGroup->addChild(symmetricOuterEdgeGroup);
		invarianceGroup->addChild(outerEdgeVertexSetIndexIndependenceGroup);
		invarianceGroup->addChild(invariantTriangleSetGroup);
		invarianceGroup->addChild(invariantInnerTriangleSetGroup);
		invarianceGroup->addChild(invariantOuterTriangleSetGroup);
		invarianceGroup->addChild(tessCoordComponentRangeGroup);
		invarianceGroup->addChild(oneMinusTessCoordComponentGroup);

		for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
		{
			const TessPrimitiveType		primitiveType	= (TessPrimitiveType)primitiveTypeI;
			const string				primName		= getTessPrimitiveTypeShaderName(primitiveType);
			const bool					triOrQuad		= primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS;

			for (int spacingI = 0; spacingI < SPACINGMODE_LAST; spacingI++)
			{
				const SpacingMode	spacing			= (SpacingMode)spacingI;
				const string		primSpacName	= primName + "_" + getSpacingModeShaderName(spacing);

				if (triOrQuad)
				{
					invariantOuterEdgeGroup->addChild		(new InvariantOuterEdgeCase			(m_context, primSpacName.c_str(), "", primitiveType, spacing));
					invariantTriangleSetGroup->addChild		(new InvariantTriangleSetCase		(m_context, primSpacName.c_str(), "", primitiveType, spacing));
					invariantInnerTriangleSetGroup->addChild(new InvariantInnerTriangleSetCase	(m_context, primSpacName.c_str(), "", primitiveType, spacing));
					invariantOuterTriangleSetGroup->addChild(new InvariantOuterTriangleSetCase	(m_context, primSpacName.c_str(), "", primitiveType, spacing));
				}

				for (int windingI = 0; windingI < WINDING_LAST; windingI++)
				{
					const Winding	winding				= (Winding)windingI;
					const string	primSpacWindName	= primSpacName + "_" + getWindingShaderName(winding);

					for (int usePointModeI = 0; usePointModeI <= 1; usePointModeI++)
					{
						const bool		usePointMode			= usePointModeI != 0;
						const string	primSpacWindPointName	= primSpacWindName + (usePointMode ? "_point_mode" : "");

						invariantPrimitiveSetGroup->addChild		(new InvariantPrimitiveSetCase			(m_context, primSpacWindPointName.c_str(), "", primitiveType, spacing, winding, usePointMode));
						symmetricOuterEdgeGroup->addChild			(new SymmetricOuterEdgeCase				(m_context, primSpacWindPointName.c_str(), "", primitiveType, spacing, winding, usePointMode));
						tessCoordComponentRangeGroup->addChild		(new TessCoordComponentRangeCase		(m_context, primSpacWindPointName.c_str(), "", primitiveType, spacing, winding, usePointMode));
						oneMinusTessCoordComponentGroup->addChild	(new OneMinusTessCoordComponentCase		(m_context, primSpacWindPointName.c_str(), "", primitiveType, spacing, winding, usePointMode));

						if (triOrQuad)
							outerEdgeVertexSetIndexIndependenceGroup->addChild(new OuterEdgeVertexSetIndexIndependenceCase(m_context, primSpacWindPointName.c_str(), "",
																														   primitiveType, spacing, winding, usePointMode));
					}
				}
			}
		}
	}

	{
		static const struct
		{
			const char*					name;
			const char*					description;
			UserDefinedIOCase::IOType	ioType;
		} ioCases[] =
		{
			{ "per_patch",					"Per-patch TCS outputs",					UserDefinedIOCase::IO_TYPE_PER_PATCH				},
			{ "per_patch_array",			"Per-patch array TCS outputs",				UserDefinedIOCase::IO_TYPE_PER_PATCH_ARRAY			},
			{ "per_patch_block",			"Per-patch TCS outputs in IO block",		UserDefinedIOCase::IO_TYPE_PER_PATCH_BLOCK			},
			{ "per_patch_block_array",		"Per-patch TCS outputs in IO block array",	UserDefinedIOCase::IO_TYPE_PER_PATCH_BLOCK_ARRAY	},
			{ "per_vertex",					"Per-vertex TCS outputs",					UserDefinedIOCase::IO_TYPE_PER_VERTEX				},
			{ "per_vertex_block",			"Per-vertex TCS outputs in IO block",		UserDefinedIOCase::IO_TYPE_PER_VERTEX_BLOCK			},
		};

		TestCaseGroup* const userDefinedIOGroup = new TestCaseGroup(m_context, "user_defined_io", "Test non-built-in per-patch and per-vertex inputs and outputs");
		addChild(userDefinedIOGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(ioCases); ++ndx)
		{
			TestCaseGroup* const ioTypeGroup = new TestCaseGroup(m_context, ioCases[ndx].name, ioCases[ndx].description);
			userDefinedIOGroup->addChild(ioTypeGroup);

			for (int vertexArraySizeI = 0; vertexArraySizeI < UserDefinedIOCase::VERTEX_IO_ARRAY_SIZE_LAST; vertexArraySizeI++)
			{
				const UserDefinedIOCase::VertexIOArraySize	vertexArraySize			= (UserDefinedIOCase::VertexIOArraySize)vertexArraySizeI;
				TestCaseGroup* const						vertexArraySizeGroup	= new TestCaseGroup(m_context,
																										vertexArraySizeI == UserDefinedIOCase::VERTEX_IO_ARRAY_SIZE_IMPLICIT
																											? "vertex_io_array_size_implicit"
																									  : vertexArraySizeI == UserDefinedIOCase::VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN
																											? "vertex_io_array_size_shader_builtin"
																									  : vertexArraySizeI == UserDefinedIOCase::VERTEX_IO_ARRAY_SIZE_EXPLICIT_QUERY
																											? "vertex_io_array_size_query"
																									  : DE_NULL,
																									    "");
				ioTypeGroup->addChild(vertexArraySizeGroup);

				for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
				{
					const TessPrimitiveType primitiveType = (TessPrimitiveType)primitiveTypeI;
					vertexArraySizeGroup->addChild(new UserDefinedIOCase(m_context, getTessPrimitiveTypeShaderName(primitiveType), "", primitiveType, ioCases[ndx].ioType, vertexArraySize, UserDefinedIOCase::TESS_CONTROL_OUT_ARRAY_SIZE_IMPLICIT,
																		 (string() + "data/tessellation/user_defined_io_" + getTessPrimitiveTypeShaderName(primitiveType) + "_ref.png").c_str()));
				}

				if (ioCases[ndx].ioType == UserDefinedIOCase::IO_TYPE_PER_VERTEX
					|| ioCases[ndx].ioType == UserDefinedIOCase::IO_TYPE_PER_VERTEX_BLOCK)
				{
					for (int primitiveTypeI = 0; primitiveTypeI < TESSPRIMITIVETYPE_LAST; primitiveTypeI++)
					{
						const TessPrimitiveType primitiveType = (TessPrimitiveType)primitiveTypeI;
						vertexArraySizeGroup->addChild(new UserDefinedIOCase(m_context, (string(getTessPrimitiveTypeShaderName(primitiveType)) + "_explicit_tcs_out_size").c_str(), "", primitiveType, ioCases[ndx].ioType, vertexArraySize, UserDefinedIOCase::TESS_CONTROL_OUT_ARRAY_SIZE_LAYOUT,
																			 (string() + "data/tessellation/user_defined_io_" + getTessPrimitiveTypeShaderName(primitiveType) + "_ref.png").c_str()));
					}
				}
			}
		}

		{
			de::MovePtr<TestCaseGroup>	negativeGroup	(new TestCaseGroup(m_context, "negative", "Negative cases"));

			{
				de::MovePtr<TestCaseGroup>			es31Group		(new TestCaseGroup(m_context, "es31", "GLSL ES 3.1 Negative cases"));
				gls::ShaderLibrary					shaderLibrary	(m_testCtx, m_context.getRenderContext(), m_context.getContextInfo());
				const std::vector<tcu::TestNode*>	children		= shaderLibrary.loadShaderFile("shaders/es31/tessellation_negative_user_defined_io.test");

				for (int i = 0; i < (int)children.size(); i++)
					es31Group->addChild(children[i]);

				negativeGroup->addChild(es31Group.release());
			}

			{
				de::MovePtr<TestCaseGroup>			es32Group		(new TestCaseGroup(m_context, "es32", "GLSL ES 3.2 Negative cases"));
				gls::ShaderLibrary					shaderLibrary	(m_testCtx, m_context.getRenderContext(), m_context.getContextInfo());
				const std::vector<tcu::TestNode*>	children		= shaderLibrary.loadShaderFile("shaders/es32/tessellation_negative_user_defined_io.test");

				for (int i = 0; i < (int)children.size(); i++)
					es32Group->addChild(children[i]);

				negativeGroup->addChild(es32Group.release());
			}

			userDefinedIOGroup->addChild(negativeGroup.release());
		}
	}
}

} // Functional
} // gles31
} // deqp
