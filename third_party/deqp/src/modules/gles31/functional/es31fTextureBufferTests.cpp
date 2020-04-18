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
 * \brief Texture buffer tests
 *//*--------------------------------------------------------------------*/

#include "es31fTextureBufferTests.hpp"

#include "glsTextureBufferCase.hpp"
#include "glsStateQueryUtil.hpp"

#include "tcuTestLog.hpp"

#include "gluRenderContext.hpp"
#include "gluContextInfo.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluStrUtil.hpp"

#include "glwEnums.hpp"

#include "deStringUtil.hpp"

#include <string>

using std::string;
using namespace deqp::gls::TextureBufferCaseUtil;
using deqp::gls::TextureBufferCase;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

string toTestName (RenderBits renderBits)
{
	struct
	{
		RenderBits	bit;
		const char*	str;
	} bitInfos[] =
	{
		{ RENDERBITS_AS_VERTEX_ARRAY,		"as_vertex_array"		},
		{ RENDERBITS_AS_INDEX_ARRAY,		"as_index_array"		},
		{ RENDERBITS_AS_VERTEX_TEXTURE,		"as_vertex_texture"		},
		{ RENDERBITS_AS_FRAGMENT_TEXTURE,	"as_fragment_texture"	}
	};

	std::ostringstream	stream;
	bool				first	= true;

	DE_ASSERT(renderBits != 0);

	for (int infoNdx = 0; infoNdx < DE_LENGTH_OF_ARRAY(bitInfos); infoNdx++)
	{
		if (renderBits & bitInfos[infoNdx].bit)
		{
			stream << (first ? "" : "_") << bitInfos[infoNdx].str;
			first = false;
		}
	}

	return stream.str();
}

string toTestName (ModifyBits modifyBits)
{
	struct
	{
		ModifyBits	bit;
		const char*	str;
	} bitInfos[] =
	{
		{ MODIFYBITS_BUFFERDATA,			"bufferdata"			},
		{ MODIFYBITS_BUFFERSUBDATA,			"buffersubdata"			},
		{ MODIFYBITS_MAPBUFFER_WRITE,		"mapbuffer_write"		},
		{ MODIFYBITS_MAPBUFFER_READWRITE,	"mapbuffer_readwrite"	}
	};

	std::ostringstream	stream;
	bool				first	= true;

	DE_ASSERT(modifyBits != 0);

	for (int infoNdx = 0; infoNdx < DE_LENGTH_OF_ARRAY(bitInfos); infoNdx++)
	{
		if (modifyBits & bitInfos[infoNdx].bit)
		{
			stream << (first ? "" : "_") << bitInfos[infoNdx].str;
			first = false;
		}
	}

	return stream.str();
}

RenderBits operator| (RenderBits a, RenderBits b)
{
	return (RenderBits)(deUint32(a) | deUint32(b));
}

} // anonymous

// Queries

namespace
{

using namespace gls::StateQueryUtil;

class LimitQueryCase : public TestCase
{
public:
						LimitQueryCase	(Context& ctx, const char* name, const char* desc, glw::GLenum target, int minLimit, QueryType type);

private:
	IterateResult		iterate			(void);

	const glw::GLenum	m_target;
	const int			m_minValue;
	const QueryType		m_type;
};

LimitQueryCase::LimitQueryCase (Context& context, const char* name, const char* desc, glw::GLenum target, int minLimit, QueryType type)
	: TestCase		(context, name, desc)
	, m_target		(target)
	, m_minValue	(minLimit)
	, m_type		(type)
{
}

LimitQueryCase::IterateResult LimitQueryCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"), "GL_EXT_texture_buffer is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	verifyStateIntegerMin(result, gl, m_target, m_minValue, m_type);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class AlignmentQueryCase : public TestCase
{
public:
						AlignmentQueryCase	(Context& ctx, const char* name, const char* desc, glw::GLenum target, int maxAlign, QueryType type);

private:
	IterateResult		iterate				(void);

	const glw::GLenum	m_target;
	const int			m_maxValue;
	const QueryType		m_type;
};

AlignmentQueryCase::AlignmentQueryCase (Context& context, const char* name, const char* desc, glw::GLenum target, int maxAlign, QueryType type)
	: TestCase		(context, name, desc)
	, m_target		(target)
	, m_maxValue	(maxAlign)
	, m_type		(type)
{
}

AlignmentQueryCase::IterateResult AlignmentQueryCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"), "GL_EXT_texture_buffer is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	verifyStateIntegerMax(result, gl, m_target, m_maxValue, m_type);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TextureBufferBindingQueryCase : public TestCase
{
public:
					TextureBufferBindingQueryCase	(Context& ctx, const char* name, const char* desc, QueryType type);

private:
	IterateResult	iterate							(void);

	const QueryType	m_type;
};

TextureBufferBindingQueryCase::TextureBufferBindingQueryCase (Context& context, const char* name, const char* desc, QueryType type)
	: TestCase		(context, name, desc)
	, m_type		(type)
{
}

TextureBufferBindingQueryCase::IterateResult TextureBufferBindingQueryCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"), "GL_EXT_texture_buffer is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial value");

