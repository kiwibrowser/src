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

#include <array>

#include "common/Assert.h"
#include "common/BitSetIterator.h"
#include "dawn_native/Toggles.h"

namespace dawn_native {
    namespace {

        struct ToggleEnumAndInfo {
            Toggle toggle;
            ToggleInfo info;
        };

        using ToggleEnumAndInfoList =
            std::array<ToggleEnumAndInfo, static_cast<size_t>(Toggle::EnumCount)>;

        static constexpr ToggleEnumAndInfoList kToggleNameAndInfoList = {
            {{Toggle::EmulateStoreAndMSAAResolve,
              {"emulate_store_and_msaa_resolve",
               "Emulate storing into multisampled color attachments and doing MSAA resolve "
               "simultaneously. This workaround is enabled by default on the Metal drivers that do "
               "not support MTLStoreActionStoreAndMultisampleResolve. To support StoreOp::Store on "
               "those platforms, we should do MSAA resolve in another render pass after ending the "
               "previous one.",
               "https://crbug.com/dawn/56"}},
             {Toggle::NonzeroClearResourcesOnCreationForTesting,
              {"nonzero_clear_resources_on_creation_for_testing",
               "Clears texture to full 1 bits as soon as they are created, but doesn't update "
               "the tracking state of the texture. This way we can test the logic of clearing "
               "textures that use recycled memory.",
               "https://crbug.com/dawn/145"}},
             {Toggle::AlwaysResolveIntoZeroLevelAndLayer,
              {"always_resolve_into_zero_level_and_layer",
               "When the resolve target is a texture view that is created on the non-zero level or "
               "layer of a texture, we first resolve into a temporarily 2D texture with only one "
               "mipmap level and one array layer, and copy the result of MSAA resolve into the "
               "true resolve target. This workaround is enabled by default on the Metal drivers "
               "that have bugs when setting non-zero resolveLevel or resolveSlice.",
               "https://crbug.com/dawn/56"}},
             {Toggle::LazyClearResourceOnFirstUse,
              {"lazy_clear_resource_on_first_use",
               "Clears resource to zero on first usage. This initializes the resource "
               "so that no dirty bits from recycled memory is present in the new resource.",
               "https://crbug.com/dawn/145"}},
             {Toggle::TurnOffVsync,
              {"turn_off_vsync",
               "Turn off vsync when rendering. In order to do performance test or run perf tests, "
               "turn off vsync so that the fps can exeed 60.",
               "https://crbug.com/dawn/237"}},
             {Toggle::UseTemporaryBufferInCompressedTextureToTextureCopy,
              {"use_temporary_buffer_in_texture_to_texture_copy",
               "Split texture-to-texture copy into two copies: copy from source texture into a "
               "temporary buffer, and copy from the temporary buffer into the destination texture "
               "when copying between compressed textures that don't have block-aligned sizes. This "
               "workaround is enabled by default on all Vulkan drivers to solve an issue in the "
               "Vulkan SPEC about the texture-to-texture copies with compressed formats. See #1005 "
               "(https://github.com/KhronosGroup/Vulkan-Docs/issues/1005) for more details.",
               "https://crbug.com/dawn/42"}},
             {Toggle::UseD3D12ResourceHeapTier2,
              {"use_d3d12_resource_heap_tier2",
               "Enable support for resource heap tier 2. Resource heap tier 2 allows mixing of "
               "texture and buffers in the same heap. This allows better heap re-use and reduces "
               "fragmentation.",
               "https://crbug.com/dawn/27"}},
             {Toggle::UseD3D12RenderPass,
              {"use_d3d12_render_pass",
               "Use the D3D12 render pass API introduced in Windows build 1809 by default. On "
               "versions of Windows prior to build 1809, or when this toggle is turned off, Dawn "
               "will emulate a render pass.",
               "https://crbug.com/dawn/36"}},
             {Toggle::UseD3D12ResidencyManagement,
              {"use_d3d12_residency_management",
               "Enable residency management. This allows page-in and page-out of resource heaps in "
               "GPU memory. This component improves overcommitted performance by keeping the most "
               "recently used resources local to the GPU. Turning this component off can cause "
               "allocation failures when application memory exceeds physical device memory.",
               "https://crbug.com/dawn/193"}},
             {Toggle::SkipValidation,
              {"skip_validation", "Skip expensive validation of Dawn commands.",
               "https://crbug.com/dawn/271"}},
             {Toggle::UseSpvc,
              {"use_spvc",
               "Enable use of spvc for shader compilation, instead of accessing spirv_cross "
               "directly.",
               "https://crbug.com/dawn/288"}},
             {Toggle::UseSpvcParser,
              {"use_spvc_parser",
               "Enable usage of spvc's internal parsing and IR generation code, instead of "
               "spirv_cross's.",
               "https://crbug.com/dawn/288"}},
             {Toggle::VulkanUseD32S8,
              {"vulkan_use_d32s8",
               "Vulkan mandates support of either D32_FLOAT_S8 or D24_UNORM_S8. When available the "
               "backend will use D32S8 (toggle to on) but setting the toggle to off will make it"
               "use the D24S8 format when possible.",
               "https://crbug.com/dawn/286"}},
             {Toggle::MetalDisableSamplerCompare,
              {"metal_disable_sampler_compare",
               "Disables the use of sampler compare on Metal. This is unsupported before A9 "
               "processors.",
               "https://crbug.com/dawn/342"}},
             {Toggle::DisableBaseVertex,
              {"disable_base_vertex",
               "Disables the use of non-zero base vertex which is unsupported on some platforms.",
               "https://crbug.com/dawn/343"}},
             {Toggle::DisableBaseInstance,
              {"disable_base_instance",
               "Disables the use of non-zero base instance which is unsupported on some "
               "platforms.",
               "https://crbug.com/dawn/343"}},
             {Toggle::UseD3D12SmallShaderVisibleHeapForTesting,
              {"use_d3d12_small_shader_visible_heap",
               "Enable use of a small D3D12 shader visible heap, instead of using a large one by "
               "default. This setting is used to test bindgroup encoding.",
               "https://crbug.com/dawn/155"}},
             {Toggle::UseDXC,
              {"use_dxc", "Use DXC instead of FXC for compiling HLSL",
               "https://crbug.com/dawn/402"}},
             {Toggle::DisableRobustness,
              {"disable_robustness", "Disable robust buffer access", "https://crbug.com/dawn/480"}},
             {Toggle::LazyClearBufferOnFirstUse,
              {"lazy_clear_buffer_on_first_use",
               "Clear buffers on their first use. This is a temporary toggle only for the "
               "development of buffer lazy initialization and will be removed after buffer lazy "
               "initialization is completely implemented.",
               "https://crbug.com/dawn/414"}}}};

    }  // anonymous namespace

