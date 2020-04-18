/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief Texture format tests.
 *//*--------------------------------------------------------------------*/

#include "ditTextureFormatTests.hpp"
#include "tcuTestLog.hpp"

#include "rrRenderer.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuFormatUtil.hpp"

#include "deRandom.hpp"
#include "deArrayUtil.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include <sstream>

namespace dit
{

namespace
{

using std::string;
using std::vector;

using tcu::TestLog;
using tcu::TextureFormat;
using tcu::TextureChannelClass;
using tcu::TextureAccessType;
using tcu::PixelBufferAccess;
using tcu::ConstPixelBufferAccess;
using tcu::Vector;
using tcu::IVec3;

// Test data

static const deUint8 s_snormInt8In[] =
{
	0x1b, 0x23, 0xc5, 0x09,
	0xb4, 0xbf, 0xbf, 0x24,
	0x1a, 0x8a, 0xdb, 0x96,
	0xc0, 0xa1, 0xde, 0x78,
};
static const deUint32 s_snormInt8FloatRef[] =
{
	0x3e59b367, 0x3e8d1a34, 0xbeeddbb7, 0x3d912245,
	0xbf193265, 0xbf03060c, 0xbf03060c, 0x3e912245,
	0x3e51a347, 0xbf6ddbb7, 0xbe952a55, 0xbf55ab57,
	0xbf010204, 0xbf3f7efe, 0xbe891224, 0x3f71e3c8,
};
static const deUint32 s_snormInt8UintRef[] =
{
	0x0000001b, 0x00000023, 0xffffffc5, 0x00000009,
	0xffffffb4, 0xffffffbf, 0xffffffbf, 0x00000024,
	0x0000001a, 0xffffff8a, 0xffffffdb, 0xffffff96,
	0xffffffc0, 0xffffffa1, 0xffffffde, 0x00000078,
};
static const deUint32 s_snormInt8IntRef[] =
{
	0x0000001b, 0x00000023, 0xffffffc5, 0x00000009,
	0xffffffb4, 0xffffffbf, 0xffffffbf, 0x00000024,
	0x0000001a, 0xffffff8a, 0xffffffdb, 0xffffff96,
	0xffffffc0, 0xffffffa1, 0xffffffde, 0x00000078,
};

static const deUint8 s_snormInt16In[] =
{
	0xa0, 0xe9, 0xaa, 0x30,
	0x16, 0x61, 0x37, 0xa2,
	0x23, 0x4c, 0x46, 0xac,
	0x8b, 0xf9, 0x36, 0x3e,
	0x92, 0x7c, 0x96, 0x81,
	0xc5, 0xb2, 0x95, 0x6e,
	0x4f, 0x1e, 0xbc, 0x49,
	0x14, 0x6c, 0x3c, 0x61,
};
static const deUint32 s_snormInt16FloatRef[] =
{
	0xbe330166, 0x3ec2a985, 0x3f422d84, 0xbf3b9377,
	0x3f184731, 0xbf27754f, 0xbd4ea19d, 0x3ef8d9f2,
	0x3f7925f2, 0xbf7cd5fa, 0xbf1a7735, 0x3f5d2bba,
	0x3e7279e5, 0x3f137927, 0x3f5829b0, 0x3f427985,
};
static const deUint32 s_snormInt16UintRef[] =
{
	0xffffe9a0, 0x000030aa, 0x00006116, 0xffffa237,
	0x00004c23, 0xffffac46, 0xfffff98b, 0x00003e36,
	0x00007c92, 0xffff8196, 0xffffb2c5, 0x00006e95,
	0x00001e4f, 0x000049bc, 0x00006c14, 0x0000613c,
};
static const deUint32 s_snormInt16IntRef[] =
{
	0xffffe9a0, 0x000030aa, 0x00006116, 0xffffa237,
	0x00004c23, 0xffffac46, 0xfffff98b, 0x00003e36,
	0x00007c92, 0xffff8196, 0xffffb2c5, 0x00006e95,
	0x00001e4f, 0x000049bc, 0x00006c14, 0x0000613c,
};

static const deUint8 s_snormInt32In[] =
{
	0xba, 0x2c, 0x02, 0xea,
	0x75, 0x59, 0x74, 0x48,
	0x32, 0xad, 0xb0, 0xda,
	0x0b, 0xf7, 0x6f, 0x49,
	0x98, 0x9b, 0x76, 0x66,
	0x79, 0x7d, 0x69, 0x33,
	0xb5, 0x74, 0x61, 0xa4,
	0x4c, 0xcd, 0x5c, 0x20,
	0xc3, 0xba, 0x90, 0xfc,
	0xe3, 0x17, 0xd0, 0x89,
	0x28, 0x61, 0x5d, 0xb0,
	0x5d, 0xc9, 0xad, 0xc9,
	0xfc, 0x8c, 0x48, 0x3c,
	0x11, 0x13, 0x40, 0x27,
	0xe4, 0x88, 0x27, 0x4f,
	0x52, 0xa2, 0x54, 0x50,
};
static const deUint32 s_snormInt32FloatRef[] =
{
	0xbe2fee9a, 0x3f10e8b3, 0xbe953d4b, 0x3f12dfee,
	0x3f4ced37, 0x3ecda5f6, 0xbf373d17, 0x3e817335,
	0xbcdbd14f, 0xbf6c5fd0, 0xbf1f453e, 0xbed948db,
	0x3ef12234, 0x3e9d004c, 0x3f1e4f12, 0x3f20a945,
};
static const deUint32 s_snormInt32UintRef[] =
{
	0xea022cba, 0x48745975, 0xdab0ad32, 0x496ff70b,
	0x66769b98, 0x33697d79, 0xa46174b5, 0x205ccd4c,
	0xfc90bac3, 0x89d017e3, 0xb05d6128, 0xc9adc95d,
	0x3c488cfc, 0x27401311, 0x4f2788e4, 0x5054a252,
};
static const deUint32 s_snormInt32IntRef[] =
{
	0xea022cba, 0x48745975, 0xdab0ad32, 0x496ff70b,
	0x66769b98, 0x33697d79, 0xa46174b5, 0x205ccd4c,
	0xfc90bac3, 0x89d017e3, 0xb05d6128, 0xc9adc95d,
	0x3c488cfc, 0x27401311, 0x4f2788e4, 0x5054a252,
};

static const deUint8 s_unormInt8In[] =
{
	0x90, 0xa0, 0xa9, 0x26,
	0x24, 0xc4, 0xa1, 0xa5,
	0xdb, 0x0e, 0x09, 0x7a,
	0x7f, 0x3d, 0xf2, 0x1f,
};
static const deUint32 s_unormInt8FloatRef[] =
{
	0x3f109091, 0x3f20a0a1, 0x3f29a9aa, 0x3e189899,
	0x3e109091, 0x3f44c4c5, 0x3f21a1a2, 0x3f25a5a6,
	0x3f5bdbdc, 0x3d60e0e1, 0x3d109091, 0x3ef4f4f5,
	0x3efefeff, 0x3e74f4f5, 0x3f72f2f3, 0x3df8f8f9,
};
static const deUint32 s_unormInt8UintRef[] =
{
	0x00000090, 0x000000a0, 0x000000a9, 0x00000026,
	0x00000024, 0x000000c4, 0x000000a1, 0x000000a5,
	0x000000db, 0x0000000e, 0x00000009, 0x0000007a,
	0x0000007f, 0x0000003d, 0x000000f2, 0x0000001f,
};
static const deUint32 s_unormInt8IntRef[] =
{
	0x00000090, 0x000000a0, 0x000000a9, 0x00000026,
	0x00000024, 0x000000c4, 0x000000a1, 0x000000a5,
	0x000000db, 0x0000000e, 0x00000009, 0x0000007a,
	0x0000007f, 0x0000003d, 0x000000f2, 0x0000001f,
};

static const deUint8 s_unormInt16In[] =
{
	0xb6, 0x85, 0xf0, 0x1a,
	0xbc, 0x76, 0x5b, 0x59,
	0xf8, 0x74, 0x80, 0x6c,
	0xb1, 0x80, 0x4a, 0xdc,
	0xeb, 0x61, 0xa3, 0x12,
	0xf6, 0x65, 0x6b, 0x25,
	0x29, 0xe0, 0xe3, 0x0d,
	0x3a, 0xac, 0xa7, 0x97,
};
static const deUint32 s_unormInt16FloatRef[] =
{
	0x3f05b686, 0x3dd780d8, 0x3eed78ed, 0x3eb2b6b3,
	0x3ee9f0ea, 0x3ed900d9, 0x3f00b181, 0x3f5c4adc,
	0x3ec3d6c4, 0x3d951895, 0x3ecbeccc, 0x3e15ac96,
	0x3f6029e0, 0x3d5e30de, 0x3f2c3aac, 0x3f17a798,
};
static const deUint32 s_unormInt16UintRef[] =
{
	0x000085b6, 0x00001af0, 0x000076bc, 0x0000595b,
	0x000074f8, 0x00006c80, 0x000080b1, 0x0000dc4a,
	0x000061eb, 0x000012a3, 0x000065f6, 0x0000256b,
	0x0000e029, 0x00000de3, 0x0000ac3a, 0x000097a7,
};
static const deUint32 s_unormInt16IntRef[] =
{
	0x000085b6, 0x00001af0, 0x000076bc, 0x0000595b,
	0x000074f8, 0x00006c80, 0x000080b1, 0x0000dc4a,
	0x000061eb, 0x000012a3, 0x000065f6, 0x0000256b,
	0x0000e029, 0x00000de3, 0x0000ac3a, 0x000097a7,
};

static const deUint8 s_unormInt24In[] =
{
	0xea, 0x65, 0x31, 0xb3,
	0x53, 0x62, 0x02, 0xf1,
	0xda, 0x3c, 0xaf, 0x31,
	0x35, 0xd6, 0x1f, 0xe4,
	0xfa, 0x3b, 0xb9, 0x48,
	0x73, 0x9a, 0xde, 0x6b,
	0x3e, 0xa5, 0x15, 0x90,
	0x95, 0xc2, 0x56, 0x8b,
	0xd2, 0x14, 0xd5, 0xe5,
	0xd0, 0x7b, 0x9f, 0x74,
	0x79, 0x58, 0x86, 0xa9,
	0xc0, 0xdf, 0xb6, 0xb4,
};
static const deUint32 s_unormInt24FloatRef[] =
{
	0x3e4597a9, 0x3ec4a767, 0x3f5af103, 0x3e46bcf1,
	0x3dfeb1a9, 0x3e6feb91, 0x3ee69173, 0x3ed7bd35,
	0x3dad29f1, 0x3f429591, 0x3f528b57, 0x3f65d515,
	0x3f1f7bd1, 0x3eb0f2e9, 0x3f40a987, 0x3f34b6e0,
};
static const deUint32 s_unormInt24UintRef[] =
{
	0x003165ea, 0x006253b3, 0x00daf102, 0x0031af3c,
	0x001fd635, 0x003bfae4, 0x007348b9, 0x006bde9a,
	0x0015a53e, 0x00c29590, 0x00d28b56, 0x00e5d514,
	0x009f7bd0, 0x00587974, 0x00c0a986, 0x00b4b6df,
};
static const deUint32 s_unormInt24IntRef[] =
{
	0x003165ea, 0x006253b3, 0x00daf102, 0x0031af3c,
	0x001fd635, 0x003bfae4, 0x007348b9, 0x006bde9a,
	0x0015a53e, 0x00c29590, 0x00d28b56, 0x00e5d514,
	0x009f7bd0, 0x00587974, 0x00c0a986, 0x00b4b6df,
};

static const deUint8 s_unormInt32In[] =
{
	0x45, 0x7d, 0xe1, 0x55,
	0xd2, 0xcb, 0xc5, 0x17,
	0x64, 0x87, 0x84, 0x50,
	0x37, 0x60, 0x54, 0xa1,
	0xa8, 0x7e, 0xea, 0x98,
	0x1a, 0xd1, 0xb4, 0x70,
	0x2d, 0xcb, 0xff, 0x13,
	0x3d, 0xd7, 0x3c, 0xe4,
	0x94, 0xd6, 0xb4, 0xf7,
	0x01, 0x58, 0x32, 0x9d,
	0x91, 0x2b, 0x49, 0x1f,
	0xd0, 0xca, 0x3d, 0x05,
	0x14, 0x5a, 0x95, 0xd0,
	0xfd, 0x64, 0x33, 0xd3,
	0x73, 0x87, 0xa5, 0xf9,
	0x6d, 0xc8, 0x39, 0x03,
};
static const deUint32 s_unormInt32FloatRef[] =
{
	0x3eabc2fb, 0x3dbe2e5f, 0x3ea1090f, 0x3f215460,
	0x3f18ea7f, 0x3ee169a2, 0x3d9ffe59, 0x3f643cd7,
	0x3f77b4d7, 0x3f1d3258, 0x3dfa495d, 0x3ca7b95a,
	0x3f50955a, 0x3f533365, 0x3f79a587, 0x3c4e721b,
};
static const deUint32 s_unormInt32UintRef[] =
{
	0x55e17d45, 0x17c5cbd2, 0x50848764, 0xa1546037,
	0x98ea7ea8, 0x70b4d11a, 0x13ffcb2d, 0xe43cd73d,
	0xf7b4d694, 0x9d325801, 0x1f492b91, 0x053dcad0,
	0xd0955a14, 0xd33364fd, 0xf9a58773, 0x0339c86d,
};
static const deUint32 s_unormInt32IntRef[] =
{
	0x55e17d45, 0x17c5cbd2, 0x50848764, 0xa1546037,
	0x98ea7ea8, 0x70b4d11a, 0x13ffcb2d, 0xe43cd73d,
	0xf7b4d694, 0x9d325801, 0x1f492b91, 0x053dcad0,
	0xd0955a14, 0xd33364fd, 0xf9a58773, 0x0339c86d,
};

static const deUint8 s_unormByte44In[] =
{
	0xdb, 0xa8, 0x29, 0x2d,
};
static const deUint32 s_unormByte44FloatRef[] =
{
	0x3f5dddde, 0x3f3bbbbc, 0x00000000, 0x3f800000,
	0x3f2aaaab, 0x3f088889, 0x00000000, 0x3f800000,
	0x3e088889, 0x3f19999a, 0x00000000, 0x3f800000,
	0x3e088889, 0x3f5dddde, 0x00000000, 0x3f800000,
};
static const deUint32 s_unormByte44IntRef[] =
{
	0x0000000d, 0x0000000b, 0x00000000, 0x00000001,
	0x0000000a, 0x00000008, 0x00000000, 0x00000001,
	0x00000002, 0x00000009, 0x00000000, 0x00000001,
	0x00000002, 0x0000000d, 0x00000000, 0x00000001,
};
static const deUint32 s_unsignedByte44FloatRef[] =
{
	0x41500000, 0x41300000, 0x00000000, 0x3f800000,
	0x41200000, 0x41000000, 0x00000000, 0x3f800000,
	0x40000000, 0x41100000, 0x00000000, 0x3f800000,
	0x40000000, 0x41500000, 0x00000000, 0x3f800000,
};

static const deUint8 s_unormShort565In[] =
{
	0xea, 0x7e, 0xcc, 0x28,
	0x38, 0xce, 0x8f, 0x16,
};
static const deUint32 s_unormShort565FloatRef[] =
{
	0x3ef7bdef, 0x3f5f7df8, 0x3ea5294a, 0x3f800000,
	0x3e25294a, 0x3dc30c31, 0x3ec6318c, 0x3f800000,
	0x3f4e739d, 0x3f471c72, 0x3f46318c, 0x3f800000,
	0x3d842108, 0x3f534d35, 0x3ef7bdef, 0x3f800000,
};
static const deUint32 s_unormShort565IntRef[] =
{
	0x0000000f, 0x00000037, 0x0000000a, 0x00000001,
	0x00000005, 0x00000006, 0x0000000c, 0x00000001,
	0x00000019, 0x00000031, 0x00000018, 0x00000001,
	0x00000002, 0x00000034, 0x0000000f, 0x00000001,
};
static const deUint32 s_unsignedShort565FloatRef[] =
{
	0x41700000, 0x425c0000, 0x41200000, 0x3f800000,
	0x40a00000, 0x40c00000, 0x41400000, 0x3f800000,
	0x41c80000, 0x42440000, 0x41c00000, 0x3f800000,
	0x40000000, 0x42500000, 0x41700000, 0x3f800000,
};

static const deUint8 s_unormShort555In[] =
{
	0x02, 0xea, 0x89, 0x13,
	0x94, 0x5a, 0x5b, 0x60,
};
static const deUint32 s_unormShort555FloatRef[] =
{
	0x3f56b5ad, 0x3f042108, 0x3d842108, 0x3f800000,
	0x3e042108, 0x3f6739ce, 0x3e94a529, 0x3f800000,
	0x3f35ad6b, 0x3f25294a, 0x3f25294a, 0x3f800000,
	0x3f46318c, 0x3d842108, 0x3f5ef7be, 0x3f800000,
};
static const deUint32 s_unormShort555IntRef[] =
{
	0x0000001a, 0x00000010, 0x00000002, 0x00000001,
	0x00000004, 0x0000001c, 0x00000009, 0x00000001,
	0x00000016, 0x00000014, 0x00000014, 0x00000001,
	0x00000018, 0x00000002, 0x0000001b, 0x00000001,
};

static const deUint8 s_unormShort4444In[] =
{
	0x19, 0xdb, 0xa8, 0xa8,
	0x72, 0x29, 0xb4, 0x2d,
};
static const deUint32 s_unormShort4444FloatRef[] =
{
	0x3f5dddde, 0x3f3bbbbc, 0x3d888889, 0x3f19999a,
	0x3f2aaaab, 0x3f088889, 0x3f2aaaab, 0x3f088889,
	0x3e088889, 0x3f19999a, 0x3eeeeeef, 0x3e088889,
	0x3e088889, 0x3f5dddde, 0x3f3bbbbc, 0x3e888889,
};
static const deUint32 s_unormShort4444IntRef[] =
{
	0x0000000d, 0x0000000b, 0x00000001, 0x00000009,
	0x0000000a, 0x00000008, 0x0000000a, 0x00000008,
	0x00000002, 0x00000009, 0x00000007, 0x00000002,
	0x00000002, 0x0000000d, 0x0000000b, 0x00000004,
};
static const deUint32 s_unsignedShort4444FloatRef[] =
{
	0x41500000, 0x41300000, 0x3f800000, 0x41100000,
	0x41200000, 0x41000000, 0x41200000, 0x41000000,
	0x40000000, 0x41100000, 0x40e00000, 0x40000000,
	0x40000000, 0x41500000, 0x41300000, 0x40800000,
};

static const deUint8 s_unormShort5551In[] =
{
	0x13, 0x89, 0x6f, 0x3c,
	0xae, 0xe9, 0xf2, 0xd9,
};
static const deUint32 s_unormShort5551FloatRef[] =
{
	0x3f0c6319, 0x3e042108, 0x3e94a529, 0x3f800000,
	0x3e6739ce, 0x3f0c6319, 0x3f3def7c, 0x3f800000,
	0x3f6f7bdf, 0x3e46318c, 0x3f3def7c, 0x00000000,
	0x3f5ef7be, 0x3e6739ce, 0x3f4e739d, 0x00000000,
};
static const deUint32 s_unormShort5551IntRef[] =
{
	0x00000011, 0x00000004, 0x00000009, 0x00000001,
	0x00000007, 0x00000011, 0x00000017, 0x00000001,
	0x0000001d, 0x00000006, 0x00000017, 0x00000000,
	0x0000001b, 0x00000007, 0x00000019, 0x00000000,
};
static const deUint32 s_unsignedShort5551FloatRef[] =
{
	0x41880000, 0x40800000, 0x41100000, 0x3f800000,
	0x40e00000, 0x41880000, 0x41b80000, 0x3f800000,
	0x41e80000, 0x40c00000, 0x41b80000, 0x00000000,
	0x41d80000, 0x40e00000, 0x41c80000, 0x00000000,
};

static const deUint8 s_unormShort1555In[] =
{
	0xf8, 0xc5, 0x1f, 0x6c,
	0xf0, 0x2f, 0xf2, 0x95,
};
static const deUint32 s_unormShort1555FloatRef[] =
{
	0x3f800000, 0x3f0c6319, 0x3ef7bdef, 0x3f46318c,
	0x00000000, 0x3f5ef7be, 0x00000000, 0x3f800000,
	0x00000000, 0x3eb5ad6b, 0x3f800000, 0x3f042108,
	0x3f800000, 0x3e25294a, 0x3ef7bdef, 0x3f14a529,
};
static const deUint32 s_unormShort1555IntRef[] =
{
	0x00000001, 0x00000011, 0x0000000f, 0x00000018,
	0x00000000, 0x0000001b, 0x00000000, 0x0000001f,
	0x00000000, 0x0000000b, 0x0000001f, 0x00000010,
	0x00000001, 0x00000005, 0x0000000f, 0x00000012,
};

static const deUint8 s_unormInt101010In[] =
{
	0x81, 0xb3, 0x67, 0x51,
	0xa9, 0x00, 0x34, 0xc5,
	0xf0, 0x2f, 0xf2, 0x95,
	0xf8, 0xc5, 0x1f, 0x6c,
};
static const deUint32 s_unormInt101010FloatRef[] =
{
	0x3ea2a8aa, 0x3f1ee7ba, 0x3e60380e, 0x3f800000,
	0x3f45314c, 0x3f50340d, 0x3d282a0b, 0x3f800000,
	0x3f15e579, 0x3f48b22d, 0x3f7f3fd0, 0x3f800000,
	0x3ed8360e, 0x3efe3f90, 0x3ebf2fcc, 0x3f800000,
};
static const deUint32 s_unormInt101010IntRef[] =
{
	0x00000145, 0x0000027b, 0x000000e0, 0x00000001,
	0x00000314, 0x00000340, 0x0000002a, 0x00000001,
	0x00000257, 0x00000322, 0x000003fc, 0x00000001,
	0x000001b0, 0x000001fc, 0x0000017e, 0x00000001,
};

static const deUint8 s_unormInt1010102RevIn[] =
{
	0xfd, 0xc6, 0xf5, 0xc4,
	0x32, 0xa8, 0xfd, 0x7d,
	0xe7, 0x3f, 0x10, 0xd0,
	0x86, 0x0d, 0x66, 0xd0,
};
static const deUint32 s_unormInt1010102RevFloatRef[] =
{
	0x3f3f6fdc, 0x3eb8ae2c, 0x3d9e278a, 0x3f800000,
	0x3d48320d, 0x3f5ab6ae, 0x3f77fdff, 0x3eaaaaab,
	0x3f79fe80, 0x3c703c0f, 0x3e80a028, 0x3f800000,
	0x3ec330cc, 0x3ec1b06c, 0x3e8320c8, 0x3f800000,
};
static const deUint32 s_unormInt1010102RevIntRef[] =
{
	0x000002fd, 0x00000171, 0x0000004f, 0x00000003,
	0x00000032, 0x0000036a, 0x000003df, 0x00000001,
	0x000003e7, 0x0000000f, 0x00000101, 0x00000003,
	0x00000186, 0x00000183, 0x00000106, 0x00000003,
};
static const deUint32 s_snormInt1010102RevFloatRef[] =
{
	0xbf01c0e0, 0x3f38dc6e, 0x3e1e4f28, 0xbf800000,
	0x3dc86432, 0xbe964b26, 0xbd844221, 0x3f800000,
	0xbd486432, 0x3cf0783c, 0x3f00c060, 0xbf800000,
	0x3f4361b1, 0x3f41e0f0, 0x3f0341a1, 0xbf800000,
};
static const deUint32 s_snormInt1010102RevIntRef[] =
{
	0xfffffefd, 0x00000171, 0x0000004f, 0xffffffff,
	0x00000032, 0xffffff6a, 0xffffffdf, 0x00000001,
	0xffffffe7, 0x0000000f, 0x00000101, 0xffffffff,
	0x00000186, 0x00000183, 0x00000106, 0xffffffff,
};

static const deUint8 s_unsignedInt1010102RevIn[] =
{
	0xb8, 0x4c, 0xfd, 0x00,
	0x65, 0x7f, 0xb2, 0x4e,
	0x11, 0x3e, 0x03, 0x23,
	0xae, 0xc9, 0xdd, 0xa2,
};
static const deUint32 s_unsignedInt1010102RevFloatRef[] =
{
	0x43380000, 0x4454c000, 0x41700000, 0x00000000,
	0x44594000, 0x431f0000, 0x436b0000, 0x3f800000,
	0x44044000, 0x434f0000, 0x440c0000, 0x00000000,
	0x43d70000, 0x445c8000, 0x440b4000, 0x40000000,
};
static const deUint32 s_unsignedInt1010102RevIntRef[] =
{
	0x000000b8, 0x00000353, 0x0000000f, 0x00000000,
	0x00000365, 0x0000009f, 0x000000eb, 0x00000001,
	0x00000211, 0x000000cf, 0x00000230, 0x00000000,
	0x000001ae, 0x00000372, 0x0000022d, 0x00000002,
};
static const deUint32 s_signedInt1010102RevFloatRef[] =
{
	0x43380000, 0x4f7fffff, 0x41700000, 0x00000000,
	0x4f7fffff, 0x431f0000, 0x436b0000, 0x3f800000,
	0x4f7ffffe, 0x434f0000, 0x4f7ffffe, 0x00000000,
	0x43d70000, 0x4f7fffff, 0x4f7ffffe, 0x4f800000,
};
static const deUint32 s_signedInt1010102RevIntRef[] =
{
	0x000000b8, 0xffffff53, 0x0000000f, 0x00000000,
	0xffffff65, 0x0000009f, 0x000000eb, 0x00000001,
	0xfffffe11, 0x000000cf, 0xfffffe30, 0x00000000,
	0x000001ae, 0xffffff72, 0xfffffe2d, 0xfffffffe,
};

static const deUint8 s_unsignedInt11f11f10fRevIn[] =
{
	0x8e, 0x1b, 0x81, 0x45,
	0xcf, 0x47, 0x50, 0x29,
	0xff, 0x5e, 0x8e, 0x93,
	0x95, 0x07, 0x45, 0x4a,
};
static const deUint32 s_unsignedInt11f11f10fRevFloatRef[] =
{
	0x3f1c0000, 0x380c0000, 0x3c580000, 0x3f800000,
	0x7fffffff, 0x3c100000, 0x3a940000, 0x3f800000,
	0x45fe0000, 0x3b960000, 0x41380000, 0x3f800000,
	0x472a0000, 0x39400000, 0x3ca40000, 0x3f800000,
};

static const deUint8 s_unsignedInt999E5RevIn[] =
{
	0x88, 0x8b, 0x50, 0x34,
	0x2b, 0x2f, 0xe2, 0x92,
	0x95, 0x7f, 0xeb, 0x18,
	0x6b, 0xe2, 0x27, 0x30,
};
static const deUint32 s_unsignedInt999E5RevFloatRef[] =
{
	0x3ac40000, 0x398a0000, 0x3a8a0000, 0x3f800000,
	0x40958000, 0x408b8000, 0x40380000, 0x3f800000,
	0x394a8000, 0x395f8000, 0x37e80000, 0x3f800000,
	0x39d60000, 0x3af88000, 0x38100000, 0x3f800000,
};

static const deUint8 s_unsignedInt1688In[] =
{
	0x02, 0x50, 0x91, 0x85,
	0xcc, 0xe2, 0xfd, 0xc8,
	0x62, 0xeb, 0x0f, 0xe6,
	0x95, 0x27, 0x26, 0x24,
};
static const deUint32 s_unsignedInt1688FloatRef[] =
{
	0x3f059186, 0x00000000, 0x00000000, 0x3f800000,
	0x3f48fdc9, 0x00000000, 0x00000000, 0x3f800000,
	0x3f660fe6, 0x00000000, 0x00000000, 0x3f800000,
	0x3e109891, 0x00000000, 0x00000000, 0x3f800000,
};
static const deUint32 s_unsignedInt1688UintRef[] =
{
	0x00000002, 0x00000000, 0x00000000, 0x00000001,
	0x000000cc, 0x00000000, 0x00000000, 0x00000001,
	0x00000062, 0x00000000, 0x00000000, 0x00000001,
	0x00000095, 0x00000000, 0x00000000, 0x00000001,
};

static const deUint8 s_unsignedInt248In[] =
{
	0xea, 0x7e, 0xcc, 0x28,
	0x38, 0xce, 0x8f, 0x16,
	0x3e, 0x4f, 0xe2, 0xfd,
	0x74, 0x5e, 0xf2, 0x30,
};
static const deUint32 s_unsignedInt248FloatRef[] =
{
	0x3e2331f9, 0x00000000, 0x00000000, 0x3f800000,
	0x3db47e71, 0x00000000, 0x00000000, 0x3f800000,
	0x3f7de250, 0x00000000, 0x00000000, 0x3f800000,
	0x3e43c979, 0x00000000, 0x00000000, 0x3f800000,
};
static const deUint32 s_unsignedInt248UintRef[] =
{
	0x000000ea, 0x00000000, 0x00000000, 0x00000001,
	0x00000038, 0x00000000, 0x00000000, 0x00000001,
	0x0000003e, 0x00000000, 0x00000000, 0x00000001,
	0x00000074, 0x00000000, 0x00000000, 0x00000001,
};

static const deUint8 s_unsignedInt248RevIn[] =
{
	0x7e, 0xcc, 0x28, 0xea,
	0xce, 0x8f, 0x16, 0x38,
	0x4f, 0xe2, 0xfd, 0x3e,
	0x5e, 0xf2, 0x30, 0x74,
};
static const deUint32 s_unsignedInt248RevFloatRef[] =
{
	0x3e2331f9, 0x00000000, 0x00000000, 0x3f800000,
	0x3db47e71, 0x00000000, 0x00000000, 0x3f800000,
	0x3f7de250, 0x00000000, 0x00000000, 0x3f800000,
	0x3e43c979, 0x00000000, 0x00000000, 0x3f800000,
};
static const deUint32 s_unsignedInt248RevUintRef[] =
{
	0x000000ea, 0x00000000, 0x00000000, 0x00000001,
	0x00000038, 0x00000000, 0x00000000, 0x00000001,
	0x0000003e, 0x00000000, 0x00000000, 0x00000001,
	0x00000074, 0x00000000, 0x00000000, 0x00000001,
};

static const deUint8 s_signedInt8In[] =
{
	0x3a, 0x5b, 0x6d, 0x6a,
	0x44, 0x56, 0x6b, 0x21,
	0x6a, 0x0b, 0x24, 0xd9,
	0xd4, 0xb4, 0xda, 0x97,
};
static const deUint32 s_signedInt8FloatRef[] =
{
	0x42680000, 0x42b60000, 0x42da0000, 0x42d40000,
	0x42880000, 0x42ac0000, 0x42d60000, 0x42040000,
	0x42d40000, 0x41300000, 0x42100000, 0xc21c0000,
	0xc2300000, 0xc2980000, 0xc2180000, 0xc2d20000,
};
static const deUint32 s_signedInt8UintRef[] =
{
	0x0000003a, 0x0000005b, 0x0000006d, 0x0000006a,
	0x00000044, 0x00000056, 0x0000006b, 0x00000021,
	0x0000006a, 0x0000000b, 0x00000024, 0xffffffd9,
	0xffffffd4, 0xffffffb4, 0xffffffda, 0xffffff97,
};
static const deUint32 s_signedInt8IntRef[] =
{
	0x0000003a, 0x0000005b, 0x0000006d, 0x0000006a,
	0x00000044, 0x00000056, 0x0000006b, 0x00000021,
	0x0000006a, 0x0000000b, 0x00000024, 0xffffffd9,
	0xffffffd4, 0xffffffb4, 0xffffffda, 0xffffff97,
};

static const deUint8 s_signedInt16In[] =
{
	0xf1, 0xdd, 0xcd, 0xc3,
	0x1c, 0xb6, 0x6f, 0x74,
	0x19, 0x13, 0x25, 0xed,
	0x16, 0xce, 0x0d, 0x0f,
	0x5c, 0xf4, 0x3c, 0xa3,
	0x6d, 0x25, 0x65, 0x6d,
	0xae, 0x5d, 0x88, 0xfa,
	0x86, 0x3e, 0x6a, 0x91,
};
static const deUint32 s_signedInt16FloatRef[] =
{
	0xc6083c00, 0xc670cc00, 0xc693c800, 0x46e8de00,
	0x4598c800, 0xc596d800, 0xc647a800, 0x4570d000,
	0xc53a4000, 0xc6b98800, 0x4615b400, 0x46daca00,
	0x46bb5c00, 0xc4af0000, 0x467a1800, 0xc6dd2c00,
};
static const deUint32 s_signedInt16UintRef[] =
{
	0xffffddf1, 0xffffc3cd, 0xffffb61c, 0x0000746f,
	0x00001319, 0xffffed25, 0xffffce16, 0x00000f0d,
	0xfffff45c, 0xffffa33c, 0x0000256d, 0x00006d65,
	0x00005dae, 0xfffffa88, 0x00003e86, 0xffff916a,
};
static const deUint32 s_signedInt16IntRef[] =
{
	0xffffddf1, 0xffffc3cd, 0xffffb61c, 0x0000746f,
	0x00001319, 0xffffed25, 0xffffce16, 0x00000f0d,
	0xfffff45c, 0xffffa33c, 0x0000256d, 0x00006d65,
	0x00005dae, 0xfffffa88, 0x00003e86, 0xffff916a,
};

static const deUint8 s_signedInt32In[] =
{
	0xc6, 0x7e, 0x50, 0x2a,
	0xec, 0x0f, 0x9b, 0x44,
	0x4d, 0xa9, 0x77, 0x0d,
	0x69, 0x4c, 0xd3, 0x76,
	0xf0, 0xb7, 0xde, 0x6b,
	0x4e, 0xe2, 0xb1, 0x58,
	0xa8, 0x9c, 0xfc, 0x6d,
	0x75, 0x8f, 0x3c, 0x7f,
	0xf3, 0x19, 0x14, 0x97,
	0xf0, 0x87, 0x5c, 0x11,
	0x95, 0x32, 0xab, 0x7a,
	0x03, 0x2b, 0xdf, 0x52,
	0x68, 0x84, 0xd9, 0x91,
	0xec, 0x2a, 0xf1, 0xd0,
	0xf7, 0x73, 0x8f, 0x0a,
	0x62, 0xd2, 0x76, 0xfd,
};
static const deUint32 s_signedInt32FloatRef[] =
{
	0x4e2941fb, 0x4e893620, 0x4d577a95, 0x4eeda699,
	0x4ed7bd70, 0x4eb163c5, 0x4edbf939, 0x4efe791f,
	0xced1d7cc, 0x4d8ae440, 0x4ef55665, 0x4ea5be56,
	0xcedc4cf7, 0xce3c3b54, 0x4d28f73f, 0xcc224b68,
};
static const deUint32 s_signedInt32UintRef[] =
{
	0x2a507ec6, 0x449b0fec, 0x0d77a94d, 0x76d34c69,
	0x6bdeb7f0, 0x58b1e24e, 0x6dfc9ca8, 0x7f3c8f75,
	0x971419f3, 0x115c87f0, 0x7aab3295, 0x52df2b03,
	0x91d98468, 0xd0f12aec, 0x0a8f73f7, 0xfd76d262,
};
static const deUint32 s_signedInt32IntRef[] =
{
	0x2a507ec6, 0x449b0fec, 0x0d77a94d, 0x76d34c69,
	0x6bdeb7f0, 0x58b1e24e, 0x6dfc9ca8, 0x7f3c8f75,
	0x971419f3, 0x115c87f0, 0x7aab3295, 0x52df2b03,
	0x91d98468, 0xd0f12aec, 0x0a8f73f7, 0xfd76d262,
};

static const deUint8 s_unsignedInt8In[] =
{
	0x68, 0xa6, 0x99, 0x6e,
	0x13, 0x90, 0x0f, 0x40,
	0x34, 0x76, 0x05, 0x9a,
	0x6c, 0x9c, 0x1d, 0x6a,
};
static const deUint32 s_unsignedInt8FloatRef[] =
{
	0x42d00000, 0x43260000, 0x43190000, 0x42dc0000,
	0x41980000, 0x43100000, 0x41700000, 0x42800000,
	0x42500000, 0x42ec0000, 0x40a00000, 0x431a0000,
	0x42d80000, 0x431c0000, 0x41e80000, 0x42d40000,
};
static const deUint32 s_unsignedInt8UintRef[] =
{
	0x00000068, 0x000000a6, 0x00000099, 0x0000006e,
	0x00000013, 0x00000090, 0x0000000f, 0x00000040,
	0x00000034, 0x00000076, 0x00000005, 0x0000009a,
	0x0000006c, 0x0000009c, 0x0000001d, 0x0000006a,
};
static const deUint32 s_unsignedInt8IntRef[] =
{
	0x00000068, 0x000000a6, 0x00000099, 0x0000006e,
	0x00000013, 0x00000090, 0x0000000f, 0x00000040,
	0x00000034, 0x00000076, 0x00000005, 0x0000009a,
	0x0000006c, 0x0000009c, 0x0000001d, 0x0000006a,
};

static const deUint8 s_unsignedInt16In[] =
{
	0xa5, 0x62, 0x98, 0x7c,
	0x13, 0x21, 0xc8, 0xf4,
	0x78, 0x0b, 0x9f, 0xc2,
	0x92, 0x1c, 0xa9, 0x25,
	0x86, 0xea, 0x1f, 0x1c,
	0x41, 0xf7, 0xe2, 0x2e,
	0x38, 0x69, 0xf2, 0x6d,
	0x01, 0xec, 0x7f, 0xc5,
};
static const deUint32 s_unsignedInt16FloatRef[] =
{
	0x46c54a00, 0x46f93000, 0x46044c00, 0x4774c800,
	0x45378000, 0x47429f00, 0x45e49000, 0x4616a400,
	0x476a8600, 0x45e0f800, 0x47774100, 0x463b8800,
	0x46d27000, 0x46dbe400, 0x476c0100, 0x47457f00,
};
static const deUint32 s_unsignedInt16UintRef[] =
{
	0x000062a5, 0x00007c98, 0x00002113, 0x0000f4c8,
	0x00000b78, 0x0000c29f, 0x00001c92, 0x000025a9,
	0x0000ea86, 0x00001c1f, 0x0000f741, 0x00002ee2,
	0x00006938, 0x00006df2, 0x0000ec01, 0x0000c57f,
};
static const deUint32 s_unsignedInt16IntRef[] =
{
	0x000062a5, 0x00007c98, 0x00002113, 0x0000f4c8,
	0x00000b78, 0x0000c29f, 0x00001c92, 0x000025a9,
	0x0000ea86, 0x00001c1f, 0x0000f741, 0x00002ee2,
	0x00006938, 0x00006df2, 0x0000ec01, 0x0000c57f,
};

static const deUint8 s_unsignedInt24In[] =
{
	0xa8, 0x11, 0x00, 0xc8,
	0xe5, 0x07, 0xd3, 0x6d,
	0x0a, 0xc7, 0xe4, 0x42,
	0x2d, 0xf7, 0x5d, 0x9c,
	0x2e, 0x18, 0xfd, 0xa4,
	0x9e, 0x90, 0x0c, 0x31,
	0x06, 0x04, 0xc4, 0xc2,
	0xde, 0xfe, 0x7c, 0x1d,
	0x57, 0x37, 0x4a, 0xf2,
	0xe2, 0xf3, 0x74, 0x8e,
	0x8f, 0xd6, 0x73, 0xc4,
	0x91, 0xa0, 0x49, 0xe3,
};
static const deUint32 s_unsignedInt24FloatRef[] =
{
	0x458d4000, 0x48fcb900, 0x4926dd30, 0x4a85c98e,
	0x4abbee5a, 0x49c174e0, 0x4b1ea4fd, 0x4a443240,
	0x4b440406, 0x4b7edec2, 0x4aae3af8, 0x4b724a37,
	0x4ae9e7c4, 0x4b568f8e, 0x4b11c473, 0x4b6349a0,
};
static const deUint32 s_unsignedInt24UintRef[] =
{
	0x000011a8, 0x0007e5c8, 0x000a6dd3, 0x0042e4c7,
	0x005df72d, 0x00182e9c, 0x009ea4fd, 0x00310c90,
	0x00c40406, 0x00fedec2, 0x00571d7c, 0x00f24a37,
	0x0074f3e2, 0x00d68f8e, 0x0091c473, 0x00e349a0,
};
static const deUint32 s_unsignedInt24IntRef[] =
{
	0x000011a8, 0x0007e5c8, 0x000a6dd3, 0x0042e4c7,
	0x005df72d, 0x00182e9c, 0x009ea4fd, 0x00310c90,
	0x00c40406, 0x00fedec2, 0x00571d7c, 0x00f24a37,
	0x0074f3e2, 0x00d68f8e, 0x0091c473, 0x00e349a0,
};

static const deUint8 s_unsignedInt32In[] =
{
	0x90, 0xb0, 0x00, 0xa8,
	0xd8, 0x42, 0x5b, 0xae,
	0x40, 0x70, 0x38, 0x2a,
	0x92, 0x76, 0xd8, 0x70,
	0x04, 0x0d, 0x67, 0x87,
	0x9c, 0xdd, 0xb1, 0xeb,
	0xfc, 0x37, 0xe6, 0x40,
	0x24, 0x9c, 0x6a, 0x0f,
	0x09, 0x0e, 0xb6, 0x2f,
	0x31, 0x95, 0x43, 0x22,
	0x24, 0xde, 0x70, 0x2a,
	0x05, 0xa2, 0x84, 0x38,
	0x16, 0x9f, 0x65, 0x0e,
	0xb2, 0x99, 0x84, 0x6d,
	0xef, 0x86, 0x94, 0xf0,
	0x25, 0x9d, 0xf9, 0x67,
};
static const deUint32 s_unsignedInt32FloatRef[] =
{
	0x4f2800b1, 0x4f2e5b43, 0x4e28e1c1, 0x4ee1b0ed,
	0x4f07670d, 0x4f6bb1de, 0x4e81cc70, 0x4d76a9c2,
	0x4e3ed838, 0x4e090e55, 0x4e29c379, 0x4e621288,
	0x4d6659f1, 0x4edb0933, 0x4f709487, 0x4ecff33a,
};
static const deUint32 s_unsignedInt32UintRef[] =
{
	0xa800b090, 0xae5b42d8, 0x2a387040, 0x70d87692,
	0x87670d04, 0xebb1dd9c, 0x40e637fc, 0x0f6a9c24,
	0x2fb60e09, 0x22439531, 0x2a70de24, 0x3884a205,
	0x0e659f16, 0x6d8499b2, 0xf09486ef, 0x67f99d25,
};
static const deUint32 s_unsignedInt32IntRef[] =
{
	0xa800b090, 0xae5b42d8, 0x2a387040, 0x70d87692,
	0x87670d04, 0xebb1dd9c, 0x40e637fc, 0x0f6a9c24,
	0x2fb60e09, 0x22439531, 0x2a70de24, 0x3884a205,
	0x0e659f16, 0x6d8499b2, 0xf09486ef, 0x67f99d25,
};

static const deUint8 s_halfFloatIn[] =
{
	0x2b, 0x74, 0x6a, 0x5d,
	0x1c, 0xb2, 0x9a, 0x4d,
	0xad, 0x55, 0x22, 0x01,
	0xce, 0x2d, 0x97, 0x0d,
	0x71, 0x31, 0x42, 0x2b,
	0xeb, 0x26, 0xc7, 0x16,
	0x94, 0xd2, 0x22, 0x79,
	0x89, 0xbd, 0xff, 0xbc,
};
static const deUint32 s_halfFloatFloatRef[] =
{
	0x46856000, 0x43ad4000, 0xbe438000, 0x41b34000,
	0x42b5a000, 0x37910000, 0x3db9c000, 0x39b2e000,
	0x3e2e2000, 0x3d684000, 0x3cdd6000, 0x3ad8e000,
	0xc2528000, 0x47244000, 0xbfb12000, 0xbf9fe000,
};
static const deUint32 s_halfFloatUintRef[] =
{
	0x000042b0, 0x0000015a, 0x00000000, 0x00000016,
	0x0000005a, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0xffffffcc, 0x0000a440, 0xffffffff, 0xffffffff,
};
static const deUint32 s_halfFloatIntRef[] =
{
	0x000042b0, 0x0000015a, 0x00000000, 0x00000016,
	0x0000005a, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0xffffffcc, 0x0000a440, 0xffffffff, 0xffffffff,
};

static const deUint8 s_floatIn[] =
{
	0x92, 0xac, 0x68, 0x36,
	0x9f, 0x42, 0x0b, 0x6e,
	0x67, 0xcf, 0x0f, 0x20,
	0x22, 0x6c, 0xe4, 0x0f,
	0xb3, 0x72, 0xc8, 0x8a,
	0x4b, 0x99, 0xc3, 0xb0,
	0xbd, 0x78, 0x5c, 0x16,
	0x1c, 0xce, 0xb7, 0x4e,
	0x15, 0xdf, 0x37, 0xfd,
	0xeb, 0x32, 0xe9, 0x47,
	0x68, 0x1a, 0xaa, 0xd0,
	0xb9, 0xba, 0x77, 0xe7,
	0x81, 0x0a, 0x42, 0x5a,
	0xb0, 0x5a, 0xee, 0x06,
	0x77, 0xb4, 0x7b, 0x57,
	0xf5, 0x35, 0xac, 0x56,
};
static const deUint32 s_floatFloatRef[] =
{
	0x3668ac92, 0x6e0b429f, 0x200fcf67, 0x0fe46c22,
	0x8ac872b3, 0xb0c3994b, 0x165c78bd, 0x4eb7ce1c,
	0xfd37df15, 0x47e932eb, 0xd0aa1a68, 0xe777bab9,
	0x5a420a81, 0x06ee5ab0, 0x577bb477, 0x56ac35f5,
};
static const deUint32 s_floatUintRef[] =
{
	0x00000000, 0x80000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x5be70e00,
	0x80000000, 0x0001d265, 0x80000000, 0x80000000,
	0x80000000, 0x00000000, 0x80000000, 0x80000000,
};
static const deUint32 s_floatIntRef[] =
{
	0x00000000, 0x80000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x5be70e00,
	0x80000000, 0x0001d265, 0x80000000, 0x80000000,
	0x80000000, 0x00000000, 0x80000000, 0x80000000,
};

static const deUint8 s_float64In[] =
{
	0xbd, 0xb6, 0xc3, 0xd2,
	0xf6, 0x62, 0x29, 0xd9,
	0x2f, 0xc2, 0x46, 0x18,
	0x6b, 0x0d, 0x0a, 0x53,
	0x6d, 0x0c, 0xf3, 0x80,
	0xbd, 0xa9, 0x12, 0x89,
	0x6b, 0x9f, 0x3d, 0xdd,
	0xb3, 0x91, 0xee, 0xf5,
	0x92, 0xac, 0x68, 0x36,
	0x9f, 0x42, 0x0b, 0x6e,
	0x67, 0xcf, 0x0f, 0x20,
	0x22, 0x6c, 0xe4, 0x0f,
	0xb3, 0x72, 0xc8, 0x8a,
	0x4b, 0x99, 0xc3, 0xb0,
	0xbd, 0x78, 0x5c, 0x16,
	0x1c, 0xce, 0xb7, 0x4e,
	0x15, 0xdf, 0x37, 0xfd,
	0xeb, 0x32, 0xe9, 0x47,
	0x68, 0x1a, 0xaa, 0xd0,
	0xb9, 0xba, 0x77, 0xe7,
	0x81, 0x0a, 0x42, 0x5a,
	0xb0, 0x5a, 0xee, 0x06,
	0x77, 0xb4, 0x7b, 0x57,
	0xf5, 0x35, 0xac, 0x56,
	0x2b, 0x74, 0x6a, 0x5d,
	0x1c, 0xb2, 0x9a, 0x4d,
	0xad, 0x55, 0x22, 0x01,
	0xce, 0x2d, 0x97, 0x0d,
	0x71, 0x31, 0x42, 0x2b,
	0xeb, 0x26, 0xc7, 0x16,
	0x94, 0xd2, 0x22, 0x79,
	0x89, 0xbd, 0xff, 0xbc,
};
static const deUint32 s_float64FloatRef[] =
{
	0xff800000, 0x7f800000, 0x80000000, 0xff800000,
	0x7f800000, 0x00000000, 0x80000000, 0x7f800000,
	0x7f499760, 0xff800000, 0x00000000, 0x7f800000,
	0x7f800000, 0x00000000, 0x00000000, 0xa7fdec4c,
};
static const deUint32 s_float64IntRef[] =
{
	0x80000000, 0x80000000, 0x00000000, 0x80000000,
	0x80000000, 0x00000000, 0x00000000, 0x80000000,
	0x80000000, 0x80000000, 0x00000000, 0x80000000,
	0x80000000, 0x00000000, 0x00000000, 0x00000000,
};

static const deUint8 s_floatUnsignedInt248RevIn[] =
{
	0xbd, 0xb6, 0xc3, 0xd2,
	0xf6, 0x62, 0x29, 0xd9,
	0x2f, 0xc2, 0x46, 0x18,
	0x6b, 0x0d, 0x0a, 0x53,
	0x6d, 0x0c, 0xf3, 0x80,
	0xbd, 0xa9, 0x12, 0x89,
	0x6b, 0x9f, 0x3d, 0xdd,
	0xb3, 0x91, 0xee, 0xf5,
};
static const deUint32 s_floatUnsignedInt248RevFloatRef[] =
{
	0xd2c3b6bd, 0x00000000, 0x00000000, 0x3f800000,
	0x1846c22f, 0x00000000, 0x00000000, 0x3f800000,
	0x80f30c6d, 0x00000000, 0x00000000, 0x3f800000,
	0xdd3d9f6b, 0x00000000, 0x00000000, 0x3f800000,
};
static const deUint32 s_floatUnsignedInt248RevUintRef[] =
{
	0x000000f6, 0x00000000, 0x00000000, 0x00000001,
	0x0000006b, 0x00000000, 0x00000000, 0x00000001,
	0x000000bd, 0x00000000, 0x00000000, 0x00000001,
	0x000000b3, 0x00000000, 0x00000000, 0x00000001,
};

static const deUint8 s_unormShort10In[] =
{
	0x80, 0x84, 0x40, 0x3b,
	0x40, 0xfd, 0x80, 0x1a,
	0x80, 0x0c, 0x80, 0x15,
	0x40, 0x11, 0x80, 0xc3,
	0x80, 0xc8, 0x80, 0xd5,
	0xc0, 0xf9, 0x00, 0x0a,
	0xc0, 0x39, 0x40, 0xd5,
	0xc0, 0x4d, 0xc0, 0x26
};
static const deUint32 s_unormShort10FloatRef[] =
{
	0x3f04a128, 0x3e6d3b4f,
	0x3f7d7f60, 0x3dd4350d,
	0x3d48320d, 0x3dac2b0b,
	0x3d8a2289, 0x3f43b0ec,
	0x3f48b22d, 0x3f55b56d,
	0x3f79fe80, 0x3d20280a,
	0x3e6739ce, 0x3f55755d,
	0x3e9ba6ea, 0x3e1b26ca
};
static const deUint32 s_unormShort10UintRef[] =
{
	0x212, 0x0ed, 0x3f5, 0x06a,
	0x032, 0x056, 0x045, 0x30e,
	0x322, 0x356, 0x3e7, 0x028,
	0x0e7, 0x355, 0x137, 0x09b,
};
static const deUint32 s_unormShort10IntRef[] =
{
	0x212, 0x0ed, 0x3f5, 0x06a,
	0x032, 0x056, 0x045, 0x30e,
	0x322, 0x356, 0x3e7, 0x028,
	0x0e7, 0x355, 0x137, 0x09b,
};

static const deUint8 s_unormShort12In[] =
{
	0x30, 0x46, 0xf0, 0x38,
	0x90, 0x85, 0xf0, 0x88,
	0x90, 0x92, 0x30, 0x5d,
	0x30, 0x3a, 0x00, 0xc9,
	0x00, 0x64, 0xb0, 0x9b,
	0x20, 0x71, 0xd0, 0x5b,
	0xa0, 0xc5, 0x70, 0x27,
	0x30, 0x0b, 0xa0, 0x53
};
static const deUint32 s_unormShort12FloatRef[] =
{
	0x3e8c68c7, 0x3e63ce3d,
	0x3f05985a, 0x3f08f890,
	0x3f12992a, 0x3eba6ba7,
	0x3e68ce8d, 0x3f490c91,
	0x3ec80c81, 0x3f1bb9bc,
	0x3ee24e25, 0x3eb7ab7b,
	0x3f45ac5b, 0x3e1dc9dd,
	0x3d330b31, 0x3ea74a75
};
static const deUint32 s_unormShort12UintRef[] =
{
	0x463, 0x38f,
	0x859, 0x88f,
	0x929, 0x5d3,
	0x3a3, 0xc90,
	0x640, 0x9bb,
	0x712, 0x5bd,
	0xc5a, 0x277,
	0x0b3, 0x53a
};
static const deUint32 s_unormShort12IntRef[] =
{
	0x463, 0x38f,
	0x859, 0x88f,
	0x929, 0x5d3,
	0x3a3, 0xc90,
	0x640, 0x9bb,
	0x712, 0x5bd,
	0xc5a, 0x277,
	0x0b3, 0x53a
};

// \todo [2015-10-12 pyry] Collapse duplicate ref arrays

static const struct
{
	const deUint8*		input;
	const int			inputSize;
	const deUint32*		floatRef;
	const deUint32*		intRef;
	const deUint32*		uintRef;
} s_formatData[] =
{
	{ s_snormInt8In,				DE_LENGTH_OF_ARRAY(s_snormInt8In),					s_snormInt8FloatRef,				s_snormInt8IntRef,				s_snormInt8UintRef				},
	{ s_snormInt16In,				DE_LENGTH_OF_ARRAY(s_snormInt16In),					s_snormInt16FloatRef,				s_snormInt16IntRef,				s_snormInt16UintRef				},
	{ s_snormInt32In,				DE_LENGTH_OF_ARRAY(s_snormInt32In),					s_snormInt32FloatRef,				s_snormInt32IntRef,				s_snormInt32UintRef				},
	{ s_unormInt8In,				DE_LENGTH_OF_ARRAY(s_unormInt8In),					s_unormInt8FloatRef,				s_unormInt8IntRef,				s_unormInt8UintRef				},
	{ s_unormInt16In,				DE_LENGTH_OF_ARRAY(s_unormInt16In),					s_unormInt16FloatRef,				s_unormInt16IntRef,				s_unormInt16UintRef				},
	{ s_unormInt24In,				DE_LENGTH_OF_ARRAY(s_unormInt24In),					s_unormInt24FloatRef,				s_unormInt24IntRef,				s_unormInt24UintRef				},
	{ s_unormInt32In,				DE_LENGTH_OF_ARRAY(s_unormInt32In),					s_unormInt32FloatRef,				s_unormInt32IntRef,				s_unormInt32UintRef				},
	{ s_unormByte44In,				DE_LENGTH_OF_ARRAY(s_unormByte44In),				s_unormByte44FloatRef,				s_unormByte44IntRef,			s_unormByte44IntRef				},
	{ s_unormShort565In,			DE_LENGTH_OF_ARRAY(s_unormShort565In),				s_unormShort565FloatRef,			s_unormShort565IntRef,			s_unormShort565IntRef,			},
	{ s_unormShort555In,			DE_LENGTH_OF_ARRAY(s_unormShort555In),				s_unormShort555FloatRef,			s_unormShort555IntRef,			s_unormShort555IntRef,			},
	{ s_unormShort4444In,			DE_LENGTH_OF_ARRAY(s_unormShort4444In),				s_unormShort4444FloatRef,			s_unormShort4444IntRef,			s_unormShort4444IntRef,			},
	{ s_unormShort5551In,			DE_LENGTH_OF_ARRAY(s_unormShort5551In),				s_unormShort5551FloatRef,			s_unormShort5551IntRef,			s_unormShort5551IntRef,			},
	{ s_unormShort1555In,			DE_LENGTH_OF_ARRAY(s_unormShort1555In),				s_unormShort1555FloatRef,			s_unormShort1555IntRef,			s_unormShort1555IntRef,			},
	{ s_unormInt101010In,			DE_LENGTH_OF_ARRAY(s_unormInt101010In),				s_unormInt101010FloatRef,			s_unormInt101010IntRef,			s_unormInt101010IntRef			},

	// \note Same input data & int reference used for {U,S}NORM_INT_1010102_REV
	{ s_unormInt1010102RevIn,		DE_LENGTH_OF_ARRAY(s_unormInt1010102RevIn),			s_snormInt1010102RevFloatRef,		s_snormInt1010102RevIntRef,		s_snormInt1010102RevIntRef		},
	{ s_unormInt1010102RevIn,		DE_LENGTH_OF_ARRAY(s_unormInt1010102RevIn),			s_unormInt1010102RevFloatRef,		s_unormInt1010102RevIntRef,		s_unormInt1010102RevIntRef		},

	// \note UNSIGNED_BYTE_* and UNSIGNED_SHORT_* re-use input data and integer reference values from UNORM_* cases
	{ s_unormByte44In,				DE_LENGTH_OF_ARRAY(s_unormByte44In),				s_unsignedByte44FloatRef,			s_unormByte44IntRef,			s_unormByte44IntRef				},
	{ s_unormShort565In,			DE_LENGTH_OF_ARRAY(s_unormShort565In),				s_unsignedShort565FloatRef,			s_unormShort565IntRef,			s_unormShort565IntRef,			},
	{ s_unormShort4444In,			DE_LENGTH_OF_ARRAY(s_unormShort4444In),				s_unsignedShort4444FloatRef,		s_unormShort4444IntRef,			s_unormShort4444IntRef,			},
	{ s_unormShort5551In,			DE_LENGTH_OF_ARRAY(s_unormShort5551In),				s_unsignedShort5551FloatRef,		s_unormShort5551IntRef,			s_unormShort5551IntRef,			},

	// \note (UN)SIGNED_INT_1010102_REV formats use same input data
	{ s_unsignedInt1010102RevIn,	DE_LENGTH_OF_ARRAY(s_unsignedInt1010102RevIn),		s_signedInt1010102RevFloatRef,		s_signedInt1010102RevIntRef,	s_signedInt1010102RevIntRef		},
	{ s_unsignedInt1010102RevIn,	DE_LENGTH_OF_ARRAY(s_unsignedInt1010102RevIn),		s_unsignedInt1010102RevFloatRef,	s_unsignedInt1010102RevIntRef,	s_unsignedInt1010102RevIntRef	},

	{ s_unsignedInt11f11f10fRevIn,	DE_LENGTH_OF_ARRAY(s_unsignedInt11f11f10fRevIn),	s_unsignedInt11f11f10fRevFloatRef,	DE_NULL,						DE_NULL							},
	{ s_unsignedInt999E5RevIn,		DE_LENGTH_OF_ARRAY(s_unsignedInt999E5RevIn),		s_unsignedInt999E5RevFloatRef,		DE_NULL,						DE_NULL							},
	{ s_unsignedInt1688In,			DE_LENGTH_OF_ARRAY(s_unsignedInt1688In),			s_unsignedInt1688FloatRef,			DE_NULL,						s_unsignedInt1688UintRef		},
	{ s_unsignedInt248In,			DE_LENGTH_OF_ARRAY(s_unsignedInt248In),				s_unsignedInt248FloatRef,			DE_NULL,						s_unsignedInt248UintRef			},
	{ s_unsignedInt248RevIn,		DE_LENGTH_OF_ARRAY(s_unsignedInt248RevIn),			s_unsignedInt248RevFloatRef,		DE_NULL,						s_unsignedInt248RevUintRef		},
	{ s_signedInt8In,				DE_LENGTH_OF_ARRAY(s_signedInt8In),					s_signedInt8FloatRef,				s_signedInt8IntRef,				s_signedInt8UintRef				},
	{ s_signedInt16In,				DE_LENGTH_OF_ARRAY(s_signedInt16In),				s_signedInt16FloatRef,				s_signedInt16IntRef,			s_signedInt16UintRef			},
	{ s_signedInt32In,				DE_LENGTH_OF_ARRAY(s_signedInt32In),				s_signedInt32FloatRef,				s_signedInt32IntRef,			s_signedInt32UintRef			},
	{ s_unsignedInt8In,				DE_LENGTH_OF_ARRAY(s_unsignedInt8In),				s_unsignedInt8FloatRef,				s_unsignedInt8IntRef,			s_unsignedInt8UintRef			},
	{ s_unsignedInt16In,			DE_LENGTH_OF_ARRAY(s_unsignedInt16In),				s_unsignedInt16FloatRef,			s_unsignedInt16IntRef,			s_unsignedInt16UintRef			},
	{ s_unsignedInt24In,			DE_LENGTH_OF_ARRAY(s_unsignedInt24In),				s_unsignedInt24FloatRef,			s_unsignedInt24IntRef,			s_unsignedInt24UintRef			},
	{ s_unsignedInt32In,			DE_LENGTH_OF_ARRAY(s_unsignedInt32In),				s_unsignedInt32FloatRef,			s_unsignedInt32IntRef,			s_unsignedInt32UintRef			},
	{ s_halfFloatIn,				DE_LENGTH_OF_ARRAY(s_halfFloatIn),					s_halfFloatFloatRef,				s_halfFloatIntRef,				s_halfFloatUintRef				},
	{ s_floatIn,					DE_LENGTH_OF_ARRAY(s_floatIn),						s_floatFloatRef,					s_floatIntRef,					s_floatUintRef					},
	{ s_float64In,					DE_LENGTH_OF_ARRAY(s_float64In),					s_float64FloatRef,					s_float64IntRef,				s_float64IntRef					},
	{ s_floatUnsignedInt248RevIn,	DE_LENGTH_OF_ARRAY(s_floatUnsignedInt248RevIn),		s_floatUnsignedInt248RevFloatRef,	DE_NULL,						s_floatUnsignedInt248RevUintRef	},

	{ s_unormShort10In,				DE_LENGTH_OF_ARRAY(s_unormShort10In),				s_unormShort10FloatRef,				s_unormShort10IntRef,			s_unormShort10UintRef			},
	{ s_unormShort12In,				DE_LENGTH_OF_ARRAY(s_unormShort12In),				s_unormShort12FloatRef,				s_unormShort12IntRef,			s_unormShort12UintRef			},
};
DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(s_formatData) == TextureFormat::CHANNELTYPE_LAST);

TextureFormat getBaseFormat (TextureFormat format)
{
	const TextureFormat::ChannelOrder baseOrders[] = { TextureFormat::RGBA, TextureFormat::RGB, TextureFormat::DS };

	for (int baseOrderNdx = 0; baseOrderNdx < DE_LENGTH_OF_ARRAY(baseOrders); baseOrderNdx++)
	{
		const TextureFormat	curBaseFmt	(baseOrders[baseOrderNdx], format.type);
		if (isValid(curBaseFmt))
			return curBaseFmt;
	}

	return format;
}

ConstPixelBufferAccess getInputAccess (TextureFormat format)
{
	const TextureFormat	inputFormat		= getBaseFormat(format);
	const int			inputPixelSize	= getPixelSize(inputFormat);
	const int			numPixels		= s_formatData[format.type].inputSize / inputPixelSize;

	DE_ASSERT(numPixels == 4);
	DE_ASSERT(numPixels*inputPixelSize == s_formatData[format.type].inputSize);

	return ConstPixelBufferAccess(format, IVec3(numPixels, 1, 1), IVec3(inputPixelSize, 0, 0), s_formatData[format.type].input);
}

template<typename T>
const deUint32* getRawReference (TextureFormat format);

template<>
const deUint32* getRawReference<float> (TextureFormat format)
{
	return s_formatData[format.type].floatRef;
}

template<>
const deUint32* getRawReference<deInt32> (TextureFormat format)
{
	return s_formatData[format.type].intRef;
}

template<>
const deUint32* getRawReference<deUint32> (TextureFormat format)
{
	return s_formatData[format.type].uintRef;
}

template<typename T>
void getReferenceValues (TextureFormat storageFormat, TextureFormat viewFormat, vector<Vector<T, 4> >& dst)
{
	const int					numPixels	= getInputAccess(storageFormat).getWidth();
	const deUint32* const		rawValues	= getRawReference<T>(storageFormat);
	const tcu::TextureSwizzle&	swizzle		= tcu::getChannelReadSwizzle(viewFormat.order);

	dst.resize(numPixels);

	for (int pixelNdx = 0; pixelNdx < numPixels; pixelNdx++)
	{
		const deUint32*	srcPixPtr	= rawValues + pixelNdx*4;
		T*				dstPixPtr	= (T*)&dst[pixelNdx];

		for (int c = 0; c < 4; c++)
		{
			switch (swizzle.components[c])
			{
				case tcu::TextureSwizzle::CHANNEL_0:
				case tcu::TextureSwizzle::CHANNEL_1:
				case tcu::TextureSwizzle::CHANNEL_2:
				case tcu::TextureSwizzle::CHANNEL_3:
					deMemcpy(dstPixPtr + c, srcPixPtr + (int)swizzle.components[c], sizeof(deUint32));
					break;

				case tcu::TextureSwizzle::CHANNEL_ZERO:
					dstPixPtr[c] = T(0);
					break;

				case tcu::TextureSwizzle::CHANNEL_ONE:
					dstPixPtr[c] = T(1);
					break;

				default:
					DE_FATAL("Unknown swizzle");
			}
		}
	}
}

template<typename T>
bool componentEqual (T a, T b)
{
	return a == b;
}

template<>
bool componentEqual<float> (float a, float b)
{
	return (a == b) || (deFloatIsNaN(a) && deFloatIsNaN(b));
}

template<typename T, int Size>
bool allComponentsEqual (const Vector<T, Size>& a, const Vector<T, Size>& b)
{
	for (int ndx = 0; ndx < Size; ndx++)
	{
		if (!componentEqual(a[ndx], b[ndx]))
			return false;
	}

	return true;
}

template<typename T>
void copyPixels (const ConstPixelBufferAccess& src, const PixelBufferAccess& dst)
{
	for (int ndx = 0; ndx < src.getWidth(); ndx++)
		dst.setPixel(src.getPixelT<T>(ndx, 0, 0), ndx, 0, 0);
}

void copyGetSetDepth (const ConstPixelBufferAccess& src, const PixelBufferAccess& dst)
{
	for (int ndx = 0; ndx < src.getWidth(); ndx++)
		dst.setPixDepth(src.getPixDepth(ndx, 0, 0), ndx, 0, 0);
}

void copyGetSetStencil (const ConstPixelBufferAccess& src, const PixelBufferAccess& dst)
{
	for (int ndx = 0; ndx < src.getWidth(); ndx++)
		dst.setPixStencil(src.getPixStencil(ndx, 0, 0), ndx, 0, 0);
}

void copyPixels (const ConstPixelBufferAccess& src, const PixelBufferAccess& dst)
{
	switch (getTextureChannelClass(dst.getFormat().type))
	{
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			copyPixels<float>(src, dst);
			break;

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			copyPixels<deInt32>(src, dst);
			break;

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			copyPixels<deUint32>(src, dst);
			break;

		default:
			DE_FATAL("Unknown channel class");
	}
}

const char* getTextureAccessTypeDescription (TextureAccessType type)
{
	static const char* s_desc[] =
	{
		"floating-point",
		"signed integer",
		"unsigned integer"
	};
	return de::getSizedArrayElement<tcu::TEXTUREACCESSTYPE_LAST>(s_desc, type);
}

template<typename T>
TextureAccessType getTextureAccessType (void);

template<>
TextureAccessType getTextureAccessType<float> (void)
{
	return tcu::TEXTUREACCESSTYPE_FLOAT;
}

template<>
TextureAccessType getTextureAccessType<deInt32> (void)
{
	return tcu::TEXTUREACCESSTYPE_SIGNED_INT;
}

template<>
TextureAccessType getTextureAccessType<deUint32> (void)
{
	return tcu::TEXTUREACCESSTYPE_UNSIGNED_INT;
}

static std::string getCaseName (TextureFormat format)
{
	std::ostringstream str;

	str << format.type << "_" << format.order;

	return de::toLower(str.str());
}

class TextureFormatCase : public tcu::TestCase
{
public:
	TextureFormatCase (tcu::TestContext& testCtx, TextureFormat format)
		: tcu::TestCase	(testCtx, getCaseName(format).c_str(), "")
		, m_format		(format)
	{
		DE_ASSERT(isValid(format));
	}

protected:
	template<typename T>
	void verifyRead (const ConstPixelBufferAccess& src)
	{
		const int				numPixels	= src.getWidth();
		vector<Vector<T, 4> >	res			(numPixels);
		vector<Vector<T, 4> >	ref;

		m_testCtx.getLog()
			<< TestLog::Message << "Verifying " << getTextureAccessTypeDescription(getTextureAccessType<T>()) << " access" << TestLog::EndMessage;

		for (int ndx = 0; ndx < numPixels; ndx++)
			res[ndx] = src.getPixelT<T>(ndx, 0, 0);

		// \note m_format != src.getFormat() for DS formats, and we specifically need to
		//		 use the combined format as storage format to get right reference values.
		getReferenceValues<T>(m_format, src.getFormat(), ref);

		for (int pixelNdx = 0; pixelNdx < numPixels; pixelNdx++)
		{
			if (!allComponentsEqual(res[pixelNdx], ref[pixelNdx]))
			{
				m_testCtx.getLog()
					<< TestLog::Message << "ERROR: at pixel " << pixelNdx << ": expected " << ref[pixelNdx] << ", got " << res[pixelNdx] << TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Comparison failed");
			}
		}
	}

