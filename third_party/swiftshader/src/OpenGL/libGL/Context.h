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
// rendering operations.

#ifndef LIBGL_CONTEXT_H_
#define LIBGL_CONTEXT_H_

#include "ResourceManager.h"
#include "common/NameSpace.hpp"
#include "common/Object.hpp"
#include "Image.hpp"
#include "Renderer/Sampler.hpp"
#include "Renderer/Vertex.hpp"
#include "common/MatrixStack.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

#include <map>
#include <list>
#include <vector>

namespace gl
{
	class Display;
	class Surface;
	class Config;

	void APIENTRY glVertexAttribArray(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);

	class Command
	{
	public:
		Command() {};
		virtual ~Command() {};

		virtual void call() = 0;
	};

	class Command0 : public Command
	{
	public:
		Command0(void (APIENTRY *function)())
			: function(function)
		{
		}

		virtual void call()
		{
			function();
		}

		void (APIENTRY *function)();
	};

	template<typename A1>
	class Command1 : public Command
	{
	public:
		Command1(void (APIENTRY *function)(A1), A1 arg1)
			: function(function)
			, argument1(arg1)
		{
		}

		virtual void call()
		{
			function(argument1);
		}

		void (APIENTRY *function)(A1);
		A1 argument1;
	};

	template<typename A1, typename A2>
	class Command2 : public Command
	{
	public:
		Command2(void (APIENTRY *function)(A1, A2), A1 arg1, A2 arg2)
			: function(function)
			, argument1(arg1)
			, argument2(arg2)
		{
		}

		virtual void call()
		{
			function(argument1, argument2);
		}

		void (APIENTRY *function)(A1, A2);
		A1 argument1;
		A2 argument2;
	};

	template<typename A1, typename A2, typename A3>
	class Command3 : public Command
	{
	public:
		Command3(void (APIENTRY *function)(A1, A2, A3), A1 arg1, A2 arg2, A3 arg3)
			: function(function)
			, argument1(arg1)
			, argument2(arg2)
			, argument3(arg3)
		{
		}

		virtual void call()
		{
			function(argument1, argument2, argument3);
		}

		void (APIENTRY *function)(A1, A2, A3);
		A1 argument1;
		A2 argument2;
		A3 argument3;
	};

	template<typename A1, typename A2, typename A3, typename A4>
	class Command4 : public Command
	{
	public:
		Command4(void (APIENTRY *function)(A1, A2, A3, A4), A1 arg1, A2 arg2, A3 arg3, A4 arg4)
			: function(function)
			, argument1(arg1)
			, argument2(arg2)
			, argument3(arg3)
			, argument4(arg4)
		{
		}

		virtual void call()
		{
			function(argument1, argument2, argument3, argument4);
		}

		void (APIENTRY *function)(A1, A2, A3, A4);
		A1 argument1;
		A2 argument2;
		A3 argument3;
		A4 argument4;
	};

	template<typename A1, typename A2, typename A3, typename A4, typename A5>
	class Command5 : public Command
	{
	public:
		Command5(void (APIENTRY *function)(A1, A2, A3, A4, A5), A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5)
			: function(function)
			, argument1(arg1)
			, argument2(arg2)
			, argument3(arg3)
			, argument4(arg4)
			, argument5(arg5)
		{
		}

		virtual void call()
		{
			function(argument1, argument2, argument3, argument4, argument5);
		}

		void (APIENTRY *function)(A1, A2, A3, A4, A5);
		A1 argument1;
		A2 argument2;
		A3 argument3;
		A4 argument4;
		A5 argument5;
	};

	template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	class Command6 : public Command
	{
	public:
		Command6(void (APIENTRY *function)(A1, A2, A3, A4, A5, A6), A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5, A6 arg6)
			: function(function)
			, argument1(arg1)
			, argument2(arg2)
			, argument3(arg3)
			, argument4(arg4)
			, argument5(arg5)
			, argument6(arg6)
		{
		}

		~Command6()
		{
			if(function == glVertexAttribArray)
			{
				delete[] argument6;
			}
		}

		virtual void call()
		{
			function(argument1, argument2, argument3, argument4, argument5, argument6);
		}

		void (APIENTRY *function)(A1, A2, A3, A4, A5, A6);
		A1 argument1;
		A2 argument2;
		A3 argument3;
		A4 argument4;
		A5 argument5;
		A6 argument6;
	};

	inline Command0 *newCommand(void (APIENTRY *function)())
	{
		return new Command0(function);
	}

	template<typename A1>
	Command1<A1> *newCommand(void (APIENTRY *function)(A1), A1 arg1)
	{
		return new Command1<A1>(function, arg1);
	}

