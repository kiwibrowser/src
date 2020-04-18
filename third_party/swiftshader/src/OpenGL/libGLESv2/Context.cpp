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

// Context.cpp: Implements the es2::Context class, managing all GL state and performing
// rendering operations. It is the GLES2 specific implementation of EGLContext.

#include "Context.h"

#include "main.h"
#include "mathutil.h"
#include "utilities.h"
#include "ResourceManager.h"
#include "Buffer.h"
#include "Fence.h"
#include "Framebuffer.h"
#include "Program.h"
#include "Query.h"
#include "Renderbuffer.h"
#include "Sampler.h"
#include "Shader.h"
#include "Texture.h"
#include "TransformFeedback.h"
#include "VertexArray.h"
#include "VertexDataManager.h"
#include "IndexDataManager.h"
#include "libEGL/Display.h"
#include "common/Surface.hpp"
#include "Common/Half.hpp"

#include <EGL/eglext.h>

#include <algorithm>
#include <string>

namespace es2
{
Context::Context(egl::Display *display, const Context *shareContext, EGLint clientVersion, const egl::Config *config)
	: egl::Context(display), clientVersion(clientVersion), config(config)
{
	sw::Context *context = new sw::Context();
	device = new es2::Device(context);

	setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	mState.depthClearValue = 1.0f;
	mState.stencilClearValue = 0;

	mState.cullFaceEnabled = false;
	mState.cullMode = GL_BACK;
	mState.frontFace = GL_CCW;
	mState.depthTestEnabled = false;
	mState.depthFunc = GL_LESS;
	mState.blendEnabled = false;
	mState.sourceBlendRGB = GL_ONE;
	mState.sourceBlendAlpha = GL_ONE;
	mState.destBlendRGB = GL_ZERO;
	mState.destBlendAlpha = GL_ZERO;
	mState.blendEquationRGB = GL_FUNC_ADD;
	mState.blendEquationAlpha = GL_FUNC_ADD;
	mState.blendColor.red = 0;
	mState.blendColor.green = 0;
	mState.blendColor.blue = 0;
	mState.blendColor.alpha = 0;
	mState.stencilTestEnabled = false;
	mState.stencilFunc = GL_ALWAYS;
	mState.stencilRef = 0;
	mState.stencilMask = 0xFFFFFFFFu;
	mState.stencilWritemask = 0xFFFFFFFFu;
	mState.stencilBackFunc = GL_ALWAYS;
	mState.stencilBackRef = 0;
	mState.stencilBackMask = 0xFFFFFFFFu;
	mState.stencilBackWritemask = 0xFFFFFFFFu;
	mState.stencilFail = GL_KEEP;
	mState.stencilPassDepthFail = GL_KEEP;
	mState.stencilPassDepthPass = GL_KEEP;
	mState.stencilBackFail = GL_KEEP;
	mState.stencilBackPassDepthFail = GL_KEEP;
	mState.stencilBackPassDepthPass = GL_KEEP;
	mState.polygonOffsetFillEnabled = false;
	mState.polygonOffsetFactor = 0.0f;
	mState.polygonOffsetUnits = 0.0f;
	mState.sampleAlphaToCoverageEnabled = false;
	mState.sampleCoverageEnabled = false;
	mState.sampleCoverageValue = 1.0f;
	mState.sampleCoverageInvert = false;
	mState.scissorTestEnabled = false;
	mState.ditherEnabled = true;
	mState.primitiveRestartFixedIndexEnabled = false;
	mState.rasterizerDiscardEnabled = false;
	mState.generateMipmapHint = GL_DONT_CARE;
	mState.fragmentShaderDerivativeHint = GL_DONT_CARE;
	mState.textureFilteringHint = GL_DONT_CARE;

	mState.lineWidth = 1.0f;

	mState.viewportX = 0;
	mState.viewportY = 0;
	mState.viewportWidth = 0;
	mState.viewportHeight = 0;
	mState.zNear = 0.0f;
	mState.zFar = 1.0f;

	mState.scissorX = 0;
	mState.scissorY = 0;
	mState.scissorWidth = 0;
	mState.scissorHeight = 0;

	mState.colorMaskRed = true;
	mState.colorMaskGreen = true;
	mState.colorMaskBlue = true;
	mState.colorMaskAlpha = true;
	mState.depthMask = true;

	if(shareContext)
	{
		mResourceManager = shareContext->mResourceManager;
		mResourceManager->addRef();
	}
	else
	{
		mResourceManager = new ResourceManager();
	}

	// [OpenGL ES 2.0.24] section 3.7 page 83:
	// In the initial state, TEXTURE_2D and TEXTURE_CUBE_MAP have twodimensional
	// and cube map texture state vectors respectively associated with them.
	// In order that access to these initial textures not be lost, they are treated as texture
	// objects all of whose names are 0.

	mTexture2DZero = new Texture2D(0);
	mTexture3DZero = new Texture3D(0);
	mTexture2DArrayZero = new Texture2DArray(0);
	mTextureCubeMapZero = new TextureCubeMap(0);
	mTexture2DRectZero = new Texture2DRect(0);
	mTextureExternalZero = new TextureExternal(0);

	mState.activeSampler = 0;

	for(int type = 0; type < TEXTURE_TYPE_COUNT; type++)
	{
		bindTexture((TextureType)type, 0);
	}

	bindVertexArray(0);
	bindArrayBuffer(0);
	bindElementArrayBuffer(0);
	bindReadFramebuffer(0);
	bindDrawFramebuffer(0);
	bindRenderbuffer(0);
	bindGenericUniformBuffer(0);
	bindTransformFeedback(0);

	mState.currentProgram = 0;

	mVertexDataManager = nullptr;
	mIndexDataManager = nullptr;

	mInvalidEnum = false;
	mInvalidValue = false;
	mInvalidOperation = false;
	mOutOfMemory = false;
	mInvalidFramebufferOperation = false;

	mHasBeenCurrent = false;

	markAllStateDirty();
}

Context::~Context()
{
	if(mState.currentProgram != 0)
	{
		Program *programObject = mResourceManager->getProgram(mState.currentProgram);
		if(programObject)
		{
			programObject->release();
		}
		mState.currentProgram = 0;
	}

	while(!mFramebufferNameSpace.empty())
	{
		deleteFramebuffer(mFramebufferNameSpace.firstName());
	}

	while(!mFenceNameSpace.empty())
	{
		deleteFence(mFenceNameSpace.firstName());
	}

	while(!mQueryNameSpace.empty())
	{
		deleteQuery(mQueryNameSpace.firstName());
	}

	while(!mVertexArrayNameSpace.empty())
	{
		deleteVertexArray(mVertexArrayNameSpace.lastName());
	}

	while(!mTransformFeedbackNameSpace.empty())
	{
		deleteTransformFeedback(mTransformFeedbackNameSpace.firstName());
	}

	for(int type = 0; type < TEXTURE_TYPE_COUNT; type++)
	{
		for(int sampler = 0; sampler < MAX_COMBINED_TEXTURE_IMAGE_UNITS; sampler++)
		{
			mState.samplerTexture[type][sampler] = nullptr;
		}
	}

	for(int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
	{
		mState.vertexAttribute[i].mBoundBuffer = nullptr;
	}

	for(int i = 0; i < QUERY_TYPE_COUNT; i++)
	{
		mState.activeQuery[i] = nullptr;
	}

	mState.arrayBuffer = nullptr;
	mState.copyReadBuffer = nullptr;
	mState.copyWriteBuffer = nullptr;
	mState.pixelPackBuffer = nullptr;
	mState.pixelUnpackBuffer = nullptr;
	mState.genericUniformBuffer = nullptr;

	for(int i = 0; i < MAX_UNIFORM_BUFFER_BINDINGS; i++) {
		mState.uniformBuffers[i].set(nullptr, 0, 0);
	}

	mState.renderbuffer = nullptr;

	for(int i = 0; i < MAX_COMBINED_TEXTURE_IMAGE_UNITS; ++i)
	{
		mState.sampler[i] = nullptr;
	}

	mTexture2DZero = nullptr;
	mTexture3DZero = nullptr;
	mTexture2DArrayZero = nullptr;
	mTextureCubeMapZero = nullptr;
	mTexture2DRectZero = nullptr;
	mTextureExternalZero = nullptr;

	delete mVertexDataManager;
	delete mIndexDataManager;

	mResourceManager->release();
	delete device;
}

void Context::makeCurrent(gl::Surface *surface)
{
	if(!mHasBeenCurrent)
	{
		mVertexDataManager = new VertexDataManager(this);
		mIndexDataManager = new IndexDataManager();

		mState.viewportX = 0;
		mState.viewportY = 0;
		mState.viewportWidth = surface ? surface->getWidth() : 0;
		mState.viewportHeight = surface ? surface->getHeight() : 0;

		mState.scissorX = 0;
		mState.scissorY = 0;
		mState.scissorWidth = surface ? surface->getWidth() : 0;
		mState.scissorHeight = surface ? surface->getHeight() : 0;

		mHasBeenCurrent = true;
	}

	if(surface)
	{
		// Wrap the existing resources into GL objects and assign them to the '0' names
		egl::Image *defaultRenderTarget = surface->getRenderTarget();
		egl::Image *depthStencil = surface->getDepthStencil();

		Colorbuffer *colorbufferZero = new Colorbuffer(defaultRenderTarget);
		DepthStencilbuffer *depthStencilbufferZero = new DepthStencilbuffer(depthStencil);
		Framebuffer *framebufferZero = new DefaultFramebuffer(colorbufferZero, depthStencilbufferZero);

		setFramebufferZero(framebufferZero);

		if(defaultRenderTarget)
		{
			defaultRenderTarget->release();
		}

		if(depthStencil)
		{
			depthStencil->release();
		}
	}
	else
	{
		setFramebufferZero(nullptr);
	}

	markAllStateDirty();
}

EGLint Context::getClientVersion() const
{
	return clientVersion;
}

EGLint Context::getConfigID() const
{
	return config->mConfigID;
}

// This function will set all of the state-related dirty flags, so that all state is set during next pre-draw.
void Context::markAllStateDirty()
{
	mAppliedProgramSerial = 0;

	mDepthStateDirty = true;
	mMaskStateDirty = true;
	mBlendStateDirty = true;
	mStencilStateDirty = true;
	mPolygonOffsetStateDirty = true;
	mSampleStateDirty = true;
	mDitherStateDirty = true;
	mFrontFaceDirty = true;
}

void Context::setClearColor(float red, float green, float blue, float alpha)
{
	mState.colorClearValue.red = red;
	mState.colorClearValue.green = green;
	mState.colorClearValue.blue = blue;
	mState.colorClearValue.alpha = alpha;
}

void Context::setClearDepth(float depth)
{
	mState.depthClearValue = depth;
}

void Context::setClearStencil(int stencil)
{
	mState.stencilClearValue = stencil;
}

void Context::setCullFaceEnabled(bool enabled)
{
	mState.cullFaceEnabled = enabled;
}

bool Context::isCullFaceEnabled() const
{
	return mState.cullFaceEnabled;
}

void Context::setCullMode(GLenum mode)
{
   mState.cullMode = mode;
}

void Context::setFrontFace(GLenum front)
{
	if(mState.frontFace != front)
	{
		mState.frontFace = front;
		mFrontFaceDirty = true;
	}
}

void Context::setDepthTestEnabled(bool enabled)
{
	if(mState.depthTestEnabled != enabled)
	{
		mState.depthTestEnabled = enabled;
		mDepthStateDirty = true;
	}
}

bool Context::isDepthTestEnabled() const
{
	return mState.depthTestEnabled;
}

void Context::setDepthFunc(GLenum depthFunc)
{
	if(mState.depthFunc != depthFunc)
	{
		mState.depthFunc = depthFunc;
		mDepthStateDirty = true;
	}
}

void Context::setDepthRange(float zNear, float zFar)
{
	mState.zNear = zNear;
	mState.zFar = zFar;
}

void Context::setBlendEnabled(bool enabled)
{
	if(mState.blendEnabled != enabled)
	{
		mState.blendEnabled = enabled;
		mBlendStateDirty = true;
	}
}

bool Context::isBlendEnabled() const
{
	return mState.blendEnabled;
}

void Context::setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha)
{
	if(mState.sourceBlendRGB != sourceRGB ||
	   mState.sourceBlendAlpha != sourceAlpha ||
	   mState.destBlendRGB != destRGB ||
	   mState.destBlendAlpha != destAlpha)
	{
		mState.sourceBlendRGB = sourceRGB;
		mState.destBlendRGB = destRGB;
		mState.sourceBlendAlpha = sourceAlpha;
		mState.destBlendAlpha = destAlpha;
		mBlendStateDirty = true;
	}
}

void Context::setBlendColor(float red, float green, float blue, float alpha)
{
	if(mState.blendColor.red != red ||
	   mState.blendColor.green != green ||
	   mState.blendColor.blue != blue ||
	   mState.blendColor.alpha != alpha)
	{
		mState.blendColor.red = red;
		mState.blendColor.green = green;
		mState.blendColor.blue = blue;
		mState.blendColor.alpha = alpha;
		mBlendStateDirty = true;
	}
}

void Context::setBlendEquation(GLenum rgbEquation, GLenum alphaEquation)
{
	if(mState.blendEquationRGB != rgbEquation ||
	   mState.blendEquationAlpha != alphaEquation)
	{
		mState.blendEquationRGB = rgbEquation;
		mState.blendEquationAlpha = alphaEquation;
		mBlendStateDirty = true;
	}
}

void Context::setStencilTestEnabled(bool enabled)
{
	if(mState.stencilTestEnabled != enabled)
	{
		mState.stencilTestEnabled = enabled;
		mStencilStateDirty = true;
	}
}

bool Context::isStencilTestEnabled() const
{
	return mState.stencilTestEnabled;
}

void Context::setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask)
{
	if(mState.stencilFunc != stencilFunc ||
	   mState.stencilRef != stencilRef ||
	   mState.stencilMask != stencilMask)
	{
		mState.stencilFunc = stencilFunc;
		mState.stencilRef = (stencilRef > 0) ? stencilRef : 0;
		mState.stencilMask = stencilMask;
		mStencilStateDirty = true;
	}
}

void Context::setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask)
{
	if(mState.stencilBackFunc != stencilBackFunc ||
	   mState.stencilBackRef != stencilBackRef ||
	   mState.stencilBackMask != stencilBackMask)
	{
		mState.stencilBackFunc = stencilBackFunc;
		mState.stencilBackRef = (stencilBackRef > 0) ? stencilBackRef : 0;
		mState.stencilBackMask = stencilBackMask;
		mStencilStateDirty = true;
	}
}

void Context::setStencilWritemask(GLuint stencilWritemask)
{
	if(mState.stencilWritemask != stencilWritemask)
	{
		mState.stencilWritemask = stencilWritemask;
		mStencilStateDirty = true;
	}
}

void Context::setStencilBackWritemask(GLuint stencilBackWritemask)
{
	if(mState.stencilBackWritemask != stencilBackWritemask)
	{
		mState.stencilBackWritemask = stencilBackWritemask;
		mStencilStateDirty = true;
	}
}

void Context::setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass)
{
	if(mState.stencilFail != stencilFail ||
	   mState.stencilPassDepthFail != stencilPassDepthFail ||
	   mState.stencilPassDepthPass != stencilPassDepthPass)
	{
		mState.stencilFail = stencilFail;
		mState.stencilPassDepthFail = stencilPassDepthFail;
		mState.stencilPassDepthPass = stencilPassDepthPass;
		mStencilStateDirty = true;
	}
}

void Context::setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass)
{
	if(mState.stencilBackFail != stencilBackFail ||
	   mState.stencilBackPassDepthFail != stencilBackPassDepthFail ||
	   mState.stencilBackPassDepthPass != stencilBackPassDepthPass)
	{
		mState.stencilBackFail = stencilBackFail;
		mState.stencilBackPassDepthFail = stencilBackPassDepthFail;
		mState.stencilBackPassDepthPass = stencilBackPassDepthPass;
		mStencilStateDirty = true;
	}
}

void Context::setPolygonOffsetFillEnabled(bool enabled)
{
	if(mState.polygonOffsetFillEnabled != enabled)
	{
		mState.polygonOffsetFillEnabled = enabled;
		mPolygonOffsetStateDirty = true;
	}
}

bool Context::isPolygonOffsetFillEnabled() const
{
	return mState.polygonOffsetFillEnabled;
}

void Context::setPolygonOffsetParams(GLfloat factor, GLfloat units)
{
	if(mState.polygonOffsetFactor != factor ||
	   mState.polygonOffsetUnits != units)
	{
		mState.polygonOffsetFactor = factor;
		mState.polygonOffsetUnits = units;
		mPolygonOffsetStateDirty = true;
	}
}

void Context::setSampleAlphaToCoverageEnabled(bool enabled)
{
	if(mState.sampleAlphaToCoverageEnabled != enabled)
	{
		mState.sampleAlphaToCoverageEnabled = enabled;
		mSampleStateDirty = true;
	}
}

bool Context::isSampleAlphaToCoverageEnabled() const
{
	return mState.sampleAlphaToCoverageEnabled;
}

void Context::setSampleCoverageEnabled(bool enabled)
{
	if(mState.sampleCoverageEnabled != enabled)
	{
		mState.sampleCoverageEnabled = enabled;
		mSampleStateDirty = true;
	}
}

bool Context::isSampleCoverageEnabled() const
{
	return mState.sampleCoverageEnabled;
}

void Context::setSampleCoverageParams(GLclampf value, bool invert)
{
	if(mState.sampleCoverageValue != value ||
	   mState.sampleCoverageInvert != invert)
	{
		mState.sampleCoverageValue = value;
		mState.sampleCoverageInvert = invert;
		mSampleStateDirty = true;
	}
}

void Context::setScissorTestEnabled(bool enabled)
{
	mState.scissorTestEnabled = enabled;
}

bool Context::isScissorTestEnabled() const
{
	return mState.scissorTestEnabled;
}

void Context::setDitherEnabled(bool enabled)
{
	if(mState.ditherEnabled != enabled)
	{
		mState.ditherEnabled = enabled;
		mDitherStateDirty = true;
	}
}

bool Context::isDitherEnabled() const
{
	return mState.ditherEnabled;
}

void Context::setPrimitiveRestartFixedIndexEnabled(bool enabled)
{
	mState.primitiveRestartFixedIndexEnabled = enabled;
}

bool Context::isPrimitiveRestartFixedIndexEnabled() const
{
	return mState.primitiveRestartFixedIndexEnabled;
}

