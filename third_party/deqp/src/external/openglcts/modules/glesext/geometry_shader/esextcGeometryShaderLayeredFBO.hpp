#ifndef _ESEXTCGEOMETRYSHADERLAYEREDFBO_HPP
#define _ESEXTCGEOMETRYSHADERLAYEREDFBO_HPP
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

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/** Class which implements methods shared by more than one conformance test related to
 *  layered framebuffers.
 */
class GeometryShaderLayeredFBOShared
{
public:
	static bool checkFBOCompleteness(tcu::TestContext& test_context, const glw::Functions& gl, glw::GLenum fbo_id,
									 glw::GLenum expected_completeness_status);

	static void deinitFBOs(const glw::Functions& gl, const glw::GLuint* fbo_ids);

	static void deinitTOs(const glw::Functions& gl, const glw::GLuint* to_ids);

	static void initFBOs(const glw::Functions& gl, glw::glFramebufferTextureFunc pGLFramebufferTexture,
						 const glw::GLuint* to_ids, glw::GLuint* out_fbo_ids);

	static void initTOs(const glw::Functions& gl, glw::glTexStorage3DMultisampleFunc pGLTexStorage3DMultisample,
						glw::GLuint* out_to_ids);

	static const unsigned int n_shared_fbo_ids;
	static const unsigned int n_shared_to_ids;
	static const glw::GLuint  shared_to_depth;
	static const glw::GLuint  shared_to_height;
	static const glw::GLuint  shared_to_width;
};

/** Implementation of Test 21.1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  Reassure GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT framebuffer
 *  incompleteness status is reported for framebuffer objects, for which some
 *  of the attachments are layered and some aren't.
 *
 *  Category: API;
 *            Dependency with OES_texture_storage_multisample_2d_array.
 *
 *  1. Create four framebuffer objects (A, B, C, D) and set up:
 *
 *  - an immutable 2D texture object A (using any color-renderable
 *    internalformat);
 *  - an immutable 2D texture object A' (using any depth-renderable
 *    internalformat);
 *  - an immutable 2D array texture object B  (using any color-renderable
 *    internalformat);
 *  - an immutable 3D texture object C  (using any color-renderable
 *    internalformat);
 *  - an immutable cube-map texture object D  (using any color-renderable
 *    internalformat);
 *  - an immutable multisample 2D texture object E (using any
 *    color-renderable internalformat and any number of samples > 1)
 *  - an immutable multisample 2D array texture object F (using any
 *    color-renderable internalformat and any number of samples > 1)
 *
 *  Resolution of all textures should be set to 4x4 (for each face, layer,
 *  slice, etc.). Use depth of 4 if appropriate.
 *
 *  2.Set up a layered FBO A using glFramebufferTextureEXT() calls, so that:
 *
 *  - base mip-map of texture object A is bound to its color attachment 0;
 *  - level 0 of texture object B is bound to color attachment 2;
 *  - level 0 of texture object A' is bound to its depth attachment;
 *  - draw buffers are set to use color attachments 0 and 2.
 *
 *  3. Set up a layered FBO B using glFramebufferTextureEXT() calls, so that:
 *
 *  - level 0 of texture object C is bound to color attachment 0;
 *  - mip-map at level 0 of texture object A is bound to color attachment 1;
 *  - layer 3 of the cube-map texture object is bound to color attachment 2;
 *  - draw buffers are set to use color attachments 0, 1 and 2.
 *
 *  4. Set up a layered FBO C using glFramebufferTextureEXT() calls so that:
 *
 *  - layer 5 of the cube-map texture object is bound to color attachment 0;
 *  - level 0 of texture object A' is bound to the depth attachment;
 *  - the only draw buffer used is color attachment 0.
 *
 *  5. Set up a layered FBO D using glFramebufferTextureEXT() calls so that:
 *
 *  - base mip-map of multisample texture object E is bound to color
 *    attachment 0;
 *  - level 0 of texture object F is bound to color attachment 1;
 *  - draw buffers are set to use color attachments 0 and 1.
 *
 *  6. Make sure that, once each of these FBOs is bound,
 *     GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT FBO status is reported.
 *
 **/
class GeometryShaderIncompleteLayeredFBOTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderIncompleteLayeredFBOTest(Context& context, const ExtParameters& extParams, const char* name,
										   const char* description);

	virtual ~GeometryShaderIncompleteLayeredFBOTest()
	{
	}

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private functions */

	/* Private variables */
	glw::GLuint* m_fbo_ids;
	glw::GLuint* m_to_ids;
};

/** Implementation of Test 21.2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. Make sure that values reported layered FBO attachment properties are
 *     valid.
 *
 *     Category: API.
 *
 *     1. Use FBO and textures as described in test case 21.1.
 *     2. Make sure that for each of the FBO attachments, value reported for
 *        GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT is as expected.
 *
 **/
class GeometryShaderIncompleteLayeredAttachmentsTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderIncompleteLayeredAttachmentsTest(Context& context, const ExtParameters& extParams, const char* name,
												   const char* description);

	virtual ~GeometryShaderIncompleteLayeredAttachmentsTest()
	{
	}

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private functions */

	/* Private variables */
	glw::GLuint* m_fbo_ids;
	glw::GLuint* m_to_ids;
};

/* Implementation of "Group 26", test 1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. glFramebufferTextureEXT() should not accept invalid targets.
 *
 *     Category: API;
 *               Coverage;
 *               Negative Test.
 *
 *     Calling glFramebufferTextureEXT() for target = GL_TEXTURE_3D should
 *     result in GL_INVALID_ENUM error.
 *
 */
