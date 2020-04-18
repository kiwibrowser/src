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

// Context.cpp: Implements the gl::Context class, managing all GL state and performing
// rendering operations.

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
#include "Shader.h"
#include "Texture.h"
#include "VertexDataManager.h"
#include "IndexDataManager.h"
#include "Display.h"
#include "Surface.h"
#include "Common/Half.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

namespace gl
{
Context::Context(const Context *shareContext)
	: modelView(32),
	  projection(2)
{
	sw::Context *context = new sw::Context();
	device = new gl::Device(context);

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
	mState.stencilMask = -1;
	mState.stencilWritemask = -1;
	mState.stencilBackFunc = GL_ALWAYS;
	mState.stencilBackRef = 0;
	mState.stencilBackMask = - 1;
	mState.stencilBackWritemask = -1;
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
	mState.generateMipmapHint = GL_DONT_CARE;
	mState.fragmentShaderDerivativeHint = GL_DONT_CARE;
	mState.colorLogicOpEnabled = false;
	mState.logicalOperation = GL_COPY;

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

	// In the initial state, TEXTURE_2D and TEXTURE_CUBE_MAP have twodimensional
	// and cube map texture state vectors respectively associated with them.
	// In order that access to these initial textures not be lost, they are treated as texture
	// objects all of whose names are 0.

	mTexture2DZero = new Texture2D(0);
	mProxyTexture2DZero = new Texture2D(0);
	mTextureCubeMapZero = new TextureCubeMap(0);

	mState.activeSampler = 0;
	bindArrayBuffer(0);
	bindElementArrayBuffer(0);
	bindTextureCubeMap(0);
	bindTexture2D(0);
	bindReadFramebuffer(0);
	bindDrawFramebuffer(0);
	bindRenderbuffer(0);

	mState.currentProgram = 0;

	mState.packAlignment = 4;
	mState.unpackAlignment = 4;

	mVertexDataManager = nullptr;
	mIndexDataManager = nullptr;

	mInvalidEnum = false;
	mInvalidValue = false;
	mInvalidOperation = false;
	mOutOfMemory = false;
	mInvalidFramebufferOperation = false;

	mHasBeenCurrent = false;

	markAllStateDirty();

	matrixMode = GL_MODELVIEW;

	listMode = 0;
	//memset(displayList, 0, sizeof(displayList));
	listIndex = 0;
	list = 0;
	firstFreeIndex = 1;

	clientTexture = GL_TEXTURE0;

	drawing = false;
	drawMode = 0;   // FIXME

	mState.vertexAttribute[sw::Color0].mCurrentValue[0] = 1.0f;
	mState.vertexAttribute[sw::Color0].mCurrentValue[1] = 1.0f;
	mState.vertexAttribute[sw::Color0].mCurrentValue[2] = 1.0f;
	mState.vertexAttribute[sw::Color0].mCurrentValue[3] = 1.0f;
	mState.vertexAttribute[sw::Normal].mCurrentValue[0] = 0.0f;
	mState.vertexAttribute[sw::Normal].mCurrentValue[1] = 0.0f;
	mState.vertexAttribute[sw::Normal].mCurrentValue[2] = 1.0f;
	mState.vertexAttribute[sw::Normal].mCurrentValue[3] = 0.0f;
	mState.vertexAttribute[sw::TexCoord0].mCurrentValue[0] = 0.0f;
	mState.vertexAttribute[sw::TexCoord0].mCurrentValue[1] = 0.0f;
	mState.vertexAttribute[sw::TexCoord0].mCurrentValue[2] = 0.0f;
	mState.vertexAttribute[sw::TexCoord0].mCurrentValue[3] = 1.0f;
	mState.vertexAttribute[sw::TexCoord1].mCurrentValue[0] = 0.0f;
	mState.vertexAttribute[sw::TexCoord1].mCurrentValue[1] = 0.0f;
	mState.vertexAttribute[sw::TexCoord1].mCurrentValue[2] = 0.0f;
	mState.vertexAttribute[sw::TexCoord1].mCurrentValue[3] = 1.0f;

	for(int i = 0; i < 8; i++)
	{
		envEnable[i] = true;
	}
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
	mState.elementArrayBuffer = nullptr;
	mState.renderbuffer = nullptr;

	mTexture2DZero = nullptr;
	mProxyTexture2DZero = nullptr;
	mTextureCubeMapZero = nullptr;

	delete mVertexDataManager;
	delete mIndexDataManager;

	mResourceManager->release();
	delete device;
}

void Context::makeCurrent(Surface *surface)
{
	if(!mHasBeenCurrent)
	{
		mVertexDataManager = new VertexDataManager(this);
		mIndexDataManager = new IndexDataManager();

		mState.viewportX = 0;
		mState.viewportY = 0;
		mState.viewportWidth = surface->getWidth();
		mState.viewportHeight = surface->getHeight();

		mState.scissorX = 0;
		mState.scissorY = 0;
		mState.scissorWidth = surface->getWidth();
		mState.scissorHeight = surface->getHeight();

		mHasBeenCurrent = true;
	}

	// Wrap the existing resources into GL objects and assign them to the '0' names
	Image *defaultRenderTarget = surface->getRenderTarget();
	Image *depthStencil = surface->getDepthStencil();

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

	markAllStateDirty();
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
	mColorLogicOperatorDirty = true;
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

void Context::setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
	mState.viewportX = x;
	mState.viewportY = y;
	mState.viewportWidth = width;
	mState.viewportHeight = height;
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

GLuint Context::getArrayBufferName() const
{
	return mState.arrayBuffer.name();
}

GLuint Context::getActiveQuery(GLenum target) const
{
	Query *queryObject = nullptr;

	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED:
		queryObject = mState.activeQuery[QUERY_ANY_SAMPLES_PASSED];
		break;
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
		queryObject = mState.activeQuery[QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE];
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
	mState.vertexAttribute[attribNum].mArrayEnabled = enabled;
}

const VertexAttribute &Context::getVertexAttribState(unsigned int attribNum)
{
	return mState.vertexAttribute[attribNum];
}

void Context::setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type, bool normalized,
                                   GLsizei stride, const void *pointer)
{
	mState.vertexAttribute[attribNum].mBoundBuffer = boundBuffer;
	mState.vertexAttribute[attribNum].mSize = size;
	mState.vertexAttribute[attribNum].mType = type;
	mState.vertexAttribute[attribNum].mNormalized = normalized;
	mState.vertexAttribute[attribNum].mStride = stride;
	mState.vertexAttribute[attribNum].mPointer = pointer;
}

const void *Context::getVertexAttribPointer(unsigned int attribNum) const
{
	return mState.vertexAttribute[attribNum].mPointer;
}

const VertexAttributeArray &Context::getVertexAttributes()
{
	return mState.vertexAttribute;
}

void Context::setPackAlignment(GLint alignment)
{
	mState.packAlignment = alignment;
}

GLint Context::getPackAlignment() const
{
	return mState.packAlignment;
}

void Context::setUnpackAlignment(GLint alignment)
{
	mState.unpackAlignment = alignment;
}

GLint Context::getUnpackAlignment() const
{
	return mState.unpackAlignment;
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

void Context::deleteBuffer(GLuint buffer)
{
	if(mResourceManager->getBuffer(buffer))
	{
		detachBuffer(buffer);
	}

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
	if(mResourceManager->getTexture(texture))
	{
		detachTexture(texture);
	}

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
	Framebuffer *framebufferObject = mFramebufferNameSpace.remove(framebuffer);

	if(framebufferObject)
	{
		detachFramebuffer(framebuffer);

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

Buffer *Context::getBuffer(GLuint handle)
{
	return mResourceManager->getBuffer(handle);
}

Shader *Context::getShader(GLuint handle)
{
	return mResourceManager->getShader(handle);
}

Program *Context::getProgram(GLuint handle)
{
	return mResourceManager->getProgram(handle);
}

Texture *Context::getTexture(GLuint handle)
{
	return mResourceManager->getTexture(handle);
}

Renderbuffer *Context::getRenderbuffer(GLuint handle)
{
	return mResourceManager->getRenderbuffer(handle);
}

Framebuffer *Context::getReadFramebuffer()
{
	return getFramebuffer(mState.readFramebuffer);
}

Framebuffer *Context::getDrawFramebuffer()
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

	mState.elementArrayBuffer = getBuffer(buffer);
}

void Context::bindTexture2D(GLuint texture)
{
	mResourceManager->checkTextureAllocation(texture, TEXTURE_2D);

	mState.samplerTexture[TEXTURE_2D][mState.activeSampler] = getTexture(texture);
}

void Context::bindTextureCubeMap(GLuint texture)
{
	mResourceManager->checkTextureAllocation(texture, TEXTURE_CUBE);

	mState.samplerTexture[TEXTURE_CUBE][mState.activeSampler] = getTexture(texture);
}

void Context::bindReadFramebuffer(GLuint framebuffer)
{
	if(!getFramebuffer(framebuffer))
	{
		mFramebufferNameSpace.insert(framebuffer, new Framebuffer());
	}

	mState.readFramebuffer = framebuffer;
}

void Context::bindDrawFramebuffer(GLuint framebuffer)
{
	if(!getFramebuffer(framebuffer))
	{
		mFramebufferNameSpace.insert(framebuffer, new Framebuffer());
	}

	mState.drawFramebuffer = framebuffer;
}

void Context::bindRenderbuffer(GLuint renderbuffer)
{
	mResourceManager->checkRenderbufferAllocation(renderbuffer);

	mState.renderbuffer = getRenderbuffer(renderbuffer);
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
			return error(GL_INVALID_OPERATION);
		}
	}

	QueryType qType;
	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED:
		qType = QUERY_ANY_SAMPLES_PASSED;
		break;
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
		qType = QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE;
		break;
	default:
		ASSERT(false);
	}