void Context::setRasterizerDiscardEnabled(bool enabled)
{
	mState.rasterizerDiscardEnabled = enabled;
}

bool Context::isRasterizerDiscardEnabled() const
{
	return mState.rasterizerDiscardEnabled;
}

void Context::setLineWidth(GLfloat width)
{
	mState.lineWidth = width;
	device->setLineWidth(clamp(width, ALIASED_LINE_WIDTH_RANGE_MIN, ALIASED_LINE_WIDTH_RANGE_MAX));
}

void Context::setGenerateMipmapHint(GLenum hint)
{
	mState.generateMipmapHint = hint;
}

void Context::setFragmentShaderDerivativeHint(GLenum hint)
{
	mState.fragmentShaderDerivativeHint = hint;
	// TODO: Propagate the hint to shader translator so we can write
	// ddx, ddx_coarse, or ddx_fine depending on the hint.
	// Ignore for now. It is valid for implementations to ignore hint.
}

void Context::setTextureFilteringHint(GLenum hint)
{
	mState.textureFilteringHint = hint;
}

void Context::setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
	mState.viewportX = x;
	mState.viewportY = y;
	mState.viewportWidth = std::min<GLsizei>(width, IMPLEMENTATION_MAX_RENDERBUFFER_SIZE);     // GL_MAX_VIEWPORT_DIMS[0]
	mState.viewportHeight = std::min<GLsizei>(height, IMPLEMENTATION_MAX_RENDERBUFFER_SIZE);   // GL_MAX_VIEWPORT_DIMS[1]
}

void Context::setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
	mState.scissorX = x;
	mState.scissorY = y;
	mState.scissorWidth = width;
	mState.scissorHeight = height;
}

void Context::setColorMask(bool red, bool green, bool blue, bool alpha)
{
	if(mState.colorMaskRed != red || mState.colorMaskGreen != green ||
	   mState.colorMaskBlue != blue || mState.colorMaskAlpha != alpha)
	{
		mState.colorMaskRed = red;
		mState.colorMaskGreen = green;
		mState.colorMaskBlue = blue;
		mState.colorMaskAlpha = alpha;
		mMaskStateDirty = true;
	}
}

unsigned int Context::getColorMask() const
{
	return (mState.colorMaskRed ? 0x1 : 0) |
	       (mState.colorMaskGreen ? 0x2 : 0) |
	       (mState.colorMaskBlue ? 0x4 : 0) |
	       (mState.colorMaskAlpha ? 0x8 : 0);
}

void Context::setDepthMask(bool mask)
{
	if(mState.depthMask != mask)
	{
		mState.depthMask = mask;
		mMaskStateDirty = true;
	}
}

void Context::setActiveSampler(unsigned int active)
{
	mState.activeSampler = active;
}

GLuint Context::getReadFramebufferName() const
{
	return mState.readFramebuffer;
}

GLuint Context::getDrawFramebufferName() const
{
	return mState.drawFramebuffer;
}

GLuint Context::getRenderbufferName() const
{
	return mState.renderbuffer.name();
}

void Context::setFramebufferReadBuffer(GLuint buf)
{
	Framebuffer *framebuffer = getReadFramebuffer();

	if(framebuffer)
	{
		framebuffer->setReadBuffer(buf);
	}
	else
	{
		return error(GL_INVALID_OPERATION);
	}
}

void Context::setFramebufferDrawBuffers(GLsizei n, const GLenum *bufs)
{
	Framebuffer *drawFramebuffer = getDrawFramebuffer();

	if(drawFramebuffer)
	{
		for(int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
		{
			drawFramebuffer->setDrawBuffer(i, (i < n) ? bufs[i] : GL_NONE);
		}
	}
	else
	{
		return error(GL_INVALID_OPERATION);
	}
}

GLuint Context::getArrayBufferName() const
{
	return mState.arrayBuffer.name();
}

GLuint Context::getElementArrayBufferName() const
{
	Buffer* elementArrayBuffer = getCurrentVertexArray()->getElementArrayBuffer();
	return elementArrayBuffer ? elementArrayBuffer->name : 0;
}

GLuint Context::getActiveQuery(GLenum target) const
{
	Query *queryObject = nullptr;

	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED_EXT:
		queryObject = mState.activeQuery[QUERY_ANY_SAMPLES_PASSED];
		break;
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
		queryObject = mState.activeQuery[QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE];
		break;
	case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
		queryObject = mState.activeQuery[QUERY_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN];
		break;
	default:
		ASSERT(false);
	}

	if(queryObject)
	{
		return queryObject->name;
	}

	return 0;
}

void Context::setVertexAttribArrayEnabled(unsigned int attribNum, bool enabled)
{
	getCurrentVertexArray()->enableAttribute(attribNum, enabled);
}

void Context::setVertexAttribDivisor(unsigned int attribNum, GLuint divisor)
{
	getCurrentVertexArray()->setVertexAttribDivisor(attribNum, divisor);
}

const VertexAttribute &Context::getVertexAttribState(unsigned int attribNum) const
{
	return getCurrentVertexArray()->getVertexAttribute(attribNum);
}

void Context::setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type,
                                   bool normalized, bool pureInteger, GLsizei stride, const void *pointer)
{
	getCurrentVertexArray()->setAttributeState(attribNum, boundBuffer, size, type, normalized, pureInteger, stride, pointer);
}

const void *Context::getVertexAttribPointer(unsigned int attribNum) const
{
	return getCurrentVertexArray()->getVertexAttribute(attribNum).mPointer;
}

const VertexAttributeArray &Context::getVertexArrayAttributes()
{
	return getCurrentVertexArray()->getVertexAttributes();
}

const VertexAttributeArray &Context::getCurrentVertexAttributes()
{
	return mState.vertexAttribute;
}

void Context::setPackAlignment(GLint alignment)
{
	mState.packParameters.alignment = alignment;
}

void Context::setUnpackAlignment(GLint alignment)
{
	mState.unpackParameters.alignment = alignment;
}

const gl::PixelStorageModes &Context::getUnpackParameters() const
{
	return mState.unpackParameters;
}

void Context::setPackRowLength(GLint rowLength)
{
	mState.packParameters.rowLength = rowLength;
}

void Context::setPackSkipPixels(GLint skipPixels)
{
	mState.packParameters.skipPixels = skipPixels;
}

void Context::setPackSkipRows(GLint skipRows)
{
	mState.packParameters.skipRows = skipRows;
}

void Context::setUnpackRowLength(GLint rowLength)
{
	mState.unpackParameters.rowLength = rowLength;
}

void Context::setUnpackImageHeight(GLint imageHeight)
{
	mState.unpackParameters.imageHeight = imageHeight;
}

void Context::setUnpackSkipPixels(GLint skipPixels)
{
	mState.unpackParameters.skipPixels = skipPixels;
}

void Context::setUnpackSkipRows(GLint skipRows)
{
	mState.unpackParameters.skipRows = skipRows;
}

void Context::setUnpackSkipImages(GLint skipImages)
{
	mState.unpackParameters.skipImages = skipImages;
}

GLuint Context::createBuffer()
{
	return mResourceManager->createBuffer();
}

GLuint Context::createProgram()
{
	return mResourceManager->createProgram();
}

GLuint Context::createShader(GLenum type)
{
	return mResourceManager->createShader(type);
}

GLuint Context::createTexture()
{
	return mResourceManager->createTexture();
}

GLuint Context::createRenderbuffer()
{
	return mResourceManager->createRenderbuffer();
}

// Returns an unused framebuffer name
GLuint Context::createFramebuffer()
{
	return mFramebufferNameSpace.allocate();
}

GLuint Context::createFence()
{
	return mFenceNameSpace.allocate(new Fence());
}

// Returns an unused query name
GLuint Context::createQuery()
{
	return mQueryNameSpace.allocate();
}

// Returns an unused vertex array name
GLuint Context::createVertexArray()
{
	return mVertexArrayNameSpace.allocate();
}

GLsync Context::createFenceSync(GLenum condition, GLbitfield flags)
{
	GLuint handle = mResourceManager->createFenceSync(condition, flags);

	return reinterpret_cast<GLsync>(static_cast<uintptr_t>(handle));
}

// Returns an unused transform feedback name
GLuint Context::createTransformFeedback()
{
	return mTransformFeedbackNameSpace.allocate();
}

// Returns an unused sampler name
GLuint Context::createSampler()
{
	return mResourceManager->createSampler();
}

void Context::deleteBuffer(GLuint buffer)
{
	detachBuffer(buffer);

	mResourceManager->deleteBuffer(buffer);
}

void Context::deleteShader(GLuint shader)
{
	mResourceManager->deleteShader(shader);
}

void Context::deleteProgram(GLuint program)
{
	mResourceManager->deleteProgram(program);
}

void Context::deleteTexture(GLuint texture)
{
	detachTexture(texture);

	mResourceManager->deleteTexture(texture);
}

void Context::deleteRenderbuffer(GLuint renderbuffer)
{
	if(mResourceManager->getRenderbuffer(renderbuffer))
	{
		detachRenderbuffer(renderbuffer);
	}

	mResourceManager->deleteRenderbuffer(renderbuffer);
}

void Context::deleteFramebuffer(GLuint framebuffer)
{
	detachFramebuffer(framebuffer);

	Framebuffer *framebufferObject = mFramebufferNameSpace.remove(framebuffer);

	if(framebufferObject)
	{
		delete framebufferObject;
	}
}

void Context::deleteFence(GLuint fence)
{
	Fence *fenceObject = mFenceNameSpace.remove(fence);

	if(fenceObject)
	{
		delete fenceObject;
	}
}

void Context::deleteQuery(GLuint query)
{
	Query *queryObject = mQueryNameSpace.remove(query);

	if(queryObject)
	{
		queryObject->release();
	}
}

void Context::deleteVertexArray(GLuint vertexArray)
{
	// [OpenGL ES 3.0.2] section 2.10 page 43:
	// If a vertex array object that is currently bound is deleted, the binding
	// for that object reverts to zero and the default vertex array becomes current.
	if(getCurrentVertexArray()->name == vertexArray)
	{
		bindVertexArray(0);
	}

	VertexArray *vertexArrayObject = mVertexArrayNameSpace.remove(vertexArray);

	if(vertexArrayObject)
	{
		delete vertexArrayObject;
	}
}

void Context::deleteFenceSync(GLsync fenceSync)
{
	// The spec specifies the underlying Fence object is not deleted until all current
	// wait commands finish. However, since the name becomes invalid, we cannot query the fence,
	// and since our API is currently designed for being called from a single thread, we can delete
	// the fence immediately.
	mResourceManager->deleteFenceSync(static_cast<GLuint>(reinterpret_cast<uintptr_t>(fenceSync)));
}

void Context::deleteTransformFeedback(GLuint transformFeedback)
{
	TransformFeedback *transformFeedbackObject = mTransformFeedbackNameSpace.remove(transformFeedback);

	if(transformFeedbackObject)
	{
		delete transformFeedbackObject;
	}
}

void Context::deleteSampler(GLuint sampler)
{
	detachSampler(sampler);

	mResourceManager->deleteSampler(sampler);
}

Buffer *Context::getBuffer(GLuint handle) const
{
	return mResourceManager->getBuffer(handle);
}

Shader *Context::getShader(GLuint handle) const
{
	return mResourceManager->getShader(handle);
}

Program *Context::getProgram(GLuint handle) const
{
	return mResourceManager->getProgram(handle);
}

Texture *Context::getTexture(GLuint handle) const
{
	return mResourceManager->getTexture(handle);
}

Renderbuffer *Context::getRenderbuffer(GLuint handle) const
{
	return mResourceManager->getRenderbuffer(handle);
}

Framebuffer *Context::getReadFramebuffer() const
{
	return getFramebuffer(mState.readFramebuffer);
}

Framebuffer *Context::getDrawFramebuffer() const
{
	return getFramebuffer(mState.drawFramebuffer);
}

void Context::bindArrayBuffer(unsigned int buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	mState.arrayBuffer = getBuffer(buffer);
}

void Context::bindElementArrayBuffer(unsigned int buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	getCurrentVertexArray()->setElementArrayBuffer(getBuffer(buffer));
}

void Context::bindCopyReadBuffer(GLuint buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	mState.copyReadBuffer = getBuffer(buffer);
}

void Context::bindCopyWriteBuffer(GLuint buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	mState.copyWriteBuffer = getBuffer(buffer);
}

void Context::bindPixelPackBuffer(GLuint buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	mState.pixelPackBuffer = getBuffer(buffer);
}

void Context::bindPixelUnpackBuffer(GLuint buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	mState.pixelUnpackBuffer = getBuffer(buffer);
}

void Context::bindTransformFeedbackBuffer(GLuint buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	TransformFeedback* transformFeedback = getTransformFeedback(mState.transformFeedback);

	if(transformFeedback)
	{
		transformFeedback->setGenericBuffer(getBuffer(buffer));
	}
}

void Context::bindTexture(TextureType type, GLuint texture)
{
	mResourceManager->checkTextureAllocation(texture, type);

	mState.samplerTexture[type][mState.activeSampler] = getTexture(texture);
}

void Context::bindReadFramebuffer(GLuint framebuffer)
{
	if(!getFramebuffer(framebuffer))
	{
		if(framebuffer == 0)
		{
			mFramebufferNameSpace.insert(framebuffer, new DefaultFramebuffer());
		}
		else
		{
			mFramebufferNameSpace.insert(framebuffer, new Framebuffer());
		}
	}

	mState.readFramebuffer = framebuffer;
}

void Context::bindDrawFramebuffer(GLuint framebuffer)
{
	if(!getFramebuffer(framebuffer))
	{
		if(framebuffer == 0)
		{
			mFramebufferNameSpace.insert(framebuffer, new DefaultFramebuffer());
		}
		else
		{
			mFramebufferNameSpace.insert(framebuffer, new Framebuffer());
		}
	}

	mState.drawFramebuffer = framebuffer;
}

void Context::bindRenderbuffer(GLuint renderbuffer)
{
	mResourceManager->checkRenderbufferAllocation(renderbuffer);

	mState.renderbuffer = getRenderbuffer(renderbuffer);
}

void Context::bindVertexArray(GLuint array)
{
	VertexArray *vertexArray = getVertexArray(array);

	if(!vertexArray)
	{
		vertexArray = new VertexArray(array);
		mVertexArrayNameSpace.insert(array, vertexArray);
	}

	mState.vertexArray = array;
}

void Context::bindGenericUniformBuffer(GLuint buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	mState.genericUniformBuffer = getBuffer(buffer);
}

void Context::bindIndexedUniformBuffer(GLuint buffer, GLuint index, GLintptr offset, GLsizeiptr size)
{
	mResourceManager->checkBufferAllocation(buffer);

	Buffer* bufferObject = getBuffer(buffer);
	mState.uniformBuffers[index].set(bufferObject, static_cast<int>(offset), static_cast<int>(size));
}

void Context::bindGenericTransformFeedbackBuffer(GLuint buffer)
{
	mResourceManager->checkBufferAllocation(buffer);

	getTransformFeedback()->setGenericBuffer(getBuffer(buffer));
}

void Context::bindIndexedTransformFeedbackBuffer(GLuint buffer, GLuint index, GLintptr offset, GLsizeiptr size)
{
	mResourceManager->checkBufferAllocation(buffer);

	Buffer* bufferObject = getBuffer(buffer);
	getTransformFeedback()->setBuffer(index, bufferObject, offset, size);
}

void Context::bindTransformFeedback(GLuint id)
{
	if(!getTransformFeedback(id))
	{
		mTransformFeedbackNameSpace.insert(id, new TransformFeedback(id));
	}

	mState.transformFeedback = id;
}

bool Context::bindSampler(GLuint unit, GLuint sampler)
{
	mResourceManager->checkSamplerAllocation(sampler);

	Sampler* samplerObject = getSampler(sampler);

	mState.sampler[unit] = samplerObject;

	return !!samplerObject;
}

void Context::useProgram(GLuint program)
{
	GLuint priorProgram = mState.currentProgram;
	mState.currentProgram = program;               // Must switch before trying to delete, otherwise it only gets flagged.

	if(priorProgram != program)
	{
		Program *newProgram = mResourceManager->getProgram(program);
		Program *oldProgram = mResourceManager->getProgram(priorProgram);

		if(newProgram)
		{
			newProgram->addRef();
		}

		if(oldProgram)
		{
			oldProgram->release();
		}
	}
}

void Context::beginQuery(GLenum target, GLuint query)
{
	// From EXT_occlusion_query_boolean: If BeginQueryEXT is called with an <id>
	// of zero, if the active query object name for <target> is non-zero (for the
	// targets ANY_SAMPLES_PASSED_EXT and ANY_SAMPLES_PASSED_CONSERVATIVE_EXT, if
	// the active query for either target is non-zero), if <id> is the name of an
	// existing query object whose type does not match <target>, or if <id> is the
	// active query object name for any query type, the error INVALID_OPERATION is
	// generated.

	// Ensure no other queries are active
	// NOTE: If other queries than occlusion are supported, we will need to check
	// separately that:
	//    a) The query ID passed is not the current active query for any target/type
	//    b) There are no active queries for the requested target (and in the case
	//       of GL_ANY_SAMPLES_PASSED_EXT and GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT,
	//       no query may be active for either if glBeginQuery targets either.
	for(int i = 0; i < QUERY_TYPE_COUNT; i++)
	{
		if(mState.activeQuery[i])
		{
			switch(mState.activeQuery[i]->getType())
			{
			case GL_ANY_SAMPLES_PASSED_EXT:
			case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
				if((target == GL_ANY_SAMPLES_PASSED_EXT) ||
				   (target == GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT))
				{
					return error(GL_INVALID_OPERATION);
				}
				break;
			case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
				if(target == GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN)
				{
					return error(GL_INVALID_OPERATION);
				}
				break;
			default:
				break;
			}
		}
	}

	QueryType qType;
	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED_EXT:
		qType = QUERY_ANY_SAMPLES_PASSED;
		break;
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
		qType = QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE;
		break;
	case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
		qType = QUERY_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN;
		break;
	default:
		UNREACHABLE(target);
		return error(GL_INVALID_ENUM);
	}

	Query *queryObject = createQuery(query, target);

	// Check that name was obtained with glGenQueries
	if(!queryObject)
	{
		return error(GL_INVALID_OPERATION);
	}

	// Check for type mismatch
	if(queryObject->getType() != target)
	{
		return error(GL_INVALID_OPERATION);
	}

	// Set query as active for specified target
	mState.activeQuery[qType] = queryObject;

	// Begin query
	queryObject->begin();
}

