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

#ifndef DAWNNATIVE_TOBACKEND_H_
#define DAWNNATIVE_TOBACKEND_H_

#include "dawn_native/Forward.h"

namespace dawn_native {

    // ToBackendTraits implements the mapping from base type to member type of BackendTraits
    template <typename T, typename BackendTraits>
    struct ToBackendTraits;

    template <typename BackendTraits>
    struct ToBackendTraits<AdapterBase, BackendTraits> {
        using BackendType = typename BackendTraits::AdapterType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<BindGroupBase, BackendTraits> {
        using BackendType = typename BackendTraits::BindGroupType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<BindGroupLayoutBase, BackendTraits> {
        using BackendType = typename BackendTraits::BindGroupLayoutType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<BufferBase, BackendTraits> {
        using BackendType = typename BackendTraits::BufferType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<CommandBufferBase, BackendTraits> {
        using BackendType = typename BackendTraits::CommandBufferType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<ComputePipelineBase, BackendTraits> {
        using BackendType = typename BackendTraits::ComputePipelineType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<DeviceBase, BackendTraits> {
        using BackendType = typename BackendTraits::DeviceType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<PipelineLayoutBase, BackendTraits> {
        using BackendType = typename BackendTraits::PipelineLayoutType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<QuerySetBase, BackendTraits> {
        using BackendType = typename BackendTraits::QuerySetType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<QueueBase, BackendTraits> {
        using BackendType = typename BackendTraits::QueueType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<RenderPipelineBase, BackendTraits> {
        using BackendType = typename BackendTraits::RenderPipelineType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<ResourceHeapBase, BackendTraits> {
        using BackendType = typename BackendTraits::ResourceHeapType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<SamplerBase, BackendTraits> {
        using BackendType = typename BackendTraits::SamplerType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<ShaderModuleBase, BackendTraits> {
        using BackendType = typename BackendTraits::ShaderModuleType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<StagingBufferBase, BackendTraits> {
        using BackendType = typename BackendTraits::StagingBufferType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<TextureBase, BackendTraits> {
        using BackendType = typename BackendTraits::TextureType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<SwapChainBase, BackendTraits> {
        using BackendType = typename BackendTraits::SwapChainType;
    };

    template <typename BackendTraits>
    struct ToBackendTraits<TextureViewBase, BackendTraits> {
        using BackendType = typename BackendTraits::TextureViewType;
    };

    // ToBackendBase implements conversion to the given BackendTraits
    // To use it in a backend, use the following:
    //   template<typename T>
    //   auto ToBackend(T&& common) -> decltype(ToBackendBase<MyBackendTraits>(common)) {
    //       return ToBackendBase<MyBackendTraits>(common);
    //   }

    template <typename BackendTraits, typename T>
    Ref<typename ToBackendTraits<T, BackendTraits>::BackendType>& ToBackendBase(Ref<T>& common) {
        return reinterpret_cast<Ref<typename ToBackendTraits<T, BackendTraits>::BackendType>&>(
            common);
    }

    template <typename BackendTraits, typename T>
    const Ref<typename ToBackendTraits<T, BackendTraits>::BackendType>& ToBackendBase(
        const Ref<T>& common) {
        return reinterpret_cast<
            const Ref<typename ToBackendTraits<T, BackendTraits>::BackendType>&>(common);
    }

    template <typename BackendTraits, typename T>
    typename ToBackendTraits<T, BackendTraits>::BackendType* ToBackendBase(T* common) {
        return reinterpret_cast<typename ToBackendTraits<T, BackendTraits>::BackendType*>(common);
    }

    template <typename BackendTraits, typename T>
    const typename ToBackendTraits<T, BackendTraits>::BackendType* ToBackendBase(const T* common) {
        return reinterpret_cast<const typename ToBackendTraits<T, BackendTraits>::BackendType*>(
            common);
    }

}  // namespace dawn_native

#endif  // DAWNNATIVE_TOBACKEND_H_