	Query *queryObject = getQuery(query, true, target);

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
	case GL_ANY_SAMPLES_PASSED:
		qType = QUERY_ANY_SAMPLES_PASSED;
		break;
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
		qType = QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE;
		break;
	default:
		ASSERT(false);
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

Framebuffer *Context::getFramebuffer(unsigned int handle)
{
	return mFramebufferNameSpace.find(handle);
}

Fence *Context::getFence(unsigned int handle)
{
	return mFenceNameSpace.find(handle);
}

Query *Context::getQuery(unsigned int handle, bool create, GLenum type)
{
	if(!mQueryNameSpace.isReserved(handle))
	{
		return nullptr;
	}
	else
	{
		Query *query = mQueryNameSpace.find(handle);
		if(!query && create)
		{
			query = new Query(handle, type);
			query->addRef();
			mQueryNameSpace.insert(handle, query);
		}

		return query;
	}
}

Buffer *Context::getArrayBuffer()
{
	return mState.arrayBuffer;
}

Buffer *Context::getElementArrayBuffer()
{
	return mState.elementArrayBuffer;
}

Program *Context::getCurrentProgram()
{
	return mResourceManager->getProgram(mState.currentProgram);
}

Texture2D *Context::getTexture2D(GLenum target)
{
	if(target == GL_TEXTURE_2D)
	{
		return static_cast<Texture2D*>(getSamplerTexture(mState.activeSampler, TEXTURE_2D));
	}
	else if(target == GL_PROXY_TEXTURE_2D)
	{
		return static_cast<Texture2D*>(getSamplerTexture(mState.activeSampler, PROXY_TEXTURE_2D));
	}
	else UNREACHABLE(target);

	return nullptr;
}

TextureCubeMap *Context::getTextureCubeMap()
{
	return static_cast<TextureCubeMap*>(getSamplerTexture(mState.activeSampler, TEXTURE_CUBE));
}

Texture *Context::getSamplerTexture(unsigned int sampler, TextureType type)
{
	GLuint texid = mState.samplerTexture[type][sampler].name();

	if(texid == 0)   // Special case: 0 refers to different initial textures based on the target
	{
		switch(type)
		{
		case TEXTURE_2D:       return mTexture2DZero;
		case PROXY_TEXTURE_2D: return mProxyTexture2DZero;
		case TEXTURE_CUBE:     return mTextureCubeMapZero;
		default: UNREACHABLE(type);
		}
	}

	return mState.samplerTexture[type][sampler];
}

bool Context::getBooleanv(GLenum pname, GLboolean *params)
{
	switch(pname)
	{
	case GL_SHADER_COMPILER:          *params = GL_TRUE;                             break;
	case GL_SAMPLE_COVERAGE_INVERT:   *params = mState.sampleCoverageInvert;         break;
	case GL_DEPTH_WRITEMASK:          *params = mState.depthMask;                    break;
	case GL_COLOR_WRITEMASK:
		params[0] = mState.colorMaskRed;
		params[1] = mState.colorMaskGreen;
		params[2] = mState.colorMaskBlue;
		params[3] = mState.colorMaskAlpha;
		break;
	case GL_CULL_FACE:                *params = mState.cullFaceEnabled;              break;
	case GL_POLYGON_OFFSET_FILL:      *params = mState.polygonOffsetFillEnabled;     break;
	case GL_SAMPLE_ALPHA_TO_COVERAGE: *params = mState.sampleAlphaToCoverageEnabled; break;
	case GL_SAMPLE_COVERAGE:          *params = mState.sampleCoverageEnabled;        break;
	case GL_SCISSOR_TEST:             *params = mState.scissorTestEnabled;           break;
	case GL_STENCIL_TEST:             *params = mState.stencilTestEnabled;           break;
	case GL_DEPTH_TEST:               *params = mState.depthTestEnabled;             break;
	case GL_BLEND:                    *params = mState.blendEnabled;                 break;
	case GL_DITHER:                   *params = mState.ditherEnabled;                break;
	default:
		return false;
	}

	return true;
}

bool Context::getFloatv(GLenum pname, GLfloat *params)
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
	case GL_MODELVIEW_MATRIX:
		for(int i = 0; i < 16; i++)
		{
			params[i] = modelView.current()[i % 4][i / 4];
		}
		break;
	case GL_PROJECTION_MATRIX:
		for(int i = 0; i < 16; i++)
		{
			params[i] = projection.current()[i % 4][i / 4];
		}
		break;
	default:
		return false;
	}

	return true;
}

