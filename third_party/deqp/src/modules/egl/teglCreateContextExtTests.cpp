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
 * \brief Simple context construction test for EGL_KHR_create_context.
 *//*--------------------------------------------------------------------*/

#include "teglCreateContextExtTests.hpp"

#include "tcuTestLog.hpp"

#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"
#include "egluConfigFilter.hpp"
#include "egluStrUtil.hpp"
#include "egluUtil.hpp"
#include "egluUnique.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"
#include "gluRenderConfig.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deSTLUtil.hpp"

#include <string>
#include <vector>
#include <set>
#include <sstream>

#include <cstring>

using std::set;
using std::string;
using std::vector;
using tcu::TestLog;

using namespace eglw;

// Make sure KHR / core values match to those in GL_ARB_robustness and GL_EXT_robustness
DE_STATIC_ASSERT(GL_RESET_NOTIFICATION_STRATEGY	== 0x8256);
DE_STATIC_ASSERT(GL_LOSE_CONTEXT_ON_RESET		== 0x8252);
DE_STATIC_ASSERT(GL_NO_RESET_NOTIFICATION		== 0x8261);

#if !defined(GL_CONTEXT_ROBUST_ACCESS)
#	define GL_CONTEXT_ROBUST_ACCESS		0x90F3
#endif

namespace deqp
{
namespace egl
{

namespace
{

size_t getAttribListLength (const EGLint* attribList)
{
	size_t size = 0;

	while (attribList[size] != EGL_NONE)
		size++;

	return size + 1;
}

string eglContextFlagsToString (EGLint flags)
{
	std::ostringstream	stream;

	if (flags == 0)
		stream << "<None>";
	else
	{
		bool first = true;

		if ((flags & EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR) != 0)
		{
			if (!first)
				stream << "|";

			first = false;

			stream << "EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR";
		}

		if ((flags & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR) != 0)
		{
			if (!first)
				stream << "|";

			first = false;

			stream << "EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR";
		}

		if ((flags & EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR) != 0)
		{
			if (!first)
				stream << "|";

			stream << "EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR";
		}
	}

	return stream.str();
}

string eglProfileMaskToString (EGLint mask)
{
	std::ostringstream	stream;

	if (mask == 0)
		stream << "<None>";
	else
	{
		bool first = true;

		if ((mask & EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR) != 0)
		{
			if (!first)
				stream << "|";

			first = false;

			stream << "EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR";
		}

		if ((mask & EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR) != 0)
		{
			if (!first)
				stream << "|";

			stream << "EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR";
		}
	}

	return stream.str();
}

const char* eglResetNotificationStrategyToString (EGLint strategy)
{
	switch (strategy)
	{
		case EGL_NO_RESET_NOTIFICATION_KHR:		return "EGL_NO_RESET_NOTIFICATION_KHR";
		case EGL_LOSE_CONTEXT_ON_RESET_KHR:		return "EGL_LOSE_CONTEXT_ON_RESET_KHR";
		default:
			return "<Unknown>";
	}
}

class CreateContextExtCase : public TestCase
{
public:
								CreateContextExtCase	(EglTestContext& eglTestCtx, EGLenum api, const EGLint* attribList, const eglu::FilterList& filter, const char* name, const char* description);
								~CreateContextExtCase	(void);

	void						executeForSurface		(EGLConfig config, EGLSurface surface);

	void						init					(void);
	void						deinit					(void);

	IterateResult				iterate					(void);
	void						checkRequiredExtensions	(void);
	void						logAttribList			(void);
	bool						validateCurrentContext	(const glw::Functions& gl);

private:
	bool						m_isOk;
	int							m_iteration;

	const eglu::FilterList		m_filter;
	vector<EGLint>				m_attribList;
	const EGLenum				m_api;

