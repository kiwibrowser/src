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
 * \brief Tests for resizing the native window of a surface.
 *//*--------------------------------------------------------------------*/

#include "teglResizeTests.hpp"

#include "teglSimpleConfigCase.hpp"

#include "tcuImageCompare.hpp"
#include "tcuSurface.hpp"
#include "tcuPlatform.hpp"
#include "tcuTestLog.hpp"
#include "tcuInterval.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuResultCollector.hpp"

#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"
#include "egluUnique.hpp"
#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "tcuTestLog.hpp"
#include "tcuVector.hpp"

#include "deThread.h"
#include "deUniquePtr.hpp"

#include <sstream>

namespace deqp
{
namespace egl
{

using std::vector;
using std::string;
using std::ostringstream;
using de::MovePtr;
using tcu::CommandLine;
using tcu::ConstPixelBufferAccess;
using tcu::Interval;
using tcu::IVec2;
using tcu::Vec3;
using tcu::Vec4;
using tcu::UVec4;
using tcu::ResultCollector;
using tcu::Surface;
using tcu::TestLog;
using eglu::AttribMap;
using eglu::NativeDisplay;
using eglu::NativeWindow;
using eglu::ScopedCurrentContext;
using eglu::UniqueSurface;
using eglu::UniqueContext;
using eglu::NativeWindowFactory;
using eglu::WindowParams;
using namespace eglw;

typedef	eglu::WindowParams::Visibility	Visibility;
typedef	TestCase::IterateResult			IterateResult;

struct ResizeParams
{
	string	name;
	string	description;
	IVec2	oldSize;
	IVec2	newSize;
};

class ResizeTest : public TestCase
{
public:
								ResizeTest	(EglTestContext&		eglTestCtx,
											 const ResizeParams&	params)
									: TestCase	(eglTestCtx,
												 params.name.c_str(),
												 params.description.c_str())
									, m_oldSize	(params.oldSize)
									, m_newSize	(params.newSize)
									, m_display	(EGL_NO_DISPLAY)
									, m_config	(DE_NULL)
									, m_log		(m_testCtx.getLog())
									, m_status	(m_log) {}

	void						init		(void);
	void						deinit		(void);

protected:
	virtual EGLenum				surfaceType	(void) const { return EGL_WINDOW_BIT; }
	void						resize		(IVec2 size);

	const IVec2					m_oldSize;
	const IVec2					m_newSize;
	EGLDisplay					m_display;
	EGLConfig					m_config;
	MovePtr<NativeWindow>		m_nativeWindow;
	MovePtr<UniqueSurface>		m_surface;
	MovePtr<UniqueContext>		m_context;
	TestLog&					m_log;
	ResultCollector				m_status;
	glw::Functions				m_gl;
};

EGLConfig getEGLConfig (const Library& egl, const EGLDisplay eglDisplay, EGLenum surfaceType)
{
	AttribMap attribMap;

	attribMap[EGL_SURFACE_TYPE]		= surfaceType;
	attribMap[EGL_RENDERABLE_TYPE]	= EGL_OPENGL_ES2_BIT;

	return eglu::chooseSingleConfig(egl, eglDisplay, attribMap);
}

void ResizeTest::init (void)
{
	TestCase::init();

	const Library&				egl				= m_eglTestCtx.getLibrary();
	const CommandLine&			cmdLine			= m_testCtx.getCommandLine();
	const EGLDisplay			eglDisplay		= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	const EGLConfig				eglConfig		= getEGLConfig(egl, eglDisplay, surfaceType());
	const EGLint				ctxAttribs[]	=
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLContext					eglContext		= egl.createContext(eglDisplay,
																   eglConfig,
																   EGL_NO_CONTEXT,
																   ctxAttribs);
	EGLU_CHECK_MSG(egl, "eglCreateContext()");
	MovePtr<UniqueContext>		context			(new UniqueContext(egl, eglDisplay, eglContext));
	const EGLint				configId		= eglu::getConfigAttribInt(egl,
																		   eglDisplay,
																		   eglConfig,
																		   EGL_CONFIG_ID);
	const Visibility			visibility		= eglu::parseWindowVisibility(cmdLine);
	NativeDisplay&				nativeDisplay	= m_eglTestCtx.getNativeDisplay();
	const NativeWindowFactory&	windowFactory	= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(),
																				  cmdLine);

	const WindowParams			windowParams	(m_oldSize.x(), m_oldSize.y(), visibility);
	MovePtr<NativeWindow>		nativeWindow	(windowFactory.createWindow(&nativeDisplay,
																			 eglDisplay,
																			 eglConfig,
																			 DE_NULL,
																			 windowParams));
	const EGLSurface			eglSurface		= eglu::createWindowSurface(nativeDisplay,
																			*nativeWindow,
																			eglDisplay,
																			eglConfig,
																			DE_NULL);
	MovePtr<UniqueSurface>		surface			(new UniqueSurface(egl, eglDisplay, eglSurface));

