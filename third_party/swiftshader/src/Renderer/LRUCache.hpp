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

#ifndef sw_LRUCache_hpp
#define sw_LRUCache_hpp

#include "Common/Math.hpp"

namespace sw
{
	template<class Key, class Data>
	class LRUCache
	{
	public:
		LRUCache(int n);

		~LRUCache();

		Data *query(const Key &key) const;
		Data *add(const Key &key, Data *data);
	
		int getSize() {return size;}
		Key &getKey(int i) {return key[i];}

	private:
		int size;
		int mask;
		int top;
		int fill;

		Key *key;
		Key **ref;
		Data **data;
	};
}

namespace sw
{
	template<class Key, class Data>
	LRUCache<Key, Data>::LRUCache(int n)
	{
		size = ceilPow2(n);
		mask = size - 1;
		top = 0;
		fill = 0;

		key = new Key[size];
		ref = new Key*[size];
		data = new Data*[size];

		for(int i = 0; i < size; i++)
		{
			data[i] = nullptr;

			ref[i] = &key[i];
		}
	}

	template<class Key, class Data>
	LRUCache<Key, Data>::~LRUCache()
	{
		delete[] key;
		key = nullptr;

		delete[] ref;
		ref = nullptr;

		for(int i = 0; i < size; i++)
		{
			if(data[i])
			{
				data[i]->unbind();
				data[i] = nullptr;
			}
		}

		delete[] data;
		data = nullptr;
	}

	template<class Key, class Data>
	Data *LRUCache<Key, Data>::query(const Key &key) const
	{
		for(int i = top; i > top - fill; i--)
		{
			int j = i & mask;

			if(key == *ref[j])
			{
				Data *hit = data[j];

				if(i != top)
				{
					// Move one up
					int k = (j + 1) & mask;

					Data *swapD = data[k];
					data[k] = data[j];
					data[j] = swapD;

					Key *swapK = ref[k];
					ref[k] = ref[j];
					ref[j] = swapK;
				}

				return hit;
			}
		}

		return nullptr;   // Not found
	}

	template<class Key, class Data>
	Data *LRUCache<Key, Data>::add(const Key &key, Data *data)
	{
		top = (top + 1) & mask;
		fill = fill + 1 < size ? fill + 1 : size;

		*ref[top] = key;

		data->bind();

		if(this->data[top])
		{
			this->data[top]->unbind();
		}

		this->data[top] = data;

		return data;
	}
}

#endif   // sw_LRUCache_hpp
