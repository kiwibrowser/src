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

#ifndef SharedLibrary_hpp
#define SharedLibrary_hpp

#if defined(_WIN32)
	#include <Windows.h>
#else
	#include <dlfcn.h>
#endif

#include <string>

void *getLibraryHandle(const char *path);
void *loadLibrary(const char *path);
void freeLibrary(void *library);
void *getProcAddress(void *library, const char *name);

template<int n>
void *loadLibrary(const std::string &libraryDirectory, const char *(&names)[n], const char *mustContainSymbol = nullptr)
{
	if(!libraryDirectory.empty())
	{
		for(int i = 0; i < n; i++)
		{
			std::string nameWithPath = libraryDirectory + names[i];
			void *library = getLibraryHandle(nameWithPath.c_str());

			if(library)
			{
				if(!mustContainSymbol || getProcAddress(library, mustContainSymbol))
				{
					return library;
				}

				freeLibrary(library);
			}
		}
	}

	for(int i = 0; i < n; i++)
	{
		void *library = getLibraryHandle(names[i]);

		if(library)
		{
			if(!mustContainSymbol || getProcAddress(library, mustContainSymbol))
			{
				return library;
			}

			freeLibrary(library);
		}
	}

	for(int i = 0; i < n; i++)
	{
		void *library = loadLibrary(names[i]);

		if(library)
		{
			if(!mustContainSymbol || getProcAddress(library, mustContainSymbol))
			{
				return library;
			}

			freeLibrary(library);
		}
	}

	return nullptr;
}

#if defined(_WIN32)
	inline void *loadLibrary(const char *path)
	{
		return (void*)LoadLibrary(path);
	}

	inline void *getLibraryHandle(const char *path)
	{
		HMODULE module = NULL;
		GetModuleHandleEx(0, path, &module);
		return (void*)module;
	}

	inline void freeLibrary(void *library)
	{
		FreeLibrary((HMODULE)library);
	}

	inline void *getProcAddress(void *library, const char *name)
	{
		return (void*)GetProcAddress((HMODULE)library, name);
	}

	inline std::string getLibraryDirectoryFromSymbol(void* symbol)
	{
		HMODULE module = NULL;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)symbol, &module);
		char filename[1024];
		if(module && (GetModuleFileName(module, filename, sizeof(filename)) != 0))
		{
			std::string directory(filename);
			return directory.substr(0, directory.find_last_of("\\/") + 1).c_str();
		}
		else
		{
			return "";
		}
	}
#else
	inline void *loadLibrary(const char *path)
	{
		return dlopen(path, RTLD_LAZY | RTLD_LOCAL);
	}

	inline void *getLibraryHandle(const char *path)
	{
		#ifdef __ANDROID__
			// bionic doesn't support RTLD_NOLOAD before L
			return dlopen(path, RTLD_NOW | RTLD_LOCAL);
		#else
			void *resident = dlopen(path, RTLD_LAZY | RTLD_NOLOAD | RTLD_LOCAL);

			if(resident)
			{
				return dlopen(path, RTLD_LAZY | RTLD_LOCAL);   // Increment reference count
			}

			return nullptr;
		#endif
	}

	inline void freeLibrary(void *library)
	{
		if(library)
		{
			dlclose(library);
		}
	}

	inline void *getProcAddress(void *library, const char *name)
	{
		void *symbol = dlsym(library, name);

		if(!symbol)
		{
			const char *reason = dlerror();   // Silence the error
			(void)reason;
		}

		return symbol;
	}

	inline std::string getLibraryDirectoryFromSymbol(void* symbol)
	{
		Dl_info dl_info;
		if(dladdr(symbol, &dl_info) != 0)
		{
			std::string directory(dl_info.dli_fname);
			return directory.substr(0, directory.find_last_of("\\/") + 1).c_str();
		}
		else
		{
			return "";
		}
	}
#endif

#endif   // SharedLibrary_hpp
