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

#include "CPUID.hpp"

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <intrin.h>
	#include <float.h>
#else
	#include <unistd.h>
	#include <sched.h>
	#include <sys/types.h>
#endif

namespace sw
{
	bool CPUID::MMX = detectMMX();
	bool CPUID::CMOV = detectCMOV();
	bool CPUID::SSE = detectSSE();
	bool CPUID::SSE2 = detectSSE2();
	bool CPUID::SSE3 = detectSSE3();
	bool CPUID::SSSE3 = detectSSSE3();
	bool CPUID::SSE4_1 = detectSSE4_1();
	int CPUID::cores = detectCoreCount();
	int CPUID::affinity = detectAffinity();

	bool CPUID::enableMMX = true;
	bool CPUID::enableCMOV = true;
	bool CPUID::enableSSE = true;
	bool CPUID::enableSSE2 = true;
	bool CPUID::enableSSE3 = true;
	bool CPUID::enableSSSE3 = true;
	bool CPUID::enableSSE4_1 = true;

	void CPUID::setEnableMMX(bool enable)
	{
		enableMMX = enable;

		if(!enableMMX)
		{
			enableSSE = false;
			enableSSE2 = false;
			enableSSE3 = false;
			enableSSSE3 = false;
			enableSSE4_1 = false;
		}
	}

	void CPUID::setEnableCMOV(bool enable)
	{
		enableCMOV = enable;

		if(!CMOV)
		{
			enableSSE = false;
			enableSSE2 = false;
			enableSSE3 = false;
			enableSSSE3 = false;
			enableSSE4_1 = false;
		}
	}

	void CPUID::setEnableSSE(bool enable)
	{
		enableSSE = enable;

		if(enableSSE)
		{
			enableMMX = true;
			enableCMOV = true;
		}
		else
		{
			enableSSE2 = false;
			enableSSE3 = false;
			enableSSSE3 = false;
			enableSSE4_1 = false;
		}
	}

	void CPUID::setEnableSSE2(bool enable)
	{
		enableSSE2 = enable;

		if(enableSSE2)
		{
			enableMMX = true;
			enableCMOV = true;
			enableSSE = true;
		}
		else
		{
			enableSSE3 = false;
			enableSSSE3 = false;
			enableSSE4_1 = false;
		}
	}

	void CPUID::setEnableSSE3(bool enable)
	{
		enableSSE3 = enable;

		if(enableSSE3)
		{
			enableMMX = true;
			enableCMOV = true;
			enableSSE = true;
			enableSSE2 = true;
		}
		else
		{
			enableSSSE3 = false;
			enableSSE4_1 = false;
		}
	}

	void CPUID::setEnableSSSE3(bool enable)
	{
		enableSSSE3 = enable;

		if(enableSSSE3)
		{
			enableMMX = true;
			enableCMOV = true;
			enableSSE = true;
			enableSSE2 = true;
			enableSSE3 = true;
		}
		else
		{
			enableSSE4_1 = false;
		}
	}

	void CPUID::setEnableSSE4_1(bool enable)
	{
		enableSSE4_1 = enable;

		if(enableSSE4_1)
		{
			enableMMX = true;
			enableCMOV = true;
			enableSSE = true;
			enableSSE2 = true;
			enableSSE3 = true;
			enableSSSE3 = true;
		}
	}

	static void cpuid(int registers[4], int info)
	{
		#if defined(__i386__) || defined(__x86_64__)
			#if defined(_WIN32)
				__cpuid(registers, info);
			#else
				__asm volatile("cpuid": "=a" (registers[0]), "=b" (registers[1]), "=c" (registers[2]), "=d" (registers[3]): "a" (info));
			#endif
		#else
			registers[0] = 0;
			registers[1] = 0;
			registers[2] = 0;
			registers[3] = 0;
		#endif
	}

	bool CPUID::detectMMX()
	{
		int registers[4];
		cpuid(registers, 1);
		return MMX = (registers[3] & 0x00800000) != 0;
	}

	bool CPUID::detectCMOV()
	{
		int registers[4];
		cpuid(registers, 1);
		return CMOV = (registers[3] & 0x00008000) != 0;
	}

	bool CPUID::detectSSE()
	{
		int registers[4];
		cpuid(registers, 1);
		return SSE = (registers[3] & 0x02000000) != 0;
	}

	bool CPUID::detectSSE2()
	{
		int registers[4];
		cpuid(registers, 1);
		return SSE2 = (registers[3] & 0x04000000) != 0;
	}

	bool CPUID::detectSSE3()
	{
		int registers[4];
		cpuid(registers, 1);
		return SSE3 = (registers[2] & 0x00000001) != 0;
	}

	bool CPUID::detectSSSE3()
	{
		int registers[4];
		cpuid(registers, 1);
		return SSSE3 = (registers[2] & 0x00000200) != 0;
	}

	bool CPUID::detectSSE4_1()
	{
		int registers[4];
		cpuid(registers, 1);
		return SSE4_1 = (registers[2] & 0x00080000) != 0;
	}

	int CPUID::detectCoreCount()
	{
		int cores = 0;

		#if defined(_WIN32)
			DWORD_PTR processAffinityMask = 1;
			DWORD_PTR systemAffinityMask = 1;

			GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);

			while(systemAffinityMask)
			{
				if(systemAffinityMask & 1)
				{
					cores++;
				}

				systemAffinityMask >>= 1;
			}
		#else
			cores = sysconf(_SC_NPROCESSORS_ONLN);
		#endif

		if(cores < 1)  cores = 1;
		if(cores > 16) cores = 16;

		return cores;   // FIXME: Number of physical cores
	}

	int CPUID::detectAffinity()
	{
		int cores = 0;

		#if defined(_WIN32)
			DWORD_PTR processAffinityMask = 1;
			DWORD_PTR systemAffinityMask = 1;

			GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);

			while(processAffinityMask)
			{
				if(processAffinityMask & 1)
				{
					cores++;
				}

				processAffinityMask >>= 1;
			}
		#else
			return detectCoreCount();   // FIXME: Assumes no affinity limitation
		#endif

		if(cores < 1)  cores = 1;
		if(cores > 16) cores = 16;

		return cores;
	}

	void CPUID::setFlushToZero(bool enable)
	{
		#if defined(_MSC_VER)
			_controlfp(enable ? _DN_FLUSH : _DN_SAVE, _MCW_DN);
		#else
			// Unimplemented
		#endif
	}

	void CPUID::setDenormalsAreZero(bool enable)
	{
		// Unimplemented
	}
}