    void TogglesSet::Set(Toggle toggle, bool enabled) {
        ASSERT(toggle != Toggle::InvalidEnum);
        const size_t toggleIndex = static_cast<size_t>(toggle);
        toggleBitset.set(toggleIndex, enabled);
    }

    bool TogglesSet::Has(Toggle toggle) const {
        ASSERT(toggle != Toggle::InvalidEnum);
        const size_t toggleIndex = static_cast<size_t>(toggle);
        return toggleBitset.test(toggleIndex);
    }

    std::vector<const char*> TogglesSet::GetContainedToggleNames() const {
        std::vector<const char*> togglesNameInUse(toggleBitset.count());

        uint32_t index = 0;
        for (uint32_t i : IterateBitSet(toggleBitset)) {
            const char* toggleName = ToggleEnumToName(static_cast<Toggle>(i));
            togglesNameInUse[index] = toggleName;
            ++index;
        }

        return togglesNameInUse;
    }

    const char* ToggleEnumToName(Toggle toggle) {
        ASSERT(toggle != Toggle::InvalidEnum);

        const ToggleEnumAndInfo& toggleNameAndInfo =
            kToggleNameAndInfoList[static_cast<size_t>(toggle)];
        ASSERT(toggleNameAndInfo.toggle == toggle);
        return toggleNameAndInfo.info.name;
    }

    const ToggleInfo* TogglesInfo::GetToggleInfo(const char* toggleName) {
        ASSERT(toggleName);

        EnsureToggleNameToEnumMapInitialized();

        const auto& iter = mToggleNameToEnumMap.find(toggleName);
        if (iter != mToggleNameToEnumMap.cend()) {
            return &kToggleNameAndInfoList[static_cast<size_t>(iter->second)].info;
        }
        return nullptr;
    }

    Toggle TogglesInfo::ToggleNameToEnum(const char* toggleName) {
        ASSERT(toggleName);

        EnsureToggleNameToEnumMapInitialized();

        const auto& iter = mToggleNameToEnumMap.find(toggleName);
        if (iter != mToggleNameToEnumMap.cend()) {
            return kToggleNameAndInfoList[static_cast<size_t>(iter->second)].toggle;
        }
        return Toggle::InvalidEnum;
    }

    void TogglesInfo::EnsureToggleNameToEnumMapInitialized() {
        if (mToggleNameToEnumMapInitialized) {
            return;
        }

        for (size_t index = 0; index < kToggleNameAndInfoList.size(); ++index) {
            const ToggleEnumAndInfo& toggleNameAndInfo = kToggleNameAndInfoList[index];
            ASSERT(index == static_cast<size_t>(toggleNameAndInfo.toggle));
            mToggleNameToEnumMap[toggleNameAndInfo.info.name] = toggleNameAndInfo.toggle;
        }

        mToggleNameToEnumMapInitialized = true;
    }

}  // namespace dawn_native
