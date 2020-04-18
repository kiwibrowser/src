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
 * \brief EGL gles2 sharing threaded tests
 *//*--------------------------------------------------------------------*/

#include "teglGLES2SharingThreadedTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuThreadUtil.hpp"

#include "deRandom.hpp"
#include "deThread.hpp"
#include "deSharedPtr.hpp"
#include "deMutex.hpp"
#include "deSemaphore.hpp"
#include "deStringUtil.hpp"

#include "deClock.h"
#include "deString.h"
#include "deMemory.h"
#include "deMath.h"

#include "gluDefs.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include <vector>
#include <string>
#include <memory>
#include <sstream>

using std::vector;
using std::string;
using de::SharedPtr;

using namespace glw;
using namespace eglw;

namespace deqp
{
namespace egl
{

namespace GLES2ThreadTest
{

class Texture;
class Buffer;
class Shader;
class Program;
class GLES2ResourceManager
{
public:

	SharedPtr<Texture>			popTexture			(int index);
	const SharedPtr<Texture>	getTexture			(int index) const { return m_textures[index]; }
	void						addTexture			(SharedPtr<Texture> texture) { m_textures.push_back(texture); }
	int							getTextureCount		(void) const { return (int)m_textures.size(); }

	SharedPtr<Buffer>			popBuffer			(int index);
	const SharedPtr<Buffer>		getBuffer			(int index) const { return m_buffers[index]; }
	void						addBuffer			(SharedPtr<Buffer> buffer) { m_buffers.push_back(buffer); }
	int							getBufferCount		(void) const { return (int)m_buffers.size(); }

	SharedPtr<Shader>			popShader			(int index);
	const SharedPtr<Shader>		getShader			(int index) const { return m_shaders[index]; }
	void						addShader			(SharedPtr<Shader> shader) { m_shaders.push_back(shader); }
	int							getShaderCount		(void) const { return (int)m_shaders.size(); }

	SharedPtr<Program>			popProgram			(int index);
	const SharedPtr<Program>	getProgram			(int index) const { return m_programs[index]; }
	void						addProgram			(SharedPtr<Program> program) { m_programs.push_back(program); }
	int							getProgramCount		(void) const { return (int)m_programs.size(); }

private:
	std::vector<SharedPtr<Texture> >	m_textures;
	std::vector<SharedPtr<Buffer> >		m_buffers;
	std::vector<SharedPtr<Shader> >		m_shaders;
	std::vector<SharedPtr<Program> >	m_programs;
};

SharedPtr<Texture> GLES2ResourceManager::popTexture (int index)
{
	SharedPtr<Texture> texture = m_textures[index];

	m_textures.erase(m_textures.begin() + index);

	return texture;
}

SharedPtr<Buffer> GLES2ResourceManager::popBuffer (int index)
{
	SharedPtr<Buffer> buffer = m_buffers[index];

	m_buffers.erase(m_buffers.begin() + index);

	return buffer;
}

SharedPtr<Shader> GLES2ResourceManager::popShader (int index)
{
	SharedPtr<Shader> shader = m_shaders[index];

	m_shaders.erase(m_shaders.begin() + index);

	return shader;
}

SharedPtr<Program> GLES2ResourceManager::popProgram (int index)
{
	SharedPtr<Program> program = m_programs[index];

	m_programs.erase(m_programs.begin() + index);

	return program;
}

class GLES2Context : public tcu::ThreadUtil::Object
{
public:
				GLES2Context		(SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<GLES2ResourceManager> resourceManager);
				~GLES2Context		(void);

	// Call generation time attributes
	SharedPtr<GLES2ResourceManager>	resourceManager;

	// Run time attributes
	EGLDisplay		display;
	EGLContext		context;

	struct
	{
		glEGLImageTargetTexture2DOESFunc	imageTargetTexture2D;
	} glExtensions;
private:
					GLES2Context		(const GLES2Context&);
	GLES2Context&	operator=			(const GLES2Context&);
};

GLES2Context::GLES2Context (SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<GLES2ResourceManager> resourceManager_)
	: tcu::ThreadUtil::Object	("Context", event)
	, resourceManager			(resourceManager_)
	, display					(EGL_NO_DISPLAY)
	, context					(EGL_NO_CONTEXT)
{
	glExtensions.imageTargetTexture2D = DE_NULL;
}

GLES2Context::~GLES2Context (void)
{
}

class Surface : public tcu::ThreadUtil::Object
{
public:
				Surface		(SharedPtr<tcu::ThreadUtil::Event> event);
				~Surface	(void);

	// Run time attributes
	EGLSurface	surface;

private:
	Surface		(const Surface&);
	Surface&	operator=	(const Surface&);
};

Surface::Surface (SharedPtr<tcu::ThreadUtil::Event> event)
	: tcu::ThreadUtil::Object	("Surface", event)
	, surface					(EGL_NO_SURFACE)
{
}

Surface::~Surface (void)
{
}

// EGL thread with thread specifig state
class EGLThread : public tcu::ThreadUtil::Thread
{
public:
								EGLThread	(const Library& egl_, const glw::Functions& gl_, int seed);
								~EGLThread	(void);
	virtual void				deinit		(void);

	const Library&				egl;
	const glw::Functions&		gl;

	// Generation time attributes
	SharedPtr<GLES2Context>		context;
	SharedPtr<Surface>			surface;

	// Runtime attributes

	SharedPtr<GLES2Context>		runtimeContext;
	EGLSurface					eglSurface;
private:
};

EGLThread::EGLThread (const Library& egl_, const glw::Functions& gl_, int seed)
	: tcu::ThreadUtil::Thread	(seed)
	, egl						(egl_)
	, gl						(gl_)
	, eglSurface				(EGL_NO_SURFACE)
{
}

void EGLThread::deinit (void)
{
	if (runtimeContext)
	{
		if (runtimeContext->context != EGL_NO_CONTEXT)
			egl.makeCurrent(runtimeContext->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		egl.destroyContext(runtimeContext->display, runtimeContext->context);
		runtimeContext->context = EGL_NO_CONTEXT;

		egl.destroySurface(runtimeContext->display, eglSurface);
		eglSurface	= EGL_NO_SURFACE;
	}

	egl.releaseThread();
}

EGLThread::~EGLThread (void)
{
	EGLThread::deinit();
}

class FenceSync
{
public:
				FenceSync	(void);
				~FenceSync	(void);

	void		init		(EGLThread& thread, bool serverSync);
	bool		waitReady	(EGLThread& thread);

	void		addWaiter	(void);

private:
	EGLDisplay	m_display;
	EGLSyncKHR	m_sync;
	de::Mutex	m_lock;
	int			m_waiterCount;
	bool		m_serverSync;
};

FenceSync::FenceSync (void)
	: m_display		(EGL_NO_DISPLAY)
	, m_sync		(NULL)
	, m_waiterCount	(0)
	, m_serverSync	(false)
{
}

FenceSync::~FenceSync (void)
{
}

void FenceSync::addWaiter (void)
{
	m_lock.lock();
	m_waiterCount++;
	m_lock.unlock();
}

void FenceSync::init (EGLThread& thread, bool serverSync)
{
	m_display		= thread.runtimeContext->display;
	m_serverSync	= serverSync;

	// Use sync only if somebody will actualy depend on it
	m_lock.lock();
	if (m_waiterCount > 0)
	{
		thread.newMessage() << "Begin -- eglCreateSyncKHR(" << ((size_t)m_display) << ", EGL_SYNC_FENCE_KHR, DE_NULL)" << tcu::ThreadUtil::Message::End;
		m_sync = thread.egl.createSyncKHR(m_display, EGL_SYNC_FENCE_KHR, DE_NULL);
		thread.newMessage() << "End -- " << ((size_t)m_sync) << " = eglCreateSyncKHR()" << tcu::ThreadUtil::Message::End;
		TCU_CHECK(m_sync);
	}
	m_lock.unlock();
}

bool FenceSync::waitReady (EGLThread& thread)
{
	bool ok = true;
	if (m_serverSync)
	{
		thread.newMessage() << "Begin -- eglWaitSyncKHR(" << ((size_t)m_display) << ", " << ((size_t)m_sync) << ", 0)" << tcu::ThreadUtil::Message::End;
		EGLint result = thread.egl.waitSyncKHR(m_display, m_sync, 0);
		thread.newMessage() << "End -- " << result << " = eglWaitSyncKHR()" << tcu::ThreadUtil::Message::End;
		ok = result == EGL_TRUE;
	}
	else
	{
		thread.newMessage() << "Begin -- eglClientWaitSyncKHR(" << ((size_t)m_display) << ", " << ((size_t)m_sync) << ", EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 1000 000 000)" << tcu::ThreadUtil::Message::End;
		EGLint result = thread.egl.clientWaitSyncKHR(m_display, m_sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 1000000000);
		thread.newMessage() << "End -- " << result << " = eglClientWaitSyncKHR()" << tcu::ThreadUtil::Message::End;
		ok = result == EGL_CONDITION_SATISFIED_KHR;
	}

	m_lock.lock();
	m_waiterCount--;
	DE_ASSERT(m_waiterCount >= 0);

	if (m_waiterCount == 0)
	{
		// \note [mika] This is no longer deterministic and eglDestroySyncKHR might happen in different places and in different threads
		thread.newMessage() << "Begin -- eglDestroySyncKHR(" << ((size_t)m_display) << ", " << ((size_t)m_sync) << ")" << tcu::ThreadUtil::Message::End;
		EGLint destroyResult = thread.egl.destroySyncKHR(m_display, m_sync);
		thread.newMessage() << "End -- " << destroyResult << " = eglDestroySyncKHR()" << tcu::ThreadUtil::Message::End;
		m_sync = DE_NULL;
	}

	m_lock.unlock();

	return ok;
}

class Object : public tcu::ThreadUtil::Object
{
public:
							Object			(const char* type, SharedPtr<tcu::ThreadUtil::Event> e, SharedPtr<FenceSync> sync);
							~Object			(void);

	void					readGL			(SharedPtr<FenceSync> sync, std::vector<SharedPtr<FenceSync> >& deps);
	void					modifyGL		(SharedPtr<FenceSync> sync, std::vector<SharedPtr<FenceSync> >& deps);

private:
	SharedPtr<FenceSync>			m_modifySync;
	vector<SharedPtr<FenceSync> >	m_readSyncs;
};

Object::Object (const char* type, SharedPtr<tcu::ThreadUtil::Event> e, SharedPtr<FenceSync> sync)
	: tcu::ThreadUtil::Object	(type, e)
	, m_modifySync				(sync)
{
}

Object::~Object	(void)
{
}

void Object::readGL (SharedPtr<FenceSync> sync, std::vector<SharedPtr<FenceSync> >& deps)
{
	if (m_modifySync)
		m_modifySync->addWaiter();

	// Make call depend on last modifying call
	deps.push_back(m_modifySync);

	// Add read dependency
	m_readSyncs.push_back(sync);
}

void Object::modifyGL (SharedPtr<FenceSync> sync, std::vector<SharedPtr<FenceSync> >& deps)
{
	// Make call depend on all reads
	for (int readNdx = 0; readNdx < (int)m_readSyncs.size(); readNdx++)
	{
		if (m_readSyncs[readNdx])
			m_readSyncs[readNdx]->addWaiter();

		deps.push_back(m_readSyncs[readNdx]);
	}

	if (m_modifySync)
		m_modifySync->addWaiter();

	deps.push_back(m_modifySync);

	// Update last modifying call
	m_modifySync = sync;

	// Clear read dependencies of last "version" of this object
	m_readSyncs.clear();
}

class Operation : public tcu::ThreadUtil::Operation
{
public:
							Operation		(const char* name, bool useSync, bool serverSync);
	virtual					~Operation		(void);

	SharedPtr<FenceSync>	getSync			(void) { return m_sync; }
	void					readGLObject	(SharedPtr<Object> object);
	void					modifyGLObject	(SharedPtr<Object> object);

	virtual void			execute			(tcu::ThreadUtil::Thread& thread);

private:
	bool								m_useSync;
	bool								m_serverSync;
	std::vector<SharedPtr<FenceSync> >	m_syncDeps;
	SharedPtr<FenceSync>				m_sync;
};

Operation::Operation (const char* name, bool useSync, bool serverSync)
	: tcu::ThreadUtil::Operation	(name)
	, m_useSync						(useSync)
	, m_serverSync					(serverSync)
	, m_sync						(useSync ? SharedPtr<FenceSync>(new FenceSync()) : SharedPtr<FenceSync>())
{
}

Operation::~Operation (void)
{
}

void Operation::readGLObject (SharedPtr<Object> object)
{
	object->read(m_event, m_deps);
	object->readGL(m_sync, m_syncDeps);
}

void Operation::modifyGLObject (SharedPtr<Object> object)
{
	object->modify(m_event, m_deps);
	object->modifyGL(m_sync, m_syncDeps);
}

void Operation::execute (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	bool success = true;

	// Wait for dependencies and check that they succeeded
	for (int depNdx = 0; depNdx < (int)m_deps.size(); depNdx++)
	{
		if (!m_deps[depNdx]->waitReady())
			success = false;
	}

	// Try execute operation
	if (success)
	{
		try
		{
			if (m_useSync)
			{
				for (int depNdx = 0; depNdx < (int)m_syncDeps.size(); depNdx++)
				{
					EGLThread* eglThread = dynamic_cast<EGLThread*>(&thread);
					DE_ASSERT(eglThread);
					if (m_syncDeps[depNdx]->waitReady(*eglThread) != tcu::ThreadUtil::Event::RESULT_OK)
					{
						success = false;
						break;
					}
				}
			}

			if (success)
			{
				exec(thread);
				if (m_useSync)
				{
					EGLThread* eglThread = dynamic_cast<EGLThread*>(&thread);
					DE_ASSERT(eglThread);
					m_sync->init(*eglThread, m_serverSync);
					thread.newMessage() << "Begin -- glFlush()" << tcu::ThreadUtil::Message::End;
					GLU_CHECK_GLW_CALL(thread.gl, flush());
					thread.newMessage() << "End -- glFlush()" << tcu::ThreadUtil::Message::End;
				}
				else
				{
					thread.newMessage() << "Begin -- glFinish()" << tcu::ThreadUtil::Message::End;
					GLU_CHECK_GLW_CALL(thread.gl, finish());
					thread.newMessage() << "End -- glFinish()" << tcu::ThreadUtil::Message::End;
				}
			}
		}
		catch (...)
		{
			// Got exception event failed
			m_event->setResult(tcu::ThreadUtil::Event::RESULT_FAILED);
			throw;
		}
	}

	if (success)
		m_event->setResult(tcu::ThreadUtil::Event::RESULT_OK);
	else
		m_event->setResult(tcu::ThreadUtil::Event::RESULT_FAILED);

	m_deps.clear();
	m_event = SharedPtr<tcu::ThreadUtil::Event>();
	m_syncDeps.clear();
	m_sync = SharedPtr<FenceSync>();
}

class EGLImage : public Object
{
public:
				EGLImage	(SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync);
	virtual		~EGLImage	(void) {}

	EGLImageKHR	image;
};

// EGLResource manager
class EGLResourceManager
{
public:

	void					addContext		(SharedPtr<GLES2Context> context) { m_contexts.push_back(context); }
	void					addSurface		(SharedPtr<Surface> surface) { m_surfaces.push_back(surface); }
	void					addImage		(SharedPtr<EGLImage> image) { m_images.push_back(image); }

	SharedPtr<Surface>		popSurface		(int index);
	SharedPtr<GLES2Context>	popContext		(int index);
	SharedPtr<EGLImage>		popImage		(int index);

	int						getContextCount	(void) const { return (int)m_contexts.size(); }
	int						getSurfaceCount	(void) const { return (int)m_surfaces.size(); }
	int						getImageCount	(void) const { return (int)m_images.size(); }

private:
	std::vector<SharedPtr<GLES2Context> >	m_contexts;
	std::vector<SharedPtr<Surface> >		m_surfaces;
	std::vector<SharedPtr<EGLImage> >		m_images;
};

SharedPtr<Surface> EGLResourceManager::popSurface (int index)
{
	SharedPtr<Surface> surface = m_surfaces[index];
	m_surfaces.erase(m_surfaces.begin() + index);
	return surface;
}

SharedPtr<GLES2Context> EGLResourceManager::popContext (int index)
{
	SharedPtr<GLES2Context> context = m_contexts[index];
	m_contexts.erase(m_contexts.begin() + index);
	return context;
}

SharedPtr<EGLImage> EGLResourceManager::popImage (int index)
{
	SharedPtr<EGLImage> image = m_images[index];
	m_images.erase(m_images.begin() + index);
	return image;
}

class CreateContext : public tcu::ThreadUtil::Operation
{
public:
				CreateContext	(EGLDisplay display, EGLConfig config, SharedPtr<GLES2Context> shared, SharedPtr<GLES2Context>& context);

	void		exec			(tcu::ThreadUtil::Thread& thread);

private:
	EGLDisplay					m_display;
	EGLConfig					m_config;
	SharedPtr<GLES2Context>		m_shared;
	SharedPtr<GLES2Context>		m_context;
};

CreateContext::CreateContext (EGLDisplay display, EGLConfig config, SharedPtr<GLES2Context> shared, SharedPtr<GLES2Context>& context)
	: tcu::ThreadUtil::Operation	("CreateContext")
	, m_display					(display)
	, m_config					(config)
	, m_shared					(shared)
{
	if (shared)
		modifyObject(SharedPtr<tcu::ThreadUtil::Object>(shared));

	context = SharedPtr<GLES2Context>(new GLES2Context(getEvent(), (shared ? shared->resourceManager : SharedPtr<GLES2ResourceManager>(new GLES2ResourceManager))));
	m_context = context;
}

void CreateContext::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	m_context->display = m_display;

	const EGLint attriblist[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	thread.newMessage() << "Begin -- eglBindAPI(EGL_OPENGL_ES_API)" << tcu::ThreadUtil::Message::End;
	EGLU_CHECK_CALL(thread.egl, bindAPI(EGL_OPENGL_ES_API));
	thread.newMessage() << "End -- eglBindAPI()" << tcu::ThreadUtil::Message::End;

	if (m_shared)
	{
		DE_ASSERT(m_shared->context != EGL_NO_CONTEXT);
		DE_ASSERT(m_shared->display != EGL_NO_DISPLAY);
		DE_ASSERT(m_shared->display == m_display);

		thread.newMessage() << "Begin -- eglCreateContext(" << m_display << ", " << m_config << ", " << m_shared->context << ", { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE })" << tcu::ThreadUtil::Message::End;
		m_context->context = thread.egl.createContext(m_display, m_config, m_shared->context, attriblist);
		thread.newMessage() << "End -- " << m_context->context << " = eglCreateContext()" << tcu::ThreadUtil::Message::End;
	}
	else
	{
		thread.newMessage() << "Begin -- eglCreateContext(" << m_display << ", " << m_config << ", EGL_NO_CONTEXT, { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE })" << tcu::ThreadUtil::Message::End;
		m_context->context = thread.egl.createContext(m_display, m_config, EGL_NO_CONTEXT, attriblist);
		thread.newMessage() << "End -- " << m_context->context << " = eglCreateContext()" << tcu::ThreadUtil::Message::End;
	}

	EGLU_CHECK_MSG(thread.egl, "Failed to create GLES2 context");
	TCU_CHECK(m_context->context != EGL_NO_CONTEXT);
}

class DestroyContext : public tcu::ThreadUtil::Operation
{
public:
							DestroyContext	(SharedPtr<GLES2Context> contex);
	void					exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<GLES2Context>	m_context;
};

DestroyContext::DestroyContext (SharedPtr<GLES2Context> contex)
	: tcu::ThreadUtil::Operation	("DestroyContext")
	, m_context					(contex)
{
	modifyObject(SharedPtr<tcu::ThreadUtil::Object>(m_context));
}

void DestroyContext::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- eglDestroyContext(" << m_context->display << ", " << m_context->context << ")" << tcu::ThreadUtil::Message::End;
	EGLU_CHECK_CALL(thread.egl, destroyContext(m_context->display, m_context->context));
	thread.newMessage() << "End -- eglDestroyContext()" << tcu::ThreadUtil::Message::End;
	m_context->display	= EGL_NO_DISPLAY;
	m_context->context	= EGL_NO_CONTEXT;
}

class MakeCurrent : public tcu::ThreadUtil::Operation
{
public:
			MakeCurrent	(EGLThread& thread, EGLDisplay display, SharedPtr<Surface> surface, SharedPtr<GLES2Context> context);

