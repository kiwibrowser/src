/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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

/*!
 * \file  glcTextureBufferTests.cpp
 * \brief Base test group for texture buffer tests
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferTests.hpp"
#include "esextcTextureBufferActiveUniformValidation.hpp"
#include "esextcTextureBufferAtomicFunctions.hpp"
#include "esextcTextureBufferBufferParameters.hpp"
#include "esextcTextureBufferErrors.hpp"
#include "esextcTextureBufferMAXSizeValidation.hpp"
#include "esextcTextureBufferOperations.hpp"
#include "esextcTextureBufferParamValueIntToFloatConversion.hpp"
#include "esextcTextureBufferParameters.hpp"
#include "esextcTextureBufferPrecision.hpp"
#include "esextcTextureBufferTextureBufferRange.hpp"

namespace glcts
{

/** Constructor
 *
 * @param context       Test context
 * @param glslVersion   GLSL version
 **/
TextureBufferTests::TextureBufferTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "texture_buffer", "Texture Buffer Tests")
{
	/* No implementation needed */
}

/** Initializes test cases for texture buffer tests
 **/
void TextureBufferTests::init(void)
{
	/* Initialize base class */
	TestCaseGroupBase::init();

	/* Texture Buffer Operations (Test 1) */

	/* Case 1 - via buffer object loads*/
	addChild(new TextureBufferOperationsViaBufferObjectLoad(m_context, m_extParams,
															"texture_buffer_operations_buffer_load", "Test 1.1"));
	/* Case 2 - via direct CPU writes*/
	addChild(new TextureBufferOperationsViaCPUWrites(m_context, m_extParams, "texture_buffer_operations_cpu_writes",
													 "Test 1.2"));
	/* Case 3 - via framebuffer readbacks to pixel buffer objects*/
	addChild(new TextureBufferOperationsViaFrambufferReadBack(
		m_context, m_extParams, "texture_buffer_operations_framebuffer_readback", "Test 1.3"));
	/* Case 4 - via transform feedback*/
	addChild(new TextureBufferOperationsViaTransformFeedback(
		m_context, m_extParams, "texture_buffer_operations_transform_feedback", "Test 1.4"));
	/* Case 5 - via image store*/
	addChild(new TextureBufferOperationsViaImageStore(m_context, m_extParams, "texture_buffer_operations_image_store",
													  "Test 1.5"));
	/* Case 6 - via ssbo writes*/
	addChild(new TextureBufferOperationsViaSSBOWrites(m_context, m_extParams, "texture_buffer_operations_ssbo_writes",
													  "Test 1.6"));

	/* Texture Buffer Max Size (Test 2)*/
	addChild(new TextureBufferMAXSizeValidation(m_context, m_extParams, "texture_buffer_max_size", "Test 2"));

	/* Texture Buffer Range (Test 3)*/
	addChild(
		new TextureBufferTextureBufferRange(m_context, m_extParams, "texture_buffer_texture_buffer_range", "Test 3"));

	/* Texture Buffer - Parameter Value from Integer To Float Conversion (Test 4)*/
	addChild(new TextureBufferParamValueIntToFloatConversion(m_context, m_extParams, "texture_buffer_conv_int_to_float",
															 "Test 4"));

	/* Texture Buffer Atomic Functions (Test 5) */
	addChild(new TextureBufferAtomicFunctions(m_context, m_extParams, "texture_buffer_atomic_functions", "Test 5"));

	/* Texture Buffer Parameters (Test 6) */
	addChild(new TextureBufferParameters(m_context, m_extParams, "texture_buffer_parameters", "Test 6"));

	/* Texture Buffer Errors (Test 7) */
	addChild(new TextureBufferErrors(m_context, m_extParams, "texture_buffer_errors", "Test 7"));

	/* Texture Buffer - Active Uniform Information Validation (Test 8)*/
	/* Vertex/Fragment Shader  */
	addChild(new TextureBufferActiveUniformValidationVSFS(
		m_context, m_extParams, "texture_buffer_active_uniform_validation_fragment_shader", "Test 8.1"));
	/* Compute Shader */
	addChild(new TextureBufferActiveUniformValidationCS(
		m_context, m_extParams, "texture_buffer_active_uniform_validation_compute_shader", "Test 8.2"));

	/* Texture Buffer Buffer Parameters (Test 9) */
	addChild(new TextureBufferBufferParameters(m_context, m_extParams, "texture_buffer_buffer_parameters", "Test 9"));

	/* Texture Buffer Precision (Test 10) */
	addChild(new TextureBufferPrecision(m_context, m_extParams, "texture_buffer_precision", "Test 10"));
}

} // namespace glcts
