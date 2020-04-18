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

#include "esextcGeometryShaderRendering.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace glcts
{

/** Constructor of the geometry shader test base class.
 *
 *  @param context     Rendering context
 *  @param name        Name of the test
 *  @param description Description of the test
 **/
GeometryShaderRendering::GeometryShaderRendering(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description)
	: TestCaseGroupBase(context, extParams, name, description)
{
	/* Left blank intentionally */
}

/** Retrieves test name for specific <input layout qualifier,
 *  output layout qualifier, draw call mode> combination.
 *
 *  @param input       Input layout qualifier.
 *  @param output_type Output layout qualifier.
 *  @param drawcall_mode Draw call mode.
 *
 *  NOTE: This function throws TestError exception if the requested combination
 *        is considered invalid.
 *
 *  @return Requested string.
 **/
const char* GeometryShaderRendering::getTestName(_shader_input input, _shader_output_type output_type,
												 glw::GLenum drawcall_mode)
{
	const char* result = NULL;

	switch (input)
	{
	case SHADER_INPUT_POINTS:
	{
		switch (output_type)
		{
		case SHADER_OUTPUT_TYPE_LINE_STRIP:
			result = "points_input_line_strip_output";
			break;
		case SHADER_OUTPUT_TYPE_POINTS:
			result = "points_input_points_output";
			break;
		case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
			result = "points_input_triangles_output";
			break;

		default:
		{
			TCU_FAIL("Unrecognized shader output type requested");
		}
		} /* switch (output_type) */

		break;
	}

	case SHADER_INPUT_LINES:
	{
		switch (output_type)
		{
		case SHADER_OUTPUT_TYPE_LINE_STRIP:
		{
			switch (drawcall_mode)
			{
			case GL_LINES:
				result = "lines_input_line_strip_output_lines_drawcall";
				break;
			case GL_LINE_STRIP:
				result = "lines_input_line_strip_output_line_strip_drawcall";
				break;
			case GL_LINE_LOOP:
				result = "lines_input_line_strip_output_line_loop_drawcall";
				break;

			default:
			{
				TCU_FAIL("UNrecognized draw call mode");
			}
			} /* switch (drawcall_mode) */

			break;
		} /* case SHADER_OUTPUT_TYPE_LINE_STRIP: */

		case SHADER_OUTPUT_TYPE_POINTS:
		{
			switch (drawcall_mode)
			{
			case GL_LINES:
				result = "lines_input_points_output_lines_drawcall";
				break;
			case GL_LINE_STRIP:
				result = "lines_input_points_output_line_strip_drawcall";
				break;
			case GL_LINE_LOOP:
				result = "lines_input_points_output_line_loop_drawcall";
				break;

			default:
			{
				TCU_FAIL("UNrecognized draw call mode");
			}
			} /* switch (drawcall_mode) */

			break;
		} /* case SHADER_OUTPUT_TYPE_POINTS: */

		case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
		{
			switch (drawcall_mode)
			{
			case GL_LINES:
				result = "lines_input_triangle_strip_output_lines_drawcall";
				break;
			case GL_LINE_STRIP:
				result = "lines_input_triangle_strip_output_line_strip_drawcall";
				break;
			case GL_LINE_LOOP:
				result = "lines_input_triangle_strip_output_line_loop_drawcall";
				break;

			default:
			{
				TCU_FAIL("UNrecognized draw call mode");
			}
			} /* switch (drawcall_mode) */

			break;
		} /* case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP: */

		default:
		{
			TCU_FAIL("Unrecognized shader output type requested");
		}
		} /* switch (output_type) */

		break;
	} /* case SHADER_INPUT_LINES:*/

	case SHADER_INPUT_LINES_WITH_ADJACENCY:
	{
		switch (output_type)
		{
		case SHADER_OUTPUT_TYPE_LINE_STRIP:
		{
			if ((drawcall_mode == GL_LINES_ADJACENCY_EXT) || (drawcall_mode == GL_LINES_ADJACENCY))
			{
				result = "lines_with_adjacency_input_line_strip_output_lines_adjacency_drawcall";
			}
			else if ((drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT) || (drawcall_mode == GL_LINE_STRIP_ADJACENCY))
			{
				result = "lines_with_adjacency_input_line_strip_output_line_strip_drawcall";
			}
			else
			{
				TCU_FAIL("Unrecognized draw call mode");
			}

			break;
		} /* case SHADER_OUTPUT_TYPE_LINE_STRIP: */

		case SHADER_OUTPUT_TYPE_POINTS:
		{
			if ((drawcall_mode == GL_LINES_ADJACENCY_EXT) || (drawcall_mode == GL_LINES_ADJACENCY))
			{
				result = "lines_with_adjacency_input_points_output_lines_adjacency_drawcall";
			}
			else if ((drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT) || (drawcall_mode == GL_LINE_STRIP_ADJACENCY))
			{
				result = "lines_with_adjacency_input_points_output_line_strip_drawcall";
			}
			else
			{
				TCU_FAIL("Unrecognized draw call mode");
			}

			break;
		} /* case SHADER_OUTPUT_TYPE_POINTS: */

		case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
		{
			if ((drawcall_mode == GL_LINES_ADJACENCY_EXT) || (drawcall_mode == GL_LINES_ADJACENCY))
			{
				result = "lines_with_adjacency_input_triangle_strip_output_lines_adjacency_drawcall";
			}
			else if ((drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT) || (drawcall_mode == GL_LINE_STRIP_ADJACENCY))
			{
				result = "lines_with_adjacency_input_triangle_strip_output_line_strip_drawcall";
			}
			else
			{
				TCU_FAIL("Unrecognized draw call mode");
			}

			break;
		} /* case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP: */

		default:
		{
			TCU_FAIL("Unrecognized shader output type requested");
		}
		} /* switch (output_type) */

		break;
	} /* case SHADER_INPUT_LINES_WITH_ADJACENCY: */

	case SHADER_INPUT_TRIANGLES:
	{
		switch (output_type)
		{
		case SHADER_OUTPUT_TYPE_LINE_STRIP:
		{
			switch (drawcall_mode)
			{
			case GL_TRIANGLES:
				result = "triangles_input_line_strip_output_triangles_drawcall";
				break;
			case GL_TRIANGLE_FAN:
				result = "triangles_input_line_strip_output_triangle_fan_drawcall";
				break;
			case GL_TRIANGLE_STRIP:
				result = "triangles_input_line_strip_output_triangle_strip_drawcall";
				break;

			default:
			{
				TCU_FAIL("Unrecognized draw call mode");
			}
			} /* switch (drawcall_mode) */

			break;
		} /* case SHADER_OUTPUT_TYPE_LINE_STRIP: */

		case SHADER_OUTPUT_TYPE_POINTS:
		{
			switch (drawcall_mode)
			{
			case GL_TRIANGLES:
				result = "triangles_input_points_output_triangles_drawcall";
				break;
			case GL_TRIANGLE_FAN:
				result = "triangles_input_points_output_triangle_fan_drawcall";
				break;
			case GL_TRIANGLE_STRIP:
				result = "triangles_input_points_output_triangle_strip_drawcall";
				break;

			default:
			{
				TCU_FAIL("Unrecognized draw call mode");
			}
			} /* switch (drawcall_mode) */

			break;
		} /* case SHADER_OUTPUT_TYPE_POINTS: */

		case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
		{
			switch (drawcall_mode)
			{
			case GL_TRIANGLES:
				result = "triangles_input_triangle_strip_output_triangles_drawcall";
				break;
			case GL_TRIANGLE_FAN:
				result = "triangles_input_triangle_strip_output_triangle_fan_drawcall";
				break;
			case GL_TRIANGLE_STRIP:
				result = "triangles_input_triangle_strip_output_triangle_strip_drawcall";
				break;

			default:
			{
				TCU_FAIL("Unrecognized draw call mode");
			}
			} /* switch (drawcall_mode) */

			break;
		} /* case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP: */

		default:
		{
			TCU_FAIL("Unrecognized shader output type requested");
		}
		} /* switch (output_type) */

		break;
	} /* case SHADER_INPUT_TRIANGLES: */

	case SHADER_INPUT_TRIANGLES_WITH_ADJACENCY:
	{
		switch (output_type)
		{
		case SHADER_OUTPUT_TYPE_LINE_STRIP:
		{
			if ((drawcall_mode == GL_TRIANGLES_ADJACENCY_EXT) || (drawcall_mode == GL_TRIANGLES_ADJACENCY))
			{
				result = "triangles_with_adjacency_input_line_strip_output_triangles_adjacency_drawcall";
			}
			else if ((drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT) ||
					 (drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY))
			{
				result = "triangles_with_adjacency_input_line_strip_output_triangle_strip_adjacency_drawcall";
			}
			else
			{
				TCU_FAIL("Unrecognized draw call mode");
			}

			break;
		} /* case SHADER_OUTPUT_TYPE_LINE_STRIP: */

		case SHADER_OUTPUT_TYPE_POINTS:
		{
			if ((drawcall_mode == GL_TRIANGLES_ADJACENCY_EXT) || (drawcall_mode == GL_TRIANGLES_ADJACENCY))
			{
				result = "triangles_with_adjacency_input_points_output_triangles_adjacency_drawcall";
			}
			else if ((drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT) ||
					 (drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY))
			{
				result = "triangles_with_adjacency_input_points_output_triangle_strip_adjacency_drawcall";
				break;
			}
			else
			{
				TCU_FAIL("Unrecognized draw call mode");
			}

			break;
		} /* case SHADER_OUTPUT_TYPE_POINTS:*/

		case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
		{
			if ((drawcall_mode == GL_TRIANGLES_ADJACENCY_EXT) || (drawcall_mode == GL_TRIANGLES_ADJACENCY))
			{
				result = "triangles_with_adjacency_input_triangle_strip_output_triangles_adjacency_drawcall";
			}
			else if ((drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT) ||
					 (drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY))
			{
				result = "triangles_with_adjacency_input_triangle_strip_output_triangle_strip_adjacency_drawcall";
			}
			else
			{
				TCU_FAIL("Unrecognized draw call mode");
			}

			break;
		} /* case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP: */

		default:
		{
			TCU_FAIL("Unrecognized shader output type requested");
		}
		} /* switch (output_type) */

		break;
	} /* case SHADER_INPUT_TRIANGLES_WITH_ADJACENCY: */

	default:
	{
		/* Unrecognized geometry shader input layout qualifier */
		TCU_FAIL("Unrecognized layout qualifier");
	}
	} /* switch (input) */

	return result;
}

/* Initializes child instances of the group that will execute actual tests. */
void GeometryShaderRendering::init(void)
{
	/* Initialize base class */
	TestCaseGroupBase::init();

	/* General variables */
	glw::GLenum   drawcall_mode = GL_NONE;
	_shader_input input			= SHADER_INPUT_UNKNOWN;

	const glw::GLenum iterations[] = { /* geometry shader input layout qualifier */ /* draw call mode. */
									   SHADER_INPUT_POINTS, GL_POINTS, SHADER_INPUT_LINES, GL_LINES, SHADER_INPUT_LINES,
									   GL_LINE_STRIP, SHADER_INPUT_LINES, GL_LINE_LOOP,
									   SHADER_INPUT_LINES_WITH_ADJACENCY, GL_LINES_ADJACENCY_EXT,
									   SHADER_INPUT_LINES_WITH_ADJACENCY, GL_LINE_STRIP_ADJACENCY_EXT,
									   SHADER_INPUT_TRIANGLES, GL_TRIANGLES, SHADER_INPUT_TRIANGLES, GL_TRIANGLE_FAN,
									   SHADER_INPUT_TRIANGLES, GL_TRIANGLE_STRIP, SHADER_INPUT_TRIANGLES_WITH_ADJACENCY,
									   GL_TRIANGLES_ADJACENCY_EXT, SHADER_INPUT_TRIANGLES_WITH_ADJACENCY,
									   GL_TRIANGLE_STRIP_ADJACENCY_EXT };
	const unsigned int n_iterations = sizeof(iterations) / sizeof(iterations[0]) / 2;
	unsigned int	   n_output		= 0;

	for (unsigned int n_iteration = 0; n_iteration < n_iterations; ++n_iteration)
	{
		/* Retrieve iteration-specific input layout qualifier and draw call mode data */
		input		  = (_shader_input)iterations[n_iteration * 2 + 0];
		drawcall_mode = iterations[n_iteration * 2 + 1];

		/* Instantiate a worker. Each worker needs to be initialized & executed separately for each
		 * of the three supported input layout qualifiers.*/
		for (n_output = 0; n_output < SHADER_OUTPUT_TYPE_COUNT; ++n_output)
		{
			_shader_output_type output_type = (_shader_output_type)n_output;
			const char*			name		= getTestName(input, output_type, drawcall_mode);

			switch (input)
			{
			case SHADER_INPUT_POINTS:
			{
				addChild(
					new GeometryShaderRenderingPointsCase(m_context, m_extParams, name, drawcall_mode, output_type));

				break;
			}

			case SHADER_INPUT_LINES:
			{
				addChild(new GeometryShaderRenderingLinesCase(m_context, m_extParams, name, false, drawcall_mode,
															  output_type));

				break;
			} /* case SHADER_INPUT_LINES:*/

			case SHADER_INPUT_LINES_WITH_ADJACENCY:
			{
				addChild(new GeometryShaderRenderingLinesCase(m_context, m_extParams, name, true, drawcall_mode,
															  output_type));

				break;
			} /* case SHADER_INPUT_LINES_WITH_ADJACENCY: */

			case SHADER_INPUT_TRIANGLES:
			{
				addChild(new GeometryShaderRenderingTrianglesCase(m_context, m_extParams, name, false, drawcall_mode,
																  output_type));

				break;
			}

			case SHADER_INPUT_TRIANGLES_WITH_ADJACENCY:
			{
				addChild(new GeometryShaderRenderingTrianglesCase(m_context, m_extParams, name, true, drawcall_mode,
																  output_type));

				break;
			}

			default:
			{
				/* Unrecognized geometry shader input layout qualifier */
				TCU_FAIL("Unrecognized layout qualifier");
			}
			} /* switch (input) */
		}	 /* for (all output layout qualifiers) */
	}		  /* for (all iterations) */
}

/** Base worker implementation */
/** Constructor.
 *
 *  @param context     Rendering context.
 *  @param testContext Test context.
 *  @param name        Name of the test.
 *  @param description Description of what the test does.
 **/
GeometryShaderRenderingCase::GeometryShaderRenderingCase(Context& context, const ExtParameters& extParams,
														 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_context(context)
	, m_instanced_raw_arrays_bo_id(0)
	, m_instanced_unordered_arrays_bo_id(0)
	, m_instanced_unordered_elements_bo_id(0)
	, m_noninstanced_raw_arrays_bo_id(0)
	, m_noninstanced_unordered_arrays_bo_id(0)
	, m_noninstanced_unordered_elements_bo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_renderingTargetSize_uniform_location(0)
	, m_singleRenderingTargetSize_uniform_location(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_fbo_id(0)
	, m_read_fbo_id(0)
	, m_to_id(0)
	, m_instanced_fbo_id(0)
	, m_instanced_read_fbo_id(0)
	, m_instanced_to_id(0)
{
	/* Left blank intentionally */
}

/** Releases all GLES objects initialized by the base class for geometry shader rendering case implementations. */
void GeometryShaderRenderingCase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_instanced_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_instanced_fbo_id);

		m_instanced_fbo_id = 0;
	}

	if (m_instanced_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_instanced_read_fbo_id);

		m_instanced_read_fbo_id = 0;
	}

	if (m_instanced_to_id != 0)
	{
		gl.deleteTextures(1, &m_instanced_to_id);

		m_instanced_to_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_instanced_raw_arrays_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_instanced_raw_arrays_bo_id);

		m_instanced_raw_arrays_bo_id = 0;
	}

	if (m_instanced_unordered_arrays_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_instanced_unordered_arrays_bo_id);

		m_instanced_unordered_arrays_bo_id = 0;
	}

	if (m_instanced_unordered_elements_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_instanced_unordered_elements_bo_id);

		m_instanced_unordered_elements_bo_id = 0;
	}

	if (m_noninstanced_raw_arrays_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_noninstanced_raw_arrays_bo_id);

		m_noninstanced_raw_arrays_bo_id = 0;
	}

	if (m_noninstanced_unordered_arrays_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_noninstanced_unordered_arrays_bo_id);

		m_noninstanced_unordered_arrays_bo_id = 0;
	}

	if (m_noninstanced_unordered_elements_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_noninstanced_unordered_elements_bo_id);

		m_noninstanced_unordered_elements_bo_id = 0;
	}

	if (m_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_read_fbo_id);

		m_read_fbo_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes actual geometry shader-based rendering test, using an user-specified draw call.
 *
 *  @param type Type of the draw call to use for the test.
 **/
void GeometryShaderRenderingCase::executeTest(GeometryShaderRenderingCase::_draw_call_type type)
{
	glw::GLuint			  draw_fbo_id				  = 0;
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	const glw::GLenum	 index_type				  = getUnorderedElementsDataType();
	const glw::GLubyte	max_elements_index		  = getUnorderedElementsMaxIndex();
	const glw::GLubyte	min_elements_index		  = getUnorderedElementsMinIndex();
	const glw::GLenum	 mode						  = getDrawCallMode();
	unsigned int		  n_elements				  = getAmountOfElementsPerInstance();
	unsigned int		  n_instance				  = 0;
	unsigned int		  n_instances				  = getAmountOfDrawInstances();
	const unsigned int	n_vertices				  = getAmountOfVerticesPerInstance();
	const glw::GLint	  position_attribute_location = gl.getAttribLocation(m_po_id, "position");
	glw::GLuint			  read_fbo_id				  = 0;
	unsigned int		  rt_height					  = 0;
	unsigned int		  rt_width					  = 0;
	unsigned int		  single_rt_height			  = 0;
	unsigned int		  single_rt_width			  = 0;

	/* Reduce n_instances to 1 for non-instanced draw call modes */
	if (type == DRAW_CALL_TYPE_GL_DRAW_ARRAYS || type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS ||
		type == DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS)
	{
		draw_fbo_id = m_fbo_id;
		n_instances = 1;
		read_fbo_id = m_read_fbo_id;
	}
	else
	{
		draw_fbo_id = m_instanced_fbo_id;
		read_fbo_id = m_instanced_read_fbo_id;
	}

	/* Retrieve render-target size */
	getRenderTargetSize(n_instances, &rt_width, &rt_height);
	getRenderTargetSize(1, &single_rt_width, &single_rt_height);

	/* Configure GL_ARRAY_BUFFER binding */
	switch (type)
	{
	case DRAW_CALL_TYPE_GL_DRAW_ARRAYS:
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, m_noninstanced_raw_arrays_bo_id);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED:
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, m_instanced_raw_arrays_bo_id);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED:
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, m_instanced_unordered_arrays_bo_id);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ELEMENTS:
	case DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS:
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, m_noninstanced_unordered_arrays_bo_id);

		break;
	}

	default:
	{
		/* Unrecognized draw call mode */
		TCU_FAIL("Unrecognized draw call mode");
	}
	} /* switch (type) */

	gl.vertexAttribPointer(position_attribute_location, 4, GL_FLOAT, GL_FALSE, 0 /* stride */, NULL);
	gl.enableVertexAttribArray(position_attribute_location);

	/* Configure GL_ELEMENT_ARRAY_BUFFER binding */
	switch (type)
	{
	case DRAW_CALL_TYPE_GL_DRAW_ARRAYS:
	case DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED:
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED:
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_instanced_unordered_elements_bo_id);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ELEMENTS:
	case DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS:
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_noninstanced_unordered_elements_bo_id);

		break;
	}

	default:
	{
		/* Unrecognized draw call mode */
		TCU_FAIL("Unrecognized draw call mode");
	}
	} /* switch (type) */

	/* Configure draw framebuffer */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo_id);
	gl.viewport(0 /* x */, 0 /* y */, rt_width, rt_height);

	/* Clear the color buffer. */
	gl.clearColor(0.0f /* red */, 0.0f /* green */, 0.0f /* blue */, 0.0f /* alpha */);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Test set-up failed.");

	/* Render the test geometry */
	gl.useProgram(m_po_id);
	gl.uniform2i(m_renderingTargetSize_uniform_location, rt_width, rt_height);
	gl.uniform2i(m_singleRenderingTargetSize_uniform_location, single_rt_width, single_rt_height);

	setUniformsBeforeDrawCall(type);

	switch (type)
	{
	case DRAW_CALL_TYPE_GL_DRAW_ARRAYS:
	{
		gl.drawArrays(mode, 0 /* first */, n_vertices);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED:
	{
		gl.drawArraysInstanced(mode, 0 /* first */, n_vertices, n_instances);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ELEMENTS:
	{
		gl.drawElements(mode, n_elements, index_type, NULL);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED:
	{
		gl.drawElementsInstanced(mode, n_elements, index_type, NULL, n_instances);

		break;
	}

	case DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS:
	{
		gl.drawRangeElements(mode, min_elements_index, max_elements_index, n_elements, index_type, NULL);

		break;
	}

	default:
	{
		/* Unrecognized draw call mode */
		TCU_FAIL("Unrecognized draw call mode");
	}
	} /* switch (type) */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Draw call failed.");

	/* Retrieve rendered data */
	unsigned char* rendered_data = new unsigned char[rt_height * rt_width * 4 /* components */];

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo_id);
	gl.readPixels(0 /* x */, 0 /* y */, rt_width, rt_height, GL_RGBA, GL_UNSIGNED_BYTE, rendered_data);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

	/* Verify if the test succeeded */
	for (n_instance = 0; n_instance < n_instances; ++n_instance)
	{
		verify(type, n_instance, rendered_data);
	} /* for (all instances) */

	/* Release the data buffer */
	delete[] rendered_data;
}

/** Initializes ES objects required to execute a set of tests for particular input layout qualifier. */
void GeometryShaderRenderingCase::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Retrieve derivative class' properties */
	std::string  fs_code	 = getFragmentShaderCode();
	std::string  gs_code	 = getGeometryShaderCode();
	unsigned int n			 = 0;
	unsigned int n_instances = getAmountOfDrawInstances();
	std::string  vs_code	 = getVertexShaderCode();

	/* Set pixel storage properties before we continue */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* Create buffer objects the derivative implementation will fill with data */
	gl.genBuffers(1, &m_instanced_raw_arrays_bo_id);
	gl.genBuffers(1, &m_instanced_unordered_arrays_bo_id);
	gl.genBuffers(1, &m_instanced_unordered_elements_bo_id);
	gl.genBuffers(1, &m_noninstanced_raw_arrays_bo_id);
	gl.genBuffers(1, &m_noninstanced_unordered_arrays_bo_id);
	gl.genBuffers(1, &m_noninstanced_unordered_elements_bo_id);

	/* Set up a separate set of objects, that will be used for testing instanced and non-instanced draw calls */
	for (n = 0; n < 2 /* non-/instanced draw calls */; ++n)
	{
		bool is_instanced = (n == 1);

		glw::GLuint* draw_fbo_id_ptr = !is_instanced ? &m_fbo_id : &m_instanced_fbo_id;
		glw::GLuint* read_fbo_id_ptr = (!is_instanced) ? &m_read_fbo_id : &m_instanced_read_fbo_id;
		unsigned int to_height		 = 0;
		unsigned int to_width		 = 0;
		glw::GLuint* to_id_ptr		 = (!is_instanced) ? &m_to_id : &m_instanced_to_id;

		/* n == 0: non-instanced draw calls, instanced otherwise */
		getRenderTargetSize((n == 0) ? 1 : n_instances, &to_width, &to_height);

		/* Create a texture object we will be rendering to */
		gl.genTextures(1, to_id_ptr);
		gl.bindTexture(GL_TEXTURE_2D, *to_id_ptr);
		gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, to_width, to_height);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure a 2D texture object");

		/* Create and configure a framebuffer object that will be used for rendering */
		gl.genFramebuffers(1, draw_fbo_id_ptr);
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, *draw_fbo_id_ptr);

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *to_id_ptr, 0 /* level */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure draw framebuffer object");

		if (gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			TCU_FAIL("Draw framebuffer is reported to be incomplete");
		} /* if (gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) */

		/* Create and bind a FBO we will use for reading rendered data */
		gl.genFramebuffers(1, read_fbo_id_ptr);
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, *read_fbo_id_ptr);

		gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *to_id_ptr, 0 /* level */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure read framebuffer object");

		if (gl.checkFramebufferStatus(GL_READ_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			TCU_FAIL("Read framebuffer is reported to be incomplete");
		} /* if (gl.checkFramebufferStatus(GL_READ_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) */

		/* Fill the iteration-specific buffer objects with data */
		glw::GLuint  raw_arrays_bo_id = (is_instanced) ? m_instanced_raw_arrays_bo_id : m_noninstanced_raw_arrays_bo_id;
		unsigned int raw_arrays_bo_size = getRawArraysDataBufferSize(is_instanced);
		const void*  raw_arrays_data	= getRawArraysDataBuffer(is_instanced);
		glw::GLuint  unordered_arrays_bo_id =
			(is_instanced) ? m_instanced_unordered_arrays_bo_id : m_noninstanced_unordered_arrays_bo_id;
		unsigned int unordered_arrays_bo_size = getUnorderedArraysDataBufferSize(is_instanced);
		const void*  unordered_arrays_data	= getUnorderedArraysDataBuffer(is_instanced);
		glw::GLuint  unordered_elements_bo_id =
			(is_instanced) ? m_instanced_unordered_elements_bo_id : m_noninstanced_unordered_elements_bo_id;
		unsigned int unordered_elements_bo_size = getUnorderedElementsDataBufferSize(is_instanced);
		const void*  unordered_elements_data	= getUnorderedElementsDataBuffer(is_instanced);

		gl.bindBuffer(GL_ARRAY_BUFFER, raw_arrays_bo_id);
		gl.bufferData(GL_ARRAY_BUFFER, raw_arrays_bo_size, raw_arrays_data, GL_STATIC_COPY);

		gl.bindBuffer(GL_ARRAY_BUFFER, unordered_arrays_bo_id);
		gl.bufferData(GL_ARRAY_BUFFER, unordered_arrays_bo_size, unordered_arrays_data, GL_STATIC_COPY);

		gl.bindBuffer(GL_ARRAY_BUFFER, unordered_elements_bo_id);
		gl.bufferData(GL_ARRAY_BUFFER, unordered_elements_bo_size, unordered_elements_data, GL_STATIC_COPY);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure test buffer objects");

	/* Create and bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate & bind a vertex array object");

	/* Create and configure program & shader objects that will be used for the test case */
	const char* fs_code_ptr = fs_code.c_str();
	const char* gs_code_ptr = gs_code.c_str();
	const char* vs_code_ptr = vs_code.c_str();

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_po_id = gl.createProgram();
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader/program objects");

	if (!buildProgram(m_po_id, m_fs_id, 1, &fs_code_ptr, m_gs_id, 1, &gs_code_ptr, m_vs_id, 1, &vs_code_ptr))
	{
		TCU_FAIL("Could not build shader program");
	}

	/* Retrieve uniform locations */
	m_renderingTargetSize_uniform_location		 = gl.getUniformLocation(m_po_id, "renderingTargetSize");
	m_singleRenderingTargetSize_uniform_location = gl.getUniformLocation(m_po_id, "singleRenderingTargetSize");
}

