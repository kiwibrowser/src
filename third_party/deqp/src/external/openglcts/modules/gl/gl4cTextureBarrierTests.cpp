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

/**
 * \file  gl4cTextureBarrierTests.cpp
 * \brief Implements conformance tests for "Texture Barrier" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cTextureBarrierTests.hpp"

#include "deMath.h"
#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

#include "glcWaiver.hpp"

namespace gl4cts
{

/*
 Base class of all test cases of the feature. Enforces the requirements below:

 * Check that the extension string or GL 4.5 is available.
 */
class TextureBarrierBaseTest : public deqp::TestCase
{
protected:
	TextureBarrierBaseTest(deqp::Context& context, TextureBarrierTests::API api, const char* name,
						   const char* description)
		: TestCase(context, name, description), m_api(api)
	{
	}

	/* Checks whether the feature is supported. */
	bool featureSupported()
	{
		return (m_api == TextureBarrierTests::API_GL_ARB_texture_barrier &&
				m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_barrier")) ||
			   m_api == TextureBarrierTests::API_GL_45core;
	}

	/* Basic test init, child classes must call it. */
	virtual void init()
	{
		if (!featureSupported())
		{
			throw tcu::NotSupportedError("Required texture_barrier extension is not supported");
		}
	}

protected:
	const TextureBarrierTests::API m_api;
};

/*
 Base class of all rendering test cases of the feature. Implements the basic outline below:

 This basic outline provides a simple tutorial on how to implement and
 what to check in the test cases of this feature.

 * Create a set of color textures and fill each of their texels with unique
 values using an arbitrary method. Set the minification and magnification
 filtering modes of the textures to NEAREST. Bind all of them for
 texturing to subsequent texture units starting from texture unit zero.

 * Create a framebuffer object and attach the set of textures so that
 texture #i is attached as color attachment #i. Set the draw buffers so
 that draw buffer #i is set to color attachment #i. Bind the framebuffer
 for rendering.

 * Render a set of primitives that cover each pixel of the framebuffer
 exactly once using the fragment shader described in the particular
 test case.

 * Expect all texels written by the draw command to have well defined
 values in accordance with the used fragment shader's functionality.
 */
class TextureBarrierBasicOutline : public TextureBarrierBaseTest
{
protected:
	TextureBarrierBasicOutline(deqp::Context& context, TextureBarrierTests::API api, const char* name,
							   const char* description)
		: TextureBarrierBaseTest(context, api, name, description)
		, m_program(0)
		, m_vao(0)
		, m_vbo(0)
		, m_fbo(0)
		, m_width(0)
		, m_height(0)
		, m_actual(DE_NULL)
	{
		for (size_t i = 0; i < NUM_TEXTURES; ++i)
		{
			m_tex[i]	   = 0;
			m_reference[i] = DE_NULL;
		}
	}

	/* Actual test cases may override it to provide an alternative vertex shader. */
	virtual const char* vsh()
	{
		return "#version 400 core\n"
			   "in vec2 Position;\n"
			   "void main() {\n"
			   "    gl_Position = vec4(Position, 0.0, 1.0);\n"
			   "}";
	}

	/* Actual test cases must override it to provide the fragment shader. */
	virtual const char* fsh() = 0;

	/* Rendering test init. */
	virtual void init()
	{
		// Call parent init (takes care of API requirements)
		TextureBarrierBaseTest::init();

		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		m_width								  = renderTarget.getWidth();
		m_height							  = renderTarget.getHeight();

#ifdef WAIVER_WITH_BUG_13788
		m_width = m_width >= 16383 ? 16382 : m_width;
#endif

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// Create textures
		gl.genTextures(NUM_TEXTURES, m_tex);
		for (GLuint i = 0; i < NUM_TEXTURES; ++i)
		{
			gl.activeTexture(GL_TEXTURE0 + i);
			gl.bindTexture(GL_TEXTURE_2D, m_tex[i]);
			gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, m_width, m_height);
			gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			m_reference[i] = new GLuint[m_width * m_height];
		}
		m_actual = new GLuint[m_width * m_height];

