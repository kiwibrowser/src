#ifndef _VKTPIPELINEVERTEXUTIL_HPP
#define _VKTPIPELINEVERTEXUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Utilities for vertex buffers.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "tcuDefs.hpp"
#include "tcuVectorUtil.hpp"

#include <vector>

namespace vkt
{
namespace pipeline
{

struct Vertex4RGBA
{
	tcu::Vec4 position;
	tcu::Vec4 color;
};

struct Vertex4Tex4
{
	tcu::Vec4 position;
	tcu::Vec4 texCoord;
};

deUint32					getVertexFormatSize				(vk::VkFormat format);
deUint32					getVertexFormatComponentCount	(vk::VkFormat format);
deUint32					getVertexFormatComponentSize	(vk::VkFormat format);
bool						isVertexFormatComponentOrderBGR	(vk::VkFormat format);
bool						isVertexFormatSint				(vk::VkFormat format);
bool						isVertexFormatUint				(vk::VkFormat format);
bool						isVertexFormatSfloat			(vk::VkFormat format);
bool						isVertexFormatUfloat			(vk::VkFormat format);
bool						isVertexFormatUnorm				(vk::VkFormat format);
bool						isVertexFormatSnorm				(vk::VkFormat format);
bool						isVertexFormatSRGB				(vk::VkFormat format);
bool						isVertexFormatSscaled			(vk::VkFormat format);
bool						isVertexFormatUscaled			(vk::VkFormat format);
bool						isVertexFormatDouble			(vk::VkFormat format);

/*! \brief Creates a pattern of 4 overlapping quads.
 *
 *  The quads are alined along the plane Z = 0, with X,Y taking values between -1 and 1.
 *  Each quad covers one of the quadrants of the scene and partially extends to the other 3 quadrants.
 *  The triangles of each quad have different winding orders (CW/CCW).
 */
std::vector<Vertex4RGBA>	createOverlappingQuads		(void);

std::vector<Vertex4Tex4>	createFullscreenQuad		(void);
std::vector<Vertex4Tex4>	createQuadMosaic			(int rows, int columns);
std::vector<Vertex4Tex4>	createQuadMosaicCube		(void);
std::vector<Vertex4Tex4>	createQuadMosaicCubeArray	(int faceArrayIndices[6]);

std::vector<Vertex4Tex4>	createTestQuadMosaic		(vk::VkImageViewType viewType);

} // pipeline
} // vkt

#endif // _VKTPIPELINEVERTEXUTIL_HPP
