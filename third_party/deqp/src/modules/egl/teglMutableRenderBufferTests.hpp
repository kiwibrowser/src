#ifndef _TEGLMUTABLERENDERBUFFERTESTS_HPP
#define _TEGLMUTABLERENDERBUFFERTESTS_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Test KHR_mutable_render_buffer
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "teglTestCase.hpp"

namespace deqp
{
namespace egl
{

class MutableRenderBufferTests : public TestCaseGroup
{
public:
								MutableRenderBufferTests	(EglTestContext& eglTestCtx);
	void						init						(void);

private:
								MutableRenderBufferTests	(const MutableRenderBufferTests&);
	MutableRenderBufferTests&	operator=					(const MutableRenderBufferTests&);
};

} // egl
} // deqp

#endif // _TEGLMUTABLERENDERBUFFERTESTS_HPP
