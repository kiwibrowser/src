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
 * \brief Integer state query tests
 *//*--------------------------------------------------------------------*/

#include "es31fIntegerStateQueryTests.hpp"
#include "tcuTestLog.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "glsStateQueryUtil.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace gls::StateQueryUtil;

const int MAX_FRAG_ATOMIC_COUNTER_BUFFERS_GLES32	= 1;
const int MAX_FRAG_ATOMIC_COUNTERS_GLES32			= 8;
const int MAX_FRAG_SHADER_STORAGE_BLOCKS_GLES32		= 4;

static const char* getVerifierSuffix (QueryType type)
{
	switch (type)
	{
		case QUERY_BOOLEAN:		return "getboolean";
		case QUERY_INTEGER:		return "getinteger";
		case QUERY_INTEGER64:	return "getinteger64";
		case QUERY_FLOAT:		return "getfloat";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

class MaxSamplesCase : public TestCase
{
public:
						MaxSamplesCase	(Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType);
private:
	IterateResult		iterate			(void);

	const glw::GLenum	m_target;
	const int			m_minValue;
	const QueryType		m_verifierType;
};

MaxSamplesCase::MaxSamplesCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_minValue		(minValue)
	, m_verifierType	(verifierType)
{
}

MaxSamplesCase::IterateResult MaxSamplesCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	verifyStateIntegerMin(result, gl, m_target, m_minValue, m_verifierType);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TexBindingCase : public TestCase
{
public:
						TexBindingCase	(Context& context, const char* name, const char* desc, glw::GLenum texTarget, glw::GLenum bindTarget, QueryType verifierType);
private:
	void				init			(void);
	IterateResult		iterate			(void);

	const glw::GLenum	m_texTarget;
	const glw::GLenum	m_bindTarget;
	const QueryType		m_verifierType;
};

TexBindingCase::TexBindingCase (Context& context, const char* name, const char* desc, glw::GLenum texTarget, glw::GLenum bindTarget, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_texTarget		(texTarget)
	, m_bindTarget		(bindTarget)
	, m_verifierType	(verifierType)
{
}

void TexBindingCase::init (void)
{
	if (contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		return;

	if (m_texTarget == GL_TEXTURE_2D_MULTISAMPLE_ARRAY && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
		throw tcu::NotSupportedError("Test requires OES_texture_storage_multisample_2d_array extension");
	if (m_texTarget == GL_TEXTURE_CUBE_MAP_ARRAY && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_cube_map_array"))
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_cube_map_array extension");
	if (m_texTarget == GL_TEXTURE_BUFFER && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"))
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_buffer extension");
}

TexBindingCase::IterateResult TexBindingCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial value");

		verifyStateInteger(result, gl, m_bindTarget, 0, m_verifierType);
	}

	// bind
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "bind", "After bind");

		glw::GLuint texture;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(m_texTarget, texture);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind texture");

		verifyStateInteger(result, gl, m_bindTarget, texture, m_verifierType);

		gl.glDeleteTextures(1, &texture);
	}

	// after delete
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "bind", "After delete");

		verifyStateInteger(result, gl, m_bindTarget, 0, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class MinimumValueCase : public TestCase
{
public:
						MinimumValueCase	(Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType);
						MinimumValueCase	(Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType, glu::ApiType minVersion);
private:
	IterateResult		iterate				(void);

	const glw::GLenum	m_target;
	const int			m_minValue;
	const QueryType		m_verifierType;
	const glu::ApiType	m_minimumVersion;
};

MinimumValueCase::MinimumValueCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_minValue		(minValue)
	, m_verifierType	(verifierType)
	, m_minimumVersion	(glu::ApiType::es(3, 1))
{
}

MinimumValueCase::MinimumValueCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType, glu::ApiType minVersion)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_minValue		(minValue)
	, m_verifierType	(verifierType)
	, m_minimumVersion	(minVersion)
{
}

MinimumValueCase::IterateResult MinimumValueCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(m_context.getRenderContext().getType(), m_minimumVersion), "Test not supported in this context version.");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	// \note: In GL ES 3.2, the following targets have different limits as in 3.1
	const int				value	= contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2))
									? (m_target == GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS	? MAX_FRAG_ATOMIC_COUNTER_BUFFERS_GLES32	// 1
									: m_target == GL_MAX_FRAGMENT_ATOMIC_COUNTERS			? MAX_FRAG_ATOMIC_COUNTERS_GLES32			// 8
									: m_target == GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS		? MAX_FRAG_SHADER_STORAGE_BLOCKS_GLES32		// 4
									: m_minValue)
									: m_minValue;

	gl.enableLogging(true);
	verifyStateIntegerMin(result, gl, m_target, value, m_verifierType);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class AlignmentCase : public TestCase
{
public:
						AlignmentCase	(Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType);
						AlignmentCase	(Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType, glu::ApiType minVersion);
private:
	IterateResult		iterate			(void);

	const glw::GLenum	m_target;
	const int			m_minValue;
	const QueryType		m_verifierType;
	const glu::ApiType	m_minimumVersion;
};

AlignmentCase::AlignmentCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_minValue		(minValue)
	, m_verifierType	(verifierType)
	, m_minimumVersion	(glu::ApiType::es(3, 1))
{
}

AlignmentCase::AlignmentCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, QueryType verifierType, glu::ApiType minVersion)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_minValue		(minValue)
	, m_verifierType	(verifierType)
	, m_minimumVersion	(minVersion)
{
}

