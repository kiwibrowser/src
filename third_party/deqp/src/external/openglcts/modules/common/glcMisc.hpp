#ifndef _GLCMISC_HPP
#define _GLCMISC_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file glcMisc.hpp
 * \brief Miscellaneous helper functions.
 */ /*-------------------------------------------------------------------*/

#include "glwDefs.hpp"

namespace glcts
{

glw::GLhalf floatToHalfFloat(float f);
glw::GLuint floatToUnisgnedF11(float f);
glw::GLuint floatToUnisgnedF10(float f);

float halfFloatToFloat(glw::GLhalf hf);
float unsignedF11ToFloat(glw::GLuint value);
float unsignedF10ToFloat(glw::GLuint value);

} // glcts

#endif // _GLCMISC_HPP
