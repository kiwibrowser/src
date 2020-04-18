#ifndef _GL4CTEXTUREVIEWTESTS_HPP
#define _GL4CTEXTUREVIEWTESTS_HPP
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

/**
 * \file  gl4cTextureViewTests.hpp
 * \brief Declares test classes for "texture view" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include "tcuVector.hpp"

namespace gl4cts
{
namespace TextureView
{

enum _format
{
	FORMAT_FLOAT,
	FORMAT_RGBE,
	FORMAT_SIGNED_INTEGER,
	FORMAT_SNORM,
	FORMAT_UNORM,
	FORMAT_UNSIGNED_INTEGER,

	FORMAT_UNDEFINED
};

enum _sampler_type
{
	SAMPLER_TYPE_FLOAT,
	SAMPLER_TYPE_SIGNED_INTEGER,
	SAMPLER_TYPE_UNSIGNED_INTEGER,

	SAMPLER_TYPE_UNDEFINED
};

enum _view_class
{
	VIEW_CLASS_FIRST,

	VIEW_CLASS_128_BITS = VIEW_CLASS_FIRST,
	VIEW_CLASS_96_BITS,
	VIEW_CLASS_64_BITS,
	VIEW_CLASS_48_BITS,
	VIEW_CLASS_32_BITS,
	VIEW_CLASS_24_BITS,
	VIEW_CLASS_16_BITS,
	VIEW_CLASS_8_BITS,
	VIEW_CLASS_RGTC1_RED,
	VIEW_CLASS_RGTC2_RG,
	VIEW_CLASS_BPTC_UNORM,
	VIEW_CLASS_BPTC_FLOAT,

	/* Always last */
	VIEW_CLASS_COUNT,
	VIEW_CLASS_UNDEFINED = VIEW_CLASS_COUNT
};

} // namespace TextureView

/** Helper class that implements various methods used across all Texture View tests. */
class TextureViewUtilities
{
public:
	/* Public type definitions */
	typedef glw::GLenum _original_texture_internalformat;
	typedef glw::GLenum _original_texture_target;
	typedef glw::GLenum _view_texture_internalformat;
	typedef glw::GLenum _view_texture_target;
	typedef std::pair<_original_texture_internalformat, _view_texture_internalformat> _internalformat_pair;
	typedef std::vector<glw::GLenum>		 _internalformats;
	typedef _internalformats::const_iterator _internalformats_const_iterator;
	typedef _internalformats::iterator		 _internalformats_iterator;
	typedef std::pair<_original_texture_target, _view_texture_target> _texture_target_pair;
	typedef std::vector<_internalformat_pair>				   _compatible_internalformat_pairs;
	typedef _compatible_internalformat_pairs::const_iterator   _compatible_internalformat_pairs_const_iterator;
	typedef std::vector<_texture_target_pair>				   _compatible_texture_target_pairs;
	typedef _compatible_texture_target_pairs::const_iterator   _compatible_texture_target_pairs_const_iterator;
	typedef std::vector<_internalformat_pair>				   _incompatible_internalformat_pairs;
	typedef _incompatible_internalformat_pairs::const_iterator _incompatible_internalformat_pairs_const_iterator;
	typedef _incompatible_internalformat_pairs::iterator	   _incompatible_internalformat_pairs_iterator;
	typedef std::vector<_texture_target_pair>				   _incompatible_texture_target_pairs;
	typedef _incompatible_texture_target_pairs::const_iterator _incompatible_texture_target_pairs_const_iterator;
	typedef _incompatible_texture_target_pairs::iterator	   _incompatible_texture_target_pairs_iterator;

	/* Public methods */

	static unsigned int getAmountOfComponentsForInternalformat(const glw::GLenum internalformat);

	static unsigned int getBlockSizeForCompressedInternalformat(const glw::GLenum internalformat);

	static void getComponentSizeForInternalformat(const glw::GLenum internalformat, unsigned int* out_rgba_size);

	static void getComponentSizeForType(const glw::GLenum type, unsigned int* out_rgba_size);

	static const char* getErrorCodeString(const glw::GLint error_code);

	static TextureView::_format getFormatOfInternalformat(const glw::GLenum internalformat);

	static glw::GLenum getGLFormatOfInternalformat(const glw::GLenum internalformat);

	static const char* getGLSLDataTypeForSamplerType(const TextureView::_sampler_type sampler_type,
													 const unsigned int				  n_components);

	static const char* getGLSLTypeForSamplerType(const TextureView::_sampler_type sampler_type);

	static _incompatible_internalformat_pairs getIllegalTextureAndViewInternalformatCombinations();

	static _incompatible_texture_target_pairs getIllegalTextureAndViewTargetCombinations();

	static _internalformats getInternalformatsFromViewClass(TextureView::_view_class view_class);

	static const char* getInternalformatString(const glw::GLenum internalformat);

	static _compatible_internalformat_pairs getLegalTextureAndViewInternalformatCombinations();

