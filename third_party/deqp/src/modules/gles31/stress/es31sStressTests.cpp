/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
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
 * \brief Stress tests.
 *//*--------------------------------------------------------------------*/

#include "es31sStressTests.hpp"

#include "es31sDrawTests.hpp"
#include "es31sVertexAttributeBindingTests.hpp"
#include "es31sTessellationGeometryInteractionTests.hpp"

namespace deqp
{
namespace gles31
{
namespace Stress
{

StressTests::StressTests (Context& context)
	: TestCaseGroup(context, "stress", "Stress tests")
{
}

StressTests::~StressTests (void)
{
}

void StressTests::init (void)
{
	addChild(new DrawTests								(m_context));
	addChild(new VertexAttributeBindingTests			(m_context));
	addChild(new TessellationGeometryInteractionTests	(m_context));
}

} // Stress
} // gles31
} // deqp
