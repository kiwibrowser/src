/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief OpenGL test context.
 */ /*-------------------------------------------------------------------*/

#include "glcContext.hpp"
#include "gluContextInfo.hpp"
#include "gluDummyRenderContext.hpp"
#include "gluFboRenderContext.hpp"
#include "gluRenderConfig.hpp"
#include "gluRenderContext.hpp"
#include "glwWrapper.hpp"
#include "tcuCommandLine.hpp"

namespace deqp
{

Context::Context(tcu::TestContext& testCtx, glu::ContextType contextType)
	: m_testCtx(testCtx), m_renderCtx(DE_NULL), m_contextInfo(DE_NULL)
{
	createRenderContext(contextType);
}

Context::~Context(void)
{
	destroyRenderContext();
}

void Context::createRenderContext(glu::ContextType& contextType, glu::ContextFlags ctxFlags)
{
	DE_ASSERT(!m_renderCtx && !m_contextInfo);

	try
	{
		glu::RenderConfig renderCfg(glu::ContextType(contextType.getAPI(), contextType.getFlags() | ctxFlags));

		glu::parseRenderConfig(&renderCfg, m_testCtx.getCommandLine());

		if (m_testCtx.getCommandLine().getRunMode() != tcu::RUNMODE_EXECUTE)
		{
			// \todo [2016-11-16 pyry] Create DummyRenderContext instead to allow generating all case lists
			//						   on a system that doesn't support some GL(ES) versions.
			renderCfg.surfaceType = glu::RenderConfig::SURFACETYPE_OFFSCREEN_GENERIC;
		}

		m_renderCtx   = glu::createRenderContext(m_testCtx.getPlatform(), m_testCtx.getCommandLine(), renderCfg);
		m_contextInfo = glu::ContextInfo::create(*m_renderCtx);

		glw::setCurrentThreadFunctions(&m_renderCtx->getFunctions());
	}
	catch (...)
	{
		destroyRenderContext();
		throw;
	}
}

void Context::destroyRenderContext(void)
{
	delete m_contextInfo;
	delete m_renderCtx;

	m_contextInfo = DE_NULL;
	m_renderCtx   = DE_NULL;
}

const tcu::RenderTarget& Context::getRenderTarget(void) const
{
	return m_renderCtx->getRenderTarget();
}

} // deqp
