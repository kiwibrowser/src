/*-------------------------------------------------------------------------
 * drawElements Base Portability Library
 * -------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief SHA1 hash functions.
 *//*--------------------------------------------------------------------*/

#include "deSha1.h"

#include "deMemory.h"

DE_BEGIN_EXTERN_C

enum
{
	CHUNK_BIT_SIZE	= 512,
	CHUNK_BYTE_SIZE	= CHUNK_BIT_SIZE / 8
};

static deUint32 leftRotate (deUint32 val, deUint32 count)
{
	DE_ASSERT(count < 32);

	return (val << count) | (val >> (32 - count));
}

void deSha1Stream_init (deSha1Stream* stream)
{
	stream->size = 0;

	/* Set the initial 16 deUint32s that contain real data to zeros. */
	deMemset(stream->data, 0, 16 * sizeof(deUint32));

	stream->hash[0] = 0x67452301u;
	stream->hash[1] = 0xEFCDAB89u;
	stream->hash[2] = 0x98BADCFEu;
	stream->hash[3] = 0x10325476u;
	stream->hash[4] = 0xC3D2E1F0u;
}

static void deSha1Stream_flushChunk (deSha1Stream* stream)
{
	DE_ASSERT(stream->size % CHUNK_BYTE_SIZE == 0 && stream->size > 0);

	{
		size_t ndx;

		/* Expand the 16 uint32s that contain the data to 80. */
		for (ndx = 16; ndx < DE_LENGTH_OF_ARRAY(stream->data); ndx++)
		{
			stream->data[ndx] = leftRotate(stream->data[ndx - 3]
										   ^ stream->data[ndx - 8]
										   ^ stream->data[ndx - 14]
										   ^ stream->data[ndx - 16], 1);
		}
	}

	{
		deUint32	a = stream->hash[0];
		deUint32	b = stream->hash[1];
		deUint32	c = stream->hash[2];
		deUint32	d = stream->hash[3];
		deUint32	e = stream->hash[4];
		size_t		ndx;

		for (ndx = 0; ndx < DE_LENGTH_OF_ARRAY(stream->data); ndx++)
		{
			deUint32 f;
			deUint32 k;

			if (ndx < 20)
			{
				f = (b & c) | ((~b) & d);
				k = 0x5A827999u;
			}
			else if (ndx < 40)
			{
				f = b ^ c ^ d;
				k = 0x6ED9EBA1u;
			}
			else if (ndx < 60)
			{
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDCu;
			}
			else
			{
				f = b ^ c ^ d;
				k = 0xCA62C1D6u;
			}

			{
				const deUint32 tmp = leftRotate(a, 5) + f + e + k + stream->data[ndx];

				e = d;
				d = c;
				c = leftRotate(b, 30);
				b = a;
				a = tmp;
			}
		}

		stream->hash[0] += a;
		stream->hash[1] += b;
		stream->hash[2] += c;
		stream->hash[3] += d;
		stream->hash[4] += e;

		/* Set the initial 16 deUint32s that contain the real data to zeros. */
		deMemset(stream->data, 0, 16 * sizeof(deUint32));
	}
}

void deSha1Stream_process (deSha1Stream* stream, size_t size, const void* data_)
{
	const deUint8* const	data			= (const deUint8*)data_;
	size_t					bytesProcessed	= 0;

	while (bytesProcessed < size)
	{
		do
		{
			const size_t bitOffset = (size_t)(8 * (4 - (1 + (stream->size % 4))));

			stream->data[(stream->size / 4) % 16] |= ((deUint32)data[bytesProcessed]) << (deUint32)bitOffset;

			stream->size++;
			bytesProcessed++;
		}
		while (stream->size % CHUNK_BYTE_SIZE != 0 && bytesProcessed < size);

		if (stream->size % CHUNK_BYTE_SIZE == 0)
			deSha1Stream_flushChunk(stream);
	}

	DE_ASSERT(bytesProcessed == size);
}

