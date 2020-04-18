/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/**
 */ /*!
 * \file  gl4cDirectStateAccessTest.cpp
 * \brief Conformance tests for the Direct State Access feature functionality.
 */ /*-----------------------------------------------------------------------------*/

/* Includes. */
#include "gl4cDirectStateAccessTests.hpp"

#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

namespace gl4cts
{
namespace DirectStateAccess
{

/** @brief Direct State Access Tests Group constructor.
 *
 *  @param [in] context     OpenGL context.
 */
Tests::Tests(deqp::Context& context) : TestCaseGroup(context, "direct_state_access", "Direct State Access Tests Suite")
{
}

/** @brief Direct State Access Tests initializer. */
void Tests::init()
{
	/* Direct State Access of Transform Feedback Objects */
	addChild(new TransformFeedback::CreationTest(m_context));
	addChild(new TransformFeedback::DefaultsTest(m_context));
	addChild(new TransformFeedback::BuffersTest(m_context));
	addChild(new TransformFeedback::ErrorsTest(m_context));
	addChild(new TransformFeedback::FunctionalTest(m_context));

	/* Direct State Access of Sampler Objects */
	addChild(new Samplers::CreationTest(m_context));
	addChild(new Samplers::DefaultsTest(m_context));
	addChild(new Samplers::ErrorsTest(m_context));
	addChild(new Samplers::FunctionalTest(m_context));

	/* Direct State Access of Program Pipeline Objects */
	addChild(new ProgramPipelines::CreationTest(m_context));
	addChild(new ProgramPipelines::DefaultsTest(m_context));
	addChild(new ProgramPipelines::ErrorsTest(m_context));
	addChild(new ProgramPipelines::FunctionalTest(m_context));

	/* Direct State Access of Query Objects */
	addChild(new Queries::CreationTest(m_context));
	addChild(new Queries::DefaultsTest(m_context));
	addChild(new Queries::ErrorsTest(m_context));
	addChild(new Queries::FunctionalTest(m_context));

	/* Direct State Access of Buffer Objects */
	addChild(new Buffers::CreationTest(m_context));
	addChild(new Buffers::DataTest(m_context));
	addChild(new Buffers::ClearTest(m_context));
	addChild(new Buffers::MapReadOnlyTest(m_context));
	addChild(new Buffers::MapReadWriteTest(m_context));
	addChild(new Buffers::MapWriteOnlyTest(m_context));
	addChild(new Buffers::MapRangeReadBitTest(m_context));
	addChild(new Buffers::MapRangeWriteBitTest(m_context));
	addChild(new Buffers::SubDataQueryTest(m_context));
	addChild(new Buffers::DefaultsTest(m_context));
	addChild(new Buffers::ErrorsTest(m_context));
	addChild(new Buffers::FunctionalTest(m_context));

	/* Direct State Access of Framebuffer Objects */
	addChild(new Framebuffers::CreationTest(m_context));
	addChild(new Framebuffers::RenderbufferAttachmentTest(m_context));
	addChild(new Framebuffers::TextureAttachmentTest(m_context));
	addChild(new Framebuffers::TextureLayerAttachmentTest(m_context));
	addChild(new Framebuffers::DrawReadBufferTest(m_context));
	addChild(new Framebuffers::DrawBuffersTest(m_context));
	addChild(new Framebuffers::InvalidateDataTest(m_context));
	addChild(new Framebuffers::InvalidateSubDataTest(m_context));
	addChild(new Framebuffers::ClearTest(m_context));
	addChild(new Framebuffers::BlitTest(m_context));
	addChild(new Framebuffers::CheckStatusTest(m_context));
	addChild(new Framebuffers::GetParametersTest(m_context));
	addChild(new Framebuffers::GetAttachmentParametersTest(m_context));
	addChild(new Framebuffers::CreationErrorsTest(m_context));
	addChild(new Framebuffers::RenderbufferAttachmentErrorsTest(m_context));
	addChild(new Framebuffers::TextureAttachmentErrorsTest(m_context));
	addChild(new Framebuffers::DrawReadBuffersErrorsTest(m_context));
	addChild(new Framebuffers::InvalidateDataAndSubDataErrorsTest(m_context));
	addChild(new Framebuffers::ClearNamedFramebufferErrorsTest(m_context));
	addChild(new Framebuffers::CheckStatusErrorsTest(m_context));
	addChild(new Framebuffers::GetParameterErrorsTest(m_context));
	addChild(new Framebuffers::GetAttachmentParameterErrorsTest(m_context));
	addChild(new Framebuffers::FunctionalTest(m_context));

	/* Direct State Access of Renderbuffer Objects */
	addChild(new Renderbuffers::CreationTest(m_context));
	addChild(new Renderbuffers::StorageTest(m_context));
	addChild(new Renderbuffers::StorageMultisampleTest(m_context));
	addChild(new Renderbuffers::GetParametersTest(m_context));
	addChild(new Renderbuffers::CreationErrorsTest(m_context));
	addChild(new Renderbuffers::StorageErrorsTest(m_context));
	addChild(new Renderbuffers::StorageMultisampleErrorsTest(m_context));
	addChild(new Renderbuffers::GetParameterErrorsTest(m_context));

	/* Direct State Access of Vertex Array Objects */
	addChild(new VertexArrays::CreationTest(m_context));
	addChild(new VertexArrays::EnableDisableAttributesTest(m_context));
	addChild(new VertexArrays::ElementBufferTest(m_context));
	addChild(new VertexArrays::VertexBuffersTest(m_context));
	addChild(new VertexArrays::AttributeFormatTest(m_context));
	addChild(new VertexArrays::AttributeBindingTest(m_context));
	addChild(new VertexArrays::AttributeBindingDivisorTest(m_context));
	addChild(new VertexArrays::GetVertexArrayTest(m_context));
	addChild(new VertexArrays::GetVertexArrayIndexedTest(m_context));
	addChild(new VertexArrays::DefaultsTest(m_context));
	addChild(new VertexArrays::CreationErrorTest(m_context));
	addChild(new VertexArrays::EnableDisableAttributeErrorsTest(m_context));
	addChild(new VertexArrays::ElementBufferErrorsTest(m_context));
	addChild(new VertexArrays::VertexBuffersErrorsTest(m_context));
	addChild(new VertexArrays::AttributeFormatErrorsTest(m_context));
	addChild(new VertexArrays::AttributeBindingErrorsTest(m_context));
	addChild(new VertexArrays::AttributeBindingDivisorErrorsTest(m_context));
	addChild(new VertexArrays::GetVertexArrayErrorsTest(m_context));
	addChild(new VertexArrays::GetVertexArrayIndexedErrorsTest(m_context));

	/* Direct State Access of Tetxure Objects */
	addChild(new Textures::CreationTest(m_context));

	addChild(new Textures::BufferTest<glw::GLbyte, 1, false>(m_context, "textures_buffer_r8i"));
	addChild(new Textures::BufferTest<glw::GLbyte, 2, false>(m_context, "textures_buffer_rg8i"));
	addChild(new Textures::BufferTest<glw::GLbyte, 4, false>(m_context, "textures_buffer_rgba8i"));

	addChild(new Textures::BufferTest<glw::GLubyte, 1, false>(m_context, "textures_buffer_r8ui"));
	addChild(new Textures::BufferTest<glw::GLubyte, 2, false>(m_context, "textures_buffer_rg8ui"));
	addChild(new Textures::BufferTest<glw::GLubyte, 4, false>(m_context, "textures_buffer_rgba8ui"));
	addChild(new Textures::BufferTest<glw::GLubyte, 1, true>(m_context, "textures_buffer_r8ui_unorm"));
	addChild(new Textures::BufferTest<glw::GLubyte, 2, true>(m_context, "textures_buffer_rg8ui_unorm"));
	addChild(new Textures::BufferTest<glw::GLubyte, 4, true>(m_context, "textures_buffer_rgba8ui_unorm"));

	addChild(new Textures::BufferTest<glw::GLshort, 1, false>(m_context, "textures_buffer_r16i"));
	addChild(new Textures::BufferTest<glw::GLshort, 2, false>(m_context, "textures_buffer_rg16i"));
	addChild(new Textures::BufferTest<glw::GLshort, 4, false>(m_context, "textures_buffer_rgba16i"));

	addChild(new Textures::BufferTest<glw::GLushort, 1, false>(m_context, "textures_buffer_r16ui"));
	addChild(new Textures::BufferTest<glw::GLushort, 2, false>(m_context, "textures_buffer_rg16ui"));
	addChild(new Textures::BufferTest<glw::GLushort, 4, false>(m_context, "textures_buffer_rgba16ui"));
	addChild(new Textures::BufferTest<glw::GLushort, 1, true>(m_context, "textures_buffer_r16ui_unorm"));
	addChild(new Textures::BufferTest<glw::GLushort, 2, true>(m_context, "textures_buffer_rg16ui_unorm"));
	addChild(new Textures::BufferTest<glw::GLushort, 4, true>(m_context, "textures_buffer_rgba16ui_unorm"));

	addChild(new Textures::BufferTest<glw::GLint, 1, false>(m_context, "textures_buffer_r32i"));
	addChild(new Textures::BufferTest<glw::GLint, 2, false>(m_context, "textures_buffer_rg32i"));
	addChild(new Textures::BufferTest<glw::GLint, 3, false>(m_context, "textures_buffer_rgb32i"));
	addChild(new Textures::BufferTest<glw::GLint, 4, false>(m_context, "textures_buffer_rgba32i"));

	addChild(new Textures::BufferTest<glw::GLuint, 1, false>(m_context, "textures_buffer_r32ui"));
	addChild(new Textures::BufferTest<glw::GLuint, 2, false>(m_context, "textures_buffer_rg32ui"));
	addChild(new Textures::BufferTest<glw::GLuint, 3, false>(m_context, "textures_buffer_rgb32ui"));
	addChild(new Textures::BufferTest<glw::GLuint, 4, false>(m_context, "textures_buffer_rgba32ui"));

	addChild(new Textures::BufferTest<glw::GLfloat, 1, true>(m_context, "textures_buffer_r32f"));
	addChild(new Textures::BufferTest<glw::GLfloat, 2, true>(m_context, "textures_buffer_rg32f"));
	addChild(new Textures::BufferTest<glw::GLfloat, 3, true>(m_context, "textures_buffer_rgb32f"));
	addChild(new Textures::BufferTest<glw::GLfloat, 4, true>(m_context, "textures_buffer_rgba32f"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 1, false, 1, false>(m_context, "textures_storage_1d_r8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 2, false, 1, false>(m_context, "textures_storage_1d_rg8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 4, false, 1, false>(m_context, "textures_storage_1d_rgba8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 1, false, 2, false>(m_context, "textures_storage_2d_r8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 2, false, 2, false>(m_context, "textures_storage_2d_rg8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 4, false, 2, false>(m_context, "textures_storage_2d_rgba8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 1, false, 3, false>(m_context, "textures_storage_3d_r8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 2, false, 3, false>(m_context, "textures_storage_3d_rg8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 4, false, 3, false>(m_context, "textures_storage_3d_rgba8i"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, false, 1, false>(m_context, "textures_storage_1d_r8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, false, 1, false>(m_context, "textures_storage_1d_rg8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, false, 1, false>(m_context, "textures_storage_1d_rgba8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, false, 2, false>(m_context, "textures_storage_2d_r8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, false, 2, false>(m_context, "textures_storage_2d_rg8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, false, 2, false>(m_context, "textures_storage_2d_rgba8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, false, 3, false>(m_context, "textures_storage_3d_r8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, false, 3, false>(m_context, "textures_storage_3d_rg8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, false, 3, false>(m_context, "textures_storage_3d_rgba8ui"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, true, 1, false>(m_context, "textures_storage_1d_r8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, true, 1, false>(m_context, "textures_storage_1d_rg8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, true, 1, false>(m_context, "textures_storage_1d_rgba8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, true, 2, false>(m_context, "textures_storage_2d_r8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, true, 2, false>(m_context, "textures_storage_2d_rg8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, true, 2, false>(m_context, "textures_storage_2d_rgba8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, true, 3, false>(m_context, "textures_storage_3d_r8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, true, 3, false>(m_context, "textures_storage_3d_rg8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, true, 3, false>(m_context, "textures_storage_3d_rgba8ui_unorm"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 1, false, 1, false>(m_context, "textures_storage_1d_r16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 2, false, 1, false>(m_context, "textures_storage_1d_rg16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 4, false, 1, false>(m_context, "textures_storage_1d_rgba16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 1, false, 2, false>(m_context, "textures_storage_2d_r16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 2, false, 2, false>(m_context, "textures_storage_2d_rg16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 4, false, 2, false>(m_context, "textures_storage_2d_rgba16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 1, false, 3, false>(m_context, "textures_storage_3d_r16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 2, false, 3, false>(m_context, "textures_storage_3d_rg16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 4, false, 3, false>(m_context, "textures_storage_3d_rgba16i"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, false, 1, false>(m_context, "textures_storage_1d_r16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, false, 1, false>(m_context, "textures_storage_1d_rg16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, false, 1, false>(m_context, "textures_storage_1d_rgba16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, false, 2, false>(m_context, "textures_storage_2d_r16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, false, 2, false>(m_context, "textures_storage_2d_rg16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, false, 2, false>(m_context, "textures_storage_2d_rgba16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, false, 3, false>(m_context, "textures_storage_3d_r16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, false, 3, false>(m_context, "textures_storage_3d_rg16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, false, 3, false>(m_context, "textures_storage_3d_rgba16ui"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, true, 1, false>(m_context, "textures_storage_1d_r16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, true, 1, false>(m_context, "textures_storage_1d_rg16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, true, 1, false>(m_context, "textures_storage_1d_rgba16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, true, 2, false>(m_context, "textures_storage_2d_r16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, true, 2, false>(m_context, "textures_storage_2d_rg16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, true, 2, false>(m_context, "textures_storage_2d_rgba16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, true, 3, false>(m_context, "textures_storage_3d_r16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, true, 3, false>(m_context, "textures_storage_3d_rg16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, true, 3, false>(m_context, "textures_storage_3d_rgba16ui_unorm"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 1, false, 1, false>(m_context, "textures_storage_1d_r32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 2, false, 1, false>(m_context, "textures_storage_1d_rg32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 3, false, 1, false>(m_context, "textures_storage_1d_rgb32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 4, false, 1, false>(m_context, "textures_storage_1d_rgba32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 1, false, 2, false>(m_context, "textures_storage_2d_r32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 2, false, 2, false>(m_context, "textures_storage_2d_rg32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 3, false, 2, false>(m_context, "textures_storage_2d_rgb32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 4, false, 2, false>(m_context, "textures_storage_2d_rgba32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 1, false, 3, false>(m_context, "textures_storage_3d_r32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 2, false, 3, false>(m_context, "textures_storage_3d_rg32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 3, false, 3, false>(m_context, "textures_storage_3d_rgb32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 4, false, 3, false>(m_context, "textures_storage_3d_rgba32i"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 1, false, 1, false>(m_context, "textures_storage_1d_r32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 2, false, 1, false>(m_context, "textures_storage_1d_rg32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 3, false, 1, false>(m_context, "textures_storage_1d_rgb32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 4, false, 1, false>(m_context, "textures_storage_1d_rgba32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 1, false, 2, false>(m_context, "textures_storage_2d_r32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 2, false, 2, false>(m_context, "textures_storage_2d_rg32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 3, false, 2, false>(m_context, "textures_storage_2d_rgb32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 4, false, 2, false>(m_context, "textures_storage_2d_rgba32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 1, false, 3, false>(m_context, "textures_storage_3d_r32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 2, false, 3, false>(m_context, "textures_storage_3d_rg32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 3, false, 3, false>(m_context, "textures_storage_3d_rgb32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 4, false, 3, false>(m_context, "textures_storage_3d_rgba32ui"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 1, true, 1, false>(m_context, "textures_storage_1d_r32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 2, true, 1, false>(m_context, "textures_storage_1d_rg32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 3, true, 1, false>(m_context, "textures_storage_1d_rgb32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 4, true, 1, false>(m_context, "textures_storage_1d_rgba32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 1, true, 2, false>(m_context, "textures_storage_2d_r32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 2, true, 2, false>(m_context, "textures_storage_2d_rg32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 3, true, 2, false>(m_context, "textures_storage_2d_rgb32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 4, true, 2, false>(m_context, "textures_storage_2d_rgba32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 1, true, 3, false>(m_context, "textures_storage_3d_r32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 2, true, 3, false>(m_context, "textures_storage_3d_rg32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 3, true, 3, false>(m_context, "textures_storage_3d_rgb32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 4, true, 3, false>(m_context, "textures_storage_3d_rgba32f"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 1, false, 1, true>(m_context, "textures_subimage_1d_r8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 2, false, 1, true>(m_context, "textures_subimage_1d_rg8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 4, false, 1, true>(m_context, "textures_subimage_1d_rgba8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 1, false, 2, true>(m_context, "textures_subimage_2d_r8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 2, false, 2, true>(m_context, "textures_subimage_2d_rg8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 4, false, 2, true>(m_context, "textures_subimage_2d_rgba8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 1, false, 3, true>(m_context, "textures_subimage_3d_r8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 2, false, 3, true>(m_context, "textures_subimage_3d_rg8i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLbyte, 4, false, 3, true>(m_context, "textures_subimage_3d_rgba8i"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, false, 1, true>(m_context, "textures_subimage_1d_r8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, false, 1, true>(m_context, "textures_subimage_1d_rg8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, false, 1, true>(m_context, "textures_subimage_1d_rgba8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, false, 2, true>(m_context, "textures_subimage_2d_r8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, false, 2, true>(m_context, "textures_subimage_2d_rg8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, false, 2, true>(m_context, "textures_subimage_2d_rgba8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, false, 3, true>(m_context, "textures_subimage_3d_r8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, false, 3, true>(m_context, "textures_subimage_3d_rg8ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, false, 3, true>(m_context, "textures_subimage_3d_rgba8ui"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, true, 1, true>(m_context, "textures_subimage_1d_r8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, true, 1, true>(m_context, "textures_subimage_1d_rg8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, true, 1, true>(m_context, "textures_subimage_1d_rgba8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, true, 2, true>(m_context, "textures_subimage_2d_r8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, true, 2, true>(m_context, "textures_subimage_2d_rg8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, true, 2, true>(m_context, "textures_subimage_2d_rgba8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 1, true, 3, true>(m_context, "textures_subimage_3d_r8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 2, true, 3, true>(m_context, "textures_subimage_3d_rg8ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLubyte, 4, true, 3, true>(m_context, "textures_subimage_3d_rgba8ui_unorm"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 1, false, 1, true>(m_context, "textures_subimage_1d_r16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 2, false, 1, true>(m_context, "textures_subimage_1d_rg16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 4, false, 1, true>(m_context, "textures_subimage_1d_rgba16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 1, false, 2, true>(m_context, "textures_subimage_2d_r16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 2, false, 2, true>(m_context, "textures_subimage_2d_rg16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 4, false, 2, true>(m_context, "textures_subimage_2d_rgba16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 1, false, 3, true>(m_context, "textures_subimage_3d_r16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 2, false, 3, true>(m_context, "textures_subimage_3d_rg16i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLshort, 4, false, 3, true>(m_context, "textures_subimage_3d_rgba16i"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, false, 1, true>(m_context, "textures_subimage_1d_r16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, false, 1, true>(m_context, "textures_subimage_1d_rg16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, false, 1, true>(m_context, "textures_subimage_1d_rgba16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, false, 2, true>(m_context, "textures_subimage_2d_r16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, false, 2, true>(m_context, "textures_subimage_2d_rg16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, false, 2, true>(m_context, "textures_subimage_2d_rgba16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, false, 3, true>(m_context, "textures_subimage_3d_r16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, false, 3, true>(m_context, "textures_subimage_3d_rg16ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, false, 3, true>(m_context, "textures_subimage_3d_rgba16ui"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, true, 1, true>(m_context, "textures_subimage_1d_r16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, true, 1, true>(m_context, "textures_subimage_1d_rg16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, true, 1, true>(m_context, "textures_subimage_1d_rgba16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, true, 2, true>(m_context, "textures_subimage_2d_r16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, true, 2, true>(m_context, "textures_subimage_2d_rg16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, true, 2, true>(m_context, "textures_subimage_2d_rgba16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 1, true, 3, true>(m_context, "textures_subimage_3d_r16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 2, true, 3, true>(m_context, "textures_subimage_3d_rg16ui_unorm"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLushort, 4, true, 3, true>(m_context, "textures_subimage_3d_rgba16ui_unorm"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 1, false, 1, true>(m_context, "textures_subimage_1d_r32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 2, false, 1, true>(m_context, "textures_subimage_1d_rg32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 3, false, 1, true>(m_context, "textures_subimage_1d_rgb32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 4, false, 1, true>(m_context, "textures_subimage_1d_rgba32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 1, false, 2, true>(m_context, "textures_subimage_2d_r32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 2, false, 2, true>(m_context, "textures_subimage_2d_rg32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 3, false, 2, true>(m_context, "textures_subimage_2d_rgb32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 4, false, 2, true>(m_context, "textures_subimage_2d_rgba32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 1, false, 3, true>(m_context, "textures_subimage_3d_r32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 2, false, 3, true>(m_context, "textures_subimage_3d_rg32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 3, false, 3, true>(m_context, "textures_subimage_3d_rgb32i"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLint, 4, false, 3, true>(m_context, "textures_subimage_3d_rgba32i"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 1, false, 1, true>(m_context, "textures_subimage_1d_r32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 2, false, 1, true>(m_context, "textures_subimage_1d_rg32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 3, false, 1, true>(m_context, "textures_subimage_1d_rgb32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 4, false, 1, true>(m_context, "textures_subimage_1d_rgba32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 1, false, 2, true>(m_context, "textures_subimage_2d_r32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 2, false, 2, true>(m_context, "textures_subimage_2d_rg32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 3, false, 2, true>(m_context, "textures_subimage_2d_rgb32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 4, false, 2, true>(m_context, "textures_subimage_2d_rgba32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 1, false, 3, true>(m_context, "textures_subimage_3d_r32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 2, false, 3, true>(m_context, "textures_subimage_3d_rg32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 3, false, 3, true>(m_context, "textures_subimage_3d_rgb32ui"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLuint, 4, false, 3, true>(m_context, "textures_subimage_3d_rgba32ui"));

	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 1, true, 1, true>(m_context, "textures_subimage_1d_r32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 2, true, 1, true>(m_context, "textures_subimage_1d_rg32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 3, true, 1, true>(m_context, "textures_subimage_1d_rgb32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 4, true, 1, true>(m_context, "textures_subimage_1d_rgba32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 1, true, 2, true>(m_context, "textures_subimage_2d_r32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 2, true, 2, true>(m_context, "textures_subimage_2d_rg32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 3, true, 2, true>(m_context, "textures_subimage_2d_rgb32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 4, true, 2, true>(m_context, "textures_subimage_2d_rgba32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 1, true, 3, true>(m_context, "textures_subimage_3d_r32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 2, true, 3, true>(m_context, "textures_subimage_3d_rg32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 3, true, 3, true>(m_context, "textures_subimage_3d_rgb32f"));
	addChild(new Textures::StorageAndSubImageTest<glw::GLfloat, 4, true, 3, true>(m_context, "textures_subimage_3d_rgba32f"));

	addChild(new Textures::StorageMultisampleTest<glw::GLbyte, 1, false, 2>(m_context, "textures_storage_multisample_2d_r8i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLbyte, 2, false, 2>(m_context, "textures_storage_multisample_2d_rg8i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLbyte, 4, false, 2>(m_context, "textures_storage_multisample_2d_rgba8i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLbyte, 1, false, 3>(m_context, "textures_storage_multisample_3d_r8i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLbyte, 2, false, 3>(m_context, "textures_storage_multisample_3d_rg8i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLbyte, 4, false, 3>(m_context, "textures_storage_multisample_3d_rgba8i"));

	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 1, false, 2>(m_context, "textures_storage_multisample_2d_r8ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 2, false, 2>(m_context, "textures_storage_multisample_2d_rg8ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 4, false, 2>(m_context, "textures_storage_multisample_2d_rgba8ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 1, false, 3>(m_context, "textures_storage_multisample_3d_r8ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 2, false, 3>(m_context, "textures_storage_multisample_3d_rg8ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 4, false, 3>(m_context, "textures_storage_multisample_3d_rgba8ui"));

	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 1, true, 2>(m_context, "textures_storage_multisample_2d_r8ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 2, true, 2>(m_context, "textures_storage_multisample_2d_rg8ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 4, true, 2>(m_context, "textures_storage_multisample_2d_rgba8ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 1, true, 3>(m_context, "textures_storage_multisample_3d_r8ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 2, true, 3>(m_context, "textures_storage_multisample_3d_rg8ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLubyte, 4, true, 3>(m_context, "textures_storage_multisample_3d_rgba8ui_unorm"));

	addChild(new Textures::StorageMultisampleTest<glw::GLshort, 1, false, 2>(m_context, "textures_storage_multisample_2d_r16i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLshort, 2, false, 2>(m_context, "textures_storage_multisample_2d_rg16i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLshort, 4, false, 2>(m_context, "textures_storage_multisample_2d_rgba16i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLshort, 1, false, 3>(m_context, "textures_storage_multisample_3d_r16i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLshort, 2, false, 3>(m_context, "textures_storage_multisample_3d_rg16i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLshort, 4, false, 3>(m_context, "textures_storage_multisample_3d_rgba16i"));

	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 1, false, 2>(m_context, "textures_storage_multisample_2d_r16u"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 2, false, 2>(m_context, "textures_storage_multisample_2d_rg16u"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 4, false, 2>(m_context, "textures_storage_multisample_2d_rgba16u"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 1, false, 3>(m_context, "textures_storage_multisample_3d_r16u"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 2, false, 3>(m_context, "textures_storage_multisample_3d_rg16u"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 4, false, 3>(m_context, "textures_storage_multisample_3d_rgba16u"));

	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 1, true, 2>(m_context, "textures_storage_multisample_2d_r16ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 2, true, 2>(m_context, "textures_storage_multisample_2d_rg16ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 4, true, 2>(m_context, "textures_storage_multisample_2d_rgba16ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 1, true, 3>(m_context, "textures_storage_multisample_3d_r16ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 2, true, 3>(m_context, "textures_storage_multisample_3d_rg16ui_unorm"));
	addChild(new Textures::StorageMultisampleTest<glw::GLushort, 4, true, 3>(m_context, "textures_storage_multisample_3d_rgba16ui_unorm"));

	addChild(new Textures::StorageMultisampleTest<glw::GLint, 1, false, 2>(m_context, "textures_storage_multisample_2d_r32i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLint, 2, false, 2>(m_context, "textures_storage_multisample_2d_rg32i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLint, 3, false, 2>(m_context, "textures_storage_multisample_2d_rgb32i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLint, 4, false, 2>(m_context, "textures_storage_multisample_2d_rgba32i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLint, 1, false, 3>(m_context, "textures_storage_multisample_3d_r32i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLint, 2, false, 3>(m_context, "textures_storage_multisample_3d_rg32i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLint, 3, false, 3>(m_context, "textures_storage_multisample_3d_rgb32i"));
	addChild(new Textures::StorageMultisampleTest<glw::GLint, 4, false, 3>(m_context, "textures_storage_multisample_3d_rgba32i"));

	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 1, false, 2>(m_context, "textures_storage_multisample_2d_r32ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 2, false, 2>(m_context, "textures_storage_multisample_2d_rg32ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 3, false, 2>(m_context, "textures_storage_multisample_2d_rgb32ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 4, false, 2>(m_context, "textures_storage_multisample_2d_rgba32ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 1, false, 3>(m_context, "textures_storage_multisample_3d_r32ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 2, false, 3>(m_context, "textures_storage_multisample_3d_rg32ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 3, false, 3>(m_context, "textures_storage_multisample_3d_rgb32ui"));
	addChild(new Textures::StorageMultisampleTest<glw::GLuint, 4, false, 3>(m_context, "textures_storage_multisample_3d_rgba32ui"));

	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 1, true, 2>(m_context, "textures_storage_multisample_2d_r32f"));
	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 2, true, 2>(m_context, "textures_storage_multisample_2d_rg32f"));
	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 3, true, 2>(m_context, "textures_storage_multisample_2d_rgb32f"));
	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 4, true, 2>(m_context, "textures_storage_multisample_2d_rgba32f"));
	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 1, true, 3>(m_context, "textures_storage_multisample_3d_r32f"));
	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 2, true, 3>(m_context, "textures_storage_multisample_3d_rg32f"));
	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 3, true, 3>(m_context, "textures_storage_multisample_3d_rgb32f"));
	addChild(new Textures::StorageMultisampleTest<glw::GLfloat, 4, true, 3>(m_context, "textures_storage_multisample_3d_rgba32f"));

	addChild(new Textures::CompressedSubImageTest(m_context));
	addChild(new Textures::CopyTest(m_context));
	addChild(new Textures::GetSetParameterTest(m_context));
	addChild(new Textures::DefaultsTest(m_context));
	addChild(new Textures::GenerateMipmapTest(m_context));
	addChild(new Textures::BindUnitTest(m_context));
	addChild(new Textures::GetImageTest(m_context));
	addChild(new Textures::GetLevelParameterTest(m_context));
	addChild(new Textures::CreationErrorsTest(m_context));
	addChild(new Textures::BufferErrorsTest(m_context));
	addChild(new Textures::BufferRangeErrorsTest(m_context));
	addChild(new Textures::StorageErrorsTest(m_context));
	addChild(new Textures::SubImageErrorsTest(m_context));
	addChild(new Textures::CopyErrorsTest(m_context));
	addChild(new Textures::ParameterSetupErrorsTest(m_context));
	addChild(new Textures::GenerateMipmapErrorsTest(m_context));
	addChild(new Textures::BindUnitErrorsTest(m_context));
	addChild(new Textures::ImageQueryErrorsTest(m_context));
	addChild(new Textures::LevelParameterErrorsTest(m_context));
	addChild(new Textures::ParameterErrorsTest(m_context));
}
} /* DirectStateAccess namespace */
} /* gl4cts namespace */
