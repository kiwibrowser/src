/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Image IO tests.
 *//*--------------------------------------------------------------------*/

#include "ditImageIOTests.hpp"
#include "tcuResource.hpp"
#include "tcuImageIO.hpp"
#include "tcuTexture.hpp"
#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "deUniquePtr.hpp"
#include "deString.h"

namespace dit
{

using tcu::TestLog;

// \todo [2013-05-28 pyry] Image output cases!

class ImageReadCase : public tcu::TestCase
{
public:
	ImageReadCase (tcu::TestContext& testCtx, const char* name, const char* filename, deUint32 expectedHash)
		: TestCase			(testCtx, name, filename)
		, m_filename		(filename)
		, m_expectedHash	(expectedHash)
	{
	}

	IterateResult iterate (void)
	{
		m_testCtx.getLog() << TestLog::Message << "Loading image from file '" << m_filename << "'" << TestLog::EndMessage;

		tcu::TextureLevel texture;
		tcu::ImageIO::loadImage(texture, m_testCtx.getArchive(), m_filename.c_str());

		m_testCtx.getLog() << TestLog::Message << "Loaded " << texture.getWidth() << "x" << texture.getHeight() << "x" << texture.getDepth() << " image with format " << texture.getFormat() << TestLog::EndMessage;

		// Check that layout is as expected
		TCU_CHECK(texture.getAccess().getRowPitch() == texture.getWidth()*texture.getFormat().getPixelSize());
		TCU_CHECK(texture.getAccess().getSlicePitch() == texture.getAccess().getRowPitch()*texture.getAccess().getHeight());

		const int		imageSize	= texture.getAccess().getSlicePitch()*texture.getDepth();
		const deUint32	hash		= deMemoryHash(texture.getAccess().getDataPtr(), imageSize);

		if (hash != m_expectedHash)
		{
			m_testCtx.getLog() << TestLog::Message << "ERROR: expected hash " << tcu::toHex(m_expectedHash) << ", got " << tcu::toHex(hash) << TestLog::EndMessage;
			m_testCtx.getLog() << TestLog::Image("Image", "Loaded image", texture.getAccess());
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Hash check failed");
		}
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

private:
	const std::string		m_filename;
	const deUint32			m_expectedHash;
};

class ImageReadTests : public tcu::TestCaseGroup
{
public:
	ImageReadTests (tcu::TestContext& testCtx)
		: TestCaseGroup(testCtx, "read", "Image read tests")
	{
	}

	void init (void)
	{
		addChild(new ImageReadCase(m_testCtx, "rgb24_256x256",	"internal/data/imageio/rgb24_256x256.png",	0x6efad777));
		addChild(new ImageReadCase(m_testCtx, "rgb24_209x181",	"internal/data/imageio/rgb24_209x181.png",	0xfd6ea668));
		addChild(new ImageReadCase(m_testCtx, "rgba32_256x256",	"internal/data/imageio/rgba32_256x256.png",	0xcf4883da));
		addChild(new ImageReadCase(m_testCtx, "rgba32_207x219",	"internal/data/imageio/rgba32_207x219.png",	0x404ba06b));
	}
};

ImageIOTests::ImageIOTests(tcu::TestContext& testCtx)
	: TestCaseGroup(testCtx, "image_io", "Image read and write tests")
{
}

ImageIOTests::~ImageIOTests (void)
{
}

void ImageIOTests::init (void)
{
	addChild(new ImageReadTests(m_testCtx));
}

} // dit
