#ifndef _ESEXTCGEOMETRYSHADERLAYEREDRENDERINGFBONOATTACHMENT_HPP
#define _ESEXTCGEOMETRYSHADERLAYEREDRENDERINGFBONOATTACHMENT_HPP
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
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

namespace glcts
{
/** Implementation of "Group 14" from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Make sure that layered rendering works correctly when using a framebuffer
 *     with no attachments, but for which default height & width & non - zero
 *     number of layers have been defined.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Create a program object, as well as a fragment, geometry and vertex
 *     shader objects.
 *
 *     Vertex shader can be boilerplate.
 *
 *     Geometry shader should :
 *
 *     - accept points on input;
 *     - output triangle strips (a maximum of(4 vertices * 4 layers) vertices
 *       will be stored);
 *     - generate "full-screen" quads (as per definition in test case 7.1) for
 *       layers 0, 1, 2, and 3.
 *     - define output uv variable and set it to valid UV coordinates for each
 *       of quad's vertices.
 *     - define output layer_id variable and set it to gl_Layer for each
 *       of quad's vertices.
 *
 *     Fragment shader should :
 *
 *     - declare an image2DArray called array_image.
 *     - using array_image store color (0, 255, 0, 0) at texel coordinates
 *       ivec3( int(128.0 * uv.x), int(128.0 * uv.y), layer_id )
 *
 *     Shader objects should be compiled and attached to the program object.
 *     The program object should then be linked.
 *
 *     2d array texture object having GL_RGBA32I internal format and
 *     resolution 128x128x4 should be created and initialized. All texels of base
 *     mip - maps of the texture should be filled with (255, 0, 0, 0).
 *
 *     The texture object should be bound to image unit 0 and array_image uniform
 *     should have binding set to 0 as well.
 *
 *     Generate a framebuffer object. Bind the FBO to GL_FRAMEBUFFER target and
 *     set its :
 *
 *     - default width & height to 128x128;
 *     - default number of layers to 4;
 *
 *     Generate and bind a vertex array object. Test should now draw
 *     a single point.
 *
 *     After rendering finishes, the test should check if all texels of base
 *     mip - maps of all layers of 2d array texture object have been set to
 *     (0, 255, 0, 0). If so, the test has passed.
 *
 *
 *  2. Make sure that, for a newly generated framebuffer object, default layer
 *     count of a framebuffer object without attachments should be equal to 0.
 *     Once modified, an updated value should be reported for the property by
 *     GLES implementation.
 *
 *     Category: API;
 *               Coverage;
 *
 *     Test should generate a FBO, bind it to GL_DRAW_FRAMEBUFFER target, and do
 *     a glGetFramebufferParameteriv() call to determine value of
 *     GL_FRAMEBUFFER_DEFAULT_LAYERS_EXT property. No error should be generated,
 *     the value reported should be equal to 0.
 *
 *     Test should now modify the property's value by doing
 *     a glFramebufferParameteri() call. It should then use the
 *     glGetFramebufferParameteriv() call again to make sure the value has been
 *     updated.
 **/
class GeometryShaderLayeredRenderingFBONoAttachment : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderLayeredRenderingFBONoAttachment(Context& context, const ExtParameters& extParams, const char* name,
												  const char* description);

	virtual ~GeometryShaderLayeredRenderingFBONoAttachment(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private variables */
	static const char* m_fs_code;
	static const char* m_vs_code;
	static const char* m_gs_code;

	static const glw::GLint m_height;
	static const glw::GLint m_width;
	static const int		m_n_layers;
	static const glw::GLint m_n_texture_components;

	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;

	glw::GLint* m_all_layers_data;
	glw::GLint* m_layer_data;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERLAYEREDRENDERINGFBONOATTACHMENT_HPP
