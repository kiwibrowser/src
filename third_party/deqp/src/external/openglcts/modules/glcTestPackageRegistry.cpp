/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 */ /*!
 * \file
 * \brief OpenGL Conformance Test Package Registry.
 */ /*-------------------------------------------------------------------*/

#include "glcTestPackageRegistry.hpp"
#include "glcConfigPackage.hpp"

#include "teglTestPackage.hpp"

#include "es2cTestPackage.hpp"
#include "tes2TestPackage.hpp"

#if defined(DEQP_GTF_AVAILABLE)
#include "gtfES2TestPackage.hpp"
#endif

#include "es3cTestPackage.hpp"
#include "tes3TestPackage.hpp"

#if defined(DEQP_GTF_AVAILABLE)
#include "gtfES3TestPackage.hpp"
#endif

#include "es31cTestPackage.hpp"
#include "esextcTestPackage.hpp"
#include "tes31TestPackage.hpp"

#if defined(DEQP_GTF_AVAILABLE)
#include "gtfES31TestPackage.hpp"
#endif

#include "es32cTestPackage.hpp"

#include "gl3cTestPackages.hpp"
#include "gl4cTestPackages.hpp"

#include "glcNoDefaultContextPackage.hpp"

#if defined(DEQP_GTF_AVAILABLE)
#include "gtfGL30TestPackage.hpp"
#include "gtfGL31TestPackage.hpp"
#include "gtfGL32TestPackage.hpp"
#include "gtfGL33TestPackage.hpp"
#include "gtfGL40TestPackage.hpp"
#include "gtfGL41TestPackage.hpp"
#include "gtfGL42TestPackage.hpp"
#include "gtfGL43TestPackage.hpp"
#include "gtfGL44TestPackage.hpp"
#include "gtfGL45TestPackage.hpp"
#include "gtfGL46TestPackage.hpp"
#endif

namespace glcts
{

static tcu::TestPackage* createConfigPackage(tcu::TestContext& testCtx)
{
	return new glcts::ConfigPackage(testCtx, "CTS-Configs");
}

static tcu::TestPackage* createES2Package(tcu::TestContext& testCtx)
{
	return new es2cts::TestPackage(testCtx, "KHR-GLES2");
}

#if DE_OS != DE_OS_ANDROID
static tcu::TestPackage* createdEQPEGLPackage(tcu::TestContext& testCtx)
{
	return new deqp::egl::TestPackage(testCtx);
}
#endif

#if DE_OS != DE_OS_ANDROID
static tcu::TestPackage* createdEQPES2Package(tcu::TestContext& testCtx)
{
	return new deqp::gles2::TestPackage(testCtx);
}
#endif

#if defined(DEQP_GTF_AVAILABLE)
static tcu::TestPackage* createES2GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::es2::TestPackage(testCtx, "GTF-GLES2");
}
#endif

static tcu::TestPackage* createES30Package(tcu::TestContext& testCtx)
{
	return new es3cts::ES30TestPackage(testCtx, "KHR-GLES3");
}

#if DE_OS != DE_OS_ANDROID
static tcu::TestPackage* createdEQPES30Package(tcu::TestContext& testCtx)
{
	return new deqp::gles3::TestPackage(testCtx);
}
#endif

#if defined(DEQP_GTF_AVAILABLE)
static tcu::TestPackage* createES30GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::es3::TestPackage(testCtx, "GTF-GLES3");
}
#endif

#if DE_OS != DE_OS_ANDROID
static tcu::TestPackage* createdEQPES31Package(tcu::TestContext& testCtx)
{
	return new deqp::gles31::TestPackage(testCtx);
}
#endif
static tcu::TestPackage* createES31Package(tcu::TestContext& testCtx)
{
	return new es31cts::ES31TestPackage(testCtx, "KHR-GLES31");
}
static tcu::TestPackage* createESEXTPackage(tcu::TestContext& testCtx)
{
	return new esextcts::ESEXTTestPackage(testCtx, "KHR-GLESEXT");
}

#if defined(DEQP_GTF_AVAILABLE)
static tcu::TestPackage* createES31GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::es31::TestPackage(testCtx, "GTF-GLES31");
}
#endif

static tcu::TestPackage* createES32Package(tcu::TestContext& testCtx)
{
	return new es32cts::ES32TestPackage(testCtx, "KHR-GLES32");
}

static tcu::TestPackage* createNoDefaultCustomContextPackage(tcu::TestContext& testCtx)
{
	return new glcts::NoDefaultContextPackage(testCtx, "KHR-NoContext");
}

static tcu::TestPackage* createGL30Package(tcu::TestContext& testCtx)
{
	return new gl3cts::GL30TestPackage(testCtx, "KHR-GL30");
}
static tcu::TestPackage* createGL31Package(tcu::TestContext& testCtx)
{
	return new gl3cts::GL31TestPackage(testCtx, "KHR-GL31");
}
static tcu::TestPackage* createGL32Package(tcu::TestContext& testCtx)
{
	return new gl3cts::GL32TestPackage(testCtx, "KHR-GL32");
}
static tcu::TestPackage* createGL33Package(tcu::TestContext& testCtx)
{
	return new gl3cts::GL33TestPackage(testCtx, "KHR-GL33");
}

