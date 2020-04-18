#ifndef _GLCWAIVER_HPP
#define _GLCWAIVER_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2017 The Khronos Group Inc.
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
 * \brief
 */ /*-------------------------------------------------------------------*/


/* Bug 13788 - Waiver request for bug related to
   rendering last 2 columns of pixels in 16K wide textures / render buffers

   This define disables testing of rendering of primitives that fit 2x2 of
   the lower left corner of the framebuffer, if framebuffer's width
   is greater than 16383.

   Disabled by default
*/
// #define WAIVER_WITH_BUG_13788

#endif // _GLCWAIVER_HPP
