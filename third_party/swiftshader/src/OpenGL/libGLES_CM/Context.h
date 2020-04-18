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

#ifndef LIBGLES_CM_CONTEXT_H_
#define LIBGLES_CM_CONTEXT_H_

#include "libEGL/Context.hpp"
#include "ResourceManager.h"
#include "common/NameSpace.hpp"
#include "common/Object.hpp"
#include "common/Image.hpp"
#include "Renderer/Sampler.hpp"
#include "common/MatrixStack.hpp"

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>

#include <map>
#include <string>

namespace gl { class Surface; }

namespace egl
{
class Display;
class Config;
}

namespace es1
{
struct TranslatedAttribute;
struct TranslatedIndexData;

class Device;
class Buffer;
class Texture;
class Texture2D;
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

enum
{
	MAX_VERTEX_ATTRIBS = sw::MAX_VERTEX_INPUTS,
	MAX_VARYING_VECTORS = 10,
	MAX_TEXTURE_UNITS = 2,
	MAX_DRAW_BUFFERS = 1,
	MAX_LIGHTS = 8,
	MAX_CLIP_PLANES = sw::MAX_CLIP_PLANES,

	MAX_MODELVIEW_STACK_DEPTH = 32,
	MAX_PROJECTION_STACK_DEPTH = 2,
	MAX_TEXTURE_STACK_DEPTH = 2,
};

const GLenum compressedTextureFormats[] =
{
	GL_ETC1_RGB8_OES,
	GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
};

const GLint NUM_COMPRESSED_TEXTURE_FORMATS = sizeof(compressedTextureFormats) / sizeof(compressedTextureFormats[0]);

const GLint multisampleCount[] = {4, 2, 1};
const GLint NUM_MULTISAMPLE_COUNTS = sizeof(multisampleCount) / sizeof(multisampleCount[0]);
const GLint IMPLEMENTATION_MAX_SAMPLES = multisampleCount[0];

const float ALIASED_LINE_WIDTH_RANGE_MIN = 1.0f;
const float ALIASED_LINE_WIDTH_RANGE_MAX = 1.0f;
const float ALIASED_POINT_SIZE_RANGE_MIN = 0.125f;
const float ALIASED_POINT_SIZE_RANGE_MAX = 8192.0f;
const float SMOOTH_LINE_WIDTH_RANGE_MIN = 1.0f;
const float SMOOTH_LINE_WIDTH_RANGE_MAX = 1.0f;
const float SMOOTH_POINT_SIZE_RANGE_MIN = 0.125f;
const float SMOOTH_POINT_SIZE_RANGE_MAX = 8192.0f;
const float MAX_TEXTURE_MAX_ANISOTROPY = 16.0f;

struct Color
{
	float red;
	float green;
	float blue;
	float alpha;
};

struct Point
{
	float x;
	float y;
	float z;
	float w;
};

struct Vector
{
	float x;
	float y;
	float z;
};

struct Attenuation
{
	float constant;
	float linear;
	float quadratic;
};

struct Light
{
	bool enabled;
	Color ambient;
	Color diffuse;
	Color specular;
	Point position;
	Vector direction;
	Attenuation attenuation;
	float spotExponent;
	float spotCutoffAngle;
};

// Helper structure describing a single vertex attribute
class VertexAttribute
{
public:
	VertexAttribute() : mType(GL_FLOAT), mSize(4), mNormalized(false), mStride(0), mPointer(nullptr), mArrayEnabled(false)
	{
		mCurrentValue[0] = 0.0f;
		mCurrentValue[1] = 0.0f;
		mCurrentValue[2] = 0.0f;
		mCurrentValue[3] = 1.0f;
	}

	int typeSize() const
	{
		switch(mType)
		{
		case GL_BYTE:           return mSize * sizeof(GLbyte);
		case GL_UNSIGNED_BYTE:  return mSize * sizeof(GLubyte);
		case GL_SHORT:          return mSize * sizeof(GLshort);
		case GL_UNSIGNED_SHORT: return mSize * sizeof(GLushort);
		case GL_FIXED:          return mSize * sizeof(GLfixed);
		case GL_FLOAT:          return mSize * sizeof(GLfloat);
		default: UNREACHABLE(mType); return mSize * sizeof(GLfloat);
		}
	}

