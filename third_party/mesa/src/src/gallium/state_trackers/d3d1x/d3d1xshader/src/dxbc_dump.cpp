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
#include <string.h>
#include <iomanip>
#include "dxbc.h"

std::ostream& operator <<(std::ostream& out, const dxbc_container& container)
{
	for(unsigned i = 0; i < container.chunks.size(); ++i)
	{
		struct dxbc_chunk_header* chunk = container.chunks[i];
		char fourcc_str[5];
		memcpy(fourcc_str, &chunk->fourcc, 4);
		fourcc_str[4] = 0;
		out << "# DXBC chunk " << std::setw(2) << i << ": " << fourcc_str << " offset " << ((char*)chunk - (char*)container.data) << " size " << bswap_le32(chunk->size) << "\n";
	}
	return out;
}
