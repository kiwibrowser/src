# Copyright 2020 The Dawn Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

DawnJSONGenerator(
    TARGET "dawn_wire"
    PRINT_NAME "Dawn wire"
    RESULT_VARIABLE "DAWN_WIRE_GEN_SOURCES"
)

add_library(dawn_wire STATIC ${DAWN_DUMMY_FILE})
target_sources(dawn_wire PRIVATE
    "${DAWN_INCLUDE_DIR}/dawn_wire/Wire.h"
    "${DAWN_INCLUDE_DIR}/dawn_wire/WireClient.h"
    "${DAWN_INCLUDE_DIR}/dawn_wire/WireServer.h"
    "${DAWN_INCLUDE_DIR}/dawn_wire/dawn_wire_export.h"
    ${DAWN_WIRE_GEN_SOURCES}
    "WireClient.cpp"
    "WireDeserializeAllocator.cpp"
    "WireDeserializeAllocator.h"
    "WireServer.cpp"
    "client/ApiObjects.h"
    "client/Buffer.cpp"
    "client/Buffer.h"
    "client/Client.cpp"
    "client/Client.h"
    "client/ClientDoers.cpp"
    "client/ClientInlineMemoryTransferService.cpp"
    "client/Device.cpp"
    "client/Device.h"
    "client/Fence.cpp"
    "client/Fence.h"
    "client/ObjectAllocator.h"
    "client/Queue.cpp"
    "client/Queue.h"
    "server/ObjectStorage.h"
    "server/Server.cpp"
    "server/Server.h"
    "server/ServerBuffer.cpp"
    "server/ServerDevice.cpp"
    "server/ServerFence.cpp"
    "server/ServerInlineMemoryTransferService.cpp"
    "server/ServerQueue.cpp"
)
target_link_libraries(dawn_wire
    PUBLIC dawn_headers
    PRIVATE dawn_common dawn_internal_config
)