	GLsizei stride() const
	{
		return mStride ? mStride : typeSize();
	}

	// From glVertexAttribPointer
	GLenum mType;
	GLint mSize;
	bool mNormalized;
	GLsizei mStride;   // 0 means natural stride

	union
	{
		const void *mPointer;
		intptr_t mOffset;
	};

	gl::BindingPointer<Buffer> mBoundBuffer;   // Captured when glVertexAttribPointer is called.

	bool mArrayEnabled;   // From glEnable/DisableVertexAttribArray
	float mCurrentValue[4];   // From glVertexAttrib
};

typedef VertexAttribute VertexAttributeArray[MAX_VERTEX_ATTRIBS];

struct TextureUnit
{
	Color color;
	GLenum environmentMode;
	GLenum combineRGB;
	GLenum combineAlpha;
	GLenum src0RGB;
	GLenum src0Alpha;
	GLenum src1RGB;
	GLenum src1Alpha;
	GLenum src2RGB;
	GLenum src2Alpha;
	GLenum operand0RGB;
	GLenum operand0Alpha;
	GLenum operand1RGB;
	GLenum operand1Alpha;
	GLenum operand2RGB;
	GLenum operand2Alpha;
};

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
	bool stencilTestEnabled;
	GLenum stencilFunc;
	GLint stencilRef;
	GLuint stencilMask;
	GLenum stencilFail;
	GLenum stencilPassDepthFail;
	GLenum stencilPassDepthPass;
	GLuint stencilWritemask;
	bool polygonOffsetFillEnabled;
	GLfloat polygonOffsetFactor;
	GLfloat polygonOffsetUnits;
	bool sampleAlphaToCoverageEnabled;
	bool sampleCoverageEnabled;
	GLclampf sampleCoverageValue;
	bool sampleCoverageInvert;
	bool scissorTestEnabled;
	bool ditherEnabled;
	GLenum shadeModel;

	GLfloat lineWidth;

	GLenum generateMipmapHint;
	GLenum perspectiveCorrectionHint;
	GLenum fogHint;

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
	gl::BindingPointer<Buffer> elementArrayBuffer;
	GLuint framebuffer;
	gl::BindingPointer<Renderbuffer> renderbuffer;

	VertexAttribute vertexAttribute[MAX_VERTEX_ATTRIBS];
	gl::BindingPointer<Texture> samplerTexture[TEXTURE_TYPE_COUNT][MAX_TEXTURE_UNITS];

	GLint unpackAlignment;
	GLint packAlignment;

	TextureUnit textureUnit[MAX_TEXTURE_UNITS];
};

class [[clang::lto_visibility_public]] Context : public egl::Context
{
public:
	Context(egl::Display *display, const Context *shareContext, const egl::Config *config);

	void makeCurrent(gl::Surface *surface) override;
	EGLint getClientVersion() const override;
	EGLint getConfigID() const override;

	void finish() override;

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

	void setAlphaTestEnabled(bool enabled);
	bool isAlphaTestEnabled() const;
	void setAlphaFunc(GLenum alphaFunc, GLclampf reference);

