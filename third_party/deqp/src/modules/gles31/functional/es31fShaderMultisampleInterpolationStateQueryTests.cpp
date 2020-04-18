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
 * \brief Multisample interpolation state query tests
 *//*--------------------------------------------------------------------*/

#include "es31fShaderMultisampleInterpolationStateQueryTests.hpp"
#include "tcuTestLog.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluRenderContext.hpp"
#include "glsStateQueryUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"


namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace gls::StateQueryUtil;

class InterpolationOffsetCase : public TestCase
{
public:
	enum TestType
	{
		TEST_MIN_OFFSET = 0,
		TEST_MAX_OFFSET,

		TEST_LAST
	};

						InterpolationOffsetCase		(Context& context, const char* name, const char* desc, QueryType verifier, TestType testType);
						~InterpolationOffsetCase	(void);

	void				init						(void);
	IterateResult		iterate						(void);

private:
	const QueryType		m_verifier;
	const TestType		m_testType;
};

InterpolationOffsetCase::InterpolationOffsetCase (Context& context, const char* name, const char* desc, QueryType verifier, TestType testType)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
	, m_testType	(testType)
{
	DE_ASSERT(m_testType < TEST_LAST);
}

InterpolationOffsetCase::~InterpolationOffsetCase (void)
{
}

void InterpolationOffsetCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation"))
		throw tcu::NotSupportedError("Test requires GL_OES_shader_multisample_interpolation extension");
}

InterpolationOffsetCase::IterateResult InterpolationOffsetCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	gl.enableLogging(true);

	if (m_testType == TEST_MAX_OFFSET)
		verifyStateFloatMin(result, gl, GL_MAX_FRAGMENT_INTERPOLATION_OFFSET, 0.5, m_verifier);
	else if (m_testType == TEST_MIN_OFFSET)
		verifyStateFloatMax(result, gl, GL_MIN_FRAGMENT_INTERPOLATION_OFFSET, -0.5, m_verifier);
	else
		DE_ASSERT(false);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class FragmentInterpolationOffsetBitsCase : public TestCase
{
public:
						FragmentInterpolationOffsetBitsCase		(Context& context, const char* name, const char* desc, QueryType verifier);
						~FragmentInterpolationOffsetBitsCase	(void);

	void				init									(void);
	IterateResult		iterate									(void);

private:
	const QueryType		m_verifier;
};

FragmentInterpolationOffsetBitsCase::FragmentInterpolationOffsetBitsCase (Context& context, const char* name, const char* desc, QueryType verifier)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

FragmentInterpolationOffsetBitsCase::~FragmentInterpolationOffsetBitsCase (void)
{
}

void FragmentInterpolationOffsetBitsCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation"))
		throw tcu::NotSupportedError("Test requires GL_OES_shader_multisample_interpolation extension");
}

FragmentInterpolationOffsetBitsCase::IterateResult FragmentInterpolationOffsetBitsCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	gl.enableLogging(true);

	verifyStateIntegerMin(result, gl, GL_FRAGMENT_INTERPOLATION_OFFSET_BITS, 4, m_verifier);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

ShaderMultisampleInterpolationStateQueryTests::ShaderMultisampleInterpolationStateQueryTests (Context& context)
	: TestCaseGroup(context, "multisample_interpolation", "Test multisample interpolation states")
{
}

ShaderMultisampleInterpolationStateQueryTests::~ShaderMultisampleInterpolationStateQueryTests (void)
{
}

void ShaderMultisampleInterpolationStateQueryTests::init (void)
{
	static const struct Verifier
	{
		QueryType		verifier;
		const char*		name;
		const char*		desc;
	} verifiers[] =
	{
		{ QUERY_BOOLEAN,	"get_boolean",		"Test using getBoolean"		},
		{ QUERY_INTEGER,	"get_integer",		"Test using getInteger"		},
		{ QUERY_FLOAT,		"get_float",		"Test using getFloat"		},
		{ QUERY_INTEGER64,	"get_integer64",	"Test using getInteger64"	},
	};

	// .min_fragment_interpolation_offset
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "min_fragment_interpolation_offset", "Test MIN_FRAGMENT_INTERPOLATION_OFFSET");
		addChild(group);

		for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)
			group->addChild(new InterpolationOffsetCase(m_context, verifiers[verifierNdx].name, verifiers[verifierNdx].desc, verifiers[verifierNdx].verifier, InterpolationOffsetCase::TEST_MIN_OFFSET));
	}

	// .max_fragment_interpolation_offset
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "max_fragment_interpolation_offset", "Test MAX_FRAGMENT_INTERPOLATION_OFFSET");
		addChild(group);

		for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)
			group->addChild(new InterpolationOffsetCase(m_context, verifiers[verifierNdx].name, verifiers[verifierNdx].desc, verifiers[verifierNdx].verifier, InterpolationOffsetCase::TEST_MAX_OFFSET));
	}

	// .fragment_interpolation_offset_bits
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "fragment_interpolation_offset_bits", "Test FRAGMENT_INTERPOLATION_OFFSET_BITS");
		addChild(group);

		for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)
			group->addChild(new FragmentInterpolationOffsetBitsCase(m_context, verifiers[verifierNdx].name, verifiers[verifierNdx].desc, verifiers[verifierNdx].verifier));
	}
}

} // Functional
} // gles31
} // deqp
