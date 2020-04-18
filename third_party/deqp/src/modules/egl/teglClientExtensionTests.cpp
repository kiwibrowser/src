/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief EGL_EXT_client_extensions tests
 *//*--------------------------------------------------------------------*/

#include "teglClientExtensionTests.hpp"

#include "tcuTestLog.hpp"

#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "deStringUtil.hpp"
#include "deSTLUtil.hpp"

#include <vector>
#include <set>
#include <string>
#include <sstream>

using std::string;
using std::vector;
using std::set;

using tcu::TestLog;

using namespace eglw;

namespace deqp
{
namespace egl
{
namespace
{

static const char* const s_displayExtensionList[] =
{
	"EGL_KHR_config_attribs",
	"EGL_KHR_lock_surface",
	"EGL_KHR_image",
	"EGL_KHR_vg_parent_image",
	"EGL_KHR_gl_texture_2D_image",
	"EGL_KHR_gl_texture_cubemap_image",
	"EGL_KHR_gl_texture_3D_image",
	"EGL_KHR_gl_renderbuffer_image",
	"EGL_KHR_reusable_sync",
	"EGL_KHR_image_base",
	"EGL_KHR_image_pixmap",
	"EGL_IMG_context_priority",
	"EGL_KHR_lock_surface2",
	"EGL_NV_coverage_sample",
	"EGL_NV_depth_nonlinear",
	"EGL_NV_sync",
	"EGL_KHR_fence_sync",
	"EGL_HI_clientpixmap",
	"EGL_HI_colorformats",
	"EGL_MESA_drm_image",
	"EGL_NV_post_sub_buffer",
	"EGL_ANGLE_query_surface_pointer",
	"EGL_ANGLE_surface_d3d_texture_2d_share_handle",
	"EGL_NV_coverage_sample_resolve",
//	"EGL_NV_system_time",	\todo [mika] Unclear which one this is
	"EGL_KHR_stream",
	"EGL_KHR_stream_consumer_gltexture",
	"EGL_KHR_stream_producer_eglsurface",
	"EGL_KHR_stream_producer_aldatalocator",
	"EGL_KHR_stream_fifo",
	"EGL_EXT_create_context_robustness",
	"EGL_ANGLE_d3d_share_handle_client_buffer",
	"EGL_KHR_create_context",
	"EGL_KHR_surfaceless_context",
	"EGL_KHR_stream_cross_process_fd",
	"EGL_EXT_multiview_window",
	"EGL_KHR_wait_sync",
	"EGL_NV_post_convert_rounding",
	"EGL_NV_native_query",
	"EGL_NV_3dvision_surface",
	"EGL_ANDROID_framebuffer_target",
	"EGL_ANDROID_blob_cache",
	"EGL_ANDROID_image_native_buffer",
	"EGL_ANDROID_native_fence_sync",
	"EGL_ANDROID_recordable",
	"EGL_EXT_buffer_age",
	"EGL_EXT_image_dma_buf_import",
	"EGL_ARM_pixmap_multisample_discard",
	"EGL_EXT_swap_buffers_with_damage",
	"EGL_NV_stream_sync",
	"EGL_KHR_cl_event",
	"EGL_KHR_get_all_proc_addresses"
};

static const char* const s_clientExtensionList[] =
{
	"EGL_EXT_platform_base",
	"EGL_EXT_client_extensions",
	"EGL_EXT_platform_x11",
	"EGL_KHR_client_get_all_proc_addresses",
	"EGL_MESA_platform_gbm",
	"EGL_EXT_platform_wayland"
};

class BaseTest : public TestCase
{
public:
					BaseTest	(EglTestContext& eglTestCtx);
	IterateResult	iterate		(void);
};

BaseTest::BaseTest (EglTestContext& eglTestCtx)
	: TestCase(eglTestCtx, "base", "Basic tests for EGL_EXT_client_extensions")
{
}

TestCase::IterateResult BaseTest::iterate (void)
{
	const Library&		egl					= m_eglTestCtx.getLibrary();
	const char* const	clientExtesionsStr	= egl.queryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	const EGLint		eglError			= egl.getError();

	if (eglError == EGL_BAD_DISPLAY)
		TCU_THROW(NotSupportedError, "EGL_EXT_client_extensions not supported");
	else if (eglError != EGL_SUCCESS)
		throw eglu::Error(eglError, "eglQueryString()", DE_NULL, __FILE__, __LINE__);

	TCU_CHECK(clientExtesionsStr);

	{
		bool				found		= false;
		std::istringstream	stream		(clientExtesionsStr);
		string				extension;

		while (std::getline(stream, extension, ' '))
		{
			if (extension == "EGL_EXT_client_extensions")
			{
				found = true;
				break;
			}
		}

		if (found)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
		{
			m_testCtx.getLog() << TestLog::Message << "eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS) didn't fail, but extension string doesn't contain EGL_EXT_client_extensions" <<TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

class CheckExtensionsTest : public TestCase
{
public:
					CheckExtensionsTest	(EglTestContext& eglTestCtx);
	IterateResult	iterate				(void);
};

CheckExtensionsTest::CheckExtensionsTest (EglTestContext& eglTestCtx)
	: TestCase(eglTestCtx, "extensions", "Check that returned extensions are client or display extensions")
{
}

TestCase::IterateResult CheckExtensionsTest::iterate (void)
{
	const Library&		egl						= m_eglTestCtx.getLibrary();
	bool				isOk					= true;
	const char* const	clientExtensionsStr		= egl.queryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	const EGLint		eglQueryError			= egl.getError();

	set<string>			knownClientExtensions	(s_clientExtensionList, s_clientExtensionList + DE_LENGTH_OF_ARRAY(s_clientExtensionList));
	set<string>			knownDisplayExtensions	(s_displayExtensionList, s_displayExtensionList + DE_LENGTH_OF_ARRAY(s_displayExtensionList));

	vector<string>		displayExtensions;
	vector<string>		clientExtensions;

	if (eglQueryError == EGL_BAD_DISPLAY)
		TCU_THROW(NotSupportedError, "EGL_EXT_client_extensions not supported");
	else if (eglQueryError != EGL_SUCCESS)
		throw eglu::Error(eglQueryError, "eglQueryString()", DE_NULL, __FILE__, __LINE__);

	TCU_CHECK(clientExtensionsStr);

	clientExtensions = de::splitString(clientExtensionsStr, ' ');

	{
		EGLDisplay	display	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

		displayExtensions = de::splitString(egl.queryString(display, EGL_EXTENSIONS), ' ');

		egl.terminate(display);
	}

	for (int extNdx = 0; extNdx < (int)clientExtensions.size(); extNdx++)
	{
		if (knownDisplayExtensions.find(clientExtensions[extNdx]) != knownDisplayExtensions.end())
		{
			m_testCtx.getLog() << TestLog::Message << "'" << clientExtensions[extNdx] << "' is not client extension" << TestLog::EndMessage;
			isOk = false;
		}
	}

	for (int extNdx = 0; extNdx < (int)displayExtensions.size(); extNdx++)
	{
		if (knownClientExtensions.find(displayExtensions[extNdx]) != knownClientExtensions.end())
		{
			m_testCtx.getLog() << TestLog::Message << "'" << displayExtensions[extNdx] << "' is not display extension" << TestLog::EndMessage;
			isOk = false;
		}
	}

	if (isOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}


class DisjointTest : public TestCase
{
public:
					DisjointTest	(EglTestContext& eglTestCtx);
	IterateResult	iterate			(void);
};

DisjointTest::DisjointTest (EglTestContext& eglTestCtx)
	: TestCase(eglTestCtx, "disjoint", "Check that client and display extensions are disjoint")
{
}

TestCase::IterateResult DisjointTest::iterate (void)
{
	const Library&		egl					= m_eglTestCtx.getLibrary();
	const char*	const	clientExtensionsStr	= egl.queryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	const EGLint		eglQueryError		= egl.getError();

	if (eglQueryError == EGL_BAD_DISPLAY)
		TCU_THROW(NotSupportedError, "EGL_EXT_client_extensions not supported");
	else if (eglQueryError != EGL_SUCCESS)
		throw eglu::Error(eglQueryError, "eglQueryString()", DE_NULL, __FILE__, __LINE__);

	vector<string>		displayExtensions;
	vector<string>		clientExtensions;

	clientExtensions = de::splitString(clientExtensionsStr, ' ');

	{
		EGLDisplay	display	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

		displayExtensions = de::splitString(egl.queryString(display, EGL_EXTENSIONS), ' ');

		egl.terminate(display);
	}

	// Log client extensions
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Client extensions", "Client extensions");

		for (int extNdx = 0; extNdx < (int)clientExtensions.size(); extNdx++)
			m_testCtx.getLog() << TestLog::Message << clientExtensions[extNdx] << TestLog::EndMessage;
	}

	// Log display extensions
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Display extensions", "Display extensions");

		for (int extNdx = 0; extNdx < (int)displayExtensions.size(); extNdx++)
			m_testCtx.getLog() << TestLog::Message << displayExtensions[extNdx] << TestLog::EndMessage;
	}

	// Check that sets are disjoint
	{
		set<string>			commonExtensionSet;
		const set<string>	clientExtensionSet(clientExtensions.begin(), clientExtensions.end());
		const set<string>	displayExtensionSet(displayExtensions.begin(), displayExtensions.end());

		for (set<string>::const_iterator iter = clientExtensionSet.begin(); iter != clientExtensionSet.end(); ++iter)
		{
			if (displayExtensionSet.find(*iter) != displayExtensionSet.end())
				commonExtensionSet.insert(*iter);
		}

		for (set<string>::const_iterator iter = commonExtensionSet.begin(); iter != commonExtensionSet.end(); ++iter)
			m_testCtx.getLog() << TestLog::Message << "Extension '" << *iter << "' exists in client and display extension sets." << TestLog::EndMessage;

		if (commonExtensionSet.empty())
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
		{
			m_testCtx.getLog() << TestLog::Message << "Extension sets are not disjoint" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

} // anonymous

ClientExtensionTests::ClientExtensionTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "client_extensions", "Test for EGL_EXT_client_extensions")
{
}

void ClientExtensionTests::init (void)
{
	addChild(new BaseTest(m_eglTestCtx));
	addChild(new DisjointTest(m_eglTestCtx));
	addChild(new CheckExtensionsTest(m_eglTestCtx));
}

} // egl
} // deqp
