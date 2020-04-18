#ifndef _GL4CVERTEXATTRIB64BITTEST_HPP
#define _GL4CVERTEXATTRIB64BITTEST_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 * @file  gl4cVertexAttrib64BitTests.hpp
 * @brief Declare classes testing GL_ARB_vertex_attrib_64bit functionality
 **/

#include "glcTestCase.hpp"

namespace gl4cts
{
class VertexAttrib64BitTests : public deqp::TestCaseGroup
{

public:
	VertexAttrib64BitTests(deqp::Context& context);

	virtual ~VertexAttrib64BitTests()
	{
	}

	virtual void init();

private:
	/* Block copy constructor and operator = */
	VertexAttrib64BitTests(const VertexAttrib64BitTests&);
	VertexAttrib64BitTests& operator=(const VertexAttrib64BitTests&);
};

} /* namespace gl4cts */

#endif // _GL4CVERTEXATTRIB64BITTEST_HPP
