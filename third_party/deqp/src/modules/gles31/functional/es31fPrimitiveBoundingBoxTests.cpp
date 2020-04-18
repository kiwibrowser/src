/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief Primitive bounding box tests.
 *//*--------------------------------------------------------------------*/

#include "es31fPrimitiveBoundingBoxTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "gluShaderProgram.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "glsStateQueryUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deRandom.hpp"
#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"

#include <vector>
#include <sstream>
#include <algorithm>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

namespace StateQueryUtil = ::deqp::gls::StateQueryUtil;

struct BoundingBox
{
	tcu::Vec4 min;
	tcu::Vec4 max;

	/*--------------------------------------------------------------------*//*!
	 * Get component by index of a 8-component vector constructed by
	 * concatenating 4-component min and max vectors.
	 *//*--------------------------------------------------------------------*/
	float&			getComponentAccess	(int ndx);
	const float&	getComponentAccess	(int ndx) const;
};

float& BoundingBox::getComponentAccess (int ndx)
{
	DE_ASSERT(ndx >= 0 && ndx < 8);
	if (ndx < 4)
		return min[ndx];
	else
		return max[ndx-4];
}

const float& BoundingBox::getComponentAccess (int ndx) const
{
	return const_cast<BoundingBox*>(this)->getComponentAccess(ndx);
}

struct ProjectedBBox
{
	tcu::Vec3	min;
	tcu::Vec3	max;
};

static ProjectedBBox projectBoundingBox (const BoundingBox& bbox)
{
	const float		wMin	= de::max(0.0f, bbox.min.w()); // clamp to w=0 as extension requires
	const float		wMax	= de::max(0.0f, bbox.max.w());
	ProjectedBBox	retVal;

	retVal.min = tcu::min(bbox.min.swizzle(0, 1, 2) / wMin,
						  bbox.min.swizzle(0, 1, 2) / wMax);
	retVal.max = tcu::max(bbox.max.swizzle(0, 1, 2) / wMin,
						  bbox.max.swizzle(0, 1, 2) / wMax);
	return retVal;
}

static tcu::IVec4 getViewportBoundingBoxArea (const ProjectedBBox& bbox, const tcu::IVec2& viewportSize, float size = 0.0f)
{
	tcu::Vec4	vertexBox;
	tcu::IVec4	pixelBox;

	vertexBox.x() = (bbox.min.x() * 0.5f + 0.5f) * (float)viewportSize.x();
	vertexBox.y() = (bbox.min.y() * 0.5f + 0.5f) * (float)viewportSize.y();
	vertexBox.z() = (bbox.max.x() * 0.5f + 0.5f) * (float)viewportSize.x();
	vertexBox.w() = (bbox.max.y() * 0.5f + 0.5f) * (float)viewportSize.y();

	pixelBox.x() = deFloorFloatToInt32(vertexBox.x() - size/2.0f);
	pixelBox.y() = deFloorFloatToInt32(vertexBox.y() - size/2.0f);
	pixelBox.z() = deCeilFloatToInt32(vertexBox.z() + size/2.0f);
	pixelBox.w() = deCeilFloatToInt32(vertexBox.w() + size/2.0f);
	return pixelBox;
}

static std::string specializeShader(Context& context, const char* code)
{
	const glu::GLSLVersion				glslVersion			= glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
	std::map<std::string, std::string>	specializationMap;

	specializationMap["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (glu::contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		specializationMap["GEOMETRY_SHADER_REQUIRE"] = "";
		specializationMap["GEOMETRY_POINT_SIZE"] = "#extension GL_EXT_geometry_point_size : require";
		specializationMap["GPU_SHADER5_REQUIRE"] = "";
		specializationMap["TESSELLATION_SHADER_REQUIRE"] = "";
		specializationMap["TESSELLATION_POINT_SIZE_REQUIRE"] = "#extension GL_EXT_tessellation_point_size : require";
		specializationMap["PRIMITIVE_BOUNDING_BOX_REQUIRE"] = "";
		specializationMap["PRIM_GL_BOUNDING_BOX"] = "gl_BoundingBox";
	}
	else
	{
		specializationMap["GEOMETRY_SHADER_REQUIRE"] = "#extension GL_EXT_geometry_shader : require";
		specializationMap["GEOMETRY_POINT_SIZE"] = "#extension GL_EXT_geometry_point_size : require";
		specializationMap["GPU_SHADER5_REQUIRE"] = "#extension GL_EXT_gpu_shader5 : require";
		specializationMap["TESSELLATION_SHADER_REQUIRE"] = "#extension GL_EXT_tessellation_shader : require";
		specializationMap["TESSELLATION_POINT_SIZE_REQUIRE"] = "#extension GL_EXT_tessellation_point_size : require";
		specializationMap["PRIMITIVE_BOUNDING_BOX_REQUIRE"] = "#extension GL_EXT_primitive_bounding_box : require";
		specializationMap["PRIM_GL_BOUNDING_BOX"] = "gl_BoundingBoxEXT";
	}

	return tcu::StringTemplate(code).specialize(specializationMap);
}

class InitialValueCase : public TestCase
{
public:
					InitialValueCase	(Context& context, const char* name, const char* desc);

	void			init				(void);
	IterateResult	iterate				(void);
};

InitialValueCase::InitialValueCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

void InitialValueCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_primitive_bounding_box"))
		throw tcu::NotSupportedError("Test requires GL_EXT_primitive_bounding_box extension");
}

InitialValueCase::IterateResult InitialValueCase::iterate (void)
{
	StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLfloat[8]>	state;
	glu::CallLogWrapper											gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	gl.enableLogging(true);

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Querying GL_PRIMITIVE_BOUNDING_BOX_EXT, expecting (-1, -1, -1, 1) (1, 1, 1, 1)"
		<< tcu::TestLog::EndMessage;

	gl.glGetFloatv(GL_PRIMITIVE_BOUNDING_BOX_EXT, state);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "query");

	if (!state.verifyValidity(m_testCtx))
		return STOP;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Got " << tcu::formatArray(&state[0], &state[8])
		<< tcu::TestLog::EndMessage;

	if ((state[0] != -1.0f) || (state[1] != -1.0f) || (state[2] != -1.0f) || (state[3] != 1.0f) ||
		(state[4] !=  1.0f) || (state[5] !=  1.0f) || (state[6] !=  1.0f) || (state[7] != 1.0f))
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Error, unexpected value"
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid initial value");
	}
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

class QueryCase : public TestCase
{
public:
	enum QueryMethod
	{
		QUERY_FLOAT = 0,
		QUERY_BOOLEAN,
		QUERY_INT,
		QUERY_INT64,

		QUERY_LAST
	};

						QueryCase	(Context& context, const char* name, const char* desc, QueryMethod method);

private:
	void				init		(void);
	IterateResult		iterate		(void);

	bool				verifyState	(glu::CallLogWrapper& gl, const BoundingBox& bbox) const;

	const QueryMethod	m_method;
};

QueryCase::QueryCase (Context& context, const char* name, const char* desc, QueryMethod method)
	: TestCase	(context, name, desc)
	, m_method	(method)
{
	DE_ASSERT(method < QUERY_LAST);
}

void QueryCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_primitive_bounding_box"))
		throw tcu::NotSupportedError("Test requires GL_EXT_primitive_bounding_box extension");
}

QueryCase::IterateResult QueryCase::iterate (void)
{
	static const BoundingBox fixedCases[] =
	{
		{ tcu::Vec4( 0.0f,  0.0f,  0.0f,  0.0f), tcu::Vec4( 0.0f,  0.0f,  0.0f,  0.0f) },
		{ tcu::Vec4(-0.0f, -0.0f, -0.0f, -0.0f), tcu::Vec4( 0.0f,  0.0f,  0.0f, -0.0f) },
		{ tcu::Vec4( 0.0f,  0.0f,  0.0f,  0.0f), tcu::Vec4( 1.0f,  1.0f,  1.0f, -1.0f) },
		{ tcu::Vec4( 2.0f,  2.0f,  2.0f,  2.0f), tcu::Vec4( 1.5f,  1.5f,  1.5f,  1.0f) },
		{ tcu::Vec4( 1.0f,  1.0f,  1.0f,  1.0f), tcu::Vec4(-1.0f, -1.0f, -1.0f, -1.0f) },
		{ tcu::Vec4( 1.0f,  1.0f,  1.0f,  0.3f), tcu::Vec4(-1.0f, -1.0f, -1.0f, -1.2f) },
	};

	const int					numRandomCases	= 9;
	glu::CallLogWrapper			gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	de::Random					rnd				(0xDE3210);
	std::vector<BoundingBox>	cases;

	cases.insert(cases.begin(), DE_ARRAY_BEGIN(fixedCases), DE_ARRAY_END(fixedCases));
	for (int ndx = 0; ndx < numRandomCases; ++ndx)
	{
		BoundingBox	boundingBox;

		// parameter evaluation order is not guaranteed, cannot just do "max = (rand(), rand(), ...)
		for (int coordNdx = 0; coordNdx < 8; ++coordNdx)
			boundingBox.getComponentAccess(coordNdx) = rnd.getFloat(-4.0f, 4.0f);

		cases.push_back(boundingBox);
	}

	gl.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	for (int caseNdx = 0; caseNdx < (int)cases.size(); ++caseNdx)
	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Iteration", "Iteration " + de::toString(caseNdx+1));
		const BoundingBox&			boundingBox	= cases[caseNdx];

		gl.glPrimitiveBoundingBox(boundingBox.min.x(), boundingBox.min.y(), boundingBox.min.z(), boundingBox.min.w(),
								  boundingBox.max.x(), boundingBox.max.y(), boundingBox.max.z(), boundingBox.max.w());

		if (!verifyState(gl, boundingBox))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected query result");
	}

	return STOP;
}

bool QueryCase::verifyState (glu::CallLogWrapper& gl, const BoundingBox& bbox) const
{
	switch (m_method)
	{
		case QUERY_FLOAT:
		{
			StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLfloat[8]>	state;
			bool														error = false;

			gl.glGetFloatv(GL_PRIMITIVE_BOUNDING_BOX_EXT, state);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "query");

			if (!state.verifyValidity(m_testCtx))
				return false;

			m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "glGetFloatv returned " << tcu::formatArray(&state[0], &state[8])
					<< tcu::TestLog::EndMessage;

			for (int ndx = 0; ndx < 8; ++ndx)
				if (state[ndx] != bbox.getComponentAccess(ndx))
					error = true;

			if (error)
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Error, unexpected value\n"
					<< "Expected ["
					<< bbox.min.x() << ", " << bbox.min.y() << ", " << bbox.min.z() << ", " << bbox.min.w() << ", "
					<< bbox.max.x() << ", " << bbox.max.y() << ", " << bbox.max.z() << ", " << bbox.max.w() << "]"
					<< tcu::TestLog::EndMessage;
				return false;
			}
			return true;
		}

		case QUERY_INT:
		{
			StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLint[8]>	state;
			bool														error = false;

			gl.glGetIntegerv(GL_PRIMITIVE_BOUNDING_BOX_EXT, state);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "query");

			if (!state.verifyValidity(m_testCtx))
				return false;

			m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "glGetIntegerv returned " << tcu::formatArray(&state[0], &state[8])
					<< tcu::TestLog::EndMessage;

			for (int ndx = 0; ndx < 8; ++ndx)
				if (state[ndx] != StateQueryUtil::roundGLfloatToNearestIntegerHalfDown<glw::GLint>(bbox.getComponentAccess(ndx)) &&
					state[ndx] != StateQueryUtil::roundGLfloatToNearestIntegerHalfUp<glw::GLint>(bbox.getComponentAccess(ndx)))
					error = true;

			if (error)
			{
				tcu::MessageBuilder builder(&m_testCtx.getLog());

				builder	<< "Error, unexpected value\n"
						<< "Expected [";

				for (int ndx = 0; ndx < 8; ++ndx)
				{
					const glw::GLint roundDown	= StateQueryUtil::roundGLfloatToNearestIntegerHalfDown<glw::GLint>(bbox.getComponentAccess(ndx));
					const glw::GLint roundUp	= StateQueryUtil::roundGLfloatToNearestIntegerHalfUp<glw::GLint>(bbox.getComponentAccess(ndx));

					if (ndx != 0)
						builder << ", ";

					if (roundDown == roundUp)
						builder << roundDown;
					else
						builder << "{" << roundDown << ", " << roundUp << "}";
				}

				builder	<< "]"
						<< tcu::TestLog::EndMessage;
				return false;
			}
			return true;
		}

		case QUERY_INT64:
		{
			StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLint64[8]>	state;
			bool																error = false;

			gl.glGetInteger64v(GL_PRIMITIVE_BOUNDING_BOX_EXT, state);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "query");

			if (!state.verifyValidity(m_testCtx))
				return false;

			m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "glGetInteger64v returned " << tcu::formatArray(&state[0], &state[8])
					<< tcu::TestLog::EndMessage;

			for (int ndx = 0; ndx < 8; ++ndx)
				if (state[ndx] != StateQueryUtil::roundGLfloatToNearestIntegerHalfDown<glw::GLint64>(bbox.getComponentAccess(ndx)) &&
					state[ndx] != StateQueryUtil::roundGLfloatToNearestIntegerHalfUp<glw::GLint64>(bbox.getComponentAccess(ndx)))
					error = true;

			if (error)
			{
				tcu::MessageBuilder builder(&m_testCtx.getLog());

				builder	<< "Error, unexpected value\n"
						<< "Expected [";

				for (int ndx = 0; ndx < 8; ++ndx)
				{
					const glw::GLint64 roundDown	= StateQueryUtil::roundGLfloatToNearestIntegerHalfDown<glw::GLint64>(bbox.getComponentAccess(ndx));
					const glw::GLint64 roundUp		= StateQueryUtil::roundGLfloatToNearestIntegerHalfUp<glw::GLint64>(bbox.getComponentAccess(ndx));

					if (ndx != 0)
						builder << ", ";

					if (roundDown == roundUp)
						builder << roundDown;
					else
						builder << "{" << roundDown << ", " << roundUp << "}";
				}

				builder	<< "]"
						<< tcu::TestLog::EndMessage;
				return false;
			}
			return true;
		}

		case QUERY_BOOLEAN:
		{
			StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLboolean[8]>	state;
			bool															error = false;

			gl.glGetBooleanv(GL_PRIMITIVE_BOUNDING_BOX_EXT, state);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "query");

			if (!state.verifyValidity(m_testCtx))
				return false;

			m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "glGetBooleanv returned ["
					<< glu::getBooleanStr(state[0]) << ", " << glu::getBooleanStr(state[1]) << ", " << glu::getBooleanStr(state[2]) << ", " << glu::getBooleanStr(state[3]) << ", "
					<< glu::getBooleanStr(state[4]) << ", " << glu::getBooleanStr(state[5]) << ", " << glu::getBooleanStr(state[6]) << ", " << glu::getBooleanStr(state[7]) << "]\n"
					<< tcu::TestLog::EndMessage;

			for (int ndx = 0; ndx < 8; ++ndx)
				if (state[ndx] != ((bbox.getComponentAccess(ndx) != 0.0f) ? (GL_TRUE) : (GL_FALSE)))
					error = true;

			if (error)
			{
				tcu::MessageBuilder builder(&m_testCtx.getLog());

				builder	<< "Error, unexpected value\n"
						<< "Expected [";

				for (int ndx = 0; ndx < 8; ++ndx)
				{
					if (ndx != 0)
						builder << ", ";

					builder << ((bbox.getComponentAccess(ndx) != 0.0f) ? ("GL_TRUE") : ("GL_FALSE"));
				}

				builder	<< "]"
						<< tcu::TestLog::EndMessage;
				return false;
			}
			return true;
		}

		default:
			DE_ASSERT(false);
			return true;
	}
}

class BBoxRenderCase : public TestCase
{
public:
	enum
	{
		FLAG_RENDERTARGET_DEFAULT	= 1u << 0, //!< render to default renderbuffer
		FLAG_RENDERTARGET_FBO		= 1u << 1, //!< render to framebuffer object

		FLAG_BBOXSIZE_EQUAL			= 1u << 2, //!< set tight primitive bounding box
		FLAG_BBOXSIZE_LARGER		= 1u << 3, //!< set padded primitive bounding box
		FLAG_BBOXSIZE_SMALLER		= 1u << 4, //!< set too small primitive bounding box

		FLAG_TESSELLATION			= 1u << 5, //!< use tessellation shader
		FLAG_GEOMETRY				= 1u << 6, //!< use geometry shader

		FLAG_SET_BBOX_STATE			= 1u << 7, //!< set primitive bounding box using global state
		FLAG_SET_BBOX_OUTPUT		= 1u << 8, //!< set primitive bounding box using tessellation output
		FLAG_PER_PRIMITIVE_BBOX		= 1u << 9, //!< set primitive bounding per primitive

		FLAGBIT_USER_BIT			= 10u //!< bits N and and up are reserved for subclasses
	};

									BBoxRenderCase					(Context& context, const char* name, const char* description, int numIterations, deUint32 flags);
									~BBoxRenderCase					(void);

protected:
	enum RenderTarget
	{
		RENDERTARGET_DEFAULT,
		RENDERTARGET_FBO,
	};
	enum BBoxSize
	{
		BBOXSIZE_EQUAL,
		BBOXSIZE_LARGER,
		BBOXSIZE_SMALLER,
	};

	enum
	{
		RENDER_TARGET_MIN_SIZE	= 256,
		FBO_SIZE				= 512,
		MIN_VIEWPORT_SIZE		= 256,
		MAX_VIEWPORT_SIZE		= 512,
	};
	DE_STATIC_ASSERT(MIN_VIEWPORT_SIZE <= RENDER_TARGET_MIN_SIZE);

	enum
	{
		VA_POS_VEC_NDX		= 0,
		VA_COL_VEC_NDX		= 1,
		VA_NUM_ATTRIB_VECS	= 2,
	};

	enum AABBRoundDirection
	{
		ROUND_INWARDS = 0,
		ROUND_OUTWARDS
	};

	struct IterationConfig
	{
		tcu::IVec2	viewportPos;
		tcu::IVec2	viewportSize;
		tcu::Vec2	patternPos;		//!< in NDC
		tcu::Vec2	patternSize;	//!< in NDC
		BoundingBox	bbox;
	};

	virtual void					init							(void);
	virtual void					deinit							(void);
	IterateResult					iterate							(void);

	virtual std::string				genVertexSource					(void) const = 0;
	virtual std::string				genFragmentSource				(void) const = 0;
	virtual std::string				genTessellationControlSource	(void) const = 0;
	virtual std::string				genTessellationEvaluationSource	(void) const = 0;
	virtual std::string				genGeometrySource				(void) const = 0;

	virtual IterationConfig			generateConfig					(int iteration, const tcu::IVec2& renderTargetSize) const = 0;
	virtual void					getAttributeData				(std::vector<tcu::Vec4>& data) const = 0;
	virtual void					renderTestPattern				(const IterationConfig& config) = 0;
	virtual void					verifyRenderResult				(const IterationConfig& config) = 0;

	IterationConfig					generateRandomConfig			(int seed, const tcu::IVec2& renderTargetSize) const;
	tcu::IVec4						getViewportPatternArea			(const tcu::Vec2& patternPos, const tcu::Vec2& patternSize, const tcu::IVec2& viewportSize, AABBRoundDirection roundDir) const;

	void							setupRender						(const IterationConfig& config);

	enum ShaderFunction
	{
		SHADER_FUNC_MIRROR_X,
		SHADER_FUNC_MIRROR_Y,
		SHADER_FUNC_INSIDE_BBOX,
	};

	const char*						genShaderFunction				(ShaderFunction func) const;

	const RenderTarget				m_renderTarget;
	const BBoxSize					m_bboxSize;
	const bool						m_hasTessellationStage;
	const bool						m_hasGeometryStage;
	const bool						m_useGlobalState;
	const bool						m_calcPerPrimitiveBBox;
	const int						m_numIterations;

	de::MovePtr<glu::ShaderProgram>	m_program;
	de::MovePtr<glu::Buffer>		m_vbo;
	de::MovePtr<glu::Framebuffer>	m_fbo;

private:
	std::vector<IterationConfig>	m_iterationConfigs;
	int								m_iteration;
};

BBoxRenderCase::BBoxRenderCase (Context& context, const char* name, const char* description, int numIterations, deUint32 flags)
	: TestCase					(context, name, description)
	, m_renderTarget			((flags & FLAG_RENDERTARGET_DEFAULT) ? (RENDERTARGET_DEFAULT) : (RENDERTARGET_FBO))
	, m_bboxSize				((flags & FLAG_BBOXSIZE_EQUAL) ? (BBOXSIZE_EQUAL) : (flags & FLAG_BBOXSIZE_SMALLER) ? (BBOXSIZE_SMALLER) : (BBOXSIZE_LARGER))
	, m_hasTessellationStage	((flags & FLAG_TESSELLATION) != 0)
	, m_hasGeometryStage		((flags & FLAG_GEOMETRY) != 0)
	, m_useGlobalState			((flags & FLAG_SET_BBOX_STATE) != 0)
	, m_calcPerPrimitiveBBox	((flags & FLAG_PER_PRIMITIVE_BBOX) != 0)
	, m_numIterations			(numIterations)
	, m_iteration				(0)
{
	// validate flags
	DE_ASSERT((((m_renderTarget == RENDERTARGET_DEFAULT)	?	(FLAG_RENDERTARGET_DEFAULT)	: (0)) |
			   ((m_renderTarget == RENDERTARGET_FBO)		?	(FLAG_RENDERTARGET_FBO)		: (0)) |
			   ((m_bboxSize == BBOXSIZE_EQUAL)				?	(FLAG_BBOXSIZE_EQUAL)		: (0)) |
			   ((m_bboxSize == BBOXSIZE_LARGER)				?	(FLAG_BBOXSIZE_LARGER)		: (0)) |
			   ((m_bboxSize == BBOXSIZE_SMALLER)			?	(FLAG_BBOXSIZE_SMALLER)		: (0)) |
			   ((m_hasTessellationStage)					?	(FLAG_TESSELLATION)			: (0)) |
			   ((m_hasGeometryStage)						?	(FLAG_GEOMETRY)				: (0)) |
			   ((m_useGlobalState)							?	(FLAG_SET_BBOX_STATE)		: (0)) |
			   ((!m_useGlobalState)							?	(FLAG_SET_BBOX_OUTPUT)		: (0)) |
			   ((m_calcPerPrimitiveBBox)					?	(FLAG_PER_PRIMITIVE_BBOX)	: (0))) == (flags & ((1u << FLAGBIT_USER_BIT) - 1)));

	DE_ASSERT(m_useGlobalState || m_hasTessellationStage); // using non-global state requires tessellation

	if (m_calcPerPrimitiveBBox)
	{
		DE_ASSERT(!m_useGlobalState); // per-primitive test requires per-primitive (non-global) state
		DE_ASSERT(m_bboxSize == BBOXSIZE_EQUAL); // smaller is hard to verify, larger not interesting
	}
}

BBoxRenderCase::~BBoxRenderCase (void)
{
	deinit();
}

void BBoxRenderCase::init (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const tcu::IVec2		renderTargetSize	= (m_renderTarget == RENDERTARGET_DEFAULT) ?
													(tcu::IVec2(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight())) :
													(tcu::IVec2(FBO_SIZE, FBO_SIZE));
	const bool				supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	// requirements
	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_primitive_bounding_box"))
		throw tcu::NotSupportedError("Test requires GL_EXT_primitive_bounding_box extension");
	if (!supportsES32 && m_hasTessellationStage && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");
	if (!supportsES32 && m_hasGeometryStage && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_geometry_shader extension");
	if (m_renderTarget == RENDERTARGET_DEFAULT && (renderTargetSize.x() < RENDER_TARGET_MIN_SIZE || renderTargetSize.y() < RENDER_TARGET_MIN_SIZE))
		throw tcu::NotSupportedError(std::string() + "Test requires " + de::toString<int>(RENDER_TARGET_MIN_SIZE) + "x" + de::toString<int>(RENDER_TARGET_MIN_SIZE) + " default framebuffer");

	// log case specifics
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Setting primitive bounding box "
			<< ((m_calcPerPrimitiveBBox)         ? ("to exactly cover each generated primitive")
			  : (m_bboxSize == BBOXSIZE_EQUAL)   ? ("to exactly cover rendered grid")
			  : (m_bboxSize == BBOXSIZE_LARGER)  ? ("to cover the grid and include some padding")
			  : (m_bboxSize == BBOXSIZE_SMALLER) ? ("to cover only a subset of the grid")
			  : (DE_NULL))
			<< ".\n"
		<< "Rendering with vertex"
			<< ((m_hasTessellationStage) ? ("-tessellation{ctrl,eval}") : (""))
			<< ((m_hasGeometryStage) ? ("-geometry") : (""))
			<< "-fragment program.\n"
		<< "Set bounding box using "
			<< ((m_useGlobalState) ? ("PRIMITIVE_BOUNDING_BOX_EXT state") : ("gl_BoundingBoxEXT output"))
			<< "\n"
		<< "Verifying rendering results are valid within the bounding box."
		<< tcu::TestLog::EndMessage;

	// resources

	{
		glu::ProgramSources sources;
		sources << glu::VertexSource(specializeShader(m_context, genVertexSource().c_str()));
		sources << glu::FragmentSource(specializeShader(m_context, genFragmentSource().c_str()));

		if (m_hasTessellationStage)
			sources << glu::TessellationControlSource(specializeShader(m_context, genTessellationControlSource().c_str()))
					<< glu::TessellationEvaluationSource(specializeShader(m_context, genTessellationEvaluationSource().c_str()));
		if (m_hasGeometryStage)
			sources << glu::GeometrySource(specializeShader(m_context, genGeometrySource().c_str()));

		m_program = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(), sources));
		GLU_EXPECT_NO_ERROR(gl.getError(), "build program");

		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "ShaderProgram", "Shader program");
			m_testCtx.getLog() << *m_program;
		}

		if (!m_program->isOk())
			throw tcu::TestError("failed to build program");
	}

	if (m_renderTarget == RENDERTARGET_FBO)
	{
		glu::Texture colorAttachment(m_context.getRenderContext());

		gl.bindTexture(GL_TEXTURE_2D, *colorAttachment);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, FBO_SIZE, FBO_SIZE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen tex");

		m_fbo = de::MovePtr<glu::Framebuffer>(new glu::Framebuffer(m_context.getRenderContext()));
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, **m_fbo);
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colorAttachment, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "attach");

		// unbind to prevent texture name deletion from removing it from current fbo attachments
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	{
		std::vector<tcu::Vec4> data;

		getAttributeData(data);

		m_vbo = de::MovePtr<glu::Buffer>(new glu::Buffer(m_context.getRenderContext()));
		gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, (int)(data.size() * sizeof(tcu::Vec4)), &data[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "create vbo");
	}

	// Iterations
	for (int iterationNdx = 0; iterationNdx < m_numIterations; ++iterationNdx)
		m_iterationConfigs.push_back(generateConfig(iterationNdx, renderTargetSize));
}