void Context::endQuery(GLenum target)
{
	QueryType qType;

	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED_EXT:                qType = QUERY_ANY_SAMPLES_PASSED;                    break;
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:   qType = QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE;       break;
	case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN: qType = QUERY_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN; break;
	default: UNREACHABLE(target); return;
	}

	Query *queryObject = mState.activeQuery[qType];

	if(!queryObject)
	{
		return error(GL_INVALID_OPERATION);
	}

	queryObject->end();

	mState.activeQuery[qType] = nullptr;
}

void Context::setFramebufferZero(Framebuffer *buffer)
{
	delete mFramebufferNameSpace.remove(0);
	mFramebufferNameSpace.insert(0, buffer);
}

void Context::setRenderbufferStorage(RenderbufferStorage *renderbuffer)
{
	Renderbuffer *renderbufferObject = mState.renderbuffer;
	renderbufferObject->setStorage(renderbuffer);
}

Framebuffer *Context::getFramebuffer(unsigned int handle) const
{
	return mFramebufferNameSpace.find(handle);
}

Fence *Context::getFence(unsigned int handle) const
{
	return mFenceNameSpace.find(handle);
}

FenceSync *Context::getFenceSync(GLsync handle) const
{
	return mResourceManager->getFenceSync(static_cast<GLuint>(reinterpret_cast<uintptr_t>(handle)));
}

Query *Context::getQuery(unsigned int handle) const
{
	return mQueryNameSpace.find(handle);
}

Query *Context::createQuery(unsigned int handle, GLenum type)
{
	if(!mQueryNameSpace.isReserved(handle))
	{
		return nullptr;
	}
	else
	{
		Query *query = mQueryNameSpace.find(handle);
		if(!query)
		{
			query = new Query(handle, type);
			query->addRef();
			mQueryNameSpace.insert(handle, query);
		}

		return query;
	}
}

VertexArray *Context::getVertexArray(GLuint array) const
{
	return mVertexArrayNameSpace.find(array);
}

VertexArray *Context::getCurrentVertexArray() const
{
	return getVertexArray(mState.vertexArray);
}

bool Context::isVertexArray(GLuint array) const
{
	return mVertexArrayNameSpace.isReserved(array);
}

