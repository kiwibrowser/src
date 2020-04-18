// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Context.h: Defines the Context class, managing all GL state and performing
// rendering operations. It is the GLES2 specific implementation of EGLContext.

#ifndef LIBGLESV2_CONTEXT_H_
#define LIBGLESV2_CONTEXT_H_

#include "ResourceManager.h"
#include "Buffer.h"
#include "libEGL/Context.hpp"
#include "common/NameSpace.hpp"
#include "common/Object.hpp"
#include "common/Image.hpp"
#include "Renderer/Sampler.hpp"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#include <map>
#include <string>

namespace egl
{
class Display;
class Config;
}

namespace es2
{
struct TranslatedAttribute;
struct TranslatedIndexData;

class Device;
class Shader;
class Program;
class Texture;
class Texture2D;
class Texture3D;
class Texture2DArray;
class TextureCubeMap;
class Texture2DRect;
class TextureExternal;
class Framebuffer;
class Renderbuffer;
class RenderbufferStorage;
class Colorbuffer;
class Depthbuffer;
class StreamingIndexBuffer;
class Stencilbuffer;
class DepthStencilbuffer;
class VertexDataManager;
class IndexDataManager;
class Fence;
class FenceSync;
class Query;
class Sampler;
class VertexArray;
class TransformFeedback;

enum
{
	MAX_VERTEX_ATTRIBS = sw::MAX_VERTEX_INPUTS,
	MAX_UNIFORM_VECTORS = 256,   // Device limit
	MAX_VERTEX_UNIFORM_VECTORS = sw::VERTEX_UNIFORM_VECTORS - 3,   // Reserve space for gl_DepthRange
	MAX_VARYING_VECTORS = MIN(sw::MAX_FRAGMENT_INPUTS, sw::MAX_VERTEX_OUTPUTS),
	MAX_TEXTURE_IMAGE_UNITS = sw::TEXTURE_IMAGE_UNITS,
	MAX_VERTEX_TEXTURE_IMAGE_UNITS = sw::VERTEX_TEXTURE_IMAGE_UNITS,
	MAX_COMBINED_TEXTURE_IMAGE_UNITS = MAX_TEXTURE_IMAGE_UNITS + MAX_VERTEX_TEXTURE_IMAGE_UNITS,
	MAX_FRAGMENT_UNIFORM_VECTORS = sw::FRAGMENT_UNIFORM_VECTORS - 3,    // Reserve space for gl_DepthRange
	MAX_ELEMENT_INDEX = 0x7FFFFFFF,
	MAX_ELEMENTS_INDICES = 0x7FFFFFFF,
	MAX_ELEMENTS_VERTICES = 0x7FFFFFFF,
	MAX_VERTEX_OUTPUT_VECTORS = 16,
	MAX_FRAGMENT_INPUT_VECTORS = 15,
	MIN_PROGRAM_TEXEL_OFFSET = sw::MIN_PROGRAM_TEXEL_OFFSET,
	MAX_PROGRAM_TEXEL_OFFSET = sw::MAX_PROGRAM_TEXEL_OFFSET,
	MAX_TEXTURE_LOD_BIAS = sw::MAX_TEXTURE_LOD,
	MAX_DRAW_BUFFERS = sw::RENDERTARGETS,
	MAX_COLOR_ATTACHMENTS = MAX(MAX_DRAW_BUFFERS, 8),
	MAX_FRAGMENT_UNIFORM_BLOCKS = sw::MAX_FRAGMENT_UNIFORM_BLOCKS,
	MAX_VERTEX_UNIFORM_BLOCKS = sw::MAX_VERTEX_UNIFORM_BLOCKS,
	MAX_FRAGMENT_UNIFORM_COMPONENTS = sw::FRAGMENT_UNIFORM_VECTORS * 4,
	MAX_VERTEX_UNIFORM_COMPONENTS = sw::VERTEX_UNIFORM_VECTORS * 4,
	MAX_UNIFORM_BLOCK_SIZE = sw::MAX_UNIFORM_BLOCK_SIZE,
	MAX_FRAGMENT_UNIFORM_BLOCKS_COMPONENTS = sw::MAX_FRAGMENT_UNIFORM_BLOCKS * MAX_UNIFORM_BLOCK_SIZE / 4,
	MAX_VERTEX_UNIFORM_BLOCKS_COMPONENTS = MAX_VERTEX_UNIFORM_BLOCKS * MAX_UNIFORM_BLOCK_SIZE / 4,
	MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS = MAX_FRAGMENT_UNIFORM_BLOCKS_COMPONENTS + MAX_FRAGMENT_UNIFORM_COMPONENTS,
	MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS = MAX_VERTEX_UNIFORM_BLOCKS_COMPONENTS + MAX_VERTEX_UNIFORM_COMPONENTS,
	MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS = 4,
	MAX_UNIFORM_BUFFER_BINDINGS = sw::MAX_UNIFORM_BUFFER_BINDINGS,
	UNIFORM_BUFFER_OFFSET_ALIGNMENT = 4,
	NUM_PROGRAM_BINARY_FORMATS = 0,
};

const GLenum compressedTextureFormats[] =
{
	GL_ETC1_RGB8_OES,
	GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,
	GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,
#if (GL_ES_VERSION_3_0)
	GL_COMPRESSED_R11_EAC,
	GL_COMPRESSED_SIGNED_R11_EAC,
	GL_COMPRESSED_RG11_EAC,
	GL_COMPRESSED_SIGNED_RG11_EAC,
	GL_COMPRESSED_RGB8_ETC2,
	GL_COMPRESSED_SRGB8_ETC2,
	GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
	GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
	GL_COMPRESSED_RGBA8_ETC2_EAC,
	GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
#if (ASTC_SUPPORT)
	GL_COMPRESSED_RGBA_ASTC_4x4_KHR,
	GL_COMPRESSED_RGBA_ASTC_5x4_KHR,
	GL_COMPRESSED_RGBA_ASTC_5x5_KHR,
	GL_COMPRESSED_RGBA_ASTC_6x5_KHR,
	GL_COMPRESSED_RGBA_ASTC_6x6_KHR,
	GL_COMPRESSED_RGBA_ASTC_8x5_KHR,
	GL_COMPRESSED_RGBA_ASTC_8x6_KHR,
	GL_COMPRESSED_RGBA_ASTC_8x8_KHR,
	GL_COMPRESSED_RGBA_ASTC_10x5_KHR,
	GL_COMPRESSED_RGBA_ASTC_10x6_KHR,
	GL_COMPRESSED_RGBA_ASTC_10x8_KHR,
	GL_COMPRESSED_RGBA_ASTC_10x10_KHR,
	GL_COMPRESSED_RGBA_ASTC_12x10_KHR,
	GL_COMPRESSED_RGBA_ASTC_12x12_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR,
	GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,
#endif // ASTC_SUPPORT
#endif // GL_ES_VERSION_3_0
};

const GLenum GL_TEXTURE_FILTERING_HINT_CHROMIUM = 0x8AF0;

const GLint NUM_COMPRESSED_TEXTURE_FORMATS = sizeof(compressedTextureFormats) / sizeof(compressedTextureFormats[0]);

const GLint multisampleCount[] = {4, 2, 1};
const GLint NUM_MULTISAMPLE_COUNTS = sizeof(multisampleCount) / sizeof(multisampleCount[0]);
const GLint IMPLEMENTATION_MAX_SAMPLES = multisampleCount[0];

const float ALIASED_LINE_WIDTH_RANGE_MIN = 1.0f;
const float ALIASED_LINE_WIDTH_RANGE_MAX = 1.0f;
const float ALIASED_POINT_SIZE_RANGE_MIN = 0.125f;
const float ALIASED_POINT_SIZE_RANGE_MAX = 8192.0f;
const float MAX_TEXTURE_MAX_ANISOTROPY = 16.0f;

enum QueryType
{
	QUERY_ANY_SAMPLES_PASSED,
	QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE,
	QUERY_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,

