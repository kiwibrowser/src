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

#include <stdlib.h>
#include <string.h>
#include "dxbc.h"

std::pair<void*, size_t> dxbc_assemble(struct dxbc_chunk_header** chunks, unsigned num_chunks)
{
	size_t data_size = 0;
	for(unsigned i = 0; i < num_chunks; ++i)
		data_size += sizeof(uint32_t) + sizeof(dxbc_chunk_header) + bswap_le32(chunks[i]->size);

	size_t total_size = sizeof(dxbc_container_header) + data_size;
	dxbc_container_header* header = (dxbc_container_header*)malloc(total_size);
	if(!header)
		return std::make_pair((void*)0, 0);

	header->fourcc = bswap_le32(FOURCC_DXBC);
	memset(header->unk, 0, sizeof(header->unk));
	header->one = bswap_le32(1);
	header->total_size = bswap_le32(total_size);
	header->chunk_count = num_chunks;

	uint32_t* chunk_offsets = (uint32_t*)(header + 1);
	uint32_t off = sizeof(struct dxbc_container_header) + num_chunks * sizeof(uint32_t);
	for(unsigned i = 0; i < num_chunks; ++i)
	{
		chunk_offsets[i] = bswap_le32(off);
		unsigned chunk_full_size = sizeof(dxbc_chunk_header) + bswap_le32(chunks[i]->size);
		memcpy((char*)header + off, chunks[i], chunk_full_size);
		off += chunk_full_size;
	}

	return std::make_pair((void*)header, total_size);
}
