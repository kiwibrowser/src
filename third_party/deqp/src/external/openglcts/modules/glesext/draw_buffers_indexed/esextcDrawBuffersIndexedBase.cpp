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
 * \file esextcDrawBuffersIndexedBase.cpp
 * \brief Base class for Draw Buffers Indexed extension tests 1-6
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedBase.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Base class constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
DrawBuffersIndexedBase::DrawBuffersIndexedBase(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description)
	: TestCaseBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

void DrawBuffersIndexedBase::init()
{
	if (!isExtensionSupported("GL_OES_draw_buffers_indexed"))
	{
		throw tcu::NotSupportedError(DRAW_BUFFERS_INDEXED_NOT_SUPPORTED);
	}
}

/** Helper class constructors
 *
 *  @param context     Test context
 **/
DrawBuffersIndexedBase::BlendMaskStateMachine::BlendMaskStateMachine(Context& context, tcu::TestLog& log)
	: state(4), gl(context.getRenderContext().getFunctions()), testLog(log)
{
}

DrawBuffersIndexedBase::BlendMaskStateMachine::BlendMaskStateMachine(Context& context, tcu::TestLog& log, int dbs)
	: state(dbs), gl(context.getRenderContext().getFunctions()), testLog(log)
{
}

/** Helper class member functions
 *
 **/
bool DrawBuffersIndexedBase::BlendMaskStateMachine::CheckEnumGeneral(glw::GLenum e, glw::GLenum s)
{
	glw::GLint	 i;
	glw::GLint64   li;
	glw::GLboolean b;
	glw::GLfloat   f;

	gl.getIntegerv(e, &i);
	gl.getInteger64v(e, &li);
	gl.getBooleanv(e, &b);
	gl.getFloatv(e, &f);
	if ((static_cast<glw::GLenum>(i) != s) || (static_cast<glw::GLenum>(li) != s) ||
		(static_cast<glw::GLenum>(f) != s) || (b != (s ? GL_TRUE : GL_FALSE)))
	{
		testLog << tcu::TestLog::Message << "General state should be set to " << s
				<< " but found the following values:\n"
				<< "int: " << i << "\n"
				<< "int64: " << li << "\n"
				<< "bool: " << b << tcu::TestLog::EndMessage;
		return false;
	}
	return true;
}

bool DrawBuffersIndexedBase::BlendMaskStateMachine::CheckEnumForBuffer(int idx, glw::GLenum e, glw::GLenum s)
{
	glw::GLint	 i;
	glw::GLint64   li;
	glw::GLboolean b;

	gl.getIntegeri_v(e, idx, &i);
	gl.getInteger64i_v(e, idx, &li);
	gl.getBooleani_v(e, idx, &b);
	if ((static_cast<glw::GLenum>(i) != s) || (static_cast<glw::GLenum>(li) != s) || (b != (s ? GL_TRUE : GL_FALSE)))
	{
		testLog << tcu::TestLog::Message << "State for " << e << " in buffer #" << idx << " should be set to " << s
				<< " but found the following values:\n"
				<< "int: " << i << "\n"
				<< "int64: " << li << "\n"
				<< "bool: " << b << tcu::TestLog::EndMessage;
		return false;
	}
	return true;
}

