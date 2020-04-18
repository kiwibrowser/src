/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/*!
 * \file esextcDrawBuffersIndexedTests.cpp
 * \brief Test group for Draw Buffers Indexed tests
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedTests.hpp"
#include "esextcDrawBuffersIndexedBlending.hpp"
#include "esextcDrawBuffersIndexedColorMasks.hpp"
#include "esextcDrawBuffersIndexedCoverage.hpp"
#include "esextcDrawBuffersIndexedDefaultState.hpp"
#include "esextcDrawBuffersIndexedNegative.hpp"
#include "esextcDrawBuffersIndexedSetGet.hpp"
#include "glwEnums.hpp"

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param glslVersion   GLSL version
 **/
DrawBuffersIndexedTests::DrawBuffersIndexedTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "draw_buffers_indexed", "Draw Buffers Indexed Tests")
{
	/* No implementation needed */
}

/** Initializes test cases for Draw Buffers Indexed tests
 **/
void DrawBuffersIndexedTests::init(void)
{
	/* Initialize base class */
	TestCaseGroupBase::init();

	/* Draw Buffers Indexed - 1. Coverage */
	addChild(new DrawBuffersIndexedCoverage(m_context, m_extParams, "coverage", "Basic coverage test"));

	/* Draw Buffers Indexed - 2. Default state */
	addChild(
		new DrawBuffersIndexedDefaultState(m_context, m_extParams, "default_state", "Default state verification test"));

	/* Draw Buffers Indexed - 3. Set and get */
	addChild(new DrawBuffersIndexedSetGet(m_context, m_extParams, "set_get", "Setting and getting state test"));

	/* Draw Buffers Indexed - 4. Color masks */
	addChild(new DrawBuffersIndexedColorMasks(m_context, m_extParams, "color_masks", "Masking color test"));

	/* Draw Buffers Indexed - 5. Blending */
	addChild(new DrawBuffersIndexedBlending(m_context, m_extParams, "blending", "Blending test"));

	/* Draw Buffers Indexed - 6. Negative */
	addChild(new DrawBuffersIndexedNegative(m_context, m_extParams, "negative", "Negative test"));
}

} // namespace glcts