bool Context::hasZeroDivisor() const
{
	// Verify there is at least one active attribute with a divisor of zero
	es2::Program *programObject = getCurrentProgram();
	for(int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
	{
		bool active = (programObject->getAttributeStream(attributeIndex) != -1);
		if(active && getCurrentVertexArray()->getVertexAttribute(attributeIndex).mDivisor == 0)
		{
			return true;
		}
	}

	return false;
}

TransformFeedback *Context::getTransformFeedback(GLuint transformFeedback) const
{
	return mTransformFeedbackNameSpace.find(transformFeedback);
}

bool Context::isTransformFeedback(GLuint array) const
{
	return mTransformFeedbackNameSpace.isReserved(array);
}

Sampler *Context::getSampler(GLuint sampler) const
{
	return mResourceManager->getSampler(sampler);
}

bool Context::isSampler(GLuint sampler) const
{
	return mResourceManager->isSampler(sampler);
}

Buffer *Context::getArrayBuffer() const
{
	return mState.arrayBuffer;
}

Buffer *Context::getElementArrayBuffer() const
{
	return getCurrentVertexArray()->getElementArrayBuffer();
}

Buffer *Context::getCopyReadBuffer() const
{
	return mState.copyReadBuffer;
}

Buffer *Context::getCopyWriteBuffer() const
{
	return mState.copyWriteBuffer;
}

Buffer *Context::getPixelPackBuffer() const
{
	return mState.pixelPackBuffer;
}

Buffer *Context::getPixelUnpackBuffer() const
{
	return mState.pixelUnpackBuffer;
}

Buffer *Context::getGenericUniformBuffer() const
{
	return mState.genericUniformBuffer;
}

GLsizei Context::getRequiredBufferSize(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type) const
{
	GLsizei inputWidth = (mState.unpackParameters.rowLength == 0) ? width : mState.unpackParameters.rowLength;
	GLsizei inputPitch = gl::ComputePitch(inputWidth, format, type, mState.unpackParameters.alignment);
	GLsizei inputHeight = (mState.unpackParameters.imageHeight == 0) ? height : mState.unpackParameters.imageHeight;
	return inputPitch * inputHeight * depth;
}

GLenum Context::getPixels(const GLvoid **pixels, GLenum type, GLsizei imageSize) const
{
	if(mState.pixelUnpackBuffer)
	{
		ASSERT(mState.pixelUnpackBuffer->name != 0);

		if(mState.pixelUnpackBuffer->isMapped())
		{
			return GL_INVALID_OPERATION;
		}

		size_t offset = static_cast<size_t>((ptrdiff_t)(*pixels));

		if(offset % GetTypeSize(type) != 0)
		{
			return GL_INVALID_OPERATION;
		}

		if(offset > mState.pixelUnpackBuffer->size())
		{
			return GL_INVALID_OPERATION;
		}

		if(mState.pixelUnpackBuffer->size() - offset < static_cast<size_t>(imageSize))
		{
			return GL_INVALID_OPERATION;
		}

		*pixels = static_cast<const unsigned char*>(mState.pixelUnpackBuffer->data()) + offset;
	}

	return GL_NO_ERROR;
}

bool Context::getBuffer(GLenum target, es2::Buffer **buffer) const
{
	switch(target)
	{
	case GL_ARRAY_BUFFER:
		*buffer = getArrayBuffer();
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		*buffer = getElementArrayBuffer();
		break;
	case GL_COPY_READ_BUFFER:
		if(clientVersion >= 3)
		{
			*buffer = getCopyReadBuffer();
			break;
		}
		else return false;
	case GL_COPY_WRITE_BUFFER:
		if(clientVersion >= 3)
		{
			*buffer = getCopyWriteBuffer();
			break;
		}
		else return false;
	case GL_PIXEL_PACK_BUFFER:
		if(clientVersion >= 3)
		{
			*buffer = getPixelPackBuffer();
			break;
		}
		else return false;
	case GL_PIXEL_UNPACK_BUFFER:
		if(clientVersion >= 3)
		{
			*buffer = getPixelUnpackBuffer();
			break;
		}
		else return false;
	case GL_TRANSFORM_FEEDBACK_BUFFER:
		if(clientVersion >= 3)
		{
			TransformFeedback* transformFeedback = getTransformFeedback();
			*buffer = transformFeedback ? static_cast<es2::Buffer*>(transformFeedback->getGenericBuffer()) : nullptr;
			break;
		}
		else return false;
	case GL_UNIFORM_BUFFER:
		if(clientVersion >= 3)
		{
			*buffer = getGenericUniformBuffer();
			break;
		}
		else return false;
	default:
		return false;
	}
	return true;
}

TransformFeedback *Context::getTransformFeedback() const
{
	return getTransformFeedback(mState.transformFeedback);
}

Program *Context::getCurrentProgram() const
{
	return mResourceManager->getProgram(mState.currentProgram);
}

Texture2D *Context::getTexture2D() const
{
	return static_cast<Texture2D*>(getSamplerTexture(mState.activeSampler, TEXTURE_2D));
}

Texture2D *Context::getTexture2D(GLenum target) const
{
	switch(target)
	{
	case GL_TEXTURE_2D:            return getTexture2D();
	case GL_TEXTURE_RECTANGLE_ARB: return getTexture2DRect();
	case GL_TEXTURE_EXTERNAL_OES:  return getTextureExternal();
	default:                       UNREACHABLE(target);
	}

	return nullptr;
}

Texture3D *Context::getTexture3D() const
{
	return static_cast<Texture3D*>(getSamplerTexture(mState.activeSampler, TEXTURE_3D));
}

Texture2DArray *Context::getTexture2DArray() const
{
	return static_cast<Texture2DArray*>(getSamplerTexture(mState.activeSampler, TEXTURE_2D_ARRAY));
}

TextureCubeMap *Context::getTextureCubeMap() const
{
	return static_cast<TextureCubeMap*>(getSamplerTexture(mState.activeSampler, TEXTURE_CUBE));
}

Texture2DRect *Context::getTexture2DRect() const
{
	return static_cast<Texture2DRect*>(getSamplerTexture(mState.activeSampler, TEXTURE_2D_RECT));
}

TextureExternal *Context::getTextureExternal() const
{
	return static_cast<TextureExternal*>(getSamplerTexture(mState.activeSampler, TEXTURE_EXTERNAL));
}

Texture *Context::getSamplerTexture(unsigned int sampler, TextureType type) const
{
	GLuint texid = mState.samplerTexture[type][sampler].name();

	if(texid == 0)   // Special case: 0 refers to different initial textures based on the target
	{
		switch(type)
		{
		case TEXTURE_2D: return mTexture2DZero;
		case TEXTURE_3D: return mTexture3DZero;
		case TEXTURE_2D_ARRAY: return mTexture2DArrayZero;
		case TEXTURE_CUBE: return mTextureCubeMapZero;
		case TEXTURE_2D_RECT: return mTexture2DRectZero;
		case TEXTURE_EXTERNAL: return mTextureExternalZero;
		default: UNREACHABLE(type);
		}
	}

	return mState.samplerTexture[type][sampler];
}

void Context::samplerParameteri(GLuint sampler, GLenum pname, GLint param)
{
	mResourceManager->checkSamplerAllocation(sampler);

	Sampler *samplerObject = getSampler(sampler);
	ASSERT(samplerObject);

	switch(pname)
	{
	case GL_TEXTURE_MIN_FILTER:         samplerObject->setMinFilter(static_cast<GLenum>(param));      break;
	case GL_TEXTURE_MAG_FILTER:         samplerObject->setMagFilter(static_cast<GLenum>(param));      break;
	case GL_TEXTURE_WRAP_S:             samplerObject->setWrapS(static_cast<GLenum>(param));          break;
	case GL_TEXTURE_WRAP_T:             samplerObject->setWrapT(static_cast<GLenum>(param));          break;
	case GL_TEXTURE_WRAP_R:             samplerObject->setWrapR(static_cast<GLenum>(param));          break;
	case GL_TEXTURE_MIN_LOD:            samplerObject->setMinLod(static_cast<GLfloat>(param));        break;
	case GL_TEXTURE_MAX_LOD:            samplerObject->setMaxLod(static_cast<GLfloat>(param));        break;
	case GL_TEXTURE_COMPARE_MODE:       samplerObject->setCompareMode(static_cast<GLenum>(param));    break;
	case GL_TEXTURE_COMPARE_FUNC:       samplerObject->setCompareFunc(static_cast<GLenum>(param));    break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT: samplerObject->setMaxAnisotropy(static_cast<GLfloat>(param)); break;
	default:                            UNREACHABLE(pname); break;
	}
}

void Context::samplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
{
	mResourceManager->checkSamplerAllocation(sampler);

	Sampler *samplerObject = getSampler(sampler);
	ASSERT(samplerObject);

	switch(pname)
	{
	case GL_TEXTURE_MIN_FILTER:         samplerObject->setMinFilter(static_cast<GLenum>(roundf(param)));   break;
	case GL_TEXTURE_MAG_FILTER:         samplerObject->setMagFilter(static_cast<GLenum>(roundf(param)));   break;
	case GL_TEXTURE_WRAP_S:             samplerObject->setWrapS(static_cast<GLenum>(roundf(param)));       break;
	case GL_TEXTURE_WRAP_T:             samplerObject->setWrapT(static_cast<GLenum>(roundf(param)));       break;
	case GL_TEXTURE_WRAP_R:             samplerObject->setWrapR(static_cast<GLenum>(roundf(param)));       break;
	case GL_TEXTURE_MIN_LOD:            samplerObject->setMinLod(param);                                   break;
	case GL_TEXTURE_MAX_LOD:            samplerObject->setMaxLod(param);                                   break;
	case GL_TEXTURE_COMPARE_MODE:       samplerObject->setCompareMode(static_cast<GLenum>(roundf(param))); break;
	case GL_TEXTURE_COMPARE_FUNC:       samplerObject->setCompareFunc(static_cast<GLenum>(roundf(param))); break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT: samplerObject->setMaxAnisotropy(param);                            break;
	default:                            UNREACHABLE(pname); break;
	}
}

GLint Context::getSamplerParameteri(GLuint sampler, GLenum pname)
{
	mResourceManager->checkSamplerAllocation(sampler);

	Sampler *samplerObject = getSampler(sampler);
	ASSERT(samplerObject);

	switch(pname)
	{
	case GL_TEXTURE_MIN_FILTER:         return static_cast<GLint>(samplerObject->getMinFilter());
	case GL_TEXTURE_MAG_FILTER:         return static_cast<GLint>(samplerObject->getMagFilter());
	case GL_TEXTURE_WRAP_S:             return static_cast<GLint>(samplerObject->getWrapS());
	case GL_TEXTURE_WRAP_T:             return static_cast<GLint>(samplerObject->getWrapT());
	case GL_TEXTURE_WRAP_R:             return static_cast<GLint>(samplerObject->getWrapR());
	case GL_TEXTURE_MIN_LOD:            return static_cast<GLint>(roundf(samplerObject->getMinLod()));
	case GL_TEXTURE_MAX_LOD:            return static_cast<GLint>(roundf(samplerObject->getMaxLod()));
	case GL_TEXTURE_COMPARE_MODE:       return static_cast<GLint>(samplerObject->getCompareMode());
	case GL_TEXTURE_COMPARE_FUNC:       return static_cast<GLint>(samplerObject->getCompareFunc());
	case GL_TEXTURE_MAX_ANISOTROPY_EXT: return static_cast<GLint>(samplerObject->getMaxAnisotropy());
	default:                            UNREACHABLE(pname); return 0;
	}
}

GLfloat Context::getSamplerParameterf(GLuint sampler, GLenum pname)
{
	mResourceManager->checkSamplerAllocation(sampler);

	Sampler *samplerObject = getSampler(sampler);
	ASSERT(samplerObject);

	switch(pname)
	{
	case GL_TEXTURE_MIN_FILTER:         return static_cast<GLfloat>(samplerObject->getMinFilter());
	case GL_TEXTURE_MAG_FILTER:         return static_cast<GLfloat>(samplerObject->getMagFilter());
	case GL_TEXTURE_WRAP_S:             return static_cast<GLfloat>(samplerObject->getWrapS());
	case GL_TEXTURE_WRAP_T:             return static_cast<GLfloat>(samplerObject->getWrapT());
	case GL_TEXTURE_WRAP_R:             return static_cast<GLfloat>(samplerObject->getWrapR());
	case GL_TEXTURE_MIN_LOD:            return samplerObject->getMinLod();
	case GL_TEXTURE_MAX_LOD:            return samplerObject->getMaxLod();
	case GL_TEXTURE_COMPARE_MODE:       return static_cast<GLfloat>(samplerObject->getCompareMode());
	case GL_TEXTURE_COMPARE_FUNC:       return static_cast<GLfloat>(samplerObject->getCompareFunc());
	case GL_TEXTURE_MAX_ANISOTROPY_EXT: return samplerObject->getMaxAnisotropy();
	default:                            UNREACHABLE(pname); return 0;
	}
}

bool Context::getBooleanv(GLenum pname, GLboolean *params) const
{
	switch(pname)
	{
	case GL_SHADER_COMPILER:          *params = GL_TRUE;                          break;
	case GL_SAMPLE_COVERAGE_INVERT:   *params = mState.sampleCoverageInvert;      break;
	case GL_DEPTH_WRITEMASK:          *params = mState.depthMask;                 break;
	case GL_COLOR_WRITEMASK:
		params[0] = mState.colorMaskRed;
		params[1] = mState.colorMaskGreen;
		params[2] = mState.colorMaskBlue;
		params[3] = mState.colorMaskAlpha;
		break;
	case GL_CULL_FACE:                *params = mState.cullFaceEnabled;                  break;
	case GL_POLYGON_OFFSET_FILL:      *params = mState.polygonOffsetFillEnabled;         break;
	case GL_SAMPLE_ALPHA_TO_COVERAGE: *params = mState.sampleAlphaToCoverageEnabled;     break;
	case GL_SAMPLE_COVERAGE:          *params = mState.sampleCoverageEnabled;            break;
	case GL_SCISSOR_TEST:             *params = mState.scissorTestEnabled;               break;
	case GL_STENCIL_TEST:             *params = mState.stencilTestEnabled;               break;
	case GL_DEPTH_TEST:               *params = mState.depthTestEnabled;                 break;
	case GL_BLEND:                    *params = mState.blendEnabled;                     break;
	case GL_DITHER:                   *params = mState.ditherEnabled;                    break;
	case GL_PRIMITIVE_RESTART_FIXED_INDEX: *params = mState.primitiveRestartFixedIndexEnabled; break;
	case GL_RASTERIZER_DISCARD:       *params = mState.rasterizerDiscardEnabled;         break;
	case GL_TRANSFORM_FEEDBACK_ACTIVE:
		{
			TransformFeedback* transformFeedback = getTransformFeedback(mState.transformFeedback);
			if(transformFeedback)
			{
				*params = transformFeedback->isActive();
				break;
			}
			else return false;
		}
	 case GL_TRANSFORM_FEEDBACK_PAUSED:
		{
			TransformFeedback* transformFeedback = getTransformFeedback(mState.transformFeedback);
			if(transformFeedback)
			{
				*params = transformFeedback->isPaused();
				break;
			}
			else return false;
		}
	default:
		return false;
	}

	return true;
}

bool Context::getFloatv(GLenum pname, GLfloat *params) const
{
	// Please note: DEPTH_CLEAR_VALUE is included in our internal getFloatv implementation
	// because it is stored as a float, despite the fact that the GL ES 2.0 spec names
	// GetIntegerv as its native query function. As it would require conversion in any
	// case, this should make no difference to the calling application.
	switch(pname)
	{
	case GL_LINE_WIDTH:               *params = mState.lineWidth;            break;
	case GL_SAMPLE_COVERAGE_VALUE:    *params = mState.sampleCoverageValue;  break;
	case GL_DEPTH_CLEAR_VALUE:        *params = mState.depthClearValue;      break;
	case GL_POLYGON_OFFSET_FACTOR:    *params = mState.polygonOffsetFactor;  break;
	case GL_POLYGON_OFFSET_UNITS:     *params = mState.polygonOffsetUnits;   break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		params[0] = ALIASED_LINE_WIDTH_RANGE_MIN;
		params[1] = ALIASED_LINE_WIDTH_RANGE_MAX;
		break;
	case GL_ALIASED_POINT_SIZE_RANGE:
		params[0] = ALIASED_POINT_SIZE_RANGE_MIN;
		params[1] = ALIASED_POINT_SIZE_RANGE_MAX;
		break;
	case GL_DEPTH_RANGE:
		params[0] = mState.zNear;
		params[1] = mState.zFar;
		break;
	case GL_COLOR_CLEAR_VALUE:
		params[0] = mState.colorClearValue.red;
		params[1] = mState.colorClearValue.green;
		params[2] = mState.colorClearValue.blue;
		params[3] = mState.colorClearValue.alpha;
		break;
	case GL_BLEND_COLOR:
		params[0] = mState.blendColor.red;
		params[1] = mState.blendColor.green;
		params[2] = mState.blendColor.blue;
		params[3] = mState.blendColor.alpha;
		break;
	case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
		*params = MAX_TEXTURE_MAX_ANISOTROPY;
		break;
	default:
		return false;
	}

	return true;
}

template bool Context::getIntegerv<GLint>(GLenum pname, GLint *params) const;
template bool Context::getIntegerv<GLint64>(GLenum pname, GLint64 *params) const;

template<typename T> bool Context::getIntegerv(GLenum pname, T *params) const
{
	// Please note: DEPTH_CLEAR_VALUE is not included in our internal getIntegerv implementation
	// because it is stored as a float, despite the fact that the GL ES 2.0 spec names
	// GetIntegerv as its native query function. As it would require conversion in any
	// case, this should make no difference to the calling application. You may find it in
	// Context::getFloatv.
	switch(pname)
	{
	case GL_MAX_VERTEX_ATTRIBS:               *params = MAX_VERTEX_ATTRIBS;               return true;
	case GL_MAX_VERTEX_UNIFORM_VECTORS:       *params = MAX_VERTEX_UNIFORM_VECTORS;       return true;
	case GL_MAX_VARYING_VECTORS:              *params = MAX_VARYING_VECTORS;              return true;
	case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: *params = MAX_COMBINED_TEXTURE_IMAGE_UNITS; return true;
	case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:   *params = MAX_VERTEX_TEXTURE_IMAGE_UNITS;   return true;
	case GL_MAX_TEXTURE_IMAGE_UNITS:          *params = MAX_TEXTURE_IMAGE_UNITS;          return true;
	case GL_MAX_FRAGMENT_UNIFORM_VECTORS:     *params = MAX_FRAGMENT_UNIFORM_VECTORS;     return true;
	case GL_MAX_RENDERBUFFER_SIZE:            *params = IMPLEMENTATION_MAX_RENDERBUFFER_SIZE; return true;
	case GL_NUM_SHADER_BINARY_FORMATS:        *params = 0;                                    return true;
	case GL_SHADER_BINARY_FORMATS:      /* no shader binary formats are supported */          return true;
	case GL_ARRAY_BUFFER_BINDING:             *params = getArrayBufferName();                 return true;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:     *params = getElementArrayBufferName();          return true;
//	case GL_FRAMEBUFFER_BINDING:            // now equivalent to GL_DRAW_FRAMEBUFFER_BINDING_ANGLE
	case GL_DRAW_FRAMEBUFFER_BINDING:         *params = mState.drawFramebuffer;               return true;
	case GL_READ_FRAMEBUFFER_BINDING:         *params = mState.readFramebuffer;               return true;
	case GL_RENDERBUFFER_BINDING:             *params = mState.renderbuffer.name();           return true;
	case GL_CURRENT_PROGRAM:                  *params = mState.currentProgram;                return true;
	case GL_PACK_ALIGNMENT:                   *params = mState.packParameters.alignment;                 return true;
	case GL_UNPACK_ALIGNMENT:                 *params = mState.unpackParameters.alignment;          return true;
	case GL_GENERATE_MIPMAP_HINT:             *params = mState.generateMipmapHint;            return true;
	case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES: *params = mState.fragmentShaderDerivativeHint; return true;
	case GL_TEXTURE_FILTERING_HINT_CHROMIUM:  *params = mState.textureFilteringHint;          return true;
	case GL_ACTIVE_TEXTURE:                   *params = (mState.activeSampler + GL_TEXTURE0); return true;
	case GL_STENCIL_FUNC:                     *params = mState.stencilFunc;                   return true;
	case GL_STENCIL_REF:                      *params = mState.stencilRef;                    return true;
	case GL_STENCIL_VALUE_MASK:               *params = sw::clampToSignedInt(mState.stencilMask); return true;
	case GL_STENCIL_BACK_FUNC:                *params = mState.stencilBackFunc;               return true;
	case GL_STENCIL_BACK_REF:                 *params = mState.stencilBackRef;                return true;
	case GL_STENCIL_BACK_VALUE_MASK:          *params = sw::clampToSignedInt(mState.stencilBackMask); return true;
	case GL_STENCIL_FAIL:                     *params = mState.stencilFail;                   return true;
	case GL_STENCIL_PASS_DEPTH_FAIL:          *params = mState.stencilPassDepthFail;          return true;
	case GL_STENCIL_PASS_DEPTH_PASS:          *params = mState.stencilPassDepthPass;          return true;
	case GL_STENCIL_BACK_FAIL:                *params = mState.stencilBackFail;               return true;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL:     *params = mState.stencilBackPassDepthFail;      return true;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS:     *params = mState.stencilBackPassDepthPass;      return true;
	case GL_DEPTH_FUNC:                       *params = mState.depthFunc;                     return true;
	case GL_BLEND_SRC_RGB:                    *params = mState.sourceBlendRGB;                return true;
	case GL_BLEND_SRC_ALPHA:                  *params = mState.sourceBlendAlpha;              return true;
	case GL_BLEND_DST_RGB:                    *params = mState.destBlendRGB;                  return true;
	case GL_BLEND_DST_ALPHA:                  *params = mState.destBlendAlpha;                return true;
	case GL_BLEND_EQUATION_RGB:               *params = mState.blendEquationRGB;              return true;
	case GL_BLEND_EQUATION_ALPHA:             *params = mState.blendEquationAlpha;            return true;
	case GL_STENCIL_WRITEMASK:                *params = sw::clampToSignedInt(mState.stencilWritemask); return true;
	case GL_STENCIL_BACK_WRITEMASK:           *params = sw::clampToSignedInt(mState.stencilBackWritemask); return true;
	case GL_STENCIL_CLEAR_VALUE:              *params = mState.stencilClearValue;             return true;
	case GL_SUBPIXEL_BITS:                    *params = 4;                                    return true;
	case GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB:
	case GL_MAX_TEXTURE_SIZE:                 *params = IMPLEMENTATION_MAX_TEXTURE_SIZE;          return true;
	case GL_MAX_CUBE_MAP_TEXTURE_SIZE:        *params = IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE; return true;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS:   *params = NUM_COMPRESSED_TEXTURE_FORMATS;           return true;
	case GL_MAX_SAMPLES:                      *params = IMPLEMENTATION_MAX_SAMPLES;               return true;
	case GL_SAMPLE_BUFFERS:
	case GL_SAMPLES:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			int width, height, samples;

			if(framebuffer && (framebuffer->completeness(width, height, samples) == GL_FRAMEBUFFER_COMPLETE))
			{
				switch(pname)
				{
				case GL_SAMPLE_BUFFERS:
					if(samples > 1)
					{
						*params = 1;
					}
					else
					{
						*params = 0;
					}
					break;
				case GL_SAMPLES:
					*params = samples;
					break;
				}
			}
			else
			{
				*params = 0;
			}
		}
		return true;
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:
		{
			Framebuffer *framebuffer = getReadFramebuffer();
			if(framebuffer)
			{
				*params = framebuffer->getImplementationColorReadType();
			}
			else
			{
				return error(GL_INVALID_OPERATION, true);
			}
		}
		return true;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
		{
			Framebuffer *framebuffer = getReadFramebuffer();
			if(framebuffer)
			{
				*params = framebuffer->getImplementationColorReadFormat();
			}
			else
			{
				return error(GL_INVALID_OPERATION, true);
			}
		}
		return true;
	case GL_MAX_VIEWPORT_DIMS:
		{
			int maxDimension = IMPLEMENTATION_MAX_RENDERBUFFER_SIZE;
			params[0] = maxDimension;
			params[1] = maxDimension;
		}
		return true;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		{
			for(int i = 0; i < NUM_COMPRESSED_TEXTURE_FORMATS; i++)
			{
				params[i] = compressedTextureFormats[i];
			}
		}
		return true;
	case GL_VIEWPORT:
		params[0] = mState.viewportX;
		params[1] = mState.viewportY;
		params[2] = mState.viewportWidth;
		params[3] = mState.viewportHeight;
		return true;
	case GL_SCISSOR_BOX:
		params[0] = mState.scissorX;
		params[1] = mState.scissorY;
		params[2] = mState.scissorWidth;
		params[3] = mState.scissorHeight;
		return true;
	case GL_CULL_FACE_MODE:                   *params = mState.cullMode;                 return true;
	case GL_FRONT_FACE:                       *params = mState.frontFace;                return true;
	case GL_RED_BITS:
	case GL_GREEN_BITS:
	case GL_BLUE_BITS:
	case GL_ALPHA_BITS:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			Renderbuffer *colorbuffer = framebuffer ? framebuffer->getColorbuffer(0) : nullptr;

			if(colorbuffer)
			{
				switch(pname)
				{
				case GL_RED_BITS:   *params = colorbuffer->getRedSize();   return true;
				case GL_GREEN_BITS: *params = colorbuffer->getGreenSize(); return true;
				case GL_BLUE_BITS:  *params = colorbuffer->getBlueSize();  return true;
				case GL_ALPHA_BITS: *params = colorbuffer->getAlphaSize(); return true;
				}
			}
			else
			{
				*params = 0;
			}
		}
		return true;
	case GL_DEPTH_BITS:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			Renderbuffer *depthbuffer = framebuffer ? framebuffer->getDepthbuffer() : nullptr;

			if(depthbuffer)
			{
				*params = depthbuffer->getDepthSize();
			}
			else
			{
				*params = 0;
			}
		}
		return true;
	case GL_STENCIL_BITS:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			Renderbuffer *stencilbuffer = framebuffer ? framebuffer->getStencilbuffer() : nullptr;

			if(stencilbuffer)
			{
				*params = stencilbuffer->getStencilSize();
			}
			else
			{
				*params = 0;
			}
		}
		return true;
	case GL_TEXTURE_BINDING_2D:
		if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
		{
			error(GL_INVALID_OPERATION);
			return false;
		}

		*params = mState.samplerTexture[TEXTURE_2D][mState.activeSampler].name();
		return true;
	case GL_TEXTURE_BINDING_CUBE_MAP:
		if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
		{
			error(GL_INVALID_OPERATION);
			return false;
		}

		*params = mState.samplerTexture[TEXTURE_CUBE][mState.activeSampler].name();
		return true;
	case GL_TEXTURE_BINDING_RECTANGLE_ARB:
		if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
		{
			error(GL_INVALID_OPERATION);
			return false;
		}

		*params = mState.samplerTexture[TEXTURE_2D_RECT][mState.activeSampler].name();
		return true;
	case GL_TEXTURE_BINDING_EXTERNAL_OES:
		if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
		{
			error(GL_INVALID_OPERATION);
			return false;
		}

		*params = mState.samplerTexture[TEXTURE_EXTERNAL][mState.activeSampler].name();
		return true;
	case GL_TEXTURE_BINDING_3D_OES:
		if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
		{
			error(GL_INVALID_OPERATION);
			return false;
		}

		*params = mState.samplerTexture[TEXTURE_3D][mState.activeSampler].name();
		return true;
	case GL_DRAW_BUFFER0:
	case GL_DRAW_BUFFER1:
	case GL_DRAW_BUFFER2:
	case GL_DRAW_BUFFER3:
	case GL_DRAW_BUFFER4:
	case GL_DRAW_BUFFER5:
	case GL_DRAW_BUFFER6:
	case GL_DRAW_BUFFER7:
	case GL_DRAW_BUFFER8:
	case GL_DRAW_BUFFER9:
	case GL_DRAW_BUFFER10:
	case GL_DRAW_BUFFER11:
	case GL_DRAW_BUFFER12:
	case GL_DRAW_BUFFER13:
	case GL_DRAW_BUFFER14:
	case GL_DRAW_BUFFER15:
		if((pname - GL_DRAW_BUFFER0) < MAX_DRAW_BUFFERS)
		{
			Framebuffer* framebuffer = getDrawFramebuffer();
			*params = framebuffer ? framebuffer->getDrawBuffer(pname - GL_DRAW_BUFFER0) : GL_NONE;
		}
		else
		{
			return false;
		}
		return true;
	case GL_MAX_DRAW_BUFFERS:
		*params = MAX_DRAW_BUFFERS;
		return true;
	case GL_MAX_COLOR_ATTACHMENTS: // Note: MAX_COLOR_ATTACHMENTS_EXT added by GL_EXT_draw_buffers
		*params = MAX_COLOR_ATTACHMENTS;
		return true;
	default:
		break;
	}

	if(clientVersion >= 3)
	{
		switch(pname)
		{
		case GL_TEXTURE_BINDING_2D_ARRAY:
			if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
			{
				error(GL_INVALID_OPERATION);
				return false;
			}

			*params = mState.samplerTexture[TEXTURE_2D_ARRAY][mState.activeSampler].name();
			return true;
		case GL_COPY_READ_BUFFER_BINDING:
			*params = mState.copyReadBuffer.name();
			return true;
		case GL_COPY_WRITE_BUFFER_BINDING:
			*params = mState.copyWriteBuffer.name();
			return true;
		case GL_MAJOR_VERSION:
			*params = clientVersion;
			return true;
		case GL_MAX_3D_TEXTURE_SIZE:
			*params = IMPLEMENTATION_MAX_3D_TEXTURE_SIZE;
			return true;
		case GL_MAX_ARRAY_TEXTURE_LAYERS:
			*params = IMPLEMENTATION_MAX_TEXTURE_SIZE;
			return true;
		case GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS:
			*params = MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS;
			return true;
		case GL_MAX_COMBINED_UNIFORM_BLOCKS:
			*params = MAX_VERTEX_UNIFORM_BLOCKS + MAX_FRAGMENT_UNIFORM_BLOCKS;
			return true;
		case GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS:
			*params = MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS;
			return true;
		case GL_MAX_ELEMENT_INDEX:
			*params = MAX_ELEMENT_INDEX;
			return true;
		case GL_MAX_ELEMENTS_INDICES:
			*params = MAX_ELEMENTS_INDICES;
			return true;
		case GL_MAX_ELEMENTS_VERTICES:
			*params = MAX_ELEMENTS_VERTICES;
			return true;
		case GL_MAX_FRAGMENT_INPUT_COMPONENTS:
			*params = MAX_FRAGMENT_INPUT_VECTORS * 4;
			return true;
		case GL_MAX_FRAGMENT_UNIFORM_BLOCKS:
			*params = MAX_FRAGMENT_UNIFORM_BLOCKS;
			return true;
		case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
			*params = MAX_FRAGMENT_UNIFORM_COMPONENTS;
			return true;
		case GL_MAX_PROGRAM_TEXEL_OFFSET:
			// Note: SwiftShader has no actual texel offset limit, so this limit can be modified if required.
			// In any case, any behavior outside the specified range is valid since the spec mentions:
			// (see OpenGL ES 3.0.5, 3.8.10.1 Scale Factor and Level of Detail, p.153)
			// "If any of the offset values are outside the range of the  implementation-defined values
			//  MIN_PROGRAM_TEXEL_OFFSET and MAX_PROGRAM_TEXEL_OFFSET, results of the texture lookup are
			//  undefined."
			*params = MAX_PROGRAM_TEXEL_OFFSET;
			return true;
		case GL_MAX_SERVER_WAIT_TIMEOUT:
			*params = 0;
			return true;
		case GL_MAX_TEXTURE_LOD_BIAS:
			*params = MAX_TEXTURE_LOD_BIAS;
			return true;
		case GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:
			*params = sw::MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS;
			return true;
		case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS:
			*params = MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS;
			return true;
		case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS:
			*params = sw::MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS;
			return true;
		case GL_MAX_UNIFORM_BLOCK_SIZE:
			*params = MAX_UNIFORM_BLOCK_SIZE;
			return true;
		case GL_MAX_UNIFORM_BUFFER_BINDINGS:
			*params = MAX_UNIFORM_BUFFER_BINDINGS;
			return true;
		case GL_MAX_VARYING_COMPONENTS:
			*params = MAX_VARYING_VECTORS * 4;
			return true;
		case GL_MAX_VERTEX_OUTPUT_COMPONENTS:
			*params = MAX_VERTEX_OUTPUT_VECTORS * 4;
			return true;
		case GL_MAX_VERTEX_UNIFORM_BLOCKS:
			*params = MAX_VERTEX_UNIFORM_BLOCKS;
			return true;
		case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
			*params = MAX_VERTEX_UNIFORM_COMPONENTS;
			return true;
		case GL_MIN_PROGRAM_TEXEL_OFFSET:
			// Note: SwiftShader has no actual texel offset limit, so this limit can be modified if required.
			// In any case, any behavior outside the specified range is valid since the spec mentions:
			// (see OpenGL ES 3.0.5, 3.8.10.1 Scale Factor and Level of Detail, p.153)
			// "If any of the offset values are outside the range of the  implementation-defined values
			//  MIN_PROGRAM_TEXEL_OFFSET and MAX_PROGRAM_TEXEL_OFFSET, results of the texture lookup are
			//  undefined."
			*params = MIN_PROGRAM_TEXEL_OFFSET;
			return true;
		case GL_MINOR_VERSION:
			*params = 0;
			return true;
		case GL_NUM_EXTENSIONS:
			GLuint numExtensions;
			getExtensions(0, &numExtensions);
			*params = numExtensions;
			return true;
		case GL_NUM_PROGRAM_BINARY_FORMATS:
			*params = NUM_PROGRAM_BINARY_FORMATS;
			return true;
		case GL_PACK_ROW_LENGTH:
			*params = mState.packParameters.rowLength;
			return true;
		case GL_PACK_SKIP_PIXELS:
			*params = mState.packParameters.skipPixels;
			return true;
		case GL_PACK_SKIP_ROWS:
			*params = mState.packParameters.skipRows;
			return true;
		case GL_PIXEL_PACK_BUFFER_BINDING:
			*params = mState.pixelPackBuffer.name();
			return true;
		case GL_PIXEL_UNPACK_BUFFER_BINDING:
			*params = mState.pixelUnpackBuffer.name();
			return true;
		case GL_PROGRAM_BINARY_FORMATS:
			// Since NUM_PROGRAM_BINARY_FORMATS is 0, the input
			// should be a 0 sized array, so don't write to params
			return true;
		case GL_READ_BUFFER:
			{
				Framebuffer* framebuffer = getReadFramebuffer();
				*params = framebuffer ? framebuffer->getReadBuffer() : GL_NONE;
			}
			return true;
		case GL_SAMPLER_BINDING:
			*params = mState.sampler[mState.activeSampler].name();
			return true;
		case GL_UNIFORM_BUFFER_BINDING:
			*params = mState.genericUniformBuffer.name();
			return true;
		case GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
			*params = UNIFORM_BUFFER_OFFSET_ALIGNMENT;
			return true;
		case GL_UNIFORM_BUFFER_SIZE:
			*params = static_cast<T>(mState.genericUniformBuffer->size());
			return true;
		case GL_UNIFORM_BUFFER_START:
			*params = static_cast<T>(mState.genericUniformBuffer->offset());
			return true;
		case GL_UNPACK_IMAGE_HEIGHT:
			*params = mState.unpackParameters.imageHeight;
			return true;
		case GL_UNPACK_ROW_LENGTH:
			*params = mState.unpackParameters.rowLength;
			return true;
		case GL_UNPACK_SKIP_IMAGES:
			*params = mState.unpackParameters.skipImages;
			return true;
		case GL_UNPACK_SKIP_PIXELS:
			*params = mState.unpackParameters.skipPixels;
			return true;
		case GL_UNPACK_SKIP_ROWS:
			*params = mState.unpackParameters.skipRows;
			return true;
		case GL_VERTEX_ARRAY_BINDING:
			*params = getCurrentVertexArray()->name;
			return true;
		case GL_TRANSFORM_FEEDBACK_BINDING:
			{
				TransformFeedback* transformFeedback = getTransformFeedback(mState.transformFeedback);
				if(transformFeedback)
				{
					*params = transformFeedback->name;
				}
				else
				{
					return false;
				}
			}
			return true;
		case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
			{
				TransformFeedback* transformFeedback = getTransformFeedback(mState.transformFeedback);
				if(transformFeedback)
				{
					*params = transformFeedback->getGenericBufferName();
				}
				else
				{
					return false;
				}
			}
			return true;
		default:
			break;
		}
	}

	return false;
}