bool Context::getIntegerv(GLenum pname, GLint *params)
{
	// Please note: DEPTH_CLEAR_VALUE is not included in our internal getIntegerv implementation
	// because it is stored as a float, despite the fact that the GL ES 2.0 spec names
	// GetIntegerv as its native query function. As it would require conversion in any
	// case, this should make no difference to the calling application. You may find it in
	// Context::getFloatv.
	switch(pname)
	{
	case GL_MAX_VERTEX_ATTRIBS:               *params = MAX_VERTEX_ATTRIBS;               break;
	case GL_MAX_VERTEX_UNIFORM_VECTORS:       *params = MAX_VERTEX_UNIFORM_VECTORS;       break;
	case GL_MAX_VERTEX_UNIFORM_COMPONENTS:    *params = MAX_VERTEX_UNIFORM_VECTORS * 4;   break;   // FIXME: Verify
	case GL_MAX_VARYING_VECTORS:              *params = MAX_VARYING_VECTORS;              break;
	case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: *params = MAX_COMBINED_TEXTURE_IMAGE_UNITS; break;
	case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:   *params = MAX_VERTEX_TEXTURE_IMAGE_UNITS;   break;
	case GL_MAX_TEXTURE_IMAGE_UNITS:          *params = MAX_TEXTURE_IMAGE_UNITS;          break;
	case GL_MAX_FRAGMENT_UNIFORM_VECTORS:     *params = MAX_FRAGMENT_UNIFORM_VECTORS;     break;
	case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:  *params = MAX_VERTEX_UNIFORM_VECTORS * 4;   break;   // FIXME: Verify
	case GL_MAX_RENDERBUFFER_SIZE:            *params = IMPLEMENTATION_MAX_RENDERBUFFER_SIZE; break;
	case GL_NUM_SHADER_BINARY_FORMATS:        *params = 0;                                    break;
	case GL_SHADER_BINARY_FORMATS:      /* no shader binary formats are supported */          break;
	case GL_ARRAY_BUFFER_BINDING:             *params = mState.arrayBuffer.name();            break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:     *params = mState.elementArrayBuffer.name();     break;
//	case GL_FRAMEBUFFER_BINDING:            // now equivalent to GL_DRAW_FRAMEBUFFER_BINDING_ANGLE
	case GL_DRAW_FRAMEBUFFER_BINDING:         *params = mState.drawFramebuffer;               break;
	case GL_READ_FRAMEBUFFER_BINDING:         *params = mState.readFramebuffer;               break;
	case GL_RENDERBUFFER_BINDING:             *params = mState.renderbuffer.name();           break;
	case GL_CURRENT_PROGRAM:                  *params = mState.currentProgram;                break;
	case GL_PACK_ALIGNMENT:                   *params = mState.packAlignment;                 break;
	case GL_UNPACK_ALIGNMENT:                 *params = mState.unpackAlignment;               break;
	case GL_GENERATE_MIPMAP_HINT:             *params = mState.generateMipmapHint;            break;
	case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:  *params = mState.fragmentShaderDerivativeHint; break;
	case GL_ACTIVE_TEXTURE:                   *params = (mState.activeSampler + GL_TEXTURE0); break;
	case GL_STENCIL_FUNC:                     *params = mState.stencilFunc;                   break;
	case GL_STENCIL_REF:                      *params = mState.stencilRef;                    break;
	case GL_STENCIL_VALUE_MASK:               *params = mState.stencilMask;                   break;
	case GL_STENCIL_BACK_FUNC:                *params = mState.stencilBackFunc;               break;
	case GL_STENCIL_BACK_REF:                 *params = mState.stencilBackRef;                break;
	case GL_STENCIL_BACK_VALUE_MASK:          *params = mState.stencilBackMask;               break;
	case GL_STENCIL_FAIL:                     *params = mState.stencilFail;                   break;
	case GL_STENCIL_PASS_DEPTH_FAIL:          *params = mState.stencilPassDepthFail;          break;
	case GL_STENCIL_PASS_DEPTH_PASS:          *params = mState.stencilPassDepthPass;          break;
	case GL_STENCIL_BACK_FAIL:                *params = mState.stencilBackFail;               break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL:     *params = mState.stencilBackPassDepthFail;      break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS:     *params = mState.stencilBackPassDepthPass;      break;
	case GL_DEPTH_FUNC:                       *params = mState.depthFunc;                     break;
	case GL_BLEND_SRC_RGB:                    *params = mState.sourceBlendRGB;                break;
	case GL_BLEND_SRC_ALPHA:                  *params = mState.sourceBlendAlpha;              break;
	case GL_BLEND_DST_RGB:                    *params = mState.destBlendRGB;                  break;
	case GL_BLEND_DST_ALPHA:                  *params = mState.destBlendAlpha;                break;
	case GL_BLEND_EQUATION_RGB:               *params = mState.blendEquationRGB;              break;
	case GL_BLEND_EQUATION_ALPHA:             *params = mState.blendEquationAlpha;            break;
	case GL_STENCIL_WRITEMASK:                *params = mState.stencilWritemask;              break;
	case GL_STENCIL_BACK_WRITEMASK:           *params = mState.stencilBackWritemask;          break;
	case GL_STENCIL_CLEAR_VALUE:              *params = mState.stencilClearValue;             break;
	case GL_SUBPIXEL_BITS:                    *params = 4;                                    break;
	case GL_MAX_TEXTURE_SIZE:                 *params = IMPLEMENTATION_MAX_TEXTURE_SIZE;      break;
	case GL_MAX_CUBE_MAP_TEXTURE_SIZE:        *params = IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE; break;
	case GL_MAX_ARRAY_TEXTURE_LAYERS:         *params = 0;                                    break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS:   *params = NUM_COMPRESSED_TEXTURE_FORMATS;       break;
	case GL_MAX_SAMPLES:                      *params = IMPLEMENTATION_MAX_SAMPLES;           break;
	case GL_SAMPLE_BUFFERS:
	case GL_SAMPLES:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			int width, height, samples;

			if(framebuffer->completeness(width, height, samples) == GL_FRAMEBUFFER_COMPLETE)
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
		break;
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:   *params = IMPLEMENTATION_COLOR_READ_TYPE;   break;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT: *params = IMPLEMENTATION_COLOR_READ_FORMAT; break;
	case GL_MAX_VIEWPORT_DIMS:
		{
			int maxDimension = IMPLEMENTATION_MAX_RENDERBUFFER_SIZE;
			params[0] = maxDimension;
			params[1] = maxDimension;
		}
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		{
			for(int i = 0; i < NUM_COMPRESSED_TEXTURE_FORMATS; i++)
			{
				params[i] = compressedTextureFormats[i];
			}
		}
		break;
	case GL_VIEWPORT:
		params[0] = mState.viewportX;
		params[1] = mState.viewportY;
		params[2] = mState.viewportWidth;
		params[3] = mState.viewportHeight;
		break;
	case GL_SCISSOR_BOX:
		params[0] = mState.scissorX;
		params[1] = mState.scissorY;
		params[2] = mState.scissorWidth;
		params[3] = mState.scissorHeight;
		break;
	case GL_CULL_FACE_MODE:                   *params = mState.cullMode;                 break;
	case GL_FRONT_FACE:                       *params = mState.frontFace;                break;
	case GL_RED_BITS:
	case GL_GREEN_BITS:
	case GL_BLUE_BITS:
	case GL_ALPHA_BITS:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			Renderbuffer *colorbuffer = framebuffer->getColorbuffer();

			if(colorbuffer)
			{
				switch(pname)
				{
				case GL_RED_BITS:   *params = colorbuffer->getRedSize();   break;
				case GL_GREEN_BITS: *params = colorbuffer->getGreenSize(); break;
				case GL_BLUE_BITS:  *params = colorbuffer->getBlueSize();  break;
				case GL_ALPHA_BITS: *params = colorbuffer->getAlphaSize(); break;
				}
			}
			else
			{
				*params = 0;
			}
		}
		break;
	case GL_DEPTH_BITS:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			Renderbuffer *depthbuffer = framebuffer->getDepthbuffer();

			if(depthbuffer)
			{
				*params = depthbuffer->getDepthSize();
			}
			else
			{
				*params = 0;
			}
		}
		break;
	case GL_STENCIL_BITS:
		{
			Framebuffer *framebuffer = getDrawFramebuffer();
			Renderbuffer *stencilbuffer = framebuffer->getStencilbuffer();

			if(stencilbuffer)
			{
				*params = stencilbuffer->getStencilSize();
			}
			else
			{
				*params = 0;
			}
		}
		break;
	case GL_TEXTURE_BINDING_2D:
		{
			if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
			{
				error(GL_INVALID_OPERATION);
				return false;
			}

			*params = mState.samplerTexture[TEXTURE_2D][mState.activeSampler].name();
		}
		break;
	case GL_TEXTURE_BINDING_CUBE_MAP:
		{
			if(mState.activeSampler > MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
			{
				error(GL_INVALID_OPERATION);
				return false;
			}

			*params = mState.samplerTexture[TEXTURE_CUBE][mState.activeSampler].name();
		}
		break;
	default:
		return false;
	}

	return true;
}

bool Context::getQueryParameterInfo(GLenum pname, GLenum *type, unsigned int *numParams)
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
	case GL_FRAMEBUFFER_BINDING:
	case GL_RENDERBUFFER_BINDING:
	case GL_CURRENT_PROGRAM:
	case GL_PACK_ALIGNMENT:
	case GL_UNPACK_ALIGNMENT:
	case GL_GENERATE_MIPMAP_HINT:
	case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
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
	case GL_SAMPLE_BUFFERS:
	case GL_SAMPLES:
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
	case GL_TEXTURE_BINDING_2D:
	case GL_TEXTURE_BINDING_CUBE_MAP:
	case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
	case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
	case GL_MAX_ARRAY_TEXTURE_LAYERS:
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