void BBoxRenderCase::deinit (void)
{
	m_program.clear();
	m_vbo.clear();
	m_fbo.clear();
}

BBoxRenderCase::IterateResult BBoxRenderCase::iterate (void)
{
	const tcu::ScopedLogSection	section		(m_testCtx.getLog(),
											 std::string() + "Iteration" + de::toString((int)m_iteration),
											 std::string() + "Iteration " + de::toString((int)m_iteration+1) + "/" + de::toString((int)m_iterationConfigs.size()));
	const IterationConfig&		config		= m_iterationConfigs[m_iteration];

	// default
	if (m_iteration == 0)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	renderTestPattern(config);
	verifyRenderResult(config);

	if (++m_iteration < (int)m_iterationConfigs.size())
		return CONTINUE;

	return STOP;
}

BBoxRenderCase::IterationConfig BBoxRenderCase::generateRandomConfig (int seed, const tcu::IVec2& renderTargetSize) const
{
	de::Random		rnd		(seed);
	IterationConfig	config;

	// viewport config
	config.viewportSize.x()	= rnd.getInt(MIN_VIEWPORT_SIZE, de::min<int>(renderTargetSize.x(), MAX_VIEWPORT_SIZE));
	config.viewportSize.y()	= rnd.getInt(MIN_VIEWPORT_SIZE, de::min<int>(renderTargetSize.y(), MAX_VIEWPORT_SIZE));
	config.viewportPos.x()	= rnd.getInt(0, renderTargetSize.x() - config.viewportSize.x());
	config.viewportPos.y()	= rnd.getInt(0, renderTargetSize.y() - config.viewportSize.y());

	// pattern location inside viewport
	config.patternSize.x()	= rnd.getFloat(0.4f, 1.4f);
	config.patternSize.y()	= rnd.getFloat(0.4f, 1.4f);
	config.patternPos.x()	= rnd.getFloat(-1.0f, 1.0f - config.patternSize.x());
	config.patternPos.y()	= rnd.getFloat(-1.0f, 1.0f - config.patternSize.y());

	// accurate bounding box
	config.bbox.min			= tcu::Vec4(config.patternPos.x(), config.patternPos.y(), 0.0f, 1.0f);
	config.bbox.max			= tcu::Vec4(config.patternPos.x() + config.patternSize.x(), config.patternPos.y() + config.patternSize.y(), 0.0f, 1.0f);

	if (m_bboxSize == BBOXSIZE_LARGER)
	{
		// increase bbox size
		config.bbox.min.x() -= rnd.getFloat() * 0.5f;
		config.bbox.min.y() -= rnd.getFloat() * 0.5f;
		config.bbox.min.z() -= rnd.getFloat() * 0.5f;

		config.bbox.max.x() += rnd.getFloat() * 0.5f;
		config.bbox.max.y() += rnd.getFloat() * 0.5f;
		config.bbox.max.z() += rnd.getFloat() * 0.5f;
	}
	else if (m_bboxSize == BBOXSIZE_SMALLER)
	{
		// reduce bbox size
		config.bbox.min.x() += rnd.getFloat() * 0.4f * config.patternSize.x();
		config.bbox.min.y() += rnd.getFloat() * 0.4f * config.patternSize.y();

		config.bbox.max.x() -= rnd.getFloat() * 0.4f * config.patternSize.x();
		config.bbox.max.y() -= rnd.getFloat() * 0.4f * config.patternSize.y();
	}

	return config;
}

tcu::IVec4 BBoxRenderCase::getViewportPatternArea (const tcu::Vec2& patternPos, const tcu::Vec2& patternSize, const tcu::IVec2& viewportSize, AABBRoundDirection roundDir) const
{
	const float	halfPixel	= 0.5f;
	tcu::Vec4	vertexBox;
	tcu::IVec4	pixelBox;

	vertexBox.x() = (patternPos.x() * 0.5f + 0.5f) * (float)viewportSize.x();
	vertexBox.y() = (patternPos.y() * 0.5f + 0.5f) * (float)viewportSize.y();
	vertexBox.z() = ((patternPos.x() + patternSize.x()) * 0.5f + 0.5f) * (float)viewportSize.x();
	vertexBox.w() = ((patternPos.y() + patternSize.y()) * 0.5f + 0.5f) * (float)viewportSize.y();

	if (roundDir == ROUND_INWARDS)
	{
		pixelBox.x() = (int)deFloatCeil(vertexBox.x()+halfPixel);
		pixelBox.y() = (int)deFloatCeil(vertexBox.y()+halfPixel);
		pixelBox.z() = (int)deFloatFloor(vertexBox.z()-halfPixel);
		pixelBox.w() = (int)deFloatFloor(vertexBox.w()-halfPixel);
	}
	else
	{
		pixelBox.x() = (int)deFloatFloor(vertexBox.x()-halfPixel);
		pixelBox.y() = (int)deFloatFloor(vertexBox.y()-halfPixel);
		pixelBox.z() = (int)deFloatCeil(vertexBox.z()+halfPixel);
		pixelBox.w() = (int)deFloatCeil(vertexBox.w()+halfPixel);
	}

	return pixelBox;
}

void BBoxRenderCase::setupRender (const IterationConfig& config)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const glw::GLint		posLocation			= gl.getAttribLocation(m_program->getProgram(), "a_position");
	const glw::GLint		colLocation			= gl.getAttribLocation(m_program->getProgram(), "a_color");
	const glw::GLint		posScaleLocation	= gl.getUniformLocation(m_program->getProgram(), "u_posScale");

	TCU_CHECK(posLocation != -1);
	TCU_CHECK(colLocation != -1);
	TCU_CHECK(posScaleLocation != -1);

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Setting viewport to ("
			<< "x: " << config.viewportPos.x() << ", "
			<< "y: " << config.viewportPos.y() << ", "
			<< "w: " << config.viewportSize.x() << ", "
			<< "h: " << config.viewportSize.y() << ")\n"
		<< "Vertex coordinates are in range:\n"
			<< "\tx: [" << config.patternPos.x() << ", " << (config.patternPos.x() + config.patternSize.x()) << "]\n"
			<< "\ty: [" << config.patternPos.y() << ", " << (config.patternPos.y() + config.patternSize.y()) << "]\n"
		<< tcu::TestLog::EndMessage;

	if (!m_calcPerPrimitiveBBox)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Setting primitive bounding box to:\n"
				<< "\t" << config.bbox.min << "\n"
				<< "\t" << config.bbox.max << "\n"
			<< tcu::TestLog::EndMessage;

	if (m_useGlobalState)
		gl.primitiveBoundingBox(config.bbox.min.x(), config.bbox.min.y(), config.bbox.min.z(), config.bbox.min.w(),
								config.bbox.max.x(), config.bbox.max.y(), config.bbox.max.z(), config.bbox.max.w());
	else
		// state is overriden by the tessellation output, set bbox to invisible area to imitiate dirty state left by application
		gl.primitiveBoundingBox(-2.0f, -2.0f, 0.0f, 1.0f,
								-1.7f, -1.7f, 0.0f, 1.0f);

	if (m_fbo)
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, **m_fbo);

	gl.viewport(config.viewportPos.x(), config.viewportPos.y(), config.viewportSize.x(), config.viewportSize.y());
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, (int)(VA_NUM_ATTRIB_VECS * sizeof(float[4])), (const float*)DE_NULL + 4 * VA_POS_VEC_NDX);
	gl.vertexAttribPointer(colLocation, 4, GL_FLOAT, GL_FALSE, (int)(VA_NUM_ATTRIB_VECS * sizeof(float[4])), (const float*)DE_NULL + 4 * VA_COL_VEC_NDX);
	gl.enableVertexAttribArray(posLocation);
	gl.enableVertexAttribArray(colLocation);
	gl.useProgram(m_program->getProgram());
	gl.uniform4f(posScaleLocation, config.patternPos.x(), config.patternPos.y(), config.patternSize.x(), config.patternSize.y());

	{
		const glw::GLint bboxMinPos = gl.getUniformLocation(m_program->getProgram(), "u_primitiveBBoxMin");
		const glw::GLint bboxMaxPos = gl.getUniformLocation(m_program->getProgram(), "u_primitiveBBoxMax");

		gl.uniform4f(bboxMinPos, config.bbox.min.x(), config.bbox.min.y(), config.bbox.min.z(), config.bbox.min.w());
		gl.uniform4f(bboxMaxPos, config.bbox.max.x(), config.bbox.max.y(), config.bbox.max.z(), config.bbox.max.w());
	}

	gl.uniform2i(gl.getUniformLocation(m_program->getProgram(), "u_viewportPos"), config.viewportPos.x(), config.viewportPos.y());
	gl.uniform2i(gl.getUniformLocation(m_program->getProgram(), "u_viewportSize"), config.viewportSize.x(), config.viewportSize.y());

	GLU_EXPECT_NO_ERROR(gl.getError(), "setup");
}

const char* BBoxRenderCase::genShaderFunction (ShaderFunction func) const
{
	switch (func)
	{
		case SHADER_FUNC_MIRROR_X:
			return	"vec4 mirrorX(in highp vec4 p)\n"
					"{\n"
					"	highp vec2 patternOffset = u_posScale.xy;\n"
					"	highp vec2 patternScale = u_posScale.zw;\n"
					"	highp vec2 patternCenter = patternOffset + patternScale * 0.5;\n"
					"	return vec4(2.0 * patternCenter.x - p.x, p.y, p.z, p.w);\n"
					"}\n";

		case SHADER_FUNC_MIRROR_Y:
			return	"vec4 mirrorY(in highp vec4 p)\n"
					"{\n"
					"	highp vec2 patternOffset = u_posScale.xy;\n"
					"	highp vec2 patternScale = u_posScale.zw;\n"
					"	highp vec2 patternCenter = patternOffset + patternScale * 0.5;\n"
					"	return vec4(p.x, 2.0 * patternCenter.y - p.y, p.z, p.w);\n"
					"}\n";

		case SHADER_FUNC_INSIDE_BBOX:
			return	"uniform highp ivec2 u_viewportPos;\n"
					"uniform highp ivec2 u_viewportSize;\n"
					"flat in highp float v_bbox_expansionSize;\n"
					"flat in highp vec3 v_bbox_clipMin;\n"
					"flat in highp vec3 v_bbox_clipMax;\n"
					"\n"
					"bool fragmentInsideTheBBox(in highp float depth)\n"
					"{\n"
					"	highp vec4 wc = vec4(floor((v_bbox_clipMin.x * 0.5 + 0.5) * float(u_viewportSize.x) - v_bbox_expansionSize/2.0),\n"
					"	                     floor((v_bbox_clipMin.y * 0.5 + 0.5) * float(u_viewportSize.y) - v_bbox_expansionSize/2.0),\n"
					"	                     ceil((v_bbox_clipMax.x * 0.5 + 0.5) * float(u_viewportSize.x) + v_bbox_expansionSize/2.0),\n"
					"	                     ceil((v_bbox_clipMax.y * 0.5 + 0.5) * float(u_viewportSize.y) + v_bbox_expansionSize/2.0));\n"
					"	if (gl_FragCoord.x < float(u_viewportPos.x) + wc.x || gl_FragCoord.x > float(u_viewportPos.x) + wc.z ||\n"
					"	    gl_FragCoord.y < float(u_viewportPos.y) + wc.y || gl_FragCoord.y > float(u_viewportPos.y) + wc.w)\n"
					"	    return false;\n"
					"	const highp float dEpsilon = 0.001;\n"
					"	if (depth*2.0-1.0 < v_bbox_clipMin.z - dEpsilon || depth*2.0-1.0 > v_bbox_clipMax.z + dEpsilon)\n"
					"	    return false;\n"
					"	return true;\n"
					"}\n";
		default:
			DE_ASSERT(false);
			return "";
	}
}

class GridRenderCase : public BBoxRenderCase
{
public:
					GridRenderCase					(Context& context, const char* name, const char* description, deUint32 flags);
					~GridRenderCase					(void);

private:
	void			init							(void);

	std::string		genVertexSource					(void) const;
	std::string		genFragmentSource				(void) const;
	std::string		genTessellationControlSource	(void) const;
	std::string		genTessellationEvaluationSource	(void) const;
	std::string		genGeometrySource				(void) const;

	IterationConfig	generateConfig					(int iteration, const tcu::IVec2& renderTargetSize) const;
	void			getAttributeData				(std::vector<tcu::Vec4>& data) const;
	void			renderTestPattern				(const IterationConfig& config);
	void			verifyRenderResult				(const IterationConfig& config);

	const int		m_gridSize;
};

GridRenderCase::GridRenderCase (Context& context, const char* name, const char* description, deUint32 flags)
	: BBoxRenderCase	(context, name, description, 12, flags)
	, m_gridSize		(24)
{
}

GridRenderCase::~GridRenderCase (void)
{
}

void GridRenderCase::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Rendering yellow-green grid to " << ((m_renderTarget == RENDERTARGET_DEFAULT) ? ("default frame buffer") : ("fbo")) << ".\n"
		<< "Grid cells are in random order, varying grid size and location for each iteration.\n"
		<< "Marking all discardable fragments (fragments outside the bounding box) with a fully saturated blue channel."
		<< tcu::TestLog::EndMessage;

	BBoxRenderCase::init();
}

std::string GridRenderCase::genVertexSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n"
			"in highp vec4 a_color;\n"
			"out highp vec4 vtx_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"\n";
	if (!m_hasTessellationStage)
	{
		DE_ASSERT(m_useGlobalState);
		buf <<	"uniform highp vec4 u_primitiveBBoxMin;\n"
				"uniform highp vec4 u_primitiveBBoxMax;\n"
				"\n"
				"flat out highp float v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize;\n"
				"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin;\n"
				"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax;\n"
				"\n";
	}

	buf <<	"void main()\n"
			"{\n"
			"	highp vec2 patternOffset = u_posScale.xy;\n"
			"	highp vec2 patternScale = u_posScale.zw;\n"
			"	gl_Position = vec4(a_position.xy * patternScale + patternOffset, a_position.z, a_position.w);\n"
			"	vtx_color = a_color;\n";

	if (!m_hasTessellationStage)
	{
		DE_ASSERT(m_useGlobalState);
		buf <<	"\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize = 0.0;\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin =\n"
				"	    min(vec3(u_primitiveBBoxMin.x, u_primitiveBBoxMin.y, u_primitiveBBoxMin.z) / u_primitiveBBoxMin.w,\n"
				"	        vec3(u_primitiveBBoxMin.x, u_primitiveBBoxMin.y, u_primitiveBBoxMin.z) / u_primitiveBBoxMax.w);\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax =\n"
				"	    min(vec3(u_primitiveBBoxMax.x, u_primitiveBBoxMax.y, u_primitiveBBoxMax.z) / u_primitiveBBoxMin.w,\n"
				"	        vec3(u_primitiveBBoxMax.x, u_primitiveBBoxMax.y, u_primitiveBBoxMax.z) / u_primitiveBBoxMax.w);\n";
	}

	buf<<	"}\n";

	return buf.str();
}

std::string GridRenderCase::genFragmentSource (void) const
{
	const char* const	colorInputName = (m_hasGeometryStage) ? ("geo_color") : (m_hasTessellationStage) ? ("tess_color") : ("vtx_color");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in mediump vec4 " << colorInputName << ";\n"
			"layout(location = 0) out mediump vec4 o_color;\n"
		<<	genShaderFunction(SHADER_FUNC_INSIDE_BBOX)
		<<	"\n"
			"void main()\n"
			"{\n"
			"	mediump vec4 baseColor = " << colorInputName << ";\n"
			"	mediump float blueChannel;\n"
			"	if (fragmentInsideTheBBox(gl_FragCoord.z))\n"
			"		blueChannel = 0.0;\n"
			"	else\n"
			"		blueChannel = 1.0;\n"
			"	o_color = vec4(baseColor.r, baseColor.g, blueChannel, baseColor.a);\n"
			"}\n";

	return buf.str();
}

std::string GridRenderCase::genTessellationControlSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"${PRIMITIVE_BOUNDING_BOX_REQUIRE}\n"
			"layout(vertices=3) out;\n"
			"\n"
			"in highp vec4 vtx_color[];\n"
			"out highp vec4 tess_ctrl_color[];\n"
			"uniform highp float u_tessellationLevel;\n"
			"uniform highp vec4 u_posScale;\n";

	if (!m_calcPerPrimitiveBBox)
	{
		buf <<	"uniform highp vec4 u_primitiveBBoxMin;\n"
				"uniform highp vec4 u_primitiveBBoxMax;\n";
	}

	buf <<	"patch out highp float vp_bbox_expansionSize;\n"
			"patch out highp vec3 vp_bbox_clipMin;\n"
			"patch out highp vec3 vp_bbox_clipMax;\n";

	if (m_calcPerPrimitiveBBox)
	{
		buf <<	"\n";
		if (m_hasGeometryStage)
			buf << genShaderFunction(SHADER_FUNC_MIRROR_X);
		buf << genShaderFunction(SHADER_FUNC_MIRROR_Y);

		buf <<	"vec4 transformVec(in highp vec4 p)\n"
				"{\n"
				"	return " << ((m_hasGeometryStage) ? ("mirrorX(mirrorY(p))") : ("mirrorY(p)")) << ";\n"
				"}\n";
	}

	buf <<	"\n"
			"void main()\n"
			"{\n"
			"	// convert to nonsensical coordinates, just in case\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position.wzxy;\n"
			"	tess_ctrl_color[gl_InvocationID] = vtx_color[gl_InvocationID];\n"
			"\n"
			"	gl_TessLevelOuter[0] = u_tessellationLevel;\n"
			"	gl_TessLevelOuter[1] = u_tessellationLevel;\n"
			"	gl_TessLevelOuter[2] = u_tessellationLevel;\n"
			"	gl_TessLevelInner[0] = u_tessellationLevel;\n";

	if (m_calcPerPrimitiveBBox)
	{
		buf <<	"\n"
				"	highp vec4 bboxMin = min(min(transformVec(gl_in[0].gl_Position),\n"
				"	                             transformVec(gl_in[1].gl_Position)),\n"
				"	                         transformVec(gl_in[2].gl_Position));\n"
				"	highp vec4 bboxMax = max(max(transformVec(gl_in[0].gl_Position),\n"
				"	                             transformVec(gl_in[1].gl_Position)),\n"
				"	                         transformVec(gl_in[2].gl_Position));\n";
	}
	else
	{
		buf <<	"\n"
				"	highp vec4 bboxMin = u_primitiveBBoxMin;\n"
				"	highp vec4 bboxMax = u_primitiveBBoxMax;\n";
	}

	if (!m_useGlobalState)
		buf <<	"\n"
				"	${PRIM_GL_BOUNDING_BOX}[0] = bboxMin;\n"
				"	${PRIM_GL_BOUNDING_BOX}[1] = bboxMax;\n";

	buf <<	"	vp_bbox_expansionSize = 0.0;\n"
			"	vp_bbox_clipMin = min(vec3(bboxMin.x, bboxMin.y, bboxMin.z) / bboxMin.w,\n"
			"	                      vec3(bboxMin.x, bboxMin.y, bboxMin.z) / bboxMax.w);\n"
			"	vp_bbox_clipMax = max(vec3(bboxMax.x, bboxMax.y, bboxMax.z) / bboxMin.w,\n"
			"	                      vec3(bboxMax.x, bboxMax.y, bboxMax.z) / bboxMax.w);\n"
			"}\n";

	return buf.str();
}

std::string GridRenderCase::genTessellationEvaluationSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"${GPU_SHADER5_REQUIRE}\n"
			"layout(triangles) in;\n"
			"\n"
			"in highp vec4 tess_ctrl_color[];\n"
			"out highp vec4 tess_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"patch in highp float vp_bbox_expansionSize;\n"
			"patch in highp vec3 vp_bbox_clipMin;\n"
			"patch in highp vec3 vp_bbox_clipMax;\n"
			"flat out highp float v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize;\n"
			"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin;\n"
			"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax;\n"
			"\n"
			"precise gl_Position;\n"
			"\n"
		<<	genShaderFunction(SHADER_FUNC_MIRROR_Y)
		<<	"void main()\n"
			"{\n"
			"	// non-trivial tessellation evaluation shader, convert from nonsensical coords, flip vertically\n"
			"	gl_Position = mirrorY(gl_TessCoord.x * gl_in[0].gl_Position.zwyx +\n"
			"	                      gl_TessCoord.y * gl_in[1].gl_Position.zwyx +\n"
			"	                      gl_TessCoord.z * gl_in[2].gl_Position.zwyx);\n"
			"	tess_color = tess_ctrl_color[0];\n"
			"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize = vp_bbox_expansionSize;\n"
			"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin = vp_bbox_clipMin;\n"
			"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax = vp_bbox_clipMax;\n"
			"}\n";

	return buf.str();
}

std::string GridRenderCase::genGeometrySource (void) const
{
	const char* const	colorInputName = (m_hasTessellationStage) ? ("tess_color") : ("vtx_color");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GEOMETRY_SHADER_REQUIRE}\n"
			"layout(triangles) in;\n"
			"layout(max_vertices=9, triangle_strip) out;\n"
			"\n"
			"in highp vec4 " << colorInputName << "[3];\n"
			"out highp vec4 geo_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"\n"
			"flat in highp float v_geo_bbox_expansionSize[3];\n"
			"flat in highp vec3 v_geo_bbox_clipMin[3];\n"
			"flat in highp vec3 v_geo_bbox_clipMax[3];\n"
			"flat out highp vec3 v_bbox_clipMin;\n"
			"flat out highp vec3 v_bbox_clipMax;\n"
			"flat out highp float v_bbox_expansionSize;\n"
		<<	genShaderFunction(SHADER_FUNC_MIRROR_X)
		<<	"\n"
			"void setVisualizationVaryings()\n"
			"{\n"
			"	v_bbox_expansionSize = v_geo_bbox_expansionSize[0];\n"
			"	v_bbox_clipMin = v_geo_bbox_clipMin[0];\n"
			"	v_bbox_clipMax = v_geo_bbox_clipMax[0];\n"
			"}\n"
			"void main()\n"
			"{\n"
			"	// Non-trivial geometry shader: 1-to-3 amplification, mirror horizontally\n"
			"	highp vec4 p0 = mirrorX(gl_in[0].gl_Position);\n"
			"	highp vec4 p1 = mirrorX(gl_in[1].gl_Position);\n"
			"	highp vec4 p2 = mirrorX(gl_in[2].gl_Position);\n"
			"	highp vec4 pCentroid = vec4((p0.xyz + p1.xyz + p2.xyz) / 3.0, 1.0);\n"
			"	highp vec4 triangleColor = " << colorInputName << "[0];\n"
			"\n"
			"	gl_Position = p0; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = p1; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = pCentroid; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	EndPrimitive();\n"
			"\n"
			"	gl_Position = p1; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = p2; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = pCentroid; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	EndPrimitive();\n"
			"\n"
			"	gl_Position = p2; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = p0; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = pCentroid; geo_color = triangleColor; setVisualizationVaryings(); EmitVertex();\n"
			"	EndPrimitive();\n"
			"}\n";

	return buf.str();
}

GridRenderCase::IterationConfig GridRenderCase::generateConfig (int iteration, const tcu::IVec2& renderTargetSize) const
{
	return generateRandomConfig(0xDEDEDEu * (deUint32)iteration, renderTargetSize);
}

