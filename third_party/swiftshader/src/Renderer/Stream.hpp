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

#ifndef sw_Stream_hpp
#define sw_Stream_hpp

#include "Common/Types.hpp"

namespace sw
{
	class Resource;

	enum StreamType ENUM_UNDERLYING_TYPE_UNSIGNED_INT
	{
		STREAMTYPE_COLOR,     // 4 normalized unsigned bytes, ZYXW order
		STREAMTYPE_UDEC3,     // 3 unsigned 10-bit fields
		STREAMTYPE_DEC3N,     // 3 normalized signed 10-bit fields
		STREAMTYPE_INDICES,   // 4 unsigned bytes, stored unconverted into X component
		STREAMTYPE_FLOAT,     // Normalization ignored
		STREAMTYPE_BYTE,
		STREAMTYPE_SBYTE,
		STREAMTYPE_SHORT,
		STREAMTYPE_USHORT,
		STREAMTYPE_INT,
		STREAMTYPE_UINT,
		STREAMTYPE_FIXED,     // Normalization ignored (16.16 format)
		STREAMTYPE_HALF,      // Normalization ignored
		STREAMTYPE_2_10_10_10_INT,
		STREAMTYPE_2_10_10_10_UINT,

		STREAMTYPE_LAST = STREAMTYPE_2_10_10_10_UINT
	};

	struct StreamResource
	{
		Resource *resource;
		const void *buffer;
		unsigned int stride;
	};

	struct Stream : public StreamResource
	{
		Stream(Resource *resource = 0, const void *buffer = 0, unsigned int stride = 0)
		{
			this->resource = resource;
			this->buffer = buffer;
			this->stride = stride;
		}

		Stream &define(StreamType type, unsigned int count, bool normalized = false)
		{
			this->type = type;
			this->count = count;
			this->normalized = normalized;

			return *this;
		}

		Stream &define(const void *buffer, StreamType type, unsigned int count, bool normalized = false)
		{
			this->buffer = buffer;
			this->type = type;
			this->count = count;
			this->normalized = normalized;

			return *this;
		}

		Stream &defaults()
		{
			static const float4 null = {0, 0, 0, 1};

			resource = 0;
			buffer = &null;
			stride = 0;
			type = STREAMTYPE_FLOAT;
			count = 0;
			normalized = false;

			return *this;
		}

		operator bool() const   // Returns true if stream contains data
		{
			return count != 0;
		}

		StreamType type;
		unsigned char count;
		bool normalized;
	};
}

#endif   // sw_Stream_hpp
