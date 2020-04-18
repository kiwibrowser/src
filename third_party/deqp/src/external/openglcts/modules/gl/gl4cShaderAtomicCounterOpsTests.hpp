#ifndef _GL4CSHADERATOMICCOUNTEROPSTESTS_HPP
#define _GL4CSHADERATOMICCOUNTEROPSTESTS_HPP
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
 * \file  gl4cShaderAtomicCounterOpsTests.hpp
 * \brief Conformance tests for the ARB_shader_atomic_counter_ops functionality.
 */ /*-------------------------------------------------------------------*/

#include "deUniquePtr.hpp"
#include "glcTestCase.hpp"
#include "gluShaderProgram.hpp"

#include <algorithm>
#include <map>
#include <string>

namespace gl4cts
{

class ShaderAtomicCounterOpsTestBase : public deqp::TestCase
{
public:
	class AtomicOperation
	{
	private:
		std::string m_function;
		glw::GLuint m_inputValue;
		glw::GLuint m_paramValue;
		glw::GLuint m_compareValue;
		bool		m_testReturnValue;

	protected:
		virtual glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const = 0;

	public:
		AtomicOperation(std::string function, glw::GLuint inputValue, glw::GLuint paramValue,
						glw::GLuint compareValue = 0, bool testReturnValue = false)
			: m_function(function)
			, m_inputValue(inputValue)
			, m_paramValue(paramValue)
			, m_compareValue(compareValue)
			, m_testReturnValue(testReturnValue)
		{
		}

		virtual ~AtomicOperation()
		{
		}

		glw::GLuint getResult(unsigned int iterations) const
		{
			glw::GLuint result = m_inputValue;
			for (unsigned int i = 0; i < iterations; ++i)
			{
				result = iterate(result, m_paramValue, m_compareValue);
			}
			return result;
		}

		inline std::string getFunction() const
		{
			return m_function;
		}
		inline glw::GLuint getInputValue() const
		{
			return m_inputValue;
		}
		inline glw::GLuint getParamValue() const
		{
			return m_paramValue;
		}
		inline glw::GLuint getCompareValue() const
		{
			return m_compareValue;
		}
		inline bool shouldTestReturnValue() const
		{
			return m_testReturnValue;
		}
	};

	class AtomicOperationAdd : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(compare);
			return input + param;
		}

	public:
		AtomicOperationAdd(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterAdd", inputValue, paramValue, 0U, true)
		{
		}
	};

	class AtomicOperationSubtract : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(compare);
			return input - param;
		}

	public:
		AtomicOperationSubtract(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterSubtract", inputValue, paramValue, 0U, true)
		{
		}
	};

	class AtomicOperationMin : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(compare);
			return std::min(input, param);
		}

	public:
		AtomicOperationMin(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterMin", inputValue, paramValue)
		{
		}
	};

	class AtomicOperationMax : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(compare);
			return std::max(input, param);
		}

	public:
		AtomicOperationMax(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterMax", inputValue, paramValue)
		{
		}
	};

	class AtomicOperationAnd : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(compare);
			return input & param;
		}

	public:
		AtomicOperationAnd(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterAnd", inputValue, paramValue)
		{
		}
	};

	class AtomicOperationOr : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(compare);
			return input | param;
		}

	public:
		AtomicOperationOr(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterOr", inputValue, paramValue)
		{
		}
	};

	class AtomicOperationXor : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(compare);
			return input ^ param;
		}

	public:
		AtomicOperationXor(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterXor", inputValue, paramValue)
		{
		}
	};

	class AtomicOperationExchange : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			DE_UNREF(input);
			DE_UNREF(compare);
			return param;
		}

	public:
		AtomicOperationExchange(glw::GLuint inputValue, glw::GLuint paramValue)
			: AtomicOperation("atomicCounterExchange", inputValue, paramValue)
		{
		}
	};

	class AtomicOperationCompSwap : public AtomicOperation
	{
	private:
		glw::GLuint iterate(glw::GLuint input, glw::GLuint param, glw::GLuint compare) const
		{
			return input == compare ? param : input;
		}

	public:
		AtomicOperationCompSwap(glw::GLuint inputValue, glw::GLuint paramValue, glw::GLuint compareValue)
			: AtomicOperation("atomicCounterCompSwap", inputValue, paramValue, compareValue)
		{
		}
	};

	class ShaderPipeline
	{
	private:
		glu::ShaderProgram* m_program;
		glu::ShaderProgram* m_programCompute;
		glu::ShaderType		m_testedShader;
		AtomicOperation*	m_atomicOp;

		std::string m_shaders[glu::SHADERTYPE_LAST];

		void renderQuad(deqp::Context& context);
		void executeComputeShader(deqp::Context& context);

	public:
		ShaderPipeline(glu::ShaderType testedShader, AtomicOperation* newOp, bool contextGL46);
		~ShaderPipeline();

		void prepareShader(std::string& shader, const std::string& tag, const std::string& replace);

		void create(deqp::Context& context);
		void use(deqp::Context& context);

		inline glu::ShaderProgram* getShaderProgram() const
		{
			return m_program;
		}

		inline AtomicOperation* getAtomicOperation() const
		{
			return m_atomicOp;
		}

		void test(deqp::Context& context);
	};

