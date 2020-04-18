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

#include "Direct3DStateBlock9.hpp"

#include "Direct3DDevice9.hpp"
#include "Direct3DVertexDeclaration9.hpp"
#include "Direct3DIndexBuffer9.hpp"
#include "Direct3DVertexBuffer9.hpp"
#include "Direct3DBaseTexture9.hpp"
#include "Direct3DPixelShader9.hpp"
#include "Direct3DVertexShader9.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DStateBlock9::Direct3DStateBlock9(Direct3DDevice9 *device, D3DSTATEBLOCKTYPE type) : device(device), type(type)
	{
		vertexDeclaration = 0;

		indexBuffer = 0;

		for(int stream = 0; stream < MAX_VERTEX_INPUTS; stream++)
		{
			streamSource[stream].vertexBuffer = 0;
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			texture[sampler] = 0;
		}

		pixelShader = 0;
		vertexShader = 0;

		clear();

		if(type == D3DSBT_PIXELSTATE || type == D3DSBT_ALL)
		{
			capturePixelRenderStates();
			capturePixelTextureStates();
			capturePixelSamplerStates();
			capturePixelShaderStates();
		}

		if(type == D3DSBT_VERTEXSTATE || type == D3DSBT_ALL)
		{
			captureVertexRenderStates();
			captureVertexSamplerStates();
			captureVertexTextureStates();
			captureNPatchMode();
			captureLightStates();
			captureVertexShaderStates();
			captureStreamSourceFrequencies();
			captureFVF();
			captureVertexDeclaration();
		}

		if(type == D3DSBT_ALL)   // Capture remaining states
		{
			captureTextures();
			captureTexturePalette();
			captureVertexStreams();
			captureIndexBuffer();
			captureViewport();
			captureScissorRectangle();
			captureTransforms();
			captureTextureTransforms();
			captureClippingPlanes();
			captureMaterial();
		}
	}

	Direct3DStateBlock9::~Direct3DStateBlock9()
	{
		clear();
	}

	long Direct3DStateBlock9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DStateBlock9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DStateBlock9::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3DStateBlock9::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DStateBlock9::Apply()
	{
		CriticalSection cs(device);

		TRACE("");

		if(device->isRecording())
		{
			return INVALIDCALL();
		}

		if(fvfCaptured)
		{
			device->SetFVF(FVF);
		}

		if(vertexDeclarationCaptured)
		{
			device->SetVertexDeclaration(vertexDeclaration);
		}

		if(indexBufferCaptured)
		{
			device->SetIndices(indexBuffer);
		}

		for(int state = D3DRS_ZENABLE; state <= D3DRS_BLENDOPALPHA; state++)
		{
			if(renderStateCaptured[state])
			{
				device->SetRenderState((D3DRENDERSTATETYPE)state, renderState[state]);
			}
		}

		if(nPatchModeCaptured)
		{
			device->SetNPatchMode(nPatchMode);
		}

		for(int stage = 0; stage < 8; stage++)
		{
			for(int state = D3DTSS_COLOROP; state <= D3DTSS_CONSTANT; state++)
			{
				if(textureStageStateCaptured[stage][state])
				{
					device->SetTextureStageState(stage, (D3DTEXTURESTAGESTATETYPE)state, textureStageState[stage][state]);
				}
			}
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			for(int state = D3DSAMP_ADDRESSU; state <= D3DSAMP_DMAPOFFSET; state++)
			{
				if(samplerStateCaptured[sampler][state])
				{
					int index = sampler < 16 ? sampler : D3DVERTEXTEXTURESAMPLER0 + (sampler - 16);
					device->SetSamplerState(index, (D3DSAMPLERSTATETYPE)state, samplerState[sampler][state]);
				}
			}
		}

		for(int stream = 0; stream < MAX_VERTEX_INPUTS; stream++)
		{
			if(streamSourceCaptured[stream])
			{
				device->SetStreamSource(stream, streamSource[stream].vertexBuffer, streamSource[stream].offset, streamSource[stream].stride);
			}

			if(streamSourceFrequencyCaptured[stream])
			{
				device->SetStreamSourceFreq(stream, streamSourceFrequency[stream]);
			}
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			if(textureCaptured[sampler])
			{
				int index = sampler < 16 ? sampler : D3DVERTEXTEXTURESAMPLER0 + (sampler - 16);
				device->SetTexture(index, texture[sampler]);
			}
		}

		for(int state = 0; state < 512; state++)
		{
			if(transformCaptured[state])
			{
				device->SetTransform((D3DTRANSFORMSTATETYPE)state, &transform[state]);
			}
		}

		if(materialCaptured)
		{
			device->SetMaterial(&material);
		}

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			if(lightCaptured[index])
			{
				device->SetLight(index, &light[index]);
			}
		}

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			if(lightEnableCaptured[index])
			{
				device->LightEnable(index, lightEnableState[index]);
			}
		}

		if(pixelShaderCaptured)
		{
			device->SetPixelShader(pixelShader);
		}

		if(vertexShaderCaptured)
		{
			device->SetVertexShader(vertexShader);
		}

		if(viewportCaptured)
		{
			device->SetViewport(&viewport);
		}

		for(int i = 0; i < MAX_PIXEL_SHADER_CONST; i++)
		{
			if(*(int*)pixelShaderConstantF[i] != 0x80000000)
			{
				device->SetPixelShaderConstantF(i, pixelShaderConstantF[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(pixelShaderConstantI[i][0] != 0x80000000)
			{
				device->SetPixelShaderConstantI(i, pixelShaderConstantI[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(pixelShaderConstantB[i] != 0x80000000)
			{
				device->SetPixelShaderConstantB(i, &pixelShaderConstantB[i], 1);
			}
		}

		for(int i = 0; i < MAX_VERTEX_SHADER_CONST; i++)
		{
			if(*(int*)vertexShaderConstantF[i] != 0x80000000)
			{
				device->SetVertexShaderConstantF(i, vertexShaderConstantF[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(vertexShaderConstantI[i][0] != 0x80000000)
			{
				device->SetVertexShaderConstantI(i, vertexShaderConstantI[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(vertexShaderConstantB[i] != 0x80000000)
			{
				device->SetVertexShaderConstantB(i, &vertexShaderConstantB[i], 1);
			}
		}

		for(int index = 0; index < 6; index++)
		{
			if(clipPlaneCaptured[index])
			{
				device->SetClipPlane(index, clipPlane[index]);
			}
		}

		if(scissorRectCaptured)
		{
			device->SetScissorRect(&scissorRect);
		}

		if(paletteNumberCaptured)
		{
			device->SetCurrentTexturePalette(paletteNumber);
		}

		return D3D_OK;
	}

	long Direct3DStateBlock9::Capture()
	{
		CriticalSection cs(device);

		TRACE("");

		if(fvfCaptured)
		{
			device->GetFVF(&FVF);
		}

		if(vertexDeclarationCaptured)
		{
			Direct3DVertexDeclaration9 *vertexDeclaration;
			device->GetVertexDeclaration(reinterpret_cast<IDirect3DVertexDeclaration9**>(&vertexDeclaration));

			if(vertexDeclaration)
			{
				vertexDeclaration->bind();
				vertexDeclaration->Release();
			}

			if(this->vertexDeclaration)
			{
				this->vertexDeclaration->unbind();
			}

			this->vertexDeclaration = vertexDeclaration;
		}

		if(indexBufferCaptured)
		{
			Direct3DIndexBuffer9 *indexBuffer;
			device->GetIndices(reinterpret_cast<IDirect3DIndexBuffer9**>(&indexBuffer));

			if(indexBuffer)
			{
				indexBuffer->bind();
				indexBuffer->Release();
			}

			if(this->indexBuffer)
			{
				this->indexBuffer->unbind();
			}

			this->indexBuffer = indexBuffer;
		}

		for(int state = 0; state < D3DRS_BLENDOPALPHA + 1; state++)
		{
			if(renderStateCaptured[state])
			{
				device->GetRenderState((D3DRENDERSTATETYPE)state, &renderState[state]);
			}
		}

		if(nPatchModeCaptured)
		{
			nPatchMode = device->GetNPatchMode();
		}

		for(int stage = 0; stage < 8; stage++)
		{
			for(int state = 0; state < D3DTSS_CONSTANT + 1; state++)
			{
				if(textureStageStateCaptured[stage][state])
				{
					device->GetTextureStageState(stage, (D3DTEXTURESTAGESTATETYPE)state, &textureStageState[stage][state]);
				}
			}
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			for(int state = 0; state < D3DSAMP_DMAPOFFSET + 1; state++)
			{
				if(samplerStateCaptured[sampler][state])
				{
					int index = sampler < 16 ? sampler : D3DVERTEXTEXTURESAMPLER0 + (sampler - 16);
					device->GetSamplerState(index, (D3DSAMPLERSTATETYPE)state, &samplerState[sampler][state]);
				}
			}
		}

		for(int stream = 0; stream < MAX_VERTEX_INPUTS; stream++)
		{
			if(streamSourceCaptured[stream])
			{
				Direct3DVertexBuffer9 *vertexBuffer;
				device->GetStreamSource(stream, reinterpret_cast<IDirect3DVertexBuffer9**>(&vertexBuffer), &streamSource[stream].offset, &streamSource[stream].stride);

				if(vertexBuffer)
				{
					vertexBuffer->bind();
					vertexBuffer->Release();
				}

				if(streamSource[stream].vertexBuffer)
				{
					streamSource[stream].vertexBuffer->unbind();
				}

				streamSource[stream].vertexBuffer = vertexBuffer;
			}

			if(streamSourceFrequencyCaptured[stream])
			{
				device->GetStreamSourceFreq(stream, &streamSourceFrequency[stream]);
			}
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			if(textureCaptured[sampler])
			{
				Direct3DBaseTexture9 *texture;
				int index = sampler < 16 ? sampler : D3DVERTEXTEXTURESAMPLER0 + (sampler - 16);
				device->GetTexture(index, reinterpret_cast<IDirect3DBaseTexture9**>(&texture));

				if(texture)
				{
					texture->bind();
					texture->Release();
				}

				if(this->texture[sampler])
				{
					this->texture[sampler]->unbind();
				}

				this->texture[sampler] = texture;
			}
		}

		for(int state = 0; state < 512; state++)
		{
			if(transformCaptured[state])
			{
				device->GetTransform((D3DTRANSFORMSTATETYPE)state, &transform[state]);
			}
		}

		if(materialCaptured)
		{
			device->GetMaterial(&material);
		}

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			if(lightCaptured[index])
			{
				device->GetLight(index, &light[index]);
			}
		}

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			if(lightEnableCaptured[index])
			{
				lightEnableState[index] = false;
				device->GetLightEnable(index, &lightEnableState[index]);
			}
		}

		if(pixelShaderCaptured)
		{
			Direct3DPixelShader9 *pixelShader;
			device->GetPixelShader(reinterpret_cast<IDirect3DPixelShader9**>(&pixelShader));

			if(pixelShader)
			{
				pixelShader->bind();
				pixelShader->Release();
			}

			if(this->pixelShader)
			{
				this->pixelShader->unbind();
			}

			this->pixelShader = pixelShader;
		}

		if(vertexShaderCaptured)
		{
			Direct3DVertexShader9 *vertexShader;
			device->GetVertexShader(reinterpret_cast<IDirect3DVertexShader9**>(&vertexShader));

			if(vertexShader)
			{
				vertexShader->bind();
				vertexShader->Release();
			}

			if(this->vertexShader)
			{
				this->vertexShader->unbind();
			}

			this->vertexShader = vertexShader;
		}

		if(viewportCaptured)
		{
			device->GetViewport(&viewport);
		}

		for(int i = 0; i < MAX_PIXEL_SHADER_CONST; i++)
		{
			if(*(int*)pixelShaderConstantF[i] != 0x80000000)
			{
				device->GetPixelShaderConstantF(i, pixelShaderConstantF[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(pixelShaderConstantI[i][0] != 0x80000000)
			{
				device->GetPixelShaderConstantI(i, pixelShaderConstantI[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(pixelShaderConstantB[i] != 0x80000000)
			{
				device->GetPixelShaderConstantB(i, &pixelShaderConstantB[i], 1);
			}
		}

		for(int i = 0; i < MAX_VERTEX_SHADER_CONST; i++)
		{
			if(*(int*)vertexShaderConstantF[i] != 0x80000000)
			{
				device->GetVertexShaderConstantF(i, vertexShaderConstantF[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(vertexShaderConstantI[i][0] != 0x80000000)
			{
				device->GetVertexShaderConstantI(i, vertexShaderConstantI[i], 1);
			}
		}

		for(int i = 0; i < 16; i++)
		{
			if(vertexShaderConstantB[i] != 0x80000000)
			{
				device->GetVertexShaderConstantB(i, &vertexShaderConstantB[i], 1);
			}
		}

		for(int index = 0; index < 6; index++)
		{
			if(clipPlaneCaptured[index])
			{
				device->GetClipPlane(index, clipPlane[index]);
			}
		}

		if(scissorRectCaptured)
		{
			device->GetScissorRect(&scissorRect);
		}

		if(paletteNumberCaptured)
		{
			device->GetCurrentTexturePalette(&paletteNumber);
		}

		return D3D_OK;
	}

	long Direct3DStateBlock9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		if(!device)
		{
			return INVALIDCALL();
		}

		this->device->AddRef();
		*device = this->device;

		return D3D_OK;
	}

	void Direct3DStateBlock9::lightEnable(unsigned long index, int enable)
	{
		if(index < 8)
		{
			lightEnableCaptured[index] = true;
			lightEnableState[index] = enable;
		}
		else ASSERT(false);   // FIXME: Support unlimited index
	}

	void Direct3DStateBlock9::setClipPlane(unsigned long index, const float *plane)
	{
		clipPlaneCaptured[index] = true;
		clipPlane[index][0] = plane[0];
		clipPlane[index][1] = plane[1];
		clipPlane[index][2] = plane[2];
		clipPlane[index][3] = plane[3];
	}

	void Direct3DStateBlock9::setCurrentTexturePalette(unsigned int paletteNumber)
	{
		paletteNumberCaptured = true;
		this->paletteNumber = paletteNumber;
	}

	void Direct3DStateBlock9::setFVF(unsigned long FVF)
	{
		fvfCaptured = true;
		this->FVF = FVF;
	}

	void Direct3DStateBlock9::setIndices(Direct3DIndexBuffer9 *indexBuffer)
	{
		if(indexBuffer) indexBuffer->bind();
		if(this->indexBuffer) this->indexBuffer->unbind();

		indexBufferCaptured = true;
		this->indexBuffer = indexBuffer;
	}

	void Direct3DStateBlock9::setLight(unsigned long index, const D3DLIGHT9 *light)
	{
		if(index < 8)
		{
			lightCaptured[index] = true;
			this->light[index] = *light;
		}
		else ASSERT(false);   // FIXME: Support unlimited index
	}

	void Direct3DStateBlock9::setMaterial(const D3DMATERIAL9 *material)
	{
		materialCaptured = true;
		this->material = *material;
	}

	void Direct3DStateBlock9::setNPatchMode(float segments)
	{
		nPatchModeCaptured = true;
		nPatchMode = segments;
	}

	void Direct3DStateBlock9::setPixelShader(Direct3DPixelShader9 *pixelShader)
	{
		if(pixelShader) pixelShader->bind();
		if(this->pixelShader) this->pixelShader->unbind();

		pixelShaderCaptured = true;
		this->pixelShader = pixelShader;
	}

	void Direct3DStateBlock9::setPixelShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		memcpy(&pixelShaderConstantB[startRegister], constantData, count * sizeof(int));
	}

	void Direct3DStateBlock9::setPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		memcpy(pixelShaderConstantF[startRegister], constantData, count * sizeof(float[4]));
	}

	void Direct3DStateBlock9::setPixelShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		memcpy(pixelShaderConstantI[startRegister], constantData, count * sizeof(int[4]));
	}

	void Direct3DStateBlock9::setRenderState(D3DRENDERSTATETYPE state, unsigned long value)
	{
		renderStateCaptured[state] = true;
		renderState[state] = value;
	}

	void Direct3DStateBlock9::setSamplerState(unsigned long index, D3DSAMPLERSTATETYPE state, unsigned long value)
	{
		unsigned int sampler = index < 16 ? index : (index - D3DVERTEXTEXTURESAMPLER0) + 16;

		if(sampler >= 16 + 4)
		{
			return;
		}

		samplerStateCaptured[sampler][state] = true;
		samplerState[sampler][state] = value;
	}

	void Direct3DStateBlock9::setScissorRect(const RECT *rect)
	{
		scissorRectCaptured = true;
		scissorRect = *rect;
	}

	void Direct3DStateBlock9::setStreamSource(unsigned int stream, Direct3DVertexBuffer9 *vertexBuffer, unsigned int offset, unsigned int stride)
	{
		if(vertexBuffer) vertexBuffer->bind();
		if(streamSource[stream].vertexBuffer) streamSource[stream].vertexBuffer->unbind();

		streamSourceCaptured[stream] = true;
		streamSource[stream].vertexBuffer = vertexBuffer;
		streamSource[stream].offset = offset;
		streamSource[stream].stride = stride;
	}

	void Direct3DStateBlock9::setStreamSourceFreq(unsigned int streamNumber, unsigned int divider)
	{
		streamSourceFrequencyCaptured[streamNumber] = true;
		streamSourceFrequency[streamNumber] = divider;
	}

	void Direct3DStateBlock9::setTexture(unsigned long index, Direct3DBaseTexture9 *texture)
	{
		unsigned int sampler = index < 16 ? index : (index - D3DVERTEXTEXTURESAMPLER0) + 16;

		if(sampler >= 16 + 4)
		{
			return;
		}

		if(texture) texture->bind();
		if(this->texture[sampler]) this->texture[sampler]->unbind();

		textureCaptured[sampler] = true;
		this->texture[sampler] = texture;
	}

	void Direct3DStateBlock9::setTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value)
	{
		textureStageStateCaptured[stage][type] = true;
		textureStageState[stage][type] = value;
	}

	void Direct3DStateBlock9::setTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		transformCaptured[state] = true;
		transform[state] = *matrix;
	}

	void Direct3DStateBlock9::setViewport(const D3DVIEWPORT9 *viewport)
	{
		viewportCaptured = true;
		this->viewport = *viewport;
	}

	void Direct3DStateBlock9::setVertexDeclaration(Direct3DVertexDeclaration9 *vertexDeclaration)
	{
		if(vertexDeclaration) vertexDeclaration->bind();
		if(this->vertexDeclaration) this->vertexDeclaration->unbind();

		vertexDeclarationCaptured = true;
		this->vertexDeclaration = vertexDeclaration;
	}

	void Direct3DStateBlock9::setVertexShader(Direct3DVertexShader9 *vertexShader)
	{
		if(vertexShader) vertexShader->bind();
		if(this->vertexShader) this->vertexShader->unbind();

		vertexShaderCaptured = true;
		this->vertexShader = vertexShader;
	}

	void Direct3DStateBlock9::setVertexShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		memcpy(&vertexShaderConstantB[startRegister], constantData, count * sizeof(int));
	}

	void Direct3DStateBlock9::setVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		memcpy(vertexShaderConstantF[startRegister], constantData, count * sizeof(float[4]));
	}

	void Direct3DStateBlock9::setVertexShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		memcpy(vertexShaderConstantI[startRegister], constantData, count * sizeof(int[4]));
	}

	void Direct3DStateBlock9::clear()
	{
		// Erase capture flags
		fvfCaptured = false;
		vertexDeclarationCaptured = false;

		indexBufferCaptured = false;

		for(int state = 0; state < D3DRS_BLENDOPALPHA + 1; state++)
		{
			renderStateCaptured[state] = false;
		}

		nPatchModeCaptured = false;

		for(int stage = 0; stage < 8; stage++)
		{
			for(int state = 0; state < D3DTSS_CONSTANT + 1; state++)
			{
				textureStageStateCaptured[stage][state] = false;
			}
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			for(int state = 0; state < D3DSAMP_DMAPOFFSET + 1; state++)
			{
				samplerStateCaptured[sampler][state] = false;
			}
		}

		for(int stream = 0; stream < MAX_VERTEX_INPUTS; stream++)
		{
			streamSourceCaptured[stream] = false;
			streamSourceFrequencyCaptured[stream] = false;
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			textureCaptured[sampler] = false;
		}

		for(int state = 0; state < 512; state++)
		{
			transformCaptured[state] = false;
		}

		materialCaptured = false;

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			lightCaptured[index] = false;
		}

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			lightEnableCaptured[index] = false;
		}

		scissorRectCaptured = false;

		pixelShaderCaptured = false;
		vertexShaderCaptured = false;

		viewportCaptured = false;

		for(int i = 0; i < MAX_PIXEL_SHADER_CONST; i++)
		{
			(int&)pixelShaderConstantF[i][0] = 0x80000000;
			(int&)pixelShaderConstantF[i][1] = 0x80000000;
			(int&)pixelShaderConstantF[i][2] = 0x80000000;
			(int&)pixelShaderConstantF[i][3] = 0x80000000;
		}

		for(int i = 0; i < MAX_VERTEX_SHADER_CONST; i++)
		{
			(int&)vertexShaderConstantF[i][0] = 0x80000000;
			(int&)vertexShaderConstantF[i][1] = 0x80000000;
			(int&)vertexShaderConstantF[i][2] = 0x80000000;
			(int&)vertexShaderConstantF[i][3] = 0x80000000;
		}

		for(int i = 0; i < 16; i++)
		{
			pixelShaderConstantI[i][0] = 0x80000000;
			pixelShaderConstantI[i][1] = 0x80000000;
			pixelShaderConstantI[i][2] = 0x80000000;
			pixelShaderConstantI[i][3] = 0x80000000;

			pixelShaderConstantB[i] = 0x80000000;

			vertexShaderConstantI[i][0] = 0x80000000;
			vertexShaderConstantI[i][1] = 0x80000000;
			vertexShaderConstantI[i][2] = 0x80000000;
			vertexShaderConstantI[i][3] = 0x80000000;

			vertexShaderConstantB[i] = 0x80000000;
		}

		for(int index = 0; index < 6; index++)
		{
			clipPlaneCaptured[index] = false;
		}

		paletteNumberCaptured = false;

		// unbind resources
		if(vertexDeclaration)
		{
			vertexDeclaration->unbind();
			vertexDeclaration = 0;
		}

		if(indexBuffer)
		{
			indexBuffer->unbind();
			indexBuffer = 0;
		}

		for(int stream = 0; stream < MAX_VERTEX_INPUTS; stream++)
		{
			if(streamSource[stream].vertexBuffer)
			{
				streamSource[stream].vertexBuffer->unbind();
				streamSource[stream].vertexBuffer = 0;
			}
		}

		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			if(texture[sampler])
			{
				texture[sampler]->unbind();
				texture[sampler] = 0;
			}
		}

		if(pixelShader)
		{
			pixelShader->unbind();
			pixelShader = 0;
		}

		if(vertexShader)
		{
			vertexShader->unbind();
			vertexShader = 0;
		}
	}

	void Direct3DStateBlock9::captureRenderState(D3DRENDERSTATETYPE state)
	{
		device->GetRenderState(state, &renderState[state]);
		renderStateCaptured[state] = true;
	}

	void Direct3DStateBlock9::captureSamplerState(unsigned long index, D3DSAMPLERSTATETYPE state)
	{
		if(index < 16)
		{
			device->GetSamplerState(index, state, &samplerState[index][state]);
			samplerStateCaptured[index][state] = true;
		}
		else if(index >= D3DVERTEXTEXTURESAMPLER0)
		{
			unsigned int sampler = 16 + (index - D3DVERTEXTEXTURESAMPLER0);

			device->GetSamplerState(index, state, &samplerState[sampler][state]);
			samplerStateCaptured[sampler][state] = true;
		}
	}

	void Direct3DStateBlock9::captureTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type)
	{
		device->GetTextureStageState(stage, type, &textureStageState[stage][type]);
		textureStageStateCaptured[stage][type] = true;
	}

	void Direct3DStateBlock9::captureTransform(D3DTRANSFORMSTATETYPE state)
	{
		device->GetTransform(state, &transform[state]);
		transformCaptured[state] = true;
	}

	void Direct3DStateBlock9::capturePixelRenderStates()
	{
		captureRenderState(D3DRS_ZENABLE);
		captureRenderState(D3DRS_FILLMODE);
		captureRenderState(D3DRS_SHADEMODE);
		captureRenderState(D3DRS_ZWRITEENABLE);
		captureRenderState(D3DRS_ALPHATESTENABLE);
		captureRenderState(D3DRS_LASTPIXEL);
		captureRenderState(D3DRS_SRCBLEND);
		captureRenderState(D3DRS_DESTBLEND);
		captureRenderState(D3DRS_ZFUNC);
		captureRenderState(D3DRS_ALPHAREF);
		captureRenderState(D3DRS_ALPHAFUNC);
		captureRenderState(D3DRS_DITHERENABLE);
		captureRenderState(D3DRS_FOGSTART);
		captureRenderState(D3DRS_FOGEND);
		captureRenderState(D3DRS_FOGDENSITY);
		captureRenderState(D3DRS_ALPHABLENDENABLE);
		captureRenderState(D3DRS_DEPTHBIAS);
		captureRenderState(D3DRS_STENCILENABLE);
		captureRenderState(D3DRS_STENCILFAIL);
		captureRenderState(D3DRS_STENCILZFAIL);
		captureRenderState(D3DRS_STENCILPASS);
		captureRenderState(D3DRS_STENCILFUNC);
		captureRenderState(D3DRS_STENCILREF);
		captureRenderState(D3DRS_STENCILMASK);
		captureRenderState(D3DRS_STENCILWRITEMASK);
		captureRenderState(D3DRS_TEXTUREFACTOR);
		captureRenderState(D3DRS_WRAP0);
		captureRenderState(D3DRS_WRAP1);
		captureRenderState(D3DRS_WRAP2);
		captureRenderState(D3DRS_WRAP3);
		captureRenderState(D3DRS_WRAP4);
		captureRenderState(D3DRS_WRAP5);
		captureRenderState(D3DRS_WRAP6);
		captureRenderState(D3DRS_WRAP7);
		captureRenderState(D3DRS_WRAP8);
		captureRenderState(D3DRS_WRAP9);
		captureRenderState(D3DRS_WRAP10);
		captureRenderState(D3DRS_WRAP11);
		captureRenderState(D3DRS_WRAP12);
		captureRenderState(D3DRS_WRAP13);
		captureRenderState(D3DRS_WRAP14);
		captureRenderState(D3DRS_WRAP15);
		captureRenderState(D3DRS_COLORWRITEENABLE);
		captureRenderState(D3DRS_BLENDOP);
		captureRenderState(D3DRS_SCISSORTESTENABLE);
		captureRenderState(D3DRS_SLOPESCALEDEPTHBIAS);
		captureRenderState(D3DRS_ANTIALIASEDLINEENABLE);
		captureRenderState(D3DRS_TWOSIDEDSTENCILMODE);
		captureRenderState(D3DRS_CCW_STENCILFAIL);
		captureRenderState(D3DRS_CCW_STENCILZFAIL);
		captureRenderState(D3DRS_CCW_STENCILPASS);
		captureRenderState(D3DRS_CCW_STENCILFUNC);
		captureRenderState(D3DRS_COLORWRITEENABLE1);
		captureRenderState(D3DRS_COLORWRITEENABLE2);
		captureRenderState(D3DRS_COLORWRITEENABLE3);
		captureRenderState(D3DRS_BLENDFACTOR);
		captureRenderState(D3DRS_SRGBWRITEENABLE);
		captureRenderState(D3DRS_SEPARATEALPHABLENDENABLE);
		captureRenderState(D3DRS_SRCBLENDALPHA);
		captureRenderState(D3DRS_DESTBLENDALPHA);
		captureRenderState(D3DRS_BLENDOPALPHA);
	}

	void Direct3DStateBlock9::capturePixelTextureStates()
	{
		for(int stage = 0; stage < 8; stage++)
		{
			captureTextureStageState(stage, D3DTSS_COLOROP);
			captureTextureStageState(stage, D3DTSS_COLORARG1);
			captureTextureStageState(stage, D3DTSS_COLORARG2);
			captureTextureStageState(stage, D3DTSS_ALPHAOP);
			captureTextureStageState(stage, D3DTSS_ALPHAARG1);
			captureTextureStageState(stage, D3DTSS_ALPHAARG2);
			captureTextureStageState(stage, D3DTSS_BUMPENVMAT00);
			captureTextureStageState(stage, D3DTSS_BUMPENVMAT01);
			captureTextureStageState(stage, D3DTSS_BUMPENVMAT10);
			captureTextureStageState(stage, D3DTSS_BUMPENVMAT11);
			captureTextureStageState(stage, D3DTSS_TEXCOORDINDEX);
			captureTextureStageState(stage, D3DTSS_BUMPENVLSCALE);
			captureTextureStageState(stage, D3DTSS_BUMPENVLOFFSET);
			captureTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS);
			captureTextureStageState(stage, D3DTSS_COLORARG0);
			captureTextureStageState(stage, D3DTSS_ALPHAARG0);
			captureTextureStageState(stage, D3DTSS_RESULTARG);
		}
	}

	void Direct3DStateBlock9::capturePixelSamplerStates()
	{
		for(int sampler = 0; sampler <= D3DVERTEXTEXTURESAMPLER3; sampler++)
		{
			captureSamplerState(sampler, D3DSAMP_ADDRESSU);
			captureSamplerState(sampler, D3DSAMP_ADDRESSV);
			captureSamplerState(sampler, D3DSAMP_ADDRESSW);
			captureSamplerState(sampler, D3DSAMP_BORDERCOLOR);
			captureSamplerState(sampler, D3DSAMP_MAGFILTER);
			captureSamplerState(sampler, D3DSAMP_MINFILTER);
			captureSamplerState(sampler, D3DSAMP_MIPFILTER);
			captureSamplerState(sampler, D3DSAMP_MIPMAPLODBIAS);
			captureSamplerState(sampler, D3DSAMP_MAXMIPLEVEL);
			captureSamplerState(sampler, D3DSAMP_MAXANISOTROPY);
			captureSamplerState(sampler, D3DSAMP_SRGBTEXTURE);
			captureSamplerState(sampler, D3DSAMP_ELEMENTINDEX);
		}
	}

	void Direct3DStateBlock9::capturePixelShaderStates()
	{
		pixelShaderCaptured = true;
		device->GetPixelShader(reinterpret_cast<IDirect3DPixelShader9**>(&pixelShader));

		if(pixelShader)
		{
			pixelShader->bind();
			pixelShader->Release();
		}

		device->GetPixelShaderConstantF(0, pixelShaderConstantF[0], 32);
		device->GetPixelShaderConstantI(0, pixelShaderConstantI[0], 16);
		device->GetPixelShaderConstantB(0, pixelShaderConstantB, 16);
	}

	void Direct3DStateBlock9::captureVertexRenderStates()
	{
		captureRenderState(D3DRS_CULLMODE);
		captureRenderState(D3DRS_FOGENABLE);
		captureRenderState(D3DRS_FOGCOLOR);
		captureRenderState(D3DRS_FOGTABLEMODE);
		captureRenderState(D3DRS_FOGSTART);
		captureRenderState(D3DRS_FOGEND);
		captureRenderState(D3DRS_FOGDENSITY);
		captureRenderState(D3DRS_RANGEFOGENABLE);
		captureRenderState(D3DRS_AMBIENT);
		captureRenderState(D3DRS_COLORVERTEX);
		captureRenderState(D3DRS_FOGVERTEXMODE);
		captureRenderState(D3DRS_CLIPPING);
		captureRenderState(D3DRS_LIGHTING);
		captureRenderState(D3DRS_LOCALVIEWER);
		captureRenderState(D3DRS_EMISSIVEMATERIALSOURCE);
		captureRenderState(D3DRS_AMBIENTMATERIALSOURCE);
		captureRenderState(D3DRS_DIFFUSEMATERIALSOURCE);
		captureRenderState(D3DRS_SPECULARMATERIALSOURCE);
		captureRenderState(D3DRS_VERTEXBLEND);
		captureRenderState(D3DRS_CLIPPLANEENABLE);
		captureRenderState(D3DRS_POINTSIZE);
		captureRenderState(D3DRS_POINTSIZE_MIN);
		captureRenderState(D3DRS_POINTSPRITEENABLE);
		captureRenderState(D3DRS_POINTSCALEENABLE);
		captureRenderState(D3DRS_POINTSCALE_A);
		captureRenderState(D3DRS_POINTSCALE_B);
		captureRenderState(D3DRS_POINTSCALE_C);
		captureRenderState(D3DRS_MULTISAMPLEANTIALIAS);
		captureRenderState(D3DRS_MULTISAMPLEMASK);
		captureRenderState(D3DRS_PATCHEDGESTYLE);
		captureRenderState(D3DRS_POINTSIZE_MAX);
		captureRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE);
		captureRenderState(D3DRS_TWEENFACTOR);
		captureRenderState(D3DRS_POSITIONDEGREE);
		captureRenderState(D3DRS_NORMALDEGREE);
		captureRenderState(D3DRS_MINTESSELLATIONLEVEL);
		captureRenderState(D3DRS_MAXTESSELLATIONLEVEL);
		captureRenderState(D3DRS_ADAPTIVETESS_X);
		captureRenderState(D3DRS_ADAPTIVETESS_Y);
		captureRenderState(D3DRS_ADAPTIVETESS_Z);
		captureRenderState(D3DRS_ADAPTIVETESS_W);
		captureRenderState(D3DRS_ENABLEADAPTIVETESSELLATION);
		captureRenderState(D3DRS_NORMALIZENORMALS);
		captureRenderState(D3DRS_SPECULARENABLE);
		captureRenderState(D3DRS_SHADEMODE);
	}

	void Direct3DStateBlock9::captureVertexSamplerStates()
	{
		for(int sampler = 0; sampler <= D3DVERTEXTEXTURESAMPLER3; sampler++)
		{
			captureSamplerState(sampler, D3DSAMP_DMAPOFFSET);
		}
	}

	void Direct3DStateBlock9::captureVertexTextureStates()
	{
		for(int stage = 0; stage < 8; stage++)
		{
			captureTextureStageState(stage, D3DTSS_TEXCOORDINDEX);
			captureTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS);
		}
	}

	void Direct3DStateBlock9::captureNPatchMode()
	{
		nPatchMode = device->GetNPatchMode();
		nPatchModeCaptured = true;
	}

	void Direct3DStateBlock9::captureLightStates()
	{
		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			long result = device->GetLight(index, &light[index]);
			lightCaptured[index] = SUCCEEDED(result);
		}

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			lightEnableState[index] = false;
			long result = device->GetLightEnable(index, &lightEnableState[index]);
			lightEnableCaptured[index] = SUCCEEDED(result);
		}
	}

	void Direct3DStateBlock9::captureVertexShaderStates()
	{
		vertexShaderCaptured = true;
		device->GetVertexShader(reinterpret_cast<IDirect3DVertexShader9**>(&vertexShader));

		if(vertexShader)
		{
			vertexShader->bind();
			vertexShader->Release();
		}

		device->GetVertexShaderConstantF(0, vertexShaderConstantF[0], MAX_VERTEX_SHADER_CONST);
		device->GetVertexShaderConstantI(0, vertexShaderConstantI[0], 16);
		device->GetVertexShaderConstantB(0, vertexShaderConstantB, 16);
	}

	void Direct3DStateBlock9::captureStreamSourceFrequencies()
	{
		for(int stream = 0; stream < MAX_VERTEX_INPUTS; stream++)
		{
			streamSourceFrequencyCaptured[stream] = true;
			device->GetStreamSourceFreq(stream, &streamSourceFrequency[stream]);
		}
	}

	void Direct3DStateBlock9::captureFVF()
	{
		device->GetFVF(&FVF);
		fvfCaptured = true;
	}

	void Direct3DStateBlock9::captureVertexDeclaration()
	{
		vertexDeclarationCaptured = true;
		device->GetVertexDeclaration(reinterpret_cast<IDirect3DVertexDeclaration9**>(&vertexDeclaration));

		if(vertexDeclaration)
		{
			vertexDeclaration->bind();
			vertexDeclaration->Release();
		}
	}

	void Direct3DStateBlock9::captureTextures()
	{
		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			textureCaptured[sampler] = true;
			int index = sampler < 16 ? sampler : D3DVERTEXTEXTURESAMPLER0 + (sampler - 16);
			device->GetTexture(index, reinterpret_cast<IDirect3DBaseTexture9**>(&texture[sampler]));

			if(texture[sampler])
			{
				texture[sampler]->bind();
				texture[sampler]->Release();
			}
		}
	}

	void Direct3DStateBlock9::captureTexturePalette()
	{
		paletteNumberCaptured = true;
		device->GetCurrentTexturePalette(&paletteNumber);
	}

	void Direct3DStateBlock9::captureVertexStreams()
	{
		for(int stream = 0; stream < MAX_VERTEX_INPUTS; stream++)
		{
			streamSourceCaptured[stream] = true;
			device->GetStreamSource(stream, reinterpret_cast<IDirect3DVertexBuffer9**>(&streamSource[stream].vertexBuffer), &streamSource[stream].offset, &streamSource[stream].stride);

			if(streamSource[stream].vertexBuffer)
			{
				streamSource[stream].vertexBuffer->bind();
				streamSource[stream].vertexBuffer->Release();
			}
		}
	}

	void Direct3DStateBlock9::captureIndexBuffer()
	{
		indexBufferCaptured = true;
		device->GetIndices(reinterpret_cast<IDirect3DIndexBuffer9**>(&indexBuffer));

		if(indexBuffer)
		{
			indexBuffer->bind();
			indexBuffer->Release();
		}
	}

	void Direct3DStateBlock9::captureViewport()
	{
		device->GetViewport(&viewport);
		viewportCaptured = true;
	}

	void Direct3DStateBlock9::captureScissorRectangle()
	{
		device->GetScissorRect(&scissorRect);
		scissorRectCaptured = true;
	}

	void Direct3DStateBlock9::captureTransforms()
	{
		captureTransform(D3DTS_VIEW);
		captureTransform(D3DTS_PROJECTION);
		captureTransform(D3DTS_WORLD);
	}

	void Direct3DStateBlock9::captureTextureTransforms()
	{
		captureTransform(D3DTS_TEXTURE0);
		captureTransform(D3DTS_TEXTURE1);
		captureTransform(D3DTS_TEXTURE2);
		captureTransform(D3DTS_TEXTURE3);
		captureTransform(D3DTS_TEXTURE4);
		captureTransform(D3DTS_TEXTURE5);
		captureTransform(D3DTS_TEXTURE6);
		captureTransform(D3DTS_TEXTURE7);
	}

	void Direct3DStateBlock9::captureClippingPlanes()
	{
		for(int index = 0; index < 6; index++)
		{
			device->GetClipPlane(index, (float*)&clipPlane[index]);
			clipPlaneCaptured[index] = true;
		}
	}

	void Direct3DStateBlock9::captureMaterial()
	{
		device->GetMaterial(&material);
		materialCaptured = true;
	}
}