		// Create framebuffer
		gl.genFramebuffers(1, &m_fbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		GLenum drawBuffers[NUM_TEXTURES];
		for (GLuint i = 0; i < NUM_TEXTURES; ++i)
		{
			gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_tex[i], 0);
			drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		gl.drawBuffers(NUM_TEXTURES, drawBuffers);

		// Create vertex array and buffer
		gl.genVertexArrays(1, &m_vao);
		gl.bindVertexArray(m_vao);

		gl.genBuffers(1, &m_vbo);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, GRID_SIZE * GRID_SIZE * sizeof(float) * 12, NULL, GL_STATIC_DRAW);

		generateVertexData((float*)gl.mapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
		gl.unmapBuffer(GL_ARRAY_BUFFER);

		gl.vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		gl.enableVertexAttribArray(0);

		// Setup state
		gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
		gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	/* Rendering test deinit. */
	virtual void deinit()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// Cleanup textures
		gl.activeTexture(GL_TEXTURE0);
		gl.deleteTextures(NUM_TEXTURES, m_tex);
		for (GLuint i = 0; i < NUM_TEXTURES; ++i)
		{
			if (DE_NULL != m_reference[i])
			{
				delete[] m_reference[i];
				m_reference[i] = DE_NULL;
			}
		}

		if (DE_NULL != m_actual)
		{
			delete[] m_actual;
			m_actual = DE_NULL;
		}

		// Cleanup framebuffer
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		gl.deleteFramebuffers(1, &m_fbo);

		// Cleanup vertex array and buffer
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		gl.bindVertexArray(0);
		gl.deleteBuffers(1, &m_vbo);
		gl.deleteVertexArrays(1, &m_vao);

		// Cleanup state
		gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
		gl.pixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	/* Generate vertex data using the following steps:
	 (1) Generate an irregular grid covering the whole screen (i.e. (-1,-1) to (1,1));
	 (2) Generate a list of triangles covering the grid;
	 (3) Shuffle the generated triangle list;
	 (4) Write the vertices of the shuffled triangle list to the destination address. */
	void generateVertexData(float* destAddr)
	{
		DE_ASSERT(destAddr != NULL);

		typedef struct
		{
			float x, y;
		} Vertex;

		typedef struct
		{
			Vertex v0, v1, v2;
		} Triangle;

		static Vertex grid[GRID_SIZE + 1][GRID_SIZE + 1];

		// Generate grid vertices
		for (int x = 0; x < GRID_SIZE + 1; ++x)
			for (int y = 0; y < GRID_SIZE + 1; ++y)
			{
				// Calculate normalized coordinates
				float normx = (((float)x) / GRID_SIZE);
				float normy = (((float)y) / GRID_SIZE);

				// Pseudo-random grid vertex coordinate with scale & bias
				grid[x][y].x = normx * 2.f - 1.f + deFloatSin(normx * DE_PI * 13.f) * 0.3f / GRID_SIZE;
				grid[x][y].y = normy * 2.f - 1.f + deFloatSin(normy * DE_PI * 13.f) * 0.3f / GRID_SIZE;
			}

		Triangle list[TRIANGLE_COUNT];

		// Generate triangle list
		for (int x = 0; x < GRID_SIZE; ++x)
			for (int y = 0; y < GRID_SIZE; ++y)
			{
				// Generate first triangle of grid block
				list[(x + y * GRID_SIZE) * 2 + 0].v0 = grid[x][y];
				list[(x + y * GRID_SIZE) * 2 + 0].v1 = grid[x + 1][y];
				list[(x + y * GRID_SIZE) * 2 + 0].v2 = grid[x + 1][y + 1];

				// Generate second triangle of grid block
				list[(x + y * GRID_SIZE) * 2 + 1].v0 = grid[x + 1][y + 1];
				list[(x + y * GRID_SIZE) * 2 + 1].v1 = grid[x][y + 1];
				list[(x + y * GRID_SIZE) * 2 + 1].v2 = grid[x][y];
			}

		// Shuffle triangle list
		for (int i = TRIANGLE_COUNT - 2; i > 0; --i)
		{
			// Pseudo-random triangle index as one operand of the exchange
			int j = (int)((list[i].v1.y + list[i].v2.x + 13.f) * 1345.13f) % i;

			Triangle xchg = list[j];
			list[j]		  = list[i];
			list[i]		  = xchg;
		}

		// Write triange list vertices to destination address
		for (int i = 0; i < TRIANGLE_COUNT; ++i)
		{
			// Write first vertex of triangle
			destAddr[i * 6 + 0] = list[i].v0.x;
			destAddr[i * 6 + 1] = list[i].v0.y;

			// Write second vertex of triangle
			destAddr[i * 6 + 2] = list[i].v1.x;
			destAddr[i * 6 + 3] = list[i].v1.y;

			// Write third vertex of triangle
			destAddr[i * 6 + 4] = list[i].v2.x;
			destAddr[i * 6 + 5] = list[i].v2.y;
		}
	}

	/* Renders a set of primitives that cover each pixel of the framebuffer exactly once. */
	void render()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// Issue the whole grid using multiple separate draw commands
		int minTriCountPerDraw = TRIANGLE_COUNT / 7;
		int first = 0, count = 0;
		while (first < VERTEX_COUNT)
		{
			// Pseudo-random number of vertices per draw
			count = deMin32(VERTEX_COUNT - first, (first % 23 + minTriCountPerDraw) * 3);

			gl.drawArrays(GL_TRIANGLES, first, count);

			first += count;
		}
	}

