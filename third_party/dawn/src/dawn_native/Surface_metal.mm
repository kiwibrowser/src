// Copyright 2020 the Dawn Authors
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

// Contains a helper function for Surface.cpp that needs to be written in ObjectiveC.

#if !defined(DAWN_ENABLE_BACKEND_METAL)
#    error "Surface_metal.mm requires the Metal backend to be enabled."
#endif  // !defined(DAWN_ENABLE_BACKEND_METAL)

#import <QuartzCore/CAMetalLayer.h>

namespace dawn_native {

    bool InheritsFromCAMetalLayer(void* obj) {
        id<NSObject> object = static_cast<id>(obj);
        return [object isKindOfClass:[CAMetalLayer class]];
    }

}  // namespace dawn_native
