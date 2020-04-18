#ifndef _ESEXTCGEOMETRYSHADERLAYEREDRENDERINGBOUNDARYCONDITION_HPP
#define _ESEXTCGEOMETRYSHADERLAYEREDRENDERINGBOUNDARYCONDITION_HPP
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
#include <vector>

namespace glcts
{
/** Implementation of "Group 10" from CTS_EXT_geometry_shader. */
class GeometryShaderLayeredRenderingBoundaryCondition : public TestCaseBase
{
public:
	/* Public methods */
	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected data types */
	struct TextureInfo
	{
		glw::GLint  m_depth;
		glw::GLenum m_draw_buffer;
		glw::GLuint m_id;
		glw::GLenum m_texture_target;
	};

	/* Protected methods */
	GeometryShaderLayeredRenderingBoundaryCondition(Context& context, const ExtParameters& extParams, const char* name,
													const char* description);

	virtual ~GeometryShaderLayeredRenderingBoundaryCondition(void);

	virtual void		initTest(void);
	virtual const char* getFragmentShaderCode();
	virtual const char* getGeometryShaderCode();
	virtual const char* getVertexShaderCode();

	virtual bool comparePixels(glw::GLint width, glw::GLint height, glw::GLint pixelSize,
							   const unsigned char* textureData, const unsigned char* referencePixel, int att,
							   int layer);

	virtual void getReferenceColor(glw::GLint layerIndex, unsigned char* colorBuffer,
								   int colorBufferSize = m_texture_components) = 0;

protected:
	/* Protected variables */
	glw::GLenum				 m_draw_mode;
	glw::GLuint				 m_n_points;
	bool					 m_is_fbo_layered;
	std::vector<TextureInfo> m_textures_info;
	unsigned char*			 m_blue_color;
	unsigned char*			 m_green_color;
	unsigned char*			 m_red_color;
	unsigned char*			 m_white_color;

private:
	/* Private constants */
	static const glw::GLint m_height;
	static const glw::GLint m_max_depth;
	static const glw::GLint m_texture_components;
	static const glw::GLint m_width;

	/* Private variables */
	glw::GLuint m_fbo_draw_id;
	glw::GLuint m_fbo_read_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/**
 *    1. Only the smallest number of layers of layered framebuffer's attachments
 *       should be affected by draw calls, if the number of layers of each
 *       attachment is not identical.
 *
 *       Category: API;
 *                 Functional Test.
 *
 *       Consider a layered FBO with the following configuration:
 *
 *       - A 3D texture object A (resolution: 4x4x2), whose base level was bound
 *         to color attachment 0;
 *       - A 3D texture object B (resolution: 4x4x4), whose base level was
 *         bound to color attachment 1;
 *
 *       All slices/layers of these texture objects are filled with black color.
 *
 *       A geometry shader should accept points on input and output triangle
 *       strips with a maximum of 4*16 vertices. The shader should emit 4
 *       "screen-space quads" (as described in test case 8.1, for instance),
 *       with each quad occupying layers at subsequently incremental indices.
 *       Each quad should have a different color (shared by all quad's vertices).
 *
 *       The test should make a draw call for a single point and then check the
 *       texture objects. Test passes if only the first two layers of the texture
 *       objects were modified.
 */
class GeometryShaderLayeredRenderingBoundaryConditionVariousTextures
	: public GeometryShaderLayeredRenderingBoundaryCondition
{
public:
	/* Public methods */
	GeometryShaderLayeredRenderingBoundaryConditionVariousTextures(Context& context, const ExtParameters& extParams,
																   const char* name, const char* description);

	virtual ~GeometryShaderLayeredRenderingBoundaryConditionVariousTextures(void)
	{
	}

protected:
	/* Protected methods */
	const char* getFragmentShaderCode();
	const char* getGeometryShaderCode();
	void getReferenceColor(glw::GLint layerIndex, unsigned char* colorBuffer, int colorBufferSize);
};

/**
 *    2. Default layer number for fragments is zero if there is no geometry shader
 *       defined for the active pipeline.
 *
 *       Category: API;
 *                 Functional Test.
 *
 *       Consider a layered FBO with the following configuration:
 *
 *       - Base level of a 3D texture object A of resolution 4x4x4 attached to
 *         color attachment 0.
 *
 *       All slices of texture object A should be filled with (0, 0, 0, 0).
 *
 *       A program object A should consist of vertex shader B and fragment shader
 *       C. Vertex shader B should:
 *
 *       - Set gl_Position to (-1, -1, 0, 1) if gl_VertexID == 0;
 *       - Set gl_Position to (-1,  1, 0, 1) if gl_VertexID == 1;
 *       - Set gl_Position to ( 1,  1, 0, 1) if gl_VertexID == 2;
 *       - Set gl_Position to ( 1, -1, 0, 1) otherwise.
 *
 *       Fragment shader C should set the only output vec4 variable to
 *       (1, 1, 1, 1).
 *
 *       Having bound the layered FBO to GL_FRAMEBUFFER targets, the test should
 *       draw a triangle fan made of 4 vertices.It should then read back first
 *       two slices of the texture object A:
 *
 *       - All texels of the first slice should be set to (255, 255, 255, 255)
 *         (assuming glReadPixels() was called with GL_RGBA format &
 *         GL_UNSIGNED_BYTE type).
 *       - All texels of the second slice should be set to (0, 0, 0, 0)
 *         (assumption as above).
 */
class GeometryShaderLayeredRenderingBoundaryConditionNoGS : public GeometryShaderLayeredRenderingBoundaryCondition
{
public:
	/* Public methods */
	GeometryShaderLayeredRenderingBoundaryConditionNoGS(Context& context, const ExtParameters& extParams,
														const char* name, const char* description);

