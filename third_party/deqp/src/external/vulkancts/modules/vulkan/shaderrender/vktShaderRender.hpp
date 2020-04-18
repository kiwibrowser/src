#ifndef _VKTSHADERRENDER_HPP
#define _VKTSHADERRENDER_HPP
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
 * \brief Vulkan ShaderRenderCase
 *//*--------------------------------------------------------------------*/

#include "tcuTexture.hpp"
#include "tcuSurface.hpp"

#include "deMemory.h"
#include "deSharedPtr.hpp"
#include "deUniquePtr.hpp"

#include "vkDefs.hpp"
#include "vkRefUtil.hpp"
#include "vkPrograms.hpp"
#include "vkRef.hpp"
#include "vkMemUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkPlatform.hpp"

#include "vktTestCaseUtil.hpp"

namespace vkt
{
namespace sr
{

class LineStream
{
public:
						LineStream		(int indent = 0)	{ m_indent = indent; }
						~LineStream		(void)				{}

	const char*			str				(void) const		{ m_string = m_stream.str(); return m_string.c_str(); }
	LineStream&			operator<<		(const char* line)	{ for (int i = 0; i < m_indent; i++) { m_stream << "\t"; } m_stream << line << "\n"; return *this; }

private:
	int					m_indent;
	std::ostringstream	m_stream;
	mutable std::string	m_string;
};

class QuadGrid;
class ShaderRenderCaseInstance;

class TextureBinding
{
public:
	enum Type
	{
		TYPE_NONE = 0,
		TYPE_1D,
		TYPE_2D,
		TYPE_3D,
		TYPE_CUBE_MAP,
		TYPE_1D_ARRAY,
		TYPE_2D_ARRAY,
		TYPE_CUBE_ARRAY,

		TYPE_LAST
	};

	enum Init
	{
		INIT_UPLOAD_DATA,
		INIT_CLEAR,

		INIT_LAST
	};

	struct Parameters
	{
		deUint32					baseMipLevel;
		vk::VkComponentMapping		componentMapping;
		vk::VkSampleCountFlagBits	samples;
		Init						initialization;

		Parameters (deUint32					baseMipLevel_		= 0,
					vk::VkComponentMapping		componentMapping_	= vk::makeComponentMappingRGBA(),
					vk::VkSampleCountFlagBits	samples_			= vk::VK_SAMPLE_COUNT_1_BIT,
					Init						initialization_		= INIT_UPLOAD_DATA)
			: baseMipLevel		(baseMipLevel_)
			, componentMapping	(componentMapping_)
			, samples			(samples_)
			, initialization	(initialization_)
		{
		}
	};

										TextureBinding		(const tcu::Archive&	archive,
															const char*				filename,
															const Type				type,
															const tcu::Sampler&		sampler);

										TextureBinding		(const tcu::Texture1D* tex1D, const tcu::Sampler& sampler);
										TextureBinding		(const tcu::Texture2D* tex2D, const tcu::Sampler& sampler);
										TextureBinding		(const tcu::Texture3D* tex3D, const tcu::Sampler& sampler);
										TextureBinding		(const tcu::TextureCube* texCube, const tcu::Sampler& sampler);
										TextureBinding		(const tcu::Texture1DArray* tex1DArray, const tcu::Sampler& sampler);
										TextureBinding		(const tcu::Texture2DArray* tex2DArray, const tcu::Sampler& sampler);
										TextureBinding		(const tcu::TextureCubeArray* texCubeArray, const tcu::Sampler& sampler);

										~TextureBinding		(void);

	Type								getType				(void) const { return m_type;		}
	const tcu::Sampler&					getSampler			(void) const { return m_sampler;	}

