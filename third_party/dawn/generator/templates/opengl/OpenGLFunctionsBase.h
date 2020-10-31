//* Copyright 2019 The Dawn Authors
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at
//*
//*     http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.

#include "dawn_native/Error.h"
#include "dawn_native/opengl/opengl_platform.h"

namespace dawn_native { namespace opengl {
    using GetProcAddress = void* (*) (const char*);

    struct OpenGLFunctionsBase {
      public:
        {% for block in header_blocks %}
            // {{block.description}}
            {% for proc in block.procs %}
                {{proc.PFNGLPROCNAME()}} {{proc.ProcName()}} = nullptr;
            {% endfor %}

        {% endfor%}

      protected:
        MaybeError LoadDesktopGLProcs(GetProcAddress getProc, int majorVersion, int minorVersion);
        MaybeError LoadOpenGLESProcs(GetProcAddress getProc, int majorVersion, int minorVersion);

      private:
        template<typename T>
        MaybeError LoadProc(GetProcAddress getProc, T* memberProc, const char* name);
    };

}}  // namespace dawn_native::opengl