	QUERY_TYPE_COUNT
};

struct Color
{
	float red;
	float green;
	float blue;
	float alpha;
};

// Helper structure describing a single vertex attribute
class VertexAttribute
{
public:
	VertexAttribute() : mType(GL_FLOAT), mSize(4), mNormalized(false), mPureInteger(false), mStride(0), mDivisor(0), mPointer(nullptr), mArrayEnabled(false)
	{
		mCurrentValue[0].f = 0.0f;
		mCurrentValue[1].f = 0.0f;
		mCurrentValue[2].f = 0.0f;
		mCurrentValue[3].f = 1.0f;
		mCurrentValueType = GL_FLOAT;
	}

	int typeSize() const
	{
		switch(mType)
		{
		case GL_BYTE:           return mSize * sizeof(GLbyte);
		case GL_UNSIGNED_BYTE:  return mSize * sizeof(GLubyte);
		case GL_SHORT:          return mSize * sizeof(GLshort);
		case GL_UNSIGNED_SHORT: return mSize * sizeof(GLushort);
		case GL_INT:            return mSize * sizeof(GLint);
		case GL_UNSIGNED_INT:   return mSize * sizeof(GLuint);
		case GL_FIXED:          return mSize * sizeof(GLfixed);
		case GL_FLOAT:          return mSize * sizeof(GLfloat);
		case GL_HALF_FLOAT_OES:
		case GL_HALF_FLOAT:     return mSize * sizeof(GLhalf);
		case GL_INT_2_10_10_10_REV:          return sizeof(GLint);
		case GL_UNSIGNED_INT_2_10_10_10_REV: return sizeof(GLuint);
		default: UNREACHABLE(mType); return mSize * sizeof(GLfloat);
		}
	}

