// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/opengl/QueueGL.h"

#include "dawn_native/opengl/BufferGL.h"
#include "dawn_native/opengl/CommandBufferGL.h"
#include "dawn_native/opengl/DeviceGL.h"
#include "dawn_platform/DawnPlatform.h"
#include "dawn_platform/tracing/TraceEvent.h"

namespace dawn_native { namespace opengl {

    Queue::Queue(Device* device) : QueueBase(device) {
    }

    MaybeError Queue::SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) {
        Device* device = ToBackend(GetDevice());

        TRACE_EVENT_BEGIN0(GetDevice()->GetPlatform(), Recording, "CommandBufferGL::Execute");
        for (uint32_t i = 0; i < commandCount; ++i) {
            ToBackend(commands[i])->Execute();
        }
        TRACE_EVENT_END0(GetDevice()->GetPlatform(), Recording, "CommandBufferGL::Execute");

        device->SubmitFenceSync();
        return {};
    }

    MaybeError Queue::WriteBufferImpl(BufferBase* buffer,
                                      uint64_t bufferOffset,
                                      const void* data,
                                      size_t size) {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;

        ToBackend(buffer)->EnsureDataInitializedAsDestination(bufferOffset, size);

        gl.BindBuffer(GL_ARRAY_BUFFER, ToBackend(buffer)->GetHandle());
        gl.BufferSubData(GL_ARRAY_BUFFER, bufferOffset, size, data);
        return {};
    }

}}  // namespace dawn_native::opengl
