#ifndef _VKTSPARSERESOURCESSHADERINTRINSICSSAMPLED_HPP
#define _VKTSPARSERESOURCESSHADERINTRINSICSSAMPLED_HPP
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
 * \file  vktSparseResourcesShaderIntrinsicsSampled.hpp
 * \brief Sparse Resources Shader Intrinsics for sampled images
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesShaderIntrinsicsBase.hpp"

namespace vkt
{
namespace sparse
{

class SparseShaderIntrinsicsCaseSampledBase : public SparseShaderIntrinsicsCaseBase
{
public:
	SparseShaderIntrinsicsCaseSampledBase		(tcu::TestContext&			testCtx,
												 const std::string&			name,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
												 : SparseShaderIntrinsicsCaseBase (testCtx, name, function, imageType, imageSize, format) {}

	void				initPrograms			(vk::SourceCollections&		programCollection) const;

	virtual std::string	sparseImageOpString		(const std::string&			resultVariable,
												 const std::string&			resultType,
												 const std::string&			image,
												 const std::string&			coord,
												 const std::string&			miplevel) const = 0;
};

class SparseShaderIntrinsicsCaseSampledExplicit : public SparseShaderIntrinsicsCaseSampledBase
{
public:
	SparseShaderIntrinsicsCaseSampledExplicit	(tcu::TestContext&			testCtx,
												 const std::string&			name,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
												 : SparseShaderIntrinsicsCaseSampledBase (testCtx, name, function, imageType, imageSize, format) {}

	TestInstance*	createInstance				(Context&					context) const;
};

class SparseCaseOpImageSparseSampleExplicitLod : public SparseShaderIntrinsicsCaseSampledExplicit
{
public:
	SparseCaseOpImageSparseSampleExplicitLod	(tcu::TestContext&			testCtx,
												 const std::string&			name,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
												 : SparseShaderIntrinsicsCaseSampledExplicit (testCtx, name, function, imageType, imageSize, format) {}

	std::string	sparseImageOpString				(const std::string&			resultVariable,
												 const std::string&			resultType,
												 const std::string&			image,
												 const std::string&			coord,
												 const std::string&			miplevel) const;
};

class SparseShaderIntrinsicsCaseSampledImplicit : public SparseShaderIntrinsicsCaseSampledBase
{
public:
	SparseShaderIntrinsicsCaseSampledImplicit	(tcu::TestContext&			testCtx,
												 const std::string&			name,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
												 : SparseShaderIntrinsicsCaseSampledBase (testCtx, name, function, imageType, imageSize, format) {}

	TestInstance*	createInstance				(Context&					context) const;
};

class SparseCaseOpImageSparseSampleImplicitLod : public SparseShaderIntrinsicsCaseSampledImplicit
{
public:
	SparseCaseOpImageSparseSampleImplicitLod	(tcu::TestContext&			testCtx,
												 const std::string&			name,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
												 : SparseShaderIntrinsicsCaseSampledImplicit (testCtx, name, function, imageType, imageSize, format) {}

	std::string	sparseImageOpString				(const std::string&			resultVariable,
												 const std::string&			resultType,
												 const std::string&			image,
												 const std::string&			coord,
												 const std::string&			miplevel) const;
};

class SparseCaseOpImageSparseGather : public SparseShaderIntrinsicsCaseSampledImplicit
{
public:
	SparseCaseOpImageSparseGather				(tcu::TestContext&			testCtx,
												 const std::string&			name,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
												 : SparseShaderIntrinsicsCaseSampledImplicit (testCtx, name, function, imageType, imageSize, format) {}

	std::string	sparseImageOpString				(const std::string&			resultVariable,
												 const std::string&			resultType,
												 const std::string&			image,
												 const std::string&			coord,
												 const std::string&			miplevel) const;
};

} // sparse
} // vkt

#endif // _VKTSPARSERESOURCESSHADERINTRINSICSSAMPLED_HPP