	m_log << TestLog::Message
		  << "Chose EGLConfig with id " << configId << ".\n"
		  << "Created initial surface with size " << m_oldSize
		  << TestLog::EndMessage;

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2, 0));
	m_config		= eglConfig;
	m_surface		= surface;
	m_context		= context;
	m_display		= eglDisplay;
	m_nativeWindow	= nativeWindow;
	EGLU_CHECK_MSG(egl, "init");
}

void ResizeTest::deinit (void)
{
	if (m_display != EGL_NO_DISPLAY)
		m_eglTestCtx.getLibrary().terminate(m_display);

	m_config		= DE_NULL;
	m_display		= EGL_NO_DISPLAY;
	m_context.clear();
	m_surface.clear();
	m_nativeWindow.clear();
}

void ResizeTest::resize (IVec2 size)
{
	m_nativeWindow->setSurfaceSize(size);
	m_testCtx.getPlatform().processEvents();
	m_log << TestLog::Message
		  << "Resized surface to size " << size
		  << TestLog::EndMessage;
}

class ChangeSurfaceSizeCase : public ResizeTest
{
public:
					ChangeSurfaceSizeCase	(EglTestContext&		eglTestCtx,
											 const ResizeParams&	params)
						: ResizeTest(eglTestCtx, params) {}

	IterateResult	iterate					(void);
};