	/* Returns a reference to the texel value of the specified image at the specified location. */
	GLuint& texel(GLuint* image, GLuint x, GLuint y)
	{
		// If out-of-bounds reads should return zero, writes should be ignored
		if ((static_cast<GLint>(x) < 0) || (x >= m_width) || (static_cast<GLint>(y) < 0) || (y >= m_height))
		{
			static GLuint zero;
			return (zero = 0);
		}

		return image[x + y * m_width];
	}

	/* Initializes the reference images and uploads them to their corresponding textures. */
	void initTextureData()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		for (GLuint i = 0; i < NUM_TEXTURES; ++i)
		{
			for (GLuint x = 0; x < m_width; ++x)
				for (GLuint y = 0; y < m_height; ++y)
				{
					texel(m_reference[i], x, y) = (i << 24) + (y << 12) + x;
				}

			gl.activeTexture(GL_TEXTURE0 + i);
			gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RED_INTEGER, GL_UNSIGNED_INT,
							 m_reference[i]);
		}
	}

	/* Updates the reference images according to a single execution of the fragment shader for each pixel. */
	virtual void updateTextureData() = 0;

	/* Verifies whether the reference images matches those of the textures we rendered to. */
	bool verifyTextureData()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		for (GLuint i = 0; i < NUM_TEXTURES; ++i)
		{
			gl.activeTexture(GL_TEXTURE0 + i);
			gl.getTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, m_actual);

			for (GLuint x = 0; x < m_width; ++x)
				for (GLuint y = 0; y < m_height; ++y)
				{
					if (texel(m_reference[i], x, y) != texel(m_actual, x, y))
					{
						return false;
					}
				}
		}

		return true;
	}

	/* Should return the number of separate test passes. */
	virtual int numTestPasses() = 0;

	/* Should return the number of rendering passes to perform. */
	virtual int numRenderPasses() = 0;

	/* Should set up configuration for a particular render pass (e.g. setting uniforms). */
	virtual void setupRenderPass(int testPass, int renderPass) = 0;

	/* Should return whether there is need for a TextureBarrier between subsequent render passes. */
	virtual bool needsBarrier() = 0;

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		tcu::TestLog&		  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();

		// Compile & link the program to use
		de::SharedPtr<glu::ShaderProgram> program(
			new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(vsh(), fsh())));
		log << (*program);
		if (!program->isOk())
		{
			TCU_FAIL("Program compilation failed");
		}
		m_program = program->getProgram();

		gl.useProgram(m_program);

		for (GLuint i = 0; i < NUM_TEXTURES; ++i)
		{
			GLchar samplerName[] = "texInput[0]";
			samplerName[9]		 = static_cast<GLchar>('0' + i);
			GLint loc			 = gl.getUniformLocation(m_program, samplerName);
			gl.uniform1i(loc, i);
		}

		for (int testPass = 0; testPass < numTestPasses(); ++testPass)
		{
			// Initialize texture data at the beginning of each test pass
			initTextureData();

			// Perform rendering passes
			for (int renderPass = 0; renderPass < numRenderPasses(); ++renderPass)
			{
				// Setup render pass
				setupRenderPass(testPass, renderPass);

				// Render a set of primitives that cover each pixel of the framebuffer exactly once
				render();

				// If a TextureBarrier is needed insert it here
				if (needsBarrier())
					gl.textureBarrier();
			}

			// Update reference data after actual rendering to avoid bubbles
			for (int renderPass = 0; renderPass < numRenderPasses(); ++renderPass)
			{
				// Setup render pass
				setupRenderPass(testPass, renderPass);

				// Update reference data
				updateTextureData();
			}

			// Verify results at the end of each test pass
			if (!verifyTextureData())
			{
				TCU_FAIL("Failed to validate rendering results");
			}
		}

		gl.useProgram(0);

		// Test case passed
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