	const tcu::Texture1D&				get1D				(void) const { DE_ASSERT(getType() == TYPE_1D && m_binding.tex1D != NULL);					return *m_binding.tex1D;		}
	const tcu::Texture2D&				get2D				(void) const { DE_ASSERT(getType() == TYPE_2D && m_binding.tex2D != NULL);					return *m_binding.tex2D;		}
	const tcu::Texture3D&				get3D				(void) const { DE_ASSERT(getType() == TYPE_3D && m_binding.tex3D != NULL);					return *m_binding.tex3D;		}
	const tcu::TextureCube&				getCube				(void) const { DE_ASSERT(getType() == TYPE_CUBE_MAP && m_binding.texCube != NULL);			return *m_binding.texCube;		}
	const tcu::Texture1DArray&			get1DArray			(void) const { DE_ASSERT(getType() == TYPE_1D_ARRAY && m_binding.tex1DArray != NULL);		return *m_binding.tex1DArray;	}
	const tcu::Texture2DArray&			get2DArray			(void) const { DE_ASSERT(getType() == TYPE_2D_ARRAY && m_binding.tex2DArray != NULL);		return *m_binding.tex2DArray;	}
	const tcu::TextureCubeArray&		getCubeArray		(void) const { DE_ASSERT(getType() == TYPE_CUBE_ARRAY && m_binding.texCubeArray != NULL);	return *m_binding.texCubeArray;	}

	void								setParameters		(const Parameters& params) { m_params = params; }
	const Parameters&					getParameters		(void) const { return m_params; }

private:
										TextureBinding		(const TextureBinding&);	// not allowed!
	TextureBinding&						operator=			(const TextureBinding&);	// not allowed!

	static de::MovePtr<tcu::Texture2D>	loadTexture2D		(const tcu::Archive& archive, const char* filename);

	Type								m_type;
	tcu::Sampler						m_sampler;
	Parameters							m_params;

	union
	{
		const tcu::Texture1D*			tex1D;
		const tcu::Texture2D*			tex2D;
		const tcu::Texture3D*			tex3D;
		const tcu::TextureCube*			texCube;
		const tcu::Texture1DArray*		tex1DArray;
		const tcu::Texture2DArray*		tex2DArray;
		const tcu::TextureCubeArray*	texCubeArray;
	} m_binding;
};

typedef de::SharedPtr<TextureBinding> TextureBindingSp;

// ShaderEvalContext.

class ShaderEvalContext
{
public:
	// Limits.
	enum
	{
		MAX_USER_ATTRIBS	= 4,
		MAX_TEXTURES		= 4
	};

	struct ShaderSampler
	{
		tcu::Sampler					sampler;
		const tcu::Texture1D*			tex1D;
		const tcu::Texture2D*			tex2D;
		const tcu::Texture3D*			tex3D;
		const tcu::TextureCube*			texCube;
		const tcu::Texture1DArray*		tex1DArray;
		const tcu::Texture2DArray*		tex2DArray;
		const tcu::TextureCubeArray*	texCubeArray;

		inline ShaderSampler (void)
			: tex1D			(DE_NULL)
			, tex2D			(DE_NULL)
			, tex3D			(DE_NULL)
			, texCube		(DE_NULL)
			, tex1DArray	(DE_NULL)
			, tex2DArray	(DE_NULL)
			, texCubeArray	(DE_NULL)
		{
		}
	};

							ShaderEvalContext		(const QuadGrid& quadGrid);
							~ShaderEvalContext		(void);

	void					reset					(float sx, float sy);

	// Inputs.
	tcu::Vec4				coords;
	tcu::Vec4				unitCoords;
	tcu::Vec4				constCoords;

	tcu::Vec4				in[MAX_USER_ATTRIBS];
	ShaderSampler			textures[MAX_TEXTURES];

	// Output.
	tcu::Vec4				color;
	bool					isDiscarded;

	// Functions.
	inline void				discard					(void)  { isDiscarded = true; }
	tcu::Vec4				texture2D				(int unitNdx, const tcu::Vec2& coords);

private:
	const QuadGrid&			m_quadGrid;
};

typedef void (*ShaderEvalFunc) (ShaderEvalContext& c);

inline void evalCoordsPassthroughX		(ShaderEvalContext& c) { c.color.x() = c.coords.x(); }
inline void evalCoordsPassthroughXY		(ShaderEvalContext& c) { c.color.xy() = c.coords.swizzle(0,1); }
inline void evalCoordsPassthroughXYZ	(ShaderEvalContext& c) { c.color.xyz() = c.coords.swizzle(0,1,2); }
inline void evalCoordsPassthrough		(ShaderEvalContext& c) { c.color = c.coords; }
inline void evalCoordsSwizzleWZYX		(ShaderEvalContext& c) { c.color = c.coords.swizzle(3,2,1,0); }

// ShaderEvaluator
// Either inherit a class with overridden evaluate() or just pass in an evalFunc.

class ShaderEvaluator
{
public:
							ShaderEvaluator			(void);
							ShaderEvaluator			(const ShaderEvalFunc evalFunc);
	virtual					~ShaderEvaluator		(void);