// Applies the render target surface, depth stencil surface, viewport rectangle and scissor rectangle
bool Context::applyRenderTarget()
{
	Framebuffer *framebuffer = getDrawFramebuffer();
	int width, height, samples;

	if(!framebuffer || framebuffer->completeness(width, height, samples) != GL_FRAMEBUFFER_COMPLETE)
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION, false);
	}

	Image *renderTarget = framebuffer->getRenderTarget();
	device->setRenderTarget(0, renderTarget);
	if(renderTarget) renderTarget->release();

	Image *depthStencil = framebuffer->getDepthStencil();
	device->setDepthStencilSurface(depthStencil);
	if(depthStencil) depthStencil->release();

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

	if(mState.scissorTestEnabled)
	{
		sw::Rect scissor = {mState.scissorX, mState.scissorY, mState.scissorX + mState.scissorWidth, mState.scissorY + mState.scissorHeight};
		scissor.clip(0, 0, width, height);

		device->setScissorRect(scissor);
		device->setScissorEnable(true);
	}
	else
	{
		device->setScissorEnable(false);
	}

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

	if(mColorLogicOperatorDirty)
	{
		if(mState.colorLogicOpEnabled)
		{
			device->setColorLogicOpEnabled(true);
			device->setLogicalOperation(es2sw::ConvertLogicalOperation(mState.logicalOperation));
		}
		else
		{
			device->setColorLogicOpEnabled(false);
		}

		mColorLogicOperatorDirty = false;
	}

	if(mStencilStateDirty || mFrontFaceDirty)
	{
		if(mState.stencilTestEnabled && framebuffer->hasStencil())
		{
			device->setStencilEnable(true);
			device->setTwoSidedStencil(true);

			if(mState.stencilWritemask != mState.stencilBackWritemask ||
			   mState.stencilRef != mState.stencilBackRef ||
			   mState.stencilMask != mState.stencilBackMask)
			{
				ERR("Separate front/back stencil writemasks, reference values, or stencil mask values are invalid under WebGL.");
				return error(GL_INVALID_OPERATION);
			}

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
		device->setColorWriteMask(0, es2sw::ConvertColorMask(mState.colorMaskRed, mState.colorMaskGreen, mState.colorMaskBlue, mState.colorMaskAlpha));
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
				float depthBias = ldexp(mState.polygonOffsetUnits, -(int)(depthbuffer->getDepthSize()));
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
}

GLenum Context::applyVertexBuffer(GLint base, GLint first, GLsizei count)
{
	TranslatedAttribute attributes[MAX_VERTEX_ATTRIBS];

	GLenum err = mVertexDataManager->prepareVertexData(first, count, attributes);
	if(err != GL_NO_ERROR)
	{
		return err;
	}

	Program *program = getCurrentProgram();

	device->resetInputStreams(false);

	for(int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
	{
		if(program && program->getAttributeStream(i) == -1)
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

		int stream = program ? program->getAttributeStream(i) : i;
		device->setInputStream(stream, attribute);
	}

	return GL_NO_ERROR;
}

// Applies the indices and element array bindings
GLenum Context::applyIndexBuffer(const void *indices, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo)
{
	GLenum err = mIndexDataManager->prepareIndexData(type, count, mState.elementArrayBuffer, indices, indexInfo);

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
	if(!programObject)
	{
		device->setVertexShader(0);
		device->setPixelShader(0);
		return;
	}
	sw::VertexShader *vertexShader = programObject->getVertexShader();
	sw::PixelShader *pixelShader = programObject->getPixelShader();

	device->setVertexShader(vertexShader);
	device->setPixelShader(pixelShader);

	if(programObject->getSerial() != mAppliedProgramSerial)
	{
		programObject->dirtyAllUniforms();
		mAppliedProgramSerial = programObject->getSerial();
	}

	programObject->applyUniforms();
}

void Context::applyTextures()
{
	applyTextures(sw::SAMPLER_PIXEL);
	//applyTextures(sw::SAMPLER_VERTEX);
}

void Context::applyTextures(sw::SamplerType samplerType)
{
	Program *programObject = getCurrentProgram();

	int samplerCount = (samplerType == sw::SAMPLER_PIXEL) ? MAX_TEXTURE_IMAGE_UNITS : MAX_VERTEX_TEXTURE_IMAGE_UNITS;   // Range of samplers of given sampler type

	for(int samplerIndex = 0; samplerIndex < samplerCount; samplerIndex++)
	{
		int textureUnit = programObject ? programObject->getSamplerMapping(samplerType, samplerIndex) : samplerIndex;   // OpenGL texture image unit index

		if(textureUnit != -1)
		{
			TextureType textureType = programObject ? programObject->getSamplerTextureType(samplerType, samplerIndex) : TEXTURE_2D;

			Texture *texture = getSamplerTexture(textureUnit, textureType);

			if(envEnable[samplerIndex] && texture->isSamplerComplete())
			{
				GLenum wrapS = texture->getWrapS();
				GLenum wrapT = texture->getWrapT();
				GLenum minFilter = texture->getMinFilter();
				GLenum magFilter = texture->getMagFilter();
				GLfloat maxAnisotropy = texture->getMaxAnisotropy();

				device->setAddressingModeU(samplerType, samplerIndex, es2sw::ConvertTextureWrap(wrapS));
				device->setAddressingModeV(samplerType, samplerIndex, es2sw::ConvertTextureWrap(wrapT));

				device->setTextureFilter(samplerType, samplerIndex, es2sw::ConvertTextureFilter(minFilter, magFilter, maxAnisotropy));
				device->setMipmapFilter(samplerType, samplerIndex, es2sw::ConvertMipMapFilter(minFilter));
				device->setMaxAnisotropy(samplerType, samplerIndex, maxAnisotropy);

				applyTexture(samplerType, samplerIndex, texture);

				device->setStageOperation(samplerIndex, sw::TextureStage::STAGE_MODULATE);
				device->setFirstArgument(samplerIndex, sw::TextureStage::SOURCE_TEXTURE);
				device->setSecondArgument(samplerIndex, sw::TextureStage::SOURCE_CURRENT);
				//device->setThirdArgument(samplerIndex, sw::TextureStage::SOURCE_CONSTANT);

				device->setStageOperationAlpha(samplerIndex, sw::TextureStage::STAGE_MODULATE);
				device->setFirstArgumentAlpha(samplerIndex, sw::TextureStage::SOURCE_TEXTURE);
				device->setSecondArgumentAlpha(samplerIndex, sw::TextureStage::SOURCE_CURRENT);
				//device->setThirdArgumentAlpha(samplerIndex, sw::TextureStage::SOURCE_CONSTANT);

				//device->setConstantColor(0, sw::Color<float>(0.0f, 0.0f, 0.0f, 0.0f));
			}
			else
			{
				applyTexture(samplerType, samplerIndex, nullptr);

				device->setStageOperation(samplerIndex, sw::TextureStage::STAGE_SELECTARG1);
				device->setFirstArgument(samplerIndex, sw::TextureStage::SOURCE_CURRENT);
				device->setSecondArgument(samplerIndex, sw::TextureStage::SOURCE_CURRENT);
				//device->setThirdArgument(samplerIndex, sw::TextureStage::SOURCE_CONSTANT);

				device->setStageOperationAlpha(samplerIndex, sw::TextureStage::STAGE_SELECTARG1);
				device->setFirstArgumentAlpha(samplerIndex, sw::TextureStage::SOURCE_CURRENT);
				device->setSecondArgumentAlpha(samplerIndex, sw::TextureStage::SOURCE_CURRENT);
				//device->setThirdArgumentAlpha(samplerIndex, sw::TextureStage::SOURCE_CONSTANT);
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
		textureUsed = program ? program->getPixelShader()->usesSampler(index) : true;
	}
	else if(type == sw::SAMPLER_VERTEX)
	{
		textureUsed = program ? program->getVertexShader()->usesSampler(index) : false;
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
		int topLevel = baseTexture->getTopLevel();

		if(baseTexture->getTarget() == GL_TEXTURE_2D)
		{
			Texture2D *texture = static_cast<Texture2D*>(baseTexture);

			for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
			{
				int surfaceLevel = mipmapLevel;

				if(surfaceLevel < 0)
				{
					surfaceLevel = 0;
				}
				else if(surfaceLevel > topLevel)
				{
					surfaceLevel = topLevel;
				}

				Image *surface = texture->getImage(surfaceLevel);
				device->setTextureLevel(sampler, 0, mipmapLevel, surface, sw::TEXTURE_2D);
			}
		}
		else if(baseTexture->getTarget() == GL_TEXTURE_CUBE_MAP)
		{
			for(int face = 0; face < 6; face++)
			{
				TextureCubeMap *cubeTexture = static_cast<TextureCubeMap*>(baseTexture);

				for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
				{
					int surfaceLevel = mipmapLevel;

					if(surfaceLevel < 0)
					{
						surfaceLevel = 0;
					}
					else if(surfaceLevel > topLevel)
					{
						surfaceLevel = topLevel;
					}

					Image *surface = cubeTexture->getImage(face, surfaceLevel);
					device->setTextureLevel(sampler, face, mipmapLevel, surface, sw::TEXTURE_CUBE);
				}
			}
		}
		else UNIMPLEMENTED();
	}
	else
	{
		device->setTextureLevel(sampler, 0, 0, 0, sw::TEXTURE_NULL);
	}
}

void Context::readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, GLsizei *bufSize, void* pixels)
{
	Framebuffer *framebuffer = getReadFramebuffer();
	int framebufferWidth, framebufferHeight, framebufferSamples;

	if(framebuffer->completeness(framebufferWidth, framebufferHeight, framebufferSamples) != GL_FRAMEBUFFER_COMPLETE)
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION);
	}

	if(getReadFramebufferName() != 0 && framebufferSamples != 0)
	{
		return error(GL_INVALID_OPERATION);
	}

	GLsizei outputPitch = ComputePitch(width, format, type, mState.packAlignment);

	// Sized query sanity check
	if(bufSize)
	{
		int requiredSize = outputPitch * height;
		if(requiredSize > *bufSize)
		{
			return error(GL_INVALID_OPERATION);
		}
	}

	Image *renderTarget = framebuffer->getRenderTarget();

	if(!renderTarget)
	{
		return error(GL_OUT_OF_MEMORY);
	}

	sw::Rect rect = {x, y, x + width, y + height};
	rect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

	unsigned char *source = (unsigned char*)renderTarget->lock(rect.x0, rect.y0, sw::LOCK_READONLY);
	unsigned char *dest = (unsigned char*)pixels;
	unsigned short *dest16 = (unsigned short*)pixels;
	int inputPitch = (int)renderTarget->getPitch();

	for(int j = 0; j < rect.y1 - rect.y0; j++)
	{
		if(renderTarget->getInternalFormat() == sw::FORMAT_A8R8G8B8 &&
		   format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE)
		{
			// Fast path for EXT_read_format_bgra, given an RGBA source buffer
			// Note that buffers with no alpha go through the slow path below
			memcpy(dest + j * outputPitch, source + j * inputPitch, (rect.x1 - rect.x0) * 4);
		}
		else
		{
			for(int i = 0; i < rect.x1 - rect.x0; i++)
			{
				float r;
				float g;
				float b;
				float a;

				switch(renderTarget->getInternalFormat())
				{
				case sw::FORMAT_R5G6B5:
					{
						unsigned short rgb = *(unsigned short*)(source + 2 * i + j * inputPitch);

						a = 1.0f;
						b = (rgb & 0x001F) * (1.0f / 0x001F);
						g = (rgb & 0x07E0) * (1.0f / 0x07E0);
						r = (rgb & 0xF800) * (1.0f / 0xF800);
					}
					break;
				case sw::FORMAT_A1R5G5B5:
					{
						unsigned short argb = *(unsigned short*)(source + 2 * i + j * inputPitch);

						a = (argb & 0x8000) ? 1.0f : 0.0f;
						b = (argb & 0x001F) * (1.0f / 0x001F);
						g = (argb & 0x03E0) * (1.0f / 0x03E0);
						r = (argb & 0x7C00) * (1.0f / 0x7C00);
					}
					break;
				case sw::FORMAT_A8R8G8B8:
					{
						unsigned int argb = *(unsigned int*)(source + 4 * i + j * inputPitch);

						a = (argb & 0xFF000000) * (1.0f / 0xFF000000);
						b = (argb & 0x000000FF) * (1.0f / 0x000000FF);
						g = (argb & 0x0000FF00) * (1.0f / 0x0000FF00);
						r = (argb & 0x00FF0000) * (1.0f / 0x00FF0000);
					}
					break;
				case sw::FORMAT_X8R8G8B8:
					{
						unsigned int xrgb = *(unsigned int*)(source + 4 * i + j * inputPitch);

						a = 1.0f;
						b = (xrgb & 0x000000FF) * (1.0f / 0x000000FF);
						g = (xrgb & 0x0000FF00) * (1.0f / 0x0000FF00);
						r = (xrgb & 0x00FF0000) * (1.0f / 0x00FF0000);
					}
					break;
				case sw::FORMAT_A2R10G10B10:
					{
						unsigned int argb = *(unsigned int*)(source + 4 * i + j * inputPitch);

						a = (argb & 0xC0000000) * (1.0f / 0xC0000000);
						b = (argb & 0x000003FF) * (1.0f / 0x000003FF);
						g = (argb & 0x000FFC00) * (1.0f / 0x000FFC00);
						r = (argb & 0x3FF00000) * (1.0f / 0x3FF00000);
					}
					break;
				case sw::FORMAT_A32B32G32R32F:
					{
						r = *((float*)(source + 16 * i + j * inputPitch) + 0);
						g = *((float*)(source + 16 * i + j * inputPitch) + 1);
						b = *((float*)(source + 16 * i + j * inputPitch) + 2);
						a = *((float*)(source + 16 * i + j * inputPitch) + 3);
					}
					break;
				case sw::FORMAT_A16B16G16R16F:
					{
						r = (float)*((sw::half*)(source + 8 * i + j * inputPitch) + 0);
						g = (float)*((sw::half*)(source + 8 * i + j * inputPitch) + 1);
						b = (float)*((sw::half*)(source + 8 * i + j * inputPitch) + 2);
						a = (float)*((sw::half*)(source + 8 * i + j * inputPitch) + 3);
					}
					break;
				default:
					UNIMPLEMENTED();   // FIXME
					UNREACHABLE(renderTarget->getInternalFormat());
				}

				switch(format)
				{
				case GL_RGBA:
					switch(type)
					{
					case GL_UNSIGNED_BYTE:
						dest[4 * i + j * outputPitch + 0] = (unsigned char)(255 * r + 0.5f);
						dest[4 * i + j * outputPitch + 1] = (unsigned char)(255 * g + 0.5f);
						dest[4 * i + j * outputPitch + 2] = (unsigned char)(255 * b + 0.5f);
						dest[4 * i + j * outputPitch + 3] = (unsigned char)(255 * a + 0.5f);
						break;
					default: UNREACHABLE(type);
					}
					break;
				case GL_BGRA_EXT:
					switch(type)
					{
					case GL_UNSIGNED_BYTE:
						dest[4 * i + j * outputPitch + 0] = (unsigned char)(255 * b + 0.5f);
						dest[4 * i + j * outputPitch + 1] = (unsigned char)(255 * g + 0.5f);
						dest[4 * i + j * outputPitch + 2] = (unsigned char)(255 * r + 0.5f);
						dest[4 * i + j * outputPitch + 3] = (unsigned char)(255 * a + 0.5f);
						break;
					case GL_UNSIGNED_SHORT_4_4_4_4_REV:
						// According to the desktop GL spec in the "Transfer of Pixel Rectangles" section
						// this type is packed as follows:
						//   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
						//  --------------------------------------------------------------------------------
						// |       4th         |        3rd         |        2nd        |   1st component   |
						//  --------------------------------------------------------------------------------
						// in the case of BGRA_EXT, B is the first component, G the second, and so forth.
						dest16[i + j * outputPitch / sizeof(unsigned short)] =
							((unsigned short)(15 * a + 0.5f) << 12)|
							((unsigned short)(15 * r + 0.5f) << 8) |
							((unsigned short)(15 * g + 0.5f) << 4) |
							((unsigned short)(15 * b + 0.5f) << 0);
						break;
					case GL_UNSIGNED_SHORT_1_5_5_5_REV:
						// According to the desktop GL spec in the "Transfer of Pixel Rectangles" section
						// this type is packed as follows:
						//   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
						//  --------------------------------------------------------------------------------
						// | 4th |          3rd           |           2nd          |      1st component     |
						//  --------------------------------------------------------------------------------
						// in the case of BGRA_EXT, B is the first component, G the second, and so forth.
						dest16[i + j * outputPitch / sizeof(unsigned short)] =
							((unsigned short)(     a + 0.5f) << 15) |
							((unsigned short)(31 * r + 0.5f) << 10) |
							((unsigned short)(31 * g + 0.5f) << 5) |
							((unsigned short)(31 * b + 0.5f) << 0);
						break;
					default: UNREACHABLE(type);
					}
					break;
				case GL_RGB:   // IMPLEMENTATION_COLOR_READ_FORMAT
					switch(type)
					{
					case GL_UNSIGNED_SHORT_5_6_5:   // IMPLEMENTATION_COLOR_READ_TYPE
						dest16[i + j * outputPitch / sizeof(unsigned short)] =
							((unsigned short)(31 * b + 0.5f) << 0) |
							((unsigned short)(63 * g + 0.5f) << 5) |
							((unsigned short)(31 * r + 0.5f) << 11);
						break;
					default: UNREACHABLE(type);
					}
					break;
				default: UNREACHABLE(format);
				}
			}
		}
	}

	renderTarget->unlock();
	renderTarget->release();
}

void Context::clear(GLbitfield mask)
{
	Framebuffer *framebuffer = getDrawFramebuffer();

	if(!framebuffer || framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION);
	}

	if(!applyRenderTarget())
	{
		return;
	}

	float depth = clamp01(mState.depthClearValue);
	int stencil = mState.stencilClearValue & 0x000000FF;

	if(mask & GL_COLOR_BUFFER_BIT)
	{
		unsigned int rgbaMask = (mState.colorMaskRed ? 0x1 : 0) |
		                        (mState.colorMaskGreen ? 0x2 : 0) |
		                        (mState.colorMaskBlue ? 0x4 : 0) |
		                        (mState.colorMaskAlpha ? 0x8 : 0);

		if(rgbaMask != 0)
		{
			device->clearColor(mState.colorClearValue.red, mState.colorClearValue.green, mState.colorClearValue.blue, mState.colorClearValue.alpha, rgbaMask);
		}
	}

	if(mask & GL_DEPTH_BUFFER_BIT)
	{
		if(mState.depthMask != 0)
		{
			device->clearDepth(depth);
		}
	}

	if(mask & GL_STENCIL_BUFFER_BIT)
	{
		if(mState.stencilWritemask != 0)
		{
			device->clearStencil(stencil, mState.stencilWritemask);
		}
	}
}

