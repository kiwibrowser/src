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
 * \brief Indexed state query tests
 *//*--------------------------------------------------------------------*/

#include "es31fIndexedStateQueryTests.hpp"
#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluObjectWrapper.hpp"
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

static const char* getVerifierSuffix (QueryType type)
{
	switch (type)
	{
		case QUERY_INDEXED_BOOLEAN:			return "getbooleani_v";
		case QUERY_INDEXED_INTEGER:			return "getintegeri_v";
		case QUERY_INDEXED_INTEGER64:		return "getinteger64i_v";
		case QUERY_INDEXED_BOOLEAN_VEC4:	return "getbooleani_v";
		case QUERY_INDEXED_INTEGER_VEC4:	return "getintegeri_v";
		case QUERY_INDEXED_INTEGER64_VEC4:	return "getinteger64i_v";
		case QUERY_INDEXED_ISENABLED:		return "isenabledi";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

void isExtensionSupported (Context& context, std::string extensionName)
{
	if (extensionName == "GL_EXT_draw_buffers_indexed" || extensionName == "GL_KHR_blend_equation_advanced")
	{
		if (!contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !context.getContextInfo().isExtensionSupported(extensionName.c_str()))
			TCU_THROW(NotSupportedError, (std::string("Extension ") + extensionName + std::string(" not supported.")).c_str());
	}
	else if (!context.getContextInfo().isExtensionSupported(extensionName.c_str()))
		TCU_THROW(NotSupportedError, (std::string("Extension ") + extensionName + std::string(" not supported.")).c_str());
}

class SampleMaskCase : public TestCase
{
public:
						SampleMaskCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

private:
	void				init			(void);
	IterateResult		iterate			(void);

	const QueryType		m_verifierType;
	int					m_maxSampleMaskWords;
};

SampleMaskCase::SampleMaskCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase				(context, name, desc)
	, m_verifierType		(verifierType)
	, m_maxSampleMaskWords	(-1)
{
}

void SampleMaskCase::init (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &m_maxSampleMaskWords);
	GLU_EXPECT_NO_ERROR(gl.getError(), "query sample mask words");

	// mask word count ok?
	if (m_maxSampleMaskWords <= 0)
		throw tcu::TestError("Minimum value of GL_MAX_SAMPLE_MASK_WORDS is 1. Got " + de::toString(m_maxSampleMaskWords));

	m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_SAMPLE_MASK_WORDS = " << m_maxSampleMaskWords << tcu::TestLog::EndMessage;
}

SampleMaskCase::IterateResult SampleMaskCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// initial values
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial values");

		for (int ndx = 0; ndx < m_maxSampleMaskWords; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_SAMPLE_MASK_VALUE, ndx, -1, m_verifierType);
	}

	// fixed values
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "fixed", "Fixed values");

		for (int ndx = 0; ndx < m_maxSampleMaskWords; ++ndx)
		{
			gl.glSampleMaski(ndx, 0);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "glSampleMaski");

			verifyStateIndexedInteger(result, gl, GL_SAMPLE_MASK_VALUE, ndx, 0, m_verifierType);
		}
	}

	// random masks
	{
		const int					numRandomTest	= 20;
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "random", "Random values");
		de::Random					rnd				(0x4312);

		for (int testNdx = 0; testNdx < numRandomTest; ++testNdx)
		{
			const glw::GLint	maskIndex		= (glw::GLint)(rnd.getUint32() % m_maxSampleMaskWords);
			glw::GLint			mask			= (glw::GLint)(rnd.getUint32());

			gl.glSampleMaski(maskIndex, mask);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "glSampleMaski");

			verifyStateIndexedInteger(result, gl, GL_SAMPLE_MASK_VALUE, maskIndex, mask, m_verifierType);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class MinValueIndexed3Case : public TestCase
{
public:
						MinValueIndexed3Case	(Context& context, const char* name, const char* desc, glw::GLenum target, const tcu::IVec3& ref, QueryType verifierType);

private:
	IterateResult		iterate					(void);

	const glw::GLenum	m_target;
	const tcu::IVec3	m_ref;
	const QueryType		m_verifierType;
};

MinValueIndexed3Case::MinValueIndexed3Case (Context& context, const char* name, const char* desc, glw::GLenum target, const tcu::IVec3& ref, QueryType verifierType)
	: TestCase				(context, name, desc)
	, m_target				(target)
	, m_ref					(ref)
	, m_verifierType		(verifierType)
{
}

