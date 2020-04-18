/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Tessellation Winding Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationWindingTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuRGBA.hpp"
#include "tcuMaybe.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkStrUtil.hpp"

#include "deUniquePtr.hpp"

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

std::string getCaseName (const TessPrimitiveType primitiveType, const ShaderLanguage shaderLanguage, const Winding winding, bool yFlip)
{
	std::ostringstream str;
	str << getShaderLanguageName(shaderLanguage) << "_" << getTessPrimitiveTypeShaderName(primitiveType) << "_" << getWindingShaderName(winding);
	if (yFlip)
		str << "_yflip";
	return str.str();
}

inline VkFrontFace mapFrontFace (const Winding winding)
{
	switch (winding)
	{
		case WINDING_CCW:	return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		case WINDING_CW:	return VK_FRONT_FACE_CLOCKWISE;
		default:
			DE_ASSERT(false);
			return VK_FRONT_FACE_LAST;
	}
}

//! Returns true when the image passes the verification.
bool verifyResultImage (tcu::TestLog&						log,
						const tcu::ConstPixelBufferAccess	image,
						const TessPrimitiveType				primitiveType,
						const VkTessellationDomainOriginKHR	domainOrigin,
						const Winding						winding,
						bool								yFlip,
						const Winding						frontFaceWinding)
{
	const bool			expectVisiblePrimitive	= ((frontFaceWinding == winding) == (domainOrigin == VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT_KHR)) != yFlip;

	const int			totalNumPixels			= image.getWidth()*image.getHeight();

	const tcu::Vec4		white					 = tcu::RGBA::white().toVec();
	const tcu::Vec4		red						 = tcu::RGBA::red().toVec();

	int					numWhitePixels			= 0;
	int					numRedPixels			= 0;

	// Count red and white pixels
	for (int y = 0; y < image.getHeight();	y++)
	for (int x = 0; x < image.getWidth();	x++)
	{
		numWhitePixels += image.getPixel(x, y) == white ? 1 : 0;
		numRedPixels   += image.getPixel(x, y) == red   ? 1 : 0;
	}

	DE_ASSERT(numWhitePixels + numRedPixels <= totalNumPixels);

	log << tcu::TestLog::Message << "Note: got " << numWhitePixels << " white and " << numRedPixels << " red pixels" << tcu::TestLog::EndMessage;

	{
		const int otherPixels = totalNumPixels - numWhitePixels - numRedPixels;
		if (otherPixels > 0)
		{
			log << tcu::TestLog::Message
				<< "Failure: Got " << otherPixels << " other than white or red pixels"
				<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	if (expectVisiblePrimitive)
	{
		if (primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
		{
			const int	badPixelTolerance	= (primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 5*de::max(image.getWidth(), image.getHeight()) : 0);

			if (de::abs(numWhitePixels - totalNumPixels/2) > badPixelTolerance)
			{
				log << tcu::TestLog::Message << "Failure: wrong number of white pixels; expected approximately " << totalNumPixels/2 << tcu::TestLog::EndMessage;
				return false;
			}

			// Check number of filled pixels (from left) in top and bottom rows to
			// determine if triangle is in right orientation.
			{
				const tcu::IVec2	expectedStart	(0, 1);
				const tcu::IVec2	expectedEnd		(image.getWidth()-1, image.getWidth());
				const tcu::IVec2	expectedTop		= yFlip ? expectedStart : expectedEnd;
				const tcu::IVec2	expectedBottom	= yFlip ? expectedEnd : expectedStart;
				int					numTopFilled	= 0;
				int					numBottomFilled	= 0;

				for (int x = 0; x < image.getWidth(); ++x)
				{
					if (image.getPixel(x, 0) == white)
						numTopFilled += 1;
					else
						break;
				}

				for (int x = 0; x < image.getWidth(); ++x)
				{
					if (image.getPixel(x, image.getHeight()-1) == white)
						numBottomFilled += 1;
					else
						break;
				}

				if (!de::inBounds(numTopFilled, expectedTop[0], expectedTop[1]) ||
					!de::inBounds(numBottomFilled, expectedBottom[0], expectedBottom[1]))
				{
					log << tcu::TestLog::Message << "Failure: triangle orientation is incorrect" << tcu::TestLog::EndMessage;
					return false;
				}
			}

		}
		else if (primitiveType == TESSPRIMITIVETYPE_QUADS)
		{
			if (numWhitePixels != totalNumPixels)
			{
				log << tcu::TestLog::Message << "Failure: expected only white pixels (full-viewport quad)" << tcu::TestLog::EndMessage;
				return false;
			}
		}
		else
			DE_ASSERT(false);
	}
	else
	{
		if (numWhitePixels != 0)
		{
			log << tcu::TestLog::Message << "Failure: expected only red pixels (everything culled)" << tcu::TestLog::EndMessage;
			return false;
		}
	}

	return true;
}

typedef tcu::Maybe<VkTessellationDomainOriginKHR> MaybeDomainOrigin;

class WindingTest : public TestCase
{
public:
								WindingTest		(tcu::TestContext&			testCtx,
												 const TessPrimitiveType	primitiveType,
												 const MaybeDomainOrigin&	domainOrigin,
												 const ShaderLanguage		shaderLanguage,
												 const Winding				winding,
												 bool						yFlip);

	void						initPrograms	(SourceCollections&			programCollection) const;
	TestInstance*				createInstance	(Context&					context) const;

private:
	const TessPrimitiveType		m_primitiveType;
	const MaybeDomainOrigin		m_domainOrigin;
	const ShaderLanguage		m_shaderLanguage;
	const Winding				m_winding;
	const bool					m_yFlip;
};

WindingTest::WindingTest (tcu::TestContext&			testCtx,
						  const TessPrimitiveType	primitiveType,
						  const MaybeDomainOrigin&	domainOrigin,
						  const ShaderLanguage		shaderLanguage,
						  const Winding				winding,
						  bool						yFlip)
	: TestCase			(testCtx, getCaseName(primitiveType, shaderLanguage, winding, yFlip), "")
	, m_primitiveType	(primitiveType)
	, m_domainOrigin	(domainOrigin)
	, m_shaderLanguage	(shaderLanguage)
	, m_winding			(winding)
	, m_yFlip			(yFlip)
{
}

void WindingTest::initPrograms (SourceCollections& programCollection) const
{
	if (m_shaderLanguage == SHADER_LANGUAGE_GLSL)
	{
		// Vertex shader - no inputs
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "}\n";

			programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
		}

		// Tessellation control shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_tessellation_shader : require\n"
				<< "\n"
				<< "layout(vertices = 1) out;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    gl_TessLevelInner[0] = 5.0;\n"
				<< "    gl_TessLevelInner[1] = 5.0;\n"
				<< "\n"
				<< "    gl_TessLevelOuter[0] = 5.0;\n"
				<< "    gl_TessLevelOuter[1] = 5.0;\n"
				<< "    gl_TessLevelOuter[2] = 5.0;\n"
				<< "    gl_TessLevelOuter[3] = 5.0;\n"
				<< "}\n";

			programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
		}

		// Tessellation evaluation shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_tessellation_shader : require\n"
				<< "\n"
				<< "layout(" << getTessPrimitiveTypeShaderName(m_primitiveType) << ", "
							 << getWindingShaderName(m_winding) << ") in;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    gl_Position = vec4(gl_TessCoord.xy*2.0 - 1.0, 0.0, 1.0);\n"
				<< "}\n";

			programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
		}

		// Fragment shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "\n"
				<< "layout(location = 0) out mediump vec4 o_color;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    o_color = vec4(1.0);\n"
				<< "}\n";

			programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
		}
	}
	else
	{
		// Vertex shader - no inputs
		{
			std::ostringstream src;
			src << "void main (void)\n"
				<< "{\n"
				<< "}\n";

			programCollection.hlslSources.add("vert") << glu::VertexSource(src.str());
		}

		// Tessellation control shader
		{
			std::ostringstream src;
			src << "struct HS_CONSTANT_OUT\n"
				<< "{\n"
				<< "    float tessLevelsOuter[4] : SV_TessFactor;\n"
				<< "    float tessLevelsInner[2] : SV_InsideTessFactor;\n"
				<< "};\n"
				<< "\n"
				<< "[domain(\"" << getDomainName(m_primitiveType) << "\")]\n"
				<< "[partitioning(\"integer\")]\n"
				<< "[outputtopology(\"" << getOutputTopologyName (m_primitiveType, m_winding, false) << "\")]\n"
				<< "[outputcontrolpoints(1)]\n"
				<< "[patchconstantfunc(\"PCF\")]\n"
				<< "void main()\n"
				<< "{\n"
				<< "}\n"
				<< "\n"
				<< "HS_CONSTANT_OUT PCF()\n"
				<< "{\n"
				<< "    HS_CONSTANT_OUT output;\n"
				<< "    output.tessLevelsInner[0] = 5.0;\n"
				<< "    output.tessLevelsInner[1] = 5.0;\n"
				<< "    output.tessLevelsOuter[0] = 5.0;\n"
				<< "    output.tessLevelsOuter[1] = 5.0;\n"
				<< "    output.tessLevelsOuter[2] = 5.0;\n"
				<< "    output.tessLevelsOuter[3] = 5.0;\n"
				<< "    return output;\n"
				<< "}\n";

			programCollection.hlslSources.add("tesc") << glu::TessellationControlSource(src.str());
		}

		// Tessellation evaluation shader
		{
			std::ostringstream src;

			src	<< "float4 main(" << (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? "float3" : "float2") << " tessCoords : SV_DOMAINLOCATION) : SV_POSITION\n"
				<< "{\n"
				<< "    return float4(tessCoords.xy*2.0 - 1, 0.0, 1.0);\n"
				<< "}\n";

			programCollection.hlslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
		}

		// Fragment shader
		{
			std::ostringstream src;
			src << "float4 main (void) : COLOR0\n"
				<< "{\n"
				<< "    return float4(1.0);\n"
				<< "}\n";

			programCollection.hlslSources.add("frag") << glu::FragmentSource(src.str());
		}
	}
}

