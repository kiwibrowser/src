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

#include "Direct3DVertexDeclaration9.hpp"

#include "Direct3DDevice9.hpp"
#include "Debug.hpp"

#include <d3d9types.h>
#include <stdio.h>
#include <assert.h>

namespace D3D9
{
	Direct3DVertexDeclaration9::Direct3DVertexDeclaration9(Direct3DDevice9 *device, const D3DVERTEXELEMENT9 *vertexElement) : device(device)
	{
		int size = sizeof(D3DVERTEXELEMENT9);
		const D3DVERTEXELEMENT9 *element = vertexElement;
		preTransformed = false;

		while(element->Stream != 0xFF)
		{
			if(element->Usage == D3DDECLUSAGE_POSITIONT)
			{
				preTransformed = true;
			}

			size += sizeof(D3DVERTEXELEMENT9);
			element++;
		}

		numElements = size / sizeof(D3DVERTEXELEMENT9);
		this->vertexElement = new D3DVERTEXELEMENT9[numElements];
		memcpy(this->vertexElement, vertexElement, size);

		FVF = computeFVF();
	}

	Direct3DVertexDeclaration9::Direct3DVertexDeclaration9(Direct3DDevice9 *device, unsigned long FVF) : device(device)
	{
		this->FVF = FVF;

		vertexElement = new D3DVERTEXELEMENT9[MAX_VERTEX_INPUTS];

		numElements = 0;
		int offset = 0;
		preTransformed = false;

		switch(FVF & D3DFVF_POSITION_MASK)
		{
		case 0:
			// No position stream
			break;
		case D3DFVF_XYZ:
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITION;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;
			break;
		case D3DFVF_XYZRHW:
			preTransformed = true;
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT4;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITIONT;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 4;
			break;
		case D3DFVF_XYZB1:
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITION;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;

			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT1;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_BLENDWEIGHT;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 1;
			break;
		case D3DFVF_XYZB2:
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITION;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;

			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT2;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_BLENDWEIGHT;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 2;
			break;
		case D3DFVF_XYZB3:
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITION;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;

			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_BLENDWEIGHT;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;
			break;
		case D3DFVF_XYZB4:
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITION;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;

			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT4;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_BLENDWEIGHT;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 4;
			break;
		case D3DFVF_XYZB5:
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITION;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;

			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT4;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_BLENDWEIGHT;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 5;
			break;
		case D3DFVF_XYZW:
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT4;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_POSITION;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 4;
			break;
		default:
			ASSERT(false);
		}

		if(FVF & D3DFVF_NORMAL)
		{
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT3;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_NORMAL;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4 * 3;
		}

		if(FVF & D3DFVF_PSIZE)
		{
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_FLOAT1;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_PSIZE;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4;
		}

		if(FVF & D3DFVF_DIFFUSE)
		{
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_D3DCOLOR;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_COLOR;
			vertexElement[numElements].UsageIndex = 0;
			numElements++;
			offset += 4;
		}

		if(FVF & D3DFVF_SPECULAR)
		{
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = D3DDECLTYPE_D3DCOLOR;
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_COLOR;
			vertexElement[numElements].UsageIndex = 1;
			numElements++;
			offset += 4;
		}

		int numTexCoord = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
		int textureFormats = (FVF >> 16) & 0xFFFF;

		static const int textureSize[4] =
		{
			2 * 4,   // D3DFVF_TEXTUREFORMAT2
			3 * 4,   // D3DFVF_TEXTUREFORMAT3
			4 * 4,   // D3DFVF_TEXTUREFORMAT4
			1 * 4    // D3DFVF_TEXTUREFORMAT1
		};

		static const D3DDECLTYPE textureType[4] =
		{
			D3DDECLTYPE_FLOAT2,   // D3DFVF_TEXTUREFORMAT2
			D3DDECLTYPE_FLOAT3,   // D3DFVF_TEXTUREFORMAT3
			D3DDECLTYPE_FLOAT4,   // D3DFVF_TEXTUREFORMAT4
			D3DDECLTYPE_FLOAT1    // D3DFVF_TEXTUREFORMAT1
		};

		for(int i = 0; i < numTexCoord; i++)
		{
			vertexElement[numElements].Stream = 0;
			vertexElement[numElements].Offset = offset;
			vertexElement[numElements].Type = textureType[textureFormats & 0x3];
			vertexElement[numElements].Method = D3DDECLMETHOD_DEFAULT;
			vertexElement[numElements].Usage = D3DDECLUSAGE_TEXCOORD;
			vertexElement[numElements].UsageIndex = i;
			numElements++;
			offset += textureSize[textureFormats & 0x3];
			textureFormats >>= 2;
		}

		// D3DDECL_END()
		vertexElement[numElements].Stream = 0xFF;
		vertexElement[numElements].Offset = 0;
		vertexElement[numElements].Type = D3DDECLTYPE_UNUSED;
		vertexElement[numElements].Method = 0;
		vertexElement[numElements].Usage = 0;
		vertexElement[numElements].UsageIndex = 0;
		numElements++;
	}