AlignmentCase::IterateResult AlignmentCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(m_context.getRenderContext().getType(), m_minimumVersion), "Test not supported in this context.");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	verifyStateIntegerMax(result, gl, m_target, m_minValue, m_verifierType);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BufferBindingCase : public TestCase
{
public:
						BufferBindingCase	(Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bindingPoint, QueryType verifierType);
private:
	IterateResult		iterate				(void);

	const glw::GLenum	m_queryTarget;
	const glw::GLenum	m_bindingPoint;
	const QueryType		m_verifierType;
};

BufferBindingCase::BufferBindingCase (Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bindingPoint, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_queryTarget		(queryTarget)
	, m_bindingPoint	(bindingPoint)
	, m_verifierType	(verifierType)
{
}

BufferBindingCase::IterateResult BufferBindingCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Initial", "Initial value");

		verifyStateInteger(result, gl, m_queryTarget, 0, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	section	(m_testCtx.getLog(), "AfterBinding", "After binding");
		glu::Buffer					buf		(m_context.getRenderContext());

		gl.glBindBuffer(m_bindingPoint, *buf);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "setup");

		verifyStateInteger(result, gl, m_queryTarget, *buf, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	section	(m_testCtx.getLog(), "AfterDelete", "After deleting");
		glw::GLuint					buf		= 0;

		gl.glGenBuffers(1, &buf);
		gl.glBindBuffer(m_bindingPoint, buf);
		gl.glDeleteBuffers(1, &buf);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "setup");

		verifyStateInteger(result, gl, m_queryTarget, 0, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ProgramPipelineBindingCase : public TestCase
{
public:
						ProgramPipelineBindingCase	(Context& context, const char* name, const char* desc, QueryType verifierType);
private:
	IterateResult		iterate						(void);

	const QueryType		m_verifierType;
};

ProgramPipelineBindingCase::ProgramPipelineBindingCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

ProgramPipelineBindingCase::IterateResult ProgramPipelineBindingCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Initial", "Initial value");

		verifyStateInteger(result, gl, GL_PROGRAM_PIPELINE_BINDING, 0, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "AfterBinding", "After binding");
		glu::ProgramPipeline		pipeline	(m_context.getRenderContext());

		gl.glBindProgramPipeline(pipeline.getPipeline());
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "setup");

		verifyStateInteger(result, gl, GL_PROGRAM_PIPELINE_BINDING, pipeline.getPipeline(), m_verifierType);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "AfterDelete", "After deleting");
		glw::GLuint					pipeline	= 0;

		gl.glGenProgramPipelines(1, &pipeline);
		gl.glBindProgramPipeline(pipeline);
		gl.glDeleteProgramPipelines(1, &pipeline);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "setup");

		verifyStateInteger(result, gl, GL_PROGRAM_PIPELINE_BINDING, 0, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class FramebufferMinimumValueCase : public TestCase
{
public:
						FramebufferMinimumValueCase	(Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, glw::GLenum tiedTo, QueryType verifierType);
private:
	IterateResult		iterate						(void);

	const glw::GLenum	m_target;
	const glw::GLenum	m_tiedTo;
	const int			m_minValue;
	const QueryType		m_verifierType;
};

FramebufferMinimumValueCase::FramebufferMinimumValueCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minValue, glw::GLenum tiedTo, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_tiedTo			(tiedTo)
	, m_minValue		(minValue)
	, m_verifierType	(verifierType)
{
}

FramebufferMinimumValueCase::IterateResult FramebufferMinimumValueCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Minimum", "Specified minimum is " + de::toString(m_minValue));

		verifyStateIntegerMin(result, gl, m_target, m_minValue, m_verifierType);
	}
	{
		const tcu::ScopedLogSection				section		(m_testCtx.getLog(), "Ties", "The limit is tied to the value of " + de::toString(glu::getGettableStateStr(m_tiedTo)));
		StateQueryMemoryWriteGuard<glw::GLint>	tiedToValue;

		gl.glGetIntegerv(m_tiedTo, &tiedToValue);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

		if (tiedToValue.verifyValidity(result))
			verifyStateIntegerMin(result, gl, m_target, tiedToValue, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class LegacyVectorLimitCase : public TestCase
{
public:
						LegacyVectorLimitCase	(Context& context, const char* name, const char* desc, glw::GLenum legacyTarget, glw::GLenum componentTarget, QueryType verifierType);
private:
	IterateResult		iterate					(void);

	const glw::GLenum	m_legacyTarget;
	const glw::GLenum	m_componentTarget;
	const QueryType		m_verifierType;
};

LegacyVectorLimitCase::LegacyVectorLimitCase (Context& context, const char* name, const char* desc, glw::GLenum legacyTarget, glw::GLenum componentTarget, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_legacyTarget	(legacyTarget)
	, m_componentTarget	(componentTarget)
	, m_verifierType	(verifierType)
{
}

LegacyVectorLimitCase::IterateResult LegacyVectorLimitCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "TiedTo", de::toString(glu::getGettableStateStr(m_legacyTarget)) +
																			" is " +
																			de::toString(glu::getGettableStateStr(m_componentTarget)) +
																			" divided by four");

		StateQueryMemoryWriteGuard<glw::GLint> value;
		gl.glGetIntegerv(m_componentTarget, &value);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

		if (value.verifyValidity(result))
			verifyStateInteger(result, gl, m_legacyTarget, ((int)value) / 4, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class CombinedUniformComponentsCase : public TestCase
{
public:
						CombinedUniformComponentsCase	(Context& context, const char* name, const char* desc, glw::GLenum target, QueryType verifierType);
						CombinedUniformComponentsCase	(Context& context, const char* name, const char* desc, glw::GLenum target, QueryType verifierType, glu::ApiType minVersion);
private:
	IterateResult		iterate									(void);
	const glw::GLenum	m_target;
	const QueryType		m_verifierType;
	const glu::ApiType	m_minimumVersion;
};

CombinedUniformComponentsCase::CombinedUniformComponentsCase (Context& context, const char* name, const char* desc, glw::GLenum target, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_verifierType	(verifierType)
	, m_minimumVersion	(glu::ApiType::es(3, 1))
{
}

CombinedUniformComponentsCase::CombinedUniformComponentsCase (Context& context, const char* name, const char* desc, glw::GLenum target, QueryType verifierType, glu::ApiType minVersion)
	: TestCase			(context, name, desc)
	, m_target			(target)
	, m_verifierType	(verifierType)
	, m_minimumVersion	(minVersion)
{
}

CombinedUniformComponentsCase::IterateResult CombinedUniformComponentsCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(m_context.getRenderContext().getType(), m_minimumVersion), "Test not supported in this context.");

	glu::CallLogWrapper		gl							(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result						(m_testCtx.getLog(), " // ERROR: ");

	const glw::GLenum		maxUniformBlocksEnum		= (m_target == GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS) ? GL_MAX_COMPUTE_UNIFORM_BLOCKS
														: (m_target == GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS) ? GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS
														: (m_target == GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS) ? GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS
														: (m_target == GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS) ? GL_MAX_GEOMETRY_UNIFORM_BLOCKS
														: -1;

	const glw::GLenum		maxUniformComponentsEnum	= (m_target == GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS) ? GL_MAX_COMPUTE_UNIFORM_COMPONENTS
														: (m_target == GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS) ? GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS
														: (m_target == GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS) ? GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS
														: (m_target == GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS) ? GL_MAX_GEOMETRY_UNIFORM_COMPONENTS
														: -1;

	gl.enableLogging(true);

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "The minimum value of MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS is MAX_COMPUTE_UNIFORM_BLOCKS x MAX_UNIFORM_BLOCK_SIZE / 4 + MAX_COMPUTE_UNIFORM_COMPONENTS"
						<< tcu::TestLog::EndMessage;

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformBlocks;
	gl.glGetIntegerv(maxUniformBlocksEnum, &maxUniformBlocks);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformBlockSize;
	gl.glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformComponents;
	gl.glGetIntegerv(maxUniformComponentsEnum, &maxUniformComponents);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	if (maxUniformBlocks.verifyValidity(result) && maxUniformBlockSize.verifyValidity(result) && maxUniformComponents.verifyValidity(result))
		verifyStateIntegerMin(result, gl, m_target, ((int)maxUniformBlocks) * ((int)maxUniformBlockSize) / 4 + (int)maxUniformComponents, m_verifierType);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TextureGatherLimitCase : public TestCase
{
public:
						TextureGatherLimitCase	(Context& context, const char* name, const char* desc, bool isMaxCase, QueryType verifierType);
private:
	IterateResult		iterate					(void);

	const bool			m_isMaxCase;
	const QueryType		m_verifierType;
};

TextureGatherLimitCase::TextureGatherLimitCase (Context& context, const char* name, const char* desc, bool isMaxCase, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_isMaxCase		(isMaxCase)
	, m_verifierType	(verifierType)
{
}

TextureGatherLimitCase::IterateResult TextureGatherLimitCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	if (m_isMaxCase)
	{
		// range [0, inf)
		verifyStateIntegerMin(result, gl, GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, 0, m_verifierType);
	}
	else
	{
		// range (-inf, 0]
		verifyStateIntegerMax(result, gl, GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, 0, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class MaxUniformBufferBindingsCase : public TestCase
{
public:
						MaxUniformBufferBindingsCase	(Context& context, const char* name, const char* desc, QueryType verifierType);
private:
	IterateResult		iterate							(void);

	const QueryType		m_verifierType;
};

MaxUniformBufferBindingsCase::MaxUniformBufferBindingsCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

MaxUniformBufferBindingsCase::IterateResult MaxUniformBufferBindingsCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	int						minMax;

	gl.enableLogging(true);

	if (contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		minMax = 72;
	}
	else
	{
		if (m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		{
			m_testCtx.getLog()	<< tcu::TestLog::Message
								<< "GL_EXT_tessellation_shader increases the minimum value of GL_MAX_UNIFORM_BUFFER_BINDINGS to 72"
								<< tcu::TestLog::EndMessage;
			minMax = 72;
		}
		else if (m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		{
			m_testCtx.getLog()	<< tcu::TestLog::Message
								<< "GL_EXT_geometry_shader increases the minimum value of GL_MAX_UNIFORM_BUFFER_BINDINGS to 48"
								<< tcu::TestLog::EndMessage;
			minMax = 48;
		}
		else
		{
			minMax = 36;
		}
	}

	// range [0, inf)
	verifyStateIntegerMin(result, gl, GL_MAX_UNIFORM_BUFFER_BINDINGS, minMax, m_verifierType);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class MaxCombinedUniformBlocksCase : public TestCase
{
public:
						MaxCombinedUniformBlocksCase	(Context& context, const char* name, const char* desc, QueryType verifierType);
private:
	IterateResult		iterate							(void);

	const QueryType		m_verifierType;
};

MaxCombinedUniformBlocksCase::MaxCombinedUniformBlocksCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

MaxCombinedUniformBlocksCase::IterateResult MaxCombinedUniformBlocksCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	int						minMax;

	gl.enableLogging(true);

	if (contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		minMax = 60;
	}
	else
	{
		if (m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		{
			m_testCtx.getLog()	<< tcu::TestLog::Message
								<< "GL_EXT_tessellation_shader increases the minimum value of GL_MAX_COMBINED_UNIFORM_BLOCKS to 60"
								<< tcu::TestLog::EndMessage;
			minMax = 60;
		}
		else if (m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		{
			m_testCtx.getLog()	<< tcu::TestLog::Message
								<< "GL_EXT_geometry_shader increases the minimum value of GL_MAX_COMBINED_UNIFORM_BLOCKS to 36"
								<< tcu::TestLog::EndMessage;
			minMax = 36;
		}
		else
		{
			minMax = 24;
		}
	}

	// range [0, inf)
	verifyStateIntegerMin(result, gl, GL_MAX_COMBINED_UNIFORM_BLOCKS, minMax, m_verifierType);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class MaxCombinedTexImageUnitsCase : public TestCase
{
public:
						MaxCombinedTexImageUnitsCase	(Context& context, const char* name, const char* desc, QueryType verifierType);
private:
	IterateResult		iterate							(void);

	const QueryType		m_verifierType;
};

MaxCombinedTexImageUnitsCase::MaxCombinedTexImageUnitsCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

MaxCombinedTexImageUnitsCase::IterateResult MaxCombinedTexImageUnitsCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	int						minMax;

	gl.enableLogging(true);
	if (contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		minMax = 96;
	}
	else
	{
		if (m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		{
			m_testCtx.getLog()	<< tcu::TestLog::Message
								<< "GL_EXT_tessellation_shader increases the minimum value of GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS to 96"
								<< tcu::TestLog::EndMessage;
			minMax = 96;
		}
		else if (m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		{
			m_testCtx.getLog()	<< tcu::TestLog::Message
								<< "GL_EXT_geometry_shader increases the minimum value of GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS to 36"
								<< tcu::TestLog::EndMessage;
			minMax = 64;
		}
		else
		{
			minMax = 48;
		}
	}

	// range [0, inf)
	verifyStateIntegerMin(result, gl, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, minMax, m_verifierType);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

IntegerStateQueryTests::IntegerStateQueryTests (Context& context)
	: TestCaseGroup(context, "integer", "Integer state query tests")
{
}

IntegerStateQueryTests::~IntegerStateQueryTests (void)
{
}

void IntegerStateQueryTests::init (void)
{
	// Verifiers
	const QueryType verifiers[]	= { QUERY_BOOLEAN, QUERY_INTEGER, QUERY_INTEGER64, QUERY_FLOAT };

#define FOR_EACH_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)	\
	{																						\
		const char* verifierSuffix = getVerifierSuffix(verifiers[verifierNdx]);				\
		const QueryType verifier = verifiers[verifierNdx];									\
		this->addChild(X);																	\
	}

	FOR_EACH_VERIFIER(new MaxSamplesCase(m_context,		(std::string() + "max_color_texture_samples_" + verifierSuffix).c_str(),				"Test GL_MAX_COLOR_TEXTURE_SAMPLES",			GL_MAX_COLOR_TEXTURE_SAMPLES,		1,	verifier))
	FOR_EACH_VERIFIER(new MaxSamplesCase(m_context,		(std::string() + "max_depth_texture_samples_" + verifierSuffix).c_str(),				"Test GL_MAX_DEPTH_TEXTURE_SAMPLES",			GL_MAX_DEPTH_TEXTURE_SAMPLES,		1,	verifier))
	FOR_EACH_VERIFIER(new MaxSamplesCase(m_context,		(std::string() + "max_integer_samples_" + verifierSuffix).c_str(),						"Test GL_MAX_INTEGER_SAMPLES",					GL_MAX_INTEGER_SAMPLES,				1,	verifier))

	FOR_EACH_VERIFIER(new TexBindingCase(m_context,		(std::string() + "texture_binding_2d_multisample_" + verifierSuffix).c_str(),			"Test TEXTURE_BINDING_2D_MULTISAMPLE",			GL_TEXTURE_2D_MULTISAMPLE,			GL_TEXTURE_BINDING_2D_MULTISAMPLE,			verifier))
	FOR_EACH_VERIFIER(new TexBindingCase(m_context,		(std::string() + "texture_binding_2d_multisample_array_" + verifierSuffix).c_str(),		"Test TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY",	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,	GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY,	verifier))
	FOR_EACH_VERIFIER(new TexBindingCase(m_context,		(std::string() + "texture_binding_cube_map_array_" + verifierSuffix).c_str(),			"Test TEXTURE_BINDING_CUBE_MAP_ARRAY",			GL_TEXTURE_CUBE_MAP_ARRAY,			GL_TEXTURE_BINDING_CUBE_MAP_ARRAY,			verifier))
	FOR_EACH_VERIFIER(new TexBindingCase(m_context,		(std::string() + "texture_binding_buffer_" + verifierSuffix).c_str(),					"Test TEXTURE_BINDING_BUFFER",					GL_TEXTURE_BUFFER,					GL_TEXTURE_BINDING_BUFFER,					verifier))

	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_attrib_relative_offset_" + verifierSuffix).c_str(),		"Test MAX_VERTEX_ATTRIB_RELATIVE_OFFSET",		GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET,	2047,	verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_attrib_bindings_" + verifierSuffix).c_str(),				"Test MAX_VERTEX_ATTRIB_BINDINGS",				GL_MAX_VERTEX_ATTRIB_BINDINGS,			16,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_attrib_stride_" + verifierSuffix).c_str(),					"Test MAX_VERTEX_ATTRIB_STRIDE",				GL_MAX_VERTEX_ATTRIB_STRIDE,			2048,	verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_sample_mask_words_" + verifierSuffix).c_str(),					"Test MAX_SAMPLE_MASK_WORDS",					GL_MAX_SAMPLE_MASK_WORDS,				1,		verifier))

	FOR_EACH_VERIFIER(new AlignmentCase(m_context,		(std::string() + "shader_storage_buffer_offset_alignment_" + verifierSuffix).c_str(),	"Test SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT",	GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,	256,	verifier))

	FOR_EACH_VERIFIER(new BufferBindingCase(m_context,	(std::string() + "draw_indirect_buffer_binding_" + verifierSuffix).c_str(),				"Test DRAW_INDIRECT_BUFFER_BINDING",			GL_DRAW_INDIRECT_BUFFER_BINDING,		GL_DRAW_INDIRECT_BUFFER,		verifier))
	FOR_EACH_VERIFIER(new BufferBindingCase(m_context,	(std::string() + "atomic_counter_buffer_binding_" + verifierSuffix).c_str(),			"Test ATOMIC_COUNTER_BUFFER_BINDING",			GL_ATOMIC_COUNTER_BUFFER_BINDING,		GL_ATOMIC_COUNTER_BUFFER,		verifier))
	FOR_EACH_VERIFIER(new BufferBindingCase(m_context,	(std::string() + "shader_storage_buffer_binding_" + verifierSuffix).c_str(),			"Test SHADER_STORAGE_BUFFER_BINDING",			GL_SHADER_STORAGE_BUFFER_BINDING,		GL_SHADER_STORAGE_BUFFER,		verifier))
	FOR_EACH_VERIFIER(new BufferBindingCase(m_context,	(std::string() + "dispatch_indirect_buffer_binding_" + verifierSuffix).c_str(),			"Test DISPATCH_INDIRECT_BUFFER_BINDING",		GL_DISPATCH_INDIRECT_BUFFER_BINDING,	GL_DISPATCH_INDIRECT_BUFFER,	verifier))

	FOR_EACH_VERIFIER(new FramebufferMinimumValueCase(m_context,	(std::string() + "max_framebuffer_width_" + verifierSuffix).c_str(),		"Test MAX_FRAMEBUFFER_WIDTH",					GL_MAX_FRAMEBUFFER_WIDTH,				2048,	GL_MAX_TEXTURE_SIZE,	verifier))
	FOR_EACH_VERIFIER(new FramebufferMinimumValueCase(m_context,	(std::string() + "max_framebuffer_height_" + verifierSuffix).c_str(),		"Test MAX_FRAMEBUFFER_HEIGHT",					GL_MAX_FRAMEBUFFER_HEIGHT,				2048,	GL_MAX_TEXTURE_SIZE,	verifier))
	FOR_EACH_VERIFIER(new FramebufferMinimumValueCase(m_context,	(std::string() + "max_framebuffer_samples_" + verifierSuffix).c_str(),		"Test MAX_FRAMEBUFFER_SAMPLES",					GL_MAX_FRAMEBUFFER_SAMPLES,				4,		GL_MAX_SAMPLES,			verifier))

	FOR_EACH_VERIFIER(new ProgramPipelineBindingCase(m_context,	(std::string() + "program_pipeline_binding_" + verifierSuffix).c_str(),			"Test PROGRAM_PIPELINE_BINDING",	verifier))

	// vertex
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_atomic_counter_buffers_" + verifierSuffix).c_str(),		"Test MAX_VERTEX_ATOMIC_COUNTER_BUFFERS",		GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS,	0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_atomic_counters_" + verifierSuffix).c_str(),				"Test MAX_VERTEX_ATOMIC_COUNTERS",				GL_MAX_VERTEX_ATOMIC_COUNTERS,			0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_image_uniforms_" + verifierSuffix).c_str(),				"Test MAX_VERTEX_IMAGE_UNIFORMS",				GL_MAX_VERTEX_IMAGE_UNIFORMS,			0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_shader_storage_blocks_" + verifierSuffix).c_str(),			"Test MAX_VERTEX_SHADER_STORAGE_BLOCKS",		GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,	0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_vertex_uniform_components_" + verifierSuffix).c_str(),			"Test MAX_VERTEX_UNIFORM_COMPONENTS",			GL_MAX_VERTEX_UNIFORM_COMPONENTS,		1024,	verifier))

	// fragment
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_fragment_atomic_counter_buffers_" + verifierSuffix).c_str(),		"Test MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS",		GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS,	0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_fragment_atomic_counters_" + verifierSuffix).c_str(),				"Test MAX_FRAGMENT_ATOMIC_COUNTERS",			GL_MAX_FRAGMENT_ATOMIC_COUNTERS,		0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_fragment_image_uniforms_" + verifierSuffix).c_str(),				"Test MAX_FRAGMENT_IMAGE_UNIFORMS",				GL_MAX_FRAGMENT_IMAGE_UNIFORMS,			0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_fragment_shader_storage_blocks_" + verifierSuffix).c_str(),		"Test MAX_FRAGMENT_SHADER_STORAGE_BLOCKS",		GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,	0,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_fragment_uniform_components_" + verifierSuffix).c_str(),			"Test MAX_FRAGMENT_UNIFORM_COMPONENTS",			GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,		1024,	verifier))

	// compute
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_work_group_invocations_" + verifierSuffix).c_str(),		"Test MAX_COMPUTE_WORK_GROUP_INVOCATIONS",		GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,		128,	verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_uniform_blocks_" + verifierSuffix).c_str(),				"Test MAX_COMPUTE_UNIFORM_BLOCKS",				GL_MAX_COMPUTE_UNIFORM_BLOCKS,				12,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_texture_image_units_" + verifierSuffix).c_str(),			"Test MAX_COMPUTE_TEXTURE_IMAGE_UNITS",			GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,			16,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_shared_memory_size_" + verifierSuffix).c_str(),			"Test MAX_COMPUTE_SHARED_MEMORY_SIZE",			GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,			16384,	verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_uniform_components_" + verifierSuffix).c_str(),			"Test MAX_COMPUTE_UNIFORM_COMPONENTS",			GL_MAX_COMPUTE_UNIFORM_COMPONENTS,			1024,	verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_atomic_counter_buffers_" + verifierSuffix).c_str(),		"Test MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS",		GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS,		1,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_atomic_counters_" + verifierSuffix).c_str(),				"Test MAX_COMPUTE_ATOMIC_COUNTERS",				GL_MAX_COMPUTE_ATOMIC_COUNTERS,				8,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_image_uniforms_" + verifierSuffix).c_str(),				"Test MAX_COMPUTE_IMAGE_UNIFORMS",				GL_MAX_COMPUTE_IMAGE_UNIFORMS,				4,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_compute_shader_storage_blocks_" + verifierSuffix).c_str(),		"Test MAX_COMPUTE_SHADER_STORAGE_BLOCKS",		GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,		4,		verifier))

	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_uniform_locations_" + verifierSuffix).c_str(),					"Test MAX_UNIFORM_LOCATIONS",					GL_MAX_UNIFORM_LOCATIONS,					1024,	verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_atomic_counter_buffer_bindings_" + verifierSuffix).c_str(),		"Test MAX_ATOMIC_COUNTER_BUFFER_BINDINGS",		GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,		1,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_atomic_counter_buffer_size_" + verifierSuffix).c_str(),			"Test MAX_ATOMIC_COUNTER_BUFFER_SIZE",			GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE,			32,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_combined_atomic_counter_buffers_" + verifierSuffix).c_str(),		"Test MAX_COMBINED_ATOMIC_COUNTER_BUFFERS",		GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS,		1,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_combined_atomic_counters_" + verifierSuffix).c_str(),				"Test MAX_COMBINED_ATOMIC_COUNTERS",			GL_MAX_COMBINED_ATOMIC_COUNTERS,			8,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_image_units_" + verifierSuffix).c_str(),							"Test MAX_IMAGE_UNITS",							GL_MAX_IMAGE_UNITS,							4,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_combined_image_uniforms_" + verifierSuffix).c_str(),				"Test MAX_COMBINED_IMAGE_UNIFORMS",				GL_MAX_COMBINED_IMAGE_UNIFORMS,				4,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_shader_storage_buffer_bindings_" + verifierSuffix).c_str(),		"Test MAX_SHADER_STORAGE_BUFFER_BINDINGS",		GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,		4,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_shader_storage_block_size_" + verifierSuffix).c_str(),			"Test MAX_SHADER_STORAGE_BLOCK_SIZE",			GL_MAX_SHADER_STORAGE_BLOCK_SIZE,			1<<27,	verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_combined_shader_storage_blocks_" + verifierSuffix).c_str(),		"Test MAX_COMBINED_SHADER_STORAGE_BLOCKS",		GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,		4,		verifier))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_combined_shader_output_resources_" + verifierSuffix).c_str(),		"Test MAX_COMBINED_SHADER_OUTPUT_RESOURCES",	GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES,	4,		verifier))

	FOR_EACH_VERIFIER(new MaxUniformBufferBindingsCase	(m_context,	(std::string() + "max_uniform_buffer_bindings_" + verifierSuffix).c_str(),				"Test MAX_UNIFORM_BUFFER_BINDINGS",				verifier))
	FOR_EACH_VERIFIER(new MaxCombinedUniformBlocksCase	(m_context,	(std::string() + "max_combined_uniform_blocks_" + verifierSuffix).c_str(),				"Test MAX_COMBINED_UNIFORM_BLOCKS",				verifier))
	FOR_EACH_VERIFIER(new MaxCombinedTexImageUnitsCase	(m_context,	(std::string() + "max_combined_texture_image_units_" + verifierSuffix).c_str(),			"Test MAX_COMBINED_TEXTURE_IMAGE_UNITS",		verifier))
	FOR_EACH_VERIFIER(new CombinedUniformComponentsCase	(m_context,	(std::string() + "max_combined_compute_uniform_components_" + verifierSuffix).c_str(),	"Test MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS",	GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, verifier))

	FOR_EACH_VERIFIER(new LegacyVectorLimitCase(m_context,	(std::string() + "max_vertex_uniform_vectors_" + verifierSuffix).c_str(),			"Test MAX_VERTEX_UNIFORM_VECTORS",				GL_MAX_VERTEX_UNIFORM_VECTORS,			GL_MAX_VERTEX_UNIFORM_COMPONENTS,	verifier))
	FOR_EACH_VERIFIER(new LegacyVectorLimitCase(m_context,	(std::string() + "max_fragment_uniform_vectors_" + verifierSuffix).c_str(),			"Test MAX_FRAGMENT_UNIFORM_VECTORS",			GL_MAX_FRAGMENT_UNIFORM_VECTORS,		GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,	verifier))

	FOR_EACH_VERIFIER(new TextureGatherLimitCase(m_context,	(std::string() + "min_program_texture_gather_offset_" + verifierSuffix).c_str(),	"Test MIN_PROGRAM_TEXTURE_GATHER_OFFSET",		false,		verifier))
	FOR_EACH_VERIFIER(new TextureGatherLimitCase(m_context,	(std::string() + "max_program_texture_gather_offset_" + verifierSuffix).c_str(),	"Test MAX_PROGRAM_TEXTURE_GATHER_OFFSET",		true,		verifier))

	// GL ES 3.2 tests
	FOR_EACH_VERIFIER(new MinimumValueCase	(m_context,	(std::string() + "max_framebuffer_layers_" + verifierSuffix).c_str(),						"Test MAX_FRAMEBUFFER_LAYERS",						GL_MAX_FRAMEBUFFER_LAYERS,						256,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase	(m_context,	(std::string() + "fragment_interpolation_offset_bits_" + verifierSuffix).c_str(),			"Test FRAGMENT_INTERPOLATION_OFFSET_BITS",			GL_FRAGMENT_INTERPOLATION_OFFSET_BITS,			4,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase	(m_context,	(std::string() + "max_texture_buffer_size_" + verifierSuffix).c_str(),						"Test MAX_TEXTURE_BUFFER_SIZE",						GL_MAX_TEXTURE_BUFFER_SIZE,						65536,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new AlignmentCase		(m_context,	(std::string() + "texture_buffer_offset_alignment_" + verifierSuffix).c_str(),				"Test TEXTURE_BUFFER_OFFSET_ALIGNMENT",				GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT,				256,	verifier,	glu::ApiType::es(3, 2)))

	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_gen_level_" + verifierSuffix).c_str(),							"Test MAX_TESS_GEN_LEVEL",							GL_MAX_TESS_GEN_LEVEL,							64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_patch_vertices_" + verifierSuffix).c_str(),							"Test MAX_PATCH_VERTICES",							GL_MAX_PATCH_VERTICES,							32,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_patch_components_" + verifierSuffix).c_str(),					"Test MAX_TESS_PATCH_COMPONENTS",					GL_MAX_TESS_PATCH_COMPONENTS,					120,	verifier,	glu::ApiType::es(3, 2)))

	// tess control
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_uniform_components_" + verifierSuffix).c_str(),			"Test MAX_TESS_CONTROL_UNIFORM_COMPONENTS",			GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS,			1024,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_texture_image_units_" + verifierSuffix).c_str(),			"Test MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS",		GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,		16,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_output_components_" + verifierSuffix).c_str(),			"Test MAX_TESS_CONTROL_OUTPUT_COMPONENTS",			GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS,			64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_total_output_components_" + verifierSuffix).c_str(),		"Test MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS",	GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS,	2048,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_input_components_" + verifierSuffix).c_str(),			"Test MAX_TESS_CONTROL_INPUT_COMPONENTS",			GL_MAX_TESS_CONTROL_INPUT_COMPONENTS,			64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_uniform_blocks_" + verifierSuffix).c_str(),				"Test MAX_TESS_CONTROL_UNIFORM_BLOCKS",				GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,				12,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_atomic_counter_buffers_" + verifierSuffix).c_str(),		"Test MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS",		GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS,		0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_atomic_counters_" + verifierSuffix).c_str(),				"Test MAX_TESS_CONTROL_ATOMIC_COUNTERS",			GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS,			0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_shader_storage_blocks_" + verifierSuffix).c_str(),		"Test MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS",		GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,		0,		verifier,	glu::ApiType::es(3, 2)))

	// tess evaluation
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_uniform_components_" + verifierSuffix).c_str(),		"Test MAX_TESS_EVALUATION_UNIFORM_COMPONENTS",		GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS,		1024,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_texture_image_units_" + verifierSuffix).c_str(),		"Test MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS",		GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,		16,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_output_components_" + verifierSuffix).c_str(),		"Test MAX_TESS_EVALUATION_OUTPUT_COMPONENTS",		GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS,		64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_input_components_" + verifierSuffix).c_str(),			"Test MAX_TESS_EVALUATION_INPUT_COMPONENTS",		GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS,		64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_uniform_blocks_" + verifierSuffix).c_str(),			"Test MAX_TESS_EVALUATION_UNIFORM_BLOCKS",			GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,			12,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_atomic_counter_buffers_" + verifierSuffix).c_str(),	"Test MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS",	GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS,	0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_atomic_counters_" + verifierSuffix).c_str(),			"Test MAX_TESS_EVALUATION_ATOMIC_COUNTERS",			GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS,			0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_shader_storage_blocks_" + verifierSuffix).c_str(),	"Test MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS",	GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,	0,		verifier,	glu::ApiType::es(3, 2)))

	// geometry
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_uniform_components_" + verifierSuffix).c_str(),				"Test MAX_GEOMETRY_UNIFORM_COMPONENTS",				GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,				1024,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_uniform_blocks_" + verifierSuffix).c_str(),					"Test MAX_GEOMETRY_UNIFORM_BLOCKS",					GL_MAX_GEOMETRY_UNIFORM_BLOCKS,					12,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_input_components_" + verifierSuffix).c_str(),				"Test MAX_GEOMETRY_INPUT_COMPONENTS",				GL_MAX_GEOMETRY_INPUT_COMPONENTS,				64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_output_components_" + verifierSuffix).c_str(),				"Test MAX_GEOMETRY_OUTPUT_COMPONENTS",				GL_MAX_GEOMETRY_OUTPUT_COMPONENTS,				64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_output_vertices_" + verifierSuffix).c_str(),					"Test MAX_GEOMETRY_OUTPUT_VERTICES",				GL_MAX_GEOMETRY_OUTPUT_VERTICES,				256,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_total_output_components_" + verifierSuffix).c_str(),			"Test MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS",		GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS,		1024,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_texture_image_units_" + verifierSuffix).c_str(),				"Test MAX_GEOMETRY_TEXTURE_IMAGE_UNITS",			GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,			16,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_shader_invocations_" + verifierSuffix).c_str(),				"Test MAX_GEOMETRY_SHADER_INVOCATIONS",				GL_MAX_GEOMETRY_SHADER_INVOCATIONS,				32,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_atomic_counter_buffers_" + verifierSuffix).c_str(),			"Test MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS",			GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS,			0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_atomic_counters_" + verifierSuffix).c_str(),					"Test MAX_GEOMETRY_ATOMIC_COUNTERS",				GL_MAX_GEOMETRY_ATOMIC_COUNTERS,				0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_shader_storage_blocks_" + verifierSuffix).c_str(),			"Test MAX_GEOMETRY_SHADER_STORAGE_BLOCKS",			GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,			0,		verifier,	glu::ApiType::es(3, 2)))

	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_control_image_uniforms_" + verifierSuffix).c_str(),				"Test MAX_TESS_CONTROL_IMAGE_UNIFORMS",				GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,				0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_tess_evaluation_image_uniforms_" + verifierSuffix).c_str(),			"Test MAX_TESS_EVALUATION_IMAGE_UNIFORMS",			GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS,			0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_geometry_image_uniforms_" + verifierSuffix).c_str(),					"Test MAX_GEOMETRY_IMAGE_UNIFORMS",					GL_MAX_GEOMETRY_IMAGE_UNIFORMS,					0,		verifier,	glu::ApiType::es(3, 2)))

	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "debug_logged_messages_" + verifierSuffix).c_str(),						"Test DEBUG_LOGGED_MESSAGES",						GL_DEBUG_LOGGED_MESSAGES,						0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "debug_next_logged_message_length_" + verifierSuffix).c_str(),				"Test DEBUG_NEXT_LOGGED_MESSAGE_LENGTH",			GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH,			0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "debug_group_stack_depth_" + verifierSuffix).c_str(),						"Test DEBUG_GROUP_STACK_DEPTH",						GL_DEBUG_GROUP_STACK_DEPTH,						0,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_debug_message_length_" + verifierSuffix).c_str(),						"Test MAX_DEBUG_MESSAGE_LENGTH",					GL_MAX_DEBUG_MESSAGE_LENGTH,					1,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_debug_logged_messages_" + verifierSuffix).c_str(),					"Test MAX_DEBUG_LOGGED_MESSAGES",					GL_MAX_DEBUG_LOGGED_MESSAGES,					1,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_debug_group_stack_depth_" + verifierSuffix).c_str(),					"Test MAX_DEBUG_GROUP_STACK_DEPTH",					GL_MAX_DEBUG_GROUP_STACK_DEPTH,					64,		verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "max_label_length_" + verifierSuffix).c_str(),								"Test MAX_LABEL_LENGTH",							GL_MAX_LABEL_LENGTH,							256,	verifier,	glu::ApiType::es(3, 2)))

	FOR_EACH_VERIFIER(new MinimumValueCase(m_context,	(std::string() + "texture_buffer_binding_" + verifierSuffix).c_str(),						"Test TEXTURE_BUFFER_BINDING",						GL_TEXTURE_BUFFER_BINDING,						0,		verifier,	glu::ApiType::es(3, 2)))

	FOR_EACH_VERIFIER(new CombinedUniformComponentsCase	(m_context,	(std::string() + "max_combined_tess_control_uniform_components_" + verifierSuffix).c_str(),		"Test MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS",	GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS,	verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new CombinedUniformComponentsCase	(m_context,	(std::string() + "max_combined_tess_evaluation_uniform_components_" + verifierSuffix).c_str(),	"Test MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS",	GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS, verifier,	glu::ApiType::es(3, 2)))
	FOR_EACH_VERIFIER(new CombinedUniformComponentsCase	(m_context,	(std::string() + "max_combined_geometry_uniform_components_" + verifierSuffix).c_str(),			"Test MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS",		GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS,		verifier,	glu::ApiType::es(3, 2)))

#undef FOR_EACH_VERIFIER
}

} // Functional
} // gles31
} // deqp