	void	exec		(tcu::ThreadUtil::Thread& thread);

private:
	EGLDisplay				m_display;
	SharedPtr<Surface>		m_surface;
	SharedPtr<GLES2Context>	m_context;
};

MakeCurrent::MakeCurrent (EGLThread& thread, EGLDisplay display, SharedPtr<Surface> surface, SharedPtr<GLES2Context> context)
	: tcu::ThreadUtil::Operation	("MakeCurrent")
	, m_display					(display)
	, m_surface					(surface)
	, m_context					(context)
{
	if (m_context)
		modifyObject(SharedPtr<tcu::ThreadUtil::Object>(m_context));

	if (m_surface)
		modifyObject(SharedPtr<tcu::ThreadUtil::Object>(m_surface));

	// Release old contexts
	if (thread.context)
	{
		modifyObject(SharedPtr<tcu::ThreadUtil::Object>(thread.context));
	}

	// Release old surface
	if (thread.surface)
	{
		modifyObject(SharedPtr<tcu::ThreadUtil::Object>(thread.surface));
	}

	thread.context	= m_context;
	thread.surface	= m_surface;
}

void MakeCurrent::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	if (m_context)
	{
		thread.eglSurface = m_surface->surface;
		thread.runtimeContext = m_context;

		DE_ASSERT(m_surface);
		thread.newMessage() << "Begin -- eglMakeCurrent(" << m_display << ", " << m_surface->surface << ", " << m_surface->surface << ", " << m_context->context << ")" << tcu::ThreadUtil::Message::End;
		EGLU_CHECK_CALL(thread.egl, makeCurrent(m_display, m_surface->surface, m_surface->surface, m_context->context));
		thread.newMessage() << "End -- eglMakeCurrent()" << tcu::ThreadUtil::Message::End;
	}
	else
	{
		thread.runtimeContext = m_context;

		thread.newMessage() << "Begin -- eglMakeCurrent(" << m_display << ", EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)" << tcu::ThreadUtil::Message::End;
		EGLU_CHECK_CALL(thread.egl, makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
		thread.newMessage() << "End -- eglMakeCurrent()" << tcu::ThreadUtil::Message::End;
	}
}

class InitGLExtension : public tcu::ThreadUtil::Operation
{
public:
			InitGLExtension		(const char* extension);

	void	exec				(tcu::ThreadUtil::Thread& thread);

private:
	std::string					m_extension;
};

InitGLExtension::InitGLExtension (const char* extension)
	: tcu::ThreadUtil::Operation	("InitGLExtension")
	, m_extension					(extension)
{
}

void InitGLExtension::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	// Check extensions
	bool found = false;

	thread.newMessage() << "Begin -- glGetString(GL_EXTENSIONS)" << tcu::ThreadUtil::Message::End;
	std::string extensions = (const char*)thread.gl.getString(GL_EXTENSIONS);
	thread.newMessage() << "End -- glGetString()" << tcu::ThreadUtil::Message::End;

	std::string::size_type pos = extensions.find(" ");

	do
	{
		std::string extension;
		if (pos != std::string::npos)
		{
			extension = extensions.substr(0, pos);
			extensions = extensions.substr(pos+1);
		}
		else
		{
			extension = extensions;
			extensions = "";
		}

		if (extension == m_extension)
		{
			found = true;
			break;
		}
		pos = extensions.find(" ");
	} while (pos != std::string::npos);

	if (!found)
		throw tcu::NotSupportedError((m_extension + " not supported").c_str(), "", __FILE__, __LINE__);


	// Query function pointers
	if (m_extension == "GL_OES_EGL_image")
	{
		thread.newMessage() << "Begin -- eglGetProcAddress(\"glEGLImageTargetTexture2DOES\")" << tcu::ThreadUtil::Message::End;
		thread.runtimeContext->glExtensions.imageTargetTexture2D = (glEGLImageTargetTexture2DOESFunc)thread.egl.getProcAddress("glEGLImageTargetTexture2DOES");
		thread.newMessage() << "End --  " << ((void*)thread.runtimeContext->glExtensions.imageTargetTexture2D) << " = eglGetProcAddress()"<< tcu::ThreadUtil::Message::End;
	}
}

class CreatePBufferSurface : public tcu::ThreadUtil::Operation
{
public:
				CreatePBufferSurface	(EGLDisplay display, EGLConfig config, EGLint width, EGLint height, SharedPtr<Surface>& surface);
	void		exec					(tcu::ThreadUtil::Thread& thread);

private:
	EGLDisplay			m_display;
	EGLConfig			m_config;
	EGLint				m_width;
	EGLint				m_height;
	SharedPtr<Surface>	m_surface;
};

CreatePBufferSurface::CreatePBufferSurface (EGLDisplay display, EGLConfig config, EGLint width, EGLint height, SharedPtr<Surface>& surface)
	: tcu::ThreadUtil::Operation	("CreatePBufferSurface")
	, m_display					(display)
	, m_config					(config)
	, m_width					(width)
	, m_height					(height)
{
	surface = SharedPtr<Surface>(new Surface(getEvent()));
	m_surface = surface;
}

void CreatePBufferSurface::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	const EGLint attriblist[] = {
		EGL_WIDTH, m_width,
		EGL_HEIGHT, m_height,
		EGL_NONE
	};

	thread.newMessage() << "Begin -- eglCreatePbufferSurface(" << m_display << ", " << m_config << ", { EGL_WIDTH, " << m_width << ", EGL_HEIGHT, " << m_height << ", EGL_NONE })" << tcu::ThreadUtil::Message::End;
	m_surface->surface = thread.egl.createPbufferSurface(m_display, m_config, attriblist);
	thread.newMessage() << "End -- " << m_surface->surface << "= eglCreatePbufferSurface()" << tcu::ThreadUtil::Message::End;
	EGLU_CHECK_MSG(thread.egl, "eglCreatePbufferSurface()");
}

class DestroySurface : public tcu::ThreadUtil::Operation
{
public:
			DestroySurface	(EGLDisplay display, SharedPtr<Surface> surface);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	EGLDisplay			m_display;
	SharedPtr<Surface>	m_surface;
};

DestroySurface::DestroySurface (EGLDisplay display, SharedPtr<Surface> surface)
	: tcu::ThreadUtil::Operation	("DestroySurface")
	, m_display					(display)
	, m_surface					(surface)
{
	modifyObject(SharedPtr<tcu::ThreadUtil::Object>(m_surface));
}

void DestroySurface::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- eglDestroySurface(" << m_display << ",  " << m_surface->surface << ")" << tcu::ThreadUtil::Message::End;
	EGLU_CHECK_CALL(thread.egl, destroySurface(m_display, m_surface->surface));
	thread.newMessage() << "End -- eglDestroySurface()" << tcu::ThreadUtil::Message::End;
}

EGLImage::EGLImage (SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync)
	: Object	("EGLImage", event, sync)
	, image		(EGL_NO_IMAGE_KHR)
{
}

class Texture : public Object
{
public:
			Texture (SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync);

	// Runtime parameters
	GLuint	texture;

	// Call generation time parameters
	bool	isDefined;

	SharedPtr<EGLImage>	sourceImage;
};

Texture::Texture (SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync)
	: Object					("Texture", event, sync)
	, texture					(0)
	, isDefined					(false)
{
}

class CreateTexture : public Operation
{
public:
			CreateTexture	(SharedPtr<Texture>& texture, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture> m_texture;
};

CreateTexture::CreateTexture (SharedPtr<Texture>& texture, bool useSync, bool serverSync)
	: Operation	("CreateTexture", useSync, serverSync)
{
	texture = SharedPtr<Texture>(new Texture(getEvent(), getSync()));
	m_texture = texture;
}

void CreateTexture::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint tex = 0;

	thread.newMessage() << "Begin -- glGenTextures(1, { 0 })" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, genTextures(1, &tex));
	thread.newMessage() << "End -- glGenTextures(1, { " << tex << " })" << tcu::ThreadUtil::Message::End;

	m_texture->texture = tex;
}

class DeleteTexture : public Operation
{
public:
			DeleteTexture	(SharedPtr<Texture> texture, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture> m_texture;
};

DeleteTexture::DeleteTexture (SharedPtr<Texture> texture, bool useSync, bool serverSync)
	: Operation		("DeleteTexture", useSync, serverSync)
	, m_texture		(texture)
{
	modifyGLObject(SharedPtr<Object>(m_texture));
}

void DeleteTexture::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint tex = m_texture->texture;

	thread.newMessage() << "Begin -- glDeleteTextures(1, { " << tex << " })" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, deleteTextures(1, &tex));
	thread.newMessage() << "End -- glDeleteTextures()" << tcu::ThreadUtil::Message::End;

	m_texture->texture = 0;
}

class TexImage2D : public Operation
{
public:
			TexImage2D	(SharedPtr<Texture> texture, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, bool useSync, bool serverSync);
	void	exec		(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture>	m_texture;
	GLint				m_level;
	GLint				m_internalFormat;
	GLsizei				m_width;
	GLsizei				m_height;
	GLenum				m_format;
	GLenum				m_type;
};

TexImage2D::TexImage2D (SharedPtr<Texture> texture, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, bool useSync, bool serverSync)
	: Operation			("TexImage2D", useSync, serverSync)
	, m_texture			(texture)
	, m_level			(level)
	, m_internalFormat	(internalFormat)
	, m_width			(width)
	, m_height			(height)
	, m_format			(format)
	, m_type			(type)
{
	modifyGLObject(SharedPtr<Object>(m_texture));
	m_texture->isDefined = true;

	// Orphang texture
	texture->sourceImage = SharedPtr<EGLImage>();
}

void TexImage2D::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	void* dummyData = thread.getDummyData(m_width*m_height*4);

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, " << m_texture->texture << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, m_texture->texture));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glTexImage2D(GL_TEXTURE_2D, " << m_level << ", " << m_internalFormat << ", " << m_width << ", " << m_height << ", 0, " << m_format << ", " << m_type << ", data)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, texImage2D(GL_TEXTURE_2D, m_level, m_internalFormat, m_width, m_height, 0, m_format, m_type, dummyData));
	thread.newMessage() << "End -- glTexImage2D()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, 0));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;
}

class TexSubImage2D : public Operation
{
public:
			TexSubImage2D	(SharedPtr<Texture> texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture>	m_texture;
	GLint				m_level;
	GLint				m_xoffset;
	GLint				m_yoffset;
	GLsizei				m_width;
	GLsizei				m_height;
	GLenum				m_format;
	GLenum				m_type;
};

TexSubImage2D::TexSubImage2D (SharedPtr<Texture> texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, bool useSync, bool serverSync)
	: Operation		("TexSubImage2D", useSync, serverSync)
	, m_texture		(texture)
	, m_level		(level)
	, m_xoffset		(xoffset)
	, m_yoffset		(yoffset)
	, m_width		(width)
	, m_height		(height)
	, m_format		(format)
	, m_type		(type)
{
	modifyGLObject(SharedPtr<Object>(m_texture));

	if (m_texture->sourceImage)
		modifyGLObject(SharedPtr<Object>(m_texture->sourceImage));
}

void TexSubImage2D::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	void* dummyData = thread.getDummyData(m_width*m_height*4);

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, " << m_texture->texture << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, m_texture->texture));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glTexSubImage2D(GL_TEXTURE_2D, " << m_level << ", " << m_xoffset << ", " << m_yoffset << ", " << m_width << ", " << m_height << ", 0, " << m_format << ", " << m_type << ", <data>)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, texSubImage2D(GL_TEXTURE_2D, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type, dummyData));
	thread.newMessage() << "End -- glSubTexImage2D()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, 0));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;
}

class CopyTexImage2D : public Operation
{
public:
			CopyTexImage2D	(SharedPtr<Texture> texture, GLint level, GLint internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture>	m_texture;
	GLint				m_level;
	GLint				m_internalFormat;
	GLint				m_x;
	GLint				m_y;
	GLsizei				m_width;
	GLsizei				m_height;
	GLint				m_border;
};

CopyTexImage2D::CopyTexImage2D (SharedPtr<Texture> texture, GLint level, GLint internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border, bool useSync, bool serverSync)
	: Operation			("CopyTexImage2D", useSync, serverSync)
	, m_texture			(texture)
	, m_level			(level)
	, m_internalFormat	(internalFormat)
	, m_x				(x)
	, m_y				(y)
	, m_width			(width)
	, m_height			(height)
	, m_border			(border)
{
	modifyGLObject(SharedPtr<Object>(m_texture));
	texture->isDefined = true;

	// Orphang texture
	texture->sourceImage = SharedPtr<EGLImage>();
}

void CopyTexImage2D::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, " << m_texture->texture << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, m_texture->texture));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glCopyTexImage2D(GL_TEXTURE_2D, " << m_level << ", " << m_internalFormat << ", " << m_x << ", " << m_y << ", " << m_width << ", " << m_height << ", " << m_border << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, copyTexImage2D(GL_TEXTURE_2D, m_level, m_internalFormat, m_x, m_y, m_width, m_height, m_border));
	thread.newMessage() << "End -- glCopyTexImage2D()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, 0));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;
}

class CopyTexSubImage2D : public Operation
{
public:
			CopyTexSubImage2D		(SharedPtr<Texture> texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, bool useSync, bool serverSync);
	void	exec					(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture>	m_texture;
	GLint				m_level;
	GLint				m_xoffset;
	GLint				m_yoffset;
	GLint				m_x;
	GLint				m_y;
	GLsizei				m_width;
	GLsizei				m_height;
};

CopyTexSubImage2D::CopyTexSubImage2D (SharedPtr<Texture> texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, bool useSync, bool serverSync)
	: Operation		("CopyTexSubImage2D", useSync, serverSync)
	, m_texture		(texture)
	, m_level		(level)
	, m_xoffset		(xoffset)
	, m_yoffset		(yoffset)
	, m_x			(x)
	, m_y			(y)
	, m_width		(width)
	, m_height		(height)
{
	modifyGLObject(SharedPtr<Object>(m_texture));

	if (m_texture->sourceImage)
		modifyGLObject(SharedPtr<Object>(m_texture->sourceImage));
}

void CopyTexSubImage2D::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, " << m_texture->texture << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, m_texture->texture));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glCopyTexSubImage2D(GL_TEXTURE_2D, " << m_level << ", " << m_xoffset << ", " << m_yoffset << ", " << m_x << ", " << m_y << ", " << m_width << ", " << m_height << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, copyTexSubImage2D(GL_TEXTURE_2D, m_level, m_xoffset, m_yoffset, m_x, m_y, m_width, m_height));
	thread.newMessage() << "End -- glCopyTexSubImage2D()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, 0));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;
}

class Buffer : public Object
{
public:
				Buffer		(SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync);

	// Runtime attributes
	GLuint		buffer;
	GLsizeiptr	size;

	// Call generation time parameters
	bool		isDefined;
};

Buffer::Buffer (SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync)
	: Object		("Buffer", event, sync)
	, buffer		(0)
	, size			(0)
	, isDefined		(false)
{
}

class CreateBuffer : public Operation
{
public:
			CreateBuffer	(SharedPtr<Buffer>& buffer, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Buffer> m_buffer;
};

CreateBuffer::CreateBuffer (SharedPtr<Buffer>& buffer, bool useSync, bool serverSync)
	: Operation	("CreateBuffer", useSync, serverSync)
{
	buffer = SharedPtr<Buffer>(new Buffer(getEvent(), getSync()));
	m_buffer = buffer;
}

void CreateBuffer::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint buffer = 0;

	thread.newMessage() << "Begin -- glGenBuffers(1, { 0 })" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, genBuffers(1, &buffer));
	thread.newMessage() << "End -- glGenBuffers(1, { " << buffer << " })" << tcu::ThreadUtil::Message::End;

	m_buffer->buffer = buffer;
}

class DeleteBuffer : public Operation
{
public:
			DeleteBuffer	(SharedPtr<Buffer> buffer, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Buffer> m_buffer;
};

DeleteBuffer::DeleteBuffer (SharedPtr<Buffer> buffer, bool useSync, bool serverSync)
	: Operation	("DeleteBuffer", useSync, serverSync)
	, m_buffer	(buffer)
{
	modifyGLObject(SharedPtr<Object>(m_buffer));
}

void DeleteBuffer::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint buffer = m_buffer->buffer;

	thread.newMessage() << "Begin -- glDeleteBuffers(1, { " << buffer << " })" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, deleteBuffers(1, &buffer));
	thread.newMessage() << "End -- glDeleteBuffers()" << tcu::ThreadUtil::Message::End;

	m_buffer->buffer = 0;
}

class BufferData : public Operation
{
public:
			BufferData	(SharedPtr<Buffer> buffer, GLenum target, GLsizeiptr size, GLenum usage, bool useSync, bool serverSync);
	void	exec		(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Buffer>	m_buffer;
	GLenum				m_target;
	GLsizeiptr			m_size;
	GLenum				m_usage;
};

BufferData::BufferData (SharedPtr<Buffer> buffer, GLenum target, GLsizeiptr size, GLenum usage, bool useSync, bool serverSync)
	: Operation	("BufferData", useSync, serverSync)
	, m_buffer	(buffer)
	, m_target	(target)
	, m_size	(size)
	, m_usage	(usage)
{
	modifyGLObject(SharedPtr<Object>(m_buffer));
	buffer->isDefined	= true;
	buffer->size		= size;
}

void BufferData::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	void* dummyData = thread.getDummyData(m_size);

