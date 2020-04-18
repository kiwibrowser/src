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

#include "Renderer.hpp"

#include "Clipper.hpp"
#include "Surface.hpp"
#include "Primitive.hpp"
#include "Polygon.hpp"
#include "Main/FrameBuffer.hpp"
#include "Main/SwiftConfig.hpp"
#include "Reactor/Reactor.hpp"
#include "Shader/Constants.hpp"
#include "Common/MutexLock.hpp"
#include "Common/CPUID.hpp"
#include "Common/Memory.hpp"
#include "Common/Resource.hpp"
#include "Common/Half.hpp"
#include "Common/Math.hpp"
#include "Common/Timer.hpp"
#include "Common/Debug.hpp"

#undef max

bool disableServer = true;

#ifndef NDEBUG
unsigned int minPrimitives = 1;
unsigned int maxPrimitives = 1 << 21;
#endif

namespace sw
{
	extern bool halfIntegerCoordinates;     // Pixel centers are not at integer coordinates
	extern bool symmetricNormalizedDepth;   // [-1, 1] instead of [0, 1]
	extern bool booleanFaceRegister;
	extern bool fullPixelPositionRegister;
	extern bool leadingVertexFirst;         // Flat shading uses first vertex, else last
	extern bool secondaryColor;             // Specular lighting is applied after texturing
	extern bool colorsDefaultToZero;

	extern bool forceWindowed;
	extern bool complementaryDepthBuffer;
	extern bool postBlendSRGB;
	extern bool exactColorRounding;
	extern TransparencyAntialiasing transparencyAntialiasing;
	extern bool forceClearRegisters;

	extern bool precacheVertex;
	extern bool precacheSetup;
	extern bool precachePixel;

	static const int batchSize = 128;
	AtomicInt threadCount(1);
	AtomicInt Renderer::unitCount(1);
	AtomicInt Renderer::clusterCount(1);

	TranscendentalPrecision logPrecision = ACCURATE;
	TranscendentalPrecision expPrecision = ACCURATE;
	TranscendentalPrecision rcpPrecision = ACCURATE;
	TranscendentalPrecision rsqPrecision = ACCURATE;
	bool perspectiveCorrection = true;

	struct Parameters
	{
		Renderer *renderer;
		int threadIndex;
	};

	DrawCall::DrawCall()
	{
		queries = 0;

		vsDirtyConstF = VERTEX_UNIFORM_VECTORS + 1;
		vsDirtyConstI = 16;
		vsDirtyConstB = 16;

		psDirtyConstF = FRAGMENT_UNIFORM_VECTORS;
		psDirtyConstI = 16;
		psDirtyConstB = 16;

		references = -1;

		data = (DrawData*)allocate(sizeof(DrawData));
		data->constants = &constants;
	}

	DrawCall::~DrawCall()
	{
		delete queries;

		deallocate(data);
	}

	Renderer::Renderer(Context *context, Conventions conventions, bool exactColorRounding) : VertexProcessor(context), PixelProcessor(context), SetupProcessor(context), context(context), viewport()
	{
		sw::halfIntegerCoordinates = conventions.halfIntegerCoordinates;
		sw::symmetricNormalizedDepth = conventions.symmetricNormalizedDepth;
		sw::booleanFaceRegister = conventions.booleanFaceRegister;
		sw::fullPixelPositionRegister = conventions.fullPixelPositionRegister;
		sw::leadingVertexFirst = conventions.leadingVertexFirst;
		sw::secondaryColor = conventions.secondaryColor;
		sw::colorsDefaultToZero = conventions.colorsDefaultToZero;
		sw::exactColorRounding = exactColorRounding;

		setRenderTarget(0, 0);
		clipper = new Clipper(symmetricNormalizedDepth);
		blitter = new Blitter;

		updateViewMatrix = true;
		updateBaseMatrix = true;
		updateProjectionMatrix = true;
		updateClipPlanes = true;

		#if PERF_HUD
			resetTimers();
		#endif

		for(int i = 0; i < 16; i++)
		{
			vertexTask[i] = 0;

			worker[i] = 0;
			resume[i] = 0;
			suspend[i] = 0;
		}

		threadsAwake = 0;
		resumeApp = new Event();

		currentDraw = 0;
		nextDraw = 0;

		qHead = 0;
		qSize = 0;

		for(int i = 0; i < 16; i++)
		{
			triangleBatch[i] = 0;
			primitiveBatch[i] = 0;
		}

		for(int draw = 0; draw < DRAW_COUNT; draw++)
		{
			drawCall[draw] = new DrawCall();
			drawList[draw] = drawCall[draw];
		}

		for(int unit = 0; unit < 16; unit++)
		{
			primitiveProgress[unit].init();
		}

		for(int cluster = 0; cluster < 16; cluster++)
		{
			pixelProgress[cluster].init();
		}

		clipFlags = 0;

		swiftConfig = new SwiftConfig(disableServer);
		updateConfiguration(true);

		sync = new Resource(0);
	}

	Renderer::~Renderer()
	{
		sync->destruct();

		delete clipper;
		clipper = nullptr;

		delete blitter;
		blitter = nullptr;

		terminateThreads();
		delete resumeApp;

		for(int draw = 0; draw < DRAW_COUNT; draw++)
		{
			delete drawCall[draw];
		}

		delete swiftConfig;
	}

	// This object has to be mem aligned
	void* Renderer::operator new(size_t size)
	{
		ASSERT(size == sizeof(Renderer)); // This operator can't be called from a derived class
		return sw::allocate(sizeof(Renderer), 16);
	}

	void Renderer::operator delete(void * mem)
	{
		sw::deallocate(mem);
	}

