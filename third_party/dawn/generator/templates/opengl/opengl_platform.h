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

#include <KHR/khrplatform.h>

using GLvoid = void;
using GLchar = char;
using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLbyte = khronos_int8_t;
using GLshort = short;
using GLint = int;
using GLsizei = int;
using GLubyte = khronos_uint8_t;
using GLushort = unsigned short;
using GLuint = unsigned int;
using GLfloat = khronos_float_t;
using GLclampf = khronos_float_t;
using GLdouble = double;
using GLclampd = double;
using GLfixed = khronos_int32_t;
using GLintptr = khronos_intptr_t;
using GLsizeiptr = khronos_ssize_t;
using GLhalf = unsigned short;
using GLint64 = khronos_int64_t;
using GLuint64 = khronos_uint64_t;
using GLsync = struct __GLsync*;
using GLDEBUGPROC = void(KHRONOS_APIENTRY*)(GLenum source,
                                            GLenum type,
                                            GLuint id,
                                            GLenum severity,
                                            GLsizei length,
                                            const GLchar* message,
                                            const void* userParam);
using GLDEBUGPROCARB = GLDEBUGPROC;
using GLDEBUGPROCKHR = GLDEBUGPROC;
using GLDEBUGPROCAMD = void(KHRONOS_APIENTRY*)(GLuint id,
                                               GLenum category,
                                               GLenum severity,
                                               GLsizei length,
                                               const GLchar* message,
                                               void* userParam);

{% for block in header_blocks %}
    // {{block.description}}
    {% for enum in block.enums %}
        #define {{enum.name}} {{enum.value}}
    {% endfor %}

    {% for proc in block.procs %}
        using {{proc.PFNGLPROCNAME()}} = {{proc.return_type}}(KHRONOS_APIENTRY *)(
            {%- for param in proc.params -%}
                {%- if not loop.first %}, {% endif -%}
                {{param.type}} {{param.name}}
            {%- endfor -%}
        );
    {% endfor %}

{% endfor%}
#undef DAWN_GL_APIENTRY
