#ifndef _GL4CGLSPIRVTESTS_HPP
#define _GL4CGLSPIRVTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file  gl4cGlSpirvTests.hpp
 * \brief Conformance tests for the GL_ARB_gl_spirv functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluShaderProgram.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"
#include <map>
#include <vector>

using namespace glu;
using namespace glw;

namespace gl4cts
{

typedef std::map<std::string, std::vector<std::string> > SpirVMapping;

typedef std::vector<std::string> CapabilitiesVec;

/**  Verifies if using SPIR-V modules for each shader stage works as expected. */
class SpirvModulesPositiveTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvModulesPositiveTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private members */
	GLuint m_texture;
	GLuint m_fbo;

	std::string m_vertex;
	std::string m_tesselationCtrl;
	std::string m_tesselationEval;
	std::string m_geometry;
	std::string m_fragment;
};

/**  Verifies if one binary module can be associated with multiple shader objects. */
class SpirvShaderBinaryMultipleShaderObjectsTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvShaderBinaryMultipleShaderObjectsTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private members */
	std::string m_spirv;
};

/**  Verifies if state queries for new features added by ARB_gl_spirv works as expected. */
class SpirvModulesStateQueriesTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvModulesStateQueriesTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private members */
	std::string m_vertex;
};

/**  Verifies if new features added by ARB_gl_spirv generate error messages as expected. */
class SpirvModulesErrorVerificationTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvModulesErrorVerificationTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private members */
	std::string m_vertex;

	GLuint m_glslShaderId;
	GLuint m_spirvShaderId;
	GLuint m_programId;
	GLuint m_textureId;
};

/**  Verifies if GLSL to Spir-V converter supports Spir-V features. */
class SpirvGlslToSpirVEnableTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvGlslToSpirVEnableTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private members */
	std::string m_vertex;
};

/**  Verifies if GLSL built-in functions are supported by Spir-V. */
class SpirvGlslToSpirVBuiltInFunctionsTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvGlslToSpirVBuiltInFunctionsTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initMappings();

	/* Private members */
	SpirVMapping m_mappings;

	std::string				  m_commonVertex;
	std::string				  m_commonTessEval;
	std::vector<ShaderSource> m_sources;
};

/**  Verifies if constant specialization feature works as expected. */
class SpirvGlslToSpirVSpecializationConstantsTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvGlslToSpirVSpecializationConstantsTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private members */
	GLuint m_texture;
	GLuint m_fbo;

	std::string m_vertex;
	std::string m_fragment;
};

/**  Verifies if Spir-V built in variable decorations works as expected. */
class SpirvValidationBuiltInVariableDecorationsTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvValidationBuiltInVariableDecorationsTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

private: /* Private structs */
	struct ValidationOutputStruct
	{
		GLubyte x, y, z;
		GLuint  value;

		ValidationOutputStruct() : x(0), y(0), z(0), value(0)
		{
		}

		ValidationOutputStruct(GLubyte _x, GLubyte _y, GLuint _value) : x(_x), y(_y), z(0), value(_value)
		{
		}

		ValidationOutputStruct(GLubyte _x, GLubyte _y, GLubyte _z, GLuint _value) : x(_x), y(_y), z(_z), value(_value)
		{
		}
	};

	typedef std::vector<ValidationOutputStruct> ValidationOutputVec;

	typedef bool (SpirvValidationBuiltInVariableDecorationsTest::*ValidationFuncPtr)(ValidationOutputVec& outputs);

	struct ValidationStruct
	{
		std::vector<ShaderSource> shaders;
		ValidationOutputVec		  outputs;
		ValidationFuncPtr		  validationFuncPtr;

		ValidationStruct() : validationFuncPtr(DE_NULL)
		{
		}

		ValidationStruct(ValidationFuncPtr funcPtr) : validationFuncPtr(funcPtr)
		{
		}
	};

	/* Private methods */
	bool validComputeFunc(ValidationOutputVec& outputs);
	bool validPerVertexFragFunc(ValidationOutputVec& outputs);
	bool validPerVertexPointFunc(ValidationOutputVec& outputs);
	bool validTesselationGeometryFunc(ValidationOutputVec& outputs);
	bool validMultiSamplingFunc(ValidationOutputVec& outputs);

	/* Private members */
	SpirVMapping m_mappings;

	std::vector<ValidationStruct> m_validations;

	std::string m_compute;
	std::string m_vertex;
	std::string m_tesselationCtrl;
	std::string m_tesselationEval;
	std::string m_geometry;
	std::string m_fragment;
};

/**  Verifies if Spir-V capabilities works as expected. */
class SpirvValidationCapabilitiesTest : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvValidationCapabilitiesTest(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

	int spirVCapabilityCutOff(std::string spirVSrcInput, std::string& spirVSrcOutput, CapabilitiesVec& capabilities,
							  int& currentCapability);

private:
	typedef std::map<glu::ShaderType, CapabilitiesVec> CapabilitiesMap;

	struct ShaderStage
	{
		std::string		name;
		ShaderSource	source;
		ShaderBinary	binary;
		CapabilitiesVec caps;

		ShaderStage()
		{
		}

		ShaderStage(std::string _name) : name(_name)
		{
		}
	};

	typedef std::vector<ShaderStage> Pipeline;

	/* Private methods */

	/* Private members */
	std::vector<Pipeline> m_pipelines;
};

/** Test group which encapsulates all sparse buffer conformance tests */
class GlSpirvTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	GlSpirvTests(deqp::Context& context);

	void init();

private:
	GlSpirvTests(const GlSpirvTests& other);
	GlSpirvTests& operator=(const GlSpirvTests& other);
};

} /* glcts namespace */

#endif // _GL4CGLSPIRVTESTS_HPP
