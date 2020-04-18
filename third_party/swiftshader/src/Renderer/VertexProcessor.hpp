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

#ifndef sw_VertexProcessor_hpp
#define sw_VertexProcessor_hpp

#include "Matrix.hpp"
#include "Context.hpp"
#include "RoutineCache.hpp"
#include "Shader/VertexShader.hpp"

namespace sw
{
	struct DrawData;

	struct VertexCache   // FIXME: Variable size
	{
		void clear();

		Vertex vertex[16][4];
		unsigned int tag[16];

		int drawCall;
	};

	struct VertexTask
	{
		unsigned int vertexCount;
		unsigned int primitiveStart;
		VertexCache vertexCache;
	};

	class VertexProcessor
	{
	public:
		struct States
		{
			unsigned int computeHash();

			uint64_t shaderID;

			bool fixedFunction             : 1;   // TODO: Eliminate by querying shader.
			bool textureSampling           : 1;   // TODO: Eliminate by querying shader.
			unsigned int positionRegister  : BITS(MAX_VERTEX_OUTPUTS);   // TODO: Eliminate by querying shader.
			unsigned int pointSizeRegister : BITS(MAX_VERTEX_OUTPUTS);   // TODO: Eliminate by querying shader.

			unsigned int vertexBlendMatrixCount               : 3;
			bool indexedVertexBlendEnable                     : 1;
			bool vertexNormalActive                           : 1;
			bool normalizeNormals                             : 1;
			bool vertexLightingActive                         : 1;
			bool diffuseActive                                : 1;
			bool specularActive                               : 1;
			bool vertexSpecularActive                         : 1;
			unsigned int vertexLightActive                    : 8;
			MaterialSource vertexDiffuseMaterialSourceActive  : BITS(MATERIAL_LAST);
			MaterialSource vertexSpecularMaterialSourceActive : BITS(MATERIAL_LAST);
			MaterialSource vertexAmbientMaterialSourceActive  : BITS(MATERIAL_LAST);
			MaterialSource vertexEmissiveMaterialSourceActive : BITS(MATERIAL_LAST);
			bool fogActive                                    : 1;
			FogMode vertexFogMode                             : BITS(FOG_LAST);
			bool rangeFogActive                               : 1;
			bool localViewerActive                            : 1;
			bool pointSizeActive                              : 1;
			bool pointScaleActive                             : 1;
			bool transformFeedbackQueryEnabled                : 1;
			uint64_t transformFeedbackEnabled                 : 64;
			unsigned char verticesPerPrimitive                : 2; // 1 (points), 2 (lines) or 3 (triangles)

			bool preTransformed : 1;
			bool superSampling  : 1;
			bool multiSampling  : 1;

			struct TextureState
			{
				TexGen texGenActive                       : BITS(TEXGEN_LAST);
				unsigned char textureTransformCountActive : 3;
				unsigned char texCoordIndexActive         : 3;
			};

			TextureState textureState[8];

			Sampler::State sampler[VERTEX_TEXTURE_IMAGE_UNITS];

			struct Input
			{
				operator bool() const   // Returns true if stream contains data
				{
					return count != 0;
				}

				StreamType type    : BITS(STREAMTYPE_LAST);
				unsigned int count : 3;
				bool normalized    : 1;
				unsigned int attribType : BITS(VertexShader::ATTRIBTYPE_LAST);
			};

			struct Output
			{
				union
				{
					unsigned char write : 4;

					struct
					{
						unsigned char xWrite : 1;
						unsigned char yWrite : 1;
						unsigned char zWrite : 1;
						unsigned char wWrite : 1;
					};
				};

				union
				{
					unsigned char clamp : 4;

					struct
					{
						unsigned char xClamp : 1;
						unsigned char yClamp : 1;
						unsigned char zClamp : 1;
						unsigned char wClamp : 1;
					};
				};
			};

			Input input[MAX_VERTEX_INPUTS];
			Output output[MAX_VERTEX_OUTPUTS];
		};

		struct State : States
		{
			State();

			bool operator==(const State &state) const;

			unsigned int hash;
		};

