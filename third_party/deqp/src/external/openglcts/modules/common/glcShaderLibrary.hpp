#ifndef _GLCSHADERLIBRARY_HPP
#define _GLCSHADERLIBRARY_HPP
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
 * \brief Shader case library.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"

#include <vector>

namespace deqp
{

class ShaderLibrary
{
public:
	ShaderLibrary(tcu::TestContext& testCtx, glu::RenderContext& renderCtx);
	~ShaderLibrary(void);

	std::vector<tcu::TestNode*> loadShaderFile(const char* fileName);

private:
	ShaderLibrary(const ShaderLibrary&);			// not allowed!
	ShaderLibrary& operator=(const ShaderLibrary&); // not allowed!

	// Member variables.
	tcu::TestContext&   m_testCtx;
	glu::RenderContext& m_renderCtx;
};

class ShaderLibraryGroup : public TestCaseGroup
{
public:
	ShaderLibraryGroup(Context& context, const char* name, const char* description, const char* filename);
	~ShaderLibraryGroup(void);

	void init(void);

private:
	std::string m_filename;
};

} // deqp

#endif // _GLCSHADERLIBRARY_HPP
