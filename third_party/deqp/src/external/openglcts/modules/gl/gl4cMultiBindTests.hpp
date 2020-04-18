#ifndef _GL4CMULTIBINDTESTS_HPP
#define _GL4CMULTIBINDTESTS_HPP
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
 * \file  gl4cMultiBindTests.hpp
 * \brief Declares test classes for "Multi Bind" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace gl4cts
{
namespace MultiBind
{
/** Implementation of test ErrorsBindBuffers. Description follows:
 *
 * Verifies that BindBuffersBase and BindBuffersRange commands generate errors
 * as expected:
 * - INVALID_ENUM when <target> is not valid;
 * - INVALID_OPERATION when <first> + <count> is greater than allowed limit;
 * - INVALID_OPERATION if any value in <buffers> is not zero or the name of
 * existing buffer;
 * - INVALID_VALUE if any value in <offsets> is less than zero;
 * - INVALID_VALUE if any pair of <offsets> and <sizes> exceeds limits.
 **/
class ErrorsBindBuffersTest : public deqp::TestCase
{
public:
	/* Public methods */
	ErrorsBindBuffersTest(deqp::Context& context);

	virtual ~ErrorsBindBuffersTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test ErrorsBindTextures. Description follows:
 *
 * Verifies that BindTextures generate errors as expected:
 * - INVALID_OPERATION when <first> + <count> exceed limits;
 * - INVALID_OPERATION if any value in <textures> is not zero or name of
 * existing texture.
 **/
class ErrorsBindTexturesTest : public deqp::TestCase
{
public:
	/* Public methods */
	ErrorsBindTexturesTest(deqp::Context& context);

	virtual ~ErrorsBindTexturesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test ErrorsBindSamplers. Description follows:
 *
 * Verifies that BindSamplers generate errors as expected:
 * - INVALID_OPERATION when <first> + <count> exceed limits;
 * - INVALID_OPERATION if any value in <samplers> is not zero or name of
 * existing texture.
 **/
class ErrorsBindSamplersTest : public deqp::TestCase
{
public:
	/* Public methods */
	ErrorsBindSamplersTest(deqp::Context& context);

	virtual ~ErrorsBindSamplersTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test ErrorsBindImageTextures. Description follows:
 *
 * Verifies that BindImageTextures generate errors as expected:
 * - INVALID_OPERATION when <first> + <count> exceed limits;
 * - INVALID_OPERATION if any value in <textures> is not zero or name of
 * existing texture;
 * - INVALID_OPERATION if any entry found in <textures> has invalid internal
 * format at level 0;
 * - INVALID_OPERATION when any entry in <textures> has any of dimensions equal
 * to 0 at level 0.
 **/
class ErrorsBindImageTexturesTest : public deqp::TestCase
{
public:
	/* Public methods */
	ErrorsBindImageTexturesTest(deqp::Context& context);

	virtual ~ErrorsBindImageTexturesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test ErrorsBindVertexBuffers. Description follows:
 *
 * Verifies that BindVertexBuffers generate errors as expected:
 * - INVALID_OPERATION when <first> + <count> exceeds limits;
 * - INVALID_OPERATION if any value in <buffers> is not zero or the name of
 * existing buffer;
 * - INVALID_VALUE if any value in <offsets> or <strides> is less than zero.
 **/
class ErrorsBindVertexBuffersTest : public deqp::TestCase
{
public:
	/* Public methods */
	ErrorsBindVertexBuffersTest(deqp::Context& context);

	virtual ~ErrorsBindVertexBuffersTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test FunctionalBindBuffersBase. Description follows:
 *
 * Verifies that BindBuffersBase works as expected.
 *
 * Steps to be done for each valid target:
 * - prepare MAX buffers with some store, where MAX is the maximum supported
 * amount of bindings points for tested target;
 *
 * - execute BindBufferBase to bind all buffers to tested target;
 * - inspect if bindings were modified;
 *
 * - execute BindBufferBase for first half of bindings with NULL as <buffers>
 * to unbind first half of bindings for tested target;
 * - inspect if bindings were modified;
 * - execute BindBufferBase for second half of bindings with NULL as <buffers>
 * to unbind rest of bindings;
 * - inspect if bindings were modified;
 *
 * - change <buffers> so first entry is invalid;
 * - execute BindBufferBase to bind all buffers to tested target; It is
 * expected that INVALID_OPERATION will be generated;
 * - inspect if all bindings but first were modified;
 *
 * - bind any buffer to first binding;
 * - execute BindBufferBase for 0 as <first>, 1 as <count> and <buffers> filled
 * with zeros to unbind 1st binding for tested target;
 * - inspect if bindings were modified;
 *
 * - unbind all buffers.
 **/
class FunctionalBindBuffersBaseTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalBindBuffersBaseTest(deqp::Context& context);

	virtual ~FunctionalBindBuffersBaseTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test FunctionalBindBuffersRange. Description follows:
 *
 * Verifies that BindBuffersRange works as expected.
 *
 * Steps to be done for each valid target:
 * - prepare MAX buffers with some store, where MAX is the maximum supported
 * amount of bindings points for tested target;
 *
 * - execute BindBufferRange to bind all buffers to tested target;
 * - inspect if bindings were modified;
 *
 * - execute BindBufferRange for first half of bindings with NULL as <buffers>
 * to unbind first half of bindings for tested target;
 * - inspect if bindings were modified;
 * - execute BindBufferRange for second half of bindings with NULL as <buffers>
 * to unbind rest of bindings for tested target;
 * - inspect if bindings were modified;
 *
 * - change <buffers> so first entry is invalid;
 * - execute BindBufferRange to bind all buffers to tested target; It is
 * expected that INVALID_OPERATION will be generated;
 * - inspect if all bindings but first were modified;
 *
 * - bind any buffer to first binding;
 * - execute BindBufferRange for 0 as <first>, 1 as <count> and <buffers>
 * filled with zeros to unbind first binding for tested target;
 * - inspect if bindings were modified;
 *
 * - unbind all buffers.
 **/
class FunctionalBindBuffersRangeTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalBindBuffersRangeTest(deqp::Context& context);

	virtual ~FunctionalBindBuffersRangeTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test FunctionalBindTextures. Description follows:
 *
 * Verify that BindTextures works as expected.
 *
 * Steps:
 * - prepare MAX_COMBINED_TEXTURE_IMAGE_UNITS textures with some store;
 * Use all valid texture targets;
 *
 * - execute BindTextures to bind all textures;
 * - inspect bindings of all texture units to verify that proper bindings were
 * set;
 *
 * - execute BindTextures for the first half of units with <textures> filled
 * with zeros, to unbind those units;
 * - inspect bindings of all texture units to verify that proper bindings were
 * unbound;
 *
 * - execute BindTextures for the second half of units with NULL as<textures>,
 * to unbind those units;
 * - inspect bindings of all texture units to verify that proper bindings were
 * unbound;
 *
 * - modify <textures> so first entry is invalid;
 * - execute BindTextures to bind all textures; It is expected that
 * INVALID_OPERATION will be generated;
 * - inspect bindings of all texture units to verify that proper bindings were
 * set;
 *
 * - unbind all textures.
 **/
class FunctionalBindTexturesTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalBindTexturesTest(deqp::Context& context);

	virtual ~FunctionalBindTexturesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test FunctionalBindSamplers. Description follows:
 *
 * Verify that BindSamplers works as expected.
 *
 * Steps:
 * - prepare MAX_COMBINED_TEXTURE_IMAGE_UNITS samplers;
 *
 * - execute BindSamplers to bind all samplers;
 * - inspect bindings to verify that proper samplers were set;
 *
 * - execute BindSamplers for first half of bindings with <samplers> filled
 * with zeros, to unbind those samplers;
 * - inspect bindings to verify that proper samplers were unbound;
 *
 * - execute BindSamplers for second half of bindings with NULL as <samplers>,
 * to unbind those samplers;
 * - inspect bindings to verify that proper samplers were unbound;
 *
 * - modify <samplers> so first entry is invalid;
 * - execute BindSamplers to bind all samplers; It is expected that
 * INVALID_OPERATION will be generated;
 * - inspect bindings to verify that proper samplers were set;
 *
 * - unbind all samplers.
 **/
class FunctionalBindSamplersTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalBindSamplersTest(deqp::Context& context);

	virtual ~FunctionalBindSamplersTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test FunctionalBindImageTextures. Description follows:
 *
 * Verify that BindImageTextures works as expected.
 *
 * Steps:
 * - prepare MAX_IMAGE_UNITS textures; Use TEXTURE_1D, TEXTURE_1D_ARRAY,
 * TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_BUFFER, TEXTURE_CUBE_MAP
 * and TEXTURE_CUBE_MAP_ARRAY; If possible use rest of targets;
 *
 * - execute BindImageTextures to bind all images;
 * - inspect bindings to verify that proper images were set;
 *
 * - execute BindImageTextures for first half of units with <textures> filled
 * with zeros, to unbind those images;
 * - inspect bindings to verify that proper images were unbound;
 *
 * - execute BindImageTextures for second half of bindings with NULL as <samples>,
 * to unbind those images;
 * - inspect bindings to verify that proper images were unbound;
 *
 * - modify <textures> so first entry is invalid;
 * - execute BindImageTextures to bind all textures; It is expected that
 * INVALID_OPERATION will be generated;
 * - inspect bindings to verify that proper images were set;
 *
 * - unbind all images.
 **/
class FunctionalBindImageTexturesTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalBindImageTexturesTest(deqp::Context& context);

	virtual ~FunctionalBindImageTexturesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test FunctionalBindVertexBuffers. Description follows:
 *
 * Verify that BindVertexBuffers works as expected.
 *
 * Steps:
 * - prepare MAX_VERTEX_ATTRIB_BINDINGS buffers;
 *
 * - execute BindVertexBuffers to bind all buffer;
 * - inspect bindings to verify that proper buffers were set;
 *
 * - execute BindVertexBuffers for first half of bindings with <buffers> filled
 * with zeros, to unbind those buffers;
 * - inspect bindings to verify that proper buffers were unbound;
 *
 * - execute BindVertexBuffers for second half of bindings with NULL as
 * <buffers>, to unbind those buffers;
 * - inspect bindings to verify that proper buffers were unbound;
 *
 * - modify <buffers> so first entry is invalid;
 * - execute BindVertexBuffers to bind all buffers; It is expected that
 * INVALID_OPERATION will be generated;
 * - inspect bindings to verify that proper buffers were set;
 *
 * - unbind all buffers.
 **/
class FunctionalBindVertexBuffersTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalBindVertexBuffersTest(deqp::Context& context);

	virtual ~FunctionalBindVertexBuffersTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test DispatchBindBuffersBase. Description follows:
 *
 * Verifies that BindBuffersBase command works as expected.
 *
 * In compute shader declare the GL_MAX_COMPUTE_UNIFORM_BLOCKS uniform blocks:
 *
 *   layout(std140, binding = 0) uniform B1 {vec4 a;} b1;
 *   layout(std140, binding = 0) uniform B2 {vec4 a;} b2;
 *
 *   ...
 *
 *   layout(std140, binding = (GL_MAX_COMPUTE_UNIFORM_BLOCKS-1)) uniform BX {
 *       vec4 a;
 *   } bx;
 *
 * The shader should sum b1.a, b2.a + ... + bx.a and store result in shader
 * storage buffer.
 *
 * In test program, create GL_MAX_COMPUTE_UNIFORM_BLOCKS buffers, bind them
 * using BindBuffersBase to B1 ... BX blocks, fill them with unique values.
 * Create and bind shader storage buffer for result. Dispatch compute shader.
 *
 * Test pass if the result is correct.
 **/
class DispatchBindBuffersBaseTest : public deqp::TestCase
{
public:
	/* Public methods */
	DispatchBindBuffersBaseTest(deqp::Context& context);

	virtual ~DispatchBindBuffersBaseTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test DispatchBindBuffersRange. Description follows:
 *
 * Verifies that BindBuffersRange command works as expected.
 *
 * In compute shader create the 4 of uniform blocks:
 *
 *   layout(std140, binding = 0) uniform B1 {int a;} b1;
 *   layout(std140, binding = 1) uniform B2 {int a;} b2;
 *   layout(std140, binding = 2) uniform B4 {int a;} b3;
 *   layout(std140, binding = 3) uniform B4 {int a;} b4;
 *
 * The shader should be sum b1.a b2.a, b3.a b4.a and store result in shader
 * storage buffer.
 *
 *
 * In test program, create the buffer filled with the following 7 bytes:
 *
 *   { 0x00 0x01 0x00 0x01 0x00 0x01 0x00 }
 *
 * Bind buffer with BindBuffersRange with the following parameters:
 *   - <target>  GL_UNIFORM_BUFFER
 *   - <first>   0
 *   - <count>   4
 *   - <buffers> { ID, ID, ID, ID },
 *   - <offsets> { 0,  1,  2,  3 },
 *   - <sizes>   { 4,  4,  4,  4 }.
 *
 * Create and bind shader storage buffer for result. Dispatch compute shader.
 *
 * Test pass if the result is 0x02020202.
 **/
class DispatchBindBuffersRangeTest : public deqp::TestCase
{
public:
	/* Public methods */
	DispatchBindBuffersRangeTest(deqp::Context& context);

	virtual ~DispatchBindBuffersRangeTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test DispatchBindTextures. Description follows:
 *
 * Verifies that BindTextures command works as expected.
 *
 * Modify DispatchBindBuffersBase test in the following aspects:
 *   - use R32UI textures instead of uniform blocks;
 *   - use GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS textures;
 *   - use all texture targets;
 *   - use 1x1x1 textures, sample point (0.5, 0.5, 0.5).
 **/
class DispatchBindTexturesTest : public deqp::TestCase
{
public:
	/* Public methods */
	DispatchBindTexturesTest(deqp::Context& context);
	virtual ~DispatchBindTexturesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test DispatchBindImageTextures. Description follows:
 *
 * Verifies that BindImageTextures command works as expected.
 *
 * Modify DispatchBindTextures test in the following aspects:
 *   - use image units instead of texture units.
 **/
class DispatchBindImageTexturesTest : public deqp::TestCase
{
public:
	/* Public methods */
	DispatchBindImageTexturesTest(deqp::Context& context);

	virtual ~DispatchBindImageTexturesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test DispatchBindSamplers. Description follows:
 *
 * Verifies that BindSamplers command works as expected.
 *
 * Prepare GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS 8x8 2D R32UI textures filled so
 * the edges are set to 1 and center area is set to 0.
 * Prepare GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS samplers. Set parameter
 * GL_TEXTURE_WRAP_S to GL_CLAMP_TO_EDGE for all samplers.
 *
 * Prepare compute program as in test DispatchBindTextures, but sample
 * point (1.5, 0.5, 0.5).
 *
 * Test pass when the result stored in storage buffer is equal to
 * GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS.
 **/
class DispatchBindSamplersTest : public deqp::TestCase
{
public:
	/* Public methods */
	DispatchBindSamplersTest(deqp::Context& context);

	virtual ~DispatchBindSamplersTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test DrawBindVertexBuffers. Description follows:
 *
 * Verifies that BindVertexBuffers command works as expected.
 *
 * Prepare program consisting of vertex, geometry and fragment shader. Vertex
 * shader should:
 *   - declare GL_MAX_VERTEX_ATTRIB_BINDINGS attributes:
 *
 *     layout(location = 0) in vec4 attr_0;
 *     layout(location = 1) in vec4 attr_0;
 *
 *     ...
 *
 *     layout(location = GL_MAX_VERTEX_ATTRIB_BINDINGS - 1) in vec4 attr_X;
 *
 *   - calcualte sum of all attributes and store result in output varying.
 * Geometry shader should:
 *   - output fullscreen quad;
 *   - pass-through the value of summed attributes to fragment shader.
 * Fragment shader should pass-through the value of summed attributes to output
 * color.
 *
 * Prepare and bind vertex array object. Prepare ARRAY buffer filled with all
 * attributes. Values should be selected so the sum is equal 1.0 at each
 * channel. Use BindVertexBuffers to set up all attributes to use the buffer.
 *
 * Prepare framebuffer with 8x8 2D RGBA8 texture attached as color 0. Fill
 * texture with 0.
 *
 * Execute drawArrays for single vertex.
 *
 * Test pass if framebuffer is filled with value 1.
 **/
class DrawBindVertexBuffersTest : public deqp::TestCase
{
public:
	/* Public methods */
	DrawBindVertexBuffersTest(deqp::Context& context);

	virtual ~DrawBindVertexBuffersTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};
} /* MultiBind */

/** Group class for multi bind conformance tests */
class MultiBindTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	MultiBindTests(deqp::Context& context);

	virtual ~MultiBindTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	MultiBindTests(const MultiBindTests& other);
	MultiBindTests& operator=(const MultiBindTests& other);
};

} /* gl4cts */

#endif // _GL4CMULTIBINDTESTS_HPP
