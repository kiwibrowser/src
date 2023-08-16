// Copyright 2017 The Dawn Authors
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

#include "dawn_native/ShaderModule.h"

#include "common/HashUtils.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/Device.h"
#include "dawn_native/Pipeline.h"
#include "dawn_native/PipelineLayout.h"

#include <spirv-tools/libspirv.hpp>
#include <spirv_cross.hpp>

#ifdef DAWN_ENABLE_WGSL
// Tint include must be after spirv_cross.hpp, because spirv-cross has its own
// version of spirv_headers.
// clang-format off
#include <tint/tint.h>
// clang-format on
#endif  // DAWN_ENABLE_WGSL

#include <sstream>

namespace dawn_native {

    namespace {
        Format::Type SpirvCrossBaseTypeToFormatType(spirv_cross::SPIRType::BaseType spirvBaseType) {
            switch (spirvBaseType) {
                case spirv_cross::SPIRType::Float:
                    return Format::Float;
                case spirv_cross::SPIRType::Int:
                    return Format::Sint;
                case spirv_cross::SPIRType::UInt:
                    return Format::Uint;
                default:
                    UNREACHABLE();
                    return Format::Other;
            }
        }

        wgpu::TextureViewDimension SpirvDimToTextureViewDimension(spv::Dim dim, bool arrayed) {
            switch (dim) {
                case spv::Dim::Dim1D:
                    return wgpu::TextureViewDimension::e1D;
                case spv::Dim::Dim2D:
                    if (arrayed) {
                        return wgpu::TextureViewDimension::e2DArray;
                    } else {
                        return wgpu::TextureViewDimension::e2D;
                    }
                case spv::Dim::Dim3D:
                    return wgpu::TextureViewDimension::e3D;
                case spv::Dim::DimCube:
                    if (arrayed) {
                        return wgpu::TextureViewDimension::CubeArray;
                    } else {
                        return wgpu::TextureViewDimension::Cube;
                    }
                default:
                    UNREACHABLE();
                    return wgpu::TextureViewDimension::Undefined;
            }
        }

        wgpu::TextureViewDimension ToWGPUTextureViewDimension(
            shaderc_spvc_texture_view_dimension dim) {
            switch (dim) {
                case shaderc_spvc_texture_view_dimension_undefined:
                    return wgpu::TextureViewDimension::Undefined;
                case shaderc_spvc_texture_view_dimension_e1D:
                    return wgpu::TextureViewDimension::e1D;
                case shaderc_spvc_texture_view_dimension_e2D:
                    return wgpu::TextureViewDimension::e2D;
                case shaderc_spvc_texture_view_dimension_e2D_array:
                    return wgpu::TextureViewDimension::e2DArray;
                case shaderc_spvc_texture_view_dimension_cube:
                    return wgpu::TextureViewDimension::Cube;
                case shaderc_spvc_texture_view_dimension_cube_array:
                    return wgpu::TextureViewDimension::CubeArray;
                case shaderc_spvc_texture_view_dimension_e3D:
                    return wgpu::TextureViewDimension::e3D;
            }
            UNREACHABLE();
        }

        Format::Type ToDawnFormatType(shaderc_spvc_texture_format_type type) {
            switch (type) {
                case shaderc_spvc_texture_format_type_float:
                    return Format::Type::Float;
                case shaderc_spvc_texture_format_type_sint:
                    return Format::Type::Sint;
                case shaderc_spvc_texture_format_type_uint:
                    return Format::Type::Uint;
                case shaderc_spvc_texture_format_type_other:
                    return Format::Type::Other;
            }
            UNREACHABLE();
        }

        wgpu::BindingType ToWGPUBindingType(shaderc_spvc_binding_type type) {
            switch (type) {
                case shaderc_spvc_binding_type_uniform_buffer:
                    return wgpu::BindingType::UniformBuffer;
                case shaderc_spvc_binding_type_storage_buffer:
                    return wgpu::BindingType::StorageBuffer;
                case shaderc_spvc_binding_type_readonly_storage_buffer:
                    return wgpu::BindingType::ReadonlyStorageBuffer;
                case shaderc_spvc_binding_type_sampler:
                    return wgpu::BindingType::Sampler;
                case shaderc_spvc_binding_type_comparison_sampler:
                    return wgpu::BindingType::ComparisonSampler;
                case shaderc_spvc_binding_type_sampled_texture:
                    return wgpu::BindingType::SampledTexture;
                case shaderc_spvc_binding_type_readonly_storage_texture:
                    return wgpu::BindingType::ReadonlyStorageTexture;
                case shaderc_spvc_binding_type_writeonly_storage_texture:
                    return wgpu::BindingType::WriteonlyStorageTexture;
                case shaderc_spvc_binding_type_storage_texture:
                    return wgpu::BindingType::StorageTexture;
                default:
                    UNREACHABLE();
            }
        }

        SingleShaderStage ToSingleShaderStage(shaderc_spvc_execution_model execution_model) {
            switch (execution_model) {
                case shaderc_spvc_execution_model_vertex:
                    return SingleShaderStage::Vertex;
                case shaderc_spvc_execution_model_fragment:
                    return SingleShaderStage::Fragment;
                case shaderc_spvc_execution_model_glcompute:
                    return SingleShaderStage::Compute;
                default:
                    UNREACHABLE();
            }
        }