void deSha1Stream_finalize (deSha1Stream* stream, deSha1* hash)
{
	/* \note First element is initialized to 0x80u and rest to 0x0. */
	static const deUint8	padding[CHUNK_BYTE_SIZE]	= { 0x80u };
	const deUint64			length						= stream->size * 8;
	deUint8					lengthData[sizeof(deUint64)];
	size_t					ndx;

	DE_ASSERT(padding[0] == 0x80u);
	DE_ASSERT(padding[1] == 0x0u);

	for (ndx = 0; ndx < sizeof(deUint64); ndx++)
		lengthData[ndx] = (deUint8)(0xffu & (length >> (8 * (sizeof(deUint64) - 1 - ndx))));

	{
		const deUint64 spaceLeftInChunk = CHUNK_BYTE_SIZE - (stream->size % CHUNK_BYTE_SIZE);

		if (spaceLeftInChunk >= 1 + sizeof(lengthData))
			deSha1Stream_process(stream, (size_t)(spaceLeftInChunk - sizeof(lengthData)), padding);
		else
			deSha1Stream_process(stream, (size_t)(CHUNK_BYTE_SIZE - (sizeof(lengthData)) - spaceLeftInChunk), padding);
	}

	deSha1Stream_process(stream, sizeof(lengthData), lengthData);
	DE_ASSERT(stream->size % CHUNK_BYTE_SIZE == 0);

	deMemcpy(hash->hash, stream->hash, sizeof(hash->hash));
}

void deSha1_compute (deSha1* hash, size_t size, const void* data)
{
	deSha1Stream stream;

	deSha1Stream_init(&stream);
	deSha1Stream_process(&stream, size, data);
	deSha1Stream_finalize(&stream, hash);
}

void deSha1_render (const deSha1* hash, char* buffer)
{
	size_t charNdx;

	for (charNdx = 0; charNdx < 40; charNdx++)
	{
		const deUint32	val32	= hash->hash[charNdx / 8];
		const deUint8	val8	= (deUint8)(0x0fu & (val32 >> (4 * (8 - 1 - (charNdx % 8)))));

		if (val8 < 10)
			buffer[charNdx] = (char)('0' + val8);
		else
			buffer[charNdx] = (char)('a' + val8 - 10);
	}
}

deBool deSha1_parse (deSha1* hash, const char* buffer)
{
	size_t charNdx;

	deMemset(hash->hash, 0, sizeof(hash->hash));

	for (charNdx = 0; charNdx < 40; charNdx++)
	{
		deUint8 val4;

		if (buffer[charNdx] >= '0' && buffer[charNdx] <= '9')
			val4 = (deUint8)(buffer[charNdx] - '0');
		else if (buffer[charNdx] >= 'a' && buffer[charNdx] <= 'f')
			val4 = (deUint8)(10 + (buffer[charNdx] - 'a'));
		else if (buffer[charNdx] >= 'A' && buffer[charNdx] <= 'F')
			val4 = (deUint8)(10 + (buffer[charNdx] - 'A'));
		else
			return DE_FALSE;

		hash->hash[charNdx / 8] |= ((deUint32)val4) << (4 * (8u - 1u - (charNdx % 8u)));
	}

	return DE_TRUE;
}

deBool deSha1_equal (const deSha1* a, const deSha1* b)
{
	/* \note deMemcmp() can only be used for equality. It doesn't provide correct ordering between hashes. */
	return deMemCmp(a->hash, b->hash, sizeof(b->hash)) == 0;
}