protected:
	enum
	{
		NUM_TEXTURES = 8,

		GRID_SIZE	  = 64,
		TRIANGLE_COUNT = GRID_SIZE * GRID_SIZE * 2,
		VERTEX_COUNT   = TRIANGLE_COUNT * 3,
	};

	GLuint  m_program;
	GLuint  m_vao;
	GLuint  m_vbo;
	GLuint  m_fbo;
	GLuint  m_tex[NUM_TEXTURES];
	GLuint  m_width, m_height;
	GLuint* m_reference[NUM_TEXTURES];
	GLuint* m_actual;
};

/*
 Base class of the rendering tests which use a fragment shader performing
 reads and writes from/to disjoint blocks of texels within a single rendering
 pass. The skeleton of these tests is as follows:

 * Using the basic outline above test that reads and writes from/to
 disjoint sets of texels work as expected. Use the following fragment
 shader as a template:

 uniform int blockSize;
 uniform int modulo;
 uniform sampler2D texture[N];
 out vec4 output[N];

 void main() {
 ivec2 texelCoord = ivec2(gl_FragCoord.xy);
 ivec2 blockCoord = texelCoord / blockSize;
 ivec2 xOffset = ivec2(blockSize, 0);
 ivec2 yOffset = ivec2(0, blockSize);

 if (((blockCoord.x + blockCoord.y) % 2) == modulo) {
 for (int i = 0; i < N; ++i) {
 output[i] = function(
 texelFetch(texture[i], texelCoord + xOffset, 0),
 texelFetch(texture[i], texelCoord - xOffset, 0),
 texelFetch(texture[i], texelCoord + yOffset, 0),
 texelFetch(texture[i], texelCoord - yOffset, 0)
 );
 }
 } else {
 discard;
 }
 }

 Where "blockSize" is the size of the disjoint rectangular sets of texels,
 "modulo" should be either zero or one (depending whether even or odd
 blocks should be fetched/written), and "function" is an arbitrary
 function of its parameters.
 */
class TextureBarrierTexelBlocksBase : public TextureBarrierBasicOutline
{
protected:
	TextureBarrierTexelBlocksBase(deqp::Context& context, TextureBarrierTests::API api, const char* name,
								  const char* description)
		: TextureBarrierBasicOutline(context, api, name, description)
		, m_blockSize(-1)
		, m_modulo(-1)
		, m_blockSizeLoc(0)
		, m_moduloLoc(0)
	{
	}

	/* Actual fragment shader source based on the provided template. */
	virtual const char* fsh()
	{
		return "#version 400 core\n"
			   "#define NUM_TEXTURES 8\n"
			   "uniform int blockSize;\n"
			   "uniform int modulo;\n"
			   "uniform usampler2D texInput[NUM_TEXTURES];\n"
			   "out uvec4 fragOutput[NUM_TEXTURES];\n"
			   "uvec4 func(uvec4 t0, uvec4 t1, uvec4 t2, uvec4 t3) {\n"
			   "   return t0 + t1 + t2 + t3;\n"
			   "}\n"
			   "void main() {\n"
			   "    ivec2 texelCoord = ivec2(gl_FragCoord.xy);\n"
			   "    ivec2 blockCoord = texelCoord / blockSize;\n"
			   "    ivec2 xOffset = ivec2(blockSize, 0);\n"
			   "    ivec2 yOffset = ivec2(0, blockSize);\n"
			   "    if (((blockCoord.x + blockCoord.y) % 2) == modulo) {\n"
			   "        for (int i = 0; i < NUM_TEXTURES; ++i) {\n"
			   "            fragOutput[i] = func(texelFetch(texInput[i], texelCoord + xOffset, 0),\n"
			   "                                 texelFetch(texInput[i], texelCoord - xOffset, 0),\n"
			   "                                 texelFetch(texInput[i], texelCoord + yOffset, 0),\n"
			   "                                 texelFetch(texInput[i], texelCoord - yOffset, 0));\n"
			   "        }\n"
			   "    } else {\n"
			   "        discard;\n"
			   "    }\n"
			   "}";
	}

