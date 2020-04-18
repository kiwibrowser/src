#ifndef _GLCAOSPMUSTPASSES_HPP
#define _GLCAOSPMUSTPASSES_HPP
/*     Copyright (C) 2016-2017 The Khronos Group Inc
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
*/

/* WARNING: This is auto-generated file. Do not modify, since changes will
 * be lost! Modify the generating script instead.
 */

const char* mustpassDir = "gl_cts/data/mustpass/gles/aosp_mustpass/3.2.4.x/";

static const RunParams aosp_mustpass_es_first_cfg[] = {
	{ glu::ApiType::es(2, 0), "master", "rgba8888d24s8ms0", "unspecified", -1, DE_NULL, 256, 256 },
	{ glu::ApiType::es(3, 0), "master", "rgba8888d24s8ms0", "unspecified", -1, DE_NULL, 256, 256 },
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 0), "rotate-portrait", "rgba8888d24s8ms0", "0", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 0), "rotate-landscape", "rgba8888d24s8ms0", "90", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 0), "rotate-reverse-portrait", "rgba8888d24s8ms0", "180", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 0), "rotate-reverse-landscape", "rgba8888d24s8ms0", "270", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 0), "multisample", "rgba8888d24s8ms4", "unspecified", -1, DE_NULL, 256, 256 },
	{ glu::ApiType::es(3, 0), "565-no-depth-no-stencil", "rgb565d0s0ms0", "unspecified", -1, DE_NULL, 256, 256 },
	{ glu::ApiType::es(3, 1), "master", "rgba8888d24s8ms0", "unspecified", -1, DE_NULL, 256, 256 },
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 1), "rotate-portrait", "rgba8888d24s8ms0", "0", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 1), "rotate-landscape", "rgba8888d24s8ms0", "90", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 1), "rotate-reverse-portrait", "rgba8888d24s8ms0", "180", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
#if DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 1), "rotate-reverse-landscape", "rgba8888d24s8ms0", "270", -1, DE_NULL, 256, 256 },
#endif // DE_OS == DE_OS_ANDROID
	{ glu::ApiType::es(3, 1), "multisample", "rgba8888d24s8ms4", "unspecified", -1, DE_NULL, 256, 256 },
	{ glu::ApiType::es(3, 1), "565-no-depth-no-stencil", "rgb565d0s0ms0", "unspecified", -1, DE_NULL, 256, 256 },
};

#endif // _GLCAOSPMUSTPASSES_HPP
