/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief ASTC tests.
 *//*--------------------------------------------------------------------*/

#include "ditAstcTests.hpp"

#include "tcuCompressedTexture.hpp"
#include "tcuAstcUtil.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"

namespace dit
{

using std::string;
using std::vector;
using namespace tcu;

namespace
{

class AstcCase : public tcu::TestCase
{
public:
								AstcCase		(tcu::TestContext& testCtx, CompressedTexFormat format);

	IterateResult				iterate			(void);

private:
	const CompressedTexFormat	m_format;
};

static const string getASTCFormatShortName (CompressedTexFormat format)
{
	DE_ASSERT(isAstcFormat(format));
	const IVec3 blockSize = getBlockPixelSize(format);
	DE_ASSERT(blockSize.z() == 1);

	return de::toString(blockSize.x()) + "x" + de::toString(blockSize.y()) + (tcu::isAstcSRGBFormat(format) ? "_srgb" : "");
}

AstcCase::AstcCase (tcu::TestContext& testCtx, CompressedTexFormat format)
	: tcu::TestCase	(testCtx, getASTCFormatShortName(format).c_str(), "")
	, m_format		(format)
{
}

void testDecompress (CompressedTexFormat format, TexDecompressionParams::AstcMode mode, size_t numBlocks, const deUint8* data)
{
	const IVec3						blockPixelSize			= getBlockPixelSize(format);
	const TexDecompressionParams	decompressionParams		(mode);
	const TextureFormat				uncompressedFormat		= getUncompressedFormat(format);
	TextureLevel					texture					(uncompressedFormat, blockPixelSize.x()*(int)numBlocks, blockPixelSize.y());

	decompress(texture.getAccess(), format, data, decompressionParams);
}

void testDecompress (CompressedTexFormat format, size_t numBlocks, const deUint8* data)
{
	testDecompress(format, TexDecompressionParams::ASTCMODE_LDR, numBlocks, data);

	if (!isAstcSRGBFormat(format))
		testDecompress(format, TexDecompressionParams::ASTCMODE_HDR, numBlocks, data);
}

void verifyBlocksValid (CompressedTexFormat format, TexDecompressionParams::AstcMode mode, size_t numBlocks, const deUint8* data)
{
	for (size_t blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		if (!astc::isValidBlock(data + blockNdx*astc::BLOCK_SIZE_BYTES, format, mode))
			TCU_FAIL("Invalid ASTC block was generated");
	}
}

inline size_t getNumBlocksFromBytes (size_t numBytes)
{
	TCU_CHECK(numBytes % astc::BLOCK_SIZE_BYTES == 0);
	return (numBytes / astc::BLOCK_SIZE_BYTES);
}

AstcCase::IterateResult AstcCase::iterate (void)
{
	vector<deUint8> generatedData;

	// Verify that can generate & decode data with all BlockTestType's
	for (int blockTestTypeNdx = 0; blockTestTypeNdx < astc::BLOCK_TEST_TYPE_LAST; blockTestTypeNdx++)
	{
		const astc::BlockTestType	blockTestType	= (const astc::BlockTestType)blockTestTypeNdx;

		if (astc::isBlockTestTypeHDROnly(blockTestType) && isAstcSRGBFormat(m_format))
			continue;

		generatedData.clear();
		astc::generateBlockCaseTestData(generatedData, m_format, blockTestType);

		testDecompress(m_format, getNumBlocksFromBytes(generatedData.size()), &generatedData[0]);

		// All but random case should generate only valid blocks
		if (blockTestType != astc::BLOCK_TEST_TYPE_RANDOM)
		{
			// \note CEMS generates HDR blocks as well
			if (!astc::isBlockTestTypeHDROnly(blockTestType) &&
				(blockTestType != astc::BLOCK_TEST_TYPE_CEMS))
				verifyBlocksValid(m_format, TexDecompressionParams::ASTCMODE_LDR, getNumBlocksFromBytes(generatedData.size()), &generatedData[0]);

			if (!isAstcSRGBFormat(m_format))
				verifyBlocksValid(m_format, TexDecompressionParams::ASTCMODE_HDR, getNumBlocksFromBytes(generatedData.size()), &generatedData[0]);
		}
	}

	// Verify generating void extent blocks (format-independent)
	{
		const size_t		numBlocks		= 1024;

		generatedData.resize(numBlocks*astc::BLOCK_SIZE_BYTES);
		astc::generateDummyVoidExtentBlocks(&generatedData[0], numBlocks);

		testDecompress(m_format, numBlocks, &generatedData[0]);

		verifyBlocksValid(m_format, TexDecompressionParams::ASTCMODE_LDR, numBlocks, &generatedData[0]);

		if (!isAstcSRGBFormat(m_format))
			verifyBlocksValid(m_format, TexDecompressionParams::ASTCMODE_HDR, numBlocks, &generatedData[0]);
	}

	// Verify generating dummy normal blocks
	{
		const size_t		numBlocks			= 1024;
		const IVec3			blockPixelSize		= getBlockPixelSize(m_format);

		generatedData.resize(numBlocks*astc::BLOCK_SIZE_BYTES);
		astc::generateDummyNormalBlocks(&generatedData[0], numBlocks, blockPixelSize.x(), blockPixelSize.y());

		testDecompress(m_format, numBlocks, &generatedData[0]);

		verifyBlocksValid(m_format, TexDecompressionParams::ASTCMODE_LDR, numBlocks, &generatedData[0]);

		if (!isAstcSRGBFormat(m_format))
			verifyBlocksValid(m_format, TexDecompressionParams::ASTCMODE_HDR, numBlocks, &generatedData[0]);
	}

	// Verify generating random valid blocks
	for (int astcModeNdx = 0; astcModeNdx < TexDecompressionParams::ASTCMODE_LAST; astcModeNdx++)
	{
		const TexDecompressionParams::AstcMode	mode		= (TexDecompressionParams::AstcMode)astcModeNdx;
		const size_t							numBlocks	= 1024;

		if (mode == tcu::TexDecompressionParams::ASTCMODE_HDR && isAstcFormat(m_format))
			continue; // sRGB is not supported in HDR mode

		generatedData.resize(numBlocks*astc::BLOCK_SIZE_BYTES);
		astc::generateRandomValidBlocks(&generatedData[0], numBlocks, m_format, mode, deInt32Hash(m_format) ^ deInt32Hash(mode));

		testDecompress(m_format, numBlocks, &generatedData[0]);

		verifyBlocksValid(m_format, mode, numBlocks, &generatedData[0]);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "All checks passed");
	return STOP;
}

} // anonymous

tcu::TestCaseGroup* createAstcTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	astcTests	(new tcu::TestCaseGroup(testCtx, "astc", "Tests for ASTC Utilities"));

	for (int formatNdx = 0; formatNdx < COMPRESSEDTEXFORMAT_LAST; formatNdx++)
	{
		const CompressedTexFormat	format	= (CompressedTexFormat)formatNdx;

		if (isAstcFormat(format))
			astcTests->addChild(new AstcCase(testCtx, format));
	}

	return astcTests.release();
}

} // dit