	/* CPU code equivalent to the fragment shader to update reference data. */
	virtual void updateTextureData()
	{
		for (GLuint x = 0; x < m_width; ++x)
			for (GLuint y = 0; y < m_height; ++y)
			{
				GLuint blockX = x / m_blockSize;
				GLuint blockY = y / m_blockSize;

				if ((static_cast<int>((blockX + blockY) % 2)) == m_modulo)
				{
					for (GLuint i = 0; i < NUM_TEXTURES; ++i)
					{
						texel(m_reference[i], x, y) =
							texel(m_reference[i], x + m_blockSize, y) + texel(m_reference[i], x - m_blockSize, y) +
							texel(m_reference[i], x, y + m_blockSize) + texel(m_reference[i], x, y - m_blockSize);
					}
				}
			}
	}

	/* Render pass setup code. Updates uniforms used by the fragment shader and
	 member variables used by the reference data update code. */
	virtual void setupRenderPass(int testPass, int renderPass)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if ((testPass == 0) && (renderPass == 0))
		{
			// Get the uniform locations in the first pass, reuse it afterwards
			m_blockSizeLoc = gl.getUniformLocation(m_program, "blockSize");
			m_moduloLoc	= gl.getUniformLocation(m_program, "modulo");
		}

		// Update block size if changed
		int newBlockSize = getBlockSize(testPass, renderPass);
		if (newBlockSize != m_blockSize)
		{
			m_blockSize = newBlockSize;
			gl.uniform1i(m_blockSizeLoc, m_blockSize);
		}

		// Update modulo if changed
		int newModulo = getModulo(testPass, renderPass);
		if (newModulo != m_modulo)
		{
			m_modulo = newModulo;
			gl.uniform1i(m_moduloLoc, m_modulo);
		}
	}

	/* Returns the block size to be used in the specified pass. */
	virtual int getBlockSize(int testPass, int renderPass) = 0;

	/* Returns the modulo value to be used in the specified pass. */
	virtual int getModulo(int testPass, int renderPass) = 0;

private:
	int   m_blockSize;
	int   m_modulo;
	GLint m_blockSizeLoc;
	GLint m_moduloLoc;
};

/*
 Test case #1: Disjoint texels

 * Using the basic outline above test that reads and writes from/to
 disjoint sets of texels work as expected.

 * Repeat the above test case with various values for blockSize (including a
 block size of one).

 * Repeat the actual rendering pass multiple times within a single test
 using a fixed value for "blockSize" and "modulo". Because the set of
 read and written texels stays disjoint the results should still be well
 defined even without the use of any synchronization primitive.
 */
class TextureBarrierDisjointTexels : public TextureBarrierTexelBlocksBase
{
public:
	TextureBarrierDisjointTexels(deqp::Context& context, TextureBarrierTests::API api, const char* name)
		: TextureBarrierTexelBlocksBase(
			  context, api, name,
			  "Using the basic outline test that reads and writes from/to disjoint sets of texels work as expected. "
			  "Repeat the test with multiple different block size values (including a block size of one). "
			  "Repeat the actual rendering pass multiple times within a single test using a fixed value for "
			  "blockSize and modulo. Because the set of read and written texels stays disjoint the result "
			  "should still be well defined even without the use of any synchronization primitive.")
	{
	}

	/* Perform multiple test passes, one for each blockSize value. */
	virtual int numTestPasses()
	{
		return 16;
	}