	static _compatible_texture_target_pairs getLegalTextureAndViewTargetCombinations();

	static void getMajorMinorVersionFromContextVersion(const glu::ContextType& context_type,
													   glw::GLint* out_major_version, glw::GLint* out_minor_version);

	static TextureView::_sampler_type getSamplerTypeForInternalformat(const glw::GLenum internalformat);

	static unsigned int getTextureDataSize(const glw::GLenum internalformat, const glw::GLenum type,
										   const unsigned int width, const unsigned int height);

	static const char* getTextureTargetString(const glw::GLenum texture_target);

	static glw::GLenum getTypeCompatibleWithInternalformat(const glw::GLenum internalformat);

	static TextureView::_view_class getViewClassForInternalformat(const glw::GLenum internalformat);

	static void initTextureStorage(const glw::Functions& gl, bool init_mutable_to, glw::GLenum texture_target,
								   glw::GLint texture_depth, glw::GLint texture_height, glw::GLint texture_width,
								   glw::GLenum texture_internalformat, glw::GLenum texture_format,
								   glw::GLenum texture_type, unsigned int n_levels_needed,
								   unsigned int n_cubemaps_needed, glw::GLint bo_id);

	static bool isInternalformatCompatibleForTextureView(glw::GLenum original_internalformat,
														 glw::GLenum view_internalformat);

	static bool isInternalformatCompressed(const glw::GLenum internalformat);

	static bool isInternalformatSRGB(const glw::GLenum internalformat);

	static bool isInternalformatSupported(glw::GLenum internalformat, const glw::GLint major_version,
										  const glw::GLint minor_version);

	static bool isLegalTextureTargetForTextureView(glw::GLenum original_texture_target,
												   glw::GLenum view_texture_target);
};

/**
 *   1. Make sure glGetTexParameterfv() and glGetTexParameteriv() report
 *      correct values for the following texture view-specific
 *      properties:
 *
 *      * GL_TEXTURE_IMMUTABLE_LEVELS; (in texture view-specific context
 *                                      only)
 *      * GL_TEXTURE_VIEW_MIN_LAYER;
 *      * GL_TEXTURE_VIEW_MIN_LEVEL;
 *      * GL_TEXTURE_VIEW_NUM_LAYERS;
 *      * GL_TEXTURE_VIEW_NUM_LEVELS;
 *
 *      These properties should be set to 0 (or GL_FALSE) by default.
 *      For textures created with glTexStorage() and glTextureView()
 *      functions, language from bullet (11) of GL_ARB_texture_view
 *      extension specification applies, as well as "Texture Views"
 *      section 8.18 of OpenGL 4.3 Core Profile specification.
 *
 *      The conformance test should check values of the aforementioned
 *      properties for the following objects:
 *
 *      1) mutable texture objects generated with glTexImage*D() calls,
 *         then bound to all supported texture targets (as described
 *         under (*) ).
 *      2) immutable texture objects generated with glTexStorage*D() calls
 *         for all supported texture targets (as described under (*) ).
 *      3) texture views using all texture targets (as described under
 *         (*) ) compatible with parent texture object's texture target.
 *         All texture targets should be considered for the parent object.
 *      4) texture views created on top of texture views described in 3).
 *
 *      For texture view cases, the test should verify that:
 *
 *      1) GL_TEXTURE_VIEW_NUM_LAYERS and GL_TEXTURE_VIEW_NUM_LEVELS are
 *         clamped as specified for cases where <numlayers> or <numlevels>
 *         arguments used for glTextureView() calls exceed beyond the
 *         original texture.
 *      2) GL_TEXTURE_VIEW_MIN_LEVEL is set to <minlevel> + value of
 *         GL_TEXTURE_VIEW_MIN_LEVEL from the original texture.
 *      3) GL_TEXTURE_VIEW_MIN_LAYER is set to <minlayer> + value of
 *         GL_TEXTURE_VIEW_MIN_LAYER from the original texture.
 *      4) GL_TEXTURE_VIEW_NUM_LEVELS is set to the lesser of <numlevels>
 *         and the value of original texture's GL_TEXTURE_VIEW_NUM_LEVELS
 *         minus <minlevels>.
 *      5) GL_TEXTURE_VIEW_NUM_LAYERS is set to the lesser of <numlayers>
 *         and the value of original texture's GL_TEXTURE_VIEW_NUM_LAYERS
 *         minus <minlayer>.
 *
 *      A single configuration of a texture object and a texture view
 *      for each valid texture target should be considered for the
 *      purpose of the test.
 *
 *      (*) Texture targets to use:
 *
 *      * GL_TEXTURE_1D;
 *      * GL_TEXTURE_1D_ARRAY;
 *      * GL_TEXTURE_2D;
 *      * GL_TEXTURE_2D_ARRAY;
 *      * GL_TEXTURE_2D_MULTISAMPLE;
 *      * GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
 *      * GL_TEXTURE_3D;
 *      * GL_TEXTURE_CUBE_MAP;
 *      * GL_TEXTURE_CUBE_MAP_ARRAY;
 *      * GL_TEXTURE_RECTANGLE;
 **/
