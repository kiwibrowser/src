#ifndef _GL4CSHADERBALLOTTESTS_HPP
#define _GL4CSHADERBALLOTTESTS_HPP
/*-------------------------------------------------------------------------
* OpenGL Conformance Test Suite
* -----------------------------
*
* Copyright (c) 2014-2017 The Khronos Group Inc.
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
* \file  gl4cShaderBallotTests.hpp
* \brief Conformance tests for the ARB_shader_ballot functionality.
*/ /*-------------------------------------------------------------------*/

#include "esextcTestCaseBase.hpp"
#include "glcTestCase.hpp"
#include "gluShaderProgram.hpp"

#include <map>
#include <vector>

namespace gl4cts
{
class ShaderBallotBaseTestCase : public glcts::TestCaseBase
{
public:
	class ShaderPipeline
	{
	private:
		glu::ShaderProgram* m_programRender;
		glu::ShaderProgram* m_programCompute;
		glu::ShaderType		m_testedShader;

		std::vector<std::string> m_shaders[glu::SHADERTYPE_LAST];
		char**					 m_shaderChunks[glu::SHADERTYPE_LAST];

		std::map<std::string, std::string> m_specializationMap;

		void renderQuad(deqp::Context& context);
		void executeComputeShader(deqp::Context& context);

	public:
		ShaderPipeline(glu::ShaderType testedShader, const std::string& contentSnippet,
					   std::map<std::string, std::string> specMap = std::map<std::string, std::string>());
		~ShaderPipeline();

		const char* const* getShaderParts(glu::ShaderType shaderType) const;
		unsigned int getShaderPartsCount(glu::ShaderType shaderType) const;

		void use(deqp::Context& context);

		inline void setShaderPrograms(glu::ShaderProgram* programRender, glu::ShaderProgram* programCompute)
		{
			m_programRender  = programRender;
			m_programCompute = programCompute;
		}

		inline const std::map<std::string, std::string>& getSpecializationMap() const
		{
			return m_specializationMap;
		}

		void test(deqp::Context& context);
	};

protected:
	/* Protected methods */
	void createShaderPrograms(ShaderPipeline& pipeline);

	/* Protected members*/
	std::vector<ShaderPipeline*> m_shaderPipelines;

	typedef std::vector<ShaderPipeline*>::iterator ShaderPipelineIter;

public:
	/* Public methods */
	ShaderBallotBaseTestCase(deqp::Context& context, const char* name, const char* description)
		: TestCaseBase(context, glcts::ExtParameters(glu::GLSL_VERSION_450, glcts::EXTENSIONTYPE_EXT), name,
					   description)
	{
	}

	virtual ~ShaderBallotBaseTestCase();

	static bool validateScreenPixels(deqp::Context& context, tcu::Vec4 desiredColor, tcu::Vec4 ignoredColor);
	static bool validateScreenPixelsSameColor(deqp::Context& context, tcu::Vec4 ignoredColor);
	static bool validateColor(tcu::Vec4 testedColor, tcu::Vec4 desiredColor);
};

/** Test verifies availability of new build-in features
**/
class ShaderBallotAvailabilityTestCase : public ShaderBallotBaseTestCase
{
public:
	/* Public methods */
	ShaderBallotAvailabilityTestCase(deqp::Context& context);

	void init();

	tcu::TestNode::IterateResult iterate();
};

/** Test verifies values of gl_SubGroup*MaskARB variables
**/
class ShaderBallotBitmasksTestCase : public ShaderBallotBaseTestCase
{
public:
	/* Public methods */
	ShaderBallotBitmasksTestCase(deqp::Context& context);

	void init();

	tcu::TestNode::IterateResult iterate();

protected:
	/* Protected members*/
	std::map<std::string, std::string> m_maskVars;

	typedef std::map<std::string, std::string>::iterator MaskVarIter;
};

/** Test verifies ballotARB calls and returned results
**/
class ShaderBallotFunctionBallotTestCase : public ShaderBallotBaseTestCase
{
public:
	/* Public methods */
	ShaderBallotFunctionBallotTestCase(deqp::Context& context);

	void init();

	tcu::TestNode::IterateResult iterate();
};

/** Test verifies readInvocationARB and readFirstInvocationARB function calls
**/
class ShaderBallotFunctionReadTestCase : public ShaderBallotBaseTestCase
{
public:
	/* Public methods */
	ShaderBallotFunctionReadTestCase(deqp::Context& context);

	void init();

	tcu::TestNode::IterateResult iterate();
};

class ShaderBallotTests : public deqp::TestCaseGroup
{
public:
	ShaderBallotTests(deqp::Context& context);
	void init(void);

private:
	ShaderBallotTests(const ShaderBallotTests& other);
	ShaderBallotTests& operator=(const ShaderBallotTests& other);
};

} /* gl4cts namespace */

#endif // _GL4CSHADERBALLOTTESTS_HPP
