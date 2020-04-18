#ifndef _GL4CTEXTUREBARRIERTESTS_HPP
#define _GL4CTEXTUREBARRIERTESTS_HPP
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

#include "glcTestCase.hpp"

namespace gl4cts
{
class TextureBarrierTests : public deqp::TestCaseGroup
{
public:
	enum API
	{
		API_GL_45core,
		API_GL_ARB_texture_barrier,
	};

	TextureBarrierTests(deqp::Context& context, API api);
	~TextureBarrierTests(void);
	void init(void);

private:
	TextureBarrierTests(const TextureBarrierTests& other);
	TextureBarrierTests& operator=(const TextureBarrierTests& other);
	API m_api;
};

} /* gl4cts namespace */

#endif // _GL4CTEXTUREBARRIERTESTS_HPP
