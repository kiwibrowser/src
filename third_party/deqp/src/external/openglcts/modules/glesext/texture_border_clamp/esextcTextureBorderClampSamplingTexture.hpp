#ifndef _ESEXTCTEXTUREBORDERCLAMPSAMPLINGTEXTURE_HPP
#define _ESEXTCTEXTUREBORDERCLAMPSAMPLINGTEXTURE_HPP
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

/*!
 * \file esextcTextureBorderClampSamplingTexture.hpp
 * \brief Verify that sampling a texture with GL_CLAMP_TO_BORDER_EXT
 * wrap mode enabled gives correct results (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "glwEnums.hpp"
#include <vector>

namespace glcts
{

/** Class to store test configuration
 */
template <typename InputType, typename OutputType>
class TestConfiguration
{
public:
	/* Public functions */
	TestConfiguration(glw::GLsizei nInputComponents, glw::GLsizei nOutputComponents, glw::GLenum target,
					  glw::GLenum inputInternalFormat, glw::GLenum outputInternalFormat, glw::GLenum filtering,
					  glw::GLenum inputFormat, glw::GLenum outputFormat, glw::GLuint width, glw::GLuint height,
					  glw::GLuint depth, InputType initValue, InputType initBorderColor, OutputType expectedValue,
					  OutputType expectedBorderColor, glw::GLenum inputType, glw::GLenum outputType);

	TestConfiguration(const TestConfiguration& configuration);

	virtual ~TestConfiguration()
	{
	}

	inline glw::GLsizei get_n_in_components(void) const
	{
		return m_n_in_components;
	}
	inline glw::GLsizei get_n_out_components(void) const
	{
		return m_n_out_components;
	}
	inline glw::GLenum get_target(void) const
	{
		return m_target;
	}
	inline glw::GLenum get_input_internal_format(void) const
	{
		return m_input_internal_format;
	}
	inline glw::GLenum get_output_internal_format(void) const
	{
		return m_output_internal_format;
	}
	inline glw::GLenum get_filtering(void) const
	{
		return m_filtering;
	}
	inline glw::GLenum get_input_format(void) const
	{
		return m_input_format;
	}
	inline glw::GLenum get_output_format(void) const
	{
		return m_output_format;
	}
	inline glw::GLuint get_width(void) const
	{
		return m_width;
	}
	inline glw::GLuint get_height(void) const
	{
		return m_height;
	}
	inline glw::GLuint get_depth(void) const
	{
		return m_depth;
	}
	inline InputType get_init_value(void) const
	{
		return m_init_value;
	}
	inline InputType get_init_border_color(void) const
	{
		return m_init_border_color;
	}
	inline OutputType get_expected_value(void) const
	{
		return m_expected_value;
	}
	inline OutputType get_expected_border_color(void) const
	{
		return m_expected_border_color;
	}
	inline glw::GLenum get_input_type(void) const
	{
		return m_input_type;
	}
	inline glw::GLenum get_output_type(void) const
	{
		return m_output_type;
	}

private:
	/* Private variables */
	glw::GLsizei m_n_in_components;
	glw::GLsizei m_n_out_components;
	glw::GLenum  m_target;
	glw::GLenum  m_input_internal_format;
	glw::GLenum  m_output_internal_format;
	glw::GLenum  m_filtering;
	glw::GLenum  m_input_format;
	glw::GLenum  m_output_format;
	glw::GLuint  m_width;
	glw::GLuint  m_height;
	glw::GLuint  m_depth;
	InputType	m_init_value;
	InputType	m_init_border_color;
	OutputType   m_expected_value;
	OutputType   m_expected_border_color;
	glw::GLenum  m_input_type;
	glw::GLenum  m_output_type;
};

