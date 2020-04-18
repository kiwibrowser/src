#ifndef _GL3CTEXTURESIZEPROMOTION_HPP
#define _GL3CTEXTURESIZEPROMOTION_HPP
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

/**
 * \file  gl3cTextureSizePromotionTests.hpp
 * \brief Declares test classes for testing of texture internal format
 promotion mechanism.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include "tcuVector.hpp"

namespace gl3cts
{
namespace TextureSizePromotion
{
/** Test group class for texture size promotion tests */
class Tests : public deqp::TestCaseGroup
{
public:
	/* Public member functions. */
	Tests(deqp::Context& context); //!< Constructor.
	virtual ~Tests()
	{
	} //!< Destructor

	virtual void init(void); //!< Initialization member function.

private:
	/* Private member functions. */
	Tests(const Tests&);			//!< Default copy constructor.
	Tests& operator=(const Tests&); //!< Default assign constructor.
};

/** Functional Test Class
 *
 *  This test verifies that implementation correctly selects sizes and types
 *  when sized internal format is requested for a texture.
 *
 *  This test should be executed only if context is at least 3.1.
 *
 *  Steps:
 *   - prepare a source texture so that each channel is filled with specific value;
 *   - execute GetTexLevelPrameter to query all TEXTURE_*_SIZE and TEXTURE_*_TYPE
 *     <pname>s corresponding with internal format of source texture; It is
 *     expected that:
 *      * reported sizes will be at least as specified;
 *      * reported types will be exactly as specified;
 *   - for each channel [R, G, B, A]:
 *      * prepare a 2D single channeled texture with format matching sampled
 *        channel and set it up as output color in framebuffer;
 *      * prepare a program that will implement the following snippet in the
 *        fragment stage and output the value of result:
 *
 *            result = texelFetch(source).C;
 *
 *      * clear output texture;
 *      * draw a full-screen quad;
 *      * verify that the output texture is filled with correct value;
 *
 *  Value is correct when:
 *   - it matches value assigned to the specified channel;
 *   - it is one for missing alpha channel;
 *   - it is zero for missing channel;
 *   - it is one for ONE;
 *   - it is zero for ZERO.
 *
 *  Repeat the steps for all supported sized internal formats and targets.
 *
 *  Depth-stencil textures can be sampled only via RED channel. Test should set
 *  DEPTH_STENCIL_TEXTURE_MODE to select which channel will be accessed.
 *
 *  For multisampled targets maximum supported number of samples should be used
 *  and fetch should be done to last sample.
 *
 *  Support of multisampled targets by TexParameter* routines was introduced in
 *  extension GL_ARB_texture_storage_multisample, which is part of core
 *  specification since 4.3.
 *
 *  List of required texture formats was changed in 4.4. Therefore the list of
 *  "supported sized internal formats" depends on context version.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	FunctionalTest(deqp::Context& context);			//!< Functional test constructor.
	virtual tcu::TestNode::IterateResult iterate(); //!< Member function to iterate over test cases.

private:
	/* Private member variables. */
	glw::GLuint m_vao;				   //!< Vertex Array Object name.
	glw::GLuint m_source_texture;	  //!< Source Texture  Object name.
	glw::GLuint m_destination_texture; //!< Destination Texture Object name.
	glw::GLuint m_framebuffer;		   //!< Framebuffer Object name.
	glw::GLuint m_program;			   //!< Program Object name.
	glw::GLint  m_max_samples;		   //!< Maximum samples available for usage in multisampled targets.

	/* Private type definitions. */

	/** Texture Internal Format Description structure
	 */
	struct TextureInternalFormatDescriptor
	{
		glu::ContextType   required_by_context;  //!< Minimum context version by which format is required.
		glw::GLenum		   internal_format;		 //!< Texture internal format.
		const glw::GLchar* internal_format_name; //!< String representing texture internal format.
		bool			   is_sRGB;				 //!< Is this format described in sRGB space.
		bool			   is_color_renderable;  //!< Is this format color renderable.

		glw::GLint min_red_size;	 //!< Minimum required red     component resolution (in bits).
		glw::GLint min_green_size;   //!< Minimum required green   component resolution (in bits).
		glw::GLint min_blue_size;	//!< Minimum required blue    component resolution (in bits).
		glw::GLint min_alpha_size;   //!< Minimum required alpha   component resolution (in bits).
		glw::GLint min_depth_size;   //!< Minimum required depth   component resolution (in bits).
		glw::GLint min_stencil_size; //!< Minimum required stencil component resolution (in bits).

		glw::GLenum expected_red_type;   //!< Expected type of red   component.
		glw::GLenum expected_green_type; //!< Expected type of green component.
		glw::GLenum expected_blue_type;  //!< Expected type of blue  component.
		glw::GLenum expected_alpha_type; //!< Expected type of alpha component.
		glw::GLenum expected_depth_type; //!< Expected type of depth component.
	};

	/** Color channels enumeration
	 */
	enum ColorChannelSelector
	{
		RED_COMPONENT,   //!< Red   component.
		GREEN_COMPONENT, //!< Green component.
		BLUE_COMPONENT,  //!< Blue  component.
		ALPHA_COMPONENT, //!< Alpha component (must be last color channel).
		COMPONENTS_COUNT //!< Number of components.
	};

	/* Private class' static constants. */
	static const glw::GLfloat s_source_texture_data_f[]; //!< Source texture for floating point type internal formats.
	static const glw::GLfloat
		s_source_texture_data_n[]; //!< Source texture for unsigned normalized integer type internal formats.
	static const glw::GLfloat
							 s_source_texture_data_sn[]; //!< Source texture for signed normalized integer type internal formats.
	static const glw::GLint  s_source_texture_data_i[];  //!< Source texture for signed integer type internal formats.
	static const glw::GLuint s_source_texture_data_ui[]; //!< Source texture for unsigned integer type internal formats.
	static const glw::GLuint s_source_texture_size;		 //!< Linear size of the source texture.
	static const glw::GLfloat s_destination_texture_data_f
		[]; //!< Destination texture data (to be sure that it was overwritten) for floating point and normalized types internal formats.
	static const glw::GLint s_destination_texture_data_i
		[]; //!< Destination texture data (to be sure that it was overwritten) signed integer type internal formats.
	static const glw::GLuint s_destination_texture_data_ui
		[]; //!< Destination texture data (to be sure that it was overwritten) unsigned integer type internal formats.

	static const glw::GLenum  s_source_texture_targets[];		//!< Targets to be tested.
	static const glw::GLchar* s_source_texture_targets_names[]; //!< Targets' names (strings) for logging purpose.
	static const glw::GLuint  s_source_texture_targets_count;   //!< Number of targets to be tested.

	static const glw::GLchar* s_color_channel_names[]; //!< Color channel names (like in enum) for logging purpose.

	static const glw::GLchar*
							  s_vertex_shader_code; //!< Vertex shader source code for drawing quad depending on vertex ID of triangle strip.
	static const glw::GLchar* s_fragment_shader_template; //!< Fragment shader source code template.

	static const TextureInternalFormatDescriptor
							 s_formats[]; //!< List of internal formats (and their descriptions) to be tested by Functional Test.
	static const glw::GLuint s_formats_size; //!< number of internal format to be tested.

	/* Private member functions. */

	/** Generate and bind an empty Vertex Array Object.
	 */
	void prepareVertexArrayObject();

	/** Generate, bind and upload source texture.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] target          Texture target to be used.
	 */
	void prepareSourceTexture(TextureInternalFormatDescriptor descriptor, glw::GLenum target);

	/** Generate, bind and clean destination texture.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] target          Texture target to be used.
	 */
	void prepareDestinationTextureAndFramebuffer(TextureInternalFormatDescriptor descriptor, glw::GLenum target);

	/** Preprocess, compile and linke GLSL program.
	 *
	 *  @param [in] target          Texture target to be used.
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] channel         Color channel to be tested.
	 *
	 *  @return Program name on success, 0 on failure.
	 */
	glw::GLuint prepareProgram(glw::GLenum target, TextureInternalFormatDescriptor descriptor,
							   ColorChannelSelector channel);

	/** Use GLSL program with source and destination textures.
	 *
	 *  @param [in] target          Texture target to be used.
	 */
	void makeProgramAndSourceTextureActive(glw::GLenum target);

	/** Check source texture queries.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] target          Texture target to be used.
	 */
	bool checkSourceTextureSizeAndType(TextureInternalFormatDescriptor descriptor, glw::GLenum target);

	/** Draw quad using GL_TRIANGLE_STRIP.
	 */
	void drawQuad();

	/** Check rendered destination texture to match expected values.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] channel         Used color channel.
	 *  @param [in] target          Texture target to be used.
	 *  @param [in] target_name     Texture target name for logging purposes.
	 *
	 *  @return True if fetched value matches expected value within the range of precission, false otherwise.
	 */
	bool checkDestinationTexture(TextureInternalFormatDescriptor descriptor, ColorChannelSelector channel,
								 glw::GLenum target, const glw::GLchar* target_name);

	/** Clean source texture object.
	 */
	void cleanSourceTexture();

	/** Clean framebuffer object.
	 */
	void cleanFramebuffer();

	/** Clean destination texture object.
	 */
	void cleanDestinationTexture();

	/** Clean program object.
	 */
	void cleanProgram();

	/** Clean vertex array object object.
	 */
	void cleanVertexArrayObject();

	/** Choose internal format of destination texture for rendered source texture.
	 *
	 *  @param [in] descriptor      Internal format description.
	 */
	glw::GLenum getDestinationFormatForChannel(TextureInternalFormatDescriptor descriptor);

	/** Is internal format a floating type.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *
	 *  @return True if internal format is floating point type, false otherwise.
	 */
	bool isFloatType(TextureInternalFormatDescriptor descriptor);

	/** Is internal format a fixed signed type.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *
	 *  @return True if internal format is fixed signed type, false otherwise.
	 */
	bool isFixedSignedType(TextureInternalFormatDescriptor descriptor);

	/** Is internal format a fixed unsigned type.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *
	 *  @return True if internal format is fixed unsigned type, false otherwise.
	 */
	bool isFixedUnsignedType(TextureInternalFormatDescriptor descriptor);

	/** Is internal format a signed integral type.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *
	 *  @return True if internal format is integral signed type, false otherwise.
	 */
	bool isIntegerSignedType(TextureInternalFormatDescriptor descriptor);

	/** Is internal format an unsigned integral type.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *
	 *  @return True if internal format is integral unsigned type, false otherwise.
	 */
	bool isIntegerUnsignedType(TextureInternalFormatDescriptor descriptor);

	/** Is internal format a depth type.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *
	 *  @return True if internal format is depth type, false otherwise.
	 */
	bool isDepthType(TextureInternalFormatDescriptor descriptor);

	/** Is internal format a stencil type.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *
	 *  @return True if internal format is stencil type, false otherwise.
	 */
	bool isStencilType(TextureInternalFormatDescriptor descriptor);

	/** Is channel of internal format a none type (does not appear in the texture internal format).
	 *
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] channel         Color channel to be queried.
	 *
	 *  @return True if internal format is none type, false otherwise.
	 */
	bool isChannelTypeNone(TextureInternalFormatDescriptor descriptor, ColorChannelSelector channel);

	/** Calculate minimal required precission for internal format's channel.
	 *
	 *  @note It is needed only for floating point and normalized fixed point types.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] channel         Color channel to be queried.
	 *
	 *  @return Minimum precission.
	 */
	glw::GLfloat getMinPrecision(TextureInternalFormatDescriptor descriptor, ColorChannelSelector channel);

	/** Is target multisample.
	 *
	 *  @param [in] target      Target.
	 *
	 *  @return True if target is multisampled, false otherwise.
	 */
	bool isTargetMultisampled(glw::GLenum target);

	/** Render data to the source texture for multisampled texture.
	 *
	 *  @param [in] descriptor      Internal format description.
	 *  @param [in] target          Texture target to be used.
	 */
	void renderDataIntoMultisampledTexture(TextureInternalFormatDescriptor descriptor, glw::GLenum target);

	/** Convert value from sRGB space to linear space.
	 *
	 *  @param [in] value           Value to be converted (sRGB space).
	 *
	 *  @return Converted value (linear space).
	 */
	float convert_from_sRGB(float value);
};
/* class TextureSizePromotion */

