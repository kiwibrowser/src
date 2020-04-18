#ifndef _ES31CLAYOUTBINDINGTESTS_HPP
#define _ES31CLAYOUTBINDINGTESTS_HPP
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

#include "glcTestCase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"

namespace glcts
{

//=========================================================================
//= utility macros/templates
//=========================================================================
template <typename T, size_t N>
std::vector<T> makeVector(const T (&data)[N])
{
	return std::vector<T>(data, data + N);
}

enum eStageType
{
	VertexShader,
	FragmentShader,
	ComputeShader
};

struct StageType
{
	const char* name;
	eStageType  type;
};

enum eSurfaceType
{
	AtomicCounter,
	Texture,
	Image,
	UniformBlock,
	ShaderStorageBuffer
};

enum eTextureType
{
	None,
	OneD,
	TwoD,
	ThreeD,
	OneDArray,
	TwoDArray
};

struct LayoutBindingParameters
{
	char const*  keyword;	  // uniform, buffer
	eSurfaceType surface_type; // Texture, Image...
	eTextureType texture_type;
	char const*  vector_type;		// lookup vector type
	char const*  uniform_type;		// sampler2D, image2D...
	char const*  coord_vector_type; // coord vector type
	char const*  access_function;   // texture(), imageLoad()...
};

class LayoutBindingTests : public TestCaseGroup
{
public:
	LayoutBindingTests(glcts::Context& context, glu::GLSLVersion glslVersion);
	~LayoutBindingTests(void);

	void init(void);

private:
	LayoutBindingTests(const LayoutBindingTests& other);
	LayoutBindingTests& operator=(const LayoutBindingTests& other);

	std::string createTestName(const StageType& stageType, const LayoutBindingParameters& testArgs);

private:
	glu::GLSLVersion m_glslVersion;

private:
	static StageType			   stageTypes[];
	static LayoutBindingParameters test_args[];
};

} // glcts

#endif // _ES31CLAYOUTBINDINGTESTS_HPP
