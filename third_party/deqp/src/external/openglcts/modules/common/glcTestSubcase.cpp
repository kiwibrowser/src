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

#include "glcTestSubcase.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"

namespace deqp
{

namespace
{
static Context* current_context;
}

GLWrapper::GLWrapper()
	: CallLogWrapper(current_context->getRenderContext().getFunctions(), current_context->getTestContext().getLog())
	, m_context(*current_context)
{
}

SubcaseBase::SubcaseBase()
{
}

SubcaseBase::~SubcaseBase()
{
}

long SubcaseBase::Setup()
{
	return NO_ERROR;
}

long SubcaseBase::Cleanup()
{
	return NO_ERROR;
}

std::string SubcaseBase::VertexShader()
{
	return "";
}

std::string SubcaseBase::VertexShader2()
{
	return "";
}

std::string SubcaseBase::TessControlShader()
{
	return "";
}

std::string SubcaseBase::TessControlShader2()
{
	return "";
}

std::string SubcaseBase::TessEvalShader()
{
	return "";
}

std::string SubcaseBase::TessEvalShader2()
{
	return "";
}

std::string SubcaseBase::GeometryShader()
{
	return "";
}

std::string SubcaseBase::GeometryShader2()
{
	return "";
}

std::string SubcaseBase::FragmentShader()
{
	return "";
}

std::string SubcaseBase::FragmentShader2()
{
	return "";
}

void WriteField(tcu::TestLog& log, const char* title, std::string message)
{
	if (message == "")
		return;
	using namespace std;
	using tcu::TestLog;
	istringstream tokens(message);
	string		  line;
	log.startSection("Details", title);
	while (getline(tokens, line))
	{
		log.writeMessage(line.c_str());
	}
	log.endSection();
}

void SubcaseBase::OutputNotSupported(std::string reason)
{
	using tcu::TestLog;
	TestLog&		   log = m_context.getTestContext().getLog();
	std::string		   msg = reason + ", test will not run.";
	std::istringstream tokens(msg);
	std::string		   line;
	log.startSection("Not supported", "Reason");
	while (getline(tokens, line))
	{
		log.writeMessage(line.c_str());
	}
	log.endSection();
}

void SubcaseBase::Documentation()
{
	using namespace std;
	using tcu::TestLog;
	TestLog& log = m_context.getTestContext().getLog();

	WriteField(log, "Title", Title());
	WriteField(log, "Purpose", Purpose());
	WriteField(log, "Method", Method());
	WriteField(log, "Pass Criteria", PassCriteria());

	//OpenGL fields
	string vsh = VertexShader();
	if (!vsh.empty())
		WriteField(log, "OpenGL vertex shader", vsh);

	string vsh2 = VertexShader2();
	if (!vsh2.empty())
		WriteField(log, "OpenGL vertex shader 2", vsh2);

	string tcsh = TessControlShader();
	if (!tcsh.empty())
		WriteField(log, "OpenGL tessellation control shader", tcsh);

	string tcsh2 = TessControlShader();
	if (!tcsh2.empty())
		WriteField(log, "OpenGL tessellation control shader 2", tcsh2);

	string tesh = TessControlShader();
	if (!tesh.empty())
		WriteField(log, "OpenGL tessellation evaluation shader", tesh);

	string tesh2 = TessControlShader();
	if (!tesh2.empty())
		WriteField(log, "OpenGL tessellation evaluation shader 2", tesh2);

	string gsh = GeometryShader();
	if (!gsh.empty())
		WriteField(log, "OpenGL geometry shader", gsh);

	string gsh2 = GeometryShader2();
	if (!gsh2.empty())
		WriteField(log, "OpenGL geometry shader 2", gsh2);

	string fsh = FragmentShader();
	if (!fsh.empty())
		WriteField(log, "OpenGL fragment shader", fsh);

	string fsh2 = FragmentShader2();
	if (!fsh2.empty())
		WriteField(log, "OpenGL fragment shader 2", fsh2);
}

TestSubcase::TestSubcase(Context& context, const char* name, SubcaseBase::SubcaseBasePtr (*factoryFunc)())
	: TestCase(context, name, "")
	, m_factoryFunc(factoryFunc)
	, m_iterationCount(context.getTestContext().getCommandLine().getTestIterationCount())
{
	if (!m_iterationCount)
		m_iterationCount = 1;
}

TestSubcase::~TestSubcase(void)
{
}

void TestSubcase::init(void)
{
}

void TestSubcase::deinit(void)
{
}

TestSubcase::IterateResult TestSubcase::iterate(void)
{
	current_context = &m_context;
	using namespace std;
	using tcu::TestLog;
	TestLog& log = m_testCtx.getLog();

	SubcaseBase::SubcaseBasePtr subcase = m_factoryFunc();
	subcase->Documentation();
	subcase->enableLogging(true);
	//Run subcase
	long subError = NO_ERROR;
	// test case setup
	try
	{
		subError = subcase->Setup();
		if (subError == ERROR)
			log.writeMessage("Test Setup() failed");
	}
	catch (const runtime_error& ex)
	{
		log.writeMessage(ex.what());
		subError = ERROR;
	}
	catch (...)
	{
		log.writeMessage("Undefined exception.");
		subError = ERROR;
	}

	// test case run
	if (subError == NO_ERROR)
	{
		try
		{
			subError = subcase->Run();
			if (subError == ERROR)
				log.writeMessage("Test Run() failed");
		}
		catch (const runtime_error& ex)
		{
			log.writeMessage(ex.what());
			subError = ERROR;
		}
		catch (...)
		{
			log.writeMessage("Undefined exception.");
			subError = ERROR;
		}
	}

	// test case cleanup
	try
	{
		if (subcase->Cleanup() == ERROR)
		{
			subError = ERROR;
			log.writeMessage("Test Cleanup() failed");
		}
	}
	catch (const runtime_error& ex)
	{
		log.writeMessage(ex.what());
		subError = ERROR;
	}
	catch (...)
	{
		log.writeMessage("Undefined exception.");
		subError = ERROR;
	}
	subcase->enableLogging(false);

	//check gl error state
	const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();
	glw::GLenum			  glError = gl.getError();
	for (int i = 0; i < 100 && gl.getError(); ++i)
		;
	if (glError != GL_NO_ERROR)
	{
		const char* name = glu::getErrorName(glError);
		if (name == DE_NULL)
			name = "UNKNOWN ERROR";
		log << TestLog::Message << "After test execution glGetError() returned: " << name
			<< ", forcing FAIL for subcase" << TestLog::EndMessage;
		subError = ERROR;
	}

	if (subError == ERROR)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}
	else if (subError == NOT_SUPPORTED)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	if (--m_iterationCount)
		return CONTINUE;
	else
		return STOP;
}
}
