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

#ifndef DAWNNATIVE_NULLBACKEND_H_
#define DAWNNATIVE_NULLBACKEND_H_

#include <dawn/dawn_wsi.h>
#include <dawn_native/DawnNative.h>

namespace dawn_native { namespace null {
    DAWN_NATIVE_EXPORT DawnSwapChainImplementation CreateNativeSwapChainImpl();
}}  // namespace dawn_native::null

#endif  // DAWNNATIVE_NULLBACKEND_H_
