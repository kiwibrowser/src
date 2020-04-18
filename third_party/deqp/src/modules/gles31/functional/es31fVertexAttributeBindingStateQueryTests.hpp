#ifndef _ES31FVERTEXATTRIBUTEBINDINGSTATEQUERYTESTS_HPP
#define _ES31FVERTEXATTRIBUTEBINDINGSTATEQUERYTESTS_HPP
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
 * \brief Vertex attribute binding state query tests.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

class VertexAttributeBindingStateQueryTests : public TestCaseGroup
{
public:
											VertexAttributeBindingStateQueryTests	(Context& context);
											~VertexAttributeBindingStateQueryTests	(void);

	void									init									(void);

private:
											VertexAttributeBindingStateQueryTests	(const VertexAttributeBindingStateQueryTests& other);
	VertexAttributeBindingStateQueryTests&	operator=								(const VertexAttributeBindingStateQueryTests& other);
};

} // Functional
} // gles31
} // deqp

#endif // _ES31FVERTEXATTRIBUTEBINDINGSTATEQUERYTESTS_HPP
