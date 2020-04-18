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

#include "VertexProcessor.hpp"

#include "Shader/VertexPipeline.hpp"
#include "Shader/VertexProgram.hpp"
#include "Shader/VertexShader.hpp"
#include "Shader/PixelShader.hpp"
#include "Shader/Constants.hpp"
#include "Common/Math.hpp"
#include "Common/Debug.hpp"

#include <string.h>

namespace sw
{
	bool precacheVertex = false;

	void VertexCache::clear()
	{
		for(int i = 0; i < 16; i++)
		{
			tag[i] = 0x80000000;
		}
	}

	unsigned int VertexProcessor::States::computeHash()
	{
		unsigned int *state = (unsigned int*)this;
		unsigned int hash = 0;

		for(unsigned int i = 0; i < sizeof(States) / 4; i++)
		{
			hash ^= state[i];
		}

		return hash;
	}

	VertexProcessor::State::State()
	{
		memset(this, 0, sizeof(State));
	}

	bool VertexProcessor::State::operator==(const State &state) const
	{
		if(hash != state.hash)
		{
			return false;
		}

		return memcmp(static_cast<const States*>(this), static_cast<const States*>(&state), sizeof(States)) == 0;
	}

	VertexProcessor::TransformFeedbackInfo::TransformFeedbackInfo()
	{
		buffer = nullptr;
		offset = 0;
		reg = 0;
		row = 0;
		col = 0;
		stride = 0;
	}

	VertexProcessor::UniformBufferInfo::UniformBufferInfo()
	{
		buffer = nullptr;
		offset = 0;
	}

	VertexProcessor::VertexProcessor(Context *context) : context(context)
	{
		for(int i = 0; i < 12; i++)
		{
			M[i] = 1;
		}

		V = 1;
		B = 1;
		P = 0;
		PB = 0;
		PBV = 0;

		for(int i = 0; i < 12; i++)
		{
			PBVM[i] = 0;
		}

		setLightingEnable(true);
		setSpecularEnable(false);

		for(int i = 0; i < 8; i++)
		{
			setLightEnable(i, false);
			setLightPosition(i, 0);
		}

		updateMatrix = true;
		updateViewMatrix = true;
		updateBaseMatrix = true;
		updateProjectionMatrix = true;
		updateLighting = true;

		for(int i = 0; i < 12; i++)
		{
			updateModelMatrix[i] = true;
		}

		routineCache = 0;
		setRoutineCacheSize(1024);
	}

	VertexProcessor::~VertexProcessor()
	{
		delete routineCache;
		routineCache = 0;
	}

	void VertexProcessor::setInputStream(int index, const Stream &stream)
	{
		context->input[index] = stream;
	}

	void VertexProcessor::resetInputStreams(bool preTransformed)
	{
		for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
		{
			context->input[i].defaults();
		}

		context->preTransformed = preTransformed;
	}

	void VertexProcessor::setFloatConstant(unsigned int index, const float value[4])
	{
		if(index < VERTEX_UNIFORM_VECTORS)
		{
			c[index][0] = value[0];
			c[index][1] = value[1];
			c[index][2] = value[2];
			c[index][3] = value[3];
		}
		else ASSERT(false);
	}

	void VertexProcessor::setIntegerConstant(unsigned int index, const int integer[4])
	{
		if(index < 16)
		{
			i[index][0] = integer[0];
			i[index][1] = integer[1];
			i[index][2] = integer[2];
			i[index][3] = integer[3];
		}
		else ASSERT(false);
	}