	GLenum currentValueType() const
	{
		return mCurrentValueType;
	}

	GLsizei stride() const
	{
		return mStride ? mStride : typeSize();
	}

	inline float getCurrentValueBitsAsFloat(int i) const
	{
		return mCurrentValue[i].f;
	}

	inline float getCurrentValueF(int i) const
	{
		switch(mCurrentValueType)
		{
		case GL_FLOAT:        return mCurrentValue[i].f;
		case GL_INT:          return static_cast<float>(mCurrentValue[i].i);
		case GL_UNSIGNED_INT: return static_cast<float>(mCurrentValue[i].ui);
		default: UNREACHABLE(mCurrentValueType); return mCurrentValue[i].f;
		}
	}

	inline GLint getCurrentValueI(int i) const
	{
		switch(mCurrentValueType)
		{
		case GL_FLOAT:        return static_cast<GLint>(mCurrentValue[i].f);
		case GL_INT:          return mCurrentValue[i].i;
		case GL_UNSIGNED_INT: return static_cast<GLint>(mCurrentValue[i].ui);
		default: UNREACHABLE(mCurrentValueType); return mCurrentValue[i].i;
		}
	}

	inline GLuint getCurrentValueUI(int i) const
	{
		switch(mCurrentValueType)
		{
		case GL_FLOAT:        return static_cast<GLuint>(mCurrentValue[i].f);
		case GL_INT:          return static_cast<GLuint>(mCurrentValue[i].i);
		case GL_UNSIGNED_INT: return mCurrentValue[i].ui;
		default: UNREACHABLE(mCurrentValueType); return mCurrentValue[i].ui;
		}
	}

	inline void setCurrentValue(const GLfloat *values)
	{
		mCurrentValue[0].f = values[0];
		mCurrentValue[1].f = values[1];
		mCurrentValue[2].f = values[2];
		mCurrentValue[3].f = values[3];
		mCurrentValueType = GL_FLOAT;
	}

