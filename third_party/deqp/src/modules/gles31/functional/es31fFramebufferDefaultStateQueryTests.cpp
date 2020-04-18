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
 * \brief Framebuffer Default State Query tests.
 *//*--------------------------------------------------------------------*/

#include "es31fFramebufferDefaultStateQueryTests.hpp"
#include "glsStateQueryUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluObjectWrapper.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

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
		case QUERY_FRAMEBUFFER_INTEGER:	return "get_framebuffer_parameteriv";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

class FramebufferTest : public TestCase
{
public:
						FramebufferTest		(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate				(void);

protected:
	virtual void		checkInitial		(tcu::ResultCollector& result, glu::CallLogWrapper& gl) = 0;
	virtual void		checkSet			(tcu::ResultCollector& result, glu::CallLogWrapper& gl) = 0;

	const QueryType		m_verifier;
};

FramebufferTest::FramebufferTest (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

FramebufferTest::IterateResult FramebufferTest::iterate (void)
{
	glu::Framebuffer		fbo		(m_context.getRenderContext());
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fbo);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "bind");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial");
		checkInitial(result, gl);
	}

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Set", "Set");
		checkSet(result, gl);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class FramebufferDimensionTest : public FramebufferTest
{
public:
	enum DimensionType
	{
		DIMENSION_WIDTH = 0,
		DIMENSION_HEIGHT,

		DIMENSION_LAST
	};

						FramebufferDimensionTest	(Context& context, QueryType verifier, DimensionType dimension, const char* name, const char* desc);
	void				checkInitial				(tcu::ResultCollector& result, glu::CallLogWrapper& gl);
	void				checkSet					(tcu::ResultCollector& result, glu::CallLogWrapper& gl);

private:
	const DimensionType	m_dimension;
};

FramebufferDimensionTest::FramebufferDimensionTest (Context& context, QueryType verifier, DimensionType dimension, const char* name, const char* desc)
	: FramebufferTest	(context, verifier, name, desc)
	, m_dimension		(dimension)
{
	DE_ASSERT(dimension < DIMENSION_LAST);
}

void FramebufferDimensionTest::checkInitial (tcu::ResultCollector& result, glu::CallLogWrapper& gl)
{
	const glw::GLenum pname = (m_dimension == DIMENSION_WIDTH) ? (GL_FRAMEBUFFER_DEFAULT_WIDTH) : (GL_FRAMEBUFFER_DEFAULT_HEIGHT);
	verifyStateFramebufferInteger(result, gl, GL_DRAW_FRAMEBUFFER, pname, 0, m_verifier);
}

void FramebufferDimensionTest::checkSet (tcu::ResultCollector& result, glu::CallLogWrapper& gl)
{
	const glw::GLenum pname = (m_dimension == DIMENSION_WIDTH) ? (GL_FRAMEBUFFER_DEFAULT_WIDTH) : (GL_FRAMEBUFFER_DEFAULT_HEIGHT);

	gl.glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, pname, 32);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set state");

	verifyStateFramebufferInteger(result, gl, GL_DRAW_FRAMEBUFFER, pname, 32, m_verifier);
}

class FramebufferSamplesTest : public FramebufferTest
{
public:
						FramebufferSamplesTest	(Context& context, QueryType verifier, const char* name, const char* desc);
	void				checkInitial			(tcu::ResultCollector& result, glu::CallLogWrapper& gl);
	void				checkSet				(tcu::ResultCollector& result, glu::CallLogWrapper& gl);
};

FramebufferSamplesTest::FramebufferSamplesTest (Context& context, QueryType verifier, const char* name, const char* desc)
	: FramebufferTest(context, verifier, name, desc)
{
}

void FramebufferSamplesTest::checkInitial (tcu::ResultCollector& result, glu::CallLogWrapper& gl)
{
	verifyStateFramebufferInteger(result, gl, GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 0, m_verifier);
}

void FramebufferSamplesTest::checkSet (tcu::ResultCollector& result, glu::CallLogWrapper& gl)
{
	gl.glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 1);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set state");
	verifyStateFramebufferIntegerMin(result, gl, GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 1, m_verifier);

	gl.glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 0);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set state");
	verifyStateFramebufferInteger(result, gl, GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 0, m_verifier);
}

class FramebufferFixedSampleLocationsTest : public FramebufferTest
{
public:
						FramebufferFixedSampleLocationsTest	(Context& context, QueryType verifier, const char* name, const char* desc);
	void				checkInitial						(tcu::ResultCollector& result, glu::CallLogWrapper& gl);
	void				checkSet							(tcu::ResultCollector& result, glu::CallLogWrapper& gl);
};

FramebufferFixedSampleLocationsTest::FramebufferFixedSampleLocationsTest (Context& context, QueryType verifier, const char* name, const char* desc)
	: FramebufferTest(context, verifier, name, desc)
{
}

void FramebufferFixedSampleLocationsTest::checkInitial (tcu::ResultCollector& result, glu::CallLogWrapper& gl)
{
	verifyStateFramebufferInteger(result, gl, GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, 0, m_verifier);
}

void FramebufferFixedSampleLocationsTest::checkSet (tcu::ResultCollector& result, glu::CallLogWrapper& gl)
{
	gl.glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set state");
	verifyStateFramebufferInteger(result, gl, GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, GL_TRUE, m_verifier);
}

} // anonymous

FramebufferDefaultStateQueryTests::FramebufferDefaultStateQueryTests (Context& context)
	: TestCaseGroup(context, "framebuffer_default", "Framebuffer Default State Query tests")
{
}

FramebufferDefaultStateQueryTests::~FramebufferDefaultStateQueryTests (void)
{
}

void FramebufferDefaultStateQueryTests::init (void)
{
	static const QueryType verifiers[] =
	{
		QUERY_FRAMEBUFFER_INTEGER,
	};

#define FOR_EACH_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)	\
	{																						\
		const char* verifierSuffix = getVerifierSuffix(verifiers[verifierNdx]);				\
		const QueryType verifier = verifiers[verifierNdx];								\
		this->addChild(X);																	\
	}

	FOR_EACH_VERIFIER(new FramebufferDimensionTest				(m_context, verifier, FramebufferDimensionTest::DIMENSION_WIDTH,	(std::string("framebuffer_default_width_") + verifierSuffix).c_str(),					"Test FRAMEBUFFER_DEFAULT_WIDTH"));
	FOR_EACH_VERIFIER(new FramebufferDimensionTest				(m_context, verifier, FramebufferDimensionTest::DIMENSION_HEIGHT,	(std::string("framebuffer_default_height_") + verifierSuffix).c_str(),					"Test FRAMEBUFFER_DEFAULT_HEIGHT"));
	FOR_EACH_VERIFIER(new FramebufferSamplesTest				(m_context, verifier,												(std::string("framebuffer_default_samples_") + verifierSuffix).c_str(),					"Test FRAMEBUFFER_DEFAULT_SAMPLES"));
	FOR_EACH_VERIFIER(new FramebufferFixedSampleLocationsTest	(m_context, verifier,												(std::string("framebuffer_default_fixed_sample_locations_") + verifierSuffix).c_str(),	"Test FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS"));

#undef FOR_EACH_VERIFIER
}

} // Functional
} // gles31
} // deqp