MinValueIndexed3Case::IterateResult MinValueIndexed3Case::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	for (int ndx = 0; ndx < 3; ++ndx)
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Element", "Element " + de::toString(ndx));

		verifyStateIndexedIntegerMin(result, gl, m_target, ndx, m_ref[ndx], m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BufferBindingCase : public TestCase
{
public:
						BufferBindingCase	(Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bufferTarget, glw::GLenum numBindingsTarget, QueryType verifierType);

private:
	IterateResult		iterate				(void);

	const glw::GLenum	m_queryTarget;
	const glw::GLenum	m_bufferTarget;
	const glw::GLenum	m_numBindingsTarget;
	const QueryType		m_verifierType;
};

BufferBindingCase::BufferBindingCase (Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bufferTarget, glw::GLenum numBindingsTarget, QueryType verifierType)
	: TestCase				(context, name, desc)
	, m_queryTarget			(queryTarget)
	, m_bufferTarget		(bufferTarget)
	, m_numBindingsTarget	(numBindingsTarget)
	, m_verifierType		(verifierType)
{
}

BufferBindingCase::IterateResult BufferBindingCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxBindings	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(m_numBindingsTarget, &maxBindings);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxBindings; ++ndx)
			verifyStateIndexedInteger(result, gl, m_queryTarget, ndx, 0, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Buffer					bufferA			(m_context.getRenderContext());
		glu::Buffer					bufferB			(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxBindings / 2;

		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "Generic", "After setting generic binding point");

			gl.glBindBuffer(m_bufferTarget, *bufferA);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "glBindBuffer");

			verifyStateIndexedInteger(result, gl, m_queryTarget, 0, 0, m_verifierType);
		}
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Indexed", "After setting with glBindBufferBase");

			gl.glBindBufferBase(m_bufferTarget, ndxA, *bufferA);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "glBindBufferBase");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxA, *bufferA, m_verifierType);
		}
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Indexed", "After setting with glBindBufferRange");

			gl.glBindBufferRange(m_bufferTarget, ndxB, *bufferB, 0, 8);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "glBindBufferRange");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxB, *bufferB, m_verifierType);
		}
		if (ndxA != ndxB)
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "DifferentStates", "Original state did not change");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxA, *bufferA, m_verifierType);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BufferStartCase : public TestCase
{
public:
						BufferStartCase		(Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bufferTarget, glw::GLenum numBindingsTarget, QueryType verifierType);

private:
	IterateResult		iterate				(void);

	const glw::GLenum	m_queryTarget;
	const glw::GLenum	m_bufferTarget;
	const glw::GLenum	m_numBindingsTarget;
	const QueryType		m_verifierType;
};

BufferStartCase::BufferStartCase (Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bufferTarget, glw::GLenum numBindingsTarget, QueryType verifierType)
	: TestCase				(context, name, desc)
	, m_queryTarget			(queryTarget)
	, m_bufferTarget		(bufferTarget)
	, m_numBindingsTarget	(numBindingsTarget)
	, m_verifierType		(verifierType)
{
}

BufferStartCase::IterateResult BufferStartCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxBindings	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(m_numBindingsTarget, &maxBindings);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxBindings; ++ndx)
			verifyStateIndexedInteger(result, gl, m_queryTarget, ndx, 0, m_verifierType);
	}


	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Buffer					bufferA			(m_context.getRenderContext());
		glu::Buffer					bufferB			(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxBindings / 2;
		int							offset			= -1;

		if (m_bufferTarget == GL_ATOMIC_COUNTER_BUFFER)
			offset = 4;
		else if (m_bufferTarget == GL_SHADER_STORAGE_BUFFER)
		{
			gl.glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &offset);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "get align");
		}
		else
			DE_ASSERT(false);

		TCU_CHECK(offset >= 0);

		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Generic", "After setting generic binding point");

			gl.glBindBuffer(m_bufferTarget, *bufferA);
			gl.glBufferData(m_bufferTarget, 16, DE_NULL, GL_DYNAMIC_READ);
			gl.glBindBuffer(m_bufferTarget, *bufferB);
			gl.glBufferData(m_bufferTarget, 32, DE_NULL, GL_DYNAMIC_READ);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen bufs");

			verifyStateIndexedInteger(result, gl, m_queryTarget, 0, 0, m_verifierType);
		}
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Indexed", "After setting with glBindBufferBase");

			gl.glBindBufferBase(m_bufferTarget, ndxA, *bufferA);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind buf");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxA, 0, m_verifierType);
		}
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Indexed", "After setting with glBindBufferRange");

			gl.glBindBufferRange(m_bufferTarget, ndxB, *bufferB, offset, 8);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind buf");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxB, offset, m_verifierType);
		}
		if (ndxA != ndxB)
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "DifferentStates", "Original state did not change");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxA, 0, m_verifierType);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BufferSizeCase : public TestCase
{
public:
						BufferSizeCase	(Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bufferTarget, glw::GLenum numBindingsTarget, QueryType verifierType);

private:
	IterateResult		iterate			(void);

	const glw::GLenum	m_queryTarget;
	const glw::GLenum	m_bufferTarget;
	const glw::GLenum	m_numBindingsTarget;
	const QueryType		m_verifierType;
};

