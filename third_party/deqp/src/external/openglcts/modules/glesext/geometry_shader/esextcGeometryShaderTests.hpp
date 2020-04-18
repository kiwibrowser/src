#ifndef _ESEXTCGEOMETRYSHADERTESTS_HPP
#define _ESEXTCGEOMETRYSHADERTESTS_HPP
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

#include "../esextcTestCaseBase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace glcts
{
/**
 * This test set verifies API coverage and functionality of the EXT_geometry_shader extension,
 * including a number of dependencies with extensions specific to OpenGL ES 3.1.
 */
class GeometryShaderTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	GeometryShaderTests(glcts::Context& context, const ExtParameters& extParams);

	~GeometryShaderTests(void)
	{
	}

	void init(void);

private:
	/* Private methods */
	GeometryShaderTests(const GeometryShaderTests& other);
	GeometryShaderTests& operator=(const GeometryShaderTests& other);
};

} // glcts

#endif // _ESEXTCGEOMETRYSHADERTESTS_HPP
