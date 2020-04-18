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

#ifndef sw_Context_hpp
#define sw_Context_hpp

#include "Sampler.hpp"
#include "TextureStage.hpp"
#include "Stream.hpp"
#include "Point.hpp"
#include "Vertex.hpp"
#include "Common/Types.hpp"

namespace sw
{
	class Sampler;
	class Surface;
	class PixelShader;
	class VertexShader;
	struct Triangle;
	struct Primitive;
	struct Vertex;
	class Resource;

	enum In   // Default input stream semantic
	{
		Position = 0,
		BlendWeight = 1,
		BlendIndices = 2,
		Normal = 3,
		PointSize = 4,
		Color0 = 5,
		Color1 = 6,
		TexCoord0 = 7,
		TexCoord1 = 8,
		TexCoord2 = 9,
		TexCoord3 = 10,
		TexCoord4 = 11,
		TexCoord5 = 12,
		TexCoord6 = 13,
		TexCoord7 = 14,
		PositionT = 15
	};

	enum DrawType ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		// These types must stay ordered by vertices per primitive. Also, if these basic types
		// are modified, verify the value assigned to task->verticesPerPrimitive in Renderer.cpp
		DRAW_POINTLIST     = 0x00,
		DRAW_LINELIST      = 0x01,
		DRAW_LINESTRIP     = 0x02,
		DRAW_LINELOOP      = 0x03,
		DRAW_TRIANGLELIST  = 0x04,
		DRAW_TRIANGLESTRIP = 0x05,
		DRAW_TRIANGLEFAN   = 0x06,
		DRAW_QUADLIST      = 0x07,

		DRAW_NONINDEXED = 0x00,
		DRAW_INDEXED8   = 0x10,
		DRAW_INDEXED16  = 0x20,
		DRAW_INDEXED32  = 0x30,

		DRAW_INDEXEDPOINTLIST8 = DRAW_POINTLIST | DRAW_INDEXED8,
		DRAW_INDEXEDLINELIST8  = DRAW_LINELIST  | DRAW_INDEXED8,
		DRAW_INDEXEDLINESTRIP8 = DRAW_LINESTRIP | DRAW_INDEXED8,
		DRAW_INDEXEDLINELOOP8  = DRAW_LINELOOP  | DRAW_INDEXED8,
		DRAW_INDEXEDTRIANGLELIST8  = DRAW_TRIANGLELIST  | DRAW_INDEXED8,
		DRAW_INDEXEDTRIANGLESTRIP8 = DRAW_TRIANGLESTRIP | DRAW_INDEXED8,
		DRAW_INDEXEDTRIANGLEFAN8   = DRAW_TRIANGLEFAN   | DRAW_INDEXED8,

		DRAW_INDEXEDPOINTLIST16 = DRAW_POINTLIST | DRAW_INDEXED16,
		DRAW_INDEXEDLINELIST16  = DRAW_LINELIST  | DRAW_INDEXED16,
		DRAW_INDEXEDLINESTRIP16 = DRAW_LINESTRIP | DRAW_INDEXED16,
		DRAW_INDEXEDLINELOOP16  = DRAW_LINELOOP  | DRAW_INDEXED16,
		DRAW_INDEXEDTRIANGLELIST16  = DRAW_TRIANGLELIST  | DRAW_INDEXED16,
		DRAW_INDEXEDTRIANGLESTRIP16 = DRAW_TRIANGLESTRIP | DRAW_INDEXED16,
		DRAW_INDEXEDTRIANGLEFAN16   = DRAW_TRIANGLEFAN   | DRAW_INDEXED16,

		DRAW_INDEXEDPOINTLIST32 = DRAW_POINTLIST | DRAW_INDEXED32,
		DRAW_INDEXEDLINELIST32  = DRAW_LINELIST  | DRAW_INDEXED32,
		DRAW_INDEXEDLINESTRIP32 = DRAW_LINESTRIP | DRAW_INDEXED32,
		DRAW_INDEXEDLINELOOP32  = DRAW_LINELOOP  | DRAW_INDEXED32,
		DRAW_INDEXEDTRIANGLELIST32  = DRAW_TRIANGLELIST  | DRAW_INDEXED32,
		DRAW_INDEXEDTRIANGLESTRIP32 = DRAW_TRIANGLESTRIP | DRAW_INDEXED32,
		DRAW_INDEXEDTRIANGLEFAN32   = DRAW_TRIANGLEFAN   | DRAW_INDEXED32,