bool DrawBuffersIndexedBase::BlendMaskStateMachine::CheckBuffer(int idx)
{
	if (gl.isEnabledi(GL_BLEND, idx) != state[idx].enable)
	{
		testLog << tcu::TestLog::Message << "Blending for buffer #" << idx << " set to: " << !state[idx].enable
				<< " but should be " << state[idx].enable << "!" << tcu::TestLog::EndMessage;
		return false;
	}

	bool result = true;

	result &= CheckEnumForBuffer(idx, GL_BLEND_EQUATION_RGB, state[idx].mode_rgb);
	result &= CheckEnumForBuffer(idx, GL_BLEND_EQUATION_ALPHA, state[idx].mode_a);
	result &= CheckEnumForBuffer(idx, GL_BLEND_SRC_RGB, state[idx].func_src_rgb);
	result &= CheckEnumForBuffer(idx, GL_BLEND_SRC_ALPHA, state[idx].func_src_a);
	result &= CheckEnumForBuffer(idx, GL_BLEND_DST_RGB, state[idx].func_dst_rgb);
	result &= CheckEnumForBuffer(idx, GL_BLEND_DST_ALPHA, state[idx].func_dst_a);

	glw::GLint	 ia[4];
	glw::GLint64   lia[4];
	glw::GLboolean ba[4];

	gl.getIntegeri_v(GL_COLOR_WRITEMASK, idx, ia);
	gl.getInteger64i_v(GL_COLOR_WRITEMASK, idx, lia);
	gl.getBooleani_v(GL_COLOR_WRITEMASK, idx, ba);
	if ((ia[0] != state[idx].mask_r) || (static_cast<int>(lia[0]) != state[idx].mask_r) ||
		(static_cast<int>(ba[0]) != state[idx].mask_r) || (ia[1] != state[idx].mask_g) ||
		(static_cast<int>(lia[1]) != state[idx].mask_g) || (static_cast<int>(ba[1]) != state[idx].mask_g) ||
		(ia[2] != state[idx].mask_b) || (static_cast<int>(lia[2]) != state[idx].mask_b) ||
		(static_cast<int>(ba[2]) != state[idx].mask_b) || (ia[3] != state[idx].mask_a) ||
		(static_cast<int>(lia[3]) != state[idx].mask_a) || (static_cast<int>(ba[3]) != state[idx].mask_a))
	{
		testLog << tcu::TestLog::Message << "GL_COLOR_WRITEMASK for buffer #" << idx << " should be set to("
				<< state[idx].mask_r << ", " << state[idx].mask_g << ", " << state[idx].mask_b << ", "
				<< state[idx].mask_a << ")\n"
				<< "but the following values was set:\n"
				<< "int: " << ia[0] << ", " << ia[1] << ", " << ia[2] << ", " << ia[3] << "\n"
				<< "int64: " << lia[0] << ", " << lia[1] << ", " << lia[2] << ", " << lia[3] << "\n"
				<< "bool: " << ba[0] << ", " << ba[1] << ", " << ba[2] << ", " << ba[3] << tcu::TestLog::EndMessage;
		result = false;
	}
	if (idx == 0)
	{
		result &= CheckEnumGeneral(GL_BLEND_EQUATION_RGB, state[idx].mode_rgb);
		result &= CheckEnumGeneral(GL_BLEND_EQUATION_ALPHA, state[idx].mode_a);
		result &= CheckEnumGeneral(GL_BLEND_SRC_RGB, state[idx].func_src_rgb);
		result &= CheckEnumGeneral(GL_BLEND_SRC_ALPHA, state[idx].func_src_a);
		result &= CheckEnumGeneral(GL_BLEND_DST_RGB, state[idx].func_dst_rgb);
		result &= CheckEnumGeneral(GL_BLEND_DST_ALPHA, state[idx].func_dst_a);

		glw::GLfloat fa[4];

		gl.getIntegerv(GL_COLOR_WRITEMASK, ia);
		gl.getInteger64v(GL_COLOR_WRITEMASK, lia);
		gl.getBooleanv(GL_COLOR_WRITEMASK, ba);
		gl.getFloatv(GL_COLOR_WRITEMASK, fa);
		if ((ia[0] != state[idx].mask_r) || (static_cast<int>(lia[0]) != state[idx].mask_r) ||
			(ia[1] != state[idx].mask_g) || (static_cast<int>(lia[1]) != state[idx].mask_g) ||
			(ia[2] != state[idx].mask_b) || (static_cast<int>(lia[2]) != state[idx].mask_b) ||
			(ia[3] != state[idx].mask_a) || (static_cast<int>(lia[3]) != state[idx].mask_a) ||
			(static_cast<int>(ba[0]) != state[idx].mask_r) || (static_cast<int>(fa[0]) != state[idx].mask_r) ||
			(static_cast<int>(ba[1]) != state[idx].mask_g) || (static_cast<int>(fa[1]) != state[idx].mask_g) ||
			(static_cast<int>(ba[2]) != state[idx].mask_b) || (static_cast<int>(fa[2]) != state[idx].mask_b) ||
			(static_cast<int>(ba[3]) != state[idx].mask_a) || (static_cast<int>(fa[3]) != state[idx].mask_a))
		{
			testLog << tcu::TestLog::Message << "GL_COLOR_WRITEMASK for buffer #" << idx << " should be set to("
					<< state[idx].mask_r << ", " << state[idx].mask_g << ", " << state[idx].mask_b << ", "
					<< state[idx].mask_a << ")\n"
					<< "but the following values was set:\n"
					<< "int: " << ia[0] << ", " << ia[1] << ", " << ia[2] << ", " << ia[3] << "\n"
					<< "int64: " << lia[0] << ", " << lia[1] << ", " << lia[2] << ", " << lia[3] << "\n"
					<< "bool: " << ba[0] << ", " << ba[1] << ", " << ba[2] << ", " << ba[3] << "\n"
					<< "float: " << fa[0] << ", " << fa[1] << ", " << fa[2] << ", " << fa[3]
					<< tcu::TestLog::EndMessage;
			result = false;
		}
	}
	return result;
}

