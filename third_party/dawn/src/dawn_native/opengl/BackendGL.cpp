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

#include "dawn_native/opengl/BackendGL.h"

#include "common/GPUInfo.h"
#include "common/Log.h"
#include "dawn_native/Instance.h"
#include "dawn_native/OpenGLBackend.h"
#include "dawn_native/opengl/DeviceGL.h"

#include <cstring>

namespace dawn_native { namespace opengl {

    namespace {

        struct Vendor {
            const char* vendorName;
            uint32_t vendorId;
        };

        const Vendor kVendors[] = {{"ATI", gpu_info::kVendorID_AMD},
                                   {"ARM", gpu_info::kVendorID_ARM},
                                   {"Imagination", gpu_info::kVendorID_ImgTec},
                                   {"Intel", gpu_info::kVendorID_Intel},
                                   {"NVIDIA", gpu_info::kVendorID_Nvidia},
                                   {"Qualcomm", gpu_info::kVendorID_Qualcomm}};

        uint32_t GetVendorIdFromVendors(const char* vendor) {
            uint32_t vendorId = 0;
            for (const auto& it : kVendors) {
                // Matching vendor name with vendor string
                if (strstr(vendor, it.vendorName) != nullptr) {
                    vendorId = it.vendorId;
                    break;
                }
            }
            return vendorId;
        }

        void KHRONOS_APIENTRY OnGLDebugMessage(GLenum source,
                                               GLenum type,
                                               GLuint id,
                                               GLenum severity,
                                               GLsizei length,
                                               const GLchar* message,
                                               const void* userParam) {
            const char* sourceText;
            switch (source) {
                case GL_DEBUG_SOURCE_API:
                    sourceText = "OpenGL";
                    break;
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                    sourceText = "Window System";
                    break;
                case GL_DEBUG_SOURCE_SHADER_COMPILER:
                    sourceText = "Shader Compiler";
                    break;
                case GL_DEBUG_SOURCE_THIRD_PARTY:
                    sourceText = "Third Party";
                    break;
                case GL_DEBUG_SOURCE_APPLICATION:
                    sourceText = "Application";
                    break;
                case GL_DEBUG_SOURCE_OTHER:
                    sourceText = "Other";
                    break;
                default:
                    sourceText = "UNKNOWN";
                    break;
            }

            const char* severityText;
            switch (severity) {
                case GL_DEBUG_SEVERITY_HIGH:
                    severityText = "High";
                    break;
                case GL_DEBUG_SEVERITY_MEDIUM:
                    severityText = "Medium";
                    break;
                case GL_DEBUG_SEVERITY_LOW:
                    severityText = "Low";
                    break;
                case GL_DEBUG_SEVERITY_NOTIFICATION:
                    severityText = "Notification";
                    break;
                default:
                    severityText = "UNKNOWN";
                    break;
            }

            if (type == GL_DEBUG_TYPE_ERROR) {
                dawn::WarningLog() << "OpenGL error:"
                                   << "\n    Source: " << sourceText      //
                                   << "\n    ID: " << id                  //
                                   << "\n    Severity: " << severityText  //
                                   << "\n    Message: " << message;

                // Abort on an error when in Debug mode.
                UNREACHABLE();
            }
        }

    }  // anonymous namespace

    // The OpenGL backend's Adapter.

    class Adapter : public AdapterBase {
      public:
        Adapter(InstanceBase* instance) : AdapterBase(instance, wgpu::BackendType::OpenGL) {
        }

        MaybeError Initialize(const AdapterDiscoveryOptions* options) {
            // Use getProc to populate the dispatch table
            DAWN_TRY(mFunctions.Initialize(options->getProc));

            // Use the debug output functionality to get notified about GL errors
            // TODO(cwallez@chromium.org): add support for the KHR_debug and ARB_debug_output
            // extensions
            bool hasDebugOutput = mFunctions.IsAtLeastGL(4, 3) || mFunctions.IsAtLeastGLES(3, 2);

            if (GetInstance()->IsBackendValidationEnabled() && hasDebugOutput) {
                mFunctions.Enable(GL_DEBUG_OUTPUT);
                mFunctions.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

                // Any GL error; dangerous undefined behavior; any shader compiler and linker errors
                mFunctions.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,
                                               0, nullptr, GL_TRUE);

                // Severe performance warnings; GLSL or other shader compiler and linker warnings;
                // use of currently deprecated behavior
                mFunctions.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,
                                               0, nullptr, GL_TRUE);

                // Performance warnings from redundant state changes; trivial undefined behavior
                // This is disabled because we do an incredible amount of redundant state changes.
                mFunctions.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0,
                                               nullptr, GL_FALSE);

