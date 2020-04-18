#ifndef _ESEXTCTESSELLATIONSHADERBARRIER_HPP
#define _ESEXTCTESSELLATIONSHADERBARRIER_HPP
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

#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/* Groups all barrier tests */
class TessellationShaderBarrierTests : public TestCaseGroupBase
{
public:
	/* Public methods */
	TessellationShaderBarrierTests(Context& context, const ExtParameters& extParams);

	virtual void init(void);
};

/** Base class for all tests that check the memory barrier functionality.
 **/
class TessellationShaderBarrierTestCase : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderBarrierTestCase(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description);

	virtual ~TessellationShaderBarrierTestCase(void)
	{
	}

	virtual void		  deinit();
	virtual void		  initTest(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected methods */
	virtual void getDrawCallArgs(glw::GLenum* out_mode, glw::GLint* out_count, glw::GLenum* out_tf_mode,
								 glw::GLint* out_n_patch_vertices, glw::GLint* out_n_instances) = 0;

	virtual const char* getTCSCode()	   = 0;
	virtual const char* getTESCode()	   = 0;
	virtual const char* getVSCode()		   = 0;
	virtual int			getXFBBufferSize() = 0;
	virtual void getXFBProperties(int* out_n_names, const char*** out_names) = 0;
	virtual bool verifyXFBBuffer(const void* data) = 0;

	/* Protected variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tcs_id;
	glw::GLuint m_tes_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/** Implementation of Test Case 22.1
 *
 *  Make sure that a barrier used in a tessellation control shader synchronizes
 *  all instances working on the same patch. Tests the following scenario:
 *
 * * invocation A can correctly read a per-vertex & per-patch attribute
 *    modified by invocation B after a barrier() call;
 **/
class TessellationShaderBarrier1 : public TessellationShaderBarrierTestCase
{
public:
	/* Public methods */
	TessellationShaderBarrier1(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderBarrier1(void)
	{
	}

protected:
	/* Protected methods */
	void getDrawCallArgs(glw::GLenum* out_mode, glw::GLint* out_count, glw::GLenum* out_tf_mode,
						 glw::GLint* out_n_patch_vertices, glw::GLint* out_n_instances);

	const char* getTCSCode();
	const char* getTESCode();
	const char* getVSCode();
	int			getXFBBufferSize();
	void getXFBProperties(int* out_n_names, const char*** out_names);
	bool verifyXFBBuffer(const void* data);

private:
	/* Private fields */
	unsigned int	   m_n_input_vertices;
	const unsigned int m_n_result_vertices;
};

/** Implementation of Test Case 22.2
 *
 *  Make sure that a barrier used in a tessellation control shader synchronizes
 *  all instances working on the same patch. Tests the following scenario:
 *
 * * invocation A writes to a per-patch output. A barrier is then issued,
 *   after which invocation B overwrites the same per-patch output. One more
 *   barrier is issued, after which invocation A should be able to read this
 *   output correctly.
 **/
class TessellationShaderBarrier2 : public TessellationShaderBarrierTestCase
{
public:
	/* Public methods */
	TessellationShaderBarrier2(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderBarrier2(void)
	{
	}

protected:
	/* Protected methods */
	void getDrawCallArgs(glw::GLenum* out_mode, glw::GLint* out_count, glw::GLenum* out_tf_mode,
						 glw::GLint* out_n_patch_vertices, glw::GLint* out_n_instances);

	const char* getTCSCode();
	const char* getTESCode();
	const char* getVSCode();
	int			getXFBBufferSize();
	void getXFBProperties(int* out_n_names, const char*** out_names);
	bool verifyXFBBuffer(const void* data);

private:
	/* Private fields */
	unsigned int	   m_n_input_vertices;
	const unsigned int m_n_result_vertices;
};

/** Implementation of Test Case 22.3
 *
 *  Make sure that a barrier used in a tessellation control shader synchronizes
 *  all instances working on the same patch. Tests the following scenario:
 *
 * * even invocations should write their gl_InvocationID value to their
 *   per-vertex output. A barrier is then issued, after which each odd invocation
 *   should read values stored by preceding even invocation, add current
 *   invocation's ID to that value and then write it to its per-vertex output.
 *   One more barrier should be issued. Then, every fourth invocation should
 *   read & sum up per-vertex outputs for four invocations following it
 *   (including the one discussed), and store it in a per-patch variable. (n+1)-th,
 *   (n+2)-th and (n+3)-th invocations should store zero in dedicated per-patch
 *   variables. 16 invocations should be considered, with 10 instances used for the
 *   draw call, each patch should consist of 8 vertices.
 **/
class TessellationShaderBarrier3 : public TessellationShaderBarrierTestCase
{
public:
	/* Public methods */
	TessellationShaderBarrier3(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderBarrier3(void)
	{
	}

protected:
	/* Protected methods */
	void getDrawCallArgs(glw::GLenum* out_mode, glw::GLint* out_count, glw::GLenum* out_tf_mode,
						 glw::GLint* out_n_patch_vertices, glw::GLint* out_n_instances);

	const char* getTCSCode();
	const char* getTESCode();
	const char* getVSCode();
	int			getXFBBufferSize();
	void getXFBProperties(int* out_n_names, const char*** out_names);
	bool verifyXFBBuffer(const void* data);

private:
	/* Private fields */
	unsigned int	   m_n_input_vertices;
	const unsigned int m_n_instances;
	const unsigned int m_n_invocations;
	const unsigned int m_n_patch_vertices;
	const unsigned int m_n_patches_per_invocation;
	const unsigned int m_n_result_vertices;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERBARRIER_HPP