	virtual void			evaluate				(ShaderEvalContext& ctx) const;

private:
							ShaderEvaluator			(const ShaderEvaluator&);   // not allowed!
	ShaderEvaluator&		operator=				(const ShaderEvaluator&);   // not allowed!

	const ShaderEvalFunc	m_evalFunc;
};

// UniformSetup

typedef void (*UniformSetupFunc) (ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords);

class UniformSetup
{
public:
							UniformSetup			(void);
							UniformSetup			(const UniformSetupFunc setup);
	virtual					~UniformSetup			(void);
	virtual void			setup					(ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords) const;

private:
							UniformSetup			(const UniformSetup&);	// not allowed!
	UniformSetup&			operator=				(const UniformSetup&);	// not allowed!

	const UniformSetupFunc	m_setupFunc;
};

typedef void (*AttributeSetupFunc) (ShaderRenderCaseInstance& instance, deUint32 numVertices);

class ShaderRenderCase : public vkt::TestCase
{
public:
													ShaderRenderCase	(tcu::TestContext&			testCtx,
																		 const std::string&			name,
																		 const std::string&			description,
																		 const bool					isVertexCase,
																		 const ShaderEvalFunc		evalFunc,
																		 const UniformSetup*		uniformSetup,
																		 const AttributeSetupFunc	attribFunc);

													ShaderRenderCase	(tcu::TestContext&			testCtx,
																		 const std::string&			name,
																		 const std::string&			description,
																		 const bool					isVertexCase,
																		 const ShaderEvaluator*		evaluator,
																		 const UniformSetup*		uniformSetup,
																		 const AttributeSetupFunc	attribFunc);

	virtual											~ShaderRenderCase	(void);
	virtual	void									initPrograms		(vk::SourceCollections& programCollection) const;
	virtual	TestInstance*							createInstance		(Context& context) const;

protected:
	std::string										m_vertShaderSource;
	std::string										m_fragShaderSource;

	const bool										m_isVertexCase;
	const de::UniquePtr<const ShaderEvaluator>		m_evaluator;
	const de::UniquePtr<const UniformSetup>			m_uniformSetup;
	const AttributeSetupFunc						m_attribFunc;
};

enum BaseUniformType
{
// Bool
	UB_FALSE,
	UB_TRUE,

// BVec4
	UB4_FALSE,
	UB4_TRUE,

// Integers
	UI_ZERO,
	UI_ONE,
	UI_TWO,
	UI_THREE,
	UI_FOUR,
	UI_FIVE,
	UI_SIX,
	UI_SEVEN,
	UI_EIGHT,
	UI_ONEHUNDREDONE,

// IVec2
	UI2_MINUS_ONE,
	UI2_ZERO,
	UI2_ONE,
	UI2_TWO,
	UI2_THREE,
	UI2_FOUR,
	UI2_FIVE,

// IVec3
	UI3_MINUS_ONE,
	UI3_ZERO,
	UI3_ONE,
	UI3_TWO,
	UI3_THREE,
	UI3_FOUR,
	UI3_FIVE,

// IVec4
	UI4_MINUS_ONE,
	UI4_ZERO,
	UI4_ONE,
	UI4_TWO,
	UI4_THREE,
	UI4_FOUR,
	UI4_FIVE,

// Float
	UF_ZERO,
	UF_ONE,
	UF_TWO,
	UF_THREE,
	UF_FOUR,
	UF_FIVE,
	UF_SIX,
	UF_SEVEN,
	UF_EIGHT,