	virtual ~GeometryShaderLayeredRenderingBoundaryConditionNoGS(void)
	{
	}

protected:
	/* Protected methods */
	const char* getVertexShaderCode();
	void getReferenceColor(glw::GLint layerIndex, unsigned char* colorBuffer, int colorBufferSize);
};

/**
 *    3. Default layer number for fragments is zero if the geometry shader used
 *       for current pipeline does not set gl_Layer in any of the execution paths.
 *
 *       Category: API;
 *                 Functional Test.
 *
 *       Modify test case 10.2 to introduce a boilerplate geometry shader that:
 *
 *       - generates a triangle strip, takes the same input and emits the same
 *         output primitive types as in test case 8.1;
 *       - does NOT set layer id for any of the outputted vertices.
 *
 *       Vertex shader can be boilerplate in this case.
 *
 *       The test should draw a single point and perform the same check as in test
 *       case 10.2.
 */
class GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet : public GeometryShaderLayeredRenderingBoundaryCondition
{
public:
	/* Public methods */
	GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet(Context& context, const ExtParameters& extParams,
															  const char* name, const char* description);

	virtual ~GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet(void)
	{
	}

protected:
	/* Protected methods */
	const char* getGeometryShaderCode();
	void getReferenceColor(glw::GLint layerIndex, unsigned char* colorBuffer, int colorBufferSize);
};

/**    4. Layer number specified by geometry shader is ignored if current draw
 *       FBO is not layered.
 *
 *       Category: API;
 *                 Functional Test.
 *
 *       Consider a framebuffer object A with the following configuration:
 *
 *       - 3D texture object A of resolution 4x4x4, with all slices filled with
 *         (0, 0, 0, 0), is bound to color attachment 0;
 *
 *       Vertex shader can be boilerplate.
 *       Geometry shader should take form of the geometry shader described in test
 *       case 10.3. However, it should set gl_Layer to 1 for all vertices emitted.
 *       Fragment shader should take form of the vertex shader C described in test
 *       case 10.2.
 *
 *       The test should then draw a single point, read first two slices of texture
 *       object A and:
 *
 *       - make sure first slice is filled with (255, 255, 255, 255), assuming
 *         glReadPixels() call with GL_RGBA format and GL_UNSIGNED_BYTE type.
 *       - make sure second slice is filled with (0, 0, 0, 0) (assumptions as
 *         above)
 *
 */
class GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO
	: public GeometryShaderLayeredRenderingBoundaryCondition
{
public:
	/* Public methods */
	GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO(Context& context, const ExtParameters& extParams,
																const char* name, const char* description);

	virtual ~GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO(void)
	{
	}

protected:
	/* Protected methods */
	virtual const char* getGeometryShaderCode();
	virtual void getReferenceColor(glw::GLint layerIndex, unsigned char* colorBuffer, int colorBufferSize);
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERLAYEREDRENDERINGBOUNDARYCONDITION_HPP
