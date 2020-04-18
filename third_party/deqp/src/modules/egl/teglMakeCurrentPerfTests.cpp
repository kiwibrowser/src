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
 * \brief eglMakeCurrent performance tests.
 *//*--------------------------------------------------------------------*/

#include "teglMakeCurrentPerfTests.hpp"

#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"
#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "tcuTestLog.hpp"

#include "deRandom.hpp"
#include "deStringUtil.hpp"

#include "deClock.h"
#include "deString.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using std::ostringstream;
using std::string;
using std::vector;

using tcu::TestLog;

using namespace eglw;

namespace deqp
{
namespace egl
{

class MakeCurrentPerfCase : public TestCase
{
public:
	enum SurfaceType
	{
		SURFACETYPE_PBUFFER	= (1<<0),
		SURFACETYPE_WINDOW	= (1<<1),
		SURFACETYPE_PIXMAP	= (1<<2)
	};

	struct Spec
	{
		SurfaceType	surfaceTypes;
		int			contextCount;
		int			surfaceCount;

		bool		release;

		int			iterationCount;
		int			sampleCount;

		string		toName			(void) const;
		string		toDescription	(void) const;
	};
					MakeCurrentPerfCase		(EglTestContext& eglTestCtx, const Spec& spec, const char* name, const char* description);
					~MakeCurrentPerfCase	(void);

	void			init					(void);
	void			deinit					(void);
	IterateResult	iterate					(void);

private:
	Spec						m_spec;
	de::Random					m_rnd;

	EGLDisplay					m_display;
	EGLConfig					m_config;
	vector<EGLContext>			m_contexts;
	vector<EGLSurface>			m_surfaces;

	vector<eglu::NativeWindow*>	m_windows;
	vector<eglu::NativePixmap*>	m_pixmaps;

	vector<deUint64>			m_samples;

	void					chooseConfig	(void);
	void					createSurfaces	(void);
	void					createContexts	(void);

	void					destroySurfaces	(void);
	void					destroyContexts	(void);

	void					createPBuffer	(void);
	void					createWindow	(void);
	void					createPixmap	(void);