	inline void setCurrentValue(const GLint *values)
	{
		mCurrentValue[0].i = values[0];
		mCurrentValue[1].i = values[1];
		mCurrentValue[2].i = values[2];
		mCurrentValue[3].i = values[3];
		mCurrentValueType = GL_INT;
	}

	inline void setCurrentValue(const GLuint *values)
	{
		mCurrentValue[0].ui = values[0];
		mCurrentValue[1].ui = values[1];
		mCurrentValue[2].ui = values[2];
		mCurrentValue[3].ui = values[3];
		mCurrentValueType = GL_UNSIGNED_INT;
	}

	// From glVertexAttribPointer
	GLenum mType;
	GLint mSize;
	bool mNormalized;
	bool mPureInteger;
	GLsizei mStride;   // 0 means natural stride
	GLuint mDivisor;   // From glVertexAttribDivisor

	union
	{
		const void *mPointer;
		intptr_t mOffset;
	};

	gl::BindingPointer<Buffer> mBoundBuffer;   // Captured when glVertexAttribPointer is called.

	bool mArrayEnabled;   // From glEnable/DisableVertexAttribArray

private:
	union ValueUnion
	{
		float f;
		GLint i;
		GLuint ui;
	};

	ValueUnion mCurrentValue[4];   // From glVertexAttrib
	GLenum mCurrentValueType;
};

typedef VertexAttribute VertexAttributeArray[MAX_VERTEX_ATTRIBS];

// Helper structure to store all raw state
struct State
{
	Color colorClearValue;
	GLclampf depthClearValue;
	int stencilClearValue;

	bool cullFaceEnabled;
	GLenum cullMode;
	GLenum frontFace;
	bool depthTestEnabled;
	GLenum depthFunc;
	bool blendEnabled;
	GLenum sourceBlendRGB;
	GLenum destBlendRGB;
	GLenum sourceBlendAlpha;
	GLenum destBlendAlpha;
	GLenum blendEquationRGB;
	GLenum blendEquationAlpha;
	Color blendColor;
	bool stencilTestEnabled;
	GLenum stencilFunc;
	GLint stencilRef;
	GLuint stencilMask;
	GLenum stencilFail;
	GLenum stencilPassDepthFail;
	GLenum stencilPassDepthPass;
	GLuint stencilWritemask;
	GLenum stencilBackFunc;
	GLint stencilBackRef;
	GLuint stencilBackMask;
	GLenum stencilBackFail;
	GLenum stencilBackPassDepthFail;
	GLenum stencilBackPassDepthPass;
	GLuint stencilBackWritemask;
	bool polygonOffsetFillEnabled;
	GLfloat polygonOffsetFactor;
	GLfloat polygonOffsetUnits;
	bool sampleAlphaToCoverageEnabled;
	bool sampleCoverageEnabled;
	GLclampf sampleCoverageValue;
	bool sampleCoverageInvert;
	bool scissorTestEnabled;
	bool ditherEnabled;
	bool primitiveRestartFixedIndexEnabled;
	bool rasterizerDiscardEnabled;
	bool colorLogicOpEnabled;
	GLenum logicalOperation;

	GLfloat lineWidth;

	GLenum generateMipmapHint;
	GLenum fragmentShaderDerivativeHint;
	GLenum textureFilteringHint;

	GLint viewportX;
	GLint viewportY;
	GLsizei viewportWidth;
	GLsizei viewportHeight;
	float zNear;
	float zFar;

	GLint scissorX;
	GLint scissorY;
	GLsizei scissorWidth;
	GLsizei scissorHeight;

	bool colorMaskRed;
	bool colorMaskGreen;
	bool colorMaskBlue;
	bool colorMaskAlpha;
	bool depthMask;