/* Checks all draw call types for a specific Geometry Shader input layout qualifier.
 *
 * @return Always STOP.
 */
tcu::TestNode::IterateResult GeometryShaderRenderingCase::iterate()
{
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	initTest();

	executeTest(GeometryShaderRenderingCase::DRAW_CALL_TYPE_GL_DRAW_ARRAYS);
	executeTest(GeometryShaderRenderingCase::DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED);
	executeTest(GeometryShaderRenderingCase::DRAW_CALL_TYPE_GL_DRAW_ELEMENTS);
	executeTest(GeometryShaderRenderingCase::DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED);
	executeTest(GeometryShaderRenderingCase::DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param drawcall_mode Draw call mode that should be tested with this test case instance.
 *  @param output_type   Geometry shader output type to be used.
 *  @param context       Rendering context;
 *  @param testContext   Test context;
 *  @param name          Test name.
 **/
GeometryShaderRenderingPointsCase::GeometryShaderRenderingPointsCase(Context& context, const ExtParameters& extParams,
																	 const char* name, glw::GLenum drawcall_mode,
																	 _shader_output_type output_type)
	: GeometryShaderRenderingCase(
		  context, extParams, name,
		  "Verifies all draw calls work correctly for specific input+output+draw call mode combination")
	, m_output_type(output_type)
{
	/* Sanity checks */
	if (drawcall_mode != GL_POINTS)
	{
		TCU_FAIL("Only GL_POINTS draw call mode is supported for 'points' geometry shader input layout qualifier test "
				 "implementation");
	}

	if (output_type != SHADER_OUTPUT_TYPE_POINTS && output_type != SHADER_OUTPUT_TYPE_LINE_STRIP &&
		output_type != SHADER_OUTPUT_TYPE_TRIANGLE_STRIP)
	{
		TCU_FAIL("Unsupported output layout qualifier type requested for 'points' geometry shader input layout "
				 "qualifier test implementation");
	}

	/* Retrieve rendertarget resolution for a single-instance case.
	 *
	 * Y coordinates will be dynamically updated in a vertex shader for
	 * multiple-instance tests. That's fine, since in multi-instanced tests
	 * we only expand the rendertarget's height; its width is unaffected.
	 */
	unsigned int rendertarget_height = 0;
	unsigned int rendertarget_width  = 0;

	getRenderTargetSize(1, &rendertarget_width, &rendertarget_height);

	/* Generate raw vertex array data. Note we do not differentiate between instanced and
	 * non-instanced cases for geometry shaders that emit points */
	const unsigned int raw_array_data_size = getRawArraysDataBufferSize(false);

	m_raw_array_data = new float[raw_array_data_size / sizeof(float)];

	for (unsigned int n_point = 0; n_point < 8 /* points, as per test spec */; ++n_point)
	{
		switch (m_output_type)
		{
		case SHADER_OUTPUT_TYPE_LINE_STRIP:
		{
			m_raw_array_data[n_point * 4 + 0] =
				-1 + ((float(3 + 7 * n_point) + 0.5f) / float(rendertarget_width)) * 2.0f;
			m_raw_array_data[n_point * 4 + 1] = -1 + ((float(3.0f + 0.5f)) / float(rendertarget_height)) * 2.0f;
			m_raw_array_data[n_point * 4 + 2] = 0.0f;
			m_raw_array_data[n_point * 4 + 3] = 1.0f;

			break;
		}

		case SHADER_OUTPUT_TYPE_POINTS:
		{
			m_raw_array_data[n_point * 4 + 0] =
				-1 + ((float(1 + 5 * n_point) + 0.5f) / float(rendertarget_width)) * 2.0f;
			m_raw_array_data[n_point * 4 + 1] = -1 + (float(1.5f + 0.5f) / float(rendertarget_height)) * 2.0f;
			m_raw_array_data[n_point * 4 + 2] = 0.0f;
			m_raw_array_data[n_point * 4 + 3] = 1.0f;

			break;
		}

		case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
		{
			m_raw_array_data[n_point * 4 + 0] = -1 + ((float(6 * n_point) + 0.5f) / float(rendertarget_width)) * 2.0f;
			m_raw_array_data[n_point * 4 + 1] = -1.0f;
			m_raw_array_data[n_point * 4 + 2] = 0.0f;
			m_raw_array_data[n_point * 4 + 3] = 1.0f;

			break;
		}

		default:
		{
			TCU_FAIL("Unsupported shader output layout qualifier requested");
		}
		} /* switch (m_output_type) */
	}	 /* for (all points) */

	/* Generate unordered data - we do not differentiate between instanced & non-instanced cases
	 * for "points" tests
	 */
	const unsigned int unordered_array_data_size	= getUnorderedArraysDataBufferSize(false);
	const unsigned int unordered_elements_data_size = getUnorderedElementsDataBufferSize(false);

	m_unordered_array_data	= new float[unordered_array_data_size / sizeof(float)];
	m_unordered_elements_data = new unsigned char[unordered_elements_data_size];

	for (unsigned int n_point = 0; n_point < 8 /* points, as per test spec */; ++n_point)
	{
		memcpy(m_unordered_array_data + n_point * 4, m_raw_array_data + (8 - n_point - 1) * 4,
			   sizeof(float) * 4 /* components */);
	} /* for (all points) */

	for (unsigned int n_point = 0; n_point < 8 /* points, as per test spec */; ++n_point)
	{
		m_unordered_elements_data[n_point] = (unsigned char)((n_point * 2) % 8 + n_point / 4);
	} /* for (all points) */
}

/** Destructor. */
GeometryShaderRenderingPointsCase::~GeometryShaderRenderingPointsCase()
{
	if (m_raw_array_data != NULL)
	{
		delete[] m_raw_array_data;

		m_raw_array_data = NULL;
	}

	if (m_unordered_array_data != NULL)
	{
		delete[] m_unordered_array_data;

		m_unordered_array_data = NULL;
	}

	if (m_unordered_elements_data != NULL)
	{
		delete[] m_unordered_elements_data;

		m_unordered_elements_data = NULL;
	}
}

/** Retrieves amount of instances that should be drawn with glDraw*Elements() calls.
 *
 *  @return As per description.
 **/
unsigned int GeometryShaderRenderingPointsCase::getAmountOfDrawInstances()
{
	return 4 /* instances */;
}

/** Retrieves amount of indices that should be used for rendering a single instance
 *  (glDraw*Elements() calls only)
 *
 *  @return As per description.
 */
unsigned int GeometryShaderRenderingPointsCase::getAmountOfElementsPerInstance()
{
	return 8 /* elements */;
}

/** Retrieves amount of vertices that should be used for rendering a single instance
 *  (glDrawArrays*() calls only)
 *
 *  @return As per description.
 **/
unsigned int GeometryShaderRenderingPointsCase::getAmountOfVerticesPerInstance()
{
	return 8 /* points */;
}

/** Draw call mode that should be used glDraw*() calls.
 *
 *  @return As per description.
 **/
glw::GLenum GeometryShaderRenderingPointsCase::getDrawCallMode()
{
	return GL_POINTS;
}

/** Source code for a fragment shader that should be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingPointsCase::getFragmentShaderCode()
{
	static std::string fs_code = "${VERSION}\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "in  vec4 gs_fs_color;\n"
								 "out vec4 result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    result = gs_fs_color;\n"
								 "}\n";

	return fs_code;
}

/** Source code for a geometry shader that should be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingPointsCase::getGeometryShaderCode()
{
	static const char* lines_gs_code = "${VERSION}\n"
									   "\n"
									   "${GEOMETRY_SHADER_ENABLE}\n"
									   "\n"
									   "layout(points)                      in;\n"
									   "layout(line_strip, max_vertices=15) out;\n"
									   "\n"
									   "in  vec4 vs_gs_color[1];\n"
									   "out vec4 gs_fs_color;\n"
									   "\n"
									   "uniform ivec2 renderingTargetSize;\n"
									   "\n"
									   "void main()\n"
									   "{\n"
									   "    float bias = 0.0035;"
									   "    for (int delta = 1; delta <= 3; ++delta)\n"
									   "    {\n"
									   "        float dx = float(delta * 2) / float(renderingTargetSize.x) + bias;\n"
									   "        float dy = float(delta * 2) / float(renderingTargetSize.y) + bias;\n"
									   "\n"
									   /* TL->TR */
									   "        gs_fs_color = vs_gs_color[0];\n"
									   "        gl_Position = gl_in[0].gl_Position + vec4(-dx, -dy, 0, 0);\n"
									   "        EmitVertex();\n"
									   "        gs_fs_color = vs_gs_color[0];\n"
									   "        gl_Position = gl_in[0].gl_Position + vec4(dx, -dy, 0, 0);\n"
									   "        EmitVertex();\n"
									   /* TR->BR */
									   "        gs_fs_color = vs_gs_color[0];\n"
									   "        gl_Position = gl_in[0].gl_Position + vec4(dx, dy, 0, 0);\n"
									   "        EmitVertex();\n"
									   /* BR->BL */
									   "        gs_fs_color = vs_gs_color[0];\n"
									   "        gl_Position = gl_in[0].gl_Position + vec4(-dx, dy, 0, 0);\n"
									   "        EmitVertex();\n"
									   /* BL->TL */
									   "        gs_fs_color = vs_gs_color[0];\n"
									   "        gl_Position = gl_in[0].gl_Position + vec4(-dx, -dy, 0, 0);\n"
									   "        EmitVertex();\n"
									   "        EndPrimitive();\n"
									   "    }\n"
									   "}\n";

	static const char* points_gs_code = "${VERSION}\n"
										"\n"
										"${GEOMETRY_SHADER_ENABLE}\n"
										"\n"
										"layout(points)                 in;\n"
										"layout(points, max_vertices=9) out;\n"
										"\n"
										"in  vec4 vs_gs_color[1];\n"
										"out vec4 gs_fs_color;\n"
										"\n"
										"uniform ivec2 renderingTargetSize;\n"
										"\n"
										"void main()\n"
										"{\n"
										"    float dx = 2.0 / float(renderingTargetSize.x);\n"
										"    float dy = 2.0 / float(renderingTargetSize.y);\n"
										"\n"
										/* TL */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(-dx, -dy, 0, 0);\n"
										"    EmitVertex();\n"
										/* TM */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(0, -dy, 0, 0);\n"
										"    EmitVertex();\n"
										/* TR */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(dx, -dy, 0, 0);\n"
										"    EmitVertex();\n"
										/* L */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(-dx, 0, 0, 0); \n"
										"    EmitVertex();\n"
										/* M */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position;\n"
										"    EmitVertex();\n"
										/* R */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(dx, 0, 0, 0);\n"
										"    EmitVertex();\n"
										/* BL */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(-dx, dy, 0, 0);\n"
										"    EmitVertex();\n"
										/* BM */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(0, dy, 0, 0);\n"
										"    EmitVertex();\n"
										/* BR */
										"    gs_fs_color  = vs_gs_color[0];\n"
										"    gl_Position  = gl_in[0].gl_Position + vec4(dx, dy, 0, 0);\n"
										"    EmitVertex();\n"
										"}\n";

	static const char* triangles_gs_code = "${VERSION}\n"
										   "\n"
										   "${GEOMETRY_SHADER_ENABLE}\n"
										   "\n"
										   "layout(points)                          in;\n"
										   "layout(triangle_strip, max_vertices=6) out;\n"
										   "\n"
										   "in  vec4 vs_gs_color[1];\n"
										   "out vec4 gs_fs_color;\n"
										   "\n"
										   "uniform ivec2 renderingTargetSize;\n"
										   "\n"
										   "void main()\n"
										   "{\n"
										   /* Assume that point position corresponds to TL corner. */
										   "    float dx = float(2.0) / float(renderingTargetSize.x);\n"
										   "    float dy = float(2.0) / float(renderingTargetSize.y);\n"
										   "\n"
										   /* BL 1 */
										   "    gs_fs_color = vec4(vs_gs_color[0].x, 0, 0, 0);\n"
										   "    gl_Position = gl_in[0].gl_Position + vec4(0, 6.0 * dy, 0, 0);\n"
										   "    EmitVertex();\n"
										   /* TL 1 */
										   "    gs_fs_color = vec4(vs_gs_color[0].x, 0, 0, 0);\n"
										   "    gl_Position = gl_in[0].gl_Position;\n"
										   "    EmitVertex();\n"
										   /* BR 1 */
										   "    gs_fs_color = vec4(vs_gs_color[0].x, 0, 0, 0);\n"
										   "    gl_Position = gl_in[0].gl_Position + vec4(6.0 * dx, 6.0 * dy, 0, 0);\n"
										   "    EmitVertex();\n"
										   "    EndPrimitive();\n"
										   /* BR 2 */
										   "    gs_fs_color = vec4(0, vs_gs_color[0].y, 0, 0);\n"
										   "    gl_Position = gl_in[0].gl_Position + vec4(6.0 * dx, 6.0 * dy, 0, 0);\n"
										   "    EmitVertex();\n"
										   /* TL 2 */
										   "    gs_fs_color = vec4(0, vs_gs_color[0].y, 0, 0);\n"
										   "    gl_Position = gl_in[0].gl_Position;\n"
										   "    EmitVertex();\n"
										   /* TR 2 */
										   "    gs_fs_color = vec4(0, vs_gs_color[0].y, 0, 0);\n"
										   "    gl_Position = gl_in[0].gl_Position + vec4(6.0 * dx, 0, 0, 0);\n"
										   "    EmitVertex();\n"
										   "    EndPrimitive();\n"
										   "}\n";
	const char* result = NULL;

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		result = lines_gs_code;

		break;
	}

	case SHADER_OUTPUT_TYPE_POINTS:
	{
		result = points_gs_code;

		break;
	}

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		result = triangles_gs_code;

		break;
	}

	default:
	{
		TCU_FAIL("Requested shader output layout qualifier is unsupported");
	}
	} /* switch (m_output_type) */

	return result;
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  vertex data to be used for glDrawArrays*() calls.
 *
 *  @param instanced Ignored.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingPointsCase::getRawArraysDataBufferSize(bool /*instanced*/)
{

	return sizeof(float) * (1 /* point */ * 4 /* coordinates */) * 8 /* points in total, as per test spec */;
}

/** Returns vertex data for the test. Only to be used for glDrawArrays*() calls.
 *
 *  @param instanced Ignored.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingPointsCase::getRawArraysDataBuffer(bool /*instanced*/)
{
	return m_raw_array_data;
}

/** Retrieves resolution of off-screen rendering buffer that should be used for the test.
 *
 *  @param n_instances Amount of draw call instances this render target will be used for.
 *  @param out_width   Deref will be used to store rendertarget's width. Must not be NULL.
 *  @param out_height  Deref will be used to store rendertarget's height. Must not be NULL.
 **/
void GeometryShaderRenderingPointsCase::getRenderTargetSize(unsigned int n_instances, unsigned int* out_width,
															unsigned int* out_height)
{
	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		*out_width  = 56;			   /* as per test spec */
		*out_height = 7 * n_instances; /* as per test spec */

		break;
	}

	case SHADER_OUTPUT_TYPE_POINTS:
	{
		*out_width  = 38;									   /* as per test spec */
		*out_height = 3 * n_instances + 2 * (n_instances - 1); /* as per test spec */

		break;
	}

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		*out_width  = 48;			   /* as per test spec */
		*out_height = 6 * n_instances; /* as per test spec */

		break;
	}

	default:
	{
		TCU_FAIL("Unsupported shader output type");
	}
	} /* switch (m_output_type) */
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  vertex data to be used for glDrawElements*() calls.
 *
 *  @param instanced Ignored.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingPointsCase::getUnorderedArraysDataBufferSize(bool /*instanced*/)
{

	/* Note: No difference between non-instanced and instanced cases for this class */
	return sizeof(float) * (1 /* point */ * 4 /* coordinates */) * 8 /* points in total, as per test spec */;
}

/** Returns vertex data for the test. Only to be used for glDrawElements*() calls.
 *
 *  @param instanced Ignored.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingPointsCase::getUnorderedArraysDataBuffer(bool /*instanced*/)
{

	/* Note: No difference between non-instanced and instanced cases for this class */
	return m_unordered_array_data;
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  index data to be used for glDrawElements*() calls.
 *
 *  @param instanced Ignored.
 *
 *  @return As per description.
 */
glw::GLuint GeometryShaderRenderingPointsCase::getUnorderedElementsDataBufferSize(bool /*instanced*/)
{

	/* Note: No difference between non-instanced and instanced cases for this class */
	return 8 /* indices */ * sizeof(unsigned char);
}

/** Returns index data for the test. Only to be used for glDrawElements*() calls.
 *
 *  @param instanced Ignored.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingPointsCase::getUnorderedElementsDataBuffer(bool /*instanced*/)
{

	/* Note: No difference between non-instanced and instanced cases for this class */
	return m_unordered_elements_data;
}

/** Returns type of the index, to be used for glDrawElements*() calls.
 *
 *  @return As per description.
 **/
glw::GLenum GeometryShaderRenderingPointsCase::getUnorderedElementsDataType()
{
	return GL_UNSIGNED_BYTE;
}

/** Retrieves maximum index value. To be used for glDrawRangeElements() test only.
 *
 *  @return As per description.
 **/
glw::GLubyte GeometryShaderRenderingPointsCase::getUnorderedElementsMaxIndex()
{
	return 8 /* up to 8 points will be rendered */;
}

/** Retrieves minimum index value. To be used for glDrawRangeElements() test only.
 *
 *  @return As per description.
 **/
glw::GLubyte GeometryShaderRenderingPointsCase::getUnorderedElementsMinIndex()
{
	return 0;
}

/** Retrieves vertex shader code to be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingPointsCase::getVertexShaderCode()
{
	static std::string lines_vs_code =
		"${VERSION}\n"
		"\n"
		"in      vec4  position;\n"
		"uniform ivec2 renderingTargetSize;\n"
		"out     vec4  vs_gs_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		/* non-instanced draw call cases */
		"    if (renderingTargetSize.y == 7)\n"
		"    {\n"
		"        gl_Position = position;"
		"    }\n"
		"    else\n"
		"    {\n"
		/* instanced draw call cases: */
		"        gl_Position = vec4(position.x, \n"
		"                           -1.0 + ((float(3 + gl_InstanceID * 7)) / float(renderingTargetSize.y - 1)) * 2.0,\n"
		"                           position.zw);\n"
		"    }\n"
		"\n"
		"    switch(gl_VertexID)\n"
		"    {\n"
		"        case 0: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"        case 1: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"        case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"        case 3: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"        case 4: vs_gs_color = vec4(1, 1, 0, 0); break;\n"
		"        case 5: vs_gs_color = vec4(1, 0, 1, 0); break;\n"
		"        case 6: vs_gs_color = vec4(1, 0, 0, 1); break;\n"
		"        case 7: vs_gs_color = vec4(1, 1, 1, 0); break;\n"
		"        case 8: vs_gs_color = vec4(1, 1, 0, 1); break;\n"
		"    }\n"
		"}\n";

	static std::string points_vs_code =
		"${VERSION}\n"
		"\n"
		"in      vec4  position;\n"
		"uniform ivec2 renderingTargetSize;\n"
		"out     vec4  vs_gs_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_PointSize = 1.0;\n"
		"\n"
		/* non-instanced draw call cases */
		"    if (renderingTargetSize.y == 3)\n"
		"    {\n"
		"        gl_Position = position;\n"
		"    }\n"
		/* instanced draw call cases */
		"    else\n"
		"    {\n"
		"        gl_Position = vec4(position.x,\n"
		"                           -1.0 + (1.5 + float(5 * gl_InstanceID)) / float(renderingTargetSize.y) * 2.0,\n"
		"                           position.zw);\n"
		"    }\n"
		"\n"
		"    switch(gl_VertexID)\n"
		"    {\n"
		"        case 0: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"        case 1: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"        case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"        case 3: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"        case 4: vs_gs_color = vec4(1, 1, 0, 0); break;\n"
		"        case 5: vs_gs_color = vec4(1, 0, 1, 0); break;\n"
		"        case 6: vs_gs_color = vec4(1, 0, 0, 1); break;\n"
		"        case 7: vs_gs_color = vec4(1, 1, 1, 0); break;\n"
		"        case 8: vs_gs_color = vec4(1, 1, 0, 1); break;\n"
		"    }\n"
		"}\n";

	static std::string triangles_vs_code =
		"${VERSION}\n"
		"\n"
		"in      vec4  position;\n"
		"uniform ivec2 renderingTargetSize;\n"
		"out     vec4  vs_gs_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    if (renderingTargetSize.y == 6)\n"
		"    {\n"
		"        gl_Position = position;\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        gl_Position = vec4(position.x,\n"
		"                           position.y + float(gl_InstanceID) * 6.0 / float(renderingTargetSize.y) * 2.0,\n"
		"                           position.zw);\n"
		"    }\n"
		"\n"
		"    switch(gl_VertexID)\n"
		"    {\n"
		"        case 0: vs_gs_color = vec4(0.1, 0.8, 0, 0); break;\n"
		"        case 1: vs_gs_color = vec4(0.2, 0.7, 0, 0); break;\n"
		"        case 2: vs_gs_color = vec4(0.3, 0.6, 1, 0); break;\n"
		"        case 3: vs_gs_color = vec4(0.4, 0.5, 0, 1); break;\n"
		"        case 4: vs_gs_color = vec4(0.5, 0.4, 0, 0); break;\n"
		"        case 5: vs_gs_color = vec4(0.6, 0.3, 1, 0); break;\n"
		"        case 6: vs_gs_color = vec4(0.7, 0.2, 0, 1); break;\n"
		"        case 7: vs_gs_color = vec4(0.8, 0.1, 1, 0); break;\n"
		"    }\n"
		"}\n";
	std::string result;

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		result = lines_vs_code;

		break;
	}

	case SHADER_OUTPUT_TYPE_POINTS:
	{
		result = points_vs_code;

		break;
	}

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		result = triangles_vs_code;

		break;
	}

	default:
	{
		TCU_FAIL("Unsupported shader output type used");
	}
	} /* switch (m_output_type) */

	return result;
}

/** Verifies that the rendered data is correct.
 *
 *  @param drawcall_type Type of the draw call that was used to render the geometry.
 *  @param instance_id   Instance ID to be verified. Use 0 for non-instanced draw calls.
 *  @param data          Contents of the rendertarget after the test has finished rendering.
 **/