class TextureViewTestGetTexParameter : public deqp::TestCase
{
public:
	/* Public methods */
	TextureViewTestGetTexParameter(deqp::Context& context);

	virtual ~TextureViewTestGetTexParameter()
	{
	}

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	enum _test_texture_type
	{
		TEST_TEXTURE_TYPE_NO_STORAGE_ALLOCATED,
		TEST_TEXTURE_TYPE_IMMUTABLE_TEXTURE_OBJECT,
		TEST_TEXTURE_TYPE_MUTABLE_TEXTURE_OBJECT,
		TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_IMMUTABLE_TEXTURE_OBJECT,
		TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_TEXTURE_VIEW,

		/* Always last */
		TEST_TEXTURE_TYPE_UNDEFINED
	};

	struct _test_run
	{
		glw::GLint expected_n_immutable_levels;
		glw::GLint expected_n_min_layer;
		glw::GLint expected_n_min_level;
		glw::GLint expected_n_num_layers;
		glw::GLint expected_n_num_levels;

		glw::GLuint parent_texture_object_id;
		glw::GLuint texture_view_object_created_from_immutable_to_id;
		glw::GLuint texture_view_object_created_from_view_to_id;

		glw::GLenum		   texture_target;
		_test_texture_type texture_type;

		/* Constructor */
		_test_run()
		{
			expected_n_immutable_levels = 0;
			expected_n_min_layer		= 0;
			expected_n_min_level		= 0;
			expected_n_num_layers		= 0;
			expected_n_num_levels		= 0;

			parent_texture_object_id						 = 0;
			texture_view_object_created_from_immutable_to_id = 0;
			texture_view_object_created_from_view_to_id		 = 0;

			texture_target = GL_NONE;
			texture_type   = TEST_TEXTURE_TYPE_UNDEFINED;
		}
	};

	typedef std::vector<_test_run>	 _test_runs;
	typedef _test_runs::const_iterator _test_runs_const_iterator;
	typedef _test_runs::iterator	   _test_runs_iterator;

	/* Private methods */
	void initTestRuns();

	/* Private fields */
	_test_runs m_test_runs;
};

/** Verify glTextureView() generates errors as described in the
 *  specification:
 *
 *  a) GL_INVALID_VALUE should be generated if <texture> is 0.
 *  b) GL_INVALID_OPERATION should be generated if <texture> is not
 *     a valid name returned by glGenTextures().
 *  c) GL_INVALID_OPERATION should be generated if <texture> has
 *     already been bound and given a target.
 *  d) GL_INVALID_VALUE should be generated if <origtexture> is not
 *     the name of a texture object.
 *  e) GL_INVALID_OPERATION error should be generated if <origtexture>
 *     is a mutable texture object.
 *  f) GL_INVALID_OPERATION error should be generated whenever the
 *     application tries to generate a texture view for a target
 *     that is incompatible with original texture's target. (as per
 *     table 8.20 from OpenGL 4.4 specification)
 *
 *    NOTE: All invalid original+view texture target combinations
 *          should be checked.
 *
 *  g) GL_INVALID_OPERATION error should be generated whenever the
 *     application tries to create a texture view, internal format
 *     of which can be found in table 8.21 of OpenGL 4.4
 *     specification, and the texture view's internal format is
 *     incompatible with parent object's internal format. Both
 *     textures and views should be used as parent objects for the
 *     purpose of the test.
 *
 *     NOTE: All invalid texture view internal formats should be
 *           checked for all applicable original object's internal
 *           formats.
 *
 *  h) GL_INVALID_OPERATION error should be generated whenever the
 *     application tries to create a texture view using an internal
 *     format that does not match the original texture's, and the
 *     original texture's internalformat cannot be found in table
 *     8.21 of OpenGL 4.4 specification.
 *
 *     NOTE: All required base, sized and compressed texture internal
 *           formats (as described in section 8.5.1 and table 8.14
 *           of OpenGL 4.4 specification) that cannot be found in
 *           table 8.21 should be considered for the purpose of this
 *           test.
 *
 *  i) GL_INVALID_VALUE error should be generated if <minlevel> is
 *     larger than the greatest level of <origtexture>.
 *  j) GL_INVALID_VALUE error should be generated if <minlayer> is
 *     larger than the greatest layer of <origtexture>.
 *  k) GL_INVALID_VALUE error should be generated if <target> is
 *     GL_TEXTURE_CUBE_MAP and <numlayers> is not 6.
 *  l) GL_INVALID_VALUE error should be generated if <target> is
 *     GL_TEXTURE_CUBE_MAP_ARRAY and <numlayers> is not a multiple
 *     of 6.
 *  m) GL_INVALID_VALUE error should be generated if <target> is
 *     GL_TEXTURE_1D and <numlayers> is not 1;
 *  n) GL_INVALID_VALUE error should be generated if <target> is
 *     GL_TEXTURE_2D and <numlayers> is not 1;
 *  o) GL_INVALID_VALUE error should be generated if <target> is
 *     GL_TEXTURE_3D and <numlayers> is not 1;
 *  p) GL_INVALID_VALUE error should be generated if <target> is
 *     GL_TEXTURE_RECTANGLE and <numlayers> is not 1;
 *  q) GL_INVALID_VALUE error should be generated if <target> is
 *     GL_TEXTURE_2D_MULTISAMPLE and <numlayers> is not 1;
 *  r) GL_INVALID_OPERATION error should be generated if <target> is
 *     GL_TEXTURE_CUBE_MAP and original texture's width does not
 *     match original texture's height for all levels.
 *  s) GL_INVALID_OPERATION error should be generated if <target> is
 *     GL_TEXTURE_CUBE_MAP_ARRAY and original texture's width does
 *     not match original texture's height for all levels.
 *
 *  NOTE: GL_INVALID_OPERATION error should also be generated for cases
 *        when any of the original texture's dimension is larger
 *        than the maximum supported corresponding dimension of the
 *        new target, but that condition can be very tricky to test
 *        in a portable manner. Hence, the conformance test will
 *        not verify if the error is generated as specified.
 *
 **/