void GridRenderCase::getAttributeData (std::vector<tcu::Vec4>& data) const
{
	const tcu::Vec4		green				(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4		yellow				(1.0f, 1.0f, 0.0f, 1.0f);
	std::vector<int>	cellOrder			(m_gridSize * m_gridSize);
	de::Random			rnd					(0xDE56789);

	// generate grid with cells in random order
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
		cellOrder[ndx] = ndx;
	rnd.shuffle(cellOrder.begin(), cellOrder.end());

	data.resize(m_gridSize * m_gridSize * 6 * 2);
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
	{
		const int			cellNdx		= cellOrder[ndx];
		const int			cellX		= cellNdx % m_gridSize;
		const int			cellY		= cellNdx / m_gridSize;
		const tcu::Vec4&	cellColor	= ((cellX+cellY)%2 == 0) ? (green) : (yellow);

		data[(ndx * 6 + 0) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(cellX+0) / float(m_gridSize), float(cellY+0) / float(m_gridSize), 0.0f, 1.0f);
		data[(ndx * 6 + 0) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = cellColor;
		data[(ndx * 6 + 1) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(cellX+1) / float(m_gridSize), float(cellY+1) / float(m_gridSize), 0.0f, 1.0f);
		data[(ndx * 6 + 1) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = cellColor;
		data[(ndx * 6 + 2) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(cellX+0) / float(m_gridSize), float(cellY+1) / float(m_gridSize), 0.0f, 1.0f);
		data[(ndx * 6 + 2) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = cellColor;
		data[(ndx * 6 + 3) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(cellX+0) / float(m_gridSize), float(cellY+0) / float(m_gridSize), 0.0f, 1.0f);
		data[(ndx * 6 + 3) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = cellColor;
		data[(ndx * 6 + 4) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(cellX+1) / float(m_gridSize), float(cellY+0) / float(m_gridSize), 0.0f, 1.0f);
		data[(ndx * 6 + 4) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = cellColor;
		data[(ndx * 6 + 5) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(cellX+1) / float(m_gridSize), float(cellY+1) / float(m_gridSize), 0.0f, 1.0f);
		data[(ndx * 6 + 5) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = cellColor;
	}
}

void GridRenderCase::renderTestPattern (const IterationConfig& config)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	setupRender(config);

	if (m_hasTessellationStage)
	{
		const glw::GLint	tessLevelPos	= gl.getUniformLocation(m_program->getProgram(), "u_tessellationLevel");
		const glw::GLfloat	tessLevel		= 2.8f; // will be rounded up

		TCU_CHECK(tessLevelPos != -1);

		m_testCtx.getLog() << tcu::TestLog::Message << "u_tessellationLevel = " << tessLevel << tcu::TestLog::EndMessage;

		gl.uniform1f(tessLevelPos, tessLevel);
		gl.patchParameteri(GL_PATCH_VERTICES, 3);
		GLU_EXPECT_NO_ERROR(gl.getError(), "patch param");
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering grid." << tcu::TestLog::EndMessage;

	gl.drawArrays((m_hasTessellationStage) ? (GL_PATCHES) : (GL_TRIANGLES), 0, m_gridSize * m_gridSize * 6);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");
}

void GridRenderCase::verifyRenderResult (const IterationConfig& config)
{
	const glw::Functions&	gl						= m_context.getRenderContext().getFunctions();
	const ProjectedBBox		projectedBBox			= projectBoundingBox(config.bbox);
	const tcu::IVec4		viewportBBoxArea		= getViewportBoundingBoxArea(projectedBBox, config.viewportSize);
	const tcu::IVec4		viewportGridOuterArea	= getViewportPatternArea(config.patternPos, config.patternSize, config.viewportSize, ROUND_OUTWARDS);
	const tcu::IVec4		viewportGridInnerArea	= getViewportPatternArea(config.patternPos, config.patternSize, config.viewportSize, ROUND_INWARDS);
	tcu::Surface			viewportSurface			(config.viewportSize.x(), config.viewportSize.y());
	tcu::Surface			errorMask				(config.viewportSize.x(), config.viewportSize.y());
	bool					anyError				= false;

	if (!m_calcPerPrimitiveBBox)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Projected bounding box: (clip space)\n"
				<< "\tx: [" << projectedBBox.min.x() << "," << projectedBBox.max.x() << "]\n"
				<< "\ty: [" << projectedBBox.min.y() << "," << projectedBBox.max.y() << "]\n"
				<< "\tz: [" << projectedBBox.min.z() << "," << projectedBBox.max.z() << "]\n"
			<< "In viewport coordinates:\n"
				<< "\tx: [" << viewportBBoxArea.x() << ", " << viewportBBoxArea.z() << "]\n"
				<< "\ty: [" << viewportBBoxArea.y() << ", " << viewportBBoxArea.w() << "]\n"
			<< "Verifying render results within the bounding box.\n"
			<< tcu::TestLog::EndMessage;
	else
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying render result."
			<< tcu::TestLog::EndMessage;

	if (m_fbo)
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, **m_fbo);
	glu::readPixels(m_context.getRenderContext(), config.viewportPos.x(), config.viewportPos.y(), viewportSurface.getAccess());

	tcu::clear(errorMask.getAccess(), tcu::IVec4(0,0,0,255));

	for (int y = de::max(viewportBBoxArea.y(), 0); y < de::min(viewportBBoxArea.w(), config.viewportSize.y()); ++y)
	for (int x = de::max(viewportBBoxArea.x(), 0); x < de::min(viewportBBoxArea.z(), config.viewportSize.x()); ++x)
	{
		const tcu::RGBA	pixel		= viewportSurface.getPixel(x, y);
		const bool		outsideGrid	= x < viewportGridOuterArea.x() ||
									  y < viewportGridOuterArea.y() ||
									  x > viewportGridOuterArea.z() ||
									  y > viewportGridOuterArea.w();
		const bool		insideGrid	= x > viewportGridInnerArea.x() &&
									  y > viewportGridInnerArea.y() &&
									  x < viewportGridInnerArea.z() &&
									  y < viewportGridInnerArea.w();

		bool			error		= false;

		if (outsideGrid)
		{
			// expect black
			if (pixel.getRed() != 0 || pixel.getGreen() != 0 || pixel.getBlue() != 0)
				error = true;
		}

		else if (insideGrid)
		{
			// expect green, yellow or a combination of these
			if (pixel.getGreen() != 255 || pixel.getBlue() != 0)
				error = true;
		}
		else
		{
			// boundary, allow anything
		}

		if (error)
		{
			errorMask.setPixel(x, y, tcu::RGBA::red());
			anyError = true;
		}
	}

	if (anyError)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewportSurface.getAccess())
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask.getAccess())
			<< tcu::TestLog::EndImageSet;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Result image ok."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewportSurface.getAccess())
			<< tcu::TestLog::EndImageSet;
	}
}

class LineRenderCase : public BBoxRenderCase
{
public:
	enum
	{
		LINEFLAG_WIDE = 1u << FLAGBIT_USER_BIT,	//!< use wide lines
	};

					LineRenderCase					(Context& context, const char* name, const char* description, deUint32 flags);
					~LineRenderCase					(void);

private:
	enum
	{
		GREEN_COMPONENT_NDX = 1,
		BLUE_COMPONENT_NDX = 2,

		SCAN_ROW_COMPONENT_NDX = GREEN_COMPONENT_NDX, // \note: scans are orthogonal to the line
		SCAN_COL_COMPONENT_NDX = BLUE_COMPONENT_NDX,
	};

	enum QueryDirection
	{
		DIRECTION_HORIZONTAL = 0,
		DIRECTION_VERTICAL,
	};

	enum ScanResult
	{
		SCANRESULT_NUM_LINES_OK_BIT		= (1 << 0),
		SCANRESULT_LINE_WIDTH_OK_BIT	= (1 << 1),
		SCANRESULT_LINE_WIDTH_WARN_BIT	= (1 << 2),
		SCANRESULT_LINE_WIDTH_ERR_BIT	= (1 << 3),
		SCANRESULT_LINE_CONT_OK_BIT		= (1 << 4),
		SCANRESULT_LINE_CONT_ERR_BIT	= (1 << 5),
		SCANRESULT_LINE_CONT_WARN_BIT	= (1 << 6),
	};

	void				init							(void);

	std::string			genVertexSource					(void) const;
	std::string			genFragmentSource				(void) const;
	std::string			genTessellationControlSource	(void) const;
	std::string			genTessellationEvaluationSource	(void) const;
	std::string			genGeometrySource				(void) const;

	IterationConfig		generateConfig					(int iteration, const tcu::IVec2& renderTargetSize) const;
	void				getAttributeData				(std::vector<tcu::Vec4>& data) const;
	void				renderTestPattern				(const IterationConfig& config);
	void				verifyRenderResult				(const IterationConfig& config);

	tcu::IVec2			getNumberOfLinesRange			(int queryAreaBegin, int queryAreaEnd, float patternStart, float patternSize, int viewportArea, QueryDirection queryDir) const;
	deUint8				scanRow							(const tcu::ConstPixelBufferAccess& access, int row, int rowBegin, int rowEnd, int rowViewportBegin, int rowViewportEnd, const tcu::IVec2& numLines, int& floodCounter) const;
	deUint8				scanColumn						(const tcu::ConstPixelBufferAccess& access, int column, int columnBegin, int columnEnd, int columnViewportBegin, int columnViewportEnd, const tcu::IVec2& numLines, int& floodCounter) const;
	bool				checkAreaNumLines				(const tcu::ConstPixelBufferAccess& access, const tcu::IVec4& area, int& floodCounter, int componentNdx, const tcu::IVec2& numLines) const;
	deUint8				checkLineContinuity				(const tcu::ConstPixelBufferAccess& access, const tcu::IVec2& begin, const tcu::IVec2& end, int componentNdx, int& messageLimitCounter) const;
	tcu::IVec2			getNumMinimaMaxima				(const tcu::ConstPixelBufferAccess& access, int componentNdx) const;
	deUint8				checkLineWidths					(const tcu::ConstPixelBufferAccess& access, const tcu::IVec2& begin, const tcu::IVec2& end, int componentNdx, int& floodCounter) const;
	void				printLineWidthError				(const tcu::IVec2& pos, int detectedLineWidth, const tcu::IVec2& lineWidthRange, bool isHorizontal, int& floodCounter) const;

	const int			m_patternSide;
	const bool			m_isWideLineCase;
	const int			m_wideLineLineWidth;
};

LineRenderCase::LineRenderCase (Context& context, const char* name, const char* description, deUint32 flags)
	: BBoxRenderCase		(context, name, description, 12, flags)
	, m_patternSide			(12)
	, m_isWideLineCase		((flags & LINEFLAG_WIDE) != 0)
	, m_wideLineLineWidth	(5)
{
}

LineRenderCase::~LineRenderCase (void)
{
}

void LineRenderCase::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Rendering line pattern to " << ((m_renderTarget == RENDERTARGET_DEFAULT) ? ("default frame buffer") : ("fbo")) << ".\n"
		<< "Vertical lines are green, horizontal lines blue. Using additive blending.\n"
		<< "Line segments are in random order, varying pattern size and location for each iteration.\n"
		<< "Marking all discardable fragments (fragments outside the bounding box) with a fully saturated red channel."
		<< tcu::TestLog::EndMessage;

	if (m_isWideLineCase)
	{
		glw::GLfloat lineWidthRange[2] = {0.0f, 0.0f};
		m_context.getRenderContext().getFunctions().getFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);

		if (lineWidthRange[1] < (float)m_wideLineLineWidth)
			throw tcu::NotSupportedError("Test requires line width " + de::toString(m_wideLineLineWidth));
	}

	BBoxRenderCase::init();
}

std::string LineRenderCase::genVertexSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n"
			"in highp vec4 a_color;\n"
			"out highp vec4 vtx_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"uniform highp float u_lineWidth;\n"
			"\n";
	if (!m_hasTessellationStage)
	{
		DE_ASSERT(m_useGlobalState);
		buf <<	"uniform highp vec4 u_primitiveBBoxMin;\n"
				"uniform highp vec4 u_primitiveBBoxMax;\n"
				"\n"
				"flat out highp float v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize;\n"
				"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin;\n"
				"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax;\n"
				"\n";
	}
	buf <<	"void main()\n"
			"{\n"
			"	highp vec2 patternOffset = u_posScale.xy;\n"
			"	highp vec2 patternScale = u_posScale.zw;\n"
			"	gl_Position = vec4(a_position.xy * patternScale + patternOffset, a_position.z, a_position.w);\n"
			"	vtx_color = a_color;\n";
	if (!m_hasTessellationStage)
	{
		DE_ASSERT(m_useGlobalState);
		buf <<	"\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize = u_lineWidth;\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin =\n"
				"	    min(vec3(u_primitiveBBoxMin.x, u_primitiveBBoxMin.y, u_primitiveBBoxMin.z) / u_primitiveBBoxMin.w,\n"
				"	        vec3(u_primitiveBBoxMin.x, u_primitiveBBoxMin.y, u_primitiveBBoxMin.z) / u_primitiveBBoxMax.w);\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax =\n"
				"	    min(vec3(u_primitiveBBoxMax.x, u_primitiveBBoxMax.y, u_primitiveBBoxMax.z) / u_primitiveBBoxMin.w,\n"
				"	        vec3(u_primitiveBBoxMax.x, u_primitiveBBoxMax.y, u_primitiveBBoxMax.z) / u_primitiveBBoxMax.w);\n";
	}
	buf <<	"}\n";

	return buf.str();
}

std::string LineRenderCase::genFragmentSource (void) const
{
	const char* const	colorInputName = (m_hasGeometryStage) ? ("geo_color") : (m_hasTessellationStage) ? ("tess_color") : ("vtx_color");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in mediump vec4 " << colorInputName << ";\n"
			"layout(location = 0) out mediump vec4 o_color;\n"
		<<	genShaderFunction(SHADER_FUNC_INSIDE_BBOX)
		<<	"\n"
			"void main()\n"
			"{\n"
			"	mediump vec4 baseColor = " << colorInputName << ";\n"
			"	mediump float redChannel;\n"
			"	if (fragmentInsideTheBBox(gl_FragCoord.z))\n"
			"		redChannel = 0.0;\n"
			"	else\n"
			"		redChannel = 1.0;\n"
			"	o_color = vec4(redChannel, baseColor.g, baseColor.b, baseColor.a);\n"
			"}\n";

	return buf.str();
}

std::string LineRenderCase::genTessellationControlSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"${PRIMITIVE_BOUNDING_BOX_REQUIRE}\n"
			"layout(vertices=2) out;"
			"\n"
			"in highp vec4 vtx_color[];\n"
			"out highp vec4 tess_ctrl_color[];\n"
			"uniform highp float u_tessellationLevel;\n"
			"uniform highp vec4 u_posScale;\n"
			"uniform highp float u_lineWidth;\n";

	if (!m_calcPerPrimitiveBBox)
	{
		buf <<	"uniform highp vec4 u_primitiveBBoxMin;\n"
				"uniform highp vec4 u_primitiveBBoxMax;\n";
	}

	buf <<	"patch out highp float vp_bbox_expansionSize;\n"
			"patch out highp vec3 vp_bbox_clipMin;\n"
			"patch out highp vec3 vp_bbox_clipMax;\n";

	if (m_calcPerPrimitiveBBox)
	{
		buf <<	"\n";
		if (m_hasGeometryStage)
			buf << genShaderFunction(SHADER_FUNC_MIRROR_X);
		buf << genShaderFunction(SHADER_FUNC_MIRROR_Y);

		buf <<	"vec4 transformVec(in highp vec4 p)\n"
				"{\n"
				"	return " << ((m_hasGeometryStage) ? ("mirrorX(mirrorY(p))") : ("mirrorY(p)")) << ";\n"
				"}\n";
	}

	buf <<	"\n"
			"void main()\n"
			"{\n"
			"	// convert to nonsensical coordinates, just in case\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position.wzxy;\n"
			"	tess_ctrl_color[gl_InvocationID] = vtx_color[gl_InvocationID];\n"
			"\n"
			"	gl_TessLevelOuter[0] = 0.8; // will be rounded up to 1\n"
			"	gl_TessLevelOuter[1] = u_tessellationLevel;\n";

	if (m_calcPerPrimitiveBBox)
	{
		buf <<	"\n"
				"	highp vec4 bboxMin = min(transformVec(gl_in[0].gl_Position),\n"
				"	                         transformVec(gl_in[1].gl_Position));\n"
				"	highp vec4 bboxMax = max(transformVec(gl_in[0].gl_Position),\n"
				"	                         transformVec(gl_in[1].gl_Position));\n";
	}
	else
	{
		buf <<	"\n"
				"	highp vec4 bboxMin = u_primitiveBBoxMin;\n"
				"	highp vec4 bboxMax = u_primitiveBBoxMax;\n";
	}

	if (!m_useGlobalState)
		buf <<	"\n"
				"	${PRIM_GL_BOUNDING_BOX}[0] = bboxMin;\n"
				"	${PRIM_GL_BOUNDING_BOX}[1] = bboxMax;\n";

	buf <<	"	vp_bbox_expansionSize = u_lineWidth;\n"
			"	vp_bbox_clipMin = min(vec3(bboxMin.x, bboxMin.y, bboxMin.z) / bboxMin.w,\n"
			"	                      vec3(bboxMin.x, bboxMin.y, bboxMin.z) / bboxMax.w);\n"
			"	vp_bbox_clipMax = max(vec3(bboxMax.x, bboxMax.y, bboxMax.z) / bboxMin.w,\n"
			"	                      vec3(bboxMax.x, bboxMax.y, bboxMax.z) / bboxMax.w);\n"
			"}\n";

	return buf.str();
}

std::string LineRenderCase::genTessellationEvaluationSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"layout(isolines) in;"
			"\n"
			"in highp vec4 tess_ctrl_color[];\n"
			"out highp vec4 tess_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"\n"
			"patch in highp float vp_bbox_expansionSize;\n"
			"patch in highp vec3 vp_bbox_clipMin;\n"
			"patch in highp vec3 vp_bbox_clipMax;\n"
			"flat out highp float v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize;\n"
			"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin;\n"
			"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax;\n"
		<<	genShaderFunction(SHADER_FUNC_MIRROR_Y)
		<<	"void main()\n"
			"{\n"
			"	// non-trivial tessellation evaluation shader, convert from nonsensical coords, flip vertically\n"
			"	gl_Position = mirrorY(mix(gl_in[0].gl_Position.zwyx, gl_in[1].gl_Position.zwyx, gl_TessCoord.x));\n"
			"	tess_color = tess_ctrl_color[0];\n"
			"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize = vp_bbox_expansionSize;\n"
			"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin = vp_bbox_clipMin;\n"
			"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax = vp_bbox_clipMax;\n"
			"}\n";

	return buf.str();
}

std::string LineRenderCase::genGeometrySource (void) const
{
	const char* const	colorInputName = (m_hasTessellationStage) ? ("tess_color") : ("vtx_color");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GEOMETRY_SHADER_REQUIRE}\n"
			"layout(lines) in;\n"
			"layout(max_vertices=5, line_strip) out;\n"
			"\n"
			"in highp vec4 " << colorInputName << "[2];\n"
			"out highp vec4 geo_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"\n"
			"\n"
			"flat in highp float v_geo_bbox_expansionSize[2];\n"
			"flat in highp vec3 v_geo_bbox_clipMin[2];\n"
			"flat in highp vec3 v_geo_bbox_clipMax[2];\n"
			"flat out highp vec3 v_bbox_clipMin;\n"
			"flat out highp vec3 v_bbox_clipMax;\n"
			"flat out highp float v_bbox_expansionSize;\n"
		<<	genShaderFunction(SHADER_FUNC_MIRROR_X)
		<<	"\n"
			"void setVisualizationVaryings()\n"
			"{\n"
			"	v_bbox_expansionSize = v_geo_bbox_expansionSize[0];\n"
			"	v_bbox_clipMin = v_geo_bbox_clipMin[0];\n"
			"	v_bbox_clipMax = v_geo_bbox_clipMax[0];\n"
			"}\n"
			"void main()\n"
			"{\n"
			"	// Non-trivial geometry shader: 1-to-3 amplification, mirror horizontally\n"
			"	highp vec4 p0 = mirrorX(gl_in[0].gl_Position);\n"
			"	highp vec4 p1 = mirrorX(gl_in[1].gl_Position);\n"
			"	highp vec4 lineColor = " << colorInputName << "[0];\n"
			"\n"
			"	// output two separate primitives, just in case\n"
			"	gl_Position = mix(p0, p1, 0.00); geo_color = lineColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = mix(p0, p1, 0.33); geo_color = lineColor; setVisualizationVaryings(); EmitVertex();\n"
			"	EndPrimitive();\n"
			"\n"
			"	gl_Position = mix(p0, p1, 0.33); geo_color = lineColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = mix(p0, p1, 0.67); geo_color = lineColor; setVisualizationVaryings(); EmitVertex();\n"
			"	gl_Position = mix(p0, p1, 1.00); geo_color = lineColor; setVisualizationVaryings(); EmitVertex();\n"
			"	EndPrimitive();\n"
			"}\n";

	return buf.str();
}

LineRenderCase::IterationConfig LineRenderCase::generateConfig (int iteration, const tcu::IVec2& renderTargetSize) const
{
	const int numMaxAttempts = 128;

	// Avoid too narrow viewports, lines could merge together. Require viewport is at least 2.5x the size of the line bodies.
	for (int attemptNdx = 0; attemptNdx < numMaxAttempts; ++attemptNdx)
	{
		const IterationConfig& config = generateRandomConfig((0xDEDEDEu * (deUint32)iteration) ^ (0xABAB13 * attemptNdx), renderTargetSize);

		if ((float)config.viewportSize.x() * (config.patternSize.x() * 0.5f) > 2.5f * (float)m_patternSide * (float)m_wideLineLineWidth &&
			(float)config.viewportSize.y() * (config.patternSize.y() * 0.5f) > 2.5f * (float)m_patternSide * (float)m_wideLineLineWidth)
		{
			return config;
		}
	}

	DE_ASSERT(false);
	return IterationConfig();
}