namespace Utilities
{
/** Build a GLSL program
 *
 *  @param [in]  gl                                     OpenGL Functions Access.
 *  @param [in]  log                                    Log outut.
 *  @param [in]  vertex_shader_source                   Pointer to C string of the vertex shader or NULL if not used.
 *  @param [in]  fragment_shader_source                 Pointer to C string of the fragment shader or NULL if not used.
 *
 *  @return OpenGL program shader ID or zero if error had occured.
 */
glw::GLuint buildProgram(glw::Functions const& gl, tcu::TestLog& log, glw::GLchar const* const vertex_shader_source,
						 glw::GLchar const* const fragment_shader_source);

/** Preprocess source string by replacing key tokens with new values.
 *
 *  @param [in] source      Source string.
 *  @param [in] key         Key, substring to be replaced.
 *  @param [in] value       Value, substring to be substituted in place of key.
 *
 *  @return Preprocessed string.
 */
std::string preprocessString(std::string source, std::string key, std::string value);

/** @brief Convert an integer to a string.
 *
 *  @param [in] i       Integer to be converted.
 *
 *  @return String representing integer.
 */
std::string itoa(glw::GLint i);
}

} /* TextureSizePromotion namespace */
} /* gl3cts namespace */

#endif // _GL3CTEXTURESIZEPROMOTION_HPP