        wgpu::TextureFormat ToWGPUTextureFormat(spv::ImageFormat format) {
            switch (format) {
                case spv::ImageFormatR8:
                    return wgpu::TextureFormat::R8Unorm;
                case spv::ImageFormatR8Snorm:
                    return wgpu::TextureFormat::R8Snorm;
                case spv::ImageFormatR8ui:
                    return wgpu::TextureFormat::R8Uint;
                case spv::ImageFormatR8i:
                    return wgpu::TextureFormat::R8Sint;
                case spv::ImageFormatR16ui:
                    return wgpu::TextureFormat::R16Uint;
                case spv::ImageFormatR16i:
                    return wgpu::TextureFormat::R16Sint;
                case spv::ImageFormatR16f:
                    return wgpu::TextureFormat::R16Float;
                case spv::ImageFormatRg8:
                    return wgpu::TextureFormat::RG8Unorm;
                case spv::ImageFormatRg8Snorm:
                    return wgpu::TextureFormat::RG8Snorm;
                case spv::ImageFormatRg8ui:
                    return wgpu::TextureFormat::RG8Uint;
                case spv::ImageFormatRg8i:
                    return wgpu::TextureFormat::RG8Sint;
                case spv::ImageFormatR32f:
                    return wgpu::TextureFormat::R32Float;
                case spv::ImageFormatR32ui:
                    return wgpu::TextureFormat::R32Uint;
                case spv::ImageFormatR32i:
                    return wgpu::TextureFormat::R32Sint;
                case spv::ImageFormatRg16ui:
                    return wgpu::TextureFormat::RG16Uint;
                case spv::ImageFormatRg16i:
                    return wgpu::TextureFormat::RG16Sint;
                case spv::ImageFormatRg16f:
                    return wgpu::TextureFormat::RG16Float;
                case spv::ImageFormatRgba8:
                    return wgpu::TextureFormat::RGBA8Unorm;
                case spv::ImageFormatRgba8Snorm:
                    return wgpu::TextureFormat::RGBA8Snorm;
                case spv::ImageFormatRgba8ui:
                    return wgpu::TextureFormat::RGBA8Uint;
                case spv::ImageFormatRgba8i:
                    return wgpu::TextureFormat::RGBA8Sint;
                case spv::ImageFormatRgb10A2:
                    return wgpu::TextureFormat::RGB10A2Unorm;
                case spv::ImageFormatR11fG11fB10f:
                    return wgpu::TextureFormat::RG11B10Float;
                case spv::ImageFormatRg32f:
                    return wgpu::TextureFormat::RG32Float;
                case spv::ImageFormatRg32ui:
                    return wgpu::TextureFormat::RG32Uint;
                case spv::ImageFormatRg32i:
                    return wgpu::TextureFormat::RG32Sint;
                case spv::ImageFormatRgba16ui:
                    return wgpu::TextureFormat::RGBA16Uint;
                case spv::ImageFormatRgba16i:
                    return wgpu::TextureFormat::RGBA16Sint;
                case spv::ImageFormatRgba16f:
                    return wgpu::TextureFormat::RGBA16Float;
                case spv::ImageFormatRgba32f:
                    return wgpu::TextureFormat::RGBA32Float;
                case spv::ImageFormatRgba32ui:
                    return wgpu::TextureFormat::RGBA32Uint;
                case spv::ImageFormatRgba32i:
                    return wgpu::TextureFormat::RGBA32Sint;
                default:
                    return wgpu::TextureFormat::Undefined;
            }
        }

        wgpu::TextureFormat ToWGPUTextureFormat(shaderc_spvc_storage_texture_format format) {
            switch (format) {
                case shaderc_spvc_storage_texture_format_r8unorm:
                    return wgpu::TextureFormat::R8Unorm;
                case shaderc_spvc_storage_texture_format_r8snorm:
                    return wgpu::TextureFormat::R8Snorm;
                case shaderc_spvc_storage_texture_format_r8uint:
                    return wgpu::TextureFormat::R8Uint;
                case shaderc_spvc_storage_texture_format_r8sint:
                    return wgpu::TextureFormat::R8Sint;
                case shaderc_spvc_storage_texture_format_r16uint:
                    return wgpu::TextureFormat::R16Uint;
                case shaderc_spvc_storage_texture_format_r16sint:
                    return wgpu::TextureFormat::R16Sint;
                case shaderc_spvc_storage_texture_format_r16float:
                    return wgpu::TextureFormat::R16Float;
                case shaderc_spvc_storage_texture_format_rg8unorm:
                    return wgpu::TextureFormat::RG8Unorm;
                case shaderc_spvc_storage_texture_format_rg8snorm:
                    return wgpu::TextureFormat::RG8Snorm;
                case shaderc_spvc_storage_texture_format_rg8uint:
                    return wgpu::TextureFormat::RG8Uint;
                case shaderc_spvc_storage_texture_format_rg8sint:
                    return wgpu::TextureFormat::RG8Sint;
                case shaderc_spvc_storage_texture_format_r32float:
                    return wgpu::TextureFormat::R32Float;
                case shaderc_spvc_storage_texture_format_r32uint:
                    return wgpu::TextureFormat::R32Uint;
                case shaderc_spvc_storage_texture_format_r32sint:
                    return wgpu::TextureFormat::R32Sint;
                case shaderc_spvc_storage_texture_format_rg16uint:
                    return wgpu::TextureFormat::RG16Uint;
                case shaderc_spvc_storage_texture_format_rg16sint:
                    return wgpu::TextureFormat::RG16Sint;
                case shaderc_spvc_storage_texture_format_rg16float:
                    return wgpu::TextureFormat::RG16Float;
                case shaderc_spvc_storage_texture_format_rgba8unorm:
                    return wgpu::TextureFormat::RGBA8Unorm;
                case shaderc_spvc_storage_texture_format_rgba8snorm:
                    return wgpu::TextureFormat::RGBA8Snorm;
                case shaderc_spvc_storage_texture_format_rgba8uint:
                    return wgpu::TextureFormat::RGBA8Uint;
                case shaderc_spvc_storage_texture_format_rgba8sint:
                    return wgpu::TextureFormat::RGBA8Sint;
                case shaderc_spvc_storage_texture_format_rgb10a2unorm:
                    return wgpu::TextureFormat::RGB10A2Unorm;
                case shaderc_spvc_storage_texture_format_rg11b10float:
                    return wgpu::TextureFormat::RG11B10Float;
                case shaderc_spvc_storage_texture_format_rg32float:
                    return wgpu::TextureFormat::RG32Float;
                case shaderc_spvc_storage_texture_format_rg32uint:
                    return wgpu::TextureFormat::RG32Uint;
                case shaderc_spvc_storage_texture_format_rg32sint:
                    return wgpu::TextureFormat::RG32Sint;
                case shaderc_spvc_storage_texture_format_rgba16uint:
                    return wgpu::TextureFormat::RGBA16Uint;
                case shaderc_spvc_storage_texture_format_rgba16sint:
                    return wgpu::TextureFormat::RGBA16Sint;
                case shaderc_spvc_storage_texture_format_rgba16float:
                    return wgpu::TextureFormat::RGBA16Float;
                case shaderc_spvc_storage_texture_format_rgba32float:
                    return wgpu::TextureFormat::RGBA32Float;
                case shaderc_spvc_storage_texture_format_rgba32uint:
                    return wgpu::TextureFormat::RGBA32Uint;
                case shaderc_spvc_storage_texture_format_rgba32sint:
                    return wgpu::TextureFormat::RGBA32Sint;
                default:
                    return wgpu::TextureFormat::Undefined;
            }
        }

