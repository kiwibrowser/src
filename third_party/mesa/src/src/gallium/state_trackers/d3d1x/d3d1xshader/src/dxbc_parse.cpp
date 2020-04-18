/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <memory>
#include "dxbc.h"
#include <d3d11shader.h>
#include <d3dcommon.h>

dxbc_container* dxbc_parse(const void* data, int size)
{
	std::auto_ptr<dxbc_container> container(new dxbc_container());
	container->data = data;
	dxbc_container_header* header = (dxbc_container_header*)data;
	uint32_t* chunk_offsets = (uint32_t*)(header + 1);
	if(bswap_le32(header->fourcc) != FOURCC_DXBC)
		return 0;
	unsigned num_chunks = bswap_le32(header->chunk_count);
	for(unsigned i = 0; i < num_chunks; ++i)
	{
		unsigned offset = bswap_le32(chunk_offsets[i]);
		dxbc_chunk_header* chunk = (dxbc_chunk_header*)((char*)data + offset);
		unsigned fourcc = bswap_le32(chunk->fourcc);
		container->chunk_map[fourcc] = i;
		container->chunks.push_back(chunk);
	}
	return container.release();
}

dxbc_chunk_header* dxbc_find_chunk(const void* data, int size, unsigned fourcc)
{
	dxbc_container_header* header = (dxbc_container_header*)data;
	uint32_t* chunk_offsets = (uint32_t*)(header + 1);
	if(bswap_le32(header->fourcc) != FOURCC_DXBC)
		return 0;
	unsigned num_chunks = bswap_le32(header->chunk_count);
	for(unsigned i = 0; i < num_chunks; ++i)
	{
		unsigned offset = bswap_le32(chunk_offsets[i]);
		dxbc_chunk_header* chunk = (dxbc_chunk_header*)((char*)data + offset);
		if(bswap_le32(chunk->fourcc) == fourcc)
			return chunk;
	}
	return 0;
}

int dxbc_parse_signature(dxbc_chunk_signature* sig, D3D11_SIGNATURE_PARAMETER_DESC** params)
{
	unsigned count = bswap_le32(sig->count);
	*params = (D3D11_SIGNATURE_PARAMETER_DESC*)malloc(sizeof(D3D11_SIGNATURE_PARAMETER_DESC) * count);

	for (unsigned i = 0; i < count; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC& param = (*params)[i];
		param.SemanticName = (char*)&sig->count + bswap_le32(sig->elements[i].name_offset);
		param.SemanticIndex = bswap_le32(sig->elements[i].semantic_index);
		param.SystemValueType = (D3D_NAME)bswap_le32(sig->elements[i].system_value_type);
		param.ComponentType = (D3D_REGISTER_COMPONENT_TYPE)bswap_le32(sig->elements[i].component_type);
		param.Register = bswap_le32(sig->elements[i].register_num);
		param.Mask = sig->elements[i].mask;
		param.ReadWriteMask = sig->elements[i].read_write_mask;
		param.Stream = sig->elements[i].stream;
	}
	return count;
}
