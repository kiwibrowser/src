#ifndef _GLCPACKEDDEPTHSTENCILTESTS_HPP
#define _GLCPACKEDDEPTHSTENCILTESTS_HPP
/*-------------------------------------------------------------------------
* OpenGL Conformance Test Suite
* -----------------------------
*
* Copyright (c) 2017 The Khronos Group Inc.
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
* \file  glcPackedDepthStencilTests.hpp
* \brief
*/ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"

namespace glcts
{

class PackedDepthStencilTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	PackedDepthStencilTests(deqp::Context& context);
	virtual ~PackedDepthStencilTests(void);

	void init(void);

private:
	/* Private methods */
	PackedDepthStencilTests(const PackedDepthStencilTests& other);
	PackedDepthStencilTests& operator=(const PackedDepthStencilTests& other);
};

} /* glcts namespace */

#endif // _GLCPACKEDDEPTHSTENCILTESTS_HPP
