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
 * \brief Color clear case.
 *//*--------------------------------------------------------------------*/

#include "teglColorClearCase.hpp"
#include "tcuTestLog.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "egluUtil.hpp"
#include "deRandom.hpp"
#include "deString.h"
#include "tcuImageCompare.hpp"
#include "tcuVector.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuPixelFormat.hpp"
#include "glwFunctions.hpp"
#include "deThread.hpp"
#include "deSemaphore.hpp"
#include "deSharedPtr.hpp"
#include "teglGLES1RenderUtil.hpp"
#include "teglGLES2RenderUtil.hpp"
#include "teglVGRenderUtil.hpp"

#include <memory>
#include <iterator>

namespace deqp
{
namespace egl
{

using tcu::TestLog;
using tcu::RGBA;
using std::vector;
using namespace eglw;

// Utilities.

struct ClearOp
{
	ClearOp (int x_, int y_, int width_, int height_, const tcu::RGBA& color_)
		: x			(x_)
		, y			(y_)
		, width		(width_)
		, height	(height_)
		, color		(color_)
	{
	}

	ClearOp (void)
		: x			(0)
		, y			(0)
		, width		(0)
		, height	(0)
		, color		(0)
	{
	}

	int			x;
	int			y;
	int			width;
	int			height;
	tcu::RGBA	color;
};

struct ApiFunctions
{
	glw::Functions	gl;
};

static ClearOp computeRandomClear (de::Random& rnd, int width, int height)
{
	int			w		= rnd.getInt(1, width);
	int			h		= rnd.getInt(1, height);
	int			x		= rnd.getInt(0, width-w);
	int			y		= rnd.getInt(0, height-h);
	tcu::RGBA	col		(rnd.getUint32());

	return ClearOp(x, y, w, h, col);
}

static void renderReference (tcu::Surface& dst, const vector<ClearOp>& clears, const tcu::PixelFormat& pixelFormat)
{
	for (vector<ClearOp>::const_iterator clearIter = clears.begin(); clearIter != clears.end(); clearIter++)
	{
		tcu::PixelBufferAccess access = tcu::getSubregion(dst.getAccess(), clearIter->x, clearIter->y, 0, clearIter->width, clearIter->height, 1);
		tcu::clear(access, pixelFormat.convertColor(clearIter->color).toIVec());
	}
}

static void renderClear (EGLint api, const ApiFunctions& func, const ClearOp& clear)
{
	switch (api)
	{
		case EGL_OPENGL_ES_BIT:			gles1::clear(clear.x, clear.y, clear.width, clear.height, clear.color.toVec());				break;
		case EGL_OPENGL_ES2_BIT:		gles2::clear(func.gl, clear.x, clear.y, clear.width, clear.height, clear.color.toVec());	break;
		case EGL_OPENGL_ES3_BIT_KHR:	gles2::clear(func.gl, clear.x, clear.y, clear.width, clear.height, clear.color.toVec());	break;
		case EGL_OPENVG_BIT:			vg::clear	(clear.x, clear.y, clear.width, clear.height, clear.color.toVec());				break;
		default:
			DE_ASSERT(DE_FALSE);
	}
}

static void finish (EGLint api, const ApiFunctions& func)
{
	switch (api)
	{
		case EGL_OPENGL_ES_BIT:			gles1::finish();		break;
		case EGL_OPENGL_ES2_BIT:		gles2::finish(func.gl);	break;
		case EGL_OPENGL_ES3_BIT_KHR:	gles2::finish(func.gl);	break;
		case EGL_OPENVG_BIT:			vg::finish();			break;
		default:
			DE_ASSERT(DE_FALSE);
	}
}

static void readPixels (EGLint api, const ApiFunctions& func, tcu::Surface& dst)
{
	switch (api)
	{
		case EGL_OPENGL_ES_BIT:			gles1::readPixels	(dst, 0, 0, dst.getWidth(), dst.getHeight());			break;
		case EGL_OPENGL_ES2_BIT:		gles2::readPixels	(func.gl, dst, 0, 0, dst.getWidth(), dst.getHeight());	break;
		case EGL_OPENGL_ES3_BIT_KHR:	gles2::readPixels	(func.gl, dst, 0, 0, dst.getWidth(), dst.getHeight());	break;
		case EGL_OPENVG_BIT:			vg::readPixels		(dst, 0, 0, dst.getWidth(), dst.getHeight());			break;
		default:
			DE_ASSERT(DE_FALSE);
	}
}

static tcu::PixelFormat getPixelFormat (const Library& egl, EGLDisplay display, EGLConfig config)
{
	tcu::PixelFormat pixelFmt;

	egl.getConfigAttrib(display, config, EGL_RED_SIZE,		&pixelFmt.redBits);
	egl.getConfigAttrib(display, config, EGL_GREEN_SIZE,	&pixelFmt.greenBits);
	egl.getConfigAttrib(display, config, EGL_BLUE_SIZE,		&pixelFmt.blueBits);
	egl.getConfigAttrib(display, config, EGL_ALPHA_SIZE,	&pixelFmt.alphaBits);

	return pixelFmt;
}

// SingleThreadColorClearCase

SingleThreadColorClearCase::SingleThreadColorClearCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint api, EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi)
	: MultiContextRenderCase(eglTestCtx, name, description, api, surfaceType, filters, numContextsPerApi)
{
}

void SingleThreadColorClearCase::executeForContexts (EGLDisplay display, EGLSurface surface, const Config& config, const std::vector<std::pair<EGLint, EGLContext> >& contexts)
{
	const Library&		egl			= m_eglTestCtx.getLibrary();

	const tcu::IVec2	surfaceSize	= eglu::getSurfaceSize(egl, display, surface);
	const int			width		= surfaceSize.x();
	const int			height		= surfaceSize.y();

	TestLog&			log			= m_testCtx.getLog();

	tcu::Surface		refFrame	(width, height);
	tcu::Surface		frame		(width, height);
	tcu::PixelFormat	pixelFmt	= getPixelFormat(egl, display, config.config);

	de::Random			rnd			(deStringHash(getName()));
	vector<ClearOp>		clears;
	const int			ctxClears	= 2;
	const int			numIters	= 3;

	ApiFunctions		funcs;

	m_eglTestCtx.initGLFunctions(&funcs.gl, glu::ApiType::es(2,0));

	// Clear to black using first context.
	{
		EGLint		api			= contexts[0].first;
		EGLContext	context		= contexts[0].second;
		ClearOp		clear		(0, 0, width, height, RGBA::black());

		egl.makeCurrent(display, surface, surface, context);
		EGLU_CHECK_MSG(egl, "eglMakeCurrent");

		renderClear(api, funcs, clear);
		finish(api, funcs);
		clears.push_back(clear);
	}

	// Render.
	for (int iterNdx = 0; iterNdx < numIters; iterNdx++)
	{
		for (vector<std::pair<EGLint, EGLContext> >::const_iterator ctxIter = contexts.begin(); ctxIter != contexts.end(); ctxIter++)
		{
			EGLint		api			= ctxIter->first;
			EGLContext	context		= ctxIter->second;

			egl.makeCurrent(display, surface, surface, context);
			EGLU_CHECK_MSG(egl, "eglMakeCurrent");

			for (int clearNdx = 0; clearNdx < ctxClears; clearNdx++)
			{
				ClearOp clear = computeRandomClear(rnd, width, height);

				renderClear(api, funcs, clear);
				clears.push_back(clear);
			}

			finish(api, funcs);
		}
	}

	// Read pixels using first context. \todo [pyry] Randomize?
	{
		EGLint		api		= contexts[0].first;
		EGLContext	context	= contexts[0].second;

		egl.makeCurrent(display, surface, surface, context);
		EGLU_CHECK_MSG(egl, "eglMakeCurrent");

		readPixels(api, funcs, frame);
	}

	egl.makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");

	// Render reference.
	renderReference(refFrame, clears, pixelFmt);

	// Compare images
	{
		tcu::RGBA eps = pixelFmt.alphaBits == 1 ? RGBA(1,1,1,127) : RGBA(1,1,1,1);
		bool imagesOk = tcu::pixelThresholdCompare(log, "ComparisonResult", "Image comparison result", refFrame, frame, eps + pixelFmt.getColorThreshold(), tcu::COMPARE_LOG_RESULT);

		if (!imagesOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
	}
}

// MultiThreadColorClearCase

enum
{
	NUM_CLEARS_PER_PACKET	= 2 //!< Number of clears performed in one context activation in one thread.
};

class ColorClearThread;

typedef de::SharedPtr<ColorClearThread>	ColorClearThreadSp;
typedef de::SharedPtr<de::Semaphore>	SemaphoreSp;

struct ClearPacket
{
	ClearPacket (void)
	{
	}

	ClearOp			clears[NUM_CLEARS_PER_PACKET];
	SemaphoreSp		wait;
	SemaphoreSp		signal;
};

class ColorClearThread : public de::Thread
{
public:
	ColorClearThread (const Library& egl, EGLDisplay display, EGLSurface surface, EGLContext context, EGLint api, const ApiFunctions& funcs, const std::vector<ClearPacket>& packets)
		: m_egl		(egl)
		, m_display	(display)
		, m_surface	(surface)
		, m_context	(context)
		, m_api		(api)
		, m_funcs	(funcs)
		, m_packets	(packets)
	{
	}

	void run (void)
	{
		for (std::vector<ClearPacket>::const_iterator packetIter = m_packets.begin(); packetIter != m_packets.end(); packetIter++)
		{
			// Wait until it is our turn.
			packetIter->wait->decrement();

			// Acquire context.
			m_egl.makeCurrent(m_display, m_surface, m_surface, m_context);

			// Execute clears.
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(packetIter->clears); ndx++)
				renderClear(m_api, m_funcs, packetIter->clears[ndx]);

			finish(m_api, m_funcs);
			// Release context.
			m_egl.makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

			// Signal completion.
			packetIter->signal->increment();
		}
		m_egl.releaseThread();
	}

private:
	const Library&					m_egl;
	EGLDisplay						m_display;
	EGLSurface						m_surface;
	EGLContext						m_context;
	EGLint							m_api;
	const ApiFunctions&				m_funcs;
	const std::vector<ClearPacket>&	m_packets;
};

MultiThreadColorClearCase::MultiThreadColorClearCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint api, EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi)
	: MultiContextRenderCase(eglTestCtx, name, description, api, surfaceType, filters, numContextsPerApi)
{
}

