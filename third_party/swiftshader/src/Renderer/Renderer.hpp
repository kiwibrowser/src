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

#ifndef sw_Renderer_hpp
#define sw_Renderer_hpp

#include "VertexProcessor.hpp"
#include "PixelProcessor.hpp"
#include "SetupProcessor.hpp"
#include "Plane.hpp"
#include "Blitter.hpp"
#include "Common/MutexLock.hpp"
#include "Common/Thread.hpp"
#include "Main/Config.hpp"

#include <list>

namespace sw
{
	class Clipper;
	class PixelShader;
	class VertexShader;
	class SwiftConfig;
	struct Task;
	class Resource;
	class Renderer;
	struct Constants;

	enum TranscendentalPrecision
	{
		APPROXIMATE,
		PARTIAL,	// 2^-10
		ACCURATE,
		WHQL,		// 2^-21
		IEEE		// 2^-23
	};

	extern TranscendentalPrecision logPrecision;
	extern TranscendentalPrecision expPrecision;
	extern TranscendentalPrecision rcpPrecision;
	extern TranscendentalPrecision rsqPrecision;
	extern bool perspectiveCorrection;

	struct Conventions
	{
		bool halfIntegerCoordinates;
		bool symmetricNormalizedDepth;
		bool booleanFaceRegister;
		bool fullPixelPositionRegister;
		bool leadingVertexFirst;
		bool secondaryColor;
		bool colorsDefaultToZero;
	};

	static const Conventions OpenGL =
	{
		true,    // halfIntegerCoordinates
		true,    // symmetricNormalizedDepth
		true,    // booleanFaceRegister
		true,    // fullPixelPositionRegister
		false,   // leadingVertexFirst
		false,   // secondaryColor
		true,    // colorsDefaultToZero
	};

	static const Conventions Direct3D =
	{
		false,   // halfIntegerCoordinates
		false,   // symmetricNormalizedDepth
		false,   // booleanFaceRegister
		false,   // fullPixelPositionRegister
		true,    // leadingVertexFirst
		true,    // secondardyColor
		false,   // colorsDefaultToZero
	};

	struct Query
	{
		enum Type { FRAGMENTS_PASSED, TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN };

		Query(Type type) : building(false), reference(0), data(0), type(type)
		{
		}

		void begin()
		{
			building = true;
			data = 0;
		}

		void end()
		{
			building = false;
		}

		bool building;
		AtomicInt reference;
		AtomicInt data;

		const Type type;
	};

	struct DrawData
	{
		const Constants *constants;

		const void *input[MAX_VERTEX_INPUTS];
		unsigned int stride[MAX_VERTEX_INPUTS];
		Texture mipmap[TOTAL_IMAGE_UNITS];
		const void *indices;

		struct VS
		{
			float4 c[VERTEX_UNIFORM_VECTORS + 1];   // One extra for indices out of range, c[VERTEX_UNIFORM_VECTORS] = {0, 0, 0, 0}
			byte* u[MAX_UNIFORM_BUFFER_BINDINGS];
			byte* t[MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS];
			unsigned int reg[MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS]; // Offset used when reading from registers, in components
			unsigned int row[MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS]; // Number of rows to read
			unsigned int col[MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS]; // Number of columns to read
			unsigned int str[MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS]; // Number of components between each varying in output buffer
			int4 i[16];
			bool b[16];
		};

		struct PS
		{
			word4 cW[8][4];
			float4 c[FRAGMENT_UNIFORM_VECTORS];
			byte* u[MAX_UNIFORM_BUFFER_BINDINGS];
			int4 i[16];
			bool b[16];
		};

		union
		{
			VS vs;
			VertexProcessor::FixedFunction ff;
		};

		PS ps;

		int instanceID;

		VertexProcessor::PointSprite point;
		float lineWidth;

