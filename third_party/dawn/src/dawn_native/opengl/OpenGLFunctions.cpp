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

#include "dawn_native/opengl/OpenGLFunctions.h"

#include <cctype>
#include <tuple>

namespace dawn_native { namespace opengl {

    MaybeError OpenGLFunctions::Initialize(GetProcAddress getProc) {
        PFNGLGETSTRINGPROC getString = reinterpret_cast<PFNGLGETSTRINGPROC>(getProc("glGetString"));
        if (getString == nullptr) {
            return DAWN_INTERNAL_ERROR("Couldn't load glGetString");
        }

        std::string version = reinterpret_cast<const char*>(getString(GL_VERSION));

        if (version.find("OpenGL ES") != std::string::npos) {
            // ES spec states that the GL_VERSION string will be in the following format:
            // "OpenGL ES N.M vendor-specific information"
            mStandard = Standard::ES;
            mMajorVersion = version[10] - '0';
            mMinorVersion = version[12] - '0';

            // The minor version shouldn't get to two digits.
            ASSERT(version.size() <= 13 || !isdigit(version[13]));

            DAWN_TRY(LoadOpenGLESProcs(getProc, mMajorVersion, mMinorVersion));
        } else {
            // OpenGL spec states the GL_VERSION string will be in the following format:
            // <version number><space><vendor-specific information>
            // The version number is either of the form major number.minor number or major
            // number.minor number.release number, where the numbers all have one or more
            // digits
            mStandard = Standard::Desktop;
            mMajorVersion = version[0] - '0';
            mMinorVersion = version[2] - '0';

            // The minor version shouldn't get to two digits.
            ASSERT(version.size() <= 3 || !isdigit(version[3]));

            DAWN_TRY(LoadDesktopGLProcs(getProc, mMajorVersion, mMinorVersion));
        }

        InitializeSupportedGLExtensions();

        return {};
    }

    void OpenGLFunctions::InitializeSupportedGLExtensions() {
        int32_t numExtensions;
        GetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

        for (int32_t i = 0; i < numExtensions; ++i) {
            const char* extensionName = reinterpret_cast<const char*>(GetStringi(GL_EXTENSIONS, i));
            mSupportedGLExtensionsSet.insert(extensionName);
        }
    }

    bool OpenGLFunctions::IsGLExtensionSupported(const char* extension) const {
        ASSERT(extension != nullptr);
        return mSupportedGLExtensionsSet.count(extension) != 0;
    }

    bool OpenGLFunctions::IsAtLeastGL(uint32_t majorVersion, uint32_t minorVersion) const {
        return mStandard == Standard::Desktop &&
               std::tie(mMajorVersion, mMinorVersion) >= std::tie(majorVersion, minorVersion);
    }

    bool OpenGLFunctions::IsAtLeastGLES(uint32_t majorVersion, uint32_t minorVersion) const {
        return mStandard == Standard::ES &&
               std::tie(mMajorVersion, mMinorVersion) >= std::tie(majorVersion, minorVersion);
    }

}}  // namespace dawn_native::opengl