	thread.newMessage() << "Begin -- glBindBuffer(" << m_target << ", " << m_buffer->buffer << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(m_target, m_buffer->buffer));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBufferData(" << m_target << ", " << m_size << ", <DATA>, " << m_usage << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bufferData(m_target, m_size, dummyData, m_usage));
	thread.newMessage() << "End -- glBufferData()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindBuffer(" << m_target << ", 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(m_target, 0));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;
}

class BufferSubData : public Operation
{
public:
			BufferSubData	(SharedPtr<Buffer> buffer, GLenum target, GLintptr offset, GLsizeiptr size, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Buffer>	m_buffer;
	GLenum				m_target;
	GLintptr			m_offset;
	GLsizeiptr			m_size;
};

BufferSubData::BufferSubData (SharedPtr<Buffer> buffer, GLenum target, GLintptr offset, GLsizeiptr size, bool useSync, bool serverSync)
	: Operation	("BufferSubData", useSync, serverSync)
	, m_buffer	(buffer)
	, m_target	(target)
	, m_offset	(offset)
	, m_size	(size)
{
	modifyGLObject(SharedPtr<Object>(m_buffer));
}

void BufferSubData::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	void* dummyData = thread.getDummyData(m_size);

	thread.newMessage() << "Begin -- glBindBuffer(" << m_target << ", " << m_buffer->buffer << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(m_target, m_buffer->buffer));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBufferSubData(" << m_target << ", " << m_offset << ", " << m_size << ", <DATA>)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bufferSubData(m_target, m_offset, m_size, dummyData));
	thread.newMessage() << "End -- glBufferSubData()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindBuffer(" << m_target << ", 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(m_target, 0));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;
}

class Shader : public Object
{
public:
				Shader		(SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync);

	GLuint		shader;
	GLenum		type;
	bool		isDefined;
	bool		compiled;
};

Shader::Shader (SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync)
	: Object		("Shader", event, sync)
	, shader		(0)
	, type			(GL_NONE)
	, isDefined		(false)
	, compiled		(false)
{
}

class CreateShader : public Operation
{
public:
			CreateShader	(GLenum type, SharedPtr<Shader>& shader, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Shader>	m_shader;
	GLenum				m_type;
};

CreateShader::CreateShader (GLenum type, SharedPtr<Shader>& shader, bool useSync, bool serverSync)
	: Operation	("CreateShader", useSync, serverSync)
	, m_type	(type)
{
	shader = SharedPtr<Shader>(new Shader(getEvent(), getSync()));
	shader->type = type;

	m_shader = shader;
}

void CreateShader::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint shader = 0;

	thread.newMessage() << "Begin -- glCreateShader(" << m_type << ")" << tcu::ThreadUtil::Message::End;
	shader = thread.gl.createShader(m_type);
	GLU_CHECK_GLW_MSG(thread.gl, "glCreateShader()");
	thread.newMessage() << "End -- " << shader  << " = glCreateShader(" << m_type << ")" << tcu::ThreadUtil::Message::End;

	m_shader->shader	= shader;
}

class DeleteShader : public Operation
{
public:
			DeleteShader	(SharedPtr<Shader> shader, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Shader> m_shader;
};

DeleteShader::DeleteShader (SharedPtr<Shader> shader, bool useSync, bool serverSync)
	: Operation	("DeleteShader", useSync, serverSync)
	, m_shader	(shader)
{
	modifyGLObject(SharedPtr<Object>(m_shader));
}

void DeleteShader::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint shader = m_shader->shader;

	thread.newMessage() << "Begin -- glDeleteShader(" << shader << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, deleteShader(shader));
	thread.newMessage() << "End -- glDeleteShader()" << tcu::ThreadUtil::Message::End;

	m_shader->shader = 0;
}

class ShaderSource : public Operation
{
public:
			ShaderSource	(SharedPtr<Shader> sharder, const char* source, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Shader>	m_shader;
	string				m_source;
};

ShaderSource::ShaderSource (SharedPtr<Shader> shader, const char* source, bool useSync, bool serverSync)
	: Operation	("ShaderSource", useSync, serverSync)
	, m_shader	(shader)
	, m_source	(source)
{
	modifyGLObject(SharedPtr<Object>(m_shader));
	m_shader->isDefined = true;
}

void ShaderSource::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	const char* shaderSource = m_source.c_str();

	thread.newMessage() << "Begin -- glShaderSource(" << m_shader->shader << ", 1, \"" << shaderSource << "\", DE_NULL)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, shaderSource(m_shader->shader, 1, &shaderSource, DE_NULL));
	thread.newMessage() << "End -- glShaderSource()" << tcu::ThreadUtil::Message::End;
}

class ShaderCompile : public Operation
{
public:
			ShaderCompile	(SharedPtr<Shader> sharder, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Shader> m_shader;
};

ShaderCompile::ShaderCompile (SharedPtr<Shader> shader, bool useSync, bool serverSync)
	: Operation	("ShaderCompile", useSync, serverSync)
	, m_shader	(shader)
{
	m_shader->compiled = true;
	modifyGLObject(SharedPtr<Object>(m_shader));
}

void ShaderCompile::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- glCompileShader(" << m_shader->shader << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, compileShader(m_shader->shader));
	thread.newMessage() << "End -- glCompileShader()" << tcu::ThreadUtil::Message::End;
}

class Program : public Object
{
public:
						Program		(SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync);

	// Generation time attributes
	SharedPtr<Shader>	vertexShader;
	SharedPtr<Shader>	fragmentShader;
	bool				linked;

	// Runtime attributes
	GLuint				program;
	GLuint				runtimeVertexShader;
	GLuint				runtimeFragmentShader;
};

Program::Program (SharedPtr<tcu::ThreadUtil::Event> event, SharedPtr<FenceSync> sync)
	: Object					("Program", event, sync)
	, linked					(false)
	, program					(0)
	, runtimeVertexShader		(0)
	, runtimeFragmentShader		(0)
{
}

class CreateProgram : public Operation
{
public:
			CreateProgram	(SharedPtr<Program>& program, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Program> m_program;
};

CreateProgram::CreateProgram (SharedPtr<Program>& program, bool useSync, bool serverSync)
	: Operation	("CreateProgram", useSync, serverSync)
{
	program = SharedPtr<Program>(new Program(getEvent(), getSync()));
	m_program = program;
}

void CreateProgram::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint program = 0;

	thread.newMessage() << "Begin -- glCreateProgram()" << tcu::ThreadUtil::Message::End;
	program = thread.gl.createProgram();
	GLU_CHECK_GLW_MSG(thread.gl, "glCreateProgram()");
	thread.newMessage() << "End -- " << program  << " = glCreateProgram()" << tcu::ThreadUtil::Message::End;

	m_program->program	= program;
}

class DeleteProgram : public Operation
{
public:
			DeleteProgram	(SharedPtr<Program> program, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Program> m_program;
};

DeleteProgram::DeleteProgram (SharedPtr<Program> program, bool useSync, bool serverSync)
	: Operation	("DeleteProgram", useSync, serverSync)
	, m_program	(program)
{
	modifyGLObject(SharedPtr<Object>(m_program));
}

void DeleteProgram::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint program = m_program->program;

	thread.newMessage() << "Begin -- glDeleteProgram(" << program << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, deleteProgram(program));
	thread.newMessage() << "End -- glDeleteProgram()" << tcu::ThreadUtil::Message::End;

	m_program->program = 0;
}

class AttachShader : public Operation
{
public:
			AttachShader	(SharedPtr<Program> sharder, SharedPtr<Shader> shader, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Program>	m_program;
	SharedPtr<Shader>	m_shader;
};

AttachShader::AttachShader (SharedPtr<Program> program, SharedPtr<Shader> shader, bool useSync, bool serverSync)
	: Operation	("AttachShader", useSync, serverSync)
	, m_program	(program)
	, m_shader	(shader)
{
	modifyGLObject(SharedPtr<Object>(m_program));
	readGLObject(SharedPtr<Object>(m_shader));

	if (m_shader->type == GL_VERTEX_SHADER)
		m_program->vertexShader = shader;
	else if (m_shader->type == GL_FRAGMENT_SHADER)
		m_program->fragmentShader = shader;
	else
		DE_ASSERT(false);
}

void AttachShader::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- glAttachShader(" << m_program->program << ", " << m_shader->shader << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, attachShader(m_program->program, m_shader->shader));
	thread.newMessage() << "End -- glAttachShader()" << tcu::ThreadUtil::Message::End;

	if (m_shader->type == GL_VERTEX_SHADER)
		m_program->runtimeVertexShader = m_shader->shader;
	else if (m_shader->type == GL_FRAGMENT_SHADER)
		m_program->runtimeFragmentShader = m_shader->shader;
	else
		DE_ASSERT(false);
}

class DetachShader : public Operation
{
public:
			DetachShader	(SharedPtr<Program> sharder, GLenum type, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Program>	m_program;
	GLenum				m_type;
};

DetachShader::DetachShader (SharedPtr<Program> program, GLenum type, bool useSync, bool serverSync)
	: Operation	("DetachShader", useSync, serverSync)
	, m_program	(program)
	, m_type	(type)
{
	modifyGLObject(SharedPtr<Object>(m_program));

	if (m_type == GL_VERTEX_SHADER)
	{
		DE_ASSERT(m_program->vertexShader);
		m_program->vertexShader = SharedPtr<Shader>();
	}
	else if (m_type == GL_FRAGMENT_SHADER)
	{
		DE_ASSERT(m_program->fragmentShader);
		m_program->fragmentShader = SharedPtr<Shader>();
	}
	else
		DE_ASSERT(false);
}

void DetachShader::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	if (m_type == GL_VERTEX_SHADER)
	{
		thread.newMessage() << "Begin -- glDetachShader(" << m_program->program << ", " << m_program->runtimeVertexShader << ")" << tcu::ThreadUtil::Message::End;
		GLU_CHECK_GLW_CALL(thread.gl, detachShader(m_program->program, m_program->runtimeVertexShader));
		thread.newMessage() << "End -- glDetachShader()" << tcu::ThreadUtil::Message::End;
		m_program->runtimeVertexShader = 0;
	}
	else if (m_type == GL_FRAGMENT_SHADER)
	{
		thread.newMessage() << "Begin -- glDetachShader(" << m_program->program << ", " << m_program->runtimeFragmentShader << ")" << tcu::ThreadUtil::Message::End;
		GLU_CHECK_GLW_CALL(thread.gl, detachShader(m_program->program, m_program->runtimeFragmentShader));
		thread.newMessage() << "End -- glDetachShader()" << tcu::ThreadUtil::Message::End;
		m_program->runtimeFragmentShader = 0;
	}
	else
		DE_ASSERT(false);
}

class LinkProgram : public Operation
{
public:
			LinkProgram	(SharedPtr<Program> program, bool useSync, bool serverSync);
	void	exec		(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Program> m_program;
};

LinkProgram::LinkProgram (SharedPtr<Program> program, bool useSync, bool serverSync)
	: Operation	("LinkProgram", useSync, serverSync)
	, m_program	(program)
{
	modifyGLObject(SharedPtr<Object>(m_program));
	program->linked = true;
}

void LinkProgram::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);
	GLuint program = m_program->program;

	thread.newMessage() << "Begin -- glLinkProgram(" << program << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, linkProgram(program));
	thread.newMessage() << "End -- glLinkProgram()" << tcu::ThreadUtil::Message::End;
}

class RenderBuffer : public Operation
{
public:
			RenderBuffer	(SharedPtr<Program> program, SharedPtr<Buffer> buffer, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Program>	m_program;
	SharedPtr<Buffer>	m_buffer;
};

RenderBuffer::RenderBuffer (SharedPtr<Program> program, SharedPtr<Buffer> buffer, bool useSync, bool serverSync)
	: Operation	("RenderBuffer", useSync, serverSync)
	, m_program	(program)
	, m_buffer	(buffer)
{
	readGLObject(SharedPtr<Object>(program));
	readGLObject(SharedPtr<Object>(buffer));
}

void RenderBuffer::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- glClearColor(0.5f, 0.5f, 0.5f, 1.0f)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, clearColor(0.5f, 0.5f, 0.5f, 1.0f));
	thread.newMessage() << "End -- glClearColor()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glClear(GL_COLOR_BUFFER_BIT)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, clear(GL_COLOR_BUFFER_BIT));
	thread.newMessage() << "End -- glClear()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glUseProgram(" << m_program->program << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, useProgram(m_program->program));
	thread.newMessage() << "End -- glUseProgram()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glGetAttribLocation(" << m_program->program << ", \"a_pos\")" << tcu::ThreadUtil::Message::End;
	GLint posLoc = thread.gl.getAttribLocation(m_program->program, "a_pos");
	GLU_CHECK_GLW_MSG(thread.gl, "glGetAttribLocation()");
	thread.newMessage() << "End -- " << posLoc << " = glGetAttribLocation()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glEnableVertexAttribArray(" << posLoc << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, enableVertexAttribArray(posLoc));
	thread.newMessage() << "End -- glEnableVertexAttribArray()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindBuffer(GL_ARRAY_BUFFER, " << m_buffer->buffer << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(GL_ARRAY_BUFFER, m_buffer->buffer));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glVertexAttribPointer(" << posLoc << ", GL_BYTE, GL_TRUE, 0, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, vertexAttribPointer(posLoc, 2, GL_BYTE, GL_TRUE, 0, 0));
	thread.newMessage() << "End -- glVertexAttribPointer()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glDrawArrays(GL_TRIANGLES, 0, " << (m_buffer->size / 2) << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, drawArrays(GL_TRIANGLES, 0, (GLsizei)m_buffer->size / 2));
	thread.newMessage() << "End -- glDrawArrays()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindBuffer(GL_ARRAY_BUFFER, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(GL_ARRAY_BUFFER, 0));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glDisableVertexAttribArray(" << posLoc << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, disableVertexAttribArray(posLoc));
	thread.newMessage() << "End -- glDisableVertexAttribArray()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glUseProgram(0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, useProgram(0));
	thread.newMessage() << "End -- glUseProgram()" << tcu::ThreadUtil::Message::End;
}

class RenderTexture : public Operation
{
public:
			RenderTexture	(SharedPtr<Program> program, SharedPtr<Texture> texture, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Program>	m_program;
	SharedPtr<Texture>	m_texture;
};

RenderTexture::RenderTexture (SharedPtr<Program> program, SharedPtr<Texture> texture, bool useSync, bool serverSync)
	: Operation	("RenderTexture", useSync, serverSync)
	, m_program	(program)
	, m_texture	(texture)
{
	readGLObject(SharedPtr<Object>(program));
	readGLObject(SharedPtr<Object>(texture));
}

void RenderTexture::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- glClearColor(0.5f, 0.5f, 0.5f, 1.0f)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, clearColor(0.5f, 0.5f, 0.5f, 1.0f));
	thread.newMessage() << "End -- glClearColor()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glClear(GL_COLOR_BUFFER_BIT)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, clear(GL_COLOR_BUFFER_BIT));
	thread.newMessage() << "End -- glClear()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glUseProgram(" << m_program->program << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, useProgram(m_program->program));
	thread.newMessage() << "End -- glUseProgram()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, " << m_texture->texture << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, m_texture->texture));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glGetUniformLocation(" << m_program->program << ", \"u_sampler\")" << tcu::ThreadUtil::Message::End;
	GLint samplerPos = thread.gl.getUniformLocation(m_program->program, "u_sampler");
	GLU_CHECK_GLW_MSG(thread.gl, "glGetUniformLocation()");
	thread.newMessage() << "End -- glGetUniformLocation()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glUniform1i(" << samplerPos << ", 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, uniform1i(samplerPos, 0));
	thread.newMessage() << "End -- glUniform1i()" << tcu::ThreadUtil::Message::End;


	thread.newMessage() << "Begin -- glGetAttribLocation(" << m_program->program << ", \"a_pos\")" << tcu::ThreadUtil::Message::End;
	GLint posLoc = thread.gl.getAttribLocation(m_program->program, "a_pos");
	GLU_CHECK_GLW_MSG(thread.gl, "glGetAttribLocation()");
	thread.newMessage() << "End -- " << posLoc << " = glGetAttribLocation()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glEnableVertexAttribArray(" << posLoc << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, enableVertexAttribArray(posLoc));
	thread.newMessage() << "End -- glEnableVertexAttribArray()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindBuffer(GL_ARRAY_BUFFER, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(GL_ARRAY_BUFFER, 0));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;


	float coords[] = {
		-1.0, -1.0,
		 1.0, -1.0,
		 1.0,  1.0,

		 1.0,  1.0,
		-1.0,  1.0,
		-1.0, -1.0
	};

	thread.newMessage() << "Begin -- glVertexAttribPointer(" << posLoc << ", GL_FLOAT, GL_FALSE, 0, <data>)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, vertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, coords));
	thread.newMessage() << "End -- glVertexAttribPointer()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glDrawArrays(GL_TRIANGLES, 0, 6)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, drawArrays(GL_TRIANGLES, 0, 6));
	thread.newMessage() << "End -- glDrawArrays()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindBuffer(GL_ARRAY_BUFFER, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindBuffer(GL_ARRAY_BUFFER, 0));
	thread.newMessage() << "End -- glBindBuffer()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, 0));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glDisableVertexAttribArray(" << posLoc << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, disableVertexAttribArray(posLoc));
	thread.newMessage() << "End -- glDisableVertexAttribArray()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glUseProgram(0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, useProgram(0));
	thread.newMessage() << "End -- glUseProgram()" << tcu::ThreadUtil::Message::End;
}

class ReadPixels : public Operation
{
public:
			ReadPixels		(int x, int y, int width, int height, GLenum format, GLenum type, SharedPtr<tcu::ThreadUtil::DataBlock>& data, bool useSync, bool serverSync);
	void	exec			(tcu::ThreadUtil::Thread& thread);

private:
	int										m_x;
	int										m_y;
	int										m_width;
	int										m_height;
	GLenum									m_format;
	GLenum									m_type;
	SharedPtr<tcu::ThreadUtil::DataBlock>	m_data;
};

ReadPixels::ReadPixels (int x, int y, int width, int height, GLenum format, GLenum type, SharedPtr<tcu::ThreadUtil::DataBlock>& data, bool useSync, bool serverSync)
	: Operation	("ReadPixels", useSync, serverSync)
	, m_x		(x)
	, m_y		(y)
	, m_width	(width)
	, m_height	(height)
	, m_format	(format)
	, m_type	(type)
{
	data = SharedPtr<tcu::ThreadUtil::DataBlock>(new tcu::ThreadUtil::DataBlock(getEvent()));
	m_data = data;
}

void ReadPixels::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	DE_ASSERT(m_type == GL_UNSIGNED_BYTE);
	DE_ASSERT(m_format == GL_RGBA);

	std::vector<deUint8> data((m_width-m_x)*(m_height-m_y)*4);

	thread.newMessage() << "Begin -- glReadPixels(" << m_x << ", " << m_y << ", " << m_width << ", " << m_height << ", " << m_format << ", " << m_type << ", <data>)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, readPixels(m_x, m_y, m_width, m_height, m_format, m_type, &(data[0])));
	thread.newMessage() << "End -- glReadPixels()" << tcu::ThreadUtil::Message::End;

	m_data->setData(data.size(), &(data[0]));
}