	unsigned int activeSampler;   // Active texture unit selector - GL_TEXTURE0
	gl::BindingPointer<Buffer> arrayBuffer;
	gl::BindingPointer<Buffer> copyReadBuffer;
	gl::BindingPointer<Buffer> copyWriteBuffer;
	gl::BindingPointer<Buffer> pixelPackBuffer;
	gl::BindingPointer<Buffer> pixelUnpackBuffer;
	gl::BindingPointer<Buffer> genericUniformBuffer;
	BufferBinding uniformBuffers[MAX_UNIFORM_BUFFER_BINDINGS];

	GLuint readFramebuffer;
	GLuint drawFramebuffer;
	gl::BindingPointer<Renderbuffer> renderbuffer;
	GLuint currentProgram;
	GLuint vertexArray;
	GLuint transformFeedback;
	gl::BindingPointer<Sampler> sampler[MAX_COMBINED_TEXTURE_IMAGE_UNITS];

	VertexAttribute vertexAttribute[MAX_VERTEX_ATTRIBS];
	gl::BindingPointer<Texture> samplerTexture[TEXTURE_TYPE_COUNT][MAX_COMBINED_TEXTURE_IMAGE_UNITS];
	gl::BindingPointer<Query> activeQuery[QUERY_TYPE_COUNT];

	gl::PixelStorageModes unpackParameters;
	gl::PixelStorageModes packParameters;
};

class [[clang::lto_visibility_public]] Context : public egl::Context
{
public:
	Context(egl::Display *display, const Context *shareContext, EGLint clientVersion, const egl::Config *config);

	void makeCurrent(gl::Surface *surface) override;
	EGLint getClientVersion() const override;
	EGLint getConfigID() const override;

	void markAllStateDirty();

	// State manipulation
	void setClearColor(float red, float green, float blue, float alpha);
	void setClearDepth(float depth);
	void setClearStencil(int stencil);

	void setCullFaceEnabled(bool enabled);
	bool isCullFaceEnabled() const;
	void setCullMode(GLenum mode);
	void setFrontFace(GLenum front);

	void setDepthTestEnabled(bool enabled);
	bool isDepthTestEnabled() const;
	void setDepthFunc(GLenum depthFunc);
	void setDepthRange(float zNear, float zFar);

	void setBlendEnabled(bool enabled);
	bool isBlendEnabled() const;
	void setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha);
	void setBlendColor(float red, float green, float blue, float alpha);
	void setBlendEquation(GLenum rgbEquation, GLenum alphaEquation);