	Direct3DVertexDeclaration9::~Direct3DVertexDeclaration9()
	{
		delete[] vertexElement;
		vertexElement = 0;
	}

	long Direct3DVertexDeclaration9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DVertexDeclaration9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DVertexDeclaration9::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3DVertexDeclaration9::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DVertexDeclaration9::GetDevice(IDirect3DDevice9 **device)
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

	long Direct3DVertexDeclaration9::GetDeclaration(D3DVERTEXELEMENT9 *declaration, unsigned int *numElements)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!declaration || !numElements)
		{
			return INVALIDCALL();
		}

		*numElements = this->numElements;

		for(int i = 0; i < this->numElements; i++)
		{
			declaration[i] = vertexElement[i];
		}

		return D3D_OK;
	}

	unsigned long Direct3DVertexDeclaration9::getFVF() const
	{
		return FVF;
	}

	bool Direct3DVertexDeclaration9::isPreTransformed() const
	{
		return preTransformed;
	}

	unsigned long Direct3DVertexDeclaration9::computeFVF()
	{
		unsigned long FVF = 0;

		int textureBits = 0;
		int numBlendWeights = 0;

		for(int i = 0; i < numElements - 1; i++)
		{
			D3DVERTEXELEMENT9 &element = vertexElement[i];

			if(element.Stream != 0)
			{
				return 0;
			}

			switch(element.Usage)
			{
			case D3DDECLUSAGE_POSITION:
				if(element.Type == D3DDECLTYPE_FLOAT3 && element.UsageIndex == 0)
				{
					FVF |= D3DFVF_XYZ;
				}
				else
				{
					return 0;
				}
				break;
			case D3DDECLUSAGE_POSITIONT:
				if(element.Type == D3DDECLTYPE_FLOAT4 && element.UsageIndex == 0)
				{
					FVF |= D3DFVF_XYZRHW;
				}
				else
				{
					return 0;
				}
				break;
			case D3DDECLUSAGE_BLENDWEIGHT:
				if(element.Type <= D3DDECLTYPE_FLOAT4 && element.UsageIndex == 0)
				{
					numBlendWeights += element.Type + 1;
				}
				else
				{
					return 0;
				}
				break;
			case D3DDECLUSAGE_BLENDINDICES:
				return 0;
				break;
			case D3DDECLUSAGE_NORMAL:
				if(element.Type == D3DDECLTYPE_FLOAT3 && element.UsageIndex == 0)
				{
					FVF |= D3DFVF_NORMAL;
				}
				else
				{
					return 0;
				}
				break;
			case D3DDECLUSAGE_PSIZE:
				if(element.Type == D3DDECLTYPE_FLOAT1 && element.UsageIndex == 0)
				{
					FVF |= D3DFVF_PSIZE;
				}
				else
				{
					return 0;
				}
				break;
			case D3DDECLUSAGE_COLOR:
				if(element.Type == D3DDECLTYPE_D3DCOLOR && element.UsageIndex < 2)
				{
					if(element.UsageIndex == 0)
					{
						FVF |= D3DFVF_DIFFUSE;
					}
					else   // element.UsageIndex == 1
					{
						FVF |= D3DFVF_SPECULAR;
					}
				}
				else
				{
					return 0;
				}
				break;
			case D3DDECLUSAGE_TEXCOORD:
				if((element.Type > D3DDECLTYPE_FLOAT4) || (element.UsageIndex > 7))
				{
					return 0;
				}

				int bit = 1 << element.UsageIndex;

				if(textureBits & bit)
				{
					return 0;
				}

				textureBits |= bit;

				switch(element.Type)
				{
				case D3DDECLTYPE_FLOAT1:
					FVF |= D3DFVF_TEXCOORDSIZE1(element.UsageIndex);
					break;
				case D3DDECLTYPE_FLOAT2:
					FVF |= D3DFVF_TEXCOORDSIZE2(element.UsageIndex);
					break;
				case D3DDECLTYPE_FLOAT3:
					FVF |= D3DFVF_TEXCOORDSIZE3(element.UsageIndex);
					break;
				case D3DDECLTYPE_FLOAT4:
					FVF |= D3DFVF_TEXCOORDSIZE4(element.UsageIndex);
					break;
				}
			}
		}

		bool isTransformed = (FVF & D3DFVF_XYZRHW) != 0;

		if(isTransformed)
		{
			if(numBlendWeights != 0)
			{
				return 0;
			}
		}
		else if((FVF & D3DFVF_XYZ) == 0)
		{
			return 0;
		}

		int positionMask = isTransformed ? 0x2 : 0x1;

		if(numBlendWeights)
		{
			positionMask += numBlendWeights + 1;
		}

		int numTexCoord = 0;

		while(textureBits & 1)
		{
			textureBits >>= 1;

			numTexCoord++;
		}

		if(textureBits)   // FVF does not allow
		{
			return 0;
		}

		FVF |= D3DFVF_POSITION_MASK & (positionMask << 1);
		FVF |= numTexCoord << D3DFVF_TEXCOUNT_SHIFT;

		return FVF;
	}
}