static tcu::TestPackage* createGL40Package(tcu::TestContext& testCtx)
{
	return new gl4cts::GL40TestPackage(testCtx, "KHR-GL40");
}
static tcu::TestPackage* createGL41Package(tcu::TestContext& testCtx)
{
	return new gl4cts::GL41TestPackage(testCtx, "KHR-GL41");
}
static tcu::TestPackage* createGL42Package(tcu::TestContext& testCtx)
{
	return new gl4cts::GL42TestPackage(testCtx, "KHR-GL42");
}
static tcu::TestPackage* createGL43Package(tcu::TestContext& testCtx)
{
	return new gl4cts::GL43TestPackage(testCtx, "KHR-GL43");
}
static tcu::TestPackage* createGL44Package(tcu::TestContext& testCtx)
{
	return new gl4cts::GL44TestPackage(testCtx, "KHR-GL44");
}
static tcu::TestPackage* createGL45Package(tcu::TestContext& testCtx)
{
	return new gl4cts::GL45TestPackage(testCtx, "KHR-GL45");
}
static tcu::TestPackage* createGL46Package(tcu::TestContext& testCtx)
{
	return new gl4cts::GL46TestPackage(testCtx, "KHR-GL46");
}

#if defined(DEQP_GTF_AVAILABLE)
static tcu::TestPackage* createGL30GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl30::TestPackage(testCtx, "GTF-GL30");
}
static tcu::TestPackage* createGL31GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl31::TestPackage(testCtx, "GTF-GL31");
}
static tcu::TestPackage* createGL32GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl32::TestPackage(testCtx, "GTF-GL32");
}
static tcu::TestPackage* createGL33GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl32::TestPackage(testCtx, "GTF-GL33");
}

static tcu::TestPackage* createGL40GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl40::TestPackage(testCtx, "GTF-GL40");
}
static tcu::TestPackage* createGL41GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl41::TestPackage(testCtx, "GTF-GL41");
}
static tcu::TestPackage* createGL42GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl42::TestPackage(testCtx, "GTF-GL42");
}
static tcu::TestPackage* createGL43GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl43::TestPackage(testCtx, "GTF-GL43");
}
static tcu::TestPackage* createGL44GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl44::TestPackage(testCtx, "GTF-GL44");
}
static tcu::TestPackage* createGL45GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl45::TestPackage(testCtx, "GTF-GL45");
}
static tcu::TestPackage* createGL46GTFPackage(tcu::TestContext& testCtx)
{
	return new gtf::gl46::TestPackage(testCtx, "GTF-GL46");
}
#endif

void registerPackages(void)
{
	tcu::TestPackageRegistry* registry = tcu::TestPackageRegistry::getSingleton();

	registry->registerPackage("CTS-Configs", createConfigPackage);

#if DE_OS != DE_OS_ANDROID
	registry->registerPackage("dEQP-EGL", createdEQPEGLPackage);
#endif
	registry->registerPackage("KHR-GLES2", createES2Package);
#if DE_OS != DE_OS_ANDROID
	registry->registerPackage("dEQP-GLES2", createdEQPES2Package);
#endif

#if defined(DEQP_GTF_AVAILABLE)
	registry->registerPackage("GTF-GLES2", createES2GTFPackage);
#endif

	registry->registerPackage("KHR-GLES3", createES30Package);
#if DE_OS != DE_OS_ANDROID
	registry->registerPackage("dEQP-GLES3", createdEQPES30Package);
#endif

#if defined(DEQP_GTF_AVAILABLE)
	registry->registerPackage("GTF-GLES3", createES30GTFPackage);
#endif

#if DE_OS != DE_OS_ANDROID
	registry->registerPackage("dEQP-GLES31", createdEQPES31Package);
#endif
	registry->registerPackage("KHR-GLES31", createES31Package);
	registry->registerPackage("KHR-GLESEXT", createESEXTPackage);

#if defined(DEQP_GTF_AVAILABLE)
	registry->registerPackage("GTF-GLES31", createES31GTFPackage);
#endif

	registry->registerPackage("KHR-GLES32", createES32Package);

	registry->registerPackage("KHR-NoContext", createNoDefaultCustomContextPackage);

	registry->registerPackage("KHR-GL30", createGL30Package);
	registry->registerPackage("KHR-GL31", createGL31Package);
	registry->registerPackage("KHR-GL32", createGL32Package);
	registry->registerPackage("KHR-GL33", createGL33Package);

	registry->registerPackage("KHR-GL40", createGL40Package);
	registry->registerPackage("KHR-GL41", createGL41Package);
	registry->registerPackage("KHR-GL42", createGL42Package);
	registry->registerPackage("KHR-GL43", createGL43Package);
	registry->registerPackage("KHR-GL44", createGL44Package);
	registry->registerPackage("KHR-GL45", createGL45Package);
	registry->registerPackage("KHR-GL46", createGL46Package);

#if defined(DEQP_GTF_AVAILABLE)
	registry->registerPackage("GTF-GL30", createGL30GTFPackage);
	registry->registerPackage("GTF-GL31", createGL31GTFPackage);
	registry->registerPackage("GTF-GL32", createGL32GTFPackage);
	registry->registerPackage("GTF-GL33", createGL33GTFPackage);

	registry->registerPackage("GTF-GL40", createGL40GTFPackage);
	registry->registerPackage("GTF-GL41", createGL41GTFPackage);
	registry->registerPackage("GTF-GL42", createGL42GTFPackage);
	registry->registerPackage("GTF-GL43", createGL43GTFPackage);
	registry->registerPackage("GTF-GL44", createGL44GTFPackage);
	registry->registerPackage("GTF-GL45", createGL45GTFPackage);
	registry->registerPackage("GTF-GL46", createGL46GTFPackage);
#endif
}
}