		struct FixedFunction
		{
			float4 transformT[12][4];
			float4 cameraTransformT[12][4];
			float4 normalTransformT[12][4];
			float4 textureTransform[8][4];

			float4 lightPosition[8];
			float4 lightAmbient[8];
			float4 lightSpecular[8];
			float4 lightDiffuse[8];
			float4 attenuationConstant[8];
			float4 attenuationLinear[8];
			float4 attenuationQuadratic[8];
			float lightRange[8];
			float4 materialDiffuse;
			float4 materialSpecular;
			float materialShininess;
			float4 globalAmbient;
			float4 materialEmission;
			float4 materialAmbient;
		};

		struct PointSprite
		{
			float4 pointSize;
			float pointSizeMin;
			float pointSizeMax;
			float pointScaleA;
			float pointScaleB;
			float pointScaleC;
		};

		typedef void (*RoutinePointer)(Vertex *output, unsigned int *batch, VertexTask *vertexTask, DrawData *draw);

		VertexProcessor(Context *context);

		virtual ~VertexProcessor();

		void setInputStream(int index, const Stream &stream);
		void resetInputStreams(bool preTransformed);

		void setFloatConstant(unsigned int index, const float value[4]);
		void setIntegerConstant(unsigned int index, const int integer[4]);
		void setBooleanConstant(unsigned int index, int boolean);

		void setUniformBuffer(int index, sw::Resource* uniformBuffer, int offset);
		void lockUniformBuffers(byte** u, sw::Resource* uniformBuffers[]);

		void setTransformFeedbackBuffer(int index, sw::Resource* transformFeedbackBuffer, int offset, unsigned int reg, unsigned int row, unsigned int col, unsigned int stride);
		void lockTransformFeedbackBuffers(byte** t, unsigned int* v, unsigned int* r, unsigned int* c, unsigned int* s, sw::Resource* transformFeedbackBuffers[]);

		// Transformations
		void setModelMatrix(const Matrix &M, int i = 0);
		void setViewMatrix(const Matrix &V);
		void setBaseMatrix(const Matrix &B);
		void setProjectionMatrix(const Matrix &P);

		// Lighting
		void setLightingEnable(bool lightingEnable);
		void setLightEnable(unsigned int light, bool lightEnable);
		void setSpecularEnable(bool specularEnable);

		void setGlobalAmbient(const Color<float> &globalAmbient);
		void setLightPosition(unsigned int light, const Point &lightPosition);
		void setLightViewPosition(unsigned int light, const Point &lightPosition);
		void setLightDiffuse(unsigned int light, const Color<float> &lightDiffuse);
		void setLightSpecular(unsigned int light, const Color<float> &lightSpecular);
		void setLightAmbient(unsigned int light, const Color<float> &lightAmbient);
		void setLightAttenuation(unsigned int light, float constant, float linear, float quadratic);
		void setLightRange(unsigned int light, float lightRange);

		void setInstanceID(int instanceID);

		void setFogEnable(bool fogEnable);
		void setVertexFogMode(FogMode fogMode);
		void setRangeFogEnable(bool enable);

		void setColorVertexEnable(bool colorVertexEnable);
		void setDiffuseMaterialSource(MaterialSource diffuseMaterialSource);
		void setSpecularMaterialSource(MaterialSource specularMaterialSource);
		void setAmbientMaterialSource(MaterialSource ambientMaterialSource);
		void setEmissiveMaterialSource(MaterialSource emissiveMaterialSource);

		void setMaterialEmission(const Color<float> &emission);
		void setMaterialAmbient(const Color<float> &materialAmbient);
		void setMaterialDiffuse(const Color<float> &diffuseColor);
		void setMaterialSpecular(const Color<float> &specularColor);
		void setMaterialShininess(float specularPower);

		void setIndexedVertexBlendEnable(bool indexedVertexBlendEnable);
		void setVertexBlendMatrixCount(unsigned int vertexBlendMatrixCount);