BufferSizeCase::BufferSizeCase (Context& context, const char* name, const char* desc, glw::GLenum queryTarget, glw::GLenum bufferTarget, glw::GLenum numBindingsTarget, QueryType verifierType)
	: TestCase				(context, name, desc)
	, m_queryTarget			(queryTarget)
	, m_bufferTarget		(bufferTarget)
	, m_numBindingsTarget	(numBindingsTarget)
	, m_verifierType		(verifierType)
{
}

BufferSizeCase::IterateResult BufferSizeCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxBindings	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(m_numBindingsTarget, &maxBindings);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxBindings; ++ndx)
			verifyStateIndexedInteger(result, gl, m_queryTarget, ndx, 0, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Buffer					bufferA			(m_context.getRenderContext());
		glu::Buffer					bufferB			(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxBindings / 2;

		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Generic", "After setting generic binding point");

			gl.glBindBuffer(m_bufferTarget, *bufferA);
			gl.glBufferData(m_bufferTarget, 16, DE_NULL, GL_DYNAMIC_READ);
			gl.glBindBuffer(m_bufferTarget, *bufferB);
			gl.glBufferData(m_bufferTarget, 32, DE_NULL, GL_DYNAMIC_READ);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen bufs");

			verifyStateIndexedInteger(result, gl, m_queryTarget, 0, 0, m_verifierType);
		}
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Indexed", "After setting with glBindBufferBase");

			gl.glBindBufferBase(m_bufferTarget, ndxA, *bufferA);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind buf");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxA, 0, m_verifierType);
		}
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Indexed", "After setting with glBindBufferRange");

			gl.glBindBufferRange(m_bufferTarget, ndxB, *bufferB, 0, 8);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind buf");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxB, 8, m_verifierType);
		}
		if (ndxA != ndxB)
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "DifferentStates", "Original state did not change");

			verifyStateIndexedInteger(result, gl, m_queryTarget, ndxA, 0, m_verifierType);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ImageBindingNameCase : public TestCase
{
public:
						ImageBindingNameCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

private:
	IterateResult		iterate					(void);

	const QueryType		m_verifierType;
};

ImageBindingNameCase::ImageBindingNameCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

ImageBindingNameCase::IterateResult ImageBindingNameCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxImages	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImages);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxImages; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_NAME, ndx, 0, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Texture				textureA		(m_context.getRenderContext());
		glu::Texture				textureB		(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxImages / 2;

		gl.glBindTexture(GL_TEXTURE_2D, *textureA);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxA, *textureA, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *textureB);
		gl.glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 32, 32, 4);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxB, *textureB, 0, GL_FALSE, 2, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_NAME, ndxA, *textureA, m_verifierType);
		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_NAME, ndxB, *textureB, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ImageBindingLevelCase : public TestCase
{
public:
						ImageBindingLevelCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

private:
	IterateResult		iterate					(void);

	const QueryType		m_verifierType;
};

ImageBindingLevelCase::ImageBindingLevelCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

ImageBindingLevelCase::IterateResult ImageBindingLevelCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxImages	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImages);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxImages; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_LEVEL, ndx, 0, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Texture				textureA		(m_context.getRenderContext());
		glu::Texture				textureB		(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxImages / 2;

		gl.glBindTexture(GL_TEXTURE_2D, *textureA);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxA, *textureA, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		gl.glBindTexture(GL_TEXTURE_2D, *textureB);
		gl.glTexStorage2D(GL_TEXTURE_2D, 3, GL_RGBA8, 32, 32);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxB, *textureB, 2, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_LEVEL, ndxA, 0, m_verifierType);
		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_LEVEL, ndxB, 2, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ImageBindingLayeredCase : public TestCase
{
public:
						ImageBindingLayeredCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

private:
	IterateResult		iterate					(void);

	const QueryType		m_verifierType;
};

ImageBindingLayeredCase::ImageBindingLayeredCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