void LineRenderCase::getAttributeData (std::vector<tcu::Vec4>& data) const
{
	const tcu::Vec4		green		(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4		blue		(0.0f, 0.0f, 1.0f, 1.0f);
	std::vector<int>	cellOrder	(m_patternSide * m_patternSide * 2);
	de::Random			rnd			(0xDE12345);

	// generate crosshatch pattern with segments in random order
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
		cellOrder[ndx] = ndx;
	rnd.shuffle(cellOrder.begin(), cellOrder.end());

	data.resize(cellOrder.size() * 4);
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
	{
		const int segmentID		= cellOrder[ndx];
		const int direction		= segmentID & 0x01;
		const int majorCoord	= (segmentID >> 1) / m_patternSide;
		const int minorCoord	= (segmentID >> 1) % m_patternSide;

		if (direction)
		{
			data[(ndx * 2 + 0) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(minorCoord) / float(m_patternSide), float(majorCoord) / float(m_patternSide), 0.0f, 1.0f);
			data[(ndx * 2 + 0) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = green;
			data[(ndx * 2 + 1) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(minorCoord) / float(m_patternSide), float(majorCoord + 1) / float(m_patternSide), 0.0f, 1.0f);
			data[(ndx * 2 + 1) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = green;
		}
		else
		{
			data[(ndx * 2 + 0) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(majorCoord) / float(m_patternSide), float(minorCoord) / float(m_patternSide), 0.0f, 1.0f);
			data[(ndx * 2 + 0) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = blue;
			data[(ndx * 2 + 1) * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(majorCoord + 1) / float(m_patternSide), float(minorCoord) / float(m_patternSide), 0.0f, 1.0f);
			data[(ndx * 2 + 1) * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = blue;
		}
	}
}

void LineRenderCase::renderTestPattern (const IterationConfig& config)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	setupRender(config);

	if (m_hasTessellationStage)
	{
		const glw::GLint	tessLevelPos	= gl.getUniformLocation(m_program->getProgram(), "u_tessellationLevel");
		const glw::GLfloat	tessLevel		= 2.8f; // will be rounded up

		TCU_CHECK(tessLevelPos != -1);

		m_testCtx.getLog() << tcu::TestLog::Message << "u_tessellationLevel = " << tessLevel << tcu::TestLog::EndMessage;

		gl.uniform1f(tessLevelPos, tessLevel);
		gl.patchParameteri(GL_PATCH_VERTICES, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "patch param");
	}

	if (m_isWideLineCase)
		gl.lineWidth((float)m_wideLineLineWidth);

	gl.uniform1f(gl.getUniformLocation(m_program->getProgram(), "u_lineWidth"), (m_isWideLineCase) ? ((float)m_wideLineLineWidth) : (1.0f));

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering pattern." << tcu::TestLog::EndMessage;

	gl.enable(GL_BLEND);
	gl.blendFunc(GL_ONE, GL_ONE);
	gl.blendEquation(GL_FUNC_ADD);

	gl.drawArrays((m_hasTessellationStage) ? (GL_PATCHES) : (GL_LINES), 0, m_patternSide * m_patternSide * 2 * 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");
}

void LineRenderCase::verifyRenderResult (const IterationConfig& config)
{
	const glw::Functions&	gl							= m_context.getRenderContext().getFunctions();
	const bool				isMsaa						= m_context.getRenderTarget().getNumSamples() > 1;
	const ProjectedBBox		projectedBBox				= projectBoundingBox(config.bbox);
	const float				lineWidth					= (m_isWideLineCase) ? ((float)m_wideLineLineWidth) : (1.0f);
	const tcu::IVec4		viewportBBoxArea			= getViewportBoundingBoxArea(projectedBBox, config.viewportSize, lineWidth);
	const tcu::IVec4		viewportPatternArea			= getViewportPatternArea(config.patternPos, config.patternSize, config.viewportSize, ROUND_INWARDS);
	const tcu::IVec2		expectedHorizontalLines		= getNumberOfLinesRange(viewportBBoxArea.y(), viewportBBoxArea.w(), config.patternPos.y(), config.patternSize.y(), config.viewportSize.y(), DIRECTION_VERTICAL);
	const tcu::IVec2		expectedVerticalLines		= getNumberOfLinesRange(viewportBBoxArea.x(), viewportBBoxArea.z(), config.patternPos.x(), config.patternSize.x(), config.viewportSize.x(), DIRECTION_HORIZONTAL);
	const tcu::IVec4		verificationArea			= tcu::IVec4(de::max(viewportBBoxArea.x(), 0),
																	 de::max(viewportBBoxArea.y(), 0),
																	 de::min(viewportBBoxArea.z(), config.viewportSize.x()),
																	 de::min(viewportBBoxArea.w(), config.viewportSize.y()));

	tcu::Surface			viewportSurface				(config.viewportSize.x(), config.viewportSize.y());
	int						messageLimitCounter			= 8;

	enum ScanResultCodes
	{
		SCANRESULT_NUM_LINES_ERR	= 0,
		SCANRESULT_LINE_WIDTH_MSAA	= 1,
		SCANRESULT_LINE_WIDTH_WARN	= 2,
		SCANRESULT_LINE_WIDTH_ERR	= 3,
		SCANRESULT_LINE_CONT_ERR	= 4,
		SCANRESULT_LINE_CONT_WARN	= 5,
		SCANRESULT_LINE_LAST
	};

	int						rowScanResult[SCANRESULT_LINE_LAST]		= {0, 0, 0, 0, 0, 0};
	int						columnScanResult[SCANRESULT_LINE_LAST]	= {0, 0, 0, 0, 0, 0};
	bool					anyError								= false;
	bool					msaaRelaxationRequired					= false;
	bool					hwIssueRelaxationRequired				= false;

	if (!m_calcPerPrimitiveBBox)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Projected bounding box: (clip space)\n"
				<< "\tx: [" << projectedBBox.min.x() << "," << projectedBBox.max.x() << "]\n"
				<< "\ty: [" << projectedBBox.min.y() << "," << projectedBBox.max.y() << "]\n"
				<< "\tz: [" << projectedBBox.min.z() << "," << projectedBBox.max.z() << "]\n"
			<< "In viewport coordinates:\n"
				<< "\tx: [" << viewportBBoxArea.x() << ", " << viewportBBoxArea.z() << "]\n"
				<< "\ty: [" << viewportBBoxArea.y() << ", " << viewportBBoxArea.w() << "]\n"
			<< "Verifying render results within the bounding box:\n"
			<< tcu::TestLog::EndMessage;
	else
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying render result:"
			<< tcu::TestLog::EndMessage;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
			<< "\tCalculating number of horizontal and vertical lines within the bounding box, expecting:\n"
			<< "\t[" << expectedHorizontalLines.x() << ", " << expectedHorizontalLines.y() << "] horizontal lines.\n"
			<< "\t[" << expectedVerticalLines.x() << ", " << expectedVerticalLines.y() << "] vertical lines.\n"
		<< tcu::TestLog::EndMessage;

	if (m_fbo)
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, **m_fbo);
	glu::readPixels(m_context.getRenderContext(), config.viewportPos.x(), config.viewportPos.y(), viewportSurface.getAccess());

	// scan rows
	for (int y = de::max(verificationArea.y(), viewportPatternArea.y()); y < de::min(verificationArea.w(), viewportPatternArea.w()); ++y)
	{
		const deUint8 result = scanRow(viewportSurface.getAccess(),
									   y,
									   verificationArea.x(),
									   verificationArea.z(),
									   de::max(verificationArea.x(), viewportPatternArea.x()),
									   de::min(verificationArea.z(), viewportPatternArea.z()),
									   expectedVerticalLines,
									   messageLimitCounter);

		if ((result & SCANRESULT_NUM_LINES_OK_BIT) == 0)
			rowScanResult[SCANRESULT_NUM_LINES_ERR]++;
		if ((result & SCANRESULT_LINE_CONT_OK_BIT) == 0)
		{
			if ((result & SCANRESULT_LINE_CONT_WARN_BIT) != 0)
				rowScanResult[SCANRESULT_LINE_CONT_WARN]++;
			else
				rowScanResult[SCANRESULT_LINE_CONT_ERR]++;
		}
		else if ((result & SCANRESULT_LINE_WIDTH_OK_BIT) == 0)
		{
			if (m_isWideLineCase && isMsaa)
			{
				// multisampled wide lines might not be supported
				rowScanResult[SCANRESULT_LINE_WIDTH_MSAA]++;
			}
			else if ((result & SCANRESULT_LINE_WIDTH_ERR_BIT) == 0 &&
					 (result & SCANRESULT_LINE_WIDTH_WARN_BIT) != 0)
			{
				rowScanResult[SCANRESULT_LINE_WIDTH_WARN]++;
			}
			else
				rowScanResult[SCANRESULT_LINE_WIDTH_ERR]++;
		}
	}

	// scan columns
	for (int x = de::max(verificationArea.x(), viewportPatternArea.x()); x < de::min(verificationArea.z(), viewportPatternArea.z()); ++x)
	{
		const deUint8 result = scanColumn(viewportSurface.getAccess(),
										  x,
										  verificationArea.y(),
										  verificationArea.w(),
										  de::min(verificationArea.y(), viewportPatternArea.y()),
										  de::min(verificationArea.w(), viewportPatternArea.w()),
										  expectedHorizontalLines,
										  messageLimitCounter);

		if ((result & SCANRESULT_NUM_LINES_OK_BIT) == 0)
			columnScanResult[SCANRESULT_NUM_LINES_ERR]++;
		if ((result & SCANRESULT_LINE_CONT_OK_BIT) == 0)
		{
			if ((result & SCANRESULT_LINE_CONT_WARN_BIT) != 0)
				columnScanResult[SCANRESULT_LINE_CONT_WARN]++;
			else
				columnScanResult[SCANRESULT_LINE_CONT_ERR]++;
		}
		else if ((result & SCANRESULT_LINE_WIDTH_OK_BIT) == 0)
		{
			if (m_isWideLineCase && isMsaa)
			{
				// multisampled wide lines might not be supported
				columnScanResult[SCANRESULT_LINE_WIDTH_MSAA]++;
			}
			else if ((result & SCANRESULT_LINE_WIDTH_ERR_BIT) == 0 &&
					 (result & SCANRESULT_LINE_WIDTH_WARN_BIT) != 0)
			{
				columnScanResult[SCANRESULT_LINE_WIDTH_WARN]++;
			}
			else
				columnScanResult[SCANRESULT_LINE_WIDTH_ERR]++;
		}
	}

	if (columnScanResult[SCANRESULT_LINE_WIDTH_ERR] != 0 || rowScanResult[SCANRESULT_LINE_WIDTH_ERR] != 0)
		anyError = true;
	else if(columnScanResult[SCANRESULT_LINE_CONT_ERR] != 0 || rowScanResult[SCANRESULT_LINE_CONT_ERR] != 0)
		anyError = true;
	else if (columnScanResult[SCANRESULT_LINE_WIDTH_MSAA] != 0 || rowScanResult[SCANRESULT_LINE_WIDTH_MSAA] != 0)
		msaaRelaxationRequired = true;
	else if (columnScanResult[SCANRESULT_LINE_WIDTH_WARN] != 0 || rowScanResult[SCANRESULT_LINE_WIDTH_WARN] != 0)
		hwIssueRelaxationRequired = true;
	else if (columnScanResult[SCANRESULT_NUM_LINES_ERR] != 0)
	{
		// found missing lines in a columnw and row line continuity check reported a warning (not an error) -> line width precision issue
		if (rowScanResult[SCANRESULT_LINE_CONT_ERR] == 0 && rowScanResult[SCANRESULT_LINE_CONT_WARN])
			hwIssueRelaxationRequired = true;
		else
			anyError = true;
	}
	else if (rowScanResult[SCANRESULT_NUM_LINES_ERR] != 0)
	{
		// found missing lines in a row and column line continuity check reported a warning (not an error) -> line width precision issue
		if (columnScanResult[SCANRESULT_LINE_CONT_ERR] == 0 && columnScanResult[SCANRESULT_LINE_CONT_WARN])
			hwIssueRelaxationRequired = true;
		else
			anyError = true;
	}

	if (anyError || msaaRelaxationRequired || hwIssueRelaxationRequired)
	{
		if (messageLimitCounter < 0)
			m_testCtx.getLog() << tcu::TestLog::Message << "Omitted " << (-messageLimitCounter) << " row/column error descriptions." << tcu::TestLog::EndMessage;

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewportSurface.getAccess())
			<< tcu::TestLog::EndImageSet;

		if (anyError)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
		else if (hwIssueRelaxationRequired)
		{
			// Line width hw issue
			m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, "Line width verification failed");
		}
		else
		{
			// MSAA wide lines are optional
			m_testCtx.setTestResult(QP_TEST_RESULT_COMPATIBILITY_WARNING, "Multisampled wide line verification failed");
		}
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Result image ok."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewportSurface.getAccess())
			<< tcu::TestLog::EndImageSet;
	}
}

tcu::IVec2 LineRenderCase::getNumberOfLinesRange (int queryAreaBegin, int queryAreaEnd, float patternStart, float patternSize, int viewportArea, QueryDirection queryDir) const
{
	// pattern is not symmetric due to mirroring
	const int	patternStartNdx	= (queryDir == DIRECTION_HORIZONTAL) ? ((m_hasGeometryStage) ? (1) : (0)) : ((m_hasTessellationStage) ? (1) : (0));
	const int	patternEndNdx	= patternStartNdx + m_patternSide;

	int			numLinesMin		= 0;
	int			numLinesMax		= 0;

	for (int lineNdx = patternStartNdx; lineNdx < patternEndNdx; ++lineNdx)
	{
		const float linePos		= (patternStart + (float(lineNdx) / float(m_patternSide)) * patternSize) * 0.5f + 0.5f;
		const float lineWidth	= (m_isWideLineCase) ? ((float)m_wideLineLineWidth) : (1.0f);

		if (linePos * (float)viewportArea > (float)queryAreaBegin + 1.0f &&
			linePos * (float)viewportArea < (float)queryAreaEnd   - 1.0f)
		{
			// line center is within the area
			++numLinesMin;
			++numLinesMax;
		}
		else if (linePos * (float)viewportArea > (float)queryAreaBegin - lineWidth*0.5f - 1.0f &&
		         linePos * (float)viewportArea < (float)queryAreaEnd   + lineWidth*0.5f + 1.0f)
		{
			// line could leak into area
			++numLinesMax;
		}
	}

	return tcu::IVec2(numLinesMin, numLinesMax);
}

deUint8 LineRenderCase::scanRow (const tcu::ConstPixelBufferAccess& access, int row, int rowBegin, int rowEnd, int rowViewportBegin, int rowViewportEnd, const tcu::IVec2& numLines, int& messageLimitCounter) const
{
	const bool		numLinesOk			= checkAreaNumLines(access, tcu::IVec4(rowBegin, row, rowEnd - rowBegin, 1), messageLimitCounter, SCAN_ROW_COMPONENT_NDX, numLines);
	const deUint8	lineWidthRes		= checkLineWidths(access, tcu::IVec2(rowBegin, row), tcu::IVec2(rowEnd, row), SCAN_ROW_COMPONENT_NDX, messageLimitCounter);
	const deUint8	lineContinuityRes	= checkLineContinuity(access, tcu::IVec2(rowViewportBegin, row), tcu::IVec2(rowViewportEnd, row), SCAN_COL_COMPONENT_NDX, messageLimitCounter);
	deUint8			result				= 0;

	if (numLinesOk)
		result |= (deUint8)SCANRESULT_NUM_LINES_OK_BIT;

	if (lineContinuityRes == 0)
		result |= (deUint8)SCANRESULT_LINE_CONT_OK_BIT;
	else
		result |= lineContinuityRes;

	if (lineWidthRes == 0)
		result |= (deUint8)SCANRESULT_LINE_WIDTH_OK_BIT;
	else
		result |= lineWidthRes;

	return result;
}

deUint8 LineRenderCase::scanColumn (const tcu::ConstPixelBufferAccess& access, int column, int columnBegin, int columnEnd, int columnViewportBegin, int columnViewportEnd, const tcu::IVec2& numLines, int& messageLimitCounter) const
{
	const bool		numLinesOk			= checkAreaNumLines(access, tcu::IVec4(column, columnBegin, 1, columnEnd - columnBegin), messageLimitCounter, SCAN_COL_COMPONENT_NDX, numLines);
	const deUint8	lineWidthRes		= checkLineWidths(access, tcu::IVec2(column, columnBegin), tcu::IVec2(column, columnEnd), SCAN_COL_COMPONENT_NDX, messageLimitCounter);
	const deUint8	lineContinuityRes	= checkLineContinuity(access, tcu::IVec2(column, columnViewportBegin), tcu::IVec2(column, columnViewportEnd), SCAN_ROW_COMPONENT_NDX, messageLimitCounter);
	deUint8			result				= 0;

	if (numLinesOk)
		result |= (deUint8)SCANRESULT_NUM_LINES_OK_BIT;

	if (lineContinuityRes == 0)
		result |= (deUint8)SCANRESULT_LINE_CONT_OK_BIT;
	else
		result |= lineContinuityRes;

	if (lineWidthRes == 0)
		result |= (deUint8)SCANRESULT_LINE_WIDTH_OK_BIT;
	else
		result |= lineWidthRes;

	return result;
}

bool LineRenderCase::checkAreaNumLines (const tcu::ConstPixelBufferAccess& access, const tcu::IVec4& area, int& messageLimitCounter, int componentNdx, const tcu::IVec2& numLines) const
{
	// Num maxima == num lines
	const tcu::ConstPixelBufferAccess	subAccess		= tcu::getSubregion(access, area.x(), area.y(), 0, area.z(), area.w(), 1);
	const tcu::IVec2					numMinimaMaxima	= getNumMinimaMaxima(subAccess, componentNdx);
	const int							numMaxima		= numMinimaMaxima.y();

	// In valid range
	if (numMaxima >= numLines.x() && numMaxima <= numLines.y())
		return true;

	if (--messageLimitCounter < 0)
		return false;

	if (area.z() == 1)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "On column " << area.x() << ", y: [" << area.y() << "," << (area.y()+area.w()) << "):\n"
				<< "\tExpected [" << numLines.x() << ", " << numLines.y() << "] lines but the number of lines in the area is " << numMaxima
			<< tcu::TestLog::EndMessage;
	else
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "On row " << area.y() << ", x: [" << area.x() << "," << (area.x()+area.z()) << "):\n"
				<< "\tExpected [" << numLines.x() << ", " << numLines.y() << "] lines but the number of lines in the area is " << numMaxima
			<< tcu::TestLog::EndMessage;

	return false;
}

tcu::IVec2 LineRenderCase::getNumMinimaMaxima (const tcu::ConstPixelBufferAccess& access, int componentNdx) const
{
	DE_ASSERT(access.getWidth() == 1 || access.getHeight() == 1);

	int previousValue	= -1;
	int previousSign	= 0;
	int numMinima		= 0;
	int numMaxima		= 0;

	for (int y = 0; y < access.getHeight(); ++y)
	for (int x = 0; x < access.getWidth(); ++x)
	{
		const int componentValue = access.getPixelInt(x, y)[componentNdx];

		if (previousValue != -1)
		{
			const int sign = (componentValue > previousValue) ? (+1) : (componentValue < previousValue) ? (-1) : (0);

			// local minima/maxima in sign changes (zero signless)
			if (sign != 0 && sign == -previousSign)
			{
				previousSign = sign;

				if (sign > 0)
					++numMinima;
				else
					++numMaxima;
			}
			else if (sign != 0 && previousSign == 0)
			{
				previousSign = sign;

				// local extreme at the start boundary
				if (sign > 0)
					++numMinima;
				else
					++numMaxima;
			}
		}

		previousValue = componentValue;
	}

	// local extreme at the end boundary
	if (previousSign > 0)
		++numMaxima;
	else if (previousSign < 0)
		++numMinima;
	else
	{
		++numMaxima;
		++numMinima;
	}

	return tcu::IVec2(numMinima, numMaxima);
}

deUint8 LineRenderCase::checkLineContinuity (const tcu::ConstPixelBufferAccess& access, const tcu::IVec2& begin, const tcu::IVec2& end, int componentNdx, int& messageLimitCounter) const
{
	bool				line					= false;
	const tcu::IVec2	advance					= (begin.x() == end.x()) ? (tcu::IVec2(0, 1)) : (tcu::IVec2(1, 0));
	int					missedPixels			= 0;
	int					totalPixels				= 0;
	deUint8				errorMask				= 0;

	for (tcu::IVec2 cursor = begin; cursor != end; cursor += advance)
	{
		const bool hit = (access.getPixelInt(cursor.x(), cursor.y())[componentNdx] != 0);

		if (hit)
			line = true;
		else if (line && !hit)
		{
			// non-continuous line detected
			const tcu::IVec2 advanceNeighbor	= tcu::IVec2(1, 1) - advance;
			const tcu::IVec2 cursorNeighborPos	= cursor + advanceNeighbor;
			const tcu::IVec2 cursorNeighborNeg	= cursor - advanceNeighbor;
			// hw precision issues may lead to a line being non-straight -> check neighboring pixels
			if ((access.getPixelInt(cursorNeighborPos.x(), cursorNeighborPos.y())[componentNdx] == 0) && (access.getPixelInt(cursorNeighborNeg.x(), cursorNeighborNeg.y())[componentNdx] == 0))
				++missedPixels;
		}
		++totalPixels;
	}

	if (missedPixels > 0)
	{
		if (--messageLimitCounter >= 0)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Found non-continuous " << ((advance.x() == 1)  ? ("horizontal") : ("vertical")) << " line near " << begin << ". "
				<< "Missed pixels: " << missedPixels
				<< tcu::TestLog::EndMessage;
		}
		// allow 10% missing pixels for warning
		if (missedPixels <= deRoundFloatToInt32((float)totalPixels * 0.1f))
			errorMask = SCANRESULT_LINE_CONT_WARN_BIT;
		else
			errorMask =  SCANRESULT_LINE_CONT_ERR_BIT;
	}

	return errorMask;
}

deUint8 LineRenderCase::checkLineWidths (const tcu::ConstPixelBufferAccess& access, const tcu::IVec2& begin, const tcu::IVec2& end, int componentNdx, int& messageLimitCounter) const
{
	const bool			multisample				= m_context.getRenderTarget().getNumSamples() > 1;
	const int			lineRenderWidth			= (m_isWideLineCase) ? (m_wideLineLineWidth) : 1;
	const tcu::IVec2	lineWidthRange			= (multisample)
													? (tcu::IVec2(lineRenderWidth, lineRenderWidth+1))	// multisampled "smooth" lines may spread to neighboring pixel
													: (tcu::IVec2(lineRenderWidth, lineRenderWidth));
	const tcu::IVec2	relaxedLineWidthRange	= (tcu::IVec2(lineRenderWidth-1, lineRenderWidth+1));

	int					lineWidth				= 0;
	bool				bboxLimitedLine			= false;
	deUint8				errorMask				= 0;

	const tcu::IVec2	advance					= (begin.x() == end.x()) ? (tcu::IVec2(0, 1)) : (tcu::IVec2(1, 0));

	// fragments before begin?
	if (access.getPixelInt(begin.x(), begin.y())[componentNdx] != 0)
	{
		bboxLimitedLine = true;

		for (tcu::IVec2 cursor = begin - advance;; cursor -= advance)
		{
			if (cursor.x() < 0 || cursor.y() < 0)
			{
				break;
			}
			else if (access.getPixelInt(cursor.x(), cursor.y())[componentNdx] != 0)
			{
				++lineWidth;
			}
			else
				break;
		}
	}

	for (tcu::IVec2 cursor = begin; cursor != end; cursor += advance)
	{
		const bool hit = (access.getPixelInt(cursor.x(), cursor.y())[componentNdx] != 0);

		if (hit)
			++lineWidth;
		else if (lineWidth)
		{
			// Line is allowed to be be thinner if it borders the bbox boundary (since part of the line might have been discarded).
			const bool incorrectLineWidth = (lineWidth < lineWidthRange.x() && !bboxLimitedLine) || (lineWidth > lineWidthRange.y());

			if (incorrectLineWidth)
			{
				const bool incorrectRelaxedLineWidth = (lineWidth < relaxedLineWidthRange.x() && !bboxLimitedLine) || (lineWidth > relaxedLineWidthRange.y());

				if (incorrectRelaxedLineWidth)
					errorMask |= SCANRESULT_LINE_WIDTH_ERR_BIT;
				else
					errorMask |= SCANRESULT_LINE_WIDTH_WARN_BIT;

				printLineWidthError(cursor, lineWidth, lineWidthRange, advance.x() == 0, messageLimitCounter);
			}

			lineWidth = 0;
			bboxLimitedLine = false;
		}
	}

	// fragments after end?
	if (lineWidth)
	{
		for (tcu::IVec2 cursor = end;; cursor += advance)
		{
			if (cursor.x() >= access.getWidth() || cursor.y() >= access.getHeight())
			{
				if (lineWidth > lineWidthRange.y())
				{
					if (lineWidth > relaxedLineWidthRange.y())
						errorMask |= SCANRESULT_LINE_WIDTH_ERR_BIT;
					else
						errorMask |= SCANRESULT_LINE_WIDTH_WARN_BIT;

					printLineWidthError(cursor, lineWidth, lineWidthRange, advance.x() == 0, messageLimitCounter);
				}

				break;
			}
			else if (access.getPixelInt(cursor.x(), cursor.y())[componentNdx] != 0)
			{
				++lineWidth;
			}
			else if (lineWidth)
			{
				// only check that line width is not larger than expected. Line width may be smaller
				// since the scanning 'cursor' is now outside the bounding box.
				const bool incorrectLineWidth = (lineWidth > lineWidthRange.y());

				if (incorrectLineWidth)
				{
					const bool incorrectRelaxedLineWidth = (lineWidth > relaxedLineWidthRange.y());

					if (incorrectRelaxedLineWidth)
						errorMask |= SCANRESULT_LINE_WIDTH_ERR_BIT;
					else
						errorMask |= SCANRESULT_LINE_WIDTH_WARN_BIT;

					printLineWidthError(cursor, lineWidth, lineWidthRange, advance.x() == 0, messageLimitCounter);
				}

				lineWidth = 0;
			}
		}
	}

	return errorMask;
}

void LineRenderCase::printLineWidthError (const tcu::IVec2& pos, int detectedLineWidth, const tcu::IVec2& lineWidthRange, bool isHorizontal, int& messageLimitCounter) const
{
	if (--messageLimitCounter < 0)
		return;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Found incorrect line width near " << pos << ": (" << ((isHorizontal) ? ("horizontal") : ("vertical")) << " line)\n"
			<< "\tExpected line width in range [" << lineWidthRange.x() << ", " << lineWidthRange.y() << "] but found " << detectedLineWidth
		<< tcu::TestLog::EndMessage;
}

class PointRenderCase : public BBoxRenderCase
{
public:
	enum
	{
		POINTFLAG_WIDE = 1u << FLAGBIT_USER_BIT,	//!< use wide points
	};
	struct GeneratedPoint
	{
		tcu::Vec2	center;
		int			size;
		bool		even;
	};

							PointRenderCase					(Context& context, const char* name, const char* description, deUint32 flags);
							~PointRenderCase				(void);

private:
	enum ResultPointType
	{
		POINT_FULL = 0,
		POINT_PARTIAL
	};

	void					init							(void);
	void					deinit							(void);

	std::string				genVertexSource					(void) const;
	std::string				genFragmentSource				(void) const;
	std::string				genTessellationControlSource	(void) const;
	std::string				genTessellationEvaluationSource	(void) const;
	std::string				genGeometrySource				(void) const;

	IterationConfig			generateConfig					(int iteration, const tcu::IVec2& renderTargetSize) const;
	void					generateAttributeData			(void);
	void					getAttributeData				(std::vector<tcu::Vec4>& data) const;
	void					renderTestPattern				(const IterationConfig& config);
	void					verifyRenderResult				(const IterationConfig& config);

	void					genReferencePointData			(const IterationConfig& config, std::vector<GeneratedPoint>& data) const;
	bool					verifyNarrowPointPattern		(const tcu::Surface& viewport, const std::vector<GeneratedPoint>& refPoints, const ProjectedBBox& bbox, int& logFloodCounter);
	bool					verifyWidePointPattern			(const tcu::Surface& viewport, const std::vector<GeneratedPoint>& refPoints, const ProjectedBBox& bbox, int& logFloodCounter);
	bool					verifyWidePoint					(const tcu::Surface& viewport, const GeneratedPoint& refPoint, const ProjectedBBox& bbox, ResultPointType pointType, int& logFloodCounter);
	bool					verifyWidePointAt				(const tcu::IVec2& pointPos, const tcu::Surface& viewport, const GeneratedPoint& refPoint, const tcu::IVec4& bbox, ResultPointType pointType, int componentNdx, int& logFloodCounter);
	tcu::IVec2				scanPointWidthAt				(const tcu::IVec2& pointPos, const tcu::Surface& viewport, int expectedPointSize, int componentNdx) const;

	const int				m_numStripes;
	const bool				m_isWidePointCase;
	std::vector<tcu::Vec4>	m_attribData;
};

PointRenderCase::PointRenderCase (Context& context, const char* name, const char* description, deUint32 flags)
	: BBoxRenderCase	(context, name, description, 12, flags)
	, m_numStripes		(4)
	, m_isWidePointCase	((flags & POINTFLAG_WIDE) != 0)
{
}

PointRenderCase::~PointRenderCase (void)
{
}

void PointRenderCase::init (void)
{
	if (m_isWidePointCase)
	{
		// extensions
		if (m_hasGeometryStage && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_point_size"))
			throw tcu::NotSupportedError("Test requires GL_EXT_geometry_point_size extension");
		if (m_hasTessellationStage && !m_hasGeometryStage && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_point_size"))
			throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_point_size extension");

		// point size range
		{
			glw::GLfloat pointSizeRange[2] = {0.0f, 0.0f};
			m_context.getRenderContext().getFunctions().getFloatv(GL_ALIASED_POINT_SIZE_RANGE, pointSizeRange);

			if (pointSizeRange[1] < 5.0f)
				throw tcu::NotSupportedError("Test requires point size 5.0");
		}
	}

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Rendering point pattern to " << ((m_renderTarget == RENDERTARGET_DEFAULT) ? ("default frame buffer") : ("fbo")) << ".\n"
		<< "Half of the points are green, half blue. Using additive blending.\n"
		<< "Points are in random order, varying pattern size and location for each iteration.\n"
		<< "Marking all discardable fragments (fragments outside the bounding box) with a fully saturated red channel."
		<< tcu::TestLog::EndMessage;

	generateAttributeData();

	BBoxRenderCase::init();
}

void PointRenderCase::deinit (void)
{
	// clear data
	m_attribData = std::vector<tcu::Vec4>();

	// deinit parent
	BBoxRenderCase::deinit();
}

std::string PointRenderCase::genVertexSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n"
			"in highp vec4 a_color;\n"
			"out highp vec4 vtx_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"\n";
	if (!m_hasTessellationStage)
	{
		DE_ASSERT(m_useGlobalState);
		buf <<	"uniform highp vec4 u_primitiveBBoxMin;\n"
				"uniform highp vec4 u_primitiveBBoxMax;\n"
				"\n"
				"flat out highp float v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize;\n"
				"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin;\n"
				"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax;\n"
				"\n";
	}

	buf <<	"void main()\n"
			"{\n"
			"	highp vec2 patternOffset = u_posScale.xy;\n"
			"	highp vec2 patternScale = u_posScale.zw;\n"
			"	highp float pointSize = "
					<< ((m_isWidePointCase && !m_hasTessellationStage && !m_hasGeometryStage) ? ("(a_color.g > 0.0) ? (5.0) : (3.0)") : ("1.0"))
					<< ";\n"
		<<	"	gl_Position = vec4(a_position.xy * patternScale + patternOffset, a_position.z, a_position.w);\n"
			"	gl_PointSize = pointSize;\n"
			"	vtx_color = a_color;\n";

	if (!m_hasTessellationStage)
	{
		DE_ASSERT(m_useGlobalState);
		buf <<	"\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_expansionSize = pointSize;\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin =\n"
				"	    min(vec3(u_primitiveBBoxMin.x, u_primitiveBBoxMin.y, u_primitiveBBoxMin.z) / u_primitiveBBoxMin.w,\n"
				"	        vec3(u_primitiveBBoxMin.x, u_primitiveBBoxMin.y, u_primitiveBBoxMin.z) / u_primitiveBBoxMax.w);\n"
				"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax =\n"
				"	    min(vec3(u_primitiveBBoxMax.x, u_primitiveBBoxMax.y, u_primitiveBBoxMax.z) / u_primitiveBBoxMin.w,\n"
				"	        vec3(u_primitiveBBoxMax.x, u_primitiveBBoxMax.y, u_primitiveBBoxMax.z) / u_primitiveBBoxMax.w);\n";
	}

	buf <<	"}\n";
	return buf.str();
}

std::string PointRenderCase::genFragmentSource (void) const
{
	const char* const	colorInputName = (m_hasGeometryStage) ? ("geo_color") : (m_hasTessellationStage) ? ("tess_color") : ("vtx_color");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in mediump vec4 " << colorInputName << ";\n"
			"layout(location = 0) out mediump vec4 o_color;\n"
		<<	genShaderFunction(SHADER_FUNC_INSIDE_BBOX)
		<<	"\n"
			"void main()\n"
			"{\n"
			"	mediump vec4 baseColor = " << colorInputName << ";\n"
			"	mediump float redChannel;\n"
			"	if (fragmentInsideTheBBox(gl_FragCoord.z))\n"
			"		redChannel = 0.0;\n"
			"	else\n"
			"		redChannel = 1.0;\n"
			"	o_color = vec4(redChannel, baseColor.g, baseColor.b, baseColor.a);\n"
			"}\n";

	return buf.str();
}

std::string PointRenderCase::genTessellationControlSource (void) const
{
	const bool			tessellationWidePoints = (m_isWidePointCase) && (!m_hasGeometryStage);
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"${PRIMITIVE_BOUNDING_BOX_REQUIRE}\n"
		<<	((tessellationWidePoints) ? ("${TESSELLATION_POINT_SIZE_REQUIRE}\n") : (""))
		<<	"layout(vertices=1) out;"
			"\n"
			"in highp vec4 vtx_color[];\n"
			"out highp vec4 tess_ctrl_color[];\n"
			"uniform highp float u_tessellationLevel;\n"
			"uniform highp vec4 u_posScale;\n";

	if (!m_calcPerPrimitiveBBox)
	{
		buf <<	"uniform highp vec4 u_primitiveBBoxMin;\n"
				"uniform highp vec4 u_primitiveBBoxMax;\n";
	}

	buf <<	"patch out highp vec3 vp_bbox_clipMin;\n"
			"patch out highp vec3 vp_bbox_clipMax;\n";

	if (m_calcPerPrimitiveBBox)
	{
		buf <<	"\n";
		if (m_hasGeometryStage)
			buf << genShaderFunction(SHADER_FUNC_MIRROR_X);
		buf << genShaderFunction(SHADER_FUNC_MIRROR_Y);

		buf <<	"vec4 transformVec(in highp vec4 p)\n"
				"{\n"
				"	return " << ((m_hasGeometryStage) ? ("mirrorX(mirrorY(p))") : ("mirrorY(p)")) << ";\n"
				"}\n";
	}

	buf <<	"\n"
			"void main()\n"
			"{\n"
			"	// convert to nonsensical coordinates, just in case\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position.wzxy;\n"
			"	tess_ctrl_color[gl_InvocationID] = vtx_color[gl_InvocationID];\n"
			"\n"
			"	gl_TessLevelOuter[0] = u_tessellationLevel;\n"
			"	gl_TessLevelOuter[1] = u_tessellationLevel;\n"
			"	gl_TessLevelOuter[2] = u_tessellationLevel;\n"
			"	gl_TessLevelOuter[3] = u_tessellationLevel;\n"
			"	gl_TessLevelInner[0] = 0.8; // will be rounded up to 1\n"
			"	gl_TessLevelInner[1] = 0.8; // will be rounded up to 1\n";

	if (m_calcPerPrimitiveBBox)
	{
		buf <<	"\n";

		if (m_hasGeometryStage)
			buf <<	"	const vec2 minExpansion = vec2(0.07 + 0.05, 0.07 + 0.02); // eval and geometry shader\n"
					"	const vec2 maxExpansion = vec2(0.07 + 0.05, 0.07 + 0.03); // eval and geometry shader\n";
		else
			buf <<	"	const vec2 minExpansion = vec2(0.07, 0.07); // eval shader\n"
					"	const vec2 maxExpansion = vec2(0.07, 0.07); // eval shader\n";

		buf <<	"	highp vec2 patternScale = u_posScale.zw;\n"
				"	highp vec4 bboxMin = transformVec(gl_in[0].gl_Position) - vec4(minExpansion * patternScale, 0.0, 0.0);\n"
				"	highp vec4 bboxMax = transformVec(gl_in[0].gl_Position) + vec4(maxExpansion * patternScale, 0.0, 0.0);\n";
	}
	else
	{
		buf <<	"\n"
				"	highp vec4 bboxMin = u_primitiveBBoxMin;\n"
				"	highp vec4 bboxMax = u_primitiveBBoxMax;\n";
	}
	if (!m_useGlobalState)
		buf <<	"\n"
				"	${PRIM_GL_BOUNDING_BOX}[0] = bboxMin;\n"
				"	${PRIM_GL_BOUNDING_BOX}[1] = bboxMax;\n";

	buf <<	"	vp_bbox_clipMin = min(vec3(bboxMin.x, bboxMin.y, bboxMin.z) / bboxMin.w,\n"
			"	                      vec3(bboxMin.x, bboxMin.y, bboxMin.z) / bboxMax.w);\n"
			"	vp_bbox_clipMax = max(vec3(bboxMax.x, bboxMax.y, bboxMax.z) / bboxMin.w,\n"
			"	                      vec3(bboxMax.x, bboxMax.y, bboxMax.z) / bboxMax.w);\n"
			"}\n";

	return buf.str();
}

std::string PointRenderCase::genTessellationEvaluationSource (void) const
{
	const bool			tessellationWidePoints = (m_isWidePointCase) && (!m_hasGeometryStage);
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
		<<	((tessellationWidePoints) ? ("${TESSELLATION_POINT_SIZE_REQUIRE}\n") : (""))
		<<	"layout(quads, point_mode) in;"
			"\n"
			"in highp vec4 tess_ctrl_color[];\n"
			"out highp vec4 tess_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"\n"
			"patch in highp vec3 vp_bbox_clipMin;\n"
			"patch in highp vec3 vp_bbox_clipMax;\n"
		<<	((!m_hasGeometryStage) ? ("flat out highp float v_bbox_expansionSize;\n") : (""))
		<<	"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin;\n"
			"flat out highp vec3 v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax;\n"
			"\n"
		<<	genShaderFunction(SHADER_FUNC_MIRROR_Y)
		<<	"void main()\n"
			"{\n"
			"	// non-trivial tessellation evaluation shader, convert from nonsensical coords, flip vertically\n"
			"	highp vec2 patternScale = u_posScale.zw;\n"
			"	highp vec4 offset = vec4((gl_TessCoord.xy * 2.0 - vec2(1.0)) * 0.07 * patternScale, 0.0, 0.0);\n"
			"	highp float pointSize = " << ((tessellationWidePoints) ? ("(tess_ctrl_color[0].g > 0.0) ? (5.0) : (3.0)") : ("1.0")) << ";\n"
			"	gl_Position = mirrorY(gl_in[0].gl_Position.zwyx + offset);\n";

	if (tessellationWidePoints)
		buf << "	gl_PointSize = pointSize;\n";

	buf <<	"	tess_color = tess_ctrl_color[0];\n"
		<<	((!m_hasGeometryStage) ? ("v_bbox_expansionSize = pointSize;\n") : (""))
		<<	"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMin = vp_bbox_clipMin;\n"
			"	v_" << (m_hasGeometryStage ? "geo_" : "") << "bbox_clipMax = vp_bbox_clipMax;\n"
			"}\n";

	return buf.str();
}

std::string PointRenderCase::genGeometrySource (void) const
{
	const char* const	colorInputName = (m_hasTessellationStage) ? ("tess_color") : ("vtx_color");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GEOMETRY_SHADER_REQUIRE}\n"
		<<	((m_isWidePointCase) ? ("${GEOMETRY_POINT_SIZE}\n") : (""))
		<<	"layout(points) in;\n"
			"layout(max_vertices=3, points) out;\n"
			"\n"
			"in highp vec4 " << colorInputName << "[1];\n"
			"out highp vec4 geo_color;\n"
			"uniform highp vec4 u_posScale;\n"
			"\n"
			"flat in highp vec3 v_geo_bbox_clipMin[1];\n"
			"flat in highp vec3 v_geo_bbox_clipMax[1];\n"
			"flat out highp vec3 v_bbox_clipMin;\n"
			"flat out highp vec3 v_bbox_clipMax;\n"
			"flat out highp float v_bbox_expansionSize;\n"
			"\n"
		<<	genShaderFunction(SHADER_FUNC_MIRROR_X)
		<<	"\n"
			"void main()\n"
			"{\n"
			"	// Non-trivial geometry shader: 1-to-3 amplification, mirror horizontally\n"
			"	highp vec4 p0 = mirrorX(gl_in[0].gl_Position);\n"
			"	highp vec4 pointColor = " << colorInputName << "[0];\n"
			"	highp vec2 patternScale = u_posScale.zw;\n"
			"	highp float pointSize = "
				<< (m_isWidePointCase ? ("(pointColor.g > 0.0) ? (5.0) : (3.0)") : ("1.0"))
				<< ";\n"
			"\n"
			"	highp vec4 offsets[3] =\n"
			"		vec4[3](\n"
			"			vec4( 0.05 * patternScale.x, 0.03 * patternScale.y, 0.0, 0.0),\n"
			"			vec4(-0.01 * patternScale.x,-0.02 * patternScale.y, 0.0, 0.0),\n"
			"			vec4(-0.05 * patternScale.x, 0.02 * patternScale.y, 0.0, 0.0)\n"
			"		);\n"
			"	for (int ndx = 0; ndx < 3; ++ndx)\n"
			"	{\n"
			"		gl_Position = p0 + offsets[ndx];\n";

	if (m_isWidePointCase)
		buf <<	"		gl_PointSize = pointSize;\n";

	buf <<	"		v_bbox_clipMin = v_geo_bbox_clipMin[0];\n"
			"		v_bbox_clipMax = v_geo_bbox_clipMax[0];\n"
			"		v_bbox_expansionSize = pointSize;\n"
			"		geo_color = pointColor;\n"
			"		EmitVertex();\n"
			"	}\n"
			"}\n";

	return buf.str();
}

PointRenderCase::IterationConfig PointRenderCase::generateConfig (int iteration, const tcu::IVec2& renderTargetSize) const
{
	IterationConfig config = generateRandomConfig(0xDEDEDEu * (deUint32)iteration, renderTargetSize);

	// equal or larger -> expand according to shader expansion
	if (m_bboxSize == BBOXSIZE_EQUAL || m_bboxSize == BBOXSIZE_LARGER)
	{
		const tcu::Vec2 patternScale = config.patternSize;

		if (m_hasTessellationStage)
		{
			config.bbox.min -= tcu::Vec4(0.07f * patternScale.x(), 0.07f * patternScale.y(), 0.0f, 0.0f);
			config.bbox.max += tcu::Vec4(0.07f * patternScale.x(), 0.07f * patternScale.y(), 0.0f, 0.0f);
		}
		if (m_hasGeometryStage)
		{
			config.bbox.min -= tcu::Vec4(0.05f * patternScale.x(), 0.02f * patternScale.y(), 0.0f, 0.0f);
			config.bbox.max += tcu::Vec4(0.05f * patternScale.x(), 0.03f * patternScale.y(), 0.0f, 0.0f);
		}
	}

	return config;
}

void PointRenderCase::generateAttributeData (void)
{
	const tcu::Vec4		green		(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4		blue		(0.0f, 0.0f, 1.0f, 1.0f);
	std::vector<int>	cellOrder	(m_numStripes * m_numStripes * 2);
	de::Random			rnd			(0xDE22446);

	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
		cellOrder[ndx] = ndx;
	rnd.shuffle(cellOrder.begin(), cellOrder.end());

	m_attribData.resize(cellOrder.size() * 2);
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
	{
		const int pointID		= cellOrder[ndx];
		const int direction		= pointID & 0x01;
		const int majorCoord	= (pointID >> 1) / m_numStripes;
		const int minorCoord	= (pointID >> 1) % m_numStripes;

		if (direction)
		{
			m_attribData[ndx * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(float(minorCoord) / float(m_numStripes), float(majorCoord) / float(m_numStripes), 0.0f, 1.0f);
			m_attribData[ndx * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = green;
		}
		else
		{
			m_attribData[ndx * VA_NUM_ATTRIB_VECS + VA_POS_VEC_NDX] = tcu::Vec4(((float)majorCoord + 0.5f) / float(m_numStripes), ((float)minorCoord + 0.5f) / float(m_numStripes), 0.0f, 1.0f);
			m_attribData[ndx * VA_NUM_ATTRIB_VECS + VA_COL_VEC_NDX] = blue;
		}
	}
}

void PointRenderCase::getAttributeData (std::vector<tcu::Vec4>& data) const
{
	data = m_attribData;
}

void PointRenderCase::renderTestPattern (const IterationConfig& config)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	setupRender(config);

	if (m_hasTessellationStage)
	{
		const glw::GLint	tessLevelPos	= gl.getUniformLocation(m_program->getProgram(), "u_tessellationLevel");
		const glw::GLfloat	tessLevel		= 0.8f; // will be rounded up

		TCU_CHECK(tessLevelPos != -1);

		m_testCtx.getLog() << tcu::TestLog::Message << "u_tessellationLevel = " << tessLevel << tcu::TestLog::EndMessage;

		gl.uniform1f(tessLevelPos, tessLevel);
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "patch param");
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering pattern." << tcu::TestLog::EndMessage;

	gl.enable(GL_BLEND);
	gl.blendFunc(GL_ONE, GL_ONE);
	gl.blendEquation(GL_FUNC_ADD);

	gl.drawArrays((m_hasTessellationStage) ? (GL_PATCHES) : (GL_POINTS), 0, m_numStripes * m_numStripes * 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");
}

void PointRenderCase::verifyRenderResult (const IterationConfig& config)
{
	const glw::Functions&		gl						= m_context.getRenderContext().getFunctions();
	const ProjectedBBox			projectedBBox			= projectBoundingBox(config.bbox);
	const tcu::IVec4			viewportBBoxArea		= getViewportBoundingBoxArea(projectedBBox, config.viewportSize);

	tcu::Surface				viewportSurface			(config.viewportSize.x(), config.viewportSize.y());
	int							logFloodCounter			= 8;
	bool						anyError;
	std::vector<GeneratedPoint>	refPoints;

	if (!m_calcPerPrimitiveBBox)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Projected bounding box: (clip space)\n"
				<< "\tx: [" << projectedBBox.min.x() << "," << projectedBBox.max.x() << "]\n"
				<< "\ty: [" << projectedBBox.min.y() << "," << projectedBBox.max.y() << "]\n"
				<< "\tz: [" << projectedBBox.min.z() << "," << projectedBBox.max.z() << "]\n"
			<< "In viewport coordinates:\n"
				<< "\tx: [" << viewportBBoxArea.x() << ", " << viewportBBoxArea.z() << "]\n"
				<< "\ty: [" << viewportBBoxArea.y() << ", " << viewportBBoxArea.w() << "]\n"
			<< "Verifying render results within the bounding box:\n"
			<< tcu::TestLog::EndMessage;
	else
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying render result:"
			<< tcu::TestLog::EndMessage;

	if (m_fbo)
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, **m_fbo);
	glu::readPixels(m_context.getRenderContext(), config.viewportPos.x(), config.viewportPos.y(), viewportSurface.getAccess());

	genReferencePointData(config, refPoints);

	if (m_isWidePointCase)
		anyError = verifyWidePointPattern(viewportSurface, refPoints, projectedBBox, logFloodCounter);
	else
		anyError = verifyNarrowPointPattern(viewportSurface, refPoints, projectedBBox, logFloodCounter);

	if (anyError)
	{
		if (logFloodCounter < 0)
			m_testCtx.getLog() << tcu::TestLog::Message << "Omitted " << (-logFloodCounter) << " error descriptions." << tcu::TestLog::EndMessage;

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewportSurface.getAccess())
			<< tcu::TestLog::EndImageSet;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Result image ok."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewportSurface.getAccess())
			<< tcu::TestLog::EndImageSet;
	}
}

struct PointSorter
{
	bool operator() (const PointRenderCase::GeneratedPoint& a, const PointRenderCase::GeneratedPoint& b) const
	{
		if (a.center.y() < b.center.y())
			return true;
		else if (a.center.y() > b.center.y())
			return false;
		else
			return (a.center.x() < b.center.x());
	}
};

void PointRenderCase::genReferencePointData (const IterationConfig& config, std::vector<GeneratedPoint>& data) const
{
	std::vector<GeneratedPoint> currentPoints;

	// vertex shader
	currentPoints.resize(m_attribData.size() / 2);
	for (int ndx = 0; ndx < (int)currentPoints.size(); ++ndx)
	{
		currentPoints[ndx].center	= m_attribData[ndx*2].swizzle(0, 1);
		currentPoints[ndx].even		= (m_attribData[ndx*2 + 1].y() == 1.0f); // is green
		currentPoints[ndx].size		= ((m_isWidePointCase) ? ((currentPoints[ndx].even) ? 5 : 3) : 1);
	}

	// tessellation
	if (m_hasTessellationStage)
	{
		std::vector<GeneratedPoint> tessellatedPoints;

		tessellatedPoints.resize(currentPoints.size() * 4);
		for (int ndx = 0; ndx < (int)currentPoints.size(); ++ndx)
		{
			const tcu::Vec2 position = tcu::Vec2(currentPoints[ndx].center.x(), 1.0f - currentPoints[ndx].center.y()); // mirror Y

			tessellatedPoints[4 * ndx + 0].center	= position + tcu::Vec2(-0.07f, -0.07f);
			tessellatedPoints[4 * ndx + 0].size		= currentPoints[ndx].size;
			tessellatedPoints[4 * ndx + 0].even		= currentPoints[ndx].even;

			tessellatedPoints[4 * ndx + 1].center	= position + tcu::Vec2( 0.07f, -0.07f);
			tessellatedPoints[4 * ndx + 1].size		= currentPoints[ndx].size;
			tessellatedPoints[4 * ndx + 1].even		= currentPoints[ndx].even;

			tessellatedPoints[4 * ndx + 2].center	= position + tcu::Vec2( 0.07f,  0.07f);
			tessellatedPoints[4 * ndx + 2].size		= currentPoints[ndx].size;
			tessellatedPoints[4 * ndx + 2].even		= currentPoints[ndx].even;

			tessellatedPoints[4 * ndx + 3].center	= position + tcu::Vec2(-0.07f,  0.07f);
			tessellatedPoints[4 * ndx + 3].size		= currentPoints[ndx].size;
			tessellatedPoints[4 * ndx + 3].even		= currentPoints[ndx].even;
		}

		currentPoints.swap(tessellatedPoints);
	}

	// geometry
	if (m_hasGeometryStage)
	{
		std::vector<GeneratedPoint> geometryShadedPoints;

		geometryShadedPoints.resize(currentPoints.size() * 3);
		for (int ndx = 0; ndx < (int)currentPoints.size(); ++ndx)
		{
			const tcu::Vec2 position = tcu::Vec2(1.0f - currentPoints[ndx].center.x(), currentPoints[ndx].center.y()); // mirror X

			geometryShadedPoints[3 * ndx + 0].center	= position + tcu::Vec2( 0.05f,  0.03f);
			geometryShadedPoints[3 * ndx + 0].size		= currentPoints[ndx].size;
			geometryShadedPoints[3 * ndx + 0].even		= currentPoints[ndx].even;

			geometryShadedPoints[3 * ndx + 1].center	= position + tcu::Vec2(-0.01f, -0.02f);
			geometryShadedPoints[3 * ndx + 1].size		= currentPoints[ndx].size;
			geometryShadedPoints[3 * ndx + 1].even		= currentPoints[ndx].even;

			geometryShadedPoints[3 * ndx + 2].center	= position + tcu::Vec2(-0.05f,  0.02f);
			geometryShadedPoints[3 * ndx + 2].size		= currentPoints[ndx].size;
			geometryShadedPoints[3 * ndx + 2].even		= currentPoints[ndx].even;
		}

		currentPoints.swap(geometryShadedPoints);
	}

	// sort from left to right, top to bottom
	std::sort(currentPoints.begin(), currentPoints.end(), PointSorter());

	// map to pattern space
	for (int ndx = 0; ndx < (int)currentPoints.size(); ++ndx)
		currentPoints[ndx].center = currentPoints[ndx].center * config.patternSize + config.patternPos;

	currentPoints.swap(data);
}

bool PointRenderCase::verifyNarrowPointPattern (const tcu::Surface& viewport, const std::vector<GeneratedPoint>& refPoints, const ProjectedBBox& bbox, int& logFloodCounter)
{
	bool anyError = false;

	// check that there is something near each sample
	for (int pointNdx = 0; pointNdx < (int)refPoints.size(); ++pointNdx)
	{
		const float				epsilon		= 1.0e-6f;
		const GeneratedPoint&	refPoint	= refPoints[pointNdx];

		// skip points not in the the bbox, treat boundary as "in"
		if (refPoint.center.x() < bbox.min.x() - epsilon ||
			refPoint.center.y() < bbox.min.y() - epsilon ||
			refPoint.center.x() > bbox.max.x() + epsilon ||
			refPoint.center.y() > bbox.max.y() + epsilon)
			continue;
		else
		{
			// transform to viewport coords
			const tcu::IVec2 pixelCenter(deRoundFloatToInt32((refPoint.center.x() * 0.5f + 0.5f) * (float)viewport.getWidth()),
										 deRoundFloatToInt32((refPoint.center.y() * 0.5f + 0.5f) * (float)viewport.getHeight()));

			// find rasterized point in the result
			if (pixelCenter.x() < 1 || pixelCenter.y() < 1 || pixelCenter.x() >= viewport.getWidth()-1 || pixelCenter.y() >= viewport.getHeight()-1)
			{
				// viewport boundary, assume point is fine
			}
			else
			{
				const int	componentNdx	= (refPoint.even) ? (1) : (2); // analyze either green or blue channel
				bool		foundResult		= false;

				// check neighborhood
				for (int dy = -1; dy < 2 && !foundResult; ++dy)
				for (int dx = -1; dx < 2 && !foundResult; ++dx)
				{
					const tcu::IVec2	testPos	(pixelCenter.x() + dx, pixelCenter.y() + dy);
					const tcu::RGBA		color	= viewport.getPixel(testPos.x(), testPos.y());

					if (color.toIVec()[componentNdx] > 0)
						foundResult = true;
				}

				if (!foundResult)
				{
					anyError = true;

					if (--logFloodCounter >= 0)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "Missing point near " << pixelCenter << ", vertex coordinates=" << refPoint.center.swizzle(0, 1) << "."
							<< tcu::TestLog::EndMessage;
					}
				}
			}
		}
	}

	return anyError;
}

bool PointRenderCase::verifyWidePointPattern (const tcu::Surface& viewport, const std::vector<GeneratedPoint>& refPoints, const ProjectedBBox& bbox, int& logFloodCounter)
{
	bool anyError = false;

	// check that there is something near each sample
	for (int pointNdx = 0; pointNdx < (int)refPoints.size(); ++pointNdx)
	{
		const GeneratedPoint& refPoint = refPoints[pointNdx];

		if (refPoint.center.x() >= bbox.min.x() &&
			refPoint.center.y() >= bbox.min.y() &&
			refPoint.center.x() <= bbox.max.x() &&
			refPoint.center.y() <= bbox.max.y())
		{
			// point fully in the bounding box
			anyError |= !verifyWidePoint(viewport, refPoint, bbox, POINT_FULL, logFloodCounter);
		}
		else if (refPoint.center.x() >= bbox.min.x() + (float)refPoint.size / 2.0f &&
				 refPoint.center.y() >= bbox.min.y() - (float)refPoint.size / 2.0f &&
				 refPoint.center.x() <= bbox.max.x() + (float)refPoint.size / 2.0f &&
				 refPoint.center.y() <= bbox.max.y() - (float)refPoint.size / 2.0f)
		{
			// point leaks into bounding box
			anyError |= !verifyWidePoint(viewport, refPoint, bbox, POINT_PARTIAL, logFloodCounter);
		}
	}

	return anyError;
}

bool PointRenderCase::verifyWidePoint (const tcu::Surface& viewport, const GeneratedPoint& refPoint, const ProjectedBBox& bbox, ResultPointType pointType, int& logFloodCounter)
{
	const int			componentNdx		= (refPoint.even) ? (1) : (2);
	const int			halfPointSizeCeil	= (refPoint.size + 1) / 2;
	const int			halfPointSizeFloor	= (refPoint.size + 1) / 2;
	const tcu::IVec4	viewportBBoxArea	= getViewportBoundingBoxArea(bbox, tcu::IVec2(viewport.getWidth(), viewport.getHeight()), (float)refPoint.size);
	const tcu::IVec4	verificationArea	= tcu::IVec4(de::max(viewportBBoxArea.x(), 0),
														 de::max(viewportBBoxArea.y(), 0),
														 de::min(viewportBBoxArea.z(), viewport.getWidth()),
														 de::min(viewportBBoxArea.w(), viewport.getHeight()));
	const tcu::IVec2	pointPos			= tcu::IVec2(deRoundFloatToInt32((refPoint.center.x()*0.5f + 0.5f) * (float)viewport.getWidth()),
														 deRoundFloatToInt32((refPoint.center.y()*0.5f + 0.5f) * (float)viewport.getHeight()));

	// find any fragment within the point that is inside the bbox, start search at the center

	if (pointPos.x() >= verificationArea.x() &&
		pointPos.y() >= verificationArea.y() &&
		pointPos.x() < verificationArea.z() &&
		pointPos.y() < verificationArea.w())
	{
		if (viewport.getPixel(pointPos.x(), pointPos.y()).toIVec()[componentNdx])
			return verifyWidePointAt(pointPos, viewport, refPoint, verificationArea, pointType, componentNdx, logFloodCounter);
	}

	for (int dy = -halfPointSizeCeil; dy <= halfPointSizeCeil; ++dy)
	for (int dx = -halfPointSizeCeil; dx <= halfPointSizeCeil; ++dx)
	{
		const tcu::IVec2 testPos = pointPos + tcu::IVec2(dx, dy);

		if (dx == 0 && dy == 0)
			continue;

		if (testPos.x() >= verificationArea.x() &&
			testPos.y() >= verificationArea.y() &&
			testPos.x() < verificationArea.z() &&
			testPos.y() < verificationArea.w())
		{
			if (viewport.getPixel(testPos.x(), testPos.y()).toIVec()[componentNdx])
				return verifyWidePointAt(testPos, viewport, refPoint, verificationArea, pointType, componentNdx, logFloodCounter);
		}
	}

	// could not find point, this is only ok near boundaries
	if (pointPos.x() + halfPointSizeFloor <  verificationArea.x() - 1 ||
		pointPos.y() + halfPointSizeFloor <  verificationArea.y() - 1 ||
		pointPos.x() - halfPointSizeFloor >= verificationArea.z() - 1 ||
		pointPos.y() - halfPointSizeFloor >= verificationArea.w() - 1)
		return true;

	if (--logFloodCounter >= 0)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Missing wide point near " << pointPos << ", vertex coordinates=" << refPoint.center.swizzle(0, 1) << "."
			<< tcu::TestLog::EndMessage;
	}

	return false;
}