		void setTextureWrap(unsigned int stage, int mask);
		void setTexGen(unsigned int stage, TexGen texGen);
		void setLocalViewer(bool localViewer);
		void setNormalizeNormals(bool normalizeNormals);
		void setTextureMatrix(int stage, const Matrix &T);
		void setTextureTransform(int stage, int count, bool project);

		void setTextureFilter(unsigned int sampler, FilterType textureFilter);
		void setMipmapFilter(unsigned int sampler, MipmapType mipmapFilter);
		void setGatherEnable(unsigned int sampler, bool enable);
		void setAddressingModeU(unsigned int sampler, AddressingMode addressingMode);
		void setAddressingModeV(unsigned int sampler, AddressingMode addressingMode);
		void setAddressingModeW(unsigned int sampler, AddressingMode addressingMode);
		void setReadSRGB(unsigned int sampler, bool sRGB);
		void setMipmapLOD(unsigned int sampler, float bias);
		void setBorderColor(unsigned int sampler, const Color<float> &borderColor);
		void setMaxAnisotropy(unsigned int stage, float maxAnisotropy);
		void setHighPrecisionFiltering(unsigned int sampler, bool highPrecisionFiltering);
		void setSwizzleR(unsigned int sampler, SwizzleType swizzleR);
		void setSwizzleG(unsigned int sampler, SwizzleType swizzleG);
		void setSwizzleB(unsigned int sampler, SwizzleType swizzleB);
		void setSwizzleA(unsigned int sampler, SwizzleType swizzleA);
		void setCompareFunc(unsigned int sampler, CompareFunc compare);
		void setBaseLevel(unsigned int sampler, int baseLevel);
		void setMaxLevel(unsigned int sampler, int maxLevel);
		void setMinLod(unsigned int sampler, float minLod);
		void setMaxLod(unsigned int sampler, float maxLod);

		void setPointSize(float pointSize);
		void setPointSizeMin(float pointSizeMin);
		void setPointSizeMax(float pointSizeMax);
		void setPointScaleA(float pointScaleA);
		void setPointScaleB(float pointScaleB);
		void setPointScaleC(float pointScaleC);

		void setTransformFeedbackQueryEnabled(bool enable);
		void enableTransformFeedback(uint64_t enable);

	protected:
		const Matrix &getModelTransform(int i);
		const Matrix &getViewTransform();

		const State update(DrawType drawType);
		Routine *routine(const State &state);

		bool isFixedFunction();
		void setRoutineCacheSize(int cacheSize);

		// Shader constants
		float4 c[VERTEX_UNIFORM_VECTORS + 1];   // One extra for indices out of range, c[VERTEX_UNIFORM_VECTORS] = {0, 0, 0, 0}
		int4 i[16];
		bool b[16];

		PointSprite point;
		FixedFunction ff;

	private:
		struct UniformBufferInfo
		{
			UniformBufferInfo();

			Resource* buffer;
			int offset;
		};
		UniformBufferInfo uniformBufferInfo[MAX_UNIFORM_BUFFER_BINDINGS];

		struct TransformFeedbackInfo
		{
			TransformFeedbackInfo();

			Resource* buffer;
			unsigned int offset;
			unsigned int reg;
			unsigned int row;
			unsigned int col;
			unsigned int stride;
		};
		TransformFeedbackInfo transformFeedbackInfo[MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS];

		void updateTransform();
		void setTransform(const Matrix &M, int i);
		void setCameraTransform(const Matrix &M, int i);
		void setNormalTransform(const Matrix &M, int i);

		Context *const context;

		RoutineCache<State> *routineCache;

	protected:
		Matrix M[12];      // Model/Geometry/World matrix
		Matrix V;          // View/Camera/Eye matrix
		Matrix B;          // Base matrix
		Matrix P;          // Projection matrix
		Matrix PB;         // P * B
		Matrix PBV;        // P * B * V
		Matrix PBVM[12];   // P * B * V * M

		// Update hierarchy
		bool updateMatrix;
		bool updateModelMatrix[12];
		bool updateViewMatrix;
		bool updateBaseMatrix;
		bool updateProjectionMatrix;
		bool updateLighting;
	};
}

#endif   // sw_VertexProcessor_hpp