void GeometryShaderRenderingPointsCase::verify(_draw_call_type drawcall_type, unsigned int instance_id,
											   const unsigned char* data)
{
	/* The array is directly related to vertex shader contents for "points" and "line_strip"
	 * outputs */
	static const unsigned char ref_data[] = { 255, 0, 0, 0,   0,   255, 0,   0, 0,   0,   255, 0,
											  0,   0, 0, 255, 255, 255, 0,   0, 255, 0,   255, 0,
											  255, 0, 0, 255, 255, 255, 255, 0, 255, 255, 0,   255 };
	/* The array refers to colors as defined in vertex shader used for "triangle_strip"
	 * output.
	 */
	static const unsigned char ref_data_triangle_strip[] = {
		(unsigned char)(0.1f * 255), (unsigned char)(0.8f * 255), 0,   0,
		(unsigned char)(0.2f * 255), (unsigned char)(0.7f * 255), 0,   0,
		(unsigned char)(0.3f * 255), (unsigned char)(0.6f * 255), 255, 0,
		(unsigned char)(0.4f * 255), (unsigned char)(0.5f * 255), 0,   255,
		(unsigned char)(0.5f * 255), (unsigned char)(0.4f * 255), 0,   0,
		(unsigned char)(0.6f * 255), (unsigned char)(0.3f * 255), 255, 0,
		(unsigned char)(0.7f * 255), (unsigned char)(0.2f * 255), 0,   255,
		(unsigned char)(0.8f * 255), (unsigned char)(0.1f * 255), 255, 0
	};
	unsigned int rt_height = 0;
	unsigned int rt_width  = 0;

	/* Retrieve render-target size */
	if (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED ||
		drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED)
	{
		getRenderTargetSize(getAmountOfDrawInstances(), &rt_width, &rt_height);
	}
	else
	{
		getRenderTargetSize(1, &rt_width, &rt_height);
	}

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		for (unsigned int n_point = 0; n_point < 8 /* points */; ++n_point)
		{
			/* Each point takes a 7x7 block. The test should verify that the second "nested" block
			 * is of the valid color. Blocks can be built on top of each other if instanced draw
			 * calls are used. */
			const unsigned char* reference_data		   = NULL;
			const unsigned char* rendered_data		   = NULL;
			const int			 row_width			   = rt_width * 4 /* components */;
			const int			 sample_region_edge_px = 5 /* pixels */;

			for (int n_edge = 0; n_edge < 4 /* edges */; ++n_edge)
			{
				/* Edge 1: TL->TR
				 * Edge 2: TR->BR
				 * Edge 3: BR->BL
				 * Edge 4: BL->TL
				 */
				int dx		= 0;
				int dy		= 0;
				int x		= 0;
				int x_start = 0;
				int y		= 0;
				int y_start = 0;

				switch (n_edge)
				{
				/* NOTE: The numbers here are as per test spec */
				case 0:
					dx		= 1 /* px */;
					dy		= 0 /* px */;
					x_start = 1 /* px */;
					y_start = 1 /* px */;
					break;
				case 1:
					dx		= 0 /* px */;
					dy		= 1 /* px */;
					x_start = 5 /* px */;
					y_start = 1 /* px */;
					break;
				case 2:
					dx		= -1 /* px */;
					dy		= 0 /* px */;
					x_start = 5 /* px */;
					y_start = 5 /* px */;
					break;
				case 3:
					dx		= 0 /* px */;
					dy		= -1 /* px */;
					x_start = 1 /* px */;
					y_start = 5 /* px */;
					break;

				default:
				{
					TCU_FAIL("Invalid edge index");
				}
				} /* switch (n_edge) */

				/* What color should the pixels have? */
				if (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ARRAYS ||
					drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED)
				{
					reference_data = ref_data + n_point * 4 /* components */;
					x			   = x_start + n_point * 7 /* pixel block size */;
					y			   = y_start + instance_id * 7 /* pixel block size */;
				}
				else
				{
					int index = m_unordered_elements_data[n_point];

					reference_data = ref_data + (8 - 1 - index) * 4 /* components */;
					x			   = x_start + index * 7 /* pixel block size */;
					y			   = y_start + instance_id * 7 /* pixel block size */;
				}

				/* Verify edge pixels */
				for (int n_pixel = 0; n_pixel < sample_region_edge_px; ++n_pixel)
				{
					rendered_data = data + y * row_width + x * 4 /* components */;

					if (memcmp(rendered_data, reference_data, 4 /* components */) != 0)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "At            (" << (int)x << ", " << (int)y
										   << "), "
											  "rendered data ("
										   << (int)rendered_data[0] << ", " << (int)rendered_data[1] << ", "
										   << (int)rendered_data[2] << ", " << (int)rendered_data[3]
										   << ") "
											  "is different from reference data ("
										   << (int)reference_data[0] << ", " << (int)reference_data[1] << ", "
										   << (int)reference_data[2] << ", " << (int)reference_data[3] << ")"
										   << tcu::TestLog::EndMessage;

						TCU_FAIL("Data comparison failed");
					} /* if (data comparison failed) */

					/* Move to next pixel */
					x += dx;
					y += dy;
				} /* for (all pixels) */
			}	 /* for (all edges) */
		}		  /* for (all points) */

		break;
	} /* case SHADER_OUTPUT_TYPE_LINE_STRIP: */

	case SHADER_OUTPUT_TYPE_POINTS:
	{
		/* For each 3px high row, we want to sample the second row. Area of a single "pixel" (incl. delta)
		 * can take no more than 5px, with the actual sampled area located in the second pixel.
		 */
		const int sample_region_width_px = 5; /* pixels */

		for (int n = 0; n < 8 /* points */; ++n)
		{
			int					 x				= 0;
			int					 y				= 0;
			const unsigned char* reference_data = NULL;
			const unsigned char* rendered_data  = NULL;

			/* Different ordering is used for indexed draw calls */
			if (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS ||
				drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED ||
				drawcall_type == DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS)
			{
				/* For indexed calls, vertex data is laid out in a reversed ordered */
				int index = m_unordered_elements_data[n];

				x			   = index * sample_region_width_px + 1;
				y			   = instance_id * 5 + 1; /* middle row */
				reference_data = ref_data + ((8 - 1 - index) * 4 /* components */);
				rendered_data  = data + (y * rt_width + x) * 4 /* components */;
			} /* if (draw call is indiced) */
			else
			{
				x			   = n * sample_region_width_px + 1;
				y			   = instance_id * 5 + 1; /* middle row */
				reference_data = ref_data + (n * 4 /* components */);
				rendered_data  = data + (y * rt_width + x) * 4 /* components */;
			}

			if (memcmp(rendered_data, reference_data, 4 /* components */) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Reference data: [" << (int)reference_data[0] << ", "
								   << (int)reference_data[1] << ", " << (int)reference_data[2] << ", "
								   << (int)reference_data[3] << "] "
																"for pixel at ("
								   << x << ", " << y << ") "
														"does not match  ["
								   << (int)rendered_data[0] << ", " << (int)rendered_data[1] << ", "
								   << (int)rendered_data[2] << ", " << (int)rendered_data[3] << ").";

				TCU_FAIL("Data comparison failed.");
			} /* if (memcmp(rendered_data, reference_data, 4) != 0) */
		}	 /* for (all points) */

		break;
	} /* case SHADER_OUTPUT_TYPE_POINTS: */

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		/* A pair of adjacent triangles is contained within a 6x6 bounding box.
		 * The box is defined by top-left corner which is located at input point's
		 * location.
		 * The left triangle should only use red channel, the one on the right
		 * should only use green channel.
		 * We find centroid for each triangle, sample its color and make sure
		 * it's valid.
		 */
		for (int n_point = 0; n_point < 8 /* points */; ++n_point)
		{
			const unsigned int   epsilon					   = 1;
			int					 point_x					   = 0;
			int					 point_y					   = 6 * instance_id;
			const unsigned char* rendered_data				   = NULL;
			const unsigned char* reference_data				   = 0;
			const unsigned int   row_width					   = rt_width * 4 /* components */;
			int					 t1[6 /* 3 * {x, y} */]		   = { 0 };
			int					 t2[6 /* 3 * {x, y} */]		   = { 0 };
			int					 t1_center[2 /*     {x, y} */] = { 0 };
			int					 t2_center[2 /*     {x, y} */] = { 0 };

			/* Different ordering is used for indexed draw calls */
			if (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS ||
				drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED ||
				drawcall_type == DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS)
			{
				int index = m_unordered_elements_data[n_point];

				point_x		   = 6 * index;
				reference_data = ref_data_triangle_strip + (8 - 1 - index) * 4 /* components */;
			}
			else
			{
				point_x		   = 6 * n_point;
				reference_data = ref_data_triangle_strip + n_point * 4 /* components */;
			}

			/* Calculate triangle vertex locations (corresponds to geometry shader logic) */
			t1[0] = point_x;
			t1[1] = point_y + 6;
			t1[2] = point_x;
			t1[3] = point_y;
			t1[4] = point_x + 6;
			t1[5] = point_y + 6;

			t2[0] = point_x + 6;
			t2[1] = point_y + 6;
			t2[2] = point_x;
			t2[3] = point_y;
			t2[4] = point_x + 6;
			t2[5] = point_y;

			/* Calculate centroid locations */
			t1_center[0] = (t1[0] + t1[2] + t1[4]) / 3;
			t2_center[0] = (t2[0] + t2[2] + t2[4]) / 3;
			t1_center[1] = (t1[1] + t1[3] + t1[5]) / 3;
			t2_center[1] = (t2[1] + t2[3] + t2[5]) / 3;

			/* Check the first triangle */
			point_x = t1_center[0];
			point_y = t1_center[1];

			rendered_data = data + point_y * row_width + point_x * 4 /* components */;

			if ((unsigned int)de::abs((int)rendered_data[0] - reference_data[0]) > epsilon || rendered_data[1] != 0 ||
				rendered_data[2] != 0 || rendered_data[3] != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "At (" << (int)point_x << ", " << (int)point_y << ") "
								   << "rendered pixel (" << (int)rendered_data[0] << ", " << (int)rendered_data[1]
								   << ", " << (int)rendered_data[2] << ", " << (int)rendered_data[3] << ") "
								   << "is different than (" << (int)reference_data[0] << ", 0, 0, 0)."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Data comparison failed.");
			}

			/* Check the other triangle */
			point_x = t2_center[0];
			point_y = t2_center[1];

			rendered_data = data + point_y * row_width + point_x * 4 /* components */;

			if (rendered_data[0] != 0 || (unsigned int)de::abs((int)rendered_data[1] - reference_data[1]) > epsilon ||
				rendered_data[2] != 0 || rendered_data[3] != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "At (" << (int)point_x << ", " << (int)point_y << ") "
								   << "rendered pixel (" << (int)rendered_data[0] << ", " << (int)rendered_data[1]
								   << ", " << (int)rendered_data[2] << ", " << (int)rendered_data[3] << ") "
								   << "is different than (0, " << (int)reference_data[1] << ", 0, 0)."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Data comparison failed.");
			}
		} /* for (all points) */

		break;
	} /* case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:*/

	default:
	{
		TCU_FAIL("Unsupported output layout qualifier requested.");

		break;
	}
	} /* switch(m_output_type) */
}

/* "lines" input primitive test implementation */
/** Constructor.
 *
 *  @param use_adjacency_data true  if the test case is being instantiated for draw call modes that will
 *                                  features adjacency data,
 *                            false otherwise.
 *  @param drawcall_mode      GL draw call mode that will be used for the tests.
 *  @param output_type        Shader output type that the test case is being instantiated for.
 *  @param context            Rendering context;
 *  @param testContext        Test context;
 *  @param name               Test name.
 **/
GeometryShaderRenderingLinesCase::GeometryShaderRenderingLinesCase(Context& context, const ExtParameters& extParams,
																   const char* name, bool use_adjacency_data,
																   glw::GLenum		   drawcall_mode,
																   _shader_output_type output_type)
	: GeometryShaderRenderingCase(
		  context, extParams, name,
		  "Verifies all draw calls work correctly for specific input+output+draw call mode combination")
	, m_output_type(output_type)
	, m_drawcall_mode(drawcall_mode)
	, m_use_adjacency_data(use_adjacency_data)
	, m_raw_array_instanced_data(0)
	, m_raw_array_instanced_data_size(0)
	, m_raw_array_noninstanced_data(0)
	, m_raw_array_noninstanced_data_size(0)
	, m_unordered_array_instanced_data(0)
	, m_unordered_array_instanced_data_size(0)
	, m_unordered_array_noninstanced_data(0)
	, m_unordered_array_noninstanced_data_size(0)
	, m_unordered_elements_instanced_data(0)
	, m_unordered_elements_instanced_data_size(0)
	, m_unordered_elements_noninstanced_data(0)
	, m_unordered_elements_noninstanced_data_size(0)
	, m_unordered_elements_max_index(16) /* maximum amount of vertices generated for this case */
	, m_unordered_elements_min_index(0)
{
	/* Sanity checks */
	if (!m_use_adjacency_data)
	{
		if (drawcall_mode != GL_LINE_LOOP && drawcall_mode != GL_LINE_STRIP && drawcall_mode != GL_LINES)
		{
			TCU_FAIL("Only GL_LINE_LOOP or GL_LINE_STRIP or GL_LINES draw call modes are supported for 'lines' "
					 "geometry shader input layout qualifier test implementation");
		}
	}
	else
	{
		if (drawcall_mode != GL_LINES_ADJACENCY_EXT && drawcall_mode != GL_LINE_STRIP_ADJACENCY_EXT)
		{
			TCU_FAIL("Only GL_LINES_ADJACENCY_EXT or GL_LINE_STRIP_ADJACENCY_EXT draw call modes are supported for "
					 "'lines_adjacency' geometry shader input layout qualifier test implementation");
		}
	}

	if (output_type != SHADER_OUTPUT_TYPE_POINTS && output_type != SHADER_OUTPUT_TYPE_LINE_STRIP &&
		output_type != SHADER_OUTPUT_TYPE_TRIANGLE_STRIP)
	{
		TCU_FAIL("Unsupported output layout qualifier type requested for 'lines' geometry shader input layout "
				 "qualifier test implementation");
	}

	/* Generate data in two flavors - one for non-instanced case, the other one for instanced case */
	for (int n_case = 0; n_case < 2 /* cases */; ++n_case)
	{
		bool			is_instanced					 = (n_case != 0);
		int				n_instances						 = 0;
		float**			raw_arrays_data_ptr				 = NULL;
		unsigned int*   raw_arrays_data_size_ptr		 = NULL;
		unsigned int	rendertarget_height				 = 0;
		unsigned int	rendertarget_width				 = 0;
		float**			unordered_arrays_data_ptr		 = NULL;
		unsigned int*   unordered_arrays_data_size_ptr   = NULL;
		unsigned char** unordered_elements_data_ptr		 = NULL;
		unsigned int*   unordered_elements_data_size_ptr = NULL;

		if (!is_instanced)
		{
			/* Non-instanced case */
			n_instances						 = 1;
			raw_arrays_data_ptr				 = &m_raw_array_noninstanced_data;
			raw_arrays_data_size_ptr		 = &m_raw_array_noninstanced_data_size;
			unordered_arrays_data_ptr		 = &m_unordered_array_noninstanced_data;
			unordered_arrays_data_size_ptr   = &m_unordered_array_noninstanced_data_size;
			unordered_elements_data_ptr		 = &m_unordered_elements_noninstanced_data;
			unordered_elements_data_size_ptr = &m_unordered_elements_noninstanced_data_size;
		}
		else
		{
			/* Instanced case */
			n_instances						 = getAmountOfDrawInstances();
			raw_arrays_data_ptr				 = &m_raw_array_instanced_data;
			raw_arrays_data_size_ptr		 = &m_raw_array_instanced_data_size;
			unordered_arrays_data_ptr		 = &m_unordered_array_instanced_data;
			unordered_arrays_data_size_ptr   = &m_unordered_array_instanced_data_size;
			unordered_elements_data_ptr		 = &m_unordered_elements_instanced_data;
			unordered_elements_data_size_ptr = &m_unordered_elements_instanced_data_size;
		}

		getRenderTargetSize(n_instances, &rendertarget_width, &rendertarget_height);

		/* Store full-screen quad coordinates that will be used for actual array data generation. */
		float dx = 2.0f / float(rendertarget_width);
		float dy = 2.0f / float(rendertarget_height);

		/* Generate raw vertex array data */

		float*		 raw_array_data_traveller = NULL;
		unsigned int single_rt_height		  = 0;
		unsigned int single_rt_width		  = 0;
		unsigned int whole_rt_width			  = 0;
		unsigned int whole_rt_height		  = 0;

		switch (m_drawcall_mode)
		{
		case GL_LINE_LOOP:
		{
			*raw_arrays_data_size_ptr = 4 /* vertices making up the line strip */ * 4 /* components */ * sizeof(float);

			break;
		}

		case GL_LINE_STRIP:
		{
			*raw_arrays_data_size_ptr = 5 /* vertices making up the line strip */ * 4 /* components */ * sizeof(float);

			break;
		}

		case GL_LINE_STRIP_ADJACENCY_EXT:
		{
			*raw_arrays_data_size_ptr =
				(5 /* vertices making up the line strip */ + 2 /* additional start/end adjacency vertices */) *
				4 /* components */
				* sizeof(float);

			break;
		}

		case GL_LINES:
		{
			*raw_arrays_data_size_ptr =
				2 /* points per line segment */ * 4 /* lines */ * 4 /* components */ * sizeof(float);

			break;
		}

		case GL_LINES_ADJACENCY_EXT:
		{
			*raw_arrays_data_size_ptr =
				4 /* points per line segment */ * 4 /* lines */ * 4 /* components */ * sizeof(float);

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized draw call mode");
		}
		} /* switch (m_drawcall_mode) */

		*raw_arrays_data_ptr	 = new float[*raw_arrays_data_size_ptr / sizeof(float)];
		raw_array_data_traveller = *raw_arrays_data_ptr;

		getRenderTargetSize(1, &single_rt_width, &single_rt_height);
		getRenderTargetSize(getAmountOfDrawInstances(), &whole_rt_width, &whole_rt_height);

		/* Generate the data */
		float				   end_y = 0;
		std::vector<tcu::Vec4> quad_coordinates;
		float				   start_y = 0;
		float				   w	   = 1.0f;

		if (n_instances != 1)
		{
			float delta = float(single_rt_height) / float(whole_rt_height);

			/* Y coordinates are calculated in a vertex shader in multi-instanced case */
			start_y = 0.0f;
			end_y   = 0.0f;
			w		= delta;
		}
		else
		{
			start_y = -1;
			end_y   = 1;
		}

		/* X, Y coordinates: correspond to X & Y locations of the vertex.
		 * Z    coordinates: set to 0 if the vertex is located on top edge, otherwise set to 1.
		 * W    coordinate:  stores the delta (single-instanced RT height / multi-instanced RT height).
		 */
		float dx_multiplier = 1.5f; /* Default value for lines and line_strip output layout qualifiers */
		float dy_multiplier = 1.5f; /* Default value for lines and line_strip output layout qualifiers */

		if (m_output_type == SHADER_OUTPUT_TYPE_TRIANGLE_STRIP)
		{
			dx_multiplier = 0.0f;
			dy_multiplier = 0.0f;
		}

		quad_coordinates.push_back(tcu::Vec4(-1 + dx_multiplier * dx, start_y + dy_multiplier * dy, 0, w)); /* TL */
		quad_coordinates.push_back(tcu::Vec4(1 - dx_multiplier * dx, start_y + dy_multiplier * dy, 0, w));  /* TR */
		quad_coordinates.push_back(tcu::Vec4(1 - dx_multiplier * dx, end_y - dy_multiplier * dy, 1, w));	/* BR */
		quad_coordinates.push_back(tcu::Vec4(-1 + dx_multiplier * dx, end_y - dy_multiplier * dy, 1, w));   /* BL */

		for (int n_line_segment = 0; n_line_segment < 4 /* edges */; ++n_line_segment)
		{
			/* Note we need to clamp coordinate indices here */
			int coordinate0_index		 = (4 + n_line_segment - 1) % 4; /* protect against negative modulo values */
			int coordinate1_index		 = (n_line_segment) % 4;
			int coordinate2_index		 = (n_line_segment + 1) % 4;
			int coordinate3_index		 = (n_line_segment + 2) % 4;
			const tcu::Vec4& coordinate0 = quad_coordinates[coordinate0_index];
			const tcu::Vec4& coordinate1 = quad_coordinates[coordinate1_index];
			const tcu::Vec4& coordinate2 = quad_coordinates[coordinate2_index];
			const tcu::Vec4& coordinate3 = quad_coordinates[coordinate3_index];

			/* For GL_LINES,      we need to explicitly define start & end-points for each segment.
			 * For GL_LINE_STRIP, we only need to explicitly define first start point. Following
			 *                    vertices define subsequent points making up the line strip.
			 * For GL_LINE_LOOP,  we need all the data we used for GL_LINE_STRIP excluding the very
			 *                    last vertex.
			 *
			 * For GL_LINES_ADJACENCY_EXT, we extend GL_LINES data by vertices preceding and following points
			 * that make up a single line segment.
			 * For GL_LINE_STRIP_ADJACENCY_EXT, we extend GL_LINE_STRIP data by including a vertex preceding the
			 * actual first vertex, and by including a vertex that follows the end vertex closing the line
			 * strip.
			 */

			/* Preceding vertex */
			if (m_drawcall_mode == GL_LINES_ADJACENCY_EXT ||
				(m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT && n_line_segment == 0))
			{
				*raw_array_data_traveller = coordinate0.x();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate0.y();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate0.z();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate0.w();
				raw_array_data_traveller++;
			}

			/* Vertex 1 */
			if ((m_drawcall_mode != GL_LINE_STRIP && m_drawcall_mode != GL_LINE_STRIP_ADJACENCY_EXT &&
				 m_drawcall_mode != GL_LINE_LOOP) ||
				((m_drawcall_mode == GL_LINE_STRIP || m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT ||
				  m_drawcall_mode == GL_LINE_LOOP) &&
				 n_line_segment == 0))
			{
				*raw_array_data_traveller = coordinate1.x();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate1.y();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate1.z();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate1.w();
				raw_array_data_traveller++;
			}

			/* Vertex 2 */
			if (m_drawcall_mode != GL_LINE_LOOP || (m_drawcall_mode == GL_LINE_LOOP && n_line_segment != 3))
			{
				*raw_array_data_traveller = coordinate2.x();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate2.y();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate2.z();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate2.w();
				raw_array_data_traveller++;
			}

			/* Following vertex */
			if (m_drawcall_mode == GL_LINES_ADJACENCY_EXT ||
				(m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT && n_line_segment == 3))
			{
				*raw_array_data_traveller = coordinate3.x();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate3.y();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate3.z();
				raw_array_data_traveller++;
				*raw_array_data_traveller = coordinate3.w();
				raw_array_data_traveller++;
			}
		} /* for (all line segments) */

		/* Generate unordered data:
		 *
		 * The way we organise data in this case is that:
		 *
		 * - For index data,  start and end points are flipped for each line segment.
		 * - For vertex data, vertex locations are stored to correspond to this order.
		 *
		 * We *DO NOT* modify the order in which we draw the line segments, since that could make the verification
		 * process even more complex than it already is.
		 */
		switch (m_drawcall_mode)
		{
		case GL_LINE_LOOP:
		{
			*unordered_arrays_data_size_ptr = *raw_arrays_data_size_ptr;
			*unordered_arrays_data_ptr		= new float[*unordered_arrays_data_size_ptr / sizeof(float)];

			*unordered_elements_data_size_ptr = sizeof(unsigned char) * 4 /* points */;
			*unordered_elements_data_ptr	  = new unsigned char[*unordered_elements_data_size_ptr];

			for (unsigned int index = 0; index < 4 /* points */; ++index)
			{
				/* Gives 3-{0, 1, 2, 3} */
				int new_index = 3 - index;

				/* Store index data */
				(*unordered_elements_data_ptr)[index] = (unsigned char)new_index;

				/* Store vertex data */
				memcpy((*unordered_arrays_data_ptr) + new_index * 4 /* components */,
					   (*raw_arrays_data_ptr) + index * 4 /* components */, sizeof(float) * 4 /* components */);
			} /* for (all indices) */

			break;
		}

		case GL_LINE_STRIP:
		{
			*unordered_arrays_data_size_ptr = *raw_arrays_data_size_ptr;
			*unordered_arrays_data_ptr		= new float[*unordered_arrays_data_size_ptr / sizeof(float)];

			*unordered_elements_data_size_ptr = sizeof(unsigned char) * 5 /* points */;
			*unordered_elements_data_ptr	  = new unsigned char[*unordered_elements_data_size_ptr];

			for (unsigned int index = 0; index < 5 /* points */; ++index)
			{
				/* Gives 4-{0, 1, 2, 3, 4} */
				int new_index = 4 - index;

				/* Store index data */
				(*unordered_elements_data_ptr)[index] = (unsigned char)new_index;

				/* Store vertex data */
				memcpy((*unordered_arrays_data_ptr) + new_index * 4 /* components */,
					   (*raw_arrays_data_ptr) + index * 4 /* components */, sizeof(float) * 4 /* components */);
			} /* for (all indices) */

			break;
		}

		case GL_LINES:
		{
			*unordered_arrays_data_size_ptr = sizeof(float) * 8 /* points */ * 4 /* components */;
			*unordered_arrays_data_ptr		= new float[*unordered_arrays_data_size_ptr / sizeof(float)];

			*unordered_elements_data_size_ptr = sizeof(unsigned char) * 8 /* points */;
			*unordered_elements_data_ptr	  = new unsigned char[*unordered_elements_data_size_ptr];

			for (unsigned int index = 0; index < 8 /* points */; ++index)
			{
				/* Gives 7-{(1, 0), (3, 2), (5, 4), (7, 6)} */
				int new_index = 7 - ((index / 2) * 2 + (index + 1) % 2);

				/* Store index data */
				(*unordered_elements_data_ptr)[index] = (unsigned char)new_index;

				/* Store vertex data */
				memcpy((*unordered_arrays_data_ptr) + new_index * 4 /* components */,
					   (*raw_arrays_data_ptr) + index * 4 /* components */, sizeof(float) * 4 /* components */);
			} /* for (all indices) */

			break;
		} /* case GL_LINES: */

		case GL_LINES_ADJACENCY_EXT:
		case GL_LINE_STRIP_ADJACENCY_EXT:
		{
			/* For adjacency case, we may simplify the approach. Since the index data is now also going
			 * to include references to adjacent vertices, we can use the same ordering as in raw arrays data.
			 * Should the implementation misinterpret the data, it will treat adjacent vertex indices as actual
			 * vertex indices, breaking the verification.
			 */
			/* For array data, just point to unique vertex locations. Use the same order as in raw arrays data case
			 * to simplify the vertex shader for the pass.
			 **/
			*unordered_arrays_data_size_ptr = sizeof(float) * 4 /* points */ * 4 /* components */;
			*unordered_arrays_data_ptr		= new float[*unordered_arrays_data_size_ptr / sizeof(float)];

			if (m_drawcall_mode == GL_LINES_ADJACENCY_EXT)
			{
				*unordered_elements_data_size_ptr =
					sizeof(unsigned char) * 4 /* vertices per line segment */ * 4 /* line segments */;
			}
			else
			{
				*unordered_elements_data_size_ptr =
					sizeof(unsigned char) * (5 /* vertices making up a line strip */ + 2 /* start/end vertices */);
			}

			*unordered_elements_data_ptr = new unsigned char[*unordered_elements_data_size_ptr];

			for (int n = 0; n < 4; ++n)
			{
				(*unordered_arrays_data_ptr)[4 * n + 0] = quad_coordinates[n].x();
				(*unordered_arrays_data_ptr)[4 * n + 1] = quad_coordinates[n].y();
				(*unordered_arrays_data_ptr)[4 * n + 2] = quad_coordinates[n].z();
				(*unordered_arrays_data_ptr)[4 * n + 3] = quad_coordinates[n].w();
			}

			/* For elements data, we just walk over the quad and make sure we turn a full circle */
			unsigned char* elements_data_traveller_ptr = *unordered_elements_data_ptr;

			for (int n = 0; n < 4; ++n)
			{
				int component0_index = (n + 4 - 1) % 4; /* protect against underflow */
				int component1_index = (n) % 4;
				int component2_index = (n + 1) % 4;
				int component3_index = (n + 2) % 4;

				if (m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT)
				{
					/* Vertex adjacent to start vertex - include only at start */
					if (n == 0)
					{
						*elements_data_traveller_ptr = (unsigned char)component0_index;

						++elements_data_traveller_ptr;
					}

					/* Vertex index */
					*elements_data_traveller_ptr = (unsigned char)component1_index;

					++elements_data_traveller_ptr;

					/* End vertex and the adjacent vertex - include only for final iteration */
					if (n == 3)
					{
						/* End vertex */
						*elements_data_traveller_ptr = (unsigned char)component2_index;

						++elements_data_traveller_ptr;

						/* Adjacent vertex */
						*elements_data_traveller_ptr = (unsigned char)component3_index;

						++elements_data_traveller_ptr;
					}
				}
				else
				{
					/* GL_LINES_ADJACENCY_EXT */
					*elements_data_traveller_ptr = (unsigned char)component0_index;
					++elements_data_traveller_ptr;
					*elements_data_traveller_ptr = (unsigned char)component1_index;
					++elements_data_traveller_ptr;
					*elements_data_traveller_ptr = (unsigned char)component2_index;
					++elements_data_traveller_ptr;
					*elements_data_traveller_ptr = (unsigned char)component3_index;
					++elements_data_traveller_ptr;
				}
			}

			break;
		} /* case GL_LINES: */

		default:
		{
			TCU_FAIL("Unrecognized draw call mode");
		}
		} /* switch (m_drawcall_mode) */
	}	 /* for (both cases) */
}

