#ifndef _VKTSHADEREXECUTOR_HPP
#define _VKTSHADEREXECUTOR_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
 *//*!
 * \file
 * \brief Vulkan ShaderExecutor
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vktTestCase.hpp"
#include "gluVarType.hpp"
#include "vkShaderProgram.hpp"

#include <vector>
#include <string>

namespace vkt
{
namespace shaderexecutor
{

//! Shader input / output variable declaration.
struct Symbol
{
	std::string				name;		//!< Symbol name.
	glu::VarType			varType;	//!< Symbol type.

	Symbol (void) {}
	Symbol (const std::string& name_, const glu::VarType& varType_) : name(name_), varType(varType_) {}
};

//! Complete shader specification.
struct ShaderSpec
{
	glu::GLSLVersion		glslVersion;
	std::vector<Symbol>		inputs;
	std::vector<Symbol>		outputs;
	std::string				globalDeclarations;	//!< These are placed into global scope. Can contain uniform declarations for example.
	std::string				source;				//!< Source snippet to be executed.
	vk::ShaderBuildOptions	buildOptions;

	ShaderSpec (void)
		: glslVersion(glu::GLSL_VERSION_310_ES)
	{}
};

enum
{
	//!< Descriptor set index for additional resources
	EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX		= 1,
};

//! Base class for shader executor.
class ShaderExecutor
{
public:
	virtual					~ShaderExecutor		(void);

	//! Execute
	virtual void			execute				(int numValues, const void* const* inputs, void* const* outputs, vk::VkDescriptorSet extraResources = (vk::VkDescriptorSet)0) = 0;

protected:
							ShaderExecutor		(Context& context, const ShaderSpec& shaderSpec)
								: m_context		(context)
								, m_shaderSpec	(shaderSpec)
							{}

	Context&				m_context;
	const ShaderSpec		m_shaderSpec;

private:
							ShaderExecutor		(const ShaderExecutor&);
	ShaderExecutor&			operator=			(const ShaderExecutor&);
};

void				generateSources		(glu::ShaderType shaderType, const ShaderSpec& shaderSpec, vk::SourceCollections& dst);
ShaderExecutor*		createExecutor		(Context& context, glu::ShaderType shaderType, const ShaderSpec& shaderSpec, vk::VkDescriptorSetLayout extraResourcesLayout = (vk::VkDescriptorSetLayout)0);

} // shaderexecutor
} // vkt

#endif // _VKTSHADEREXECUTOR_HPP