class WindingTestInstance : public TestInstance
{
public:
								WindingTestInstance (Context&					context,
													 const TessPrimitiveType	primitiveType,
													 const MaybeDomainOrigin&	domainOrigin,
													 const Winding				winding,
													 bool						yFlip);

	tcu::TestStatus				iterate				(void);

private:
	void						requireExtension	(const char* name) const;

	const TessPrimitiveType		m_primitiveType;
	const MaybeDomainOrigin		m_domainOrigin;
	const Winding				m_winding;
	const bool					m_yFlip;
};

WindingTestInstance::WindingTestInstance (Context&					context,
										  const TessPrimitiveType	primitiveType,
										  const MaybeDomainOrigin&	domainOrigin,
										  const Winding				winding,
										  bool						yFlip)
	: TestInstance		(context)
	, m_primitiveType	(primitiveType)
	, m_domainOrigin	(domainOrigin)
	, m_winding			(winding)
	, m_yFlip			(yFlip)
{
	if (m_yFlip)
		requireExtension("VK_KHR_maintenance1");

	if ((bool)m_domainOrigin)
		requireExtension("VK_KHR_maintenance2");
}

void WindingTestInstance::requireExtension (const char* name) const
{
	if (!de::contains(m_context.getDeviceExtensions().begin(), m_context.getDeviceExtensions().end(), name))
		TCU_THROW(NotSupportedError, (std::string(name) + " is not supported").c_str());
}

