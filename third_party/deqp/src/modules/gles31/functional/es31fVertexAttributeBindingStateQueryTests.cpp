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
 * \brief Vertex attribute binding state query tests.
 *//*--------------------------------------------------------------------*/

#include "es31fVertexAttributeBindingStateQueryTests.hpp"
#include "tcuTestLog.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluRenderContext.hpp"
#include "gluObjectWrapper.hpp"
#include "gluStrUtil.hpp"
#include "glsStateQueryUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "glsStateQueryUtil.hpp"
#include "deRandom.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace gls::StateQueryUtil;

class AttributeCase : public TestCase
{
public:
						AttributeCase		(Context& context, const char* name, const char* desc, QueryType verifier);

	IterateResult		iterate				(void);
	virtual void		test				(tcu::ResultCollector& result) = 0;

protected:
	const QueryType		m_verifier;
};

AttributeCase::AttributeCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

AttributeCase::IterateResult AttributeCase::iterate (void)
{
	tcu::ResultCollector result(m_testCtx.getLog(), " // ERROR: ");

	test(result);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class AttributeBindingCase : public AttributeCase
{
public:
			AttributeBindingCase	(Context& context, const char* name, const char* desc, QueryType verifier);
	void	test					(tcu::ResultCollector& result);
};

AttributeBindingCase::AttributeBindingCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: AttributeCase(context, name, desc, verifier)
{
}

void AttributeBindingCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::VertexArray	vao			(m_context.getRenderContext());
	glw::GLint			maxAttrs	= -1;

	gl.enableLogging(true);

	gl.glBindVertexArray(*vao);
	gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrs);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial values");

		for (int attr = 0; attr < de::max(16, maxAttrs); ++attr)
			verifyStateAttributeInteger(result, gl, GL_VERTEX_ATTRIB_BINDING, attr, attr, m_verifier);
	}

	// is part of vao
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "vao", "VAO state");
		glu::VertexArray			otherVao		(m_context.getRenderContext());

		// set to value A in vao1
		gl.glVertexAttribBinding(1, 4);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexAttribBinding");

		// set to value B in vao2
		gl.glBindVertexArray(*otherVao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		gl.glVertexAttribBinding(1, 7);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexAttribBinding");

		// check value is still ok in original vao
		gl.glBindVertexArray(*vao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		verifyStateAttributeInteger(result, gl, GL_VERTEX_ATTRIB_BINDING, 1, 4, m_verifier);
	}

	// random values
	{
		const tcu::ScopedLogSection	section			(m_testCtx.getLog(), "random", "Random values");
		de::Random					rnd				(0xabc);
		const int					numRandomTests	= 10;

		for (int randomTestNdx = 0; randomTestNdx < numRandomTests; ++randomTestNdx)
		{
			// switch random va to random binding
			const int	va				= rnd.getInt(0, de::max(16, maxAttrs)-1);
			const int	binding			= rnd.getInt(0, 16);

			gl.glVertexAttribBinding(va, binding);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexAttribBinding");

			verifyStateAttributeInteger(result, gl, GL_VERTEX_ATTRIB_BINDING, va, binding, m_verifier);
		}
	}
}

class AttributeRelativeOffsetCase : public AttributeCase
{
public:
			AttributeRelativeOffsetCase	(Context& context, const char* name, const char* desc, QueryType verifier);
	void	test						(tcu::ResultCollector& result);
};

AttributeRelativeOffsetCase::AttributeRelativeOffsetCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: AttributeCase(context, name, desc, verifier)
{
}

void AttributeRelativeOffsetCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::VertexArray	vao			(m_context.getRenderContext());
	glw::GLint			maxAttrs	= -1;

	gl.enableLogging(true);

	gl.glBindVertexArray(*vao);
	gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrs);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial values");

		for (int attr = 0; attr < de::max(16, maxAttrs); ++attr)
			verifyStateAttributeInteger(result, gl, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, attr, 0, m_verifier);
	}

	// is part of vao
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "vao", "VAO state");
		glu::VertexArray			otherVao		(m_context.getRenderContext());

		// set to value A in vao1
		gl.glVertexAttribFormat(1, 4, GL_FLOAT, GL_FALSE, 9);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexAttribFormat");

		// set to value B in vao2
		gl.glBindVertexArray(*otherVao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		gl.glVertexAttribFormat(1, 4, GL_FLOAT, GL_FALSE, 21);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexAttribFormat");

		// check value is still ok in original vao
		gl.glBindVertexArray(*vao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		verifyStateAttributeInteger(result, gl, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, 1, 9, m_verifier);
	}

	// random values
	{
		const tcu::ScopedLogSection	section			(m_testCtx.getLog(), "random", "Random values");
		de::Random					rnd				(0xabc);
		const int					numRandomTests	= 10;

		for (int randomTestNdx = 0; randomTestNdx < numRandomTests; ++randomTestNdx)
		{
			const int	va				= rnd.getInt(0, de::max(16, maxAttrs)-1);
			const int	offset			= rnd.getInt(0, 2047);

			gl.glVertexAttribFormat(va, 4, GL_FLOAT, GL_FALSE, offset);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexAttribFormat");

			verifyStateAttributeInteger(result, gl, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, va, offset, m_verifier);
		}
	}
}

class IndexedCase : public TestCase
{
public:
						IndexedCase			(Context& context, const char* name, const char* desc, QueryType verifier);

	IterateResult		iterate				(void);
	virtual void		test				(tcu::ResultCollector& result) = 0;

protected:
	const QueryType		m_verifier;
};

IndexedCase::IndexedCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

IndexedCase::IterateResult IndexedCase::iterate (void)
{
	tcu::ResultCollector result(m_testCtx.getLog(), " // ERROR: ");

	test(result);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class VertexBindingDivisorCase : public IndexedCase
{
public:
			VertexBindingDivisorCase	(Context& context, const char* name, const char* desc, QueryType verifier);
	void	test						(tcu::ResultCollector& result);
};

VertexBindingDivisorCase::VertexBindingDivisorCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: IndexedCase(context, name, desc, verifier)
{
}

void VertexBindingDivisorCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::VertexArray	vao					(m_context.getRenderContext());
	glw::GLint			reportedMaxBindings	= -1;
	glw::GLint			maxBindings;

	gl.enableLogging(true);

	gl.glBindVertexArray(*vao);
	gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &reportedMaxBindings);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	maxBindings = de::max(16, reportedMaxBindings);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial values");

		for (int binding = 0; binding < maxBindings; ++binding)
			verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_DIVISOR, binding, 0, m_verifier);
	}

	// is part of vao
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "vao", "VAO state");
		glu::VertexArray			otherVao		(m_context.getRenderContext());

		// set to value A in vao1
		gl.glVertexBindingDivisor(1, 4);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexBindingDivisor");

		// set to value B in vao2
		gl.glBindVertexArray(*otherVao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		gl.glVertexBindingDivisor(1, 9);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexBindingDivisor");

		// check value is still ok in original vao
		gl.glBindVertexArray(*vao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_DIVISOR, 1, 4, m_verifier);
	}

	// random values
	{
		const tcu::ScopedLogSection	section			(m_testCtx.getLog(), "random", "Random values");
		de::Random					rnd				(0xabc);
		const int					numRandomTests	= 10;

		for (int randomTestNdx = 0; randomTestNdx < numRandomTests; ++randomTestNdx)
		{
			const int	binding			= rnd.getInt(0, maxBindings-1);
			const int	divisor			= rnd.getInt(0, 2047);

			gl.glVertexBindingDivisor(binding, divisor);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glVertexBindingDivisor");

			verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_DIVISOR, binding, divisor, m_verifier);
		}
	}
}

class VertexBindingOffsetCase : public IndexedCase
{
public:
			VertexBindingOffsetCase		(Context& context, const char* name, const char* desc, QueryType verifier);
	void	test						(tcu::ResultCollector& result);
};

VertexBindingOffsetCase::VertexBindingOffsetCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: IndexedCase(context, name, desc, verifier)
{
}

void VertexBindingOffsetCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::VertexArray	vao					(m_context.getRenderContext());
	glu::Buffer			buffer				(m_context.getRenderContext());
	glw::GLint			reportedMaxBindings	= -1;
	glw::GLint			maxBindings;

	gl.enableLogging(true);

	gl.glBindVertexArray(*vao);
	gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &reportedMaxBindings);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	maxBindings = de::max(16, reportedMaxBindings);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial values");

		for (int binding = 0; binding < maxBindings; ++binding)
			verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_OFFSET, binding, 0, m_verifier);
	}

	// is part of vao
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "vao", "VAO state");
		glu::VertexArray			otherVao		(m_context.getRenderContext());

		// set to value A in vao1
		gl.glBindVertexBuffer(1, *buffer, 4, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

		// set to value B in vao2
		gl.glBindVertexArray(*otherVao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		gl.glBindVertexBuffer(1, *buffer, 13, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

		// check value is still ok in original vao
		gl.glBindVertexArray(*vao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_OFFSET, 1, 4, m_verifier);
	}

	// random values
	{
		const tcu::ScopedLogSection	section			(m_testCtx.getLog(), "random", "Random values");
		de::Random					rnd				(0xabc);
		const int					numRandomTests	= 10;

		for (int randomTestNdx = 0; randomTestNdx < numRandomTests; ++randomTestNdx)
		{
			const int	binding			= rnd.getInt(0, maxBindings-1);
			const int	offset			= rnd.getInt(0, 4000);

			gl.glBindVertexBuffer(binding, *buffer, offset, 32);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

			verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_OFFSET, binding, offset, m_verifier);
		}
	}
}

class VertexBindingStrideCase : public IndexedCase
{
public:
			VertexBindingStrideCase		(Context& context, const char* name, const char* desc, QueryType verifier);
	void	test						(tcu::ResultCollector& result);
};

VertexBindingStrideCase::VertexBindingStrideCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: IndexedCase(context, name, desc, verifier)
{
}

void VertexBindingStrideCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::VertexArray	vao					(m_context.getRenderContext());
	glu::Buffer			buffer				(m_context.getRenderContext());
	glw::GLint			reportedMaxBindings	= -1;
	glw::GLint			maxBindings;

	gl.enableLogging(true);

	gl.glBindVertexArray(*vao);
	gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &reportedMaxBindings);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	maxBindings = de::max(16, reportedMaxBindings);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial values");

		for (int binding = 0; binding < maxBindings; ++binding)
			verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_STRIDE, binding, 16, m_verifier);
	}

	// is part of vao
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "vao", "VAO state");
		glu::VertexArray			otherVao		(m_context.getRenderContext());

		// set to value A in vao1
		gl.glBindVertexBuffer(1, *buffer, 0, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

		// set to value B in vao2
		gl.glBindVertexArray(*otherVao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		gl.glBindVertexBuffer(1, *buffer, 0, 64);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

		// check value is still ok in original vao
		gl.glBindVertexArray(*vao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_STRIDE, 1, 32, m_verifier);
	}

	// random values
	{
		const tcu::ScopedLogSection	section			(m_testCtx.getLog(), "random", "Random values");
		de::Random					rnd				(0xabc);
		const int					numRandomTests	= 10;

		for (int randomTestNdx = 0; randomTestNdx < numRandomTests; ++randomTestNdx)
		{
			const int	binding			= rnd.getInt(0, maxBindings-1);
			const int	stride			= rnd.getInt(0, 2048);

			gl.glBindVertexBuffer(binding, *buffer, 0, stride);
			GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

			verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_STRIDE, binding, stride, m_verifier);
		}
	}
}

class VertexBindingBufferCase : public IndexedCase
{
public:
			VertexBindingBufferCase		(Context& context, const char* name, const char* desc, QueryType verifier);
	void	test						(tcu::ResultCollector& result);
};

VertexBindingBufferCase::VertexBindingBufferCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: IndexedCase(context, name, desc, verifier)
{
}

void VertexBindingBufferCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::VertexArray	vao					(m_context.getRenderContext());
	glu::Buffer			buffer				(m_context.getRenderContext());
	glw::GLint			reportedMaxBindings	= -1;
	glw::GLint			maxBindings;

	gl.enableLogging(true);

	gl.glBindVertexArray(*vao);
	gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &reportedMaxBindings);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	maxBindings = de::max(16, reportedMaxBindings);

	// initial
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "initial", "Initial values");

		for (int binding = 0; binding < maxBindings; ++binding)
			verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_BUFFER, binding, 0, m_verifier);
	}

	// is part of vao
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "vao", "VAO state");
		glu::VertexArray			otherVao		(m_context.getRenderContext());
		glu::Buffer					otherBuffer		(m_context.getRenderContext());

		// set to value A in vao1
		gl.glBindVertexBuffer(1, *buffer, 0, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

		// set to value B in vao2
		gl.glBindVertexArray(*otherVao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");
		gl.glBindVertexBuffer(1, *otherBuffer, 0, 32);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexBuffer");

		// check value is still ok in original vao
		gl.glBindVertexArray(*vao);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glBindVertexArray");

		verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_BUFFER, 1, *buffer, m_verifier);
	}

	// Is detached in delete from active vao and not from deactive
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "autoUnbind", "Unbind on delete");
		glu::VertexArray			otherVao		(m_context.getRenderContext());
		glw::GLuint					otherBuffer		= -1;

		gl.glGenBuffers(1, &otherBuffer);

		// set in vao1 and vao2
		gl.glBindVertexBuffer(1, otherBuffer, 0, 32);
		gl.glBindVertexArray(*otherVao);
		gl.glBindVertexBuffer(1, otherBuffer, 0, 32);

		// delete buffer. This unbinds it from active (vao2) but not from unactive
		gl.glDeleteBuffers(1, &otherBuffer);
		verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_BUFFER, 1, 0, m_verifier);

		gl.glBindVertexArray(*vao);
		verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_BUFFER, 1, otherBuffer, m_verifier);
	}
}

class MixedVertexBindingDivisorCase : public IndexedCase
{
public:
			MixedVertexBindingDivisorCase	(Context& context, const char* name, const char* desc);
	void	test							(tcu::ResultCollector& result);
};

MixedVertexBindingDivisorCase::MixedVertexBindingDivisorCase (Context& context, const char* name, const char* desc)
	: IndexedCase(context, name, desc, QUERY_INDEXED_INTEGER)
{
}

void MixedVertexBindingDivisorCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::VertexArray	vao					(m_context.getRenderContext());

	gl.enableLogging(true);

	gl.glBindVertexArray(*vao);
	gl.glVertexAttribDivisor(1, 4);
	verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_DIVISOR, 1, 4, m_verifier);
}

class MixedVertexBindingOffsetCase : public IndexedCase
{
public:
			MixedVertexBindingOffsetCase	(Context& context, const char* name, const char* desc);
	void	test							(tcu::ResultCollector& result);
};

MixedVertexBindingOffsetCase::MixedVertexBindingOffsetCase (Context& context, const char* name, const char* desc)
	: IndexedCase(context, name, desc, QUERY_INDEXED_INTEGER)
{
}

void MixedVertexBindingOffsetCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::Buffer			buffer				(m_context.getRenderContext());

	gl.enableLogging(true);

	gl.glBindBuffer(GL_ARRAY_BUFFER, *buffer);
	gl.glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (const deUint8*)DE_NULL + 12);

	verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_OFFSET, 1, 12, m_verifier);
}

class MixedVertexBindingStrideCase : public IndexedCase
{
public:
			MixedVertexBindingStrideCase	(Context& context, const char* name, const char* desc);
	void	test							(tcu::ResultCollector& result);
};

MixedVertexBindingStrideCase::MixedVertexBindingStrideCase (Context& context, const char* name, const char* desc)
	: IndexedCase(context, name, desc, QUERY_INDEXED_INTEGER)
{
}

void MixedVertexBindingStrideCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::Buffer			buffer				(m_context.getRenderContext());

	gl.enableLogging(true);

	gl.glBindBuffer(GL_ARRAY_BUFFER, *buffer);
	gl.glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 12, 0);
	verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_STRIDE, 1, 12, m_verifier);

	// test effectiveStride
	gl.glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_STRIDE, 1, 16, m_verifier);
}

class MixedVertexBindingBufferCase : public IndexedCase
{
public:
			MixedVertexBindingBufferCase	(Context& context, const char* name, const char* desc);
	void	test							(tcu::ResultCollector& result);
};

MixedVertexBindingBufferCase::MixedVertexBindingBufferCase (Context& context, const char* name, const char* desc)
	: IndexedCase(context, name, desc, QUERY_INDEXED_INTEGER)
{
}

void MixedVertexBindingBufferCase::test (tcu::ResultCollector& result)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::Buffer			buffer				(m_context.getRenderContext());

	gl.enableLogging(true);

	gl.glBindBuffer(GL_ARRAY_BUFFER, *buffer);
	gl.glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	verifyStateIndexedInteger(result, gl, GL_VERTEX_BINDING_BUFFER, 1, *buffer, m_verifier);
}

} // anonymous