	UF_HALF,
	UF_THIRD,
	UF_FOURTH,
	UF_FIFTH,
	UF_SIXTH,
	UF_SEVENTH,
	UF_EIGHTH,

// Vec2
	UV2_MINUS_ONE,
	UV2_ZERO,
	UV2_ONE,
	UV2_TWO,
	UV2_THREE,

	UV2_HALF,

// Vec3
	UV3_MINUS_ONE,
	UV3_ZERO,
	UV3_ONE,
	UV3_TWO,
	UV3_THREE,

	UV3_HALF,

// Vec4
	UV4_MINUS_ONE,
	UV4_ZERO,
	UV4_ONE,
	UV4_TWO,
	UV4_THREE,

	UV4_HALF,

	UV4_BLACK,
	UV4_GRAY,
	UV4_WHITE,

// Last
	U_LAST
};

enum BaseAttributeType
{
// User attributes
	A_IN0,
	A_IN1,
	A_IN2,
	A_IN3,

// Matrices
	MAT2,
	MAT2x3,
	MAT2x4,
	MAT3x2,
	MAT3,
	MAT3x4,
	MAT4x2,
	MAT4x3,
	MAT4
};

// ShaderRenderCaseInstance.

class ShaderRenderCaseInstance : public vkt::TestInstance
{
public:
	enum ImageBackingMode
	{
		IMAGE_BACKING_MODE_REGULAR = 0,
		IMAGE_BACKING_MODE_SPARSE,
	};

	// Default wertex and fragment grid sizes are used by a large collection of tests
	// to generate input sets. Some tests might change their behavior if the
	// default grid size values are altered, so care should be taken to confirm that
	// any changes to default values do not produce regressions.
	// If a particular tests needs to use a different grid size value, rather than
	// modifying the default grid size values for all tests, it is recommended that
	// the test specifies the required grid size using the gridSize parameter in the
	// ShaderRenderCaseInstance constuctor instead.
	enum
	{
		GRID_SIZE_DEFAULTS			= 0,
		GRID_SIZE_DEFAULT_VERTEX	= 90,
		GRID_SIZE_DEFAULT_FRAGMENT	= 4,
	};

														ShaderRenderCaseInstance	(Context&					context);
														ShaderRenderCaseInstance	(Context&					context,
																					const bool					isVertexCase,
																					const ShaderEvaluator&		evaluator,
																					const UniformSetup&			uniformSetup,
																					const AttributeSetupFunc	attribFunc,
																					const ImageBackingMode		imageBackingMode = IMAGE_BACKING_MODE_REGULAR,
																					const deUint32				gridSize = static_cast<deUint32>(GRID_SIZE_DEFAULTS));

	virtual												~ShaderRenderCaseInstance	(void);
	virtual tcu::TestStatus								iterate						(void);

	void												addAttribute				(deUint32			bindingLocation,
																					vk::VkFormat		format,
																					deUint32			sizePerElement,
																					deUint32			count,
																					const void*			data);
	void												useAttribute				(deUint32			bindingLocation,
																					BaseAttributeType	type);

	template<typename T>
	void												addUniform					(deUint32				bindingLocation,
																					vk::VkDescriptorType	descriptorType,
																					const T&				data);
	void												addUniform					(deUint32				bindingLocation,
																					vk::VkDescriptorType	descriptorType,
																					size_t					dataSize,
																					const void*				data);
	void												useUniform					(deUint32				bindingLocation,
																					BaseUniformType			type);
	void												useSampler					(deUint32				bindingLocation,
																					deUint32				textureId);

	static const tcu::Vec4								getDefaultConstCoords		(void) { return tcu::Vec4(0.125f, 0.25f, 0.5f, 1.0f); }
	void												setPushConstantRanges		(const deUint32 rangeCount, const vk::VkPushConstantRange* const pcRanges);
	virtual void										updatePushConstants			(vk::VkCommandBuffer commandBuffer, vk::VkPipelineLayout pipelineLayout);

protected:
														ShaderRenderCaseInstance	(Context&					context,
																					 const bool					isVertexCase,
																					 const ShaderEvaluator*		evaluator,
																					 const UniformSetup*		uniformSetup,
																					 const AttributeSetupFunc	attribFunc,
																					 const ImageBackingMode		imageBackingMode = IMAGE_BACKING_MODE_REGULAR,
																					 const deUint32				gridSize = static_cast<deUint32>(GRID_SIZE_DEFAULTS));