	void setStencilTestEnabled(bool enabled);
	bool isStencilTestEnabled() const;
	void setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask);
	void setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask);
	void setStencilWritemask(GLuint stencilWritemask);
	void setStencilBackWritemask(GLuint stencilBackWritemask);
	void setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass);
	void setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass);

	void setPolygonOffsetFillEnabled(bool enabled);
	bool isPolygonOffsetFillEnabled() const;
	void setPolygonOffsetParams(GLfloat factor, GLfloat units);

	void setSampleAlphaToCoverageEnabled(bool enabled);
	bool isSampleAlphaToCoverageEnabled() const;
	void setSampleCoverageEnabled(bool enabled);
	bool isSampleCoverageEnabled() const;
	void setSampleCoverageParams(GLclampf value, bool invert);

	void setDitherEnabled(bool enabled);
	bool isDitherEnabled() const;

	void setPrimitiveRestartFixedIndexEnabled(bool enabled);
	bool isPrimitiveRestartFixedIndexEnabled() const;

	void setRasterizerDiscardEnabled(bool enabled);
	bool isRasterizerDiscardEnabled() const;

	void setLineWidth(GLfloat width);

	void setGenerateMipmapHint(GLenum hint);
	void setFragmentShaderDerivativeHint(GLenum hint);
	void setTextureFilteringHint(GLenum hint);

	void setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height);

	void setScissorTestEnabled(bool enabled);
	bool isScissorTestEnabled() const;
	void setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height);

	void setColorMask(bool red, bool green, bool blue, bool alpha);
	unsigned int getColorMask() const;
	void setDepthMask(bool mask);

	void setActiveSampler(unsigned int active);

	GLuint getReadFramebufferName() const;
	GLuint getDrawFramebufferName() const;
	GLuint getRenderbufferName() const;

	void setFramebufferReadBuffer(GLenum buf);
	void setFramebufferDrawBuffers(GLsizei n, const GLenum *bufs);

	GLuint getActiveQuery(GLenum target) const;

	GLuint getArrayBufferName() const;
	GLuint getElementArrayBufferName() const;

	void setVertexAttribArrayEnabled(unsigned int attribNum, bool enabled);
	void setVertexAttribDivisor(unsigned int attribNum, GLuint divisor);
	const VertexAttribute &getVertexAttribState(unsigned int attribNum) const;
	void setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type,
	                          bool normalized, bool pureInteger, GLsizei stride, const void *pointer);
	const void *getVertexAttribPointer(unsigned int attribNum) const;

	const VertexAttributeArray &getVertexArrayAttributes();
	// Context attribute current values can be queried independently from VAO current values
	const VertexAttributeArray &getCurrentVertexAttributes();

	void setUnpackAlignment(GLint alignment);
	void setUnpackRowLength(GLint rowLength);
	void setUnpackImageHeight(GLint imageHeight);
	void setUnpackSkipPixels(GLint skipPixels);
	void setUnpackSkipRows(GLint skipRows);
	void setUnpackSkipImages(GLint skipImages);
	const gl::PixelStorageModes &getUnpackParameters() const;

	void setPackAlignment(GLint alignment);
	void setPackRowLength(GLint rowLength);
	void setPackSkipPixels(GLint skipPixels);
	void setPackSkipRows(GLint skipRows);

	// These create and destroy methods are merely pass-throughs to
	// ResourceManager, which owns these object types
	GLuint createBuffer();
	GLuint createShader(GLenum type);
	GLuint createProgram();
	GLuint createTexture();
	GLuint createRenderbuffer();
	GLuint createSampler();
	GLsync createFenceSync(GLenum condition, GLbitfield flags);

	void deleteBuffer(GLuint buffer);
	void deleteShader(GLuint shader);
	void deleteProgram(GLuint program);
	void deleteTexture(GLuint texture);
	void deleteRenderbuffer(GLuint renderbuffer);
	void deleteSampler(GLuint sampler);
	void deleteFenceSync(GLsync fenceSync);

	// Framebuffers are owned by the Context, so these methods do not pass through
	GLuint createFramebuffer();
	void deleteFramebuffer(GLuint framebuffer);

	// Fences are owned by the Context
	GLuint createFence();
	void deleteFence(GLuint fence);

	// Queries are owned by the Context
	GLuint createQuery();
	void deleteQuery(GLuint query);

	// Vertex arrays are owned by the Context
	GLuint createVertexArray();
	void deleteVertexArray(GLuint array);

	// Transform feedbacks are owned by the Context
	GLuint createTransformFeedback();
	void deleteTransformFeedback(GLuint transformFeedback);

	void bindArrayBuffer(GLuint buffer);
	void bindElementArrayBuffer(GLuint buffer);
	void bindCopyReadBuffer(GLuint buffer);
	void bindCopyWriteBuffer(GLuint buffer);
	void bindPixelPackBuffer(GLuint buffer);
	void bindPixelUnpackBuffer(GLuint buffer);
	void bindTransformFeedbackBuffer(GLuint buffer);
	void bindTexture(TextureType type, GLuint texture);
	void bindReadFramebuffer(GLuint framebuffer);
	void bindDrawFramebuffer(GLuint framebuffer);
	void bindRenderbuffer(GLuint renderbuffer);
	void bindVertexArray(GLuint array);
	void bindGenericUniformBuffer(GLuint buffer);
	void bindIndexedUniformBuffer(GLuint buffer, GLuint index, GLintptr offset, GLsizeiptr size);
	void bindGenericTransformFeedbackBuffer(GLuint buffer);
	void bindIndexedTransformFeedbackBuffer(GLuint buffer, GLuint index, GLintptr offset, GLsizeiptr size);
	void bindTransformFeedback(GLuint transformFeedback);
	bool bindSampler(GLuint unit, GLuint sampler);
	void useProgram(GLuint program);

	void beginQuery(GLenum target, GLuint query);
	void endQuery(GLenum target);

	void setFramebufferZero(Framebuffer *framebuffer);

	void setRenderbufferStorage(RenderbufferStorage *renderbuffer);

	void setVertexAttrib(GLuint index, const GLfloat *values);
	void setVertexAttrib(GLuint index, const GLint *values);
	void setVertexAttrib(GLuint index, const GLuint *values);

	Buffer *getBuffer(GLuint handle) const;
	Fence *getFence(GLuint handle) const;
	FenceSync *getFenceSync(GLsync handle) const;
	Shader *getShader(GLuint handle) const;
	Program *getProgram(GLuint handle) const;
	virtual Texture *getTexture(GLuint handle) const;
	Framebuffer *getFramebuffer(GLuint handle) const;
	virtual Renderbuffer *getRenderbuffer(GLuint handle) const;
	Query *getQuery(GLuint handle) const;
	VertexArray *getVertexArray(GLuint array) const;
	VertexArray *getCurrentVertexArray() const;
	bool isVertexArray(GLuint array) const;
	TransformFeedback *getTransformFeedback(GLuint transformFeedback) const;
	bool isTransformFeedback(GLuint transformFeedback) const;
	TransformFeedback *getTransformFeedback() const;
	Sampler *getSampler(GLuint sampler) const;
	bool isSampler(GLuint sampler) const;

	Buffer *getArrayBuffer() const;
	Buffer *getElementArrayBuffer() const;
	Buffer *getCopyReadBuffer() const;
	Buffer *getCopyWriteBuffer() const;
	Buffer *getPixelPackBuffer() const;
	Buffer *getPixelUnpackBuffer() const;
	Buffer *getGenericUniformBuffer() const;
	GLsizei getRequiredBufferSize(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type) const;
	GLenum getPixels(const GLvoid **data, GLenum type, GLsizei imageSize) const;
	bool getBuffer(GLenum target, es2::Buffer **buffer) const;
	Program *getCurrentProgram() const;
	Texture2D *getTexture2D() const;
	Texture2D *getTexture2D(GLenum target) const;
	Texture3D *getTexture3D() const;
	Texture2DArray *getTexture2DArray() const;
	TextureCubeMap *getTextureCubeMap() const;
	Texture2DRect *getTexture2DRect() const;
	TextureExternal *getTextureExternal() const;
	Texture *getSamplerTexture(unsigned int sampler, TextureType type) const;
	Framebuffer *getReadFramebuffer() const;
	Framebuffer *getDrawFramebuffer() const;

	bool getFloatv(GLenum pname, GLfloat *params) const;
	template<typename T> bool getIntegerv(GLenum pname, T *params) const;
	bool getBooleanv(GLenum pname, GLboolean *params) const;
	template<typename T> bool getTransformFeedbackiv(GLuint index, GLenum pname, T *param) const;
	template<typename T> bool getUniformBufferiv(GLuint index, GLenum pname, T *param) const;
	void samplerParameteri(GLuint sampler, GLenum pname, GLint param);
	void samplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
	GLint getSamplerParameteri(GLuint sampler, GLenum pname);
	GLfloat getSamplerParameterf(GLuint sampler, GLenum pname);

	bool getQueryParameterInfo(GLenum pname, GLenum *type, unsigned int *numParams) const;

	bool hasZeroDivisor() const;

	void drawArrays(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount = 1);
	void drawElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount = 1);
	void blit(sw::Surface *source, const sw::SliceRect &sRect, sw::Surface *dest, const sw::SliceRect &dRect) override;
	void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei *bufSize, void* pixels);
	void clear(GLbitfield mask);
	void clearColorBuffer(GLint drawbuffer, const GLint *value);
	void clearColorBuffer(GLint drawbuffer, const GLuint *value);
	void clearColorBuffer(GLint drawbuffer, const GLfloat *value);
	void clearDepthBuffer(const GLfloat value);
	void clearStencilBuffer(const GLint value);
	void finish() override;
	void flush();

	void recordInvalidEnum();
	void recordInvalidValue();
	void recordInvalidOperation();
	void recordOutOfMemory();
	void recordInvalidFramebufferOperation();

	GLenum getError();

	static int getSupportedMultisampleCount(int requested);

	void blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
	                     GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
	                     GLbitfield mask, bool filter, bool allowPartialDepthStencilBlit);

	void bindTexImage(gl::Surface *surface) override;
	EGLenum validateSharedImage(EGLenum target, GLuint name, GLuint textureLevel) override;
	egl::Image *createSharedImage(EGLenum target, GLuint name, GLuint textureLevel) override;
	egl::Image *getSharedImage(GLeglImageOES image);

	Device *getDevice();

	const GLubyte *getExtensions(GLuint index, GLuint *numExt = nullptr) const;