		PixelProcessor::Stencil stencil[2];   // clockwise, counterclockwise
		PixelProcessor::Stencil stencilCCW;
		PixelProcessor::Fog fog;
		PixelProcessor::Factor factor;
		unsigned int occlusion[16];   // Number of pixels passing depth test

		#if PERF_PROFILE
			int64_t cycles[PERF_TIMERS][16];
		#endif

		TextureStage::Uniforms textureStage[8];

		float4 Wx16;
		float4 Hx16;
		float4 X0x16;
		float4 Y0x16;
		float4 XXXX;
		float4 YYYY;
		float4 halfPixelX;
		float4 halfPixelY;
		float viewportHeight;
		float slopeDepthBias;
		float depthRange;
		float depthNear;
		Plane clipPlane[6];

		unsigned int *colorBuffer[RENDERTARGETS];
		int colorPitchB[RENDERTARGETS];
		int colorSliceB[RENDERTARGETS];
		float *depthBuffer;
		int depthPitchB;
		int depthSliceB;
		unsigned char *stencilBuffer;
		int stencilPitchB;
		int stencilSliceB;

		int scissorX0;
		int scissorX1;
		int scissorY0;
		int scissorY1;

		float4 a2c0;
		float4 a2c1;
		float4 a2c2;
		float4 a2c3;
	};

	struct DrawCall
	{
		DrawCall();

		~DrawCall();

		AtomicInt drawType;
		AtomicInt batchSize;

		Routine *vertexRoutine;
		Routine *setupRoutine;
		Routine *pixelRoutine;

		VertexProcessor::RoutinePointer vertexPointer;
		SetupProcessor::RoutinePointer setupPointer;
		PixelProcessor::RoutinePointer pixelPointer;

		int (Renderer::*setupPrimitives)(int batch, int count);
		SetupProcessor::State setupState;

		Resource *vertexStream[MAX_VERTEX_INPUTS];
		Resource *indexBuffer;
		Surface *renderTarget[RENDERTARGETS];
		Surface *depthBuffer;
		Surface *stencilBuffer;
		Resource *texture[TOTAL_IMAGE_UNITS];
		Resource* pUniformBuffers[MAX_UNIFORM_BUFFER_BINDINGS];
		Resource* vUniformBuffers[MAX_UNIFORM_BUFFER_BINDINGS];
		Resource* transformFeedbackBuffers[MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS];

		unsigned int vsDirtyConstF;
		unsigned int vsDirtyConstI;
		unsigned int vsDirtyConstB;

		unsigned int psDirtyConstF;
		unsigned int psDirtyConstI;
		unsigned int psDirtyConstB;

		std::list<Query*> *queries;

		AtomicInt clipFlags;

		AtomicInt primitive;    // Current primitive to enter pipeline
		AtomicInt count;        // Number of primitives to render
		AtomicInt references;   // Remaining references to this draw call, 0 when done drawing, -1 when resources unlocked and slot is free

		DrawData *data;
	};

	struct Viewport
	{
		float x0;
		float y0;
		float width;
		float height;
		float minZ;
		float maxZ;
	};

	class Renderer : public VertexProcessor, public PixelProcessor, public SetupProcessor
	{
		struct Task
		{
			enum Type
			{
				PRIMITIVES,
				PIXELS,

				RESUME,
				SUSPEND
			};

			AtomicInt type;
			AtomicInt primitiveUnit;
			AtomicInt pixelCluster;
		};

		struct PrimitiveProgress
		{
			void init()
			{
				drawCall = 0;
				firstPrimitive = 0;
				primitiveCount = 0;
				visible = 0;
				references = 0;
			}

			AtomicInt drawCall;
			AtomicInt firstPrimitive;
			AtomicInt primitiveCount;
			AtomicInt visible;
			AtomicInt references;
		};

		struct PixelProgress
		{
			void init()
			{
				drawCall = 0;
				processedPrimitives = 0;
				executing = false;
			}

			AtomicInt drawCall;
			AtomicInt processedPrimitives;
			AtomicInt executing;
		};