	virtual void										setup						(void);
	virtual void										setupUniforms				(const tcu::Vec4& constCoords);
	virtual void										setupDefaultInputs			(void);

	void												render						(deUint32					numVertices,
																					 deUint32					numTriangles,
																					 const deUint16*			indices,
																					 const tcu::Vec4&			constCoords		= getDefaultConstCoords());

	void												render						(deUint32					numVertices,
																					 deUint32					numIndices,
																					 const deUint16*			indices,
																					 vk::VkPrimitiveTopology	topology,
																					 const tcu::Vec4&			constCoords		= getDefaultConstCoords());

	const tcu::TextureLevel&							getResultImage				(void) const { return m_resultImage; }

	const tcu::UVec2									getViewportSize				(void) const;

	void												setSampleCount				(vk::VkSampleCountFlagBits sampleCount);

	bool												isMultiSampling				(void) const;

	ImageBackingMode									m_imageBackingMode;

	deUint32											m_quadGridSize;
private:

	struct SparseContext
	{
											SparseContext	(vkt::Context& context);

		vkt::Context&						m_context;
		const deUint32						m_queueFamilyIndex;
		vk::Unique<vk::VkDevice>			m_device;
		vk::DeviceDriver					m_deviceInterface;
		const vk::VkQueue					m_queue;
		const de::UniquePtr<vk::Allocator>	m_allocator;
	private:
		vk::Move<vk::VkDevice>				createDevice	(void) const;
		vk::Allocator*						createAllocator	(void) const;

	};

	de::UniquePtr<SparseContext>						m_sparseContext;
protected:
	vk::Allocator&										m_memAlloc;
	const tcu::Vec4										m_clearColor;
	const bool											m_isVertexCase;

	std::vector<tcu::Mat4>								m_userAttribTransforms;
	std::vector<TextureBindingSp>						m_textures;

	std::string											m_vertexShaderName;
	std::string											m_fragmentShaderName;
	tcu::UVec2											m_renderSize;
	vk::VkFormat										m_colorFormat;

private:
	typedef std::vector<tcu::ConstPixelBufferAccess>	TextureLayerData;
	typedef std::vector<TextureLayerData>				TextureData;

	void												uploadImage					(const tcu::TextureFormat&		texFormat,
																					 const TextureData&				textureData,
																					 const tcu::Sampler&			refSampler,
																					 deUint32						mipLevels,
																					 deUint32						arrayLayers,
																					 vk::VkImage					destImage);

	void												clearImage					(const tcu::Sampler&			refSampler,
																					 deUint32						mipLevels,
																					 deUint32						arrayLayers,
																					 vk::VkImage					destImage);

	void												checkSparseSupport			(const vk::VkImageCreateInfo&	imageInfo) const;

	void												uploadSparseImage			(const tcu::TextureFormat&		texFormat,
																					 const TextureData&				textureData,
																					 const tcu::Sampler&			refSampler,
																					 const deUint32					mipLevels,
																					 const deUint32					arrayLayers,
																					 const vk::VkImage				sparseImage,
																					 const vk::VkImageCreateInfo&	imageCreateInfo,
																					 const tcu::UVec3				texSize);

	void												createSamplerUniform		(deUint32						bindingLocation,
																					 TextureBinding::Type			textureType,
																					 TextureBinding::Init			textureInit,
																					 const tcu::TextureFormat&		texFormat,
																					 const tcu::UVec3				texSize,
																					 const TextureData&				textureData,
																					 const tcu::Sampler&			refSampler,
																					 deUint32						mipLevels,
																					 deUint32						arrayLayers,
																					 TextureBinding::Parameters		textureParams);

	void												setupUniformData			(deUint32 bindingLocation, size_t size, const void* dataPtr);

