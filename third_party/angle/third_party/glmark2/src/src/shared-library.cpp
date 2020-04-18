/*
 * Copyright © 2019 Jamie Madill
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Jamie Madill
 */
#include "shared-library.h"

#include <stdio.h>

#if defined(WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

/******************
 * Public methods *
 ******************/
SharedLibrary::SharedLibrary() : handle_(nullptr) {}

SharedLibrary::~SharedLibrary()
{
    close();
}

bool SharedLibrary::open(const char *libName)
{
#if defined(WIN32)
    handle_ = LoadLibraryA(libName);
#else
    handle_ = dlopen(libName, RTLD_NOW | RTLD_NODELETE);
#endif
    return (handle_ != nullptr);
}

bool SharedLibrary::open_from_alternatives(std::initializer_list<const char*> alt_names)
{
    for (const char *name : alt_names) {
        if (open(name))
            return true;
    }

    return false;
}

void SharedLibrary::close()
{
    if (handle_)
    {
#if defined(WIN32)
        FreeLibrary(reinterpret_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif
    }
}

void *SharedLibrary::handle() const
{
    return handle_;
}

void *SharedLibrary::load(const char *symbol) const
{
#if defined(WIN32)
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle_), symbol));
#else
    return dlsym(handle_, symbol);
#endif
}

/*******************
 * Private methods *
 *******************/