ImageBindingLayeredCase::IterateResult ImageBindingLayeredCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxImages	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImages);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxImages; ++ndx)
			verifyStateIndexedBoolean(result, gl, GL_IMAGE_BINDING_LAYERED, ndx, false, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Texture				textureA		(m_context.getRenderContext());
		glu::Texture				textureB		(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxImages / 2;

		gl.glBindTexture(GL_TEXTURE_2D, *textureA);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxA, *textureA, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *textureB);
		gl.glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 32, 32, 4);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxB, *textureB, 0, GL_TRUE, 2, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		verifyStateIndexedBoolean(result, gl, GL_IMAGE_BINDING_LAYERED, ndxA, false, m_verifierType);
		verifyStateIndexedBoolean(result, gl, GL_IMAGE_BINDING_LAYERED, ndxB, true, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ImageBindingLayerCase : public TestCase
{
public:
						ImageBindingLayerCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

private:
	IterateResult		iterate					(void);

	const QueryType		m_verifierType;
};

ImageBindingLayerCase::ImageBindingLayerCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

ImageBindingLayerCase::IterateResult ImageBindingLayerCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxImages	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImages);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxImages; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_LAYER, ndx, 0, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Texture				textureA		(m_context.getRenderContext());
		glu::Texture				textureB		(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxImages / 2;

		gl.glBindTexture(GL_TEXTURE_2D, *textureA);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxA, *textureA, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *textureB);
		gl.glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 32, 32, 4);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxB, *textureB, 0, GL_TRUE, 2, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_LAYER, ndxA, 0, m_verifierType);
		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_LAYER, ndxB, 2, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ImageBindingAccessCase : public TestCase
{
public:
						ImageBindingAccessCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

private:
	IterateResult		iterate					(void);

	const QueryType		m_verifierType;
};

ImageBindingAccessCase::ImageBindingAccessCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

ImageBindingAccessCase::IterateResult ImageBindingAccessCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxImages	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImages);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxImages; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_ACCESS, ndx, GL_READ_ONLY, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Texture				textureA		(m_context.getRenderContext());
		glu::Texture				textureB		(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxImages / 2;

		gl.glBindTexture(GL_TEXTURE_2D, *textureA);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxA, *textureA, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *textureB);
		gl.glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 32, 32, 4);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxB, *textureB, 0, GL_TRUE, 2, GL_READ_WRITE, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_ACCESS, ndxA, GL_READ_ONLY, m_verifierType);
		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_ACCESS, ndxB, GL_READ_WRITE, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ImageBindingFormatCase : public TestCase
{
public:
						ImageBindingFormatCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

private:
	IterateResult		iterate					(void);

	const QueryType		m_verifierType;
};

ImageBindingFormatCase::ImageBindingFormatCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

