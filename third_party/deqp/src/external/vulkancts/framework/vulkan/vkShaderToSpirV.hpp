#ifndef _VKSHADERTOSPIRV_HPP
#define _VKSHADERTOSPIRV_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief Shading language (GLSL/HLSL) to SPIR-V.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkPrograms.hpp"
#include "gluShaderProgram.hpp"

#include <ostream>

namespace vk
{

/*--------------------------------------------------------------------*//*!
 * \brief Compile GLSL program to SPIR-V binary
 * \param src
 * \param dst
 * \param buildInfo
 * \return True if compilation and linking succeeded, false otherwise
 *
 * If deqp was built without glslang (and thus compiler is not available)
 * tcu::NotSupportedError will be thrown instead.
 *
 * \note No linking is currently supported so src may contain source
 *       for only one shader stage.
 *//*--------------------------------------------------------------------*/
bool	compileGlslToSpirV		(const GlslSource& src, std::vector<deUint32>* dst, glu::ShaderProgramInfo* buildInfo);

/*--------------------------------------------------------------------*//*!
 * \brief Compile HLSL program to SPIR-V binary
 * \param src
 * \param dst
 * \param buildInfo
 * \return True if compilation and linking succeeded, false otherwise
 *
 * If deqp was built without glslang (and thus compiler is not available)
 * tcu::NotSupportedError will be thrown instead.
 *
 * \note No linking is currently supported so src may contain source
 *       for only one shader stage.
 *//*--------------------------------------------------------------------*/
bool	compileHlslToSpirV		(const HlslSource& src, std::vector<deUint32>* dst, glu::ShaderProgramInfo* buildInfo);

/*--------------------------------------------------------------------*//*!
 * \brief Strip SPIR-V binary
 * \param src
 * \param dst
 *
 * Removes OpName and OpMemberName opcodes from SPIR-V binary
 *
 * If deqp was built without glslang (and thus compiler is not available)
 * tcu::NotSupportedError will be thrown instead.
 *//*--------------------------------------------------------------------*/
void	stripSpirVDebugInfo		(const size_t numSrcInstrs, const deUint32* srcInstrs, std::vector<deUint32>* dst);

} // vk

#endif // _VKSHADERTOSPIRV_HPP