	void verifyRead (const ConstPixelBufferAccess& src)
	{
		// \todo [2016-08-04 pyry] Overflow in float->int conversion is not defined and
		//						   produces different results depending on arch.
		const bool	isFloat32Or64	= src.getFormat().type == tcu::TextureFormat::FLOAT ||
									  src.getFormat().type == tcu::TextureFormat::FLOAT64;

		if (isAccessValid(src.getFormat(), tcu::TEXTUREACCESSTYPE_FLOAT))
			verifyRead<float>(src);

		if (isAccessValid(src.getFormat(), tcu::TEXTUREACCESSTYPE_UNSIGNED_INT) && !isFloat32Or64)
			verifyRead<deUint32>(src);

		if (isAccessValid(src.getFormat(), tcu::TEXTUREACCESSTYPE_SIGNED_INT) && !isFloat32Or64)
			verifyRead<deInt32>(src);
	}

	void verifyGetPixDepth (const ConstPixelBufferAccess& refAccess, const ConstPixelBufferAccess& combinedAccess)
	{
		m_testCtx.getLog()
			<< TestLog::Message << "Verifying getPixDepth()" << TestLog::EndMessage;

		for (int pixelNdx = 0; pixelNdx < refAccess.getWidth(); pixelNdx++)
		{
			const float		ref		= refAccess.getPixel(pixelNdx, 0, 0).x();
			const float		res		= combinedAccess.getPixDepth(pixelNdx, 0, 0);

			if (!componentEqual(res, ref))
			{
				m_testCtx.getLog()
					<< TestLog::Message << "ERROR: at pixel " << pixelNdx << ": expected " << ref << ", got " << res << TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Comparison failed");
			}
		}
	}