	/* Perform multiple render passes. As the same blockSize and modulo value is used between render passes the rendering
	 results should still stay well defined. */
	virtual int numRenderPasses()
	{
		return 5;
	}

	/* No need for a texture barrier as reads and writes happen from/to disjoint set of texels within a single test pass. */
	virtual bool needsBarrier()
	{
		return false;
	}

	/* Try different values for blockSize, but the value must stay constant within a single test pass. */
	virtual int getBlockSize(int testPass, int renderPass)
	{
		(void)renderPass;
		return testPass + 1;
	}

	/* Use a fixed value for modulo. */
	virtual int getModulo(int testPass, int renderPass)
	{
		(void)testPass;
		(void)renderPass;
		return 0;
	}
};

/*
 Test case #2: Overlapping texels (with texture barrier)

 * Using the basic outline above test that reads and writes from/to
 disjoint sets of texels work as expected.

 * Repeat the actual rendering pass multiple times within a single test,
 but this time use different values for "blockSize" and "modulo" and
 call TextureBarrier between subsequent rendering passes to ensure
 well defined results.
 */
class TextureBarrierOverlappingTexels : public TextureBarrierTexelBlocksBase
{
public:
	TextureBarrierOverlappingTexels(deqp::Context& context, TextureBarrierTests::API api, const char* name)
		: TextureBarrierTexelBlocksBase(
			  context, api, name,
			  "Using the basic outline test that reads and writes from/to overlapping sets of texels work "
			  "as expected if there is a call to TextureBarrier between subsequent rendering passes. Test "
			  "this by using different values for blockSize and modulo for each rendering pass.")
	{
	}

	/* A single test pass is sufficient. */
	virtual int numTestPasses()
	{
		return 1;
	}

	/* Perform several render passes to provoke a lot of overlap between read and written texel blocks. */
	virtual int numRenderPasses()
	{
		return 42;
	}

	/* We need texture barriers between render passes as reads and writes in different render passes do overlap. */
	virtual bool needsBarrier()
	{
		return true;
	}

	/* Use a pseudo-random blockSize for each render pass. */
	virtual int getBlockSize(int testPass, int renderPass)
	{
		(void)testPass;
		return (5 + renderPass * 3) % 7 + 1;
	}

	/* Use a pseudo-random modulo for each render pass. */
	virtual int getModulo(int testPass, int renderPass)
	{
		(void)testPass;
		return (renderPass * 3) % 2;
	}
};

/*
 Base class of the rendering tests which use a fragment shader performing a
 single read and write of each texel. The skeleton of these tests is as follows:

 * Using the basic outline above test that a single read and write of each
 texel, where the read is in the fragment shader invocation that writes
 the same texel, works as expected. Use the following fragment shader as
 a template:

 uniform sampler2D texture[N];
 out vec4 output[N];

 void main() {
 ivec2 texelCoord = ivec2(gl_FragCoord.xy);

 for (int i = 0; i < N; ++i) {
 output[i] = function(texelFetch(texture[i], texelCoord, 0);
 }
 }

 Where "function" is an arbitrary function of its parameter.
 */
class TextureBarrierSameTexelRWBase : public TextureBarrierBasicOutline
{
protected:
	TextureBarrierSameTexelRWBase(deqp::Context& context, TextureBarrierTests::API api, const char* name,
								  const char* description)
		: TextureBarrierBasicOutline(context, api, name, description)
	{
	}

	/* Actual fragment shader source based on the provided template. */
	virtual const char* fsh()
	{
		return "#version 400 core\n"
			   "#define NUM_TEXTURES 8\n"
			   "uniform usampler2D texInput[NUM_TEXTURES];\n"
			   "out uvec4 fragOutput[NUM_TEXTURES];\n"
			   "uvec4 func(uvec4 t) {\n"
			   "    return t + 1;\n"
			   "}\n"
			   "void main() {\n"
			   "    ivec2 texelCoord = ivec2(gl_FragCoord.xy);\n"
			   "    for (int i = 0; i < NUM_TEXTURES; ++i) {\n"
			   "        fragOutput[i] = func(texelFetch(texInput[i], texelCoord, 0));\n"
			   "    }\n"
			   "}";
	}