template bool Context::getTransformFeedbackiv<GLint>(GLuint index, GLenum pname, GLint *param) const;
template bool Context::getTransformFeedbackiv<GLint64>(GLuint index, GLenum pname, GLint64 *param) const;

template<typename T> bool Context::getTransformFeedbackiv(GLuint index, GLenum pname, T *param) const
{
	TransformFeedback* transformFeedback = getTransformFeedback(mState.transformFeedback);
	if(!transformFeedback)
	{
		return false;
	}

	switch(pname)
	{
	case GL_TRANSFORM_FEEDBACK_BINDING: // GLint, initially 0
		*param = transformFeedback->name;
		break;
	case GL_TRANSFORM_FEEDBACK_ACTIVE: // boolean, initially GL_FALSE
		*param = transformFeedback->isActive();
		break;
	case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING: // name, initially 0
		*param = transformFeedback->getBufferName(index);
		break;
	case GL_TRANSFORM_FEEDBACK_PAUSED: // boolean, initially GL_FALSE
		*param = transformFeedback->isPaused();
		break;
	case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE: // indexed[n] 64-bit integer, initially 0
		if(transformFeedback->getBuffer(index))
		{
			*param = transformFeedback->getSize(index);
			break;
		}
		else return false;
	case GL_TRANSFORM_FEEDBACK_BUFFER_START: // indexed[n] 64-bit integer, initially 0
		if(transformFeedback->getBuffer(index))
		{
			*param = transformFeedback->getOffset(index);
		break;
		}
		else return false;
	default:
		return false;
	}

	return true;
}

template bool Context::getUniformBufferiv<GLint>(GLuint index, GLenum pname, GLint *param) const;
template bool Context::getUniformBufferiv<GLint64>(GLuint index, GLenum pname, GLint64 *param) const;

template<typename T> bool Context::getUniformBufferiv(GLuint index, GLenum pname, T *param) const
{
	switch(pname)
	{
	case GL_UNIFORM_BUFFER_BINDING:
	case GL_UNIFORM_BUFFER_SIZE:
	case GL_UNIFORM_BUFFER_START:
		if(index >= MAX_UNIFORM_BUFFER_BINDINGS)
		{
			return error(GL_INVALID_VALUE, true);
		}
		break;
	default:
		break;
	}

	const BufferBinding& uniformBuffer = mState.uniformBuffers[index];

	switch(pname)
	{
	case GL_UNIFORM_BUFFER_BINDING: // name, initially 0
		*param = uniformBuffer.get().name();
		break;
	case GL_UNIFORM_BUFFER_SIZE: // indexed[n] 64-bit integer, initially 0
		*param = uniformBuffer.getSize();
		break;
	case GL_UNIFORM_BUFFER_START: // indexed[n] 64-bit integer, initially 0
		*param = uniformBuffer.getOffset();
		break;
	default:
		return false;
	}

	return true;
}

bool Context::getQueryParameterInfo(GLenum pname, GLenum *type, unsigned int *numParams) const
{
	// Please note: the query type returned for DEPTH_CLEAR_VALUE in this implementation
	// is FLOAT rather than INT, as would be suggested by the GL ES 2.0 spec. This is due
	// to the fact that it is stored internally as a float, and so would require conversion
	// if returned from Context::getIntegerv. Since this conversion is already implemented
	// in the case that one calls glGetIntegerv to retrieve a float-typed state variable, we
	// place DEPTH_CLEAR_VALUE with the floats. This should make no difference to the calling
	// application.
	switch(pname)
	{
	case GL_COMPRESSED_TEXTURE_FORMATS:
		{
			*type = GL_INT;
			*numParams = NUM_COMPRESSED_TEXTURE_FORMATS;
		}
		break;
	case GL_SHADER_BINARY_FORMATS:
		{
			*type = GL_INT;
			*numParams = 0;
		}
		break;
	case GL_MAX_VERTEX_ATTRIBS:
	case GL_MAX_VERTEX_UNIFORM_VECTORS:
	case GL_MAX_VARYING_VECTORS:
	case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
	case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
	case GL_MAX_TEXTURE_IMAGE_UNITS:
	case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
	case GL_MAX_RENDERBUFFER_SIZE:
	case GL_NUM_SHADER_BINARY_FORMATS:
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
	case GL_ARRAY_BUFFER_BINDING:
	case GL_FRAMEBUFFER_BINDING:        // Same as GL_DRAW_FRAMEBUFFER_BINDING_ANGLE
	case GL_READ_FRAMEBUFFER_BINDING:   // Same as GL_READ_FRAMEBUFFER_BINDING_ANGLE
	case GL_RENDERBUFFER_BINDING:
	case GL_CURRENT_PROGRAM:
	case GL_PACK_ALIGNMENT:
	case GL_UNPACK_ALIGNMENT:
	case GL_GENERATE_MIPMAP_HINT:
	case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
	case GL_TEXTURE_FILTERING_HINT_CHROMIUM:
	case GL_RED_BITS:
	case GL_GREEN_BITS:
	case GL_BLUE_BITS:
	case GL_ALPHA_BITS:
	case GL_DEPTH_BITS:
	case GL_STENCIL_BITS:
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
	case GL_CULL_FACE_MODE:
	case GL_FRONT_FACE:
	case GL_ACTIVE_TEXTURE:
	case GL_STENCIL_FUNC:
	case GL_STENCIL_VALUE_MASK:
	case GL_STENCIL_REF:
	case GL_STENCIL_FAIL:
	case GL_STENCIL_PASS_DEPTH_FAIL:
	case GL_STENCIL_PASS_DEPTH_PASS:
	case GL_STENCIL_BACK_FUNC:
	case GL_STENCIL_BACK_VALUE_MASK:
	case GL_STENCIL_BACK_REF:
	case GL_STENCIL_BACK_FAIL:
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
	case GL_STENCIL_BACK_PASS_DEPTH_PASS:
	case GL_DEPTH_FUNC:
	case GL_BLEND_SRC_RGB:
	case GL_BLEND_SRC_ALPHA:
	case GL_BLEND_DST_RGB:
	case GL_BLEND_DST_ALPHA:
	case GL_BLEND_EQUATION_RGB:
	case GL_BLEND_EQUATION_ALPHA:
	case GL_STENCIL_WRITEMASK:
	case GL_STENCIL_BACK_WRITEMASK:
	case GL_STENCIL_CLEAR_VALUE:
	case GL_SUBPIXEL_BITS:
	case GL_MAX_TEXTURE_SIZE:
	case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
	case GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB:
	case GL_SAMPLE_BUFFERS:
	case GL_SAMPLES:
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
	case GL_TEXTURE_BINDING_2D:
	case GL_TEXTURE_BINDING_CUBE_MAP:
	case GL_TEXTURE_BINDING_RECTANGLE_ARB:
	case GL_TEXTURE_BINDING_EXTERNAL_OES:
	case GL_TEXTURE_BINDING_3D_OES:
	case GL_COPY_READ_BUFFER_BINDING:
	case GL_COPY_WRITE_BUFFER_BINDING:
	case GL_DRAW_BUFFER0:
	case GL_DRAW_BUFFER1:
	case GL_DRAW_BUFFER2:
	case GL_DRAW_BUFFER3:
	case GL_DRAW_BUFFER4:
	case GL_DRAW_BUFFER5:
	case GL_DRAW_BUFFER6:
	case GL_DRAW_BUFFER7:
	case GL_DRAW_BUFFER8:
	case GL_DRAW_BUFFER9:
	case GL_DRAW_BUFFER10:
	case GL_DRAW_BUFFER11:
	case GL_DRAW_BUFFER12:
	case GL_DRAW_BUFFER13:
	case GL_DRAW_BUFFER14:
	case GL_DRAW_BUFFER15:
	case GL_MAJOR_VERSION:
	case GL_MAX_3D_TEXTURE_SIZE:
	case GL_MAX_ARRAY_TEXTURE_LAYERS:
	case GL_MAX_COLOR_ATTACHMENTS:
	case GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS:
	case GL_MAX_COMBINED_UNIFORM_BLOCKS:
	case GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS:
	case GL_MAX_DRAW_BUFFERS:
	case GL_MAX_ELEMENT_INDEX:
	case GL_MAX_ELEMENTS_INDICES:
	case GL_MAX_ELEMENTS_VERTICES:
	case GL_MAX_FRAGMENT_INPUT_COMPONENTS:
	case GL_MAX_FRAGMENT_UNIFORM_BLOCKS:
	case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
	case GL_MAX_PROGRAM_TEXEL_OFFSET:
	case GL_MAX_SERVER_WAIT_TIMEOUT:
	case GL_MAX_TEXTURE_LOD_BIAS:
	case GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:
	case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS:
	case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS:
	case GL_MAX_UNIFORM_BLOCK_SIZE:
	case GL_MAX_UNIFORM_BUFFER_BINDINGS:
	case GL_MAX_VARYING_COMPONENTS:
	case GL_MAX_VERTEX_OUTPUT_COMPONENTS:
	case GL_MAX_VERTEX_UNIFORM_BLOCKS:
	case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
	case GL_MIN_PROGRAM_TEXEL_OFFSET:
	case GL_MINOR_VERSION:
	case GL_NUM_EXTENSIONS:
	case GL_NUM_PROGRAM_BINARY_FORMATS:
	case GL_PACK_ROW_LENGTH:
	case GL_PACK_SKIP_PIXELS:
	case GL_PACK_SKIP_ROWS:
	case GL_PIXEL_PACK_BUFFER_BINDING:
	case GL_PIXEL_UNPACK_BUFFER_BINDING:
	case GL_PROGRAM_BINARY_FORMATS:
	case GL_READ_BUFFER:
	case GL_SAMPLER_BINDING:
	case GL_TEXTURE_BINDING_2D_ARRAY:
	case GL_UNIFORM_BUFFER_BINDING:
	case GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
	case GL_UNIFORM_BUFFER_SIZE:
	case GL_UNIFORM_BUFFER_START:
	case GL_UNPACK_IMAGE_HEIGHT:
	case GL_UNPACK_ROW_LENGTH:
	case GL_UNPACK_SKIP_IMAGES:
	case GL_UNPACK_SKIP_PIXELS:
	case GL_UNPACK_SKIP_ROWS:
	case GL_VERTEX_ARRAY_BINDING:
	case GL_TRANSFORM_FEEDBACK_BINDING:
	case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
		{
			*type = GL_INT;
			*numParams = 1;
		}
		break;
	case GL_MAX_SAMPLES:
		{
			*type = GL_INT;
			*numParams = 1;
		}
		break;
	case GL_MAX_VIEWPORT_DIMS:
		{
			*type = GL_INT;
			*numParams = 2;
		}
		break;
	case GL_VIEWPORT:
	case GL_SCISSOR_BOX:
		{
			*type = GL_INT;
			*numParams = 4;
		}
		break;
	case GL_SHADER_COMPILER:
	case GL_SAMPLE_COVERAGE_INVERT:
	case GL_DEPTH_WRITEMASK:
	case GL_CULL_FACE:                // CULL_FACE through DITHER are natural to IsEnabled,
	case GL_POLYGON_OFFSET_FILL:      // but can be retrieved through the Get{Type}v queries.
	case GL_SAMPLE_ALPHA_TO_COVERAGE: // For this purpose, they are treated here as bool-natural
	case GL_SAMPLE_COVERAGE:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
	case GL_PRIMITIVE_RESTART_FIXED_INDEX:
	case GL_RASTERIZER_DISCARD:
	case GL_TRANSFORM_FEEDBACK_ACTIVE:
	case GL_TRANSFORM_FEEDBACK_PAUSED:
		{
			*type = GL_BOOL;
			*numParams = 1;
		}
		break;
	case GL_COLOR_WRITEMASK:
		{
			*type = GL_BOOL;
			*numParams = 4;
		}
		break;
	case GL_POLYGON_OFFSET_FACTOR:
	case GL_POLYGON_OFFSET_UNITS:
	case GL_SAMPLE_COVERAGE_VALUE:
	case GL_DEPTH_CLEAR_VALUE:
	case GL_LINE_WIDTH:
		{
			*type = GL_FLOAT;
			*numParams = 1;
		}
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
	case GL_ALIASED_POINT_SIZE_RANGE:
	case GL_DEPTH_RANGE:
		{
			*type = GL_FLOAT;
			*numParams = 2;
		}
		break;
	case GL_COLOR_CLEAR_VALUE:
	case GL_BLEND_COLOR:
		{
			*type = GL_FLOAT;
			*numParams = 4;
		}
		break;
	case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
		*type = GL_FLOAT;
		*numParams = 1;
		break;
	default:
		return false;
	}

	return true;
}

void Context::applyScissor(int width, int height)
{
	if(mState.scissorTestEnabled)
	{
		sw::Rect scissor = { mState.scissorX, mState.scissorY, mState.scissorX + mState.scissorWidth, mState.scissorY + mState.scissorHeight };
		scissor.clip(0, 0, width, height);

		device->setScissorRect(scissor);
		device->setScissorEnable(true);
	}
	else
	{
		device->setScissorEnable(false);
	}
}

// Applies the render target surface, depth stencil surface, viewport rectangle and scissor rectangle
bool Context::applyRenderTarget()
{
	Framebuffer *framebuffer = getDrawFramebuffer();
	int width, height, samples;

	if(!framebuffer || (framebuffer->completeness(width, height, samples) != GL_FRAMEBUFFER_COMPLETE))
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION, false);
	}

	for(int i = 0; i < MAX_DRAW_BUFFERS; i++)
	{
		if(framebuffer->getDrawBuffer(i) != GL_NONE)
		{
			egl::Image *renderTarget = framebuffer->getRenderTarget(i);
			GLint layer = framebuffer->getColorbufferLayer(i);
			device->setRenderTarget(i, renderTarget, layer);
			if(renderTarget) renderTarget->release();
		}
		else
		{
			device->setRenderTarget(i, nullptr, 0);
		}
	}

	egl::Image *depthBuffer = framebuffer->getDepthBuffer();
	GLint dLayer = framebuffer->getDepthbufferLayer();
	device->setDepthBuffer(depthBuffer, dLayer);
	if(depthBuffer) depthBuffer->release();

	egl::Image *stencilBuffer = framebuffer->getStencilBuffer();
	GLint sLayer = framebuffer->getStencilbufferLayer();
	device->setStencilBuffer(stencilBuffer, sLayer);
	if(stencilBuffer) stencilBuffer->release();

	Viewport viewport;
	float zNear = clamp01(mState.zNear);
	float zFar = clamp01(mState.zFar);

	viewport.x0 = mState.viewportX;
	viewport.y0 = mState.viewportY;
	viewport.width = mState.viewportWidth;
	viewport.height = mState.viewportHeight;
	viewport.minZ = zNear;
	viewport.maxZ = zFar;

	device->setViewport(viewport);

	applyScissor(width, height);

	Program *program = getCurrentProgram();

	if(program)
	{
		GLfloat nearFarDiff[3] = {zNear, zFar, zFar - zNear};
		program->setUniform1fv(program->getUniformLocation("gl_DepthRange.near"), 1, &nearFarDiff[0]);
		program->setUniform1fv(program->getUniformLocation("gl_DepthRange.far"), 1, &nearFarDiff[1]);
		program->setUniform1fv(program->getUniformLocation("gl_DepthRange.diff"), 1, &nearFarDiff[2]);
	}

	return true;
}

