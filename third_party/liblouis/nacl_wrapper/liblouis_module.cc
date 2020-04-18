// Copyright 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "liblouis_module.h"

#include <cstddef>

#include "liblouis_instance.h"

namespace liblouis_nacl {

LibLouisModule::LibLouisModule() : pp::Module() {}

LibLouisModule::~LibLouisModule() {}

pp::Instance* LibLouisModule::CreateInstance(PP_Instance instance) {
  static bool created = false;
  if (!created) {
    created = true;
    return new LibLouisInstance(instance);
  }
  return NULL;
}

}  // namespace liblouis_nacl

namespace pp {

Module* CreateModule() {
  static bool created = false;
  if (!created) {
    created = true;
    return new liblouis_nacl::LibLouisModule();
  }
  return NULL;
}

}  // namespace pp
