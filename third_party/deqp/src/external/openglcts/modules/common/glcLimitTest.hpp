#ifndef _GLCLIMITTEST_HPP
#define _GLCLIMITTEST_HPP
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
 * \brief Limits tests.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluDefs.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "tcuVectorUtil.hpp"

#include <string>

namespace glcts
{

template <typename DataType>
class LimitCase : public deqp::TestCase
{
public:
	LimitCase(deqp::Context& context, const char* caseName, deUint32 limitToken, DataType limitBoundry,
			  bool isBoundryMaximum, const char* glslVersion = "", const char* glslBuiltin = "",
			  const char* glslExtension = "");
	virtual ~LimitCase(void);

	tcu::TestNode::IterateResult iterate(void);

protected:
	bool isWithinBoundry(DataType value, bool isBuiltin = false) const;
	std::string createShader() const;

	// those functions require specialization for some data types
	DataType getLimitValue(const glw::Functions& gl) const;
	std::string getGLSLDataType() const;
	bool isEqual(DataType a, DataType b) const;
	bool isGreater(DataType a, DataType b) const;
	bool isSmaller(DataType a, DataType b) const;

private:
	LimitCase(const LimitCase&);			// not allowed!
	LimitCase& operator=(const LimitCase&); // not allowed!

	deUint32		  m_limitToken;
	DataType		  m_limitBoundry; // min/max value
	bool			  m_isBoundryMaximum;
	const std::string m_glslVersion;
	const std::string m_glslBuiltin;
	const std::string m_glslExtension;
};

#include "glcLimitTest.inl"

} // glcts

#endif // _GLCLIMITTEST_HPP
