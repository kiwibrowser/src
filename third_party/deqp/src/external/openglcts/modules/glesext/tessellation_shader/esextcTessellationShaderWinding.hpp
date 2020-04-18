#ifndef _ESEXTCTESSELLATIONSHADERWINDING_HPP
#define _ESEXTCTESSELLATIONSHADERWINDING_HPP
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
 * \file esextcTessellationShaderWinding.hpp
 * \brief Test winding order with tessellation shaders
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "esextcTessellationShaderUtils.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace glcts
{

class TesselationShaderWindingTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TesselationShaderWindingTests(glcts::Context& context, const ExtParameters& extParams);

	virtual ~TesselationShaderWindingTests(void)
	{
	}

	void init(void);

private:
	/* Private methods */
	TesselationShaderWindingTests(const TesselationShaderWindingTests& other);
	TesselationShaderWindingTests& operator=(const TesselationShaderWindingTests& other);
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERWINDING_HPP