class TextureViewTestErrors : public deqp::TestCase
{
public:
	/* Public methods */
	TextureViewTestErrors(deqp::Context& context);

	virtual ~TextureViewTestErrors()
	{
	}

	virtual void deinit();

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private fields */
	glw::GLuint m_bo_id;
	glw::GLuint m_reference_immutable_to_1d_id;
	glw::GLuint m_reference_immutable_to_2d_id;
	glw::GLuint m_reference_immutable_to_2d_array_id;
	glw::GLuint m_reference_immutable_to_2d_array_32_by_33_id;
	glw::GLuint m_reference_immutable_to_2d_multisample_id;
	glw::GLuint m_reference_immutable_to_3d_id;
	glw::GLuint m_reference_immutable_to_cube_map_id;
	glw::GLuint m_reference_immutable_to_cube_map_array_id;
	glw::GLuint m_reference_immutable_to_rectangle_id;
	glw::GLuint m_reference_mutable_to_2d_id;
	glw::GLuint m_test_modified_to_id_1;
	glw::GLuint m_test_modified_to_id_2;
	glw::GLuint m_test_modified_to_id_3;
	glw::GLuint m_view_bound_to_id;
	glw::GLuint m_view_never_bound_to_id;
};

/** Verify that sampling data from texture views, that use internal
 *  format which is compatible with the original texture's internal
 *  format, works correctly.
 *
 *  For simplicity, both the parent object and its corresponding
 *  view should use the same GL_RGBA8 internal format. Both texture
 *  and view should be used as parent objects for the purpose of
 *  the test.
 *
 *  The test should iterate over all texture targets enlisted in
 *  table 8.20 from OpenGL 4.4 specification, excluding GL_TEXTURE_BUFFER.
 *  For each such texture target, an immutable texture object that
 *  will be used as original object should be created. The object
 *  should be defined as follows:
 *
 *  a) For 1D texture targets, width of 4 should be used.
 *  b) For 2D texture targets, width and height of 4 should be used.
 *  c) For 3D texture targets, depth, width and height of 4 should
 *     be used;
 *  d) Cube-map texture objects should use a 4x4 face size;
 *  e) Arrayed 1D/2D/cube-map texture objects should have a depth
 *     of 4.
 *  f) Each slice/face/layer-face mip-map should be filled with
 *     a static color. If original texture is multi-sampled, subsequent
 *     samples should be assigned a different color. Exact R/G/B/A
 *     intensities are arbitrary but they must be unique across all
 *     mip-maps and samples considered for a particular iteration.
 *     Mip-maps should *not* be generated - instead, it is expected
 *     the test will fill them manually with content using API (for
 *     non-multisampled cases), or by means of a simple FS+VS
 *     program (for multisampled render targets).
 *
 *  For each such original texture object, texture views should be
 *  created for all compatible texture targets. These views should
 *  be configured as below:
 *
 *  * minlayer:  1 (for arrayed/cube-map/cube-map arrayed views),
 *               0 otherwise;
 *  * numlayers: 12 (for cube-map arrayed views),
 *               6 (for cube-map views)
 *               2 (for arrayed views),
 *               1 otherwise;
 *
 *  * minlevel:  1;
 *  * numlevels: 2;
 *
 *  For testing purposes, the test should use the following rendering
 *  stages (forming a program object):
 *
 *  - Vertex shader;
 *  - Tessellation control;
 *  - Tessellation evaluation;
 *  - Geometry shader;         (should output a triangle strip forming
 *                              a full-screen quad);
 *  - Fragment shader;
 *
 *  In each stage (excluding fragment shader), as many samples as
 *  available (for multisample views) OR a single texel should be
 *  sampled at central (0.5, 0.5, ...) location using textureLod().
 *  The data should then be compared with reference values and the
 *  validation result should be passed to next stage using
 *  an output variable. Subsequent stages should pass validation results
 *  from all previous stages to following stages until geometry shader
 *  stage is reached. The information should be captured by the test
 *  using XFB and verified.
 *  Fragment shader should output a green colour, if the sampling
 *  operation returned a valid set of colors for (U, V) location,
 *  corresponding to rasterized fragment location, red otherwise.
 *
 *  The test passes if the sampling operation worked correctly in
 *  all stages for all texture's internal format+view's internal
 *  format combinations.
 *
 **/