	public:
		Renderer(Context *context, Conventions conventions, bool exactColorRounding);

		virtual ~Renderer();

		void *operator new(size_t size);
		void operator delete(void * mem);

		void draw(DrawType drawType, unsigned int indexOffset, unsigned int count, bool update = true);

		void clear(void *value, Format format, Surface *dest, const Rect &rect, unsigned int rgbaMask);
		void blit(Surface *source, const SliceRectF &sRect, Surface *dest, const SliceRect &dRect, bool filter, bool isStencil = false, bool sRGBconversion = true);
		void blit3D(Surface *source, Surface *dest);

		void setIndexBuffer(Resource *indexBuffer);

		void setMultiSampleMask(unsigned int mask);
		void setTransparencyAntialiasing(TransparencyAntialiasing transparencyAntialiasing);

		void setTextureResource(unsigned int sampler, Resource *resource);
		void setTextureLevel(unsigned int sampler, unsigned int face, unsigned int level, Surface *surface, TextureType type);

		void setTextureFilter(SamplerType type, int sampler, FilterType textureFilter);
		void setMipmapFilter(SamplerType type, int sampler, MipmapType mipmapFilter);
		void setGatherEnable(SamplerType type, int sampler, bool enable);
		void setAddressingModeU(SamplerType type, int sampler, AddressingMode addressingMode);
		void setAddressingModeV(SamplerType type, int sampler, AddressingMode addressingMode);
		void setAddressingModeW(SamplerType type, int sampler, AddressingMode addressingMode);
		void setReadSRGB(SamplerType type, int sampler, bool sRGB);
		void setMipmapLOD(SamplerType type, int sampler, float bias);
		void setBorderColor(SamplerType type, int sampler, const Color<float> &borderColor);
		void setMaxAnisotropy(SamplerType type, int sampler, float maxAnisotropy);
		void setHighPrecisionFiltering(SamplerType type, int sampler, bool highPrecisionFiltering);
		void setSwizzleR(SamplerType type, int sampler, SwizzleType swizzleR);
		void setSwizzleG(SamplerType type, int sampler, SwizzleType swizzleG);
		void setSwizzleB(SamplerType type, int sampler, SwizzleType swizzleB);
		void setSwizzleA(SamplerType type, int sampler, SwizzleType swizzleA);
		void setCompareFunc(SamplerType type, int sampler, CompareFunc compare);
		void setBaseLevel(SamplerType type, int sampler, int baseLevel);
		void setMaxLevel(SamplerType type, int sampler, int maxLevel);
		void setMinLod(SamplerType type, int sampler, float minLod);
		void setMaxLod(SamplerType type, int sampler, float maxLod);

		void setPointSpriteEnable(bool pointSpriteEnable);
		void setPointScaleEnable(bool pointScaleEnable);
		void setLineWidth(float width);

		void setDepthBias(float bias);
		void setSlopeDepthBias(float slopeBias);

		void setRasterizerDiscard(bool rasterizerDiscard);

		// Programmable pipelines
		void setPixelShader(const PixelShader *shader);
		void setVertexShader(const VertexShader *shader);

		void setPixelShaderConstantF(unsigned int index, const float value[4], unsigned int count = 1);
		void setPixelShaderConstantI(unsigned int index, const int value[4], unsigned int count = 1);
		void setPixelShaderConstantB(unsigned int index, const int *boolean, unsigned int count = 1);

		void setVertexShaderConstantF(unsigned int index, const float value[4], unsigned int count = 1);
		void setVertexShaderConstantI(unsigned int index, const int value[4], unsigned int count = 1);
		void setVertexShaderConstantB(unsigned int index, const int *boolean, unsigned int count = 1);

		// Viewport & Clipper
		void setViewport(const Viewport &viewport);
		void setScissor(const Rect &scissor);
		void setClipFlags(int flags);
		void setClipPlane(unsigned int index, const float plane[4]);

