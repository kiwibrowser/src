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

#ifndef D3D8_Direct3DStateBlock8_hpp
#define D3D8_Direct3DStateBlock8_hpp

#include "Unknown.hpp"

#include <vector>

#include <d3d8.h>

namespace D3D8
{
	class Direct3DDevice8;
	class Direct3DBaseTexture8;
	class Direct3DVertexBuffer8;
	class Direct3DIndexBuffer8;

	class Direct3DStateBlock8 : public Unknown
	{
	public:
		Direct3DStateBlock8(Direct3DDevice8 *device, D3DSTATEBLOCKTYPE type);

		~Direct3DStateBlock8() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		long __stdcall Apply();
		long __stdcall Capture();
		long __stdcall GetDevice(IDirect3DDevice8 **device);

		// Internal methods
		void lightEnable(unsigned long index, int enable);
		void setClipPlane(unsigned long index, const float *plane);
		void setCurrentTexturePalette(unsigned int paletteNumber);
		void setFVF(unsigned long FVF);
		void setIndices(Direct3DIndexBuffer8 *indexData, unsigned int baseVertexIndex);
		void setLight(unsigned long index, const D3DLIGHT8 *light);
		void setMaterial(const D3DMATERIAL8 *material);
		void setPixelShader(unsigned long shaderHandle);
		void setPixelShaderConstant(unsigned int startRegister, const void *constantData, unsigned int count);
		void setRenderState(D3DRENDERSTATETYPE state, unsigned long value);
		void setScissorRect(const RECT *rect);
		void setStreamSource(unsigned int stream, Direct3DVertexBuffer8 *data, unsigned int stride);
		void setTexture(unsigned long stage, Direct3DBaseTexture8 *texture);
		void setTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value);
		void setTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix);
		void setViewport(const D3DVIEWPORT8 *viewport);
		void setVertexShader(unsigned long shaderHandle);
		void setVertexShaderConstant(unsigned int startRegister, const void *constantData, unsigned int count);

	private:
		// Individual states
		void captureRenderState(D3DRENDERSTATETYPE state);
		void captureTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type);
		void captureTransform(D3DTRANSFORMSTATETYPE state);

		// Pixel states
		void capturePixelRenderStates();
		void capturePixelTextureStates();
		void capturePixelShaderStates();

		// Vertex states
		void captureVertexRenderStates();
		void captureVertexTextureStates();
		void captureLightStates();
		void captureVertexShaderStates();

		// All (remaining) states
		void captureTextures();
		void captureVertexTextures();
		void captureDisplacementTextures();
		void captureTexturePalette();
		void captureVertexStreams();
		void captureIndexBuffer();
		void captureViewport();
		void captureTransforms();
		void captureTextureTransforms();
		void captureClippingPlanes();
		void captureMaterial();

		// Creation parameters
		Direct3DDevice8 *const device;
		const D3DSTATEBLOCKTYPE type;

		// State data
		bool vertexShaderCaptured;
		unsigned long vertexShaderHandle;

		bool pixelShaderCaptured;
		unsigned long pixelShaderHandle;

		bool indexBufferCaptured;
		Direct3DIndexBuffer8 *indexBuffer;
		unsigned int baseVertexIndex;

		bool renderStateCaptured[D3DRS_NORMALORDER + 1];
		unsigned long renderState[D3DRS_NORMALORDER + 1];

		bool textureStageStateCaptured[8][D3DTSS_RESULTARG + 1];
		unsigned long textureStageState[8][D3DTSS_RESULTARG + 1];

		bool streamSourceCaptured[16];
		struct StreamSource
		{
			Direct3DVertexBuffer8 *vertexBuffer;
			unsigned int stride;
		};
		StreamSource streamSource[16];

		bool textureCaptured[8];
		Direct3DBaseTexture8 *texture[8];

		bool transformCaptured[512];
		D3DMATRIX transform[512];

		bool viewportCaptured;
		D3DVIEWPORT8 viewport;

		bool clipPlaneCaptured[6];
		float clipPlane[6][4];

		bool materialCaptured;
		D3DMATERIAL8 material;

		bool lightCaptured[8];   // FIXME: Unlimited index
		D3DLIGHT8 light[8];

		bool lightEnableCaptured[8];   // FIXME: Unlimited index
		int lightEnableState[8];

		float pixelShaderConstant[8][4];
		float vertexShaderConstant[256][4];

		bool scissorRectCaptured;
		RECT scissorRect;

		bool paletteNumberCaptured;
		unsigned int paletteNumber;

		void clear();
	};
}

#endif   // D3D8_Direct3DStateBlock8_hpp