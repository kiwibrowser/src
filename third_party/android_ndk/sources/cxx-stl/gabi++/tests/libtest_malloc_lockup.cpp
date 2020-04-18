// Copyright (C) 2011 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include <stdlib.h>
#include <unistd.h>

#include <cxxabi.h>

#define PAYLOAD(s)   s, sizeof(s) - 1

// Override malloc() and free() to ensure they are never called!
//
// Because GAbi++ is statically linked into this shared library with -Bsymbolic,
// any malloc() and free() calls it contains will be compiled as direct calls to
// this function, bypassing the dynamic linker's PLT indirections and avoiding
// the symbols from the C library.
extern "C" void* malloc(size_t size) {
  write(2, PAYLOAD("ERROR: malloc called!"));
  exit(1);
}

extern "C" void free(void*) {
  write(2, PAYLOAD("ERROR: free called!"));
  exit(2);
}

// Called by the test program to force creation of a thread-specific
// C++ runtime object.
extern "C" void* get_globals() {
  return (void*) __cxxabiv1::__cxa_get_globals();
}