	void verifyGetPixStencil (const ConstPixelBufferAccess& refAccess, const ConstPixelBufferAccess& combinedAccess)
	{
		m_testCtx.getLog()
			<< TestLog::Message << "Verifying getPixStencil()" << TestLog::EndMessage;

		for (int pixelNdx = 0; pixelNdx < refAccess.getWidth(); pixelNdx++)
		{
			const int	ref		= refAccess.getPixelInt(pixelNdx, 0, 0).x();
			const int	res		= combinedAccess.getPixStencil(pixelNdx, 0, 0);

			if (!componentEqual(res, ref))
			{
				m_testCtx.getLog()
					<< TestLog::Message << "ERROR: at pixel " << pixelNdx << ": expected " << ref << ", got " << res << TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Comparison failed");
			}
		}
	}

	void verifyInfoQueries (void)
	{
		const tcu::TextureChannelClass	chnClass	= tcu::getTextureChannelClass(m_format.type);
		const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(m_format);

		if (tcu::isCombinedDepthStencilType(m_format.type))
			TCU_CHECK(chnClass == tcu::TEXTURECHANNELCLASS_LAST);
		else
			TCU_CHECK(de::inBounds(chnClass, (tcu::TextureChannelClass)0, tcu::TEXTURECHANNELCLASS_LAST));

		DE_UNREF(fmtInfo);
	}