		verifyStateInteger(result, gl, GL_TEXTURE_BUFFER_BINDING, 0, m_type);
	}

	// bind
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "bind", "After bind");

		glw::GLuint buffer;

		gl.glGenBuffers(1, &buffer);
		gl.glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind buffer");

		verifyStateInteger(result, gl, GL_TEXTURE_BUFFER_BINDING, buffer, m_type);

		gl.glDeleteBuffers(1, &buffer);
	}

	// after delete
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "bind", "After delete");

		verifyStateInteger(result, gl, GL_TEXTURE_BUFFER_BINDING, 0, m_type);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TextureBindingBufferQueryCase : public TestCase
{
public:
					TextureBindingBufferQueryCase	(Context& ctx, const char* name, const char* desc, QueryType type);

private:
	IterateResult	iterate							(void);

	const QueryType	m_type;
};

TextureBindingBufferQueryCase::TextureBindingBufferQueryCase (Context& context, const char* name, const char* desc, QueryType type)
	: TestCase		(context, name, desc)
	, m_type		(type)
{
}

TextureBindingBufferQueryCase::IterateResult TextureBindingBufferQueryCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"), "GL_EXT_texture_buffer is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial value");

		verifyStateInteger(result, gl, GL_TEXTURE_BINDING_BUFFER, 0, m_type);
	}

	// bind
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "bind", "After bind");

		glw::GLuint texture;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(GL_TEXTURE_BUFFER, texture);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind texture");

		verifyStateInteger(result, gl, GL_TEXTURE_BINDING_BUFFER, texture, m_type);

		gl.glDeleteTextures(1, &texture);
	}

	// after delete
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "bind", "After delete");

		verifyStateInteger(result, gl, GL_TEXTURE_BINDING_BUFFER, 0, m_type);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TextureBufferDataStoreQueryCase : public TestCase
{
public:
					TextureBufferDataStoreQueryCase	(Context& ctx, const char* name, const char* desc, QueryType type);

private:
	IterateResult	iterate							(void);

	const QueryType	m_type;
};

TextureBufferDataStoreQueryCase::TextureBufferDataStoreQueryCase (Context& context, const char* name, const char* desc, QueryType type)
	: TestCase		(context, name, desc)
	, m_type		(type)
{
}

TextureBufferDataStoreQueryCase::IterateResult TextureBufferDataStoreQueryCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"), "GL_EXT_texture_buffer is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// non-buffer
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "NonBuffer", "Non-buffer");

		glw::GLuint	texture;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(GL_TEXTURE_2D, texture);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "gen texture");

		verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_2D, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, 0, m_type);

		gl.glDeleteTextures(1, &texture);
	}

	// buffer
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Buffer", "Texture buffer");

		glw::GLuint	texture;
		glw::GLuint	buffer;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(GL_TEXTURE_BUFFER, texture);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind texture");

		gl.glGenBuffers(1, &buffer);
		gl.glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		gl.glBufferData(GL_TEXTURE_BUFFER, 32, DE_NULL, GL_STATIC_DRAW);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind buf");

		gl.glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, buffer);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "tex buffer");

		verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, buffer, m_type);

		gl.glDeleteTextures(1, &texture);
		gl.glDeleteBuffers(1, &buffer);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TextureBufferOffsetQueryCase : public TestCase
{
public:
					TextureBufferOffsetQueryCase	(Context& ctx, const char* name, const char* desc, QueryType type);

private:
	IterateResult	iterate							(void);

	const QueryType	m_type;
};

TextureBufferOffsetQueryCase::TextureBufferOffsetQueryCase (Context& context, const char* name, const char* desc, QueryType type)
	: TestCase		(context, name, desc)
	, m_type		(type)
{
}