/** Destructor. */
GeometryShaderRenderingLinesCase::~GeometryShaderRenderingLinesCase()
{
	if (m_raw_array_instanced_data != NULL)
	{
		delete[] m_raw_array_instanced_data;

		m_raw_array_instanced_data = NULL;
	}

	if (m_raw_array_noninstanced_data != NULL)
	{
		delete[] m_raw_array_noninstanced_data;

		m_raw_array_noninstanced_data = NULL;
	}

	if (m_unordered_array_instanced_data != NULL)
	{
		delete[] m_unordered_array_instanced_data;

		m_unordered_array_instanced_data = NULL;
	}

	if (m_unordered_array_noninstanced_data != NULL)
	{
		delete[] m_unordered_array_noninstanced_data;

		m_unordered_array_noninstanced_data = NULL;
	}

	if (m_unordered_elements_instanced_data != NULL)
	{
		delete[] m_unordered_elements_instanced_data;

		m_unordered_elements_instanced_data = NULL;
	}

	if (m_unordered_elements_noninstanced_data != NULL)
	{
		delete[] m_unordered_elements_noninstanced_data;

		m_unordered_elements_noninstanced_data = NULL;
	}
}

/** Retrieves amount of instances that should be drawn with glDraw*Elements() calls.
 *
 *  @return As per description.
 **/
unsigned int GeometryShaderRenderingLinesCase::getAmountOfDrawInstances()
{
	return 4;
}

/** Retrieves amount of indices that should be used for rendering a single instance
 *  (glDraw*Elements() calls only)
 *
 *  @return As per description.
 */
unsigned int GeometryShaderRenderingLinesCase::getAmountOfElementsPerInstance()
{
	unsigned int result = 0;

	switch (m_drawcall_mode)
	{
	case GL_LINE_LOOP:
		result = 4;
		break;
	case GL_LINE_STRIP:
		result = 5;
		break;
	case GL_LINE_STRIP_ADJACENCY_EXT:
		result = 7;
		break;
	case GL_LINES:
		result = 8;
		break;
	case GL_LINES_ADJACENCY_EXT:
		result = 16;
		break;

	default:
	{
		TCU_FAIL("Unrecognized draw call mode");
	}
	} /* switch (m_drawcall_mode) */

	return result;
}

/** Retrieves amount of vertices that should be used for rendering a single instance
 *  (glDrawArrays*() calls only)
 *
 *  @return As per description.
 **/
unsigned int GeometryShaderRenderingLinesCase::getAmountOfVerticesPerInstance()
{
	unsigned int result = 0;

	switch (m_drawcall_mode)
	{
	case GL_LINE_LOOP:
		result = 4;
		break;
	case GL_LINE_STRIP:
		result = 5;
		break;
	case GL_LINE_STRIP_ADJACENCY_EXT:
		result = 7;
		break;
	case GL_LINES:
		result = 8;
		break;
	case GL_LINES_ADJACENCY_EXT:
		result = 16;
		break;

	default:
	{
		TCU_FAIL("Unrecognized draw call mode");
	}
	} /* switch (m_drawcall_mode) */

	return result;
}

/** Draw call mode that should be used glDraw*() calls.
 *
 *  @return As per description.
 **/
glw::GLenum GeometryShaderRenderingLinesCase::getDrawCallMode()
{
	return m_drawcall_mode;
}

/** Source code for a fragment shader that should be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingLinesCase::getFragmentShaderCode()
{
	static std::string fs_code = "${VERSION}\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "in  vec4 gs_fs_color;\n"
								 "out vec4 result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    result = gs_fs_color;\n"
								 "}\n";

	return fs_code;
}

/** Source code for a geometry shader that should be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingLinesCase::getGeometryShaderCode()
{
	static const char* lines_in_line_strip_out_gs_code_preamble = "${VERSION}\n"
																  "\n"
																  "${GEOMETRY_SHADER_ENABLE}\n"
																  "\n"
																  "layout(lines)                      in;\n"
																  "layout(line_strip, max_vertices=6) out;\n"
																  "\n"
																  "#define N_VERTICES_IN (2)\n"
																  "#define N_VERTEX0     (0)\n"
																  "#define N_VERTEX1     (1)\n"
																  "\n";

	static const char* lines_adjacency_in_line_strip_out_gs_code_preamble = "${VERSION}\n"
																			"\n"
																			"${GEOMETRY_SHADER_ENABLE}\n"
																			"\n"
																			"layout(lines_adjacency)            in;\n"
																			"layout(line_strip, max_vertices=6) out;\n"
																			"\n"
																			"#define N_VERTICES_IN (4)\n"
																			"#define N_VERTEX0     (1)\n"
																			"#define N_VERTEX1     (2)\n"
																			"\n";

	static const char* lines_gs_code_main = "\n"
											"in  vec4 vs_gs_color[N_VERTICES_IN];\n"
											"out vec4 gs_fs_color;\n"
											"\n"
											"uniform ivec2 renderingTargetSize;\n"
											"\n"
											"void main()\n"
											"{\n"
											"    float dx = float(2.0) / float(renderingTargetSize.x);\n"
											"    float dy = float(2.0) / float(renderingTargetSize.y);\n"
											"\n"
											"    vec4 start_pos = gl_in[N_VERTEX0].gl_Position;\n"
											"    vec4 end_pos   = gl_in[N_VERTEX1].gl_Position;\n"
											"    vec4 start_col = vs_gs_color[N_VERTEX0];\n"
											"    vec4 end_col   = vs_gs_color[N_VERTEX1];\n"
											"\n"
											/* Determine if this is a horizontal or vertical edge */
											"    if (start_pos.x != end_pos.x)\n"
											"    {\n"
											/* Bottom line segment */
											"        gl_Position = vec4(-1.0, start_pos.y + dy, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"\n"
											"        gl_Position = vec4(1.0, end_pos.y + dy, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"        EndPrimitive();\n"
											/* Middle line segment */
											"        gl_Position = vec4(-1.0, start_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"\n"
											"        gl_Position = vec4(1.0, end_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"        EndPrimitive();\n"
											/* Top line segment */
											"        gl_Position = vec4(-1.0, start_pos.y - dy, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"\n"
											"        gl_Position = vec4(1.0, end_pos.y - dy, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"        EndPrimitive();\n"
											"    }\n"
											"    else\n"
											"    {\n"
											/* Left line segment */
											"        gl_Position = vec4(start_pos.x - dx, start_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"\n"
											"        gl_Position = vec4(end_pos.x - dx, end_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"        EndPrimitive();\n"
											/* Middle line segment */
											"        gl_Position = vec4(start_pos.x, start_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"\n"
											"        gl_Position = vec4(end_pos.x, end_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"        EndPrimitive();\n"
											/* Right line segment */
											"        gl_Position = vec4(start_pos.x + dx, start_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"\n"
											"        gl_Position = vec4(end_pos.x + dx, end_pos.y, 0, 1);\n"
											"        gs_fs_color = mix(start_col, end_col, 0.5);\n"
											"        EmitVertex();\n"
											"        EndPrimitive();\n"
											"    }\n"
											"}\n";

	static const char* lines_in_points_out_gs_code_preamble = "${VERSION}\n"
															  "\n"
															  "${GEOMETRY_SHADER_ENABLE}\n"
															  "\n"
															  "layout(lines)                   in;\n"
															  "layout(points, max_vertices=72) out;\n"
															  "\n"
															  "#define N_VERTEX0     (0)\n"
															  "#define N_VERTEX1     (1)\n"
															  "#define N_VERTICES_IN (2)\n"
															  "\n";

	static const char* lines_adjacency_in_points_out_gs_code_preamble = "${VERSION}\n"
																		"\n"
																		"${GEOMETRY_SHADER_ENABLE}\n"
																		"\n"
																		"layout(lines_adjacency)         in;\n"
																		"layout(points, max_vertices=72) out;\n"
																		"\n"
																		"#define N_VERTEX0     (1)\n"
																		"#define N_VERTEX1     (2)\n"
																		"#define N_VERTICES_IN (4)\n"
																		"\n";

	static const char* points_gs_code_main = "\n"
											 "in  vec4 vs_gs_color[N_VERTICES_IN];\n"
											 "out vec4 gs_fs_color;\n"
											 "\n"
											 "uniform ivec2 renderingTargetSize;\n"
											 "\n"
											 "void main()\n"
											 "{\n"
											 "    float dx = float(2.0) / float(renderingTargetSize.x);\n"
											 "    float dy = float(2.0) / float(renderingTargetSize.y);\n"
											 "\n"
											 "    vec4 start_pos = gl_in[N_VERTEX0].gl_Position;\n"
											 "    vec4 end_pos   = gl_in[N_VERTEX1].gl_Position;\n"
											 "    vec4 start_col = vs_gs_color[N_VERTEX0];\n"
											 "    vec4 end_col   = vs_gs_color[N_VERTEX1];\n"
											 "    vec4 delta_col = (end_col - start_col) / vec4(7.0);\n"
											 "    vec4 delta_pos = (end_pos - start_pos) / vec4(7.0);\n"
											 "\n"
											 "    for (int n_point = 0; n_point < 8; ++n_point)\n"
											 "    {\n"
											 "        vec4 ref_color = start_col + vec4(float(n_point)) * delta_col;\n"
											 "        vec4 ref_pos   = start_pos + vec4(float(n_point)) * delta_pos;\n"
											 "\n"
											 /* TL */
											 "        gl_Position  = ref_pos + vec4(-dx, -dy, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* TM */
											 "        gl_Position  = ref_pos + vec4(0, -dy, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* TR */
											 "        gl_Position  = ref_pos + vec4(dx, -dy, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* ML */
											 "        gl_Position  = ref_pos + vec4(-dx, 0, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* MM */
											 "        gl_Position  = ref_pos + vec4(0, 0, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* MR */
											 "        gl_Position  = ref_pos + vec4(dx, 0, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* BL */
											 "        gl_Position  = ref_pos + vec4(-dx, dy, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* BM */
											 "        gl_Position  = ref_pos + vec4(0, dy, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 /* BR */
											 "        gl_Position  = ref_pos + vec4(dx, dy, 0, 0);\n"
											 "        gs_fs_color  = ref_color;\n"
											 "        EmitVertex();\n"
											 "    }\n"
											 "}\n";

	static const char* lines_adjacency_in_triangle_strip_out_gs_code_preamble =
		"${VERSION}\n"
		"\n"
		"${GEOMETRY_SHADER_ENABLE}\n"
		"\n"
		"layout(lines_adjacency)                in;\n"
		"layout(triangle_strip, max_vertices=3) out;\n"
		"\n"
		"#define N_VERTEX0     (1)\n"
		"#define N_VERTEX1     (2)\n"
		"#define N_VERTICES_IN (4)\n";

	static const char* lines_in_triangle_strip_out_gs_code_preamble = "${VERSION}\n"
																	  "\n"
																	  "${GEOMETRY_SHADER_ENABLE}\n"
																	  "\n"
																	  "layout(lines)                          in;\n"
																	  "layout(triangle_strip, max_vertices=3) out;\n"
																	  "\n"
																	  "#define N_VERTEX0     (0)\n"
																	  "#define N_VERTEX1     (1)\n"
																	  "#define N_VERTICES_IN (2)\n";

	static const char* triangles_gs_code_main = "flat in   int instance_id[N_VERTICES_IN];\n"
												"     in  vec4 vs_gs_color[N_VERTICES_IN];\n"
												"     out vec4 gs_fs_color;\n"
												"\n"
												"uniform ivec2 renderingTargetSize;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    float dx = float(1.5) / float(renderingTargetSize.x);\n"
												"    float dy = float(1.5) / float(renderingTargetSize.y);\n"
												"\n"
												"    gl_Position = gl_in[N_VERTEX0].gl_Position;\n"
												"    gs_fs_color = vs_gs_color[N_VERTEX0];\n"
												"    EmitVertex();\n"
												"\n"
												"    gl_Position = gl_in[N_VERTEX1].gl_Position;\n"
												"    gs_fs_color = vs_gs_color[N_VERTEX0];\n"
												"    EmitVertex();\n"
												"\n"
												"    if (renderingTargetSize.y == 45 /* block size */)\n"
												"    {\n"
												"        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
												"    }\n"
												"    else\n"
												"    {\n"
												/* Each block takes 1/4th of the total render target.
		 * Third vertex should be placed in the middle of the block.
		 */
												"        float y = -1.0 + 1.0 / 4.0 + float(instance_id[0]) * 0.5;\n"
												"\n"
												"        gl_Position = vec4(0.0, y, 0.0, 1.0);\n"
												"    }\n"
												"\n"
												"    gs_fs_color = vs_gs_color[N_VERTEX0];\n"
												"    EmitVertex();\n"
												"\n"
												"    EndPrimitive();\n"
												"}\n";
	std::string result;

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		std::stringstream lines_adjacency_gs_code_stringstream;
		std::string		  lines_adjacency_gs_code_string;
		std::stringstream lines_gs_code_stringstream;
		std::string		  lines_gs_code_string;

		if (m_drawcall_mode == GL_LINES_ADJACENCY_EXT || m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT)
		{
			/* First request for lines_adjacency GS, form the string */
			lines_adjacency_gs_code_stringstream << lines_adjacency_in_line_strip_out_gs_code_preamble
												 << lines_gs_code_main;

			lines_adjacency_gs_code_string = lines_adjacency_gs_code_stringstream.str();
			result						   = lines_adjacency_gs_code_string;
		}
		else if (m_drawcall_mode == GL_LINES || m_drawcall_mode == GL_LINE_LOOP || m_drawcall_mode == GL_LINE_STRIP)
		{
			/* First request for lines GS, form the string */
			lines_gs_code_stringstream << lines_in_line_strip_out_gs_code_preamble << lines_gs_code_main;

			lines_gs_code_string = lines_gs_code_stringstream.str();
			result				 = lines_gs_code_string;
		}
		else
		{
			TCU_FAIL("Unrecognized draw call mode");
		}

		break;
	}

	case SHADER_OUTPUT_TYPE_POINTS:
	{
		std::stringstream lines_adjacency_gs_code_stringstream;
		std::string		  lines_adjacency_gs_code_string;
		std::stringstream lines_gs_code_stringstream;
		std::string		  lines_gs_code_string;

		if (m_drawcall_mode == GL_LINES_ADJACENCY_EXT || m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT)
		{
			/* First request for lines_adjacency GS, form the string */
			lines_adjacency_gs_code_stringstream << lines_adjacency_in_points_out_gs_code_preamble
												 << points_gs_code_main;

			lines_adjacency_gs_code_string = lines_adjacency_gs_code_stringstream.str();
			result						   = lines_adjacency_gs_code_string;
		}
		else if (m_drawcall_mode == GL_LINES || m_drawcall_mode == GL_LINE_LOOP || m_drawcall_mode == GL_LINE_STRIP)
		{
			/* First request for lines GS, form the string */
			lines_gs_code_stringstream << lines_in_points_out_gs_code_preamble << points_gs_code_main;

			lines_gs_code_string = lines_gs_code_stringstream.str();
			result				 = lines_gs_code_string;
		}
		else
		{
			TCU_FAIL("Unrecognized draw call mode");
		}

		break;
	}

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		std::stringstream lines_adjacency_gs_code_stringstream;
		std::string		  lines_adjacency_gs_code_string;
		std::stringstream lines_gs_code_stringstream;
		std::string		  lines_gs_code_string;

		if (m_drawcall_mode == GL_LINES_ADJACENCY_EXT || m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT)
		{
			/* First request for lines_adjacency GS, form the string */
			lines_adjacency_gs_code_stringstream << lines_adjacency_in_triangle_strip_out_gs_code_preamble
												 << triangles_gs_code_main;

			lines_adjacency_gs_code_string = lines_adjacency_gs_code_stringstream.str();
			result						   = lines_adjacency_gs_code_string;
		}
		else if (m_drawcall_mode == GL_LINES || m_drawcall_mode == GL_LINE_LOOP || m_drawcall_mode == GL_LINE_STRIP)
		{
			/* First request for lines GS, form the string */
			lines_gs_code_stringstream << lines_in_triangle_strip_out_gs_code_preamble << triangles_gs_code_main;

			lines_gs_code_string = lines_gs_code_stringstream.str();
			result				 = lines_gs_code_string;
		}
		else
		{
			TCU_FAIL("Unrecognized draw call mode");
		}

		break;
	}

	default:
	{
		TCU_FAIL("Requested shader output layout qualifier is unsupported");
	}
	} /* switch (m_output_type) */

	return result;
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  vertex data to be used for glDrawArrays*() calls.
 *
 *  @param instanced True if the data is to be used in regard to instanced draw calls,
 *                   false otherwise.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingLinesCase::getRawArraysDataBufferSize(bool instanced)
{
	return instanced ? m_raw_array_instanced_data_size : m_raw_array_noninstanced_data_size;
}

/** Returns vertex data for the test. Only to be used for glDrawArrays*() calls.
 *
 *  @param instanced True if the data is to be used in regard to instanced draw calls,
 *                   false otherwise.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingLinesCase::getRawArraysDataBuffer(bool instanced)
{
	return instanced ? m_raw_array_instanced_data : m_raw_array_noninstanced_data;
}

/** Retrieves resolution of off-screen rendering buffer that should be used for the test.
 *
 *  @param n_instances Amount of draw call instances this render target will be used for.
 *  @param out_width   Deref will be used to store rendertarget's width. Must not be NULL.
 *  @param out_height  Deref will be used to store rendertarget's height. Must not be NULL.
 **/
void GeometryShaderRenderingLinesCase::getRenderTargetSize(unsigned int n_instances, unsigned int* out_width,
														   unsigned int* out_height)
{
	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	case SHADER_OUTPUT_TYPE_POINTS:
	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		/* For SHADER_OUTPUT_TYPE_POINTS:
		 * An edge size of 45px should be used. Given that each input will generate a 3x3 block,
		 * this should give us a delta of 3px between the "quads".
		 *
		 * For instanced draw calls, use a delta of 3px as well.
		 *
		 * For SHADER_OUTPUT_TYPE_LINE_STRIP:
		 * Each rectangle outline will take a 45x45 block. No vertical delta needs to be used.
		 *
		 * For SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
		 * Each combination of 4 triangles makes up a triangles that takes 45x45 area.
		 * No vertical delta needs to be used.
		 */
		*out_width  = 3 /* 'pixel' size */ * 8 + 3 /* Delta size */ * 7;
		*out_height = (3 /* 'pixel' size */ * 8 + 3 /* Delta size */ * 7) * n_instances;

		break;
	}

	default:
	{
		TCU_FAIL("Unsupported shader output type");
	}
	} /* switch (m_output_type) */
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  vertex data to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingLinesCase::getUnorderedArraysDataBufferSize(bool instanced)
{
	return (instanced) ? m_unordered_array_instanced_data_size : m_unordered_array_noninstanced_data_size;
}

/** Returns vertex data for the test. Only to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingLinesCase::getUnorderedArraysDataBuffer(bool instanced)
{
	return instanced ? m_unordered_array_instanced_data : m_unordered_array_noninstanced_data;
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  index data to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingLinesCase::getUnorderedElementsDataBufferSize(bool instanced)
{
	return instanced ? m_unordered_elements_instanced_data_size : m_unordered_elements_noninstanced_data_size;
}

/** Returns index data for the test. Only to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 **/
const void* GeometryShaderRenderingLinesCase::getUnorderedElementsDataBuffer(bool instanced)
{
	return instanced ? m_unordered_elements_instanced_data : m_unordered_elements_noninstanced_data;
}

/** Returns type of the index, to be used for glDrawElements*() calls.
 *
 *  @return As per description.
 **/
glw::GLenum GeometryShaderRenderingLinesCase::getUnorderedElementsDataType()
{
	return GL_UNSIGNED_BYTE;
}

/** Retrieves maximum index value. To be used for glDrawRangeElements() test only.
 *
 *  @return As per description.
 **/
glw::GLubyte GeometryShaderRenderingLinesCase::getUnorderedElementsMaxIndex()
{
	return m_unordered_elements_max_index;
}

