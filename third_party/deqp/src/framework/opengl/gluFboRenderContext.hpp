#ifndef _GLUFBORENDERCONTEXT_HPP
#define _GLUFBORENDERCONTEXT_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES Utilities
 * ------------------------------------------------
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
 * \brief OpenGL ES context wrapper that uses FBO as default framebuffer.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "gluRenderContext.hpp"
#include "tcuRenderTarget.hpp"

namespace tcu
{
class CommandLine;
}

namespace glu
{

class ContextFactory;
struct RenderConfig;

/*--------------------------------------------------------------------*//*!
 * \brief OpenGL ES context wrapper that uses FBO as default framebuffer.
 *//*--------------------------------------------------------------------*/
class FboRenderContext : public RenderContext
{
public:
										FboRenderContext		(RenderContext* context, const RenderConfig& config);
										FboRenderContext		(const ContextFactory& factory, const RenderConfig& config, const tcu::CommandLine& cmdLine);
	virtual								~FboRenderContext		(void);

	virtual ContextType					getType					(void) const { return m_context->getType();			}
	virtual const  glw::Functions&		getFunctions			(void) const { return m_context->getFunctions();	}
	virtual const tcu::RenderTarget&	getRenderTarget			(void) const { return m_renderTarget;				}
	virtual void						postIterate				(void);

	virtual deUint32					getDefaultFramebuffer	(void) const { return m_framebuffer; }
	virtual glw::GenericFuncType		getProcAddress			(const char* name) const { return m_context->getProcAddress(name); }

	virtual void						makeCurrent				(void);

private:
	void								createFramebuffer		(const RenderConfig& config);
	void								destroyFramebuffer		(void);

	RenderContext*						m_context;
	deUint32							m_framebuffer;
	deUint32							m_colorBuffer;
	deUint32							m_depthStencilBuffer;
	tcu::RenderTarget					m_renderTarget;
};

// RenderConfig to format mapping utilities, useful for platforms like iOS.
deUint32	chooseColorFormat			(const RenderConfig& config);
deUint32	chooseDepthStencilFormat	(const RenderConfig& config);

} // glu

#endif // _GLUFBORENDERCONTEXT_HPP
