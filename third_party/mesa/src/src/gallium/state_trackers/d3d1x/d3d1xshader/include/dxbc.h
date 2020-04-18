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

#ifndef DXBC_H_
#define DXBC_H_

#include <stdint.h>
#include <vector>
#include <map>
#include <iostream>
#include "le32.h"

#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24 ))
#define FOURCC_DXBC FOURCC('D', 'X', 'B', 'C')
#define FOURCC_RDEF FOURCC('R', 'D', 'E', 'F')
#define FOURCC_ISGN FOURCC('I', 'S', 'G', 'N')
#define FOURCC_OSGN FOURCC('O', 'S', 'G', 'N')
#define FOURCC_SHDR FOURCC('S', 'H', 'D', 'R')
#define FOURCC_SHEX FOURCC('S', 'H', 'E', 'X')
#define FOURCC_STAT FOURCC('S', 'T', 'A', 'T')
#define FOURCC_PCSG FOURCC('P', 'C', 'S', 'G')

/* this is always little-endian! */
struct dxbc_chunk_header
{
	unsigned fourcc;
	unsigned size;
};

/* this is always little-endian! */
struct dxbc_chunk_signature : public dxbc_chunk_header
{
	uint32_t count;
	uint32_t unk;
	struct
	{
		uint32_t name_offset;
		uint32_t semantic_index;
		uint32_t system_value_type;
		uint32_t component_type;
		uint32_t register_num;
		uint8_t mask;
		uint8_t read_write_mask;
		uint8_t stream; /* TODO: guess! */
		uint8_t unused;
	} elements[];
};

struct dxbc_container
{
	const void* data;
	std::vector<dxbc_chunk_header*> chunks;
	std::map<unsigned, unsigned> chunk_map;
};

struct dxbc_container_header
{
	unsigned fourcc;
	uint32_t unk[4];
	uint32_t one;
	uint32_t total_size;
	uint32_t chunk_count;
};

dxbc_container* dxbc_parse(const void* data, int size);
std::ostream& operator <<(std::ostream& out, const dxbc_container& container);

dxbc_chunk_header* dxbc_find_chunk(const void* data, int size, unsigned fourcc);

static inline dxbc_chunk_header* dxbc_find_shader_bytecode(const void* data, int size)
{
	dxbc_chunk_header* chunk;
	chunk = dxbc_find_chunk(data, size, FOURCC_SHDR);
	if(!chunk)
		chunk = dxbc_find_chunk(data, size, FOURCC_SHEX);
	return chunk;
}

#define DXBC_FIND_INPUT_SIGNATURE    0
#define DXBC_FIND_OUTPUT_SIGNATURE   1
#define DXBC_FIND_PATCH_SIGNATURE    2

static inline dxbc_chunk_signature* dxbc_find_signature(const void* data, int size, unsigned kind)
{
	unsigned fourcc;
	switch(kind) {
	case DXBC_FIND_INPUT_SIGNATURE:  fourcc = FOURCC_ISGN; break;
	case DXBC_FIND_OUTPUT_SIGNATURE: fourcc = FOURCC_OSGN; break;
	case DXBC_FIND_PATCH_SIGNATURE:  fourcc = FOURCC_PCSG; break;
	default:
		return NULL;
	}
	return (dxbc_chunk_signature*)dxbc_find_chunk(data, size, fourcc);
}

struct _D3D11_SIGNATURE_PARAMETER_DESC;
typedef struct _D3D11_SIGNATURE_PARAMETER_DESC D3D11_SIGNATURE_PARAMETER_DESC;
int dxbc_parse_signature(dxbc_chunk_signature* sig, D3D11_SIGNATURE_PARAMETER_DESC** params);

std::pair<void*, size_t> dxbc_assemble(struct dxbc_chunk_header** chunks, unsigned num_chunks);

#endif /* DXBC_H_ */
