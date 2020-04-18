/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Utility class to build seeds from different data types.
 *
 * Values are first XORed with type specifig mask, which makes sure that
 * two values with different types, but same bit presentation produce
 * different results. Then values are passed through 32 bit crc.
 *//*--------------------------------------------------------------------*/

#include "tcuSeedBuilder.hpp"

#include "deMemory.h"

namespace tcu
{

namespace
{

deUint32 advanceCrc32 (deUint32 oldCrc, size_t len, const deUint8* data)
{
	const deUint32	generator	= 0x04C11DB7u;
	deUint32		crc			= oldCrc;

	for (size_t i = 0; i < len; i++)
	{
		const deUint32 current = static_cast<deUint32>(data[i]);
		crc = crc ^ current;

		for (size_t bitNdx = 0; bitNdx < 8; bitNdx++)
		{
			if (crc & 1u)
				crc = (crc >> 1u) ^ generator;
			else
				crc = (crc >> 1u);
		}
	}

	return crc;
}

} // anonymous

SeedBuilder::SeedBuilder (void)
	: m_hash (0xccf139d7u)
{
}

void SeedBuilder::feed (size_t size, const void* ptr)
{
	m_hash = advanceCrc32(m_hash, size, (const deUint8*)ptr);
}

SeedBuilder& operator<< (SeedBuilder& builder, bool value)
{
	const deUint8 val = (value ? 54: 7);

	builder.feed(sizeof(val), &val);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deInt8 value)
{
	const deInt8 val = value ^ 75;

	builder.feed(sizeof(val), &val);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deUint8 value)
{
	const deUint8 val = value ^ 140u;

	builder.feed(sizeof(val), &val);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deInt16 value)
{
	const deInt16	val		= value ^ 555;
	const deUint8	data[]	=
	{
		(deUint8)(((deUint16)val) & 0xFFu),
		(deUint8)(((deUint16)val) >> 8),
	};

	builder.feed(sizeof(data), data);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deUint16 value)
{
	const deUint16	val		= value ^ 37323u;
	const deUint8	data[]	=
	{
		(deUint8)(val & 0xFFu),
		(deUint8)(val >> 8),
	};

	builder.feed(sizeof(data), data);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deInt32 value)
{
	const deInt32	val		= value ^ 53054741;
	const deUint8	data[]	=
	{
		(deUint8)(((deUint32)val) & 0xFFu),
		(deUint8)((((deUint32)val) >> 8) & 0xFFu),
		(deUint8)((((deUint32)val) >> 16) & 0xFFu),
		(deUint8)((((deUint32)val) >> 24) & 0xFFu),
	};

	builder.feed(sizeof(data), data);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deUint32 value)
{
	const deUint32	val		= value ^ 1977303630u;
	const deUint8	data[]	=
	{
		(deUint8)(val & 0xFFu),
		(deUint8)((val >> 8) & 0xFFu),
		(deUint8)((val >> 16) & 0xFFu),
		(deUint8)((val >> 24) & 0xFFu),
	};

	builder.feed(sizeof(data), data);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deInt64 value)
{
	const deInt64	val		= value ^ 772935234179004386ll;
	const deUint8	data[]	=
	{
		(deUint8)(((deUint64)val) & 0xFFu),
		(deUint8)((((deUint64)val) >> 8) & 0xFFu),
		(deUint8)((((deUint64)val) >> 16) & 0xFFu),
		(deUint8)((((deUint64)val) >> 24) & 0xFFu),

		(deUint8)((((deUint64)val) >> 32) & 0xFFu),
		(deUint8)((((deUint64)val) >> 40) & 0xFFu),
		(deUint8)((((deUint64)val) >> 48) & 0xFFu),
		(deUint8)((((deUint64)val) >> 56) & 0xFFu),
	};

	builder.feed(sizeof(data), data);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, deUint64 value)
{
	const deUint64	val		= value ^ 4664937258000467599ull;
	const deUint8	data[]	=
	{
		(deUint8)(val & 0xFFu),
		(deUint8)((val >> 8) & 0xFFu),
		(deUint8)((val >> 16) & 0xFFu),
		(deUint8)((val >> 24) & 0xFFu),

		(deUint8)((val >> 32) & 0xFFu),
		(deUint8)((val >> 40) & 0xFFu),
		(deUint8)((val >> 48) & 0xFFu),
		(deUint8)((val >> 56) & 0xFFu),
	};

	builder.feed(sizeof(data), data);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, float value)
{
	// \note Assume that float has same endianess as uint32.
	deUint32 val;

	deMemcpy(&val, &value, sizeof(deUint32));

	{
		const deUint8	data[]	=
		{
			(deUint8)(val & 0xFFu),
			(deUint8)((val >> 8) & 0xFFu),
			(deUint8)((val >> 16) & 0xFFu),
			(deUint8)((val >> 24) & 0xFFu),
		};

		builder.feed(sizeof(data), data);
		return builder;
	}
}

SeedBuilder& operator<< (SeedBuilder& builder, double value)
{
	// \note Assume that double has same endianess as uint64.
	deUint64 val;

	deMemcpy(&val, &value, sizeof(deUint64));

	const deUint8	data[]	=
	{
		(deUint8)(val & 0xFFu),
		(deUint8)((val >> 8) & 0xFFu),
		(deUint8)((val >> 16) & 0xFFu),
		(deUint8)((val >> 24) & 0xFFu),

		(deUint8)((val >> 32) & 0xFFu),
		(deUint8)((val >> 40) & 0xFFu),
		(deUint8)((val >> 48) & 0xFFu),
		(deUint8)((val >> 56) & 0xFFu),
	};

	builder.feed(sizeof(data), data);
	return builder;
}

SeedBuilder& operator<< (SeedBuilder& builder, const std::string& value)
{
	builder.feed(value.length(), value.c_str());
	return builder;
}

} // tcu