        std::string GetShaderDeclarationString(BindGroupIndex group, BindingNumber binding) {
            std::ostringstream ostream;
            ostream << "the shader module declaration at set " << static_cast<uint32_t>(group)
                    << " binding " << static_cast<uint32_t>(binding);
            return ostream.str();
        }
    }  // anonymous namespace

    MaybeError ValidateSpirv(DeviceBase*, const uint32_t* code, uint32_t codeSize) {
        spvtools::SpirvTools spirvTools(SPV_ENV_VULKAN_1_1);

        std::ostringstream errorStream;
        errorStream << "SPIRV Validation failure:" << std::endl;

        spirvTools.SetMessageConsumer([&errorStream](spv_message_level_t level, const char*,
                                                     const spv_position_t& position,
                                                     const char* message) {
            switch (level) {
                case SPV_MSG_FATAL:
                case SPV_MSG_INTERNAL_ERROR:
                case SPV_MSG_ERROR:
                    errorStream << "error: line " << position.index << ": " << message << std::endl;
                    break;
                case SPV_MSG_WARNING:
                    errorStream << "warning: line " << position.index << ": " << message
                                << std::endl;
                    break;
                case SPV_MSG_INFO:
                    errorStream << "info: line " << position.index << ": " << message << std::endl;
                    break;
                default:
                    break;
            }
        });

        if (!spirvTools.Validate(code, codeSize)) {
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        return {};
    }

#ifdef DAWN_ENABLE_WGSL
    MaybeError ValidateWGSL(const char* source) {
        std::ostringstream errorStream;
        errorStream << "Tint WGSL failure:" << std::endl;

        tint::Context context;
        tint::reader::wgsl::Parser parser(&context, source);

        if (!parser.Parse()) {
            errorStream << "Parser: " << parser.error() << std::endl;
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        tint::ast::Module module = parser.module();
        if (!module.IsValid()) {
            errorStream << "Invalid module generated..." << std::endl;
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        tint::TypeDeterminer type_determiner(&context, &module);
        if (!type_determiner.Determine()) {
            errorStream << "Type Determination: " << type_determiner.error();
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        tint::Validator validator;
        if (!validator.Validate(module)) {
            errorStream << "Validation: " << validator.error() << std::endl;
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        return {};
    }

    ResultOrError<std::vector<uint32_t>> ConvertWGSLToSPIRV(const char* source) {
        std::ostringstream errorStream;
        errorStream << "Tint WGSL->SPIR-V failure:" << std::endl;

        tint::Context context;
        tint::reader::wgsl::Parser parser(&context, source);

        // TODO: This is a duplicate parse with ValidateWGSL, need to store
        // state between calls to avoid this.
        if (!parser.Parse()) {
            errorStream << "Parser: " << parser.error() << std::endl;
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        tint::ast::Module module = parser.module();
        if (!module.IsValid()) {
            errorStream << "Invalid module generated..." << std::endl;
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        tint::TypeDeterminer type_determiner(&context, &module);
        if (!type_determiner.Determine()) {
            errorStream << "Type Determination: " << type_determiner.error();
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        tint::writer::spirv::Generator generator(std::move(module));
        if (!generator.Generate()) {
            errorStream << "Generator: " << generator.error() << std::endl;
            return DAWN_VALIDATION_ERROR(errorStream.str().c_str());
        }

        std::vector<uint32_t> spirv = generator.result();
        return std::move(spirv);
    }
#endif  // DAWN_ENABLE_WGSL

    MaybeError ValidateShaderModuleDescriptor(DeviceBase* device,
                                              const ShaderModuleDescriptor* descriptor) {
        const ChainedStruct* chainedDescriptor = descriptor->nextInChain;
        if (chainedDescriptor == nullptr) {
            return DAWN_VALIDATION_ERROR("Shader module descriptor missing chained descriptor");
        }
        // For now only a single SPIRV or WGSL subdescriptor is allowed.
        if (chainedDescriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR(
                "Shader module descriptor chained nextInChain must be nullptr");
        }

        switch (chainedDescriptor->sType) {
            case wgpu::SType::ShaderModuleSPIRVDescriptor: {
                const auto* spirvDesc =
                    static_cast<const ShaderModuleSPIRVDescriptor*>(chainedDescriptor);
                DAWN_TRY(ValidateSpirv(device, spirvDesc->code, spirvDesc->codeSize));
                break;
            }

            case wgpu::SType::ShaderModuleWGSLDescriptor: {
#ifdef DAWN_ENABLE_WGSL
                const auto* wgslDesc =
                    static_cast<const ShaderModuleWGSLDescriptor*>(chainedDescriptor);
                DAWN_TRY(ValidateWGSL(wgslDesc->source));
                break;
#else
                return DAWN_VALIDATION_ERROR("WGSL not supported (yet)");
#endif  // DAWN_ENABLE_WGSL
            }
            default:
                return DAWN_VALIDATION_ERROR("Unsupported sType");
        }

        return {};
    }  // namespace

    // ShaderModuleBase

    ShaderModuleBase::ShaderModuleBase(DeviceBase* device, const ShaderModuleDescriptor* descriptor)
        : CachedObject(device), mType(Type::Undefined) {
        ASSERT(descriptor->nextInChain != nullptr);
        switch (descriptor->nextInChain->sType) {
            case wgpu::SType::ShaderModuleSPIRVDescriptor: {
                mType = Type::Spirv;
                const auto* spirvDesc =
                    static_cast<const ShaderModuleSPIRVDescriptor*>(descriptor->nextInChain);
                mSpirv.assign(spirvDesc->code, spirvDesc->code + spirvDesc->codeSize);
                break;
            }
            case wgpu::SType::ShaderModuleWGSLDescriptor: {
                mType = Type::Wgsl;
                const auto* wgslDesc =
                    static_cast<const ShaderModuleWGSLDescriptor*>(descriptor->nextInChain);
                mWgsl = std::string(wgslDesc->source);
                break;
            }
            default:
                UNREACHABLE();
        }

        mFragmentOutputFormatBaseTypes.fill(Format::Other);
        if (GetDevice()->IsToggleEnabled(Toggle::UseSpvcParser)) {
            mSpvcContext.SetUseSpvcParser(true);
        }
    }

    ShaderModuleBase::ShaderModuleBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : CachedObject(device, tag), mType(Type::Undefined) {
    }

    ShaderModuleBase::~ShaderModuleBase() {
        if (IsCachedReference()) {
            GetDevice()->UncacheShaderModule(this);
        }
    }

    // static
    ShaderModuleBase* ShaderModuleBase::MakeError(DeviceBase* device) {
        return new ShaderModuleBase(device, ObjectBase::kError);
    }

    MaybeError ShaderModuleBase::ExtractSpirvInfo(const spirv_cross::Compiler& compiler) {
        ASSERT(!IsError());
        if (GetDevice()->IsToggleEnabled(Toggle::UseSpvc)) {
            DAWN_TRY(ExtractSpirvInfoWithSpvc());
        } else {
            DAWN_TRY(ExtractSpirvInfoWithSpirvCross(compiler));
        }
        return {};
    }

    MaybeError ShaderModuleBase::ExtractSpirvInfoWithSpvc() {
        shaderc_spvc_execution_model execution_model;
        DAWN_TRY(CheckSpvcSuccess(mSpvcContext.GetExecutionModel(&execution_model),
                                  "Unable to get execution model for shader."));
        mExecutionModel = ToSingleShaderStage(execution_model);

        size_t push_constant_buffers_count;
        DAWN_TRY(
            CheckSpvcSuccess(mSpvcContext.GetPushConstantBufferCount(&push_constant_buffers_count),
                             "Unable to get push constant buffer count for shader."));

        // TODO(rharrison): This should be handled by spirv-val pass in spvc,
        // but need to confirm.
        if (push_constant_buffers_count > 0) {
            return DAWN_VALIDATION_ERROR("Push constants aren't supported.");
        }

        // Fill in bindingInfo with the SPIRV bindings
        auto ExtractResourcesBinding =
            [this](std::vector<shaderc_spvc_binding_info> bindings) -> MaybeError {
            for (const auto& binding : bindings) {
                BindGroupIndex bindGroupIndex(binding.set);

                if (bindGroupIndex >= kMaxBindGroupsTyped) {
                    return DAWN_VALIDATION_ERROR("Bind group index over limits in the SPIRV");
                }

                const auto& it = mBindingInfo[bindGroupIndex].emplace(
                    BindingNumber(binding.binding), ShaderBindingInfo{});
                if (!it.second) {
                    return DAWN_VALIDATION_ERROR("Shader has duplicate bindings");
                }

                ShaderBindingInfo* info = &it.first->second;
                info->id = binding.id;
                info->base_type_id = binding.base_type_id;
                info->type = ToWGPUBindingType(binding.binding_type);

                switch (info->type) {
                    case wgpu::BindingType::SampledTexture: {
                        info->multisampled = binding.multisampled;
                        info->viewDimension = ToWGPUTextureViewDimension(binding.texture_dimension);
                        info->textureComponentType =
                            ToDawnFormatType(binding.texture_component_type);
                        break;
                    }
                    case wgpu::BindingType::StorageTexture:
                    case wgpu::BindingType::ReadonlyStorageTexture:
                    case wgpu::BindingType::WriteonlyStorageTexture: {
                        wgpu::TextureFormat storageTextureFormat =
                            ToWGPUTextureFormat(binding.storage_texture_format);
                        if (storageTextureFormat == wgpu::TextureFormat::Undefined) {
                            return DAWN_VALIDATION_ERROR(
                                "Invalid image format declaration on storage image");
                        }
                        const Format& format =
                            GetDevice()->GetValidInternalFormat(storageTextureFormat);
                        if (!format.supportsStorageUsage) {
                            return DAWN_VALIDATION_ERROR(
                                "The storage texture format is not supported");
                        }
                        info->multisampled = binding.multisampled;
                        info->storageTextureFormat = storageTextureFormat;
                        info->viewDimension = ToWGPUTextureViewDimension(binding.texture_dimension);
                        break;
                    }
                    case wgpu::BindingType::UniformBuffer:
                    case wgpu::BindingType::StorageBuffer:
                    case wgpu::BindingType::ReadonlyStorageBuffer:
                        info->minBufferBindingSize = binding.minimum_buffer_size;
                        break;
                    default:
                        break;
                }
            }
            return {};
        };

        std::vector<shaderc_spvc_binding_info> resource_bindings;
        DAWN_TRY(CheckSpvcSuccess(mSpvcContext.GetBindingInfo(
                                      shaderc_spvc_shader_resource_uniform_buffers,
                                      shaderc_spvc_binding_type_uniform_buffer, &resource_bindings),
                                  "Unable to get binding info for uniform buffers from shader"));
        DAWN_TRY(ExtractResourcesBinding(resource_bindings));

        DAWN_TRY(CheckSpvcSuccess(
            mSpvcContext.GetBindingInfo(shaderc_spvc_shader_resource_separate_images,
                                        shaderc_spvc_binding_type_sampled_texture,
                                        &resource_bindings),
            "Unable to get binding info for sampled textures from shader"));
        DAWN_TRY(ExtractResourcesBinding(resource_bindings));

        DAWN_TRY(CheckSpvcSuccess(
            mSpvcContext.GetBindingInfo(shaderc_spvc_shader_resource_separate_samplers,
                                        shaderc_spvc_binding_type_sampler, &resource_bindings),
            "Unable to get binding info for samples from shader"));
        DAWN_TRY(ExtractResourcesBinding(resource_bindings));

        DAWN_TRY(CheckSpvcSuccess(mSpvcContext.GetBindingInfo(
                                      shaderc_spvc_shader_resource_storage_buffers,
                                      shaderc_spvc_binding_type_storage_buffer, &resource_bindings),
                                  "Unable to get binding info for storage buffers from shader"));
        DAWN_TRY(ExtractResourcesBinding(resource_bindings));

        DAWN_TRY(CheckSpvcSuccess(
            mSpvcContext.GetBindingInfo(shaderc_spvc_shader_resource_storage_images,
                                        shaderc_spvc_binding_type_storage_texture,
                                        &resource_bindings),
            "Unable to get binding info for storage textures from shader"));
        DAWN_TRY(ExtractResourcesBinding(resource_bindings));

        std::vector<shaderc_spvc_resource_location_info> input_stage_locations;
        DAWN_TRY(CheckSpvcSuccess(mSpvcContext.GetInputStageLocationInfo(&input_stage_locations),
                                  "Unable to get input stage location information from shader"));

        for (const auto& input : input_stage_locations) {
            if (mExecutionModel == SingleShaderStage::Vertex) {
                if (input.location >= kMaxVertexAttributes) {
                    return DAWN_VALIDATION_ERROR("Attribute location over limits in the SPIRV");
                }
                mUsedVertexAttributes.set(input.location);
            } else if (mExecutionModel == SingleShaderStage::Fragment) {
                // Without a location qualifier on vertex inputs, spirv_cross::CompilerMSL gives
                // them all the location 0, causing a compile error.
                if (!input.has_location) {
                    return DAWN_VALIDATION_ERROR("Need location qualifier on fragment input");
                }
            }
        }

        std::vector<shaderc_spvc_resource_location_info> output_stage_locations;
        DAWN_TRY(CheckSpvcSuccess(mSpvcContext.GetOutputStageLocationInfo(&output_stage_locations),
                                  "Unable to get output stage location information from shader"));

        for (const auto& output : output_stage_locations) {
            if (mExecutionModel == SingleShaderStage::Vertex) {
                // Without a location qualifier on vertex outputs, spirv_cross::CompilerMSL
                // gives them all the location 0, causing a compile error.
                if (!output.has_location) {
                    return DAWN_VALIDATION_ERROR("Need location qualifier on vertex output");
                }
            } else if (mExecutionModel == SingleShaderStage::Fragment) {
                if (output.location >= kMaxColorAttachments) {
                    return DAWN_VALIDATION_ERROR(
                        "Fragment output location over limits in the SPIRV");
                }
            }
        }

        if (mExecutionModel == SingleShaderStage::Fragment) {
            std::vector<shaderc_spvc_resource_type_info> output_types;
            DAWN_TRY(CheckSpvcSuccess(mSpvcContext.GetOutputStageTypeInfo(&output_types),
                                      "Unable to get output stage type information from shader"));

            for (const auto& output : output_types) {
                if (output.type == shaderc_spvc_texture_format_type_other) {
                    return DAWN_VALIDATION_ERROR("Unexpected Fragment output type");
                }
                mFragmentOutputFormatBaseTypes[output.location] = ToDawnFormatType(output.type);
            }
        }
        return {};
    }

    MaybeError ShaderModuleBase::ExtractSpirvInfoWithSpirvCross(
        const spirv_cross::Compiler& compiler) {
        // TODO(cwallez@chromium.org): make errors here creation errors
        // currently errors here do not prevent the shadermodule from being used
        const auto& resources = compiler.get_shader_resources();

        switch (compiler.get_execution_model()) {
            case spv::ExecutionModelVertex:
                mExecutionModel = SingleShaderStage::Vertex;
                break;
            case spv::ExecutionModelFragment:
                mExecutionModel = SingleShaderStage::Fragment;
                break;
            case spv::ExecutionModelGLCompute:
                mExecutionModel = SingleShaderStage::Compute;
                break;
            default:
                UNREACHABLE();
                return DAWN_VALIDATION_ERROR("Unexpected shader execution model");
        }

        if (resources.push_constant_buffers.size() > 0) {
            return DAWN_VALIDATION_ERROR("Push constants aren't supported.");
        }

        if (resources.sampled_images.size() > 0) {
            return DAWN_VALIDATION_ERROR("Combined images and samplers aren't supported.");
        }

        // Fill in bindingInfo with the SPIRV bindings
        auto ExtractResourcesBinding =
            [this](const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
                   const spirv_cross::Compiler& compiler,
                   wgpu::BindingType bindingType) -> MaybeError {
            for (const auto& resource : resources) {
                if (!compiler.get_decoration_bitset(resource.id).get(spv::DecorationBinding)) {
                    return DAWN_VALIDATION_ERROR("No Binding decoration set for resource");
                }

                if (!compiler.get_decoration_bitset(resource.id)
                         .get(spv::DecorationDescriptorSet)) {
                    return DAWN_VALIDATION_ERROR("No Descriptor Decoration set for resource");
                }

                BindingNumber bindingNumber(
                    compiler.get_decoration(resource.id, spv::DecorationBinding));
                BindGroupIndex bindGroupIndex(
                    compiler.get_decoration(resource.id, spv::DecorationDescriptorSet));

                if (bindGroupIndex >= kMaxBindGroupsTyped) {
                    return DAWN_VALIDATION_ERROR("Bind group index over limits in the SPIRV");
                }

                const auto& it =
                    mBindingInfo[bindGroupIndex].emplace(bindingNumber, ShaderBindingInfo{});
                if (!it.second) {
                    return DAWN_VALIDATION_ERROR("Shader has duplicate bindings");
                }

                ShaderBindingInfo* info = &it.first->second;
                info->id = resource.id;
                info->base_type_id = resource.base_type_id;

                if (bindingType == wgpu::BindingType::UniformBuffer ||
                    bindingType == wgpu::BindingType::StorageBuffer ||
                    bindingType == wgpu::BindingType::ReadonlyStorageBuffer) {
                    // Determine buffer size, with a minimum of 1 element in the runtime array
                    spirv_cross::SPIRType type = compiler.get_type(info->base_type_id);
                    info->minBufferBindingSize =
                        compiler.get_declared_struct_size_runtime_array(type, 1);
                }

                switch (bindingType) {
                    case wgpu::BindingType::SampledTexture: {
                        spirv_cross::SPIRType::ImageType imageType =
                            compiler.get_type(info->base_type_id).image;
                        spirv_cross::SPIRType::BaseType textureComponentType =
                            compiler.get_type(imageType.type).basetype;

                        info->multisampled = imageType.ms;
                        info->viewDimension =
                            SpirvDimToTextureViewDimension(imageType.dim, imageType.arrayed);
                        info->textureComponentType =
                            SpirvCrossBaseTypeToFormatType(textureComponentType);
                        info->type = bindingType;
                        break;
                    }
                    case wgpu::BindingType::StorageBuffer: {
                        // Differentiate between readonly storage bindings and writable ones
                        // based on the NonWritable decoration
                        spirv_cross::Bitset flags = compiler.get_buffer_block_flags(resource.id);
                        if (flags.get(spv::DecorationNonWritable)) {
                            info->type = wgpu::BindingType::ReadonlyStorageBuffer;
                        } else {
                            info->type = wgpu::BindingType::StorageBuffer;
                        }
                        break;
                    }
                    case wgpu::BindingType::StorageTexture: {
                        spirv_cross::Bitset flags = compiler.get_decoration_bitset(resource.id);
                        if (flags.get(spv::DecorationNonReadable)) {
                            info->type = wgpu::BindingType::WriteonlyStorageTexture;
                        } else if (flags.get(spv::DecorationNonWritable)) {
                            info->type = wgpu::BindingType::ReadonlyStorageTexture;
                        } else {
                            info->type = wgpu::BindingType::StorageTexture;
                        }

                        spirv_cross::SPIRType::ImageType imageType =
                            compiler.get_type(info->base_type_id).image;
                        wgpu::TextureFormat storageTextureFormat =
                            ToWGPUTextureFormat(imageType.format);
                        if (storageTextureFormat == wgpu::TextureFormat::Undefined) {
                            return DAWN_VALIDATION_ERROR(
                                "Invalid image format declaration on storage image");
                        }
                        const Format& format =
                            GetDevice()->GetValidInternalFormat(storageTextureFormat);
                        if (!format.supportsStorageUsage) {
                            return DAWN_VALIDATION_ERROR(
                                "The storage texture format is not supported");
                        }
                        info->multisampled = imageType.ms;
                        info->storageTextureFormat = storageTextureFormat;
                        info->viewDimension =
                            SpirvDimToTextureViewDimension(imageType.dim, imageType.arrayed);
                        break;
                    }
                    default:
                        info->type = bindingType;
                }
            }
            return {};
        };

        DAWN_TRY(ExtractResourcesBinding(resources.uniform_buffers, compiler,
                                         wgpu::BindingType::UniformBuffer));
        DAWN_TRY(ExtractResourcesBinding(resources.separate_images, compiler,
                                         wgpu::BindingType::SampledTexture));
        DAWN_TRY(ExtractResourcesBinding(resources.separate_samplers, compiler,
                                         wgpu::BindingType::Sampler));
        DAWN_TRY(ExtractResourcesBinding(resources.storage_buffers, compiler,
                                         wgpu::BindingType::StorageBuffer));
        DAWN_TRY(ExtractResourcesBinding(resources.storage_images, compiler,
                                         wgpu::BindingType::StorageTexture));

        // Extract the vertex attributes
        if (mExecutionModel == SingleShaderStage::Vertex) {
            for (const auto& attrib : resources.stage_inputs) {
                if (!(compiler.get_decoration_bitset(attrib.id).get(spv::DecorationLocation))) {
                    return DAWN_VALIDATION_ERROR(
                        "Unable to find Location decoration for Vertex input");
                }
                uint32_t location = compiler.get_decoration(attrib.id, spv::DecorationLocation);

                if (location >= kMaxVertexAttributes) {
                    return DAWN_VALIDATION_ERROR("Attribute location over limits in the SPIRV");
                }

                mUsedVertexAttributes.set(location);
            }

            // Without a location qualifier on vertex outputs, spirv_cross::CompilerMSL gives
            // them all the location 0, causing a compile error.
            for (const auto& attrib : resources.stage_outputs) {
                if (!compiler.get_decoration_bitset(attrib.id).get(spv::DecorationLocation)) {
                    return DAWN_VALIDATION_ERROR("Need location qualifier on vertex output");
                }
            }
        }

        if (mExecutionModel == SingleShaderStage::Fragment) {
            // Without a location qualifier on vertex inputs, spirv_cross::CompilerMSL gives
            // them all the location 0, causing a compile error.
            for (const auto& attrib : resources.stage_inputs) {
                if (!compiler.get_decoration_bitset(attrib.id).get(spv::DecorationLocation)) {
                    return DAWN_VALIDATION_ERROR("Need location qualifier on fragment input");
                }
            }

            for (const auto& fragmentOutput : resources.stage_outputs) {
                if (!compiler.get_decoration_bitset(fragmentOutput.id)
                         .get(spv::DecorationLocation)) {
                    return DAWN_VALIDATION_ERROR(
                        "Unable to find Location decoration for Fragment output");
                }
                uint32_t location =
                    compiler.get_decoration(fragmentOutput.id, spv::DecorationLocation);
                if (location >= kMaxColorAttachments) {
                    return DAWN_VALIDATION_ERROR(
                        "Fragment output location over limits in the SPIRV");
                }

                spirv_cross::SPIRType::BaseType shaderFragmentOutputBaseType =
                    compiler.get_type(fragmentOutput.base_type_id).basetype;
                Format::Type formatType =
                    SpirvCrossBaseTypeToFormatType(shaderFragmentOutputBaseType);
                if (formatType == Format::Type::Other) {
                    return DAWN_VALIDATION_ERROR("Unexpected Fragment output type");
                }
                mFragmentOutputFormatBaseTypes[location] = formatType;
            }
        }
        return {};
    }

    const ShaderModuleBase::ModuleBindingInfo& ShaderModuleBase::GetBindingInfo() const {
        ASSERT(!IsError());
        return mBindingInfo;
    }

    const std::bitset<kMaxVertexAttributes>& ShaderModuleBase::GetUsedVertexAttributes() const {
        ASSERT(!IsError());
        return mUsedVertexAttributes;
    }

    const ShaderModuleBase::FragmentOutputBaseTypes& ShaderModuleBase::GetFragmentOutputBaseTypes()
        const {
        ASSERT(!IsError());
        return mFragmentOutputFormatBaseTypes;
    }

    SingleShaderStage ShaderModuleBase::GetExecutionModel() const {
        ASSERT(!IsError());
        return mExecutionModel;
    }

    RequiredBufferSizes ShaderModuleBase::ComputeRequiredBufferSizesForLayout(
        const PipelineLayoutBase* layout) const {
        RequiredBufferSizes bufferSizes;
        for (BindGroupIndex group : IterateBitSet(layout->GetBindGroupLayoutsMask())) {
            bufferSizes[group] =
                GetBindGroupMinBufferSizes(mBindingInfo[group], layout->GetBindGroupLayout(group));
        }

        return bufferSizes;
    }

    std::vector<uint64_t> ShaderModuleBase::GetBindGroupMinBufferSizes(
        const BindingInfoMap& shaderMap,
        const BindGroupLayoutBase* layout) const {
        std::vector<uint64_t> requiredBufferSizes(layout->GetUnverifiedBufferCount());
        uint32_t packedIdx = 0;

        for (BindingIndex bindingIndex{0}; bindingIndex < layout->GetBufferCount();
             ++bindingIndex) {
            const BindingInfo& bindingInfo = layout->GetBindingInfo(bindingIndex);
            if (bindingInfo.minBufferBindingSize != 0) {
                // Skip bindings that have minimum buffer size set in the layout
                continue;
            }

            ASSERT(packedIdx < requiredBufferSizes.size());
            const auto& shaderInfo = shaderMap.find(bindingInfo.binding);
            if (shaderInfo != shaderMap.end()) {
                requiredBufferSizes[packedIdx] = shaderInfo->second.minBufferBindingSize;
            } else {
                // We have to include buffers if they are included in the bind group's
                // packed vector. We don't actually need to check these at draw time, so
                // if this is a problem in the future we can optimize it further.
                requiredBufferSizes[packedIdx] = 0;
            }
            ++packedIdx;
        }

        return requiredBufferSizes;
    }

    MaybeError ShaderModuleBase::ValidateCompatibilityWithPipelineLayout(
        const PipelineLayoutBase* layout) const {
        ASSERT(!IsError());

        for (BindGroupIndex group : IterateBitSet(layout->GetBindGroupLayoutsMask())) {
            DAWN_TRY(
                ValidateCompatibilityWithBindGroupLayout(group, layout->GetBindGroupLayout(group)));
        }

        for (BindGroupIndex group : IterateBitSet(~layout->GetBindGroupLayoutsMask())) {
            if (mBindingInfo[group].size() > 0) {
                std::ostringstream ostream;
                ostream << "No bind group layout entry matches the declaration set "
                        << static_cast<uint32_t>(group) << " in the shader module";
                return DAWN_VALIDATION_ERROR(ostream.str());
            }
        }

        return {};
    }

    MaybeError ShaderModuleBase::ValidateCompatibilityWithBindGroupLayout(
        BindGroupIndex group,
        const BindGroupLayoutBase* layout) const {
        ASSERT(!IsError());

        const BindGroupLayoutBase::BindingMap& bindingMap = layout->GetBindingMap();

        // Iterate over all bindings used by this group in the shader, and find the
        // corresponding binding in the BindGroupLayout, if it exists.
        for (const auto& it : mBindingInfo[group]) {
            BindingNumber bindingNumber = it.first;
            const ShaderBindingInfo& moduleInfo = it.second;

            const auto& bindingIt = bindingMap.find(bindingNumber);
            if (bindingIt == bindingMap.end()) {
                return DAWN_VALIDATION_ERROR("Missing bind group layout entry for " +
                                             GetShaderDeclarationString(group, bindingNumber));
            }
            BindingIndex bindingIndex(bindingIt->second);

            const BindingInfo& bindingInfo = layout->GetBindingInfo(bindingIndex);

            if (bindingInfo.type != moduleInfo.type) {
                // Binding mismatch between shader and bind group is invalid. For example, a
                // writable binding in the shader with a readonly storage buffer in the bind group
                // layout is invalid. However, a readonly binding in the shader with a writable
                // storage buffer in the bind group layout is valid.
                bool validBindingConversion =
                    bindingInfo.type == wgpu::BindingType::StorageBuffer &&
                    moduleInfo.type == wgpu::BindingType::ReadonlyStorageBuffer;

                // TODO(crbug.com/dawn/367): Temporarily allow using either a sampler or a
                // comparison sampler until we can perform the proper shader analysis of what type
                // is used in the shader module.
                validBindingConversion |= (bindingInfo.type == wgpu::BindingType::Sampler &&
                                           moduleInfo.type == wgpu::BindingType::ComparisonSampler);
                validBindingConversion |=
                    (bindingInfo.type == wgpu::BindingType::ComparisonSampler &&
                     moduleInfo.type == wgpu::BindingType::Sampler);

                if (!validBindingConversion) {
                    return DAWN_VALIDATION_ERROR(
                        "The binding type of the bind group layout entry conflicts " +
                        GetShaderDeclarationString(group, bindingNumber));
                }
            }

            if ((bindingInfo.visibility & StageBit(mExecutionModel)) == 0) {
                return DAWN_VALIDATION_ERROR("The bind group layout entry for " +
                                             GetShaderDeclarationString(group, bindingNumber) +
                                             " is not visible for the shader stage");
            }

            switch (bindingInfo.type) {
                case wgpu::BindingType::SampledTexture: {
                    if (bindingInfo.textureComponentType != moduleInfo.textureComponentType) {
                        return DAWN_VALIDATION_ERROR(
                            "The textureComponentType of the bind group layout entry is different "
                            "from " +
                            GetShaderDeclarationString(group, bindingNumber));
                    }

                    if (bindingInfo.viewDimension != moduleInfo.viewDimension) {
                        return DAWN_VALIDATION_ERROR(
                            "The viewDimension of the bind group layout entry is different "
                            "from " +
                            GetShaderDeclarationString(group, bindingNumber));
                    }
                    break;
                }

                case wgpu::BindingType::ReadonlyStorageTexture:
                case wgpu::BindingType::WriteonlyStorageTexture: {
                    ASSERT(bindingInfo.storageTextureFormat != wgpu::TextureFormat::Undefined);
                    ASSERT(moduleInfo.storageTextureFormat != wgpu::TextureFormat::Undefined);
                    if (bindingInfo.storageTextureFormat != moduleInfo.storageTextureFormat) {
                        return DAWN_VALIDATION_ERROR(
                            "The storageTextureFormat of the bind group layout entry is different "
                            "from " +
                            GetShaderDeclarationString(group, bindingNumber));
                    }
                    if (bindingInfo.viewDimension != moduleInfo.viewDimension) {
                        return DAWN_VALIDATION_ERROR(
                            "The viewDimension of the bind group layout entry is different "
                            "from " +
                            GetShaderDeclarationString(group, bindingNumber));
                    }
                    break;
                }

                case wgpu::BindingType::UniformBuffer:
                case wgpu::BindingType::ReadonlyStorageBuffer:
                case wgpu::BindingType::StorageBuffer: {
                    if (bindingInfo.minBufferBindingSize != 0 &&
                        moduleInfo.minBufferBindingSize > bindingInfo.minBufferBindingSize) {
                        return DAWN_VALIDATION_ERROR(
                            "The minimum buffer size of the bind group layout entry is smaller "
                            "than " +
                            GetShaderDeclarationString(group, bindingNumber));
                    }
                    break;
                }
                case wgpu::BindingType::Sampler:
                case wgpu::BindingType::ComparisonSampler:
                    break;

                case wgpu::BindingType::StorageTexture:
                default:
                    UNREACHABLE();
                    return DAWN_VALIDATION_ERROR("Unsupported binding type");
            }
        }

        return {};
    }

    size_t ShaderModuleBase::HashFunc::operator()(const ShaderModuleBase* module) const {
        size_t hash = 0;

        for (uint32_t word : module->mSpirv) {
            HashCombine(&hash, word);
        }

        return hash;
    }

    bool ShaderModuleBase::EqualityFunc::operator()(const ShaderModuleBase* a,
                                                    const ShaderModuleBase* b) const {
        return a->mSpirv == b->mSpirv;
    }

    MaybeError ShaderModuleBase::CheckSpvcSuccess(shaderc_spvc_status status,
                                                  const char* error_msg) {
        if (status != shaderc_spvc_status_success) {
            return DAWN_VALIDATION_ERROR(error_msg);
        }
        return {};
    }

    shaderc_spvc::Context* ShaderModuleBase::GetContext() {
        return &mSpvcContext;
    }

    const std::vector<uint32_t>& ShaderModuleBase::GetSpirv() const {
        return mSpirv;
    }

    shaderc_spvc::CompileOptions ShaderModuleBase::GetCompileOptions() const {
        shaderc_spvc::CompileOptions options;
        options.SetValidate(GetDevice()->IsValidationEnabled());
        return options;
    }

    MaybeError ShaderModuleBase::InitializeBase() {
        if (mType == Type::Wgsl) {
#ifdef DAWN_ENABLE_WGSL
            DAWN_TRY_ASSIGN(mSpirv, ConvertWGSLToSPIRV(mWgsl.c_str()));
#else
            return DAWN_VALIDATION_ERROR("WGSL not supported (yet)");
#endif  // DAWN_ENABLE_WGSL
        }

        return {};
    }
}  // namespace dawn_native
