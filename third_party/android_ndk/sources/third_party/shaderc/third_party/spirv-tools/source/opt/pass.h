// Copyright (c) 2016 Google Inc.
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

#ifndef LIBSPIRV_OPT_PASS_H_
#define LIBSPIRV_OPT_PASS_H_

#include <algorithm>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include <utility>

#include "module.h"
#include "spirv-tools/libspirv.hpp"

namespace spvtools {
namespace opt {

// Abstract class of a pass. All passes should implement this abstract class
// and all analysis and transformation is done via the Process() method.
class Pass {
 public:
  // The status of processing a module using a pass.
  //
  // The numbers for the cases are assigned to make sure that Failure & anything
  // is Failure, SuccessWithChange & any success is SuccessWithChange.
  enum class Status {
    Failure = 0x00,
    SuccessWithChange = 0x10,
    SuccessWithoutChange = 0x11,
  };

  using ProcessFunction = std::function<bool(ir::Function*)>;

  // Constructs a new pass.
  //
  // The constructed instance will have an empty message consumer, which just
  // ignores all messages from the library. Use SetMessageConsumer() to supply
  // one if messages are of concern.
  Pass() : consumer_(nullptr) {}

  // Destructs the pass.
  virtual ~Pass() = default;

  // Returns a descriptive name for this pass.
  virtual const char* name() const = 0;

  // Sets the message consumer to the given |consumer|. |consumer| which will be
  // invoked every time there is a message to be communicated to the outside.
  void SetMessageConsumer(MessageConsumer c) { consumer_ = std::move(c); }
  // Returns the reference to the message consumer for this pass.
  const MessageConsumer& consumer() const { return consumer_; }

  // Add to |todo| all ids of functions called in |func|.
  void AddCalls(ir::Function* func, std::queue<uint32_t>* todo);

  // 
  bool ProcessEntryPointCallTree(ProcessFunction& pfn, ir::Module* module);

  // Processes the given |module|. Returns Status::Failure if errors occur when
  // processing. Returns the corresponding Status::Success if processing is
  // succesful to indicate whether changes are made to the module.
  virtual Status Process(ir::Module* module) = 0;

 private:
  MessageConsumer consumer_;  // Message consumer.
};

}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_PASS_H_
