#ifndef _GLCSHADERLIBRARYCASE_HPP
#define _GLCSHADERLIBRARYCASE_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Shader test case.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderUtil.hpp"
#include "tcuSurface.hpp"

#include <string>
#include <vector>

namespace deqp
{
namespace sl
{

// ShaderCase node.

class ShaderCase : public tcu::TestCase
{
public:
	enum CaseType
	{
		CASETYPE_COMPLETE = 0,  //!< Has both shaders.
		CASETYPE_VERTEX_ONLY,   //!< Has only vertex shader.
		CASETYPE_FRAGMENT_ONLY, //!< Has only fragment shader.

		CASETYPE_LAST
	};

	enum ExpectResult
	{
		EXPECT_PASS = 0,
		EXPECT_COMPILE_FAIL,
		EXPECT_LINK_FAIL,

		EXPECT_LAST
	};

	struct Value
	{
		enum StorageType
		{
			STORAGE_UNIFORM,
			STORAGE_INPUT,
			STORAGE_OUTPUT,

			STORAGE_LAST
		};

		/* \todo [2010-03-31 petri] Replace with another vector to allow a) arrays, b) compact representation */
		union Element {
			float   float32;
			deInt32 int32;
			deInt32 bool32;
		};

		StorageType			 storageType;
		std::string			 valueName;
		glu::DataType		 dataType;
		int					 arrayLength; // Number of elements in array (currently always 1).
		std::vector<Element> elements;	// Scalar values (length dataType.scalarSize * arrayLength).
	};

	struct ValueBlock
	{
		int				   arrayLength; // Combined array length of each value (lengths must be same, or one).
		std::vector<Value> values;
		ValueBlock(void)
		{
			arrayLength = 0;
			values.empty();
		}
	};

	// Methods.
	ShaderCase(tcu::TestContext& testCtx, glu::RenderContext& renderCtx, const char* caseName, const char* description,
			   ExpectResult expectResult, const std::vector<ValueBlock>& valueBlocks, glu::GLSLVersion targetVersion,
			   const char* vertexSource, const char* fragmentSource);

	virtual ~ShaderCase(void);

	CaseType getCaseType(void) const
	{
		return m_caseType;
	}
	const std::vector<ValueBlock>& getValueBlocks(void) const
	{
		return m_valueBlocks;
	}
	const char* getVertexSource(void) const
	{
		return m_vertexSource.c_str();
	}
	const char* getFragmentSource(void) const
	{
		return m_fragmentSource.c_str();
	}

	bool		  execute(void);
	IterateResult iterate(void);

private:
	ShaderCase(const ShaderCase&);			  // not allowed!
	ShaderCase& operator=(const ShaderCase&); // not allowed!

	std::string genVertexShader(const ValueBlock& valueBlock);
	std::string genFragmentShader(const ValueBlock& valueBlock);
	std::string specializeVertexShader(const char* src, const ValueBlock& valueBlock);
	std::string specializeFragmentShader(const char* src, const ValueBlock& valueBlock);

	void specializeShaders(const char* vertexSource, const char* fragmentSource, std::string& outVertexSource,
						   std::string& outFragmentSource, const ValueBlock& valueBlock);

	void dumpValues(const ValueBlock& valueBlock, int arrayNdx);

	bool checkPixels(tcu::Surface& surface, int minX, int maxX, int minY, int maxY);

	// Member variables.
	glu::RenderContext&		m_renderCtx;
	CaseType				m_caseType;
	ExpectResult			m_expectResult;
	std::vector<ValueBlock> m_valueBlocks;
	glu::GLSLVersion		m_targetVersion;
	std::string				m_vertexSource;
	std::string				m_fragmentSource;
};

} // sl
} // deqp

#endif // _GLCSHADERLIBRARYCASE_HPP
