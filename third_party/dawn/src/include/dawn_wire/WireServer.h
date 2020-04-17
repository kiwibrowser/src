// Copyright 2019 The Dawn Authors
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

#ifndef DAWNWIRE_WIRESERVER_H_
#define DAWNWIRE_WIRESERVER_H_

#include <memory>

#include "dawn_wire/Wire.h"

namespace dawn_wire {

    namespace server {
        class Server;
    }

    class DAWN_WIRE_EXPORT WireServer : public CommandHandler {
      public:
        WireServer(DawnDevice device, const DawnProcTable& procs, CommandSerializer* serializer);
        ~WireServer();

        const char* HandleCommands(const char* commands, size_t size) override final;

        bool InjectTexture(DawnTexture texture, uint32_t id, uint32_t generation);

      private:
        std::unique_ptr<server::Server> mImpl;
    };

}  // namespace dawn_wire

#endif  // DAWNWIRE_WIRESERVER_H_