	template<typename A1, typename A2>
	Command2<A1, A2> *newCommand(void (APIENTRY *function)(A1, A2), A1 arg1, A2 arg2)
	{
		return new Command2<A1, A2>(function, arg1, arg2);
	}

	template<typename A1, typename A2, typename A3>
	Command3<A1, A2, A3> *newCommand(void (APIENTRY *function)(A1, A2, A3), A1 arg1, A2 arg2, A3 arg3)
	{
		return new Command3<A1, A2, A3>(function, arg1, arg2, arg3);
	}

	template<typename A1, typename A2, typename A3, typename A4>
	Command4<A1, A2, A3, A4> *newCommand(void (APIENTRY *function)(A1, A2, A3, A4), A1 arg1, A2 arg2, A3 arg3, A4 arg4)
	{
		return new Command4<A1, A2, A3, A4>(function, arg1, arg2, arg3, arg4);
	}

	template<typename A1, typename A2, typename A3, typename A4, typename A5>
	Command5<A1, A2, A3, A4, A5> *newCommand(void (APIENTRY *function)(A1, A2, A3, A4, A5), A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5)
	{
		return new Command5<A1, A2, A3, A4, A5>(function, arg1, arg2, arg3, arg4, arg5);
	}

	template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	Command6<A1, A2, A3, A4, A5, A6> *newCommand(void (APIENTRY *function)(A1, A2, A3, A4, A5, A6), A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5, A6 arg6)
	{
		return new Command6<A1, A2, A3, A4, A5, A6>(function, arg1, arg2, arg3, arg4, arg5, arg6);
	}

	class DisplayList
	{
	public:
		DisplayList()
		{
		}

		~DisplayList()
		{
			while(!list.empty())
			{
				delete list.back();
				list.pop_back();
			}
		}

		void call()
		{
			for(CommandList::iterator command = list.begin(); command != list.end(); command++)
			{
				(*command)->call();
			}
		}

		typedef std::list<Command*> CommandList;
		CommandList list;
	};

struct TranslatedAttribute;
struct TranslatedIndexData;

class Device;
class Buffer;
class Shader;
class Program;
class Texture;
class Texture2D;
class TextureCubeMap;
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
class Query;

enum
{
	MAX_VERTEX_ATTRIBS = 9,
	MAX_UNIFORM_VECTORS = 256,   // Device limit
	MAX_VERTEX_UNIFORM_VECTORS = sw::VERTEX_UNIFORM_VECTORS - 3,   // Reserve space for gl_DepthRange
	MAX_VARYING_VECTORS = 10,
	MAX_TEXTURE_IMAGE_UNITS = 2,
	MAX_VERTEX_TEXTURE_IMAGE_UNITS = 1,
	MAX_COMBINED_TEXTURE_IMAGE_UNITS = MAX_TEXTURE_IMAGE_UNITS + MAX_VERTEX_TEXTURE_IMAGE_UNITS,
	MAX_FRAGMENT_UNIFORM_VECTORS = sw::FRAGMENT_UNIFORM_VECTORS - 3,    // Reserve space for gl_DepthRange
	MAX_DRAW_BUFFERS = 1,

	IMPLEMENTATION_COLOR_READ_FORMAT = GL_RGB,
	IMPLEMENTATION_COLOR_READ_TYPE = GL_UNSIGNED_SHORT_5_6_5
};

const GLenum compressedTextureFormats[] =
{
	GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
};

const GLint NUM_COMPRESSED_TEXTURE_FORMATS = sizeof(compressedTextureFormats) / sizeof(compressedTextureFormats[0]);

const GLint multisampleCount[] = {4, 2, 1};
const GLint NUM_MULTISAMPLE_COUNTS = sizeof(multisampleCount) / sizeof(multisampleCount[0]);
const GLint IMPLEMENTATION_MAX_SAMPLES = multisampleCount[0];

const float ALIASED_LINE_WIDTH_RANGE_MIN = 1.0f;
const float ALIASED_LINE_WIDTH_RANGE_MAX = 128.0f;
const float ALIASED_POINT_SIZE_RANGE_MIN = 0.125f;
const float ALIASED_POINT_SIZE_RANGE_MAX = 8192.0f;
const float MAX_TEXTURE_MAX_ANISOTROPY = 16.0f;

enum QueryType
{
	QUERY_ANY_SAMPLES_PASSED,
	QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE,

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
	VertexAttribute() : mType(GL_FLOAT), mSize(0), mNormalized(false), mStride(0), mPointer(nullptr), mArrayEnabled(false)
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