void deSha1_selfTest (void)
{
	const char* const validHashStrings[] =
	{
		"ac890cfca05717c05dc831996b2289251da2984e",
		"0f87ba807acb3e6effe617249f30453a524a2ea3",
		"6f483cc3fa820e58ed9f83c83bdf8d213293b3ad"
	};

	const char* const invalidHashStrings[] =
	{
		" c890cfca05717c05dc831996b2289251da2984e",
		"0f87ba807acb3e6 ffe617249f30453a524a2ea3",
		"6f483cc3fa820e58ed9f83c83bdf8d213293b3a ",

		"mc890cfca05717c05dc831996b2289251da2984e",
		"0f87ba807acb3e6effe617249fm0453a524a2ea3",
		"6f483cc3fa820e58ed9f83c83bdf8d213293b3an",

		"ac890cfca05717c05dc83\n996b2289251da2984e",
		"0f87ba807acb3e6effe617\t49f30453a524a2ea3",
		"ac890cfca05717c05dc831\096b2289251da2984e",
		"6f483cc3fa{20e58ed9f83c83bdf8d213293b3ad"
	};

	const struct
	{
		const char* const hash;
		const char* const data;
	} stringHashPairs[] =
	{
		/* Generated using sha1sum. */
		{ "da39a3ee5e6b4b0d3255bfef95601890afd80709", "" },
		{ "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d", "hello" },
		{ "ec1919e856540f42bd0e6f6c1ffe2fbd73419975",
			"Cherry is a browser-based GUI for controlling deqp test runs and analysing the test results."
		}
	};

	const int garbage = 0xde;

	/* Test parsing valid sha1 strings. */
	{
		size_t stringNdx;

		for (stringNdx = 0; stringNdx < DE_LENGTH_OF_ARRAY(validHashStrings); stringNdx++)
		{
			deSha1 hash;
			deMemset(&hash, garbage, sizeof(deSha1));
			DE_TEST_ASSERT(deSha1_parse(&hash, validHashStrings[stringNdx]));
		}
	}

	/* Test parsing invalid sha1 strings. */
	{
		size_t stringNdx;

		for (stringNdx = 0; stringNdx < DE_LENGTH_OF_ARRAY(invalidHashStrings); stringNdx++)
		{
			deSha1 hash;
			deMemset(&hash, garbage, sizeof(deSha1));
			DE_TEST_ASSERT(!deSha1_parse(&hash, invalidHashStrings[stringNdx]));
		}
	}

	/* Compare valid hash strings for equality. */
	{
		size_t stringNdx;

		for (stringNdx = 0; stringNdx < DE_LENGTH_OF_ARRAY(validHashStrings); stringNdx++)
		{
			deSha1 hashA;
			deSha1 hashB;

			deMemset(&hashA, garbage, sizeof(deSha1));
			deMemset(&hashB, garbage, sizeof(deSha1));

			DE_TEST_ASSERT(deSha1_parse(&hashA, validHashStrings[stringNdx]));
			DE_TEST_ASSERT(deSha1_parse(&hashB, validHashStrings[stringNdx]));

			DE_TEST_ASSERT(deSha1_equal(&hashA, &hashA));
			DE_TEST_ASSERT(deSha1_equal(&hashA, &hashB));
			DE_TEST_ASSERT(deSha1_equal(&hashB, &hashA));
		}
	}

	/* Compare valid different hash strings for equality. */
	{
		size_t stringANdx;
		size_t stringBNdx;

		for (stringANdx = 0; stringANdx < DE_LENGTH_OF_ARRAY(validHashStrings); stringANdx++)
		for (stringBNdx = 0; stringBNdx < DE_LENGTH_OF_ARRAY(validHashStrings); stringBNdx++)
		{
			deSha1 hashA;
			deSha1 hashB;

			if (stringANdx == stringBNdx)
				continue;

			deMemset(&hashA, garbage, sizeof(deSha1));
			deMemset(&hashB, garbage, sizeof(deSha1));

			DE_TEST_ASSERT(deSha1_parse(&hashA, validHashStrings[stringANdx]));
			DE_TEST_ASSERT(deSha1_parse(&hashB, validHashStrings[stringBNdx]));

			DE_TEST_ASSERT(!deSha1_equal(&hashA, &hashB));
			DE_TEST_ASSERT(!deSha1_equal(&hashB, &hashA));
		}
	}

	/* Test rendering hash as string. */
	{
		size_t stringNdx;

		for (stringNdx = 0; stringNdx < DE_LENGTH_OF_ARRAY(validHashStrings); stringNdx++)
		{
			char	result[40];
			deSha1	hash;

			deMemset(&hash, garbage, sizeof(hash));
			deMemset(&result, garbage, sizeof(result));

			DE_TEST_ASSERT(deSha1_parse(&hash, validHashStrings[stringNdx]));
			deSha1_render(&hash, result);

			DE_TEST_ASSERT(strncmp(result, validHashStrings[stringNdx], 40) == 0);
		}
	}

	/* Test hash against few pre-computed cases. */
	{
		size_t ndx;

		for (ndx = 0; ndx < DE_LENGTH_OF_ARRAY(stringHashPairs); ndx++)
		{
			deSha1 result;
			deSha1 reference;

			deSha1_compute(&result, strlen(stringHashPairs[ndx].data),  stringHashPairs[ndx].data);
			DE_TEST_ASSERT(deSha1_parse(&reference, stringHashPairs[ndx].hash));

			DE_TEST_ASSERT(deSha1_equal(&reference, &result));
		}
	}

	/* Test hash stream against few pre-computed cases. */
	{
		size_t ndx;

		for (ndx = 0; ndx < DE_LENGTH_OF_ARRAY(stringHashPairs); ndx++)
		{
			const char* const	data	= stringHashPairs[ndx].data;
			const size_t		size	= strlen(data);

			deSha1Stream		stream;
			deSha1				result;
			deSha1				reference;

			deSha1Stream_init(&stream);

			deSha1Stream_process(&stream, size/2, data);
			deSha1Stream_process(&stream, size - (size/2), data + size/2);

			deSha1Stream_finalize(&stream, &result);

			deSha1_compute(&result, strlen(stringHashPairs[ndx].data),  stringHashPairs[ndx].data);
			DE_TEST_ASSERT(deSha1_parse(&reference, stringHashPairs[ndx].hash));

			DE_TEST_ASSERT(deSha1_equal(&reference, &result));
		}
	}
}

DE_END_EXTERN_C