	const TextureFormat		m_format;
};

class ColorFormatCase : public TextureFormatCase
{
public:
	ColorFormatCase (tcu::TestContext& testCtx, TextureFormat format)
		: TextureFormatCase(testCtx, format)
	{
		DE_ASSERT(format.order != TextureFormat::D &&
				  format.order != TextureFormat::S &&
				  format.order != TextureFormat::DS);
	}

	IterateResult iterate (void)
	{
		const ConstPixelBufferAccess	inputAccess		= getInputAccess(m_format);
		vector<deUint8>					tmpMem			(getPixelSize(inputAccess.getFormat())*inputAccess.getWidth());
		const PixelBufferAccess			tmpAccess		(inputAccess.getFormat(), inputAccess.getWidth(), 1, 1, &tmpMem[0]);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		verifyInfoQueries();

		verifyRead(inputAccess);

		// \todo [2015-10-12 pyry] Handle lossy conversion with *NORM_INT32
		if (m_format.type != TextureFormat::UNORM_INT32 && m_format.type != TextureFormat::SNORM_INT32)
		{
			m_testCtx.getLog() << TestLog::Message << "Copying with getPixel() -> setPixel()" << TestLog::EndMessage;
			copyPixels(inputAccess, tmpAccess);
			verifyRead(tmpAccess);
		}

		return STOP;
	}
};

class DepthFormatCase : public TextureFormatCase
{
public:
	DepthFormatCase (tcu::TestContext& testCtx, TextureFormat format)
		: TextureFormatCase(testCtx, format)
	{
		DE_ASSERT(format.order == TextureFormat::D);
	}

