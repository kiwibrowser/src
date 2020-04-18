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
 * \brief Negative Fragment Pipe API tests.
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeFragmentApiTests.hpp"

#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluRenderContext.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{

using tcu::TestLog;
using glu::CallLogWrapper;
using namespace glw;

using tcu::TestLog;

void scissor (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if either width or height is negative.");
	ctx.glScissor(0, 0, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glScissor(0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glScissor(0, 0, -1, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void depth_func (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if func is not an accepted value.");
	ctx.glDepthFunc(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void viewport (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if either width or height is negative.");
	ctx.glViewport(0, 0, -1, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glViewport(0, 0, 1, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glViewport(0, 0, -1, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// Stencil functions
void stencil_func (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if func is not one of the eight accepted values.");
	ctx.glStencilFunc(-1, 0, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void stencil_func_separate (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if face is not GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK.");
	ctx.glStencilFuncSeparate(-1, GL_NEVER, 0, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if func is not one of the eight accepted values.");
	ctx.glStencilFuncSeparate(GL_FRONT, -1, 0, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void stencil_op (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if sfail, dpfail, or dppass is any value other than the defined symbolic constant values.");
	ctx.glStencilOp(-1, GL_ZERO, GL_REPLACE);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glStencilOp(GL_KEEP, -1, GL_REPLACE);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glStencilOp(GL_KEEP, GL_ZERO, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void stencil_op_separate (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if face is any value other than GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK.");
	ctx.glStencilOpSeparate(-1, GL_KEEP, GL_ZERO, GL_REPLACE);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if sfail, dpfail, or dppass is any value other than the eight defined symbolic constant values.");
	ctx.glStencilOpSeparate(GL_FRONT, -1, GL_ZERO, GL_REPLACE);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glStencilOpSeparate(GL_FRONT, GL_KEEP, -1, GL_REPLACE);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_ZERO, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void stencil_mask_separate (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if face is not GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK.");
	ctx.glStencilMaskSeparate(-1, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

// Blend functions
void blend_equation (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if mode is not GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MAX or GL_MIN.");
	ctx.glBlendEquation(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void blend_equation_separate (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if modeRGB is not GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MAX or GL_MIN.");
	ctx.glBlendEquationSeparate(-1, GL_FUNC_ADD);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
	ctx.beginSection("GL_INVALID_ENUM is generated if modeAlpha is not GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MAX or GL_MIN.");
	ctx.glBlendEquationSeparate(GL_FUNC_ADD, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void blend_equationi (NegativeTestContext& ctx)
{
	glw::GLint maxDrawBuffers = -1;

	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !ctx.getContextInfo().isExtensionSupported("GL_EXT_draw_buffers_indexed"))
		throw tcu::NotSupportedError("GL_EXT_draw_buffers_indexed is not supported", DE_NULL, __FILE__, __LINE__);

	ctx.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	ctx.beginSection("GL_INVALID_ENUM is generated if mode is not GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MAX or GL_MIN.");
	ctx.glBlendEquationi(0, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
	ctx.beginSection("GL_INVALID_VALUE is generated if buf is not in the range zero to the value of MAX_DRAW_BUFFERS minus one.");
	ctx.glBlendEquationi(-1, GL_FUNC_ADD);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glBlendEquationi(maxDrawBuffers, GL_FUNC_ADD);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void blend_equation_separatei (NegativeTestContext& ctx)
{
	glw::GLint maxDrawBuffers = -1;

	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !ctx.getContextInfo().isExtensionSupported("GL_EXT_draw_buffers_indexed"))
		throw tcu::NotSupportedError("GL_EXT_draw_buffers_indexed is not supported", DE_NULL, __FILE__, __LINE__);

	ctx.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	ctx.beginSection("GL_INVALID_ENUM is generated if modeRGB is not GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MAX or GL_MIN.");
	ctx.glBlendEquationSeparatei(0, -1, GL_FUNC_ADD);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
	ctx.beginSection("GL_INVALID_ENUM is generated if modeAlpha is not GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MAX or GL_MIN.");
	ctx.glBlendEquationSeparatei(0, GL_FUNC_ADD, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
	ctx.beginSection("GL_INVALID_VALUE is generated if buf is not in the range zero to the value of MAX_DRAW_BUFFERS minus one.");
	ctx.glBlendEquationSeparatei(-1, GL_FUNC_ADD, GL_FUNC_ADD);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glBlendEquationSeparatei(maxDrawBuffers, GL_FUNC_ADD, GL_FUNC_ADD);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void blend_func (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if either sfactor or dfactor is not an accepted value.");
	ctx.glBlendFunc(-1, GL_ONE);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFunc(GL_ONE, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void blend_func_separate (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if srcRGB, dstRGB, srcAlpha, or dstAlpha is not an accepted value.");
	ctx.glBlendFuncSeparate(-1, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFuncSeparate(GL_ZERO, -1, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFuncSeparate(GL_ZERO, GL_ONE, -1, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_SRC_COLOR, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void blend_funci (NegativeTestContext& ctx)
{
	glw::GLint maxDrawBuffers = -1;

	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !ctx.getContextInfo().isExtensionSupported("GL_EXT_draw_buffers_indexed"))
		throw tcu::NotSupportedError("GL_EXT_draw_buffers_indexed is not supported", DE_NULL, __FILE__, __LINE__);

	ctx.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	ctx.beginSection("GL_INVALID_ENUM is generated if either sfactor or dfactor is not an accepted value.");
	ctx.glBlendFunci(0, -1, GL_ONE);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFunci(0, GL_ONE, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
	ctx.beginSection("GL_INVALID_VALUE is generated if buf is not in the range zero to the value of MAX_DRAW_BUFFERS minus one.");
	ctx.glBlendFunci(-1, GL_ONE, GL_ONE);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glBlendFunci(maxDrawBuffers, GL_ONE, GL_ONE);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void blend_func_separatei (NegativeTestContext& ctx)
{
	glw::GLint maxDrawBuffers = -1;

	if (!glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !ctx.getContextInfo().isExtensionSupported("GL_EXT_draw_buffers_indexed"))
		throw tcu::NotSupportedError("GL_EXT_draw_buffers_indexed is not supported", DE_NULL, __FILE__, __LINE__);

	ctx.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	ctx.beginSection("GL_INVALID_ENUM is generated if srcRGB, dstRGB, srcAlpha, or dstAlpha is not an accepted value.");
	ctx.glBlendFuncSeparatei(0, -1, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFuncSeparatei(0, GL_ZERO, -1, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFuncSeparatei(0, GL_ZERO, GL_ONE, -1, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBlendFuncSeparatei(0, GL_ZERO, GL_ONE, GL_SRC_COLOR, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
	ctx.beginSection("GL_INVALID_VALUE is generated if buf is not in the range zero to the value of MAX_DRAW_BUFFERS minus one.");
	ctx.glBlendFuncSeparatei(-1, GL_ONE, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glBlendFuncSeparatei(maxDrawBuffers, GL_ONE, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// Rasterization API functions
void cull_face (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if mode is not an accepted value.");
	ctx.glCullFace(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void front_face (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if mode is not an accepted value.");
	ctx.glFrontFace(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void line_width (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if width is less than or equal to 0.");
	ctx.glLineWidth(0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glLineWidth(-1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// Asynchronous queries
void gen_queries (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if n is negative.");
	GLuint ids = 0;
	ctx.glGenQueries	(-1, &ids);
	ctx.expectError		(GL_INVALID_VALUE);
	ctx.endSection();
}

void begin_query (NegativeTestContext& ctx)
{
	GLuint ids[3];
	ctx.glGenQueries	(3, ids);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not one of the accepted tokens.");
	ctx.glBeginQuery	(-1, ids[0]);
	ctx.expectError		(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if ctx.glBeginQuery is executed while a query object of the same target is already active.");
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED, ids[0]);
	ctx.expectError		(GL_NO_ERROR);
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED, ids[1]);
	ctx.expectError		(GL_INVALID_OPERATION);
	// \note GL_ANY_SAMPLES_PASSED and GL_ANY_SAMPLES_PASSED_CONSERVATIVE alias to the same target for the purposes of this error.
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, ids[1]);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glBeginQuery	(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, ids[1]);
	ctx.expectError		(GL_NO_ERROR);
	ctx.glBeginQuery	(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, ids[2]);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glEndQuery		(GL_ANY_SAMPLES_PASSED);
	ctx.glEndQuery		(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	ctx.expectError		(GL_NO_ERROR);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if id is 0.");
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED, 0);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if id not a name returned from a previous call to ctx.glGenQueries, or if such a name has since been deleted with ctx.glDeleteQueries.");
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED, -1);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glDeleteQueries	(1, &ids[2]);
	ctx.expectError		(GL_NO_ERROR);
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED, ids[2]);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if id is the name of an already active query object.");
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED, ids[0]);
	ctx.expectError		(GL_NO_ERROR);
	ctx.glBeginQuery	(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, ids[0]);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if id refers to an existing query object whose type does not does not match target.");
	ctx.glEndQuery		(GL_ANY_SAMPLES_PASSED);
	ctx.expectError		(GL_NO_ERROR);
	ctx.glBeginQuery	(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, ids[0]);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteQueries	(2, &ids[0]);
	ctx.expectError		(GL_NO_ERROR);
}

void end_query (NegativeTestContext& ctx)
{
	GLuint id = 0;
	ctx.glGenQueries	(1, &id);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not one of the accepted tokens.");
	ctx.glEndQuery		(-1);
	ctx.expectError		(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if ctx.glEndQuery is executed when a query object of the same target is not active.");
	ctx.glEndQuery		(GL_ANY_SAMPLES_PASSED);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glBeginQuery	(GL_ANY_SAMPLES_PASSED, id);
	ctx.expectError		(GL_NO_ERROR);
	ctx.glEndQuery		(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glEndQuery		(GL_ANY_SAMPLES_PASSED);
	ctx.expectError		(GL_NO_ERROR);
	ctx.endSection();

	ctx.glDeleteQueries	(1, &id);
	ctx.expectError		(GL_NO_ERROR);
}

void delete_queries (NegativeTestContext& ctx)
{
	GLuint id = 0;
	ctx.glGenQueries	(1, &id);

	ctx.beginSection("GL_INVALID_VALUE is generated if n is negative.");
	ctx.glDeleteQueries	(-1, &id);
	ctx.expectError		(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteQueries	(1, &id);
}

// Sync objects
void fence_sync (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if condition is not GL_SYNC_GPU_COMMANDS_COMPLETE.");
	ctx.glFenceSync(-1, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if flags is not zero.");
	ctx.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0x0010);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void wait_sync (NegativeTestContext& ctx)
{
	GLsync sync = ctx.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if sync is not the name of a sync object.");
	ctx.glWaitSync(0, 0, GL_TIMEOUT_IGNORED);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if flags is not zero.");
	ctx.glWaitSync(sync, 0x0010, GL_TIMEOUT_IGNORED);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if timeout is not GL_TIMEOUT_IGNORED.");
	ctx.glWaitSync(sync, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteSync(sync);
}

void client_wait_sync (NegativeTestContext& ctx)
{
	GLsync sync = ctx.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if sync is not the name of an existing sync object.");
	ctx.glClientWaitSync (0, 0, 10000);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if flags contains any unsupported flag.");
	ctx.glClientWaitSync(sync, 0x00000004, 10000);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteSync(sync);
}

void delete_sync (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if sync is neither zero or the name of a sync object.");
	ctx.glDeleteSync((GLsync)1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glDeleteSync(0);
	ctx.expectError(GL_NO_ERROR);
	ctx.endSection();
}

std::vector<FunctionContainer> getNegativeFragmentApiTestFunctions ()
{
	FunctionContainer funcs[] =
	{
		{scissor,					"scissor",					"Invalid glScissor() usage"					},
		{depth_func,				"depth_func",				"Invalid glDepthFunc() usage"				},
		{viewport,					"viewport",					"Invalid glViewport() usage"				},
		{stencil_func,				"stencil_func",				"Invalid glStencilFunc() usage"				},
		{stencil_func_separate,		"stencil_func_separate",	"Invalid glStencilFuncSeparate() usage"		},
		{stencil_op,				"stencil_op",				"Invalid glStencilOp() usage"				},
		{stencil_op_separate,		"stencil_op_separate",		"Invalid glStencilOpSeparate() usage"		},
		{stencil_mask_separate,		"stencil_mask_separate",	"Invalid glStencilMaskSeparate() usage"		},
		{blend_equation,			"blend_equation",			"Invalid glBlendEquation() usage"			},
		{blend_equationi,			"blend_equationi",			"Invalid glBlendEquationi() usage"			},
		{blend_equation_separate,	"blend_equation_separate",	"Invalid glBlendEquationSeparate() usage"	},
		{blend_equation_separatei,	"blend_equation_separatei",	"Invalid glBlendEquationSeparatei() usage"	},
		{blend_func,				"blend_func",				"Invalid glBlendFunc() usage"				},
		{blend_funci,				"blend_funci",				"Invalid glBlendFunci() usage"				},
		{blend_func_separate,		"blend_func_separate",		"Invalid glBlendFuncSeparate() usage"		},
		{blend_func_separatei,		"blend_func_separatei",		"Invalid glBlendFuncSeparatei() usage"		},
		{cull_face,					"cull_face",				"Invalid glCullFace() usage"				},
		{front_face,				"front_face",				"Invalid glFrontFace() usage"				},
		{line_width,				"line_width",				"Invalid glLineWidth() usage"				},
		{gen_queries,				"gen_queries",				"Invalid glGenQueries() usage"				},
		{begin_query,				"begin_query",				"Invalid glBeginQuery() usage"				},
		{end_query,					"end_query",				"Invalid glEndQuery() usage"				},
		{delete_queries,			"delete_queries",			"Invalid glDeleteQueries() usage"			},
		{fence_sync,				"fence_sync",				"Invalid glFenceSync() usage"				},
		{wait_sync,					"wait_sync",				"Invalid glWaitSync() usage"				},
		{client_wait_sync,			"client_wait_sync",			"Invalid glClientWaitSync() usage"			},
		{delete_sync,				"delete_sync",				"Invalid glDeleteSync() usage"				},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