	EGLDisplay					m_display;
	vector<EGLConfig>			m_configs;
	glu::ContextType			m_glContextType;
};

glu::ContextType attribListToContextType (EGLenum api, const EGLint* attribList)
{
	EGLint				majorVersion	= 1;
	EGLint				minorVersion	= 0;
	glu::ContextFlags	flags			= glu::ContextFlags(0);
	glu::Profile		profile			= api == EGL_OPENGL_ES_API ? glu::PROFILE_ES : glu::PROFILE_CORE;
	const EGLint*		iter			= attribList;

	while ((*iter) != EGL_NONE)
	{
		switch (*iter)
		{
			case EGL_CONTEXT_MAJOR_VERSION_KHR:
				iter++;
				majorVersion = (*iter);
				iter++;
				break;

			case EGL_CONTEXT_MINOR_VERSION_KHR:
				iter++;
				minorVersion = (*iter);
				iter++;
				break;

			case EGL_CONTEXT_FLAGS_KHR:
				iter++;

				if ((*iter & EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR) != 0)
					flags = flags | glu::CONTEXT_ROBUST;

				if ((*iter & EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR) != 0)
					flags = flags | glu::CONTEXT_DEBUG;

				if ((*iter & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR) != 0)
					flags = flags | glu::CONTEXT_FORWARD_COMPATIBLE;

				iter++;
				break;

			case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
				iter++;

				if (*iter == EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR)
					profile = glu::PROFILE_COMPATIBILITY;
				else if (*iter != EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR)
					throw tcu::InternalError("Indeterminate OpenGL profile");

				iter++;
				break;

			case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
				iter += 2;
				break;

			case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT:
				iter += 2;
				break;

			case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
				iter += 2;
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}
	}

	return glu::ContextType(majorVersion, minorVersion, profile, flags);
}

CreateContextExtCase::CreateContextExtCase (EglTestContext& eglTestCtx, EGLenum api, const EGLint* attribList, const eglu::FilterList& filter, const char* name, const char* description)
	: TestCase			(eglTestCtx, name, description)
	, m_isOk			(true)
	, m_iteration		(0)
	, m_filter			(filter)
	, m_attribList		(attribList, attribList + getAttribListLength(attribList))
	, m_api				(api)
	, m_display			(EGL_NO_DISPLAY)
	, m_glContextType	(attribListToContextType(api, attribList))
{
}

CreateContextExtCase::~CreateContextExtCase (void)
{
	deinit();
}

void CreateContextExtCase::init (void)
{
	m_display	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_configs	= eglu::chooseConfigs(m_eglTestCtx.getLibrary(), m_display, m_filter);
}

void CreateContextExtCase::deinit (void)
{
	m_attribList.clear();
	m_configs.clear();

	if (m_display != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

void CreateContextExtCase::logAttribList (void)
{
	const EGLint*		iter = &(m_attribList[0]);
	std::ostringstream	attribListString;

	while ((*iter) != EGL_NONE)
	{
		switch (*iter)
		{
			case EGL_CONTEXT_MAJOR_VERSION_KHR:
				iter++;
				attribListString << "EGL_CONTEXT_MAJOR_VERSION_KHR(EGL_CONTEXT_CLIENT_VERSION), " << (*iter) << ", ";
				iter++;
				break;

			case EGL_CONTEXT_MINOR_VERSION_KHR:
				iter++;
				attribListString << "EGL_CONTEXT_MINOR_VERSION_KHR, " << (*iter) << ", ";
				iter++;
				break;

			case EGL_CONTEXT_FLAGS_KHR:
				iter++;
				attribListString << "EGL_CONTEXT_FLAGS_KHR, " << eglContextFlagsToString(*iter) << ", ";
				iter++;
				break;

			case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
				iter++;
				attribListString << "EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, " << eglProfileMaskToString(*iter) << ", ";
				iter++;
				break;

			case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
				iter++;
				attribListString << "EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR, " << eglResetNotificationStrategyToString(*iter) << ", ";
				iter++;
				break;

			case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
				iter++;
				attribListString << "EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT, ";

				if (*iter == EGL_FALSE || *iter == EGL_TRUE)
					attribListString << (*iter ? "EGL_TRUE" : "EGL_FALSE");
				else
					attribListString << (*iter);
				iter++;
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}
	}

	attribListString << "EGL_NONE";
	m_testCtx.getLog() << TestLog::Message << "EGL attrib list: { " << attribListString.str() << " }" << TestLog::EndMessage;
}

void CreateContextExtCase::checkRequiredExtensions (void)
{
	bool			isOk = true;
	set<string>		requiredExtensions;
	vector<string>	extensions			= eglu::getDisplayExtensions(m_eglTestCtx.getLibrary(), m_display);

	{
		const EGLint* iter = &(m_attribList[0]);

		while ((*iter) != EGL_NONE)
		{
			switch (*iter)
			{
				case EGL_CONTEXT_MAJOR_VERSION_KHR:
					iter++;
					iter++;
					break;

				case EGL_CONTEXT_MINOR_VERSION_KHR:
					iter++;
					requiredExtensions.insert("EGL_KHR_create_context");
					iter++;
					break;

				case EGL_CONTEXT_FLAGS_KHR:
					iter++;
					requiredExtensions.insert("EGL_KHR_create_context");
					iter++;
					break;

				case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
					iter++;
					requiredExtensions.insert("EGL_KHR_create_context");
					iter++;
					break;

				case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
					iter++;
					requiredExtensions.insert("EGL_KHR_create_context");
					iter++;
					break;

				case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT:
					iter++;
					requiredExtensions.insert("EGL_EXT_create_context_robustness");
					iter++;
					break;

				case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
					iter++;
					requiredExtensions.insert("EGL_EXT_create_context_robustness");
					iter++;
					break;

				default:
					DE_ASSERT(DE_FALSE);
			}
		}
	}

	for (std::set<string>::const_iterator reqExt = requiredExtensions.begin(); reqExt != requiredExtensions.end(); ++reqExt)
	{
		if (!de::contains(extensions.begin(), extensions.end(), *reqExt))
		{
			m_testCtx.getLog() << TestLog::Message << "Required extension '" << (*reqExt) << "' not supported" << TestLog::EndMessage;
			isOk = false;
		}
	}

	if (!isOk)
		TCU_THROW(NotSupportedError, "Required extensions not supported");
}

bool checkVersionString (TestLog& log, const glw::Functions& gl, bool desktop, int major, int minor)
{
	const char* const	versionStr	= (const char*)gl.getString(GL_VERSION);
	const char*			iter		= versionStr;

	int majorVersion = 0;
	int minorVersion = 0;

	// Check embedded version prefixes
	if (!desktop)
	{
		const char* prefix		= NULL;
		const char* prefixIter	= NULL;

		if (major == 1)
			prefix = "OpenGL ES-CM ";
		else
			prefix = "OpenGL ES ";

		prefixIter = prefix;

		while (*prefixIter)
		{
			if ((*prefixIter) != (*iter))
			{
				log << TestLog::Message << "Invalid version string prefix. Expected '" << prefix << "'." << TestLog::EndMessage;
				return false;
			}

			prefixIter++;
			iter++;
		}
	}

	while ((*iter) && (*iter) != '.')
	{
		const int val = (*iter) - '0';

		// Not a number
		if (val < 0 || val > 9)
		{
			log << TestLog::Message << "Failed to parse major version number. Not a number." << TestLog::EndMessage;
			return false;
		}

		// Leading zero
		if (majorVersion == 0 && val == 0)
		{
			log << TestLog::Message << "Failed to parse major version number. Begins with zero." << TestLog::EndMessage;
			return false;
		}

		majorVersion = majorVersion * 10 + val;

		iter++;
	}

	// Invalid format
	if ((*iter) != '.')
	{
		log << TestLog::Message << "Failed to parse version. Expected '.' after major version number." << TestLog::EndMessage;
		return false;
	}

	iter++;

	while ((*iter) && (*iter) != ' ' && (*iter) != '.')
	{
		const int val = (*iter) - '0';

		// Not a number
		if (val < 0 || val > 9)
		{
			log << TestLog::Message << "Failed to parse minor version number. Not a number." << TestLog::EndMessage;
			return false;
		}

		// Leading zero
		if (minorVersion == 0 && val == 0)
		{
			// Leading zeros in minor version
			if ((*(iter + 1)) != ' ' && (*(iter + 1)) != '.' && (*(iter + 1)) != '\0')
			{
				log << TestLog::Message << "Failed to parse minor version number. Leading zeros." << TestLog::EndMessage;
				return false;
			}
		}

		minorVersion = minorVersion * 10 + val;

		iter++;
	}

	// Invalid format
	if ((*iter) != ' ' && (*iter) != '.' && (*iter) != '\0')
		return false;

	if (desktop)
	{
		if (majorVersion < major)
		{
			log << TestLog::Message << "Major version is less than required." << TestLog::EndMessage;
			return false;
		}
		else if (majorVersion == major && minorVersion < minor)
		{
			log << TestLog::Message << "Minor version is less than required." << TestLog::EndMessage;
			return false;
		}
		else if (majorVersion == major && minorVersion == minor)
			return true;

		if (major < 3 || (major == 3 && minor == 0))
		{
			if (majorVersion == 3 && minorVersion == 1)
			{
				if (glu::hasExtension(gl, glu::ApiType::core(3, 1), "GL_ARB_compatibility"))
					return true;
				else
				{
					log << TestLog::Message << "Required OpenGL 3.0 or earlier. Got OpenGL 3.1 without GL_ARB_compatibility." << TestLog::EndMessage;
					return false;
				}
			}
			else if (majorVersion > 3 || (majorVersion == 3 && minorVersion >= minor))
			{
				deInt32 profile = 0;

				gl.getIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");

				if (profile == GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
					return true;
				else
				{
					log << TestLog::Message << "Required OpenGL 3.0 or earlier. Got later version without compatibility profile." << TestLog::EndMessage;
					return false;
				}
			}
			else
				DE_ASSERT(false);

			return false;
		}
		else if (major == 3 && minor == 1)
		{
			if (majorVersion > 3 || (majorVersion == 3 && minorVersion >= minor))
			{
				deInt32 profile = 0;

				gl.getIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");

				if (profile == GL_CONTEXT_CORE_PROFILE_BIT)
					return true;
				else
				{
					log << TestLog::Message << "Required OpenGL 3.1. Got later version without core profile." << TestLog::EndMessage;
					return false;
				}
			}
			else
				DE_ASSERT(false);

			return false;
		}
		else
		{
			log << TestLog::Message << "Couldn't do any further compatibilyt checks." << TestLog::EndMessage;
			return true;
		}
	}
	else
	{
		if (majorVersion < major)
		{
			log << TestLog::Message << "Major version is less than required." << TestLog::EndMessage;
			return false;
		}
		else if (majorVersion == major && minorVersion < minor)
		{
			log << TestLog::Message << "Minor version is less than required." << TestLog::EndMessage;
			return false;
		}
		else
			return true;
	}
}

bool checkVersionQueries (TestLog& log, const glw::Functions& gl, int major, int minor)
{
	deInt32 majorVersion = 0;
	deInt32	minorVersion = 0;

	gl.getIntegerv(GL_MAJOR_VERSION, &majorVersion);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");

	gl.getIntegerv(GL_MINOR_VERSION, &minorVersion);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");

	if (majorVersion < major || (majorVersion == major && minorVersion < minor))
	{
		if (majorVersion < major)
			log << TestLog::Message << "glGetIntegerv(GL_MAJOR_VERSION) returned '" << majorVersion << "' expected at least '" << major << "'" << TestLog::EndMessage;
		else if (majorVersion == major && minorVersion < minor)
			log << TestLog::Message << "glGetIntegerv(GL_MINOR_VERSION) returned '" << minorVersion << "' expected '" << minor << "'" << TestLog::EndMessage;
		else
			DE_ASSERT(false);

		return false;
	}
	else
		return true;
}

bool CreateContextExtCase::validateCurrentContext (const glw::Functions& gl)
{
	bool				isOk					= true;
	TestLog&			log						= m_testCtx.getLog();
	const EGLint*		iter					= &(m_attribList[0]);

	EGLint				majorVersion			= -1;
	EGLint				minorVersion			= -1;
	EGLint				contextFlags			= -1;
	EGLint				profileMask				= -1;
	EGLint				notificationStrategy	= -1;
	EGLint				robustAccessExt			= -1;
	EGLint				notificationStrategyExt	= -1;

	while ((*iter) != EGL_NONE)
	{
		switch (*iter)
		{
			case EGL_CONTEXT_MAJOR_VERSION_KHR:
				iter++;
				majorVersion = (*iter);
				iter++;
				break;

			case EGL_CONTEXT_MINOR_VERSION_KHR:
				iter++;
				minorVersion = (*iter);
				iter++;
				break;

			case EGL_CONTEXT_FLAGS_KHR:
				iter++;
				contextFlags = (*iter);
				iter++;
				break;

			case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
				iter++;
				profileMask = (*iter);
				iter++;
				break;

			case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
				iter++;
				notificationStrategy = (*iter);
				iter++;
				break;

			case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
				iter++;
				robustAccessExt = *iter;
				iter++;
				break;

			case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT:
				iter++;
				notificationStrategyExt = *iter;
				iter++;
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}
	}

	const string version = (const char*)gl.getString(GL_VERSION);

	log << TestLog::Message << "GL_VERSION: '" << version << "'" << TestLog::EndMessage;

	if (majorVersion == -1)
		majorVersion = 1;

	if (minorVersion == -1)
		minorVersion = 0;

	if (m_api == EGL_OPENGL_ES_API)
	{
		if (!checkVersionString(log, gl, false, majorVersion, minorVersion))
			isOk = false;

		if (majorVersion == 3)
		{
			if (!checkVersionQueries(log, gl, majorVersion, minorVersion))
				isOk = false;
		}
	}
	else if (m_api == EGL_OPENGL_API)
	{
		if (!checkVersionString(log, gl, true, majorVersion, minorVersion))
			isOk = false;

		if (majorVersion >= 3)
		{
			if (!checkVersionQueries(log, gl, majorVersion, minorVersion))
				isOk = false;
		}
	}
	else
		DE_ASSERT(false);


	if (contextFlags != -1)
	{
		if (m_api == EGL_OPENGL_API && (majorVersion > 3 || (majorVersion == 3 && minorVersion >= 1)))
		{
			deInt32 contextFlagsGL;

			DE_ASSERT(m_api == EGL_OPENGL_API);

			if (contextFlags == -1)
				contextFlags = 0;

			gl.getIntegerv(GL_CONTEXT_FLAGS, &contextFlagsGL);

			if (contextFlags != contextFlagsGL)
			{
				log << TestLog::Message << "Invalid GL_CONTEXT_FLAGS. Expected '" << eglContextFlagsToString(contextFlags) << "' got '" << eglContextFlagsToString(contextFlagsGL) << "'" << TestLog::EndMessage;
				isOk = false;
			}
		}
	}

	if (profileMask != -1 || (m_api == EGL_OPENGL_API && (majorVersion >= 3)))
	{
		if (profileMask == -1)
			profileMask = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR;

		DE_ASSERT(m_api == EGL_OPENGL_API);

		if (majorVersion < 3 || (majorVersion == 3 && minorVersion < 2))
		{
			// \note Ignore profile masks. This is not an error
		}
		else
		{
			deInt32 profileMaskGL = 0;

			gl.getIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMaskGL);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");

			if (profileMask != profileMaskGL)
			{
				log << TestLog::Message << "Invalid GL_CONTEXT_PROFILE_MASK. Expected '" << eglProfileMaskToString(profileMask) << "' got '" << eglProfileMaskToString(profileMaskGL) << "'" << TestLog::EndMessage;
				isOk = false;
			}
		}
	}

	DE_ASSERT(notificationStrategy == -1 || notificationStrategyExt == -1);

	if (notificationStrategy != -1 || notificationStrategyExt != -1)
	{
		const deInt32	expected	= notificationStrategy != -1 ? notificationStrategy : notificationStrategyExt;
		deInt32			strategy	= 0;

		gl.getIntegerv(GL_RESET_NOTIFICATION_STRATEGY, &strategy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");

		if (expected == EGL_NO_RESET_NOTIFICATION && strategy != GL_NO_RESET_NOTIFICATION)
		{
			log << TestLog::Message << "glGetIntegerv(GL_RESET_NOTIFICATION_STRATEGY) returned '" << strategy << "', expected 'GL_NO_RESET_NOTIFICATION'" << TestLog::EndMessage;
			isOk = false;
		}
		else if (expected == EGL_LOSE_CONTEXT_ON_RESET && strategy != GL_LOSE_CONTEXT_ON_RESET)
		{
			log << TestLog::Message << "glGetIntegerv(GL_RESET_NOTIFICATION_STRATEGY) returned '" << strategy << "', expected 'GL_LOSE_CONTEXT_ON_RESET'" << TestLog::EndMessage;
			isOk = false;
		}
	}

	if (robustAccessExt == EGL_TRUE)
	{
		if (m_api == EGL_OPENGL_API)
		{
			if (!glu::hasExtension(gl, glu::ApiType::core(majorVersion, minorVersion), "GL_ARB_robustness"))
			{
				log << TestLog::Message << "Created robustness context but it doesn't support GL_ARB_robustness." << TestLog::EndMessage;
				isOk = false;
			}
		}
		else if (m_api == EGL_OPENGL_ES_API)
		{
			if (!glu::hasExtension(gl, glu::ApiType::es(majorVersion, minorVersion), "GL_EXT_robustness"))
			{
				log << TestLog::Message << "Created robustness context but it doesn't support GL_EXT_robustness." << TestLog::EndMessage;
				isOk = false;
			}
		}

		if (m_api == EGL_OPENGL_API && (majorVersion > 3 || (majorVersion == 3 && minorVersion >= 1)))
		{
			deInt32 contextFlagsGL;

			DE_ASSERT(m_api == EGL_OPENGL_API);

			gl.getIntegerv(GL_CONTEXT_FLAGS, &contextFlagsGL);

			if ((contextFlagsGL & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT) != 0)
			{
				log << TestLog::Message << "Invalid GL_CONTEXT_FLAGS. GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT to be set, got '" << eglContextFlagsToString(contextFlagsGL) << "'" << TestLog::EndMessage;
				isOk = false;
			}
		}
		else if (m_api == EGL_OPENGL_ES_API)
		{
			deUint8 robustAccessGL;

			gl.getBooleanv(GL_CONTEXT_ROBUST_ACCESS, &robustAccessGL);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv()");

			if (robustAccessGL != GL_TRUE)
			{
				log << TestLog::Message << "Invalid GL_CONTEXT_ROBUST_ACCESS returned by glGetBooleanv(). Got '" << robustAccessGL << "' expected GL_TRUE." << TestLog::EndMessage;
				isOk = false;
			}
		}

	}

	return isOk;
}

TestCase::IterateResult CreateContextExtCase::iterate (void)
{
	if (m_iteration == 0)
	{
		logAttribList();
		checkRequiredExtensions();
	}

	if (m_iteration < (int)m_configs.size())
	{
		const Library&		egl				= m_eglTestCtx.getLibrary();
		const EGLConfig		config			= m_configs[m_iteration];
		const EGLint		surfaceTypes	= eglu::getConfigAttribInt(egl, m_display, config, EGL_SURFACE_TYPE);
		const EGLint		configId		= eglu::getConfigAttribInt(egl, m_display, config, EGL_CONFIG_ID);

		if ((surfaceTypes & EGL_PBUFFER_BIT) != 0)
		{
			tcu::ScopedLogSection	section			(m_testCtx.getLog(), ("EGLConfig ID: " + de::toString(configId) + " with PBuffer").c_str(), ("EGLConfig ID: " + de::toString(configId)).c_str());
			const EGLint			attribList[]	=
			{
				EGL_WIDTH,	64,
				EGL_HEIGHT,	64,
				EGL_NONE
			};
			eglu::UniqueSurface		surface			(egl, m_display, egl.createPbufferSurface(m_display, config, attribList));
			EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface");

			executeForSurface(config, *surface);
		}
		else if ((surfaceTypes & EGL_WINDOW_BIT) != 0)
		{
			const eglu::NativeWindowFactory&	factory	= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

			de::UniquePtr<eglu::NativeWindow>	window	(factory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_display, config, DE_NULL, eglu::WindowParams(256, 256, eglu::parseWindowVisibility(m_testCtx.getCommandLine()))));
			eglu::UniqueSurface					surface	(egl, m_display, eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *window, m_display, config, DE_NULL));

			executeForSurface(config, *surface);
		}
		else if ((surfaceTypes & EGL_PIXMAP_BIT) != 0)
		{
			const eglu::NativePixmapFactory&	factory	= eglu::selectNativePixmapFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

			de::UniquePtr<eglu::NativePixmap>	pixmap	(factory.createPixmap(&m_eglTestCtx.getNativeDisplay(), m_display, config, DE_NULL, 256, 256));
			eglu::UniqueSurface					surface	(egl, m_display, eglu::createPixmapSurface(m_eglTestCtx.getNativeDisplay(), *pixmap, m_display, config, DE_NULL));

			executeForSurface(config, *surface);
		}
		else // No supported surface type
			TCU_FAIL("Invalid or empty surface type bits");

		m_iteration++;
		return CONTINUE;
	}
	else
	{
		if (m_configs.size() == 0)
		{
			m_testCtx.getLog() << TestLog::Message << "No supported configs found" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "No supported configs found");
		}
		else if (m_isOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		return STOP;
	}
}

void CreateContextExtCase::executeForSurface (EGLConfig config, EGLSurface surface)
{
	const Library&	egl		= m_eglTestCtx.getLibrary();

	EGLU_CHECK_CALL(egl, bindAPI(m_api));

	try
	{
		glw::Functions		gl;
		eglu::UniqueContext	context	(egl, m_display, egl.createContext(m_display, config, EGL_NO_CONTEXT, &m_attribList[0]));
		EGLU_CHECK_MSG(egl, "eglCreateContext");

		EGLU_CHECK_CALL(egl, makeCurrent(m_display, surface, surface, *context));

		m_eglTestCtx.initGLFunctions(&gl, m_glContextType.getAPI());

		if (!validateCurrentContext(gl))
			m_isOk = false;
	}
	catch (const eglu::Error& error)
	{
		if (error.getError() == EGL_BAD_MATCH)
			m_testCtx.getLog() << TestLog::Message << "Context creation failed with error EGL_BAD_CONTEXT. Config doesn't support api version." << TestLog::EndMessage;
		else if (error.getError() == EGL_BAD_CONFIG)
			m_testCtx.getLog() << TestLog::Message << "Context creation failed with error EGL_BAD_MATCH. Context attribute compination not supported." << TestLog::EndMessage;
		else
		{
			m_testCtx.getLog() << TestLog::Message << "Context creation failed with error " << eglu::getErrorStr(error.getError()) << ". Error is not result of unsupported api etc." << TestLog::EndMessage;
			m_isOk = false;
		}
	}

	EGLU_CHECK_CALL(egl, makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

class CreateContextExtGroup : public TestCaseGroup
{
public:
						CreateContextExtGroup	(EglTestContext& eglTestCtx, EGLenum api, EGLint apiBit, const EGLint* attribList, const char* name, const char* description);
	virtual				~CreateContextExtGroup	(void);

	void				init					(void);

private:
	const EGLenum		m_api;
	const EGLint		m_apiBit;
	vector<EGLint>		m_attribList;
};

CreateContextExtGroup::CreateContextExtGroup (EglTestContext& eglTestCtx, EGLenum api, EGLint apiBit, const EGLint* attribList, const char* name, const char* description)
	: TestCaseGroup (eglTestCtx, name, description)
	, m_api			(api)
	, m_apiBit		(apiBit)
	, m_attribList	(attribList, attribList + getAttribListLength(attribList))
{
}

CreateContextExtGroup::~CreateContextExtGroup (void)
{
}


template <int Red, int Green, int Blue, int Alpha>
static bool colorBits (const eglu::CandidateConfig& c)
{
	return c.redSize()		== Red		&&
		   c.greenSize()	== Green	&&
		   c.blueSize()		== Blue		&&
		   c.alphaSize()	== Alpha;
}

static bool	hasDepth	(const eglu::CandidateConfig& c)	{ return c.depthSize() > 0;		}
static bool	noDepth		(const eglu::CandidateConfig& c)	{ return c.depthSize() == 0;	}
static bool	hasStencil	(const eglu::CandidateConfig& c)	{ return c.stencilSize() > 0;	}
static bool	noStencil	(const eglu::CandidateConfig& c)	{ return c.stencilSize() == 0;	}

template <deUint32 Type>
static bool renderable (const eglu::CandidateConfig& c)
{
	return (c.renderableType() & Type) == Type;
}

static eglu::ConfigFilter getRenderableFilter (deUint32 bits)
{
	switch (bits)
	{
		case EGL_OPENGL_ES2_BIT:	return renderable<EGL_OPENGL_ES2_BIT>;
		case EGL_OPENGL_ES3_BIT:	return renderable<EGL_OPENGL_ES3_BIT>;
		case EGL_OPENGL_BIT:		return renderable<EGL_OPENGL_BIT>;
		default:
			DE_ASSERT(false);
			return renderable<0>;
	}
}

void CreateContextExtGroup::init (void)
{
	const struct
	{
		const char*				name;
		const char*				description;

		eglu::ConfigFilter		colorFilter;
		eglu::ConfigFilter		depthFilter;
		eglu::ConfigFilter		stencilFilter;
	} groups[] =
	{
		{ "rgb565_no_depth_no_stencil",		"RGB565 configs without depth or stencil",		colorBits<5, 6, 5, 0>, noDepth,		noStencil	},
		{ "rgb565_no_depth_stencil",		"RGB565 configs with stencil and no depth",		colorBits<5, 6, 5, 0>, noDepth,		hasStencil	},
		{ "rgb565_depth_no_stencil",		"RGB565 configs with depth and no stencil",		colorBits<5, 6, 5, 0>, hasDepth,	noStencil	},
		{ "rgb565_depth_stencil",			"RGB565 configs with depth and stencil",		colorBits<5, 6, 5, 0>, hasDepth,	hasStencil	},

		{ "rgb888_no_depth_no_stencil",		"RGB888 configs without depth or stencil",		colorBits<8, 8, 8, 0>, noDepth,		noStencil	},
		{ "rgb888_no_depth_stencil",		"RGB888 configs with stencil and no depth",		colorBits<8, 8, 8, 0>, noDepth,		hasStencil	},
		{ "rgb888_depth_no_stencil",		"RGB888 configs with depth and no stencil",		colorBits<8, 8, 8, 0>, hasDepth,	noStencil	},
		{ "rgb888_depth_stencil",			"RGB888 configs with depth and stencil",		colorBits<8, 8, 8, 0>, hasDepth,	hasStencil	},

		{ "rgba4444_no_depth_no_stencil",	"RGBA4444 configs without depth or stencil",	colorBits<4, 4, 4, 4>, noDepth,		noStencil	},
		{ "rgba4444_no_depth_stencil",		"RGBA4444 configs with stencil and no depth",	colorBits<4, 4, 4, 4>, noDepth,		hasStencil	},
		{ "rgba4444_depth_no_stencil",		"RGBA4444 configs with depth and no stencil",	colorBits<4, 4, 4, 4>, hasDepth,	noStencil	},
		{ "rgba4444_depth_stencil",			"RGBA4444 configs with depth and stencil",		colorBits<4, 4, 4, 4>, hasDepth,	hasStencil	},

		{ "rgba5551_no_depth_no_stencil",	"RGBA5551 configs without depth or stencil",	colorBits<5, 5, 5, 1>, noDepth,		noStencil	},
		{ "rgba5551_no_depth_stencil",		"RGBA5551 configs with stencil and no depth",	colorBits<5, 5, 5, 1>, noDepth,		hasStencil	},
		{ "rgba5551_depth_no_stencil",		"RGBA5551 configs with depth and no stencil",	colorBits<5, 5, 5, 1>, hasDepth,	noStencil	},
		{ "rgba5551_depth_stencil",			"RGBA5551 configs with depth and stencil",		colorBits<5, 5, 5, 1>, hasDepth,	hasStencil	},

		{ "rgba8888_no_depth_no_stencil",	"RGBA8888 configs without depth or stencil",	colorBits<8, 8, 8, 8>, noDepth,		noStencil	},
		{ "rgba8888_no_depth_stencil",		"RGBA8888 configs with stencil and no depth",	colorBits<8, 8, 8, 8>, noDepth,		hasStencil	},
		{ "rgba8888_depth_no_stencil",		"RGBA8888 configs with depth and no stencil",	colorBits<8, 8, 8, 8>, hasDepth,	noStencil	},
		{ "rgba8888_depth_stencil",			"RGBA8888 configs with depth and stencil",		colorBits<8, 8, 8, 8>, hasDepth,	hasStencil	}
	};

	for (int groupNdx = 0; groupNdx < DE_LENGTH_OF_ARRAY(groups); groupNdx++)
	{
		eglu::FilterList filter;

		filter << groups[groupNdx].colorFilter
			   << groups[groupNdx].depthFilter
			   << groups[groupNdx].stencilFilter
			   << getRenderableFilter(m_apiBit);

		addChild(new CreateContextExtCase(m_eglTestCtx, m_api, &(m_attribList[0]), filter, groups[groupNdx].name, groups[groupNdx].description));
	}
	// \todo [mika] Add other group
}

} // anonymous

CreateContextExtTests::CreateContextExtTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "create_context_ext", "EGL_KHR_create_context tests.")
{
}

CreateContextExtTests::~CreateContextExtTests (void)
{
}

void CreateContextExtTests::init (void)
{
	const size_t	maxAttributeCount = 10;
	const struct
	{
		const char*	name;
		const char*	description;
		EGLenum		api;
		EGLint		apiBit;
		EGLint		attribList[maxAttributeCount];
	} groupList[] =
	{
#if 0
		// \todo [mika] Not supported by glw
		// OpenGL ES 1.x
		{ "gles_10", "Create OpenGL ES 1.0 context", EGL_OPENGL_ES_API, EGL_OPENGL_ES_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 1, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE} },
		{ "gles_11", "Create OpenGL ES 1.1 context", EGL_OPENGL_ES_API, EGL_OPENGL_ES_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 1, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE } },
#endif
		// OpenGL ES 2.x
		{ "gles_20", "Create OpenGL ES 2.0 context", EGL_OPENGL_ES_API, EGL_OPENGL_ES2_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 2, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE } },
		// OpenGL ES 3.x
		{ "gles_30", "Create OpenGL ES 3.0 context", EGL_OPENGL_ES_API, EGL_OPENGL_ES3_BIT_KHR,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE} },
#if 0
		// \todo [mika] Not supported by glw
		// \note [mika] Should we really test 1.x?
		{ "gl_10", "Create OpenGL 1.0 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 1, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE} },
		{ "gl_11", "Create OpenGL 1.1 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 1, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE } },

		// OpenGL 2.x
		{ "gl_20", "Create OpenGL 2.0 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 2, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE } },
		{ "gl_21", "Create OpenGL 2.1 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 2, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE } },
#endif
		// OpenGL 3.x
		{ "gl_30", "Create OpenGL 3.0 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE } },
		{ "robust_gl_30", "Create robust OpenGL 3.0 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },
		{ "gl_31", "Create OpenGL 3.1 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE } },
		{ "robust_gl_31", "Create robust OpenGL 3.1 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },
		{ "gl_32", "Create OpenGL 3.2 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 2, EGL_NONE } },
		{ "robust_gl_32", "Create robust OpenGL 3.2 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 2, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },
		{ "gl_33", "Create OpenGL 3.3 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 3, EGL_NONE } },
		{ "robust_gl_33", "Create robust OpenGL 3.3 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 3, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },

		// OpenGL 4.x
		{ "gl_40", "Create OpenGL 4.0 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE } },
		{ "robust_gl_40", "Create robust OpenGL 4.0 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },
		{ "gl_41", "Create OpenGL 4.1 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE } },
		{ "robust_gl_41", "Create robust OpenGL 4.1 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },
		{ "gl_42", "Create OpenGL 4.2 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 2, EGL_NONE } },
		{ "robust_gl_42", "Create robust OpenGL 4.2 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 2, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },
		{ "gl_43", "Create OpenGL 4.3 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 3, EGL_NONE } },
		{ "robust_gl_43", "Create robust OpenGL 4.3 context", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 3, EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR, EGL_NONE } },

		// Robust contexts with EGL_EXT_create_context_robustness
		{ "robust_gles_2_ext", "Create robust OpenGL ES 2.0 context with EGL_EXT_create_context_robustness.", EGL_OPENGL_ES_API, EGL_OPENGL_ES2_BIT,
			{ EGL_CONTEXT_CLIENT_VERSION, 2, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT, EGL_TRUE, EGL_NONE } },
		{ "robust_gles_3_ext", "Create robust OpenGL ES 3.0 context with EGL_EXT_create_context_robustness.", EGL_OPENGL_ES_API, EGL_OPENGL_ES3_BIT_KHR,
			{ EGL_CONTEXT_CLIENT_VERSION, 3, EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT, EGL_TRUE, EGL_NONE } },
#if 0
	// glu/glw doesn't support any version of OpenGL and EGL doesn't allow use of EGL_CONTEXT_CLIENT_VERSION with OpenGL and doesn't define which OpenGL version should be returned.
		{ "robust_gl_ext", "Create robust OpenGL context with EGL_EXT_create_context_robustness.", EGL_OPENGL_API, EGL_OPENGL_BIT,
			{ EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT, EGL_TRUE, EGL_NONE } }
#endif
	};

	for (int groupNdx = 0; groupNdx < DE_LENGTH_OF_ARRAY(groupList); groupNdx++)
		addChild(new CreateContextExtGroup(m_eglTestCtx, groupList[groupNdx].api, groupList[groupNdx].apiBit, groupList[groupNdx].attribList, groupList[groupNdx].name, groupList[groupNdx].description));
}

} // egl
} // deqp
