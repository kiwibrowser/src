#ifndef _ES31CTEXTURESTORAGEMULTISAMPLEGETTEXLEVELPARAMETERIFVTESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLEGETTEXLEVELPARAMETERIFVTESTS_HPP
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
 */ /*!
 * \file  es31cTextureStorageMultisampleGetTexLevelParameterifvTests.hpp
 * \brief Declares test classes for glGetTexLevelParameter{i, f}v() conformance
 *        tests. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"
#include "glwEnums.hpp"

namespace glcts
{
/** Test case: Verifies glGetTexLevelParameter{if}v() entry-points work correctly for
 *             all ES3.1 texture targets.
 **/
class MultisampleTextureGetTexLevelParametervFunctionalTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetTexLevelParametervFunctionalTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	typedef struct texture_properties
	{
		glw::GLint	 depth;
		glw::GLboolean fixedsamplelocations;
		glw::GLenum	format;
		glw::GLint	 height;
		glw::GLenum	internalformat;
		bool		   is_2d_texture;
		bool		   is_cm_texture;
		bool		   is_multisample_texture;
		glw::GLint	 samples;
		glw::GLenum	target;
		glw::GLuint*   to_id_ptr;
		glw::GLenum	type;
		glw::GLenum	width;

		glw::GLint				expected_compressed;
		glw::GLint				expected_texture_alpha_size;
		std::vector<glw::GLint> expected_texture_alpha_types;
		glw::GLint				expected_texture_blue_size;
		std::vector<glw::GLint> expected_texture_blue_types;
		glw::GLint				expected_texture_depth;
		glw::GLint				expected_texture_depth_size;
		std::vector<glw::GLint> expected_texture_depth_types;
		glw::GLint				expected_texture_fixed_sample_locations;
		glw::GLint				expected_texture_green_size;
		std::vector<glw::GLint> expected_texture_green_types;
		glw::GLint				expected_texture_height;
		glw::GLint				expected_texture_internal_format;
		glw::GLint				expected_texture_red_size;
		std::vector<glw::GLint> expected_texture_red_types;
		glw::GLint				expected_texture_samples;
		glw::GLint				expected_texture_shared_size;
		glw::GLint				expected_texture_stencil_size;
		glw::GLint				expected_texture_width;

		texture_properties()
			: depth(0)
			, fixedsamplelocations(GL_FALSE)
			, format(0)
			, height(0)
			, internalformat(0)
			, is_2d_texture(false)
			, is_cm_texture(false)
			, is_multisample_texture(false)
			, samples(0)
			, target(GL_NONE)
			, to_id_ptr(NULL)
			, type(0)
			, width(0)
			, expected_compressed(0)
			, expected_texture_alpha_size(0)
			, expected_texture_blue_size(0)
			, expected_texture_depth(0)
			, expected_texture_depth_size(0)
			, expected_texture_fixed_sample_locations(0)
			, expected_texture_green_size(0)
			, expected_texture_height(0)
			, expected_texture_internal_format(0)
			, expected_texture_red_size(0)
			, expected_texture_samples(0)
			, expected_texture_shared_size(0)
			, expected_texture_stencil_size(0)
			, expected_texture_width(0)
		{
			/* Left blank intentionally */
		}
	} _texture_properties;

	/* Private variables */
	glw::GLboolean gl_oes_texture_storage_multisample_2d_array_supported;
	glw::GLuint	to_2d;
	glw::GLuint	to_2d_array;
	glw::GLuint	to_2d_multisample;
	glw::GLuint	to_2d_multisample_array;
	glw::GLuint	to_3d;
	glw::GLuint	to_cubemap;
};

/** Test case: Verifies glGetTexLevelParameter{if}v() entry-points work correctly for
 *             maximum LOD.
 *
 **/
class MultisampleTextureGetTexLevelParametervWorksForMaximumLodTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetTexLevelParametervWorksForMaximumLodTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLboolean gl_oes_texture_storage_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: GL_INVALID_ENUM is reported for invalid texture target. */
class MultisampleTextureGetTexLevelParametervInvalidTextureTargetRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetTexLevelParametervInvalidTextureTargetRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLfloat float_data;
	glw::GLint   int_data;
};

/** Test case: GL_INVALID_VALUE is reported for invalid value argument. */
class MultisampleTextureGetTexLevelParametervInvalidValueArgumentRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetTexLevelParametervInvalidValueArgumentRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLfloat float_data;
	glw::GLint   int_data;
};

/** Test case: GL_INVALID_VALUE is reported for negative <lod> argument value. */
class MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLfloat float_data;
	glw::GLint   int_data;
	glw::GLuint  to_id;
};

} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLEGETTEXLEVELPARAMETERIFVTESTS_HPP