void MultiThreadColorClearCase::executeForContexts (EGLDisplay display, EGLSurface surface, const Config& config, const std::vector<std::pair<EGLint, EGLContext> >& contexts)
{
	const Library&		egl			= m_eglTestCtx.getLibrary();

	const tcu::IVec2	surfaceSize	= eglu::getSurfaceSize(egl, display, surface);
	const int			width		= surfaceSize.x();
	const int			height		= surfaceSize.y();

	TestLog&			log			= m_testCtx.getLog();

	tcu::Surface		refFrame	(width, height);
	tcu::Surface		frame		(width, height);
	tcu::PixelFormat	pixelFmt	= getPixelFormat(egl, display, config.config);

	de::Random			rnd			(deStringHash(getName()));

	ApiFunctions		funcs;

	m_eglTestCtx.initGLFunctions(&funcs.gl, glu::ApiType::es(2,0));

	// Create clear packets.
	const int						numPacketsPerThread		= 2;
	int								numThreads				= (int)contexts.size();
	int								numPackets				= numThreads * numPacketsPerThread;

	vector<SemaphoreSp>				semaphores				(numPackets+1);
	vector<vector<ClearPacket> >	packets					(numThreads);
	vector<ColorClearThreadSp>		threads					(numThreads);

	// Initialize semaphores.
	for (vector<SemaphoreSp>::iterator sem = semaphores.begin(); sem != semaphores.end(); ++sem)
		*sem = SemaphoreSp(new de::Semaphore(0));

	// Create packets.
	for (int threadNdx = 0; threadNdx < numThreads; threadNdx++)
	{
		packets[threadNdx].resize(numPacketsPerThread);

		for (int packetNdx = 0; packetNdx < numPacketsPerThread; packetNdx++)
		{
			ClearPacket& packet = packets[threadNdx][packetNdx];

			// Threads take turns with packets.
			packet.wait		= semaphores[packetNdx*numThreads + threadNdx];
			packet.signal	= semaphores[packetNdx*numThreads + threadNdx + 1];

			for (int clearNdx = 0; clearNdx < DE_LENGTH_OF_ARRAY(packet.clears); clearNdx++)
			{
				// First clear is always full-screen black.
				if (threadNdx == 0 && packetNdx == 0 && clearNdx == 0)
					packet.clears[clearNdx] = ClearOp(0, 0, width, height, RGBA::black());
				else
					packet.clears[clearNdx] = computeRandomClear(rnd, width, height);
			}
		}
	}

	// Create and launch threads (actual rendering starts once first semaphore is signaled).
	for (int threadNdx = 0; threadNdx < numThreads; threadNdx++)
	{
		threads[threadNdx] = ColorClearThreadSp(new ColorClearThread(egl, display, surface, contexts[threadNdx].second, contexts[threadNdx].first, funcs, packets[threadNdx]));
		threads[threadNdx]->start();
	}

	// Signal start and wait until complete.
	semaphores.front()->increment();
	semaphores.back()->decrement();

	// Read pixels using first context. \todo [pyry] Randomize?
	{
		EGLint		api		= contexts[0].first;
		EGLContext	context	= contexts[0].second;

		egl.makeCurrent(display, surface, surface, context);
		EGLU_CHECK_MSG(egl, "eglMakeCurrent");

		readPixels(api, funcs, frame);
	}

	egl.makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");

	// Join threads.
	for (int threadNdx = 0; threadNdx < numThreads; threadNdx++)
		threads[threadNdx]->join();

	// Render reference.
	for (int packetNdx = 0; packetNdx < numPacketsPerThread; packetNdx++)
	{
		for (int threadNdx = 0; threadNdx < numThreads; threadNdx++)
		{
			const ClearPacket& packet = packets[threadNdx][packetNdx];
			for (int clearNdx = 0; clearNdx < DE_LENGTH_OF_ARRAY(packet.clears); clearNdx++)
			{
				tcu::PixelBufferAccess access = tcu::getSubregion(refFrame.getAccess(),
																  packet.clears[clearNdx].x, packet.clears[clearNdx].y, 0,
																  packet.clears[clearNdx].width, packet.clears[clearNdx].height, 1);
				tcu::clear(access, pixelFmt.convertColor(packet.clears[clearNdx].color).toIVec());
			}
		}
	}

	// Compare images
	{
		tcu::RGBA eps = pixelFmt.alphaBits == 1 ? RGBA(1,1,1,127) : RGBA(1,1,1,1);
		bool imagesOk = tcu::pixelThresholdCompare(log, "ComparisonResult", "Image comparison result", refFrame, frame, eps + pixelFmt.getColorThreshold(), tcu::COMPARE_LOG_RESULT);

		if (!imagesOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
	}
}

} // egl
} // deqp
