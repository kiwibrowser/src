#ifndef _ESEXTCGEOMETRYSHADERLAYEREDRENDERING_HPP
#define _ESEXTCGEOMETRYSHADERLAYEREDRENDERING_HPP
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
/** Implementation of "Group 7" from CTS_EXT_geometry_shader. Description follows:
 *
 * 1. Make sure that layered rendering works correctly for cube-map texture
 *    color attachments.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Initialize cube-map texture object, assuming that cube-map face textures
 *    should have a resolution of 64x64 and use GL_RGBA8 internal format.
 *
 *    Use glFramebufferTextureEXT() to attach base level of cube-map texture to
 *    color attachment 0.
 *
 *    Vertex shader can be boilerplate.
 *
 *    Geometry shader should take point primitives on input and output triangle
 *    strips. The shader will not emit more than 6 faces*16 vertices.
 *
 *    Consider a triangle strip consisting of the following vertices:
 *
 *    1) ( 1,  1, 0, 1)
 *    2) ( 1, -1, 0, 1)
 *    3) (-1,  1, 0, 1)
 *    4) (-1, -1, 0, 1)
 *
 *    which is a "full-screen" quad in screen coordinates space. Remembering
 *    that each quad should be separated with EndPrimitive() call, quad
 *    primitives should be emitted to all six layers. Remember to set gl_Layer
 *    to corresponding layer ID for each of the vertices.
 *
 *    Fragment shader should set result variable to:
 *
 *    1) (1, 0, 0, 0) if gl_Layer == 0 (+X face)
 *    2) (0, 1, 0, 0) if gl_Layer == 1 (-X face)
 *    3) (0, 0, 1, 0) if gl_Layer == 2 (+Y face)
 *    4) (0, 0, 0, 1) if gl_Layer == 3 (-Y face)
 *    5) (1, 1, 0, 0) if gl_Layer == 4(+Z face)
 *    6) (1, 0, 1, 0) if gl_Layer == 5(-Z face)
 *
 *    The test should then read cube-map texture data and verify that all
 *    texels of each cube map face texture are set to correct values.
 *
 *
 * 2. Make sure that layered rendering works correctly for a set of 3D texture
 *    color attachments.
 *
 *    Category: API;
 *              Functional test.
 *
 *    Modify test case 7.1, so that:
 *
 *    - the test uses a 64x64x64 3D texture instead of a cube-map texture;
 *    - the test attaches 4 first levels of the texture instead of cube-map
 *      texture faces to consequent color attachments;
 *    - the test only writes to 4 layers instead of 6.
 *
 *    The change of number of layers we write to is related to a minimum
 *    maximum of color attachments implementations must expose to be
 *    GLES3.0-compliant.
 *
 *
 * 3. Make sure that layered rendering works correctly for a set of 2D array
 *    texture color attachments.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 7.2, so that the test uses a 64x64x64 2D array texture
 *    instead of a 3D texture;
 *
 *
 * 4. Make sure that layered rendering works correctly for a set of multisample
 *    2D array  texture color attachments.
 *
 *    Category: API;
 *              Dependency on OES_texture_storage_multisample_2d_array;
 *              Functional Test.
 *
 *    Modify test case 7.2, so that the test uses a 64x64x64 multisample 2D
 *    array texture instead of a 3D texture;
 **/
class GeometryShaderLayeredRendering : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderLayeredRendering(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~GeometryShaderLayeredRendering(void)
	{
	}

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private type definitions */
	/** Describes type of the test iteration being considered */
	typedef enum {
		LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY,
		LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY,
		LAYERED_RENDERING_TEST_ITERATION_3D,
		LAYERED_RENDERING_TEST_ITERATION_CUBEMAP,

		/* Always last */
		LAYERED_RENDERING_TEST_ITERATION_LAST
	} _layered_rendering_test_iteration;

	/** Holds data necessary to perform a single iteration of a single layered rendering test iteration */
	typedef struct
	{
		glw::GLuint fbo_id;
		glw::GLuint fs_id;
		glw::GLuint gs_id;
		glw::GLuint po_id;
		glw::GLuint to_id;
		glw::GLuint vs_id;

		const char** fs_parts;
		const char** gs_parts;
		const char** vs_parts;
		unsigned int n_fs_parts;
		unsigned int n_gs_parts;
		unsigned int n_vs_parts;

		_layered_rendering_test_iteration iteration;
		unsigned int					  n_layers;
	} _layered_rendering_test;

	/* Private functions */
	bool buildProgramForLRTest(_layered_rendering_test* test);

	/* Private variables */
	static const unsigned char m_layered_rendering_expected_layer_data[6 * 4];
	static const char*		   m_layered_rendering_fs_code;
	static const char*		   m_layered_rendering_gs_code_preamble;
	static const char*		   m_layered_rendering_gs_code_2d_array;
	static const char*		   m_layered_rendering_gs_code_2d_marray;
	static const char*		   m_layered_rendering_gs_code_3d;
	static const char*		   m_layered_rendering_gs_code_cm;
	static const char*		   m_layered_rendering_gs_code_main;
	static const char*		   m_layered_rendering_vs_code;

	glw::GLuint m_vao_id;

	/* Holds pointers to test instances that are to be executed */
	_layered_rendering_test m_tests[LAYERED_RENDERING_TEST_ITERATION_LAST];
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERLAYEREDRENDERING_HPP