bool PointRenderCase::verifyWidePointAt (const tcu::IVec2& pointPos, const tcu::Surface& viewport, const GeneratedPoint& refPoint, const tcu::IVec4& bbox, ResultPointType pointType, int componentNdx, int& logFloodCounter)
{
	const int				expectedPointSize		= refPoint.size;
	bool					viewportClippedTop		= false;
	bool					viewportClippedBottom	= false;
	bool					primitiveClippedTop		= false;
	bool					primitiveClippedBottom	= false;
	std::vector<tcu::IVec2>	widthsUpwards;
	std::vector<tcu::IVec2>	widthsDownwards;
	std::vector<tcu::IVec2>	widths;

	// search upwards
	for (int y = pointPos.y();; --y)
	{
		if (y < bbox.y() || y < 0)
		{
			if (y < bbox.y())
				primitiveClippedTop = true;
			if (y < 0)
				viewportClippedTop = true;
			break;
		}
		else if (pointPos.y() - y > expectedPointSize)
		{
			// no need to go further than point height
			break;
		}
		else if (viewport.getPixel(pointPos.x(), y).toIVec()[componentNdx] == 0)
		{
			break;
		}
		else
		{
			widthsUpwards.push_back(scanPointWidthAt(tcu::IVec2(pointPos.x(), y), viewport, expectedPointSize, componentNdx));
		}
	}

	// top is clipped
	if ((viewportClippedTop || (pointType == POINT_PARTIAL && primitiveClippedTop)) && !widthsUpwards.empty())
	{
		const tcu::IVec2&	range			= widthsUpwards.back();
		const bool			squareFits		= (range.y() - range.x() + 1) >= expectedPointSize;
		const bool			widthClipped	= (pointType == POINT_PARTIAL) && (range.x() <= bbox.x() || range.y() >= bbox.z());

		if (squareFits || widthClipped)
			return true;
	}

	// and downwards
	for (int y = pointPos.y()+1;; ++y)
	{
		if (y >= bbox.w() || y >= viewport.getHeight())
		{
			if (y >= bbox.w())
				primitiveClippedBottom = true;
			if (y >= viewport.getHeight())
				viewportClippedBottom = true;
			break;
		}
		else if (y - pointPos.y() > expectedPointSize)
		{
			// no need to go further than point height
			break;
		}
		else if (viewport.getPixel(pointPos.x(), y).toIVec()[componentNdx] == 0)
		{
			break;
		}
		else
		{
			widthsDownwards.push_back(scanPointWidthAt(tcu::IVec2(pointPos.x(), y), viewport, expectedPointSize, componentNdx));
		}
	}

	// bottom is clipped
	if ((viewportClippedBottom || (pointType == POINT_PARTIAL && primitiveClippedBottom)) && !(widthsDownwards.empty() && widthsUpwards.empty()))
	{
		const tcu::IVec2&	range			= (widthsDownwards.empty()) ? (widthsUpwards.front()) : (widthsDownwards.back());
		const bool			squareFits		= (range.y() - range.x() + 1) >= expectedPointSize;
		const bool			bboxClipped		= (pointType == POINT_PARTIAL) && (range.x() <= bbox.x() || range.y() >= bbox.z()-1);
		const bool			viewportClipped	= range.x() <= 0 || range.y() >= viewport.getWidth()-1;

		if (squareFits || bboxClipped || viewportClipped)
			return true;
	}

	// would square point would fit into the rasterized area

	for (int ndx = 0; ndx < (int)widthsUpwards.size(); ++ndx)
		widths.push_back(widthsUpwards[(int)widthsUpwards.size() - ndx - 1]);
	for (int ndx = 0; ndx < (int)widthsDownwards.size(); ++ndx)
		widths.push_back(widthsDownwards[ndx]);
	DE_ASSERT(!widths.empty());

	for (int y = 0; y < (int)widths.size() - expectedPointSize + 1; ++y)
	{
		tcu::IVec2 unionRange = widths[y];

		for (int dy = 1; dy < expectedPointSize; ++dy)
		{
			unionRange.x() = de::max(unionRange.x(), widths[y+dy].x());
			unionRange.y() = de::min(unionRange.y(), widths[y+dy].y());
		}

		// would a N x N block fit here?
		{
			const bool squareFits		= (unionRange.y() - unionRange.x() + 1) >= expectedPointSize;
			const bool bboxClipped		= (pointType == POINT_PARTIAL) && (unionRange.x() <= bbox.x() || unionRange.y() >= bbox.z()-1);
			const bool viewportClipped	= unionRange.x() <= 0 || unionRange.y() >= viewport.getWidth()-1;

			if (squareFits || bboxClipped || viewportClipped)
				return true;
		}
	}

	if (--logFloodCounter >= 0)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Missing " << expectedPointSize << "x" << expectedPointSize << " point near " << pointPos << ", vertex coordinates=" << refPoint.center.swizzle(0, 1) << "."
			<< tcu::TestLog::EndMessage;
	}
	return false;
}