	BindingPointer<Buffer> mBoundBuffer;   // Captured when glVertexAttribPointer is called.

	bool mArrayEnabled;   // From glEnable/DisableVertexAttribArray
	float mCurrentValue[4];   // From glVertexAttrib
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
	bool colorLogicOpEnabled;
	GLenum logicalOperation;

	GLfloat lineWidth;

	GLenum generateMipmapHint;
	GLenum fragmentShaderDerivativeHint;

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
	BindingPointer<Buffer> arrayBuffer;
	BindingPointer<Buffer> elementArrayBuffer;
	GLuint readFramebuffer;
	GLuint drawFramebuffer;
	BindingPointer<Renderbuffer> renderbuffer;
	GLuint currentProgram;

	VertexAttribute vertexAttribute[MAX_VERTEX_ATTRIBS];
	BindingPointer<Texture> samplerTexture[TEXTURE_TYPE_COUNT][MAX_COMBINED_TEXTURE_IMAGE_UNITS];
	BindingPointer<Query> activeQuery[QUERY_TYPE_COUNT];

	GLint unpackAlignment;
	GLint packAlignment;
};

class Context
{
public:
	Context(const Context *shareContext);

	~Context();

	void makeCurrent(Surface *surface);

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

	void setLineWidth(GLfloat width);

	void setGenerateMipmapHint(GLenum hint);
	void setFragmentShaderDerivativeHint(GLenum hint);

	void setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height);

