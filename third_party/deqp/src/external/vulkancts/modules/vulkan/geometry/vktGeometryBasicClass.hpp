#ifndef _VKTGEOMETRYBASICCLASS_HPP
#define _VKTGEOMETRYBASICCLASS_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
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
 *//*!
 * \file
 * \brief Geometry Basic Class
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vktTestCase.hpp"
#include "vkTypeUtil.hpp"
#include "vkRef.hpp"
#include "vktGeometryTestsUtil.hpp"

namespace vkt
{
namespace geometry
{

class GeometryExpanderRenderTestInstance : public TestInstance
{
public:
									GeometryExpanderRenderTestInstance	(Context&						context,
																		 const vk::VkPrimitiveTopology	primitiveType,
																		 const char*					name);

	tcu::TestStatus					iterate								(void);

protected:
	virtual vk::Move<vk::VkPipelineLayout>	createPipelineLayout		(const vk::DeviceInterface& vk, const vk::VkDevice device);
	virtual void							bindDescriptorSets			(const vk::DeviceInterface&		/*vk*/,
																		 const vk::VkDevice				/*device*/,
																		 vk::Allocator&					/*memAlloc*/,
																		 const vk::VkCommandBuffer&		/*cmdBuffer*/,
																		 const vk::VkPipelineLayout&	/*pipelineLayout*/){};
	virtual void						drawCommand						(const vk::VkCommandBuffer&		cmdBuffer);

	const vk::VkPrimitiveTopology	m_primitiveType;
	const std::string				m_name;
	int								m_numDrawVertices;
	std::vector<tcu::Vec4>			m_vertexPosData;
	std::vector<tcu::Vec4>			m_vertexAttrData;

};

} // geometry
} // vkt

#endif // _VKTGEOMETRYBASICCLASS_HPP