class TextureViewTestViewSampling : public deqp::TestCase
{
public:
	/* Public methods */
	TextureViewTestViewSampling(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods */
	virtual void deinit();

private:
	/* Private type declarations */
	struct _reference_color_storage
	{
		tcu::Vec4* data;

		unsigned int n_faces;
		unsigned int n_layers;
		unsigned int n_mipmaps;
		unsigned int n_samples;

		/** Constructor.
		 *
		 *  @param in_n_faces   Amount of faces to initialize the storage for.
		 *                      Must not be 0.
		 *  @param in_n_layers  Amount of layers to initialize the storage for.
		 *                      Must not be 0.
		 *  @param in_n_mipmaps Amount of mip-maps to initialize the storage for.
		 *                      Must not be 0.
		 *  @param in_n_samples Amount of samples to initialize the storage for.
		 *                      Must not be 0.
		 **/
		explicit _reference_color_storage(const unsigned int in_n_faces, const unsigned int in_n_layers,
										  const unsigned int in_n_mipmaps, const unsigned int in_n_samples)
		{
			DE_ASSERT(in_n_faces != 0);
			DE_ASSERT(in_n_layers != 0);
			DE_ASSERT(in_n_mipmaps != 0);
			DE_ASSERT(in_n_samples != 0);

			n_faces   = in_n_faces;
			n_layers  = in_n_layers;
			n_mipmaps = in_n_mipmaps;
			n_samples = in_n_samples;

			data = new tcu::Vec4[in_n_faces * in_n_layers * in_n_mipmaps * in_n_samples];
		}

		/** Destructor */
		~_reference_color_storage()
		{
			if (data != DE_NULL)
			{
				delete[] data;

				data = DE_NULL;
			}
		}
	};

	/* Private methods */
	void deinitIterationSpecificProgramAndShaderObjects();
	void deinitPerSampleFillerProgramAndShaderObjects();
	void deinitTextureObjects();
	bool executeTest();
	void initIterationSpecificProgramObject();
	void initParentTextureContents();
	void initPerSampleFillerProgramObject();
	void initTest();

	void initTextureObject(bool is_view_texture, glw::GLenum texture_target, glw::GLenum view_texture_target);

	tcu::Vec4 getRandomReferenceColor();

	tcu::Vec4 getReferenceColor(unsigned int n_layer, unsigned int n_face, unsigned int n_mipmap,
								unsigned int n_sample);

	glw::GLint getMaxConformantSampleCount(glw::GLenum target, glw::GLenum internalFormat);

	void resetReferenceColorStorage(unsigned int n_layers, unsigned int n_faces, unsigned int n_mipmaps,
									unsigned int n_samples);

	void setReferenceColor(unsigned int n_layer, unsigned int n_face, unsigned int n_mipmap, unsigned int n_sample,
						   tcu::Vec4 color);

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLint  m_po_lod_location;
	glw::GLint  m_po_n_face_location;
	glw::GLint  m_po_reference_colors_location;
	glw::GLint  m_po_texture_location;
	glw::GLint  m_po_z_float_location;
	glw::GLint  m_po_z_int_location;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;

	glw::GLuint m_per_sample_filler_fs_id;
	glw::GLuint m_per_sample_filler_gs_id;
	glw::GLuint m_per_sample_filler_po_id;
	glw::GLint  m_per_sample_filler_po_layer_id_location;
	glw::GLint  m_per_sample_filler_po_reference_colors_location;
	glw::GLuint m_per_sample_filler_vs_id;

	glw::GLuint m_result_to_id;
	glw::GLuint m_to_id;
	glw::GLuint m_view_to_id;

	glw::GLuint m_fbo_id;
	glw::GLuint m_vao_id;

	glw::GLint m_max_color_texture_samples_gl_value;

	glw::GLuint m_iteration_parent_texture_depth;
	glw::GLuint m_iteration_parent_texture_height;
	glw::GLuint m_iteration_parent_texture_n_levels;
	glw::GLuint m_iteration_parent_texture_n_samples;
	glw::GLenum m_iteration_parent_texture_target;
	glw::GLuint m_iteration_parent_texture_width;
	glw::GLuint m_iteration_view_texture_minlayer;
	glw::GLuint m_iteration_view_texture_numlayers;
	glw::GLuint m_iteration_view_texture_minlevel;
	glw::GLuint m_iteration_view_texture_numlevels;
	glw::GLenum m_iteration_view_texture_target;

	const glw::GLuint m_reference_texture_depth;
	const glw::GLuint m_reference_texture_height;
	const glw::GLuint m_reference_texture_n_mipmaps;
	const glw::GLuint m_reference_texture_width;

	_reference_color_storage* m_reference_color_storage;
	unsigned char*			  m_result_data;
};

/** Verify view class functionality.
 *
 *  Consider all view classes presented in table 8.20 of OpenGL 4.4
 *  Specification. For each view class, consider all internal format
 *  combinations for the purpose of the test.
 *
 *  For each internal format, a texture object of specified internal
 *  format and of 2x2 base mip-map resolution should be used.
 *
 *  For each internal format, a program object consisting of a vertex
 *  shader stage should be used. The shader should sample all 4 texels
 *  of the view and store retrieved data in output variables.
 *  These values should then be XFBed to the test.
 *
 *  The test passes if all retrieved values for all pairs considered
 *  are found to be valid.
 *
 **/
class TextureViewTestViewClasses : public deqp::TestCase
{
public:
	/* Public methods */
	TextureViewTestViewClasses(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods */
	virtual void deinit();

private:
	/* Private methods */
	void getComponentDataForByteAlignedInternalformat(const unsigned char* data, const unsigned int n_components,
													  const unsigned int*		 component_sizes,
													  const TextureView::_format format, void* result);

	void initBufferObject(glw::GLenum texture_internalformat, glw::GLenum view_internalformat);

	void initProgramObject(glw::GLenum texture_internalformat, glw::GLenum view_internalformat);

	void initTest();

	void initTextureObject(bool should_init_parent_texture, glw::GLenum texture_internalformat,
						   glw::GLenum view_internalformat);

	void verifyResultData(glw::GLenum texture_internalformat, glw::GLenum view_internalformat,
						  const unsigned char* texture_data_ptr, const unsigned char* view_data_ptr);

	/* Private fields */
	glw::GLuint m_bo_id;
	glw::GLuint m_po_id;
	glw::GLuint m_to_id;
	glw::GLuint m_to_temp_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_view_to_id;
	glw::GLuint m_vs_id;

	unsigned char* m_decompressed_mipmap_data;
	unsigned char* m_mipmap_data;

	unsigned int	   m_bo_size;
	bool			   m_has_test_failed;
	const unsigned int m_texture_height;
	const glw::GLenum  m_texture_unit_for_parent_texture;
	const glw::GLenum  m_texture_unit_for_view_texture;
	const unsigned int m_texture_width;
	unsigned int	   m_view_data_offset;
};

/**
 *  Verify view/parent texture coherency.
 *
 *  Consider an original 2D texture of 64x64 base mip-map resolution,
 *  using GL_RGBA8 internal format, filled with horizontal linear
 *  gradient from (0.0, 0.1, 1.0, 1.0) to (1.0, 0.9, 0.0, 0.0).
 *  All subsequent mip-maps are to be generated by glGenerateMipmap().
 *
 *  A 2D texture view should be generated from this texture, using
 *  mipmaps from levels 1 to 2 (inclusive).
 *
 *  The test should verify the following scenarios are handled
 *  correctly by GL implementation:
 *
 *  1) glTexSubImage2D() should be used on the view to replace portion
 *     of the gradient with a static color. A vertex shader should
 *     then be used to sample the parent texture at central
 *     location of that sub-region and verify it has been replaced
 *     with the static color. Result of the comparison (true/false)
 *     should be XFBed and verified by the test; *No* memory barrier
 *     is to be used between the first two steps.
 *  2) The view should be updated as in step 1), with glTexSubImage2D()
 *     being replaced with glBlitFramebuffer(). A texture filled
 *     with a static color should be used as a source for the blitting
 *     operation.
 *  3) A program object should be used to fill the view with
 *     reversed (1.0, 0.9, 0.0, 0.0)->(0.0, 0.1, 1.0, 1.0) gradient.
 *     Contents of the parent texture should then be validated with
 *     another program object and the result should be verified by
 *     XFBing the comparison outcome, similar to what has been
 *     described for step 1). Again, *no* memory barrier is to be
 *     used in-between.
 *  4) The view should be bound to an image unit. The contents of
 *     that view should then be completely replaced with the reversed
 *     gradient by doing a sufficient amount of writes, assuming each vertex
 *     shader invocation performs a single store operation in an
 *     unique location. A GL_TEXTURE_FETCH_BARRIER_BIT memory
 *     barrier should be issued. The contents of the parent texture
 *     should then be verified, as described in step 1) and 3).
 *  5) The view should be updated as in step 4). However, this time
 *     a GL_TEXTURE_UPDATE_BARRIER_BIT memory barrier should be issued.
 *     The contents of the parent texture should then be read with
 *     a glGetTexImage() call and verified by the test.
 *
 *     (NOTE: cases 4) and 5) should only be executed on implementations
 *            reporting GL_ARB_shader_image_load_store extension
 *            support, owing to lack of glMemoryBarrier() support
 *            in OpenGL 4.0)
 *
 *  6), 7), 8), 9) Execute tests 1), 2), 3), 4), 5), replacing "texture
 *                 views" with "parent textures" and "parent textures"
 *                 with "texture views".
 */
class TextureViewTestCoherency : public deqp::TestCase
{
public:
	/* Public methods */
	TextureViewTestCoherency(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	enum _barrier_type
	{
		BARRIER_TYPE_NONE,
		BARRIER_TYPE_TEXTURE_FETCH_BARRIER_BIT,
		BARRIER_TYPE_TEXTURE_UPDATE_BUFFER_BIT
	};

	enum _texture_type
	{
		TEXTURE_TYPE_PARENT_TEXTURE,
		TEXTURE_TYPE_TEXTURE_VIEW,
		TEXTURE_TYPE_IMAGE
	};

	enum _verification_mean
	{
		VERIFICATION_MEAN_PROGRAM,
		VERIFICATION_MEAN_GLGETTEXIMAGE
	};

	/* Private methods */
	void checkAPICallCoherency(_texture_type texture_type, bool should_use_glTexSubImage2D);

	void checkProgramWriteCoherency(_texture_type texture_type, bool should_use_images, _barrier_type barrier_type,
									_verification_mean verification_mean);

	unsigned char* getHorizontalGradientData() const;

	void getReadPropertiesForTextureType(_texture_type texture_type, glw::GLuint* out_to_id,
										 unsigned int* out_read_lod) const;

	unsigned char* getStaticColorTextureData(unsigned int width, unsigned int height) const;

	void getWritePropertiesForTextureType(_texture_type texture_type, glw::GLuint* out_to_id, unsigned int* out_width,
										  unsigned int* out_height) const;

	void initBufferObjects();
	void initFBO();
	void initPrograms();
	void initTextureContents();
	void initTextures();
	void initVAO();

	/* Private fields */
	bool		m_are_images_supported;
	glw::GLuint m_bo_id;
	glw::GLuint m_draw_fbo_id;
	glw::GLuint m_gradient_verification_po_id;
	glw::GLint  m_gradient_verification_po_sample_exact_uv_location;
	glw::GLint  m_gradient_verification_po_lod_location;
	glw::GLint  m_gradient_verification_po_texture_location;
	glw::GLuint m_gradient_verification_vs_id;
	glw::GLint  m_gradient_image_write_image_size_location;
	glw::GLuint m_gradient_image_write_po_id;
	glw::GLuint m_gradient_image_write_vs_id;
	glw::GLuint m_gradient_write_po_id;
	glw::GLuint m_gradient_write_fs_id;
	glw::GLuint m_gradient_write_vs_id;
	glw::GLuint m_read_fbo_id;
	glw::GLuint m_static_to_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_view_to_id;
	glw::GLint  m_verification_po_expected_color_location;
	glw::GLint  m_verification_po_lod_location;
	glw::GLuint m_verification_po_id;
	glw::GLuint m_verification_vs_id;

	unsigned char m_static_color_byte[4 /* rgba */];
	float		  m_static_color_float[4 /* rgba */];

	const unsigned int m_static_texture_height;
	const unsigned int m_static_texture_width;
	const unsigned int m_texture_height;
	const unsigned int m_texture_n_components;
	const unsigned int m_texture_n_levels;
	const unsigned int m_texture_width;
};

/** Verify GL_TEXTURE_BASE_LEVEL and GL_TEXTURE_MAX_LEVEL are
 *  interpreted relative to the view, not to the original data
 *  store.
 *
 *  Consider original 2D texture of 64x64 base mip-map resolution,
 *  using GL_RGBA8 internal format, filled with:
 *
 *  * horizontal linear gradient from (0.0, 0.1, 0,2, 0.3) to
 *    (1.0, 0.9, 0.8, 0.7) at level 0;
 *  * horizontal linear gradient from (0.1, 0.2, 0.3, 0.4) to
 *    (0.9, 0.8, 0.7, 0.6) at level 1;
 *  * horizontal linear gradient from (0.2, 0.3, 0.4, 0.5) to
 *    (0.8, 0.7, 0.6, 0.5) at level 2;
 *  * ..and so on.
 *
 *  GL_TEXTURE_BASE_LEVEL of the texture should be set at 2,
 *  GL_TEXTURE_MAX_LEVEL of the texture should be set at 4.
 *
 *  A 2D texture view should be generated from this texture, using
 *  mipmaps from 0 to 5 (inclusive).
 *
 *  GL_TEXTURE_BASE_LEVEL of the view should be set at 1,
 *  GL_TEXTURE_MAX_LEVEL of the view should be set at 2.
 *
 *  The test should perform the following:
 *
 *  1) First, a FS+VS program should be executed twice. The vertex
 *     shader should output 4 vertices forming a triangle-strip, forming
 *     a quad spanning from (-1, -1, 0, 1) to (1, 1, 0, 1). Each
 *     vertex should be assigned a corresponding UV location.
 *     The fragment shader should sample input texture view with
 *     textureLod() at fragment-specific UV using LOD 0 in the
 *     first run, and LOD 1 in the second run.
 *     The sampled vec4 should be written into a draw buffer, to which
 *     a texture has been attached of exactly the same resolution
 *     and internal format as the input texture view's mip-map being
 *     processed at the time of invocation.
 *
 *     This pass should provide us with LOD 0 and LOD 1 texture data,
 *     as configured with GL_TEXTURE_BASE_LEVEL and
 *     GL_TEXTURE_MAX_LEVEL view parameters.
 *
 *  2) Having executed the aforementioned program, the test should
 *     download the textures that step 1 has rendered to using
 *     glGetTexImage() and verify correct mipmaps have been sampled
 *     by textureLod().
 *
 *  Test passes if the retrieved texture data is valid.
 **/
class TextureViewTestBaseAndMaxLevels : public deqp::TestCase
{
public:
	/* Public methods */
	TextureViewTestBaseAndMaxLevels(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initProgram();
	void initTest();
	void initTextures();

	/* Private fields */
	const unsigned int m_texture_height;
	const unsigned int m_texture_n_components;
	const unsigned int m_texture_n_levels;
	const unsigned int m_texture_width;
	const unsigned int m_view_height;
	const unsigned int m_view_width;

	unsigned char* m_layer_data_lod0;
	unsigned char* m_layer_data_lod1;

	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_po_id;
	glw::GLint  m_po_lod_index_uniform_location;
	glw::GLint  m_po_to_sampler_uniform_location;
	glw::GLuint m_result_to_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_view_to_id;
	glw::GLuint m_vs_id;
};

/** Verify texture view reference counting is implemented correctly.
 *
 *  Parent texture object A should be an immutable 2D texture of
 *  64x64 resolution and of GL_RGBA8 internalformat. Each mip-map
 *  should be filled with a different static color.
 *
 *  View B should be created from texture A, and view C should be
 *  instantiated from view B. The views should use the same texture
 *  target and internalformat as texture A. <minlayer> and <minlevel>
 *  values of 0 should be used for view generation. All available
 *  texture layers and levels should be used for the views.
 *
 *  The test should:
 *
 *  a) Sample the texture and both views, make sure correct data can
 *     be sampled in VS stage;
 *  b) Delete texture A;
 *  c) Sample both views, make sure correct data can be sampled in
 *     VS stage;
 *  d) Delete view B;
 *  e) Sample view C, make sure correct data can be sampled in VS
 *     stage;
 *
 *  A program object consisting only of vertex shader should be used
 *  by the test. The shader should sample all mip-maps of the bound
 *  texture at (0.5, 0.5) and compare the retrieved texels against
 *  reference values. Comparison outcome should be XFBed back to
 *  the test implementation.
 *
 *  Test passes if the sampling operation is reported to work
 *  correctly for all steps.
 *
 **/
class TextureViewTestReferenceCounting : public deqp::TestCase
{
public:
	/* Public methods */
	TextureViewTestReferenceCounting(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	struct _norm_vec4
	{
		unsigned char rgba[4];

		/* Constructor
		 *
		 * @param r Red value to store for the component
		 * @param g Green value to store for the component
		 * @param b Blue value to store for the component
		 * @param a Alpha value to store for the component.
		 */
		explicit _norm_vec4(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
		{
			rgba[0] = r;
			rgba[1] = g;
			rgba[2] = b;
			rgba[3] = a;
		}
	};

	/* Private methods */
	void initProgram();
	void initTest();
	void initTextures();
	void initXFB();

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_parent_to_id;
	glw::GLuint m_po_id;
	glw::GLint  m_po_expected_texel_uniform_location;
	glw::GLint  m_po_lod_uniform_location;
	glw::GLuint m_vao_id;
	glw::GLuint m_view_to_id;
	glw::GLuint m_view_view_to_id;
	glw::GLuint m_vs_id;

	const glw::GLuint m_texture_height;
	const glw::GLuint m_texture_n_levels;
	const glw::GLuint m_texture_width;

	std::vector<_norm_vec4> m_mipmap_colors;
};

/** Group class for texture view conformance tests */
class TextureViewTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	TextureViewTests(deqp::Context& context);
	virtual ~TextureViewTests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	TextureViewTests(const TextureViewTests&);
	TextureViewTests& operator=(const TextureViewTests&);
};

} /* gl4cts namespace */

#endif // _GL4CTEXTUREVIEWTESTS_HPP