private:
	/* Private methods*/
	void fillAtomicCounterBuffer(AtomicOperation* atomicOp);
	bool checkAtomicCounterBuffer(AtomicOperation* atomicOp);

	void bindBuffers();
	bool validateColor(tcu::Vec4 testedColor, tcu::Vec4 desiredColor);
	bool validateScreenPixels(tcu::Vec4 desiredColor, tcu::Vec4 ignoredColor);

protected:
	/* Protected members */
	glw::GLuint					  m_atomicCounterBuffer;
	glw::GLuint					  m_atomicCounterCallsBuffer;
	bool						  m_contextSupportsGL46;
	std::vector<AtomicOperation*> m_operations;
	std::vector<ShaderPipeline>   m_shaderPipelines;

	/* Protected methods */
	inline void addOperation(AtomicOperation* newOp)
	{
		for (unsigned int i = 0; i < glu::SHADERTYPE_LAST; ++i)
		{
			m_shaderPipelines.push_back(ShaderPipeline((glu::ShaderType)i, newOp, m_contextSupportsGL46));
		}
	}

	virtual void setOperations() = 0;

public:
	/* Public methods */
	ShaderAtomicCounterOpsTestBase(deqp::Context& context, const char* name, const char* description);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();

	typedef std::vector<ShaderPipeline>::iterator   ShaderPipelineIter;
	typedef std::vector<AtomicOperation*>::iterator AtomicOperationIter;
};

/** Test verifies new built-in addition and substraction atomic counter operations
**/
class ShaderAtomicCounterOpsAdditionSubstractionTestCase : public ShaderAtomicCounterOpsTestBase
{
private:
	/* Private methods */
	void setOperations();

public:
	/* Public methods */
	ShaderAtomicCounterOpsAdditionSubstractionTestCase(deqp::Context& context);
};

/** Test verifies new built-in minimum and maximum atomic counter operations
**/
class ShaderAtomicCounterOpsMinMaxTestCase : public ShaderAtomicCounterOpsTestBase
{
private:
	/* Private methods */
	void setOperations();

public:
	/* Public methods */
	ShaderAtomicCounterOpsMinMaxTestCase(deqp::Context& context);
};

/** Test verifies new built-in bitwise atomic counter operations
**/
class ShaderAtomicCounterOpsBitwiseTestCase : public ShaderAtomicCounterOpsTestBase
{
private:
	/* Private methods */
	void setOperations();

public:
	/* Public methods */
	ShaderAtomicCounterOpsBitwiseTestCase(deqp::Context& context);
};

/** Test verifies new built-in exchange and swap atomic counter operations
**/
class ShaderAtomicCounterOpsExchangeTestCase : public ShaderAtomicCounterOpsTestBase
{
private:
	/* Private methods */
	void setOperations();

public:
	/* Public methods */
	ShaderAtomicCounterOpsExchangeTestCase(deqp::Context& context);
};

/** Test group which encapsulates all ARB_shader_atomic_counter_ops conformance tests */
class ShaderAtomicCounterOps : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ShaderAtomicCounterOps(deqp::Context& context);

	void init();

private:
	ShaderAtomicCounterOps(const ShaderAtomicCounterOps& other);
	ShaderAtomicCounterOps& operator=(const ShaderAtomicCounterOps& other);
};

} /* glcts namespace */

#endif // _GL4CSHADERATOMICCOUNTEROPSTESTS_HPP