	void					logTestInfo		(void);
	void					logResults		(void);
	// Disabled
							MakeCurrentPerfCase	(const MakeCurrentPerfCase&);
	MakeCurrentPerfCase&	operator=			(const MakeCurrentPerfCase&);
};

string MakeCurrentPerfCase::Spec::toName (void) const
{
	ostringstream name;

	name << "context";

	if (contextCount > 1)
		name << "s_" << contextCount;

	if ((surfaceTypes & SURFACETYPE_WINDOW) != 0)
		name << "_window" << (surfaceCount > 1 ? "s" : "");

	if ((surfaceTypes & SURFACETYPE_PIXMAP) != 0)
		name << "_pixmap" << (surfaceCount > 1 ? "s" : "");

	if ((surfaceTypes & SURFACETYPE_PBUFFER) != 0)
		name << "_pbuffer" << (surfaceCount > 1 ? "s" : "");

	if (surfaceCount > 1)
		name << "_" << surfaceCount;

	if (release)
		name << "_release";

	return name.str();
}

string MakeCurrentPerfCase::Spec::toDescription (void) const
{
	// \todo [mika] Generate descrpition
	return toName();
}

MakeCurrentPerfCase::MakeCurrentPerfCase (EglTestContext& eglTestCtx, const Spec& spec, const char* name, const char* description)
	: TestCase		(eglTestCtx, tcu::NODETYPE_PERFORMANCE, name, description)
	, m_spec		(spec)
	, m_rnd			(deStringHash(name))
	, m_display		(EGL_NO_DISPLAY)
	, m_config		(DE_NULL)
{
}

MakeCurrentPerfCase::~MakeCurrentPerfCase (void)
{
	deinit();
}

void MakeCurrentPerfCase::init (void)
{
	m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

	chooseConfig();
	createContexts();
	createSurfaces();
}

void MakeCurrentPerfCase::deinit (void)
{
	destroyContexts();
	destroySurfaces();

	if (m_display != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

void MakeCurrentPerfCase::chooseConfig (void)
{
	const EGLint	surfaceBits	= ((m_spec.surfaceTypes & SURFACETYPE_WINDOW) != 0 ? EGL_WINDOW_BIT : 0)
									| ((m_spec.surfaceTypes & SURFACETYPE_PIXMAP) != 0 ? EGL_PIXMAP_BIT : 0)
									| ((m_spec.surfaceTypes & SURFACETYPE_PBUFFER) != 0 ? EGL_PBUFFER_BIT : 0);

	const EGLint	attribList[] = {
		EGL_SURFACE_TYPE,		surfaceBits,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	const Library&	egl			= m_eglTestCtx.getLibrary();
	EGLint			configCount = 0;

	EGLU_CHECK_CALL(egl, chooseConfig(m_display, attribList, &m_config, 1, &configCount));

	if (configCount <= 0)
		throw tcu::NotSupportedError("No compatible configs found");
}

void MakeCurrentPerfCase::createSurfaces (void)
{
	vector<SurfaceType> types;

	if ((m_spec.surfaceTypes & SURFACETYPE_WINDOW) != 0)
		types.push_back(SURFACETYPE_WINDOW);

	if ((m_spec.surfaceTypes & SURFACETYPE_PIXMAP) != 0)
		types.push_back(SURFACETYPE_PIXMAP);

	if ((m_spec.surfaceTypes & SURFACETYPE_PBUFFER) != 0)
		types.push_back(SURFACETYPE_PBUFFER);

	DE_ASSERT((int)types.size() <= m_spec.surfaceCount);

	// Create surfaces
	for (int surfaceNdx = 0; surfaceNdx < m_spec.surfaceCount; surfaceNdx++)
	{
		SurfaceType type = types[surfaceNdx % types.size()];

		switch (type)
		{
			case SURFACETYPE_PBUFFER:
				createPBuffer();
				break;

			case SURFACETYPE_WINDOW:
				createWindow();
				break;

			case SURFACETYPE_PIXMAP:
				createPixmap();
				break;

			default:
				DE_ASSERT(false);
		};
	}
}

void MakeCurrentPerfCase::createPBuffer (void)
{
	const Library&	egl		= m_eglTestCtx.getLibrary();
	const EGLint	width	= 256;
	const EGLint	height	= 256;

	const EGLint attribList[] = {
		EGL_WIDTH,	width,
		EGL_HEIGHT, height,
		EGL_NONE
	};

	EGLSurface	surface = egl.createPbufferSurface(m_display, m_config, attribList);

	EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface()");

	m_surfaces.push_back(surface);
}

void MakeCurrentPerfCase::createWindow (void)
{
	const Library&						egl				= m_eglTestCtx.getLibrary();
	const EGLint						width			= 256;
	const EGLint						height			= 256;

	const eglu::NativeWindowFactory&	windowFactory	= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

	eglu::NativeWindow*					window			= DE_NULL;
	EGLSurface							surface			= EGL_NO_SURFACE;

	try
	{
		window	= windowFactory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_display, m_config, DE_NULL, eglu::WindowParams(width, height, eglu::parseWindowVisibility(m_eglTestCtx.getTestContext().getCommandLine())));
		surface	= eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *window, m_display, m_config, DE_NULL);
	}
	catch (...)
	{
		if (surface != EGL_NO_SURFACE)
			egl.destroySurface(m_display, surface);

		delete window;
		throw;
	}

	m_windows.push_back(window);
	m_surfaces.push_back(surface);
}

void MakeCurrentPerfCase::createPixmap (void)
{
	const Library&						egl				= m_eglTestCtx.getLibrary();
	const EGLint						width			= 256;
	const EGLint						height			= 256;

	const eglu::NativePixmapFactory&	pixmapFactory	= eglu::selectNativePixmapFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

	eglu::NativePixmap*					pixmap			= DE_NULL;
	EGLSurface							surface			= EGL_NO_SURFACE;

	try
	{
		pixmap	= pixmapFactory.createPixmap(&m_eglTestCtx.getNativeDisplay(), m_display, m_config, DE_NULL, width, height);
		surface	= eglu::createPixmapSurface(m_eglTestCtx.getNativeDisplay(), *pixmap, m_display, m_config, DE_NULL);
	}
	catch (...)
	{
		if (surface != EGL_NO_SURFACE)
			egl.destroySurface(m_display, surface);

		delete pixmap;
		throw;
	}

	m_pixmaps.push_back(pixmap);
	m_surfaces.push_back(surface);
}

void MakeCurrentPerfCase::destroySurfaces (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (m_surfaces.size() > 0)
	{
		EGLDisplay display = m_display;

		// Destroy surfaces
		for (vector<EGLSurface>::iterator iter = m_surfaces.begin(); iter != m_surfaces.end(); ++iter)
		{
			if (*iter != EGL_NO_SURFACE)
				EGLU_CHECK_CALL(egl, destroySurface(display, *iter));
			*iter = EGL_NO_SURFACE;
		}

		m_surfaces.clear();

		// Destroy pixmaps
		for (vector<eglu::NativePixmap*>::iterator iter = m_pixmaps.begin(); iter != m_pixmaps.end(); ++iter)
		{
			delete *iter;
			*iter = NULL;
		}

		m_pixmaps.clear();

		// Destroy windows
		for (vector<eglu::NativeWindow*>::iterator iter = m_windows.begin(); iter != m_windows.end(); ++iter)
		{
			delete *iter;
			*iter = NULL;
		}

		m_windows.clear();

		// Clear all surface handles
		m_surfaces.clear();
	}
}

void MakeCurrentPerfCase::createContexts (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	for (int contextNdx = 0; contextNdx < m_spec.contextCount; contextNdx++)
	{
		const EGLint attribList[] = {
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
		};

		EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));
		EGLContext context = egl.createContext(m_display, m_config, EGL_NO_CONTEXT, attribList);
		EGLU_CHECK_MSG(egl, "eglCreateContext()");

		m_contexts.push_back(context);
	}
}

void MakeCurrentPerfCase::destroyContexts (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();
	if (m_contexts.size() > 0)
	{
		EGLDisplay display = m_display;

		for (vector<EGLContext>::iterator iter = m_contexts.begin(); iter != m_contexts.end(); ++iter)
		{
			if (*iter != EGL_NO_CONTEXT)
				EGLU_CHECK_CALL(egl, destroyContext(display, *iter));
			*iter = EGL_NO_CONTEXT;
		}

		m_contexts.clear();
	}
}

void MakeCurrentPerfCase::logTestInfo (void)
{
	TestLog& log = m_testCtx.getLog();

	{
		tcu::ScopedLogSection	section(log, "Test Info", "Test case information.");

		log << TestLog::Message << "Context count: "	<< m_contexts.size()											<< TestLog::EndMessage;
		log << TestLog::Message << "Surfaces count: "	<< m_surfaces.size()											<< TestLog::EndMessage;
		log << TestLog::Message << "Sample count: "	<< m_spec.sampleCount												<< TestLog::EndMessage;
		log << TestLog::Message << "Iteration count: "	<< m_spec.iterationCount										<< TestLog::EndMessage;
		log << TestLog::Message << "Window count: "	<< m_windows.size()													<< TestLog::EndMessage;
		log << TestLog::Message << "Pixmap count: "	<< m_pixmaps.size()													<< TestLog::EndMessage;
		log << TestLog::Message << "PBuffer count: "	<< (m_surfaces.size() - m_windows.size() - m_pixmaps.size())	<< TestLog::EndMessage;

		if (m_spec.release)
			log << TestLog::Message << "Context is released after each use. Both binding and releasing context are included in result time." << TestLog::EndMessage;
	}
}

void MakeCurrentPerfCase::logResults (void)
{
	TestLog& log = m_testCtx.getLog();

	log << TestLog::SampleList("Result", "Result")
		<< TestLog::SampleInfo << TestLog::ValueInfo("Time", "Time", "us", QP_SAMPLE_VALUE_TAG_RESPONSE)
		<< TestLog::EndSampleInfo;

	for (int sampleNdx = 0; sampleNdx < (int)m_samples.size(); sampleNdx++)
		log << TestLog::Sample << deInt64(m_samples[sampleNdx]) << TestLog::EndSample;

	log << TestLog::EndSampleList;

	// Log stats
	{
		deUint64	totalTimeUs				= 0;
		deUint64	totalIterationCount		= 0;

		float		iterationTimeMeanUs		= 0.0f;
		float		iterationTimeMedianUs	= 0.0f;
		float		iterationTimeVarianceUs	= 0.0f;
		float		iterationTimeSkewnessUs	= 0.0f;
		float		iterationTimeMinUs		= std::numeric_limits<float>::max();
		float		iterationTimeMaxUs		= 0.0f;

		std::sort(m_samples.begin(), m_samples.end());

		// Calculate totals
		for (int sampleNdx = 0; sampleNdx < (int)m_samples.size(); sampleNdx++)
		{
			totalTimeUs			+= m_samples[sampleNdx];
			totalIterationCount	+= m_spec.iterationCount;
		}

		// Calculate mean and median
		iterationTimeMeanUs		= ((float)(((double)totalTimeUs) / (double)totalIterationCount));
		iterationTimeMedianUs	= ((float)(((double)m_samples[m_samples.size() / 2]) / (double)m_spec.iterationCount));

		// Calculate variance
		for (int sampleNdx = 0; sampleNdx < (int)m_samples.size(); sampleNdx++)
		{
			float iterationTimeUs	= (float)(((double)m_samples[sampleNdx]) / m_spec.iterationCount);
			iterationTimeVarianceUs	+= std::pow(iterationTimeUs - iterationTimeMedianUs, 2.0f);
		}

		// Calculate min and max
		for (int sampleNdx = 0; sampleNdx < (int)m_samples.size(); sampleNdx++)
		{
			float iterationTimeUs	= (float)(((double)m_samples[sampleNdx]) / m_spec.iterationCount);
			iterationTimeMinUs		= std::min<float>(iterationTimeMinUs, iterationTimeUs);
			iterationTimeMaxUs		= std::max<float>(iterationTimeMaxUs, iterationTimeUs);
		}

		iterationTimeVarianceUs /= (float)m_samples.size();

		// Calculate skewness
		for (int sampleNdx = 0; sampleNdx < (int)m_samples.size(); sampleNdx++)
		{
			float iterationTimeUs	= (float)(((double)m_samples[sampleNdx]) / m_spec.iterationCount);
			iterationTimeSkewnessUs	= std::pow((iterationTimeUs - iterationTimeMedianUs) / iterationTimeVarianceUs, 2.0f);
		}

		iterationTimeSkewnessUs /= (float)m_samples.size();

		{
			tcu::ScopedLogSection	section(log, "Result", "Statistics from results.");

			log << TestLog::Message << "Total time: "	<< totalTimeUs				<< "us" << TestLog::EndMessage;
			log << TestLog::Message << "Mean: "			<< iterationTimeMeanUs		<< "us" << TestLog::EndMessage;
			log << TestLog::Message << "Median: "		<< iterationTimeMedianUs	<< "us" << TestLog::EndMessage;
			log << TestLog::Message << "Variance: "		<< iterationTimeVarianceUs	<< "us" << TestLog::EndMessage;
			log << TestLog::Message << "Skewness: "		<< iterationTimeSkewnessUs	<< "us" << TestLog::EndMessage;
			log << TestLog::Message << "Min: "			<< iterationTimeMinUs		<< "us" << TestLog::EndMessage;
			log << TestLog::Message << "Max: "			<< iterationTimeMaxUs		<< "us" << TestLog::EndMessage;
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, de::floatToString((float)(((double)totalTimeUs)/(double)totalIterationCount), 2).c_str());
	}
}

TestCase::IterateResult MakeCurrentPerfCase::iterate (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();
	if (m_samples.size() == 0)
		logTestInfo();

	{
		EGLDisplay	display		= m_display;
		deUint64	beginTimeUs	= deGetMicroseconds();

		for (int iteration = 0; iteration < m_spec.iterationCount; iteration++)
		{
			EGLContext	context = m_contexts[m_rnd.getUint32() % m_contexts.size()];
			EGLSurface	surface	= m_surfaces[m_rnd.getUint32() % m_surfaces.size()];

			egl.makeCurrent(display, surface, surface, context);

			if (m_spec.release)
				egl.makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		}

		m_samples.push_back(deGetMicroseconds() - beginTimeUs);

		egl.makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		EGLU_CHECK_MSG(egl, "eglMakeCurrent()");
	}

	if ((int)m_samples.size() == m_spec.sampleCount)
	{
		logResults();
		return STOP;
	}
	else
		return CONTINUE;
}

MakeCurrentPerfTests::MakeCurrentPerfTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "make_current", "eglMakeCurrent performance tests")
{
}