TextureBufferOffsetQueryCase::IterateResult TextureBufferOffsetQueryCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"), "GL_EXT_texture_buffer is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// non-buffer
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "NonBuffer", "Non-buffer");

		glw::GLuint	texture;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(GL_TEXTURE_2D, texture);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "gen texture");

		verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_2D, 0, GL_TEXTURE_BUFFER_OFFSET, 0, m_type);

		gl.glDeleteTextures(1, &texture);
	}

	// buffer
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Buffer", "Texture buffer");

		glw::GLuint	texture;
		glw::GLuint	buffer;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(GL_TEXTURE_BUFFER, texture);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind texture");

		gl.glGenBuffers(1, &buffer);
		gl.glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		gl.glBufferData(GL_TEXTURE_BUFFER, 1024, DE_NULL, GL_STATIC_DRAW);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind buf");

		{
			const tcu::ScopedLogSection subsection(m_testCtx.getLog(), "Offset0", "Offset 0");
			gl.glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "tex buffer");

			verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_OFFSET, 0, m_type);
		}
		{
			const tcu::ScopedLogSection subsection(m_testCtx.getLog(), "Offset256", "Offset 256");
			gl.glTexBufferRange(GL_TEXTURE_BUFFER, GL_R32UI, buffer, 256, 512);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "tex buffer");

			verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_OFFSET, 256, m_type);
		}

		gl.glDeleteTextures(1, &texture);
		gl.glDeleteBuffers(1, &buffer);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TextureBufferSizeQueryCase : public TestCase
{
public:
					TextureBufferSizeQueryCase	(Context& ctx, const char* name, const char* desc, QueryType type);

private:
	IterateResult	iterate						(void);

	const QueryType	m_type;
};

TextureBufferSizeQueryCase::TextureBufferSizeQueryCase (Context& context, const char* name, const char* desc, QueryType type)
	: TestCase		(context, name, desc)
	, m_type		(type)
{
}

TextureBufferSizeQueryCase::IterateResult TextureBufferSizeQueryCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"), "GL_EXT_texture_buffer is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// non-buffer
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "NonBuffer", "Non-buffer");

		glw::GLuint	texture;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(GL_TEXTURE_2D, texture);
		gl.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "gen texture");

		verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_2D, 0, GL_TEXTURE_BUFFER_SIZE, 0, m_type);

		gl.glDeleteTextures(1, &texture);
	}

	// buffer
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Buffer", "Texture buffer");

		glw::GLuint	texture;
		glw::GLuint	buffer;

		gl.glGenTextures(1, &texture);
		gl.glBindTexture(GL_TEXTURE_BUFFER, texture);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind texture");

		gl.glGenBuffers(1, &buffer);
		gl.glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		gl.glBufferData(GL_TEXTURE_BUFFER, 1024, DE_NULL, GL_STATIC_DRAW);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "bind buf");

		{
			const tcu::ScopedLogSection subsection(m_testCtx.getLog(), "SizeAll", "Bind whole buffer");
			gl.glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "tex buffer");

			verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_SIZE, 1024, m_type);
		}
		{
			const tcu::ScopedLogSection subsection(m_testCtx.getLog(), "Partial", "Partial buffer");
			gl.glTexBufferRange(GL_TEXTURE_BUFFER, GL_R32UI, buffer, 256, 512);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "tex buffer");

			verifyStateTextureLevelInteger(result, gl, GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_SIZE, 512, m_type);
		}

		gl.glDeleteTextures(1, &texture);
		gl.glDeleteBuffers(1, &buffer);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