ImageBindingFormatCase::IterateResult ImageBindingFormatCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxImages	= -1;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImages);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxImages; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_FORMAT, ndx, GL_R32UI, m_verifierType);
	}

	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSetting", "After setting");
		glu::Texture				textureA		(m_context.getRenderContext());
		glu::Texture				textureB		(m_context.getRenderContext());
		const int					ndxA			= 0;
		const int					ndxB			= maxImages / 2;

		gl.glBindTexture(GL_TEXTURE_2D, *textureA);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxA, *textureA, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *textureB);
		gl.glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, 32, 32, 4);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

		gl.glBindImageTexture(ndxB, *textureB, 0, GL_TRUE, 2, GL_READ_WRITE, GL_R32F);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind unit");

		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_FORMAT, ndxA, GL_RGBA8UI, m_verifierType);
		verifyStateIndexedInteger(result, gl, GL_IMAGE_BINDING_FORMAT, ndxB, GL_R32F, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class EnableBlendCase : public TestCase
{
public:
						EnableBlendCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

	void				init			(void);
private:
	IterateResult		iterate			(void);

	const QueryType		m_verifierType;
};

EnableBlendCase::EnableBlendCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

void EnableBlendCase::init (void)
{
	isExtensionSupported(m_context, "GL_EXT_draw_buffers_indexed");
}

EnableBlendCase::IterateResult EnableBlendCase::iterate (void)
{
	glu::CallLogWrapper		gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result			(m_testCtx.getLog(), " // ERROR: ");
	deInt32					maxDrawBuffers = 0;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBoolean(result, gl, GL_BLEND, ndx, false, m_verifierType);
	}
	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSettingCommon", "After setting common");

		gl.glEnable(GL_BLEND);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBoolean(result, gl, GL_BLEND, ndx, true, m_verifierType);

	}
	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterSettingIndexed", "After setting indexed");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
		{
			if (ndx % 2 == 0)
				gl.glEnablei(GL_BLEND, ndx);
			else
				gl.glDisablei(GL_BLEND, ndx);
		}

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBoolean(result, gl, GL_BLEND, ndx, (ndx % 2 == 0), m_verifierType);
	}
	{
		const tcu::ScopedLogSection	superSection	(m_testCtx.getLog(), "AfterResettingIndexedWithCommon", "After resetting indexed with common");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
		{
			if (ndx % 2 == 0)
				gl.glEnablei(GL_BLEND, ndx);
			else
				gl.glDisablei(GL_BLEND, ndx);
		}

		gl.glEnable(GL_BLEND);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBoolean(result, gl, GL_BLEND, ndx, true, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ColorMaskCase : public TestCase
{
public:
						ColorMaskCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

	void				init			(void);
private:
	IterateResult		iterate			(void);

	const QueryType		m_verifierType;
};

ColorMaskCase::ColorMaskCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

void ColorMaskCase::init (void)
{
	isExtensionSupported(m_context, "GL_EXT_draw_buffers_indexed");
}

ColorMaskCase::IterateResult ColorMaskCase::iterate (void)
{
	glu::CallLogWrapper		gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result			(m_testCtx.getLog(), " // ERROR: ");
	deInt32					maxDrawBuffers = 0;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBooleanVec4(result, gl, GL_COLOR_WRITEMASK, ndx, tcu::BVec4(true), m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingCommon", "After setting common");

		gl.glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_FALSE);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBooleanVec4(result, gl, GL_COLOR_WRITEMASK, ndx, tcu::BVec4(false, true, true, false), m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingIndexed", "After setting indexed");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glColorMaski(ndx, (ndx % 2 == 0 ? GL_TRUE : GL_FALSE), (ndx % 2 == 1 ? GL_TRUE : GL_FALSE), (ndx % 2 == 0 ? GL_TRUE : GL_FALSE), (ndx % 2 == 1 ? GL_TRUE : GL_FALSE));

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBooleanVec4(result, gl, GL_COLOR_WRITEMASK, ndx, (ndx % 2 == 0 ? tcu::BVec4(true, false, true, false) : tcu::BVec4(false, true, false, true)), m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterResettingIndexedWithCommon", "After resetting indexed with common");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glColorMaski(ndx, (ndx % 2 == 0 ? GL_TRUE : GL_FALSE), (ndx % 2 == 1 ? GL_TRUE : GL_FALSE), (ndx % 2 == 0 ? GL_TRUE : GL_FALSE), (ndx % 2 == 1 ? GL_TRUE : GL_FALSE));

		gl.glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_FALSE);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedBooleanVec4(result, gl, GL_COLOR_WRITEMASK, ndx, tcu::BVec4(false, true, true, false), m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BlendFuncCase : public TestCase
{
public:
						BlendFuncCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

	void				init			(void);
private:
	IterateResult		iterate			(void);

	const QueryType		m_verifierType;
};

BlendFuncCase::BlendFuncCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

void BlendFuncCase::init (void)
{
	isExtensionSupported(m_context, "GL_EXT_draw_buffers_indexed");
}

BlendFuncCase::IterateResult BlendFuncCase::iterate (void)
{
	const deUint32 blendFuncs[] =
	{
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_CONSTANT_COLOR,
		GL_ONE_MINUS_CONSTANT_COLOR,
		GL_CONSTANT_ALPHA,
		GL_ONE_MINUS_CONSTANT_ALPHA,
		GL_SRC_ALPHA_SATURATE
	};

	glu::CallLogWrapper		gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result			(m_testCtx.getLog(), " // ERROR: ");
	deInt32					maxDrawBuffers = 0;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_RGB, ndx, GL_ONE, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_RGB, ndx, GL_ZERO, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_ALPHA, ndx, GL_ONE, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_ALPHA, ndx, GL_ZERO, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingCommon", "After setting common");

		gl.glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_RGB, ndx, GL_SRC_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_RGB, ndx, GL_DST_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_ALPHA, ndx, GL_SRC_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_ALPHA, ndx, GL_DST_ALPHA, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingCommonSeparate", "After setting common separate");

		gl.glBlendFuncSeparate(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR, GL_ONE_MINUS_DST_ALPHA);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_RGB, ndx, GL_SRC_COLOR, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_RGB, ndx, GL_ONE_MINUS_SRC_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_ALPHA, ndx, GL_DST_COLOR, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_ALPHA, ndx, GL_ONE_MINUS_DST_ALPHA, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingIndexed", "After setting indexed");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendFunci(ndx, blendFuncs[ndx % DE_LENGTH_OF_ARRAY(blendFuncs)], blendFuncs[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendFuncs)]);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_RGB, ndx, blendFuncs[ndx % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_RGB, ndx, blendFuncs[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_ALPHA, ndx, blendFuncs[ndx % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_ALPHA, ndx, blendFuncs[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingIndexedSeparate", "After setting indexed separate");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendFuncSeparatei(ndx, blendFuncs[(ndx + 3) % DE_LENGTH_OF_ARRAY(blendFuncs)],
										 blendFuncs[(ndx + 2) % DE_LENGTH_OF_ARRAY(blendFuncs)],
										 blendFuncs[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendFuncs)],
										 blendFuncs[(ndx + 0) % DE_LENGTH_OF_ARRAY(blendFuncs)]);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_RGB, ndx, blendFuncs[(ndx + 3) % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_RGB, ndx, blendFuncs[(ndx + 2) % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_ALPHA, ndx, blendFuncs[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_ALPHA, ndx, blendFuncs[(ndx + 0) % DE_LENGTH_OF_ARRAY(blendFuncs)], m_verifierType);

	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterResettingIndexedWithCommon", "After resetting indexed with common");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendFunci(ndx, blendFuncs[ndx % DE_LENGTH_OF_ARRAY(blendFuncs)], blendFuncs[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendFuncs)]);

		gl.glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_RGB, ndx, GL_SRC_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_RGB, ndx, GL_DST_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_ALPHA, ndx, GL_SRC_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_ALPHA, ndx, GL_DST_ALPHA, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterResettingIndexedWithCommonSeparate", "After resetting indexed with common separate");

		gl.glBlendFuncSeparate(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR, GL_ONE_MINUS_DST_ALPHA);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendFuncSeparatei(ndx, blendFuncs[(ndx + 3) % DE_LENGTH_OF_ARRAY(blendFuncs)],
										 blendFuncs[(ndx + 2) % DE_LENGTH_OF_ARRAY(blendFuncs)],
										 blendFuncs[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendFuncs)],
										 blendFuncs[(ndx + 0) % DE_LENGTH_OF_ARRAY(blendFuncs)]);

		gl.glBlendFuncSeparate(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR, GL_ONE_MINUS_DST_ALPHA);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_RGB, ndx, GL_SRC_COLOR, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_RGB, ndx, GL_ONE_MINUS_SRC_ALPHA, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_SRC_ALPHA, ndx, GL_DST_COLOR, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_DST_ALPHA, ndx, GL_ONE_MINUS_DST_ALPHA, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BlendEquationCase : public TestCase
{
public:
						BlendEquationCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

	void				init				(void);
private:
	IterateResult		iterate				(void);

	const QueryType		m_verifierType;
};

BlendEquationCase::BlendEquationCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

void BlendEquationCase::init (void)
{
	isExtensionSupported(m_context, "GL_EXT_draw_buffers_indexed");
}

BlendEquationCase::IterateResult BlendEquationCase::iterate (void)
{
	const deUint32 blendEquations[] =
	{
		GL_FUNC_ADD,
		GL_FUNC_SUBTRACT,
		GL_FUNC_REVERSE_SUBTRACT,
		GL_MIN,
		GL_MAX
	};

	glu::CallLogWrapper		gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result			(m_testCtx.getLog(), " // ERROR: ");
	deInt32					maxDrawBuffers = 0;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial value");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_FUNC_ADD, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_FUNC_ADD, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingCommon", "After setting common");

		gl.glBlendEquation(GL_FUNC_SUBTRACT);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_FUNC_SUBTRACT, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_FUNC_SUBTRACT, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingCommonSeparate", "After setting common separate");

		gl.glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_SUBTRACT);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_FUNC_REVERSE_SUBTRACT, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_FUNC_SUBTRACT, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingIndexed", "After setting indexed");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendEquationi(ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)]);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)], m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingIndexedSeparate", "After setting indexed separate");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendEquationSeparatei(ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)], blendEquations[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendEquations)]);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, blendEquations[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendEquations)], m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterResettingIndexedWithCommon", "After resetting indexed with common");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendEquationi(ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)]);

		gl.glBlendEquation(GL_FUNC_SUBTRACT);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_FUNC_SUBTRACT, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_FUNC_SUBTRACT, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterResettingIndexedWithCommonSeparate", "After resetting indexed with common separate");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendEquationSeparatei(ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)], blendEquations[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendEquations)]);

		gl.glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_SUBTRACT);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_FUNC_REVERSE_SUBTRACT, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_FUNC_SUBTRACT, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BlendEquationAdvancedCase : public TestCase
{
public:
						BlendEquationAdvancedCase	(Context& context, const char* name, const char* desc, QueryType verifierType);

	void				init				(void);
private:
	IterateResult		iterate				(void);

	const QueryType		m_verifierType;
};

