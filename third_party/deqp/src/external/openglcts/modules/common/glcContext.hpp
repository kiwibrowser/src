#ifndef _GLCCONTEXT_HPP
#define _GLCCONTEXT_HPP
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
 * \brief OpenGL context.
 */ /*-------------------------------------------------------------------*/

#include "gluRenderConfig.hpp"
#include "gluRenderContext.hpp"
#include "tcuDefs.hpp"
#include "tcuTestContext.hpp"

namespace glu
{
class RenderContext;
class ContextInfo;
}

namespace tcu
{
class RenderTarget;
}

namespace deqp
{

class Context
{
public:
	Context(tcu::TestContext& testCtx, glu::ContextType contextType = glu::ContextType());
	~Context(void);

	tcu::TestContext& getTestContext(void)
	{
		return m_testCtx;
	}

	glu::RenderContext& getRenderContext(void)
	{
		return *m_renderCtx;
	}

	void setRenderContext(glu::RenderContext* renderCtx)
	{
		m_renderCtx = renderCtx;
	}

	const glu::ContextInfo& getContextInfo(void) const
	{
		return *m_contextInfo;
	}

	const tcu::RenderTarget& getRenderTarget(void) const;

private:
	Context(const Context& other);
	Context& operator=(const Context& other);

	void createRenderContext(glu::ContextType& contextType, glu::ContextFlags ctxFlags = (glu::ContextFlags)0);
	void destroyRenderContext(void);

	tcu::TestContext&   m_testCtx;
	glu::RenderContext* m_renderCtx;
	glu::ContextInfo*   m_contextInfo;
	glu::ContextType	m_contextType;
};

} // deqp

#endif // _GLCCONTEXT_HPP