private:
	~Context() override;

	void applyScissor(int width, int height);
	bool applyRenderTarget();
	void applyState(GLenum drawMode);
	GLenum applyVertexBuffer(GLint base, GLint first, GLsizei count, GLsizei instanceId);
	GLenum applyIndexBuffer(const void *indices, GLuint start, GLuint end, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);
	void applyShaders();
	void applyTextures();
	void applyTextures(sw::SamplerType type);
	void applyTexture(sw::SamplerType type, int sampler, Texture *texture);
	void clearColorBuffer(GLint drawbuffer, void *value, sw::Format format);

	void detachBuffer(GLuint buffer);
	void detachTexture(GLuint texture);
	void detachFramebuffer(GLuint framebuffer);
	void detachRenderbuffer(GLuint renderbuffer);
	void detachSampler(GLuint sampler);

	bool cullSkipsDraw(GLenum drawMode);
	bool isTriangleMode(GLenum drawMode);

	Query *createQuery(GLuint handle, GLenum type);

	const EGLint clientVersion;
	const egl::Config *const config;

	State mState;

	gl::BindingPointer<Texture2D> mTexture2DZero;
	gl::BindingPointer<Texture3D> mTexture3DZero;
	gl::BindingPointer<Texture2DArray> mTexture2DArrayZero;
	gl::BindingPointer<TextureCubeMap> mTextureCubeMapZero;
	gl::BindingPointer<Texture2DRect> mTexture2DRectZero;
	gl::BindingPointer<TextureExternal> mTextureExternalZero;

	gl::NameSpace<Framebuffer> mFramebufferNameSpace;
	gl::NameSpace<Fence, 0> mFenceNameSpace;
	gl::NameSpace<Query> mQueryNameSpace;
	gl::NameSpace<VertexArray> mVertexArrayNameSpace;
	gl::NameSpace<TransformFeedback> mTransformFeedbackNameSpace;

	VertexDataManager *mVertexDataManager;
	IndexDataManager *mIndexDataManager;

	// Recorded errors
	bool mInvalidEnum;
	bool mInvalidValue;
	bool mInvalidOperation;
	bool mOutOfMemory;
	bool mInvalidFramebufferOperation;

	bool mHasBeenCurrent;

	unsigned int mAppliedProgramSerial;

	// state caching flags
	bool mDepthStateDirty;
	bool mMaskStateDirty;
	bool mBlendStateDirty;
	bool mStencilStateDirty;
	bool mPolygonOffsetStateDirty;
	bool mSampleStateDirty;
	bool mFrontFaceDirty;
	bool mDitherStateDirty;

	Device *device;
	ResourceManager *mResourceManager;
};
}

#endif   // INCLUDE_CONTEXT_H_