	IterateResult iterate (void)
	{
		const ConstPixelBufferAccess	inputAccess			= getInputAccess(m_format);
		vector<deUint8>					tmpMem				(getPixelSize(inputAccess.getFormat())*inputAccess.getWidth());
		const PixelBufferAccess			tmpAccess			(inputAccess.getFormat(), inputAccess.getWidth(), 1, 1, &tmpMem[0]);
		const ConstPixelBufferAccess	inputDepthAccess	= getEffectiveDepthStencilAccess(inputAccess, tcu::Sampler::MODE_DEPTH);
		const PixelBufferAccess			tmpDepthAccess		= getEffectiveDepthStencilAccess(tmpAccess, tcu::Sampler::MODE_DEPTH);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		verifyInfoQueries();

		verifyRead(inputDepthAccess);

		m_testCtx.getLog() << TestLog::Message << "Copying with getPixel() -> setPixel()" << TestLog::EndMessage;
		copyPixels(inputDepthAccess, tmpDepthAccess);
		verifyRead(tmpDepthAccess);

		verifyGetPixDepth(inputDepthAccess, inputAccess);

		m_testCtx.getLog() << TestLog::Message << "Copying both depth getPixDepth() -> setPixDepth()" << TestLog::EndMessage;
		tcu::clear(tmpDepthAccess, tcu::Vec4(0.0f));
		copyGetSetDepth(inputAccess, tmpAccess);
		verifyGetPixDepth(tmpDepthAccess, tmpAccess);

		return STOP;
	}
};

class StencilFormatCase : public TextureFormatCase
{
public:
	StencilFormatCase (tcu::TestContext& testCtx, TextureFormat format)
		: TextureFormatCase(testCtx, format)
	{
		DE_ASSERT(format.order == TextureFormat::S);
	}

