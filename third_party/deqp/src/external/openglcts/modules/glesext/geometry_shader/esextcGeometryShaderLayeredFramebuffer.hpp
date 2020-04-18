#ifndef _ESEXTCGEOMETRYSHADERLAYEREDFRAMEBUFFER_HPP
#define _ESEXTCGEOMETRYSHADERLAYEREDFRAMEBUFFER_HPP
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

namespace glcts
{
/** Implementation of Tests 11.3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. Make sure blending and color writes work correctly for layered rendering.
 *
 *  Category: API;
 *            Functional Test.
 *
 *  Consider a layered framebuffer object A:
 *
 *  - 3D texture object A of resolution 4x4x4, internalformat GL_RGBA8, is
 *    bound to color attachment 0.
 *
 *  For texture object A, each slice should be filled with texels of value
 *  (0, slice_index / 4, slice_index / 8, slice_index / 12), where
 *  slice_index corresponds to slice index considered.
 *
 *  Depth and stencil tests should be disabled.
 *
 *  Blending should be enabled. Source blend function should be set to
 *  GL_ONE_MINUS_SRC_COLOR, destination blend function should be set to
 *  GL_DST_COLOR.
 *
 *  A vertex shader for the test can be boilerplate.
 *
 *  Geometry shader should take points on input and output a maximum of
 *  4 (layers) * (4 (sets of) * 4 (coordinates) ) vertices forming a triangle
 *  strip (as in test case 7.1). The shader should emit 4 "full-screen quads"
 *  for layers 0, 1, 2 and 3.
 *
 *  A fragment shader for the test should set the only output vec4 variable
 *  to (0.2, 0.2, 0.2, 0.2).
 *
 *  Using a program object built of these three shaders *and* with the
 *  framebuffer object A bound to both targets *and* with stencil test
 *  stage configured as discussed, the test should draw a single point.
 *
 *  Test passes if result texel data is valid for all slices, given active
 *  blending equation and function configuration.
 *
 **/
class GeometryShaderLayeredFramebufferBlending : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderLayeredFramebufferBlending(Context& context, const ExtParameters& extParams, const char* name,
											 const char* description);

	virtual ~GeometryShaderLayeredFramebufferBlending()
	{
	}

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private functions */

	/* Private variables */
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_read_fbo_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/** Implementation of Tests 11.4 and 11.5 from CTS_EXT_geometry_shader. Description follows:
 *
 * 4. Make sure glClear*() commands clear all layers of a layered attachment.
 *
 *  Category: API;
 *            Functional Test.
 *
 *  Consider a layered framebuffer object A with the following configuration:
 *
 *  - 3D texture of resolution 4x4x4 and of internal format GL_RGBA8, bound
 *  to color attachment 0. The texture contents should be as follows:
 *
 *  1) Texels of the first slice should be set to  (255, 0,   0,   0).
 *  2) Texels of the second slice should be set to (0,   255, 0,   0).
 *  3) Texels of the third slice should be set to  (0,   0,   255, 0).
 *  4) Texels of the fourth slice should be set to (255, 255, 0,   0).
 *
 *  FBO A should be bound to GL_FRAMEBUFFER target. Clear color should be set
 *  to (64, 128, 255, 32).
 *
 *  For each glClear*() command, reset all slices of the texture object with
 *  values as above, and clear color attachment 0. The test should then
 *  verify that each slice was set to (64, 128, 255, 32).
 *
 * 5. glReadPixels() calls done for a layered attachment should always work on
 *    layer zero.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Consider a 3D texture object A, configured exactly like in test case 11.4.
 *
 *    Consider a layered framebuffer object B. Texture object A's first layer
 *    should be attached to color attachment 0.
 *
 *    The test should do a glReadPixels() call with GL_RGBA format and
 *    GL_UNSIGNED_BYTE type. The data read should be equal to (255, 0, 0, 0)
 *    for all texels.
 *
 **/
class GeometryShaderLayeredFramebufferClear : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderLayeredFramebufferClear(Context& context, const ExtParameters& extParams, const char* name,
										  const char* description);

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private functions */

	/* Private variables */
	glw::GLuint m_fbo_char_id;
	glw::GLuint m_fbo_int_id;
	glw::GLuint m_fbo_uint_id;
	glw::GLuint m_read_fbo_id;
	glw::GLuint m_to_rgba32i_id;
	glw::GLuint m_to_rgba32ui_id;
	glw::GLuint m_to_rgba8_id;
};

