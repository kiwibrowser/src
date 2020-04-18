#ifndef _ESEXTCTESSELLATIONSHADERERRORS_HPP
#define _ESEXTCTESSELLATIONSHADERERRORS_HPP
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

/* Groups all building error tests */
class TessellationShaderErrors : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TessellationShaderErrors(Context& context, const ExtParameters& extParams);

	virtual void init(void);
};

/** Base class for all test classes that implement Tessellation Shader
 *  Test Case 4 test cases. */
class TessellationShaderErrorsTestCaseBase : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderErrorsTestCaseBase(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TessellationShaderErrorsTestCaseBase()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected type definitions */
	/** Define valid compilation results. */
	typedef enum {
		COMPILATION_RESULT_MUST_SUCCEED,
		COMPILATION_RESULT_CAN_FAIL,
		COMPILATION_RESULT_MUST_FAIL,

		COMPILATION_RESULT_UNKNOWN
	} _compilation_result;

	/** Define recognized stages of a rendering pipeline. Used to
	 *  form a program object.
	 */
	typedef enum {
		PIPELINE_STAGE_FIRST	= 0,
		PIPELINE_STAGE_FRAGMENT = PIPELINE_STAGE_FIRST,
		PIPELINE_STAGE_TESSELLATION_CONTROL,
		PIPELINE_STAGE_TESSELLATION_EVALUATION,
		PIPELINE_STAGE_VERTEX,

		PIPELINE_STAGE_COUNT,
		PIPELINE_STAGE_UNKNOWN = PIPELINE_STAGE_COUNT,
	} _pipeline_stage;

	/** Define valid linking operation results. */
	typedef enum {
		LINKING_RESULT_MUST_SUCCEED,
		LINKING_RESULT_MUST_FAIL,

		LINKING_RESULT_UNKNOWN
	} _linking_result;

	/* Protected methods */
	virtual unsigned int		getAmountOfProgramObjects();
	virtual _compilation_result getCompilationResult(_pipeline_stage pipeline_stage) = 0;
	virtual std::string getFragmentShaderCode(unsigned int n_program_object);
	virtual _linking_result getLinkingResult()											   = 0;
	virtual std::string getTessellationControlShaderCode(unsigned int n_program_object)	= 0;
	virtual std::string getTessellationEvaluationShaderCode(unsigned int n_program_object) = 0;
	virtual std::string getVertexShaderCode(unsigned int n_program_object);
	virtual bool isPipelineStageUsed(_pipeline_stage stage) = 0;

private:
	/* Private methods */
	glw::GLenum getGLEnumForPipelineStage(_pipeline_stage stage);

	/* Private variables */
	glw::GLuint* m_fs_ids;
	unsigned int m_n_program_objects;
	glw::GLuint* m_po_ids;
	glw::GLuint* m_tc_ids;
	glw::GLuint* m_te_ids;
	glw::GLuint* m_vs_ids;
};

/** Make sure that declaring per-vertex input blocks in
 *  a non-arrayed manner in tessellation control shaders results in
 *  a compile- or link-time error.
 *
 **/
class TessellationShaderError1InputBlocks : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError1InputBlocks(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError1InputBlocks(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	std::string getVertexShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/** Make sure that declaring per-vertex input variables in
 *  a non-arrayed manner in tessellation control shaders results in
 *  a compile- or link-time error.
 *
 **/
class TessellationShaderError1InputVariables : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError1InputVariables(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError1InputVariables(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	std::string getVertexShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that declaring per-vertex output blocks in
 *  a non-arrayed manner in tessellation control shaders results in
 *  a compile- or link-time error.
 */
class TessellationShaderError2OutputBlocks : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError2OutputBlocks(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError2OutputBlocks(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that declaring per-vertex output variables in
 *  a non-arrayed manner in tessellation control shaders results in
 *  a compile- or link-time error.
 */
class TessellationShaderError2OutputVariables : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError2OutputVariables(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError2OutputVariables(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that declaring per-vertex input blocks in
 *  a non-arrayed manner in tessellation evaluation shaders results
 *  in a compile- or link-time error.
 */
class TessellationShaderError3InputBlocks : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError3InputBlocks(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError3InputBlocks(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that declaring per-vertex input variables in
 *  a non-arrayed manner in tessellation evaluation shaders results
 *  in a compile- or link-time error.
 */
class TessellationShaderError3InputVariables : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError3InputVariables(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError3InputVariables(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that using an array size different than gl_MaxPatchVertices for
 *  per-vertex input blocks in tessellation control shaders results in a compile-
 *  or link-time error.
 */
class TessellationShaderError4InputBlocks : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError4InputBlocks(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError4InputBlocks(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that using an array size different than gl_MaxPatchVertices for
 *  per-vertex input variables in tessellation control shaders results in a compile-
 *  or link-time error.
 */
class TessellationShaderError4InputVariables : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError4InputVariables(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError4InputVariables(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that using an array size different than gl_MaxPatchVertices for
 *  per-vertex input blocks in tessellation evaluation shaders
 *  results in a compile- or link-time error.
 */
class TessellationShaderError5InputBlocks : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError5InputBlocks(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError5InputBlocks(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that using an array size different than gl_MaxPatchVertices for
 *  per-vertex input variables in tessellation evaluation shaders results in
 *  a compile- or link-time error.
 */
class TessellationShaderError5InputVariables : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError5InputVariables(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError5InputVariables(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*
 *  Make sure that a program object will fail to link, or that the relevant
 *  tessellation control shader object fails to compile, if the output patch
 *  vertex count specified by the tessellation control shader object attached
 *  to the program is <= 0 or > GL_MAX_PATCH_VERTICES_EXT;
 */
class TessellationShaderError6 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError6(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError6(void)
	{
	}

protected:
	/* Protected methods */
	unsigned int		getAmountOfProgramObjects();
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure it is a compile- or link-time error to write to a per-vertex output
 *  variable in a tessellation control shader at index which is not equal
 *  to gl_InvocationID;
 */
class TessellationShaderError7 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError7(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError7(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure it is a compile-time error to define input per-patch attributes
 *  in a tessellation control shader.
 *
 */
class TessellationShaderError8 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError8(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError8(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*
 *  Make sure it is a compile- or link-time error to define output per-patch attributes
 *  in a tessellation evaluation shader.
 */
class TessellationShaderError9 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError9(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError9(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure that it is a link-time error to use a different type or qualification
 *  for a per-patch input variable in a tessellation evaluation shader, than was
 *  used to define a corresponding output variable in a tessellation control shader.
 */
class TessellationShaderError10 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError10(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError10(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure it is a link-time error not to declare primitive mode in
 *  input layout of a tessellation evaluation shader object.
 */
class TessellationShaderError11 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError11(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError11(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure it is a compile- or link-time error to access gl_TessCoord as if it
 *  was an array.
 */
class TessellationShaderError12 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError12(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError12(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

/*  Make sure it is a compile- or link-time error to access gl_TessCoord as if it
 *  was a member of gl_in array.
 */
class TessellationShaderError13 : public TessellationShaderErrorsTestCaseBase
{
public:
	/* Public methods */
	TessellationShaderError13(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderError13(void)
	{
	}

protected:
	/* Protected methods */
	_compilation_result getCompilationResult(_pipeline_stage pipeline_stage);
	_linking_result getLinkingResult();
	std::string getTessellationControlShaderCode(unsigned int n_program_object);
	std::string getTessellationEvaluationShaderCode(unsigned int n_program_object);
	bool isPipelineStageUsed(_pipeline_stage stage);
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERERRORS_HPP