/** Retrieves minimum index value. To be used for glDrawRangeElements() test only.
 *
 *  @return As per description.
 **/
glw::GLubyte GeometryShaderRenderingLinesCase::getUnorderedElementsMinIndex()
{
	return m_unordered_elements_min_index;
}

/** Retrieves vertex shader code to be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingLinesCase::getVertexShaderCode()
{
	static std::string vs_code =
		"${VERSION}\n"
		"\n"
		"     in      vec4  position;\n"
		"     uniform bool  is_indexed_draw_call;\n"
		"     uniform bool  is_gl_lines_adjacency_draw_call;\n"
		"     uniform bool  is_gl_line_strip_adjacency_draw_call;\n"
		"     uniform bool  is_gl_lines_draw_call;\n"
		"     uniform bool  is_gl_line_loop_draw_call;\n"
		"     uniform ivec2 renderingTargetSize;\n"
		"flat out     int   instance_id;\n"
		"     out     vec4  vs_gs_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    instance_id = gl_InstanceID;\n"
		"\n"
		/* non-instanced */
		"    if (renderingTargetSize.y == 45 /* block size */)\n"
		"    {\n"
		"        gl_Position = position;\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        bool  represents_top_edge = (position.z == 0.0);\n"
		"        float delta               = position.w;\n"
		"        float y                   = 0.0;\n"
		"\n"
		"        if (represents_top_edge)\n"
		"        {\n"
		/* top vertices */
		"            y = position.y + float(gl_InstanceID) * delta * 2.0 - 1.0;\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            y = position.y + float(gl_InstanceID) * delta * 2.0 - 1.0 + delta * 2.0;\n"
		/* bottom vertices */
		"        }\n"
		"\n"
		"        gl_Position = vec4(position.x,\n"
		"                           y,\n"
		"                           position.z,\n"
		"                           1.0);\n"
		"    }\n"
		"\n"
		"    vs_gs_color = vec4(0, 0, 0, 0);\n"
		"\n"
		"    if (is_gl_line_loop_draw_call)\n"
		"    {\n"
		/* GL_LINE_LOOP */
		"        if (!is_indexed_draw_call)\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 0: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"                case 1: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"                case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"                case 3: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 3: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"                case 0: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"                case 1: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"                case 2: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"    else\n"
		"    if (is_gl_line_strip_adjacency_draw_call)\n"
		"    {\n"
		/* GL_LINE_STRIP_ADJACENCY_EXT */
		"        if (!is_indexed_draw_call)\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 1:\n"
		"                case 5: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"                case 2: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"                case 3: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"                case 0:\n"
		"                case 4: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 0: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"                case 1: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"                case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"                case 3: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"    else\n"
		"    if (is_gl_lines_adjacency_draw_call)\n"
		"    {\n"
		/* GL_LINES_ADJACENCY_EXT */
		"        if (!is_indexed_draw_call)\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 1:\n"
		"                case 14: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"\n"
		"                case 2:\n"
		"                case 5: vs_gs_color = vec4(0, 1, 0, 0); break;\n"

		"                case 6:\n"
		"                case 9: vs_gs_color = vec4(0, 0, 1, 0); break;\n"

		"                case 10:\n"
		"                case 13: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 0: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"                case 1: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"                case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"                case 3: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"    else\n"
		"    if (is_gl_lines_draw_call)\n"
		"    {\n"
		/* GL_LINES */
		"        if (!is_indexed_draw_call)\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 0:\n"
		"                case 7: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"\n"
		"                case 1:\n"
		"                case 2: vs_gs_color = vec4(0, 1, 0, 0); break;\n"

		"                case 3:\n"
		"                case 4: vs_gs_color = vec4(0, 0, 1, 0); break;\n"

		"                case 5:\n"
		"                case 6: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 6:\n"
		"                case 1: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"\n"
		"                case 7:\n"
		"                case 4: vs_gs_color = vec4(0, 1, 0, 0); break;\n"

		"                case 5:\n"
		"                case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"

		"                case 3:\n"
		"                case 0: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"    else\n"
		"    {\n"
		/* GL_LINE_STRIP */
		"        if (!is_indexed_draw_call)\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 0:\n"
		"                case 4: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"                case 1: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"                case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"                case 3: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"            }\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            switch(gl_VertexID)\n"
		"            {\n"
		"                case 0:\n"
		"                case 4: vs_gs_color = vec4(1, 0, 0, 0); break;\n"
		"                case 1: vs_gs_color = vec4(0, 0, 0, 1); break;\n"
		"                case 2: vs_gs_color = vec4(0, 0, 1, 0); break;\n"
		"                case 3: vs_gs_color = vec4(0, 1, 0, 0); break;\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"}\n";

	std::string result;

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	case SHADER_OUTPUT_TYPE_POINTS:
	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		result = vs_code;

		break;
	}

	default:
	{
		TCU_FAIL("Unsupported shader output type used");
	}
	} /* switch (m_output_type) */

	return result;
}

/** Sets test-specific uniforms for a program object that is then used for the draw call.
 *
 *  @param drawcall_type Type of the draw call that is to follow right after this function is called.
 **/
void GeometryShaderRenderingLinesCase::setUniformsBeforeDrawCall(_draw_call_type drawcall_type)
{
	const glw::Functions& gl							  = m_context.getRenderContext().getFunctions();
	glw::GLint is_gl_line_loop_draw_call_uniform_location = gl.getUniformLocation(m_po_id, "is_gl_line_loop_draw_call");
	glw::GLint is_gl_line_strip_adjacency_draw_call_uniform_location =
		gl.getUniformLocation(m_po_id, "is_gl_line_strip_adjacency_draw_call");
	glw::GLint is_gl_lines_adjacency_draw_call_uniform_location =
		gl.getUniformLocation(m_po_id, "is_gl_lines_adjacency_draw_call");
	glw::GLint is_gl_lines_draw_call_uniform_location = gl.getUniformLocation(m_po_id, "is_gl_lines_draw_call");
	glw::GLint is_indexed_draw_call_uniform_location  = gl.getUniformLocation(m_po_id, "is_indexed_draw_call");

	TCU_CHECK(is_gl_line_loop_draw_call_uniform_location != -1);
	TCU_CHECK(is_gl_line_strip_adjacency_draw_call_uniform_location != -1);
	TCU_CHECK(is_gl_lines_adjacency_draw_call_uniform_location != -1);
	TCU_CHECK(is_gl_lines_draw_call_uniform_location != -1);
	TCU_CHECK(is_indexed_draw_call_uniform_location != -1);

	gl.uniform1i(is_indexed_draw_call_uniform_location, (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS ||
														 drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED ||
														 drawcall_type == DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS));
	gl.uniform1i(is_gl_line_loop_draw_call_uniform_location, (m_drawcall_mode == GL_LINE_LOOP));
	gl.uniform1i(is_gl_line_strip_adjacency_draw_call_uniform_location,
				 (m_drawcall_mode == GL_LINE_STRIP_ADJACENCY_EXT));
	gl.uniform1i(is_gl_lines_adjacency_draw_call_uniform_location, (m_drawcall_mode == GL_LINES_ADJACENCY_EXT));
	gl.uniform1i(is_gl_lines_draw_call_uniform_location, (m_drawcall_mode == GL_LINES));
}

/** Verifies that the rendered data is correct.
 *
 *  @param drawcall_type Type of the draw call that was used to render the geometry.
 *  @param instance_id   Instance ID to be verified. Use 0 for non-instanced draw calls.
 *  @param data          Contents of the rendertarget after the test has finished rendering.
 **/
void GeometryShaderRenderingLinesCase::verify(_draw_call_type drawcall_type, unsigned int instance_id,
											  const unsigned char* data)
{
	const float  epsilon		  = 1.0f / 256.0f;
	unsigned int rt_height		  = 0;
	unsigned int rt_width		  = 0;
	unsigned int single_rt_height = 0;
	unsigned int single_rt_width  = 0;

	if (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED ||
		drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED)
	{
		getRenderTargetSize(getAmountOfDrawInstances(), &rt_width, &rt_height);
	}
	else
	{
		getRenderTargetSize(1, &rt_width, &rt_height);
	}

	getRenderTargetSize(1, &single_rt_width, &single_rt_height);

	/* Verification is output type-specific */
	const unsigned int row_width = rt_width * 4 /* components */;

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		/* The test renders a rectangle outline with 3 line segments for each edge.
		 * The verification checks color of each edge's middle line segment.
		 *
		 * Corners are skipped to keep the implementation simple. */
		for (unsigned int n_edge = 0; n_edge < 4 /* edges */; ++n_edge)
		{
			/* Determine edge-specific properties:
			 *
			 * Edge 0 - top edge;
			 * Edge 1 - right edge;
			 * Edge 2 - bottom edge;
			 * Edge 3 - left edge.
			 *
			 **/
			int		  end_x = 0;
			int		  end_y = 0;
			tcu::Vec4 expected_rgba;
			int		  start_x = 0;
			int		  start_y = 0;

			switch (n_edge)
			{
			case 0:
			{
				/* Top edge */
				start_x = 3;								  /* skip the corner */
				start_y = 1 + instance_id * single_rt_height; /* middle segment */

				end_x = single_rt_width - 3; /* skip the corner */
				end_y = start_y;

				expected_rgba = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f) * tcu::Vec4(0.5f) +
								tcu::Vec4(0.0f, 1.0f, 0.0f, 0.0f) * tcu::Vec4(0.5f);

				break;
			}

			case 1:
			{
				/* Right edge */
				start_x = single_rt_width - 2;				  /* middle segment */
				start_y = 3 + instance_id * single_rt_height; /* skip the corner */

				end_x = start_x;
				end_y = start_y + single_rt_height - 6; /* skip the corners */

				expected_rgba = tcu::Vec4(0.0f, 1.0f, 0.0f, 0.0f) * tcu::Vec4(0.5f) +
								tcu::Vec4(0.0f, 0.0f, 1.0f, 0.0f) * tcu::Vec4(0.5f);

				break;
			}

			case 2:
			{
				/* Bottom edge */
				start_x = 3;													 /* skip the corner */
				start_y = single_rt_height - 3 + instance_id * single_rt_height; /* middle segment */

				end_x = single_rt_width - 6; /* skip the corners */
				end_y = start_y;

				expected_rgba = tcu::Vec4(0.0f, 0.0f, 1.0f, 0.0f) * tcu::Vec4(0.5f) +
								tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f) * tcu::Vec4(0.5f);

				break;
			}

			case 3:
			{
				/* Left edge */
				start_x = 1;								  /* middle segment */
				start_y = 3 + instance_id * single_rt_height; /* skip the corner */

				end_x = start_x;
				end_y = start_y + single_rt_height - 6; /* skip the corners */

				expected_rgba = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f) * tcu::Vec4(0.5f) +
								tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f) * tcu::Vec4(0.5f);

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized edge index");
			}
			} /* switch(n_edge) */

			/* Move over the edge and make sure the rendered pixels are valid */
			int dx		 = (end_x != start_x) ? 1 : 0;
			int dy		 = (end_y != start_y) ? 1 : 0;
			int n_pixels = (end_x - start_x) + (end_y - start_y);

			for (int n_pixel = 0; n_pixel < n_pixels; ++n_pixel)
			{
				int cur_x = start_x + n_pixel * dx;
				int cur_y = start_y + n_pixel * dy;

				/* Calculate expected and rendered pixel color */
				const unsigned char* read_data		  = data + cur_y * row_width + cur_x * 4 /* components */;
				float				 rendered_rgba[4] = { 0 };

				for (unsigned int n_channel = 0; n_channel < 4 /* RGBA */; ++n_channel)
				{
					rendered_rgba[n_channel] = float(read_data[n_channel]) / 255.0f;
				}

				/* Compare the data */
				if (de::abs(rendered_rgba[0] - expected_rgba[0]) > epsilon ||
					de::abs(rendered_rgba[1] - expected_rgba[1]) > epsilon ||
					de::abs(rendered_rgba[2] - expected_rgba[2]) > epsilon ||
					de::abs(rendered_rgba[3] - expected_rgba[3]) > epsilon)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data at (" << cur_x << ", " << cur_y
									   << ") "
									   << "equal (" << rendered_rgba[0] << ", " << rendered_rgba[1] << ", "
									   << rendered_rgba[2] << ", " << rendered_rgba[3] << ") "
									   << "exceeds allowed epsilon when compared to reference data equal ("
									   << expected_rgba[0] << ", " << expected_rgba[1] << ", " << expected_rgba[2]
									   << ", " << expected_rgba[3] << ")." << tcu::TestLog::EndMessage;

					TCU_FAIL("Data comparison failed");
				} /* if (data comparison failed) */
			}	 /* for (all points) */
		}		  /* for (all edges) */

		break;
	} /* case SHADER_OUTPUT_TYPE_LINE_STRIP: */

	case SHADER_OUTPUT_TYPE_POINTS:
	{
		/* Verify centers of the points generated by geometry shader */
		int dx = 0;
		int dy = 0;

		bool ignore_first_point = false;
		int  start_x			= 0;
		int  start_y			= 0;

		tcu::Vec4 end_color;
		tcu::Vec4 start_color;

		for (unsigned int n_edge = 0; n_edge < 4 /* edges */; ++n_edge)
		{
			/* Determine edge-specific properties:
			 *
			 * Edge 0 - top edge; NOTE: the test should skip point at (1px, 1px) as it is overwritten by
			 *          edge 3!
			 * Edge 1 - right edge;
			 * Edge 2 - bottom edge;
			 * Edge 3 - left edge.
			 *
			 **/
			switch (n_edge)
			{
			case 0:
			{
				dx				   = 6;
				dy				   = 0;
				ignore_first_point = true;
				start_x			   = 1;
				start_y			   = 1 + instance_id * single_rt_height;

				end_color   = tcu::Vec4(0.0f, 1.0f, 0.0f, 0.0f);
				start_color = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f);

				break;
			}

			case 1:
			{
				dx				   = 0;
				dy				   = 6;
				ignore_first_point = false;
				start_x			   = single_rt_width - 1;
				start_y			   = 1 + instance_id * single_rt_height;

				end_color   = tcu::Vec4(0.0f, 0.0f, 1.0f, 0.0f);
				start_color = tcu::Vec4(0.0f, 1.0f, 0.0f, 0.0f);

				break;
			}

			case 2:
			{
				dx				   = -6;
				dy				   = 0;
				ignore_first_point = false;
				start_x			   = single_rt_width - 1;
				start_y			   = single_rt_height - 1 + instance_id * single_rt_height;

				end_color   = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
				start_color = tcu::Vec4(0.0f, 0.0f, 1.0f, 0.0f);

				break;
			}

			case 3:
			{
				dx				   = 0;
				dy				   = -6;
				ignore_first_point = false;
				start_x			   = 1;
				start_y			   = single_rt_height - 1 + instance_id * single_rt_height;

				end_color   = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f);
				start_color = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized edge index");
			}
			} /* switch(n_edge) */

			for (unsigned int n_point = 0; n_point < 8 /* points */; ++n_point)
			{
				int cur_x = start_x + n_point * dx;
				int cur_y = start_y + n_point * dy;

				/* Skip the iteration if we're dealing with a first point, for which
				 * the comparison should be skipped */
				if (ignore_first_point && n_point == 0)
				{
					continue;
				}

				/* Calculate expected and rendered pixel color */
				const unsigned char* read_data		   = data + cur_y * row_width + cur_x * 4 /* components */;
				float				 reference_rgba[4] = { 0 };
				float				 rendered_rgba[4]  = { 0 };

				for (unsigned int n_channel = 0; n_channel < 4 /* RGBA */; ++n_channel)
				{
					reference_rgba[n_channel] = start_color[n_channel] +
												(end_color[n_channel] - start_color[n_channel]) * float(n_point) / 7.0f;
					rendered_rgba[n_channel] = (float)(read_data[n_channel]) / 255.0f;
				}

				/* Compare the data */
				if (de::abs(rendered_rgba[0] - reference_rgba[0]) > epsilon ||
					de::abs(rendered_rgba[1] - reference_rgba[1]) > epsilon ||
					de::abs(rendered_rgba[2] - reference_rgba[2]) > epsilon ||
					de::abs(rendered_rgba[3] - reference_rgba[3]) > epsilon)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data at (" << cur_x << ", " << cur_y
									   << ") "
									   << "equal (" << rendered_rgba[0] << ", " << rendered_rgba[1] << ", "
									   << rendered_rgba[2] << ", " << rendered_rgba[3] << ") "
									   << "exceeds allowed epsilon when compared to reference data equal ("
									   << reference_rgba[0] << ", " << reference_rgba[1] << ", " << reference_rgba[2]
									   << ", " << reference_rgba[3] << ")." << tcu::TestLog::EndMessage;

					TCU_FAIL("Data comparison failed");
				} /* if (data comparison failed) */

			} /* for (all points) */
		}	 /* for (all edges) */

		/* Done */
		break;
	} /* case SHADER_OUTPUT_TYPE_POINTS: */

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		/* The test renders four triangles - two vertices are taken from each edge and the third
		 * one is set at (0, 0, 0, 1) (screen-space). The rendering output is verified by
		 * sampling centroids off each triangle */
		for (unsigned int n_edge = 0; n_edge < 4 /* edges */; ++n_edge)
		{
			/* Determine edge-specific properties */
			tcu::Vec2 centroid;
			tcu::Vec4 reference_rgba;
			tcu::Vec2 v1;
			tcu::Vec2 v2;
			tcu::Vec2 v3 = tcu::Vec2(0.5f, 0.5f + float(instance_id));

			switch (n_edge)
			{
			case 0:
			{
				/* Top edge */
				v1 = tcu::Vec2(0.0f, float(instance_id) + 0.0f);
				v2 = tcu::Vec2(1.0f, float(instance_id) + 0.0f);

				reference_rgba = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f);

				break;
			}

			case 1:
			{
				/* Right edge */
				v1 = tcu::Vec2(1.0f, float(instance_id) + 0.0f);
				v2 = tcu::Vec2(1.0f, float(instance_id) + 1.0f);

				reference_rgba = tcu::Vec4(0.0f, 1.0f, 0.0f, 0.0f);

				break;
			}

			case 2:
			{
				/* Bottom edge */
				v1 = tcu::Vec2(0.0f, float(instance_id) + 1.0f);
				v2 = tcu::Vec2(1.0f, float(instance_id) + 1.0f);

				reference_rgba = tcu::Vec4(0.0f, 0.0f, 1.0f, 0.0f);

				break;
			}

			case 3:
			{
				/* Left edge */
				v1 = tcu::Vec2(0.0f, float(instance_id) + 0.0f);
				v2 = tcu::Vec2(0.0f, float(instance_id) + 1.0f);

				reference_rgba = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized edge index");
			}
			} /* switch (n_edge) */

			/* Calculate centroid of the triangle. */
			centroid[0] = (v1[0] + v2[0] + v3[0]) / 3.0f;
			centroid[1] = (v1[1] + v2[1] + v3[1]) / 3.0f;

			/* Retrieve the sample */
			int centroid_xy[2] = { int(centroid[0] * float(single_rt_width)),
								   int(centroid[1] * float(single_rt_height)) };
			const unsigned char* rendered_rgba_ubyte =
				data + centroid_xy[1] * row_width + centroid_xy[0] * 4 /* components */;
			const float rendered_rgba_float[] = {
				float(rendered_rgba_ubyte[0]) / 255.0f, float(rendered_rgba_ubyte[1]) / 255.0f,
				float(rendered_rgba_ubyte[2]) / 255.0f, float(rendered_rgba_ubyte[3]) / 255.0f,
			};

			/* Compare the reference and rendered pixels */
			if (de::abs(rendered_rgba_float[0] - reference_rgba[0]) > epsilon ||
				de::abs(rendered_rgba_float[1] - reference_rgba[1]) > epsilon ||
				de::abs(rendered_rgba_float[2] - reference_rgba[2]) > epsilon ||
				de::abs(rendered_rgba_float[3] - reference_rgba[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data at (" << centroid_xy[0] << ", "
								   << centroid_xy[1] << ") "
								   << "equal (" << rendered_rgba_float[0] << ", " << rendered_rgba_float[1] << ", "
								   << rendered_rgba_float[2] << ", " << rendered_rgba_float[3] << ") "
								   << "exceeds allowed epsilon when compared to reference data equal ("
								   << reference_rgba[0] << ", " << reference_rgba[1] << ", " << reference_rgba[2]
								   << ", " << reference_rgba[3] << ")." << tcu::TestLog::EndMessage;

				TCU_FAIL("Data comparison failed");
			} /* if (data comparison failed) */
		}	 /* for (all edges) */

		break;
	}

	default:
	{
		TCU_FAIL("Unsupported shader output type used");
	}
	} /* switch (m_output_type) */
}

/** Constructor.
 *
 *  @param use_adjacency_data true  if the test case is being instantiated for draw call modes that will
 *                                  features adjacency data,
 *                            false otherwise.
 *  @param drawcall_mode      GL draw call mode that will be used for the tests.
 *  @param output_type        Shader output type that the test case is being instantiated for.
 *  @param context            Rendering context;
 *  @param testContext        Test context;
 *  @param name               Test name.
 **/