	void Renderer::draw(DrawType drawType, unsigned int indexOffset, unsigned int count, bool update)
	{
		#ifndef NDEBUG
			if(count < minPrimitives || count > maxPrimitives)
			{
				return;
			}
		#endif

		context->drawType = drawType;

		updateConfiguration();
		updateClipper();

		int ss = context->getSuperSampleCount();
		int ms = context->getMultiSampleCount();

		for(int q = 0; q < ss; q++)
		{
			unsigned int oldMultiSampleMask = context->multiSampleMask;
			context->multiSampleMask = (context->sampleMask >> (ms * q)) & ((unsigned)0xFFFFFFFF >> (32 - ms));

			if(!context->multiSampleMask)
			{
				continue;
			}

			sync->lock(sw::PRIVATE);

			if(update || oldMultiSampleMask != context->multiSampleMask)
			{
				vertexState = VertexProcessor::update(drawType);
				setupState = SetupProcessor::update();
				pixelState = PixelProcessor::update();

				vertexRoutine = VertexProcessor::routine(vertexState);
				setupRoutine = SetupProcessor::routine(setupState);
				pixelRoutine = PixelProcessor::routine(pixelState);
			}

			int batch = batchSize / ms;

			int (Renderer::*setupPrimitives)(int batch, int count);

			if(context->isDrawTriangle())
			{
				switch(context->fillMode)
				{
				case FILL_SOLID:
					setupPrimitives = &Renderer::setupSolidTriangles;
					break;
				case FILL_WIREFRAME:
					setupPrimitives = &Renderer::setupWireframeTriangle;
					batch = 1;
					break;
				case FILL_VERTEX:
					setupPrimitives = &Renderer::setupVertexTriangle;
					batch = 1;
					break;
				default:
					ASSERT(false);
					return;
				}
			}
			else if(context->isDrawLine())
			{
				setupPrimitives = &Renderer::setupLines;
			}
			else   // Point draw
			{
				setupPrimitives = &Renderer::setupPoints;
			}

			DrawCall *draw = nullptr;

			do
			{
				for(int i = 0; i < DRAW_COUNT; i++)
				{
					if(drawCall[i]->references == -1)
					{
						draw = drawCall[i];
						drawList[nextDraw & DRAW_COUNT_BITS] = draw;

						break;
					}
				}

				if(!draw)
				{
					resumeApp->wait();
				}
			}
			while(!draw);

			DrawData *data = draw->data;

			if(queries.size() != 0)
			{
				draw->queries = new std::list<Query*>();
				bool includePrimitivesWrittenQueries = vertexState.transformFeedbackQueryEnabled && vertexState.transformFeedbackEnabled;
				for(auto &query : queries)
				{
					if(includePrimitivesWrittenQueries || (query->type != Query::TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN))
					{
						++query->reference; // Atomic
						draw->queries->push_back(query);
					}
				}
			}

			draw->drawType = drawType;
			draw->batchSize = batch;

			vertexRoutine->bind();
			setupRoutine->bind();
			pixelRoutine->bind();

			draw->vertexRoutine = vertexRoutine;
			draw->setupRoutine = setupRoutine;
			draw->pixelRoutine = pixelRoutine;
			draw->vertexPointer = (VertexProcessor::RoutinePointer)vertexRoutine->getEntry();
			draw->setupPointer = (SetupProcessor::RoutinePointer)setupRoutine->getEntry();
			draw->pixelPointer = (PixelProcessor::RoutinePointer)pixelRoutine->getEntry();
			draw->setupPrimitives = setupPrimitives;
			draw->setupState = setupState;

			for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
			{
				draw->vertexStream[i] = context->input[i].resource;
				data->input[i] = context->input[i].buffer;
				data->stride[i] = context->input[i].stride;

				if(draw->vertexStream[i])
				{
					draw->vertexStream[i]->lock(PUBLIC, PRIVATE);
				}
			}

			if(context->indexBuffer)
			{
				data->indices = (unsigned char*)context->indexBuffer->lock(PUBLIC, PRIVATE) + indexOffset;
			}

			draw->indexBuffer = context->indexBuffer;

			for(int sampler = 0; sampler < TOTAL_IMAGE_UNITS; sampler++)
			{
				draw->texture[sampler] = 0;
			}

			for(int sampler = 0; sampler < TEXTURE_IMAGE_UNITS; sampler++)
			{
				if(pixelState.sampler[sampler].textureType != TEXTURE_NULL)
				{
					draw->texture[sampler] = context->texture[sampler];
					draw->texture[sampler]->lock(PUBLIC, isReadWriteTexture(sampler) ? MANAGED : PRIVATE);   // If the texure is both read and written, use the same read/write lock as render targets

					data->mipmap[sampler] = context->sampler[sampler].getTextureData();
				}
			}

			if(context->pixelShader)
			{
				if(draw->psDirtyConstF)
				{
					memcpy(&data->ps.cW, PixelProcessor::cW, sizeof(word4) * 4 * (draw->psDirtyConstF < 8 ? draw->psDirtyConstF : 8));
					memcpy(&data->ps.c, PixelProcessor::c, sizeof(float4) * draw->psDirtyConstF);
					draw->psDirtyConstF = 0;
				}

				if(draw->psDirtyConstI)
				{
					memcpy(&data->ps.i, PixelProcessor::i, sizeof(int4) * draw->psDirtyConstI);
					draw->psDirtyConstI = 0;
				}

				if(draw->psDirtyConstB)
				{
					memcpy(&data->ps.b, PixelProcessor::b, sizeof(bool) * draw->psDirtyConstB);
					draw->psDirtyConstB = 0;
				}

				PixelProcessor::lockUniformBuffers(data->ps.u, draw->pUniformBuffers);
			}
			else
			{
				for(int i = 0; i < MAX_UNIFORM_BUFFER_BINDINGS; i++)
				{
					draw->pUniformBuffers[i] = nullptr;
				}
			}

			if(context->pixelShaderModel() <= 0x0104)
			{
				for(int stage = 0; stage < 8; stage++)
				{
					if(pixelState.textureStage[stage].stageOperation != TextureStage::STAGE_DISABLE || context->pixelShader)
					{
						data->textureStage[stage] = context->textureStage[stage].uniforms;
					}
					else break;
				}
			}

			if(context->vertexShader)
			{
				if(context->vertexShader->getShaderModel() >= 0x0300)
				{
					for(int sampler = 0; sampler < VERTEX_TEXTURE_IMAGE_UNITS; sampler++)
					{
						if(vertexState.sampler[sampler].textureType != TEXTURE_NULL)
						{
							draw->texture[TEXTURE_IMAGE_UNITS + sampler] = context->texture[TEXTURE_IMAGE_UNITS + sampler];
							draw->texture[TEXTURE_IMAGE_UNITS + sampler]->lock(PUBLIC, PRIVATE);

							data->mipmap[TEXTURE_IMAGE_UNITS + sampler] = context->sampler[TEXTURE_IMAGE_UNITS + sampler].getTextureData();
						}
					}
				}

				if(draw->vsDirtyConstF)
				{
					memcpy(&data->vs.c, VertexProcessor::c, sizeof(float4) * draw->vsDirtyConstF);
					draw->vsDirtyConstF = 0;
				}

				if(draw->vsDirtyConstI)
				{
					memcpy(&data->vs.i, VertexProcessor::i, sizeof(int4) * draw->vsDirtyConstI);
					draw->vsDirtyConstI = 0;
				}

				if(draw->vsDirtyConstB)
				{
					memcpy(&data->vs.b, VertexProcessor::b, sizeof(bool) * draw->vsDirtyConstB);
					draw->vsDirtyConstB = 0;
				}

				if(context->vertexShader->isInstanceIdDeclared())
				{
					data->instanceID = context->instanceID;
				}

				VertexProcessor::lockUniformBuffers(data->vs.u, draw->vUniformBuffers);
				VertexProcessor::lockTransformFeedbackBuffers(data->vs.t, data->vs.reg, data->vs.row, data->vs.col, data->vs.str, draw->transformFeedbackBuffers);
			}
			else
			{
				data->ff = ff;

				draw->vsDirtyConstF = VERTEX_UNIFORM_VECTORS + 1;
				draw->vsDirtyConstI = 16;
				draw->vsDirtyConstB = 16;

				for(int i = 0; i < MAX_UNIFORM_BUFFER_BINDINGS; i++)
				{
					draw->vUniformBuffers[i] = nullptr;
				}

				for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS; i++)
				{
					draw->transformFeedbackBuffers[i] = nullptr;
				}
			}

			if(pixelState.stencilActive)
			{
				data->stencil[0] = stencil;
				data->stencil[1] = stencilCCW;
			}

			if(pixelState.fogActive)
			{
				data->fog = fog;
			}

			if(setupState.isDrawPoint)
			{
				data->point = point;
			}

			data->lineWidth = context->lineWidth;

			data->factor = factor;

			if(pixelState.transparencyAntialiasing == TRANSPARENCY_ALPHA_TO_COVERAGE)
			{
				float ref = context->alphaReference * (1.0f / 255.0f);
				float margin = sw::min(ref, 1.0f - ref);

				if(ms == 4)
				{
					data->a2c0 = replicate(ref - margin * 0.6f);
					data->a2c1 = replicate(ref - margin * 0.2f);
					data->a2c2 = replicate(ref + margin * 0.2f);
					data->a2c3 = replicate(ref + margin * 0.6f);
				}
				else if(ms == 2)
				{
					data->a2c0 = replicate(ref - margin * 0.3f);
					data->a2c1 = replicate(ref + margin * 0.3f);
				}
				else ASSERT(false);
			}

			if(pixelState.occlusionEnabled)
			{
				for(int cluster = 0; cluster < clusterCount; cluster++)
				{
					data->occlusion[cluster] = 0;
				}
			}

			#if PERF_PROFILE
				for(int cluster = 0; cluster < clusterCount; cluster++)
				{
					for(int i = 0; i < PERF_TIMERS; i++)
					{
						data->cycles[i][cluster] = 0;
					}
				}
			#endif

			// Viewport
			{
				float W = 0.5f * viewport.width;
				float H = 0.5f * viewport.height;
				float X0 = viewport.x0 + W;
				float Y0 = viewport.y0 + H;
				float N = viewport.minZ;
				float F = viewport.maxZ;
				float Z = F - N;

				if(context->isDrawTriangle(false))
				{
					N += context->depthBias;
				}

				if(complementaryDepthBuffer)
				{
					Z = -Z;
					N = 1 - N;
				}

				static const float X[5][16] =   // Fragment offsets
				{
					{+0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 1 sample
					{-0.2500f, +0.2500f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 2 samples
					{-0.3000f, +0.1000f, +0.3000f, -0.1000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 4 samples
					{+0.1875f, -0.3125f, +0.3125f, -0.4375f, -0.0625f, +0.4375f, +0.0625f, -0.1875f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 8 samples
					{+0.2553f, -0.1155f, +0.1661f, -0.1828f, +0.2293f, -0.4132f, -0.1773f, -0.0577f, +0.3891f, -0.4656f, +0.4103f, +0.4248f, -0.2109f, +0.3966f, -0.2664f, -0.3872f}    // 16 samples
				};

				static const float Y[5][16] =   // Fragment offsets
				{
					{+0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 1 sample
					{-0.2500f, +0.2500f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 2 samples
					{-0.1000f, -0.3000f, +0.1000f, +0.3000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 4 samples
					{-0.4375f, -0.3125f, -0.1875f, -0.0625f, +0.0625f, +0.1875f, +0.3125f, +0.4375f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f, +0.0000f},   // 8 samples
					{-0.4503f, +0.1883f, +0.3684f, -0.4668f, -0.0690f, -0.1315f, +0.4999f, +0.0728f, +0.1070f, -0.3086f, +0.3725f, -0.1547f, -0.1102f, -0.3588f, +0.1789f, +0.0269f}    // 16 samples
				};

				int s = sw::log2(ss);

				data->Wx16 = replicate(W * 16);
				data->Hx16 = replicate(H * 16);
				data->X0x16 = replicate(X0 * 16 - 8);
				data->Y0x16 = replicate(Y0 * 16 - 8);
				data->XXXX = replicate(X[s][q] / W);
				data->YYYY = replicate(Y[s][q] / H);
				data->halfPixelX = replicate(0.5f / W);
				data->halfPixelY = replicate(0.5f / H);
				data->viewportHeight = abs(viewport.height);
				data->slopeDepthBias = context->slopeDepthBias;
				data->depthRange = Z;
				data->depthNear = N;
				draw->clipFlags = clipFlags;

				if(clipFlags)
				{
					if(clipFlags & Clipper::CLIP_PLANE0) data->clipPlane[0] = clipPlane[0];
					if(clipFlags & Clipper::CLIP_PLANE1) data->clipPlane[1] = clipPlane[1];
					if(clipFlags & Clipper::CLIP_PLANE2) data->clipPlane[2] = clipPlane[2];
					if(clipFlags & Clipper::CLIP_PLANE3) data->clipPlane[3] = clipPlane[3];
					if(clipFlags & Clipper::CLIP_PLANE4) data->clipPlane[4] = clipPlane[4];
					if(clipFlags & Clipper::CLIP_PLANE5) data->clipPlane[5] = clipPlane[5];
				}
			}

			// Target
			{
				for(int index = 0; index < RENDERTARGETS; index++)
				{
					draw->renderTarget[index] = context->renderTarget[index];

					if(draw->renderTarget[index])
					{
						unsigned int layer = context->renderTargetLayer[index];
						data->colorBuffer[index] = (unsigned int*)context->renderTarget[index]->lockInternal(0, 0, layer, LOCK_READWRITE, MANAGED);
						data->colorBuffer[index] += q * ms * context->renderTarget[index]->getSliceB(true);
						data->colorPitchB[index] = context->renderTarget[index]->getInternalPitchB();
						data->colorSliceB[index] = context->renderTarget[index]->getInternalSliceB();
					}
				}

				draw->depthBuffer = context->depthBuffer;
				draw->stencilBuffer = context->stencilBuffer;

				if(draw->depthBuffer)
				{
					unsigned int layer = context->depthBufferLayer;
					data->depthBuffer = (float*)context->depthBuffer->lockInternal(0, 0, layer, LOCK_READWRITE, MANAGED);
					data->depthBuffer += q * ms * context->depthBuffer->getSliceB(true);
					data->depthPitchB = context->depthBuffer->getInternalPitchB();
					data->depthSliceB = context->depthBuffer->getInternalSliceB();
				}

				if(draw->stencilBuffer)
				{
					unsigned int layer = context->stencilBufferLayer;
					data->stencilBuffer = (unsigned char*)context->stencilBuffer->lockStencil(0, 0, layer, MANAGED);
					data->stencilBuffer += q * ms * context->stencilBuffer->getSliceB(true);
					data->stencilPitchB = context->stencilBuffer->getStencilPitchB();
					data->stencilSliceB = context->stencilBuffer->getStencilSliceB();
				}
			}

			// Scissor
			{
				data->scissorX0 = scissor.x0;
				data->scissorX1 = scissor.x1;
				data->scissorY0 = scissor.y0;
				data->scissorY1 = scissor.y1;
			}

			draw->primitive = 0;
			draw->count = count;

			draw->references = (count + batch - 1) / batch;

			schedulerMutex.lock();
			++nextDraw; // Atomic
			schedulerMutex.unlock();

			#ifndef NDEBUG
			if(threadCount == 1)   // Use main thread for draw execution
			{
				threadsAwake = 1;
				task[0].type = Task::RESUME;

				taskLoop(0);
			}
			else
			#endif
			{
				if(!threadsAwake)
				{
					suspend[0]->wait();

					threadsAwake = 1;
					task[0].type = Task::RESUME;

					resume[0]->signal();
				}
			}
		}
	}

	void Renderer::clear(void *value, Format format, Surface *dest, const Rect &clearRect, unsigned int rgbaMask)
	{
		blitter->clear(value, format, dest, clearRect, rgbaMask);
	}

	void Renderer::blit(Surface *source, const SliceRectF &sRect, Surface *dest, const SliceRect &dRect, bool filter, bool isStencil, bool sRGBconversion)
	{
		blitter->blit(source, sRect, dest, dRect, {filter, isStencil, sRGBconversion});
	}

	void Renderer::blit3D(Surface *source, Surface *dest)
	{
		blitter->blit3D(source, dest);
	}

	void Renderer::threadFunction(void *parameters)
	{
		Renderer *renderer = static_cast<Parameters*>(parameters)->renderer;
		int threadIndex = static_cast<Parameters*>(parameters)->threadIndex;

		if(logPrecision < IEEE)
		{
			CPUID::setFlushToZero(true);
			CPUID::setDenormalsAreZero(true);
		}

		renderer->threadLoop(threadIndex);
	}

	void Renderer::threadLoop(int threadIndex)
	{
		while(!exitThreads)
		{
			taskLoop(threadIndex);

			suspend[threadIndex]->signal();
			resume[threadIndex]->wait();
		}
	}

	void Renderer::taskLoop(int threadIndex)
	{
		while(task[threadIndex].type != Task::SUSPEND)
		{
			scheduleTask(threadIndex);
			executeTask(threadIndex);
		}
	}

	void Renderer::findAvailableTasks()
	{
		// Find pixel tasks
		for(int cluster = 0; cluster < clusterCount; cluster++)
		{
			if(!pixelProgress[cluster].executing)
			{
				for(int unit = 0; unit < unitCount; unit++)
				{
					if(primitiveProgress[unit].references > 0)   // Contains processed primitives
					{
						if(pixelProgress[cluster].drawCall == primitiveProgress[unit].drawCall)
						{
							if(pixelProgress[cluster].processedPrimitives == primitiveProgress[unit].firstPrimitive)   // Previous primitives have been rendered
							{
								Task &task = taskQueue[qHead];
								task.type = Task::PIXELS;
								task.primitiveUnit = unit;
								task.pixelCluster = cluster;

								pixelProgress[cluster].executing = true;

								// Commit to the task queue
								qHead = (qHead + 1) & TASK_COUNT_BITS;
								qSize++;

								break;
							}
						}
					}
				}
			}
		}

		// Find primitive tasks
		if(currentDraw == nextDraw)
		{
			return;   // No more primitives to process
		}

		for(int unit = 0; unit < unitCount; unit++)
		{
			DrawCall *draw = drawList[currentDraw & DRAW_COUNT_BITS];

			int primitive = draw->primitive;
			int count = draw->count;

			if(primitive >= count)
			{
				++currentDraw; // Atomic

				if(currentDraw == nextDraw)
				{
					return;   // No more primitives to process
				}

				draw = drawList[currentDraw & DRAW_COUNT_BITS];
			}

			if(!primitiveProgress[unit].references)   // Task not already being executed and not still in use by a pixel unit
			{
				primitive = draw->primitive;
				count = draw->count;
				int batch = draw->batchSize;

				primitiveProgress[unit].drawCall = currentDraw;
				primitiveProgress[unit].firstPrimitive = primitive;
				primitiveProgress[unit].primitiveCount = count - primitive >= batch ? batch : count - primitive;

				draw->primitive += batch;

				Task &task = taskQueue[qHead];
				task.type = Task::PRIMITIVES;
				task.primitiveUnit = unit;

				primitiveProgress[unit].references = -1;

				// Commit to the task queue
				qHead = (qHead + 1) & TASK_COUNT_BITS;
				qSize++;
			}
		}
	}

	void Renderer::scheduleTask(int threadIndex)
	{
		schedulerMutex.lock();

		int curThreadsAwake = threadsAwake;

		if((int)qSize < threadCount - curThreadsAwake + 1)
		{
			findAvailableTasks();
		}

		if(qSize != 0)
		{
			task[threadIndex] = taskQueue[(qHead - qSize) & TASK_COUNT_BITS];
			qSize--;

			if(curThreadsAwake != threadCount)
			{
				int wakeup = qSize - curThreadsAwake + 1;

				for(int i = 0; i < threadCount && wakeup > 0; i++)
				{
					if(task[i].type == Task::SUSPEND)
					{
						suspend[i]->wait();
						task[i].type = Task::RESUME;
						resume[i]->signal();

						++threadsAwake; // Atomic
						wakeup--;
					}
				}
			}
		}
		else
		{
			task[threadIndex].type = Task::SUSPEND;

			--threadsAwake; // Atomic
		}

		schedulerMutex.unlock();
	}

	void Renderer::executeTask(int threadIndex)
	{
		#if PERF_HUD
			int64_t startTick = Timer::ticks();
		#endif

		switch(task[threadIndex].type)
		{
		case Task::PRIMITIVES:
			{
				int unit = task[threadIndex].primitiveUnit;

				int input = primitiveProgress[unit].firstPrimitive;
				int count = primitiveProgress[unit].primitiveCount;
				DrawCall *draw = drawList[primitiveProgress[unit].drawCall & DRAW_COUNT_BITS];
				int (Renderer::*setupPrimitives)(int batch, int count) = draw->setupPrimitives;

				processPrimitiveVertices(unit, input, count, draw->count, threadIndex);

				#if PERF_HUD
					int64_t time = Timer::ticks();
					vertexTime[threadIndex] += time - startTick;
					startTick = time;
				#endif

				int visible = 0;

				if(!draw->setupState.rasterizerDiscard)
				{
					visible = (this->*setupPrimitives)(unit, count);
				}

				primitiveProgress[unit].visible = visible;
				primitiveProgress[unit].references = clusterCount;

				#if PERF_HUD
					setupTime[threadIndex] += Timer::ticks() - startTick;
				#endif
			}
			break;
		case Task::PIXELS:
			{
				int unit = task[threadIndex].primitiveUnit;
				int visible = primitiveProgress[unit].visible;

				if(visible > 0)
				{
					int cluster = task[threadIndex].pixelCluster;
					Primitive *primitive = primitiveBatch[unit];
					DrawCall *draw = drawList[pixelProgress[cluster].drawCall & DRAW_COUNT_BITS];
					DrawData *data = draw->data;
					PixelProcessor::RoutinePointer pixelRoutine = draw->pixelPointer;

					pixelRoutine(primitive, visible, cluster, data);
				}

				finishRendering(task[threadIndex]);

				#if PERF_HUD
					pixelTime[threadIndex] += Timer::ticks() - startTick;
				#endif
			}
			break;
		case Task::RESUME:
			break;
		case Task::SUSPEND:
			break;
		default:
			ASSERT(false);
		}
	}

	void Renderer::synchronize()
	{
		sync->lock(sw::PUBLIC);
		sync->unlock();
	}

	void Renderer::finishRendering(Task &pixelTask)
	{
		int unit = pixelTask.primitiveUnit;
		int cluster = pixelTask.pixelCluster;

		DrawCall &draw = *drawList[primitiveProgress[unit].drawCall & DRAW_COUNT_BITS];
		DrawData &data = *draw.data;
		int primitive = primitiveProgress[unit].firstPrimitive;
		int count = primitiveProgress[unit].primitiveCount;
		int processedPrimitives = primitive + count;

		pixelProgress[cluster].processedPrimitives = processedPrimitives;

		if(pixelProgress[cluster].processedPrimitives >= draw.count)
		{
			++pixelProgress[cluster].drawCall; // Atomic
			pixelProgress[cluster].processedPrimitives = 0;
		}

		int ref = primitiveProgress[unit].references--; // Atomic

		if(ref == 0)
		{
			ref = draw.references--; // Atomic

			if(ref == 0)
			{
				#if PERF_PROFILE
					for(int cluster = 0; cluster < clusterCount; cluster++)
					{
						for(int i = 0; i < PERF_TIMERS; i++)
						{
							profiler.cycles[i] += data.cycles[i][cluster];
						}
					}
				#endif

				if(draw.queries)
				{
					for(auto &query : *(draw.queries))
					{
						switch(query->type)
						{
						case Query::FRAGMENTS_PASSED:
							for(int cluster = 0; cluster < clusterCount; cluster++)
							{
								query->data += data.occlusion[cluster];
							}
							break;
						case Query::TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
							query->data += processedPrimitives;
							break;
						default:
							break;
						}

						--query->reference; // Atomic
					}

					delete draw.queries;
					draw.queries = 0;
				}

				for(int i = 0; i < RENDERTARGETS; i++)
				{
					if(draw.renderTarget[i])
					{
						draw.renderTarget[i]->unlockInternal();
					}
				}

				if(draw.depthBuffer)
				{
					draw.depthBuffer->unlockInternal();
				}

				if(draw.stencilBuffer)
				{
					draw.stencilBuffer->unlockStencil();
				}

				for(int i = 0; i < TOTAL_IMAGE_UNITS; i++)
				{
					if(draw.texture[i])
					{
						draw.texture[i]->unlock();
					}
				}

				for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
				{
					if(draw.vertexStream[i])
					{
						draw.vertexStream[i]->unlock();
					}
				}

				if(draw.indexBuffer)
				{
					draw.indexBuffer->unlock();
				}

				for(int i = 0; i < MAX_UNIFORM_BUFFER_BINDINGS; i++)
				{
					if(draw.pUniformBuffers[i])
					{
						draw.pUniformBuffers[i]->unlock();
					}
					if(draw.vUniformBuffers[i])
					{
						draw.vUniformBuffers[i]->unlock();
					}
				}

				for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS; i++)
				{
					if(draw.transformFeedbackBuffers[i])
					{
						draw.transformFeedbackBuffers[i]->unlock();
					}
				}

				draw.vertexRoutine->unbind();
				draw.setupRoutine->unbind();
				draw.pixelRoutine->unbind();

				sync->unlock();

				draw.references = -1;
				resumeApp->signal();
			}
		}

		pixelProgress[cluster].executing = false;
	}

	void Renderer::processPrimitiveVertices(int unit, unsigned int start, unsigned int triangleCount, unsigned int loop, int thread)
	{
		Triangle *triangle = triangleBatch[unit];
		int primitiveDrawCall = primitiveProgress[unit].drawCall;
		DrawCall *draw = drawList[primitiveDrawCall & DRAW_COUNT_BITS];
		DrawData *data = draw->data;
		VertexTask *task = vertexTask[thread];

		const void *indices = data->indices;
		VertexProcessor::RoutinePointer vertexRoutine = draw->vertexPointer;

		if(task->vertexCache.drawCall != primitiveDrawCall)
		{
			task->vertexCache.clear();
			task->vertexCache.drawCall = primitiveDrawCall;
		}

		unsigned int batch[128][3];   // FIXME: Adjust to dynamic batch size

		switch(draw->drawType)
		{
		case DRAW_POINTLIST:
			{
				unsigned int index = start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index;
					batch[i][1] = index;
					batch[i][2] = index;

					index += 1;
				}
			}
			break;
		case DRAW_LINELIST:
			{
				unsigned int index = 2 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index + 0;
					batch[i][1] = index + 1;
					batch[i][2] = index + 1;

					index += 2;
				}
			}
			break;
		case DRAW_LINESTRIP:
			{
				unsigned int index = start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index + 0;
					batch[i][1] = index + 1;
					batch[i][2] = index + 1;

					index += 1;
				}
			}
			break;
		case DRAW_LINELOOP:
			{
				unsigned int index = start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = (index + 0) % loop;
					batch[i][1] = (index + 1) % loop;
					batch[i][2] = (index + 1) % loop;

					index += 1;
				}
			}
			break;
		case DRAW_TRIANGLELIST:
			{
				unsigned int index = 3 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index + 0;
					batch[i][1] = index + 1;
					batch[i][2] = index + 2;

					index += 3;
				}
			}
			break;
		case DRAW_TRIANGLESTRIP:
			{
				unsigned int index = start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					if(leadingVertexFirst)
					{
						batch[i][0] = index + 0;
						batch[i][1] = index + (index & 1) + 1;
						batch[i][2] = index + (~index & 1) + 1;
					}
					else
					{
						batch[i][0] = index + (index & 1);
						batch[i][1] = index + (~index & 1);
						batch[i][2] = index + 2;
					}

					index += 1;
				}
			}
			break;
		case DRAW_TRIANGLEFAN:
			{
				unsigned int index = start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					if(leadingVertexFirst)
					{
						batch[i][0] = index + 1;
						batch[i][1] = index + 2;
						batch[i][2] = 0;
					}
					else
					{
						batch[i][0] = 0;
						batch[i][1] = index + 1;
						batch[i][2] = index + 2;
					}

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDPOINTLIST8:
			{
				const unsigned char *index = (const unsigned char*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = *index;
					batch[i][1] = *index;
					batch[i][2] = *index;

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDPOINTLIST16:
			{
				const unsigned short *index = (const unsigned short*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = *index;
					batch[i][1] = *index;
					batch[i][2] = *index;

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDPOINTLIST32:
			{
				const unsigned int *index = (const unsigned int*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = *index;
					batch[i][1] = *index;
					batch[i][2] = *index;

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDLINELIST8:
			{
				const unsigned char *index = (const unsigned char*)indices + 2 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[1];

					index += 2;
				}
			}
			break;
		case DRAW_INDEXEDLINELIST16:
			{
				const unsigned short *index = (const unsigned short*)indices + 2 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[1];

					index += 2;
				}
			}
			break;
		case DRAW_INDEXEDLINELIST32:
			{
				const unsigned int *index = (const unsigned int*)indices + 2 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[1];

					index += 2;
				}
			}
			break;
		case DRAW_INDEXEDLINESTRIP8:
			{
				const unsigned char *index = (const unsigned char*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[1];

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDLINESTRIP16:
			{
				const unsigned short *index = (const unsigned short*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[1];

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDLINESTRIP32:
			{
				const unsigned int *index = (const unsigned int*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[1];

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDLINELOOP8:
			{
				const unsigned char *index = (const unsigned char*)indices;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[(start + i + 0) % loop];
					batch[i][1] = index[(start + i + 1) % loop];
					batch[i][2] = index[(start + i + 1) % loop];
				}
			}
			break;
		case DRAW_INDEXEDLINELOOP16:
			{
				const unsigned short *index = (const unsigned short*)indices;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[(start + i + 0) % loop];
					batch[i][1] = index[(start + i + 1) % loop];
					batch[i][2] = index[(start + i + 1) % loop];
				}
			}
			break;
		case DRAW_INDEXEDLINELOOP32:
			{
				const unsigned int *index = (const unsigned int*)indices;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[(start + i + 0) % loop];
					batch[i][1] = index[(start + i + 1) % loop];
					batch[i][2] = index[(start + i + 1) % loop];
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLELIST8:
			{
				const unsigned char *index = (const unsigned char*)indices + 3 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[2];

					index += 3;
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLELIST16:
			{
				const unsigned short *index = (const unsigned short*)indices + 3 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[2];

					index += 3;
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLELIST32:
			{
				const unsigned int *index = (const unsigned int*)indices + 3 * start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[1];
					batch[i][2] = index[2];

					index += 3;
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLESTRIP8:
			{
				const unsigned char *index = (const unsigned char*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[((start + i) & 1) + 1];
					batch[i][2] = index[(~(start + i) & 1) + 1];

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLESTRIP16:
			{
				const unsigned short *index = (const unsigned short*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[((start + i) & 1) + 1];
					batch[i][2] = index[(~(start + i) & 1) + 1];

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLESTRIP32:
			{
				const unsigned int *index = (const unsigned int*)indices + start;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[0];
					batch[i][1] = index[((start + i) & 1) + 1];
					batch[i][2] = index[(~(start + i) & 1) + 1];

					index += 1;
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLEFAN8:
			{
				const unsigned char *index = (const unsigned char*)indices;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[start + i + 1];
					batch[i][1] = index[start + i + 2];
					batch[i][2] = index[0];
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLEFAN16:
			{
				const unsigned short *index = (const unsigned short*)indices;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[start + i + 1];
					batch[i][1] = index[start + i + 2];
					batch[i][2] = index[0];
				}
			}
			break;
		case DRAW_INDEXEDTRIANGLEFAN32:
			{
				const unsigned int *index = (const unsigned int*)indices;

				for(unsigned int i = 0; i < triangleCount; i++)
				{
					batch[i][0] = index[start + i + 1];
					batch[i][1] = index[start + i + 2];
					batch[i][2] = index[0];
				}
			}
			break;
		case DRAW_QUADLIST:
			{
				unsigned int index = 4 * start / 2;

				for(unsigned int i = 0; i < triangleCount; i += 2)
				{
					batch[i+0][0] = index + 0;
					batch[i+0][1] = index + 1;
					batch[i+0][2] = index + 2;

					batch[i+1][0] = index + 0;
					batch[i+1][1] = index + 2;
					batch[i+1][2] = index + 3;

					index += 4;
				}
			}
			break;
		default:
			ASSERT(false);
			return;
		}

		task->primitiveStart = start;
		task->vertexCount = triangleCount * 3;
		vertexRoutine(&triangle->v0, (unsigned int*)&batch, task, data);
	}

	int Renderer::setupSolidTriangles(int unit, int count)
	{
		Triangle *triangle = triangleBatch[unit];
		Primitive *primitive = primitiveBatch[unit];

		DrawCall &draw = *drawList[primitiveProgress[unit].drawCall & DRAW_COUNT_BITS];
		SetupProcessor::State &state = draw.setupState;
		const SetupProcessor::RoutinePointer &setupRoutine = draw.setupPointer;

		int ms = state.multiSample;
		int pos = state.positionRegister;
		const DrawData *data = draw.data;
		int visible = 0;

		for(int i = 0; i < count; i++, triangle++)
		{
			Vertex &v0 = triangle->v0;
			Vertex &v1 = triangle->v1;
			Vertex &v2 = triangle->v2;

			if((v0.clipFlags & v1.clipFlags & v2.clipFlags) == Clipper::CLIP_FINITE)
			{
				Polygon polygon(&v0.v[pos], &v1.v[pos], &v2.v[pos]);

				int clipFlagsOr = v0.clipFlags | v1.clipFlags | v2.clipFlags | draw.clipFlags;

				if(clipFlagsOr != Clipper::CLIP_FINITE)
				{
					if(!clipper->clip(polygon, clipFlagsOr, draw))
					{
						continue;
					}
				}

				if(setupRoutine(primitive, triangle, &polygon, data))
				{
					primitive += ms;
					visible++;
				}
			}
		}

		return visible;
	}

	int Renderer::setupWireframeTriangle(int unit, int count)
	{
		Triangle *triangle = triangleBatch[unit];
		Primitive *primitive = primitiveBatch[unit];
		int visible = 0;

		DrawCall &draw = *drawList[primitiveProgress[unit].drawCall & DRAW_COUNT_BITS];
		SetupProcessor::State &state = draw.setupState;

		const Vertex &v0 = triangle[0].v0;
		const Vertex &v1 = triangle[0].v1;
		const Vertex &v2 = triangle[0].v2;

		float d = (v0.y * v1.x - v0.x * v1.y) * v2.w + (v0.x * v2.y - v0.y * v2.x) * v1.w + (v2.x * v1.y - v1.x * v2.y) * v0.w;

		if(state.cullMode == CULL_CLOCKWISE)
		{
			if(d >= 0) return 0;
		}
		else if(state.cullMode == CULL_COUNTERCLOCKWISE)
		{
			if(d <= 0) return 0;
		}

		// Copy attributes
		triangle[1].v0 = v1;
		triangle[1].v1 = v2;
		triangle[2].v0 = v2;
		triangle[2].v1 = v0;

		if(state.color[0][0].flat)   // FIXME
		{
			for(int i = 0; i < 2; i++)
			{
				triangle[1].v0.C[i] = triangle[0].v0.C[i];
				triangle[1].v1.C[i] = triangle[0].v0.C[i];
				triangle[2].v0.C[i] = triangle[0].v0.C[i];
				triangle[2].v1.C[i] = triangle[0].v0.C[i];
			}
		}

		for(int i = 0; i < 3; i++)
		{
			if(setupLine(*primitive, *triangle, draw))
			{
				primitive->area = 0.5f * d;

				primitive++;
				visible++;
			}

			triangle++;
		}

		return visible;
	}

	int Renderer::setupVertexTriangle(int unit, int count)
	{
		Triangle *triangle = triangleBatch[unit];
		Primitive *primitive = primitiveBatch[unit];
		int visible = 0;

		DrawCall &draw = *drawList[primitiveProgress[unit].drawCall & DRAW_COUNT_BITS];
		SetupProcessor::State &state = draw.setupState;

		const Vertex &v0 = triangle[0].v0;
		const Vertex &v1 = triangle[0].v1;
		const Vertex &v2 = triangle[0].v2;

		float d = (v0.y * v1.x - v0.x * v1.y) * v2.w + (v0.x * v2.y - v0.y * v2.x) * v1.w + (v2.x * v1.y - v1.x * v2.y) * v0.w;

		if(state.cullMode == CULL_CLOCKWISE)
		{
			if(d >= 0) return 0;
		}
		else if(state.cullMode == CULL_COUNTERCLOCKWISE)
		{
			if(d <= 0) return 0;
		}

		// Copy attributes
		triangle[1].v0 = v1;
		triangle[2].v0 = v2;

		for(int i = 0; i < 3; i++)
		{
			if(setupPoint(*primitive, *triangle, draw))
			{
				primitive->area = 0.5f * d;

				primitive++;
				visible++;
			}

			triangle++;
		}

		return visible;
	}

	int Renderer::setupLines(int unit, int count)
	{
		Triangle *triangle = triangleBatch[unit];
		Primitive *primitive = primitiveBatch[unit];
		int visible = 0;

		DrawCall &draw = *drawList[primitiveProgress[unit].drawCall & DRAW_COUNT_BITS];
		SetupProcessor::State &state = draw.setupState;

		int ms = state.multiSample;

		for(int i = 0; i < count; i++)
		{
			if(setupLine(*primitive, *triangle, draw))
			{
				primitive += ms;
				visible++;
			}

			triangle++;
		}

		return visible;
	}

	int Renderer::setupPoints(int unit, int count)
	{
		Triangle *triangle = triangleBatch[unit];
		Primitive *primitive = primitiveBatch[unit];
		int visible = 0;

		DrawCall &draw = *drawList[primitiveProgress[unit].drawCall & DRAW_COUNT_BITS];
		SetupProcessor::State &state = draw.setupState;

		int ms = state.multiSample;

		for(int i = 0; i < count; i++)
		{
			if(setupPoint(*primitive, *triangle, draw))
			{
				primitive += ms;
				visible++;
			}

			triangle++;
		}

		return visible;
	}

	bool Renderer::setupLine(Primitive &primitive, Triangle &triangle, const DrawCall &draw)
	{
		const SetupProcessor::RoutinePointer &setupRoutine = draw.setupPointer;
		const SetupProcessor::State &state = draw.setupState;
		const DrawData &data = *draw.data;

		float lineWidth = data.lineWidth;

		Vertex &v0 = triangle.v0;
		Vertex &v1 = triangle.v1;

		int pos = state.positionRegister;

		const float4 &P0 = v0.v[pos];
		const float4 &P1 = v1.v[pos];

		if(P0.w <= 0 && P1.w <= 0)
		{
			return false;
		}

		const float W = data.Wx16[0] * (1.0f / 16.0f);
		const float H = data.Hx16[0] * (1.0f / 16.0f);

		float dx = W * (P1.x / P1.w - P0.x / P0.w);
		float dy = H * (P1.y / P1.w - P0.y / P0.w);

		if(dx == 0 && dy == 0)
		{
			return false;
		}

		if(state.multiSample > 1)   // Rectangle
		{
			float4 P[4];
			int C[4];

			P[0] = P0;
			P[1] = P1;
			P[2] = P1;
			P[3] = P0;

			float scale = lineWidth * 0.5f / sqrt(dx*dx + dy*dy);

			dx *= scale;
			dy *= scale;

			float dx0h = dx * P0.w / H;
			float dy0w = dy * P0.w / W;

			float dx1h = dx * P1.w / H;
			float dy1w = dy * P1.w / W;

			P[0].x += -dy0w;
			P[0].y += +dx0h;
			C[0] = clipper->computeClipFlags(P[0]);

			P[1].x += -dy1w;
			P[1].y += +dx1h;
			C[1] = clipper->computeClipFlags(P[1]);

			P[2].x += +dy1w;
			P[2].y += -dx1h;
			C[2] = clipper->computeClipFlags(P[2]);

			P[3].x += +dy0w;
			P[3].y += -dx0h;
			C[3] = clipper->computeClipFlags(P[3]);

			if((C[0] & C[1] & C[2] & C[3]) == Clipper::CLIP_FINITE)
			{
				Polygon polygon(P, 4);

				int clipFlagsOr = C[0] | C[1] | C[2] | C[3] | draw.clipFlags;

				if(clipFlagsOr != Clipper::CLIP_FINITE)
				{
					if(!clipper->clip(polygon, clipFlagsOr, draw))
					{
						return false;
					}
				}

				return setupRoutine(&primitive, &triangle, &polygon, &data);
			}
		}
		else   // Diamond test convention
		{
			float4 P[8];
			int C[8];

			P[0] = P0;
			P[1] = P0;
			P[2] = P0;
			P[3] = P0;
			P[4] = P1;
			P[5] = P1;
			P[6] = P1;
			P[7] = P1;

			float dx0 = lineWidth * 0.5f * P0.w / W;
			float dy0 = lineWidth * 0.5f * P0.w / H;

			float dx1 = lineWidth * 0.5f * P1.w / W;
			float dy1 = lineWidth * 0.5f * P1.w / H;

			P[0].x += -dx0;
			C[0] = clipper->computeClipFlags(P[0]);

			P[1].y += +dy0;
			C[1] = clipper->computeClipFlags(P[1]);

			P[2].x += +dx0;
			C[2] = clipper->computeClipFlags(P[2]);

			P[3].y += -dy0;
			C[3] = clipper->computeClipFlags(P[3]);

			P[4].x += -dx1;
			C[4] = clipper->computeClipFlags(P[4]);

			P[5].y += +dy1;
			C[5] = clipper->computeClipFlags(P[5]);

			P[6].x += +dx1;
			C[6] = clipper->computeClipFlags(P[6]);

			P[7].y += -dy1;
			C[7] = clipper->computeClipFlags(P[7]);

			if((C[0] & C[1] & C[2] & C[3] & C[4] & C[5] & C[6] & C[7]) == Clipper::CLIP_FINITE)
			{
				float4 L[6];

				if(dx > -dy)
				{
					if(dx > dy)   // Right
					{
						L[0] = P[0];
						L[1] = P[1];
						L[2] = P[5];
						L[3] = P[6];
						L[4] = P[7];
						L[5] = P[3];
					}
					else   // Down
					{
						L[0] = P[0];
						L[1] = P[4];
						L[2] = P[5];
						L[3] = P[6];
						L[4] = P[2];
						L[5] = P[3];
					}
				}
				else
				{
					if(dx > dy)   // Up
					{
						L[0] = P[0];
						L[1] = P[1];
						L[2] = P[2];
						L[3] = P[6];
						L[4] = P[7];
						L[5] = P[4];
					}
					else   // Left
					{
						L[0] = P[1];
						L[1] = P[2];
						L[2] = P[3];
						L[3] = P[7];
						L[4] = P[4];
						L[5] = P[5];
					}
				}

				Polygon polygon(L, 6);

				int clipFlagsOr = C[0] | C[1] | C[2] | C[3] | C[4] | C[5] | C[6] | C[7] | draw.clipFlags;

				if(clipFlagsOr != Clipper::CLIP_FINITE)
				{
					if(!clipper->clip(polygon, clipFlagsOr, draw))
					{
						return false;
					}
				}

				return setupRoutine(&primitive, &triangle, &polygon, &data);
			}
		}

		return false;
	}

	bool Renderer::setupPoint(Primitive &primitive, Triangle &triangle, const DrawCall &draw)
	{
		const SetupProcessor::RoutinePointer &setupRoutine = draw.setupPointer;
		const SetupProcessor::State &state = draw.setupState;
		const DrawData &data = *draw.data;

		Vertex &v = triangle.v0;

		float pSize;

		int pts = state.pointSizeRegister;

		if(state.pointSizeRegister != Unused)
		{
			pSize = v.v[pts].y;
		}
		else
		{
			pSize = data.point.pointSize[0];
		}

		pSize = clamp(pSize, data.point.pointSizeMin, data.point.pointSizeMax);

		float4 P[4];
		int C[4];

		int pos = state.positionRegister;

		P[0] = v.v[pos];
		P[1] = v.v[pos];
		P[2] = v.v[pos];
		P[3] = v.v[pos];

		const float X = pSize * P[0].w * data.halfPixelX[0];
		const float Y = pSize * P[0].w * data.halfPixelY[0];

		P[0].x -= X;
		P[0].y += Y;
		C[0] = clipper->computeClipFlags(P[0]);

		P[1].x += X;
		P[1].y += Y;
		C[1] = clipper->computeClipFlags(P[1]);

		P[2].x += X;
		P[2].y -= Y;
		C[2] = clipper->computeClipFlags(P[2]);

		P[3].x -= X;
		P[3].y -= Y;
		C[3] = clipper->computeClipFlags(P[3]);

		triangle.v1 = triangle.v0;
		triangle.v2 = triangle.v0;

		triangle.v1.X += iround(16 * 0.5f * pSize);
		triangle.v2.Y -= iround(16 * 0.5f * pSize) * (data.Hx16[0] > 0.0f ? 1 : -1);   // Both Direct3D and OpenGL expect (0, 0) in the top-left corner

		Polygon polygon(P, 4);

		if((C[0] & C[1] & C[2] & C[3]) == Clipper::CLIP_FINITE)
		{
			int clipFlagsOr = C[0] | C[1] | C[2] | C[3] | draw.clipFlags;

			if(clipFlagsOr != Clipper::CLIP_FINITE)
			{
				if(!clipper->clip(polygon, clipFlagsOr, draw))
				{
					return false;
				}
			}

			return setupRoutine(&primitive, &triangle, &polygon, &data);
		}

		return false;
	}

	void Renderer::initializeThreads()
	{
		unitCount = ceilPow2(threadCount);
		clusterCount = ceilPow2(threadCount);

		for(int i = 0; i < unitCount; i++)
		{
			triangleBatch[i] = (Triangle*)allocate(batchSize * sizeof(Triangle));
			primitiveBatch[i] = (Primitive*)allocate(batchSize * sizeof(Primitive));
		}

		for(int i = 0; i < threadCount; i++)
		{
			vertexTask[i] = (VertexTask*)allocate(sizeof(VertexTask));
			vertexTask[i]->vertexCache.drawCall = -1;

			task[i].type = Task::SUSPEND;

			resume[i] = new Event();
			suspend[i] = new Event();

			Parameters parameters;
			parameters.threadIndex = i;
			parameters.renderer = this;

			exitThreads = false;
			worker[i] = new Thread(threadFunction, &parameters);

			suspend[i]->wait();
			suspend[i]->signal();
		}
	}

	void Renderer::terminateThreads()
	{
		while(threadsAwake != 0)
		{
			Thread::sleep(1);
		}

		for(int thread = 0; thread < threadCount; thread++)
		{
			if(worker[thread])
			{
				exitThreads = true;
				resume[thread]->signal();
				worker[thread]->join();

				delete worker[thread];
				worker[thread] = 0;
				delete resume[thread];
				resume[thread] = 0;
				delete suspend[thread];
				suspend[thread] = 0;
			}

			deallocate(vertexTask[thread]);
			vertexTask[thread] = 0;
		}

		for(int i = 0; i < 16; i++)
		{
			deallocate(triangleBatch[i]);
			triangleBatch[i] = 0;

			deallocate(primitiveBatch[i]);
			primitiveBatch[i] = 0;
		}
	}

	void Renderer::loadConstants(const VertexShader *vertexShader)
	{
		if(!vertexShader) return;

		size_t count = vertexShader->getLength();

		for(size_t i = 0; i < count; i++)
		{
			const Shader::Instruction *instruction = vertexShader->getInstruction(i);

			if(instruction->opcode == Shader::OPCODE_DEF)
			{
				int index = instruction->dst.index;
				float value[4];

				value[0] = instruction->src[0].value[0];
				value[1] = instruction->src[0].value[1];
				value[2] = instruction->src[0].value[2];
				value[3] = instruction->src[0].value[3];

				setVertexShaderConstantF(index, value);
			}
			else if(instruction->opcode == Shader::OPCODE_DEFI)
			{
				int index = instruction->dst.index;
				int integer[4];

				integer[0] = instruction->src[0].integer[0];
				integer[1] = instruction->src[0].integer[1];
				integer[2] = instruction->src[0].integer[2];
				integer[3] = instruction->src[0].integer[3];

				setVertexShaderConstantI(index, integer);
			}
			else if(instruction->opcode == Shader::OPCODE_DEFB)
			{
				int index = instruction->dst.index;
				int boolean = instruction->src[0].boolean[0];

				setVertexShaderConstantB(index, &boolean);
			}
		}
	}

	void Renderer::loadConstants(const PixelShader *pixelShader)
	{
		if(!pixelShader) return;

		size_t count = pixelShader->getLength();

		for(size_t i = 0; i < count; i++)
		{
			const Shader::Instruction *instruction = pixelShader->getInstruction(i);

			if(instruction->opcode == Shader::OPCODE_DEF)
			{
				int index = instruction->dst.index;
				float value[4];

				value[0] = instruction->src[0].value[0];
				value[1] = instruction->src[0].value[1];
				value[2] = instruction->src[0].value[2];
				value[3] = instruction->src[0].value[3];

				setPixelShaderConstantF(index, value);
			}
			else if(instruction->opcode == Shader::OPCODE_DEFI)
			{
				int index = instruction->dst.index;
				int integer[4];

				integer[0] = instruction->src[0].integer[0];
				integer[1] = instruction->src[0].integer[1];
				integer[2] = instruction->src[0].integer[2];
				integer[3] = instruction->src[0].integer[3];

				setPixelShaderConstantI(index, integer);
			}
			else if(instruction->opcode == Shader::OPCODE_DEFB)
			{
				int index = instruction->dst.index;
				int boolean = instruction->src[0].boolean[0];

				setPixelShaderConstantB(index, &boolean);
			}
		}
	}

	void Renderer::setIndexBuffer(Resource *indexBuffer)
	{
		context->indexBuffer = indexBuffer;
	}

	void Renderer::setMultiSampleMask(unsigned int mask)
	{
		context->sampleMask = mask;
	}

	void Renderer::setTransparencyAntialiasing(TransparencyAntialiasing transparencyAntialiasing)
	{
		sw::transparencyAntialiasing = transparencyAntialiasing;
	}

	bool Renderer::isReadWriteTexture(int sampler)
	{
		for(int index = 0; index < RENDERTARGETS; index++)
		{
			if(context->renderTarget[index] && context->texture[sampler] == context->renderTarget[index]->getResource())
			{
				return true;
			}
		}

		if(context->depthBuffer && context->texture[sampler] == context->depthBuffer->getResource())
		{
			return true;
		}

		return false;
	}

	void Renderer::updateClipper()
	{
		if(updateClipPlanes)
		{
			if(VertexProcessor::isFixedFunction())   // User plane in world space
			{
				const Matrix &scissorWorld = getViewTransform();

				if(clipFlags & Clipper::CLIP_PLANE0) clipPlane[0] = scissorWorld * userPlane[0];
				if(clipFlags & Clipper::CLIP_PLANE1) clipPlane[1] = scissorWorld * userPlane[1];
				if(clipFlags & Clipper::CLIP_PLANE2) clipPlane[2] = scissorWorld * userPlane[2];
				if(clipFlags & Clipper::CLIP_PLANE3) clipPlane[3] = scissorWorld * userPlane[3];
				if(clipFlags & Clipper::CLIP_PLANE4) clipPlane[4] = scissorWorld * userPlane[4];
				if(clipFlags & Clipper::CLIP_PLANE5) clipPlane[5] = scissorWorld * userPlane[5];
			}
			else   // User plane in clip space
			{
				if(clipFlags & Clipper::CLIP_PLANE0) clipPlane[0] = userPlane[0];
				if(clipFlags & Clipper::CLIP_PLANE1) clipPlane[1] = userPlane[1];
				if(clipFlags & Clipper::CLIP_PLANE2) clipPlane[2] = userPlane[2];
				if(clipFlags & Clipper::CLIP_PLANE3) clipPlane[3] = userPlane[3];
				if(clipFlags & Clipper::CLIP_PLANE4) clipPlane[4] = userPlane[4];
				if(clipFlags & Clipper::CLIP_PLANE5) clipPlane[5] = userPlane[5];
			}

			updateClipPlanes = false;
		}
	}

	void Renderer::setTextureResource(unsigned int sampler, Resource *resource)
	{
		ASSERT(sampler < TOTAL_IMAGE_UNITS);

		context->texture[sampler] = resource;
	}

	void Renderer::setTextureLevel(unsigned int sampler, unsigned int face, unsigned int level, Surface *surface, TextureType type)
	{
		ASSERT(sampler < TOTAL_IMAGE_UNITS && face < 6 && level < MIPMAP_LEVELS);

		context->sampler[sampler].setTextureLevel(face, level, surface, type);
	}

	void Renderer::setTextureFilter(SamplerType type, int sampler, FilterType textureFilter)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setTextureFilter(sampler, textureFilter);
		}
		else
		{
			VertexProcessor::setTextureFilter(sampler, textureFilter);
		}
	}

	void Renderer::setMipmapFilter(SamplerType type, int sampler, MipmapType mipmapFilter)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setMipmapFilter(sampler, mipmapFilter);
		}
		else
		{
			VertexProcessor::setMipmapFilter(sampler, mipmapFilter);
		}
	}

	void Renderer::setGatherEnable(SamplerType type, int sampler, bool enable)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setGatherEnable(sampler, enable);
		}
		else
		{
			VertexProcessor::setGatherEnable(sampler, enable);
		}
	}

	void Renderer::setAddressingModeU(SamplerType type, int sampler, AddressingMode addressMode)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setAddressingModeU(sampler, addressMode);
		}
		else
		{
			VertexProcessor::setAddressingModeU(sampler, addressMode);
		}
	}

	void Renderer::setAddressingModeV(SamplerType type, int sampler, AddressingMode addressMode)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setAddressingModeV(sampler, addressMode);
		}
		else
		{
			VertexProcessor::setAddressingModeV(sampler, addressMode);
		}
	}

	void Renderer::setAddressingModeW(SamplerType type, int sampler, AddressingMode addressMode)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setAddressingModeW(sampler, addressMode);
		}
		else
		{
			VertexProcessor::setAddressingModeW(sampler, addressMode);
		}
	}

	void Renderer::setReadSRGB(SamplerType type, int sampler, bool sRGB)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setReadSRGB(sampler, sRGB);
		}
		else
		{
			VertexProcessor::setReadSRGB(sampler, sRGB);
		}
	}

	void Renderer::setMipmapLOD(SamplerType type, int sampler, float bias)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setMipmapLOD(sampler, bias);
		}
		else
		{
			VertexProcessor::setMipmapLOD(sampler, bias);
		}
	}

