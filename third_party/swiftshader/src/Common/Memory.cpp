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

#include "Memory.hpp"

#include "Types.hpp"
#include "Debug.hpp"

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <intrin.h>
#else
	#include <errno.h>
	#include <sys/mman.h>
	#include <stdlib.h>
	#include <unistd.h>
#endif

#include <memory.h>

#undef allocate
#undef deallocate

#if (defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined (_M_X64)) && !defined(__x86__)
#define __x86__
#endif

namespace sw
{
namespace
{
struct Allocation
{
//	size_t bytes;
	unsigned char *block;
};

void *allocateRaw(size_t bytes, size_t alignment)
{
	ASSERT((alignment & (alignment - 1)) == 0);   // Power of 2 alignment.

	#if defined(LINUX_ENABLE_NAMED_MMAP)
		void *allocation;
		int result = posix_memalign(&allocation, alignment, bytes);
		if(result != 0)
		{
			errno = result;
			allocation = nullptr;
		}
		return allocation;
	#else
		unsigned char *block = new unsigned char[bytes + sizeof(Allocation) + alignment];
		unsigned char *aligned = nullptr;

		if(block)
		{
			aligned = (unsigned char*)((uintptr_t)(block + sizeof(Allocation) + alignment - 1) & -(intptr_t)alignment);
			Allocation *allocation = (Allocation*)(aligned - sizeof(Allocation));

		//	allocation->bytes = bytes;
			allocation->block = block;
		}

		return aligned;
	#endif
}

#if defined(LINUX_ENABLE_NAMED_MMAP)
// Create a file descriptor for anonymous memory with the given
// name. Returns -1 on failure.
// TODO: remove once libc wrapper exists.
int memfd_create(const char* name, unsigned int flags)
{
	#if __aarch64__
	#define __NR_memfd_create 279
	#elif __arm__
	#define __NR_memfd_create 279
	#elif __powerpc64__
	#define __NR_memfd_create 360
	#elif __i386__
	#define __NR_memfd_create 356
	#elif __x86_64__
	#define __NR_memfd_create 319
	#endif /* __NR_memfd_create__ */
	#ifdef __NR_memfd_create
		// In the event of no system call this returns -1 with errno set
		// as ENOSYS.
		return syscall(__NR_memfd_create, name, flags);
	#else
		return -1;
	#endif
}

// Returns a file descriptor for use with an anonymous mmap, if
// memfd_create fails, -1 is returned. Note, the mappings should be
// MAP_PRIVATE so that underlying pages aren't shared.
int anonymousFd()
{
	static int fd = memfd_create("SwiftShader JIT", 0);
	return fd;
}

// Ensure there is enough space in the "anonymous" fd for length.
void ensureAnonFileSize(int anonFd, size_t length)
{
	static size_t fileSize = 0;
	if(length > fileSize)
	{
		ftruncate(anonFd, length);
		fileSize = length;
	}
}
#endif  // defined(LINUX_ENABLE_NAMED_MMAP)

}  // anonymous namespace

size_t memoryPageSize()
{
	static int pageSize = 0;

	if(pageSize == 0)
	{
		#if defined(_WIN32)
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			pageSize = systemInfo.dwPageSize;
		#else
			pageSize = sysconf(_SC_PAGESIZE);
		#endif
	}

	return pageSize;
}

void *allocate(size_t bytes, size_t alignment)
{
	void *memory = allocateRaw(bytes, alignment);

	if(memory)
	{
		memset(memory, 0, bytes);
	}

	return memory;
}

void deallocate(void *memory)
{
	#if defined(LINUX_ENABLE_NAMED_MMAP)
		free(memory);
	#else
		if(memory)
		{
			unsigned char *aligned = (unsigned char*)memory;
			Allocation *allocation = (Allocation*)(aligned - sizeof(Allocation));

			delete[] allocation->block;
		}
	#endif
}

void *allocateExecutable(size_t bytes)
{
	size_t pageSize = memoryPageSize();
	size_t length = (bytes + pageSize - 1) & ~(pageSize - 1);
	void *mapping;

	#if defined(LINUX_ENABLE_NAMED_MMAP)
		// Try to name the memory region for the executable code,
		// to aid profilers.
		int anonFd = anonymousFd();
		if(anonFd == -1)
		{
			mapping = mmap(nullptr, length, PROT_READ | PROT_WRITE,
			               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		}
		else
		{
			ensureAnonFileSize(anonFd, length);
			mapping = mmap(nullptr, length, PROT_READ | PROT_WRITE,
			               MAP_PRIVATE, anonFd, 0);
		}

		if(mapping == MAP_FAILED)
		{
			mapping = nullptr;
		}
	#else
		mapping = allocate(length, pageSize);
	#endif

	return mapping;
}

void markExecutable(void *memory, size_t bytes)
{
	#if defined(_WIN32)
		unsigned long oldProtection;
		VirtualProtect(memory, bytes, PAGE_EXECUTE_READ, &oldProtection);
	#else
		mprotect(memory, bytes, PROT_READ | PROT_EXEC);
	#endif
}

void deallocateExecutable(void *memory, size_t bytes)
{
	#if defined(_WIN32)
		unsigned long oldProtection;
		VirtualProtect(memory, bytes, PAGE_READWRITE, &oldProtection);
		deallocate(memory);
	#elif defined(LINUX_ENABLE_NAMED_MMAP)
		size_t pageSize = memoryPageSize();
		size_t length = (bytes + pageSize - 1) & ~(pageSize - 1);
		munmap(memory, length);
	#else
		mprotect(memory, bytes, PROT_READ | PROT_WRITE);
		deallocate(memory);
	#endif
}

void clear(uint16_t *memory, uint16_t element, size_t count)
{
	#if defined(_MSC_VER) && defined(__x86__)
		__stosw(memory, element, count);
	#elif defined(__GNUC__) && defined(__x86__)
		__asm__("rep stosw" : : "D"(memory), "a"(element), "c"(count));
	#else
		for(size_t i = 0; i < count; i++)
		{
			memory[i] = element;
		}
	#endif
}

void clear(uint32_t *memory, uint32_t element, size_t count)
{
	#if defined(_MSC_VER) && defined(__x86__)
		__stosd((unsigned long*)memory, element, count);
	#elif defined(__GNUC__) && defined(__x86__)
		__asm__("rep stosl" : : "D"(memory), "a"(element), "c"(count));
	#else
		for(size_t i = 0; i < count; i++)
		{
			memory[i] = element;
		}
	#endif
}
}