void Context::drawArrays(GLenum mode, GLint first, GLsizei count)
{
	if(!mState.currentProgram)
	{
		device->setProjectionMatrix(projection.current());
		device->setViewMatrix(modelView.current());
		device->setTextureMatrix(0, texture[0].current());
		device->setTextureMatrix(1, texture[1].current());
		device->setTextureTransform(0, texture[0].isIdentity() ? 0 : 4, false);
		device->setTextureTransform(1, texture[1].isIdentity() ? 0 : 4, false);
		device->setTexGen(0, sw::TEXGEN_NONE);
		device->setTexGen(1, sw::TEXGEN_NONE);
	}

	PrimitiveType primitiveType;
	int primitiveCount;

	if(!es2sw::ConvertPrimitiveType(mode, count, primitiveType, primitiveCount))
		return error(GL_INVALID_ENUM);

	if(primitiveCount <= 0)
	{
		return;
	}

	if(!applyRenderTarget())
	{
		return;
	}

	applyState(mode);

	GLenum err = applyVertexBuffer(0, first, count);
	if(err != GL_NO_ERROR)
	{
		return error(err);
	}

	applyShaders();
	applyTextures();

	if(getCurrentProgram() && !getCurrentProgram()->validateSamplers(false))
	{
		return error(GL_INVALID_OPERATION);
	}

	if(!cullSkipsDraw(mode))
	{
		device->drawPrimitive(primitiveType, primitiveCount);
	}
}

