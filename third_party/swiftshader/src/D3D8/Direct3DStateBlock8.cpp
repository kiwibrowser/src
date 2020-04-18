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

#include "Direct3DStateBlock8.hpp"

#include "Direct3DDevice8.hpp"
#include "Direct3DBaseTexture8.hpp"
#include "Direct3DVertexBuffer8.hpp"
#include "Direct3DIndexBuffer8.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D8
{
	Direct3DStateBlock8::Direct3DStateBlock8(Direct3DDevice8 *device, D3DSTATEBLOCKTYPE type) : device(device), type(type)
	{
		vertexShaderHandle = 0;
		pixelShaderHandle = 0;
		indexBuffer = 0;

		for(int stream = 0; stream < 16; stream++)
		{
			streamSource[stream].vertexBuffer = 0;
		}

		for(int stage = 0; stage < 8; stage++)
		{
			texture[stage] = 0;
		}

		clear();

		if(type == D3DSBT_PIXELSTATE || type == D3DSBT_ALL)
		{
			capturePixelRenderStates();
			capturePixelTextureStates();
			capturePixelShaderStates();
		}
		
		if(type == D3DSBT_VERTEXSTATE || type == D3DSBT_ALL)
		{
			captureVertexRenderStates();
			captureVertexTextureStates();
			captureLightStates();
			captureVertexShaderStates();
		}

		if(type == D3DSBT_ALL)   // Capture remaining states
		{
			captureTextures();
			captureVertexTextures();
			captureDisplacementTextures();
			captureTexturePalette();
			captureVertexStreams();
			captureIndexBuffer();
			captureViewport();
			captureTransforms();
			captureTextureTransforms();
			captureClippingPlanes();
			captureMaterial();
		}
	}

	Direct3DStateBlock8::~Direct3DStateBlock8()
	{
		clear();
	}

	long Direct3DStateBlock8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		ASSERT(false);   // Internal object

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DStateBlock8::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}
	
	unsigned long Direct3DStateBlock8::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DStateBlock8::Apply()
	{
		TRACE("");

		if(vertexShaderCaptured)
		{
			device->SetVertexShader(vertexShaderHandle);
		}

		if(pixelShaderCaptured)
		{
			device->SetPixelShader(pixelShaderHandle);
		}

		if(indexBufferCaptured)
		{
			device->SetIndices(indexBuffer, baseVertexIndex);
		}

		for(int state = 0; state < D3DRS_NORMALORDER + 1; state++)
		{
			if(renderStateCaptured[state])
			{
				device->SetRenderState((D3DRENDERSTATETYPE)state, renderState[state]);
			}
		}

		for(int stage = 0; stage < 8; stage++)
		{
			for(int state = 0; state < D3DTSS_RESULTARG + 1; state++)
			{
				if(textureStageStateCaptured[stage][state])
				{
					device->SetTextureStageState(stage, (D3DTEXTURESTAGESTATETYPE)state, textureStageState[stage][state]);
				}
			}
		}

		for(int stream = 0; stream < 16; stream++)
		{
			if(streamSourceCaptured[stream])
			{
				device->SetStreamSource(stream, streamSource[stream].vertexBuffer, streamSource[stream].stride);
			}
		}

		for(int stage = 0; stage < 8; stage++)
		{
			if(textureCaptured[stage])
			{
				device->SetTexture(stage, texture[stage]);
			}
		}

		for(int state = 0; state < 512; state++)
		{
			if(transformCaptured[state])
			{
				device->SetTransform((D3DTRANSFORMSTATETYPE)state, &transform[state]);
			}
		}

		if(viewportCaptured)
		{
			device->SetViewport(&viewport);
		}

		for(int index = 0; index < 6; index++)
		{
			if(clipPlaneCaptured[index])
			{
				device->SetClipPlane(index, clipPlane[index]);
			}
		}

		return D3D_OK;
	}

	long Direct3DStateBlock8::Capture()
	{
		TRACE("");

		if(vertexShaderCaptured)
		{		
			device->GetVertexShader(&vertexShaderHandle);
		}

		if(pixelShaderCaptured)
		{		
			device->GetPixelShader(&pixelShaderHandle);
		}

		if(indexBufferCaptured)
		{
			if(indexBuffer)
			{
				indexBuffer->Release();
			}

			device->GetIndices(reinterpret_cast<IDirect3DIndexBuffer8**>(&indexBuffer), &baseVertexIndex);
		}

		for(int state = 0; state < D3DRS_NORMALORDER + 1; state++)
		{
			if(renderStateCaptured[state])
			{
				device->GetRenderState((D3DRENDERSTATETYPE)state, &renderState[state]);
			}
		}

		for(int stage = 0; stage < 8; stage++)
		{
			for(int state = 0; state < D3DTSS_RESULTARG + 1; state++)
			{
				if(textureStageStateCaptured[stage][state])
				{
					device->GetTextureStageState(stage, (D3DTEXTURESTAGESTATETYPE)state, &textureStageState[stage][state]);
				}
			}
		}

		for(int stream = 0; stream < 16; stream++)
		{
			if(streamSourceCaptured[stream])
			{
				if(streamSource[stream].vertexBuffer)
				{
					streamSource[stream].vertexBuffer->Release();
				}

				device->GetStreamSource(stream, reinterpret_cast<IDirect3DVertexBuffer8**>(&streamSource[stream].vertexBuffer), &streamSource[stream].stride);
			}
		}

		for(int stage = 0; stage < 8; stage++)
		{
			if(textureCaptured[stage])
			{
				if(texture[stage])
				{
					texture[stage]->Release();
				}

				device->GetTexture(stage, reinterpret_cast<IDirect3DBaseTexture8**>(&texture[stage]));
			}
		}

		for(int state = 0; state < 512; state++)
		{
			if(transformCaptured[state])
			{
				device->GetTransform((D3DTRANSFORMSTATETYPE)state, &transform[state]);
			}
		}

		if(viewportCaptured)
		{
			device->GetViewport(&viewport);
		}

		for(int index = 0; index < 6; index++)
		{
			if(clipPlaneCaptured[index])
			{
				device->GetClipPlane(index, clipPlane[index]);
			}
		}

		return D3D_OK;
	}

	long Direct3DStateBlock8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		if(!device)
		{
			return INVALIDCALL();
		}

		this->device->AddRef();
		*device = this->device;

		return D3D_OK;
	}

	void Direct3DStateBlock8::lightEnable(unsigned long index, int enable)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::setClipPlane(unsigned long index, const float *plane)
	{
		clipPlaneCaptured[index] = true;
		clipPlane[index][0] = plane[0];
		clipPlane[index][1] = plane[1];
		clipPlane[index][2] = plane[2];
		clipPlane[index][3] = plane[3];
	}

	void Direct3DStateBlock8::setCurrentTexturePalette(unsigned int paletteNumber)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::setFVF(unsigned long FVF)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::setIndices(Direct3DIndexBuffer8 *indexData, unsigned int baseVertexIndex)
	{
		if(indexData) indexData->AddRef();

		indexBufferCaptured = true;
		indexBuffer = indexData;
		this->baseVertexIndex = baseVertexIndex;
	}

	void Direct3DStateBlock8::setLight(unsigned long index, const D3DLIGHT8 *light)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::setMaterial(const D3DMATERIAL8 *material)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::setPixelShader(unsigned long shaderHandle)		
	{
		pixelShaderCaptured = true;
		pixelShaderHandle = shaderHandle;
	}

	void Direct3DStateBlock8::setPixelShaderConstant(unsigned int startRegister, const void *constantData, unsigned int count)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::setRenderState(D3DRENDERSTATETYPE state, unsigned long value)
	{
		renderStateCaptured[state] = true;
		renderState[state] = value;
	}

	void Direct3DStateBlock8::setScissorRect(const RECT *rect)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::setStreamSource(unsigned int stream, Direct3DVertexBuffer8 *data, unsigned int stride)
	{
		if(data) data->AddRef();

		streamSourceCaptured[stream] = true;
		streamSource[stream].vertexBuffer = data;
		streamSource[stream].stride = stride;
	}

	void Direct3DStateBlock8::setTexture(unsigned long stage, Direct3DBaseTexture8 *texture)
	{
		if(texture) texture->AddRef();

		textureCaptured[stage] = true;
		this->texture[stage] = texture;
	}

	void Direct3DStateBlock8::setTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value)
	{
		textureStageStateCaptured[stage][type] = true;
		textureStageState[stage][type] = value;
	}

	void Direct3DStateBlock8::setTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		transformCaptured[state] = true;
		transform[state] = *matrix;
	}

	void Direct3DStateBlock8::setViewport(const D3DVIEWPORT8 *viewport)
	{
		viewportCaptured = true;
		this->viewport = *viewport;
	}

	void Direct3DStateBlock8::setVertexShader(unsigned long shaderHandle)
	{
		vertexShaderCaptured = true;
		vertexShaderHandle = shaderHandle;
	}

	void Direct3DStateBlock8::setVertexShaderConstant(unsigned int startRegister, const void *constantData, unsigned int count)
	{
		UNIMPLEMENTED();
	}

	void Direct3DStateBlock8::clear()
	{
		// Erase capture flags
		vertexShaderCaptured = false;
		pixelShaderCaptured = false;
		indexBufferCaptured = false;

		for(int state = 0; state < D3DRS_NORMALORDER + 1; state++)
		{
			renderStateCaptured[state] = false;
		}

		for(int stage = 0; stage < 8; stage++)
		{
			for(int state = 0; state < D3DTSS_RESULTARG + 1; state++)
			{
				textureStageStateCaptured[stage][state] = false;
			}
		}

		for(int stream = 0; stream < 16; stream++)
		{
			streamSourceCaptured[stream] = false;
		}

		for(int stage = 0; stage < 8; stage++)
		{
			textureCaptured[stage] = false;
		}

		for(int state = 0; state < 512; state++)
		{
			transformCaptured[state] = false;
		}

		viewportCaptured = false;

		for(int index = 0; index < 6; index++)
		{
			clipPlaneCaptured[index] = false;
		}

		// Release resources
		vertexShaderHandle = 0;
		pixelShaderHandle = 0;

		if(indexBuffer)
		{
			indexBuffer->Release();
			indexBuffer = 0;
		}

		for(int stream = 0; stream < 16; stream++)
		{
			if(streamSource[stream].vertexBuffer)
			{
				streamSource[stream].vertexBuffer->Release();
				streamSource[stream].vertexBuffer = 0;
			}
		}

		for(int stage = 0; stage < 8; stage++)
		{
			if(texture[stage])
			{
				texture[stage]->Release();
				texture[stage] = 0;
			}
		}
	}

	void Direct3DStateBlock8::captureRenderState(D3DRENDERSTATETYPE state)
	{
		device->GetRenderState(state, &renderState[state]);
		renderStateCaptured[state] = true;
	}

	void Direct3DStateBlock8::captureTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type)
	{
		device->GetTextureStageState(stage, type, &textureStageState[stage][type]);
		textureStageStateCaptured[stage][type] = true;
	}

	void Direct3DStateBlock8::captureTransform(D3DTRANSFORMSTATETYPE state)
	{
		device->GetTransform(state, &transform[state]);
		transformCaptured[state] = true;
	}

	void Direct3DStateBlock8::capturePixelRenderStates()
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
		captureRenderState(D3DRS_ZBIAS);
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
		captureRenderState(D3DRS_COLORWRITEENABLE);
		captureRenderState(D3DRS_BLENDOP);
	}

	void Direct3DStateBlock8::capturePixelTextureStates()
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
		
			captureTextureStageState(stage, D3DTSS_ADDRESSU);
			captureTextureStageState(stage, D3DTSS_ADDRESSV);
			captureTextureStageState(stage, D3DTSS_ADDRESSW); 
			captureTextureStageState(stage, D3DTSS_BORDERCOLOR);
			captureTextureStageState(stage, D3DTSS_MAGFILTER);
			captureTextureStageState(stage, D3DTSS_MINFILTER);
			captureTextureStageState(stage, D3DTSS_MIPFILTER);
			captureTextureStageState(stage, D3DTSS_MIPMAPLODBIAS);
			captureTextureStageState(stage, D3DTSS_MAXMIPLEVEL);
			captureTextureStageState(stage, D3DTSS_MAXANISOTROPY);
		}
	}

	void Direct3DStateBlock8::capturePixelShaderStates()
	{
		pixelShaderCaptured = true;
		device->GetPixelShader(&pixelShaderHandle);

		device->GetPixelShaderConstant(0, pixelShaderConstant, 8);
	}

	void Direct3DStateBlock8::captureVertexRenderStates()
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
		captureRenderState(D3DRS_NORMALIZENORMALS);
		captureRenderState(D3DRS_SPECULARENABLE);
		captureRenderState(D3DRS_SHADEMODE);
	}

	void Direct3DStateBlock8::captureVertexTextureStates()
	{
		for(int stage = 0; stage < 8; stage++)
		{
			captureTextureStageState(stage, D3DTSS_TEXCOORDINDEX);
			captureTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS);
		}
	}

	void Direct3DStateBlock8::captureLightStates()
	{
		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			device->GetLight(index, &light[index]);
			lightCaptured[index] = true;
		}

		for(int index = 0; index < 8; index++)   // FIXME: Support unlimited index
		{
			lightEnableState[index] = false;
			device->GetLightEnable(index, &lightEnableState[index]);
			lightEnableCaptured[index] = true;
		}
	}

	void Direct3DStateBlock8::captureVertexShaderStates()
	{
		vertexShaderCaptured = true;
		device->GetVertexShader(&vertexShaderHandle);

		device->GetVertexShaderConstant(0, vertexShaderConstant[0], 256);
	}

	void Direct3DStateBlock8::captureTextures()
	{
		for(int sampler = 0; sampler < 8; sampler++)
		{
			textureCaptured[sampler] = true;
			device->GetTexture(sampler, reinterpret_cast<IDirect3DBaseTexture8**>(&texture[sampler]));

			if(texture[sampler])
			{
				texture[sampler]->bind();
				texture[sampler]->Release();
			}
		}
	}

	void Direct3DStateBlock8::captureVertexTextures()
	{
		// FIXME
	}

	void Direct3DStateBlock8::captureDisplacementTextures()
	{
		// FIXME	
	}

	void Direct3DStateBlock8::captureTexturePalette()
	{
		paletteNumberCaptured = true;
		device->GetCurrentTexturePalette(&paletteNumber);
	}

	void Direct3DStateBlock8::captureVertexStreams()
	{
		for(int stream = 0; stream < 16; stream++)
		{
			streamSourceCaptured[stream] = true;
			device->GetStreamSource(stream, reinterpret_cast<IDirect3DVertexBuffer8**>(&streamSource[stream].vertexBuffer),  &streamSource[stream].stride);
			
			if(streamSource[stream].vertexBuffer)
			{
				streamSource[stream].vertexBuffer->bind();
				streamSource[stream].vertexBuffer->Release();
			}
		}
	}

	void Direct3DStateBlock8::captureIndexBuffer()
	{
		indexBufferCaptured = true;
		device->GetIndices(reinterpret_cast<IDirect3DIndexBuffer8**>(&indexBuffer), &baseVertexIndex);

		if(indexBuffer)
		{
			indexBuffer->bind();
			indexBuffer->Release();
		}
	}

	void Direct3DStateBlock8::captureViewport()
	{
		device->GetViewport(&viewport);
		viewportCaptured = true;
	}

	void Direct3DStateBlock8::captureTransforms()
	{
		captureTransform(D3DTS_VIEW);
		captureTransform(D3DTS_PROJECTION);
		captureTransform(D3DTS_WORLD);
	}

	void Direct3DStateBlock8::captureTextureTransforms()
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

	void Direct3DStateBlock8::captureClippingPlanes()
	{
		for(int index = 0; index < 6; index++)
		{
			device->GetClipPlane(index, (float*)&clipPlane[index]);
			clipPlaneCaptured[index] = true;
		}
	}

	void Direct3DStateBlock8::captureMaterial()
	{
		device->GetMaterial(&material);
		materialCaptured = true;
	}
}