#ifndef _GLCKHRONOSMUSTPASSES_HPP
#define _GLCKHRONOSMUSTPASSES_HPP
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

const char* mustpassDir = "gl_cts/data/mustpass/gles/khronos_mustpass/3.2.4.x/";

static const RunParams khronos_mustpass_es_first_cfg[] = {
	{ glu::ApiType::es(2, 0), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 0), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 1), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 2), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 2), "khr-master", DE_NULL, "unspecified", 2, DE_NULL, 113, 47 },
	{ glu::ApiType::es(3, 2), "khr-master", DE_NULL, "unspecified", 3, "rgba8888d24s8", 64, -1 },
	{ glu::ApiType::es(3, 2), "khr-master", DE_NULL, "unspecified", 3, "rgba8888d24s8", -1, 64 },
};

static const RunParams khronos_mustpass_es_other_cfg[] = {
	{ glu::ApiType::es(2, 0), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 0), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 1), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 2), "khr-master", DE_NULL, "unspecified", 1, DE_NULL, 64, 64 },
	{ glu::ApiType::es(3, 2), "khr-master", DE_NULL, "unspecified", 2, DE_NULL, 113, 47 },
};

#endif // _GLCKHRONOSMUSTPASSES_HPP