void Context::drawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	if(!mState.currentProgram)
	{
		return;
	}

	if(!indices && !mState.elementArrayBuffer)
	{
		return error(GL_INVALID_OPERATION);
	}

	PrimitiveType primitiveType;
	int primitiveCount;

	if(!es2sw::ConvertPrimitiveType(mode, count, primitiveType, primitiveCount))
		return error(GL_INVALID_ENUM);

	if(primitiveCount <= 0)
	{
		return;
	}

	if(!applyRenderTarget())
	{
		return;
	}

	applyState(mode);

	TranslatedIndexData indexInfo;
	GLenum err = applyIndexBuffer(indices, count, mode, type, &indexInfo);
	if(err != GL_NO_ERROR)
	{
		return error(err);
	}

	GLsizei vertexCount = indexInfo.maxIndex - indexInfo.minIndex + 1;
	err = applyVertexBuffer(-(int)indexInfo.minIndex, indexInfo.minIndex, vertexCount);
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

	if(!cullSkipsDraw(mode))
	{
		device->drawIndexedPrimitive(primitiveType, indexInfo.indexOffset, primitiveCount, IndexDataManager::typeSize(type));
	}
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
	// If a buffer object is deleted while it is bound, all bindings to that object in the current context
	// (i.e. in the thread that called Delete-Buffers) are reset to zero.

	if(mState.arrayBuffer.name() == buffer)
	{
		mState.arrayBuffer = nullptr;
	}

	if(mState.elementArrayBuffer.name() == buffer)
	{
		mState.elementArrayBuffer = nullptr;
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
	// If a renderbuffer that is currently bound to RENDERBUFFER is deleted, it is as though BindRenderbuffer
	// had been executed with the target RENDERBUFFER and name of zero.

	if(mState.renderbuffer.name() == renderbuffer)
	{
		bindRenderbuffer(0);
	}

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

void Context::setVertexAttrib(GLuint index, float x, float y, float z, float w)
{
	ASSERT(index < MAX_VERTEX_ATTRIBS);

	mState.vertexAttribute[index].mCurrentValue[0] = x;
	mState.vertexAttribute[index].mCurrentValue[1] = y;
	mState.vertexAttribute[index].mCurrentValue[2] = z;
	mState.vertexAttribute[index].mCurrentValue[3] = w;

	mVertexDataManager->dirtyCurrentValue(index);
}

void Context::blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                              GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                              GLbitfield mask)
{
	Framebuffer *readFramebuffer = getReadFramebuffer();
	Framebuffer *drawFramebuffer = getDrawFramebuffer();

	int readBufferWidth, readBufferHeight, readBufferSamples;
	int drawBufferWidth, drawBufferHeight, drawBufferSamples;

	if(!readFramebuffer || readFramebuffer->completeness(readBufferWidth, readBufferHeight, readBufferSamples) != GL_FRAMEBUFFER_COMPLETE ||
	   !drawFramebuffer || drawFramebuffer->completeness(drawBufferWidth, drawBufferHeight, drawBufferSamples) != GL_FRAMEBUFFER_COMPLETE)
	{
		return error(GL_INVALID_FRAMEBUFFER_OPERATION);
	}

	if(drawBufferSamples > 1)
	{
		return error(GL_INVALID_OPERATION);
	}

	sw::SliceRect sourceRect;
	sw::SliceRect destRect;

	if(srcX0 < srcX1)
	{
		sourceRect.x0 = srcX0;
		sourceRect.x1 = srcX1;
		destRect.x0 = dstX0;
		destRect.x1 = dstX1;
	}
	else
	{
		sourceRect.x0 = srcX1;
		destRect.x0 = dstX1;
		sourceRect.x1 = srcX0;
		destRect.x1 = dstX0;
	}

	if(srcY0 < srcY1)
	{
		sourceRect.y0 = srcY0;
		destRect.y0 = dstY0;
		sourceRect.y1 = srcY1;
		destRect.y1 = dstY1;
	}
	else
	{
		sourceRect.y0 = srcY1;
		destRect.y0 = dstY1;
		sourceRect.y1 = srcY0;
		destRect.y1 = dstY0;
	}

	sw::Rect sourceScissoredRect = sourceRect;
	sw::Rect destScissoredRect = destRect;

	if(mState.scissorTestEnabled)   // Only write to parts of the destination framebuffer which pass the scissor test
	{
		if(destRect.x0 < mState.scissorX)
		{
			int xDiff = mState.scissorX - destRect.x0;
			destScissoredRect.x0 = mState.scissorX;
			sourceScissoredRect.x0 += xDiff;
		}

		if(destRect.x1 > mState.scissorX + mState.scissorWidth)
		{
			int xDiff = destRect.x1 - (mState.scissorX + mState.scissorWidth);
			destScissoredRect.x1 = mState.scissorX + mState.scissorWidth;
			sourceScissoredRect.x1 -= xDiff;
		}

		if(destRect.y0 < mState.scissorY)
		{
			int yDiff = mState.scissorY - destRect.y0;
			destScissoredRect.y0 = mState.scissorY;
			sourceScissoredRect.y0 += yDiff;
		}

		if(destRect.y1 > mState.scissorY + mState.scissorHeight)
		{
			int yDiff = destRect.y1 - (mState.scissorY + mState.scissorHeight);
			destScissoredRect.y1 = mState.scissorY + mState.scissorHeight;
			sourceScissoredRect.y1 -= yDiff;
		}
	}

	sw::Rect sourceTrimmedRect = sourceScissoredRect;
	sw::Rect destTrimmedRect = destScissoredRect;

	// The source & destination rectangles also may need to be trimmed if they fall out of the bounds of
	// the actual draw and read surfaces.
	if(sourceTrimmedRect.x0 < 0)
	{
		int xDiff = 0 - sourceTrimmedRect.x0;
		sourceTrimmedRect.x0 = 0;
		destTrimmedRect.x0 += xDiff;
	}

	if(sourceTrimmedRect.x1 > readBufferWidth)
	{
		int xDiff = sourceTrimmedRect.x1 - readBufferWidth;
		sourceTrimmedRect.x1 = readBufferWidth;
		destTrimmedRect.x1 -= xDiff;
	}

	if(sourceTrimmedRect.y0 < 0)
	{
		int yDiff = 0 - sourceTrimmedRect.y0;
		sourceTrimmedRect.y0 = 0;
		destTrimmedRect.y0 += yDiff;
	}

	if(sourceTrimmedRect.y1 > readBufferHeight)
	{
		int yDiff = sourceTrimmedRect.y1 - readBufferHeight;
		sourceTrimmedRect.y1 = readBufferHeight;
		destTrimmedRect.y1 -= yDiff;
	}

	if(destTrimmedRect.x0 < 0)
	{
		int xDiff = 0 - destTrimmedRect.x0;
		destTrimmedRect.x0 = 0;
		sourceTrimmedRect.x0 += xDiff;
	}

	if(destTrimmedRect.x1 > drawBufferWidth)
	{
		int xDiff = destTrimmedRect.x1 - drawBufferWidth;
		destTrimmedRect.x1 = drawBufferWidth;
		sourceTrimmedRect.x1 -= xDiff;
	}

	if(destTrimmedRect.y0 < 0)
	{
		int yDiff = 0 - destTrimmedRect.y0;
		destTrimmedRect.y0 = 0;
		sourceTrimmedRect.y0 += yDiff;
	}

	if(destTrimmedRect.y1 > drawBufferHeight)
	{
		int yDiff = destTrimmedRect.y1 - drawBufferHeight;
		destTrimmedRect.y1 = drawBufferHeight;
		sourceTrimmedRect.y1 -= yDiff;
	}

	bool partialBufferCopy = false;

	if(sourceTrimmedRect.y1 - sourceTrimmedRect.y0 < readBufferHeight ||
	   sourceTrimmedRect.x1 - sourceTrimmedRect.x0 < readBufferWidth ||
	   destTrimmedRect.y1 - destTrimmedRect.y0 < drawBufferHeight ||
	   destTrimmedRect.x1 - destTrimmedRect.x0 < drawBufferWidth ||
	   sourceTrimmedRect.y0 != 0 || destTrimmedRect.y0 != 0 || sourceTrimmedRect.x0 != 0 || destTrimmedRect.x0 != 0)
	{
		partialBufferCopy = true;
	}

	bool blitRenderTarget = false;
	bool blitDepthStencil = false;

	if(mask & GL_COLOR_BUFFER_BIT)
	{
		const bool validReadType = readFramebuffer->getColorbufferType() == GL_TEXTURE_2D ||
		                           readFramebuffer->getColorbufferType() == GL_RENDERBUFFER;
		const bool validDrawType = drawFramebuffer->getColorbufferType() == GL_TEXTURE_2D ||
		                           drawFramebuffer->getColorbufferType() == GL_RENDERBUFFER;
		if(!validReadType || !validDrawType ||
		   readFramebuffer->getColorbuffer()->getInternalFormat() != drawFramebuffer->getColorbuffer()->getInternalFormat())
		{
			ERR("Color buffer format conversion in BlitFramebufferANGLE not supported by this implementation");
			return error(GL_INVALID_OPERATION);
		}

		if(partialBufferCopy && readBufferSamples > 1)
		{
			return error(GL_INVALID_OPERATION);
		}

		blitRenderTarget = true;
	}

	if(mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
	{
		Renderbuffer *readDSBuffer = nullptr;
		Renderbuffer *drawDSBuffer = nullptr;

		// We support OES_packed_depth_stencil, and do not support a separately attached depth and stencil buffer, so if we have
		// both a depth and stencil buffer, it will be the same buffer.

		if(mask & GL_DEPTH_BUFFER_BIT)
		{
			if(readFramebuffer->getDepthbuffer() && drawFramebuffer->getDepthbuffer())
			{
				if(readFramebuffer->getDepthbufferType() != drawFramebuffer->getDepthbufferType() ||
				   readFramebuffer->getDepthbuffer()->getInternalFormat() != drawFramebuffer->getDepthbuffer()->getInternalFormat())
				{
					return error(GL_INVALID_OPERATION);
				}

				blitDepthStencil = true;
				readDSBuffer = readFramebuffer->getDepthbuffer();
				drawDSBuffer = drawFramebuffer->getDepthbuffer();
			}
		}

		if(mask & GL_STENCIL_BUFFER_BIT)
		{
			if(readFramebuffer->getStencilbuffer() && drawFramebuffer->getStencilbuffer())
			{
				if(readFramebuffer->getStencilbufferType() != drawFramebuffer->getStencilbufferType() ||
				   readFramebuffer->getStencilbuffer()->getInternalFormat() != drawFramebuffer->getStencilbuffer()->getInternalFormat())
				{
					return error(GL_INVALID_OPERATION);
				}

				blitDepthStencil = true;
				readDSBuffer = readFramebuffer->getStencilbuffer();
				drawDSBuffer = drawFramebuffer->getStencilbuffer();
			}
		}

		if(partialBufferCopy)
		{
			ERR("Only whole-buffer depth and stencil blits are supported by this implementation.");
			return error(GL_INVALID_OPERATION);   // Only whole-buffer copies are permitted
		}

		if((drawDSBuffer && drawDSBuffer->getSamples() > 1) ||
		   (readDSBuffer && readDSBuffer->getSamples() > 1))
		{
			return error(GL_INVALID_OPERATION);
		}
	}

	if(blitRenderTarget || blitDepthStencil)
	{
		if(blitRenderTarget)
		{
			Image *readRenderTarget = readFramebuffer->getRenderTarget();
			Image *drawRenderTarget = drawFramebuffer->getRenderTarget();

			bool success = device->stretchRect(readRenderTarget, &sourceRect, drawRenderTarget, &destRect, false);

			readRenderTarget->release();
			drawRenderTarget->release();

			if(!success)
			{
				ERR("BlitFramebufferANGLE failed.");
				return;
			}
		}

		if(blitDepthStencil)
		{
			bool success = device->stretchRect(readFramebuffer->getDepthStencil(), nullptr, drawFramebuffer->getDepthStencil(), nullptr, false);

			if(!success)
			{
				ERR("BlitFramebufferANGLE failed.");
				return;
			}
		}
	}
}

void Context::setMatrixMode(GLenum mode)
{
	matrixMode = mode;
}

sw::MatrixStack &Context::currentMatrixStack()
{
	switch(matrixMode)
	{
	case GL_MODELVIEW:  return modelView;                     break;
	case GL_PROJECTION: return projection;                    break;
	case GL_TEXTURE:    return texture[mState.activeSampler]; break;
	default:            UNREACHABLE(matrixMode); return modelView;      break;
	}
}

void Context::loadIdentity()
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().identity();
}