/*   Implementation of Test 7 from CTS_EXT_texture_border_clamp. Description follows
 *
 *    Verify that sampling a texture with GL_CLAMP_TO_BORDER_EXT wrap mode
 *    enabled for all R/S/T dimensions gives correct results.
 *
 *    Category:           Functional test;
 *
 *    Suggested priority: Must-have.
 *
 *    This test should iterate over the following texture targets supported by
 *    ES3.1:
 *
 *    - 2D textures;
 *    - 2D array textures;
 *    - 3D textures;
 *
 *    (note that cube-map texture targets are left out, as seamless filtering
 *    renders the border color effectively useless)
 *
 *    For each texture target, the test should iterate over the following the
 *    set of internal formats:
 *
 *    - GL_RGBA32F;
 *    - GL_R32UI;
 *    - GL_R32I;
 *    - GL_RGBA8;
 *    - GL_DEPTH_COMPONENT32F;
 *    - GL_DEPTH_COMPONENT16;
 *    - At least one compressed internal format described in the extension
 *    specification.
 *
 *    Note: For glCompressedTexImage2D() or glCompressedTexImage3D() calls,
 *          it is expected that predefined valid blobs will be used.
 *
 *    The texture size used in the test should be 256x256 for 2d textures,
 *    256x256x6 for 2d array textures and 3d textures (smaller sizes are
 *    allowed for compressed internal format).
 *
 *    For each texture we should have two iterations, one with GL_LINEAR
 *    minification filtering, the second with GL_NEAREST minification
 *    filtering.
 *
 *    Reference texture data should be as follows:
 *
 *    * Floating-point:         (0.0, 0.0, 0.0, 0.0);
 *    * Unsigned integer:       (0);
 *    * Signed integer:         (0);
 *    * Normalized fixed-point: (0, 0, 0, 0);
 *    * Depth (floating-point): (0.0);
 *    * Depth (unsigned short): (0);
 *
 *    The border color should be set to:
 *
 *    * Floating-point:         (1.0, 1.0, 1.0, 1.0);
 *    * Unsigned integer:       (255, 255, 255, 255);
 *    * Signed integer:         (255, 255, 255, 255);
 *    * Normalized fixed-point: (255, 255, 255, 255);
 *    * Depth (floating-point): (1.0, 1.0, 1.0, 1.0);
 *    * Depth (unsigned short): (255, 255, 255, 255);
 *
 *    In each iteration, the test should render a full-screen quad to
 *    a two-dimensional texture of resolution 256x256 (smaller sizes are
 *    allowed for compressed internal format) and internal format
 *    compatible with the format of the texture used in this iteration
 *    (we take into account that values stored in floating-point textures
 *    are bigger than 0.0 and smaller than 1.0):
 *
 *    - GL_RGBA8;
 *    - GL_R32UI;
 *    - GL_R32I;
 *    - GL_RGBA8;
 *    - GL_R8;
 *    - GL_R8;
 *
 *    The following UVs should be outputted by the vertex shader:
 *
 *    - (-1, -1) for bottom-left corner of the viewport;
 *    - (-1,  2) for top-left corner of the viewport;
 *    - ( 2,  2) for top-right corner of the viewport;
 *    - ( 2, -1) for bottom-right corner of the viewport;
 *
 *    The fragment shader should sample an iteration-specific texture sampler
 *    at given UV location. The shader should output the result of this
 *    sampling as the color value.
 *
 *    The test succeeds, if result texture is valid for all texture target +
 *    internal format combinations.
 *
 *    Verification process for each iteration should be as follows:
 *
 *    1) Download rendered data to process space;
 *    2) The expected rendering outcome for GL_NEAREST minification filtering
 *    is as depicted below:
 *
 *                 (-1, -1)    (0, -1)  (1,  -1)     (2, -1)
 *                         *-------+-------+-------*
 *                         |       |       |       |
 *                         |  BC   |  BC   |   BC  |
 *                         |       |       |       |
 *                 (-1, 0) +-------+-------+-------+ (2, 0)
 *                         |       |       |       |
 *                         |  BC   |   0   |   BC  |
 *                         |       |       |       |
 *                 (-1, 1) +-------+-------+-------+ (2, 1)
 *                         |       |       |       |
 *                         |  BC   |  BC   |   BC  |
 *                         |       |       |       |
 *                         *-------+-------+-------*
 *                 (-1, 2)      (0, 2)  (1, 2)       (2, 2)
 *
 *    BC means the border color for the used internal format.
 *
 *    Values in the brackets correspond to UV space. Top-left corner corresponds
 *    to bottom-left corner of the data in GL orientation (that is: as retrieved
 *    by glReadPixels() call).
 *
 *    Note for 2D array texture: assuming a texture of depth n_layers,
 *    for which 0..n_layers-1 have been defined (inclusive), the test should
 *    sample data from -1, 0 .. n_layers layers. It is expected that for sampling
 *    outside of <0, n_layers-1> set of layers the layer number will be clamped
 *    the the range <0, n_layers-1>. This means that result textures for border
 *    cases should be the same as for sampling from inside of <0, n_layers-1>
 *    range.
 *
 *    Note for 3D texture: assuming a texture of depth n_layers,
 *    for which 0..n_layers-1 have been defined (inclusive), the test should
 *    sample data from -1, 0 .. n_layers layers. It is expected that sampling
 *    outside <0, n_layers-1> set of layers or slices of the texture should
 *    return the border color defined for the texture object being sampled.
 *    This means that result textures for border cases should be completely
 *    filled with BC color.
 *
 *    Iteration passes if centres of all rectangles contain the correct colors.
 *
 *    3) The expected rendering outcome for GL_LINEAR minification filtering is
 *    to some extent similar to the one for GL_NEAREST, with the difference that
 *    the transition between 0 and BC values is smooth (some values in between 0
 *    and BC may appear on the edge of the rectangles). For this case we need
 *    a different way of checking if the rendering outcome is correct. We should
 *    start in the middle of the texture and check values of texels moving in four
 *    directions - to the left, right, bottom, top. In each direction the values of
 *    the texels should form a monotonically increasing series.
 */
template <typename InputType, typename OutputType>
class TextureBorderClampSamplingTexture : public TestCaseBase
{
public:
	/* Public methods */
	TextureBorderClampSamplingTexture(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description,
									  const TestConfiguration<InputType, OutputType>& configuration);

	virtual ~TextureBorderClampSamplingTexture()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);
	void setInitData(std::vector<InputType>& buffer);
	void checkFramebufferStatus(glw::GLenum framebuffer);
	bool checkResult(OutputType expectedValue, OutputType expectedBorderColor, glw::GLint layer);
	bool checkNearest(std::vector<OutputType>& buffer, OutputType expectedValue, OutputType expectedBorderColor,
					  glw::GLint layer);
	bool checkLinear(std::vector<OutputType>& buffer, glw::GLint layer);
	void		 createTextures(void);
	glw::GLfloat getCoordinateValue(glw::GLint index);
	std::string getFragmentShaderCode(void);
	std::string getVertexShaderCode(void);
	glw::GLint  getStartingLayerIndex();
	glw::GLint  getLastLayerIndex();

	/* Private variables */
	glw::GLint  m_attr_position_location;
	glw::GLint  m_attr_texcoord_location;
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_sampler_id;
	TestConfiguration<InputType, OutputType> m_test_configuration;
	glw::GLuint m_input_to_id;
	glw::GLuint m_output_to_id;
	glw::GLuint m_position_vbo_id;
	glw::GLuint m_text_coord_vbo_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vao_id;

	/* Private static variables */
	static const glw::GLuint m_texture_unit;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPSAMPLINGTEXTURE_HPP
