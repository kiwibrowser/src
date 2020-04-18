#ifndef _VKTTEXTURETESTUTIL_HPP
#define _VKTTEXTURETESTUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Texture test utilities.
 *
 * About coordinates:
 *  + Quads consist of 2 triangles, rendered using explicit indices.
 *  + All TextureTestUtil functions and classes expect texture coordinates
 *    for quads to be specified in order (-1, -1), (-1, 1), (1, -1), (1, 1).
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuSurface.hpp"

#include "vkDefs.hpp"
#include "vktTestCase.hpp"

#include "gluShaderProgram.hpp"
#include "gluTextureTestUtil.hpp"
#include "deSharedPtr.hpp"

#include "../pipeline/vktPipelineImageUtil.hpp"

namespace vkt
{

namespace texture
{

namespace util
{

enum Program
{
	PROGRAM_2D_FLOAT = 0,
	PROGRAM_2D_INT,
	PROGRAM_2D_UINT,
	PROGRAM_2D_SHADOW,

	PROGRAM_2D_FLOAT_BIAS,
	PROGRAM_2D_INT_BIAS,
	PROGRAM_2D_UINT_BIAS,
	PROGRAM_2D_SHADOW_BIAS,

	PROGRAM_1D_FLOAT,
	PROGRAM_1D_INT,
	PROGRAM_1D_UINT,
	PROGRAM_1D_SHADOW,

	PROGRAM_1D_FLOAT_BIAS,
	PROGRAM_1D_INT_BIAS,
	PROGRAM_1D_UINT_BIAS,
	PROGRAM_1D_SHADOW_BIAS,

	PROGRAM_CUBE_FLOAT,
	PROGRAM_CUBE_INT,
	PROGRAM_CUBE_UINT,
	PROGRAM_CUBE_SHADOW,

	PROGRAM_CUBE_FLOAT_BIAS,
	PROGRAM_CUBE_INT_BIAS,
	PROGRAM_CUBE_UINT_BIAS,
	PROGRAM_CUBE_SHADOW_BIAS,

	PROGRAM_1D_ARRAY_FLOAT,
	PROGRAM_1D_ARRAY_INT,
	PROGRAM_1D_ARRAY_UINT,
	PROGRAM_1D_ARRAY_SHADOW,

	PROGRAM_2D_ARRAY_FLOAT,
	PROGRAM_2D_ARRAY_INT,
	PROGRAM_2D_ARRAY_UINT,
	PROGRAM_2D_ARRAY_SHADOW,

	PROGRAM_3D_FLOAT,
	PROGRAM_3D_INT,
	PROGRAM_3D_UINT,

	PROGRAM_3D_FLOAT_BIAS,
	PROGRAM_3D_INT_BIAS,
	PROGRAM_3D_UINT_BIAS,

	PROGRAM_CUBE_ARRAY_FLOAT,
	PROGRAM_CUBE_ARRAY_INT,
	PROGRAM_CUBE_ARRAY_UINT,
	PROGRAM_CUBE_ARRAY_SHADOW,

	PROGRAM_BUFFER_FLOAT,
	PROGRAM_BUFFER_INT,
	PROGRAM_BUFFER_UINT,

	PROGRAM_LAST
};

void initializePrograms(vk::SourceCollections& programCollection, glu::Precision texCoordPrecision, const std::vector<Program>& programs);

typedef de::SharedPtr<pipeline::TestTexture>		TestTextureSp;
typedef de::SharedPtr<pipeline::TestTexture2D>		TestTexture2DSp;
typedef de::SharedPtr<pipeline::TestTextureCube>	TestTextureCubeSp;
typedef de::SharedPtr<pipeline::TestTexture2DArray>	TestTexture2DArraySp;
typedef de::SharedPtr<pipeline::TestTexture3D>		TestTexture3DSp;

class TextureBinding {
public:
	enum Type
	{
		TYPE_NONE = 0,
		TYPE_2D,
		TYPE_CUBE_MAP,
		TYPE_2D_ARRAY,
		TYPE_3D,

		TYPE_LAST
	};

	enum ImageBackingMode
	{
		IMAGE_BACKING_MODE_REGULAR = 0,
		IMAGE_BACKING_MODE_SPARSE,

		IMAGE_BACKING_MODE_LAST
	};
													TextureBinding				(Context& context);
													TextureBinding				(Context& context, const TestTextureSp& textureData, const Type type,
																				 const ImageBackingMode backingMode = IMAGE_BACKING_MODE_REGULAR);
	vk::VkImage										getImage					(void) { return *m_textureImage; }
	vk::VkImageView									getImageView				(void) { return *m_textureImageView; }
	Type											getType						(void) { return m_type; }
	const pipeline::TestTexture&					getTestTexture				(void) { return *m_textureData; }
	void											updateTextureViewMipLevels	(deUint32 baseLevel, deUint32 maxLevel);

private:
													TextureBinding				(const TextureBinding&);	// not allowed!
	TextureBinding&									operator=					(const TextureBinding&);	// not allowed!

	void											updateTextureData			(const TestTextureSp& textureData, const Type type);

	Context&										m_context;
	Type											m_type;
	ImageBackingMode								m_backingMode;
	TestTextureSp									m_textureData;
	vk::Move<vk::VkImage>							m_textureImage;
	de::MovePtr<vk::Allocation>						m_textureImageMemory;
	vk::Move<vk::VkImageView>						m_textureImageView;
	std::vector<de::SharedPtr<vk::Allocation> >		m_allocations;
};

typedef de::SharedPtr<TextureBinding>	TextureBindingSp;

class TextureRenderer
{
public:
										TextureRenderer				(Context& context, vk::VkSampleCountFlagBits sampleCount, deUint32 renderWidth, deUint32 renderHeight);
										~TextureRenderer			(void);

	void								renderQuad					(tcu::Surface& result, int texUnit, const float* texCoord, glu::TextureTestUtil::TextureType texType);
	void								renderQuad					(tcu::Surface& result, int texUnit, const float* texCoord, const glu::TextureTestUtil::ReferenceParams& params);
	void								renderQuad					(tcu::Surface&									result,
																	 const float*									positions,
																	 const int										texUnit,
																	 const float*									texCoord,
																	 const glu::TextureTestUtil::ReferenceParams&	params,
																	 const float									maxAnisotropy);

	void								clearImage					(vk::VkImage image);
	void								add2DTexture				(const TestTexture2DSp& texture,
																	 TextureBinding::ImageBackingMode backingMode = TextureBinding::IMAGE_BACKING_MODE_REGULAR);
	const pipeline::TestTexture2D&		get2DTexture				(int textureIndex) const;

	void								addCubeTexture				(const TestTextureCubeSp& texture,
																	 TextureBinding::ImageBackingMode backingMode = TextureBinding::IMAGE_BACKING_MODE_REGULAR);
	const pipeline::TestTextureCube&	getCubeTexture				(int textureIndex) const;

	void								add2DArrayTexture			(const TestTexture2DArraySp& texture,
																	 TextureBinding::ImageBackingMode backingMode = TextureBinding::IMAGE_BACKING_MODE_REGULAR);
	const pipeline::TestTexture2DArray&	get2DArrayTexture			(int textureIndex) const;

	void								add3DTexture				(const TestTexture3DSp& texture,
																	 TextureBinding::ImageBackingMode backingMode = TextureBinding::IMAGE_BACKING_MODE_REGULAR);
	const pipeline::TestTexture3D&		get3DTexture				(int textureIndex) const;

	void								setViewport					(float viewportX, float viewportY, float viewportW, float viewportH);

	TextureBinding*						getTextureBinding			(int textureIndex) const;

	deUint32							getRenderWidth				(void) const;
	deUint32							getRenderHeight				(void) const;

protected:
										TextureRenderer				(const TextureRenderer& other);
	TextureRenderer&					operator=					(const TextureRenderer& other);

	Context&							m_context;
	tcu::TestLog&						m_log;

	const deUint32						m_renderWidth;
	const deUint32						m_renderHeight;
	const vk::VkSampleCountFlagBits		m_sampleCount;
	const deBool						m_multisampling;

	const vk::VkFormat					m_imageFormat;
	const tcu::TextureFormat			m_textureFormat;

	vk::Move<vk::VkImage>				m_image;
	de::MovePtr<vk::Allocation>			m_imageMemory;
	vk::Move<vk::VkImageView>			m_imageView;

	vk::Move<vk::VkImage>				m_resolvedImage;
	de::MovePtr<vk::Allocation>			m_resolvedImageMemory;
	vk::Move<vk::VkImageView>			m_resolvedImageView;

	vk::Move<vk::VkCommandPool>			m_commandPool;
	vk::Move<vk::VkRenderPass>			m_renderPass;
	vk::Move<vk::VkFramebuffer>			m_frameBuffer;

	vk::Move<vk::VkDescriptorPool>		m_descriptorPool;