tcu::IVec2 PointRenderCase::scanPointWidthAt (const tcu::IVec2& pointPos, const tcu::Surface& viewport, int expectedPointSize, int componentNdx) const
{
	int minX = pointPos.x();
	int maxX = pointPos.x();

	// search horizontally for a point edges
	for (int x = pointPos.x()-1; x >= 0; --x)
	{
		if (viewport.getPixel(x, pointPos.y()).toIVec()[componentNdx] == 0)
			break;

		// no need to go further than point width
		if (pointPos.x() - x > expectedPointSize)
			break;

		minX = x;
	}
	for (int x = pointPos.x()+1; x < viewport.getWidth(); ++x)
	{
		if (viewport.getPixel(x, pointPos.y()).toIVec()[componentNdx] == 0)
			break;

		// no need to go further than point width
		if (x - pointPos.x() > expectedPointSize)
			break;

		maxX = x;
	}

	return tcu::IVec2(minX, maxX);
}

class BlitFboCase : public TestCase
{
public:
	enum RenderTarget
	{
		TARGET_DEFAULT = 0,
		TARGET_FBO,

		TARGET_LAST
	};

							BlitFboCase						(Context& context, const char* name, const char* description, RenderTarget src, RenderTarget dst);
							~BlitFboCase					(void);

private:
	enum
	{
		FBO_SIZE = 256,
	};

	struct BlitArgs
	{
		tcu::IVec4	src;
		tcu::IVec4	dst;
		tcu::Vec4	bboxMin;
		tcu::Vec4	bboxMax;
		bool		linear;
	};

	void							init					(void);
	void							deinit					(void);
	IterateResult					iterate					(void);

	void							fillSourceWithPattern	(void);
	bool							verifyImage				(const BlitArgs& args);

	const RenderTarget				m_src;
	const RenderTarget				m_dst;

	std::vector<BlitArgs>			m_iterations;
	int								m_iteration;
	de::MovePtr<glu::Framebuffer>	m_srcFbo;
	de::MovePtr<glu::Framebuffer>	m_dstFbo;
	de::MovePtr<glu::Renderbuffer>	m_srcRbo;
	de::MovePtr<glu::Renderbuffer>	m_dstRbo;
	de::MovePtr<glu::ShaderProgram>	m_program;
	de::MovePtr<glu::Buffer>		m_vbo;
};

BlitFboCase::BlitFboCase (Context& context, const char* name, const char* description, RenderTarget src, RenderTarget dst)
	: TestCase		(context, name, description)
	, m_src			(src)
	, m_dst			(dst)
	, m_iteration	(0)
{
	DE_ASSERT(src < TARGET_LAST);
	DE_ASSERT(dst < TARGET_LAST);
}

BlitFboCase::~BlitFboCase (void)
{
	deinit();
}

void BlitFboCase::init (void)
{
	const int				numIterations			= 12;
	const bool				defaultFBMultisampled	= (m_context.getRenderTarget().getNumSamples() > 1);
	const glw::Functions&	gl						= m_context.getRenderContext().getFunctions();
	de::Random				rnd						(0xABC123);

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Using BlitFramebuffer to blit area from "
			<< ((m_src == TARGET_DEFAULT) ? ("default fb") : ("fbo"))
			<< " to "
			<< ((m_dst == TARGET_DEFAULT) ? ("default fb") : ("fbo"))
			<< ".\n"
		<< "Varying blit arguments and primitive bounding box between iterations.\n"
		<< "Expecting bounding box to have no effect on blitting.\n"
		<< "Source framebuffer is filled with green-yellow grid.\n"
		<< tcu::TestLog::EndMessage;

	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_primitive_bounding_box"))
		throw tcu::NotSupportedError("Test requires GL_EXT_primitive_bounding_box extension");
	if (m_dst == TARGET_DEFAULT && defaultFBMultisampled)
		throw tcu::NotSupportedError("Test requires non-multisampled default framebuffer");

	// resources

	if (m_src == TARGET_FBO)
	{
		m_srcRbo = de::MovePtr<glu::Renderbuffer>(new glu::Renderbuffer(m_context.getRenderContext()));
		gl.bindRenderbuffer(GL_RENDERBUFFER, **m_srcRbo);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, FBO_SIZE, FBO_SIZE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "src rbo");

		m_srcFbo = de::MovePtr<glu::Framebuffer>(new glu::Framebuffer(m_context.getRenderContext()));
		gl.bindFramebuffer(GL_FRAMEBUFFER, **m_srcFbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, **m_srcRbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "src fbo");
	}

	if (m_dst == TARGET_FBO)
	{
		m_dstRbo = de::MovePtr<glu::Renderbuffer>(new glu::Renderbuffer(m_context.getRenderContext()));
		gl.bindRenderbuffer(GL_RENDERBUFFER, **m_dstRbo);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, FBO_SIZE, FBO_SIZE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "dst rbo");

		m_dstFbo = de::MovePtr<glu::Framebuffer>(new glu::Framebuffer(m_context.getRenderContext()));
		gl.bindFramebuffer(GL_FRAMEBUFFER, **m_dstFbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, **m_dstRbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "dst fbo");
	}

	{
		const char* const vertexSource =	"${GLSL_VERSION_DECL}\n"
													"in highp vec4 a_position;\n"
													"out highp vec4 v_position;\n"
													"void main()\n"
													"{\n"
													"	gl_Position = a_position;\n"
													"	v_position = a_position;\n"
													"}\n";
		const char* const fragmentSource =	"${GLSL_VERSION_DECL}\n"
													"in mediump vec4 v_position;\n"
													"layout(location=0) out mediump vec4 dEQP_FragColor;\n"
													"void main()\n"
													"{\n"
													"	const mediump vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
													"	const mediump vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
													"	dEQP_FragColor = (step(0.1, mod(v_position.x, 0.2)) == step(0.1, mod(v_position.y, 0.2))) ? (green) : (yellow);\n"
													"}\n";

		m_program = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(specializeShader(m_context, vertexSource)) << glu::FragmentSource(specializeShader(m_context, fragmentSource))));

		if (!m_program->isOk())
		{
			m_testCtx.getLog() << *m_program;
			throw tcu::TestError("failed to build program");
		}
	}

	{
		static const tcu::Vec4 s_quadCoords[] =
		{
			tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
			tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f),
			tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		};

		m_vbo = de::MovePtr<glu::Buffer>(new glu::Buffer(m_context.getRenderContext()));

		gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(s_quadCoords), s_quadCoords, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set buf");
	}

	// gen iterations

	{
		const tcu::IVec2 srcSize = (m_src == TARGET_DEFAULT) ? (tcu::IVec2(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight())) : (tcu::IVec2(FBO_SIZE, FBO_SIZE));
		const tcu::IVec2 dstSize = (m_dst == TARGET_DEFAULT) ? (tcu::IVec2(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight())) : (tcu::IVec2(FBO_SIZE, FBO_SIZE));

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "srcSize = " << srcSize << "\n"
			<< "dstSize = " << dstSize << "\n"
			<< tcu::TestLog::EndMessage;

		for (int ndx = 0; ndx < numIterations; ++ndx)
		{
			BlitArgs args;

			if (m_src == TARGET_DEFAULT && defaultFBMultisampled)
			{
				const tcu::IVec2	unionSize	= tcu::IVec2(de::min(srcSize.x(), dstSize.x()), de::min(srcSize.y(), dstSize.y()));
				const int			srcWidth	= rnd.getInt(1, unionSize.x());
				const int			srcHeight	= rnd.getInt(1, unionSize.y());
				const int			srcX		= rnd.getInt(0, unionSize.x() - srcWidth);
				const int			srcY		= rnd.getInt(0, unionSize.y() - srcHeight);

				args.src.x() = srcX;
				args.src.y() = srcY;
				args.src.z() = srcX + srcWidth;
				args.src.w() = srcY + srcHeight;

				args.dst = args.src;
			}
			else
			{
				const int	srcWidth	= rnd.getInt(1, srcSize.x());
				const int	srcHeight	= rnd.getInt(1, srcSize.y());
				const int	srcX		= rnd.getInt(0, srcSize.x() - srcWidth);
				const int	srcY		= rnd.getInt(0, srcSize.y() - srcHeight);
				const int	dstWidth	= rnd.getInt(1, dstSize.x());
				const int	dstHeight	= rnd.getInt(1, dstSize.y());
				const int	dstX		= rnd.getInt(-(dstWidth / 2), dstSize.x() - (dstWidth+1) / 2);		// allow dst go out of bounds
				const int	dstY		= rnd.getInt(-(dstHeight / 2), dstSize.y() - (dstHeight+1)  / 2);

				args.src.x() = srcX;
				args.src.y() = srcY;
				args.src.z() = srcX + srcWidth;
				args.src.w() = srcY + srcHeight;
				args.dst.x() = dstX;
				args.dst.y() = dstY;
				args.dst.z() = dstX + dstWidth;
				args.dst.w() = dstY + dstHeight;
			}

			args.bboxMin.x() = rnd.getFloat(-1.1f, 1.1f);
			args.bboxMin.y() = rnd.getFloat(-1.1f, 1.1f);
			args.bboxMin.z() = rnd.getFloat(-1.1f, 1.1f);
			args.bboxMin.w() = rnd.getFloat( 0.9f, 1.1f);

			args.bboxMax.x() = rnd.getFloat(-1.1f, 1.1f);
			args.bboxMax.y() = rnd.getFloat(-1.1f, 1.1f);
			args.bboxMax.z() = rnd.getFloat(-1.1f, 1.1f);
			args.bboxMax.w() = rnd.getFloat( 0.9f, 1.1f);

			if (args.bboxMin.x() / args.bboxMin.w() > args.bboxMax.x() / args.bboxMax.w())
				std::swap(args.bboxMin.x(), args.bboxMax.x());
			if (args.bboxMin.y() / args.bboxMin.w() > args.bboxMax.y() / args.bboxMax.w())
				std::swap(args.bboxMin.y(), args.bboxMax.y());
			if (args.bboxMin.z() / args.bboxMin.w() > args.bboxMax.z() / args.bboxMax.w())
				std::swap(args.bboxMin.z(), args.bboxMax.z());

			args.linear = rnd.getBool();

			m_iterations.push_back(args);
		}
	}
}

void BlitFboCase::deinit (void)
{
	m_srcFbo.clear();
	m_srcRbo.clear();
	m_dstFbo.clear();
	m_dstRbo.clear();
	m_program.clear();
	m_vbo.clear();
}

BlitFboCase::IterateResult BlitFboCase::iterate (void)
{
	const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Iteration" + de::toString(m_iteration), "Iteration " + de::toString(m_iteration+1) + " / " + de::toString((int)m_iterations.size()));
	const BlitArgs&				blitCfg		= m_iterations[m_iteration];
	const glw::Functions&		gl			= m_context.getRenderContext().getFunctions();

	if (m_iteration == 0)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// fill source with test pattern. Default fb must be filled for each iteration because contents might not survive the swap
	if (m_src == TARGET_DEFAULT || m_iteration == 0)
		fillSourceWithPattern();

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Set bounding box:\n"
		<< "\tmin:" << blitCfg.bboxMin << "\n"
		<< "\tmax:" << blitCfg.bboxMax << "\n"
		<< "Blit:\n"
		<<	"\tsrc: " << blitCfg.src << "\n"
		<<	"\tdst: " << blitCfg.dst << "\n"
		<<	"\tfilter: " << ((blitCfg.linear) ? ("linear") : ("nearest"))
		<< tcu::TestLog::EndMessage;

	gl.primitiveBoundingBox(blitCfg.bboxMin.x(), blitCfg.bboxMin.y(), blitCfg.bboxMin.z(), blitCfg.bboxMin.w(),
							blitCfg.bboxMax.x(), blitCfg.bboxMax.y(), blitCfg.bboxMax.z(), blitCfg.bboxMax.w());

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, (m_dst == TARGET_FBO) ? (**m_dstFbo) : (m_context.getRenderContext().getDefaultFramebuffer()));
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, (m_src == TARGET_FBO) ? (**m_srcFbo) : (m_context.getRenderContext().getDefaultFramebuffer()));
	gl.blitFramebuffer(blitCfg.src.x(), blitCfg.src.y(), blitCfg.src.z(), blitCfg.src.w(),
					   blitCfg.dst.x(), blitCfg.dst.y(), blitCfg.dst.z(), blitCfg.dst.w(),
					   GL_COLOR_BUFFER_BIT,
					   ((blitCfg.linear) ? (GL_LINEAR) : (GL_NEAREST)));
	GLU_EXPECT_NO_ERROR(gl.getError(), "blit");

	if (!verifyImage(blitCfg))
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got unexpected blit result");

	return (++m_iteration == (int)m_iterations.size()) ? (STOP) : (CONTINUE);
}

bool BlitFboCase::verifyImage (const BlitArgs& args)
{
	const int				colorThreshold	= 4; //!< this test case is not about how color is preserved, allow almost anything
	const tcu::IVec2		dstSize			= (m_dst == TARGET_DEFAULT) ? (tcu::IVec2(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight())) : (tcu::IVec2(FBO_SIZE, FBO_SIZE));
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	tcu::Surface			viewport		(dstSize.x(), dstSize.y());
	tcu::Surface			errorMask		(dstSize.x(), dstSize.y());
	bool					anyError		= false;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying blit result"
		<< tcu::TestLog::EndMessage;

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, (m_dst == TARGET_FBO) ? (**m_dstFbo) : (m_context.getRenderContext().getDefaultFramebuffer()));
	glu::readPixels(m_context.getRenderContext(), 0, 0, viewport.getAccess());

	tcu::clear(errorMask.getAccess(), tcu::IVec4(0, 0, 0, 255));

	for (int y = 0; y < dstSize.y(); ++y)
	for (int x = 0; x < dstSize.x(); ++x)
	{
		const tcu::RGBA color	= viewport.getPixel(x, y);
		const bool		inside	= (x >= args.dst.x() && x < args.dst.z() && y >= args.dst.y() && y < args.dst.w());
		const bool		error	= (inside) ? (color.getGreen() < 255 - colorThreshold || color.getBlue() > colorThreshold)
										   : (color.getRed() > colorThreshold || color.getGreen() > colorThreshold || color.getBlue() > colorThreshold);

		if (error)
		{
			anyError = true;
			errorMask.setPixel(x, y, tcu::RGBA::red());
		}
	}

	if (anyError)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewport.getAccess())
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask.getAccess())
			<< tcu::TestLog::EndImageSet;
		return false;
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Result image ok."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewport.getAccess())
			<< tcu::TestLog::EndImageSet;
		return true;
	}
}

void BlitFboCase::fillSourceWithPattern (void)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	const tcu::IVec2		srcSize		= (m_src == TARGET_DEFAULT) ? (tcu::IVec2(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight())) : (tcu::IVec2(FBO_SIZE, FBO_SIZE));
	const int				posLocation	= gl.getAttribLocation(m_program->getProgram(), "a_position");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, (m_src == TARGET_FBO) ? (**m_srcFbo) : (m_context.getRenderContext().getDefaultFramebuffer()));
	gl.viewport(0, 0, srcSize.x(), srcSize.y());
	gl.useProgram(m_program->getProgram());

	gl.clearColor(0.0f, 0.0f, 1.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.enableVertexAttribArray(posLocation);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 4 * (int)sizeof(float), NULL);
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");
}

class DepthDrawCase : public TestCase
{
public:
	enum DepthType
	{
		DEPTH_BUILTIN = 0,
		DEPTH_USER_DEFINED,

		DEPTH_LAST
	};
	enum BBoxState
	{
		STATE_GLOBAL = 0,
		STATE_PER_PRIMITIVE,

		STATE_LAST
	};
	enum BBoxSize
	{
		BBOX_EQUAL = 0,
		BBOX_LARGER,

		BBOX_LAST
	};

									DepthDrawCase					(Context& context, const char* name, const char* description, DepthType depthType, BBoxState state, BBoxSize bboxSize);
									~DepthDrawCase					(void);

private:
	void							init							(void);
	void							deinit							(void);
	IterateResult					iterate							(void);

	std::string						genVertexSource					(void) const;
	std::string						genFragmentSource				(void) const;
	std::string						genTessellationControlSource	(void) const;
	std::string						genTessellationEvaluationSource	(void) const;
	void							generateAttributeData			(std::vector<tcu::Vec4>& data) const;
	bool							verifyImage						(const tcu::Surface& viewport) const;

	enum
	{
		RENDER_AREA_SIZE = 256,
	};

	struct LayerInfo
	{
		float		zOffset;
		float		zScale;
		tcu::Vec4	color1;
		tcu::Vec4	color2;
	};

	const int						m_numLayers;
	const int						m_gridSize;

	const DepthType					m_depthType;
	const BBoxState					m_state;
	const BBoxSize					m_bboxSize;

	de::MovePtr<glu::ShaderProgram>	m_program;
	de::MovePtr<glu::Buffer>		m_vbo;
	std::vector<LayerInfo>			m_layers;
};

DepthDrawCase::DepthDrawCase (Context& context, const char* name, const char* description, DepthType depthType, BBoxState state, BBoxSize bboxSize)
	: TestCase		(context, name, description)
	, m_numLayers	(14)
	, m_gridSize	(24)
	, m_depthType	(depthType)
	, m_state		(state)
	, m_bboxSize	(bboxSize)
{
	DE_ASSERT(depthType < DEPTH_LAST);
	DE_ASSERT(state < STATE_LAST);
	DE_ASSERT(bboxSize < BBOX_LAST);
}

DepthDrawCase::~DepthDrawCase (void)
{
	deinit();
}

void DepthDrawCase::init (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const bool				supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	// requirements

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_primitive_bounding_box"))
		throw tcu::NotSupportedError("Test requires GL_EXT_primitive_bounding_box extension");
	if (m_state == STATE_PER_PRIMITIVE && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");
	if (m_context.getRenderTarget().getDepthBits() == 0)
		throw tcu::NotSupportedError("Test requires depth buffer");
	if (m_context.getRenderTarget().getWidth() < RENDER_AREA_SIZE || m_context.getRenderTarget().getHeight() < RENDER_AREA_SIZE)
		throw tcu::NotSupportedError("Test requires " + de::toString<int>(RENDER_AREA_SIZE) + "x" + de::toString<int>(RENDER_AREA_SIZE) + " viewport");

	// log
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Rendering multiple triangle grids with with different z coordinates.\n"
		<< "Topmost grid is green-yellow, other grids are blue-red.\n"
		<< "Expecting only the green-yellow grid to be visible.\n"
		<< "Setting primitive bounding box "
			<< ((m_bboxSize == BBOX_EQUAL) ? ("to exactly cover") : ("to cover"))
			<< ((m_state == STATE_GLOBAL) ? (" each grid") : (" each triangle"))
			<< ((m_bboxSize == BBOX_EQUAL) ? (".") : (" and include some padding."))
			<< "\n"
		<< "Set bounding box using "
			<< ((m_state == STATE_GLOBAL) ? ("PRIMITIVE_BOUNDING_BOX_EXT state") : ("gl_BoundingBoxEXT output"))
			<< "\n"
		<< ((m_depthType == DEPTH_USER_DEFINED) ? ("Fragment depth is set in the fragment shader") : (""))
		<< tcu::TestLog::EndMessage;

	// resources

	{
		glu::ProgramSources sources;
		sources << glu::VertexSource(specializeShader(m_context, genVertexSource().c_str()));
		sources << glu::FragmentSource(specializeShader(m_context, genFragmentSource().c_str()));

		if (m_state == STATE_PER_PRIMITIVE)
			sources << glu::TessellationControlSource(specializeShader(m_context, genTessellationControlSource().c_str()))
					<< glu::TessellationEvaluationSource(specializeShader(m_context, genTessellationEvaluationSource().c_str()));

		m_program = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(), sources));
		GLU_EXPECT_NO_ERROR(gl.getError(), "build program");

		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "ShaderProgram", "Shader program");
			m_testCtx.getLog() << *m_program;
		}

		if (!m_program->isOk())
			throw tcu::TestError("failed to build program");
	}

	{
		std::vector<tcu::Vec4> data;

		generateAttributeData(data);

		m_vbo = de::MovePtr<glu::Buffer>(new glu::Buffer(m_context.getRenderContext()));
		gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, (int)(sizeof(tcu::Vec4) * data.size()), &data[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "buf upload");
	}

	// gen layers
	{
		de::Random rnd(0x12345);

		m_layers.resize(m_numLayers);
		for (int layerNdx = 0; layerNdx < m_numLayers; ++layerNdx)
		{
			m_layers[layerNdx].zOffset	= ((float)layerNdx / (float)m_numLayers) * 2.0f - 1.0f;
			m_layers[layerNdx].zScale	= (2.0f / (float)m_numLayers);
			m_layers[layerNdx].color1	= (layerNdx == 0) ? (tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f)) : (tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
			m_layers[layerNdx].color2	= (layerNdx == 0) ? (tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f)) : (tcu::Vec4(1.0f, 0.0f, 1.0f, 1.0f));
		}
		rnd.shuffle(m_layers.begin(), m_layers.end());
	}
}

void DepthDrawCase::deinit (void)
{
	m_program.clear();
	m_vbo.clear();
}

DepthDrawCase::IterateResult DepthDrawCase::iterate (void)
{
	const bool				hasTessellation		= (m_state == STATE_PER_PRIMITIVE);
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const glw::GLint		posLocation			= gl.getAttribLocation(m_program->getProgram(), "a_position");
	const glw::GLint		colLocation			= gl.getAttribLocation(m_program->getProgram(), "a_colorMix");
	const glw::GLint		depthBiasLocation	= gl.getUniformLocation(m_program->getProgram(), "u_depthBias");
	const glw::GLint		depthScaleLocation	= gl.getUniformLocation(m_program->getProgram(), "u_depthScale");
	const glw::GLint		color1Location		= gl.getUniformLocation(m_program->getProgram(), "u_color1");
	const glw::GLint		color2Location		= gl.getUniformLocation(m_program->getProgram(), "u_color2");

	tcu::Surface			viewport			(RENDER_AREA_SIZE, RENDER_AREA_SIZE);
	de::Random				rnd					(0x213237);

	TCU_CHECK(posLocation != -1);
	TCU_CHECK(colLocation != -1);
	TCU_CHECK(depthBiasLocation != -1);
	TCU_CHECK(depthScaleLocation != -1);
	TCU_CHECK(color1Location != -1);
	TCU_CHECK(color2Location != -1);

	gl.viewport(0, 0, RENDER_AREA_SIZE, RENDER_AREA_SIZE);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clearDepthf(1.0f);
	gl.depthFunc(GL_LESS);
	gl.enable(GL_DEPTH_TEST);
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup viewport");

	gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, (int)(8 * sizeof(float)), (const float*)DE_NULL);
	gl.vertexAttribPointer(colLocation, 4, GL_FLOAT, GL_FALSE, (int)(8 * sizeof(float)), (const float*)DE_NULL + 4);
	gl.enableVertexAttribArray(posLocation);
	gl.enableVertexAttribArray(colLocation);
	gl.useProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup va");

	if (hasTessellation)
		gl.patchParameteri(GL_PATCH_VERTICES, 3);

	for (int layerNdx = 0; layerNdx < m_numLayers; ++layerNdx)
	{
		gl.uniform1f(depthBiasLocation, m_layers[layerNdx].zOffset);
		gl.uniform1f(depthScaleLocation, m_layers[layerNdx].zScale);
		gl.uniform4fv(color1Location, 1, m_layers[layerNdx].color1.getPtr());
		gl.uniform4fv(color2Location, 1, m_layers[layerNdx].color2.getPtr());

		if (m_state == STATE_GLOBAL)
		{
			const float negPadding = (m_bboxSize == BBOX_EQUAL) ? (0.0f) : (rnd.getFloat() * 0.3f);
			const float posPadding = (m_bboxSize == BBOX_EQUAL) ? (0.0f) : (rnd.getFloat() * 0.3f);

			gl.primitiveBoundingBox(-1.0f, -1.0f, m_layers[layerNdx].zOffset - negPadding, 1.0f,
									1.0f,  1.0f, (m_layers[layerNdx].zOffset + m_layers[layerNdx].zScale + posPadding), 1.0f);
		}

		gl.drawArrays((hasTessellation) ? (GL_PATCHES) : (GL_TRIANGLES), 0, m_gridSize * m_gridSize * 6);
	}

	glu::readPixels(m_context.getRenderContext(), 0, 0, viewport.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "render and read");

	if (verifyImage(viewport))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");

	return STOP;
}