GeometryShaderRenderingTrianglesCase::GeometryShaderRenderingTrianglesCase(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, bool use_adjacency_data,
																		   glw::GLenum		   drawcall_mode,
																		   _shader_output_type output_type)
	: GeometryShaderRenderingCase(
		  context, extParams, name,
		  "Verifies all draw calls work correctly for specific input+output+draw call mode combination")
	, m_output_type(output_type)
	, m_drawcall_mode(drawcall_mode)
	, m_use_adjacency_data(use_adjacency_data)
	, m_raw_array_instanced_data(0)
	, m_raw_array_instanced_data_size(0)
	, m_raw_array_noninstanced_data(0)
	, m_raw_array_noninstanced_data_size(0)
	, m_unordered_array_instanced_data(0)
	, m_unordered_array_instanced_data_size(0)
	, m_unordered_array_noninstanced_data(0)
	, m_unordered_array_noninstanced_data_size(0)
	, m_unordered_elements_instanced_data(0)
	, m_unordered_elements_instanced_data_size(0)
	, m_unordered_elements_noninstanced_data(0)
	, m_unordered_elements_noninstanced_data_size(0)
	, m_unordered_elements_max_index(24) /* maximum amount of vertices generated by this test case */
	, m_unordered_elements_min_index(0)
{
	/* Sanity checks */
	if (!m_use_adjacency_data)
	{
		if (drawcall_mode != GL_TRIANGLES && drawcall_mode != GL_TRIANGLE_STRIP && drawcall_mode != GL_TRIANGLE_FAN)
		{
			TCU_FAIL("Only GL_TRIANGLES or GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN draw call modes are supported for "
					 "'triangles' geometry shader input layout qualifier test implementation");
		}
	}
	else
	{
		if (drawcall_mode != GL_TRIANGLES_ADJACENCY_EXT && drawcall_mode != GL_TRIANGLE_STRIP_ADJACENCY_EXT)
		{
			TCU_FAIL("Only GL_TRIANGLES_ADJACENCY_EXT or GL_TRIANGLE_STRIP_ADJACENCY_EXT draw call modes are supported "
					 "for 'triangles_adjacency' geometry shader input layout qualifier test implementation");
		}
	}

	if (output_type != SHADER_OUTPUT_TYPE_POINTS && output_type != SHADER_OUTPUT_TYPE_LINE_STRIP &&
		output_type != SHADER_OUTPUT_TYPE_TRIANGLE_STRIP)
	{
		TCU_FAIL("Unsupported output layout qualifier type requested for 'triangles' geometry shader input layout "
				 "qualifier test implementation");
	}

	/* Generate data in two flavors - one for non-instanced case, the other one for instanced case */
	for (int n_case = 0; n_case < 2 /* cases */; ++n_case)
	{
		bool			is_instanced					 = (n_case != 0);
		int				n_instances						 = 0;
		float**			raw_arrays_data_ptr				 = NULL;
		unsigned int*   raw_arrays_data_size_ptr		 = NULL;
		unsigned int	rendertarget_height				 = 0;
		unsigned int	rendertarget_width				 = 0;
		float**			unordered_arrays_data_ptr		 = NULL;
		unsigned int*   unordered_arrays_data_size_ptr   = NULL;
		unsigned char** unordered_elements_data_ptr		 = NULL;
		unsigned int*   unordered_elements_data_size_ptr = NULL;

		if (!is_instanced)
		{
			/* Non-instanced case */
			n_instances						 = 1;
			raw_arrays_data_ptr				 = &m_raw_array_noninstanced_data;
			raw_arrays_data_size_ptr		 = &m_raw_array_noninstanced_data_size;
			unordered_arrays_data_ptr		 = &m_unordered_array_noninstanced_data;
			unordered_arrays_data_size_ptr   = &m_unordered_array_noninstanced_data_size;
			unordered_elements_data_ptr		 = &m_unordered_elements_noninstanced_data;
			unordered_elements_data_size_ptr = &m_unordered_elements_noninstanced_data_size;
		}
		else
		{
			/* Instanced case */
			n_instances						 = getAmountOfDrawInstances();
			raw_arrays_data_ptr				 = &m_raw_array_instanced_data;
			raw_arrays_data_size_ptr		 = &m_raw_array_instanced_data_size;
			unordered_arrays_data_ptr		 = &m_unordered_array_instanced_data;
			unordered_arrays_data_size_ptr   = &m_unordered_array_instanced_data_size;
			unordered_elements_data_ptr		 = &m_unordered_elements_instanced_data;
			unordered_elements_data_size_ptr = &m_unordered_elements_instanced_data_size;
		}

		getRenderTargetSize(n_instances, &rendertarget_width, &rendertarget_height);

		/* Store full-screen quad coordinates that will be used for actual array data generation. */
		float dx = 2.0f / float(rendertarget_width);
		float dy = 2.0f / float(rendertarget_height);

		/* Generate raw vertex array data */
		unsigned int single_rt_height = 0;
		unsigned int single_rt_width  = 0;
		unsigned int whole_rt_width   = 0;
		unsigned int whole_rt_height  = 0;

		*raw_arrays_data_size_ptr =
			static_cast<unsigned int>(getAmountOfVerticesPerInstance() * 4 /* components */ * sizeof(float));
		*raw_arrays_data_ptr = new float[*raw_arrays_data_size_ptr / sizeof(float)];

		getRenderTargetSize(1, &single_rt_width, &single_rt_height);
		getRenderTargetSize(getAmountOfDrawInstances(), &whole_rt_width, &whole_rt_height);

		/* Generate the general coordinates storage first.
		 *
		 * For non-instanced draw calls, we only need to draw a single instance, hence we are free
		 * to use screen-space coordinates.
		 * For instanced draw calls, we'll have the vertex shader add gl_InstanceID-specific deltas
		 * to make sure the vertices are laid out correctly, so map <-1, 1> range to <0, screen_space_height_of_single_instance>
		 */
		std::vector<tcu::Vec4> data_coordinates;
		float				   dx_multiplier = 0.0f;
		float				   dy_multiplier = 0.0f;
		float				   end_y		 = 0.0f;
		float				   mid_y		 = 0.0f;
		float				   start_y		 = 0.0f;

		if (is_instanced)
		{
			start_y = -1.0f;
			end_y   = start_y + float(single_rt_height) / float(whole_rt_height) * 2.0f;
			mid_y   = start_y + (end_y - start_y) * 0.5f;
		}
		else
		{
			end_y   = 1.0f;
			mid_y   = 0.0f;
			start_y = -1.0f;
		}

		if (output_type == SHADER_OUTPUT_TYPE_POINTS)
		{
			dx_multiplier = 1.5f;
			dy_multiplier = 1.5f;
		}
		else if (output_type == SHADER_OUTPUT_TYPE_LINE_STRIP)
		{
			dx_multiplier = 1.5f;
			dy_multiplier = 1.5f;
		}

		/* W stores information whether given vertex is the middle vertex */
		data_coordinates.push_back(tcu::Vec4(0, mid_y, 0, 1));						  /* Middle vertex */
		data_coordinates.push_back(tcu::Vec4(-1 + dx * dx_multiplier, mid_y, 0, 0));  /* Left vertex */
		data_coordinates.push_back(tcu::Vec4(0, start_y + dy * dy_multiplier, 0, 0)); /* Top vertex */
		data_coordinates.push_back(tcu::Vec4(1 - dx * dx_multiplier, mid_y, 0, 0));   /* Right vertex */
		data_coordinates.push_back(tcu::Vec4(0, end_y - dy * dy_multiplier, 0, 0));   /* Bottom vertex */

		/* Now that we have the general storage ready, we can generate raw array data for specific draw
		 * call that this specific test instance will be verifying */
		int	n_raw_array_indices	 = 0;
		int	raw_array_indices[24]   = { -1 }; /* 12 is a max for all supported input geometry */
		float* raw_array_traveller_ptr = *raw_arrays_data_ptr;

		for (unsigned int n = 0; n < sizeof(raw_array_indices) / sizeof(raw_array_indices[0]); ++n)
		{
			raw_array_indices[n] = -1;
		}

		switch (m_drawcall_mode)
		{
		case GL_TRIANGLES:
		{
			/* ABC triangle */
			raw_array_indices[0] = 0;
			raw_array_indices[1] = 1;
			raw_array_indices[2] = 2;

			/* ACD triangle */
			raw_array_indices[3] = 0;
			raw_array_indices[4] = 2;
			raw_array_indices[5] = 3;

			/* ADE triangle */
			raw_array_indices[6] = 0;
			raw_array_indices[7] = 3;
			raw_array_indices[8] = 4;

			/* AEB triangle */
			raw_array_indices[9]  = 0;
			raw_array_indices[10] = 4;
			raw_array_indices[11] = 1;

			n_raw_array_indices = 12;

			break;
		} /* case GL_TRIANGLES: */

		case GL_TRIANGLES_ADJACENCY_EXT:
		{
			/* Note: Geometry shader used by this test does not rely on adjacency data
			 *       so we will fill corresponding indices with meaningless information
			 *       (always first vertex data) */
			/* ABC triangle */
			raw_array_indices[0] = 0;
			raw_array_indices[1] = 0;
			raw_array_indices[2] = 1;
			raw_array_indices[3] = 0;
			raw_array_indices[4] = 2;
			raw_array_indices[5] = 0;

			/* ACD triangle */
			raw_array_indices[6]  = 0;
			raw_array_indices[7]  = 0;
			raw_array_indices[8]  = 2;
			raw_array_indices[9]  = 0;
			raw_array_indices[10] = 3;
			raw_array_indices[11] = 0;

			/* ADE triangle */
			raw_array_indices[12] = 0;
			raw_array_indices[13] = 0;
			raw_array_indices[14] = 3;
			raw_array_indices[15] = 0;
			raw_array_indices[16] = 4;
			raw_array_indices[17] = 0;

			/* AEB triangle */
			raw_array_indices[18] = 0;
			raw_array_indices[19] = 0;
			raw_array_indices[20] = 4;
			raw_array_indices[21] = 0;
			raw_array_indices[22] = 1;
			raw_array_indices[23] = 0;

			n_raw_array_indices = 24;

			break;
		} /* case GL_TRIANGLES_ADJACENCY_EXT:*/

		case GL_TRIANGLE_FAN:
		{
			/* ABCDEB */
			raw_array_indices[0] = 0;
			raw_array_indices[1] = 1;
			raw_array_indices[2] = 2;
			raw_array_indices[3] = 3;
			raw_array_indices[4] = 4;
			raw_array_indices[5] = 1;

			n_raw_array_indices = 6;

			break;
		} /* case GL_TRIANGLE_FAN: */

		case GL_TRIANGLE_STRIP:
		{
			/* BACDAEB.
			 *
			 * Note that this will generate a degenerate triangle (ACD & CDA) but that's fine
			 * since we only sample triangle centroids in this test.
			 */
			raw_array_indices[0] = 1;
			raw_array_indices[1] = 0;
			raw_array_indices[2] = 2;
			raw_array_indices[3] = 3;
			raw_array_indices[4] = 0;
			raw_array_indices[5] = 4;
			raw_array_indices[6] = 1;

			n_raw_array_indices = 7;

			break;
		} /* case GL_TRIANGLE_STRIP: */

		case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
		{
			/* Order as in GL_TRIANGLE_STRIP case. Adjacency data not needed for the test,
			 * hence any data can be used.
			 */
			raw_array_indices[0]  = 1;
			raw_array_indices[1]  = 0;
			raw_array_indices[2]  = 0;
			raw_array_indices[3]  = 0;
			raw_array_indices[4]  = 2;
			raw_array_indices[5]  = 0;
			raw_array_indices[6]  = 3;
			raw_array_indices[7]  = 0;
			raw_array_indices[8]  = 0;
			raw_array_indices[9]  = 0;
			raw_array_indices[10] = 4;
			raw_array_indices[11] = 0;
			raw_array_indices[12] = 1;
			raw_array_indices[13] = 0;

			n_raw_array_indices = 14;
			break;
		} /* case GL_TRIANGLE_STRIP_ADJACENCY_EXT: */

		default:
		{
			TCU_FAIL("Unsupported draw call mode");
		}
		} /* switch (m_drawcall_mode) */

		for (int index = 0; index < n_raw_array_indices; ++index)
		{
			if (raw_array_indices[index] != -1)
			{
				*raw_array_traveller_ptr = data_coordinates[raw_array_indices[index]].x();
				raw_array_traveller_ptr++;
				*raw_array_traveller_ptr = data_coordinates[raw_array_indices[index]].y();
				raw_array_traveller_ptr++;
				*raw_array_traveller_ptr = data_coordinates[raw_array_indices[index]].z();
				raw_array_traveller_ptr++;
				*raw_array_traveller_ptr = data_coordinates[raw_array_indices[index]].w();
				raw_array_traveller_ptr++;
			}
		}

		/* Generate unordered data:
		 *
		 * Store vertices in reversed order and configure indices so that the pipeline receives
		 * vertex data in original order */
		*unordered_arrays_data_size_ptr   = *raw_arrays_data_size_ptr;
		*unordered_arrays_data_ptr		  = new float[*unordered_arrays_data_size_ptr];
		*unordered_elements_data_size_ptr = static_cast<unsigned int>(n_raw_array_indices * sizeof(unsigned char));
		*unordered_elements_data_ptr	  = new unsigned char[*unordered_elements_data_size_ptr];

		/* Set unordered array data first */
		for (int index = 0; index < n_raw_array_indices; ++index)
		{
			memcpy(*unordered_arrays_data_ptr + 4 /* components */ * (n_raw_array_indices - 1 - index),
				   *raw_arrays_data_ptr + 4 /* components */ * index, sizeof(float) * 4 /* components */);
		}

		/* Followed by index data */
		for (int n = 0; n < n_raw_array_indices; ++n)
		{
			(*unordered_elements_data_ptr)[n] = (unsigned char)(n_raw_array_indices - 1 - n);
		}
	} /* for (both cases) */
}

/** Destructor. */
GeometryShaderRenderingTrianglesCase::~GeometryShaderRenderingTrianglesCase()
{
	if (m_raw_array_instanced_data != NULL)
	{
		delete[] m_raw_array_instanced_data;

		m_raw_array_instanced_data = NULL;
	}

	if (m_raw_array_noninstanced_data != NULL)
	{
		delete[] m_raw_array_noninstanced_data;

		m_raw_array_noninstanced_data = NULL;
	}

	if (m_unordered_array_instanced_data != NULL)
	{
		delete[] m_unordered_array_instanced_data;

		m_unordered_array_instanced_data = NULL;
	}

	if (m_unordered_array_noninstanced_data != NULL)
	{
		delete[] m_unordered_array_noninstanced_data;

		m_unordered_array_noninstanced_data = NULL;
	}

	if (m_unordered_elements_instanced_data != NULL)
	{
		delete[] m_unordered_elements_instanced_data;

		m_unordered_elements_instanced_data = NULL;
	}

	if (m_unordered_elements_noninstanced_data != NULL)
	{
		delete[] m_unordered_elements_noninstanced_data;

		m_unordered_elements_noninstanced_data = NULL;
	}
}

/** Retrieves amount of instances that should be drawn with glDraw*Elements() calls.
 *
 *  @return As per description.
 **/
unsigned int GeometryShaderRenderingTrianglesCase::getAmountOfDrawInstances()
{
	return 8;
}

/** Retrieves amount of indices that should be used for rendering a single instance
 *  (glDraw*Elements() calls only)
 *
 *  @return As per description.
 */
unsigned int GeometryShaderRenderingTrianglesCase::getAmountOfElementsPerInstance()
{
	/* No difference between instanced and non-instanced case. */
	return static_cast<unsigned int>(m_unordered_elements_instanced_data_size / sizeof(unsigned char));
}

/** Retrieves amount of vertices that should be used for rendering a single instance
 *  (glDrawArrays*() calls only)
 *
 *  @return As per description.
 **/
unsigned int GeometryShaderRenderingTrianglesCase::getAmountOfVerticesPerInstance()
{
	unsigned int result = 0;

	switch (m_drawcall_mode)
	{
	case GL_TRIANGLES:
	{
		result = 3 /* vertices making up a single triangle */ * 4 /* triangles */;

		break;
	}

	case GL_TRIANGLES_ADJACENCY_EXT:
	{
		result = 3 /* vertices making up a single triangle */ * 4 /* triangles */ * 2 /* adjacency data */;

		break;
	}

	case GL_TRIANGLE_FAN:
	{
		result = 6 /* vertices in total */;

		break;
	}

	case GL_TRIANGLE_STRIP:
	{
		result = 7 /* vertices in total */;

		break;
	}

	case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
	{
		/* As per extension specification */
		result = (5 /* triangles */ + 2) * 2;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized draw call mode");
	}
	} /* switch (m_drawcall_mode) */

	return result;
}

/** Draw call mode that should be used glDraw*() calls.
 *
 *  @return As per description.
 **/
glw::GLenum GeometryShaderRenderingTrianglesCase::getDrawCallMode()
{
	return m_drawcall_mode;
}

