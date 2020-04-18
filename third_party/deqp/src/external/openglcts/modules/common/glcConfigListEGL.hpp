#ifndef _GLCCONFIGLISTEGL_HPP
#define _GLCCONFIGLISTEGL_HPP
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
 * \brief CTS rendering configuration list utility.
 */ /*-------------------------------------------------------------------*/

#include "glcConfigList.hpp"
#include "tcuDefs.hpp"

namespace glcts
{

void getConfigListEGL(tcu::Platform& platform, glu::ApiType type, ConfigList& configList);

} // glcts

#endif // _GLCCONFIGLISTEGL_HPP
