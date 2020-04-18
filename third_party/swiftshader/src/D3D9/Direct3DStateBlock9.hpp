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

#ifndef D3D9_Direct3DStateBlock9_hpp
#define D3D9_Direct3DStateBlock9_hpp

#include "Direct3DDevice9.hpp"
#include "Unknown.hpp"

#include <vector>

#include <d3d9.h>

namespace D3D9
{
	class Direct3DDevice9;
	class Direct3DVertexDeclaration9;
	class Direct3DIndexBuffer9;
	class Direct3DVertexBuffer9;
	class Direct3DBaseTexture9;
	class Direct3DPixelShader9;
	class Direct3DVertexShader9;

	class Direct3DStateBlock9 : public IDirect3DStateBlock9, public Unknown
	{
	public:
		Direct3DStateBlock9(Direct3DDevice9 *device, D3DSTATEBLOCKTYPE type);

		~Direct3DStateBlock9() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DStateBlock9 methods
		long __stdcall Apply() override;
		long __stdcall Capture() override;
		long __stdcall GetDevice(IDirect3DDevice9 **device) override;

		// Internal methods
		void lightEnable(unsigned long index, int enable);
		void setClipPlane(unsigned long index, const float *plane);
		void setCurrentTexturePalette(unsigned int paletteNumber);
		void setFVF(unsigned long FVF);
		void setIndices(Direct3DIndexBuffer9 *indexData);
		void setLight(unsigned long index, const D3DLIGHT9 *light);
		void setMaterial(const D3DMATERIAL9 *material);
		void setNPatchMode(float segments);
		void setPixelShader(Direct3DPixelShader9 *shader);
		void setPixelShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count);
		void setPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count);
		void setPixelShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count);
		void setRenderState(D3DRENDERSTATETYPE state, unsigned long value);
		void setSamplerState(unsigned long index, D3DSAMPLERSTATETYPE state, unsigned long value);
		void setScissorRect(const RECT *rect);
		void setStreamSource(unsigned int stream, Direct3DVertexBuffer9 *data, unsigned int offset, unsigned int stride);
		void setStreamSourceFreq(unsigned int streamNumber, unsigned int divider);
		void setTexture(unsigned long index, Direct3DBaseTexture9 *texture);
		void setTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value);
		void setTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix);
		void setViewport(const D3DVIEWPORT9 *viewport);
		void setVertexDeclaration(Direct3DVertexDeclaration9 *declaration);
		void setVertexShader(Direct3DVertexShader9 *shader);
		void setVertexShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count);
		void setVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count);
		void setVertexShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count);

	private:
		// Individual states
		void captureRenderState(D3DRENDERSTATETYPE state);
		void captureSamplerState(unsigned long index, D3DSAMPLERSTATETYPE state);
		void captureTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type);
		void captureTransform(D3DTRANSFORMSTATETYPE state);

		// Pixel states
		void capturePixelRenderStates();
		void capturePixelTextureStates();
		void capturePixelSamplerStates();
		void capturePixelShaderStates();

		// Vertex states
		void captureVertexRenderStates();
		void captureVertexSamplerStates();
		void captureVertexTextureStates();
		void captureNPatchMode();
		void captureLightStates();
		void captureVertexShaderStates();
		void captureStreamSourceFrequencies();
		void captureVertexDeclaration();
		void captureFVF();

		// All (remaining) states
		void captureTextures();
		void captureTexturePalette();
		void captureVertexStreams();
		void captureIndexBuffer();
		void captureViewport();
		void captureScissorRectangle();
		void captureTransforms();
		void captureTextureTransforms();
		void captureClippingPlanes();
		void captureMaterial();

		// Creation parameters
		Direct3DDevice9 *const device;
		const D3DSTATEBLOCKTYPE type;

		// State data
		bool vertexDeclarationCaptured;
		Direct3DVertexDeclaration9 *vertexDeclaration;

		bool fvfCaptured;
		unsigned long FVF;

		bool indexBufferCaptured;
		Direct3DIndexBuffer9 *indexBuffer;

		bool renderStateCaptured[D3DRS_BLENDOPALPHA + 1];
		unsigned long renderState[D3DRS_BLENDOPALPHA + 1];

		bool nPatchModeCaptured;
		float nPatchMode;

		bool textureStageStateCaptured[8][D3DTSS_CONSTANT + 1];
		unsigned long textureStageState[8][D3DTSS_CONSTANT + 1];

		bool samplerStateCaptured[16 + 4][D3DSAMP_DMAPOFFSET + 1];
		unsigned long samplerState[16 + 4][D3DSAMP_DMAPOFFSET + 1];

		bool streamSourceCaptured[MAX_VERTEX_INPUTS];
		struct StreamSource
		{
			Direct3DVertexBuffer9 *vertexBuffer;
			unsigned int offset;
			unsigned int stride;
		};
		StreamSource streamSource[MAX_VERTEX_INPUTS];

		bool streamSourceFrequencyCaptured[MAX_VERTEX_INPUTS];
		unsigned int streamSourceFrequency[MAX_VERTEX_INPUTS];

		bool textureCaptured[16 + 4];
		Direct3DBaseTexture9 *texture[16 + 4];

		bool transformCaptured[512];
		D3DMATRIX transform[512];

		bool materialCaptured;
		D3DMATERIAL9 material;

		bool lightCaptured[8];   // FIXME: Unlimited index
		D3DLIGHT9 light[8];

		bool lightEnableCaptured[8];   // FIXME: Unlimited index
		int lightEnableState[8];

		bool pixelShaderCaptured;
		Direct3DPixelShader9 *pixelShader;

		bool vertexShaderCaptured;
		Direct3DVertexShader9 *vertexShader;

		bool viewportCaptured;
		D3DVIEWPORT9 viewport;

		float pixelShaderConstantF[MAX_PIXEL_SHADER_CONST][4];
		int pixelShaderConstantI[16][4];
		int pixelShaderConstantB[16];

		float vertexShaderConstantF[MAX_VERTEX_SHADER_CONST][4];
		int vertexShaderConstantI[16][4];
		int vertexShaderConstantB[16];

		bool clipPlaneCaptured[6];
		float clipPlane[6][4];

		bool scissorRectCaptured;
		RECT scissorRect;

		bool paletteNumberCaptured;
		unsigned int paletteNumber;

		void clear();
	};
}

#endif   // D3D9_Direct3DStateBlock9_hpp