/** Source code for a fragment shader that should be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingTrianglesCase::getFragmentShaderCode()
{
	static std::string fs_code = "${VERSION}\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "in  vec4 gs_fs_color;\n"
								 "out vec4 result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    result = gs_fs_color;\n"
								 "}\n";

	return fs_code;
}

/** Source code for a geometry shader that should be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingTrianglesCase::getGeometryShaderCode()
{
	static const char* gs_triangles_in_lines_out_preamble = "${VERSION}\n"
															"\n"
															"${GEOMETRY_SHADER_ENABLE}\n"
															"\n"
															"layout(triangles)                   in;\n"
															"layout(line_strip, max_vertices=24) out;\n"
															"\n"
															"#define N_VERTICES (3)\n"
															"#define N_VERTEX0  (0)\n"
															"#define N_VERTEX1  (1)\n"
															"#define N_VERTEX2  (2)\n"
															"\n";

	static const char* gs_triangles_adjacency_in_lines_out_preamble = "${VERSION}\n"
																	  "\n"
																	  "${GEOMETRY_SHADER_ENABLE}\n"
																	  "\n"
																	  "layout(triangles_adjacency)         in;\n"
																	  "layout(line_strip, max_vertices=24) out;\n"
																	  "\n"
																	  "#define N_VERTICES (6)\n"
																	  "#define N_VERTEX0  (0)\n"
																	  "#define N_VERTEX1  (2)\n"
																	  "#define N_VERTEX2  (4)\n"
																	  "\n";

	static const char* gs_lines_code =
		"uniform ivec2 renderingTargetSize;\n"
		"\n"
		"in  vec4 vs_gs_color[N_VERTICES];\n"
		"out vec4 gs_fs_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    float dx = 2.0 / float(renderingTargetSize.x);\n"
		"    float dy = 2.0 / float(renderingTargetSize.y);\n"
		"\n"
		"    vec2 min_xy = gl_in[N_VERTEX0].gl_Position.xy;\n"
		"    vec2 max_xy = gl_in[N_VERTEX0].gl_Position.xy;\n"
		"\n"
		"    min_xy[0] = min(gl_in[N_VERTEX1].gl_Position.x, min_xy[0]);\n"
		"    min_xy[0] = min(gl_in[N_VERTEX2].gl_Position.x, min_xy[0]);\n"
		"    min_xy[1] = min(gl_in[N_VERTEX1].gl_Position.y, min_xy[1]);\n"
		"    min_xy[1] = min(gl_in[N_VERTEX2].gl_Position.y, min_xy[1]);\n"
		"\n"
		"    max_xy[0] = max(gl_in[N_VERTEX1].gl_Position.x, max_xy[0]);\n"
		"    max_xy[0] = max(gl_in[N_VERTEX2].gl_Position.x, max_xy[0]);\n"
		"    max_xy[1] = max(gl_in[N_VERTEX1].gl_Position.y, max_xy[1]);\n"
		"    max_xy[1] = max(gl_in[N_VERTEX2].gl_Position.y, max_xy[1]);\n"
		"\n"
		"    for (int n = 0; n < 3 /* segment \"sub\"-line */; ++n)\n"
		"    {\n"
		"        float hor_line_delta = 0.0;\n"
		"        float ver_line_delta = 0.0;\n"
		"\n"
		"        switch(n)\n"
		"        {\n"
		"            case 0: hor_line_delta = -dx; ver_line_delta = -dy; break;\n"
		"            case 1:                                             break;\n"
		"            case 2: hor_line_delta =  dx; ver_line_delta =  dy; break;\n"
		"        }\n"
		"\n"
		/* BL->TL */
		"        gl_Position = vec4(min_xy[0] - hor_line_delta, min_xy[1] - ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vs_gs_color[N_VERTEX0];\n"
		"        EmitVertex();\n"
		"        gl_Position = vec4(min_xy[0] - hor_line_delta, max_xy[1] + ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vs_gs_color[N_VERTEX0];\n"
		"        EmitVertex();\n"
		"        EndPrimitive();\n"
		/* TL->TR */
		"        gl_Position = vec4(max_xy[0] + hor_line_delta, min_xy[1] - ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vs_gs_color[N_VERTEX1];\n"
		"        EmitVertex();\n"
		"        gl_Position = vec4(min_xy[0] - hor_line_delta, min_xy[1] - ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vs_gs_color[N_VERTEX1];\n"
		"        EmitVertex();\n"
		"        EndPrimitive();\n"
		/* TR->BR */
		"        gl_Position = vec4(max_xy[0] + hor_line_delta, max_xy[1] + ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vs_gs_color[N_VERTEX2];\n"
		"        EmitVertex();\n"
		"        gl_Position = vec4(max_xy[0] + hor_line_delta, min_xy[1] - ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vs_gs_color[N_VERTEX2];\n"
		"        EmitVertex();\n"
		"        EndPrimitive();\n"
		/* BR->BL */
		"        gl_Position = vec4(min_xy[0] - hor_line_delta, max_xy[1] + ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vec4(vs_gs_color[N_VERTEX0].r, vs_gs_color[N_VERTEX1].g, vs_gs_color[N_VERTEX2].b, "
		"vs_gs_color[N_VERTEX0].a);\n"
		"        EmitVertex();\n"
		"        gl_Position = vec4(max_xy[0] + hor_line_delta, max_xy[1] + ver_line_delta, 0, 1);\n"
		"        gs_fs_color = vec4(vs_gs_color[N_VERTEX0].r, vs_gs_color[N_VERTEX1].g, vs_gs_color[N_VERTEX2].b, "
		"vs_gs_color[N_VERTEX0].a);\n"
		"        EmitVertex();\n"
		"        EndPrimitive();\n"
		"    }\n"
		"}\n";

	static const char* gs_triangles_in_points_out_preamble = "${VERSION}\n"
															 "\n"
															 "${GEOMETRY_SHADER_ENABLE}\n"
															 "\n"
															 "layout(triangles)               in;\n"
															 "layout(points, max_vertices=27) out;\n"
															 "\n"
															 "#define N_VERTICES (3)\n"
															 "#define N_VERTEX0  (0)\n"
															 "#define N_VERTEX1  (1)\n"
															 "#define N_VERTEX2  (2)\n"
															 "\n";

	static const char* gs_triangles_adjacency_in_points_out_preamble = "${VERSION}\n"
																	   "\n"
																	   "${GEOMETRY_SHADER_ENABLE}\n"
																	   "\n"
																	   "layout(triangles_adjacency)     in;\n"
																	   "layout(points, max_vertices=27) out;\n"
																	   "\n"
																	   "#define N_VERTICES (6)\n"
																	   "#define N_VERTEX0  (0)\n"
																	   "#define N_VERTEX1  (2)\n"
																	   "#define N_VERTEX2  (4)\n"
																	   "\n";

	static const char* gs_points_code =
		"uniform ivec2 renderingTargetSize;\n"
		"\n"
		"in  vec4 vs_gs_color[N_VERTICES];\n"
		"out vec4 gs_fs_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    float dx = 2.0 / float(renderingTargetSize.x);\n"
		"    float dy = 2.0 / float(renderingTargetSize.y);\n"
		"\n"
		"    for (int n = 0; n < 3 /* vertices */; ++n)\n"
		"    {\n"
		"        int vertex_index = (n == 0) ? N_VERTEX0 : \n"
		"                           (n == 1) ? N_VERTEX1 : \n"
		"                                      N_VERTEX2;\n"
		/* TL */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(-dx, -dy, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* TM */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(0, -dy, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* TR */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(dx, -dy, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* ML */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(-dx, 0, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* MM */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(0, 0, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* MR */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(dx, 0, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* BL */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(-dx, dy, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* BM */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(0, dy, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		/* BR */
		"        gl_Position = vec4(gl_in[vertex_index].gl_Position.xyz, 1) + vec4(dx, dy, 0, 0);\n"
		"        gs_fs_color = vs_gs_color[vertex_index];\n"
		"        EmitVertex();\n"
		"    }\n"
		"}\n";

	static const char* gs_triangles_in_triangles_out_preamble = "${VERSION}\n"
																"\n"
																"${GEOMETRY_SHADER_ENABLE}\n"
																"\n"
																"layout(triangles)                      in;\n"
																"layout(triangle_strip, max_vertices=6) out;\n"
																"\n"
																"#define N_VERTICES (3)\n"
																"#define N_VERTEX0  (0)\n"
																"#define N_VERTEX1  (1)\n"
																"#define N_VERTEX2  (2)\n"
																"\n";

	static const char* gs_triangles_adjacency_in_triangles_out_preamble =
		"${VERSION}\n"
		"\n"
		"${GEOMETRY_SHADER_ENABLE}\n"
		"\n"
		"layout(triangles_adjacency)            in;\n"
		"layout(triangle_strip, max_vertices=6) out;\n"
		"\n"
		"#define N_VERTICES (6)\n"
		"#define N_VERTEX0  (0)\n"
		"#define N_VERTEX1  (2)\n"
		"#define N_VERTEX2  (4)\n"
		"\n";

	static const char* gs_triangles_code = "uniform ivec2 renderingTargetSize;\n"
										   "\n"
										   "in  vec4 vs_gs_color[N_VERTICES];\n"
										   "out vec4 gs_fs_color;\n"
										   "\n"
										   "void main()\n"
										   "{\n"
										   "    float dx = 2.0 / float(renderingTargetSize.x);\n"
										   "    float dy = 2.0 / float(renderingTargetSize.y);\n"
										   "\n"
										   "    vec4 v0 = gl_in[N_VERTEX0].gl_Position;\n"
										   "    vec4 v1 = gl_in[N_VERTEX1].gl_Position;\n"
										   "    vec4 v2 = gl_in[N_VERTEX2].gl_Position;\n"
										   "    vec4 a  = v0;\n"
										   "    vec4 b;\n"
										   "    vec4 c;\n"
										   "    vec4 d;\n"
										   "\n"
										   /* Sort incoming vertices:
		 *
		 * a) a - vertex in origin.
		 * b) b - vertex located on the same height as a.
		 * c) c - remaining vertex.
		 */
										   "    if (abs(v1.w) >= 1.0)\n"
										   "    {\n"
										   "        a = v1;\n"
										   "    \n"
										   "        if (abs(v0.x - a.x) < dx)\n"
										   "        {\n"
										   "            b = v0;\n"
										   "            c = v2;\n"
										   "        }\n"
										   "        else\n"
										   "        {\n"
										   "            b = v2;\n"
										   "            c = v0;\n"
										   "        }\n"
										   "    }\n"
										   "    else\n"
										   "    if (abs(v2.w) >= 1.0)\n"
										   "    {\n"
										   "        a = v2;\n"
										   "    \n"
										   "        if (abs(v0.x - a.x) < dx)\n"
										   "        {\n"
										   "            b = v0;\n"
										   "            c = v1;\n"
										   "        }\n"
										   "        else\n"
										   "        {\n"
										   "            b = v1;\n"
										   "            c = v0;\n"
										   "        }\n"
										   "    }\n"
										   "    else\n"
										   "    {\n"
										   "        if (abs(v1.x - a.x) < dx)\n"
										   "        {\n"
										   "            b = v1;\n"
										   "            c = v2;\n"
										   "        }\n"
										   "        else\n"
										   "        {\n"
										   "            b = v2;\n"
										   "            c = v1;\n"
										   "        }\n"
										   "    }\n"
										   "    \n"
										   "    d = (b + c) * vec4(0.5);\n"
										   /* Now given the following configuration: (orientation does not matter)
		 *
		 * B
		 * |\
                                                * | D
		 * |  \
                                                * A---C
		 *
		 * emit ABD and ACD triangles */

										   /* First sub-triangle */
										   "    gl_Position = vec4(a.xyz, 1);\n"
										   "    gs_fs_color = vs_gs_color[N_VERTEX0];\n"
										   "    EmitVertex();\n"
										   "\n"
										   "    gl_Position = vec4(b.xyz, 1);\n"
										   "    gs_fs_color = vs_gs_color[N_VERTEX0];\n"
										   "    EmitVertex();\n"
										   "\n"
										   "    gl_Position = vec4(d.xyz, 1);\n"
										   "    gs_fs_color = vs_gs_color[N_VERTEX0];\n"
										   "    EmitVertex();\n"
										   "    EndPrimitive();\n"
										   /* Second sub-triangle */
										   "    gl_Position = vec4(a.xyz, 1);\n"
										   "    gs_fs_color = vec4(2.0, 1.0, 1.0, 1.0) * vs_gs_color[N_VERTEX2];\n"
										   "    EmitVertex();\n"
										   "\n"
										   "    gl_Position = vec4(c.xyz, 1);\n"
										   "    gs_fs_color = vec4(2.0, 1.0, 1.0, 1.0) * vs_gs_color[N_VERTEX2];\n"
										   "    EmitVertex();\n"
										   "\n"
										   "    gl_Position = vec4(d.xyz, 1);\n"
										   "    gs_fs_color = vec4(2.0, 1.0, 1.0, 1.0) * vs_gs_color[N_VERTEX2];\n"
										   "    EmitVertex();\n"
										   "    EndPrimitive();\n"
										   "}\n";

	std::string result;

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_POINTS:
	{
		std::stringstream gs_triangles_code_stringstream;
		std::string		  gs_triangles_code_string;
		std::stringstream gs_triangles_adjacency_code_stringstream;
		std::string		  gs_triangles_adjacency_code_string;

		switch (m_drawcall_mode)
		{
		case GL_TRIANGLE_FAN:
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLES:
		{
			gs_triangles_code_stringstream << gs_triangles_in_points_out_preamble << gs_points_code;

			gs_triangles_code_string = gs_triangles_code_stringstream.str();
			result					 = gs_triangles_code_string;

			break;
		}

		case GL_TRIANGLES_ADJACENCY_EXT:
		case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
		{
			gs_triangles_adjacency_code_stringstream << gs_triangles_adjacency_in_points_out_preamble << gs_points_code;

			gs_triangles_adjacency_code_string = gs_triangles_adjacency_code_stringstream.str();
			result							   = gs_triangles_adjacency_code_string;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized draw call mode");
		}
		}

		break;
	}

	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		std::stringstream gs_triangles_code_stringstream;
		std::string		  gs_triangles_code_string;
		std::stringstream gs_triangles_adjacency_code_stringstream;
		std::string		  gs_triangles_adjacency_code_string;

		switch (m_drawcall_mode)
		{
		case GL_TRIANGLE_FAN:
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLES:
		{
			gs_triangles_code_stringstream << gs_triangles_in_lines_out_preamble << gs_lines_code;

			gs_triangles_code_string = gs_triangles_code_stringstream.str();
			result					 = gs_triangles_code_string;

			break;
		}

		case GL_TRIANGLES_ADJACENCY_EXT:
		case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
		{
			gs_triangles_adjacency_code_stringstream << gs_triangles_adjacency_in_lines_out_preamble << gs_lines_code;

			gs_triangles_adjacency_code_string = gs_triangles_adjacency_code_stringstream.str();
			result							   = gs_triangles_adjacency_code_string;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized draw call mode");
		}
		}

		break;
	}

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		std::stringstream gs_triangles_code_stringstream;
		std::string		  gs_triangles_code_string;
		std::stringstream gs_triangles_adjacency_code_stringstream;
		std::string		  gs_triangles_adjacency_code_string;

		switch (m_drawcall_mode)
		{
		case GL_TRIANGLE_FAN:
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLES:
		{
			gs_triangles_code_stringstream << gs_triangles_in_triangles_out_preamble << gs_triangles_code;

			gs_triangles_code_string = gs_triangles_code_stringstream.str();
			result					 = gs_triangles_code_string;

			break;
		} /* case GL_TRIANGLES: */

		case GL_TRIANGLES_ADJACENCY_EXT:
		case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
		{
			gs_triangles_adjacency_code_stringstream << gs_triangles_adjacency_in_triangles_out_preamble
													 << gs_triangles_code;

			gs_triangles_adjacency_code_string = gs_triangles_adjacency_code_stringstream.str();
			result							   = gs_triangles_adjacency_code_string;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized draw call mode");
		}
		} /* switch (m_drawcall_mode) */

		break;
	} /* case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP: */

	default:
	{
		TCU_FAIL("Unsupported shader output type");
	}
	} /* switch (drawcall_mode) */

	return result;
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  vertex data to be used for glDrawArrays*() calls.
 *
 *  @param instanced True if the data is to be used in regard to instanced draw calls,
 *                   false otherwise.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingTrianglesCase::getRawArraysDataBufferSize(bool instanced)
{
	return instanced ? m_raw_array_instanced_data_size : m_raw_array_noninstanced_data_size;
}

/** Returns vertex data for the test. Only to be used for glDrawArrays*() calls.
 *
 *  @param instanced True if the data is to be used in regard to instanced draw calls,
 *                   false otherwise.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingTrianglesCase::getRawArraysDataBuffer(bool instanced)
{
	return instanced ? m_raw_array_instanced_data : m_raw_array_noninstanced_data;
}

/** Retrieves resolution of off-screen rendering buffer that should be used for the test.
 *
 *  @param n_instances Amount of draw call instances this render target will be used for.
 *  @param out_width   Deref will be used to store rendertarget's width. Must not be NULL.
 *  @param out_height  Deref will be used to store rendertarget's height. Must not be NULL.
 **/
void GeometryShaderRenderingTrianglesCase::getRenderTargetSize(unsigned int n_instances, unsigned int* out_width,
															   unsigned int* out_height)
{
	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	case SHADER_OUTPUT_TYPE_POINTS:
	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		*out_width  = 29;				/* as per test spec */
		*out_height = 29 * n_instances; /* as per test spec */

		break;
	}

	default:
	{
		TCU_FAIL("Unsupported output type");
	}
	}
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  vertex data to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingTrianglesCase::getUnorderedArraysDataBufferSize(bool instanced)
{
	return (instanced) ? m_unordered_array_instanced_data_size : m_unordered_array_noninstanced_data_size;
}

/** Returns vertex data for the test. Only to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingTrianglesCase::getUnorderedArraysDataBuffer(bool instanced)
{
	return instanced ? m_unordered_array_instanced_data : m_unordered_array_noninstanced_data;
}

/** Returns amount of bytes that should be allocated for a buffer object to hold
 *  index data to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 *  @return As per description.
 **/
glw::GLuint GeometryShaderRenderingTrianglesCase::getUnorderedElementsDataBufferSize(bool instanced)
{
	return instanced ? m_unordered_elements_instanced_data_size : m_unordered_elements_noninstanced_data_size;
}

/** Returns index data for the test. Only to be used for glDrawElements*() calls.
 *
 *  @param instanced true if the call is being made for an instanced draw call, false otherwise.
 *
 *  @return As per description.
 **/
const void* GeometryShaderRenderingTrianglesCase::getUnorderedElementsDataBuffer(bool instanced)
{
	return instanced ? m_unordered_elements_instanced_data : m_unordered_elements_noninstanced_data;
}

/** Returns type of the index, to be used for glDrawElements*() calls.
 *
 *  @return As per description.
 **/
glw::GLenum GeometryShaderRenderingTrianglesCase::getUnorderedElementsDataType()
{
	return GL_UNSIGNED_BYTE;
}

/** Retrieves maximum index value. To be used for glDrawRangeElements() test only.
 *
 *  @return As per description.
 **/
glw::GLubyte GeometryShaderRenderingTrianglesCase::getUnorderedElementsMaxIndex()
{
	return m_unordered_elements_max_index;
}

/** Retrieves minimum index value. To be used for glDrawRangeElements() test only.
 *
 *  @return As per description.
 **/
glw::GLubyte GeometryShaderRenderingTrianglesCase::getUnorderedElementsMinIndex()
{
	return m_unordered_elements_min_index;
}

/** Retrieves vertex shader code to be used for the test.
 *
 *  @return As per description.
 **/
std::string GeometryShaderRenderingTrianglesCase::getVertexShaderCode()
{
	static std::string vs_code =
		"${VERSION}\n"
		"\n"
		"in  vec4 position;\n"
		"out vec4 vs_gs_color;\n"
		"\n"
		"uniform bool  is_lines_output;\n"
		"uniform bool  is_indexed_draw_call;\n"
		"uniform bool  is_instanced_draw_call;\n"
		"uniform bool  is_points_output;\n"
		"uniform bool  is_triangle_fan_input;\n"
		"uniform bool  is_triangle_strip_input;\n"
		"uniform bool  is_triangle_strip_adjacency_input;\n"
		"uniform bool  is_triangles_adjacency_input;\n"
		"uniform bool  is_triangles_input;\n"
		"uniform bool  is_triangles_output;\n"
		"uniform ivec2 renderingTargetSize;\n"
		"uniform ivec2 singleRenderingTargetSize;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = position + vec4(float(gl_InstanceID) ) * vec4(0, float(singleRenderingTargetSize.y) / "
		"float(renderingTargetSize.y), 0, 0) * vec4(2.0);\n"
		"    vs_gs_color = vec4(1.0, 0.0, 0.0, 0.0);\n"
		"\n"
		/*********************************** LINES OUTPUT *******************************/
		"    if (is_lines_output)\n"
		"    {\n"
		"        if (!is_indexed_draw_call)\n"
		"        {\n"
		/* Non-indiced draw calls: */
		/*  GL_TRIANGLE_FAN draw call mode */
		"            if (is_triangle_fan_input)\n"
		"            {\n"
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                   case 0:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 1:\n"
		"                   case 5:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 2:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 4:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_fan_input) */
		"            else\n"
		/* GL_TRIANGLE_STRIP draw call mode */
		"            if (is_triangle_strip_input)\n"
		"            {\n"
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                   case 1:\n"
		"                   case 6: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 0:\n"
		"                   case 4:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 2:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 5:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_input) */
		"            else\n"
		/* GL_TRIANGLE_STRIP_ADJACENCY_EXT draw call mode */
		"            if (is_triangle_strip_adjacency_input)\n"
		"            {\n"
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                   case 2:\n"
		"                   case 12: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 0:\n"
		"                   case 8:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 4:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 6:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 10: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_adjacency_input) */
		"            else\n"
		"            if (is_triangles_input)\n"
		"            {\n"
		/* GL_TRIANGLES draw call mode */
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                   case 0: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 1: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 2: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 3: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 4: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 5: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 6: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 7: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 8: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 9:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 10: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 11: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangles_input) */
		"            else\n"
		"            if (is_triangles_adjacency_input)\n"
		"            {\n"
		/* GL_TRIANGLES_ADJACENCY_EXT draw call mode */
		"                vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
		"\n"
		"                switch(gl_VertexID)\n"
		"                {\n"
		/* TL triangle */
		"                    case 0: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 2: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                    case 4: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		/* TR triangle */
		"                    case 6: vs_gs_color  = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 8: vs_gs_color  = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                    case 10: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		/* BR triangle */
		"                    case 12: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 14: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                    case 16: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		/* BL triangle */
		"                    case 18: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 20: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                    case 22: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                }\n" /* switch (gl_VertexID) */
		"            }\n"	 /* if (is_triangles_adjacency_input) */
		"        }\n"		  /* if (!is_indexed_draw_call) */
		"        else\n"
		"        {\n"
		/* Indiced draw call: */
		"            if (is_triangles_input)\n"
		"            {\n"
		/* GL_TRIANGLES draw call mode */
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                    case 11: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 10: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                    case 9:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                    case 8:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 7:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                    case 6:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                    case 5:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 4:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                    case 3:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                    case 2:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 1:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                    case 0:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangles_input) */
		"            else\n"
		"            if (is_triangle_fan_input)\n"
		"            {\n"
		/* GL_TRIANGLE_FAN draw call mode */
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                   case 5:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 4:\n"
		"                   case 0:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 2:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 1:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /*( if (is_triangle_fan_input) */
		"            else\n"
		"            if (is_triangle_strip_input)\n"
		"            {\n"
		/* GL_TRIANGLE_STRIP draw call mode */
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                   case 5:\n"
		"                   case 0: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 6:\n"
		"                   case 2:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 4:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 1:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_input) */
		"            else\n"
		"            if (is_triangle_strip_adjacency_input)\n"
		"            {\n"
		/* GL_TRIANGLE_STRIP_ADJACENCY_EXT draw call mode */
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                   case 11:\n"
		"                   case 1:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 13:\n"
		"                   case 5:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 9:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   case 7:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_adjacency_input) */
		"            else\n"
		"            if (is_triangles_adjacency_input)\n"
		"            {\n"
		/* GL_TRIANGLES_ADJACENCY_EXT draw call mode */
		"                vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
		"\n"
		"                switch(gl_VertexID)\n"
		"                {\n"
		/* TL triangle */
		"                    case 23: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 21: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                    case 19: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		/* TR triangle */
		"                    case 17: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 15: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                    case 13: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		/* BR triangle */
		"                    case 11: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 9:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                    case 7:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		/* BL triangle */
		"                    case 5: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 3: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                    case 1: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangles_adjacency_input) */
		"        }\n"
		"    }\n" /* if (is_lines_output) */
		"    else\n"
		/*********************************** POINTS OUTPUT *******************************/
		"    if (is_points_output)\n"
		"    {\n"
		"        if (!is_indexed_draw_call)\n"
		"        {\n"
		/* Non-indiced draw calls */
		"            if (is_triangles_adjacency_input)\n"
		"            {\n"
		/* GL_TRIANGLES_ADJACENCY_EXT draw call mode */
		"                vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
		"\n"
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                    case 0:\n"
		"                    case 6:\n"
		"                    case 12:\n"
		"                    case 18: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                    case 2:\n"
		"                    case 22: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                    case 4:\n"
		"                    case 8: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 10:\n"
		"                    case 14: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                    case 16:\n"
		"                    case 20: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                }\n" /* switch (gl_VertexID) */
		"            }\n"	 /* if (is_triangles_adjacency_input) */
		"            else\n"
		"            if (is_triangle_fan_input)\n"
		"            {\n"
		/* GL_TRIANGLE_FAN draw call mode */
		"                switch(gl_VertexID)\n"
		"                {\n"
		"                   case 0:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 1:\n"
		"                   case 5:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 2:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 4:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_fan_input) */
		"            else\n"
		"            if (is_triangle_strip_input)\n"
		"            {\n"
		/* GL_TRIANGLE_STRIP draw call mode */
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                   case 1:\n"
		"                   case 4:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 0:\n"
		"                   case 6:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 2:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 5:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_input) */
		"            else\n"
		"            if (is_triangle_strip_adjacency_input)\n"
		"            {\n"
		/* GL_TRIANGLE_STRIP_ADJACENCY_EXT draw call mode */
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                   case 2:\n"
		"                   case 8:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 0:\n"
		"                   case 12: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 4:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 6:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 10: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_input) */
		"            else\n"
		"            if (is_triangles_input)\n"
		"            {\n"
		/* GL_TRIANGLES draw call mode */
		"                switch (gl_VertexID)\n"
		"                {\n"
		/* A */
		"                    case 0:\n"
		"                    case 3:\n"
		"                    case 6:\n"
		"                    case 9: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		/* B */
		"                    case 1:\n"
		"                    case 11: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		/* C */
		"                    case 2:\n"
		"                    case 4: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		/* D */
		"                    case 5:\n"
		"                    case 7: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		/* E */
		"                    case 8:\n"
		"                    case 10: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                }\n" /* switch (gl_VertexID) */
		"            }\n"	 /* if (is_triangles_input) */
		"        }\n"		  /* if (!is_indexed_draw_call) "*/
		"        else\n"
		"        {\n"
		/* Indiced draw calls: */
		"            if (is_triangle_fan_input)\n"
		"            {\n"
		/* GL_TRIANGLE_FAN input */
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                   case 5:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 4:\n"
		"                   case 0:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 2:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 1:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_fan_input) */
		"            else\n"
		"            if (is_triangle_strip_input)\n"
		"            {\n"
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                   case 5:\n"
		"                   case 2:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 6:\n"
		"                   case 0:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 4:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 1:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_input) */
		"            else\n"
		"            if (is_triangle_strip_adjacency_input)\n"
		"            {\n"
		/* GL_TRIANGLE_STRIP_ADJACENCY_EXT draw call mode */
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                   case 11:\n"
		"                   case 5:  vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                   case 13:\n"
		"                   case 1:  vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                   case 9:  vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                   case 7:  vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                   case 3:  vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                   default: vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0); break;\n"
		"                }\n" /* switch(gl_VertexID) */
		"            }\n"	 /* if (is_triangle_strip_adjacency_input) */
		"            else\n"
		"            if (is_triangles_adjacency_input)\n"
		"            {\n"
		"                vs_gs_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
		/* GL_TRIANGLES_ADJACENCY_EXT input */
		"                switch (gl_VertexID)\n"
		"                {\n"
		/* A */
		"                    case 23:\n"
		"                    case 17:\n"
		"                    case 11:\n"
		"                    case 5: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		/* B */
		"                    case 21:\n"
		"                    case 1: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		/* C */
		"                    case 19:\n"
		"                    case 15: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		/* D */
		"                    case 13:\n"
		"                    case 9: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		/* E */
		"                    case 7:\n"
		"                    case 3: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                }\n" /* switch (gl_VertexID) */
		"            }\n"	 /* if (is_triangles_adjacency_input) */
		"            else\n"
		"            if (is_triangles_input)\n"
		"            {\n"
		/* GL_TRIANGLES input */
		"                switch (gl_VertexID)\n"
		"                {\n"
		"                    case 11:\n"
		"                    case 8:\n"
		"                    case 5:\n"
		"                    case 2: vs_gs_color = vec4(0.4, 0.5, 0.6, 0.7); break;\n"
		"                    case 10:\n"
		"                    case 0: vs_gs_color = vec4(0.5, 0.6, 0.7, 0.8); break;\n"
		"                    case 9:\n"
		"                    case 7: vs_gs_color = vec4(0.1, 0.2, 0.3, 0.4); break;\n"
		"                    case 6:\n"
		"                    case 4: vs_gs_color = vec4(0.2, 0.3, 0.4, 0.5); break;\n"
		"                    case 3:\n"
		"                    case 1: vs_gs_color = vec4(0.3, 0.4, 0.5, 0.6); break;\n"
		"                }\n" /* switch (gl_VertexID) */
		"            }\n"	 /* if (is_triangles_input) */
		"        }\n"
		"    }\n" /* if (is_points_output) */
		"    else\n"
		/*********************************** TRIANGLES OUTPUT *******************************/
		"    if (is_triangles_output)\n"
		"    {\n"
		"        int vertex_id = 0;\n"
		"\n"
		"        if (!is_indexed_draw_call && is_triangles_adjacency_input && (gl_VertexID % 2 == 0) )\n"
		"        {\n"
		"            vertex_id = gl_VertexID / 2 + 1;\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            vertex_id = gl_VertexID + 1;\n"
		"        }\n"
		"\n"
		"        vs_gs_color = vec4(float(vertex_id) / 48.0, float(vertex_id % 3) / 2.0, float(vertex_id % 4) / 3.0, "
		"float(vertex_id % 5) / 4.0);\n"
		"    }\n"
		"}\n";
	std::string result;

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	case SHADER_OUTPUT_TYPE_POINTS:
	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		result = vs_code;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized output type");
	}
	} /* switch (m_output_type) */

	return result;
}

/** Sets test-specific uniforms for a program object that is then used for the draw call.
 *
 *  @param drawcall_type Type of the draw call that is to follow right after this function is called.
 **/
void GeometryShaderRenderingTrianglesCase::setUniformsBeforeDrawCall(_draw_call_type drawcall_type)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint is_lines_output_uniform_location			= gl.getUniformLocation(m_po_id, "is_lines_output");
	glw::GLint is_indexed_draw_call_uniform_location	= gl.getUniformLocation(m_po_id, "is_indexed_draw_call");
	glw::GLint is_instanced_draw_call_uniform_location  = gl.getUniformLocation(m_po_id, "is_instanced_draw_call");
	glw::GLint is_points_output_uniform_location		= gl.getUniformLocation(m_po_id, "is_points_output");
	glw::GLint is_triangle_fan_input_uniform_location   = gl.getUniformLocation(m_po_id, "is_triangle_fan_input");
	glw::GLint is_triangle_strip_input_uniform_location = gl.getUniformLocation(m_po_id, "is_triangle_strip_input");
	glw::GLint is_triangle_strip_adjacency_input_uniform_location =
		gl.getUniformLocation(m_po_id, "is_triangle_strip_adjacency_input");
	glw::GLint is_triangles_adjacency_input_uniform_location =
		gl.getUniformLocation(m_po_id, "is_triangles_adjacency_input");
	glw::GLint is_triangles_input_uniform_location  = gl.getUniformLocation(m_po_id, "is_triangles_input");
	glw::GLint is_triangles_output_uniform_location = gl.getUniformLocation(m_po_id, "is_triangles_output");

	gl.uniform1i(is_lines_output_uniform_location, m_output_type == SHADER_OUTPUT_TYPE_LINE_STRIP);
	gl.uniform1i(is_points_output_uniform_location, m_output_type == SHADER_OUTPUT_TYPE_POINTS);
	gl.uniform1i(is_triangle_fan_input_uniform_location, m_drawcall_mode == GL_TRIANGLE_FAN);
	gl.uniform1i(is_triangle_strip_input_uniform_location, m_drawcall_mode == GL_TRIANGLE_STRIP);
	gl.uniform1i(is_triangles_adjacency_input_uniform_location, m_drawcall_mode == GL_TRIANGLES_ADJACENCY_EXT);
	gl.uniform1i(is_triangle_strip_adjacency_input_uniform_location,
				 m_drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT);
	gl.uniform1i(is_triangles_input_uniform_location, m_drawcall_mode == GL_TRIANGLES);
	gl.uniform1i(is_triangles_output_uniform_location, m_output_type == SHADER_OUTPUT_TYPE_TRIANGLE_STRIP);
	gl.uniform1i(is_indexed_draw_call_uniform_location, (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS ||
														 drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED ||
														 drawcall_type == DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS));
	gl.uniform1i(is_instanced_draw_call_uniform_location,
				 (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED) ||
					 (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED));
}

/** Verifies that the rendered data is correct.
 *
 *  @param drawcall_type Type of the draw call that was used to render the geometry.
 *  @param instance_id   Instance ID to be verified. Use 0 for non-instanced draw calls.
 *  @param data          Contents of the rendertarget after the test has finished rendering.
 **/