tcu::TestStatus WindingTestInstance::iterate (void)
{
	if (m_yFlip && !de::contains(m_context.getDeviceExtensions().begin(), m_context.getDeviceExtensions().end(), "VK_KHR_maintenance1"))
		TCU_THROW(NotSupportedError, "Extension VK_KHR_maintenance1 not supported");

	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Color attachment

	const tcu::IVec2			  renderSize				 = tcu::IVec2(64, 64);
	const VkFormat				  colorFormat				 = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageSubresourceRange colorImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Image					  colorAttachmentImage		 (vk, device, allocator,
															 makeImageCreateInfo(renderSize, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1u),
															 MemoryRequirement::Any);

	// Color output buffer: image will be copied here for verification

	const VkDeviceSize	colorBufferSizeBytes = renderSize.x()*renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer		colorBuffer			 (vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Pipeline

	const Unique<VkImageView>		colorAttachmentView(makeImageView                       (vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange));
	const Unique<VkRenderPass>		renderPass         (makeRenderPass                      (vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer        (makeFramebuffer                     (vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkPipelineLayout>	pipelineLayout     (makePipelineLayoutWithoutDescriptors(vk, device));

	const VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;

	// Front face is static state, so we have to create two pipelines.

	const Unique<VkPipeline> pipelineCounterClockwise(GraphicsPipelineBuilder()
		.setCullModeFlags				(cullMode)
		.setFrontFace					(VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.setShader						(vk, device, VK_SHADER_STAGE_VERTEX_BIT,				   m_context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader						(vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,    m_context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader						(vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get("tese"), DE_NULL)
		.setShader						(vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				   m_context.getBinaryCollection().get("frag"), DE_NULL)
		.setTessellationDomainOrigin	(m_domainOrigin)
		.build							(vk, device, *pipelineLayout, *renderPass));

	const Unique<VkPipeline> pipelineClockwise(GraphicsPipelineBuilder()
		.setCullModeFlags				(cullMode)
		.setFrontFace					(VK_FRONT_FACE_CLOCKWISE)
		.setShader						(vk, device, VK_SHADER_STAGE_VERTEX_BIT,				   m_context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader						(vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	   m_context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader						(vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get("tese"), DE_NULL)
		.setShader						(vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				   m_context.getBinaryCollection().get("frag"), DE_NULL)
		.setTessellationDomainOrigin	(m_domainOrigin)
		.build							(vk, device, *pipelineLayout, *renderPass));

	const struct // not static
	{
		Winding		frontFaceWinding;
		VkPipeline	pipeline;
	} testCases[] =
	{
		{ WINDING_CCW,	*pipelineCounterClockwise },
		{ WINDING_CW,	*pipelineClockwise		  },
	};

	tcu::TestLog& log = m_context.getTestContext().getLog();
	log << tcu::TestLog::Message << "Pipeline uses " << getCullModeFlagsStr(cullMode) << tcu::TestLog::EndMessage;

	bool success = true;

	// Draw commands

	const Unique<VkCommandPool>   cmdPool  (makeCommandPool  (vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(testCases); ++caseNdx)
	{
		const Winding frontFaceWinding = testCases[caseNdx].frontFaceWinding;

		log << tcu::TestLog::Message << "Setting " << getFrontFaceName(mapFrontFace(frontFaceWinding)) << tcu::TestLog::EndMessage;

		// Reset the command buffer and begin.
		beginCommandBuffer(vk, *cmdBuffer);

		// Change color attachment image layout
		{
			// State is slightly different on the first iteration.
			const VkImageLayout currentLayout = (caseNdx == 0 ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			const VkAccessFlags srcFlags	  = (caseNdx == 0 ? (VkAccessFlags)0          : (VkAccessFlags)VK_ACCESS_TRANSFER_READ_BIT);

			const VkImageMemoryBarrier colorAttachmentLayoutBarrier = makeImageMemoryBarrier(
				srcFlags, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				*colorAttachmentImage, colorImageSubresourceRange);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentLayoutBarrier);
		}

		// Begin render pass
		{
			const VkRect2D renderArea = {
				makeOffset2D(0, 0),
				makeExtent2D(renderSize.x(), renderSize.y()),
			};
			const tcu::Vec4 clearColor = tcu::RGBA::red().toVec();

			beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderArea, clearColor);
		}

		const VkViewport viewport =
		{
			0.0f,															// float	x;
			m_yFlip ? static_cast<float>(renderSize.y()) : 0.0f,			// float	y;
			static_cast<float>(renderSize.x()),								// float	width;
			static_cast<float>(m_yFlip ? -renderSize.y() : renderSize.y()),	// float	height;
			0.0f,															// float	minDepth;
			1.0f,															// float	maxDepth;
		};
		vk.cmdSetViewport(*cmdBuffer, 0, 1, &viewport);

		const VkRect2D scissor =
		{
			makeOffset2D(0, 0),
			makeExtent2D(renderSize.x(), renderSize.y()),
		};
		vk.cmdSetScissor(*cmdBuffer, 0, 1, &scissor);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, testCases[caseNdx].pipeline);

		// Process a single abstract vertex.
		vk.cmdDraw(*cmdBuffer, 1u, 1u, 0u, 0u);
		endRenderPass(vk, *cmdBuffer);

		// Copy render result to a host-visible buffer
		{
			const VkImageMemoryBarrier colorAttachmentPreCopyBarrier = makeImageMemoryBarrier(
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*colorAttachmentImage, colorImageSubresourceRange);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentPreCopyBarrier);
		}
		{
			const VkBufferImageCopy copyRegion = makeBufferImageCopy(makeExtent3D(renderSize.x(), renderSize.y(), 1), makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u));
			vk.cmdCopyImageToBuffer(*cmdBuffer, *colorAttachmentImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &copyRegion);
		}
		{
			const VkBufferMemoryBarrier postCopyBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *colorBuffer, 0ull, colorBufferSizeBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, 1u, &postCopyBarrier, 0u, DE_NULL);
		}

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);

		{
			// Log rendered image
			const Allocation& colorBufferAlloc = colorBuffer.getAllocation();
			invalidateMappedMemoryRange(vk, device, colorBufferAlloc.getMemory(), colorBufferAlloc.getOffset(), colorBufferSizeBytes);

			const tcu::ConstPixelBufferAccess imagePixelAccess(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1, colorBufferAlloc.getHostPtr());
			log << tcu::TestLog::Image("color0", "Rendered image", imagePixelAccess);

			// Verify case result
			success = verifyResultImage(log,
										imagePixelAccess,
										m_primitiveType,
										!m_domainOrigin ? VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT_KHR : *m_domainOrigin,
										m_winding,
										m_yFlip,
										frontFaceWinding) && success;
		}
	}  // for windingNdx

	return (success ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Failure"));
}

TestInstance* WindingTest::createInstance (Context& context) const
{
	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER);

	return new WindingTestInstance(context, m_primitiveType, m_domainOrigin, m_winding, m_yFlip);
}

void populateWindingGroup (tcu::TestCaseGroup* group, tcu::Maybe<VkTessellationDomainOriginKHR> domainOrigin)
{
	static const TessPrimitiveType primitivesNoIsolines[] =
	{
		TESSPRIMITIVETYPE_TRIANGLES,
		TESSPRIMITIVETYPE_QUADS,
	};

	static const ShaderLanguage shaderLanguage[] =
	{
		SHADER_LANGUAGE_GLSL,
		SHADER_LANGUAGE_HLSL,
	};

	for (int primitiveTypeNdx = 0; primitiveTypeNdx < DE_LENGTH_OF_ARRAY(primitivesNoIsolines); ++primitiveTypeNdx)
	for (int shaderLanguageNdx = 0; shaderLanguageNdx < DE_LENGTH_OF_ARRAY(shaderLanguage); ++shaderLanguageNdx)
	for (int windingNdx = 0; windingNdx < WINDING_LAST; ++windingNdx)
	{
		group->addChild(new WindingTest(group->getTestContext(), primitivesNoIsolines[primitiveTypeNdx], domainOrigin, shaderLanguage[shaderLanguageNdx], (Winding)windingNdx, false));
		group->addChild(new WindingTest(group->getTestContext(), primitivesNoIsolines[primitiveTypeNdx], domainOrigin, shaderLanguage[shaderLanguageNdx], (Winding)windingNdx, true));
	}
}

} // anonymous

//! These tests correspond to dEQP-GLES31.functional.tessellation.winding.*
tcu::TestCaseGroup* createWindingTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "winding", "Test the cw and ccw input layout qualifiers"));

	addTestGroup(group.get(), "default_domain",		"No tessellation domain specified",	populateWindingGroup,	tcu::nothing<VkTessellationDomainOriginKHR>());
	addTestGroup(group.get(), "lower_left_domain",	"Lower left tessellation domain",	populateWindingGroup,	tcu::just(VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT_KHR));
	addTestGroup(group.get(), "upper_left_domain",	"Upper left tessellation domain",	populateWindingGroup,	tcu::just(VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT_KHR));

	return group.release();
}

} // tessellation
} // vkt