	void Renderer::setBorderColor(SamplerType type, int sampler, const Color<float> &borderColor)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setBorderColor(sampler, borderColor);
		}
		else
		{
			VertexProcessor::setBorderColor(sampler, borderColor);
		}
	}

	void Renderer::setMaxAnisotropy(SamplerType type, int sampler, float maxAnisotropy)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setMaxAnisotropy(sampler, maxAnisotropy);
		}
		else
		{
			VertexProcessor::setMaxAnisotropy(sampler, maxAnisotropy);
		}
	}

	void Renderer::setHighPrecisionFiltering(SamplerType type, int sampler, bool highPrecisionFiltering)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setHighPrecisionFiltering(sampler, highPrecisionFiltering);
		}
		else
		{
			VertexProcessor::setHighPrecisionFiltering(sampler, highPrecisionFiltering);
		}
	}

	void Renderer::setSwizzleR(SamplerType type, int sampler, SwizzleType swizzleR)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setSwizzleR(sampler, swizzleR);
		}
		else
		{
			VertexProcessor::setSwizzleR(sampler, swizzleR);
		}
	}

	void Renderer::setSwizzleG(SamplerType type, int sampler, SwizzleType swizzleG)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setSwizzleG(sampler, swizzleG);
		}
		else
		{
			VertexProcessor::setSwizzleG(sampler, swizzleG);
		}
	}

	void Renderer::setSwizzleB(SamplerType type, int sampler, SwizzleType swizzleB)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setSwizzleB(sampler, swizzleB);
		}
		else
		{
			VertexProcessor::setSwizzleB(sampler, swizzleB);
		}
	}

	void Renderer::setSwizzleA(SamplerType type, int sampler, SwizzleType swizzleA)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setSwizzleA(sampler, swizzleA);
		}
		else
		{
			VertexProcessor::setSwizzleA(sampler, swizzleA);
		}
	}

	void Renderer::setCompareFunc(SamplerType type, int sampler, CompareFunc compFunc)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setCompareFunc(sampler, compFunc);
		}
		else
		{
			VertexProcessor::setCompareFunc(sampler, compFunc);
		}
	}

	void Renderer::setBaseLevel(SamplerType type, int sampler, int baseLevel)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setBaseLevel(sampler, baseLevel);
		}
		else
		{
			VertexProcessor::setBaseLevel(sampler, baseLevel);
		}
	}

	void Renderer::setMaxLevel(SamplerType type, int sampler, int maxLevel)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setMaxLevel(sampler, maxLevel);
		}
		else
		{
			VertexProcessor::setMaxLevel(sampler, maxLevel);
		}
	}

	void Renderer::setMinLod(SamplerType type, int sampler, float minLod)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setMinLod(sampler, minLod);
		}
		else
		{
			VertexProcessor::setMinLod(sampler, minLod);
		}
	}

	void Renderer::setMaxLod(SamplerType type, int sampler, float maxLod)
	{
		if(type == SAMPLER_PIXEL)
		{
			PixelProcessor::setMaxLod(sampler, maxLod);
		}
		else
		{
			VertexProcessor::setMaxLod(sampler, maxLod);
		}
	}

	void Renderer::setPointSpriteEnable(bool pointSpriteEnable)
	{
		context->setPointSpriteEnable(pointSpriteEnable);
	}

	void Renderer::setPointScaleEnable(bool pointScaleEnable)
	{
		context->setPointScaleEnable(pointScaleEnable);
	}

	void Renderer::setLineWidth(float width)
	{
		context->lineWidth = width;
	}

	void Renderer::setDepthBias(float bias)
	{
		context->depthBias = bias;
	}

	void Renderer::setSlopeDepthBias(float slopeBias)
	{
		context->slopeDepthBias = slopeBias;
	}

	void Renderer::setRasterizerDiscard(bool rasterizerDiscard)
	{
		context->rasterizerDiscard = rasterizerDiscard;
	}

	void Renderer::setPixelShader(const PixelShader *shader)
	{
		context->pixelShader = shader;

		loadConstants(shader);
	}

	void Renderer::setVertexShader(const VertexShader *shader)
	{
		context->vertexShader = shader;

		loadConstants(shader);
	}

	void Renderer::setPixelShaderConstantF(unsigned int index, const float value[4], unsigned int count)
	{
		for(unsigned int i = 0; i < DRAW_COUNT; i++)
		{
			if(drawCall[i]->psDirtyConstF < index + count)
			{
				drawCall[i]->psDirtyConstF = index + count;
			}
		}

		for(unsigned int i = 0; i < count; i++)
		{
			PixelProcessor::setFloatConstant(index + i, value);
			value += 4;
		}
	}

	void Renderer::setPixelShaderConstantI(unsigned int index, const int value[4], unsigned int count)
	{
		for(unsigned int i = 0; i < DRAW_COUNT; i++)
		{
			if(drawCall[i]->psDirtyConstI < index + count)
			{
				drawCall[i]->psDirtyConstI = index + count;
			}
		}

		for(unsigned int i = 0; i < count; i++)
		{
			PixelProcessor::setIntegerConstant(index + i, value);
			value += 4;
		}
	}

	void Renderer::setPixelShaderConstantB(unsigned int index, const int *boolean, unsigned int count)
	{
		for(unsigned int i = 0; i < DRAW_COUNT; i++)
		{
			if(drawCall[i]->psDirtyConstB < index + count)
			{
				drawCall[i]->psDirtyConstB = index + count;
			}
		}

		for(unsigned int i = 0; i < count; i++)
		{
			PixelProcessor::setBooleanConstant(index + i, *boolean);
			boolean++;
		}
	}

	void Renderer::setVertexShaderConstantF(unsigned int index, const float value[4], unsigned int count)
	{
		for(unsigned int i = 0; i < DRAW_COUNT; i++)
		{
			if(drawCall[i]->vsDirtyConstF < index + count)
			{
				drawCall[i]->vsDirtyConstF = index + count;
			}
		}

		for(unsigned int i = 0; i < count; i++)
		{
			VertexProcessor::setFloatConstant(index + i, value);
			value += 4;
		}
	}

	void Renderer::setVertexShaderConstantI(unsigned int index, const int value[4], unsigned int count)
	{
		for(unsigned int i = 0; i < DRAW_COUNT; i++)
		{
			if(drawCall[i]->vsDirtyConstI < index + count)
			{
				drawCall[i]->vsDirtyConstI = index + count;
			}
		}

		for(unsigned int i = 0; i < count; i++)
		{
			VertexProcessor::setIntegerConstant(index + i, value);
			value += 4;
		}
	}

	void Renderer::setVertexShaderConstantB(unsigned int index, const int *boolean, unsigned int count)
	{
		for(unsigned int i = 0; i < DRAW_COUNT; i++)
		{
			if(drawCall[i]->vsDirtyConstB < index + count)
			{
				drawCall[i]->vsDirtyConstB = index + count;
			}
		}

		for(unsigned int i = 0; i < count; i++)
		{
			VertexProcessor::setBooleanConstant(index + i, *boolean);
			boolean++;
		}
	}

	void Renderer::setModelMatrix(const Matrix &M, int i)
	{
		VertexProcessor::setModelMatrix(M, i);
	}

	void Renderer::setViewMatrix(const Matrix &V)
	{
		VertexProcessor::setViewMatrix(V);
		updateClipPlanes = true;
	}

	void Renderer::setBaseMatrix(const Matrix &B)
	{
		VertexProcessor::setBaseMatrix(B);
		updateClipPlanes = true;
	}

	void Renderer::setProjectionMatrix(const Matrix &P)
	{
		VertexProcessor::setProjectionMatrix(P);
		updateClipPlanes = true;
	}

	void Renderer::addQuery(Query *query)
	{
		queries.push_back(query);
	}

	void Renderer::removeQuery(Query *query)
	{
		queries.remove(query);
	}

	#if PERF_HUD
		int Renderer::getThreadCount()
		{
			return threadCount;
		}

		int64_t Renderer::getVertexTime(int thread)
		{
			return vertexTime[thread];
		}

		int64_t Renderer::getSetupTime(int thread)
		{
			return setupTime[thread];
		}

		int64_t Renderer::getPixelTime(int thread)
		{
			return pixelTime[thread];
		}

		void Renderer::resetTimers()
		{
			for(int thread = 0; thread < threadCount; thread++)
			{
				vertexTime[thread] = 0;
				setupTime[thread] = 0;
				pixelTime[thread] = 0;
			}
		}
	#endif

	void Renderer::setViewport(const Viewport &viewport)
	{
		this->viewport = viewport;
	}

	void Renderer::setScissor(const Rect &scissor)
	{
		this->scissor = scissor;
	}

	void Renderer::setClipFlags(int flags)
	{
		clipFlags = flags << 8;   // Bottom 8 bits used by legacy frustum
	}

	void Renderer::setClipPlane(unsigned int index, const float plane[4])
	{
		if(index < MAX_CLIP_PLANES)
		{
			userPlane[index] = plane;
		}
		else ASSERT(false);

		updateClipPlanes = true;
	}

	void Renderer::updateConfiguration(bool initialUpdate)
	{
		bool newConfiguration = swiftConfig->hasNewConfiguration();

		if(newConfiguration || initialUpdate)
		{
			terminateThreads();

			SwiftConfig::Configuration configuration = {};
			swiftConfig->getConfiguration(configuration);

			precacheVertex = !newConfiguration && configuration.precache;
			precacheSetup = !newConfiguration && configuration.precache;
			precachePixel = !newConfiguration && configuration.precache;

			VertexProcessor::setRoutineCacheSize(configuration.vertexRoutineCacheSize);
			PixelProcessor::setRoutineCacheSize(configuration.pixelRoutineCacheSize);
			SetupProcessor::setRoutineCacheSize(configuration.setupRoutineCacheSize);

			switch(configuration.textureSampleQuality)
			{
			case 0:  Sampler::setFilterQuality(FILTER_POINT);       break;
			case 1:  Sampler::setFilterQuality(FILTER_LINEAR);      break;
			case 2:  Sampler::setFilterQuality(FILTER_ANISOTROPIC); break;
			default: Sampler::setFilterQuality(FILTER_ANISOTROPIC); break;
			}

			switch(configuration.mipmapQuality)
			{
			case 0:  Sampler::setMipmapQuality(MIPMAP_POINT);  break;
			case 1:  Sampler::setMipmapQuality(MIPMAP_LINEAR); break;
			default: Sampler::setMipmapQuality(MIPMAP_LINEAR); break;
			}

			setPerspectiveCorrection(configuration.perspectiveCorrection);

			switch(configuration.transcendentalPrecision)
			{
			case 0:
				logPrecision = APPROXIMATE;
				expPrecision = APPROXIMATE;
				rcpPrecision = APPROXIMATE;
				rsqPrecision = APPROXIMATE;
				break;
			case 1:
				logPrecision = PARTIAL;
				expPrecision = PARTIAL;
				rcpPrecision = PARTIAL;
				rsqPrecision = PARTIAL;
				break;
			case 2:
				logPrecision = ACCURATE;
				expPrecision = ACCURATE;
				rcpPrecision = ACCURATE;
				rsqPrecision = ACCURATE;
				break;
			case 3:
				logPrecision = WHQL;
				expPrecision = WHQL;
				rcpPrecision = WHQL;
				rsqPrecision = WHQL;
				break;
			case 4:
				logPrecision = IEEE;
				expPrecision = IEEE;
				rcpPrecision = IEEE;
				rsqPrecision = IEEE;
				break;
			default:
				logPrecision = ACCURATE;
				expPrecision = ACCURATE;
				rcpPrecision = ACCURATE;
				rsqPrecision = ACCURATE;
				break;
			}

			switch(configuration.transparencyAntialiasing)
			{
			case 0:  transparencyAntialiasing = TRANSPARENCY_NONE;              break;
			case 1:  transparencyAntialiasing = TRANSPARENCY_ALPHA_TO_COVERAGE; break;
			default: transparencyAntialiasing = TRANSPARENCY_NONE;              break;
			}

			switch(configuration.threadCount)
			{
			case -1: threadCount = CPUID::coreCount();        break;
			case 0:  threadCount = CPUID::processAffinity();  break;
			default: threadCount = configuration.threadCount; break;
			}

			CPUID::setEnableSSE4_1(configuration.enableSSE4_1);
			CPUID::setEnableSSSE3(configuration.enableSSSE3);
			CPUID::setEnableSSE3(configuration.enableSSE3);
			CPUID::setEnableSSE2(configuration.enableSSE2);
			CPUID::setEnableSSE(configuration.enableSSE);

			for(int pass = 0; pass < 10; pass++)
			{
				optimization[pass] = configuration.optimization[pass];
			}

			forceWindowed = configuration.forceWindowed;
			complementaryDepthBuffer = configuration.complementaryDepthBuffer;
			postBlendSRGB = configuration.postBlendSRGB;
			exactColorRounding = configuration.exactColorRounding;
			forceClearRegisters = configuration.forceClearRegisters;

		#ifndef NDEBUG
			minPrimitives = configuration.minPrimitives;
			maxPrimitives = configuration.maxPrimitives;
		#endif
		}

		if(!initialUpdate && !worker[0])
		{
			initializeThreads();
		}
	}
}
