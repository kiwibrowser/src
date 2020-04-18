#ifndef _ESEXTCDRAWBUFFERSINDEXEDBASE_HPP
#define _ESEXTCDRAWBUFFERSINDEXEDBASE_HPP
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
 * \file  esextcDrawBuffersIndexedBase.hpp
 * \brief Base class for Draw Buffers Indexed extension tests 1-6
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "glwEnums.hpp"
#include <vector>

namespace glcts
{

/** Base class for Draw Buffers Indexed tests
 **/
class DrawBuffersIndexedBase : public TestCaseBase
{
public:
	/** Public methods
	 **/
	DrawBuffersIndexedBase(Context& context, const ExtParameters& extParams, const char* name, const char* description);

	virtual ~DrawBuffersIndexedBase()
	{
	}

private:
	virtual void init(void);

protected:
	class BlendMaskState
	{
	public:
		glw::GLenum	enable;
		glw::GLenum	mode_rgb;
		glw::GLenum	mode_a;
		glw::GLenum	func_src_rgb;
		glw::GLenum	func_src_a;
		glw::GLenum	func_dst_rgb;
		glw::GLenum	func_dst_a;
		glw::GLboolean mask_r;
		glw::GLboolean mask_g;
		glw::GLboolean mask_b;
		glw::GLboolean mask_a;

		BlendMaskState()
			: enable(GL_FALSE)
			, mode_rgb(GL_FUNC_ADD)
			, mode_a(GL_FUNC_ADD)
			, func_src_rgb(GL_ONE)
			, func_src_a(GL_ONE)
			, func_dst_rgb(GL_ZERO)
			, func_dst_a(GL_ZERO)
			, mask_r(GL_TRUE)
			, mask_g(GL_TRUE)
			, mask_b(GL_TRUE)
			, mask_a(GL_TRUE)
		{
		}
	};

	class BlendMaskStateMachine
	{
	public:
		BlendMaskStateMachine(Context& context, tcu::TestLog& log);
		BlendMaskStateMachine(Context& context, tcu::TestLog& log, int dbs);

		std::vector<BlendMaskState> state;

		bool CheckEnumGeneral(glw::GLenum e, glw::GLenum state);
		bool CheckEnumForBuffer(int idx, glw::GLenum e, glw::GLenum state);
		bool CheckBuffer(int idx);
		bool CheckAll();

		void SetEnablei(int idx);
		void SetDisablei(int idx);
		void SetColorMaski(int idx, glw::GLboolean r, glw::GLboolean g, glw::GLboolean b, glw::GLboolean a);
		void SetBlendEquationi(int idx, glw::GLenum mode);
		void SetBlendEquationSeparatei(int idx, glw::GLenum rgb, glw::GLenum a);
		void SetBlendFunci(int idx, glw::GLenum src, glw::GLenum dst);
		void SetBlendFuncSeparatei(int idx, glw::GLenum src_rgb, glw::GLenum dst_rgb, glw::GLenum src_a,
								   glw::GLenum dst_a);
		void SetEnable();
		void SetDisable();
		void SetColorMask(glw::GLboolean r, glw::GLboolean g, glw::GLboolean b, glw::GLboolean a);
		void SetBlendEquation(glw::GLenum mode);
		void SetBlendEquationSeparate(glw::GLenum rgb, glw::GLenum a);
		void SetBlendFunc(glw::GLenum src, glw::GLenum dst);
		void SetBlendFuncSeparate(glw::GLenum src_rgb, glw::GLenum dst_rgb, glw::GLenum src_a, glw::GLenum dst_a);
		void SetDefaults();

	private:
		const glw::Functions& gl;
		tcu::TestLog&		  testLog;
	};
};
}
#endif // _ESEXTCDRAWBUFFERSINDEXEDBASE_HPP