	vk::Move<vk::VkBuffer>				m_uniformBuffer;
	de::MovePtr<vk::Allocation>			m_uniformBufferMemory;
	const vk::VkDeviceSize				m_uniformBufferSize;

	vk::Move<vk::VkBuffer>				m_vertexIndexBuffer;
	de::MovePtr<vk::Allocation>			m_vertexIndexBufferMemory;
	static const vk::VkDeviceSize		s_vertexIndexBufferSize;
	static const deUint16				s_vertexIndices[6];

	vk::Move<vk::VkFence>				m_fence;

	vk::Move<vk::VkBuffer>				m_resultBuffer;
	de::MovePtr<vk::Allocation>			m_resultBufferMemory;
	const vk::VkDeviceSize				m_resultBufferSize;

	std::vector<TextureBindingSp>		m_textureBindings;

	float								m_viewportOffsetX;
	float								m_viewportOffsetY;
	float								m_viewportWidth;
	float								m_viewportHeight;

private:
	vk::Move<vk::VkDescriptorSet>		makeDescriptorSet			(const vk::VkDescriptorPool descriptorPool, const vk::VkDescriptorSetLayout setLayout) const;
	void								addImageTransitionBarrier	(vk::VkCommandBuffer commandBuffer, vk::VkImage image, vk::VkPipelineStageFlags srcStageMask, vk::VkPipelineStageFlags dstStageMask, vk::VkAccessFlags srcAccessMask, vk::VkAccessFlags dstAccessMask, vk::VkImageLayout oldLayout, vk::VkImageLayout newLayout) const;

};

tcu::Sampler createSampler (tcu::Sampler::WrapMode wrapU, tcu::Sampler::WrapMode wrapV, tcu::Sampler::WrapMode wrapW, tcu::Sampler::FilterMode minFilterMode, tcu::Sampler::FilterMode magFilterMode);
tcu::Sampler createSampler (tcu::Sampler::WrapMode wrapU, tcu::Sampler::WrapMode wrapV, tcu::Sampler::FilterMode minFilterMode, tcu::Sampler::FilterMode magFilterMode);
tcu::Sampler createSampler (tcu::Sampler::WrapMode wrapU, tcu::Sampler::FilterMode minFilterMode, tcu::Sampler::FilterMode magFilterMode);

TestTexture2DSp loadTexture2D (const tcu::Archive& archive, const std::vector<std::string>& filenames);
TestTextureCubeSp loadTextureCube (const tcu::Archive& archive, const std::vector<std::string>& filenames);

template <typename INSTANCE_TYPE>
class TextureTestCase : public TestCase
{
public:
										TextureTestCase	(tcu::TestContext& context, const std::string& name, const std::string& description, const typename INSTANCE_TYPE::ParameterType& testParameters)
												: TestCase				(context, name, description)
												, m_testsParameters		(testParameters)
										{}

	virtual TestInstance*				createInstance				(Context& context) const
										{
											return new INSTANCE_TYPE(context, m_testsParameters);
										}

	virtual void						initPrograms				(vk::SourceCollections& programCollection) const
										{
											initializePrograms(programCollection, m_testsParameters.texCoordPrecision, m_testsParameters.programs);
										}

protected:
	const typename INSTANCE_TYPE::ParameterType m_testsParameters;
};

struct TextureCommonTestCaseParameters
{
								TextureCommonTestCaseParameters	(void);

	vk::VkSampleCountFlagBits	sampleCount;
	glu::Precision				texCoordPrecision;

	tcu::Sampler::FilterMode	minFilter;
	tcu::Sampler::FilterMode	magFilter;
	tcu::Sampler::WrapMode		wrapS;
	tcu::Sampler::WrapMode		wrapT;

	vk::VkFormat				format;

	std::vector<util::Program>	programs;
};

struct Texture2DTestCaseParameters : public TextureCommonTestCaseParameters
{
								Texture2DTestCaseParameters		(void);
	int							width;
	int							height;
};

struct TextureCubeTestCaseParameters : public TextureCommonTestCaseParameters
{
								TextureCubeTestCaseParameters	(void);
	int							size;
};

struct Texture2DArrayTestCaseParameters : public Texture2DTestCaseParameters
{
								Texture2DArrayTestCaseParameters(void);
	int							numLayers;
};

struct Texture3DTestCaseParameters : public Texture2DTestCaseParameters
{
								Texture3DTestCaseParameters		(void);
	tcu::Sampler::WrapMode		wrapR;
	int							depth;
};

} // util
} // texture
} // vkt

#endif // _VKTTEXTURETESTUTIL_HPP
