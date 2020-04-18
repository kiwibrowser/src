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
 *//*
 * \file  vktSparseResourcesShaderIntrinsics.cpp
 * \brief Sparse Resources Shader Intrinsics
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesShaderIntrinsicsSampled.hpp"
#include "vktSparseResourcesShaderIntrinsicsStorage.hpp"

using namespace vk;

namespace vkt
{
namespace sparse
{

tcu::TestCaseGroup* createSparseResourcesShaderIntrinsicsTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "shader_intrinsics", "Sparse Resources Shader Intrinsics"));

	static const deUint32 sizeCountPerImageType = 4u;

	struct ImageParameters
	{
		ImageType	imageType;
		tcu::UVec3	imageSizes[sizeCountPerImageType];
	};

	static const ImageParameters imageParametersArray[] =
	{
		{ IMAGE_TYPE_2D,		{ tcu::UVec3(512u, 256u, 1u),	tcu::UVec3(128u, 128u, 1u), tcu::UVec3(503u, 137u, 1u), tcu::UVec3(11u, 37u, 1u) } },
		{ IMAGE_TYPE_2D_ARRAY,	{ tcu::UVec3(512u, 256u, 6u),	tcu::UVec3(128u, 128u, 8u),	tcu::UVec3(503u, 137u, 3u),	tcu::UVec3(11u, 37u, 3u) } },
		{ IMAGE_TYPE_CUBE,		{ tcu::UVec3(256u, 256u, 1u),	tcu::UVec3(128u, 128u, 1u),	tcu::UVec3(137u, 137u, 1u),	tcu::UVec3(11u, 11u, 1u) } },
		{ IMAGE_TYPE_CUBE_ARRAY,{ tcu::UVec3(256u, 256u, 6u),	tcu::UVec3(128u, 128u, 8u),	tcu::UVec3(137u, 137u, 3u),	tcu::UVec3(11u, 11u, 3u) } },
		{ IMAGE_TYPE_3D,		{ tcu::UVec3(256u, 256u, 16u),	tcu::UVec3(128u, 128u, 8u),	tcu::UVec3(503u, 137u, 3u),	tcu::UVec3(11u, 37u, 3u) } }
	};

	static const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8)
	};

	static const std::string functions[SPARSE_SPIRV_FUNCTION_TYPE_LAST] =
	{
		"_sparse_fetch",
		"_sparse_read",
		"_sparse_sample_explicit_lod",
		"_sparse_sample_implicit_lod",
		"_sparse_gather",
	};

	for (deUint32 functionNdx = 0; functionNdx < SPARSE_SPIRV_FUNCTION_TYPE_LAST; ++functionNdx)
	{
		const SpirVFunction function = static_cast<SpirVFunction>(functionNdx);

		for (deInt32 imageTypeNdx = 0; imageTypeNdx < DE_LENGTH_OF_ARRAY(imageParametersArray); ++imageTypeNdx)
		{
			const ImageType					imageType = imageParametersArray[imageTypeNdx].imageType;
			de::MovePtr<tcu::TestCaseGroup> imageTypeGroup(new tcu::TestCaseGroup(testCtx, (getImageTypeName(imageType) + functions[functionNdx]).c_str(), ""));

			for (deInt32 formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
			{
				const tcu::TextureFormat&		format = formats[formatNdx];
				de::MovePtr<tcu::TestCaseGroup> formatGroup(new tcu::TestCaseGroup(testCtx, getShaderImageFormatQualifier(format).c_str(), ""));

				for (deInt32 imageSizeNdx = 0; imageSizeNdx < DE_LENGTH_OF_ARRAY(imageParametersArray[imageTypeNdx].imageSizes); ++imageSizeNdx)
				{
					const tcu::UVec3 imageSize = imageParametersArray[imageTypeNdx].imageSizes[imageSizeNdx];

					std::ostringstream stream;
					stream << imageSize.x() << "_" << imageSize.y() << "_" << imageSize.z();

					switch (function)
					{
					case SPARSE_FETCH:
						if ((imageType == IMAGE_TYPE_CUBE) || (imageType == IMAGE_TYPE_CUBE_ARRAY)) continue;
					case SPARSE_SAMPLE_EXPLICIT_LOD:
					case SPARSE_SAMPLE_IMPLICIT_LOD:
					case SPARSE_GATHER:
						if ((imageType == IMAGE_TYPE_CUBE) || (imageType == IMAGE_TYPE_CUBE_ARRAY) || (imageType == IMAGE_TYPE_3D)) continue;
						break;
					default:
						break;
					}

					switch (function)
					{
					case SPARSE_FETCH:
						formatGroup->addChild(new SparseCaseOpImageSparseFetch(testCtx, stream.str(), function, imageType, imageSize, format));
						break;
					case SPARSE_READ:
						formatGroup->addChild(new SparseCaseOpImageSparseRead(testCtx, stream.str(), function, imageType, imageSize, format));
						break;
					case SPARSE_SAMPLE_EXPLICIT_LOD:
						formatGroup->addChild(new SparseCaseOpImageSparseSampleExplicitLod(testCtx, stream.str(), function, imageType, imageSize, format));
						break;
					case SPARSE_SAMPLE_IMPLICIT_LOD:
						formatGroup->addChild(new SparseCaseOpImageSparseSampleImplicitLod(testCtx, stream.str(), function, imageType, imageSize, format));
						break;
					case SPARSE_GATHER:
						formatGroup->addChild(new SparseCaseOpImageSparseGather(testCtx, stream.str(), function, imageType, imageSize, format));
						break;
					default:
						DE_ASSERT(0);
						break;
					}
				}
				imageTypeGroup->addChild(formatGroup.release());
			}
			testGroup->addChild(imageTypeGroup.release());
		}
	}

	return testGroup.release();
}

} // sparse
} // vkt
