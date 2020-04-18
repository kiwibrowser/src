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

#ifndef sw_CPUID_hpp
#define sw_CPUID_hpp

namespace sw
{
	#if !defined(__i386__) && defined(_M_IX86)
		#define __i386__ 1
	#endif

	#if !defined(__x86_64__) && (defined(_M_AMD64) || defined (_M_X64))
		#define __x86_64__ 1
	#endif

	class CPUID
	{
	public:
		static bool supportsMMX();
		static bool supportsCMOV();
		static bool supportsMMX2();   // MMX instructions added by SSE: pshufw, pmulhuw, pmovmskb, pavgw/b, pextrw, pinsrw, pmaxsw/ub, etc.
		static bool supportsSSE();
		static bool supportsSSE2();
		static bool supportsSSE3();
		static bool supportsSSSE3();
		static bool supportsSSE4_1();
		static int coreCount();
		static int processAffinity();

		static void setEnableMMX(bool enable);
		static void setEnableCMOV(bool enable);
		static void setEnableSSE(bool enable);
		static void setEnableSSE2(bool enable);
		static void setEnableSSE3(bool enable);
		static void setEnableSSSE3(bool enable);
		static void setEnableSSE4_1(bool enable);

		static void setFlushToZero(bool enable);        // Denormal results are written as zero
		static void setDenormalsAreZero(bool enable);   // Denormal inputs are read as zero

	private:
		static bool MMX;
		static bool CMOV;
		static bool SSE;
		static bool SSE2;
		static bool SSE3;
		static bool SSSE3;
		static bool SSE4_1;
		static int cores;
		static int affinity;

		static bool enableMMX;
		static bool enableCMOV;
		static bool enableSSE;
		static bool enableSSE2;
		static bool enableSSE3;
		static bool enableSSSE3;
		static bool enableSSE4_1;

		static bool detectMMX();
		static bool detectCMOV();
		static bool detectSSE();
		static bool detectSSE2();
		static bool detectSSE3();
		static bool detectSSSE3();
		static bool detectSSE4_1();
		static int detectCoreCount();
		static int detectAffinity();
	};
}

namespace sw
{
	inline bool CPUID::supportsMMX()
	{
		return MMX && enableMMX;
	}

	inline bool CPUID::supportsCMOV()
	{
		return CMOV && enableCMOV;
	}

	inline bool CPUID::supportsMMX2()
	{
		return supportsSSE();   // Coincides with 64-bit integer vector instructions supported by SSE
	}

	inline bool CPUID::supportsSSE()
	{
		return SSE && enableSSE;
	}

	inline bool CPUID::supportsSSE2()
	{
		return SSE2 && enableSSE2;
	}

	inline bool CPUID::supportsSSE3()
	{
		return SSE3 && enableSSE3;
	}

	inline bool CPUID::supportsSSSE3()
	{
		return SSSE3 && enableSSSE3;
	}

	inline bool CPUID::supportsSSE4_1()
	{
		return SSE4_1 && enableSSE4_1;
	}

	inline int CPUID::coreCount()
	{
		return cores;
	}

	inline int CPUID::processAffinity()
	{
		return affinity;
	}
}

#endif   // sw_CPUID_hpp