	void VertexProcessor::setBooleanConstant(unsigned int index, int boolean)
	{
		if(index < 16)
		{
			b[index] = boolean != 0;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setUniformBuffer(int index, sw::Resource* buffer, int offset)
	{
		uniformBufferInfo[index].buffer = buffer;
		uniformBufferInfo[index].offset = offset;
	}

	void VertexProcessor::lockUniformBuffers(byte** u, sw::Resource* uniformBuffers[])
	{
		for(int i = 0; i < MAX_UNIFORM_BUFFER_BINDINGS; ++i)
		{
			u[i] = uniformBufferInfo[i].buffer ? static_cast<byte*>(uniformBufferInfo[i].buffer->lock(PUBLIC, PRIVATE)) + uniformBufferInfo[i].offset : nullptr;
			uniformBuffers[i] = uniformBufferInfo[i].buffer;
		}
	}

	void VertexProcessor::setTransformFeedbackBuffer(int index, sw::Resource* buffer, int offset, unsigned int reg, unsigned int row, unsigned int col, unsigned int stride)
	{
		transformFeedbackInfo[index].buffer = buffer;
		transformFeedbackInfo[index].offset = offset;
		transformFeedbackInfo[index].reg = reg;
		transformFeedbackInfo[index].row = row;
		transformFeedbackInfo[index].col = col;
		transformFeedbackInfo[index].stride = stride;
	}

	void VertexProcessor::lockTransformFeedbackBuffers(byte** t, unsigned int* v, unsigned int* r, unsigned int* c, unsigned int* s, sw::Resource* transformFeedbackBuffers[])
	{
		for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS; ++i)
		{
			t[i] = transformFeedbackInfo[i].buffer ? static_cast<byte*>(transformFeedbackInfo[i].buffer->lock(PUBLIC, PRIVATE)) + transformFeedbackInfo[i].offset : nullptr;
			transformFeedbackBuffers[i] = transformFeedbackInfo[i].buffer;
			v[i] = transformFeedbackInfo[i].reg;
			r[i] = transformFeedbackInfo[i].row;
			c[i] = transformFeedbackInfo[i].col;
			s[i] = transformFeedbackInfo[i].stride;
		}
	}

	void VertexProcessor::setModelMatrix(const Matrix &M, int i)
	{
		if(i < 12)
		{
			this->M[i] = M;

			updateMatrix = true;
			updateModelMatrix[i] = true;
			updateLighting = true;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setViewMatrix(const Matrix &V)
	{
		this->V = V;

		updateMatrix = true;
		updateViewMatrix = true;
	}

	void VertexProcessor::setBaseMatrix(const Matrix &B)
	{
		this->B = B;

		updateMatrix = true;
		updateBaseMatrix = true;
	}

	void VertexProcessor::setProjectionMatrix(const Matrix &P)
	{
		this->P = P;
		context->wBasedFog = (P[3][0] != 0.0f) || (P[3][1] != 0.0f) || (P[3][2] != 0.0f) || (P[3][3] != 1.0f);

		updateMatrix = true;
		updateProjectionMatrix = true;
	}

	void VertexProcessor::setLightingEnable(bool lightingEnable)
	{
		context->setLightingEnable(lightingEnable);

		updateLighting = true;
	}

	void VertexProcessor::setLightEnable(unsigned int light, bool lightEnable)
	{
		if(light < 8)
		{
			context->setLightEnable(light, lightEnable);
		}
		else ASSERT(false);

		updateLighting = true;
	}

	void VertexProcessor::setSpecularEnable(bool specularEnable)
	{
		context->setSpecularEnable(specularEnable);

		updateLighting = true;
	}

	void VertexProcessor::setLightPosition(unsigned int light, const Point &lightPosition)
	{
		if(light < 8)
		{
			context->setLightPosition(light, lightPosition);
		}
		else ASSERT(false);

		updateLighting = true;
	}

	void VertexProcessor::setLightDiffuse(unsigned int light, const Color<float> &lightDiffuse)
	{
		if(light < 8)
		{
			ff.lightDiffuse[light][0] = lightDiffuse.r;
			ff.lightDiffuse[light][1] = lightDiffuse.g;
			ff.lightDiffuse[light][2] = lightDiffuse.b;
			ff.lightDiffuse[light][3] = lightDiffuse.a;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setLightSpecular(unsigned int light, const Color<float> &lightSpecular)
	{
		if(light < 8)
		{
			ff.lightSpecular[light][0] = lightSpecular.r;
			ff.lightSpecular[light][1] = lightSpecular.g;
			ff.lightSpecular[light][2] = lightSpecular.b;
			ff.lightSpecular[light][3] = lightSpecular.a;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setLightAmbient(unsigned int light, const Color<float> &lightAmbient)
	{
		if(light < 8)
		{
			ff.lightAmbient[light][0] = lightAmbient.r;
			ff.lightAmbient[light][1] = lightAmbient.g;
			ff.lightAmbient[light][2] = lightAmbient.b;
			ff.lightAmbient[light][3] = lightAmbient.a;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setLightAttenuation(unsigned int light, float constant, float linear, float quadratic)
	{
		if(light < 8)
		{
			ff.attenuationConstant[light] = replicate(constant);
			ff.attenuationLinear[light] = replicate(linear);
			ff.attenuationQuadratic[light] = replicate(quadratic);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setLightRange(unsigned int light, float lightRange)
	{
		if(light < 8)
		{
			ff.lightRange[light] = lightRange;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setFogEnable(bool fogEnable)
	{
		context->fogEnable = fogEnable;
	}

	void VertexProcessor::setVertexFogMode(FogMode fogMode)
	{
		context->vertexFogMode = fogMode;
	}

	void VertexProcessor::setInstanceID(int instanceID)
	{
		context->instanceID = instanceID;
	}

	void VertexProcessor::setColorVertexEnable(bool colorVertexEnable)
	{
		context->setColorVertexEnable(colorVertexEnable);
	}

	void VertexProcessor::setDiffuseMaterialSource(MaterialSource diffuseMaterialSource)
	{
		context->setDiffuseMaterialSource(diffuseMaterialSource);
	}

	void VertexProcessor::setSpecularMaterialSource(MaterialSource specularMaterialSource)
	{
		context->setSpecularMaterialSource(specularMaterialSource);
	}

	void VertexProcessor::setAmbientMaterialSource(MaterialSource ambientMaterialSource)
	{
		context->setAmbientMaterialSource(ambientMaterialSource);
	}

	void VertexProcessor::setEmissiveMaterialSource(MaterialSource emissiveMaterialSource)
	{
		context->setEmissiveMaterialSource(emissiveMaterialSource);
	}

	void VertexProcessor::setGlobalAmbient(const Color<float> &globalAmbient)
	{
		ff.globalAmbient[0] = globalAmbient.r;
		ff.globalAmbient[1] = globalAmbient.g;
		ff.globalAmbient[2] = globalAmbient.b;
		ff.globalAmbient[3] = globalAmbient.a;
	}

	void VertexProcessor::setMaterialEmission(const Color<float> &emission)
	{
		ff.materialEmission[0] = emission.r;
		ff.materialEmission[1] = emission.g;
		ff.materialEmission[2] = emission.b;
		ff.materialEmission[3] = emission.a;
	}

	void VertexProcessor::setMaterialAmbient(const Color<float> &materialAmbient)
	{
		ff.materialAmbient[0] = materialAmbient.r;
		ff.materialAmbient[1] = materialAmbient.g;
		ff.materialAmbient[2] = materialAmbient.b;
		ff.materialAmbient[3] = materialAmbient.a;
	}

	void VertexProcessor::setMaterialDiffuse(const Color<float> &diffuseColor)
	{
		ff.materialDiffuse[0] = diffuseColor.r;
		ff.materialDiffuse[1] = diffuseColor.g;
		ff.materialDiffuse[2] = diffuseColor.b;
		ff.materialDiffuse[3] = diffuseColor.a;
	}

	void VertexProcessor::setMaterialSpecular(const Color<float> &specularColor)
	{
		ff.materialSpecular[0] = specularColor.r;
		ff.materialSpecular[1] = specularColor.g;
		ff.materialSpecular[2] = specularColor.b;
		ff.materialSpecular[3] = specularColor.a;
	}

	void VertexProcessor::setMaterialShininess(float specularPower)
	{
		ff.materialShininess = specularPower;
	}

	void VertexProcessor::setLightViewPosition(unsigned int light, const Point &P)
	{
		if(light < 8)
		{
			ff.lightPosition[light][0] = P.x;
			ff.lightPosition[light][1] = P.y;
			ff.lightPosition[light][2] = P.z;
			ff.lightPosition[light][3] = 1;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setRangeFogEnable(bool enable)
	{
		context->rangeFogEnable = enable;
	}

	void VertexProcessor::setIndexedVertexBlendEnable(bool indexedVertexBlendEnable)
	{
		context->indexedVertexBlendEnable = indexedVertexBlendEnable;
	}

	void VertexProcessor::setVertexBlendMatrixCount(unsigned int vertexBlendMatrixCount)
	{
		if(vertexBlendMatrixCount <= 4)
		{
			context->vertexBlendMatrixCount = vertexBlendMatrixCount;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setTextureWrap(unsigned int stage, int mask)
	{
		if(stage < TEXTURE_IMAGE_UNITS)
		{
			context->textureWrap[stage] = mask;
		}
		else ASSERT(false);

		context->textureWrapActive = false;

		for(int i = 0; i < TEXTURE_IMAGE_UNITS; i++)
		{
			context->textureWrapActive |= (context->textureWrap[i] != 0x00);
		}
	}

	void VertexProcessor::setTexGen(unsigned int stage, TexGen texGen)
	{
		if(stage < 8)
		{
			context->texGen[stage] = texGen;
		}
		else ASSERT(false);
	}

	void VertexProcessor::setLocalViewer(bool localViewer)
	{
		context->localViewer = localViewer;
	}

	void VertexProcessor::setNormalizeNormals(bool normalizeNormals)
	{
		context->normalizeNormals = normalizeNormals;
	}

	void VertexProcessor::setTextureMatrix(int stage, const Matrix &T)
	{
		for(int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				ff.textureTransform[stage][i][j] = T[i][j];
			}
		}
	}

	void VertexProcessor::setTextureTransform(int stage, int count, bool project)
	{
		context->textureTransformCount[stage] = count;
		context->textureTransformProject[stage] = project;
	}

	void VertexProcessor::setTextureFilter(unsigned int sampler, FilterType textureFilter)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setTextureFilter(textureFilter);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setMipmapFilter(unsigned int sampler, MipmapType mipmapFilter)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setMipmapFilter(mipmapFilter);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setGatherEnable(unsigned int sampler, bool enable)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setGatherEnable(enable);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setAddressingModeU(unsigned int sampler, AddressingMode addressMode)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setAddressingModeU(addressMode);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setAddressingModeV(unsigned int sampler, AddressingMode addressMode)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setAddressingModeV(addressMode);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setAddressingModeW(unsigned int sampler, AddressingMode addressMode)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setAddressingModeW(addressMode);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setReadSRGB(unsigned int sampler, bool sRGB)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setReadSRGB(sRGB);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setMipmapLOD(unsigned int sampler, float bias)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setMipmapLOD(bias);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setBorderColor(unsigned int sampler, const Color<float> &borderColor)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setBorderColor(borderColor);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setMaxAnisotropy(unsigned int sampler, float maxAnisotropy)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setMaxAnisotropy(maxAnisotropy);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setHighPrecisionFiltering(unsigned int sampler, bool highPrecisionFiltering)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setHighPrecisionFiltering(highPrecisionFiltering);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setSwizzleR(unsigned int sampler, SwizzleType swizzleR)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setSwizzleR(swizzleR);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setSwizzleG(unsigned int sampler, SwizzleType swizzleG)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setSwizzleG(swizzleG);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setSwizzleB(unsigned int sampler, SwizzleType swizzleB)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setSwizzleB(swizzleB);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setSwizzleA(unsigned int sampler, SwizzleType swizzleA)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setSwizzleA(swizzleA);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setCompareFunc(unsigned int sampler, CompareFunc compFunc)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setCompareFunc(compFunc);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setBaseLevel(unsigned int sampler, int baseLevel)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setBaseLevel(baseLevel);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setMaxLevel(unsigned int sampler, int maxLevel)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setMaxLevel(maxLevel);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setMinLod(unsigned int sampler, float minLod)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setMinLod(minLod);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setMaxLod(unsigned int sampler, float maxLod)
	{
		if(sampler < VERTEX_TEXTURE_IMAGE_UNITS)
		{
			context->sampler[TEXTURE_IMAGE_UNITS + sampler].setMaxLod(maxLod);
		}
		else ASSERT(false);
	}

	void VertexProcessor::setPointSize(float pointSize)
	{
		point.pointSize = replicate(pointSize);
	}

	void VertexProcessor::setPointSizeMin(float pointSizeMin)
	{
		point.pointSizeMin = pointSizeMin;
	}

	void VertexProcessor::setPointSizeMax(float pointSizeMax)
	{
		point.pointSizeMax = pointSizeMax;
	}

	void VertexProcessor::setPointScaleA(float pointScaleA)
	{
		point.pointScaleA = pointScaleA;
	}

	void VertexProcessor::setPointScaleB(float pointScaleB)
	{
		point.pointScaleB = pointScaleB;
	}

	void VertexProcessor::setPointScaleC(float pointScaleC)
	{
		point.pointScaleC = pointScaleC;
	}

	void VertexProcessor::setTransformFeedbackQueryEnabled(bool enable)
	{
		context->transformFeedbackQueryEnabled = enable;
	}

	void VertexProcessor::enableTransformFeedback(uint64_t enable)
	{
		context->transformFeedbackEnabled = enable;
	}

	const Matrix &VertexProcessor::getModelTransform(int i)
	{
		updateTransform();
		return PBVM[i];
	}

	const Matrix &VertexProcessor::getViewTransform()
	{
		updateTransform();
		return PBV;
	}

	bool VertexProcessor::isFixedFunction()
	{
		return !context->vertexShader;
	}

	void VertexProcessor::setTransform(const Matrix &M, int i)
	{
		ff.transformT[i][0][0] = M[0][0];
		ff.transformT[i][0][1] = M[1][0];
		ff.transformT[i][0][2] = M[2][0];
		ff.transformT[i][0][3] = M[3][0];

		ff.transformT[i][1][0] = M[0][1];
		ff.transformT[i][1][1] = M[1][1];
		ff.transformT[i][1][2] = M[2][1];
		ff.transformT[i][1][3] = M[3][1];

		ff.transformT[i][2][0] = M[0][2];
		ff.transformT[i][2][1] = M[1][2];
		ff.transformT[i][2][2] = M[2][2];
		ff.transformT[i][2][3] = M[3][2];

		ff.transformT[i][3][0] = M[0][3];
		ff.transformT[i][3][1] = M[1][3];
		ff.transformT[i][3][2] = M[2][3];
		ff.transformT[i][3][3] = M[3][3];
	}

	void VertexProcessor::setCameraTransform(const Matrix &M, int i)
	{
		ff.cameraTransformT[i][0][0] = M[0][0];
		ff.cameraTransformT[i][0][1] = M[1][0];
		ff.cameraTransformT[i][0][2] = M[2][0];
		ff.cameraTransformT[i][0][3] = M[3][0];

		ff.cameraTransformT[i][1][0] = M[0][1];
		ff.cameraTransformT[i][1][1] = M[1][1];
		ff.cameraTransformT[i][1][2] = M[2][1];
		ff.cameraTransformT[i][1][3] = M[3][1];

		ff.cameraTransformT[i][2][0] = M[0][2];
		ff.cameraTransformT[i][2][1] = M[1][2];
		ff.cameraTransformT[i][2][2] = M[2][2];
		ff.cameraTransformT[i][2][3] = M[3][2];

		ff.cameraTransformT[i][3][0] = M[0][3];
		ff.cameraTransformT[i][3][1] = M[1][3];
		ff.cameraTransformT[i][3][2] = M[2][3];
		ff.cameraTransformT[i][3][3] = M[3][3];
	}

	void VertexProcessor::setNormalTransform(const Matrix &M, int i)
	{
		ff.normalTransformT[i][0][0] = M[0][0];
		ff.normalTransformT[i][0][1] = M[1][0];
		ff.normalTransformT[i][0][2] = M[2][0];
		ff.normalTransformT[i][0][3] = M[3][0];

		ff.normalTransformT[i][1][0] = M[0][1];
		ff.normalTransformT[i][1][1] = M[1][1];
		ff.normalTransformT[i][1][2] = M[2][1];
		ff.normalTransformT[i][1][3] = M[3][1];

		ff.normalTransformT[i][2][0] = M[0][2];
		ff.normalTransformT[i][2][1] = M[1][2];
		ff.normalTransformT[i][2][2] = M[2][2];
		ff.normalTransformT[i][2][3] = M[3][2];

		ff.normalTransformT[i][3][0] = M[0][3];
		ff.normalTransformT[i][3][1] = M[1][3];
		ff.normalTransformT[i][3][2] = M[2][3];
		ff.normalTransformT[i][3][3] = M[3][3];
	}

	void VertexProcessor::updateTransform()
	{
		if(!updateMatrix) return;

		int activeMatrices = context->indexedVertexBlendEnable ? 12 : max(context->vertexBlendMatrixCount, 1);

		if(updateProjectionMatrix)
		{
			PB = P * B;
			PBV = PB * V;

			for(int i = 0; i < activeMatrices; i++)
			{
				PBVM[i] = PBV * M[i];
				updateModelMatrix[i] = false;
			}

			updateProjectionMatrix = false;
			updateBaseMatrix = false;
			updateViewMatrix = false;
		}

		if(updateBaseMatrix)
		{
			PB = P * B;
			PBV = PB * V;

			for(int i = 0; i < activeMatrices; i++)
			{
				PBVM[i] = PBV * M[i];
				updateModelMatrix[i] = false;
			}

			updateBaseMatrix = false;
			updateViewMatrix = false;
		}

		if(updateViewMatrix)
		{
			PBV = PB * V;

			for(int i = 0; i < activeMatrices; i++)
			{
				PBVM[i] = PBV * M[i];
				updateModelMatrix[i] = false;
			}

			updateViewMatrix = false;
		}

		for(int i = 0; i < activeMatrices; i++)
		{
			if(updateModelMatrix[i])
			{
				PBVM[i] = PBV * M[i];
				updateModelMatrix[i] = false;
			}
		}

		for(int i = 0; i < activeMatrices; i++)
		{
			setTransform(PBVM[i], i);
			setCameraTransform(B * V * M[i], i);
			setNormalTransform(~!(B * V * M[i]), i);
		}

		updateMatrix = false;
	}

	void VertexProcessor::setRoutineCacheSize(int cacheSize)
	{
		delete routineCache;
		routineCache = new RoutineCache<State>(clamp(cacheSize, 1, 65536), precacheVertex ? "sw-vertex" : 0);
	}

	const VertexProcessor::State VertexProcessor::update(DrawType drawType)
	{
		if(isFixedFunction())
		{
			updateTransform();

			if(updateLighting)
			{
				for(int i = 0; i < 8; i++)
				{
					if(context->vertexLightActive(i))
					{
						// Light position in camera coordinates
						setLightViewPosition(i, B * V * context->getLightPosition(i));
					}
				}

				updateLighting = false;
			}
		}

		State state;

		if(context->vertexShader)
		{
			state.shaderID = context->vertexShader->getSerialID();
		}
		else
		{
			state.shaderID = 0;
		}

		state.fixedFunction = !context->vertexShader && context->pixelShaderModel() < 0x0300;
		state.textureSampling = context->vertexShader ? context->vertexShader->containsTextureSampling() : false;
		state.positionRegister = context->vertexShader ? context->vertexShader->getPositionRegister() : Pos;
		state.pointSizeRegister = context->vertexShader ? context->vertexShader->getPointSizeRegister() : Pts;

		state.vertexBlendMatrixCount = context->vertexBlendMatrixCountActive();
		state.indexedVertexBlendEnable = context->indexedVertexBlendActive();
		state.vertexNormalActive = context->vertexNormalActive();
		state.normalizeNormals = context->normalizeNormalsActive();
		state.vertexLightingActive = context->vertexLightingActive();
		state.diffuseActive = context->diffuseActive();
		state.specularActive = context->specularActive();
		state.vertexSpecularActive = context->vertexSpecularActive();

		state.vertexLightActive = context->vertexLightActive(0) << 0 |
		                          context->vertexLightActive(1) << 1 |
		                          context->vertexLightActive(2) << 2 |
		                          context->vertexLightActive(3) << 3 |
		                          context->vertexLightActive(4) << 4 |
		                          context->vertexLightActive(5) << 5 |
		                          context->vertexLightActive(6) << 6 |
		                          context->vertexLightActive(7) << 7;

		state.vertexDiffuseMaterialSourceActive = context->vertexDiffuseMaterialSourceActive();
		state.vertexSpecularMaterialSourceActive = context->vertexSpecularMaterialSourceActive();
		state.vertexAmbientMaterialSourceActive = context->vertexAmbientMaterialSourceActive();
		state.vertexEmissiveMaterialSourceActive = context->vertexEmissiveMaterialSourceActive();
		state.fogActive = context->fogActive();
		state.vertexFogMode = context->vertexFogModeActive();
		state.rangeFogActive = context->rangeFogActive();
		state.localViewerActive = context->localViewerActive();
		state.pointSizeActive = context->pointSizeActive();
		state.pointScaleActive = context->pointScaleActive();

		state.preTransformed = context->preTransformed;
		state.superSampling = context->getSuperSampleCount() > 1;
		state.multiSampling = context->getMultiSampleCount() > 1;

		state.transformFeedbackQueryEnabled = context->transformFeedbackQueryEnabled;
		state.transformFeedbackEnabled = context->transformFeedbackEnabled;

		// Note: Quads aren't handled for verticesPerPrimitive, but verticesPerPrimitive is used for transform feedback,
		//       which is an OpenGL ES 3.0 feature, and OpenGL ES 3.0 doesn't support quads as a primitive type.
		DrawType type = static_cast<DrawType>(static_cast<unsigned int>(drawType) & 0xF);
		state.verticesPerPrimitive = 1 + (type >= DRAW_LINELIST) + (type >= DRAW_TRIANGLELIST);

		for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
		{
			state.input[i].type = context->input[i].type;
			state.input[i].count = context->input[i].count;
			state.input[i].normalized = context->input[i].normalized;
			state.input[i].attribType = context->vertexShader ? context->vertexShader->getAttribType(i) : VertexShader::ATTRIBTYPE_FLOAT;
		}

		if(!context->vertexShader)
		{
			for(int i = 0; i < 8; i++)
			{
			//	state.textureState[i].vertexTextureActive = context->vertexTextureActive(i, 0);
				state.textureState[i].texGenActive = context->texGenActive(i);
				state.textureState[i].textureTransformCountActive = context->textureTransformCountActive(i);
				state.textureState[i].texCoordIndexActive = context->texCoordIndexActive(i);
			}
		}
		else
		{
			for(unsigned int i = 0; i < VERTEX_TEXTURE_IMAGE_UNITS; i++)
			{
				if(context->vertexShader->usesSampler(i))
				{
					state.sampler[i] = context->sampler[TEXTURE_IMAGE_UNITS + i].samplerState();
				}
			}
		}

		if(context->vertexShader)   // FIXME: Also when pre-transformed?
		{
			for(int i = 0; i < MAX_VERTEX_OUTPUTS; i++)
			{
				state.output[i].xWrite = context->vertexShader->getOutput(i, 0).active();
				state.output[i].yWrite = context->vertexShader->getOutput(i, 1).active();
				state.output[i].zWrite = context->vertexShader->getOutput(i, 2).active();
				state.output[i].wWrite = context->vertexShader->getOutput(i, 3).active();
			}
		}
		else if(!context->preTransformed || context->pixelShaderModel() < 0x0300)
		{
			state.output[Pos].write = 0xF;

			if(context->diffuseActive() && (context->lightingEnable || context->input[Color0]))
			{
				state.output[C0].write = 0xF;
			}

			if(context->specularActive())
			{
				state.output[C1].write = 0xF;
			}

			for(int stage = 0; stage < 8; stage++)
			{
				if(context->texCoordActive(stage, 0)) state.output[T0 + stage].write |= 0x01;
				if(context->texCoordActive(stage, 1)) state.output[T0 + stage].write |= 0x02;
				if(context->texCoordActive(stage, 2)) state.output[T0 + stage].write |= 0x04;
				if(context->texCoordActive(stage, 3)) state.output[T0 + stage].write |= 0x08;
			}

			if(context->fogActive())
			{
				state.output[Fog].xWrite = true;
			}

			if(context->pointSizeActive())
			{
				state.output[Pts].yWrite = true;
			}
		}
		else
		{
			state.output[Pos].write = 0xF;

			for(int i = 0; i < 2; i++)
			{
				if(context->input[Color0 + i])
				{
					state.output[C0 + i].write = 0xF;
				}
			}

			for(int i = 0; i < 8; i++)
			{
				if(context->input[TexCoord0 + i])
				{
					state.output[T0 + i].write = 0xF;
				}
			}

			if(context->input[PointSize])
			{
				state.output[Pts].yWrite = true;
			}
		}

		if(context->vertexShaderModel() < 0x0300)
		{
			state.output[C0].clamp = 0xF;
			state.output[C1].clamp = 0xF;
			state.output[Fog].xClamp = true;
		}

		state.hash = state.computeHash();

		return state;
	}

	Routine *VertexProcessor::routine(const State &state)
	{
		Routine *routine = routineCache->query(state);

		if(!routine)   // Create one
		{
			VertexRoutine *generator = nullptr;

			if(state.fixedFunction)
			{
				generator = new VertexPipeline(state);
			}
			else
			{
				generator = new VertexProgram(state, context->vertexShader);
			}

			generator->generate();
			routine = (*generator)(L"VertexRoutine_%0.8X", state.shaderID);
			delete generator;

			routineCache->add(state, routine);
		}

		return routine;
	}
}
