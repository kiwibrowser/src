#ifndef _VKSPIRVASM_HPP
#define _VKSPIRVASM_HPP
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
 * \brief SPIR-V assembly to binary.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkPrograms.hpp"

#include <ostream>

namespace vk
{

//! Assemble SPIR-V program. Will fail with NotSupportedError if compiler is not available.
bool	assembleSpirV		(const SpirVAsmSource* program, std::vector<deUint32>* dst, SpirVProgramInfo* buildInfo);

//! Disassemble SPIR-V binary. Throws tcu::NotSupportedError if disassembler is not available
void	disassembleSpirV	(size_t binarySizeInWords, const deUint32* binary, std::ostream* dst);

//! Validate SPIR-V binary, returning true if validation succeeds. Will fail with NotSupportedError if compiler is not available.
bool	validateSpirV		(size_t binarySizeInWords, const deUint32* binary, std::ostream* infoLog);

} // vk

#endif // _VKSPIRVASM_HPP