void drawRectangle (const glw::Functions& gl, IVec2 pos, IVec2 size, Vec3 color)
{
	gl.clearColor(color.x(), color.y(), color.z(), 1.0);
	gl.scissor(pos.x(), pos.y(), size.x(), size.y());
	gl.enable(GL_SCISSOR_TEST);
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.disable(GL_SCISSOR_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Rectangle drawing with glScissor and glClear failed.");
}

void initSurface (const glw::Functions& gl, IVec2 oldSize)
{
	const Vec3	frameColor	(0.0f, 0.0f, 1.0f);
	const Vec3	fillColor	(1.0f, 0.0f, 0.0f);
	const Vec3	markColor	(0.0f, 1.0f, 0.0f);

	drawRectangle(gl, IVec2(0, 0), oldSize, frameColor);
	drawRectangle(gl, IVec2(2, 2), oldSize - IVec2(4, 4), fillColor);

	drawRectangle(gl, IVec2(0, 0), IVec2(8, 4), markColor);
	drawRectangle(gl, oldSize - IVec2(16, 16), IVec2(8, 4), markColor);
	drawRectangle(gl, IVec2(0, oldSize.y() - 16), IVec2(8, 4), markColor);
	drawRectangle(gl, IVec2(oldSize.x() - 16, 0), IVec2(8, 4), markColor);
}

bool compareRectangles (const ConstPixelBufferAccess& rectA,
						const ConstPixelBufferAccess& rectB)
{
	const int width		= rectA.getWidth();
	const int height	= rectA.getHeight();
	const int depth		= rectA.getDepth();

	if (rectB.getWidth() != width || rectB.getHeight() != height || rectB.getDepth() != depth)
		return false;

	for (int z = 0; z < depth; ++z)
		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				if (rectA.getPixel(x, y, z) != rectB.getPixel(x, y, z))
					return false;

	return true;
}

// Check whether `oldSurface` and `newSurface` share a common corner.
bool compareCorners (const Surface& oldSurface, const Surface& newSurface)
{
	const int	oldWidth	= oldSurface.getWidth();
	const int	oldHeight	= oldSurface.getHeight();
	const int	newWidth	= newSurface.getWidth();
	const int	newHeight	= newSurface.getHeight();
	const int	minWidth	= de::min(oldWidth, newWidth);
	const int	minHeight	= de::min(oldHeight, newHeight);

	for (int xCorner = 0; xCorner < 2; ++xCorner)
	{
		const int oldX = xCorner == 0 ? 0 : oldWidth - minWidth;
		const int newX = xCorner == 0 ? 0 : newWidth - minWidth;

		for (int yCorner = 0; yCorner < 2; ++yCorner)
		{
			const int				oldY		= yCorner == 0 ? 0 : oldHeight - minHeight;
			const int				newY		= yCorner == 0 ? 0 : newHeight - minHeight;
			ConstPixelBufferAccess	oldAccess	=
				getSubregion(oldSurface.getAccess(), oldX, oldY, minWidth, minHeight);
			ConstPixelBufferAccess	newAccess	=
				getSubregion(newSurface.getAccess(), newX, newY, minWidth, minHeight);

			if (compareRectangles(oldAccess, newAccess))
				return true;
		}
	}

	return false;
}

Surface readSurface (const glw::Functions& gl, IVec2 size)
{
	Surface ret (size.x(), size.y());
	gl.readPixels(0, 0, size.x(), size.y(), GL_RGBA, GL_UNSIGNED_BYTE,
				  ret.getAccess().getDataPtr());

	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed");
	return ret;
}

template <typename T>
inline bool hasBits (T bitSet, T requiredBits)
{
	return (bitSet & requiredBits) == requiredBits;
}

IVec2 getNativeSurfaceSize (const NativeWindow& nativeWindow,
							IVec2				reqSize)
{
	if (hasBits(nativeWindow.getCapabilities(), NativeWindow::CAPABILITY_GET_SURFACE_SIZE))
		return nativeWindow.getSurfaceSize();
	return reqSize; // assume we got the requested size
}

IVec2 checkSurfaceSize (const Library&		egl,
						EGLDisplay			eglDisplay,
						EGLSurface			eglSurface,
						const NativeWindow&	nativeWindow,
						IVec2				reqSize,
						ResultCollector&	status)
{
	const IVec2		nativeSize	= getNativeSurfaceSize(nativeWindow, reqSize);
	IVec2			eglSize		= eglu::getSurfaceSize(egl, eglDisplay, eglSurface);
	ostringstream	oss;

	oss << "Size of EGL surface " << eglSize
		<< " differs from size of native window " << nativeSize;
	status.check(eglSize == nativeSize, oss.str());

	return eglSize;
}

IterateResult ChangeSurfaceSizeCase::iterate (void)
{
	const Library&			egl			= m_eglTestCtx.getLibrary();
	ScopedCurrentContext	currentCtx	(egl, m_display, **m_surface, **m_surface, **m_context);
	IVec2					oldEglSize	= checkSurfaceSize(egl,
														   m_display,
														   **m_surface,
														   *m_nativeWindow,
														   m_oldSize,
														   m_status);

	initSurface(m_gl, oldEglSize);

	this->resize(m_newSize);

	egl.swapBuffers(m_display, **m_surface);
	EGLU_CHECK_MSG(egl, "eglSwapBuffers()");
	checkSurfaceSize(egl, m_display, **m_surface, *m_nativeWindow, m_newSize, m_status);

	m_status.setTestContextResult(m_testCtx);
	return STOP;
}

class PreserveBackBufferCase : public ResizeTest
{
public:
					PreserveBackBufferCase	(EglTestContext&		eglTestCtx,
											 const ResizeParams&	params)
						: ResizeTest(eglTestCtx, params) {}

	IterateResult	iterate					(void);
	EGLenum			surfaceType				(void) const;
};

EGLenum PreserveBackBufferCase::surfaceType (void) const
{
	return EGL_WINDOW_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
}

IterateResult PreserveBackBufferCase::iterate (void)
{
	const Library&			egl			= m_eglTestCtx.getLibrary();
	ScopedCurrentContext	currentCtx	(egl, m_display, **m_surface, **m_surface, **m_context);

	EGLU_CHECK_CALL(egl, surfaceAttrib(m_display, **m_surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED));

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "GL state erroneous upon initialization!");

	{
		const IVec2 oldEglSize = eglu::getSurfaceSize(egl, m_display, **m_surface);
		initSurface(m_gl, oldEglSize);

		m_gl.finish();
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glFinish() failed");

		{
			const Surface oldSurface = readSurface(m_gl, oldEglSize);

			egl.swapBuffers(m_display, **m_surface);
			this->resize(m_newSize);
			egl.swapBuffers(m_display, **m_surface);
			EGLU_CHECK_MSG(egl, "eglSwapBuffers()");

			{
				const IVec2		newEglSize	= eglu::getSurfaceSize(egl, m_display, **m_surface);
				const Surface	newSurface	= readSurface(m_gl, newEglSize);

				m_log << TestLog::ImageSet("Corner comparison",
										   "Comparing old and new surfaces at all corners")
					  << TestLog::Image("Before resizing", "Before resizing", oldSurface)
					  << TestLog::Image("After resizing", "After resizing", newSurface)
					  << TestLog::EndImageSet;

				m_status.checkResult(compareCorners(oldSurface, newSurface),
									 QP_TEST_RESULT_QUALITY_WARNING,
									 "Resizing the native window changed the contents of "
									 "the EGL surface");
			}
		}
	}

	m_status.setTestContextResult(m_testCtx);
	return STOP;
}

typedef tcu::Vector<Interval, 2> IvVec2;

class UpdateResolutionCase : public ResizeTest
{
public:
					UpdateResolutionCase	(EglTestContext&		eglTestCtx,
											 const ResizeParams&	params)
						: ResizeTest(eglTestCtx, params) {}