void Context::pushMatrix()
{
	//if(drawing)
	//{
	//    return error(GL_INVALID_OPERATION);
	//}

	if(!currentMatrixStack().push())
	{
		return error(GL_STACK_OVERFLOW);
	}
}

void Context::popMatrix()
{
	//if(drawing)
	//{
	//    return error(GL_INVALID_OPERATION);
	//}

	if(!currentMatrixStack().pop())
	{
		return error(GL_STACK_OVERFLOW);
	}
}

void Context::rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().rotate(angle, x, y, z);
}

void Context::translate(GLfloat x, GLfloat y, GLfloat z)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().translate(x, y, z);
}

void Context::scale(GLfloat x, GLfloat y, GLfloat z)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().scale(x, y, z);
}

void Context::multiply(const GLdouble *m)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().multiply(m);
}

void Context::multiply(const GLfloat *m)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().multiply(m);
}

void Context::frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().frustum(left, right, bottom, top, zNear, zFar);
}

void Context::ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	currentMatrixStack().ortho(left, right, bottom, top, zNear, zFar);
}

void Context::setLightingEnabled(bool enable)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	device->setLightingEnable(enable);
}

void Context::setFogEnabled(bool enable)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	device->setFogEnable(enable);
}

void Context::setAlphaTestEnabled(bool enable)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	device->setAlphaTestEnable(enable);
}

void Context::alphaFunc(GLenum func, GLclampf ref)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	switch(func)
	{
	case GL_NEVER:    device->setAlphaCompare(sw::ALPHA_NEVER);        break;
	case GL_LESS:     device->setAlphaCompare(sw::ALPHA_LESS);         break;
	case GL_EQUAL:    device->setAlphaCompare(sw::ALPHA_EQUAL);        break;
	case GL_LEQUAL:   device->setAlphaCompare(sw::ALPHA_LESSEQUAL);    break;
	case GL_GREATER:  device->setAlphaCompare(sw::ALPHA_GREATER);      break;
	case GL_NOTEQUAL: device->setAlphaCompare(sw::ALPHA_NOTEQUAL);     break;
	case GL_GEQUAL:   device->setAlphaCompare(sw::ALPHA_GREATEREQUAL); break;
	case GL_ALWAYS:   device->setAlphaCompare(sw::ALPHA_ALWAYS);       break;
	default: UNREACHABLE(func);
	}

	device->setAlphaReference(gl::clamp01(ref));
}

void Context::setTexture2DEnabled(bool enable)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	envEnable[mState.activeSampler] = enable;
}

void Context::setShadeModel(GLenum mode)
{
	//if(drawing)
	//{
	//    return error(GL_INVALID_OPERATION);
	//}

	switch(mode)
	{
	case GL_FLAT:   device->setShadingMode(sw::SHADING_FLAT);    break;
	case GL_SMOOTH: device->setShadingMode(sw::SHADING_GOURAUD); break;
	default: return error(GL_INVALID_ENUM);
	}
}

void Context::setLightEnabled(int index, bool enable)
{
	device->setLightEnable(index, enable);
}

void Context::setNormalizeNormalsEnabled(bool enable)
{
	device->setNormalizeNormals(enable);
}

GLuint Context::genLists(GLsizei range)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION, 0);
	}

	int firstIndex = std::max(1u, firstFreeIndex);
	for(; true; firstIndex++)
	{
		int empty = 0;
		for(; empty < range; empty++)
		{
			if(displayList[firstIndex + empty] != 0)
			{
				break;
			}
		}

		if(empty == range)
		{
			for(int i = firstIndex; i < firstIndex + range; i++)
			{
				displayList[i] = new DisplayList();
			}

			if(firstIndex == firstFreeIndex)
			{
				firstFreeIndex = firstIndex + range;
			}

			return firstIndex;
		}
	}

	return 0;
}

void Context::newList(GLuint list, GLenum mode)
{
	if(drawing || listIndex != 0)
	{
		return error(GL_INVALID_OPERATION);
	}

	ASSERT(!this->list);
	this->list = new DisplayList();

	listIndex = list;
	listMode = mode;
}

void Context::endList()
{
	if(drawing || listIndex == 0)
	{
		return error(GL_INVALID_OPERATION);
	}

	ASSERT(list);
	delete displayList[listIndex];
	displayList[listIndex] = list;
	list = 0;

	listIndex = 0;
	listMode = 0;
}

void Context::callList(GLuint list)
{
	// As per GL specifications, if the list does not exist, it is ignored
	if(displayList[list])
	{
		displayList[list]->call();
	}
}

void Context::deleteList(GLuint list)
{
	delete displayList[list];
	displayList[list] = 0;
	displayList.erase(list);
	firstFreeIndex = std::min(firstFreeIndex , list);
}

void Context::listCommand(Command *command)
{
	ASSERT(list);
	list->list.push_back(command);

	if(listMode == GL_COMPILE_AND_EXECUTE)
	{
		listMode = 0;
		command->call();
		listMode = GL_COMPILE_AND_EXECUTE;
	}
}

void APIENTRY glVertexAttribArray(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	TRACE("(GLuint index = %d, GLint size = %d, GLenum type = 0x%X, "
	      "GLboolean normalized = %d, GLsizei stride = %d, const GLvoid* ptr = %p)",
	      index, size, type, normalized, stride, ptr);

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->setVertexAttribState(index, context->getArrayBuffer(), size, type, (normalized == GL_TRUE), stride, ptr);
		context->setVertexAttribArrayEnabled(index, ptr != 0);
	}
}

void Context::captureAttribs()
{
	memcpy(clientAttribute, mState.vertexAttribute, sizeof(mState.vertexAttribute));
}