// Applies the fixed-function state (culling, depth test, alpha blending, stenciling, etc)
void Context::applyState(GLenum drawMode)
{
	Framebuffer *framebuffer = getDrawFramebuffer();

	if(mState.cullFaceEnabled)
	{
		device->setCullMode(es2sw::ConvertCullMode(mState.cullMode, mState.frontFace));
	}
	else
	{
		device->setCullMode(sw::CULL_NONE);
	}

	if(mDepthStateDirty)
	{
		if(mState.depthTestEnabled)
		{
			device->setDepthBufferEnable(true);
			device->setDepthCompare(es2sw::ConvertDepthComparison(mState.depthFunc));
		}
		else
		{
			device->setDepthBufferEnable(false);
		}

		mDepthStateDirty = false;
	}

	if(mBlendStateDirty)
	{
		if(mState.blendEnabled)
		{
			device->setAlphaBlendEnable(true);
			device->setSeparateAlphaBlendEnable(true);

			device->setBlendConstant(es2sw::ConvertColor(mState.blendColor));

			device->setSourceBlendFactor(es2sw::ConvertBlendFunc(mState.sourceBlendRGB));
			device->setDestBlendFactor(es2sw::ConvertBlendFunc(mState.destBlendRGB));
			device->setBlendOperation(es2sw::ConvertBlendOp(mState.blendEquationRGB));

			device->setSourceBlendFactorAlpha(es2sw::ConvertBlendFunc(mState.sourceBlendAlpha));
			device->setDestBlendFactorAlpha(es2sw::ConvertBlendFunc(mState.destBlendAlpha));
			device->setBlendOperationAlpha(es2sw::ConvertBlendOp(mState.blendEquationAlpha));
		}
		else
		{
			device->setAlphaBlendEnable(false);
		}

		mBlendStateDirty = false;
	}

	if(mStencilStateDirty || mFrontFaceDirty)
	{
		if(mState.stencilTestEnabled && framebuffer->hasStencil())
		{
			device->setStencilEnable(true);
			device->setTwoSidedStencil(true);

			// get the maximum size of the stencil ref
			Renderbuffer *stencilbuffer = framebuffer->getStencilbuffer();
			GLuint maxStencil = (1 << stencilbuffer->getStencilSize()) - 1;

			if(mState.frontFace == GL_CCW)
			{
				device->setStencilWriteMask(mState.stencilWritemask);
				device->setStencilCompare(es2sw::ConvertStencilComparison(mState.stencilFunc));

				device->setStencilReference((mState.stencilRef < (GLint)maxStencil) ? mState.stencilRef : maxStencil);
				device->setStencilMask(mState.stencilMask);

				device->setStencilFailOperation(es2sw::ConvertStencilOp(mState.stencilFail));
				device->setStencilZFailOperation(es2sw::ConvertStencilOp(mState.stencilPassDepthFail));
				device->setStencilPassOperation(es2sw::ConvertStencilOp(mState.stencilPassDepthPass));

				device->setStencilWriteMaskCCW(mState.stencilBackWritemask);
				device->setStencilCompareCCW(es2sw::ConvertStencilComparison(mState.stencilBackFunc));

				device->setStencilReferenceCCW((mState.stencilBackRef < (GLint)maxStencil) ? mState.stencilBackRef : maxStencil);
				device->setStencilMaskCCW(mState.stencilBackMask);

				device->setStencilFailOperationCCW(es2sw::ConvertStencilOp(mState.stencilBackFail));
				device->setStencilZFailOperationCCW(es2sw::ConvertStencilOp(mState.stencilBackPassDepthFail));
				device->setStencilPassOperationCCW(es2sw::ConvertStencilOp(mState.stencilBackPassDepthPass));
			}
			else
			{
				device->setStencilWriteMaskCCW(mState.stencilWritemask);
				device->setStencilCompareCCW(es2sw::ConvertStencilComparison(mState.stencilFunc));

				device->setStencilReferenceCCW((mState.stencilRef < (GLint)maxStencil) ? mState.stencilRef : maxStencil);
				device->setStencilMaskCCW(mState.stencilMask);

				device->setStencilFailOperationCCW(es2sw::ConvertStencilOp(mState.stencilFail));
				device->setStencilZFailOperationCCW(es2sw::ConvertStencilOp(mState.stencilPassDepthFail));
				device->setStencilPassOperationCCW(es2sw::ConvertStencilOp(mState.stencilPassDepthPass));

				device->setStencilWriteMask(mState.stencilBackWritemask);
				device->setStencilCompare(es2sw::ConvertStencilComparison(mState.stencilBackFunc));

				device->setStencilReference((mState.stencilBackRef < (GLint)maxStencil) ? mState.stencilBackRef : maxStencil);
				device->setStencilMask(mState.stencilBackMask);

				device->setStencilFailOperation(es2sw::ConvertStencilOp(mState.stencilBackFail));
				device->setStencilZFailOperation(es2sw::ConvertStencilOp(mState.stencilBackPassDepthFail));
				device->setStencilPassOperation(es2sw::ConvertStencilOp(mState.stencilBackPassDepthPass));
			}
		}
		else
		{
			device->setStencilEnable(false);
		}

		mStencilStateDirty = false;
		mFrontFaceDirty = false;
	}

	if(mMaskStateDirty)
	{
		for(int i = 0; i < MAX_DRAW_BUFFERS; i++)
		{
			device->setColorWriteMask(i, es2sw::ConvertColorMask(mState.colorMaskRed, mState.colorMaskGreen, mState.colorMaskBlue, mState.colorMaskAlpha));
		}

		device->setDepthWriteEnable(mState.depthMask);

		mMaskStateDirty = false;
	}

	if(mPolygonOffsetStateDirty)
	{
		if(mState.polygonOffsetFillEnabled)
		{
			Renderbuffer *depthbuffer = framebuffer->getDepthbuffer();
			if(depthbuffer)
			{
				device->setSlopeDepthBias(mState.polygonOffsetFactor);
				float depthBias = ldexp(mState.polygonOffsetUnits, -23);   // We use 32-bit floating-point for all depth formats, with 23 mantissa bits.
				device->setDepthBias(depthBias);
			}
		}
		else
		{
			device->setSlopeDepthBias(0);
			device->setDepthBias(0);
		}

		mPolygonOffsetStateDirty = false;
	}

	if(mSampleStateDirty)
	{
		if(mState.sampleAlphaToCoverageEnabled)
		{
			device->setTransparencyAntialiasing(sw::TRANSPARENCY_ALPHA_TO_COVERAGE);
		}
		else
		{
			device->setTransparencyAntialiasing(sw::TRANSPARENCY_NONE);
		}

		if(mState.sampleCoverageEnabled)
		{
			unsigned int mask = 0;
			if(mState.sampleCoverageValue != 0)
			{
				int width, height, samples;
				framebuffer->completeness(width, height, samples);

				float threshold = 0.5f;

				for(int i = 0; i < samples; i++)
				{
					mask <<= 1;

					if((i + 1) * mState.sampleCoverageValue >= threshold)
					{
						threshold += 1.0f;
						mask |= 1;
					}
				}
			}

			if(mState.sampleCoverageInvert)
			{
				mask = ~mask;
			}

			device->setMultiSampleMask(mask);
		}
		else
		{
			device->setMultiSampleMask(0xFFFFFFFF);
		}

		mSampleStateDirty = false;
	}

	if(mDitherStateDirty)
	{
	//	UNIMPLEMENTED();   // FIXME

		mDitherStateDirty = false;
	}

	device->setRasterizerDiscard(mState.rasterizerDiscardEnabled);
}

GLenum Context::applyVertexBuffer(GLint base, GLint first, GLsizei count, GLsizei instanceId)
{
	TranslatedAttribute attributes[MAX_VERTEX_ATTRIBS];

	GLenum err = mVertexDataManager->prepareVertexData(first, count, attributes, instanceId);
	if(err != GL_NO_ERROR)
	{
		return err;
	}

	Program *program = getCurrentProgram();

	device->resetInputStreams(false);

	for(int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
	{
		if(program->getAttributeStream(i) == -1)
		{
			continue;
		}

		sw::Resource *resource = attributes[i].vertexBuffer;
		const void *buffer = (char*)resource->data() + attributes[i].offset;

		int stride = attributes[i].stride;

		buffer = (char*)buffer + stride * base;

		sw::Stream attribute(resource, buffer, stride);

		attribute.type = attributes[i].type;
		attribute.count = attributes[i].count;
		attribute.normalized = attributes[i].normalized;

		int stream = program->getAttributeStream(i);
		device->setInputStream(stream, attribute);
	}

	return GL_NO_ERROR;
}

// Applies the indices and element array bindings
GLenum Context::applyIndexBuffer(const void *indices, GLuint start, GLuint end, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo)
{
	GLenum err = mIndexDataManager->prepareIndexData(mode, type, start, end, count, getCurrentVertexArray()->getElementArrayBuffer(), indices, indexInfo, isPrimitiveRestartFixedIndexEnabled());

	if(err == GL_NO_ERROR)
	{
		device->setIndexBuffer(indexInfo->indexBuffer);
	}

	return err;
}

// Applies the shaders and shader constants
void Context::applyShaders()
{
	Program *programObject = getCurrentProgram();
	sw::VertexShader *vertexShader = programObject->getVertexShader();
	sw::PixelShader *pixelShader = programObject->getPixelShader();

	device->setVertexShader(vertexShader);
	device->setPixelShader(pixelShader);

	if(programObject->getSerial() != mAppliedProgramSerial)
	{
		programObject->dirtyAllUniforms();
		mAppliedProgramSerial = programObject->getSerial();
	}

	programObject->applyTransformFeedback(device, getTransformFeedback());
	programObject->applyUniformBuffers(device, mState.uniformBuffers);
	programObject->applyUniforms(device);
}

void Context::applyTextures()
{
	applyTextures(sw::SAMPLER_PIXEL);
	applyTextures(sw::SAMPLER_VERTEX);
}

void Context::applyTextures(sw::SamplerType samplerType)
{
	Program *programObject = getCurrentProgram();

	int samplerCount = (samplerType == sw::SAMPLER_PIXEL) ? MAX_TEXTURE_IMAGE_UNITS : MAX_VERTEX_TEXTURE_IMAGE_UNITS;   // Range of samplers of given sampler type

	for(int samplerIndex = 0; samplerIndex < samplerCount; samplerIndex++)
	{
		int textureUnit = programObject->getSamplerMapping(samplerType, samplerIndex);   // OpenGL texture image unit index

		if(textureUnit != -1)
		{
			TextureType textureType = programObject->getSamplerTextureType(samplerType, samplerIndex);

			Texture *texture = getSamplerTexture(textureUnit, textureType);

			if(texture->isSamplerComplete())
			{
				GLenum wrapS, wrapT, wrapR, minFilter, magFilter, compFunc, compMode;
				GLfloat minLOD, maxLOD, maxAnisotropy;

				Sampler *samplerObject = mState.sampler[textureUnit];
				if(samplerObject)
				{
					wrapS = samplerObject->getWrapS();
					wrapT = samplerObject->getWrapT();
					wrapR = samplerObject->getWrapR();
					minFilter = samplerObject->getMinFilter();
					magFilter = samplerObject->getMagFilter();
					minLOD = samplerObject->getMinLod();
					maxLOD = samplerObject->getMaxLod();
					compFunc = samplerObject->getCompareFunc();
					compMode = samplerObject->getCompareMode();
					maxAnisotropy = samplerObject->getMaxAnisotropy();
				}
				else
				{
					wrapS = texture->getWrapS();
					wrapT = texture->getWrapT();
					wrapR = texture->getWrapR();
					minFilter = texture->getMinFilter();
					magFilter = texture->getMagFilter();
					minLOD = texture->getMinLOD();
					maxLOD = texture->getMaxLOD();
					compFunc = texture->getCompareFunc();
					compMode = texture->getCompareMode();
					maxAnisotropy = texture->getMaxAnisotropy();
				}

				GLint baseLevel = texture->getBaseLevel();
				GLint maxLevel = texture->getMaxLevel();
				GLenum swizzleR = texture->getSwizzleR();
				GLenum swizzleG = texture->getSwizzleG();
				GLenum swizzleB = texture->getSwizzleB();
				GLenum swizzleA = texture->getSwizzleA();

				device->setAddressingModeU(samplerType, samplerIndex, es2sw::ConvertTextureWrap(wrapS));
				device->setAddressingModeV(samplerType, samplerIndex, es2sw::ConvertTextureWrap(wrapT));
				device->setAddressingModeW(samplerType, samplerIndex, es2sw::ConvertTextureWrap(wrapR));
				device->setCompareFunc(samplerType, samplerIndex, es2sw::ConvertCompareFunc(compFunc, compMode));
				device->setSwizzleR(samplerType, samplerIndex, es2sw::ConvertSwizzleType(swizzleR));
				device->setSwizzleG(samplerType, samplerIndex, es2sw::ConvertSwizzleType(swizzleG));
				device->setSwizzleB(samplerType, samplerIndex, es2sw::ConvertSwizzleType(swizzleB));
				device->setSwizzleA(samplerType, samplerIndex, es2sw::ConvertSwizzleType(swizzleA));
				device->setMinLod(samplerType, samplerIndex, minLOD);
				device->setMaxLod(samplerType, samplerIndex, maxLOD);
				device->setBaseLevel(samplerType, samplerIndex, baseLevel);
				device->setMaxLevel(samplerType, samplerIndex, maxLevel);
				device->setTextureFilter(samplerType, samplerIndex, es2sw::ConvertTextureFilter(minFilter, magFilter, maxAnisotropy));
				device->setMipmapFilter(samplerType, samplerIndex, es2sw::ConvertMipMapFilter(minFilter));
				device->setMaxAnisotropy(samplerType, samplerIndex, maxAnisotropy);
				device->setHighPrecisionFiltering(samplerType, samplerIndex, mState.textureFilteringHint == GL_NICEST);

				applyTexture(samplerType, samplerIndex, texture);
			}
			else
			{
				applyTexture(samplerType, samplerIndex, nullptr);
			}
		}
		else
		{
			applyTexture(samplerType, samplerIndex, nullptr);
		}
	}
}

void Context::applyTexture(sw::SamplerType type, int index, Texture *baseTexture)
{
	Program *program = getCurrentProgram();
	int sampler = (type == sw::SAMPLER_PIXEL) ? index : 16 + index;
	bool textureUsed = false;

	if(type == sw::SAMPLER_PIXEL)
	{
		textureUsed = program->getPixelShader()->usesSampler(index);
	}
	else if(type == sw::SAMPLER_VERTEX)
	{
		textureUsed = program->getVertexShader()->usesSampler(index);
	}
	else UNREACHABLE(type);

	sw::Resource *resource = nullptr;

	if(baseTexture && textureUsed)
	{
		resource = baseTexture->getResource();
	}

	device->setTextureResource(sampler, resource);

	if(baseTexture && textureUsed)
	{
		int baseLevel = baseTexture->getBaseLevel();
		int maxLevel = std::min(baseTexture->getTopLevel(), baseTexture->getMaxLevel());
		GLenum target = baseTexture->getTarget();

		switch(target)
		{
		case GL_TEXTURE_2D:
		case GL_TEXTURE_EXTERNAL_OES:
		case GL_TEXTURE_RECTANGLE_ARB:
			{
				Texture2D *texture = static_cast<Texture2D*>(baseTexture);

				for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
				{
					int surfaceLevel = mipmapLevel + baseLevel;

					if(surfaceLevel > maxLevel)
					{
						surfaceLevel = maxLevel;
					}

					egl::Image *surface = texture->getImage(surfaceLevel);
					device->setTextureLevel(sampler, 0, mipmapLevel, surface,
					                        (target == GL_TEXTURE_RECTANGLE_ARB) ? sw::TEXTURE_RECTANGLE : sw::TEXTURE_2D);
				}
			}
			break;
		case GL_TEXTURE_3D:
			{
				Texture3D *texture = static_cast<Texture3D*>(baseTexture);

				for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
				{
					int surfaceLevel = mipmapLevel + baseLevel;

					if(surfaceLevel > maxLevel)
					{
						surfaceLevel = maxLevel;
					}

					egl::Image *surface = texture->getImage(surfaceLevel);
					device->setTextureLevel(sampler, 0, mipmapLevel, surface, sw::TEXTURE_3D);
				}
			}
			break;
		case GL_TEXTURE_2D_ARRAY:
			{
				Texture2DArray *texture = static_cast<Texture2DArray*>(baseTexture);

				for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
				{
					int surfaceLevel = mipmapLevel + baseLevel;

					if(surfaceLevel > maxLevel)
					{
						surfaceLevel = maxLevel;
					}

					egl::Image *surface = texture->getImage(surfaceLevel);
					device->setTextureLevel(sampler, 0, mipmapLevel, surface, sw::TEXTURE_2D_ARRAY);
				}
			}
			break;
		case GL_TEXTURE_CUBE_MAP:
			{
				TextureCubeMap *cubeTexture = static_cast<TextureCubeMap*>(baseTexture);

				for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
				{
					cubeTexture->updateBorders(mipmapLevel);

					for(int face = 0; face < 6; face++)
					{
						int surfaceLevel = mipmapLevel + baseLevel;

						if(surfaceLevel > maxLevel)
						{
							surfaceLevel = maxLevel;
						}

						egl::Image *surface = cubeTexture->getImage(face, surfaceLevel);
						device->setTextureLevel(sampler, face, mipmapLevel, surface, sw::TEXTURE_CUBE);
					}
				}
			}
			break;
		default:
			UNIMPLEMENTED();
			break;
		}
	}
	else
	{
		device->setTextureLevel(sampler, 0, 0, 0, sw::TEXTURE_NULL);
	}
}

void Context::readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei *bufSize, void* pixels)
{
	Framebuffer *framebuffer = getReadFramebuffer();
	int framebufferWidth, framebufferHeight, framebufferSamples;

	if(!framebuffer || (framebuffer->completeness(framebufferWidth, framebufferHeight, framebufferSamples) != GL_FRAMEBUFFER_COMPLETE))
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION);
	}

	if(getReadFramebufferName() != 0 && framebufferSamples != 0)
	{
		return error(GL_INVALID_OPERATION);
	}

	if(!IsValidReadPixelsFormatType(framebuffer, format, type, clientVersion))
	{
		return error(GL_INVALID_OPERATION);
	}

	GLsizei outputWidth = (mState.packParameters.rowLength > 0) ? mState.packParameters.rowLength : width;
	GLsizei outputPitch = gl::ComputePitch(outputWidth, format, type, mState.packParameters.alignment);
	GLsizei outputHeight = (mState.packParameters.imageHeight == 0) ? height : mState.packParameters.imageHeight;
	pixels = getPixelPackBuffer() ? (unsigned char*)getPixelPackBuffer()->data() + (ptrdiff_t)pixels : (unsigned char*)pixels;
	pixels = ((char*)pixels) + gl::ComputePackingOffset(format, type, outputWidth, outputHeight, mState.packParameters);

	// Sized query sanity check
	if(bufSize)
	{
		int requiredSize = outputPitch * height;
		if(requiredSize > *bufSize)
		{
			return error(GL_INVALID_OPERATION);
		}
	}

	egl::Image *renderTarget = nullptr;
	switch(format)
	{
	case GL_DEPTH_COMPONENT:   // GL_NV_read_depth
		renderTarget = framebuffer->getDepthBuffer();
		break;
	default:
		renderTarget = framebuffer->getReadRenderTarget();
		break;
	}

	if(!renderTarget)
	{
		return error(GL_INVALID_OPERATION);
	}

	sw::RectF rect((float)x, (float)y, (float)(x + width), (float)(y + height));
	sw::Rect dstRect(0, 0, width, height);
	rect.clip(0.0f, 0.0f, (float)renderTarget->getWidth(), (float)renderTarget->getHeight());

	sw::Surface *externalSurface = sw::Surface::create(width, height, 1, gl::ConvertReadFormatType(format, type), pixels, outputPitch, outputPitch * outputHeight);
	sw::SliceRectF sliceRect(rect);
	sw::SliceRect dstSliceRect(dstRect);
	device->blit(renderTarget, sliceRect, externalSurface, dstSliceRect, false, false, false);
	delete externalSurface;

	renderTarget->release();
}