/** Implementation of Tests 11.2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. Make sure depth test stage works correctly for layered rendering.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Consider a layered framebuffer object A:
 *
 *     - 2D array texture object A of resolution 4x4x4, internalformat GL_RGBA8, all
 *       slices set to (0, 0, 0, 0) is bound to color attachment 0.
 *     - 2D array texture object B of resolution 4x4x4, internalformat
 *       GL_DEPTH_COMPONENT32F is bound to depth attachment.
 *
 *     For texture object B, all slices should be filled with depth value 0.5.
 *
 *     Stencil test should be disabled.
 *
 *     A vertex shader for the test can be boilerplate.
 *
 *     Geometry shader should take points on input and output a maximum of
 *     4 (layers) * (4 (sets of) * 4 (coordinates) ) vertices forming a triangle
 *     strip (as in test case 7.1). The shader should emit 4 "full-screen quads"
 *     for layers 0, 1, 2 and 3, with an exception that the depth of each of
 *     these quads should be equal to -1 + (quad index) / 2. For instance:
 *     first layer's quad would be placed at depth -1.0, second layer's quad
 *     would be placed at depth -0.5, and so on.
 *
 *     A fragment shader for the test should set the only output vec4 variable
 *     to (1, 1, 1, 1).
 *
 *     Using a program object built of these three shaders *and* with the
 *     framebuffer object A bound to both targets *and* with stencil test stage
 *     configured as discussed, the test should draw a single point.
 *
 *     Test passes if the following conditions are met at this point:
 *
 *     * All texels of slice 0 of the texture object A are set to
 *       (255, 255, 255, 255).
 *     * All texels of slice 1 of the texture object A are set to
 *       (255, 255, 255, 255).
 *     * All texels of slice 2 of the texture object A are set to
 *       (0, 0, 0, 0);
 *     * All texels of slice 3 of the texture object A are set to
 *       (0, 0, 0, 0);
 **/
class GeometryShaderLayeredFramebufferDepth : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderLayeredFramebufferDepth(Context& context, const ExtParameters& extParams, const char* name,
										  const char* description);

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private functions */

	/* Private variables */
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_read_fbo_id;
	glw::GLuint m_to_a_id;
	glw::GLuint m_to_b_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/** Implementation of Tests 11.1 from CTS_EXT_geometry_shader. Description follows:
 *
 * Make sure stencil test stage works correctly for layered rendering.
 *
 * Category: API;
 *           Functional Test.
 *
 * Consider a layered framebuffer object A:
 *
 * - 2D array texture object A of resolution 4x4x4, internalformat GL_RGBA8, all
 *   slices set to (0, 0, 0, 0) is bound to color attachment 0.
 * - 2D array texture object B of resolution 4x4x4, internalformat
 *   GL_DEPTH32F_STENCIL8 is bound to depth+stencil attachments.
 *
 * For texture object B, first slice (of index 0) should be filled with
 * stencil value 2, second slice should be filled with stencil value 1,
 * third and fourth slices should be filled with stencil value 0.
 *
 * Stencil function should be set to GL_LESS, reference value should
 * be set at 0.
 *
 * Depth tests should be disabled.
 *
 * A vertex shader for the test can be boilerplate.
 *
 * Geometry shader should take points on input and output a maximum of
 * 4 (layers) * (4 (sets of) *4 (coordinates) ) vertices forming a triangle
 * strip (as in test case 7.1). The shader should emit 4 "full-screen quads"
 * for layers 0, 1, 2 and 3.
 *
 * A fragment shader for the test should set the only output vec4 variable
 * to (1, 1, 1, 1).
 *
 * Using a program object built of these three shaders *and* with the
 * framebuffer object A bound to both targets *and* with stencil test stage
 * configured as discussed, the test should draw a single point.
 *
 * Test passes if the following conditions are met at this point:
 *
 * * All texels of slice 0 of the texture object A are set to
 *   (255, 255, 255, 255).
 * * All texels of slice 1 of the texture object A are set to
 *   (255, 255, 255, 255).
 * * All texels of slice 2 of the texture object A are set to
 *   (0, 0, 0, 0);
 * * All texels of slice 3 of the texture object A are set to
 *   (0, 0, 0, 0);
 *
 **/
class GeometryShaderLayeredFramebufferStencil : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderLayeredFramebufferStencil(Context& context, const ExtParameters& extParams, const char* name,
											const char* description);

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private functions */

	/* Private variables */
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_to_a_id;
	glw::GLuint m_to_b_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERLAYEREDFRAMEBUFFER_HPP
