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

#ifndef UTILS_COMBORENDERBUNDLEENCODERDESCRIPTOR_H_
#define UTILS_COMBORENDERBUNDLEENCODERDESCRIPTOR_H_

#include <dawn/webgpu_cpp.h>

#include "common/Constants.h"

#include <array>

namespace utils {

    class ComboRenderBundleEncoderDescriptor : public wgpu::RenderBundleEncoderDescriptor {
      public:
        ComboRenderBundleEncoderDescriptor();

        std::array<wgpu::TextureFormat, kMaxColorAttachments> cColorFormats;
    };

}  // namespace utils

#endif  // UTILS_COMBORENDERBUNDLEENCODERDESCRIPTOR_H_