void Context::clear(GLbitfield mask)
{
	if(mState.rasterizerDiscardEnabled)
	{
		return;
	}

	Framebuffer *framebuffer = getDrawFramebuffer();

	if(!framebuffer || (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE))
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION);
	}

	if(!applyRenderTarget())
	{
		return;
	}

	if(mask & GL_COLOR_BUFFER_BIT)
	{
		unsigned int rgbaMask = getColorMask();

		if(rgbaMask != 0)
		{
			device->clearColor(mState.colorClearValue.red, mState.colorClearValue.green, mState.colorClearValue.blue, mState.colorClearValue.alpha, rgbaMask);
		}
	}

	if(mask & GL_DEPTH_BUFFER_BIT)
	{
		if(mState.depthMask != 0)
		{
			float depth = clamp01(mState.depthClearValue);
			device->clearDepth(depth);
		}
	}

	if(mask & GL_STENCIL_BUFFER_BIT)
	{
		if(mState.stencilWritemask != 0)
		{
			int stencil = mState.stencilClearValue & 0x000000FF;
			device->clearStencil(stencil, mState.stencilWritemask);
		}
	}
}

void Context::clearColorBuffer(GLint drawbuffer, void *value, sw::Format format)
{
	unsigned int rgbaMask = getColorMask();
	if(rgbaMask && !mState.rasterizerDiscardEnabled)
	{
		Framebuffer *framebuffer = getDrawFramebuffer();
		if(!framebuffer)
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION);
		}
		egl::Image *colorbuffer = framebuffer->getRenderTarget(drawbuffer);

		if(colorbuffer)
		{
			sw::Rect clearRect = colorbuffer->getRect();

			if(mState.scissorTestEnabled)
			{
				clearRect.clip(mState.scissorX, mState.scissorY, mState.scissorX + mState.scissorWidth, mState.scissorY + mState.scissorHeight);
			}

			device->clear(value, format, colorbuffer, clearRect, rgbaMask);

			colorbuffer->release();
		}
	}
}

void Context::clearColorBuffer(GLint drawbuffer, const GLint *value)
{
	clearColorBuffer(drawbuffer, (void*)value, sw::FORMAT_A32B32G32R32I);
}

void Context::clearColorBuffer(GLint drawbuffer, const GLuint *value)
{
	clearColorBuffer(drawbuffer, (void*)value, sw::FORMAT_A32B32G32R32UI);
}

void Context::clearColorBuffer(GLint drawbuffer, const GLfloat *value)
{
	clearColorBuffer(drawbuffer, (void*)value, sw::FORMAT_A32B32G32R32F);
}

void Context::clearDepthBuffer(const GLfloat value)
{
	if(mState.depthMask && !mState.rasterizerDiscardEnabled)
	{
		Framebuffer *framebuffer = getDrawFramebuffer();
		if(!framebuffer)
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION);
		}
		egl::Image *depthbuffer = framebuffer->getDepthBuffer();

		if(depthbuffer)
		{
			float depth = clamp01(value);
			sw::Rect clearRect = depthbuffer->getRect();

			if(mState.scissorTestEnabled)
			{
				clearRect.clip(mState.scissorX, mState.scissorY, mState.scissorX + mState.scissorWidth, mState.scissorY + mState.scissorHeight);
			}

			depthbuffer->clearDepth(depth, clearRect.x0, clearRect.y0, clearRect.width(), clearRect.height());

			depthbuffer->release();
		}
	}
}

void Context::clearStencilBuffer(const GLint value)
{
	if(mState.stencilWritemask && !mState.rasterizerDiscardEnabled)
	{
		Framebuffer *framebuffer = getDrawFramebuffer();
		if(!framebuffer)
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION);
		}
		egl::Image *stencilbuffer = framebuffer->getStencilBuffer();

		if(stencilbuffer)
		{
			unsigned char stencil = value < 0 ? 0 : static_cast<unsigned char>(value & 0x000000FF);
			sw::Rect clearRect = stencilbuffer->getRect();

			if(mState.scissorTestEnabled)
			{
				clearRect.clip(mState.scissorX, mState.scissorY, mState.scissorX + mState.scissorWidth, mState.scissorY + mState.scissorHeight);
			}

			stencilbuffer->clearStencil(stencil, static_cast<unsigned char>(mState.stencilWritemask), clearRect.x0, clearRect.y0, clearRect.width(), clearRect.height());

			stencilbuffer->release();
		}
	}
}

void Context::drawArrays(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount)
{
	if(!applyRenderTarget())
	{
		return;
	}

	if(mState.currentProgram == 0)
	{
		return;   // Nothing to process.
	}

	sw::DrawType primitiveType;
	int primitiveCount;
	int verticesPerPrimitive;

	if(!es2sw::ConvertPrimitiveType(mode, count, GL_NONE, primitiveType, primitiveCount, verticesPerPrimitive))
	{
		return error(GL_INVALID_ENUM);
	}

	applyState(mode);

	for(int i = 0; i < instanceCount; ++i)
	{
		device->setInstanceID(i);

		GLenum err = applyVertexBuffer(0, first, count, i);
		if(err != GL_NO_ERROR)
		{
			return error(err);
		}

		applyShaders();
		applyTextures();

		if(!getCurrentProgram()->validateSamplers(false))
		{
			return error(GL_INVALID_OPERATION);
		}

		if(primitiveCount <= 0)
		{
			return;
		}

		TransformFeedback* transformFeedback = getTransformFeedback();
		if(!cullSkipsDraw(mode) || (transformFeedback->isActive() && !transformFeedback->isPaused()))
		{
			device->drawPrimitive(primitiveType, primitiveCount);
		}
		if(transformFeedback)
		{
			transformFeedback->addVertexOffset(primitiveCount * verticesPerPrimitive);
		}
	}
}

void Context::drawElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount)
{
	if(!applyRenderTarget())
	{
		return;
	}

	if(mState.currentProgram == 0)
	{
		return;   // Nothing to process.
	}

	if(!indices && !getCurrentVertexArray()->getElementArrayBuffer())
	{
		return error(GL_INVALID_OPERATION);
	}

	GLenum internalMode = mode;
	if(isPrimitiveRestartFixedIndexEnabled())
	{
		switch(mode)
		{
		case GL_TRIANGLE_FAN:
		case GL_TRIANGLE_STRIP:
			internalMode = GL_TRIANGLES;
			break;
		case GL_LINE_LOOP:
		case GL_LINE_STRIP:
			internalMode = GL_LINES;
			break;
		default:
			break;
		}
	}

	sw::DrawType primitiveType;
	int primitiveCount;
	int verticesPerPrimitive;

	if(!es2sw::ConvertPrimitiveType(internalMode, count, type, primitiveType, primitiveCount, verticesPerPrimitive))
	{
		return error(GL_INVALID_ENUM);
	}

	TranslatedIndexData indexInfo(primitiveCount);
	GLenum err = applyIndexBuffer(indices, start, end, count, mode, type, &indexInfo);
	if(err != GL_NO_ERROR)
	{
		return error(err);
	}

	applyState(internalMode);

	for(int i = 0; i < instanceCount; ++i)
	{
		device->setInstanceID(i);

		GLsizei vertexCount = indexInfo.maxIndex - indexInfo.minIndex + 1;
		err = applyVertexBuffer(-(int)indexInfo.minIndex, indexInfo.minIndex, vertexCount, i);
		if(err != GL_NO_ERROR)
		{
			return error(err);
		}

		applyShaders();
		applyTextures();

		if(!getCurrentProgram()->validateSamplers(false))
		{
			return error(GL_INVALID_OPERATION);
		}

		if(primitiveCount <= 0)
		{
			return;
		}

		TransformFeedback* transformFeedback = getTransformFeedback();
		if(!cullSkipsDraw(internalMode) || (transformFeedback->isActive() && !transformFeedback->isPaused()))
		{
			device->drawIndexedPrimitive(primitiveType, indexInfo.indexOffset, indexInfo.primitiveCount);
		}
		if(transformFeedback)
		{
			transformFeedback->addVertexOffset(indexInfo.primitiveCount * verticesPerPrimitive);
		}
	}
}

void Context::blit(sw::Surface *source, const sw::SliceRect &sRect, sw::Surface *dest, const sw::SliceRect &dRect)
{
	sw::SliceRectF sRectF((float)sRect.x0, (float)sRect.y0, (float)sRect.x1, (float)sRect.y1, sRect.slice);
	device->blit(source, sRectF, dest, dRect, false);
}

void Context::finish()
{
	device->finish();
}

void Context::flush()
{
	// We don't queue anything without processing it as fast as possible
}

void Context::recordInvalidEnum()
{
	mInvalidEnum = true;
}

void Context::recordInvalidValue()
{
	mInvalidValue = true;
}

void Context::recordInvalidOperation()
{
	mInvalidOperation = true;
}

void Context::recordOutOfMemory()
{
	mOutOfMemory = true;
}

void Context::recordInvalidFramebufferOperation()
{
	mInvalidFramebufferOperation = true;
}

// Get one of the recorded errors and clear its flag, if any.
// [OpenGL ES 2.0.24] section 2.5 page 13.
GLenum Context::getError()
{
	if(mInvalidEnum)
	{
		mInvalidEnum = false;

		return GL_INVALID_ENUM;
	}

	if(mInvalidValue)
	{
		mInvalidValue = false;

		return GL_INVALID_VALUE;
	}

	if(mInvalidOperation)
	{
		mInvalidOperation = false;

		return GL_INVALID_OPERATION;
	}

	if(mOutOfMemory)
	{
		mOutOfMemory = false;

		return GL_OUT_OF_MEMORY;
	}

	if(mInvalidFramebufferOperation)
	{
		mInvalidFramebufferOperation = false;

		return GL_INVALID_FRAMEBUFFER_OPERATION;
	}

	return GL_NO_ERROR;
}

int Context::getSupportedMultisampleCount(int requested)
{
	int supported = 0;

	for(int i = NUM_MULTISAMPLE_COUNTS - 1; i >= 0; i--)
	{
		if(supported >= requested)
		{
			return supported;
		}

		supported = multisampleCount[i];
	}

	return supported;
}