void GeometryShaderRenderingTrianglesCase::verify(_draw_call_type drawcall_type, unsigned int instance_id,
												  const unsigned char* data)
{
	const float  epsilon		  = 1.0f / 256.0f;
	float		 dx				  = 0.0f;
	float		 dy				  = 0.0f;
	unsigned int single_rt_height = 0;
	unsigned int single_rt_width  = 0;

	getRenderTargetSize(1, &single_rt_width, &single_rt_height);

	dx = 2.0f / float(single_rt_width);
	dy = 2.0f / float(single_rt_height);

	switch (m_output_type)
	{
	case SHADER_OUTPUT_TYPE_LINE_STRIP:
	{
		/* Reference color data, taken directly from vertex shader. These are assigned
		 * to subsequent vertex ids, starting from 0.
		 */
		const float ref_color_data[] = { /* TL triangle */
										 0.1f, 0.2f, 0.3f, 0.4f, 0.2f, 0.3f, 0.4f, 0.5f, 0.3f, 0.4f, 0.5f, 0.6f,
										 /* TR triangle */
										 0.1f, 0.2f, 0.3f, 0.4f, 0.3f, 0.4f, 0.5f, 0.6f, 0.4f, 0.5f, 0.6f, 0.7f,
										 /* BR triangle */
										 0.1f, 0.2f, 0.3f, 0.4f, 0.4f, 0.5f, 0.6f, 0.7f, 0.5f, 0.6f, 0.7f, 0.8f,
										 /* BL triangle */
										 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.2f, 0.3f, 0.4f, 0.5f
		};

		/* Assuming single-instanced case, the test divides the screen-space into four adjacent quads.
		 * Internal edges will overlap. Given their different coloring, we exclude them from verification.
		 * This means we're only interested in checking the quad exteriors.
		 *
		 * GS takes a single triangle (ABC), calculates max & min XY of the BB and:
		 * - uses (A.r, A.g, A.b, A.a) for left line segment   (bottom left->top-left)     (line segment index in code:0)
		 * - uses (B.r, B.g, B.b, B.a) for top line segment    (top-left->top-right)       (line segment index in code:1)
		 * - uses (C.r, C.g, C.b, C.a) for right line segment  (top-right->bottom-right)   (line segment index in code:2)
		 * - uses (A.r, B.g, C.b, A.a) for bottom line segment (bottom-right->bottom-left) (line segment index in code:3)
		 *
		 * The test feeds input triangles arranged in the following order:
		 * 1) Top-left corner;     (quarter index in code:0);
		 * 2) Top-right corner;    (quarter index in code:1);
		 * 3) Bottom-right corner; (quarter index in code:2);
		 * 4) Bottom-left corner;  (quarter index in code:3);
		 *
		 * The test renders line segments of width 3 by rendering three lines of width 1 next to each other.
		 *
		 * Sample locations are precomputed - they are centers of line segments being considered.
		 *
		 * Expected color is computed on-the-fly in code, to ease investigation for driver developers,
		 * in case this test fails on any ES (or GL) implementation.
		 */
		int   ref_line_segment_index = -1;
		int   ref_quarter_index		 = -1;
		float ref_sample_rgba[4]	 = { 0, 0, 0, 0 };
		float ref_sample_xy_float[2] = { 0 };
		int   ref_sample_xy_int[2]   = { 0 };
		int   ref_triangle_index	 = -1;

		for (int n_edge = 0; n_edge < 8 /* edges */; ++n_edge)
		{
			switch (n_edge)
			{
			case 0:
			{
				/* Top left half, left edge */
				ref_line_segment_index = 0;
				ref_quarter_index	  = 0;
				ref_triangle_index	 = 0;

				break;
			}

			case 1:
			{
				/* Top-left half, top edge. */
				ref_line_segment_index = 1;
				ref_quarter_index	  = 0;
				ref_triangle_index	 = 0;

				break;
			}

			case 2:
			{
				/* Top-right half, top edge. */
				ref_line_segment_index = 1;
				ref_quarter_index	  = 1;
				ref_triangle_index	 = 1;

				break;
			}

			case 3:
			{
				/* Top-right half, right edge. */
				ref_line_segment_index = 2;
				ref_quarter_index	  = 1;
				ref_triangle_index	 = 1;

				break;
			}

			case 4:
			{
				/* Bottom-right half, right edge. */
				ref_line_segment_index = 2;
				ref_quarter_index	  = 2;
				ref_triangle_index	 = 2;

				break;
			}

			case 5:
			{
				/* Bottom-right half, bottom edge. */
				ref_line_segment_index = 3;
				ref_quarter_index	  = 2;
				ref_triangle_index	 = 2;

				break;
			}

			case 6:
			{
				/* Bottom-left half, bottom edge. */
				ref_line_segment_index = 3;
				ref_quarter_index	  = 3;
				ref_triangle_index	 = 3;

				break;
			}

			case 7:
			{
				/* Bottom-left half, left edge. */
				ref_line_segment_index = 0;
				ref_quarter_index	  = 3;
				ref_triangle_index	 = 3;

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized edge index");
			}
			} /* switch (n_edge) */

			/* Compute reference color.
			 *
			 * When drawing with GL_TRIANGLE_STRIP data or GL_TRIANGLE_STRIP_ADJACENCY_EXT, top-right triangle is drawn two times.
			 * During second re-draw, a different combination of vertex colors is used.
			 * Take this into account.
			 */
			switch (ref_line_segment_index)
			{
			case 0:
			{
				/* Left segment */
				memcpy(ref_sample_rgba,
					   ref_color_data + ref_triangle_index * 4 /* components */ * 3 /* reference colors */ +
						   0 /* first vertex data */,
					   sizeof(float) * 4 /* components */);

				if ((m_drawcall_mode == GL_TRIANGLE_STRIP || m_drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT) &&
					ref_triangle_index == 1 /* top-right triangle */)
				{
					/* First vertex */
					ref_sample_rgba[0] = 0.3f;
					ref_sample_rgba[1] = 0.4f;
					ref_sample_rgba[2] = 0.5f;
					ref_sample_rgba[3] = 0.6f;
				}

				break;
			}

			case 1:
			{
				/* Top segment */
				memcpy(ref_sample_rgba,
					   ref_color_data + ref_triangle_index * 4 /* components */ * 3 /* reference colors */ +
						   4 /* second vertex data */,
					   sizeof(float) * 4 /* components */);

				if ((m_drawcall_mode == GL_TRIANGLE_STRIP || m_drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT) &&
					ref_triangle_index == 1 /* top-right triangle */)
				{
					/* Second vertex */
					ref_sample_rgba[0] = 0.4f;
					ref_sample_rgba[1] = 0.5f;
					ref_sample_rgba[2] = 0.6f;
					ref_sample_rgba[3] = 0.7f;
				}
				break;
			}

			case 2:
			{
				/* Right segment */
				memcpy(ref_sample_rgba,
					   ref_color_data + ref_triangle_index * 4 /* components */ * 3 /* reference colors */ +
						   8 /* third vertex data */,
					   sizeof(float) * 4 /* components */);

				if ((m_drawcall_mode == GL_TRIANGLE_STRIP || m_drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT) &&
					ref_triangle_index == 1 /* top-right triangle */)
				{
					/* Third vertex */
					ref_sample_rgba[0] = 0.1f;
					ref_sample_rgba[1] = 0.2f;
					ref_sample_rgba[2] = 0.3f;
					ref_sample_rgba[3] = 0.4f;
				}

				break;
			}

			case 3:
			{
				/* Bottom segment */
				ref_sample_rgba[0] = ref_color_data[ref_triangle_index * 4 /* components */ * 3 /* reference colors */ +
													0 /* 1st vertex, red */];
				ref_sample_rgba[1] = ref_color_data[ref_triangle_index * 4 /* components */ * 3 /* reference colors */ +
													5 /* 2nd vertex, green */];
				ref_sample_rgba[2] = ref_color_data[ref_triangle_index * 4 /* components */ * 3 /* reference colors */ +
													10 /* 3rd vertex, blue */];
				ref_sample_rgba[3] = ref_color_data[ref_triangle_index * 4 /* components */ * 3 /* reference colors */ +
													3 /* 1st vertex, alpha */];

				if ((m_drawcall_mode == GL_TRIANGLE_STRIP || m_drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT) &&
					ref_triangle_index == 1 /* top-right triangle */)
				{
					/* Combination of components of three vertices */
					ref_sample_rgba[0] = 0.3f;
					ref_sample_rgba[1] = 0.5f;
					ref_sample_rgba[2] = 0.3f;
					ref_sample_rgba[3] = 0.6f;
				}

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized line segment index");
			}
			}

			/* Retrieve quad coordinates */
			float quad_x1y1x2y2[4] = { 0 };

			switch (ref_quarter_index)
			{
			case 0:
			{
				/* Top-left quarter */
				quad_x1y1x2y2[0] = 0.0f;
				quad_x1y1x2y2[1] = 0.0f;
				quad_x1y1x2y2[2] = 0.5f;
				quad_x1y1x2y2[3] = 0.5f;

				break;
			}

			case 1:
			{
				/* Top-right quarter */
				quad_x1y1x2y2[0] = 0.5f;
				quad_x1y1x2y2[1] = 0.0f;
				quad_x1y1x2y2[2] = 1.0f;
				quad_x1y1x2y2[3] = 0.5f;

				break;
			}

			case 2:
			{
				/* Bottom-right quarter */
				quad_x1y1x2y2[0] = 0.5f;
				quad_x1y1x2y2[1] = 0.5f;
				quad_x1y1x2y2[2] = 1.0f;
				quad_x1y1x2y2[3] = 1.0f;

				break;
			}

			case 3:
			{
				/* Bottom-left quarter */
				quad_x1y1x2y2[0] = 0.0f;
				quad_x1y1x2y2[1] = 0.5f;
				quad_x1y1x2y2[2] = 0.5f;
				quad_x1y1x2y2[3] = 1.0f;

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized quarter index");
			}
			} /* switch (ref_quarter_index) */

			/* Reduce quad coordinates to line segment coordinates */
			switch (ref_line_segment_index)
			{
			case 0:
			{
				/* Left segment */
				quad_x1y1x2y2[2] = quad_x1y1x2y2[0];

				break;
			}

			case 1:
			{
				/* Top segment */
				quad_x1y1x2y2[3] = quad_x1y1x2y2[1];

				break;
			}

			case 2:
			{
				/* Right segment */
				quad_x1y1x2y2[0] = quad_x1y1x2y2[2];

				break;
			}

			case 3:
			{
				/* Bottom segment */
				quad_x1y1x2y2[1] = quad_x1y1x2y2[3];

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized line segment index");
			}
			}

			/* Compute sample location. Move one pixel ahead in direction of the bottom-right corner to make
			 * sure we sample from the center of the line segment.
			 **/
			ref_sample_xy_float[0] = quad_x1y1x2y2[0] + (quad_x1y1x2y2[2] - quad_x1y1x2y2[0]) * 0.5f;
			ref_sample_xy_float[1] = quad_x1y1x2y2[1] + (quad_x1y1x2y2[3] - quad_x1y1x2y2[1]) * 0.5f;

			ref_sample_xy_int[0] = int(ref_sample_xy_float[0] * float(single_rt_width - 1));
			ref_sample_xy_int[1] = int(ref_sample_xy_float[1] * float(single_rt_height - 1));

			/* If this is n-th instance, offset sample locations so that they point to rendering results specific
			 * to instance of our concern */
			ref_sample_xy_int[1] += instance_id * single_rt_height;

			/* Compare the rendered data with reference data */
			const int			 pixel_size = 4 /* components */;
			const int			 row_width  = single_rt_width * pixel_size;
			const unsigned char* rendered_data_ptr =
				data + row_width * ref_sample_xy_int[1] + ref_sample_xy_int[0] * pixel_size;
			const float rendered_data[4] = { float(rendered_data_ptr[0]) / 255.0f, float(rendered_data_ptr[1]) / 255.0f,
											 float(rendered_data_ptr[2]) / 255.0f,
											 float(rendered_data_ptr[3]) / 255.0f };

			if (de::abs(rendered_data[0] - ref_sample_rgba[0]) > epsilon ||
				de::abs(rendered_data[1] - ref_sample_rgba[1]) > epsilon ||
				de::abs(rendered_data[2] - ref_sample_rgba[2]) > epsilon ||
				de::abs(rendered_data[3] - ref_sample_rgba[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data at (" << ref_sample_xy_int[0] << ", "
								   << ref_sample_xy_int[1] << ") "
								   << "equal (" << rendered_data[0] << ", " << rendered_data[1] << ", "
								   << rendered_data[2] << ", " << rendered_data[3] << ") "
								   << "exceeds allowed epsilon when compared to reference data equal ("
								   << ref_sample_rgba[0] << ", " << ref_sample_rgba[1] << ", " << ref_sample_rgba[2]
								   << ", " << ref_sample_rgba[3] << ")." << tcu::TestLog::EndMessage;

				TCU_FAIL("Data comparison failed");
			} /* if (data comparison failed) */
		}	 /* for (all edges) */

		/* All done */
		break;
	}

	case SHADER_OUTPUT_TYPE_POINTS:
	{
		/* In this case, the test should have rendered 6 large points.
		 * Verification checks if centers of these blobs carry valid values.
		 */
		const float ref_data[] = {
			/* Position (<-1, 1> x <-1, 1>) */
			0.0f,		-1.0f + float(instance_id) * 2.0f + dy, /* top */
			0.1f,		0.2f,
			0.3f,		0.4f, /* rgba */

			0.0f,		0.0f + float(instance_id) * 2.0f, /* middle */
			0.4f,		0.5f,
			0.6f,		0.7f, /* rgba */

			-1.0f + dx, 0.0f + float(instance_id) * 2.0f, /* left */
			0.5f,		0.6f,
			0.7f,		0.8f, /* rgba */

			1.0f - dx,  0.0f + float(instance_id) * 2.0f, /* right */
			0.2f,		0.3f,
			0.4f,		0.5f, /* rgba */

			0.0f,		1.0f + float(instance_id) * 2.0f - dy, /* bottom */
			0.3f,		0.4f,
			0.5f,		0.6f, /* rgba */
		};
		const unsigned int n_points = 5; /* total number of points to check */

		for (unsigned int n_point = 0; n_point < n_points; ++n_point)
		{
			/* Retrieve data from the array */
			tcu::Vec4 color_float = tcu::Vec4(ref_data[n_point * 6 + 2], ref_data[n_point * 6 + 3],
											  ref_data[n_point * 6 + 4], ref_data[n_point * 6 + 5]);
			tcu::Vec2 position_float = tcu::Vec2(ref_data[n_point * 6 + 0], ref_data[n_point * 6 + 1]);

			/* Convert position data to texture space */
			int position[] = { int((position_float[0] * 0.5f + 0.5f) * float(single_rt_width)),
							   int((position_float[1] * 0.5f + 0.5f) * float(single_rt_height)) };

			/* Compare the data */
			const int			 pixel_size	= 4 /* components */;
			const int			 row_width	 = single_rt_width * pixel_size;
			const unsigned char* rendered_data = data + row_width * position[1] + position[0] * pixel_size;
			float rendered_data_float[]		   = { float(rendered_data[0]) / 255.0f, float(rendered_data[1]) / 255.0f,
											float(rendered_data[2]) / 255.0f, float(rendered_data[3]) / 255.0f };

			if (de::abs(rendered_data_float[0] - color_float[0]) > epsilon ||
				de::abs(rendered_data_float[1] - color_float[1]) > epsilon ||
				de::abs(rendered_data_float[2] - color_float[2]) > epsilon ||
				de::abs(rendered_data_float[3] - color_float[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data at (" << position[0] << ", "
								   << position[1] << ") "
								   << "equal (" << rendered_data_float[0] << ", " << rendered_data_float[1] << ", "
								   << rendered_data_float[2] << ", " << rendered_data_float[3] << ") "
								   << "exceeds allowed epsilon when compared to reference data equal ("
								   << color_float[0] << ", " << color_float[1] << ", " << color_float[2] << ", "
								   << color_float[3] << ")." << tcu::TestLog::EndMessage;

				TCU_FAIL("Data comparison failed");
			}
		} /* for (all points) */

		break;
	}

	case SHADER_OUTPUT_TYPE_TRIANGLE_STRIP:
	{
		/* The test feeds the rendering pipeline with 4 triangles (per instance), which should
		 * be converted to 8 differently colored triangles at the geometry shader stage.
		 */
		for (unsigned int n_triangle = 0; n_triangle < 8 /* triangles */; ++n_triangle)
		{
			/* Retrieve base triangle-specific properties */
			tcu::Vec2 base_triangle_v1;
			tcu::Vec2 base_triangle_v2;
			tcu::Vec2 base_triangle_v3;

			switch (n_triangle / 2)
			/* two sub-triangles per a triangle must be checked */
			{
			case 0:
			{
				/* Top-left triangle */
				base_triangle_v1[0] = 0.5f;
				base_triangle_v1[1] = 0.5f;
				base_triangle_v2[0] = 0.0f;
				base_triangle_v2[1] = 0.5f;
				base_triangle_v3[0] = 0.5f;
				base_triangle_v3[1] = 0.0f;

				break;
			}

			case 1:
			{
				/* Top-right triangle */
				base_triangle_v1[0] = 0.5f;
				base_triangle_v1[1] = 0.5f;
				base_triangle_v2[0] = 0.5f;
				base_triangle_v2[1] = 0.0f;
				base_triangle_v3[0] = 1.0f;
				base_triangle_v3[1] = 0.5f;

				break;
			}

			case 2:
			{
				/* Bottom-right triangle */
				base_triangle_v1[0] = 0.5f;
				base_triangle_v1[1] = 0.5f;
				base_triangle_v2[0] = 1.0f;
				base_triangle_v2[1] = 0.5f;
				base_triangle_v3[0] = 0.5f;
				base_triangle_v3[1] = 1.0f;

				break;
			}

			case 3:
			{
				/* Bottom-left triangle */
				base_triangle_v1[0] = 0.5f;
				base_triangle_v1[1] = 0.5f;
				base_triangle_v2[0] = 0.0f;
				base_triangle_v2[1] = 0.5f;
				base_triangle_v3[0] = 0.5f;
				base_triangle_v3[1] = 1.0f;

				break;
			}
			} /* switch (n_triangle) */

			/* Compute coordinates of the iteration-specific triangle. This logic has been
			 * ported directly from the geometry shader */
			tcu::Vec4   expected_sample_color_rgba;
			glw::GLuint expected_sample_color_vertex_id = 0;
			tcu::Vec2   triangle_vertex1;
			tcu::Vec2   triangle_vertex2;
			tcu::Vec2   triangle_vertex3;
			tcu::Vec2   a = base_triangle_v1;
			tcu::Vec2   b;
			tcu::Vec2   c;
			tcu::Vec2   d;

			if (base_triangle_v2.x() == 0.5f && base_triangle_v2.y() == 0.5f)
			{
				a = base_triangle_v2;

				if (de::abs(base_triangle_v1.x() - a.x()) < dx * 0.5f)
				{
					b = base_triangle_v1;
					c = base_triangle_v3;
				}
				else
				{
					b = base_triangle_v3;
					c = base_triangle_v1;
				}
			}
			else if (base_triangle_v3.x() == 0.5f && base_triangle_v3.y() == 0.5f)
			{
				a = base_triangle_v3;

				if (de::abs(base_triangle_v1.x() - a.x()) < dx * 0.5f)
				{
					b = base_triangle_v1;
					c = base_triangle_v2;
				}
				else
				{
					b = base_triangle_v2;
					c = base_triangle_v1;
				}
			}
			else
			{
				if (de::abs(base_triangle_v2.x() - a.x()) < dx * 0.5f)
				{
					b = base_triangle_v2;
					c = base_triangle_v3;
				}
				else
				{
					b = base_triangle_v3;
					c = base_triangle_v2;
				}
			}

			d = (b + c) * tcu::Vec2(0.5f);

			if (n_triangle % 2 == 0)
			{
				switch (m_drawcall_mode)
				{
				case GL_TRIANGLES:
				case GL_TRIANGLES_ADJACENCY_EXT:
				{
					/* First sub-triangle */
					expected_sample_color_vertex_id = 3 /* vertices per triangle */ * (n_triangle / 2) + 0;

					break;
				} /* GL_TRIANGLEs or GL_TRIANGLES_ADJACENCY_EXT cases */

				case GL_TRIANGLE_FAN:
				{
					/* First sub-triangle in this case is always assigned vertex id of 0, as 0 stands for
					 * the hub vertex */
					expected_sample_color_vertex_id = 0;

					break;
				}

				case GL_TRIANGLE_STRIP:
				case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
				{
					bool is_adjacency_data_present = (m_drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT);

					/* These vertex ids correspond to index of vertex A which changes between triangle
					 * orientations, hence the repeated expected vertex ID for cases 4 and 6 */
					switch (n_triangle)
					{
					/* Top-left triangle, first sub-triangle */
					case 0:
					{
						expected_sample_color_vertex_id = 0;

						break;
					}

					/* Top-right triangle, first sub-triangle */
					case 2:
					{
						expected_sample_color_vertex_id = 2 * (is_adjacency_data_present ? 2 : 1);

						break;
					}

					/* Bottom-right triangle, first sub-triangle */
					case 4:
					{
						expected_sample_color_vertex_id = 4 * (is_adjacency_data_present ? 2 : 1);

						break;
					}

					/* Bottom-left triangle, first sub-triangle */
					case 6:
					{
						expected_sample_color_vertex_id = 4 * (is_adjacency_data_present ? 2 : 1);

						break;
					}

					default:
					{
						TCU_FAIL("Unrecognized triangle index");
					}
					}

					break;
				}

				default:
				{
					TCU_FAIL("Unrecognized draw call mode");
				}
				} /* switch (m_drawcall_mode) */

				triangle_vertex1 = a;
				triangle_vertex2 = b;
				triangle_vertex3 = d;
			} /* if (n_triangle % 2 == 0) */
			else
			{
				/* Second sub-triangle */
				switch (m_drawcall_mode)
				{
				case GL_TRIANGLES:
				case GL_TRIANGLES_ADJACENCY_EXT:
				{
					expected_sample_color_vertex_id = 3 /* vertices per triangle */ * (n_triangle / 2) + 2;

					break;
				}

				case GL_TRIANGLE_FAN:
				{
					expected_sample_color_vertex_id = 2 + n_triangle / 2;

					break;
				}

				case GL_TRIANGLE_STRIP:
				case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
				{
					bool is_adjacency_data_present = (m_drawcall_mode == GL_TRIANGLE_STRIP_ADJACENCY_EXT);

					switch (n_triangle)
					{
					/* Top-left triangle, second sub-triangle */
					case 1:
					{
						expected_sample_color_vertex_id = 2 * (is_adjacency_data_present ? 2 : 1);

						break;
					}

					/* Top-right triangle, second sub-triangle */
					case 3:
					{
						expected_sample_color_vertex_id = 4 * (is_adjacency_data_present ? 2 : 1);

						break;
					}

					/* Bottom-right triangle, second sub-triangle */
					case 5:
					{
						expected_sample_color_vertex_id = 5 * (is_adjacency_data_present ? 2 : 1);

						break;
					}

					/* Bottom-left triangle, second sub-triangle */
					case 7:
					{
						expected_sample_color_vertex_id = 6 * (is_adjacency_data_present ? 2 : 1);

						break;
					}

					default:
					{
						TCU_FAIL("Unrecognized triangle index");
					}
					}

					break;
				}

				default:
				{
					TCU_FAIL("UNrecognized draw call mode");
				}
				} /* switch (m_drawcall_mode) */

				triangle_vertex1 = a;
				triangle_vertex2 = c;
				triangle_vertex3 = d;
			}

			if (drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS ||
				drawcall_type == DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED ||
				drawcall_type == DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS)
			{
				switch (m_drawcall_mode)
				{
				case GL_TRIANGLE_FAN:
				case GL_TRIANGLE_STRIP:
				case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
				case GL_TRIANGLES:
				{
					expected_sample_color_vertex_id =
						(getAmountOfVerticesPerInstance() - 1) - expected_sample_color_vertex_id;

					break;
				}

				case GL_TRIANGLES_ADJACENCY_EXT:
				{
					/* In adjacency input, every even vertex is used for rendering. */
					expected_sample_color_vertex_id =
						(getAmountOfVerticesPerInstance() - 1) - expected_sample_color_vertex_id * 2;

					break;
				}

				default:
				{
					TCU_FAIL("Unrecognized draw call mode");
				}
				}
			}

			/* Compute sample's reference color - logic as in vertex shader */
			expected_sample_color_rgba[0] = float((expected_sample_color_vertex_id + 1)) / 48.0f;
			expected_sample_color_rgba[1] = float((expected_sample_color_vertex_id + 1) % 3) / 2.0f;
			expected_sample_color_rgba[2] = float((expected_sample_color_vertex_id + 1) % 4) / 3.0f;
			expected_sample_color_rgba[3] = float((expected_sample_color_vertex_id + 1) % 5) / 4.0f;

			if (n_triangle % 2 == 1)
			{
				expected_sample_color_rgba[0] *= 2.0f;
			}

			/* Compute sample location */
			tcu::Vec2 sample_location;
			int		  sample_location_int[2]; /* X & Y coordinates */

			sample_location		   = (triangle_vertex1 + triangle_vertex2 + triangle_vertex3) / 3.0f;
			sample_location_int[0] = int(sample_location[0] * static_cast<float>(single_rt_width - 1) + 0.5f);
			sample_location_int[1] = int(sample_location[1] * static_cast<float>(single_rt_height - 1) + 0.5f) +
									 instance_id * single_rt_height;

			/* Retrieve rendered data */
			const unsigned int   pixel_size = 4 /* components */;
			const unsigned int   row_width  = single_rt_width * pixel_size;
			const unsigned char* rendered_data =
				data + row_width * sample_location_int[1] + sample_location_int[0] * pixel_size;

			tcu::Vec4 rendered_data_rgba =
				tcu::Vec4(float(rendered_data[0]) / 255.0f, float(rendered_data[1]) / 255.0f,
						  float(rendered_data[2]) / 255.0f, float(rendered_data[3]) / 255.0f);

			/* Compare rendered data with reference color information */
			if (de::abs(rendered_data_rgba[0] - expected_sample_color_rgba[0]) > epsilon ||
				de::abs(rendered_data_rgba[1] - expected_sample_color_rgba[1]) > epsilon ||
				de::abs(rendered_data_rgba[2] - expected_sample_color_rgba[2]) > epsilon ||
				de::abs(rendered_data_rgba[3] - expected_sample_color_rgba[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data at (" << sample_location[0] << ", "
								   << sample_location[1] << ") "
								   << "equal (" << rendered_data_rgba[0] << ", " << rendered_data_rgba[1] << ", "
								   << rendered_data_rgba[2] << ", " << rendered_data_rgba[3] << ") "
								   << "exceeds allowed epsilon when compared to reference data equal ("
								   << expected_sample_color_rgba[0] << ", " << expected_sample_color_rgba[1] << ", "
								   << expected_sample_color_rgba[2] << ", " << expected_sample_color_rgba[3] << ")."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Data comparison failed");
			} /* if (data comparison failed) */
		}	 /* for (all triangles) */
		/* All done */
		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized output type");
	}
	}
}

} // namespace glcts