std::string DepthDrawCase::genVertexSource (void) const
{
	const bool			hasTessellation	= (m_state == STATE_PER_PRIMITIVE);
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n"
			"in highp vec4 a_colorMix;\n"
			"out highp vec4 vtx_colorMix;\n";

	if (!hasTessellation && m_depthType == DEPTH_USER_DEFINED)
		buf << "out highp float v_fragDepth;\n";

	if (!hasTessellation)
		buf <<	"uniform highp float u_depthBias;\n"
				"uniform highp float u_depthScale;\n";

	buf <<	"\n"
			"void main()\n"
			"{\n";

	if (hasTessellation)
		buf << "	gl_Position = a_position;\n";
	else if (m_depthType == DEPTH_USER_DEFINED)
		buf <<	"	highp float dummyZ = a_position.z;\n"
				"	highp float writtenZ = a_position.w;\n"
				"	gl_Position = vec4(a_position.xy, dummyZ, 1.0);\n"
				"	v_fragDepth = writtenZ * u_depthScale + u_depthBias;\n";
	else
		buf <<	"	highp float writtenZ = a_position.w;\n"
				"	gl_Position = vec4(a_position.xy, writtenZ * u_depthScale + u_depthBias, 1.0);\n";

	buf <<	"	vtx_colorMix = a_colorMix;\n"
			"}\n";

	return buf.str();
}

std::string DepthDrawCase::genFragmentSource (void) const
{
	const bool			hasTessellation	= (m_state == STATE_PER_PRIMITIVE);
	const char* const	colorMixName	= (hasTessellation) ? ("tess_eval_colorMix") : ("vtx_colorMix");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in mediump vec4 " << colorMixName << ";\n";

	if (m_depthType == DEPTH_USER_DEFINED)
		buf << "in mediump float v_fragDepth;\n";

	buf <<	"layout(location = 0) out mediump vec4 o_color;\n"
			"uniform highp vec4 u_color1;\n"
			"uniform highp vec4 u_color2;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	o_color = mix(u_color1, u_color2, " << colorMixName << ");\n";

	if (m_depthType == DEPTH_USER_DEFINED)
		buf << "	gl_FragDepth = v_fragDepth * 0.5 + 0.5;\n";

	buf <<	"}\n";

	return buf.str();
}

std::string DepthDrawCase::genTessellationControlSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"${PRIMITIVE_BOUNDING_BOX_REQUIRE}\n"
			"layout(vertices=3) out;\n"
			"\n"
			"uniform highp float u_depthBias;\n"
			"uniform highp float u_depthScale;\n"
			"\n"
			"in highp vec4 vtx_colorMix[];\n"
			"out highp vec4 tess_ctrl_colorMix[];\n"
			"\n"
			"void main()\n"
			"{\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			"	tess_ctrl_colorMix[gl_InvocationID] = vtx_colorMix[0];\n"
			"\n"
			"	gl_TessLevelOuter[0] = 2.8;\n"
			"	gl_TessLevelOuter[1] = 2.8;\n"
			"	gl_TessLevelOuter[2] = 2.8;\n"
			"	gl_TessLevelInner[0] = 2.8;\n"
			"\n"
			"	// real Z stored in w component\n"
			"	highp vec4 minBound = vec4(min(min(vec3(gl_in[0].gl_Position.xy, gl_in[0].gl_Position.w * u_depthScale + u_depthBias),\n"
			"	                                   vec3(gl_in[1].gl_Position.xy, gl_in[1].gl_Position.w * u_depthScale + u_depthBias)),\n"
			"	                               vec3(gl_in[2].gl_Position.xy, gl_in[2].gl_Position.w * u_depthScale + u_depthBias)), 1.0);\n"
			"	highp vec4 maxBound = vec4(max(max(vec3(gl_in[0].gl_Position.xy, gl_in[0].gl_Position.w * u_depthScale + u_depthBias),\n"
			"	                                   vec3(gl_in[1].gl_Position.xy, gl_in[1].gl_Position.w * u_depthScale + u_depthBias)),\n"
			"	                               vec3(gl_in[2].gl_Position.xy, gl_in[2].gl_Position.w * u_depthScale + u_depthBias)), 1.0);\n";

	if (m_bboxSize == BBOX_EQUAL)
		buf <<	"	${PRIM_GL_BOUNDING_BOX}[0] = minBound;\n"
				"	${PRIM_GL_BOUNDING_BOX}[1] = maxBound;\n";
	else
		buf <<	"	highp float nedPadding = mod(gl_in[0].gl_Position.z, 0.3);\n"
				"	highp float posPadding = mod(gl_in[1].gl_Position.z, 0.3);\n"
				"	${PRIM_GL_BOUNDING_BOX}[0] = minBound - vec4(0.0, 0.0, nedPadding, 0.0);\n"
				"	${PRIM_GL_BOUNDING_BOX}[1] = maxBound + vec4(0.0, 0.0, posPadding, 0.0);\n";

	buf <<	"}\n";

	return buf.str();
}

std::string DepthDrawCase::genTessellationEvaluationSource (void) const
{
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"${GPU_SHADER5_REQUIRE}\n"
			"layout(triangles) in;\n"
			"\n"
			"in highp vec4 tess_ctrl_colorMix[];\n"
			"out highp vec4 tess_eval_colorMix;\n";

	if (m_depthType == DEPTH_USER_DEFINED)
		buf << "out highp float v_fragDepth;\n";

	buf <<	"uniform highp float u_depthBias;\n"
			"uniform highp float u_depthScale;\n"
			"\n"
			"precise gl_Position;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	highp vec4 tessellatedPos = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;\n";

	if (m_depthType == DEPTH_USER_DEFINED)
		buf <<	"	highp float dummyZ = tessellatedPos.z;\n"
				"	highp float writtenZ = tessellatedPos.w;\n"
				"	gl_Position = vec4(tessellatedPos.xy, dummyZ, 1.0);\n"
				"	v_fragDepth = writtenZ * u_depthScale + u_depthBias;\n";
	else
		buf <<	"	highp float writtenZ = tessellatedPos.w;\n"
				"	gl_Position = vec4(tessellatedPos.xy, writtenZ * u_depthScale + u_depthBias, 1.0);\n";

	buf <<	"	tess_eval_colorMix = tess_ctrl_colorMix[0];\n"
			"}\n";

	return buf.str();
}

void DepthDrawCase::generateAttributeData (std::vector<tcu::Vec4>& data) const
{
	const tcu::Vec4		color1				(0.0f, 0.0f, 0.0f, 0.0f); // mix weights
	const tcu::Vec4		color2				(1.0f, 1.0f, 1.0f, 1.0f);
	std::vector<int>	cellOrder			(m_gridSize * m_gridSize);
	de::Random			rnd					(0xAB54321);

	// generate grid with cells in random order
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
		cellOrder[ndx] = ndx;
	rnd.shuffle(cellOrder.begin(), cellOrder.end());

	data.resize(m_gridSize * m_gridSize * 6 * 2);
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
	{
		const int			cellNdx		= cellOrder[ndx];
		const int			cellX		= cellNdx % m_gridSize;
		const int			cellY		= cellNdx / m_gridSize;
		const tcu::Vec4&	cellColor	= ((cellX+cellY)%2 == 0) ? (color1) : (color2);

		data[ndx * 6 * 2 +  0] = tcu::Vec4(float(cellX+0) / float(m_gridSize) * 2.0f - 1.0f, float(cellY+0) / float(m_gridSize) * 2.0f - 1.0f, 0.0f, 0.0f);	data[ndx * 6 * 2 +  1] = cellColor;
		data[ndx * 6 * 2 +  2] = tcu::Vec4(float(cellX+1) / float(m_gridSize) * 2.0f - 1.0f, float(cellY+1) / float(m_gridSize) * 2.0f - 1.0f, 0.0f, 0.0f);	data[ndx * 6 * 2 +  3] = cellColor;
		data[ndx * 6 * 2 +  4] = tcu::Vec4(float(cellX+0) / float(m_gridSize) * 2.0f - 1.0f, float(cellY+1) / float(m_gridSize) * 2.0f - 1.0f, 0.0f, 0.0f);	data[ndx * 6 * 2 +  5] = cellColor;
		data[ndx * 6 * 2 +  6] = tcu::Vec4(float(cellX+0) / float(m_gridSize) * 2.0f - 1.0f, float(cellY+0) / float(m_gridSize) * 2.0f - 1.0f, 0.0f, 0.0f);	data[ndx * 6 * 2 +  7] = cellColor;
		data[ndx * 6 * 2 +  8] = tcu::Vec4(float(cellX+1) / float(m_gridSize) * 2.0f - 1.0f, float(cellY+0) / float(m_gridSize) * 2.0f - 1.0f, 0.0f, 0.0f);	data[ndx * 6 * 2 +  9] = cellColor;
		data[ndx * 6 * 2 + 10] = tcu::Vec4(float(cellX+1) / float(m_gridSize) * 2.0f - 1.0f, float(cellY+1) / float(m_gridSize) * 2.0f - 1.0f, 0.0f, 0.0f);	data[ndx * 6 * 2 + 11] = cellColor;

		// Fill Z with random values (fake Z)
		for (int vtxNdx = 0; vtxNdx < 6; ++vtxNdx)
			data[ndx * 6 * 2 + 2*vtxNdx].z() = rnd.getFloat(0.0f, 1.0);

		// Fill W with other random values (written Z)
		for (int vtxNdx = 0; vtxNdx < 6; ++vtxNdx)
			data[ndx * 6 * 2 + 2*vtxNdx].w() = rnd.getFloat(0.0f, 1.0);
	}
}

bool DepthDrawCase::verifyImage (const tcu::Surface& viewport) const
{
	tcu::Surface	errorMask	(viewport.getWidth(), viewport.getHeight());
	bool			anyError	= false;

	tcu::clear(errorMask.getAccess(), tcu::IVec4(0,0,0,255));

	for (int y = 0; y < viewport.getHeight(); ++y)
	for (int x = 0; x < viewport.getWidth(); ++x)
	{
		const tcu::RGBA	pixel		= viewport.getPixel(x, y);
		bool			error		= false;

		// expect green, yellow or a combination of these
		if (pixel.getGreen() != 255 || pixel.getBlue() != 0)
			error = true;

		if (error)
		{
			errorMask.setPixel(x, y, tcu::RGBA::red());
			anyError = true;
		}
	}

	if (anyError)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewport.getAccess())
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask.getAccess())
			<< tcu::TestLog::EndImageSet;
	else
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Result image ok."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Viewport", "Viewport contents", viewport.getAccess())
			<< tcu::TestLog::EndImageSet;

	return !anyError;
}

class ClearCase : public TestCase
{
public:
	enum
	{
		SCISSOR_CLEAR_BIT		= 1 << 0,
		DRAW_TRIANGLE_BIT		= 1 << 1,
		PER_PRIMITIVE_BBOX_BIT	= 1 << 2,
		FULLSCREEN_SCISSOR_BIT	= 1 << 3,
	};

									ClearCase						(Context& context, const char* name, const char* description, deUint32 flags);
									~ClearCase						(void);

private:
	struct DrawObject
	{
		int firstNdx;
		int numVertices;
	};

	void							init							(void);
	void							deinit							(void);
	IterateResult					iterate							(void);

	void							createVbo						(void);
	void							createProgram					(void);
	void							renderTo						(tcu::Surface& dst, bool useBBox);
	bool							verifyImagesEqual				(const tcu::PixelBufferAccess& withoutBBox, const tcu::PixelBufferAccess& withBBox);
	bool							verifyImageResultValid			(const tcu::PixelBufferAccess& result);

	std::string						genVertexSource					(void) const;
	std::string						genFragmentSource				(void) const;
	std::string						genTessellationControlSource	(bool setBBox) const;
	std::string						genTessellationEvaluationSource	(void) const;

	const bool						m_scissoredClear;
	const bool						m_fullscreenScissor;
	const bool						m_drawTriangles;
	const bool						m_useGlobalState;

	de::MovePtr<glu::Buffer>		m_vbo;
	de::MovePtr<glu::ShaderProgram>	m_perPrimitiveProgram;
	de::MovePtr<glu::ShaderProgram>	m_basicProgram;
	std::vector<DrawObject>			m_drawObjects;
	std::vector<tcu::Vec4>			m_objectVertices;
};

ClearCase::ClearCase (Context& context, const char* name, const char* description, deUint32 flags)
	: TestCase				(context, name, description)
	, m_scissoredClear		((flags & SCISSOR_CLEAR_BIT) != 0)
	, m_fullscreenScissor	((flags & FULLSCREEN_SCISSOR_BIT) != 0)
	, m_drawTriangles		((flags & DRAW_TRIANGLE_BIT) != 0)
	, m_useGlobalState		((flags & PER_PRIMITIVE_BBOX_BIT) == 0)
{
	DE_ASSERT(m_useGlobalState || m_drawTriangles); // per-triangle bbox requires triangles
	DE_ASSERT(!m_fullscreenScissor || m_scissoredClear); // fullscreenScissor requires scissoredClear
}

ClearCase::~ClearCase (void)
{
	deinit();
}

void ClearCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_primitive_bounding_box"))
		throw tcu::NotSupportedError("Test requires GL_EXT_primitive_bounding_box extension");
	if (m_drawTriangles && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Doing multiple"
			<< ((m_scissoredClear) ? (" scissored") : (""))
			<< " color buffer clears"
			<< ((m_drawTriangles) ? (" and drawing some geometry between them") : (""))
			<< ".\n"
		<< ((m_scissoredClear && m_fullscreenScissor) ? ("Setting scissor area to cover entire viewport.\n") : (""))
		<< "Rendering with and without setting the bounding box.\n"
		<< "Expecting bounding box to have no effect on clears (i.e. results are constant).\n"
		<< "Set bounding box using "
			<< ((m_useGlobalState) ? ("PRIMITIVE_BOUNDING_BOX_EXT state") : ("gl_BoundingBoxEXT output"))
			<< ".\n"
		<< "Clear color is green with yellowish shades.\n"
		<< ((m_drawTriangles) ? ("Primitive color is yellow with greenish shades.\n") : (""))
		<< tcu::TestLog::EndMessage;

	if (m_drawTriangles)
	{
		createVbo();
		createProgram();
	}
}

void ClearCase::deinit (void)
{
	m_vbo.clear();
	m_perPrimitiveProgram.clear();
	m_basicProgram.clear();
	m_drawObjects = std::vector<DrawObject>();
	m_objectVertices = std::vector<tcu::Vec4>();
}

ClearCase::IterateResult ClearCase::iterate (void)
{
	const tcu::IVec2	renderTargetSize	(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight());
	tcu::Surface		resultWithoutBBox	(renderTargetSize.x(), renderTargetSize.y());
	tcu::Surface		resultWithBBox		(renderTargetSize.x(), renderTargetSize.y());

	// render with and without bbox set
	for (int passNdx = 0; passNdx < 2; ++passNdx)
	{
		const bool		useBBox			= (passNdx == 1);
		tcu::Surface&	destination		= (useBBox) ? (resultWithBBox) : (resultWithoutBBox);

		renderTo(destination, useBBox);
	}

	// Verify images are equal and that the image does not contain (trivially detectable) garbage

	if (!verifyImagesEqual(resultWithoutBBox.getAccess(), resultWithBBox.getAccess()))
	{
		// verifyImagesEqual will print out the image and error mask
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
	}
	else if (!verifyImageResultValid(resultWithBBox.getAccess()))
	{
		// verifyImageResultValid will print out the image and error mask
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result verification failed");
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image comparison passed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Result", "Result", resultWithBBox.getAccess())
			<< tcu::TestLog::EndImageSet;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

void ClearCase::createVbo (void)
{
	const int				numObjects	= 16;
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	de::Random				rnd			(deStringHash(getName()));

	m_vbo = de::MovePtr<glu::Buffer>(new glu::Buffer(m_context.getRenderContext()));

	for (int objectNdx = 0; objectNdx < numObjects; ++objectNdx)
	{
		const int	numTriangles	= rnd.getInt(1, 4);
		const float	minX			= rnd.getFloat(-1.2f, 0.8f);
		const float	minY			= rnd.getFloat(-1.2f, 0.8f);
		const float	maxX			= minX + rnd.getFloat(0.2f, 1.0f);
		const float	maxY			= minY + rnd.getFloat(0.2f, 1.0f);

		DrawObject	drawObject;
		drawObject.firstNdx = (int)m_objectVertices.size();
		drawObject.numVertices = numTriangles * 3;

		m_drawObjects.push_back(drawObject);

		for (int triangleNdx = 0; triangleNdx < numTriangles; ++triangleNdx)
		for (int vertexNdx = 0; vertexNdx < 3; ++vertexNdx)
		{
			const float posX = rnd.getFloat(minX, maxX);
			const float posY = rnd.getFloat(minY, maxY);
			const float posZ = rnd.getFloat(-0.7f, 0.7f);
			const float posW = rnd.getFloat(0.9f, 1.1f);

			m_objectVertices.push_back(tcu::Vec4(posX, posY, posZ, posW));
		}
	}

	gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
	gl.bufferData(GL_ARRAY_BUFFER, (int)(m_objectVertices.size() * sizeof(tcu::Vec4)), &m_objectVertices[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "buffer upload");
}

void ClearCase::createProgram (void)
{
	m_basicProgram = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(),
																			glu::ProgramSources()
																				<< glu::VertexSource(specializeShader(m_context, genVertexSource().c_str()))
																				<< glu::FragmentSource(specializeShader(m_context, genFragmentSource().c_str()))
																				<< glu::TessellationControlSource(specializeShader(m_context, genTessellationControlSource(false).c_str()))
																				<< glu::TessellationEvaluationSource(specializeShader(m_context, genTessellationEvaluationSource().c_str()))));

	m_testCtx.getLog()
		<< tcu::TestLog::Section("Program", "Shader program")
		<< *m_basicProgram
		<< tcu::TestLog::EndSection;

	if (!m_basicProgram->isOk())
		throw tcu::TestError("shader build failed");

	if (!m_useGlobalState)
	{
		m_perPrimitiveProgram = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(),
																					   glu::ProgramSources()
																							<< glu::VertexSource(specializeShader(m_context, genVertexSource().c_str()))
																							<< glu::FragmentSource(specializeShader(m_context, genFragmentSource().c_str()))
																							<< glu::TessellationControlSource(specializeShader(m_context, genTessellationControlSource(true).c_str()))
																							<< glu::TessellationEvaluationSource(specializeShader(m_context, genTessellationEvaluationSource().c_str()))));

		m_testCtx.getLog()
			<< tcu::TestLog::Section("PerPrimitiveProgram", "Shader program that sets the bounding box")
			<< *m_perPrimitiveProgram
			<< tcu::TestLog::EndSection;

		if (!m_perPrimitiveProgram->isOk())
			throw tcu::TestError("shader build failed");
	}
}

void ClearCase::renderTo (tcu::Surface& dst, bool useBBox)
{
	const int				numOps				= 45;
	const tcu::Vec4			yellow				(1.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4			green				(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::IVec2		renderTargetSize	(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight());
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	de::Random				rnd					(deStringHash(getName()));
	glu::VertexArray		vao					(m_context.getRenderContext());

	// always do the initial clear
	gl.disable(GL_SCISSOR_TEST);
	gl.viewport(0, 0, renderTargetSize.x(), renderTargetSize.y());
	gl.clearColor(yellow.x(), yellow.y(), yellow.z(), yellow.w());
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.finish();

	// prepare draw
	if (m_scissoredClear)
		gl.enable(GL_SCISSOR_TEST);

	if (m_drawTriangles)
	{
		const deUint32	programHandle		= (m_useGlobalState || !useBBox) ? (m_basicProgram->getProgram()) : (m_perPrimitiveProgram->getProgram());
		const int		positionAttribLoc	= gl.getAttribLocation(programHandle, "a_position");

		TCU_CHECK(positionAttribLoc != -1);

		gl.useProgram(programHandle);
		gl.bindVertexArray(*vao);
		gl.enableVertexAttribArray(positionAttribLoc);
		gl.vertexAttribPointer(positionAttribLoc, 4, GL_FLOAT, GL_FALSE, (int)sizeof(tcu::Vec4), DE_NULL);
		gl.patchParameteri(GL_PATCH_VERTICES, 3);
	}

	// do random scissor/clearldraw operations
	for (int opNdx = 0; opNdx < numOps; ++opNdx)
	{
		const int	drawObjNdx				= (m_drawTriangles) ? (rnd.getInt(0, (int)m_drawObjects.size() - 1)) : (0);
		const int	objectVertexStartNdx	= (m_drawTriangles) ? (m_drawObjects[drawObjNdx].firstNdx) : (0);
		const int	objectVertexLength		= (m_drawTriangles) ? (m_drawObjects[drawObjNdx].numVertices) : (0);
		tcu::Vec4	bboxMin;
		tcu::Vec4	bboxMax;

		if (m_drawTriangles)
		{
			bboxMin = tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			bboxMax = tcu::Vec4(-1.0f, -1.0f, -1.0f, -1.0f);

			// calc bbox
			for (int vertexNdx = objectVertexStartNdx; vertexNdx < objectVertexStartNdx + objectVertexLength; ++vertexNdx)
			for (int componentNdx = 0; componentNdx < 4; ++componentNdx)
			{
				bboxMin[componentNdx] = de::min(bboxMin[componentNdx], m_objectVertices[vertexNdx][componentNdx]);
				bboxMax[componentNdx] = de::max(bboxMax[componentNdx], m_objectVertices[vertexNdx][componentNdx]);
			}
		}
		else
		{
			// no geometry, just random something
			bboxMin.x() = rnd.getFloat(-1.2f, 1.0f);
			bboxMin.y() = rnd.getFloat(-1.2f, 1.0f);
			bboxMin.z() = rnd.getFloat(-1.2f, 1.0f);
			bboxMin.w() = 1.0f;
			bboxMax.x() = bboxMin.x() + rnd.getFloat(0.2f, 1.0f);
			bboxMax.y() = bboxMin.y() + rnd.getFloat(0.2f, 1.0f);
			bboxMax.z() = bboxMin.z() + rnd.getFloat(0.2f, 1.0f);
			bboxMax.w() = 1.0f;
		}

		if (m_scissoredClear)
		{
			const int scissorX = (m_fullscreenScissor) ? (0)					: rnd.getInt(0, renderTargetSize.x()-1);
			const int scissorY = (m_fullscreenScissor) ? (0)					: rnd.getInt(0, renderTargetSize.y()-1);
			const int scissorW = (m_fullscreenScissor) ? (renderTargetSize.x())	: rnd.getInt(0, renderTargetSize.x()-scissorX);
			const int scissorH = (m_fullscreenScissor) ? (renderTargetSize.y())	: rnd.getInt(0, renderTargetSize.y()-scissorY);

			gl.scissor(scissorX, scissorY, scissorW, scissorH);
		}

		{
			const tcu::Vec4 color = tcu::mix(green, yellow, rnd.getFloat() * 0.4f); // greenish
			gl.clearColor(color.x(), color.y(), color.z(), color.w());
			gl.clear(GL_COLOR_BUFFER_BIT);
		}

		if (useBBox)
		{
			DE_ASSERT(m_useGlobalState || m_drawTriangles); // !m_useGlobalState -> m_drawTriangles
			if (m_useGlobalState)
				gl.primitiveBoundingBox(bboxMin.x(), bboxMin.y(), bboxMin.z(), bboxMin.w(),
										bboxMax.x(), bboxMax.y(), bboxMax.z(), bboxMax.w());
		}

		if (m_drawTriangles)
			gl.drawArrays(GL_PATCHES, objectVertexStartNdx, objectVertexLength);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "post draw");
	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

bool ClearCase::verifyImagesEqual (const tcu::PixelBufferAccess& withoutBBox, const tcu::PixelBufferAccess& withBBox)
{
	DE_ASSERT(withoutBBox.getWidth() == withBBox.getWidth());
	DE_ASSERT(withoutBBox.getHeight() == withBBox.getHeight());

	tcu::Surface	errorMask	(withoutBBox.getWidth(), withoutBBox.getHeight());
	bool			anyError	= false;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toIVec());

	for (int y = 0; y < withoutBBox.getHeight(); ++y)
	for (int x = 0; x < withoutBBox.getWidth(); ++x)
	{
		if (withoutBBox.getPixelInt(x, y) != withBBox.getPixelInt(x, y))
		{
			errorMask.setPixel(x, y, tcu::RGBA::red());
			anyError = true;
		}
	}

	if (anyError)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image comparison failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image comparison")
			<< tcu::TestLog::Image("WithoutBBox", "Result with bounding box not set", withoutBBox)
			<< tcu::TestLog::Image("WithBBox", "Result with bounding box set", withBBox)
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask.getAccess())
			<< tcu::TestLog::EndImageSet;
	}

	return !anyError;
}

bool ClearCase::verifyImageResultValid (const tcu::PixelBufferAccess& result)
{
	tcu::Surface	errorMask	(result.getWidth(), result.getHeight());
	bool			anyError	= false;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toIVec());

	for (int y = 0; y < result.getHeight(); ++y)
	for (int x = 0; x < result.getWidth(); ++x)
	{
		const tcu::IVec4 pixel = result.getPixelInt(x, y);

		// allow green, yellow and any shade between
		if (pixel[1] != 255 || pixel[2] != 0)
		{
			errorMask.setPixel(x, y, tcu::RGBA::red());
			anyError = true;
		}
	}

	if (anyError)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("ResultImage", "Result image", result)
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask)
			<< tcu::TestLog::EndImageSet;
	}

	return !anyError;
}

static const char* const s_yellowishPosOnlyVertexSource =	"${GLSL_VERSION_DECL}\n"
															"in highp vec4 a_position;\n"
															"out highp vec4 v_vertex_color;\n"
															"void main()\n"
															"{\n"
															"	gl_Position = a_position;\n"
															"	// yellowish shade\n"
															"	highp float redComponent = 0.5 + float(gl_VertexID % 5) / 8.0;\n"
															"	v_vertex_color = vec4(redComponent, 1.0, 0.0, 1.0);\n"
															"}\n";

static const char* const s_basicColorFragmentSource =	"${GLSL_VERSION_DECL}\n"
														"in mediump vec4 v_color;\n"
														"layout(location = 0) out mediump vec4 o_color;\n"
														"void main()\n"
														"{\n"
														"	o_color = v_color;\n"
														"}\n";


static const char* const s_basicColorTessEvalSource =	"${GLSL_VERSION_DECL}\n"
														"${TESSELLATION_SHADER_REQUIRE}\n"
														"${GPU_SHADER5_REQUIRE}\n"
														"layout(triangles) in;\n"
														"in highp vec4 v_tess_eval_color[];\n"
														"out highp vec4 v_color;\n"
														"precise gl_Position;\n"
														"void main()\n"
														"{\n"
														"	gl_Position = gl_TessCoord.x * gl_in[0].gl_Position\n"
														"	            + gl_TessCoord.y * gl_in[1].gl_Position\n"
														"	            + gl_TessCoord.z * gl_in[2].gl_Position;\n"
														"	v_color = gl_TessCoord.x * v_tess_eval_color[0]\n"
														"	        + gl_TessCoord.y * v_tess_eval_color[1]\n"
														"	        + gl_TessCoord.z * v_tess_eval_color[2];\n"
														"}\n";