class GeometryShaderFramebufferTextureInvalidTarget : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFramebufferTextureInvalidTarget(Context& context, const ExtParameters& extParams, const char* name,
												  const char* description);

	virtual ~GeometryShaderFramebufferTextureInvalidTarget()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fbo_id;
	glw::GLuint m_to_id;
};

/* Implementation of "Group 26", test 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. glFramebufferTextureEXT() should generate GL_INVALID_OPERATION error if
 *     there is no framebuffer object bound to requested framebuffer target.
 *
 *     Category: API;
 *               Coverage;
 *               Negative Test.
 *
 *     Try to issue glFramebufferTextureEXT() calls for  GL_DRAW_FRAMEBUFFER,
 *     GL_READ_FRAMEBUFFER and GL_FRAMEBUFFER framebuffer targets, assuming the
 *     texture object these calls refer to exists.
 *
 *     Default binding for both GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER
 *     framebuffer target is zero, so after each of these calls
 *     GL_INVALID_OPERATION error should be generated.
 *
 */
class GeometryShaderFramebufferTextureNoFBOBoundToTarget : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFramebufferTextureNoFBOBoundToTarget(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description);

	virtual ~GeometryShaderFramebufferTextureNoFBOBoundToTarget()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_to_id;
};

/* Implementation of "Group 26", test 3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. glFramebufferTextureEXT() should generate GL_INVALID_ENUM if attachment
 *     argument is set to invalid value.
 *
 *     Category: API;
 *               Coverage;
 *               Negative Test.
 *
 *     Try to issue glFramebufferTextureEXT() call for GL_COLOR_ATTACHMENTi
 *     where i is equal to value reported for GL_MAX_COLOR_ATTACHMENTS.
 *
 */
class GeometryShaderFramebufferTextureInvalidAttachment : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFramebufferTextureInvalidAttachment(Context& context, const ExtParameters& extParams,
													  const char* name, const char* description);

	virtual ~GeometryShaderFramebufferTextureInvalidAttachment()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fbo_id;
	glw::GLuint m_to_id;
};

/* Implementation of "Group 26", test 4 from CTS_EXT_geometry_shader. Description follows:
 *
 *  4. glFramebufferTextureEXT() should generate GL_INVALID_VALUE if texture is
 *     not the name of a texture object.
 *
 *     Category: API;
 *               Coverage;
 *               Negative Test.
 *
 *     Try to issue glFramebufferTextureEXT() call for non-existing texture ids
 *     1, 10 and 100, with a FBO bound to specified FBO target. GL_INVALID_VALUE
 *     error should be reported for each attempt.
 *
 */
class GeometryShaderFramebufferTextureInvalidValue : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFramebufferTextureInvalidValue(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description);

	virtual ~GeometryShaderFramebufferTextureInvalidValue()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fbo_id;
};

/* Implementation of "Group 26", test 5 from CTS_EXT_geometry_shader. Description follows:
 *
 *  5. glFramebufferTextureEXT() should generate GL_INVALID_OPERATION if level
 *     argument is an invalid texture level number.
 *
 *     Category: API;
 *               Coverage;
 *               Negative Test.
 *
 *     Consider a 2D texture array object A and a 3D texture array object B
 *     (each of 4x4x4 resolution), and a framebuffer object C. Base mip-map
 *     should be filled with any data, and descendant mip-maps should be
 *     generated with glGenerateMipmap() call.
 *
 *     glFramebufferTextureEXT() should fail with GL_INVALID_OPERATION error
 *     when used for the FBO C and either of the textures and for level = 3.
 *
 */
class GeometryShaderFramebufferTextureInvalidLevelNumber : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFramebufferTextureInvalidLevelNumber(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description);

	virtual ~GeometryShaderFramebufferTextureInvalidLevelNumber()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint   m_fbo_id;
	glw::GLubyte* m_texels;
	glw::GLushort m_tex_depth;
	glw::GLushort m_tex_height;
	glw::GLushort m_tex_width;
	glw::GLuint   m_to_2d_array_id;
	glw::GLuint   m_to_3d_id;
};

/* Implementation of "Group 26", test 6 from CTS_EXT_geometry_shader. Description follows:
 *
 *  6. glFramebufferTextureEXT() should generate GL_INVALID_OPERATION if texture
 *     argument refers to a buffer texture.
 *
 *     Category: API;
 *               Coverage;
 *               Dependency on EXT_texture_buffer;
 *               Negative Test.
 *
 *     Consider a framebuffer object A, buffer object B and a buffer texture C
 *     using buffer object B as a data source.
 *
 *     glFramebufferTextureEXT() should fail with GL_INVALID_OPERATION error
 *     when used for the FBO A and texture argument relates to ID of buffer
 *     texture C.
 *
 */
class GeometryShaderFramebufferTextureArgumentRefersToBufferTexture : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFramebufferTextureArgumentRefersToBufferTexture(Context& context, const ExtParameters& extParams,
																  const char* name, const char* description);

	virtual ~GeometryShaderFramebufferTextureArgumentRefersToBufferTexture()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fbo_id;
	glw::GLuint m_tbo_id;
	glw::GLuint m_tex_height;
	glw::GLuint m_tex_width;
	glw::GLint* m_texels;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERLAYEREDFBO_HPP
