#ifndef _GLCTESTSUBCASE_HPP
#define _GLCTESTSUBCASE_HPP
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

#include "deSharedPtr.hpp"
#include "glcTestCase.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluStrUtil.hpp"
#include "tcuDefs.hpp"

#include <exception>

#define NO_ERROR 0
#define ERROR -1
#define NOT_SUPPORTED 0x10
#define NL "\n"

namespace deqp
{

class GLWrapper : public glu::CallLogWrapper
{
public:
	virtual ~GLWrapper()
	{
	}

	GLWrapper();
	Context& m_context;
};

class SubcaseBase : public GLWrapper
{
public:
	typedef de::SharedPtr<SubcaseBase> SubcaseBasePtr;
	SubcaseBase();
	virtual ~SubcaseBase();
	virtual long Run() = 0;
	virtual long Setup();
	virtual long Cleanup();

	virtual std::string Title()		   = 0;
	virtual std::string Purpose()	  = 0;
	virtual std::string Method()	   = 0;
	virtual std::string PassCriteria() = 0;
	std::string			VertexShader();
	std::string			VertexShader2();

	std::string TessControlShader();
	std::string TessControlShader2();

	std::string TessEvalShader();
	std::string TessEvalShader2();

	std::string GeometryShader();
	std::string GeometryShader2();

	std::string FragmentShader();
	std::string FragmentShader2();
	void		Documentation();
	void OutputNotSupported(std::string message);
};

class TestSubcase : public TestCase
{
public:
	TestSubcase(Context& context, const char* name, SubcaseBase::SubcaseBasePtr (*factoryFunc)());
	virtual ~TestSubcase(void);

	IterateResult iterate(void);

	template <class Type>
	static SubcaseBase::SubcaseBasePtr Create()
	{
		return SubcaseBase::SubcaseBasePtr(new Type());
	}

private:
	TestSubcase(const TestSubcase& other);
	TestSubcase& operator=(const TestSubcase& other);
	void init(void);
	void deinit(void);
	SubcaseBase::SubcaseBasePtr (*m_factoryFunc)();
	int m_iterationCount;
};

} // namespace deqp

#endif // _GLCTESTSUBCASE_HPP