	void setScissorTestEnabled(bool enabled);
	bool isScissorTestEnabled() const;
	void setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height);

	void setColorMask(bool red, bool green, bool blue, bool alpha);
	void setDepthMask(bool mask);

	void setActiveSampler(unsigned int active);

	GLuint getReadFramebufferName() const;
	GLuint getDrawFramebufferName() const;
	GLuint getRenderbufferName() const;

	GLuint getActiveQuery(GLenum target) const;

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

	// These create  and destroy methods are merely pass-throughs to
	// ResourceManager, which owns these object types
	GLuint createBuffer();
	GLuint createShader(GLenum type);
	GLuint createProgram();
	GLuint createTexture();
	GLuint createRenderbuffer();

	void deleteBuffer(GLuint buffer);
	void deleteShader(GLuint shader);
	void deleteProgram(GLuint program);
	void deleteTexture(GLuint texture);
	void deleteRenderbuffer(GLuint renderbuffer);

	// Framebuffers are owned by the Context, so these methods do not pass through
	GLuint createFramebuffer();
	void deleteFramebuffer(GLuint framebuffer);

	// Fences are owned by the Context
	GLuint createFence();
	void deleteFence(GLuint fence);

	// Queries are owned by the Context
	GLuint createQuery();
	void deleteQuery(GLuint query);

	void bindArrayBuffer(GLuint buffer);
	void bindElementArrayBuffer(GLuint buffer);
	void bindTexture2D(GLuint texture);
	void bindTextureCubeMap(GLuint texture);
	void bindReadFramebuffer(GLuint framebuffer);
	void bindDrawFramebuffer(GLuint framebuffer);
	void bindRenderbuffer(GLuint renderbuffer);
	void useProgram(GLuint program);

	void beginQuery(GLenum target, GLuint query);
	void endQuery(GLenum target);

	void setFramebufferZero(Framebuffer *framebuffer);

	void setRenderbufferStorage(RenderbufferStorage *renderbuffer);

	void setVertexAttrib(GLuint index, float x, float y, float z, float w);

	Buffer *getBuffer(GLuint handle);
	Fence *getFence(GLuint handle);
	Shader *getShader(GLuint handle);
	Program *getProgram(GLuint handle);
	Texture *getTexture(GLuint handle);
	Framebuffer *getFramebuffer(GLuint handle);
	Renderbuffer *getRenderbuffer(GLuint handle);
	Query *getQuery(GLuint handle, bool create, GLenum type);

	Buffer *getArrayBuffer();
	Buffer *getElementArrayBuffer();
	Program *getCurrentProgram();
	Texture2D *getTexture2D(GLenum target);
	TextureCubeMap *getTextureCubeMap();
	Texture *getSamplerTexture(unsigned int sampler, TextureType type);
	Framebuffer *getReadFramebuffer();
	Framebuffer *getDrawFramebuffer();

	bool getFloatv(GLenum pname, GLfloat *params);
	bool getIntegerv(GLenum pname, GLint *params);
	bool getBooleanv(GLenum pname, GLboolean *params);

	bool getQueryParameterInfo(GLenum pname, GLenum *type, unsigned int *numParams);

	void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei *bufSize, void* pixels);
	void clear(GLbitfield mask);
	void drawArrays(GLenum mode, GLint first, GLsizei count);
	void drawElements(GLenum mode, GLsizei count, GLenum type, const void *indices);
	void finish();
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
	                     GLbitfield mask);

	void setMatrixMode(GLenum mode);
	void loadIdentity();
	void pushMatrix();
	void popMatrix();
	void rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void translate(GLfloat x, GLfloat y, GLfloat z);
	void scale(GLfloat x, GLfloat y, GLfloat z);
	void multiply(const GLdouble *m);
	void multiply(const GLfloat *m);
	void frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void ortho(double left, double right, double bottom, double top, double zNear, double zFar);   // FIXME: GLdouble

	void setLightingEnabled(bool enabled);
	void setFogEnabled(bool enabled);
	void setAlphaTestEnabled(bool enabled);
	void alphaFunc(GLenum func, GLclampf ref);
	void setTexture2DEnabled(bool enabled);
	void setShadeModel(GLenum mode);
	void setLightEnabled(int index, bool enable);
	void setNormalizeNormalsEnabled(bool enable);

	GLuint genLists(GLsizei range);
	void newList(GLuint list, GLenum mode);
	void endList();
	void callList(GLuint list);
	void deleteList(GLuint list);
	GLuint getListIndex() {return listIndex;}
	GLenum getListMode() {return listMode;}
	void listCommand(Command *command);

	void captureAttribs();
	void captureDrawArrays(GLenum mode, GLint first, GLsizei count);
	void restoreAttribs();
	void clientActiveTexture(GLenum texture);
	GLenum getClientActiveTexture() const;
	unsigned int getActiveTexture() const;

	void begin(GLenum mode);
	void position(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void end();

	void setColorMaterialEnabled(bool enable);
	void setColorMaterialMode(GLenum mode);

	void setColorLogicOpEnabled(bool colorLogicOpEnabled);
	bool isColorLogicOpEnabled();
	void setLogicalOperation(GLenum logicalOperation);

	Device *getDevice();

private:
	bool applyRenderTarget();
	void applyState(GLenum drawMode);
	GLenum applyVertexBuffer(GLint base, GLint first, GLsizei count);
	GLenum applyIndexBuffer(const void *indices, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);
	void applyShaders();
	void applyTextures();
	void applyTextures(sw::SamplerType type);
	void applyTexture(sw::SamplerType type, int sampler, Texture *texture);

	void detachBuffer(GLuint buffer);
	void detachTexture(GLuint texture);
	void detachFramebuffer(GLuint framebuffer);
	void detachRenderbuffer(GLuint renderbuffer);

	bool cullSkipsDraw(GLenum drawMode);
	bool isTriangleMode(GLenum drawMode);

	State mState;

	BindingPointer<Texture2D> mTexture2DZero;
	BindingPointer<Texture2D> mProxyTexture2DZero;
	BindingPointer<TextureCubeMap> mTextureCubeMapZero;

	gl::NameSpace<Framebuffer> mFramebufferNameSpace;
	gl::NameSpace<Fence, 0> mFenceNameSpace;
	gl::NameSpace<Query> mQueryNameSpace;

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
	bool mPixelPackingStateDirty;
	bool mBlendStateDirty;
	bool mStencilStateDirty;
	bool mPolygonOffsetStateDirty;
	bool mSampleStateDirty;
	bool mFrontFaceDirty;
	bool mDitherStateDirty;
	bool mColorLogicOperatorDirty;

	Device *device;
	ResourceManager *mResourceManager;

	sw::MatrixStack &currentMatrixStack();
	GLenum matrixMode;
	sw::MatrixStack modelView;
	sw::MatrixStack projection;
	sw::MatrixStack texture[8];

	GLenum listMode;
	//std::map<GLuint, GLuint> listMap;
	std::map<GLuint, DisplayList*> displayList;
	DisplayList *list;
	GLuint listIndex;
	GLuint firstFreeIndex;

	GLenum clientTexture;

	bool drawing;
	GLenum drawMode;

	struct InVertex
	{
		sw::float4 P;    // Position
		sw::float4 N;    // Normal
		sw::float4 C;    // Color
		sw::float4 T0;   // Texture coordinate
		sw::float4 T1;
	};

	std::vector<InVertex> vertex;

	VertexAttribute clientAttribute[MAX_VERTEX_ATTRIBS];

	bool envEnable[8];
};
}

#endif   // INCLUDE_CONTEXT_H_