	/* CPU code equivalent to the fragment shader to update reference data. */
	virtual void updateTextureData()
	{
		for (GLuint x = 0; x < m_width; ++x)
			for (GLuint y = 0; y < m_height; ++y)
			{
				for (GLuint i = 0; i < NUM_TEXTURES; ++i)
				{
					texel(m_reference[i], x, y)++;
				}
			}
	}

	/* The fragment shader used by these tests doesn't have any parameters, thus no need for render pass setup code. */
	virtual void setupRenderPass(int testPass, int renderPass)
	{
		(void)testPass;
		(void)renderPass;
	}
};

/*
 Test case #3: Single read and write of the same texel

 * Using the basic outline above test that a single read and write of each
 texel, where the read is in the fragment shader invocation that writes
 the same texel, works as expected.
 */
class TextureBarrierSameTexelRW : public TextureBarrierSameTexelRWBase
{
public:
	TextureBarrierSameTexelRW(deqp::Context& context, TextureBarrierTests::API api, const char* name)
		: TextureBarrierSameTexelRWBase(
			  context, api, name,
			  "Using the basic outline tests that a single read and write of each texel, where the read "
			  "is in the fragment shader invocation that writes the same texel, works as expected.")
	{
	}

	/* A single test pass is sufficient. */
	virtual int numTestPasses()
	{
		return 1;
	}

	/* Well defined behavior is guaranteed only in case of a single pass. */
	virtual int numRenderPasses()
	{
		return 1;
	}

	/* A single read and write of the same texel doesn't require a texture barrier. */
	virtual bool needsBarrier()
	{
		return false;
	}
};

/*
 Test case #4: Multipass read and write of the same texel (with texture barrier)

 * Using the basic outline above test that a single read and write of each
 texel, where the read is in the fragment shader invocation that writes
 the same texel, works as expected.

 * Repeat the above test case but this time perform multiple iterations
 of the actual rendering pass and use a call to TextureBarrier between
 them to ensure consistency.
 */
class TextureBarrierSameTexelRWMultipass : public TextureBarrierSameTexelRWBase
{
public:
	TextureBarrierSameTexelRWMultipass(deqp::Context& context, TextureBarrierTests::API api, const char* name)
		: TextureBarrierSameTexelRWBase(
			  context, api, name,
			  "Using the basic outline tests that multiple reads and writes of each texel, where the read "
			  "is in the fragment shader invocation that writes the same texel, works as expected if there "
			  "is a call to TextureBarrier between each subsequent read-after-write.")
	{
	}

	/* A single test pass is sufficient. */
	virtual int numTestPasses()
	{
		return 1;
	}

	/* Perform several render passes to provoke read-after-write hazards. */
	virtual int numRenderPasses()
	{
		return 42;
	}

	/* We need to use texture barriers in between rendering passes to avoid read-after-write hazards. */
	virtual bool needsBarrier()
	{
		return true;
	}
};

const char* apiToTestName(TextureBarrierTests::API api)
{
	switch (api)
	{
	case TextureBarrierTests::API_GL_45core:
		return "texture_barrier";
	case TextureBarrierTests::API_GL_ARB_texture_barrier:
		return "texture_barrier_ARB";
	}
	DE_ASSERT(0);
	return "";
}

/** Constructor.
 *
 *  @param context Rendering context.
 *  @param api     API to test (core vs ARB extension)
 **/
TextureBarrierTests::TextureBarrierTests(deqp::Context& context, API api)
	: TestCaseGroup(context, apiToTestName(api), "Verifies \"texture_barrier\" functionality"), m_api(api)
{
	/* Left blank on purpose */
}

/** Destructor.
 *
 **/
TextureBarrierTests::~TextureBarrierTests()
{
}

/** Initializes the texture_barrier test group.
 *
 **/
void TextureBarrierTests::init(void)
{
	addChild(new TextureBarrierDisjointTexels(m_context, m_api, "disjoint-texels"));
	addChild(new TextureBarrierOverlappingTexels(m_context, m_api, "overlapping-texels"));
	addChild(new TextureBarrierSameTexelRW(m_context, m_api, "same-texel-rw"));
	addChild(new TextureBarrierSameTexelRWMultipass(m_context, m_api, "same-texel-rw-multipass"));
}
} /* glcts namespace */