		// Partial transform
		void setModelMatrix(const Matrix &M, int i = 0);
		void setViewMatrix(const Matrix &V);
		void setBaseMatrix(const Matrix &B);
		void setProjectionMatrix(const Matrix &P);

		void addQuery(Query *query);
		void removeQuery(Query *query);

		void synchronize();

		#if PERF_HUD
			// Performance timers
			int getThreadCount();
			int64_t getVertexTime(int thread);
			int64_t getSetupTime(int thread);
			int64_t getPixelTime(int thread);
			void resetTimers();
		#endif

		static int getClusterCount() { return clusterCount; }

	private:
		static void threadFunction(void *parameters);
		void threadLoop(int threadIndex);
		void taskLoop(int threadIndex);
		void findAvailableTasks();
		void scheduleTask(int threadIndex);
		void executeTask(int threadIndex);
		void finishRendering(Task &pixelTask);

		void processPrimitiveVertices(int unit, unsigned int start, unsigned int count, unsigned int loop, int thread);

		int setupSolidTriangles(int batch, int count);
		int setupWireframeTriangle(int batch, int count);
		int setupVertexTriangle(int batch, int count);
		int setupLines(int batch, int count);
		int setupPoints(int batch, int count);

		bool setupLine(Primitive &primitive, Triangle &triangle, const DrawCall &draw);
		bool setupPoint(Primitive &primitive, Triangle &triangle, const DrawCall &draw);

		bool isReadWriteTexture(int sampler);
		void updateClipper();
		void updateConfiguration(bool initialUpdate = false);
		void initializeThreads();
		void terminateThreads();

		void loadConstants(const VertexShader *vertexShader);
		void loadConstants(const PixelShader *pixelShader);

		Context *context;
		Clipper *clipper;
		Blitter *blitter;
		Viewport viewport;
		Rect scissor;
		int clipFlags;

		Triangle *triangleBatch[16];
		Primitive *primitiveBatch[16];

		// User-defined clipping planes
		Plane userPlane[MAX_CLIP_PLANES];
		Plane clipPlane[MAX_CLIP_PLANES];   // Tranformed to clip space
		bool updateClipPlanes;

		AtomicInt exitThreads;
		AtomicInt threadsAwake;
		Thread *worker[16];
		Event *resume[16];         // Events for resuming threads
		Event *suspend[16];        // Events for suspending threads
		Event *resumeApp;          // Event for resuming the application thread

		PrimitiveProgress primitiveProgress[16];
		PixelProgress pixelProgress[16];
		Task task[16];   // Current tasks for threads

		enum {
			DRAW_COUNT = 16,   // Number of draw calls buffered (must be power of 2)
			DRAW_COUNT_BITS = DRAW_COUNT - 1,
		};
		DrawCall *drawCall[DRAW_COUNT];
		DrawCall *drawList[DRAW_COUNT];

		AtomicInt currentDraw;
		AtomicInt nextDraw;

		enum {
			TASK_COUNT = 32,   // Size of the task queue (must be power of 2)
			TASK_COUNT_BITS = TASK_COUNT - 1,
		};
		Task taskQueue[TASK_COUNT];
		AtomicInt qHead;
		AtomicInt qSize;

		static AtomicInt unitCount;
		static AtomicInt clusterCount;

		MutexLock schedulerMutex;

		#if PERF_HUD
			int64_t vertexTime[16];
			int64_t setupTime[16];
			int64_t pixelTime[16];
		#endif

		VertexTask *vertexTask[16];

		SwiftConfig *swiftConfig;

		std::list<Query*> queries;
		Resource *sync;

		VertexProcessor::State vertexState;
		SetupProcessor::State setupState;
		PixelProcessor::State pixelState;

		Routine *vertexRoutine;
		Routine *setupRoutine;
		Routine *pixelRoutine;
	};
}

#endif   // sw_Renderer_hpp