	void												computeVertexReference		(tcu::Surface& result, const QuadGrid& quadGrid);
	void												computeFragmentReference	(tcu::Surface& result, const QuadGrid& quadGrid);
	bool												compareImages				(const tcu::Surface&	resImage,
																					 const tcu::Surface&	refImage,
																					 float					errorThreshold);

private:
	const ShaderEvaluator*								m_evaluator;
	const UniformSetup*									m_uniformSetup;
	const AttributeSetupFunc							m_attribFunc;
	de::MovePtr<QuadGrid>								m_quadGrid;
	tcu::TextureLevel									m_resultImage;

	struct EnabledBaseAttribute
	{
		deUint32			location;
		BaseAttributeType	type;
	};
	std::vector<EnabledBaseAttribute>					m_enabledBaseAttributes;

	de::MovePtr<vk::DescriptorSetLayoutBuilder>			m_descriptorSetLayoutBuilder;
	de::MovePtr<vk::DescriptorPoolBuilder>				m_descriptorPoolBuilder;
	de::MovePtr<vk::DescriptorSetUpdateBuilder>			m_descriptorSetUpdateBuilder;

	typedef de::SharedPtr<vk::Unique<vk::VkBuffer> >		VkBufferSp;
	typedef de::SharedPtr<vk::Unique<vk::VkImage> >			VkImageSp;
	typedef de::SharedPtr<vk::Unique<vk::VkImageView> >		VkImageViewSp;
	typedef de::SharedPtr<vk::Unique<vk::VkSampler> >		VkSamplerSp;
	typedef de::SharedPtr<vk::Allocation>					AllocationSp;

	class UniformInfo
	{
	public:
									UniformInfo		(void) {}
		virtual						~UniformInfo	(void) {}

		vk::VkDescriptorType		type;
		deUint32					location;
	};

	class BufferUniform : public UniformInfo
	{
	public:
									BufferUniform	(void) {}
		virtual						~BufferUniform	(void) {}

		VkBufferSp					buffer;
		AllocationSp				alloc;
		vk::VkDescriptorBufferInfo	descriptor;
	};

	class SamplerUniform : public UniformInfo
	{
	public:
									SamplerUniform	(void) {}
		virtual						~SamplerUniform	(void) {}

		VkImageSp					image;
		VkImageViewSp				imageView;
		VkSamplerSp					sampler;
		AllocationSp				alloc;
		vk::VkDescriptorImageInfo	descriptor;
	};

	typedef de::SharedPtr<de::UniquePtr<UniformInfo> >	UniformInfoSp;
	std::vector<UniformInfoSp>							m_uniformInfos;

	std::vector< de::SharedPtr<vk::Allocation> >		m_allocations;

	std::vector<vk::VkVertexInputBindingDescription>	m_vertexBindingDescription;
	std::vector<vk::VkVertexInputAttributeDescription>	m_vertexAttributeDescription;

	std::vector<VkBufferSp>								m_vertexBuffers;
	std::vector<AllocationSp>							m_vertexBufferAllocs;

	vk::VkSampleCountFlagBits							m_sampleCount;
	std::vector<vk::VkPushConstantRange>				m_pushConstantRanges;

	// Wrapper functions around m_context calls to support sparse cases.
	vk::VkDevice										getDevice						(void) const;
	deUint32											getUniversalQueueFamilyIndex	(void) const;
	const vk::DeviceInterface&							getDeviceInterface				(void) const;
	vk::VkQueue											getUniversalQueue				(void) const;
	vk::VkPhysicalDevice								getPhysicalDevice				(void) const;
	const vk::InstanceInterface&						getInstanceInterface			(void) const;
	SparseContext*										createSparseContext				(void) const;
	vk::Allocator&										getAllocator					(void) const;
};

template<typename T>
void ShaderRenderCaseInstance::addUniform (deUint32 bindingLocation, vk::VkDescriptorType descriptorType, const T& data)
{
	addUniform(bindingLocation, descriptorType, sizeof(T), &data);
}

} // sr
} // vkt

#endif // _VKTSHADERRENDER_HPP