TestCaseGroup* createTextureBufferTests (Context& context)
{
	TestCaseGroup* const root = new TestCaseGroup(context, "texture_buffer", "Texture buffer syncronization tests");

	const size_t bufferSizes[] =
	{
		512,
		513,
		65536,
		65537,
		131071
	};

	const size_t rangeSizes[] =
	{
		512,
		513,
		65537,
		98304,
	};

	const size_t offsets[] =
	{
		1,
		7
	};

	const RenderBits renderTypeCombinations[] =
	{
		RENDERBITS_AS_VERTEX_ARRAY,
									  RENDERBITS_AS_INDEX_ARRAY,
		RENDERBITS_AS_VERTEX_ARRAY	| RENDERBITS_AS_INDEX_ARRAY,

																  RENDERBITS_AS_VERTEX_TEXTURE,
		RENDERBITS_AS_VERTEX_ARRAY	|							  RENDERBITS_AS_VERTEX_TEXTURE,
									  RENDERBITS_AS_INDEX_ARRAY	| RENDERBITS_AS_VERTEX_TEXTURE,
		RENDERBITS_AS_VERTEX_ARRAY	| RENDERBITS_AS_INDEX_ARRAY	| RENDERBITS_AS_VERTEX_TEXTURE,

																								  RENDERBITS_AS_FRAGMENT_TEXTURE,
		RENDERBITS_AS_VERTEX_ARRAY	|															  RENDERBITS_AS_FRAGMENT_TEXTURE,
									  RENDERBITS_AS_INDEX_ARRAY |								  RENDERBITS_AS_FRAGMENT_TEXTURE,
		RENDERBITS_AS_VERTEX_ARRAY	| RENDERBITS_AS_INDEX_ARRAY |								  RENDERBITS_AS_FRAGMENT_TEXTURE,
																  RENDERBITS_AS_VERTEX_TEXTURE	| RENDERBITS_AS_FRAGMENT_TEXTURE,
		RENDERBITS_AS_VERTEX_ARRAY	|							  RENDERBITS_AS_VERTEX_TEXTURE	| RENDERBITS_AS_FRAGMENT_TEXTURE,
									  RENDERBITS_AS_INDEX_ARRAY	| RENDERBITS_AS_VERTEX_TEXTURE	| RENDERBITS_AS_FRAGMENT_TEXTURE,
		RENDERBITS_AS_VERTEX_ARRAY	| RENDERBITS_AS_INDEX_ARRAY	| RENDERBITS_AS_VERTEX_TEXTURE	| RENDERBITS_AS_FRAGMENT_TEXTURE
	};

	const ModifyBits modifyTypes[] =
	{
		MODIFYBITS_BUFFERDATA,
		MODIFYBITS_BUFFERSUBDATA,
		MODIFYBITS_MAPBUFFER_WRITE,
		MODIFYBITS_MAPBUFFER_READWRITE
	};

	// State and limit queries
	{
		TestCaseGroup* const queryGroup = new TestCaseGroup(context, "state_query", "Query states and limits");
		root->addChild(queryGroup);

		queryGroup->addChild(new LimitQueryCase		(context, "max_texture_buffer_size_getboolean",				"Test MAX_TEXTURE_BUFFER_SIZE",			GL_MAX_TEXTURE_BUFFER_SIZE,			65536,	QUERY_BOOLEAN));
		queryGroup->addChild(new LimitQueryCase		(context, "max_texture_buffer_size_getinteger",				"Test MAX_TEXTURE_BUFFER_SIZE",			GL_MAX_TEXTURE_BUFFER_SIZE,			65536,	QUERY_INTEGER));
		queryGroup->addChild(new LimitQueryCase		(context, "max_texture_buffer_size_getinteger64",			"Test MAX_TEXTURE_BUFFER_SIZE",			GL_MAX_TEXTURE_BUFFER_SIZE,			65536,	QUERY_INTEGER64));
		queryGroup->addChild(new LimitQueryCase		(context, "max_texture_buffer_size_getfloat",				"Test MAX_TEXTURE_BUFFER_SIZE",			GL_MAX_TEXTURE_BUFFER_SIZE,			65536,	QUERY_FLOAT));
		queryGroup->addChild(new AlignmentQueryCase	(context, "texture_buffer_offset_alignment_getboolean",		"Test TEXTURE_BUFFER_OFFSET_ALIGNMENT",	GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT,	256,	QUERY_BOOLEAN));
		queryGroup->addChild(new AlignmentQueryCase	(context, "texture_buffer_offset_alignment_getinteger",		"Test TEXTURE_BUFFER_OFFSET_ALIGNMENT",	GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT,	256,	QUERY_INTEGER));
		queryGroup->addChild(new AlignmentQueryCase	(context, "texture_buffer_offset_alignment_getinteger64",	"Test TEXTURE_BUFFER_OFFSET_ALIGNMENT",	GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT,	256,	QUERY_INTEGER64));
		queryGroup->addChild(new AlignmentQueryCase	(context, "texture_buffer_offset_alignment_getfloat",		"Test TEXTURE_BUFFER_OFFSET_ALIGNMENT",	GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT,	256,	QUERY_FLOAT));

		queryGroup->addChild(new TextureBufferBindingQueryCase(context, "texture_buffer_binding_getboolean",	"TEXTURE_BUFFER_BINDING", QUERY_BOOLEAN));
		queryGroup->addChild(new TextureBufferBindingQueryCase(context, "texture_buffer_binding_getinteger",	"TEXTURE_BUFFER_BINDING", QUERY_INTEGER));
		queryGroup->addChild(new TextureBufferBindingQueryCase(context, "texture_buffer_binding_getinteger64",	"TEXTURE_BUFFER_BINDING", QUERY_INTEGER64));
		queryGroup->addChild(new TextureBufferBindingQueryCase(context, "texture_buffer_binding_getfloat",		"TEXTURE_BUFFER_BINDING", QUERY_FLOAT));

		queryGroup->addChild(new TextureBindingBufferQueryCase(context, "texture_binding_buffer_getboolean",	"TEXTURE_BINDING_BUFFER", QUERY_BOOLEAN));
		queryGroup->addChild(new TextureBindingBufferQueryCase(context, "texture_binding_buffer_getinteger",	"TEXTURE_BINDING_BUFFER", QUERY_INTEGER));
		queryGroup->addChild(new TextureBindingBufferQueryCase(context, "texture_binding_buffer_getinteger64",	"TEXTURE_BINDING_BUFFER", QUERY_INTEGER64));
		queryGroup->addChild(new TextureBindingBufferQueryCase(context, "texture_binding_buffer_getfloat",		"TEXTURE_BINDING_BUFFER", QUERY_FLOAT));

		queryGroup->addChild(new TextureBufferDataStoreQueryCase(context, "texture_buffer_data_store_binding_integer",	"TEXTURE_BUFFER_DATA_STORE_BINDING", QUERY_TEXTURE_LEVEL_INTEGER));
		queryGroup->addChild(new TextureBufferDataStoreQueryCase(context, "texture_buffer_data_store_binding_float",	"TEXTURE_BUFFER_DATA_STORE_BINDING", QUERY_TEXTURE_LEVEL_FLOAT));

		queryGroup->addChild(new TextureBufferOffsetQueryCase(context, "texture_buffer_offset_integer",	"TEXTURE_BUFFER_OFFSET", QUERY_TEXTURE_LEVEL_INTEGER));
		queryGroup->addChild(new TextureBufferOffsetQueryCase(context, "texture_buffer_offset_float",	"TEXTURE_BUFFER_OFFSET", QUERY_TEXTURE_LEVEL_FLOAT));

		queryGroup->addChild(new TextureBufferSizeQueryCase(context, "texture_buffer_size_integer",	"TEXTURE_BUFFER_SIZE", QUERY_TEXTURE_LEVEL_INTEGER));
		queryGroup->addChild(new TextureBufferSizeQueryCase(context, "texture_buffer_size_float",	"TEXTURE_BUFFER_SIZE", QUERY_TEXTURE_LEVEL_FLOAT));
	}

	// Rendering test
	{
		TestCaseGroup* const renderGroup = new TestCaseGroup(context, "render", "Setup texture buffer with glBufferData and render data in different ways");
		root->addChild(renderGroup);

		for (int renderTypeNdx = 0; renderTypeNdx < DE_LENGTH_OF_ARRAY(renderTypeCombinations); renderTypeNdx++)
		{
			const RenderBits		renderType		= renderTypeCombinations[renderTypeNdx];
			TestCaseGroup* const	renderTypeGroup	= new TestCaseGroup(context, toTestName(renderType).c_str(), toTestName(renderType).c_str());

			renderGroup->addChild(renderTypeGroup);

			for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(bufferSizes); sizeNdx++)
			{
				const size_t size	= bufferSizes[sizeNdx];
				const string name	("buffer_size_" + de::toString(size));

				renderTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, size, 0, 0, RENDERBITS_NONE, MODIFYBITS_NONE, renderType, name.c_str(), name.c_str()));
			}

			for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(rangeSizes); sizeNdx++)
			{
				const size_t size		= rangeSizes[sizeNdx];
				const string name		("range_size_" + de::toString(size));
				const size_t bufferSize	= 131072;

				renderTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, bufferSize, 0, size, RENDERBITS_NONE, MODIFYBITS_NONE, renderType, name.c_str(), name.c_str()));
			}

			for (int offsetNdx = 0; offsetNdx < DE_LENGTH_OF_ARRAY(offsets); offsetNdx++)
			{
				const size_t offset		= offsets[offsetNdx];
				const size_t bufferSize	= 131072;
				const size_t size		= 65537;
				const string name		("offset_" + de::toString(offset) + "_alignments");

				renderTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, bufferSize, offset, size, RENDERBITS_NONE, MODIFYBITS_NONE, renderType, name.c_str(), name.c_str()));
			}
		}
	}

	// Modify tests
	{
		TestCaseGroup* const modifyGroup = new TestCaseGroup(context, "modify", "Modify texture buffer content in multiple ways");
		root->addChild(modifyGroup);

		for (int modifyNdx = 0; modifyNdx < DE_LENGTH_OF_ARRAY(modifyTypes); modifyNdx++)
		{
			const ModifyBits		modifyType		= modifyTypes[modifyNdx];
			TestCaseGroup* const	modifyTypeGroup	= new TestCaseGroup(context, toTestName(modifyType).c_str(), toTestName(modifyType).c_str());

			modifyGroup->addChild(modifyTypeGroup);

			for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(bufferSizes); sizeNdx++)
			{
				const size_t	size	= bufferSizes[sizeNdx];
				const string	name	("buffer_size_" + de::toString(size));

				modifyTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, size, 0, 0, RENDERBITS_NONE, modifyType, RENDERBITS_AS_FRAGMENT_TEXTURE, name.c_str(), name.c_str()));
			}

			for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(rangeSizes); sizeNdx++)
			{
				const size_t size		= rangeSizes[sizeNdx];
				const string name		("range_size_" + de::toString(size));
				const size_t bufferSize	= 131072;

				modifyTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, bufferSize, 0, size, RENDERBITS_NONE, modifyType, RENDERBITS_AS_FRAGMENT_TEXTURE, name.c_str(), name.c_str()));
			}

			for (int offsetNdx = 0; offsetNdx < DE_LENGTH_OF_ARRAY(offsets); offsetNdx++)
			{
				const size_t offset		= offsets[offsetNdx];
				const size_t bufferSize	= 131072;
				const size_t size		= 65537;
				const string name		("offset_" + de::toString(offset) + "_alignments");

				modifyTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, bufferSize, offset, size, RENDERBITS_NONE, modifyType, RENDERBITS_AS_FRAGMENT_TEXTURE, name.c_str(), name.c_str()));
			}
		}
	}

	// Modify-Render tests
	{
		TestCaseGroup* const modifyRenderGroup = new TestCaseGroup(context, "modify_render", "Modify texture buffer content in multiple ways and render in different ways");
		root->addChild(modifyRenderGroup);

		for (int modifyNdx = 0; modifyNdx < DE_LENGTH_OF_ARRAY(modifyTypes); modifyNdx++)
		{
			const ModifyBits		modifyType		= modifyTypes[modifyNdx];
			TestCaseGroup* const	modifyTypeGroup	= new TestCaseGroup(context, toTestName(modifyType).c_str(), toTestName(modifyType).c_str());

			modifyRenderGroup->addChild(modifyTypeGroup);

			for (int renderTypeNdx = 0; renderTypeNdx < DE_LENGTH_OF_ARRAY(renderTypeCombinations); renderTypeNdx++)
			{
				const RenderBits	renderType	= renderTypeCombinations[renderTypeNdx];
				const size_t		size		= 16*1024;
				const string		name		(toTestName(renderType));

				modifyTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, size, 0, 0, RENDERBITS_NONE, modifyType, renderType, name.c_str(), name.c_str()));
			}
		}
	}

	// Render-Modify tests
	{
		TestCaseGroup* const renderModifyGroup = new TestCaseGroup(context, "render_modify", "Render texture buffer and modify.");
		root->addChild(renderModifyGroup);

		for (int renderTypeNdx = 0; renderTypeNdx < DE_LENGTH_OF_ARRAY(renderTypeCombinations); renderTypeNdx++)
		{
			const RenderBits		renderType		= renderTypeCombinations[renderTypeNdx];
			TestCaseGroup* const	renderTypeGroup	= new TestCaseGroup(context, toTestName(renderType).c_str(), toTestName(renderType).c_str());

			renderModifyGroup->addChild(renderTypeGroup);

			for (int modifyNdx = 0; modifyNdx < DE_LENGTH_OF_ARRAY(modifyTypes); modifyNdx++)
			{
				const ModifyBits	modifyType	= modifyTypes[modifyNdx];
				const size_t		size		= 16*1024;
				const string		name		(toTestName(modifyType));

				renderTypeGroup->addChild(new TextureBufferCase(context.getTestContext(), context.getRenderContext(), GL_RGBA8, size, 0, 0, renderType, modifyType, RENDERBITS_AS_FRAGMENT_TEXTURE, name.c_str(), name.c_str()));
			}
		}
	}

	return root;
}

} // Functional
} // gles31
} // deqp
