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
 * \file glcLimitTest.cpp
 * \brief Definition of template class.
 */ /*-------------------------------------------------------------------*/

using namespace glw;

template<typename DataType>
LimitCase<DataType>::LimitCase(deqp::Context& context,
							   const char* caseName,
							   deUint32 limitToken,
							   DataType limitBoundry,
							   bool isBoundryMaximum,
							   const char* glslVersion,
							   const char* glslBuiltin,
							   const char* glslExtension)
	: deqp::TestCase(context, caseName, "Token limit validation.")
	, m_limitToken(limitToken)
	, m_limitBoundry(limitBoundry)
	, m_isBoundryMaximum(isBoundryMaximum)
	, m_glslVersion(glslVersion)
	, m_glslBuiltin(glslBuiltin)
	, m_glslExtension(glslExtension)
{
}

template<typename DataType>
LimitCase<DataType>::~LimitCase(void)
{
}

template<typename DataType>
tcu::TestNode::IterateResult LimitCase<DataType>::iterate(void)
{
	DataType limitValue = DataType();
	const Functions& gl = m_context.getRenderContext().getFunctions();

	// make sure that limit or builtin was specified
	DE_ASSERT(m_limitToken || !m_glslBuiltin.empty());

	// check if limit was specified
	if (m_limitToken)
	{
		// check if limit is not smaller or greater then boundry defined in specification
		limitValue = getLimitValue(gl);
		if (!isWithinBoundry(limitValue))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		// if glsl builtin wasn't defined then test already passed
		if (m_glslBuiltin.empty())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
			return STOP;
		}
	}

	// create compute shader to check glsl builtin
	std::string shaderSource = createShader();
	const GLchar* source = shaderSource.c_str();
	const GLuint program = gl.createProgram();
	GLuint shader = gl.createShader(GL_COMPUTE_SHADER);
	gl.attachShader(program, shader);
	gl.deleteShader(shader);
	gl.shaderSource(shader, 1, &source, NULL);
	gl.compileShader(shader);
	gl.linkProgram(program);

	GLint status;
	gl.getProgramiv(program, GL_LINK_STATUS, &status);
	GLint length;
	gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	if (length > 1)
	{
		std::vector<GLchar> log(length);
		gl.getProgramInfoLog(program, length, NULL, &log[0]);
		m_testCtx.getLog() << tcu::TestLog::Message
						   << &log[0]
						   << tcu::TestLog::EndMessage;
	}
	if (status == GL_FALSE)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.useProgram(program);

	GLuint buffer;
	gl.genBuffers(1, &buffer);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DataType), NULL, GL_DYNAMIC_DRAW);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	gl.dispatchCompute(1, 1, 1);

	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	DataType* data = static_cast<DataType*>(gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DataType), GL_MAP_READ_BIT));
	DataType builtinValue = data[0];
	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	if (m_limitToken)
	{
		// limit token was specified - compare builtin to it
		if (isEqual(limitValue, builtinValue))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Shader builtin has value: "
							   << builtinValue
							   << " which is different then the value of corresponding limit: "
							   << limitValue
							   << tcu::TestLog::EndMessage;
		}
	}
	else
	{
		// limit token was not specified - compare builtin to the boundry
		if (isWithinBoundry(builtinValue, true))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Shader builtin value is: "
							   << builtinValue
							   << " which is outside of specified boundry."
							   << tcu::TestLog::EndMessage;
		}
	}

	return STOP;
}

template<typename DataType>
bool LimitCase<DataType>::isWithinBoundry(DataType value, bool isBuiltin) const
{
	if (m_isBoundryMaximum)
	{
		// value should be smaller or euqual to boundry
		if (isGreater(value, m_limitBoundry))
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << (isBuiltin ? "Builtin" : "Limit")
							   << " value is: "
							   << value
							   << " when it should not be greater than "
							   << m_limitBoundry
							   << tcu::TestLog::EndMessage;
			return false;
		}
	}
	else
	{
		// value should be greater or euqual to boundry
		if (isSmaller(value, m_limitBoundry))
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << (isBuiltin ? "Builtin" : "Limit")
							   << " value is: "
							   << value
							   << "when it should not be smaller than "
							   << m_limitBoundry
							   << tcu::TestLog::EndMessage;
			return false;
		}
	}

	return true;
}

template<typename DataType>
std::string LimitCase<DataType>::createShader() const
{
		std::stringstream shader;
		shader << "#version " << m_glslVersion << "\n";
		if (!m_glslExtension.empty())
			shader << "#extension " + m_glslExtension + " : require\n";
		shader << "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
				  "layout(std430) buffer Output {\n"
			   << getGLSLDataType() <<" data; } g_out;"
				  "void main() { "
				  "g_out.data = " << m_glslBuiltin << "; }";
		return shader.str();
}

template<typename DataType>
std::string LimitCase<DataType>::getGLSLDataType() const
{
	return "int";
}

template<>
std::string LimitCase<GLfloat>::getGLSLDataType() const
{
	return "float";
}

template<>
std::string LimitCase<tcu::IVec3>::getGLSLDataType() const
{
	return "ivec3";
}

template<typename DataType>
bool LimitCase<DataType>::isEqual(DataType a, DataType b) const
{
	return a == b;
}

template<>
bool LimitCase<tcu::IVec3>::isEqual(tcu::IVec3 a, tcu::IVec3 b) const
{
	tcu::BVec3 bVec = tcu::equal(a, b);
	return tcu::boolAll(bVec);
}

template<typename DataType>
bool LimitCase<DataType>::isGreater(DataType a, DataType b) const
{
	return a > b;
}

template<>
bool LimitCase<tcu::IVec3>::isGreater(tcu::IVec3 a, tcu::IVec3 b) const
{
	tcu::BVec3 bVec = tcu::greaterThan(a, b);
	return tcu::boolAll(bVec);
}

template<typename DataType>
bool LimitCase<DataType>::isSmaller(DataType a, DataType b) const
{
	return a < b;
}

template<>
bool LimitCase<tcu::IVec3>::isSmaller(tcu::IVec3 a, tcu::IVec3 b) const
{
	tcu::BVec3 bVec = tcu::lessThan(a, b);
	return tcu::boolAll(bVec);
}

template<typename DataType>
DataType LimitCase<DataType>::getLimitValue(const Functions&) const
{
	return DataType();
}

template<>
GLint LimitCase<GLint>::getLimitValue(const Functions& gl) const
{
	GLint value = -1;
	gl.getIntegerv(m_limitToken, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv");
	return value;
}

template<>
GLint64 LimitCase<GLint64>::getLimitValue(const Functions& gl) const
{
	GLint64 value = -1;
	gl.getInteger64v(m_limitToken, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInteger64v");
	return value;
}

template<>
GLuint64 LimitCase<GLuint64>::getLimitValue(const Functions& gl) const
{
	GLint64 value = -1;
	gl.getInteger64v(m_limitToken, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInteger64v");
	return static_cast<GLuint64>(value);
}

template<>
GLfloat LimitCase<GLfloat>::getLimitValue(const Functions& gl) const
{
	GLfloat value = -1;
	gl.getFloatv(m_limitToken, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv");
	return value;
}

template<>
tcu::IVec3 LimitCase<tcu::IVec3>::getLimitValue(const Functions& gl) const
{
	tcu::IVec3 value(-1);
	for (int i = 0; i < 3; i++)
		gl.getIntegeri_v(m_limitToken, (GLuint)i, &value[i]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegeri_v");
	return value;
}