void Context::captureDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	ASSERT(first == 0);   // FIXME: UNIMPLEMENTED!

	for(GLuint i = 0; i < MAX_VERTEX_ATTRIBS; i++)
	{
		GLint size = mState.vertexAttribute[i].mSize;
		GLenum type = mState.vertexAttribute[i].mType;
		GLboolean normalized = mState.vertexAttribute[i].mNormalized;
		GLsizei stride = mState.vertexAttribute[i].mStride;
		const GLvoid *pointer = mState.vertexAttribute[i].mPointer;

		size_t length = count * mState.vertexAttribute[i].stride();

		if(mState.vertexAttribute[i].mArrayEnabled)
		{
			ASSERT(pointer);   // FIXME: Add to condition?
			const int padding = 1024;   // For SIMD processing of vertices   // FIXME: Still necessary?
			void *buffer = new unsigned char[length + padding];
			memcpy(buffer, pointer, length);

			listCommand(gl::newCommand(glVertexAttribArray, i, size, type, normalized, stride, (const void*)buffer));
		}
		else
		{
			listCommand(gl::newCommand(glVertexAttribArray, i, size, type, normalized, stride, (const void*)0));
		}
	}
}

void Context::restoreAttribs()
{
	memcpy(mState.vertexAttribute, clientAttribute, sizeof(mState.vertexAttribute));
}

void Context::clientActiveTexture(GLenum texture)
{
	clientTexture = texture;
}

GLenum Context::getClientActiveTexture() const
{
	return clientTexture;
}

unsigned int Context::getActiveTexture() const
{
	return mState.activeSampler;
}

void Context::begin(GLenum mode)
{
	if(drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	drawing = true;
	drawMode = mode;

	vertex.clear();
}

void Context::position(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	InVertex v;

	v.P.x = x;
	v.P.y = y;
	v.P.z = z;
	v.P.w = w;
	v.C.x = mState.vertexAttribute[sw::Color0].mCurrentValue[0];
	v.C.y = mState.vertexAttribute[sw::Color0].mCurrentValue[1];
	v.C.z = mState.vertexAttribute[sw::Color0].mCurrentValue[2];
	v.C.w = mState.vertexAttribute[sw::Color0].mCurrentValue[3];
	v.N.x = mState.vertexAttribute[sw::Normal].mCurrentValue[0];
	v.N.y = mState.vertexAttribute[sw::Normal].mCurrentValue[1];
	v.N.z = mState.vertexAttribute[sw::Normal].mCurrentValue[2];
	v.N.w = mState.vertexAttribute[sw::Normal].mCurrentValue[3];
	v.T0.x = mState.vertexAttribute[sw::TexCoord0].mCurrentValue[0];
	v.T0.y = mState.vertexAttribute[sw::TexCoord0].mCurrentValue[1];
	v.T0.z = mState.vertexAttribute[sw::TexCoord0].mCurrentValue[2];
	v.T0.w = mState.vertexAttribute[sw::TexCoord0].mCurrentValue[3];
	v.T1.x = mState.vertexAttribute[sw::TexCoord1].mCurrentValue[0];
	v.T1.y = mState.vertexAttribute[sw::TexCoord1].mCurrentValue[1];
	v.T1.z = mState.vertexAttribute[sw::TexCoord1].mCurrentValue[2];
	v.T1.w = mState.vertexAttribute[sw::TexCoord1].mCurrentValue[3];

	vertex.push_back(v);
}

void Context::end()
{
	if(!drawing)
	{
		return error(GL_INVALID_OPERATION);
	}

	device->setProjectionMatrix(projection.current());
	device->setViewMatrix(modelView.current());
	device->setTextureMatrix(0, texture[0].current());
	device->setTextureMatrix(1, texture[1].current());
	device->setTextureTransform(0, texture[0].isIdentity() ? 0 : 4, false);
	device->setTextureTransform(1, texture[1].isIdentity() ? 0 : 4, false);

	captureAttribs();

	for(int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
	{
		mState.vertexAttribute[i].mArrayEnabled = false;
	}

	setVertexAttribState(sw::Position, 0, 4, GL_FLOAT, false, sizeof(InVertex), &vertex[0].P);
	setVertexAttribState(sw::Normal, 0, 4, GL_FLOAT, false, sizeof(InVertex), &vertex[0].N);
	setVertexAttribState(sw::Color0, 0, 4, GL_FLOAT, false, sizeof(InVertex), &vertex[0].C);
	setVertexAttribState(sw::TexCoord0, 0, 2, GL_FLOAT, false, sizeof(InVertex), &vertex[0].T0);
	setVertexAttribState(sw::TexCoord1, 0, 2, GL_FLOAT, false, sizeof(InVertex), &vertex[0].T1);

	mState.vertexAttribute[sw::Position].mArrayEnabled = true;
	mState.vertexAttribute[sw::Normal].mArrayEnabled = true;
	mState.vertexAttribute[sw::Color0].mArrayEnabled = true;
	mState.vertexAttribute[sw::TexCoord0].mArrayEnabled = true;
	mState.vertexAttribute[sw::TexCoord1].mArrayEnabled = true;

	applyState(drawMode);

	GLenum err = applyVertexBuffer(0, 0, vertex.size());
	if(err != GL_NO_ERROR)
	{
		return error(err);
	}

	applyTextures();

	switch(drawMode)
	{
	case GL_POINTS:
		UNIMPLEMENTED();
		break;
	case GL_LINES:
		UNIMPLEMENTED();
		break;
	case GL_LINE_STRIP:
		UNIMPLEMENTED();
		break;
	case GL_LINE_LOOP:
		UNIMPLEMENTED();
		break;
	case GL_TRIANGLES:
		UNIMPLEMENTED();
		break;
	case GL_TRIANGLE_STRIP:
		device->drawPrimitive(DRAW_TRIANGLESTRIP, vertex.size() - 2);
		break;
	case GL_TRIANGLE_FAN:
		UNIMPLEMENTED();
		break;
	case GL_QUADS:
		UNIMPLEMENTED();
		break;
	case GL_QUAD_STRIP:
		UNIMPLEMENTED();
		break;
	case GL_POLYGON:
		UNIMPLEMENTED();
		break;
	default:
		UNREACHABLE(drawMode);
	}

	restoreAttribs();

	drawing = false;
}

void Context::setColorLogicOpEnabled(bool colorLogicOpEnabled)
{
	if(mState.colorLogicOpEnabled != colorLogicOpEnabled)
	{
		mState.colorLogicOpEnabled = colorLogicOpEnabled;
		mColorLogicOperatorDirty = true;
	}
}

bool Context::isColorLogicOpEnabled()
{
	return mState.colorLogicOpEnabled;
}

void Context::setLogicalOperation(GLenum logicalOperation)
{
	if(mState.logicalOperation != logicalOperation)
	{
		mState.logicalOperation = logicalOperation;
		mColorLogicOperatorDirty = true;
	}
}

void Context::setColorMaterialEnabled(bool enable)
{
	device->setColorVertexEnable(enable);
}

void Context::setColorMaterialMode(GLenum mode)
{
	switch(mode)
	{
	case GL_EMISSION:
		device->setDiffuseMaterialSource(sw::MATERIAL_MATERIAL);
		device->setSpecularMaterialSource(sw::MATERIAL_MATERIAL);
		device->setAmbientMaterialSource(sw::MATERIAL_MATERIAL);
		device->setEmissiveMaterialSource(sw::MATERIAL_COLOR1);
		break;
	case GL_AMBIENT:
		device->setDiffuseMaterialSource(sw::MATERIAL_MATERIAL);
		device->setSpecularMaterialSource(sw::MATERIAL_MATERIAL);
		device->setAmbientMaterialSource(sw::MATERIAL_COLOR1);
		device->setEmissiveMaterialSource(sw::MATERIAL_MATERIAL);
		break;
	case GL_DIFFUSE:
		device->setDiffuseMaterialSource(sw::MATERIAL_COLOR1);
		device->setSpecularMaterialSource(sw::MATERIAL_MATERIAL);
		device->setAmbientMaterialSource(sw::MATERIAL_MATERIAL);
		device->setEmissiveMaterialSource(sw::MATERIAL_MATERIAL);
		break;
	case GL_SPECULAR:
		device->setDiffuseMaterialSource(sw::MATERIAL_MATERIAL);
		device->setSpecularMaterialSource(sw::MATERIAL_COLOR1);
		device->setAmbientMaterialSource(sw::MATERIAL_MATERIAL);
		device->setEmissiveMaterialSource(sw::MATERIAL_MATERIAL);
		break;
	case GL_AMBIENT_AND_DIFFUSE:
		device->setDiffuseMaterialSource(sw::MATERIAL_COLOR1);
		device->setSpecularMaterialSource(sw::MATERIAL_MATERIAL);
		device->setAmbientMaterialSource(sw::MATERIAL_COLOR1);
		device->setEmissiveMaterialSource(sw::MATERIAL_MATERIAL);
		break;
	default:
		UNREACHABLE(mode);
	}
}

Device *Context::getDevice()
{
	return device;
}

}
