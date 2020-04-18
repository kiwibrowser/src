#ifndef _ES2FBUFFERTESTUTIL_HPP
#define _ES2FBUFFERTESTUTIL_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 2.0 Module
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
 * \brief Buffer test utilities.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuTestLog.hpp"
#include "gluCallLogWrapper.hpp"
#include "tes2TestCase.hpp"

#include <vector>
#include <set>

namespace glu
{
class ShaderProgram;
}

namespace deqp
{
namespace gles2
{
namespace Functional
{
namespace BufferTestUtil
{

// Helper functions.

void			fillWithRandomBytes		(deUint8* ptr, int numBytes, deUint32 seed);
bool			compareByteArrays		(tcu::TestLog& log, const deUint8* resPtr, const deUint8* refPtr, int numBytes);
const char*		getBufferTargetName		(deUint32 target);
const char*		getUsageHintName		(deUint32 hint);

// Base class for buffer cases.

class BufferCase : public TestCase, public glu::CallLogWrapper
{
public:
							BufferCase							(Context& context, const char* name, const char* description);
	virtual					~BufferCase							(void);

	void					init								(void);
	void					deinit								(void);

	deUint32				genBuffer							(void);
	void					deleteBuffer						(deUint32 buffer);
	void					checkError							(void);

private:
	// Resource handles for cleanup in case of unexpected iterate() termination.
	std::set<deUint32>		m_allocatedBuffers;
};

// Reference buffer.

class ReferenceBuffer
{
public:
							ReferenceBuffer			(void) {}
							~ReferenceBuffer		(void) {}

	void					setSize					(int numBytes);
	void					setData					(int numBytes, const deUint8* bytes);
	void					setSubData				(int offset, int numBytes, const deUint8* bytes);

	deUint8*				getPtr					(int offset = 0)		{ return &m_data[offset]; }
	const deUint8*			getPtr					(int offset = 0) const	{ return &m_data[offset]; }

private:
	std::vector<deUint8>	m_data;
};

// Buffer verifier system.

enum VerifyType
{
	VERIFY_AS_VERTEX_ARRAY	= 0,
	VERIFY_AS_INDEX_ARRAY,

	VERIFY_LAST
};

class BufferVerifierBase : public glu::CallLogWrapper
{
public:
							BufferVerifierBase		(Context& context);
	virtual					~BufferVerifierBase		(void) {}

	virtual int				getMinSize				(void) const = DE_NULL;
	virtual int				getAlignment			(void) const = DE_NULL;
	virtual bool			verify					(deUint32 buffer, const deUint8* reference, int offset, int numBytes) = DE_NULL;

protected:
	Context&				m_context;

private:
							BufferVerifierBase		(const BufferVerifierBase& other);
	BufferVerifierBase&		operator=				(const BufferVerifierBase& other);
};

class BufferVerifier
{
public:
							BufferVerifier			(Context& context, VerifyType verifyType);
							~BufferVerifier			(void);

	int						getMinSize				(void) const { return m_verifier->getMinSize();		}
	int						getAlignment			(void) const { return m_verifier->getAlignment();	}

	// \note Offset is applied to reference pointer as well.
	bool					verify					(deUint32 buffer, const deUint8* reference, int offset, int numBytes);

private:
							BufferVerifier			(const BufferVerifier& other);
	BufferVerifier&			operator=				(const BufferVerifier& other);

	BufferVerifierBase*		m_verifier;
};

class VertexArrayVerifier : public BufferVerifierBase
{
public:
						VertexArrayVerifier		(Context& context);
						~VertexArrayVerifier	(void);

	int					getMinSize				(void) const { return 3*4; }
	int					getAlignment			(void) const { return 1; }
	bool				verify					(deUint32 buffer, const deUint8* reference, int offset, int numBytes);

private:
	glu::ShaderProgram*	m_program;
	deUint32			m_posLoc;
	deUint32			m_byteVecLoc;
};

class IndexArrayVerifier : public BufferVerifierBase
{
public:
						IndexArrayVerifier		(Context& context);
						~IndexArrayVerifier		(void);

	int					getMinSize				(void) const { return 2; }
	int					getAlignment			(void) const { return 1; }
	bool				verify					(deUint32 buffer, const deUint8* reference, int offset, int numBytes);

private:
	glu::ShaderProgram*	m_program;
	deUint32			m_posLoc;
	deUint32			m_colorLoc;
};

} // BufferTestUtil
} // Functional
} // gles2
} // deqp

#endif // _ES2FBUFFERTESTUTIL_HPP
