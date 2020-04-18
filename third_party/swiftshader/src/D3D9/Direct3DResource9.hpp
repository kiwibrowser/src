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

#ifndef D3D9_Direct3DResource9_hpp
#define D3D9_Direct3DResource9_hpp

#include "Unknown.hpp"

#include <d3d9.h>

#include <map>

namespace D3D9
{
	class Direct3DDevice9;

	class Direct3DResource9 : public IDirect3DResource9, public Unknown
	{
	public:
		Direct3DResource9(Direct3DDevice9 *device, D3DRESOURCETYPE type, D3DPOOL pool, unsigned int size);

		~Direct3DResource9() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DResource9 methods
		long __stdcall GetDevice(IDirect3DDevice9 **device) override;
		long __stdcall SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags) override;
		long __stdcall GetPrivateData(const GUID &guid, void *data, unsigned long *size) override;
		long __stdcall FreePrivateData(const GUID &guid) override;
		unsigned long __stdcall SetPriority(unsigned long newPriority) override;
		unsigned long __stdcall GetPriority() override;
		void __stdcall PreLoad() override;
		D3DRESOURCETYPE __stdcall GetType() override;

		// Internal methods
		static unsigned int getMemoryUsage();
		D3DPOOL getPool() const;

	protected:
		// Creation parameters
		Direct3DDevice9 *const device;
		const D3DRESOURCETYPE type;
		const D3DPOOL pool;
		const unsigned int size;

	private:
		unsigned long priority;

		struct PrivateData
		{
			PrivateData();
			PrivateData(const void *data, int size, bool managed);

			~PrivateData();

			PrivateData &operator=(const PrivateData &privateData);

			void *data;
			unsigned long size;
			bool managed;   // IUnknown interface
		};

		struct CompareGUID
		{
			bool operator()(const GUID& left, const GUID& right) const
			{
				return memcmp(&left, &right, sizeof(GUID)) < 0;
			}
		};

		typedef std::map<GUID, PrivateData, CompareGUID> PrivateDataMap;
		typedef PrivateDataMap::iterator Iterator;
		PrivateDataMap privateData;

		static unsigned int memoryUsage;
	};
}

#endif   // D3D9_Direct3DResource9_hpp