		DRAW_LAST = DRAW_INDEXEDTRIANGLEFAN32
	};

	enum FillMode ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		FILL_SOLID,
		FILL_WIREFRAME,
		FILL_VERTEX,

		FILL_LAST = FILL_VERTEX
	};

	enum ShadingMode ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		SHADING_FLAT,
		SHADING_GOURAUD,

		SHADING_LAST = SHADING_GOURAUD
	};

	enum DepthCompareMode ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		DEPTH_ALWAYS,
		DEPTH_NEVER,
		DEPTH_EQUAL,
		DEPTH_NOTEQUAL,
		DEPTH_LESS,
		DEPTH_LESSEQUAL,
		DEPTH_GREATER,
		DEPTH_GREATEREQUAL,

		DEPTH_LAST = DEPTH_GREATEREQUAL
	};

	enum StencilCompareMode ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		STENCIL_ALWAYS,
		STENCIL_NEVER,
		STENCIL_EQUAL,
		STENCIL_NOTEQUAL,
		STENCIL_LESS,
		STENCIL_LESSEQUAL,
		STENCIL_GREATER,
		STENCIL_GREATEREQUAL,

		STENCIL_LAST = STENCIL_GREATEREQUAL
	};

	enum StencilOperation ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		OPERATION_KEEP,
		OPERATION_ZERO,
		OPERATION_REPLACE,
		OPERATION_INCRSAT,
		OPERATION_DECRSAT,
		OPERATION_INVERT,
		OPERATION_INCR,
		OPERATION_DECR,

		OPERATION_LAST = OPERATION_DECR
	};

	enum AlphaCompareMode ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		ALPHA_ALWAYS,
		ALPHA_NEVER,
		ALPHA_EQUAL,
		ALPHA_NOTEQUAL,
		ALPHA_LESS,
		ALPHA_LESSEQUAL,
		ALPHA_GREATER,
		ALPHA_GREATEREQUAL,

		ALPHA_LAST = ALPHA_GREATEREQUAL
	};

	enum CullMode ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		CULL_NONE,
		CULL_CLOCKWISE,
		CULL_COUNTERCLOCKWISE,

		CULL_LAST = CULL_COUNTERCLOCKWISE
	};

	enum BlendFactor ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		BLEND_ZERO,
		BLEND_ONE,
		BLEND_SOURCE,
		BLEND_INVSOURCE,
		BLEND_DEST,
		BLEND_INVDEST,
		BLEND_SOURCEALPHA,
		BLEND_INVSOURCEALPHA,
		BLEND_DESTALPHA,
		BLEND_INVDESTALPHA,
		BLEND_SRCALPHASAT,
		BLEND_CONSTANT,
		BLEND_INVCONSTANT,
		BLEND_CONSTANTALPHA,
		BLEND_INVCONSTANTALPHA,

		BLEND_LAST = BLEND_INVCONSTANTALPHA
	};

	enum BlendOperation ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		BLENDOP_ADD,
		BLENDOP_SUB,
		BLENDOP_INVSUB,
		BLENDOP_MIN,
		BLENDOP_MAX,

		BLENDOP_SOURCE,   // Copy source
		BLENDOP_DEST,     // Copy dest
		BLENDOP_NULL,     // Nullify result

		BLENDOP_LAST = BLENDOP_NULL
	};

	enum LogicalOperation ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		LOGICALOP_CLEAR,
		LOGICALOP_SET,
		LOGICALOP_COPY,
		LOGICALOP_COPY_INVERTED,
		LOGICALOP_NOOP,
		LOGICALOP_INVERT,
		LOGICALOP_AND,
		LOGICALOP_NAND,
		LOGICALOP_OR,
		LOGICALOP_NOR,
		LOGICALOP_XOR,
		LOGICALOP_EQUIV,
		LOGICALOP_AND_REVERSE,
		LOGICALOP_AND_INVERTED,
		LOGICALOP_OR_REVERSE,
		LOGICALOP_OR_INVERTED,

		LOGICALOP_LAST = LOGICALOP_OR_INVERTED
	};

	enum MaterialSource ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		MATERIAL_MATERIAL,
		MATERIAL_COLOR1,
		MATERIAL_COLOR2,

		MATERIAL_LAST = MATERIAL_COLOR2
	};

	enum FogMode ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		FOG_NONE,
		FOG_LINEAR,
		FOG_EXP,
		FOG_EXP2,

		FOG_LAST = FOG_EXP2
	};

	enum TexGen ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		TEXGEN_PASSTHRU,
		TEXGEN_NORMAL,
		TEXGEN_POSITION,
		TEXGEN_REFLECTION,
		TEXGEN_SPHEREMAP,
		TEXGEN_NONE,

		TEXGEN_LAST = TEXGEN_NONE
	};

	enum TransparencyAntialiasing ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		TRANSPARENCY_NONE,
		TRANSPARENCY_ALPHA_TO_COVERAGE,

		TRANSPARENCY_LAST = TRANSPARENCY_ALPHA_TO_COVERAGE
	};

	class Context
	{
	public:
		Context();

		~Context();

		void *operator new(size_t bytes);
		void operator delete(void *pointer, size_t bytes);

		bool isDrawPoint(bool fillModeAware = false) const;
		bool isDrawLine(bool fillModeAware = false) const;
		bool isDrawTriangle(bool fillModeAware = false) const;

		void init();

		const float &exp2Bias();   // NOTE: Needs address for JIT

		const Point &getLightPosition(int light);

		void setGlobalMipmapBias(float bias);

		// Set fixed-function vertex pipeline states
		void setLightingEnable(bool lightingEnable);
		void setSpecularEnable(bool specularEnable);
		void setLightEnable(int light, bool lightEnable);
		void setLightPosition(int light, Point worldLightPosition);

		void setColorVertexEnable(bool colorVertexEnable);
		void setAmbientMaterialSource(MaterialSource ambientMaterialSource);
		void setDiffuseMaterialSource(MaterialSource diffuseMaterialSource);
		void setSpecularMaterialSource(MaterialSource specularMaterialSource);
		void setEmissiveMaterialSource(MaterialSource emissiveMaterialSource);

		void setPointSpriteEnable(bool pointSpriteEnable);
		void setPointScaleEnable(bool pointScaleEnable);

		// Set fixed-function pixel pipeline states, return true when modified
		bool setDepthBufferEnable(bool depthBufferEnable);

		bool setAlphaBlendEnable(bool alphaBlendEnable);
		bool setSourceBlendFactor(BlendFactor sourceBlendFactor);
		bool setDestBlendFactor(BlendFactor destBlendFactor);
		bool setBlendOperation(BlendOperation blendOperation);

		bool setSeparateAlphaBlendEnable(bool separateAlphaBlendEnable);
		bool setSourceBlendFactorAlpha(BlendFactor sourceBlendFactorAlpha);
		bool setDestBlendFactorAlpha(BlendFactor destBlendFactorAlpha);
		bool setBlendOperationAlpha(BlendOperation blendOperationAlpha);

		bool setColorWriteMask(int index, int colorWriteMask);
		bool setWriteSRGB(bool sRGB);

		bool setColorLogicOpEnabled(bool colorLogicOpEnabled);
		bool setLogicalOperation(LogicalOperation logicalOperation);

		// Active fixed-function pixel pipeline states
		bool fogActive();
		bool pointSizeActive();
		FogMode pixelFogActive();
		bool depthWriteActive();
		bool alphaTestActive();
		bool depthBufferActive();
		bool stencilActive();

		bool perspectiveActive();

		// Active fixed-function vertex pipeline states
		bool vertexLightingActive();
		bool texCoordActive(int coordinate, int component);
		bool texCoordActive(int coordinate);
		bool isProjectionComponent(unsigned int coordinate, int component);
		bool vertexSpecularInputActive();
		bool vertexSpecularActive();
		bool vertexNormalActive();
		bool vertexLightActive();
		bool vertexLightActive(int i);
		MaterialSource vertexDiffuseMaterialSourceActive();
		MaterialSource vertexSpecularMaterialSourceActive();
		MaterialSource vertexAmbientMaterialSourceActive();
		MaterialSource vertexEmissiveMaterialSourceActive();

		bool pointSpriteActive();
		bool pointScaleActive();

		bool alphaBlendActive();
		BlendFactor sourceBlendFactor();
		BlendFactor destBlendFactor();
		BlendOperation blendOperation();

		BlendFactor sourceBlendFactorAlpha();
		BlendFactor destBlendFactorAlpha();
		BlendOperation blendOperationAlpha();

		LogicalOperation colorLogicOp();
		LogicalOperation indexLogicOp();

		bool indexedVertexBlendActive();
		int vertexBlendMatrixCountActive();
		bool localViewerActive();
		bool normalizeNormalsActive();
		FogMode vertexFogModeActive();
		bool rangeFogActive();

		TexGen texGenActive(int stage);
		int textureTransformCountActive(int stage);
		int texCoordIndexActive(int stage);

		// Active context states
		bool diffuseUsed();     // Used by pixel processor but not provided by vertex processor
		bool diffuseUsed(int component);     // Used by pixel processor but not provided by vertex processor
		bool diffuseActive();
		bool diffuseActive(int component);
		bool specularUsed();
		bool specularUsed(int component);
		bool specularActive();
		bool specularActive(int component);
		bool colorActive(int color, int component);
		bool textureActive();
		bool textureActive(int coordinate);
		bool textureActive(int coordinate, int component);

		unsigned short pixelShaderModel() const;
		unsigned short vertexShaderModel() const;

		int getMultiSampleCount() const;
		int getSuperSampleCount() const;

		DrawType drawType;

		bool stencilEnable;
		StencilCompareMode stencilCompareMode;
		int stencilReference;
		int stencilMask;
		StencilOperation stencilFailOperation;
		StencilOperation stencilPassOperation;
		StencilOperation stencilZFailOperation;
		int stencilWriteMask;

		bool twoSidedStencil;
		StencilCompareMode stencilCompareModeCCW;
		int stencilReferenceCCW;
		int stencilMaskCCW;
		StencilOperation stencilFailOperationCCW;
		StencilOperation stencilPassOperationCCW;
		StencilOperation stencilZFailOperationCCW;
		int stencilWriteMaskCCW;

		// Pixel processor states
		AlphaCompareMode alphaCompareMode;
		bool alphaTestEnable;
		FillMode fillMode;
		ShadingMode shadingMode;

		CullMode cullMode;
		float alphaReference;

		float depthBias;
		float slopeDepthBias;

		TextureStage textureStage[8];
		Sampler sampler[TOTAL_IMAGE_UNITS];

		Format renderTargetInternalFormat(int index);
		int colorWriteActive();
		int colorWriteActive(int index);
		bool colorUsed();

		Resource *texture[TOTAL_IMAGE_UNITS];
		Stream input[MAX_VERTEX_INPUTS];
		Resource *indexBuffer;

		bool preTransformed;   // FIXME: Private

		float fogStart;
		float fogEnd;

		void computeIllumination();

		bool textureWrapActive;
		unsigned char textureWrap[TEXTURE_IMAGE_UNITS];
		TexGen texGen[8];
		bool localViewer;
		bool normalizeNormals;
		int textureTransformCount[8];
		bool textureTransformProject[8];

		Surface *renderTarget[RENDERTARGETS];
		unsigned int renderTargetLayer[RENDERTARGETS];
		Surface *depthBuffer;
		unsigned int depthBufferLayer;
		Surface *stencilBuffer;
		unsigned int stencilBufferLayer;

		// Fog
		bool fogEnable;
		FogMode pixelFogMode;
		FogMode vertexFogMode;
		bool wBasedFog;
		bool rangeFogEnable;

		// Vertex blending
		bool indexedVertexBlendEnable;
		int vertexBlendMatrixCount;

		// Shaders
		const PixelShader *pixelShader;
		const VertexShader *vertexShader;

		// Global mipmap bias
		float bias;

		// Instancing
		int instanceID;

		// Fixed-function vertex pipeline state
		bool lightingEnable;
		bool specularEnable;
		bool lightEnable[8];
		Point worldLightPosition[8];

		MaterialSource ambientMaterialSource;
		MaterialSource diffuseMaterialSource;
		MaterialSource specularMaterialSource;
		MaterialSource emissiveMaterialSource;
		bool colorVertexEnable;

		bool occlusionEnabled;
		bool transformFeedbackQueryEnabled;
		uint64_t transformFeedbackEnabled;

		// Pixel processor states
		bool rasterizerDiscard;
		bool depthBufferEnable;
		DepthCompareMode depthCompareMode;
		bool depthWriteEnable;

		bool alphaBlendEnable;
		BlendFactor sourceBlendFactorState;
		BlendFactor destBlendFactorState;
		BlendOperation blendOperationState;

		bool separateAlphaBlendEnable;
		BlendFactor sourceBlendFactorStateAlpha;
		BlendFactor destBlendFactorStateAlpha;
		BlendOperation blendOperationStateAlpha;

		bool pointSpriteEnable;
		bool pointScaleEnable;
		float lineWidth;

		int colorWriteMask[RENDERTARGETS];   // RGBA
		bool writeSRGB;
		unsigned int sampleMask;
		unsigned int multiSampleMask;

		bool colorLogicOpEnabled;
		LogicalOperation logicalOperation;
	};
}

#endif   // sw_Context_hpp