	void setBlendEnabled(bool enabled);
	bool isBlendEnabled() const;
	void setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha);
	void setBlendEquation(GLenum rgbEquation, GLenum alphaEquation);

	void setStencilTestEnabled(bool enabled);
	bool isStencilTestEnabled() const;
	void setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask);
	void setStencilWritemask(GLuint stencilWritemask);
	void setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass);

	void setPolygonOffsetFillEnabled(bool enabled);
	bool isPolygonOffsetFillEnabled() const;
	void setPolygonOffsetParams(GLfloat factor, GLfloat units);

	void setSampleAlphaToCoverageEnabled(bool enabled);
	bool isSampleAlphaToCoverageEnabled() const;
	void setSampleCoverageEnabled(bool enabled);
	bool isSampleCoverageEnabled() const;
	void setSampleCoverageParams(GLclampf value, bool invert);

	void setShadeModel(GLenum mode);
	void setDitherEnabled(bool enabled);
	bool isDitherEnabled() const;
	void setLightingEnabled(bool enabled);
	bool isLightingEnabled() const;
	void setLightEnabled(int index, bool enable);
	bool isLightEnabled(int index) const;
	void setLightAmbient(int index, float r, float g, float b, float a);
	void setLightDiffuse(int index, float r, float g, float b, float a);
	void setLightSpecular(int index, float r, float g, float b, float a);
	void setLightPosition(int index, float x, float y, float z, float w);
	void setLightDirection(int index, float x, float y, float z);
	void setLightAttenuationConstant(int index, float constant);
	void setLightAttenuationLinear(int index, float linear);
	void setLightAttenuationQuadratic(int index, float quadratic);
	void setSpotLightExponent(int index, float exponent);
	void setSpotLightCutoff(int index, float cutoff);

	void setGlobalAmbient(float red, float green, float blue, float alpha);
	void setMaterialAmbient(float red, float green, float blue, float alpha);
	void setMaterialDiffuse(float red, float green, float blue, float alpha);
	void setMaterialSpecular(float red, float green, float blue, float alpha);
	void setMaterialEmission(float red, float green, float blue, float alpha);
	void setMaterialShininess(float shininess);
	void setLightModelTwoSide(bool enable);

	void setFogEnabled(bool enabled);
	bool isFogEnabled() const;
	void setFogMode(GLenum mode);
	void setFogDensity(float fogDensity);
	void setFogStart(float fogStart);
	void setFogEnd(float fogEnd);
	void setFogColor(float r, float g, float b, float a);

	void setTexture2Denabled(bool enabled);
	bool isTexture2Denabled() const;
	void setTextureExternalEnabled(bool enabled);
	bool isTextureExternalEnabled() const;
	void clientActiveTexture(GLenum texture);
	GLenum getClientActiveTexture() const;
	unsigned int getActiveTexture() const;

	void setTextureEnvMode(GLenum texEnvMode);
	void setTextureEnvColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void setCombineRGB(GLenum combineRGB);
	void setCombineAlpha(GLenum combineAlpha);
	void setOperand0RGB(GLenum operand);
	void setOperand1RGB(GLenum operand);
	void setOperand2RGB(GLenum operand);
	void setOperand0Alpha(GLenum operand);
	void setOperand1Alpha(GLenum operand);
	void setOperand2Alpha(GLenum operand);
	void setSrc0RGB(GLenum src);
	void setSrc1RGB(GLenum src);
	void setSrc2RGB(GLenum src);
	void setSrc0Alpha(GLenum src);
	void setSrc1Alpha(GLenum src);
	void setSrc2Alpha(GLenum src);

	void setLineWidth(GLfloat width);

	void setGenerateMipmapHint(GLenum hint);
	void setPerspectiveCorrectionHint(GLenum hint);
	void setFogHint(GLenum hint);

	void setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height);

	void setScissorTestEnabled(bool enabled);
	bool isScissorTestEnabled() const;
	void setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height);

	void setColorMask(bool red, bool green, bool blue, bool alpha);
	void setDepthMask(bool mask);

	void setActiveSampler(unsigned int active);

	GLuint getFramebufferName() const;
	GLuint getRenderbufferName() const;

	GLuint getArrayBufferName() const;

	void setVertexAttribArrayEnabled(unsigned int attribNum, bool enabled);
	const VertexAttribute &getVertexAttribState(unsigned int attribNum);
	void setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type,
	                          bool normalized, GLsizei stride, const void *pointer);
	const void *getVertexAttribPointer(unsigned int attribNum) const;

	const VertexAttributeArray &getVertexAttributes();

	void setUnpackAlignment(GLint alignment);
	GLint getUnpackAlignment() const;

	void setPackAlignment(GLint alignment);
	GLint getPackAlignment() const;

	// These create and destroy methods are merely pass-throughs to
	// ResourceManager, which owns these object types
	GLuint createBuffer();
	GLuint createTexture();
	GLuint createRenderbuffer();

	void deleteBuffer(GLuint buffer);
	void deleteTexture(GLuint texture);
	void deleteRenderbuffer(GLuint renderbuffer);

	// Framebuffers are owned by the Context, so these methods do not pass through
	GLuint createFramebuffer();
	void deleteFramebuffer(GLuint framebuffer);

	void bindArrayBuffer(GLuint buffer);
	void bindElementArrayBuffer(GLuint buffer);
	void bindTexture(TextureType type, GLuint texture);
	void bindTextureExternal(GLuint texture);
	void bindFramebuffer(GLuint framebuffer);
	void bindRenderbuffer(GLuint renderbuffer);

	void setFramebufferZero(Framebuffer *framebuffer);

	void setRenderbufferStorage(RenderbufferStorage *renderbuffer);

	void setVertexAttrib(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

	Buffer *getBuffer(GLuint handle);
	virtual Texture *getTexture(GLuint handle);
	Framebuffer *getFramebuffer(GLuint handle);
	virtual Renderbuffer *getRenderbuffer(GLuint handle);

	Buffer *getArrayBuffer();
	Buffer *getElementArrayBuffer();
	Texture2D *getTexture2D();
	TextureExternal *getTextureExternal();
	Texture *getSamplerTexture(unsigned int sampler, TextureType type);
	Framebuffer *getFramebuffer();

	bool getFloatv(GLenum pname, GLfloat *params);
	bool getIntegerv(GLenum pname, GLint *params);
	bool getBooleanv(GLenum pname, GLboolean *params);
	bool getPointerv(GLenum pname, const GLvoid **params);

	int getQueryParameterNum(GLenum pname);
	bool isQueryParameterInt(GLenum pname);
	bool isQueryParameterFloat(GLenum pname);
	bool isQueryParameterBool(GLenum pname);
	bool isQueryParameterPointer(GLenum pname);

	void drawArrays(GLenum mode, GLint first, GLsizei count);
	void drawElements(GLenum mode, GLsizei count, GLenum type, const void *indices);
	void drawTexture(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);
	void blit(sw::Surface *source, const sw::SliceRect &sRect, sw::Surface *dest, const sw::SliceRect &dRect) override;
	void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei *bufSize, void* pixels);
	void clear(GLbitfield mask);
	void flush();

	void recordInvalidEnum();
	void recordInvalidValue();
	void recordInvalidOperation();
	void recordOutOfMemory();
	void recordInvalidFramebufferOperation();
	void recordMatrixStackOverflow();
	void recordMatrixStackUnderflow();

	GLenum getError();

	static int getSupportedMultisampleCount(int requested);

	void bindTexImage(gl::Surface *surface) override;
	EGLenum validateSharedImage(EGLenum target, GLuint name, GLuint textureLevel) override;
	egl::Image *createSharedImage(EGLenum target, GLuint name, GLuint textureLevel) override;
	egl::Image *getSharedImage(GLeglImageOES image);

	Device *getDevice();

	void setMatrixMode(GLenum mode);
	void loadIdentity();
	void load(const GLfloat *m);
	void pushMatrix();
	void popMatrix();
	void rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void translate(GLfloat x, GLfloat y, GLfloat z);
	void scale(GLfloat x, GLfloat y, GLfloat z);
	void multiply(const GLfloat *m);
	void frustum(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
	void ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

	void setClipPlane(int index, const float plane[4]);
	void setClipPlaneEnabled(int index, bool enable);
	bool isClipPlaneEnabled(int index) const;

	void setColorLogicOpEnabled(bool enable);
	bool isColorLogicOpEnabled() const;
	void setLogicalOperation(GLenum logicOp);

	void setPointSmoothEnabled(bool enable);
	bool isPointSmoothEnabled() const;

	void setLineSmoothEnabled(bool enable);
	bool isLineSmoothEnabled() const;

	void setColorMaterialEnabled(bool enable);
	bool isColorMaterialEnabled() const;

	void setNormalizeEnabled(bool enable);
	bool isNormalizeEnabled() const;

	void setRescaleNormalEnabled(bool enable);
	bool isRescaleNormalEnabled() const;

	void setVertexArrayEnabled(bool enable);
	bool isVertexArrayEnabled() const;

	void setNormalArrayEnabled(bool enable);
	bool isNormalArrayEnabled() const;

	void setColorArrayEnabled(bool enable);
	bool isColorArrayEnabled() const;

	void setPointSizeArrayEnabled(bool enable);
	bool isPointSizeArrayEnabled() const;

	void setTextureCoordArrayEnabled(bool enable);
	bool isTextureCoordArrayEnabled() const;

	void setMultisampleEnabled(bool enable);
	bool isMultisampleEnabled() const;

	void setSampleAlphaToOneEnabled(bool enable);
	bool isSampleAlphaToOneEnabled() const;

	void setPointSpriteEnabled(bool enable);
	bool isPointSpriteEnabled() const;
	void setPointSizeMin(float min);
	void setPointSizeMax(float max);
	void setPointDistanceAttenuation(float a, float b, float c);
	void setPointFadeThresholdSize(float threshold);

private:
	~Context() override;

	bool applyRenderTarget();
	void applyState(GLenum drawMode);
	GLenum applyVertexBuffer(GLint base, GLint first, GLsizei count);
	GLenum applyIndexBuffer(const void *indices, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);
	void applyTextures();
	void applyTexture(int sampler, Texture *texture);

	void detachBuffer(GLuint buffer);
	void detachTexture(GLuint texture);
	void detachFramebuffer(GLuint framebuffer);
	void detachRenderbuffer(GLuint renderbuffer);

	bool cullSkipsDraw(GLenum drawMode);
	bool isTriangleMode(GLenum drawMode);

	const egl::Config *const config;

	State mState;

	gl::BindingPointer<Texture2D> mTexture2DZero;
	gl::BindingPointer<TextureExternal> mTextureExternalZero;

	gl::NameSpace<Framebuffer> mFramebufferNameSpace;

	VertexDataManager *mVertexDataManager;
	IndexDataManager *mIndexDataManager;

	bool lightingEnabled;
	Light light[MAX_LIGHTS];
	Color globalAmbient;
	Color materialAmbient;
	Color materialDiffuse;
	Color materialSpecular;
	Color materialEmission;
	GLfloat materialShininess;
	bool lightModelTwoSide;

	// Recorded errors
	bool mInvalidEnum;
	bool mInvalidValue;
	bool mInvalidOperation;
	bool mOutOfMemory;
	bool mInvalidFramebufferOperation;
	bool mMatrixStackOverflow;
	bool mMatrixStackUnderflow;

	bool mHasBeenCurrent;

	// state caching flags
	bool mDepthStateDirty;
	bool mMaskStateDirty;
	bool mBlendStateDirty;
	bool mStencilStateDirty;
	bool mPolygonOffsetStateDirty;
	bool mSampleStateDirty;
	bool mFrontFaceDirty;
	bool mDitherStateDirty;

	sw::MatrixStack &currentMatrixStack();
	GLenum matrixMode;
	sw::MatrixStack modelViewStack;
	sw::MatrixStack projectionStack;
	sw::MatrixStack textureStack0;
	sw::MatrixStack textureStack1;

	bool texture2Denabled[MAX_TEXTURE_UNITS];
	bool textureExternalEnabled[MAX_TEXTURE_UNITS];
	GLenum clientTexture;

	int clipFlags;

	bool alphaTestEnabled;
	GLenum alphaTestFunc;
	float alphaTestRef;

	bool fogEnabled;
	GLenum fogMode;
	float fogDensity;
	float fogStart;
	float fogEnd;
	Color fogColor;

	bool lineSmoothEnabled;
	bool colorMaterialEnabled;
	bool normalizeEnabled;
	bool rescaleNormalEnabled;
	bool multisampleEnabled;
	bool sampleAlphaToOneEnabled;

	bool pointSpriteEnabled;
	bool pointSmoothEnabled;
	float pointSizeMin;
	float pointSizeMax;
	Attenuation pointDistanceAttenuation;
	float pointFadeThresholdSize;

	bool colorLogicOpEnabled;
	GLenum logicalOperation;

	Device *device;
	ResourceManager *mResourceManager;
};
}

#endif   // INCLUDE_CONTEXT_H_