class CreateImageFromTexture : public Operation
{
public:
	// \note [mika] Unlike eglCreateImageKHR this operation requires current context and uses it for creating EGLImage
	//				Current context is required to support EGL sync objects in current tests system
			CreateImageFromTexture	(SharedPtr<EGLImage>& image, SharedPtr<Texture> texture, bool useSync, bool serverSync);
	void	exec					(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture>		m_texture;
	SharedPtr<EGLImage>		m_image;
};

CreateImageFromTexture::CreateImageFromTexture (SharedPtr<EGLImage>& image, SharedPtr<Texture> texture, bool useSync, bool serverSync)
	: Operation	("CreateImageFromTexture", useSync, serverSync)
{
	modifyGLObject(SharedPtr<Object>(texture));
	image = SharedPtr<EGLImage>(new EGLImage(getEvent(), getSync()));

	m_image					= image;
	m_texture				= texture;
	m_texture->sourceImage	= m_image;
}

void CreateImageFromTexture::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	EGLint attribList[] = {
		EGL_GL_TEXTURE_LEVEL_KHR, 0,
		EGL_NONE
	};

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, " << m_texture->texture << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, m_texture->texture));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	// Make texture image complete...
	thread.newMessage() << "Begin -- glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	thread.newMessage() << "End -- glTexParameteri()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	thread.newMessage() << "End -- glTexParameteri()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	thread.newMessage() << "End -- glTexParameteri()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	thread.newMessage() << "End -- glTexParameteri()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- eglCreateImageKHR(" << thread.runtimeContext->display << ", " << thread.runtimeContext->context << ", EGL_GL_TEXTURE_2D_KHR, " << m_texture->texture << ", { EGL_GL_TEXTURE_LEVEL_KHR, 0, EGL_NONE })" << tcu::ThreadUtil::Message::End;
	m_image->image = thread.egl.createImageKHR(thread.runtimeContext->display, thread.runtimeContext->context, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(deUintptr)m_texture->texture, attribList);
	EGLU_CHECK_MSG(thread.egl, "eglCreateImageKHR()");
	thread.newMessage() << "End -- " << m_image->image << " = eglCreateImageKHR()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, 0));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;
}

class DestroyImage : public Operation
{
public:
	// \note [mika] Unlike eglDestroyImageKHR this operation requires current context and uses it for creating EGLImage
	//				Current context is required to support EGL sync objects in current tests system
			DestroyImage		(SharedPtr<EGLImage> image, bool useSync, bool serverSync);
	void	exec				(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<EGLImage>		m_image;
};

DestroyImage::DestroyImage (SharedPtr<EGLImage> image, bool useSync, bool serverSync)
	: Operation	("CreateImageFromTexture", useSync, serverSync)
	, m_image	(image)
{
	modifyGLObject(SharedPtr<Object>(image));
}

void DestroyImage::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- eglDestroyImageKHR(" << thread.runtimeContext->display << ", " << m_image->image << ")" << tcu::ThreadUtil::Message::End;
	thread.egl.destroyImageKHR(thread.runtimeContext->display, m_image->image);
	m_image->image = EGL_NO_IMAGE_KHR;
	EGLU_CHECK_MSG(thread.egl, "eglDestroyImageKHR()");
	thread.newMessage() << "End -- eglDestroyImageKHR()" << tcu::ThreadUtil::Message::End;
}

class DefineTextureFromImage : public Operation
{
public:
			DefineTextureFromImage	(SharedPtr<Texture> texture, SharedPtr<EGLImage> image, bool useSync, bool serverSync);
	void	exec					(tcu::ThreadUtil::Thread& thread);

private:
	SharedPtr<Texture>	m_texture;
	SharedPtr<EGLImage>	m_image;
};

DefineTextureFromImage::DefineTextureFromImage (SharedPtr<Texture> texture, SharedPtr<EGLImage> image, bool useSync, bool serverSync)
	: Operation	("DefineTextureFromImage", useSync, serverSync)
{
	readGLObject(SharedPtr<Object>(image));
	modifyGLObject(SharedPtr<Object>(texture));

	texture->isDefined		= true;
	texture->sourceImage	= image;

	m_image		= image;
	m_texture	= texture;
}

void DefineTextureFromImage::exec (tcu::ThreadUtil::Thread& t)
{
	EGLThread& thread = dynamic_cast<EGLThread&>(t);

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, " << m_texture->texture << ")" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, m_texture->texture));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, " << m_image->image << ")" << tcu::ThreadUtil::Message::End;
	thread.runtimeContext->glExtensions.imageTargetTexture2D(GL_TEXTURE_2D, m_image->image);
	GLU_CHECK_GLW_MSG(thread.gl, "glEGLImageTargetTexture2DOES()");
	thread.newMessage() << "End -- glEGLImageTargetTexture2DOES()" << tcu::ThreadUtil::Message::End;

	thread.newMessage() << "Begin -- glBindTexture(GL_TEXTURE_2D, 0)" << tcu::ThreadUtil::Message::End;
	GLU_CHECK_GLW_CALL(thread.gl, bindTexture(GL_TEXTURE_2D, 0));
	thread.newMessage() << "End -- glBindTexture()" << tcu::ThreadUtil::Message::End;
}

} // GLES2ThreadTest

static void requireEGLExtension (const Library& egl, EGLDisplay eglDisplay, const char* requiredExtension)
{
	if (!eglu::hasExtension(egl, eglDisplay, requiredExtension))
		TCU_THROW(NotSupportedError, (string(requiredExtension) + " not supported").c_str());
}

enum OperationId
{
	THREADOPERATIONID_NONE = 0,

	THREADOPERATIONID_CREATE_BUFFER,
	THREADOPERATIONID_DESTROY_BUFFER,
	THREADOPERATIONID_BUFFER_DATA,
	THREADOPERATIONID_BUFFER_SUBDATA,

	THREADOPERATIONID_CREATE_TEXTURE,
	THREADOPERATIONID_DESTROY_TEXTURE,
	THREADOPERATIONID_TEXIMAGE2D,
	THREADOPERATIONID_TEXSUBIMAGE2D,
	THREADOPERATIONID_COPYTEXIMAGE2D,
	THREADOPERATIONID_COPYTEXSUBIMAGE2D,

	THREADOPERATIONID_CREATE_VERTEX_SHADER,
	THREADOPERATIONID_CREATE_FRAGMENT_SHADER,
	THREADOPERATIONID_DESTROY_SHADER,
	THREADOPERATIONID_SHADER_SOURCE,
	THREADOPERATIONID_SHADER_COMPILE,

	THREADOPERATIONID_ATTACH_SHADER,
	THREADOPERATIONID_DETACH_SHADER,

	THREADOPERATIONID_CREATE_PROGRAM,
	THREADOPERATIONID_DESTROY_PROGRAM,
	THREADOPERATIONID_LINK_PROGRAM,

	THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE,
	THREADOPERATIONID_DESTROY_IMAGE,
	THREADOPERATIONID_TEXTURE_FROM_IMAGE,

	THREADOPERATIONID_LAST
};

class GLES2SharingRandomTest : public TestCase
{
public:
	struct TestConfig
	{
				TestConfig		(void);
		int		threadCount;
		int		operationCount;
		bool	serverSync;
		bool	useFenceSync;
		bool	useImages;

		float probabilities[THREADOPERATIONID_LAST][THREADOPERATIONID_LAST];
	};
						GLES2SharingRandomTest		(EglTestContext& context, const TestConfig& config, const char* name, const char* description);
						~GLES2SharingRandomTest		(void);

	void				init						(void);
	void				deinit						(void);
	IterateResult		iterate						(void);

	void				addRandomOperation			(GLES2ThreadTest::EGLResourceManager& resourceManager);

private:
	TestConfig				m_config;
	int						m_seed;
	de::Random				m_random;
	tcu::TestLog&			m_log;
	bool					m_threadsStarted;
	bool					m_threadsRunning;
	bool					m_executionReady;
	bool					m_requiresRestart;
	deUint64				m_beginTimeUs;
	deUint64				m_timeOutUs;
	deUint32				m_sleepTimeMs;
	deUint64				m_timeOutTimeUs;

	std::vector<GLES2ThreadTest::EGLThread*>	m_threads;

	EGLDisplay				m_eglDisplay;
	EGLConfig				m_eglConfig;
	OperationId				m_lastOperation;

	glw::Functions			m_gl;
};

GLES2SharingRandomTest::TestConfig::TestConfig (void)
	: threadCount		(0)
	, operationCount	(0)
	, serverSync		(false)
	, useFenceSync		(false)
	, useImages			(false)
{
	deMemset(probabilities, 0, sizeof(probabilities));
}

GLES2SharingRandomTest::GLES2SharingRandomTest (EglTestContext& context, const TestConfig& config, const char* name, const char* description)
	: TestCase			(context, name, description)
	, m_config			(config)
	, m_seed			(deStringHash(name))
	, m_random			(deStringHash(name))
	, m_log				(m_testCtx.getLog())
	, m_threadsStarted	(false)
	, m_threadsRunning	(false)
	, m_executionReady	(false)
	, m_requiresRestart	(false)
	, m_beginTimeUs		(0)
	, m_timeOutUs		(10000000)	// 10 seconds
	, m_sleepTimeMs		(1)		// 1 milliseconds
	, m_timeOutTimeUs	(0)
	, m_eglDisplay		(EGL_NO_DISPLAY)
	, m_eglConfig		(0)
	, m_lastOperation	(THREADOPERATIONID_NONE)
{
}

GLES2SharingRandomTest::~GLES2SharingRandomTest (void)
{
	GLES2SharingRandomTest::deinit();
}

void GLES2SharingRandomTest::init (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	const EGLint attribList[] =
	{
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_ALPHA_SIZE,			1,
		EGL_NONE
	};

	m_eglDisplay	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_eglConfig		= eglu::chooseSingleConfig(egl, m_eglDisplay, attribList);

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));

	// Check extensions
	if (m_config.useFenceSync)
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_fence_sync");

	if (m_config.serverSync)
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_wait_sync");

	if (m_config.useImages)
	{
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_image_base");
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_gl_texture_2D_image");
	}

	GLES2ThreadTest::EGLResourceManager resourceManager;
	// Create contexts
	for (int threadNdx = 0; threadNdx < m_config.threadCount; threadNdx++)
	{
		m_threads.push_back(new GLES2ThreadTest::EGLThread(egl, m_gl, deInt32Hash(m_seed+threadNdx)));
		SharedPtr<GLES2ThreadTest::GLES2Context> context;
		SharedPtr<GLES2ThreadTest::GLES2Context> shared = (threadNdx > 0 ? resourceManager.popContext(0) : SharedPtr<GLES2ThreadTest::GLES2Context>());
		m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreateContext(m_eglDisplay, m_eglConfig, shared, context));

		resourceManager.addContext(context);

		if (shared)
			resourceManager.addContext(shared);
	}

	// Create surfaces
	for (int threadNdx = 0; threadNdx < m_config.threadCount; threadNdx++)
	{
		SharedPtr<GLES2ThreadTest::Surface> surface;
		m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreatePBufferSurface(m_eglDisplay, m_eglConfig, 400, 400, surface));
		resourceManager.addSurface(surface);
	}

	// Make contexts current
	for (int threadNdx = 0; threadNdx < m_config.threadCount; threadNdx++)
	{
		m_threads[threadNdx]->addOperation(new GLES2ThreadTest::MakeCurrent(*m_threads[threadNdx], m_eglDisplay, resourceManager.popSurface(0), resourceManager.popContext(0)));
	}

	// Operations to check fence sync support
	if (m_config.useFenceSync)
	{
		for (int threadNdx = 0; threadNdx < m_config.threadCount; threadNdx++)
		{
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::InitGLExtension("GL_OES_EGL_sync"));
		}
	}

	// Init EGLimage support
	if (m_config.useImages)
	{
		for (int threadNdx = 0; threadNdx < m_config.threadCount; threadNdx++)
		{
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::InitGLExtension("GL_OES_EGL_image"));
		}
	}

	// Add random operations
	for (int operationNdx = 0; operationNdx < m_config.operationCount; operationNdx++)
		addRandomOperation(resourceManager);

	{
		int threadNdx  = 0;

		// Destroy images
		// \note Android reference counts EGLDisplays so we can't trust the eglTerminate() to clean up resources
		while (resourceManager.getImageCount() > 0)
		{
			const SharedPtr<GLES2ThreadTest::EGLImage> image = resourceManager.popImage(0);

			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DestroyImage(image, m_config.useFenceSync, m_config.serverSync));

			threadNdx = (threadNdx + 1) % m_config.threadCount;
		}
	}

	// Release contexts
	for (int threadNdx = 0; threadNdx < m_config.threadCount; threadNdx++)
	{
		SharedPtr<GLES2ThreadTest::GLES2Context>	context = m_threads[threadNdx]->context;
		SharedPtr<GLES2ThreadTest::Surface>			surface = m_threads[threadNdx]->surface;

		m_threads[threadNdx]->addOperation(new GLES2ThreadTest::MakeCurrent(*m_threads[threadNdx], m_eglDisplay, SharedPtr<GLES2ThreadTest::Surface>(), SharedPtr<GLES2ThreadTest::GLES2Context>()));

		resourceManager.addSurface(surface);
		resourceManager.addContext(context);
	}

	// Destroy contexts
	for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DestroyContext(resourceManager.popContext(0)));

	// Destroy surfaces
	for (int threadNdx = 0; threadNdx < m_config.threadCount; threadNdx++)
		m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DestroySurface(m_eglDisplay, resourceManager.popSurface(0)));
}

void GLES2SharingRandomTest::deinit (void)
{
	for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
	{
		delete m_threads[threadNdx];
		m_threads[threadNdx] = DE_NULL;
	}

	m_threads.clear();

	if (m_eglDisplay != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_eglDisplay);
		m_eglDisplay = EGL_NO_DISPLAY;
	}

	TCU_CHECK(!m_requiresRestart);
}