VertexAttributeBindingStateQueryTests::VertexAttributeBindingStateQueryTests (Context& context)
	: TestCaseGroup(context, "vertex_attribute_binding", "Query vertex attribute binding state.")
{
}

VertexAttributeBindingStateQueryTests::~VertexAttributeBindingStateQueryTests (void)
{
}

void VertexAttributeBindingStateQueryTests::init (void)
{
	tcu::TestCaseGroup* const attributeGroup	= new TestCaseGroup(m_context, "vertex_attrib", "Vertex attribute state");
	tcu::TestCaseGroup* const indexedGroup		= new TestCaseGroup(m_context, "indexed", "Indexed state");

	addChild(attributeGroup);
	addChild(indexedGroup);

	// .vertex_attrib
	{
		static const struct Verifier
		{
			const char*		suffix;
			QueryType		type;
		} verifiers[] =
		{
			{ "",						QUERY_ATTRIBUTE_INTEGER					},	// avoid renaming tests
			{ "_getvertexattribfv",		QUERY_ATTRIBUTE_FLOAT					},
			{ "_getvertexattribiiv",	QUERY_ATTRIBUTE_PURE_INTEGER			},
			{ "_getvertexattribiuiv",	QUERY_ATTRIBUTE_PURE_UNSIGNED_INTEGER	},
		};

		for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)
		{
			attributeGroup->addChild(new AttributeBindingCase		(m_context,	(std::string("vertex_attrib_binding") + verifiers[verifierNdx].suffix).c_str(),			"Test VERTEX_ATTRIB_BINDING",			verifiers[verifierNdx].type));
			attributeGroup->addChild(new AttributeRelativeOffsetCase(m_context,	(std::string("vertex_attrib_relative_offset") + verifiers[verifierNdx].suffix).c_str(),	"Test VERTEX_ATTRIB_RELATIVE_OFFSET",	verifiers[verifierNdx].type));
		}
	}

	// .indexed
	{
		static const struct Verifier
		{
			const char*		name;
			QueryType		type;
		} verifiers[] =
		{
			{ "getintegeri",	QUERY_INDEXED_INTEGER	},
			{ "getintegeri64",	QUERY_INDEXED_INTEGER64	},
			{ "getboolean",		QUERY_INDEXED_BOOLEAN	},
		};

		// states

		for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)
		{
			indexedGroup->addChild(new VertexBindingDivisorCase	(m_context, (std::string("vertex_binding_divisor_") + verifiers[verifierNdx].name).c_str(),	"Test VERTEX_BINDING_DIVISOR",	verifiers[verifierNdx].type));
			indexedGroup->addChild(new VertexBindingOffsetCase	(m_context, (std::string("vertex_binding_offset_") + verifiers[verifierNdx].name).c_str(),	"Test VERTEX_BINDING_OFFSET",	verifiers[verifierNdx].type));
			indexedGroup->addChild(new VertexBindingStrideCase	(m_context, (std::string("vertex_binding_stride_") + verifiers[verifierNdx].name).c_str(),	"Test VERTEX_BINDING_STRIDE",	verifiers[verifierNdx].type));
			indexedGroup->addChild(new VertexBindingBufferCase	(m_context, (std::string("vertex_binding_buffer_") + verifiers[verifierNdx].name).c_str(),	"Test VERTEX_BINDING_BUFFER",	verifiers[verifierNdx].type));
		}

		// mixed apis

		indexedGroup->addChild(new MixedVertexBindingDivisorCase(m_context, "vertex_binding_divisor_mixed",	"Test VERTEX_BINDING_DIVISOR"));
		indexedGroup->addChild(new MixedVertexBindingOffsetCase	(m_context, "vertex_binding_offset_mixed",	"Test VERTEX_BINDING_OFFSET"));
		indexedGroup->addChild(new MixedVertexBindingStrideCase	(m_context, "vertex_binding_stride_mixed",	"Test VERTEX_BINDING_STRIDE"));
		indexedGroup->addChild(new MixedVertexBindingBufferCase	(m_context, "vertex_binding_buffer_mixed",	"Test VERTEX_BINDING_BUFFER"));
	}
}

} // Functional
} // gles31
} // deqp