void MakeCurrentPerfTests::init (void)
{
	const int iterationCount	= 100;
	const int sampleCount		= 100;

	// Add simple test group
	{
		TestCaseGroup* simple = new TestCaseGroup(m_eglTestCtx, "simple", "Simple eglMakeCurrent performance tests using single context and surface");

		const MakeCurrentPerfCase::SurfaceType types[] = {
			MakeCurrentPerfCase::SURFACETYPE_PBUFFER,
			MakeCurrentPerfCase::SURFACETYPE_PIXMAP,
			MakeCurrentPerfCase::SURFACETYPE_WINDOW
		};

		for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(types); typeNdx++)
		{
			for (int releaseNdx = 0; releaseNdx < 2; releaseNdx++)
			{
				MakeCurrentPerfCase::Spec spec;

				spec.surfaceTypes	= types[typeNdx];
				spec.contextCount	= 1;
				spec.surfaceCount	= 1;
				spec.release		= (releaseNdx == 1);
				spec.iterationCount	= iterationCount;
				spec.sampleCount	= sampleCount;

				simple->addChild(new MakeCurrentPerfCase(m_eglTestCtx, spec, spec.toName().c_str(), spec.toDescription().c_str()));
			}
		}

		addChild(simple);
	}

	// Add multi context test group
	{
		TestCaseGroup* multiContext = new TestCaseGroup(m_eglTestCtx, "multi_context", "eglMakeCurrent performance tests using multiple contexts and single surface");

		const MakeCurrentPerfCase::SurfaceType types[] = {
			MakeCurrentPerfCase::SURFACETYPE_PBUFFER,
			MakeCurrentPerfCase::SURFACETYPE_PIXMAP,
			MakeCurrentPerfCase::SURFACETYPE_WINDOW
		};

		const int contextCounts[] = {
			10, 100
		};

		for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(contextCounts); contextCountNdx++)
		{
			for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(types); typeNdx++)
			{
				for (int releaseNdx = 0; releaseNdx < 2; releaseNdx++)
				{
					MakeCurrentPerfCase::Spec spec;

					spec.surfaceTypes	= types[typeNdx];
					spec.contextCount	= contextCounts[contextCountNdx];
					spec.surfaceCount	= 1;
					spec.release		= (releaseNdx == 1);
					spec.iterationCount	= iterationCount;
					spec.sampleCount	= sampleCount;

					multiContext->addChild(new MakeCurrentPerfCase(m_eglTestCtx, spec, spec.toName().c_str(), spec.toDescription().c_str()));
				}
			}
		}

		addChild(multiContext);
	}

	// Add multi surface test group
	{
		TestCaseGroup* multiSurface = new TestCaseGroup(m_eglTestCtx, "multi_surface", "eglMakeCurrent performance tests using single context and multiple surfaces");

		const MakeCurrentPerfCase::SurfaceType types[] = {
			MakeCurrentPerfCase::SURFACETYPE_PBUFFER,
			MakeCurrentPerfCase::SURFACETYPE_PIXMAP,
			MakeCurrentPerfCase::SURFACETYPE_WINDOW,

			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PBUFFER	|MakeCurrentPerfCase::SURFACETYPE_PIXMAP),
			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PBUFFER	|MakeCurrentPerfCase::SURFACETYPE_WINDOW),
			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PIXMAP	|MakeCurrentPerfCase::SURFACETYPE_WINDOW),

			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PBUFFER|MakeCurrentPerfCase::SURFACETYPE_PIXMAP|MakeCurrentPerfCase::SURFACETYPE_WINDOW)
		};

		const int surfaceCounts[] = {
			10, 100
		};

		for (int surfaceCountNdx = 0; surfaceCountNdx < DE_LENGTH_OF_ARRAY(surfaceCounts); surfaceCountNdx++)
		{
			for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(types); typeNdx++)
			{
				for (int releaseNdx = 0; releaseNdx < 2; releaseNdx++)
				{
					MakeCurrentPerfCase::Spec spec;

					spec.surfaceTypes	= types[typeNdx];
					spec.surfaceCount	= surfaceCounts[surfaceCountNdx];
					spec.contextCount	= 1;
					spec.release		= (releaseNdx == 1);
					spec.iterationCount	= iterationCount;
					spec.sampleCount	= sampleCount;

					multiSurface->addChild(new MakeCurrentPerfCase(m_eglTestCtx, spec, spec.toName().c_str(), spec.toDescription().c_str()));
				}
			}
		}

		addChild(multiSurface);
	}

	// Add Complex? test group
	{
		TestCaseGroup* multi = new TestCaseGroup(m_eglTestCtx, "complex", "eglMakeCurrent performance tests using multiple contexts and multiple surfaces");

		const MakeCurrentPerfCase::SurfaceType types[] = {
			MakeCurrentPerfCase::SURFACETYPE_PBUFFER,
			MakeCurrentPerfCase::SURFACETYPE_PIXMAP,
			MakeCurrentPerfCase::SURFACETYPE_WINDOW,

			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PBUFFER	|MakeCurrentPerfCase::SURFACETYPE_PIXMAP),
			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PBUFFER	|MakeCurrentPerfCase::SURFACETYPE_WINDOW),
			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PIXMAP	|MakeCurrentPerfCase::SURFACETYPE_WINDOW),

			(MakeCurrentPerfCase::SurfaceType)(MakeCurrentPerfCase::SURFACETYPE_PBUFFER|MakeCurrentPerfCase::SURFACETYPE_PIXMAP|MakeCurrentPerfCase::SURFACETYPE_WINDOW)
		};

		const int surfaceCounts[] = {
			10, 100
		};


		const int contextCounts[] = {
			10, 100
		};

		for (int surfaceCountNdx = 0; surfaceCountNdx < DE_LENGTH_OF_ARRAY(surfaceCounts); surfaceCountNdx++)
		{
			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(contextCounts); contextCountNdx++)
			{
				for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(types); typeNdx++)
				{
					for (int releaseNdx = 0; releaseNdx < 2; releaseNdx++)
					{
						MakeCurrentPerfCase::Spec spec;

						spec.surfaceTypes	= types[typeNdx];
						spec.contextCount	= contextCounts[contextCountNdx];
						spec.surfaceCount	= surfaceCounts[surfaceCountNdx];
						spec.release		= (releaseNdx == 1);
						spec.iterationCount	= iterationCount;
						spec.sampleCount	= sampleCount;

						multi->addChild(new MakeCurrentPerfCase(m_eglTestCtx, spec, spec.toName().c_str(), spec.toDescription().c_str()));
					}
				}
			}
		}

		addChild(multi);
	}
}

} // egl
} // deqp
