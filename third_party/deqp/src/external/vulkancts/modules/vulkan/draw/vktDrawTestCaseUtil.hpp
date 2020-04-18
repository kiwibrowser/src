#ifndef _VKTDRAWTESTCASEUTIL_HPP
#define _VKTDRAWTESTCASEUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Intel Corporation
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
 * \brief Draw Test Case Utils
 *//*--------------------------------------------------------------------*/


#include "tcuDefs.hpp"
#include "tcuResource.hpp"

#include "vktTestCase.hpp"

#include "gluShaderUtil.hpp"
#include "vkPrograms.hpp"

#include "deUniquePtr.hpp"

#include <map>

namespace vkt
{
namespace Draw
{

class ShaderSourceProvider
{
public:
	static std::string getSource (tcu::Archive& archive, const char* path)
	{
		de::UniquePtr<tcu::Resource> resource(archive.getResource(path));

		std::vector<deUint8> readBuffer(resource->getSize() + 1);
		resource->read(&readBuffer[0], resource->getSize());
		readBuffer[readBuffer.size() - 1] = 0;

		return std::string(reinterpret_cast<const char*>(&readBuffer[0]));
	}
};

typedef std::map<glu::ShaderType, const char*> ShaderMap;

struct TestSpecBase
{
	ShaderMap				shaders;
	vk::VkPrimitiveTopology	topology;
};

template<typename Instance>
class InstanceFactory : public TestCase
{
public:
	InstanceFactory (tcu::TestContext& testCtx, const std::string& name, const std::string& desc, typename Instance::TestSpec testSpec)
		: TestCase		(testCtx, name, desc)
		, m_testSpec	(testSpec)
	{
	}

	TestInstance* createInstance (Context& context) const
	{
		return new Instance(context, m_testSpec);
	}

	virtual void initPrograms (vk::SourceCollections& programCollection) const
	{
		for (ShaderMap::const_iterator i = m_testSpec.shaders.begin(); i != m_testSpec.shaders.end(); ++i)
		{
			programCollection.glslSources.add(i->second) <<
				glu::ShaderSource(i->first, ShaderSourceProvider::getSource(m_testCtx.getArchive(), i->second));
		}
	}

private:
	const typename Instance::TestSpec m_testSpec;
};

} // Draw
} // vkt

#endif // _VKTDRAWTESTCASEUTIL_HPP