void GLES2SharingRandomTest::addRandomOperation (GLES2ThreadTest::EGLResourceManager& resourceManager)
{
	int threadNdx	= m_random.getUint32() % (deUint32)m_threads.size();

	std::vector<OperationId>	operations;
	std::vector<float>			weights;

	operations.push_back(THREADOPERATIONID_CREATE_BUFFER);
	weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_CREATE_BUFFER]);

	operations.push_back(THREADOPERATIONID_CREATE_TEXTURE);
	weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_CREATE_TEXTURE]);

	operations.push_back(THREADOPERATIONID_CREATE_VERTEX_SHADER);
	weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_CREATE_VERTEX_SHADER]);

	operations.push_back(THREADOPERATIONID_CREATE_FRAGMENT_SHADER);
	weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]);

	operations.push_back(THREADOPERATIONID_CREATE_PROGRAM);
	weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_CREATE_PROGRAM]);

	int destroyableBufferNdx				= -1;
	int destroyableTextureNdx				= -1;
	int destroyableShaderNdx				= -1;
	int destroyableProgramNdx				= -1;

	int vertexShaderNdx						= -1;
	int fragmentShaderNdx					= -1;

	int definedTextureNdx					= -1;

	int definedBufferNdx					= -1;

	int definedShaderNdx					= -1;

	int detachableProgramNdx				= -1;
	GLenum detachShaderType					= GL_VERTEX_SHADER;

	int unusedVertexAttachmentProgramNdx	= -1;
	int unusedFragmentAttachmentProgramNdx	= -1;

	int linkableProgramNdx					= -1;

	int attachProgramNdx					= -1;
	int attachShaderNdx						= -1;

	int nonSiblingTextureNdx				= -1;

	if (m_threads[threadNdx]->context->resourceManager->getBufferCount() > 0)
		destroyableBufferNdx = m_random.getUint32() % m_threads[threadNdx]->context->resourceManager->getBufferCount();

	if (m_threads[threadNdx]->context->resourceManager->getTextureCount() > 0)
		destroyableTextureNdx = m_random.getUint32() % m_threads[threadNdx]->context->resourceManager->getTextureCount();

	if (m_threads[threadNdx]->context->resourceManager->getShaderCount() > 0)
		destroyableShaderNdx = m_random.getUint32() % m_threads[threadNdx]->context->resourceManager->getShaderCount();

	if (m_threads[threadNdx]->context->resourceManager->getProgramCount() > 0)
		destroyableProgramNdx = m_random.getUint32() % m_threads[threadNdx]->context->resourceManager->getProgramCount();

	// Check what kind of buffers we have
	for (int bufferNdx = 0; bufferNdx < m_threads[threadNdx]->context->resourceManager->getBufferCount(); bufferNdx++)
	{
		SharedPtr<GLES2ThreadTest::Buffer> buffer = m_threads[threadNdx]->context->resourceManager->getBuffer(bufferNdx);

		if (buffer->isDefined)
		{
			if (definedBufferNdx == -1)
				definedBufferNdx = bufferNdx;
			else if (m_random.getBool())
				definedBufferNdx = bufferNdx;
		}
	}

	// Check what kind of textures we have
	for (int textureNdx = 0; textureNdx < m_threads[threadNdx]->context->resourceManager->getTextureCount(); textureNdx++)
	{
		SharedPtr<GLES2ThreadTest::Texture> texture = m_threads[threadNdx]->context->resourceManager->getTexture(textureNdx);

		if (texture->isDefined)
		{
			if (definedTextureNdx == -1)
				definedTextureNdx = textureNdx;
			else if (m_random.getBool())
				definedTextureNdx = textureNdx;

			if (!texture->sourceImage)
			{
				if (nonSiblingTextureNdx == -1)
					nonSiblingTextureNdx = textureNdx;
				else if (m_random.getBool())
					nonSiblingTextureNdx = textureNdx;
			}
		}

	}

	// Check what kind of shaders we have
	for (int shaderNdx = 0; shaderNdx < m_threads[threadNdx]->context->resourceManager->getShaderCount(); shaderNdx++)
	{
		SharedPtr<GLES2ThreadTest::Shader> shader = m_threads[threadNdx]->context->resourceManager->getShader(shaderNdx);

		// Defined shader found
		if (shader->isDefined)
		{
			if (definedShaderNdx == -1)
				definedShaderNdx = shaderNdx;
			else if (m_random.getBool())
				definedShaderNdx = shaderNdx;
		}

		// Vertex shader found
		if (shader->type == GL_VERTEX_SHADER)
		{
			if (vertexShaderNdx == -1)
				vertexShaderNdx = shaderNdx;
			else if (m_random.getBool())
				vertexShaderNdx = shaderNdx;
		}

		// Fragmet shader found
		if (shader->type == GL_FRAGMENT_SHADER)
		{
			if (fragmentShaderNdx == -1)
				fragmentShaderNdx = shaderNdx;
			else if (m_random.getBool())
				fragmentShaderNdx = shaderNdx;
		}
	}

	// Check what kind of programs we have
	for (int programNdx = 0; programNdx < m_threads[threadNdx]->context->resourceManager->getProgramCount(); programNdx++)
	{
		SharedPtr<GLES2ThreadTest::Program> program = m_threads[threadNdx]->context->resourceManager->getProgram(programNdx);

		// Program that can be detached
		if (program->vertexShader || program->fragmentShader)
		{
			if (detachableProgramNdx == -1)
			{
				detachableProgramNdx = programNdx;

				if (program->vertexShader)
					detachShaderType = GL_VERTEX_SHADER;
				else if (program->fragmentShader)
					detachShaderType = GL_FRAGMENT_SHADER;
				else
					DE_ASSERT(false);
			}
			else if (m_random.getBool())
			{
				detachableProgramNdx = programNdx;

				if (program->vertexShader)
					detachShaderType = GL_VERTEX_SHADER;
				else if (program->fragmentShader)
					detachShaderType = GL_FRAGMENT_SHADER;
				else
					DE_ASSERT(false);
			}
		}

		// Program that can be attached vertex shader
		if (!program->vertexShader)
		{
			if (unusedVertexAttachmentProgramNdx == -1)
				unusedVertexAttachmentProgramNdx = programNdx;
			else if (m_random.getBool())
				unusedVertexAttachmentProgramNdx = programNdx;
		}

		// Program that can be attached fragment shader
		if (!program->fragmentShader)
		{
			if (unusedFragmentAttachmentProgramNdx == -1)
				unusedFragmentAttachmentProgramNdx = programNdx;
			else if (m_random.getBool())
				unusedFragmentAttachmentProgramNdx = programNdx;
		}

		// Program that can be linked
		if (program->vertexShader && program->fragmentShader)
		{
			if (linkableProgramNdx == -1)
				linkableProgramNdx = programNdx;
			else if (m_random.getBool())
				linkableProgramNdx = programNdx;
		}
	}

	// Has images
	if (resourceManager.getImageCount() > 0)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_DESTROY_IMAGE]);
		operations.push_back(THREADOPERATIONID_DESTROY_IMAGE);

		if (m_threads[threadNdx]->context->resourceManager->getTextureCount() > 0)
		{
			weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_TEXTURE_FROM_IMAGE]);
			operations.push_back(THREADOPERATIONID_TEXTURE_FROM_IMAGE);
		}
	}

	// Has buffer
	if (destroyableBufferNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_DESTROY_BUFFER]);
		operations.push_back(THREADOPERATIONID_DESTROY_BUFFER);

		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_BUFFER_DATA]);
		operations.push_back(THREADOPERATIONID_BUFFER_DATA);
	}

	// Has buffer with defined data
	if (definedBufferNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_BUFFER_SUBDATA]);
		operations.push_back(THREADOPERATIONID_BUFFER_SUBDATA);
	}

	// Has texture
	if (destroyableTextureNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_DESTROY_TEXTURE]);
		operations.push_back(THREADOPERATIONID_DESTROY_TEXTURE);

		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_TEXIMAGE2D]);
		operations.push_back(THREADOPERATIONID_TEXIMAGE2D);

		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_COPYTEXIMAGE2D]);
		operations.push_back(THREADOPERATIONID_COPYTEXIMAGE2D);
	}

	// Has texture with data
	if (definedTextureNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_TEXSUBIMAGE2D]);
		operations.push_back(THREADOPERATIONID_TEXSUBIMAGE2D);

		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_COPYTEXSUBIMAGE2D]);
		operations.push_back(THREADOPERATIONID_COPYTEXSUBIMAGE2D);
	}

	// Has texture that can be used as EGLimage source
	if (nonSiblingTextureNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]);
		operations.push_back(THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE);
	}

	// Has shader
	if (destroyableShaderNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_DESTROY_SHADER]);
		operations.push_back(THREADOPERATIONID_DESTROY_SHADER);

		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_SHADER_SOURCE]);
		operations.push_back(THREADOPERATIONID_SHADER_SOURCE);
	}

	// Has shader with defined source
	if (definedShaderNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_SHADER_COMPILE]);
		operations.push_back(THREADOPERATIONID_SHADER_COMPILE);
	}

	// Has program
	if (destroyableProgramNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_DESTROY_PROGRAM]);
		operations.push_back(THREADOPERATIONID_DESTROY_PROGRAM);
	}

	// Has program that can be linked
	if (linkableProgramNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_LINK_PROGRAM]);
		operations.push_back(THREADOPERATIONID_LINK_PROGRAM);
	}

	// has program with attachments
	if (detachableProgramNdx != -1)
	{
		weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_DETACH_SHADER]);
		operations.push_back(THREADOPERATIONID_DETACH_SHADER);
	}

	// Has program and shader pair that can be attached
	if (fragmentShaderNdx != -1 && unusedFragmentAttachmentProgramNdx != -1)
	{
		if (attachProgramNdx == -1)
		{
			DE_ASSERT(attachShaderNdx == -1);
			attachProgramNdx = unusedFragmentAttachmentProgramNdx;
			attachShaderNdx = fragmentShaderNdx;

			weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_ATTACH_SHADER]);
			operations.push_back(THREADOPERATIONID_ATTACH_SHADER);
		}
		else if (m_random.getBool())
		{
			attachProgramNdx = unusedFragmentAttachmentProgramNdx;
			attachShaderNdx = fragmentShaderNdx;
		}
	}

	if (vertexShaderNdx != -1 && unusedVertexAttachmentProgramNdx != -1)
	{
		if (attachProgramNdx == -1)
		{
			DE_ASSERT(attachShaderNdx == -1);
			attachProgramNdx = unusedVertexAttachmentProgramNdx;
			attachShaderNdx = vertexShaderNdx;

			weights.push_back(m_config.probabilities[m_lastOperation][THREADOPERATIONID_ATTACH_SHADER]);
			operations.push_back(THREADOPERATIONID_ATTACH_SHADER);
		}
		else if (m_random.getBool())
		{
			attachProgramNdx = unusedVertexAttachmentProgramNdx;
			attachShaderNdx = vertexShaderNdx;
		}
	}

	OperationId op = m_random.chooseWeighted<OperationId, std::vector<OperationId> ::iterator>(operations.begin(), operations.end(), weights.begin());

	switch (op)
	{
		case THREADOPERATIONID_CREATE_BUFFER:
		{
			SharedPtr<GLES2ThreadTest::Buffer> buffer;
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreateBuffer(buffer, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addBuffer(buffer);
			break;
		}

		case THREADOPERATIONID_DESTROY_BUFFER:
		{
			SharedPtr<GLES2ThreadTest::Buffer> buffer = m_threads[threadNdx]->context->resourceManager->popBuffer(destroyableBufferNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DeleteBuffer(buffer, m_config.useFenceSync, m_config.serverSync));
			break;
		}

		case THREADOPERATIONID_BUFFER_DATA:
		{
			SharedPtr<GLES2ThreadTest::Buffer> buffer = m_threads[threadNdx]->context->resourceManager->popBuffer(destroyableBufferNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::BufferData(buffer, GL_ARRAY_BUFFER, 1024, GL_DYNAMIC_DRAW, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addBuffer(buffer);
			break;
		}

		case THREADOPERATIONID_BUFFER_SUBDATA:
		{
			SharedPtr<GLES2ThreadTest::Buffer> buffer = m_threads[threadNdx]->context->resourceManager->popBuffer(definedBufferNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::BufferSubData(buffer, GL_ARRAY_BUFFER, 1, 20, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addBuffer(buffer);
			break;
		}

		case THREADOPERATIONID_CREATE_TEXTURE:
		{
			SharedPtr<GLES2ThreadTest::Texture> texture;
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreateTexture(texture, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addTexture(texture);
			break;
		}

		case THREADOPERATIONID_DESTROY_TEXTURE:
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DeleteTexture(m_threads[threadNdx]->context->resourceManager->popTexture(destroyableTextureNdx), m_config.useFenceSync, m_config.serverSync));
			break;

		case THREADOPERATIONID_TEXIMAGE2D:
		{
			SharedPtr<GLES2ThreadTest::Texture> texture = m_threads[threadNdx]->context->resourceManager->popTexture(destroyableTextureNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::TexImage2D(texture, 0, GL_RGBA, 400, 400, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addTexture(texture);
			break;
		}

		case THREADOPERATIONID_TEXSUBIMAGE2D:
		{
			SharedPtr<GLES2ThreadTest::Texture> texture = m_threads[threadNdx]->context->resourceManager->popTexture(definedTextureNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::TexSubImage2D(texture, 0, 30, 30, 50, 50, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addTexture(texture);
			break;
		}

		case THREADOPERATIONID_COPYTEXIMAGE2D:
		{
			SharedPtr<GLES2ThreadTest::Texture> texture = m_threads[threadNdx]->context->resourceManager->popTexture(destroyableTextureNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CopyTexImage2D(texture, 0, GL_RGBA, 20, 20, 300, 300, 0, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addTexture(texture);
			break;
		}

		case THREADOPERATIONID_COPYTEXSUBIMAGE2D:
		{
			SharedPtr<GLES2ThreadTest::Texture> texture = m_threads[threadNdx]->context->resourceManager->popTexture(definedTextureNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CopyTexSubImage2D(texture, 0, 10, 10, 30, 30, 50, 50, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addTexture(texture);
			break;
		}

		case THREADOPERATIONID_CREATE_VERTEX_SHADER:
		{
			SharedPtr<GLES2ThreadTest::Shader> shader;
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreateShader(GL_VERTEX_SHADER, shader, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addShader(shader);
			break;
		}

		case THREADOPERATIONID_CREATE_FRAGMENT_SHADER:
		{
			SharedPtr<GLES2ThreadTest::Shader> shader;
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreateShader(GL_FRAGMENT_SHADER, shader, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addShader(shader);
			break;
		}

		case THREADOPERATIONID_DESTROY_SHADER:
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DeleteShader(m_threads[threadNdx]->context->resourceManager->popShader(destroyableShaderNdx), m_config.useFenceSync, m_config.serverSync));
			break;

		case THREADOPERATIONID_SHADER_SOURCE:
		{
			const char* vertexShaderSource =
				"attribute mediump vec4 a_pos;\n"
				"varying mediump vec4 v_pos;\n"
				"void main (void)\n"
				"{\n"
				"\tv_pos = a_pos;\n"
				"\tgl_Position = a_pos;\n"
				"}\n";

			const char* fragmentShaderSource =
				"varying mediump vec4 v_pos;\n"
				"void main (void)\n"
				"{\n"
				"\tgl_FragColor = v_pos;\n"
				"}\n";
			SharedPtr<GLES2ThreadTest::Shader> shader = m_threads[threadNdx]->context->resourceManager->popShader(destroyableShaderNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::ShaderSource(shader, (shader->type == GL_VERTEX_SHADER ? vertexShaderSource : fragmentShaderSource), m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addShader(shader);
			break;
		}

		case THREADOPERATIONID_SHADER_COMPILE:
		{
			SharedPtr<GLES2ThreadTest::Shader> shader = m_threads[threadNdx]->context->resourceManager->popShader(definedShaderNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::ShaderCompile(shader, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addShader(shader);
			break;
		}

		case THREADOPERATIONID_CREATE_PROGRAM:
		{
			SharedPtr<GLES2ThreadTest::Program> program;
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreateProgram(program, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addProgram(program);
			break;
		}

		case THREADOPERATIONID_DESTROY_PROGRAM:
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DeleteProgram(m_threads[threadNdx]->context->resourceManager->popProgram(destroyableProgramNdx), m_config.useFenceSync, m_config.serverSync));
			break;

		case THREADOPERATIONID_ATTACH_SHADER:
		{
			SharedPtr<GLES2ThreadTest::Program>	program = m_threads[threadNdx]->context->resourceManager->popProgram(attachProgramNdx);
			SharedPtr<GLES2ThreadTest::Shader>	shader	= m_threads[threadNdx]->context->resourceManager->popShader(attachShaderNdx);

			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::AttachShader(program, shader, m_config.useFenceSync, m_config.serverSync));

			m_threads[threadNdx]->context->resourceManager->addProgram(program);
			m_threads[threadNdx]->context->resourceManager->addShader(shader);
			break;
		}

		case THREADOPERATIONID_DETACH_SHADER:
		{
			SharedPtr<GLES2ThreadTest::Program>	program = m_threads[threadNdx]->context->resourceManager->popProgram(detachableProgramNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DetachShader(program, detachShaderType, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addProgram(program);
			break;
		}

		case THREADOPERATIONID_LINK_PROGRAM:
		{
			SharedPtr<GLES2ThreadTest::Program>	program = m_threads[threadNdx]->context->resourceManager->popProgram(linkableProgramNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::LinkProgram(program, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addProgram(program);
			break;
		}

		case THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE:
		{
			SharedPtr<GLES2ThreadTest::EGLImage> image;
			SharedPtr<GLES2ThreadTest::Texture> texture = m_threads[threadNdx]->context->resourceManager->popTexture(nonSiblingTextureNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::CreateImageFromTexture(image, texture, m_config.useFenceSync, m_config.serverSync));
			// \note [mika] Can source be added back to resourceManager?
			m_threads[threadNdx]->context->resourceManager->addTexture(texture);
			resourceManager.addImage(image);
			break;
		}

		case THREADOPERATIONID_DESTROY_IMAGE:
		{
			int imageNdx = m_random.getInt(0, resourceManager.getImageCount()-1);
			SharedPtr<GLES2ThreadTest::EGLImage> image = resourceManager.popImage(imageNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DestroyImage(image, m_config.useFenceSync, m_config.serverSync));
			break;
		}

		case THREADOPERATIONID_TEXTURE_FROM_IMAGE:
		{
			int imageNdx = m_random.getInt(0, resourceManager.getImageCount()-1);
			SharedPtr<GLES2ThreadTest::Texture> texture = m_threads[threadNdx]->context->resourceManager->popTexture(destroyableTextureNdx);
			SharedPtr<GLES2ThreadTest::EGLImage> image = resourceManager.popImage(imageNdx);
			m_threads[threadNdx]->addOperation(new GLES2ThreadTest::DefineTextureFromImage(texture, image, m_config.useFenceSync, m_config.serverSync));
			m_threads[threadNdx]->context->resourceManager->addTexture(texture);
			resourceManager.addImage(image);
			break;
		}

		default:
			DE_ASSERT(false);
	}

	m_lastOperation = op;
}

tcu::TestCase::IterateResult GLES2SharingRandomTest::iterate (void)
{
	if (!m_threadsStarted)
	{
		m_beginTimeUs = deGetMicroseconds();

		// Execute threads
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			m_threads[threadNdx]->exec();

		m_threadsStarted = true;
		m_threadsRunning = true;
	}

	if (m_threadsRunning)
	{
		// Wait threads to finish
		int readyThreads = 0;
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		{
			const tcu::ThreadUtil::Thread::ThreadStatus status = m_threads[threadNdx]->getStatus();

			if (status != tcu::ThreadUtil::Thread::THREADSTATUS_RUNNING && status != tcu::ThreadUtil::Thread::THREADSTATUS_NOT_STARTED)
				readyThreads++;
		}

		if (readyThreads == (int)m_threads.size())
		{
			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
				m_threads[threadNdx]->join();

			m_executionReady	= true;
			m_requiresRestart	= false;
		}

		if (deGetMicroseconds() - m_beginTimeUs > m_timeOutUs)
		{
			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			{
				if (m_threads[threadNdx]->getStatus() != tcu::ThreadUtil::Thread::THREADSTATUS_RUNNING)
				{
					if (m_threads[threadNdx]->isStarted())
						m_threads[threadNdx]->join();
				}
			}
			m_executionReady	= true;
			m_requiresRestart	= true;
			m_timeOutTimeUs		= deGetMicroseconds();
		}
		else
		{
			deSleep(m_sleepTimeMs);
		}
	}

	if (m_executionReady)
	{
		std::vector<int> indices(m_threads.size(), 0);

		if (m_timeOutTimeUs != 0)
			m_log << tcu::TestLog::Message << "Execution timeout limit reached. Trying to get per thread logs. This is potentially dangerous." << tcu::TestLog::EndMessage;

		while (true)
		{
			int			firstThread = -1;

			// Find first thread with messages
			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			{
				if (m_threads[threadNdx]->getMessageCount() > indices[threadNdx])
				{
					firstThread = threadNdx;
					break;
				}
			}

			// No more messages
			if (firstThread == -1)
				break;

			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			{
				// No more messages in this thread
				if (m_threads[threadNdx]->getMessageCount() <= indices[threadNdx])
					continue;

				if ((m_threads[threadNdx]->getMessage(indices[threadNdx]).getTime() - m_beginTimeUs) < (m_threads[firstThread]->getMessage(indices[firstThread]).getTime() - m_beginTimeUs))
					firstThread = threadNdx;
			}

			tcu::ThreadUtil::Message message = m_threads[firstThread]->getMessage(indices[firstThread]);

			m_log << tcu::TestLog::Message << "[" << (message.getTime() - m_beginTimeUs) << "] (" << firstThread << ") " << message.getMessage() << tcu::TestLog::EndMessage;
			indices[firstThread]++;
		}

		if (m_timeOutTimeUs != 0)
			m_log << tcu::TestLog::Message << "[" << (m_timeOutTimeUs - m_beginTimeUs) << "] Execution timeout limit reached" << tcu::TestLog::EndMessage;

		bool isOk = true;
		bool notSupported = false;

		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		{
			const tcu::ThreadUtil::Thread::ThreadStatus status = m_threads[threadNdx]->getStatus();

			switch (status)
			{
				case tcu::ThreadUtil::Thread::THREADSTATUS_FAILED:
				case tcu::ThreadUtil::Thread::THREADSTATUS_INIT_FAILED:
				case tcu::ThreadUtil::Thread::THREADSTATUS_RUNNING:
					isOk = false;
					break;

				case tcu::ThreadUtil::Thread::THREADSTATUS_NOT_SUPPORTED:
					notSupported = true;
					break;

				case tcu::ThreadUtil::Thread::THREADSTATUS_READY:
					// Nothing
					break;

				default:
					DE_ASSERT(false);
					isOk = false;
			};
		}

		if (notSupported)
			throw tcu::NotSupportedError("Thread threw tcu::NotSupportedError", "", __FILE__, __LINE__);

		if (isOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		return STOP;
	}

	return CONTINUE;
}

class GLES2ThreadedSharingTest : public TestCase
{
public:
	struct TestConfig
	{
		enum ResourceType
		{
			RESOURCETYPE_BUFFER = 0,
			RESOURCETYPE_TEXTURE,
			RESOURCETYPE_VERTEX_SHADER,
			RESOURCETYPE_FRAGMENT_SHADER,
			RESOURCETYPE_PROGRAM,
			RESOURCETYPE_IMAGE
		};

		ResourceType	resourceType;
		bool			singleContext;
		int				define;
		int				modify;
		bool			useFenceSync;
		bool			serverSync;
		bool			render;
	};
						GLES2ThreadedSharingTest	(EglTestContext& context, const TestConfig& config, const char* name, const char* description);
						~GLES2ThreadedSharingTest	(void);

	void				init						(void);
	void				deinit						(void);
	IterateResult		iterate						(void);

	void				addBufferOperations			(void);
	void				addTextureOperations		(void);
	void				addImageOperations			(void);
	void				addShaderOperations			(GLenum type);
	void				addProgramOperations		(void);

private:
	TestConfig				m_config;
	tcu::TestLog&			m_log;
	int						m_seed;
	bool					m_threadsStarted;
	bool					m_threadsRunning;
	bool					m_executionReady;
	bool					m_requiresRestart;
	deUint64				m_beginTimeUs;
	deUint64				m_timeOutUs;
	deUint32				m_sleepTimeMs;
	deUint64				m_timeOutTimeUs;

	std::vector<GLES2ThreadTest::EGLThread*>	m_threads;

	EGLDisplay				m_eglDisplay;
	EGLConfig				m_eglConfig;
	glw::Functions			m_gl;
};

GLES2ThreadedSharingTest::GLES2ThreadedSharingTest (EglTestContext& context, const TestConfig& config, const char* name, const char* description)
	: TestCase			(context, name, description)
	, m_config			(config)
	, m_log				(m_testCtx.getLog())
	, m_seed			(deStringHash(name))
	, m_threadsStarted	(false)
	, m_threadsRunning	(false)
	, m_executionReady	(false)
	, m_requiresRestart	(false)
	, m_beginTimeUs		(0)
	, m_timeOutUs		(10000000)	// 10 seconds
	, m_sleepTimeMs		(1)			// 1 milliseconds
	, m_timeOutTimeUs	(0)
	, m_eglDisplay		(EGL_NO_DISPLAY)
	, m_eglConfig		(0)
{
}

GLES2ThreadedSharingTest::~GLES2ThreadedSharingTest (void)
{
	GLES2ThreadedSharingTest::deinit();
}

void GLES2ThreadedSharingTest::init (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	const EGLint attribList[] =
	{
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_ALPHA_SIZE,			1,
		EGL_NONE
	};

	m_eglDisplay	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_eglConfig		= eglu::chooseSingleConfig(egl, m_eglDisplay, attribList);

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));

	// Check extensions
	if (m_config.useFenceSync)
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_fence_sync");

	if (m_config.serverSync)
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_wait_sync");

	if (m_config.resourceType == TestConfig::RESOURCETYPE_IMAGE)
	{
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_image_base");
		requireEGLExtension(egl, m_eglDisplay, "EGL_KHR_gl_texture_2D_image");
	}

	// Create threads
	m_threads.push_back(new GLES2ThreadTest::EGLThread(egl, m_gl, deInt32Hash(m_seed)));
	m_threads.push_back(new GLES2ThreadTest::EGLThread(egl, m_gl, deInt32Hash(m_seed*200)));

	SharedPtr<GLES2ThreadTest::GLES2Context> contex1;
	SharedPtr<GLES2ThreadTest::GLES2Context> contex2;

	SharedPtr<GLES2ThreadTest::Surface> surface1;
	SharedPtr<GLES2ThreadTest::Surface> surface2;

	// Create contexts
	m_threads[0]->addOperation(new GLES2ThreadTest::CreateContext(m_eglDisplay, m_eglConfig, SharedPtr<GLES2ThreadTest::GLES2Context>(), contex1));
	m_threads[1]->addOperation(new GLES2ThreadTest::CreateContext(m_eglDisplay, m_eglConfig, contex1, contex2));

	// Create surfaces
	m_threads[0]->addOperation(new GLES2ThreadTest::CreatePBufferSurface(m_eglDisplay, m_eglConfig, 400, 400, surface1));
	m_threads[1]->addOperation(new GLES2ThreadTest::CreatePBufferSurface(m_eglDisplay, m_eglConfig, 400, 400, surface2));

	// Make current contexts
	m_threads[0]->addOperation(new GLES2ThreadTest::MakeCurrent(*m_threads[0], m_eglDisplay, surface1, contex1));
	m_threads[1]->addOperation(new GLES2ThreadTest::MakeCurrent(*m_threads[1], m_eglDisplay, surface2, contex2));
	// Operations to check fence sync support
	if (m_config.useFenceSync)
	{
		m_threads[0]->addOperation(new GLES2ThreadTest::InitGLExtension("GL_OES_EGL_sync"));
		m_threads[1]->addOperation(new GLES2ThreadTest::InitGLExtension("GL_OES_EGL_sync"));
	}


	switch (m_config.resourceType)
	{
		case TestConfig::RESOURCETYPE_BUFFER:
			addBufferOperations();
			break;

		case TestConfig::RESOURCETYPE_TEXTURE:
			addTextureOperations();
			break;

		case TestConfig::RESOURCETYPE_IMAGE:
			addImageOperations();
			break;

		case TestConfig::RESOURCETYPE_VERTEX_SHADER:
			addShaderOperations(GL_VERTEX_SHADER);
			break;

		case TestConfig::RESOURCETYPE_FRAGMENT_SHADER:
			addShaderOperations(GL_FRAGMENT_SHADER);
			break;

		case TestConfig::RESOURCETYPE_PROGRAM:
			addProgramOperations();
			break;

		default:
			DE_ASSERT(false);
	}

	// Relaese contexts
	m_threads[0]->addOperation(new GLES2ThreadTest::MakeCurrent(*m_threads[0], m_eglDisplay, SharedPtr<GLES2ThreadTest::Surface>(), SharedPtr<GLES2ThreadTest::GLES2Context>()));
	m_threads[1]->addOperation(new GLES2ThreadTest::MakeCurrent(*m_threads[0], m_eglDisplay, SharedPtr<GLES2ThreadTest::Surface>(), SharedPtr<GLES2ThreadTest::GLES2Context>()));

	// Destory context
	m_threads[0]->addOperation(new GLES2ThreadTest::DestroyContext(contex1));
	m_threads[1]->addOperation(new GLES2ThreadTest::DestroyContext(contex2));

	// Destroy surfaces
	m_threads[0]->addOperation(new GLES2ThreadTest::DestroySurface(m_eglDisplay, surface1));
	m_threads[1]->addOperation(new GLES2ThreadTest::DestroySurface(m_eglDisplay, surface2));
}

void GLES2ThreadedSharingTest::addBufferOperations (void)
{
	// Add operations for verify
	SharedPtr<GLES2ThreadTest::Shader>	vertexShader;
	SharedPtr<GLES2ThreadTest::Shader>	fragmentShader;
	SharedPtr<GLES2ThreadTest::Program>	program;

	if (m_config.render)
	{
		const char* vertexShaderSource =
		"attribute highp vec2 a_pos;\n"
		"varying mediump vec2 v_pos;\n"
		"void main(void)\n"
		"{\n"
		"\tv_pos = a_pos;\n"
		"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
		"}\n";

		const char* fragmentShaderSource =
		"varying mediump vec2 v_pos;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = vec4(v_pos, 0.5, 1.0);\n"
		"}\n";

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_VERTEX_SHADER, vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(vertexShader, vertexShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(vertexShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_FRAGMENT_SHADER, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(fragmentShader, fragmentShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(fragmentShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateProgram(program, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, vertexShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::LinkProgram(program, m_config.useFenceSync, m_config.serverSync));
	}

	SharedPtr<GLES2ThreadTest::Buffer> buffer;

	m_threads[0]->addOperation(new GLES2ThreadTest::CreateBuffer(buffer, m_config.useFenceSync, m_config.serverSync));

	if (m_config.define)
	{
		if (m_config.modify || m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::BufferData(buffer, GL_ARRAY_BUFFER, 1024, GL_DYNAMIC_DRAW, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::BufferData(buffer, GL_ARRAY_BUFFER, 1024, GL_DYNAMIC_DRAW, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify)
	{
		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::BufferSubData(buffer, GL_ARRAY_BUFFER, 17, 17, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::BufferSubData(buffer, GL_ARRAY_BUFFER, 17, 17, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.render)
	{
		m_threads[0]->addOperation(new GLES2ThreadTest::RenderBuffer(program, buffer, m_config.useFenceSync, m_config.serverSync));
		m_threads[1]->addOperation(new GLES2ThreadTest::RenderBuffer(program, buffer, m_config.useFenceSync, m_config.serverSync));

		SharedPtr<tcu::ThreadUtil::DataBlock> pixels1;
		SharedPtr<tcu::ThreadUtil::DataBlock> pixels2;

		m_threads[0]->addOperation(new GLES2ThreadTest::ReadPixels(0, 0, 400, 400, GL_RGBA, GL_UNSIGNED_BYTE, pixels1, m_config.useFenceSync, m_config.serverSync));
		m_threads[1]->addOperation(new GLES2ThreadTest::ReadPixels(0, 0, 400, 400, GL_RGBA, GL_UNSIGNED_BYTE, pixels2, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new tcu::ThreadUtil::CompareData(pixels1, pixels2));
	}

	if (m_config.modify || m_config.render)
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteBuffer(buffer, m_config.useFenceSync, m_config.serverSync));
	else
		m_threads[1]->addOperation(new GLES2ThreadTest::DeleteBuffer(buffer, m_config.useFenceSync, m_config.serverSync));

	if (m_config.render)
	{
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteProgram(program, m_config.useFenceSync, m_config.serverSync));
	}
}

void GLES2ThreadedSharingTest::addTextureOperations (void)
{
	// Add operations for verify
	SharedPtr<GLES2ThreadTest::Shader>	vertexShader;
	SharedPtr<GLES2ThreadTest::Shader>	fragmentShader;
	SharedPtr<GLES2ThreadTest::Program>	program;

	if (m_config.render)
	{
		const char* vertexShaderSource =
		"attribute highp vec2 a_pos;\n"
		"varying mediump vec2 v_pos;\n"
		"void main(void)\n"
		"{\n"
		"\tv_pos = a_pos;\n"
		"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
		"}\n";

		const char* fragmentShaderSource =
		"varying mediump vec2 v_pos;\n"
		"uniform sampler2D u_sampler;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = texture2D(u_sampler, v_pos);\n"
		"}\n";

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_VERTEX_SHADER, vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(vertexShader, vertexShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(vertexShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_FRAGMENT_SHADER, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(fragmentShader, fragmentShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(fragmentShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateProgram(program, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, vertexShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::LinkProgram(program, m_config.useFenceSync, m_config.serverSync));
	}

	SharedPtr<GLES2ThreadTest::Texture> texture;

	m_threads[0]->addOperation(new GLES2ThreadTest::CreateTexture(texture, m_config.useFenceSync, m_config.serverSync));

	if (m_config.define == 1)
	{
		if (m_config.modify || m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::TexImage2D(texture, 0, GL_RGBA, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::TexImage2D(texture, 0, GL_RGBA, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.define == 2)
	{
		if (m_config.modify || m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::CopyTexImage2D(texture, 0, GL_RGBA, 17, 17, 256, 256, 0, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::CopyTexImage2D(texture, 0, GL_RGBA, 17, 17, 256, 256, 0, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify == 1)
	{
		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::TexSubImage2D(texture, 0, 17, 17, 29, 29, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::TexSubImage2D(texture, 0, 17, 17, 29, 29, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify == 2)
	{
		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::CopyTexSubImage2D(texture, 0, 7, 7, 17, 17, 29, 29, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::CopyTexSubImage2D(texture, 0, 7, 7, 17, 17, 29, 29, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.render)
	{
		SharedPtr<tcu::ThreadUtil::DataBlock> pixels1;
		SharedPtr<tcu::ThreadUtil::DataBlock> pixels2;

		m_threads[0]->addOperation(new GLES2ThreadTest::RenderTexture(program, texture, m_config.useFenceSync, m_config.serverSync));
		m_threads[1]->addOperation(new GLES2ThreadTest::RenderTexture(program, texture, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::ReadPixels(0, 0, 400, 400, GL_RGBA, GL_UNSIGNED_BYTE, pixels1, m_config.useFenceSync, m_config.serverSync));
		m_threads[1]->addOperation(new GLES2ThreadTest::ReadPixels(0, 0, 400, 400, GL_RGBA, GL_UNSIGNED_BYTE, pixels2, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new tcu::ThreadUtil::CompareData(pixels1, pixels2));
	}

	if (m_config.modify || m_config.render)
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteTexture(texture, m_config.useFenceSync, m_config.serverSync));
	else
		m_threads[1]->addOperation(new GLES2ThreadTest::DeleteTexture(texture, m_config.useFenceSync, m_config.serverSync));

	if (m_config.render)
	{
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteProgram(program, m_config.useFenceSync, m_config.serverSync));
	}
}

void GLES2ThreadedSharingTest::addImageOperations (void)
{
	// Add operations for verify
	SharedPtr<GLES2ThreadTest::Shader>	vertexShader;
	SharedPtr<GLES2ThreadTest::Shader>	fragmentShader;
	SharedPtr<GLES2ThreadTest::Program>	program;

	m_threads[0]->addOperation(new GLES2ThreadTest::InitGLExtension("GL_OES_EGL_image"));
	m_threads[1]->addOperation(new GLES2ThreadTest::InitGLExtension("GL_OES_EGL_image"));

	if (m_config.render)
	{
		const char* vertexShaderSource =
		"attribute highp vec2 a_pos;\n"
		"varying mediump vec2 v_pos;\n"
		"void main(void)\n"
		"{\n"
		"\tv_pos = a_pos;\n"
		"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
		"}\n";

		const char* fragmentShaderSource =
		"varying mediump vec2 v_pos;\n"
		"uniform sampler2D u_sampler;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = texture2D(u_sampler, v_pos);\n"
		"}\n";

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_VERTEX_SHADER, vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(vertexShader, vertexShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(vertexShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_FRAGMENT_SHADER, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(fragmentShader, fragmentShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(fragmentShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateProgram(program, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, vertexShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::LinkProgram(program, m_config.useFenceSync, m_config.serverSync));
	}

	SharedPtr<GLES2ThreadTest::Texture>		sourceTexture;
	SharedPtr<GLES2ThreadTest::Texture>		texture;
	SharedPtr<GLES2ThreadTest::EGLImage>	image;

	m_threads[0]->addOperation(new GLES2ThreadTest::CreateTexture(sourceTexture, m_config.useFenceSync, m_config.serverSync));
	m_threads[0]->addOperation(new GLES2ThreadTest::TexImage2D(sourceTexture, 0, GL_RGBA, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));

	if (m_config.define == 1)
	{
		if (m_config.modify || m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::CreateImageFromTexture(image, sourceTexture, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::CreateImageFromTexture(image, sourceTexture, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.define == 2)
	{
		m_threads[0]->addOperation(new GLES2ThreadTest::CreateImageFromTexture(image, sourceTexture, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::CreateTexture(texture, m_config.useFenceSync, m_config.serverSync));

		if (m_config.modify || m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::DefineTextureFromImage(texture, image, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::DefineTextureFromImage(texture, image, m_config.useFenceSync, m_config.serverSync));
	}

	m_threads[0]->addOperation(new GLES2ThreadTest::DeleteTexture(sourceTexture, m_config.useFenceSync, m_config.serverSync));

	if (m_config.modify == 1)
	{
		DE_ASSERT(m_config.define != 1);

		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::TexSubImage2D(texture, 0, 17, 17, 29, 29, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::TexSubImage2D(texture, 0, 17, 17, 29, 29, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify == 2)
	{
		DE_ASSERT(m_config.define != 1);

		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::CopyTexSubImage2D(texture, 0, 7, 7, 17, 17, 29, 29, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::CopyTexSubImage2D(texture, 0, 7, 7, 17, 17, 29, 29, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify == 3)
	{
		DE_ASSERT(m_config.define != 1);

		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::TexImage2D(texture, 0, GL_RGBA, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::TexImage2D(texture, 0, GL_RGBA, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify == 4)
	{
		DE_ASSERT(m_config.define != 1);

		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::CopyTexImage2D(texture, 0, GL_RGBA, 7, 7, 256, 256, 0, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::CopyTexImage2D(texture, 0, GL_RGBA, 7, 7, 256, 256, 0, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.render)
	{
		DE_ASSERT(m_config.define != 1);

		SharedPtr<tcu::ThreadUtil::DataBlock> pixels1;
		SharedPtr<tcu::ThreadUtil::DataBlock> pixels2;

		m_threads[0]->addOperation(new GLES2ThreadTest::RenderTexture(program, texture, m_config.useFenceSync, m_config.serverSync));
		m_threads[1]->addOperation(new GLES2ThreadTest::RenderTexture(program, texture, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::ReadPixels(0, 0, 400, 400, GL_RGBA, GL_UNSIGNED_BYTE, pixels1, m_config.useFenceSync, m_config.serverSync));
		m_threads[1]->addOperation(new GLES2ThreadTest::ReadPixels(0, 0, 400, 400, GL_RGBA, GL_UNSIGNED_BYTE, pixels2, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new tcu::ThreadUtil::CompareData(pixels1, pixels2));
	}

	if (texture)
	{
		if (m_config.modify || m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::DeleteTexture(texture, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::DeleteTexture(texture, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify || m_config.render)
		m_threads[0]->addOperation(new GLES2ThreadTest::DestroyImage(image, m_config.useFenceSync, m_config.serverSync));
	else
		m_threads[1]->addOperation(new GLES2ThreadTest::DestroyImage(image, m_config.useFenceSync, m_config.serverSync));

	if (m_config.render)
	{
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteProgram(program, m_config.useFenceSync, m_config.serverSync));
	}
}

void GLES2ThreadedSharingTest::addShaderOperations (GLenum type)
{
	SharedPtr<GLES2ThreadTest::Shader> shader;

	m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(type, shader, m_config.useFenceSync, m_config.serverSync));

	if (m_config.define)
	{
		const char* vertexShaderSource =
		"attribute mediump vec4 a_pos;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_Position = a_pos;\n"
		"}";

		const char* fragmentShaderSource =
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
		"}";

		if (m_config.modify || m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(shader, (type == GL_VERTEX_SHADER ? vertexShaderSource : fragmentShaderSource), m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::ShaderSource(shader, (type == GL_VERTEX_SHADER ? vertexShaderSource : fragmentShaderSource), m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify)
	{
		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(shader, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::ShaderCompile(shader, m_config.useFenceSync, m_config.serverSync));
	}

	DE_ASSERT(!m_config.render);

	if (m_config.modify || m_config.render)
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(shader, m_config.useFenceSync, m_config.serverSync));
	else
		m_threads[1]->addOperation(new GLES2ThreadTest::DeleteShader(shader, m_config.useFenceSync, m_config.serverSync));
}

void GLES2ThreadedSharingTest::addProgramOperations (void)
{
	// Add operations for verify
	SharedPtr<GLES2ThreadTest::Shader>	vertexShader;
	SharedPtr<GLES2ThreadTest::Shader>	fragmentShader;

	if (m_config.define)
	{
		const char* vertexShaderSource =
		"attribute highp vec2 a_pos;\n"
		"varying mediump vec2 v_pos;\n"
		"void main(void)\n"
		"{\n"
		"\tv_pos = a_pos;\n"
		"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
		"}\n";

		const char* fragmentShaderSource =
		"varying mediump vec2 v_pos;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = vec4(v_pos, 0.5, 1.0);\n"
		"}\n";

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_VERTEX_SHADER, vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(vertexShader, vertexShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(vertexShader, m_config.useFenceSync, m_config.serverSync));

		m_threads[0]->addOperation(new GLES2ThreadTest::CreateShader(GL_FRAGMENT_SHADER, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderSource(fragmentShader, fragmentShaderSource, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::ShaderCompile(fragmentShader, m_config.useFenceSync, m_config.serverSync));
	}

	SharedPtr<GLES2ThreadTest::Program> program;

	m_threads[0]->addOperation(new GLES2ThreadTest::CreateProgram(program, m_config.useFenceSync, m_config.serverSync));

	if (m_config.define)
	{
		// Attach shaders
		if (m_config.modify || m_config.render)
		{
			m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, vertexShader, m_config.useFenceSync, m_config.serverSync));
			m_threads[0]->addOperation(new GLES2ThreadTest::AttachShader(program, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		}
		else
		{
			m_threads[1]->addOperation(new GLES2ThreadTest::AttachShader(program, vertexShader, m_config.useFenceSync, m_config.serverSync));
			m_threads[1]->addOperation(new GLES2ThreadTest::AttachShader(program, fragmentShader, m_config.useFenceSync, m_config.serverSync));
		}
	}

	if (m_config.modify == 1)
	{
		// Link program
		if (m_config.render)
			m_threads[0]->addOperation(new GLES2ThreadTest::LinkProgram(program, m_config.useFenceSync, m_config.serverSync));
		else
			m_threads[1]->addOperation(new GLES2ThreadTest::LinkProgram(program, m_config.useFenceSync, m_config.serverSync));
	}

	if (m_config.modify == 2)
	{
		// Link program
		if (m_config.render)
		{
			m_threads[0]->addOperation(new GLES2ThreadTest::DetachShader(program, GL_VERTEX_SHADER, m_config.useFenceSync, m_config.serverSync));
			m_threads[0]->addOperation(new GLES2ThreadTest::DetachShader(program, GL_FRAGMENT_SHADER, m_config.useFenceSync, m_config.serverSync));
		}
		else
		{
			m_threads[1]->addOperation(new GLES2ThreadTest::DetachShader(program, GL_VERTEX_SHADER, m_config.useFenceSync, m_config.serverSync));
			m_threads[1]->addOperation(new GLES2ThreadTest::DetachShader(program, GL_FRAGMENT_SHADER, m_config.useFenceSync, m_config.serverSync));
		}
	}

	if (m_config.render)
	{
		DE_ASSERT(false);
	}

	if (m_config.modify || m_config.render)
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteProgram(program, m_config.useFenceSync, m_config.serverSync));
	else
		m_threads[1]->addOperation(new GLES2ThreadTest::DeleteProgram(program, m_config.useFenceSync, m_config.serverSync));

	if (m_config.render)
	{
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(vertexShader, m_config.useFenceSync, m_config.serverSync));
		m_threads[0]->addOperation(new GLES2ThreadTest::DeleteShader(fragmentShader, m_config.useFenceSync, m_config.serverSync));
	}
}

void GLES2ThreadedSharingTest::deinit (void)
{
	for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
	{
		delete m_threads[threadNdx];
		m_threads[threadNdx] = DE_NULL;
	}

	m_threads.clear();

	if (m_eglDisplay != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_eglDisplay);
		m_eglDisplay = EGL_NO_DISPLAY;
	}

	TCU_CHECK(!m_requiresRestart);
}

tcu::TestCase::IterateResult GLES2ThreadedSharingTest::iterate (void)
{
	if (!m_threadsStarted)
	{
		m_beginTimeUs = deGetMicroseconds();

		// Execute threads
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			m_threads[threadNdx]->exec();

		m_threadsStarted = true;
		m_threadsRunning = true;
	}

	if (m_threadsRunning)
	{
		// Wait threads to finish
		int readyThreads = 0;
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		{
			const tcu::ThreadUtil::Thread::ThreadStatus status = m_threads[threadNdx]->getStatus();

			if (status != tcu::ThreadUtil::Thread::THREADSTATUS_RUNNING && status != tcu::ThreadUtil::Thread::THREADSTATUS_NOT_STARTED)
				readyThreads++;
		}

		if (readyThreads == (int)m_threads.size())
		{
			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
				m_threads[threadNdx]->join();

			m_executionReady	= true;
			m_requiresRestart	= false;
		}

		if (deGetMicroseconds() - m_beginTimeUs > m_timeOutUs)
		{
			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			{
				if (m_threads[threadNdx]->getStatus() != tcu::ThreadUtil::Thread::THREADSTATUS_RUNNING)
					m_threads[threadNdx]->join();
			}
			m_executionReady	= true;
			m_requiresRestart	= true;
			m_timeOutTimeUs		= deGetMicroseconds();
		}
		else
		{
			deSleep(m_sleepTimeMs);
		}
	}

	if (m_executionReady)
	{
		std::vector<int> indices(m_threads.size(), 0);

		if (m_timeOutTimeUs != 0)
			m_log << tcu::TestLog::Message << "Execution timeout limit reached. Trying to get per thread logs. This is potentially dangerous." << tcu::TestLog::EndMessage;

		while (true)
		{
			int			firstThread = -1;

			// Find first thread with messages
			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			{
				if (m_threads[threadNdx]->getMessageCount() > indices[threadNdx])
				{
					firstThread = threadNdx;
					break;
				}
			}

			// No more messages
			if (firstThread == -1)
				break;

			for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			{
				// No more messages in this thread
				if (m_threads[threadNdx]->getMessageCount() <= indices[threadNdx])
					continue;

				if ((m_threads[threadNdx]->getMessage(indices[threadNdx]).getTime() - m_beginTimeUs) < (m_threads[firstThread]->getMessage(indices[firstThread]).getTime() - m_beginTimeUs))
					firstThread = threadNdx;
			}

			tcu::ThreadUtil::Message message = m_threads[firstThread]->getMessage(indices[firstThread]);

			m_log << tcu::TestLog::Message << "[" << (message.getTime() - m_beginTimeUs) << "] (" << firstThread << ") " << message.getMessage() << tcu::TestLog::EndMessage;
			indices[firstThread]++;
		}

		if (m_timeOutTimeUs != 0)
			m_log << tcu::TestLog::Message << "[" << (m_timeOutTimeUs - m_beginTimeUs) << "] Execution timeout limit reached" << tcu::TestLog::EndMessage;

		bool isOk = true;
		bool notSupported = false;

		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		{
			const tcu::ThreadUtil::Thread::ThreadStatus status = m_threads[threadNdx]->getStatus();

			switch (status)
			{
				case tcu::ThreadUtil::Thread::THREADSTATUS_FAILED:
				case tcu::ThreadUtil::Thread::THREADSTATUS_INIT_FAILED:
				case tcu::ThreadUtil::Thread::THREADSTATUS_RUNNING:
					isOk = false;
					break;

				case tcu::ThreadUtil::Thread::THREADSTATUS_NOT_SUPPORTED:
					notSupported = true;
					break;

				case tcu::ThreadUtil::Thread::THREADSTATUS_READY:
					// Nothing
					break;

				default:
					DE_ASSERT(false);
					isOk = false;
			};
		}

		if (notSupported)
			throw tcu::NotSupportedError("Thread threw tcu::NotSupportedError", "", __FILE__, __LINE__);

		if (isOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		return STOP;
	}

	return CONTINUE;
}

static void addSimpleTests (EglTestContext& ctx, tcu::TestCaseGroup* group, bool useSync, bool serverSync)
{
	{
		TestCaseGroup* bufferTests = new TestCaseGroup(ctx, "buffers", "Buffer management tests");

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_BUFFER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 0;
			config.modify = 0;
			config.render = false;
			bufferTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "gen_delete", "Generate and delete buffer"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_BUFFER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = false;
			bufferTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "bufferdata", "Generate, set data and delete buffer"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_BUFFER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 1;
			config.render = false;
			bufferTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "buffersubdata", "Generate, set data, update data and delete buffer"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_BUFFER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = true;
			bufferTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "bufferdata_render", "Generate, set data, render and delete buffer"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_BUFFER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 1;
			config.render = true;
			bufferTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "buffersubdata_render", "Generate, set data, update data, render and delete buffer"));
		}

		group->addChild(bufferTests);
	}

	{
		TestCaseGroup* textureTests = new TestCaseGroup(ctx, "textures", "Texture management tests");

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 0;
			config.modify = 0;
			config.render = false;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "gen_delete", "Generate and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = false;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d", "Generate, set data and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 1;
			config.render = false;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d_texsubimage2d", "Generate, set data, update data and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 2;
			config.render = false;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d_copytexsubimage2d", "Generate, set data, update data and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = true;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d_render", "Generate, set data, render and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 1;
			config.render = true;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d_texsubimage2d_render", "Generate, set data, update data, render and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 2;
			config.render = true;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d_copytexsubimage2d_render", "Generate, set data, update data, render and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 0;
			config.render = false;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d", "Generate, set data and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 1;
			config.render = false;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d_texsubimage2d", "Generate, set data, update data and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 2;
			config.render = false;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d_copytexsubimage2d", "Generate, set data, update data and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 0;
			config.render = true;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d_render", "Generate, set data, render and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 1;
			config.render = true;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d_texsubimage2d_render", "Generate, set data, update data, render and delete texture"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_TEXTURE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 2;
			config.render = true;
			textureTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d_copytexsubimage2d_render", "Generate, set data, update data, render and delete texture"));
		}

		group->addChild(textureTests);
	}

	{
		TestCaseGroup* shaderTests = new TestCaseGroup(ctx, "shaders", "Shader management tests");

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_VERTEX_SHADER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 0;
			config.modify = 0;
			config.render = false;
			shaderTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "vtx_create_destroy", "Create and delete shader"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_VERTEX_SHADER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = false;
			shaderTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "vtx_shadersource", "Create, set source and delete shader"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_VERTEX_SHADER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 1;
			config.render = false;
			shaderTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "vtx_compile", "Create, set source, compile and delete shader"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_FRAGMENT_SHADER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 0;
			config.modify = 0;
			config.render = false;
			shaderTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "frag_create_destroy", "Create and delete shader"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_FRAGMENT_SHADER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = false;
			shaderTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "frag_shadersource", "Create, set source and delete shader"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_FRAGMENT_SHADER;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 1;
			config.render = false;
			shaderTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "frag_compile", "Create, set source, compile and delete shader"));
		}

		group->addChild(shaderTests);
	}

	{
		TestCaseGroup* programTests = new TestCaseGroup(ctx, "programs", "Program management tests");

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_PROGRAM;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 0;
			config.modify = 0;
			config.render = false;
			programTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "create_destroy", "Create and delete program"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_PROGRAM;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = false;
			programTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "attach", "Create, attach shaders and delete program"));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_PROGRAM;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 1;
			config.render = false;
			programTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "link", "Create, attach shaders, link and delete program"));
		}

		group->addChild(programTests);
	}

	{
		TestCaseGroup* imageTests = new TestCaseGroup(ctx, "images", "Image management tests");

		TestCaseGroup* textureSourceTests = new TestCaseGroup(ctx, "texture_source", "Image management tests with texture source.");
		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 1;
			config.modify = 0;
			config.render = false;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "create_destroy", "Create and destroy EGLImage."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 0;
			config.render = false;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "create_texture", "Create texture from image."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 1;
			config.render = false;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "texsubimage2d", "Modify texture created from image with glTexSubImage2D."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 2;
			config.render = false;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copytexsubimage2d", "Modify texture created from image with glCopyTexSubImage2D."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 3;
			config.render = false;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d", "Modify texture created from image with glTexImage2D."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 4;
			config.render = false;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d", "Modify texture created from image with glCopyTexImage2D."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 0;
			config.render = true;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "create_texture_render", "Create texture from image and render."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 1;
			config.render = true;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "texsubimage2d_render", "Modify texture created from image and render."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 2;
			config.render = true;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copytexsubimage2d_render", "Modify texture created from image and render."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 3;
			config.render = true;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "teximage2d_render", "Modify texture created from image and render."));
		}

		{
			GLES2ThreadedSharingTest::TestConfig config;

			config.resourceType = GLES2ThreadedSharingTest::TestConfig::RESOURCETYPE_IMAGE;
			config.useFenceSync = useSync;
			config.serverSync	= serverSync;
			config.define = 2;
			config.modify = 4;
			config.render = true;
			textureSourceTests->addChild(new GLES2ThreadedSharingTest(ctx, config, "copyteximage2d_render", "Modify texture created from image and render."));
		}

		imageTests->addChild(textureSourceTests);

		group->addChild(imageTests);
	}

}

static void addRandomTests (EglTestContext& ctx, tcu::TestCaseGroup* group, bool useSync, bool serverSync)
{
	{
		TestCaseGroup* textureTests = new TestCaseGroup(ctx, "textures", "Texture management tests");

		{
			TestCaseGroup* genTextureTests = new TestCaseGroup(ctx, "gen_delete", "Texture gen and delete tests");

			for (int textureTestNdx = 0; textureTestNdx < 20; textureTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + textureTestNdx % 5;
				config.operationCount	= 30 + textureTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.75f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.5f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.5f;

				std::string	name = de::toString(textureTestNdx);
				genTextureTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			textureTests->addChild(genTextureTests);
		}

		{
			TestCaseGroup* texImage2DTests = new TestCaseGroup(ctx, "teximage2d", "Texture gen, delete and teximage2D tests");

			for (int textureTestNdx = 0; textureTestNdx < 20; textureTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + textureTestNdx % 5;
				config.operationCount	= 40 + textureTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]		= 0.80f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]		= 0.30f;

				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]		= 0.40f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]		= 0.40f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]			= 0.20f;

				std::string	name = de::toString(textureTestNdx);
				texImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			textureTests->addChild(texImage2DTests);
		}

		{
			TestCaseGroup* texSubImage2DTests = new TestCaseGroup(ctx, "texsubimage2d", "Texture gen, delete, teximage2D and texsubimage2d tests");

			for (int textureTestNdx = 0; textureTestNdx < 20; textureTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + textureTestNdx % 5;
				config.operationCount	= 50 + textureTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]		= 0.80f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXSUBIMAGE2D]		= 0.05f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]		= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXSUBIMAGE2D]	= 0.10f;

				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]		= 0.20f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]		= 0.20f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]			= 0.10f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXSUBIMAGE2D]			= 0.50f;

				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.20f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]		= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]			= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_TEXSUBIMAGE2D]		= 0.30f;

				std::string	name = de::toString(textureTestNdx);
				texSubImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			textureTests->addChild(texSubImage2DTests);
		}

		{
			TestCaseGroup* copyTexImage2DTests = new TestCaseGroup(ctx, "copyteximage2d", "Texture gen, delete and copyteximage2d tests");

			for (int textureTestNdx = 0; textureTestNdx < 20; textureTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + textureTestNdx % 5;
				config.operationCount	= 40 + textureTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_COPYTEXIMAGE2D]	= 0.80f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]	= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_COPYTEXIMAGE2D]	= 0.30f;

				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.40f;
				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]	= 0.40f;
				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_COPYTEXIMAGE2D]	= 0.20f;


				std::string	name = de::toString(textureTestNdx);
				copyTexImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			textureTests->addChild(copyTexImage2DTests);
		}

		{
			TestCaseGroup* copyTexSubImage2DTests = new TestCaseGroup(ctx, "copytexsubimage2d", "Texture gen, delete, teximage2D and copytexsubimage2d tests");

			for (int textureTestNdx = 0; textureTestNdx < 20; textureTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + textureTestNdx % 5;
				config.operationCount	= 50 + textureTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]					= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]		= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]			= 0.80f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_COPYTEXSUBIMAGE2D]		= 0.05f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]		= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]		= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]			= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_COPYTEXSUBIMAGE2D]	= 0.10f;

				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]			= 0.20f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]			= 0.20f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]				= 0.10f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_COPYTEXSUBIMAGE2D]			= 0.50f;

				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]	= 0.20f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]		= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]			= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_COPYTEXSUBIMAGE2D]	= 0.30f;


				std::string	name = de::toString(textureTestNdx);
				copyTexSubImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			textureTests->addChild(copyTexSubImage2DTests);
		}

		group->addChild(textureTests);

		TestCaseGroup* bufferTests = new TestCaseGroup(ctx, "buffers", "Buffer management tests");

		{
			TestCaseGroup* genBufferTests = new TestCaseGroup(ctx, "gen_delete", "Buffer gen and delete tests");

			for (int bufferTestNdx = 0; bufferTestNdx < 20; bufferTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + bufferTestNdx % 5;
				config.operationCount	= 30 + bufferTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_BUFFER]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_DESTROY_BUFFER]		= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_CREATE_BUFFER]		= 0.75f;

				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_DESTROY_BUFFER]	= 0.5f;
				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_CREATE_BUFFER]		= 0.5f;

				std::string	name = de::toString(bufferTestNdx);
				genBufferTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			bufferTests->addChild(genBufferTests);
		}

		{
			TestCaseGroup* texImage2DTests = new TestCaseGroup(ctx, "bufferdata", "Buffer gen, delete and bufferdata tests");

			for (int bufferTestNdx = 0; bufferTestNdx < 20; bufferTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + bufferTestNdx % 5;
				config.operationCount	= 40 + bufferTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_BUFFER]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_DESTROY_BUFFER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_CREATE_BUFFER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_BUFFER_DATA]		= 0.80f;

				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_DESTROY_BUFFER]	= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_CREATE_BUFFER]		= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_BUFFER_DATA]		= 0.30f;

				config.probabilities[THREADOPERATIONID_BUFFER_DATA][THREADOPERATIONID_DESTROY_BUFFER]		= 0.40f;
				config.probabilities[THREADOPERATIONID_BUFFER_DATA][THREADOPERATIONID_CREATE_BUFFER]		= 0.40f;
				config.probabilities[THREADOPERATIONID_BUFFER_DATA][THREADOPERATIONID_BUFFER_DATA]			= 0.20f;

				std::string	name = de::toString(bufferTestNdx);
				texImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			bufferTests->addChild(texImage2DTests);
		}

		{
			TestCaseGroup* texSubImage2DTests = new TestCaseGroup(ctx, "buffersubdata", "Buffer gen, delete, bufferdata and bufferdata tests");

			for (int bufferTestNdx = 0; bufferTestNdx < 20; bufferTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + bufferTestNdx % 5;
				config.operationCount	= 50 + bufferTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_BUFFER]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_DESTROY_BUFFER]		= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_CREATE_BUFFER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_BUFFER_DATA]		= 0.80f;
				config.probabilities[THREADOPERATIONID_CREATE_BUFFER][THREADOPERATIONID_BUFFER_SUBDATA]		= 0.05f;

				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_DESTROY_BUFFER]	= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_CREATE_BUFFER]		= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_BUFFER_DATA]		= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_BUFFER][THREADOPERATIONID_BUFFER_SUBDATA]	= 0.10f;

				config.probabilities[THREADOPERATIONID_BUFFER_DATA][THREADOPERATIONID_DESTROY_BUFFER]		= 0.20f;
				config.probabilities[THREADOPERATIONID_BUFFER_DATA][THREADOPERATIONID_CREATE_BUFFER]		= 0.20f;
				config.probabilities[THREADOPERATIONID_BUFFER_DATA][THREADOPERATIONID_BUFFER_DATA]			= 0.10f;
				config.probabilities[THREADOPERATIONID_BUFFER_DATA][THREADOPERATIONID_BUFFER_SUBDATA]		= 0.50f;

				config.probabilities[THREADOPERATIONID_BUFFER_SUBDATA][THREADOPERATIONID_DESTROY_BUFFER]	= 0.20f;
				config.probabilities[THREADOPERATIONID_BUFFER_SUBDATA][THREADOPERATIONID_CREATE_BUFFER]		= 0.25f;
				config.probabilities[THREADOPERATIONID_BUFFER_SUBDATA][THREADOPERATIONID_BUFFER_DATA]		= 0.25f;
				config.probabilities[THREADOPERATIONID_BUFFER_SUBDATA][THREADOPERATIONID_BUFFER_SUBDATA]	= 0.30f;

				std::string	name = de::toString(bufferTestNdx);
				texSubImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			bufferTests->addChild(texSubImage2DTests);
		}

		group->addChild(bufferTests);

		TestCaseGroup* shaderTests = new TestCaseGroup(ctx, "shaders", "Shader management tests");

		{
			TestCaseGroup* createShaderTests = new TestCaseGroup(ctx, "create_destroy", "Shader create and destroy tests");

			for (int shaderTestNdx = 0; shaderTestNdx < 20; shaderTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + shaderTestNdx % 5;
				config.operationCount	= 30 + shaderTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_VERTEX_SHADER]						= 0.5f;
				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]						= 0.5f;

				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_DESTROY_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.40f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]		= 0.40f;

				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_DESTROY_SHADER]			= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.40f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]	= 0.40f;

				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_DESTROY_SHADER]					= 0.5f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.25f;

				std::string	name = de::toString(shaderTestNdx);
				createShaderTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			shaderTests->addChild(createShaderTests);
		}

		{
			TestCaseGroup* texImage2DTests = new TestCaseGroup(ctx, "source", "Shader create, destroy and source tests");

			for (int shaderTestNdx = 0; shaderTestNdx < 20; shaderTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + shaderTestNdx % 5;
				config.operationCount	= 40 + shaderTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_VERTEX_SHADER]						= 0.5f;
				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]						= 0.5f;

				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_DESTROY_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]		= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.50f;

				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_DESTROY_SHADER]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]	= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.50f;

				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_DESTROY_SHADER]					= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_SHADER_SOURCE]						= 0.10f;

				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_DESTROY_SHADER]						= 0.20f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_SHADER_SOURCE]						= 0.40f;

				std::string	name = de::toString(shaderTestNdx);
				texImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			shaderTests->addChild(texImage2DTests);
		}

		{
			TestCaseGroup* texSubImage2DTests = new TestCaseGroup(ctx, "compile", "Shader create, destroy, source and compile tests");

			for (int shaderTestNdx = 0; shaderTestNdx < 20; shaderTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + shaderTestNdx % 5;
				config.operationCount	= 50 + shaderTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_VERTEX_SHADER]						= 0.5f;
				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]						= 0.5f;

				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_DESTROY_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]		= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.50f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_SHADER_COMPILE]				= 0.10f;

				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_DESTROY_SHADER]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]	= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.50f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_SHADER_COMPILE]			= 0.10f;

				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_DESTROY_SHADER]					= 0.30f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_SHADER_SOURCE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_SHADER_COMPILE]					= 0.10f;

				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_DESTROY_SHADER]						= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_SHADER_SOURCE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_SHADER_COMPILE]						= 0.50f;

				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_DESTROY_SHADER]					= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_SHADER_SOURCE]						= 0.30f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_SHADER_COMPILE]					= 0.30f;

				std::string	name = de::toString(shaderTestNdx);
				texSubImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			shaderTests->addChild(texSubImage2DTests);
		}

		group->addChild(shaderTests);

		TestCaseGroup* programTests = new TestCaseGroup(ctx, "programs", "Program management tests");

		{
			TestCaseGroup* createProgramTests = new TestCaseGroup(ctx, "create_destroy", "Program create and destroy tests");

			for (int programTestNdx = 0; programTestNdx < 20; programTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + programTestNdx % 5;
				config.operationCount	= 30 + programTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_PROGRAM]				= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_DESTROY_PROGRAM]	= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_CREATE_PROGRAM]	= 0.75f;

				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_DESTROY_PROGRAM]	= 0.5f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_CREATE_PROGRAM]	= 0.5f;

				std::string	name = de::toString(programTestNdx);
				createProgramTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			programTests->addChild(createProgramTests);
		}

		{
			TestCaseGroup* texImage2DTests = new TestCaseGroup(ctx, "attach_detach", "Program create, destroy, attach and detach tests");

			for (int programTestNdx = 0; programTestNdx < 20; programTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + programTestNdx % 5;
				config.operationCount	= 60 + programTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_VERTEX_SHADER]						= 0.35f;
				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]						= 0.35f;
				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_PROGRAM]								= 0.30f;

				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_DESTROY_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_SHADER_COMPILE]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_PROGRAM]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]				= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_ATTACH_SHADER]				= 0.15f;

				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_DESTROY_SHADER]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]	= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_SHADER_COMPILE]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_PROGRAM]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]			= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_ATTACH_SHADER]				= 0.15f;

				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_DESTROY_SHADER]					= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_SHADER_SOURCE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_SHADER_COMPILE]					= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_ATTACH_SHADER]						= 0.15f;

				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_DESTROY_SHADER]						= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_SHADER_SOURCE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_SHADER_COMPILE]						= 0.50f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_PROGRAM]						= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_ATTACH_SHADER]						= 0.25f;

				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_DESTROY_SHADER]					= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_SHADER_SOURCE]						= 0.30f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_SHADER_COMPILE]					= 0.30f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_PROGRAM]					= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_ATTACH_SHADER]						= 0.35f;

				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_DESTROY_SHADER]					= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_SHADER_SOURCE]						= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_SHADER_COMPILE]					= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_CREATE_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_ATTACH_SHADER]						= 0.40f;

				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_DESTROY_SHADER]					= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_SHADER_SOURCE]					= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_SHADER_COMPILE]					= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_CREATE_PROGRAM]					= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_ATTACH_SHADER]					= 0.10f;

				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_DESTROY_SHADER]						= 0.20f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_SHADER_SOURCE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_SHADER_COMPILE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_CREATE_PROGRAM]						= 0.15f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_ATTACH_SHADER]						= 0.30f;

				std::string	name = de::toString(programTestNdx);
				texImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			programTests->addChild(texImage2DTests);
		}

		{
			TestCaseGroup* texSubImage2DTests = new TestCaseGroup(ctx, "link", "Program create, destroy, attach and link tests");

			for (int programTestNdx = 0; programTestNdx < 20; programTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + programTestNdx % 5;
				config.operationCount	= 70 + programTestNdx;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_VERTEX_SHADER]						= 0.35f;
				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]						= 0.35f;
				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_PROGRAM]								= 0.30f;

				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_DESTROY_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_SHADER_COMPILE]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_CREATE_PROGRAM]				= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]				= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_ATTACH_SHADER]				= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_VERTEX_SHADER][THREADOPERATIONID_LINK_PROGRAM]				= 0.10f;

				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_DESTROY_SHADER]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]		= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]	= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_SHADER_SOURCE]				= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_SHADER_COMPILE]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_CREATE_PROGRAM]			= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]			= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_ATTACH_SHADER]				= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_FRAGMENT_SHADER][THREADOPERATIONID_LINK_PROGRAM]				= 0.10f;

				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_DESTROY_SHADER]					= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_SHADER_SOURCE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_SHADER_COMPILE]					= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_CREATE_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_ATTACH_SHADER]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_SHADER][THREADOPERATIONID_LINK_PROGRAM]						= 0.10f;

				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_DESTROY_SHADER]						= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]				= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_SHADER_SOURCE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_SHADER_COMPILE]						= 0.50f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_CREATE_PROGRAM]						= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_ATTACH_SHADER]						= 0.25f;
				config.probabilities[THREADOPERATIONID_SHADER_SOURCE][THREADOPERATIONID_LINK_PROGRAM]						= 0.20f;

				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_DESTROY_SHADER]					= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.15f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_SHADER_SOURCE]						= 0.30f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_SHADER_COMPILE]					= 0.30f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_CREATE_PROGRAM]					= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.10f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_ATTACH_SHADER]						= 0.35f;
				config.probabilities[THREADOPERATIONID_SHADER_COMPILE][THREADOPERATIONID_LINK_PROGRAM]						= 0.20f;

				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_DESTROY_SHADER]					= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_SHADER_SOURCE]						= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_SHADER_COMPILE]					= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_CREATE_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.05f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_ATTACH_SHADER]						= 0.40f;
				config.probabilities[THREADOPERATIONID_CREATE_PROGRAM][THREADOPERATIONID_LINK_PROGRAM]						= 0.05f;

				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_DESTROY_SHADER]					= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]			= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_SHADER_SOURCE]					= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_SHADER_COMPILE]					= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_CREATE_PROGRAM]					= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_ATTACH_SHADER]					= 0.10f;
				config.probabilities[THREADOPERATIONID_DESTROY_PROGRAM][THREADOPERATIONID_LINK_PROGRAM]						= 0.05f;

				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_DESTROY_SHADER]						= 0.20f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_SHADER_SOURCE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_SHADER_COMPILE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_CREATE_PROGRAM]						= 0.15f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_DESTROY_PROGRAM]					= 0.15f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_ATTACH_SHADER]						= 0.30f;
				config.probabilities[THREADOPERATIONID_ATTACH_SHADER][THREADOPERATIONID_LINK_PROGRAM]						= 0.30f;

				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_DESTROY_SHADER]						= 0.20f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_CREATE_VERTEX_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_CREATE_FRAGMENT_SHADER]				= 0.20f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_SHADER_SOURCE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_SHADER_COMPILE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_CREATE_PROGRAM]						= 0.20f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_DESTROY_PROGRAM]						= 0.15f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_ATTACH_SHADER]						= 0.10f;
				config.probabilities[THREADOPERATIONID_LINK_PROGRAM][THREADOPERATIONID_LINK_PROGRAM]						= 0.05f;

				std::string	name = de::toString(programTestNdx);
				texSubImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			programTests->addChild(texSubImage2DTests);
		}

		group->addChild(programTests);

		TestCaseGroup* imageTests = new TestCaseGroup(ctx, "images", "Image management tests");

		{
			TestCaseGroup* texImage2DTests = new TestCaseGroup(ctx, "create_destroy", "Image gen, delete and teximage2D tests");

			for (int imageTestNdx = 0; imageTestNdx < 20; imageTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + imageTestNdx % 5;
				config.operationCount	= 70 + imageTestNdx;
				config.useImages		= true;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]									= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.40f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.35f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.30f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.35f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.15f;

				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]	= 0.40f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]				= 0.35f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]					= 0.15f;

				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.40f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.35f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;

				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]					= 0.40f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_IMAGE]								= 0.35f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;

				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.30f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXIMAGE2D]						= 0.15f;

				std::string	name = de::toString(imageTestNdx);
				texImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			imageTests->addChild(texImage2DTests);
		}

		{
			TestCaseGroup* texImage2DTests = new TestCaseGroup(ctx, "teximage2d", "Image gen, delete and teximage2D tests");

			for (int imageTestNdx = 0; imageTestNdx < 20; imageTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + imageTestNdx % 5;
				config.operationCount	= 70 + imageTestNdx;
				config.useImages		= true;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]									= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.20f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.15f;

				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]	= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]					= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]			= 0.15f;

				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;

				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_IMAGE]								= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;

				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXIMAGE2D]						= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]				= 0.15f;

				std::string	name = de::toString(imageTestNdx);
				texImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			imageTests->addChild(texImage2DTests);
		}

		{
			TestCaseGroup* texSubImage2DTests = new TestCaseGroup(ctx, "texsubimage2d", "Image gen, delete, teximage2D and texsubimage2d tests");

			for (int imageTestNdx = 0; imageTestNdx < 20; imageTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + imageTestNdx % 5;
				config.operationCount	= 70 + imageTestNdx;
				config.useImages		= true;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]									= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXSUBIMAGE2D]							= 0.10f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXSUBIMAGE2D]						= 0.10f;

				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]	= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]					= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]			= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXSUBIMAGE2D]				= 0.10f;

				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXSUBIMAGE2D]							= 0.10f;

				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_IMAGE]								= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXSUBIMAGE2D]								= 0.10f;

				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXIMAGE2D]						= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]				= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXSUBIMAGE2D]						= 0.10f;

				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_DESTROY_IMAGE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXSUBIMAGE2D][THREADOPERATIONID_TEXSUBIMAGE2D]							= 0.10f;

				std::string	name = de::toString(imageTestNdx);
				texSubImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			imageTests->addChild(texSubImage2DTests);
		}

		{
			TestCaseGroup* copyTexImage2DTests = new TestCaseGroup(ctx, "copyteximage2d", "Image gen, delete and copyteximage2d tests");

			for (int imageTestNdx = 0; imageTestNdx < 20; imageTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + imageTestNdx % 5;
				config.operationCount	= 70 + imageTestNdx;
				config.useImages		= true;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]									= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_COPYTEXIMAGE2D]						= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.20f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_COPYTEXIMAGE2D]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.15f;

				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]	= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_COPYTEXIMAGE2D]				= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]			= 0.15f;

				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_COPYTEXIMAGE2D]							= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;

				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_DESTROY_IMAGE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_COPYTEXIMAGE2D]						= 0.15f;
				config.probabilities[THREADOPERATIONID_COPYTEXIMAGE2D][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.15f;

				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_COPYTEXIMAGE2D]					= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]				= 0.15f;

				std::string	name = de::toString(imageTestNdx);
				copyTexImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			imageTests->addChild(copyTexImage2DTests);
		}

		{
			TestCaseGroup* copyTexSubImage2DTests = new TestCaseGroup(ctx, "copytexsubimage2d", "Image gen, delete, teximage2D and copytexsubimage2d tests");

			for (int imageTestNdx = 0; imageTestNdx < 20; imageTestNdx++)
			{
				GLES2SharingRandomTest::TestConfig config;
				config.useFenceSync		= useSync;
				config.serverSync		= serverSync;
				config.threadCount		= 2 + imageTestNdx % 5;
				config.operationCount	= 70 + imageTestNdx;
				config.useImages		= true;

				config.probabilities[THREADOPERATIONID_NONE][THREADOPERATIONID_CREATE_TEXTURE]									= 1.0f;

				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.10f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.30f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.20f;
				config.probabilities[THREADOPERATIONID_CREATE_TEXTURE][THREADOPERATIONID_COPYTEXSUBIMAGE2D]						= 0.10f;

				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]						= 0.20f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]							= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_TEXTURE][THREADOPERATIONID_COPYTEXSUBIMAGE2D]					= 0.10f;

				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]	= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_DESTROY_IMAGE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXIMAGE2D]					= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]			= 0.15f;
				config.probabilities[THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE][THREADOPERATIONID_COPYTEXSUBIMAGE2D]			= 0.10f;

				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]				= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_DESTROY_IMAGE][THREADOPERATIONID_COPYTEXSUBIMAGE2D]						= 0.10f;

				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]							= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_DESTROY_IMAGE]								= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]								= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_TEXTURE_FROM_IMAGE]						= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXIMAGE2D][THREADOPERATIONID_COPYTEXSUBIMAGE2D]							= 0.10f;

				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_DESTROY_IMAGE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXIMAGE2D]						= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_TEXTURE_FROM_IMAGE]				= 0.15f;
				config.probabilities[THREADOPERATIONID_TEXTURE_FROM_IMAGE][THREADOPERATIONID_COPYTEXSUBIMAGE2D]					= 0.10f;

				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_DESTROY_TEXTURE]					= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_CREATE_TEXTURE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_CREATE_IMAGE_FROM_TEXTURE]			= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_DESTROY_IMAGE]						= 0.25f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_TEXIMAGE2D]							= 0.15f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_TEXTURE_FROM_IMAGE]					= 0.15f;
				config.probabilities[THREADOPERATIONID_COPYTEXSUBIMAGE2D][THREADOPERATIONID_COPYTEXSUBIMAGE2D]					= 0.10f;


				std::string	name = de::toString(imageTestNdx);
				copyTexSubImage2DTests->addChild(new GLES2SharingRandomTest(ctx, config, name.c_str(), name.c_str()));
			}

			imageTests->addChild(copyTexSubImage2DTests);
		}

		group->addChild(imageTests);
	}
}

