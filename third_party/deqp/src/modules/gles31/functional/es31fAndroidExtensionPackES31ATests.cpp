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
 * \brief ANDROID_extension_pack_es31a tests
 *//*--------------------------------------------------------------------*/

#include "es31fAndroidExtensionPackES31ATests.hpp"
#include "glsStateQueryUtil.hpp"
#include "glsShaderLibrary.hpp"
#include "tcuTestLog.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static std::string genExtensionTestName (const char* extensionName)
{
	DE_ASSERT(deStringBeginsWith(extensionName, "GL_"));
	return de::toLower(std::string(extensionName + 3));
}

class ExtensionPackTestCase : public TestCase
{
public:
			ExtensionPackTestCase	(Context& context, const char* name, const char* description);

protected:
	void	init					(void);
};

ExtensionPackTestCase::ExtensionPackTestCase (Context& context, const char* name, const char* description)
	: TestCase	(context, name, description)
{
}

void ExtensionPackTestCase::init (void)
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ANDROID_extension_pack_es31a"))
		throw tcu::NotSupportedError("Test requires GL_ANDROID_extension_pack_es31a extension");
}

class ImplementationLimitCase : public ExtensionPackTestCase
{
public:
							ImplementationLimitCase	(Context& context, const char* name, const char* description, glw::GLenum target, int limit);

private:
	IterateResult			iterate					(void);

	const glw::GLenum		m_target;
	const int				m_limit;
};

ImplementationLimitCase::ImplementationLimitCase (Context& context, const char* name, const char* description, glw::GLenum target, int limit)
	: ExtensionPackTestCase	(context, name, description)
	, m_target				(target)
	, m_limit				(limit)
{
}

ImplementationLimitCase::IterateResult ImplementationLimitCase::iterate (void)
{
	using namespace gls::StateQueryUtil;

	glu::CallLogWrapper						gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	StateQueryMemoryWriteGuard<glw::GLint>	result;

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Querying " << glu::getGettableStateName(m_target) << ", expecting at least " << m_limit
		<< tcu::TestLog::EndMessage;

	gl.enableLogging(true);
	gl.glGetIntegerv(m_target, &result);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "implementation limit query failed");

	if (result.verifyValidity(m_testCtx) && result < m_limit)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "// ERROR: Got " << result << ", expected at least " << m_limit
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Too low implementation limit");
	}

	return STOP;
}

class SubExtensionCase : public ExtensionPackTestCase
{
public:
						SubExtensionCase	(Context& context, const char* name, const char* description, const char* extension);

private:
	IterateResult		iterate				(void);

	const std::string	m_extension;
};

SubExtensionCase::SubExtensionCase (Context& context, const char* name, const char* description, const char* extension)
	: ExtensionPackTestCase	(context, name, description)
	, m_extension			(extension)
{
}

SubExtensionCase::IterateResult SubExtensionCase::iterate (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying that extension \"" << m_extension << "\" is supported."
		<< tcu::TestLog::EndMessage;

	if (m_context.getContextInfo().isExtensionSupported(m_extension.c_str()))
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Extension is supported."
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Error, extension is not supported."
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Required extension not supported");
	}

	return STOP;
}

} //anonymous

AndroidExtensionPackES31ATests::AndroidExtensionPackES31ATests (Context& context)
	: TestCaseGroup(context, "android_extension_pack", "ANDROID_extension_pack_es31a extension tests")
{
}

AndroidExtensionPackES31ATests::~AndroidExtensionPackES31ATests (void)
{
}

void AndroidExtensionPackES31ATests::init (void)
{
	// .limits
	{
		static const struct
		{
			const char*	name;
			glw::GLenum	target;
			int			limit;
		} limits[] =
		{
			{
				"max_fragment_atomic_counter_buffers",
				GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS,
				1
			},
			{
				"max_fragment_atomic_counters",
				GL_MAX_FRAGMENT_ATOMIC_COUNTERS,
				8
			},
			{
				"max_fragment_image_uniforms",
				GL_MAX_FRAGMENT_IMAGE_UNIFORMS,
				4
			},
			{
				"max_fragment_shader_storage_blocks",
				GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,
				4
			},
		};

		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "limits", "Implementation limits");
		addChild(group);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(limits); ++ndx)
			group->addChild(new ImplementationLimitCase(m_context,
														limits[ndx].name,
														(std::string() + "Check " + limits[ndx].name + " is at least " + de::toString(limits[ndx].limit)).c_str(),
														limits[ndx].target,
														limits[ndx].limit));
	}

	// .extensions
	{
		static const char* const subExtensions[] =
		{
			"GL_KHR_debug",
			"GL_KHR_texture_compression_astc_ldr",
			"GL_KHR_blend_equation_advanced",
			"GL_OES_sample_shading",
			"GL_OES_sample_variables",
			"GL_OES_shader_image_atomic",
			"GL_OES_shader_multisample_interpolation",
			"GL_OES_texture_stencil8",
			"GL_OES_texture_storage_multisample_2d_array",
			"GL_EXT_copy_image",
			"GL_EXT_draw_buffers_indexed",
			"GL_EXT_geometry_shader",
			"GL_EXT_gpu_shader5",
			"GL_EXT_primitive_bounding_box",
			"GL_EXT_shader_io_blocks",
			"GL_EXT_tessellation_shader",
			"GL_EXT_texture_border_clamp",
			"GL_EXT_texture_buffer",
			"GL_EXT_texture_cube_map_array",
			"GL_EXT_texture_sRGB_decode",
		};

		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "extensions", "Required extensions");
		addChild(group);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(subExtensions); ++ndx)
		{
			const std::string name			= genExtensionTestName(subExtensions[ndx]);
			const std::string description	= "Check that extension " + name + " is supported if extension pack is supported";
			group->addChild(new SubExtensionCase(m_context, name.c_str(), description.c_str(), subExtensions[ndx]));
		}
	}

	// .shaders
	{
		gls::ShaderLibrary					shaderLibrary	(m_testCtx, m_context.getRenderContext(), m_context.getContextInfo());
		tcu::TestCaseGroup* const			group			= new tcu::TestCaseGroup(m_testCtx, "shaders", "Shader tests");

		{
			const std::vector<tcu::TestNode*>&	children		= shaderLibrary.loadShaderFile("shaders/es31/android_extension_pack.test");
			tcu::TestCaseGroup* const			groupES31		= new tcu::TestCaseGroup(m_testCtx, "es31", "GLSL ES 3.1 Shader tests", children);

			group->addChild(groupES31);
		}

		{
			const std::vector<tcu::TestNode*>&	children		= shaderLibrary.loadShaderFile("shaders/es32/android_extension_pack.test");
			tcu::TestCaseGroup* const			groupES32		= new tcu::TestCaseGroup(m_testCtx, "es32", "GLSL ES 3.2 Shader tests", children);

			group->addChild(groupES32);
		}

		addChild(group);
	}
}

} // Functional
} // gles31
} // deqp
