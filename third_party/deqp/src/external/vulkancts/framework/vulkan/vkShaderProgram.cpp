/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief Shader source program.
 *//*--------------------------------------------------------------------*/

#include "vkShaderProgram.hpp"

#include "tcuTestLog.hpp"

namespace vk
{

GlslSource& GlslSource::operator<< (const glu::ShaderSource& shaderSource)
{
	sources[shaderSource.shaderType].push_back(shaderSource.source);
	return *this;
}

GlslSource& GlslSource::operator<< (const ShaderBuildOptions& buildOptions_)
{
	buildOptions = buildOptions_;
	return *this;
}

HlslSource& HlslSource::operator<< (const glu::ShaderSource& shaderSource)
{
	sources[shaderSource.shaderType].push_back(shaderSource.source);
	return *this;
}

HlslSource& HlslSource::operator<< (const ShaderBuildOptions& buildOptions_)
{
	buildOptions = buildOptions_;
	return *this;
}

tcu::TestLog& logShader(tcu::TestLog& log, const std::vector<std::string> (&sources)[glu::SHADERTYPE_LAST])
{
	log << tcu::TestLog::ShaderProgram(false, "(Source only)");

	try
	{
		for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
		{
			for (size_t shaderNdx = 0; shaderNdx < sources[shaderType].size(); shaderNdx++)
			{
				log << tcu::TestLog::Shader(glu::getLogShaderType((glu::ShaderType)shaderType),
											sources[shaderType][shaderNdx],
											false, "");
			}
		}
	}
	catch (...)
	{
		log << tcu::TestLog::EndShaderProgram;
		throw;
	}

	log << tcu::TestLog::EndShaderProgram;

	return log;
}

tcu::TestLog& operator<< (tcu::TestLog& log, const GlslSource& shaderSource)
{
	return logShader(log, shaderSource.sources);
}

tcu::TestLog& operator<< (tcu::TestLog& log, const HlslSource& shaderSource)
{
	return logShader(log, shaderSource.sources);
}

} // vk
