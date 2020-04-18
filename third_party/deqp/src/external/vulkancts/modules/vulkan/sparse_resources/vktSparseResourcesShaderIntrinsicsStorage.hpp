#ifndef _VKTSPARSERESOURCESSHADERINTRINSICSSTORAGE_HPP
#define _VKTSPARSERESOURCESSHADERINTRINSICSSTORAGE_HPP
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
 * \file  vktSparseResourcesShaderIntrinsicsStorage.hpp
 * \brief Sparse Resources Shader Intrinsics for storage images
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesShaderIntrinsicsBase.hpp"

namespace vkt
{
namespace sparse
{

class SparseShaderIntrinsicsCaseStorage : public SparseShaderIntrinsicsCaseBase
{
public:
	SparseShaderIntrinsicsCaseStorage		(tcu::TestContext&			testCtx,
											 const std::string&			name,
											 const SpirVFunction		function,
											 const ImageType			imageType,
											 const tcu::UVec3&			imageSize,
											 const tcu::TextureFormat&	format)
											 : SparseShaderIntrinsicsCaseBase (testCtx, name, function, imageType, imageSize, format) {}

	void				initPrograms		(vk::SourceCollections&		programCollection) const;

	virtual std::string	getSparseImageTypeName				(void) const = 0;
	virtual std::string	getUniformConstSparseImageTypeName	(void) const = 0;

	virtual std::string	sparseImageOpString	(const std::string&			resultVariable,
											 const std::string&			resultType,
											 const std::string&			image,
											 const std::string&			coord,
											 const std::string&			mipLevel) const = 0;
};

class SparseCaseOpImageSparseFetch : public SparseShaderIntrinsicsCaseStorage
{
public:
	SparseCaseOpImageSparseFetch			(tcu::TestContext&			testCtx,
											 const std::string&			name,
											 const SpirVFunction		function,
											 const ImageType			imageType,
											 const tcu::UVec3&			imageSize,
											 const tcu::TextureFormat&	format)
											 : SparseShaderIntrinsicsCaseStorage (testCtx, name, function, imageType, imageSize, format) {}

	TestInstance* createInstance			(Context& context) const;

	std::string	getSparseImageTypeName				(void) const;
	std::string	getUniformConstSparseImageTypeName	(void) const;

	std::string	sparseImageOpString			(const std::string&			resultVariable,
											 const std::string&			resultType,
											 const std::string&			image,
											 const std::string&			coord,
											 const std::string&			mipLevel) const;
};

class SparseCaseOpImageSparseRead : public SparseShaderIntrinsicsCaseStorage
{
public:
	SparseCaseOpImageSparseRead				(tcu::TestContext&			testCtx,
											 const std::string&			name,
											 const SpirVFunction		function,
											 const ImageType			imageType,
											 const tcu::UVec3&			imageSize,
											 const tcu::TextureFormat&	format)
											 : SparseShaderIntrinsicsCaseStorage (testCtx, name, function, imageType, imageSize, format) {}

	TestInstance* createInstance			(Context& context) const;

	std::string	getSparseImageTypeName				(void) const;
	std::string	getUniformConstSparseImageTypeName	(void) const;

	std::string	sparseImageOpString			(const std::string& resultVariable,
											 const std::string& resultType,
											 const std::string& image,
											 const std::string& coord,
											 const std::string& mipLevel) const;
};

} // sparse
} // vkt

#endif // _VKTSPARSERESOURCESSHADERINTRINSICSSTORAGE_HPP