	IterateResult iterate (void)
	{
		const ConstPixelBufferAccess	inputAccess			= getInputAccess(m_format);
		vector<deUint8>					tmpMem				(getPixelSize(inputAccess.getFormat())*inputAccess.getWidth());
		const PixelBufferAccess			tmpAccess			(inputAccess.getFormat(), inputAccess.getWidth(), 1, 1, &tmpMem[0]);
		const ConstPixelBufferAccess	inputStencilAccess	= getEffectiveDepthStencilAccess(inputAccess, tcu::Sampler::MODE_STENCIL);
		const PixelBufferAccess			tmpStencilAccess	= getEffectiveDepthStencilAccess(tmpAccess, tcu::Sampler::MODE_STENCIL);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		verifyInfoQueries();

		verifyRead(inputStencilAccess);

		m_testCtx.getLog() << TestLog::Message << "Copying with getPixel() -> setPixel()" << TestLog::EndMessage;
		copyPixels(inputStencilAccess, tmpStencilAccess);
		verifyRead(tmpStencilAccess);

		verifyGetPixStencil(inputStencilAccess, inputAccess);

		m_testCtx.getLog() << TestLog::Message << "Copying both depth getPixStencil() -> setPixStencil()" << TestLog::EndMessage;
		tcu::clear(tmpStencilAccess, tcu::IVec4(0));
		copyGetSetStencil(inputAccess, tmpAccess);
		verifyGetPixStencil(tmpStencilAccess, tmpAccess);

		return STOP;
	}
};

class DepthStencilFormatCase : public TextureFormatCase
{
public:
	DepthStencilFormatCase (tcu::TestContext& testCtx, TextureFormat format)
		: TextureFormatCase(testCtx, format)
	{
		DE_ASSERT(format.order == TextureFormat::DS);
	}