BlendEquationAdvancedCase::BlendEquationAdvancedCase (Context& context, const char* name, const char* desc, QueryType verifierType)
	: TestCase			(context, name, desc)
	, m_verifierType	(verifierType)
{
}

void BlendEquationAdvancedCase::init (void)
{
	isExtensionSupported(m_context, "GL_EXT_draw_buffers_indexed");
	isExtensionSupported(m_context, "GL_KHR_blend_equation_advanced");
}

BlendEquationAdvancedCase::IterateResult BlendEquationAdvancedCase::iterate (void)
{
	const deUint32 blendEquations[] =
	{
		GL_FUNC_ADD,
		GL_FUNC_SUBTRACT,
		GL_FUNC_REVERSE_SUBTRACT,
		GL_MIN,
		GL_MAX
	};

	const deUint32 blendEquationAdvanced[] =
	{
		GL_MULTIPLY,
		GL_SCREEN,
		GL_OVERLAY,
		GL_DARKEN,
		GL_LIGHTEN,
		GL_COLORDODGE,
		GL_COLORBURN,
		GL_HARDLIGHT,
		GL_SOFTLIGHT,
		GL_DIFFERENCE,
		GL_EXCLUSION,
		GL_HSL_HUE,
		GL_HSL_SATURATION,
		GL_HSL_COLOR,
		GL_HSL_LUMINOSITY
	};

	glu::CallLogWrapper		gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result			(m_testCtx.getLog(), " // ERROR: ");
	deInt32					maxDrawBuffers = 0;

	gl.enableLogging(true);

	gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetIntegerv");

	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingCommon", "After setting common");

		gl.glBlendEquation(GL_SCREEN);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_SCREEN, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_SCREEN, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterSettingIndexed", "After setting indexed");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendEquationi(ndx, blendEquationAdvanced[ndx % DE_LENGTH_OF_ARRAY(blendEquationAdvanced)]);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, blendEquationAdvanced[ndx % DE_LENGTH_OF_ARRAY(blendEquationAdvanced)], m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, blendEquationAdvanced[ndx % DE_LENGTH_OF_ARRAY(blendEquationAdvanced)], m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterResettingIndexedWithCommon", "After resetting indexed with common");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendEquationi(ndx, blendEquationAdvanced[ndx % DE_LENGTH_OF_ARRAY(blendEquationAdvanced)]);

		gl.glBlendEquation(GL_MULTIPLY);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_MULTIPLY, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_MULTIPLY, m_verifierType);
	}
	{
		const tcu::ScopedLogSection section (m_testCtx.getLog(), "AfterResettingIndexedSeparateWithCommon", "After resetting indexed separate with common");

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			gl.glBlendEquationSeparatei(ndx, blendEquations[ndx % DE_LENGTH_OF_ARRAY(blendEquations)], blendEquations[(ndx + 1) % DE_LENGTH_OF_ARRAY(blendEquations)]);

		gl.glBlendEquation(GL_LIGHTEN);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_RGB, ndx, GL_LIGHTEN, m_verifierType);

		for (int ndx = 0; ndx < maxDrawBuffers; ++ndx)
			verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION_ALPHA, ndx, GL_LIGHTEN, m_verifierType);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