void Context::detachBuffer(GLuint buffer)
{
	// [OpenGL ES 2.0.24] section 2.9 page 22:
	// If a buffer object is deleted while it is bound, all bindings to that object in the current context
	// (i.e. in the thread that called Delete-Buffers) are reset to zero.

	if(mState.copyReadBuffer.name() == buffer)
	{
		mState.copyReadBuffer = nullptr;
	}

	if(mState.copyWriteBuffer.name() == buffer)
	{
		mState.copyWriteBuffer = nullptr;
	}

	if(mState.pixelPackBuffer.name() == buffer)
	{
		mState.pixelPackBuffer = nullptr;
	}

	if(mState.pixelUnpackBuffer.name() == buffer)
	{
		mState.pixelUnpackBuffer = nullptr;
	}

	if(mState.genericUniformBuffer.name() == buffer)
	{
		mState.genericUniformBuffer = nullptr;
	}

	if(getArrayBufferName() == buffer)
	{
		mState.arrayBuffer = nullptr;
	}

	// Only detach from the current transform feedback
	TransformFeedback* currentTransformFeedback = getTransformFeedback();
	if(currentTransformFeedback)
	{
		currentTransformFeedback->detachBuffer(buffer);
	}

	// Only detach from the current vertex array
	VertexArray* currentVertexArray = getCurrentVertexArray();
	if(currentVertexArray)
	{
		currentVertexArray->detachBuffer(buffer);
	}

	for(int attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
	{
		if(mState.vertexAttribute[attribute].mBoundBuffer.name() == buffer)
		{
			mState.vertexAttribute[attribute].mBoundBuffer = nullptr;
		}
	}
}

void Context::detachTexture(GLuint texture)
{
	// [OpenGL ES 2.0.24] section 3.8 page 84:
	// If a texture object is deleted, it is as if all texture units which are bound to that texture object are
	// rebound to texture object zero

	for(int type = 0; type < TEXTURE_TYPE_COUNT; type++)
	{
		for(int sampler = 0; sampler < MAX_COMBINED_TEXTURE_IMAGE_UNITS; sampler++)
		{
			if(mState.samplerTexture[type][sampler].name() == texture)
			{
				mState.samplerTexture[type][sampler] = nullptr;
			}
		}
	}

	// [OpenGL ES 2.0.24] section 4.4 page 112:
	// If a texture object is deleted while its image is attached to the currently bound framebuffer, then it is
	// as if FramebufferTexture2D had been called, with a texture of 0, for each attachment point to which this
	// image was attached in the currently bound framebuffer.

	Framebuffer *readFramebuffer = getReadFramebuffer();
	Framebuffer *drawFramebuffer = getDrawFramebuffer();

	if(readFramebuffer)
	{
		readFramebuffer->detachTexture(texture);
	}

	if(drawFramebuffer && drawFramebuffer != readFramebuffer)
	{
		drawFramebuffer->detachTexture(texture);
	}
}

void Context::detachFramebuffer(GLuint framebuffer)
{
	// [OpenGL ES 2.0.24] section 4.4 page 107:
	// If a framebuffer that is currently bound to the target FRAMEBUFFER is deleted, it is as though
	// BindFramebuffer had been executed with the target of FRAMEBUFFER and framebuffer of zero.

	if(mState.readFramebuffer == framebuffer)
	{
		bindReadFramebuffer(0);
	}

	if(mState.drawFramebuffer == framebuffer)
	{
		bindDrawFramebuffer(0);
	}
}

void Context::detachRenderbuffer(GLuint renderbuffer)
{
	// [OpenGL ES 2.0.24] section 4.4 page 109:
	// If a renderbuffer that is currently bound to RENDERBUFFER is deleted, it is as though BindRenderbuffer
	// had been executed with the target RENDERBUFFER and name of zero.

	if(mState.renderbuffer.name() == renderbuffer)
	{
		bindRenderbuffer(0);
	}

	// [OpenGL ES 2.0.24] section 4.4 page 111:
	// If a renderbuffer object is deleted while its image is attached to the currently bound framebuffer,
	// then it is as if FramebufferRenderbuffer had been called, with a renderbuffer of 0, for each attachment
	// point to which this image was attached in the currently bound framebuffer.

	Framebuffer *readFramebuffer = getReadFramebuffer();
	Framebuffer *drawFramebuffer = getDrawFramebuffer();

	if(readFramebuffer)
	{
		readFramebuffer->detachRenderbuffer(renderbuffer);
	}

	if(drawFramebuffer && drawFramebuffer != readFramebuffer)
	{
		drawFramebuffer->detachRenderbuffer(renderbuffer);
	}
}

void Context::detachSampler(GLuint sampler)
{
	// [OpenGL ES 3.0.2] section 3.8.2 pages 123-124:
	// If a sampler object that is currently bound to one or more texture units is
	// deleted, it is as though BindSampler is called once for each texture unit to
	// which the sampler is bound, with unit set to the texture unit and sampler set to zero.
	for(size_t textureUnit = 0; textureUnit < MAX_COMBINED_TEXTURE_IMAGE_UNITS; ++textureUnit)
	{
		gl::BindingPointer<Sampler> &samplerBinding = mState.sampler[textureUnit];
		if(samplerBinding.name() == sampler)
		{
			samplerBinding = nullptr;
		}
	}
}

bool Context::cullSkipsDraw(GLenum drawMode)
{
	return mState.cullFaceEnabled && mState.cullMode == GL_FRONT_AND_BACK && isTriangleMode(drawMode);
}

bool Context::isTriangleMode(GLenum drawMode)
{
	switch(drawMode)
	{
	case GL_TRIANGLES:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_STRIP:
		return true;
	case GL_POINTS:
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
		return false;
	default: UNREACHABLE(drawMode);
	}

	return false;
}

void Context::setVertexAttrib(GLuint index, const GLfloat *values)
{
	ASSERT(index < MAX_VERTEX_ATTRIBS);

	mState.vertexAttribute[index].setCurrentValue(values);

	mVertexDataManager->dirtyCurrentValue(index);
}

void Context::setVertexAttrib(GLuint index, const GLint *values)
{
	ASSERT(index < MAX_VERTEX_ATTRIBS);

	mState.vertexAttribute[index].setCurrentValue(values);

	mVertexDataManager->dirtyCurrentValue(index);
}

void Context::setVertexAttrib(GLuint index, const GLuint *values)
{
	ASSERT(index < MAX_VERTEX_ATTRIBS);

	mState.vertexAttribute[index].setCurrentValue(values);

	mVertexDataManager->dirtyCurrentValue(index);
}

void Context::blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                              GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                              GLbitfield mask, bool filter, bool allowPartialDepthStencilBlit)
{
	Framebuffer *readFramebuffer = getReadFramebuffer();
	Framebuffer *drawFramebuffer = getDrawFramebuffer();

	int readBufferWidth, readBufferHeight, readBufferSamples;
	int drawBufferWidth, drawBufferHeight, drawBufferSamples;

	if(!readFramebuffer || (readFramebuffer->completeness(readBufferWidth, readBufferHeight, readBufferSamples) != GL_FRAMEBUFFER_COMPLETE) ||
	   !drawFramebuffer || (drawFramebuffer->completeness(drawBufferWidth, drawBufferHeight, drawBufferSamples) != GL_FRAMEBUFFER_COMPLETE))
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION);
	}

	if(drawBufferSamples > 1)
	{
		return error(GL_INVALID_OPERATION);
	}

	sw::SliceRect sourceRect;
	sw::SliceRect destRect;
	bool flipX = (srcX0 < srcX1) ^ (dstX0 < dstX1);
	bool flipY = (srcY0 < srcY1) ^ (dstY0 < dstY1);

	if(srcX0 < srcX1)
	{
		sourceRect.x0 = srcX0;
		sourceRect.x1 = srcX1;
	}
	else
	{
		sourceRect.x0 = srcX1;
		sourceRect.x1 = srcX0;
	}

	if(dstX0 < dstX1)
	{
		destRect.x0 = dstX0;
		destRect.x1 = dstX1;
	}
	else
	{
		destRect.x0 = dstX1;
		destRect.x1 = dstX0;
	}

	if(srcY0 < srcY1)
	{
		sourceRect.y0 = srcY0;
		sourceRect.y1 = srcY1;
	}
	else
	{
		sourceRect.y0 = srcY1;
		sourceRect.y1 = srcY0;
	}

	if(dstY0 < dstY1)
	{
		destRect.y0 = dstY0;
		destRect.y1 = dstY1;
	}
	else
	{
		destRect.y0 = dstY1;
		destRect.y1 = dstY0;
	}

	sw::RectF sourceScissoredRect(static_cast<float>(sourceRect.x0), static_cast<float>(sourceRect.y0),
	                              static_cast<float>(sourceRect.x1), static_cast<float>(sourceRect.y1));
	sw::Rect destScissoredRect = destRect;

	if(mState.scissorTestEnabled)   // Only write to parts of the destination framebuffer which pass the scissor test
	{
		sw::Rect scissorRect(mState.scissorX, mState.scissorY, mState.scissorX + mState.scissorWidth, mState.scissorY + mState.scissorHeight);
		Device::ClipDstRect(sourceScissoredRect, destScissoredRect, scissorRect, flipX, flipY);
	}

	sw::SliceRectF sourceTrimmedRect = sourceScissoredRect;
	sw::SliceRect destTrimmedRect = destScissoredRect;

	// The source & destination rectangles also may need to be trimmed if
	// they fall out of the bounds of the actual draw and read surfaces.
	sw::Rect sourceTrimRect(0, 0, readBufferWidth, readBufferHeight);
	Device::ClipSrcRect(sourceTrimmedRect, destTrimmedRect, sourceTrimRect, flipX, flipY);

	sw::Rect destTrimRect(0, 0, drawBufferWidth, drawBufferHeight);
	Device::ClipDstRect(sourceTrimmedRect, destTrimmedRect, destTrimRect, flipX, flipY);

	bool partialBufferCopy = false;

	if(sourceTrimmedRect.y1 - sourceTrimmedRect.y0 < readBufferHeight ||
	   sourceTrimmedRect.x1 - sourceTrimmedRect.x0 < readBufferWidth ||
	   destTrimmedRect.y1 - destTrimmedRect.y0 < drawBufferHeight ||
	   destTrimmedRect.x1 - destTrimmedRect.x0 < drawBufferWidth ||
	   sourceTrimmedRect.y0 != 0 || destTrimmedRect.y0 != 0 || sourceTrimmedRect.x0 != 0 || destTrimmedRect.x0 != 0)
	{
		partialBufferCopy = true;
	}

	bool sameBounds = (srcX0 == dstX0 && srcY0 == dstY0 && srcX1 == dstX1 && srcY1 == dstY1);
	bool blitRenderTarget = false;
	bool blitDepth = false;
	bool blitStencil = false;

	if(mask & GL_COLOR_BUFFER_BIT)
	{
		GLenum readColorbufferType = readFramebuffer->getReadBufferType();
		GLenum drawColorbufferType = drawFramebuffer->getColorbufferType(0);
		const bool validReadType = readColorbufferType == GL_TEXTURE_2D || readColorbufferType == GL_TEXTURE_RECTANGLE_ARB || Framebuffer::IsRenderbuffer(readColorbufferType);
		const bool validDrawType = drawColorbufferType == GL_TEXTURE_2D || drawColorbufferType == GL_TEXTURE_RECTANGLE_ARB || Framebuffer::IsRenderbuffer(drawColorbufferType);
		if(!validReadType || !validDrawType)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(partialBufferCopy && readBufferSamples > 1 && !sameBounds)
		{
			return error(GL_INVALID_OPERATION);
		}

		// The GL ES 3.0.2 spec (pg 193) states that:
		// 1) If the read buffer is fixed point format, the draw buffer must be as well
		// 2) If the read buffer is an unsigned integer format, the draw buffer must be
		// as well
		// 3) If the read buffer is a signed integer format, the draw buffer must be as
		// well
		es2::Renderbuffer *readRenderbuffer = readFramebuffer->getReadColorbuffer();
		es2::Renderbuffer *drawRenderbuffer = drawFramebuffer->getColorbuffer(0);
		GLint readFormat = readRenderbuffer->getFormat();
		GLint drawFormat = drawRenderbuffer->getFormat();
		GLenum readComponentType = GetComponentType(readFormat, GL_COLOR_ATTACHMENT0);
		GLenum drawComponentType = GetComponentType(drawFormat, GL_COLOR_ATTACHMENT0);
		bool readFixedPoint = ((readComponentType == GL_UNSIGNED_NORMALIZED) ||
		                       (readComponentType == GL_SIGNED_NORMALIZED));
		bool drawFixedPoint = ((drawComponentType == GL_UNSIGNED_NORMALIZED) ||
		                       (drawComponentType == GL_SIGNED_NORMALIZED));
		bool readFixedOrFloat = (readFixedPoint || (readComponentType == GL_FLOAT));
		bool drawFixedOrFloat = (drawFixedPoint || (drawComponentType == GL_FLOAT));

		if(readFixedOrFloat != drawFixedOrFloat)
		{
			return error(GL_INVALID_OPERATION);
		}

		if((readComponentType == GL_UNSIGNED_INT) && (drawComponentType != GL_UNSIGNED_INT))
		{
			return error(GL_INVALID_OPERATION);
		}

		if((readComponentType == GL_INT) && (drawComponentType != GL_INT))
		{
			return error(GL_INVALID_OPERATION);
		}

		// Cannot filter integer data
		if(((readComponentType == GL_UNSIGNED_INT) || (readComponentType == GL_INT)) && filter)
		{
			return error(GL_INVALID_OPERATION);
		}

		if((readRenderbuffer->getSamples() > 0) && (readFormat != drawFormat))
		{
			// RGBA8 and BGRA8 should be interchangeable here
			if(!(((readFormat == GL_RGBA8) && (drawFormat == GL_BGRA8_EXT)) ||
				 ((readFormat == GL_BGRA8_EXT) && (drawFormat == GL_RGBA8))))
			{
				return error(GL_INVALID_OPERATION);
			}
		}

		blitRenderTarget = true;
	}

	if(mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
	{
		Renderbuffer *readDSBuffer = nullptr;
		Renderbuffer *drawDSBuffer = nullptr;

		if(mask & GL_DEPTH_BUFFER_BIT)
		{
			if(readFramebuffer->getDepthbuffer() && drawFramebuffer->getDepthbuffer())
			{
				GLenum readDepthBufferType = readFramebuffer->getDepthbufferType();
				GLenum drawDepthBufferType = drawFramebuffer->getDepthbufferType();
				if((readDepthBufferType != drawDepthBufferType) &&
				   !(Framebuffer::IsRenderbuffer(readDepthBufferType) && Framebuffer::IsRenderbuffer(drawDepthBufferType)))
				{
					return error(GL_INVALID_OPERATION);
				}

				blitDepth = true;
				readDSBuffer = readFramebuffer->getDepthbuffer();
				drawDSBuffer = drawFramebuffer->getDepthbuffer();

				if(readDSBuffer->getFormat() != drawDSBuffer->getFormat())
				{
					return error(GL_INVALID_OPERATION);
				}
			}
		}

		if(mask & GL_STENCIL_BUFFER_BIT)
		{
			if(readFramebuffer->getStencilbuffer() && drawFramebuffer->getStencilbuffer())
			{
				GLenum readStencilBufferType = readFramebuffer->getStencilbufferType();
				GLenum drawStencilBufferType = drawFramebuffer->getStencilbufferType();
				if((readStencilBufferType != drawStencilBufferType) &&
				   !(Framebuffer::IsRenderbuffer(readStencilBufferType) && Framebuffer::IsRenderbuffer(drawStencilBufferType)))
				{
					return error(GL_INVALID_OPERATION);
				}

				blitStencil = true;
				readDSBuffer = readFramebuffer->getStencilbuffer();
				drawDSBuffer = drawFramebuffer->getStencilbuffer();

				if(readDSBuffer->getFormat() != drawDSBuffer->getFormat())
				{
					return error(GL_INVALID_OPERATION);
				}
			}
		}

		if(partialBufferCopy && !allowPartialDepthStencilBlit)
		{
			ERR("Only whole-buffer depth and stencil blits are supported by ANGLE_framebuffer_blit.");
			return error(GL_INVALID_OPERATION);   // Only whole-buffer copies are permitted
		}

		// OpenGL ES 3.0.4 spec, p.199:
		// ...an INVALID_OPERATION error is generated if the formats of the read
		// and draw framebuffers are not identical or if the source and destination
		// rectangles are not defined with the same(X0, Y 0) and (X1, Y 1) bounds.
		// If SAMPLE_BUFFERS for the draw framebuffer is greater than zero, an
		// INVALID_OPERATION error is generated.
		if((drawDSBuffer && drawDSBuffer->getSamples() > 1) ||
		   ((readDSBuffer && readDSBuffer->getSamples() > 1) &&
		    (!sameBounds || (drawDSBuffer->getFormat() != readDSBuffer->getFormat()))))
		{
			return error(GL_INVALID_OPERATION);
		}
	}

	if(blitRenderTarget || blitDepth || blitStencil)
	{
		if(flipX)
		{
			swap(destTrimmedRect.x0, destTrimmedRect.x1);
		}
		if(flipY)
		{
			swap(destTrimmedRect.y0, destTrimmedRect.y1);
		}

		if(blitRenderTarget)
		{
			egl::Image *readRenderTarget = readFramebuffer->getReadRenderTarget();
			egl::Image *drawRenderTarget = drawFramebuffer->getRenderTarget(0);

			bool success = device->stretchRect(readRenderTarget, &sourceTrimmedRect, drawRenderTarget, &destTrimmedRect, (filter ? Device::USE_FILTER : 0) | Device::COLOR_BUFFER);

			readRenderTarget->release();
			drawRenderTarget->release();

			if(!success)
			{
				ERR("BlitFramebuffer failed.");
				return;
			}
		}

		if(blitDepth)
		{
			egl::Image *readRenderTarget = readFramebuffer->getDepthBuffer();
			egl::Image *drawRenderTarget = drawFramebuffer->getDepthBuffer();

			bool success = device->stretchRect(readRenderTarget, &sourceTrimmedRect, drawRenderTarget, &destTrimmedRect, (filter ? Device::USE_FILTER : 0) | Device::DEPTH_BUFFER);

			readRenderTarget->release();
			drawRenderTarget->release();

			if(!success)
			{
				ERR("BlitFramebuffer failed.");
				return;
			}
		}

		if(blitStencil)
		{
			egl::Image *readRenderTarget = readFramebuffer->getStencilBuffer();
			egl::Image *drawRenderTarget = drawFramebuffer->getStencilBuffer();

			bool success = device->stretchRect(readRenderTarget, &sourceTrimmedRect, drawRenderTarget, &destTrimmedRect, (filter ? Device::USE_FILTER : 0) | Device::STENCIL_BUFFER);

			readRenderTarget->release();
			drawRenderTarget->release();

			if(!success)
			{
				ERR("BlitFramebuffer failed.");
				return;
			}
		}
	}
}

void Context::bindTexImage(gl::Surface *surface)
{
	bool isRect = (surface->getTextureTarget() == EGL_TEXTURE_RECTANGLE_ANGLE);
	es2::Texture2D *textureObject = isRect ? getTexture2DRect() : getTexture2D();

	if(textureObject)
	{
		textureObject->bindTexImage(surface);
	}
}

EGLenum Context::validateSharedImage(EGLenum target, GLuint name, GLuint textureLevel)
{
	GLenum textureTarget = GL_NONE;

	switch(target)
	{
	case EGL_GL_TEXTURE_2D_KHR:
		textureTarget = GL_TEXTURE_2D;
		break;
	case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
	case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
	case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
	case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
	case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
	case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
		textureTarget = GL_TEXTURE_CUBE_MAP;
		break;
	case EGL_GL_RENDERBUFFER_KHR:
		break;
	default:
		return EGL_BAD_PARAMETER;
	}

	if(textureLevel >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
	{
		return EGL_BAD_MATCH;
	}

	if(textureTarget != GL_NONE)
	{
		es2::Texture *texture = getTexture(name);

		if(!texture || texture->getTarget() != textureTarget)
		{
			return EGL_BAD_PARAMETER;
		}

		if(texture->isShared(textureTarget, textureLevel))   // Bound to an EGLSurface or already an EGLImage sibling
		{
			return EGL_BAD_ACCESS;
		}

		if(textureLevel != 0 && !texture->isSamplerComplete())
		{
			return EGL_BAD_PARAMETER;
		}

		if(textureLevel == 0 && !(texture->isSamplerComplete() && texture->getTopLevel() == 0))
		{
			return EGL_BAD_PARAMETER;
		}
	}
	else if(target == EGL_GL_RENDERBUFFER_KHR)
	{
		es2::Renderbuffer *renderbuffer = getRenderbuffer(name);

		if(!renderbuffer)
		{
			return EGL_BAD_PARAMETER;
		}

		if(renderbuffer->isShared())   // Already an EGLImage sibling
		{
			return EGL_BAD_ACCESS;
		}
	}
	else UNREACHABLE(target);

	return EGL_SUCCESS;
}

egl::Image *Context::createSharedImage(EGLenum target, GLuint name, GLuint textureLevel)
{
	GLenum textureTarget = GL_NONE;

	switch(target)
	{
	case EGL_GL_TEXTURE_2D_KHR:                  textureTarget = GL_TEXTURE_2D;                  break;
	case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR: textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X; break;
	case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR: textureTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_X; break;
	case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR: textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Y; break;
	case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR: textureTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y; break;
	case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR: textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Z; break;
	case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR: textureTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; break;
	}

	if(textureTarget != GL_NONE)
	{
		es2::Texture *texture = getTexture(name);

		return texture->createSharedImage(textureTarget, textureLevel);
	}
	else if(target == EGL_GL_RENDERBUFFER_KHR)
	{
		es2::Renderbuffer *renderbuffer = getRenderbuffer(name);

		return renderbuffer->createSharedImage();
	}
	else UNREACHABLE(target);

	return nullptr;
}

egl::Image *Context::getSharedImage(GLeglImageOES image)
{
	return display->getSharedImage(image);
}

Device *Context::getDevice()
{
	return device;
}

const GLubyte *Context::getExtensions(GLuint index, GLuint *numExt) const
{
	// Keep list sorted in following order:
	// OES extensions
	// EXT extensions
	// Vendor extensions
	static const char *es2extensions[] =
	{
		"GL_OES_compressed_ETC1_RGB8_texture",
		"GL_OES_depth24",
		"GL_OES_depth32",
		"GL_OES_depth_texture",
		"GL_OES_depth_texture_cube_map",
		"GL_OES_EGL_image",
		"GL_OES_EGL_image_external",
		"GL_OES_EGL_sync",
		"GL_OES_element_index_uint",
		"GL_OES_framebuffer_object",
		"GL_OES_packed_depth_stencil",
		"GL_OES_rgb8_rgba8",
		"GL_OES_standard_derivatives",
		"GL_OES_surfaceless_context",
		"GL_OES_texture_float",
		"GL_OES_texture_float_linear",
		"GL_OES_texture_half_float",
		"GL_OES_texture_half_float_linear",
		"GL_OES_texture_npot",
		"GL_OES_texture_3D",
		"GL_OES_vertex_array_object",
		"GL_OES_vertex_half_float",
		"GL_EXT_blend_minmax",
		"GL_EXT_color_buffer_half_float",
		"GL_EXT_draw_buffers",
		"GL_EXT_instanced_arrays",
		"GL_EXT_occlusion_query_boolean",
		"GL_EXT_read_format_bgra",
		"GL_EXT_texture_compression_dxt1",
		"GL_EXT_texture_filter_anisotropic",
		"GL_EXT_texture_format_BGRA8888",
		"GL_EXT_texture_rg",
#if (ASTC_SUPPORT)
		"GL_KHR_texture_compression_astc_hdr",
		"GL_KHR_texture_compression_astc_ldr",
#endif
		"GL_ARB_texture_rectangle",
		"GL_ANGLE_framebuffer_blit",
		"GL_ANGLE_framebuffer_multisample",
		"GL_ANGLE_instanced_arrays",
		"GL_ANGLE_texture_compression_dxt3",
		"GL_ANGLE_texture_compression_dxt5",
		"GL_APPLE_texture_format_BGRA8888",
		"GL_CHROMIUM_color_buffer_float_rgba", // A subset of EXT_color_buffer_float on top of OpenGL ES 2.0
		"GL_CHROMIUM_texture_filtering_hint",
		"GL_NV_fence",
		"GL_NV_framebuffer_blit",
		"GL_NV_read_depth",
	};

	// Extensions exclusive to OpenGL ES 3.0 and above.
	static const char *es3extensions[] =
	{
		"GL_EXT_color_buffer_float",
	};

	GLuint numES2extensions = sizeof(es2extensions) / sizeof(es2extensions[0]);
	GLuint numExtensions = numES2extensions;

	if(clientVersion >= 3)
	{
		numExtensions += sizeof(es3extensions) / sizeof(es3extensions[0]);
	}

	if(numExt)
	{
		*numExt = numExtensions;

		return nullptr;
	}

	if(index == GL_INVALID_INDEX)
	{
		static std::string extensionsCat;

		if(extensionsCat.empty() && (numExtensions > 0))
		{
			for(const char *extension : es2extensions)
			{
				extensionsCat += std::string(extension) + " ";
			}

			if(clientVersion >= 3)
			{
				for(const char *extension : es3extensions)
				{
					extensionsCat += std::string(extension) + " ";
				}
			}
		}

		return (const GLubyte*)extensionsCat.c_str();
	}

	if(index >= numExtensions)
	{
		return nullptr;
	}

	if(index < numES2extensions)
	{
		return (const GLubyte*)es2extensions[index];
	}
	else
	{
		return (const GLubyte*)es3extensions[index - numES2extensions];
	}
}

}

NO_SANITIZE_FUNCTION egl::Context *es2CreateContext(egl::Display *display, const egl::Context *shareContext, int clientVersion, const egl::Config *config)
{
	ASSERT(!shareContext || shareContext->getClientVersion() == clientVersion);   // Should be checked by eglCreateContext
	return new es2::Context(display, static_cast<const es2::Context*>(shareContext), clientVersion, config);
}