	IterateResult iterate (void)
	{
		const ConstPixelBufferAccess	inputAccess			= getInputAccess(m_format);
		vector<deUint8>					tmpMem				(getPixelSize(inputAccess.getFormat())*inputAccess.getWidth());
		const PixelBufferAccess			tmpAccess			(inputAccess.getFormat(), inputAccess.getWidth(), 1, 1, &tmpMem[0]);
		const ConstPixelBufferAccess	inputDepthAccess	= getEffectiveDepthStencilAccess(inputAccess, tcu::Sampler::MODE_DEPTH);
		const ConstPixelBufferAccess	inputStencilAccess	= getEffectiveDepthStencilAccess(inputAccess, tcu::Sampler::MODE_STENCIL);
		const PixelBufferAccess			tmpDepthAccess		= getEffectiveDepthStencilAccess(tmpAccess, tcu::Sampler::MODE_DEPTH);
		const PixelBufferAccess			tmpStencilAccess	= getEffectiveDepthStencilAccess(tmpAccess, tcu::Sampler::MODE_STENCIL);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		verifyInfoQueries();

		verifyRead(inputDepthAccess);
		verifyRead(inputStencilAccess);

		verifyGetPixDepth(inputDepthAccess, inputAccess);
		verifyGetPixStencil(inputStencilAccess, inputAccess);

		m_testCtx.getLog() << TestLog::Message << "Copying both depth and stencil with getPixel() -> setPixel()" << TestLog::EndMessage;
		copyPixels(inputDepthAccess, tmpDepthAccess);
		copyPixels(inputStencilAccess, tmpStencilAccess);
		verifyRead(tmpDepthAccess);
		verifyRead(tmpStencilAccess);

		m_testCtx.getLog() << TestLog::Message << "Copying both depth and stencil with getPix{Depth,Stencil}() -> setPix{Depth,Stencil}()" << TestLog::EndMessage;
		copyGetSetDepth(inputDepthAccess, tmpDepthAccess);
		copyGetSetStencil(inputStencilAccess, tmpStencilAccess);
		verifyRead(tmpDepthAccess);
		verifyRead(tmpStencilAccess);

		m_testCtx.getLog() << TestLog::Message << "Verifying that clearing depth component with clearDepth() doesn't affect stencil" << TestLog::EndMessage;
		tcu::copy(tmpAccess, inputAccess);
		tcu::clearDepth(tmpAccess, 0.0f);
		verifyRead(tmpStencilAccess);

		m_testCtx.getLog() << TestLog::Message << "Verifying that clearing stencil component with clearStencil() doesn't affect depth" << TestLog::EndMessage;
		tcu::copy(tmpAccess, inputAccess);
		tcu::clearStencil(tmpAccess, 0);
		verifyRead(tmpDepthAccess);

		m_testCtx.getLog() << TestLog::Message << "Verifying that clearing depth component with setPixDepth() doesn't affect stencil" << TestLog::EndMessage;
		tcu::copy(tmpAccess, inputAccess);

		for (int ndx = 0; ndx < tmpAccess.getWidth(); ndx++)
			tmpAccess.setPixDepth(0.0f, ndx, 0, 0);

		verifyRead(tmpStencilAccess);

		m_testCtx.getLog() << TestLog::Message << "Verifying that clearing stencil component with setPixStencil() doesn't affect depth" << TestLog::EndMessage;
		tcu::copy(tmpAccess, inputAccess);

		for (int ndx = 0; ndx < tmpAccess.getWidth(); ndx++)
			tmpAccess.setPixStencil(0, ndx, 0, 0);

		verifyRead(tmpDepthAccess);

		return STOP;
	}
};

} // anonymous

tcu::TestCaseGroup* createTextureFormatTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group	(new tcu::TestCaseGroup(testCtx, "texture_format", "Texture format tests"));

	for (int channelType = 0; channelType < TextureFormat::CHANNELTYPE_LAST; channelType++)
	{
		for (int channelOrder = 0; channelOrder < TextureFormat::CHANNELORDER_LAST; channelOrder++)
		{
			const TextureFormat		format		((TextureFormat::ChannelOrder)channelOrder, (TextureFormat::ChannelType)channelType);

			if (!isValid(format))
				continue;

			if (tcu::isSRGB(format))
				continue; // \todo [2015-10-12 pyry] Tests for sRGB formats (need thresholds)

			if (format.order == TextureFormat::DS)
				group->addChild(new DepthStencilFormatCase(testCtx, format));
			else if (format.order == TextureFormat::D)
				group->addChild(new DepthFormatCase(testCtx, format));
			else if (format.order == TextureFormat::S)
				group->addChild(new StencilFormatCase(testCtx, format));
			else
				group->addChild(new ColorFormatCase(testCtx, format));
		}
	}

	return group.release();
}

} // dit