                // Any message which is not an error or performance concern
                mFunctions.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                                               GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
                                               GL_FALSE);
                mFunctions.DebugMessageCallback(&OnGLDebugMessage, nullptr);
            }

            // Set state that never changes between devices.
            mFunctions.Enable(GL_DEPTH_TEST);
            mFunctions.Enable(GL_SCISSOR_TEST);
            mFunctions.Enable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
            mFunctions.Enable(GL_MULTISAMPLE);
            mFunctions.Enable(GL_FRAMEBUFFER_SRGB);
            mFunctions.Enable(GL_SAMPLE_MASK);

            mPCIInfo.name = reinterpret_cast<const char*>(mFunctions.GetString(GL_RENDERER));

            // Workaroud to find vendor id from vendor name
            const char* vendor = reinterpret_cast<const char*>(mFunctions.GetString(GL_VENDOR));
            mPCIInfo.vendorId = GetVendorIdFromVendors(vendor);

            InitializeSupportedExtensions();

            return {};
        }

        ~Adapter() override = default;

      private:
        OpenGLFunctions mFunctions;

        ResultOrError<DeviceBase*> CreateDeviceImpl(const DeviceDescriptor* descriptor) override {
            // There is no limit on the number of devices created from this adapter because they can
            // all share the same backing OpenGL context.
            return Device::Create(this, descriptor, mFunctions);
        }

        void InitializeSupportedExtensions() {
            // TextureCompressionBC
            {
                // BC1, BC2 and BC3 are not supported in OpenGL or OpenGL ES core features.
                bool supportsS3TC =
                    mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_s3tc");

                // COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT and
                // COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT requires both GL_EXT_texture_sRGB and
                // GL_EXT_texture_compression_s3tc on desktop OpenGL drivers.
                // (https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_sRGB.txt)
                bool supportsTextureSRGB = mFunctions.IsGLExtensionSupported("GL_EXT_texture_sRGB");

                // GL_EXT_texture_compression_s3tc_srgb is an extension in OpenGL ES.
                bool supportsS3TCSRGB =
                    mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_s3tc_srgb");

                // BC4 and BC5
                bool supportsRGTC =
                    mFunctions.IsAtLeastGL(3, 0) ||
                    mFunctions.IsGLExtensionSupported("GL_ARB_texture_compression_rgtc") ||
                    mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_rgtc");

                // BC6 and BC7
                bool supportsBPTC =
                    mFunctions.IsAtLeastGL(4, 2) ||
                    mFunctions.IsGLExtensionSupported("GL_ARB_texture_compression_bptc") ||
                    mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_bptc");

                if (supportsS3TC && (supportsTextureSRGB || supportsS3TCSRGB) && supportsRGTC &&
                    supportsBPTC) {
                    mSupportedExtensions.EnableExtension(
                        dawn_native::Extension::TextureCompressionBC);
                }
            }
        }
    };

    // Implementation of the OpenGL backend's BackendConnection

    Backend::Backend(InstanceBase* instance)
        : BackendConnection(instance, wgpu::BackendType::OpenGL) {
    }

    std::vector<std::unique_ptr<AdapterBase>> Backend::DiscoverDefaultAdapters() {
        // The OpenGL backend needs at least "getProcAddress" to discover an adapter.
        return {};
    }

    ResultOrError<std::vector<std::unique_ptr<AdapterBase>>> Backend::DiscoverAdapters(
        const AdapterDiscoveryOptionsBase* optionsBase) {
        // TODO(cwallez@chromium.org): For now only create a single OpenGL adapter because don't
        // know how to handle MakeCurrent.
        if (mCreatedAdapter) {
            return DAWN_VALIDATION_ERROR("The OpenGL backend can only create a single adapter");
        }

        ASSERT(optionsBase->backendType == WGPUBackendType_OpenGL);
        const AdapterDiscoveryOptions* options =
            static_cast<const AdapterDiscoveryOptions*>(optionsBase);

        if (options->getProc == nullptr) {
            return DAWN_VALIDATION_ERROR("AdapterDiscoveryOptions::getProc must be set");
        }

        std::unique_ptr<Adapter> adapter = std::make_unique<Adapter>(GetInstance());
        DAWN_TRY(adapter->Initialize(options));

        mCreatedAdapter = true;
        std::vector<std::unique_ptr<AdapterBase>> adapters;
        adapters.push_back(std::unique_ptr<AdapterBase>(adapter.release()));
        return std::move(adapters);
    }

    BackendConnection* Connect(InstanceBase* instance) {
        return new Backend(instance);
    }

}}  // namespace dawn_native::opengl