	IterateResult	iterate					(void);

private:
	IvVec2			getNativePixelsPerInch	(void);
};

IvVec2 ivVec2 (const IVec2& vec)
{
	return IvVec2(double(vec.x()), double(vec.y()));
}

Interval approximateInt (int i)
{
	const Interval margin(-1.0, 1.0); // The resolution may be rounded

	return (Interval(i) + margin) & Interval(0.0, TCU_INFINITY);
}

IvVec2 UpdateResolutionCase::getNativePixelsPerInch	(void)
{
	const Library&	egl			= m_eglTestCtx.getLibrary();
	const int		inchPer10km	= 254 * EGL_DISPLAY_SCALING;
	const IVec2		bufSize		= eglu::getSurfaceSize(egl, m_display, **m_surface);
	const IVec2		winSize		= m_nativeWindow->getScreenSize();
	const IVec2		bufPp10km	= eglu::getSurfaceResolution(egl, m_display, **m_surface);
	const IvVec2	bufPpiI		= (IvVec2(approximateInt(bufPp10km.x()),
										  approximateInt(bufPp10km.y()))
								   / Interval(inchPer10km));
	const IvVec2	winPpiI		= ivVec2(winSize) * bufPpiI / ivVec2(bufSize);
	const IVec2		winPpi		(int(winPpiI.x().midpoint()), int(winPpiI.y().midpoint()));

	m_log << TestLog::Message
		  << "EGL surface size: "							<< bufSize		<< "\n"
		  << "EGL surface pixel density (pixels / 10 km): "	<< bufPp10km	<< "\n"
		  << "Native window size: "							<< winSize		<< "\n"
		  << "Native pixel density (ppi): "					<< winPpi		<< "\n"
		  << TestLog::EndMessage;

	m_status.checkResult(bufPp10km.x() >= 1 && bufPp10km.y() >= 1,
						 QP_TEST_RESULT_QUALITY_WARNING,
						 "Surface pixel density is less than one pixel per 10 km. "
						 "Is the surface really visible from space?");

	return winPpiI;
}

IterateResult UpdateResolutionCase::iterate (void)
{
	const Library&			egl			= m_eglTestCtx.getLibrary();
	ScopedCurrentContext	currentCtx	(egl, m_display, **m_surface, **m_surface, **m_context);

	if (!hasBits(m_nativeWindow->getCapabilities(),
				 NativeWindow::CAPABILITY_GET_SCREEN_SIZE))
		TCU_THROW(NotSupportedError, "Unable to determine surface size in screen pixels");

	{
		const IVec2 oldEglSize = eglu::getSurfaceSize(egl, m_display, **m_surface);
		initSurface(m_gl, oldEglSize);
	}
	{
		const IvVec2 oldPpi = this->getNativePixelsPerInch();
		this->resize(m_newSize);
		EGLU_CHECK_CALL(egl, swapBuffers(m_display, **m_surface));
		{
			const IvVec2 newPpi = this->getNativePixelsPerInch();
			m_status.check(oldPpi.x().intersects(newPpi.x()) &&
						   oldPpi.y().intersects(newPpi.y()),
						   "Window PPI differs after resizing");
		}
	}

	m_status.setTestContextResult(m_testCtx);
	return STOP;
}

ResizeTests::ResizeTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "resize", "Tests resizing the native surface")
{
}

template <class Case>
TestCaseGroup* createCaseGroup(EglTestContext&	eglTestCtx,
							   const string&	name,
							   const string&	desc)
{
	const ResizeParams		params[]	=
	{
		{ "shrink",			"Shrink in both dimensions",
		  IVec2(128, 128),	IVec2(32, 32) },
		{ "grow",			"Grow in both dimensions",
		  IVec2(32, 32),	IVec2(128, 128) },
		{ "stretch_width",	"Grow horizontally, shrink vertically",
		  IVec2(32, 128),	IVec2(128, 32) },
		{ "stretch_height",	"Grow vertically, shrink horizontally",
		  IVec2(128, 32),	IVec2(32, 128) },
	};
	TestCaseGroup* const	group		=
		new TestCaseGroup(eglTestCtx, name.c_str(), desc.c_str());

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(params); ++ndx)
		group->addChild(new Case(eglTestCtx, params[ndx]));

	return group;
}

void ResizeTests::init (void)
{
	addChild(createCaseGroup<ChangeSurfaceSizeCase>(m_eglTestCtx,
													"surface_size",
													"EGL surface size update"));
	addChild(createCaseGroup<PreserveBackBufferCase>(m_eglTestCtx,
													 "back_buffer",
													 "Back buffer contents"));
	addChild(createCaseGroup<UpdateResolutionCase>(m_eglTestCtx,
												   "pixel_density",
												   "Pixel density"));
}

} // egl
} // deqp
