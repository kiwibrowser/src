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
 * \brief Spirv program and binary info.
 *//*--------------------------------------------------------------------*/

#include "vkSpirVProgram.hpp"

#include "tcuTestLog.hpp"

namespace vk
{

tcu::TestLog& operator<< (tcu::TestLog& log, const SpirVProgramInfo& shaderInfo)
{
	log << tcu::TestLog::ShaderProgram(shaderInfo.compileOk , shaderInfo.infoLog)
		<< tcu::TestLog::SpirVAssemblySource(shaderInfo.source)
		<< tcu::TestLog::EndShaderProgram;

	// Write statistics
	log << tcu::TestLog::Float(	"SpirVAssemblyTime",
								"SpirV assembly time",
								"ms", QP_KEY_TAG_TIME, (float)shaderInfo.compileTimeUs / 1000.0f);
	return log;
}

tcu::TestLog& operator<< (tcu::TestLog& log, const SpirVAsmSource& source)
{
	log << tcu::TestLog::ShaderProgram(true , "")
		<< tcu::TestLog::SpirVAssemblySource(source.source)
		<< tcu::TestLog::EndShaderProgram;

	return log;
}

} // vk