GLES2SharingThreadedTests::GLES2SharingThreadedTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "multithread", "EGL GLES2 sharing multithread tests")
{
}

void GLES2SharingThreadedTests::init (void)
{
	tcu::TestCaseGroup* simpleTests = new TestCaseGroup(m_eglTestCtx, "simple", "Simple multithreaded tests");
	addSimpleTests(m_eglTestCtx, simpleTests, false, false);
	addChild(simpleTests);

	TestCaseGroup* randomTests = new TestCaseGroup(m_eglTestCtx, "random", "Random tests");
	addRandomTests(m_eglTestCtx, randomTests, false, false);
	addChild(randomTests);

	tcu::TestCaseGroup* simpleTestsSync = new TestCaseGroup(m_eglTestCtx, "simple_egl_sync", "Simple multithreaded tests with EGL_KHR_fence_sync");
	addSimpleTests(m_eglTestCtx, simpleTestsSync, true, false);
	addChild(simpleTestsSync);

	TestCaseGroup* randomTestsSync = new TestCaseGroup(m_eglTestCtx, "random_egl_sync", "Random tests with EGL_KHR_fence_sync");
	addRandomTests(m_eglTestCtx, randomTestsSync, true, false);
	addChild(randomTestsSync);

	tcu::TestCaseGroup* simpleTestsServerSync = new TestCaseGroup(m_eglTestCtx, "simple_egl_server_sync", "Simple multithreaded tests with EGL_KHR_fence_sync and EGL_KHR_wait_sync");
	addSimpleTests(m_eglTestCtx, simpleTestsServerSync, true, true);
	addChild(simpleTestsServerSync);

	TestCaseGroup* randomTestsServerSync = new TestCaseGroup(m_eglTestCtx, "random_egl_server_sync", "Random tests with EGL_KHR_fence_sync and EGL_KHR_wait_sync");
	addRandomTests(m_eglTestCtx, randomTestsServerSync, true, true);
	addChild(randomTestsServerSync);
}

} // egl
} // deqp