IndexedStateQueryTests::IndexedStateQueryTests (Context& context)
	: TestCaseGroup(context, "indexed", "Indexed state queries")
{
}

IndexedStateQueryTests::~IndexedStateQueryTests (void)
{
}

void IndexedStateQueryTests::init (void)
{
	static const QueryType verifiers[] = { QUERY_INDEXED_BOOLEAN, QUERY_INDEXED_INTEGER, QUERY_INDEXED_INTEGER64 };
	static const QueryType vec4Verifiers[] = { QUERY_INDEXED_BOOLEAN_VEC4, QUERY_INDEXED_INTEGER_VEC4, QUERY_INDEXED_INTEGER64_VEC4 };

#define FOR_EACH_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)	\
	{																						\
		const QueryType verifier = verifiers[verifierNdx];									\
		const char* verifierSuffix = getVerifierSuffix(verifier);							\
		this->addChild(X);																	\
	}

#define FOR_EACH_VEC4_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(vec4Verifiers); ++verifierNdx)	\
	{																							\
		const QueryType verifier = vec4Verifiers[verifierNdx];									\
		const char* verifierSuffix = getVerifierSuffix(verifier);								\
		this->addChild(X);																		\
	}

	FOR_EACH_VERIFIER(new SampleMaskCase			(m_context, (std::string() + "sample_mask_value_" + verifierSuffix).c_str(),				"Test SAMPLE_MASK_VALUE", verifier))

	FOR_EACH_VERIFIER(new MinValueIndexed3Case		(m_context, (std::string() + "max_compute_work_group_count_" + verifierSuffix).c_str(),		"Test MAX_COMPUTE_WORK_GROUP_COUNT",	GL_MAX_COMPUTE_WORK_GROUP_COUNT,	tcu::IVec3(65535,65535,65535),	verifier))
	FOR_EACH_VERIFIER(new MinValueIndexed3Case		(m_context, (std::string() + "max_compute_work_group_size_" + verifierSuffix).c_str(),		"Test MAX_COMPUTE_WORK_GROUP_SIZE",		GL_MAX_COMPUTE_WORK_GROUP_SIZE,		tcu::IVec3(128, 128, 64),		verifier))

	FOR_EACH_VERIFIER(new BufferBindingCase			(m_context, (std::string() + "atomic_counter_buffer_binding_" + verifierSuffix).c_str(),	"Test ATOMIC_COUNTER_BUFFER_BINDING",	GL_ATOMIC_COUNTER_BUFFER_BINDING,	GL_ATOMIC_COUNTER_BUFFER,	GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,	verifier))
	FOR_EACH_VERIFIER(new BufferStartCase			(m_context, (std::string() + "atomic_counter_buffer_start_" + verifierSuffix).c_str(),		"Test ATOMIC_COUNTER_BUFFER_START",		GL_ATOMIC_COUNTER_BUFFER_START,		GL_ATOMIC_COUNTER_BUFFER,	GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,	verifier))
	FOR_EACH_VERIFIER(new BufferSizeCase			(m_context, (std::string() + "atomic_counter_buffer_size_" + verifierSuffix).c_str(),		"Test ATOMIC_COUNTER_BUFFER_SIZE",		GL_ATOMIC_COUNTER_BUFFER_SIZE,		GL_ATOMIC_COUNTER_BUFFER,	GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,	verifier))

	FOR_EACH_VERIFIER(new BufferBindingCase			(m_context, (std::string() + "shader_storage_buffer_binding_" + verifierSuffix).c_str(),	"Test SHADER_STORAGE_BUFFER_BINDING",	GL_SHADER_STORAGE_BUFFER_BINDING,	GL_SHADER_STORAGE_BUFFER,	GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,	verifier))
	FOR_EACH_VERIFIER(new BufferStartCase			(m_context, (std::string() + "shader_storage_buffer_start_" + verifierSuffix).c_str(),		"Test SHADER_STORAGE_BUFFER_START",		GL_SHADER_STORAGE_BUFFER_START,		GL_SHADER_STORAGE_BUFFER,	GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,	verifier))
	FOR_EACH_VERIFIER(new BufferSizeCase			(m_context, (std::string() + "shader_storage_buffer_size_" + verifierSuffix).c_str(),		"Test SHADER_STORAGE_BUFFER_SIZE",		GL_SHADER_STORAGE_BUFFER_SIZE,		GL_SHADER_STORAGE_BUFFER,	GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,	verifier))

	FOR_EACH_VERIFIER(new ImageBindingNameCase		(m_context, (std::string() + "image_binding_name_" + verifierSuffix).c_str(),				"Test IMAGE_BINDING_NAME",				verifier))
	FOR_EACH_VERIFIER(new ImageBindingLevelCase		(m_context, (std::string() + "image_binding_level_" + verifierSuffix).c_str(),				"Test IMAGE_BINDING_LEVEL",				verifier))
	FOR_EACH_VERIFIER(new ImageBindingLayeredCase	(m_context, (std::string() + "image_binding_layered_" + verifierSuffix).c_str(),			"Test IMAGE_BINDING_LAYERED",			verifier))
	FOR_EACH_VERIFIER(new ImageBindingLayerCase		(m_context, (std::string() + "image_binding_layer_" + verifierSuffix).c_str(),				"Test IMAGE_BINDING_LAYER",				verifier))
	FOR_EACH_VERIFIER(new ImageBindingAccessCase	(m_context, (std::string() + "image_binding_access_" + verifierSuffix).c_str(),				"Test IMAGE_BINDING_ACCESS",			verifier))
	FOR_EACH_VERIFIER(new ImageBindingFormatCase	(m_context, (std::string() + "image_binding_format_" + verifierSuffix).c_str(),				"Test IMAGE_BINDING_FORMAT",			verifier))

	{
		const QueryType verifier = QUERY_INDEXED_ISENABLED;
		const char* verifierSuffix = getVerifierSuffix(verifier);
		this->addChild(new EnableBlendCase			(m_context, (std::string() + "blend_" + verifierSuffix).c_str(),							"BLEND",								verifier));
	}
	FOR_EACH_VEC4_VERIFIER(new ColorMaskCase		(m_context, (std::string() + "color_mask_" + verifierSuffix).c_str(),						"COLOR_WRITEMASK",						verifier))
	FOR_EACH_VERIFIER(new BlendFuncCase				(m_context, (std::string() + "blend_func_" + verifierSuffix).c_str(),						"BLEND_SRC and BLEND_DST",				verifier))
	FOR_EACH_VERIFIER(new BlendEquationCase			(m_context, (std::string() + "blend_equation_" + verifierSuffix).c_str(),					"BLEND_EQUATION_RGB and BLEND_DST",		verifier))
	FOR_EACH_VERIFIER(new BlendEquationAdvancedCase	(m_context, (std::string() + "blend_equation_advanced_" + verifierSuffix).c_str(),			"BLEND_EQUATION_RGB and BLEND_DST",		verifier))

#undef FOR_EACH_VEC4_VERIFIER
#undef FOR_EACH_VERIFIER
}

} // Functional
} // gles31
} // deqp