bool DrawBuffersIndexedBase::BlendMaskStateMachine::CheckAll()
{
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		if (!CheckBuffer(i))
		{
			return false;
		}
	}
	return true;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetEnablei(int idx)
{
	gl.enablei(GL_BLEND, idx);
	state[idx].enable = GL_TRUE;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetDisablei(int idx)
{
	gl.disablei(GL_BLEND, idx);
	state[idx].enable = GL_FALSE;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetColorMaski(int idx, glw::GLboolean r, glw::GLboolean g,
																  glw::GLboolean b, glw::GLboolean a)
{
	gl.colorMaski(idx, r, g, b, a);
	state[idx].mask_r = r;
	state[idx].mask_g = g;
	state[idx].mask_b = b;
	state[idx].mask_a = a;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendEquationi(int idx, glw::GLenum mode)
{
	gl.blendEquationi(idx, mode);
	state[idx].mode_rgb = mode;
	state[idx].mode_a   = mode;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendEquationSeparatei(int idx, glw::GLenum rgb, glw::GLenum a)
{
	gl.blendEquationSeparatei(idx, rgb, a);
	state[idx].mode_rgb = rgb;
	state[idx].mode_a   = a;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendFunci(int idx, glw::GLenum src, glw::GLenum dst)
{
	gl.blendFunci(idx, src, dst);
	state[idx].func_src_rgb = src;
	state[idx].func_src_a   = src;
	state[idx].func_dst_rgb = dst;
	state[idx].func_dst_a   = dst;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendFuncSeparatei(int idx, glw::GLenum src_rgb,
																		  glw::GLenum dst_rgb, glw::GLenum src_a,
																		  glw::GLenum dst_a)
{
	gl.blendFuncSeparatei(idx, src_rgb, dst_rgb, src_a, dst_a);
	state[idx].func_src_rgb = src_rgb;
	state[idx].func_src_a   = src_a;
	state[idx].func_dst_rgb = dst_rgb;
	state[idx].func_dst_a   = dst_a;
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetEnable()
{
	// all draw buffers
	gl.enable(GL_BLEND);
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		state[i].enable = GL_TRUE;
	}
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetDisable()
{
	// all draw buffers
	gl.disable(GL_BLEND);
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		state[i].enable = GL_FALSE;
	}
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetColorMask(glw::GLboolean r, glw::GLboolean g, glw::GLboolean b,
																 glw::GLboolean a)
{
	// all draw buffers
	gl.colorMask(r, g, b, a);
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		state[i].mask_r = r;
		state[i].mask_g = g;
		state[i].mask_b = b;
		state[i].mask_a = a;
	}
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendEquation(glw::GLenum mode)
{
	// all draw buffers
	gl.blendEquation(mode);
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		state[i].mode_rgb = mode;
		state[i].mode_a   = mode;
	}
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendEquationSeparate(glw::GLenum rgb, glw::GLenum a)
{
	// all draw buffers
	gl.blendEquationSeparate(rgb, a);
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		state[i].mode_rgb = rgb;
		state[i].mode_a   = a;
	}
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendFunc(glw::GLenum src, glw::GLenum dst)
{
	// all draw buffers
	gl.blendFunc(src, dst);
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		state[i].func_src_rgb = src;
		state[i].func_src_a   = src;
		state[i].func_dst_rgb = dst;
		state[i].func_dst_a   = dst;
	}
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetBlendFuncSeparate(glw::GLenum src_rgb, glw::GLenum dst_rgb,
																		 glw::GLenum src_a, glw::GLenum dst_a)
{
	// all draw buffers
	gl.blendFuncSeparate(src_rgb, dst_rgb, src_a, dst_a);
	for (unsigned int i = 0; i < state.size(); ++i)
	{
		state[i].func_src_rgb = src_rgb;
		state[i].func_src_a   = src_a;
		state[i].func_dst_rgb = dst_rgb;
		state[i].func_dst_a   = dst_a;
	}
}

void DrawBuffersIndexedBase::BlendMaskStateMachine::SetDefaults()
{
	SetDisable();
	SetColorMask(1, 1, 1, 1);
	SetBlendEquation(GL_FUNC_ADD);
	SetBlendFunc(GL_ONE, GL_ZERO);
}

} // namespace glcts
