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

#include "dxbc.h"
#include "sm4.h"
#include "../sm4_to_tgsi.h"
#include "tgsi/tgsi_dump.h"
#include <iostream>
#include <fstream>

void usage()
{
	std::cerr << "Gallium Direct3D10/11 Shader to TGSI converter\n";
	std::cerr << "This program is free software, released under a MIT-like license\n";
	std::cerr << "Not affiliated with or endorsed by Microsoft in any way\n";
	std::cerr << "Latest version available from http://cgit.freedesktop.org/mesa/mesa/\n";
	std::cerr << "\n";
	std::cerr << "Usage: dxbc2tgsi FILE\n";
	std::cerr << std::endl;
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		usage();
		return 1;
	}

	std::vector<char> data;
	std::ifstream in(argv[1]);
	char c;
	in >> std::noskipws;
	while(in >> c)
		data.push_back(c);
	in.close();

	dxbc_container* dxbc = dxbc_parse(&data[0], data.size());
	if(dxbc)
	{
		std::cout << *dxbc;
		dxbc_chunk_header* sm4_chunk = dxbc_find_shader_bytecode(&data[0], data.size());
		if(sm4_chunk)
		{
			sm4_program* sm4 = sm4_parse(sm4_chunk + 1, bswap_le32(sm4_chunk->size));
			if(sm4)
			{
				const struct tgsi_token* tokens = (const struct tgsi_token*)sm4_to_tgsi(*sm4);
				if(tokens)
				{
					std::cout << *sm4;
					std::cout << "\n# TGSI program: " << std::endl;
					tgsi_dump(tokens, 0);
				}
			}
		}
		delete dxbc;
	}
}