std::string ClearCase::genVertexSource (void) const
{
	return	s_yellowishPosOnlyVertexSource;
}

std::string ClearCase::genFragmentSource (void) const
{
	return s_basicColorFragmentSource;
}

std::string ClearCase::genTessellationControlSource (bool setBBox) const
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n";

	if (setBBox)
		buf << "${PRIMITIVE_BOUNDING_BOX_REQUIRE}\n";

	buf <<	"layout(vertices=3) out;\n"
			"in highp vec4 v_vertex_color[];\n"
			"out highp vec4 v_tess_eval_color[];\n"
			"void main()\n"
			"{\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			"	v_tess_eval_color[gl_InvocationID] = v_vertex_color[gl_InvocationID];\n"
			"	gl_TessLevelOuter[0] = 2.8;\n"
			"	gl_TessLevelOuter[1] = 2.8;\n"
			"	gl_TessLevelOuter[2] = 2.8;\n"
			"	gl_TessLevelInner[0] = 2.8;\n";

	if (setBBox)
	{
		buf <<	"\n"
		"	${PRIM_GL_BOUNDING_BOX}[0] = min(min(gl_in[0].gl_Position,\n"
		"	                               gl_in[1].gl_Position),\n"
		"	                           gl_in[2].gl_Position);\n"
		"	${PRIM_GL_BOUNDING_BOX}[1] = max(max(gl_in[0].gl_Position,\n"
		"	                               gl_in[1].gl_Position),\n"
		"	                           gl_in[2].gl_Position);\n";
	}

	buf << "}\n";
	return buf.str();
}

std::string ClearCase::genTessellationEvaluationSource (void) const
{
	return s_basicColorTessEvalSource;
}

class ViewportCallOrderCase : public TestCase
{
public:
	enum CallOrder
	{
		VIEWPORT_FIRST = 0,
		BBOX_FIRST,

		ORDER_LAST
	};

									ViewportCallOrderCase			(Context& context, const char* name, const char* description, CallOrder callOrder);
									~ViewportCallOrderCase			(void);

private:
	void							init							(void);
	void							deinit							(void);
	IterateResult					iterate							(void);

	void							genVbo							(void);
	void							genProgram						(void);
	bool							verifyImage						(const tcu::PixelBufferAccess& result);

	std::string						genVertexSource					(void) const;
	std::string						genFragmentSource				(void) const;
	std::string						genTessellationControlSource	(void) const;
	std::string						genTessellationEvaluationSource	(void) const;

	const CallOrder					m_callOrder;

	de::MovePtr<glu::Buffer>		m_vbo;
	de::MovePtr<glu::ShaderProgram>	m_program;
	int								m_numVertices;
};

ViewportCallOrderCase::ViewportCallOrderCase (Context& context, const char* name, const char* description, CallOrder callOrder)
	: TestCase		(context, name, description)
	, m_callOrder	(callOrder)
	, m_numVertices	(-1)
{
	DE_ASSERT(m_callOrder < ORDER_LAST);
}

ViewportCallOrderCase::~ViewportCallOrderCase (void)
{
	deinit();
}

void ViewportCallOrderCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_primitive_bounding_box"))
		throw tcu::NotSupportedError("Test requires GL_EXT_primitive_bounding_box extension");

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing call order of state setting functions have no effect on the rendering.\n"
		<< "Setting viewport and bounding box in the following order:\n"
			<< ((m_callOrder == VIEWPORT_FIRST)
				? ("\tFirst viewport with glViewport function.\n")
				: ("\tFirst bounding box with glPrimitiveBoundingBoxEXT function.\n"))
			<< ((m_callOrder == VIEWPORT_FIRST)
				? ("\tThen bounding box with glPrimitiveBoundingBoxEXT function.\n")
				: ("\tThen viewport with glViewport function.\n"))
		<< "Verifying rendering result."
		<< tcu::TestLog::EndMessage;

	// resources
	genVbo();
	genProgram();
}

void ViewportCallOrderCase::deinit (void)
{
	m_vbo.clear();
	m_program.clear();
}

ViewportCallOrderCase::IterateResult ViewportCallOrderCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const tcu::IVec2		viewportSize	= tcu::IVec2(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight());
	const glw::GLint		posLocation		= gl.getAttribLocation(m_program->getProgram(), "a_position");
	tcu::Surface			resultSurface	(viewportSize.x(), viewportSize.y());

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	// set state
	for (int orderNdx = 0; orderNdx < 2; ++orderNdx)
	{
		if ((orderNdx == 0 && m_callOrder == VIEWPORT_FIRST) ||
			(orderNdx == 1 && m_callOrder == BBOX_FIRST))
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Setting viewport to cover the left half of the render target.\n"
				<< "\t(0, 0, " << (viewportSize.x()/2) << ", " << viewportSize.y() << ")"
				<< tcu::TestLog::EndMessage;

			gl.viewport(0, 0, viewportSize.x()/2, viewportSize.y());
		}
		else
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Setting bounding box to cover the right half of the clip space.\n"
				<< "\t(0.0, -1.0, -1.0, 1.0) .. (1.0, 1.0, 1.0f, 1.0)"
				<< tcu::TestLog::EndMessage;

			gl.primitiveBoundingBox(0.0f, -1.0f, -1.0f, 1.0f,
									1.0f,  1.0f,  1.0f, 1.0f);
		}
	}

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Rendering mesh covering the right half of the clip space."
		<< tcu::TestLog::EndMessage;

	gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, sizeof(float[4]), (const float*)DE_NULL);
	gl.enableVertexAttribArray(posLocation);
	gl.useProgram(m_program->getProgram());
	gl.patchParameteri(GL_PATCH_VERTICES, 3);
	gl.drawArrays(GL_PATCHES, 0, m_numVertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "post-draw");

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying image"
		<< tcu::TestLog::EndMessage;
	glu::readPixels(m_context.getRenderContext(), 0, 0, resultSurface.getAccess());

	if (!verifyImage(resultSurface.getAccess()))
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Result ok."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("Result", "Result", resultSurface.getAccess())
			<< tcu::TestLog::EndImageSet;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	return STOP;
}

void ViewportCallOrderCase::genVbo (void)
{
	const int				gridSize	= 6;
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	std::vector<tcu::Vec4>	data		(gridSize * gridSize * 2 * 3);
	std::vector<int>		cellOrder	(gridSize * gridSize * 2);
	de::Random				rnd			(0x55443322);

	// generate grid with triangles in random order
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
		cellOrder[ndx] = ndx;
	rnd.shuffle(cellOrder.begin(), cellOrder.end());

	// generate grid filling the right half of the clip space: (x: 0.0, y: -1.0) .. (x: 1.0, y: 1.0)
	for (int ndx = 0; ndx < (int)cellOrder.size(); ++ndx)
	{
		const int			cellNdx		= cellOrder[ndx];
		const bool			cellSide	= ((cellNdx % 2) == 0);
		const int			cellX		= (cellNdx / 2) % gridSize;
		const int			cellY		= (cellNdx / 2) / gridSize;

		if (cellSide)
		{
			data[ndx * 3 + 0] = tcu::Vec4(float(cellX+0) / float(gridSize), (float(cellY+0) / float(gridSize)) * 2.0f - 1.0f, 0.0f, 1.0f);
			data[ndx * 3 + 1] = tcu::Vec4(float(cellX+1) / float(gridSize), (float(cellY+1) / float(gridSize)) * 2.0f - 1.0f, 0.0f, 1.0f);
			data[ndx * 3 + 2] = tcu::Vec4(float(cellX+0) / float(gridSize), (float(cellY+1) / float(gridSize)) * 2.0f - 1.0f, 0.0f, 1.0f);
		}
		else
		{
			data[ndx * 3 + 0] = tcu::Vec4(float(cellX+0) / float(gridSize), (float(cellY+0) / float(gridSize)) * 2.0f - 1.0f, 0.0f, 1.0f);
			data[ndx * 3 + 1] = tcu::Vec4(float(cellX+1) / float(gridSize), (float(cellY+0) / float(gridSize)) * 2.0f - 1.0f, 0.0f, 1.0f);
			data[ndx * 3 + 2] = tcu::Vec4(float(cellX+1) / float(gridSize), (float(cellY+1) / float(gridSize)) * 2.0f - 1.0f, 0.0f, 1.0f);
		}
	}

	m_vbo = de::MovePtr<glu::Buffer>(new glu::Buffer(m_context.getRenderContext()));
	gl.bindBuffer(GL_ARRAY_BUFFER, **m_vbo);
	gl.bufferData(GL_ARRAY_BUFFER, (int)(data.size() * sizeof(tcu::Vec4)), &data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "create vbo");

	m_numVertices = (int)data.size();
}

void ViewportCallOrderCase::genProgram (void)
{
	m_program = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(),
																	   glu::ProgramSources()
																			<< glu::VertexSource(specializeShader(m_context, genVertexSource().c_str()))
																			<< glu::FragmentSource(specializeShader(m_context, genFragmentSource().c_str()))
																			<< glu::TessellationControlSource(specializeShader(m_context, genTessellationControlSource().c_str()))
																			<< glu::TessellationEvaluationSource(specializeShader(m_context, genTessellationEvaluationSource().c_str()))));

	m_testCtx.getLog()
		<< tcu::TestLog::Section("Program", "Shader program")
		<< *m_program
		<< tcu::TestLog::EndSection;

	if (!m_program->isOk())
		throw tcu::TestError("shader build failed");
}

bool ViewportCallOrderCase::verifyImage (const tcu::PixelBufferAccess& result)
{
	const tcu::IVec2	insideBorder	(deCeilFloatToInt32(0.25f * (float)result.getWidth()) + 1, deFloorFloatToInt32(0.5f * (float)result.getWidth()) - 1);
	const tcu::IVec2	outsideBorder	(deFloorFloatToInt32(0.25f * (float)result.getWidth()) - 1, deCeilFloatToInt32(0.5f * (float)result.getWidth()) + 1);
	tcu::Surface		errorMask		(result.getWidth(), result.getHeight());
	bool				anyError		= false;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toIVec());

	for (int y = 0; y < result.getHeight(); ++y)
	for (int x = 0; x < result.getWidth(); ++x)
	{
		const tcu::IVec4	pixel			= result.getPixelInt(x, y);
		const bool			insideMeshArea	= x >= insideBorder.x() && x <= insideBorder.x();
		const bool			outsideMeshArea = x <= outsideBorder.x() && x >= outsideBorder.x();

		// inside mesh, allow green, yellow and any shade between
		// outside mesh, allow background (black) only
		// in the border area, allow anything
		if ((insideMeshArea && (pixel[1] != 255 || pixel[2] != 0)) ||
			(outsideMeshArea && (pixel[0] != 0 || pixel[1] != 0 || pixel[2] != 0)))
		{
			errorMask.setPixel(x, y, tcu::RGBA::red());
			anyError = true;
		}
	}

	if (anyError)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Images", "Image verification")
			<< tcu::TestLog::Image("ResultImage", "Result image", result)
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask)
			<< tcu::TestLog::EndImageSet;
	}

	return !anyError;
}

std::string ViewportCallOrderCase::genVertexSource (void) const
{
	return	s_yellowishPosOnlyVertexSource;
}

std::string ViewportCallOrderCase::genFragmentSource (void) const
{
	return s_basicColorFragmentSource;
}

std::string ViewportCallOrderCase::genTessellationControlSource (void) const
{
	return	"${GLSL_VERSION_DECL}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"layout(vertices=3) out;\n"
			"in highp vec4 v_vertex_color[];\n"
			"out highp vec4 v_tess_eval_color[];\n"
			"void main()\n"
			"{\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			"	v_tess_eval_color[gl_InvocationID] = v_vertex_color[gl_InvocationID];\n"
			"	gl_TessLevelOuter[0] = 2.8;\n"
			"	gl_TessLevelOuter[1] = 2.8;\n"
			"	gl_TessLevelOuter[2] = 2.8;\n"
			"	gl_TessLevelInner[0] = 2.8;\n"
			"}\n";
}

std::string ViewportCallOrderCase::genTessellationEvaluationSource (void) const
{
	return s_basicColorTessEvalSource;
}

} // anonymous

PrimitiveBoundingBoxTests::PrimitiveBoundingBoxTests (Context& context)
	: TestCaseGroup(context, "primitive_bounding_box", "Tests for EXT_primitive_bounding_box")
{
}

PrimitiveBoundingBoxTests::~PrimitiveBoundingBoxTests (void)
{
}

void PrimitiveBoundingBoxTests::init (void)
{
	static const struct
	{
		const char*	name;
		const char*	description;
		deUint32	methodFlags;
	} stateSetMethods[] =
	{
		{
			"global_state",
			"Set bounding box using PRIMITIVE_BOUNDING_BOX_EXT state",
			BBoxRenderCase::FLAG_SET_BBOX_STATE,
		},
		{
			"tessellation_set_per_draw",
			"Set bounding box using gl_BoundingBoxEXT, use same value for all primitives",
			BBoxRenderCase::FLAG_SET_BBOX_OUTPUT,
		},
		{
			"tessellation_set_per_primitive",
			"Set bounding box using gl_BoundingBoxEXT, use per-primitive bounding box",
			BBoxRenderCase::FLAG_SET_BBOX_OUTPUT | BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX,
		},
	};
	static const struct
	{
		const char*	name;
		const char*	description;
		deUint32	stageFlags;
	} pipelineConfigs[] =
	{
		{
			"vertex_fragment",
			"Render with vertex-fragment program",
			0u
		},
		{
			"vertex_tessellation_fragment",
			"Render with vertex-tessellation{ctrl,eval}-fragment program",
			BBoxRenderCase::FLAG_TESSELLATION
		},
		{
			"vertex_geometry_fragment",
			"Render with vertex-tessellation{ctrl,eval}-geometry-fragment program",
			BBoxRenderCase::FLAG_GEOMETRY
		},
		{
			"vertex_tessellation_geometry_fragment",
			"Render with vertex-geometry-fragment program",
			BBoxRenderCase::FLAG_TESSELLATION | BBoxRenderCase::FLAG_GEOMETRY
		},
	};
	static const struct
	{
		const char*	name;
		const char*	description;
		deUint32	flags;
		deUint32	invalidFlags;
		deUint32	requiredFlags;
	} usageConfigs[] =
	{
		{
			"default_framebuffer_bbox_equal",
			"Render to default framebuffer, set tight bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_DEFAULT | BBoxRenderCase::FLAG_BBOXSIZE_EQUAL,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX,
			0
		},
		{
			"default_framebuffer_bbox_larger",
			"Render to default framebuffer, set padded bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_DEFAULT | BBoxRenderCase::FLAG_BBOXSIZE_LARGER,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX,
			0
		},
		{
			"default_framebuffer_bbox_smaller",
			"Render to default framebuffer, set too small bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_DEFAULT | BBoxRenderCase::FLAG_BBOXSIZE_SMALLER,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX,
			0
		},
		{
			"fbo_bbox_equal",
			"Render to texture, set tight bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_FBO | BBoxRenderCase::FLAG_BBOXSIZE_EQUAL,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX,
			0
		},
		{
			"fbo_bbox_larger",
			"Render to texture, set padded bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_FBO | BBoxRenderCase::FLAG_BBOXSIZE_LARGER,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX,
			0
		},
		{
			"fbo_bbox_smaller",
			"Render to texture, set too small bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_FBO | BBoxRenderCase::FLAG_BBOXSIZE_SMALLER,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX,
			0
		},
		{
			"default_framebuffer",
			"Render to default framebuffer, set tight bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_DEFAULT | BBoxRenderCase::FLAG_BBOXSIZE_EQUAL,
			0,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX
		},
		{
			"fbo",
			"Render to texture, set tight bounding box",
			BBoxRenderCase::FLAG_RENDERTARGET_FBO | BBoxRenderCase::FLAG_BBOXSIZE_EQUAL,
			0,
			BBoxRenderCase::FLAG_PER_PRIMITIVE_BBOX
		},
	};
	enum PrimitiveRenderType
	{
		TYPE_TRIANGLE,
		TYPE_LINE,
		TYPE_POINT,
	};
	const struct
	{
		const char*			name;
		const char*			description;
		PrimitiveRenderType	type;
		deUint32			flags;
	} primitiveTypes[] =
	{
		{
			"triangles",
			"Triangle render tests",
			TYPE_TRIANGLE,
			0
		},
		{
			"lines",
			"Line render tests",
			TYPE_LINE,
			0
		},
		{
			"points",
			"Point render tests",
			TYPE_POINT,
			0
		},
		{
			"wide_lines",
			"Wide line render tests",
			TYPE_LINE,
			LineRenderCase::LINEFLAG_WIDE
		},
		{
			"wide_points",
			"Wide point render tests",
			TYPE_POINT,
			PointRenderCase::POINTFLAG_WIDE
		},
	};

	// .state_query
	{
		tcu::TestCaseGroup* const stateQueryGroup = new tcu::TestCaseGroup(m_testCtx, "state_query", "State queries");
		addChild(stateQueryGroup);

		stateQueryGroup->addChild(new InitialValueCase	(m_context,	"initial_value",	"Initial value case"));
		stateQueryGroup->addChild(new QueryCase			(m_context,	"getfloat",			"getFloatv",			QueryCase::QUERY_FLOAT));
		stateQueryGroup->addChild(new QueryCase			(m_context,	"getboolean",		"getBooleanv",			QueryCase::QUERY_BOOLEAN));
		stateQueryGroup->addChild(new QueryCase			(m_context,	"getinteger",		"getIntegerv",			QueryCase::QUERY_INT));
		stateQueryGroup->addChild(new QueryCase			(m_context,	"getinteger64",		"getInteger64v",		QueryCase::QUERY_INT64));
	}

	// .triangles
	// .(wide_)lines
	// .(wide_)points
	for (int primitiveTypeNdx = 0; primitiveTypeNdx < DE_LENGTH_OF_ARRAY(primitiveTypes); ++primitiveTypeNdx)
	{
		tcu::TestCaseGroup* const primitiveGroup = new tcu::TestCaseGroup(m_testCtx, primitiveTypes[primitiveTypeNdx].name, primitiveTypes[primitiveTypeNdx].description);
		addChild(primitiveGroup);

		for (int stateSetMethodNdx = 0; stateSetMethodNdx < DE_LENGTH_OF_ARRAY(stateSetMethods); ++stateSetMethodNdx)
		{
			tcu::TestCaseGroup* const methodGroup = new tcu::TestCaseGroup(m_testCtx, stateSetMethods[stateSetMethodNdx].name, stateSetMethods[stateSetMethodNdx].description);
			primitiveGroup->addChild(methodGroup);

			for (int pipelineConfigNdx = 0; pipelineConfigNdx < DE_LENGTH_OF_ARRAY(pipelineConfigs); ++pipelineConfigNdx)
			{
				if ((stateSetMethods[stateSetMethodNdx].methodFlags & BBoxRenderCase::FLAG_SET_BBOX_OUTPUT) != 0 &&
					(pipelineConfigs[pipelineConfigNdx].stageFlags  & BBoxRenderCase::FLAG_TESSELLATION)    == 0)
				{
					// invalid config combination
				}
				else
				{
					tcu::TestCaseGroup* const pipelineGroup = new tcu::TestCaseGroup(m_testCtx, pipelineConfigs[pipelineConfigNdx].name, pipelineConfigs[pipelineConfigNdx].description);
					methodGroup->addChild(pipelineGroup);

					for (int usageNdx = 0; usageNdx < DE_LENGTH_OF_ARRAY(usageConfigs); ++usageNdx)
					{
						const deUint32 flags = primitiveTypes[primitiveTypeNdx].flags         |
											   stateSetMethods[stateSetMethodNdx].methodFlags |
											   pipelineConfigs[pipelineConfigNdx].stageFlags  |
											   usageConfigs[usageNdx].flags;

						if (usageConfigs[usageNdx].invalidFlags && (flags & usageConfigs[usageNdx].invalidFlags) != 0)
							continue;
						if (usageConfigs[usageNdx].requiredFlags && (flags & usageConfigs[usageNdx].requiredFlags) == 0)
							continue;

						switch (primitiveTypes[primitiveTypeNdx].type)
						{
							case TYPE_TRIANGLE:
								pipelineGroup->addChild(new GridRenderCase(m_context, usageConfigs[usageNdx].name, usageConfigs[usageNdx].description, flags));
								break;
							case TYPE_LINE:
								pipelineGroup->addChild(new LineRenderCase(m_context, usageConfigs[usageNdx].name, usageConfigs[usageNdx].description, flags));
								break;
							case TYPE_POINT:
								pipelineGroup->addChild(new PointRenderCase(m_context, usageConfigs[usageNdx].name, usageConfigs[usageNdx].description, flags));
								break;
							default:
								DE_ASSERT(false);
						}
					}
				}
			}
		}
	}

	// .depth
	{
		static const struct
		{
			const char*					name;
			const char*					description;
			DepthDrawCase::DepthType	depthMethod;
		} depthMethods[] =
		{
			{
				"builtin_depth",
				"Fragment depth not modified in fragment shader",
				DepthDrawCase::DEPTH_BUILTIN
			},
			{
				"user_defined_depth",
				"Fragment depth is defined in the fragment shader",
				DepthDrawCase::DEPTH_USER_DEFINED
			},
		};
		static const struct
		{
			const char*					name;
			const char*					description;
			DepthDrawCase::BBoxState	bboxState;
			DepthDrawCase::BBoxSize		bboxSize;
		} depthCases[] =
		{
			{
				"global_state_bbox_equal",
				"Test tight bounding box with global bbox state",
				DepthDrawCase::STATE_GLOBAL,
				DepthDrawCase::BBOX_EQUAL,
			},
			{
				"global_state_bbox_larger",
				"Test padded bounding box with global bbox state",
				DepthDrawCase::STATE_GLOBAL,
				DepthDrawCase::BBOX_LARGER,
			},
			{
				"per_primitive_bbox_equal",
				"Test tight bounding box with tessellation output bbox",
				DepthDrawCase::STATE_PER_PRIMITIVE,
				DepthDrawCase::BBOX_EQUAL,
			},
			{
				"per_primitive_bbox_larger",
				"Test padded bounding box with tessellation output bbox",
				DepthDrawCase::STATE_PER_PRIMITIVE,
				DepthDrawCase::BBOX_LARGER,
			},
		};

		tcu::TestCaseGroup* const depthGroup = new tcu::TestCaseGroup(m_testCtx, "depth", "Test bounding box depth component");
		addChild(depthGroup);

		// .builtin_depth
		// .user_defined_depth
		for (int depthNdx = 0; depthNdx < DE_LENGTH_OF_ARRAY(depthMethods); ++depthNdx)
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, depthMethods[depthNdx].name, depthMethods[depthNdx].description);
			depthGroup->addChild(group);

			for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(depthCases); ++caseNdx)
				group->addChild(new DepthDrawCase(m_context, depthCases[caseNdx].name, depthCases[caseNdx].description, depthMethods[depthNdx].depthMethod, depthCases[caseNdx].bboxState, depthCases[caseNdx].bboxSize));
		}
	}

	// .blit_fbo
	{
		tcu::TestCaseGroup* const blitFboGroup = new tcu::TestCaseGroup(m_testCtx, "blit_fbo", "Test bounding box does not affect blitting");
		addChild(blitFboGroup);

		blitFboGroup->addChild(new BlitFboCase(m_context, "blit_default_to_fbo", "Blit from default fb to fbo", BlitFboCase::TARGET_DEFAULT, BlitFboCase::TARGET_FBO));
		blitFboGroup->addChild(new BlitFboCase(m_context, "blit_fbo_to_default", "Blit from fbo to default fb", BlitFboCase::TARGET_FBO,     BlitFboCase::TARGET_DEFAULT));
		blitFboGroup->addChild(new BlitFboCase(m_context, "blit_fbo_to_fbo",     "Blit from fbo to fbo",        BlitFboCase::TARGET_FBO,     BlitFboCase::TARGET_FBO));
	}

	// .clear
	{
		tcu::TestCaseGroup* const clearGroup = new tcu::TestCaseGroup(m_testCtx, "clear", "Test bounding box does not clears");
		addChild(clearGroup);

		clearGroup->addChild(new ClearCase(m_context, "full_clear",                                             "Do full clears",                                               0));
		clearGroup->addChild(new ClearCase(m_context, "full_clear_with_triangles",                              "Do full clears and render some geometry",                      ClearCase::DRAW_TRIANGLE_BIT));
		clearGroup->addChild(new ClearCase(m_context, "full_clear_with_triangles_per_primitive_bbox",           "Do full clears and render some geometry",                      ClearCase::DRAW_TRIANGLE_BIT | ClearCase::PER_PRIMITIVE_BBOX_BIT));
		clearGroup->addChild(new ClearCase(m_context, "scissored_clear",                                        "Do scissored clears",                                          ClearCase::SCISSOR_CLEAR_BIT));
		clearGroup->addChild(new ClearCase(m_context, "scissored_clear_with_triangles",                         "Do scissored clears and render some geometry",                 ClearCase::SCISSOR_CLEAR_BIT | ClearCase::DRAW_TRIANGLE_BIT));
		clearGroup->addChild(new ClearCase(m_context, "scissored_clear_with_triangles_per_primitive_bbox",      "Do scissored clears and render some geometry",                 ClearCase::SCISSOR_CLEAR_BIT | ClearCase::DRAW_TRIANGLE_BIT | ClearCase::PER_PRIMITIVE_BBOX_BIT));
		clearGroup->addChild(new ClearCase(m_context, "scissored_full_clear",                                   "Do full clears with enabled scissor",                          ClearCase::FULLSCREEN_SCISSOR_BIT | ClearCase::SCISSOR_CLEAR_BIT));
		clearGroup->addChild(new ClearCase(m_context, "scissored_full_clear_with_triangles",                    "Do full clears with enabled scissor and render some geometry", ClearCase::FULLSCREEN_SCISSOR_BIT | ClearCase::SCISSOR_CLEAR_BIT | ClearCase::DRAW_TRIANGLE_BIT));
		clearGroup->addChild(new ClearCase(m_context, "scissored_full_clear_with_triangles_per_primitive_bbox", "Do full clears with enabled scissor and render some geometry", ClearCase::FULLSCREEN_SCISSOR_BIT | ClearCase::SCISSOR_CLEAR_BIT | ClearCase::DRAW_TRIANGLE_BIT | ClearCase::PER_PRIMITIVE_BBOX_BIT));
	}

	// .call_order (Khronos bug #13262)
	{
		tcu::TestCaseGroup* const callOrderGroup = new tcu::TestCaseGroup(m_testCtx, "call_order", "Test viewport and bounding box calls have no effect");
		addChild(callOrderGroup);

		callOrderGroup->addChild(new ViewportCallOrderCase(m_context, "viewport_first_bbox_second", "Set up viewport first and bbox after", ViewportCallOrderCase::VIEWPORT_FIRST));
		callOrderGroup->addChild(new ViewportCallOrderCase(m_context, "bbox_first_viewport_second", "Set up bbox first and viewport after", ViewportCallOrderCase::BBOX_FIRST));
	}
}

} // Functional
} // gles31
} // deqp
