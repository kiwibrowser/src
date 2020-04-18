/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Tobin Ehlis <tobine@google.com>
 */

#include "descriptor_sets.h"
#include "vk_enum_string_helper.h"
#include "vk_safe_struct.h"
#include <sstream>

// Construct DescriptorSetLayout instance from given create info
cvdescriptorset::DescriptorSetLayout::DescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo *p_create_info,
                                                          const VkDescriptorSetLayout layout)
    : layout_(layout), binding_count_(p_create_info->bindingCount), descriptor_count_(0), dynamic_descriptor_count_(0) {
    uint32_t global_index = 0;
    for (uint32_t i = 0; i < binding_count_; ++i) {
        descriptor_count_ += p_create_info->pBindings[i].descriptorCount;
        binding_to_index_map_[p_create_info->pBindings[i].binding] = i;
        binding_to_global_start_index_map_[p_create_info->pBindings[i].binding] = global_index;
        global_index += p_create_info->pBindings[i].descriptorCount ? p_create_info->pBindings[i].descriptorCount - 1 : 0;
        binding_to_global_end_index_map_[p_create_info->pBindings[i].binding] = global_index;
        global_index += p_create_info->pBindings[i].descriptorCount ? 1 : 0;
        bindings_.push_back(safe_VkDescriptorSetLayoutBinding(&p_create_info->pBindings[i]));
        // In cases where we should ignore pImmutableSamplers make sure it's NULL
        if ((p_create_info->pBindings[i].pImmutableSamplers) &&
            ((p_create_info->pBindings[i].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) &&
             (p_create_info->pBindings[i].descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER))) {
            bindings_.back().pImmutableSamplers = nullptr;
        }
        if (p_create_info->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
            p_create_info->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            dynamic_descriptor_count_ += p_create_info->pBindings[i].descriptorCount;
        }
    }
}

// Validate descriptor set layout create info
bool cvdescriptorset::DescriptorSetLayout::ValidateCreateInfo(debug_report_data *report_data,
                                                              const VkDescriptorSetLayoutCreateInfo *create_info) {
    bool skip = false;
    std::unordered_set<uint32_t> bindings;
    for (uint32_t i = 0; i < create_info->bindingCount; ++i) {
        if (!bindings.insert(create_info->pBindings[i].binding).second) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_02345, "DS", "duplicated binding number in VkDescriptorSetLayoutBinding. %s",
                            validation_error_map[VALIDATION_ERROR_02345]);
        }
    }
    return skip;
}

// put all bindings into the given set
void cvdescriptorset::DescriptorSetLayout::FillBindingSet(std::unordered_set<uint32_t> *binding_set) const {
    for (auto binding_index_pair : binding_to_index_map_)
        binding_set->insert(binding_index_pair.first);
}

VkDescriptorSetLayoutBinding const *
cvdescriptorset::DescriptorSetLayout::GetDescriptorSetLayoutBindingPtrFromBinding(const uint32_t binding) const {
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].ptr();
    }
    return nullptr;
}
VkDescriptorSetLayoutBinding const *
cvdescriptorset::DescriptorSetLayout::GetDescriptorSetLayoutBindingPtrFromIndex(const uint32_t index) const {
    if (index >= bindings_.size())
        return nullptr;
    return bindings_[index].ptr();
}
// Return descriptorCount for given binding, 0 if index is unavailable
uint32_t cvdescriptorset::DescriptorSetLayout::GetDescriptorCountFromBinding(const uint32_t binding) const {
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].descriptorCount;
    }
    return 0;
}
// Return descriptorCount for given index, 0 if index is unavailable
uint32_t cvdescriptorset::DescriptorSetLayout::GetDescriptorCountFromIndex(const uint32_t index) const {
    if (index >= bindings_.size())
        return 0;
    return bindings_[index].descriptorCount;
}
// For the given binding, return descriptorType
VkDescriptorType cvdescriptorset::DescriptorSetLayout::GetTypeFromBinding(const uint32_t binding) const {
    assert(binding_to_index_map_.count(binding));
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].descriptorType;
    }
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
// For the given index, return descriptorType
VkDescriptorType cvdescriptorset::DescriptorSetLayout::GetTypeFromIndex(const uint32_t index) const {
    assert(index < bindings_.size());
    return bindings_[index].descriptorType;
}
// For the given global index, return descriptorType
//  Currently just counting up through bindings_, may improve this in future
VkDescriptorType cvdescriptorset::DescriptorSetLayout::GetTypeFromGlobalIndex(const uint32_t index) const {
    uint32_t global_offset = 0;
    for (auto binding : bindings_) {
        global_offset += binding.descriptorCount;
        if (index < global_offset)
            return binding.descriptorType;
    }
    assert(0); // requested global index is out of bounds
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
// For the given binding, return stageFlags
VkShaderStageFlags cvdescriptorset::DescriptorSetLayout::GetStageFlagsFromBinding(const uint32_t binding) const {
    assert(binding_to_index_map_.count(binding));
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].stageFlags;
    }
    return VkShaderStageFlags(0);
}
// For the given binding, return start index
uint32_t cvdescriptorset::DescriptorSetLayout::GetGlobalStartIndexFromBinding(const uint32_t binding) const {
    assert(binding_to_global_start_index_map_.count(binding));
    const auto &btgsi_itr = binding_to_global_start_index_map_.find(binding);
    if (btgsi_itr != binding_to_global_start_index_map_.end()) {
        return btgsi_itr->second;
    }
    // In error case max uint32_t so index is out of bounds to break ASAP
    assert(0);
    return 0xFFFFFFFF;
}
// For the given binding, return end index
uint32_t cvdescriptorset::DescriptorSetLayout::GetGlobalEndIndexFromBinding(const uint32_t binding) const {
    assert(binding_to_global_end_index_map_.count(binding));
    const auto &btgei_itr = binding_to_global_end_index_map_.find(binding);
    if (btgei_itr != binding_to_global_end_index_map_.end()) {
        return btgei_itr->second;
    }
    // In error case max uint32_t so index is out of bounds to break ASAP
    assert(0);
    return 0xFFFFFFFF;
}
// For given binding, return ptr to ImmutableSampler array
VkSampler const *cvdescriptorset::DescriptorSetLayout::GetImmutableSamplerPtrFromBinding(const uint32_t binding) const {
    assert(binding_to_index_map_.count(binding));
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].pImmutableSamplers;
    }
    return nullptr;
}
// For given index, return ptr to ImmutableSampler array
VkSampler const *cvdescriptorset::DescriptorSetLayout::GetImmutableSamplerPtrFromIndex(const uint32_t index) const {
    assert(index < bindings_.size());
    return bindings_[index].pImmutableSamplers;
}
// If our layout is compatible with rh_ds_layout, return true,
//  else return false and fill in error_msg will description of what causes incompatibility
bool cvdescriptorset::DescriptorSetLayout::IsCompatible(const DescriptorSetLayout *rh_ds_layout, std::string *error_msg) const {
    // Trivial case
    if (layout_ == rh_ds_layout->GetDescriptorSetLayout())
        return true;
    if (descriptor_count_ != rh_ds_layout->descriptor_count_) {
        std::stringstream error_str;
        error_str << "DescriptorSetLayout " << layout_ << " has " << descriptor_count_ << " descriptors, but DescriptorSetLayout "
                  << rh_ds_layout->GetDescriptorSetLayout() << " has " << rh_ds_layout->descriptor_count_ << " descriptors.";
        *error_msg = error_str.str();
        return false; // trivial fail case
    }
    // Descriptor counts match so need to go through bindings one-by-one
    //  and verify that type and stageFlags match
    for (auto binding : bindings_) {
        // TODO : Do we also need to check immutable samplers?
        // VkDescriptorSetLayoutBinding *rh_binding;
        if (binding.descriptorCount != rh_ds_layout->GetDescriptorCountFromBinding(binding.binding)) {
            std::stringstream error_str;
            error_str << "Binding " << binding.binding << " for DescriptorSetLayout " << layout_ << " has a descriptorCount of "
                      << binding.descriptorCount << " but binding " << binding.binding << " for DescriptorSetLayout "
                      << rh_ds_layout->GetDescriptorSetLayout() << " has a descriptorCount of "
                      << rh_ds_layout->GetDescriptorCountFromBinding(binding.binding);
            *error_msg = error_str.str();
            return false;
        } else if (binding.descriptorType != rh_ds_layout->GetTypeFromBinding(binding.binding)) {
            std::stringstream error_str;
            error_str << "Binding " << binding.binding << " for DescriptorSetLayout " << layout_ << " is type '"
                      << string_VkDescriptorType(binding.descriptorType) << "' but binding " << binding.binding
                      << " for DescriptorSetLayout " << rh_ds_layout->GetDescriptorSetLayout() << " is type '"
                      << string_VkDescriptorType(rh_ds_layout->GetTypeFromBinding(binding.binding)) << "'";
            *error_msg = error_str.str();
            return false;
        } else if (binding.stageFlags != rh_ds_layout->GetStageFlagsFromBinding(binding.binding)) {
            std::stringstream error_str;
            error_str << "Binding " << binding.binding << " for DescriptorSetLayout " << layout_ << " has stageFlags "
                      << binding.stageFlags << " but binding " << binding.binding << " for DescriptorSetLayout "
                      << rh_ds_layout->GetDescriptorSetLayout() << " has stageFlags "
                      << rh_ds_layout->GetStageFlagsFromBinding(binding.binding);
            *error_msg = error_str.str();
            return false;
        }
    }
    return true;
}

bool cvdescriptorset::DescriptorSetLayout::IsNextBindingConsistent(const uint32_t binding) const {
    if (!binding_to_index_map_.count(binding + 1))
        return false;
    auto const &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        const auto &next_bi_itr = binding_to_index_map_.find(binding + 1);
        if (next_bi_itr != binding_to_index_map_.end()) {
            auto type = bindings_[bi_itr->second].descriptorType;
            auto stage_flags = bindings_[bi_itr->second].stageFlags;
            auto immut_samp = bindings_[bi_itr->second].pImmutableSamplers ? true : false;
            if ((type != bindings_[next_bi_itr->second].descriptorType) ||
                (stage_flags != bindings_[next_bi_itr->second].stageFlags) ||
                (immut_samp != (bindings_[next_bi_itr->second].pImmutableSamplers ? true : false))) {
                return false;
            }
            return true;
        }
    }
    return false;
}
// Starting at offset descriptor of given binding, parse over update_count
//  descriptor updates and verify that for any binding boundaries that are crossed, the next binding(s) are all consistent
//  Consistency means that their type, stage flags, and whether or not they use immutable samplers matches
//  If so, return true. If not, fill in error_msg and return false
bool cvdescriptorset::DescriptorSetLayout::VerifyUpdateConsistency(uint32_t current_binding, uint32_t offset, uint32_t update_count,
                                                                   const char *type, const VkDescriptorSet set,
                                                                   std::string *error_msg) const {
    // Verify consecutive bindings match (if needed)
    auto orig_binding = current_binding;
    // Track count of descriptors in the current_bindings that are remaining to be updated
    auto binding_remaining = GetDescriptorCountFromBinding(current_binding);
    // First, it's legal to offset beyond your own binding so handle that case
    //  Really this is just searching for the binding in which the update begins and adjusting offset accordingly
    while (offset >= binding_remaining) {
        // Advance to next binding, decrement offset by binding size
        offset -= binding_remaining;
        binding_remaining = GetDescriptorCountFromBinding(++current_binding);
    }
    binding_remaining -= offset;
    while (update_count > binding_remaining) { // While our updates overstep current binding
        // Verify next consecutive binding matches type, stage flags & immutable sampler use
        if (!IsNextBindingConsistent(current_binding++)) {
            std::stringstream error_str;
            error_str << "Attempting " << type << " descriptor set " << set << " binding #" << orig_binding << " with #"
                      << update_count << " descriptors being updated but this update oversteps the bounds of this binding and the "
                                         "next binding is not consistent with current binding so this update is invalid.";
            *error_msg = error_str.str();
            return false;
        }
        // For sake of this check consider the bindings updated and grab count for next binding
        update_count -= binding_remaining;
        binding_remaining = GetDescriptorCountFromBinding(current_binding);
    }
    return true;
}

cvdescriptorset::AllocateDescriptorSetsData::AllocateDescriptorSetsData(uint32_t count)
    : required_descriptors_by_type{}, layout_nodes(count, nullptr) {}

cvdescriptorset::DescriptorSet::DescriptorSet(const VkDescriptorSet set, const VkDescriptorPool pool,
                                              const DescriptorSetLayout *layout, const core_validation::layer_data *dev_data)
    : some_update_(false), set_(set), pool_state_(nullptr), p_layout_(layout), device_data_(dev_data) {
    pool_state_ = getDescriptorPoolState(dev_data, pool);
    // Foreach binding, create default descriptors of given type
    for (uint32_t i = 0; i < p_layout_->GetBindingCount(); ++i) {
        auto type = p_layout_->GetTypeFromIndex(i);
        switch (type) {
        case VK_DESCRIPTOR_TYPE_SAMPLER: {
            auto immut_sampler = p_layout_->GetImmutableSamplerPtrFromIndex(i);
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di) {
                if (immut_sampler)
                    descriptors_.emplace_back(new SamplerDescriptor(immut_sampler + di));
                else
                    descriptors_.emplace_back(new SamplerDescriptor());
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
            auto immut = p_layout_->GetImmutableSamplerPtrFromIndex(i);
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di) {
                if (immut)
                    descriptors_.emplace_back(new ImageSamplerDescriptor(immut + di));
                else
                    descriptors_.emplace_back(new ImageSamplerDescriptor());
            }
            break;
        }
        // ImageDescriptors
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di)
                descriptors_.emplace_back(new ImageDescriptor(type));
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di)
                descriptors_.emplace_back(new TexelDescriptor(type));
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di)
                descriptors_.emplace_back(new BufferDescriptor(type));
            break;
        default:
            assert(0); // Bad descriptor type specified
            break;
        }
    }
}

cvdescriptorset::DescriptorSet::~DescriptorSet() {
    InvalidateBoundCmdBuffers();
}


static std::string string_descriptor_req_view_type(descriptor_req req) {
    std::string result("");
    for (unsigned i = 0; i <= VK_IMAGE_VIEW_TYPE_END_RANGE; i++) {
        if (req & (1 << i)) {
            if (result.size()) result += ", ";
            result += string_VkImageViewType(VkImageViewType(i));
        }
    }

    if (!result.size())
        result = "(none)";

    return result;
}


// Is this sets underlying layout compatible with passed in layout according to "Pipeline Layout Compatibility" in spec?
bool cvdescriptorset::DescriptorSet::IsCompatible(const DescriptorSetLayout *layout, std::string *error) const {
    return layout->IsCompatible(p_layout_, error);
}

// Validate that the state of this set is appropriate for the given bindings and dynamic_offsets at Draw time
//  This includes validating that all descriptors in the given bindings are updated,
//  that any update buffers are valid, and that any dynamic offsets are within the bounds of their buffers.
// Return true if state is acceptable, or false and write an error message into error string
bool cvdescriptorset::DescriptorSet::ValidateDrawState(const std::map<uint32_t, descriptor_req> &bindings,
                                                       const std::vector<uint32_t> &dynamic_offsets, std::string *error) const {
    auto dyn_offset_index = 0;
    for (auto binding_pair : bindings) {
        auto binding = binding_pair.first;
        if (!p_layout_->HasBinding(binding)) {
            std::stringstream error_str;
            error_str << "Attempting to validate DrawState for binding #" << binding
                      << " which is an invalid binding for this descriptor set.";
            *error = error_str.str();
            return false;
        }
        auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(binding);
        if (descriptors_[start_idx]->IsImmutableSampler()) {
            // Nothing to do for strictly immutable sampler
        } else {
            auto end_idx = p_layout_->GetGlobalEndIndexFromBinding(binding);
            for (uint32_t i = start_idx; i <= end_idx; ++i) {
                if (!descriptors_[i]->updated) {
                    std::stringstream error_str;
                    error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                              << " is being used in draw but has not been updated.";
                    *error = error_str.str();
                    return false;
                } else {
                    auto descriptor_class = descriptors_[i]->GetClass();
                    if (descriptor_class == GeneralBuffer) {
                        // Verify that buffers are valid
                        auto buffer = static_cast<BufferDescriptor *>(descriptors_[i].get())->GetBuffer();
                        auto buffer_node = getBufferNode(device_data_, buffer);
                        if (!buffer_node) {
                            std::stringstream error_str;
                            error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                                      << " references invalid buffer " << buffer << ".";
                            *error = error_str.str();
                            return false;
                        } else {
                            auto mem_entry = getMemObjInfo(device_data_, buffer_node->binding.mem);
                            if (!mem_entry) {
                                std::stringstream error_str;
                                error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                                          << " uses buffer " << buffer << " that references invalid memory "
                                          << buffer_node->binding.mem << ".";
                                *error = error_str.str();
                                return false;
                            }
                        }
                        if (descriptors_[i]->IsDynamic()) {
                            // Validate that dynamic offsets are within the buffer
                            auto buffer_size = buffer_node->createInfo.size;
                            auto range = static_cast<BufferDescriptor *>(descriptors_[i].get())->GetRange();
                            auto desc_offset = static_cast<BufferDescriptor *>(descriptors_[i].get())->GetOffset();
                            auto dyn_offset = dynamic_offsets[dyn_offset_index++];
                            if (VK_WHOLE_SIZE == range) {
                                if ((dyn_offset + desc_offset) > buffer_size) {
                                    std::stringstream error_str;
                                    error_str << "Dynamic descriptor in binding #" << binding << " at global descriptor index " << i
                                              << " uses buffer " << buffer
                                              << " with update range of VK_WHOLE_SIZE has dynamic offset " << dyn_offset
                                              << " combined with offset " << desc_offset << " that oversteps the buffer size of "
                                              << buffer_size << ".";
                                    *error = error_str.str();
                                    return false;
                                }
                            } else {
                                if ((dyn_offset + desc_offset + range) > buffer_size) {
                                    std::stringstream error_str;
                                    error_str << "Dynamic descriptor in binding #" << binding << " at global descriptor index " << i
                                              << " uses buffer " << buffer << " with dynamic offset " << dyn_offset
                                              << " combined with offset " << desc_offset << " and range " << range
                                              << " that oversteps the buffer size of " << buffer_size << ".";
                                    *error = error_str.str();
                                    return false;
                                }
                            }
                        }
                    }
                    else if (descriptor_class == ImageSampler || descriptor_class == Image) {
                        auto image_view = (descriptor_class == ImageSampler)
                                ? static_cast<ImageSamplerDescriptor *>(descriptors_[i].get())->GetImageView()
                                : static_cast<ImageDescriptor *>(descriptors_[i].get())->GetImageView();
                        auto reqs = binding_pair.second;

                        auto image_view_state = getImageViewState(device_data_, image_view);
                        assert(image_view_state);
                        auto image_view_ci = image_view_state->create_info;

                        if ((reqs & DESCRIPTOR_REQ_ALL_VIEW_TYPE_BITS) && (~reqs & (1 << image_view_ci.viewType))) {
                            // bad view type
                            std::stringstream error_str;
                            error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                                      << " requires an image view of type " << string_descriptor_req_view_type(reqs)
                                      << " but got " << string_VkImageViewType(image_view_ci.viewType) << ".";
                            *error = error_str.str();
                            return false;
                        }

                        auto image_node = getImageState(device_data_, image_view_ci.image);
                        assert(image_node);

                        if ((reqs & DESCRIPTOR_REQ_SINGLE_SAMPLE) &&
                            image_node->createInfo.samples != VK_SAMPLE_COUNT_1_BIT) {
                            std::stringstream error_str;
                            error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                                      << " requires bound image to have VK_SAMPLE_COUNT_1_BIT but got "
                                      << string_VkSampleCountFlagBits(image_node->createInfo.samples) << ".";
                            *error = error_str.str();
                            return false;
                        }

                        if ((reqs & DESCRIPTOR_REQ_MULTI_SAMPLE) &&
                            image_node->createInfo.samples == VK_SAMPLE_COUNT_1_BIT) {
                            std::stringstream error_str;
                            error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                                      << " requires bound image to have multiple samples, but got VK_SAMPLE_COUNT_1_BIT.";
                            *error = error_str.str();
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

// For given bindings, place any update buffers or images into the passed-in unordered_sets
uint32_t cvdescriptorset::DescriptorSet::GetStorageUpdates(const std::map<uint32_t, descriptor_req> &bindings,
                                                           std::unordered_set<VkBuffer> *buffer_set,
                                                           std::unordered_set<VkImageView> *image_set) const {
    auto num_updates = 0;
    for (auto binding_pair : bindings) {
        auto binding = binding_pair.first;
        // If a binding doesn't exist, skip it
        if (!p_layout_->HasBinding(binding)) {
            continue;
        }
        auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(binding);
        if (descriptors_[start_idx]->IsStorage()) {
            if (Image == descriptors_[start_idx]->descriptor_class) {
                for (uint32_t i = 0; i < p_layout_->GetDescriptorCountFromBinding(binding); ++i) {
                    if (descriptors_[start_idx + i]->updated) {
                        image_set->insert(static_cast<ImageDescriptor *>(descriptors_[start_idx + i].get())->GetImageView());
                        num_updates++;
                    }
                }
            } else if (TexelBuffer == descriptors_[start_idx]->descriptor_class) {
                for (uint32_t i = 0; i < p_layout_->GetDescriptorCountFromBinding(binding); ++i) {
                    if (descriptors_[start_idx + i]->updated) {
                        auto bufferview = static_cast<TexelDescriptor *>(descriptors_[start_idx + i].get())->GetBufferView();
                        auto bv_state = getBufferViewState(device_data_, bufferview);
                        if (bv_state) {
                            buffer_set->insert(bv_state->create_info.buffer);
                            num_updates++;
                        }
                    }
                }
            } else if (GeneralBuffer == descriptors_[start_idx]->descriptor_class) {
                for (uint32_t i = 0; i < p_layout_->GetDescriptorCountFromBinding(binding); ++i) {
                    if (descriptors_[start_idx + i]->updated) {
                        buffer_set->insert(static_cast<BufferDescriptor *>(descriptors_[start_idx + i].get())->GetBuffer());
                        num_updates++;
                    }
                }
            }
        }
    }
    return num_updates;
}
// Set is being deleted or updates so invalidate all bound cmd buffers
void cvdescriptorset::DescriptorSet::InvalidateBoundCmdBuffers() {
    core_validation::invalidateCommandBuffers(cb_bindings,
                                              {reinterpret_cast<uint64_t &>(set_), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT});
}
// Perform write update in given update struct
void cvdescriptorset::DescriptorSet::PerformWriteUpdate(const VkWriteDescriptorSet *update) {
    auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding) + update->dstArrayElement;
    // perform update
    for (uint32_t di = 0; di < update->descriptorCount; ++di) {
        descriptors_[start_idx + di]->WriteUpdate(update, di);
    }
    if (update->descriptorCount)
        some_update_ = true;

    InvalidateBoundCmdBuffers();
}
// Validate Copy update
bool cvdescriptorset::DescriptorSet::ValidateCopyUpdate(const debug_report_data *report_data, const VkCopyDescriptorSet *update,
                                                        const DescriptorSet *src_set, UNIQUE_VALIDATION_ERROR_CODE *error_code,
                                                        std::string *error_msg) {
    // Verify idle ds
    if (in_use.load()) {
        // TODO : Re-using Allocate Idle error code, need copy update idle error code
        *error_code = VALIDATION_ERROR_00919;
        std::stringstream error_str;
        error_str << "Cannot call vkUpdateDescriptorSets() to perform copy update on descriptor set " << set_
                  << " that is in use by a command buffer.";
        *error_msg = error_str.str();
        return false;
    }
    if (!p_layout_->HasBinding(update->dstBinding)) {
        *error_code = VALIDATION_ERROR_00966;
        std::stringstream error_str;
        error_str << "DescriptorSet " << set_ << " does not have copy update dest binding of " << update->dstBinding << ".";
        *error_msg = error_str.str();
        return false;
    }
    if (!src_set->HasBinding(update->srcBinding)) {
        *error_code = VALIDATION_ERROR_00964;
        std::stringstream error_str;
        error_str << "DescriptorSet " << set_ << " does not have copy update src binding of " << update->srcBinding << ".";
        *error_msg = error_str.str();
        return false;
    }
    // src & dst set bindings are valid
    // Check bounds of src & dst
    auto src_start_idx = src_set->GetGlobalStartIndexFromBinding(update->srcBinding) + update->srcArrayElement;
    if ((src_start_idx + update->descriptorCount) > src_set->GetTotalDescriptorCount()) {
        // SRC update out of bounds
        *error_code = VALIDATION_ERROR_00965;
        std::stringstream error_str;
        error_str << "Attempting copy update from descriptorSet " << update->srcSet << " binding#" << update->srcBinding
                  << " with offset index of " << src_set->GetGlobalStartIndexFromBinding(update->srcBinding)
                  << " plus update array offset of " << update->srcArrayElement << " and update of " << update->descriptorCount
                  << " descriptors oversteps total number of descriptors in set: " << src_set->GetTotalDescriptorCount() << ".";
        *error_msg = error_str.str();
        return false;
    }
    auto dst_start_idx = p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding) + update->dstArrayElement;
    if ((dst_start_idx + update->descriptorCount) > p_layout_->GetTotalDescriptorCount()) {
        // DST update out of bounds
        *error_code = VALIDATION_ERROR_00967;
        std::stringstream error_str;
        error_str << "Attempting copy update to descriptorSet " << set_ << " binding#" << update->dstBinding
                  << " with offset index of " << p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding)
                  << " plus update array offset of " << update->dstArrayElement << " and update of " << update->descriptorCount
                  << " descriptors oversteps total number of descriptors in set: " << p_layout_->GetTotalDescriptorCount() << ".";
        *error_msg = error_str.str();
        return false;
    }
    // Check that types match
    // TODO : Base default error case going from here is VALIDATION_ERROR_00968 which covers all consistency issues, need more
    // fine-grained error codes
    *error_code = VALIDATION_ERROR_00968;
    auto src_type = src_set->GetTypeFromBinding(update->srcBinding);
    auto dst_type = p_layout_->GetTypeFromBinding(update->dstBinding);
    if (src_type != dst_type) {
        std::stringstream error_str;
        error_str << "Attempting copy update to descriptorSet " << set_ << " binding #" << update->dstBinding << " with type "
                  << string_VkDescriptorType(dst_type) << " from descriptorSet " << src_set->GetSet() << " binding #"
                  << update->srcBinding << " with type " << string_VkDescriptorType(src_type) << ". Types do not match.";
        *error_msg = error_str.str();
        return false;
    }
    // Verify consistency of src & dst bindings if update crosses binding boundaries
    if ((!src_set->GetLayout()->VerifyUpdateConsistency(update->srcBinding, update->srcArrayElement, update->descriptorCount,
                                                        "copy update from", src_set->GetSet(), error_msg)) ||
        (!p_layout_->VerifyUpdateConsistency(update->dstBinding, update->dstArrayElement, update->descriptorCount, "copy update to",
                                             set_, error_msg))) {
        return false;
    }
    // First make sure source descriptors are updated
    for (uint32_t i = 0; i < update->descriptorCount; ++i) {
        if (!src_set->descriptors_[src_start_idx + i]) {
            std::stringstream error_str;
            error_str << "Attempting copy update from descriptorSet " << src_set << " binding #" << update->srcBinding << " but descriptor at array offset "
                      << update->srcArrayElement + i << " has not been updated.";
            *error_msg = error_str.str();
            return false;
        }
    }
    // Update parameters all look good and descriptor updated so verify update contents
    if (!VerifyCopyUpdateContents(update, src_set, src_type, src_start_idx, error_code, error_msg))
        return false;

    // All checks passed so update is good
    return true;
}
// Perform Copy update
void cvdescriptorset::DescriptorSet::PerformCopyUpdate(const VkCopyDescriptorSet *update, const DescriptorSet *src_set) {
    auto src_start_idx = src_set->GetGlobalStartIndexFromBinding(update->srcBinding) + update->srcArrayElement;
    auto dst_start_idx = p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding) + update->dstArrayElement;
    // Update parameters all look good so perform update
    for (uint32_t di = 0; di < update->descriptorCount; ++di) {
        descriptors_[dst_start_idx + di]->CopyUpdate(src_set->descriptors_[src_start_idx + di].get());
    }
    if (update->descriptorCount)
        some_update_ = true;

    InvalidateBoundCmdBuffers();
}

// Bind cb_node to this set and this set to cb_node.
// Prereq: This should be called for a set that has been confirmed to be active for the given cb_node, meaning it's going
//   to be used in a draw by the given cb_node
void cvdescriptorset::DescriptorSet::BindCommandBuffer(GLOBAL_CB_NODE *cb_node, const std::unordered_set<uint32_t> &bindings) {
    // bind cb to this descriptor set
    cb_bindings.insert(cb_node);
    // Add bindings for descriptor set, the set's pool, and individual objects in the set
    cb_node->object_bindings.insert({reinterpret_cast<uint64_t &>(set_), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT});
    pool_state_->cb_bindings.insert(cb_node);
    cb_node->object_bindings.insert(
        {reinterpret_cast<uint64_t &>(pool_state_->pool), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT});
    // For the active slots, use set# to look up descriptorSet from boundDescriptorSets, and bind all of that descriptor set's
    // resources
    for (auto binding : bindings) {
        auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(binding);
        auto end_idx = p_layout_->GetGlobalEndIndexFromBinding(binding);
        for (uint32_t i = start_idx; i <= end_idx; ++i) {
            descriptors_[i]->BindCommandBuffer(device_data_, cb_node);
        }
    }
}

cvdescriptorset::SamplerDescriptor::SamplerDescriptor() : sampler_(VK_NULL_HANDLE), immutable_(false) {
    updated = false;
    descriptor_class = PlainSampler;
};

cvdescriptorset::SamplerDescriptor::SamplerDescriptor(const VkSampler *immut) : sampler_(VK_NULL_HANDLE), immutable_(false) {
    updated = false;
    descriptor_class = PlainSampler;
    if (immut) {
        sampler_ = *immut;
        immutable_ = true;
        updated = true;
    }
}
// Validate given sampler. Currently this only checks to make sure it exists in the samplerMap
bool cvdescriptorset::ValidateSampler(const VkSampler sampler, const core_validation::layer_data *dev_data) {
    return (getSamplerState(dev_data, sampler) != nullptr);
}

bool cvdescriptorset::ValidateImageUpdate(VkImageView image_view, VkImageLayout image_layout, VkDescriptorType type,
                                          const core_validation::layer_data *dev_data, UNIQUE_VALIDATION_ERROR_CODE *error_code,
                                          std::string *error_msg) {
    // TODO : Defaulting to 00943 for all cases here. Need to create new error codes for various cases.
    *error_code = VALIDATION_ERROR_00943;
    auto iv_state = getImageViewState(dev_data, image_view);
    if (!iv_state) {
        std::stringstream error_str;
        error_str << "Invalid VkImageView: " << image_view;
        *error_msg = error_str.str();
        return false;
    }
    // Note that when an imageview is created, we validated that memory is bound so no need to re-check here
    // Validate that imageLayout is compatible with aspect_mask and image format
    //  and validate that image usage bits are correct for given usage
    VkImageAspectFlags aspect_mask = iv_state->create_info.subresourceRange.aspectMask;
    VkImage image = iv_state->create_info.image;
    VkFormat format = VK_FORMAT_MAX_ENUM;
    VkImageUsageFlags usage = 0;
    auto image_node = getImageState(dev_data, image);
    if (image_node) {
        format = image_node->createInfo.format;
        usage = image_node->createInfo.usage;
        // Validate that memory is bound to image
        if (ValidateMemoryIsBoundToImage(dev_data, image_node, "vkUpdateDescriptorSets()")) {
            // TODO : Need new code(s) for language in 11.6 Memory Association
            *error_msg = "No memory bound to image.";
            return false;
        }
    } else {
        // Also need to check the swapchains.
        auto swapchain = getSwapchainFromImage(dev_data, image);
        if (swapchain) {
            auto swapchain_node = getSwapchainNode(dev_data, swapchain);
            if (swapchain_node) {
                format = swapchain_node->createInfo.imageFormat;
            }
        }
    }
    // First validate that format and layout are compatible
    if (format == VK_FORMAT_MAX_ENUM) {
        std::stringstream error_str;
        error_str << "Invalid image (" << image << ") in imageView (" << image_view << ").";
        *error_msg = error_str.str();
        return false;
    }
    // TODO : The various image aspect and format checks here are based on general spec language in 11.5 Image Views section under
    // vkCreateImageView(). What's the best way to create unique id for these cases?
    bool ds = vk_format_is_depth_or_stencil(format);
    switch (image_layout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Only Color bit must be set
        if ((aspect_mask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
            std::stringstream error_str;
            error_str << "ImageView (" << image_view << ") uses layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL but does "
                                                        "not have VK_IMAGE_ASPECT_COLOR_BIT set.";
            *error_msg = error_str.str();
            return false;
        }
        // format must NOT be DS
        if (ds) {
            std::stringstream error_str;
            error_str << "ImageView (" << image_view
                      << ") uses layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL but the image format is "
                      << string_VkFormat(format) << " which is not a color format.";
            *error_msg = error_str.str();
            return false;
        }
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        // Depth or stencil bit must be set, but both must NOT be set
        if (aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT) {
            if (aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT) {
                // both  must NOT be set
                std::stringstream error_str;
                error_str << "ImageView (" << image_view << ") has both STENCIL and DEPTH aspects set";
                *error_msg = error_str.str();
                return false;
            }
        } else if (!(aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
            // Neither were set
            std::stringstream error_str;
            error_str << "ImageView (" << image_view << ") has layout " << string_VkImageLayout(image_layout)
                      << " but does not have STENCIL or DEPTH aspects set";
            *error_msg = error_str.str();
            return false;
        }
        // format must be DS
        if (!ds) {
            std::stringstream error_str;
            error_str << "ImageView (" << image_view << ") has layout " << string_VkImageLayout(image_layout)
                      << " but the image format is " << string_VkFormat(format) << " which is not a depth/stencil format.";
            *error_msg = error_str.str();
            return false;
        }
        break;
    default:
        // For other layouts if the source is depth/stencil image, both aspect bits must not be set
        if (ds) {
            if (aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT) {
                if (aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT) {
                    // both  must NOT be set
                    std::stringstream error_str;
                    error_str << "ImageView (" << image_view << ") has layout " << string_VkImageLayout(image_layout)
                              << " and is using depth/stencil image of format " << string_VkFormat(format)
                              << " but it has both STENCIL and DEPTH aspects set, which is illegal. When using a depth/stencil "
                                 "image in a descriptor set, please only set either VK_IMAGE_ASPECT_DEPTH_BIT or "
                                 "VK_IMAGE_ASPECT_STENCIL_BIT depending on whether it will be used for depth reads or stencil "
                                 "reads respectively.";
                    *error_msg = error_str.str();
                    return false;
                }
            }
        }
        break;
    }
    // Now validate that usage flags are correctly set for given type of update
    //  As we're switching per-type, if any type has specific layout requirements, check those here as well
    // TODO : The various image usage bit requirements are in general spec language for VkImageUsageFlags bit block in 11.3 Images
    // under vkCreateImage()
    // TODO : Need to also validate case VALIDATION_ERROR_00952 where STORAGE_IMAGE & INPUT_ATTACH types must have been created with
    // identify swizzle
    std::string error_usage_bit;
    switch (type) {
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
        if (!(usage & VK_IMAGE_USAGE_SAMPLED_BIT)) {
            error_usage_bit = "VK_IMAGE_USAGE_SAMPLED_BIT";
        }
        break;
    }
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
        if (!(usage & VK_IMAGE_USAGE_STORAGE_BIT)) {
            error_usage_bit = "VK_IMAGE_USAGE_STORAGE_BIT";
        } else if (VK_IMAGE_LAYOUT_GENERAL != image_layout) {
            std::stringstream error_str;
            // TODO : Need to create custom enum error code for this case
            error_str << "ImageView (" << image_view << ") of VK_DESCRIPTOR_TYPE_STORAGE_IMAGE type is being updated with layout "
                      << string_VkImageLayout(image_layout)
                      << " but according to spec section 13.1 Descriptor Types, 'Load and store operations on storage images can "
                         "only be done on images in VK_IMAGE_LAYOUT_GENERAL layout.'";
            *error_msg = error_str.str();
            return false;
        }
        break;
    }
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
        if (!(usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) {
            error_usage_bit = "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
        }
        break;
    }
    default:
        break;
    }
    if (!error_usage_bit.empty()) {
        std::stringstream error_str;
        error_str << "ImageView (" << image_view << ") with usage mask 0x" << usage
                  << " being used for a descriptor update of type " << string_VkDescriptorType(type) << " does not have "
                  << error_usage_bit << " set.";
        *error_msg = error_str.str();
        return false;
    }
    return true;
}

void cvdescriptorset::SamplerDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index) {
    sampler_ = update->pImageInfo[index].sampler;
    updated = true;
}

void cvdescriptorset::SamplerDescriptor::CopyUpdate(const Descriptor *src) {
    if (!immutable_) {
        auto update_sampler = static_cast<const SamplerDescriptor *>(src)->sampler_;
        sampler_ = update_sampler;
    }
    updated = true;
}

void cvdescriptorset::SamplerDescriptor::BindCommandBuffer(const core_validation::layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    if (!immutable_) {
        auto sampler_state = getSamplerState(dev_data, sampler_);
        if (sampler_state)
            core_validation::AddCommandBufferBindingSampler(cb_node, sampler_state);
    }
}

cvdescriptorset::ImageSamplerDescriptor::ImageSamplerDescriptor()
    : sampler_(VK_NULL_HANDLE), immutable_(false), image_view_(VK_NULL_HANDLE), image_layout_(VK_IMAGE_LAYOUT_UNDEFINED) {
    updated = false;
    descriptor_class = ImageSampler;
}

cvdescriptorset::ImageSamplerDescriptor::ImageSamplerDescriptor(const VkSampler *immut)
    : sampler_(VK_NULL_HANDLE), immutable_(true), image_view_(VK_NULL_HANDLE), image_layout_(VK_IMAGE_LAYOUT_UNDEFINED) {
    updated = false;
    descriptor_class = ImageSampler;
    if (immut) {
        sampler_ = *immut;
        immutable_ = true;
        updated = true;
    }
}

void cvdescriptorset::ImageSamplerDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index) {
    updated = true;
    const auto &image_info = update->pImageInfo[index];
    sampler_ = image_info.sampler;
    image_view_ = image_info.imageView;
    image_layout_ = image_info.imageLayout;
}

void cvdescriptorset::ImageSamplerDescriptor::CopyUpdate(const Descriptor *src) {
    if (!immutable_) {
        auto update_sampler = static_cast<const ImageSamplerDescriptor *>(src)->sampler_;
        sampler_ = update_sampler;
    }
    auto image_view = static_cast<const ImageSamplerDescriptor *>(src)->image_view_;
    auto image_layout = static_cast<const ImageSamplerDescriptor *>(src)->image_layout_;
    updated = true;
    image_view_ = image_view;
    image_layout_ = image_layout;
}

void cvdescriptorset::ImageSamplerDescriptor::BindCommandBuffer(const core_validation::layer_data *dev_data,
                                                                GLOBAL_CB_NODE *cb_node) {
    // First add binding for any non-immutable sampler
    if (!immutable_) {
        auto sampler_state = getSamplerState(dev_data, sampler_);
        if (sampler_state)
            core_validation::AddCommandBufferBindingSampler(cb_node, sampler_state);
    }
    // Add binding for image
    auto iv_state = getImageViewState(dev_data, image_view_);
    if (iv_state) {
        core_validation::AddCommandBufferBindingImageView(dev_data, cb_node, iv_state);
    }
}

cvdescriptorset::ImageDescriptor::ImageDescriptor(const VkDescriptorType type)
    : storage_(false), image_view_(VK_NULL_HANDLE), image_layout_(VK_IMAGE_LAYOUT_UNDEFINED) {
    updated = false;
    descriptor_class = Image;
    if (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE == type)
        storage_ = true;
};

void cvdescriptorset::ImageDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index) {
    updated = true;
    const auto &image_info = update->pImageInfo[index];
    image_view_ = image_info.imageView;
    image_layout_ = image_info.imageLayout;
}

void cvdescriptorset::ImageDescriptor::CopyUpdate(const Descriptor *src) {
    auto image_view = static_cast<const ImageDescriptor *>(src)->image_view_;
    auto image_layout = static_cast<const ImageDescriptor *>(src)->image_layout_;
    updated = true;
    image_view_ = image_view;
    image_layout_ = image_layout;
}

void cvdescriptorset::ImageDescriptor::BindCommandBuffer(const core_validation::layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    // Add binding for image
    auto iv_state = getImageViewState(dev_data, image_view_);
    if (iv_state) {
        core_validation::AddCommandBufferBindingImageView(dev_data, cb_node, iv_state);
    }
}

cvdescriptorset::BufferDescriptor::BufferDescriptor(const VkDescriptorType type)
    : storage_(false), dynamic_(false), buffer_(VK_NULL_HANDLE), offset_(0), range_(0) {
    updated = false;
    descriptor_class = GeneralBuffer;
    if (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC == type) {
        dynamic_ = true;
    } else if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == type) {
        storage_ = true;
    } else if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC == type) {
        dynamic_ = true;
        storage_ = true;
    }
}
void cvdescriptorset::BufferDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index) {
    updated = true;
    const auto &buffer_info = update->pBufferInfo[index];
    buffer_ = buffer_info.buffer;
    offset_ = buffer_info.offset;
    range_ = buffer_info.range;
}

void cvdescriptorset::BufferDescriptor::CopyUpdate(const Descriptor *src) {
    auto buff_desc = static_cast<const BufferDescriptor *>(src);
    updated = true;
    buffer_ = buff_desc->buffer_;
    offset_ = buff_desc->offset_;
    range_ = buff_desc->range_;
}

void cvdescriptorset::BufferDescriptor::BindCommandBuffer(const core_validation::layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    auto buffer_node = getBufferNode(dev_data, buffer_);
    if (buffer_node)
        core_validation::AddCommandBufferBindingBuffer(dev_data, cb_node, buffer_node);
}

cvdescriptorset::TexelDescriptor::TexelDescriptor(const VkDescriptorType type) : buffer_view_(VK_NULL_HANDLE), storage_(false) {
    updated = false;
    descriptor_class = TexelBuffer;
    if (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER == type)
        storage_ = true;
};

void cvdescriptorset::TexelDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index) {
    updated = true;
    buffer_view_ = update->pTexelBufferView[index];
}

void cvdescriptorset::TexelDescriptor::CopyUpdate(const Descriptor *src) {
    updated = true;
    buffer_view_ = static_cast<const TexelDescriptor *>(src)->buffer_view_;
}

void cvdescriptorset::TexelDescriptor::BindCommandBuffer(const core_validation::layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    auto bv_state = getBufferViewState(dev_data, buffer_view_);
    if (bv_state) {
        core_validation::AddCommandBufferBindingBufferView(dev_data, cb_node, bv_state);
    }
}

// This is a helper function that iterates over a set of Write and Copy updates, pulls the DescriptorSet* for updated
//  sets, and then calls their respective Validate[Write|Copy]Update functions.
// If the update hits an issue for which the callback returns "true", meaning that the call down the chain should
//  be skipped, then true is returned.
// If there is no issue with the update, then false is returned.
bool cvdescriptorset::ValidateUpdateDescriptorSets(const debug_report_data *report_data,
                                                   const core_validation::layer_data *dev_data, uint32_t write_count,
                                                   const VkWriteDescriptorSet *p_wds, uint32_t copy_count,
                                                   const VkCopyDescriptorSet *p_cds) {
    bool skip_call = false;
    // Validate Write updates
    for (uint32_t i = 0; i < write_count; i++) {
        auto dest_set = p_wds[i].dstSet;
        auto set_node = core_validation::getSetNode(dev_data, dest_set);
        if (!set_node) {
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                        reinterpret_cast<uint64_t &>(dest_set), __LINE__, DRAWSTATE_INVALID_DESCRIPTOR_SET, "DS",
                        "Cannot call vkUpdateDescriptorSets() on descriptor set 0x%" PRIxLEAST64 " that has not been allocated.",
                        reinterpret_cast<uint64_t &>(dest_set));
        } else {
            UNIQUE_VALIDATION_ERROR_CODE error_code;
            std::string error_str;
            if (!set_node->ValidateWriteUpdate(report_data, &p_wds[i], &error_code, &error_str)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                     reinterpret_cast<uint64_t &>(dest_set), __LINE__, error_code, "DS",
                                     "vkUpdateDescriptorsSets() failed write update validation for Descriptor Set 0x%" PRIx64
                                     " with error: %s. %s",
                                     reinterpret_cast<uint64_t &>(dest_set), error_str.c_str(), validation_error_map[error_code]);
            }
        }
    }
    // Now validate copy updates
    for (uint32_t i = 0; i < copy_count; ++i) {
        auto dst_set = p_cds[i].dstSet;
        auto src_set = p_cds[i].srcSet;
        auto src_node = core_validation::getSetNode(dev_data, src_set);
        auto dst_node = core_validation::getSetNode(dev_data, dst_set);
        if (!src_node) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                 reinterpret_cast<uint64_t &>(src_set), __LINE__, VALIDATION_ERROR_00971, "DS",
                                 "Cannot call vkUpdateDescriptorSets() to copy from descriptor set 0x%" PRIxLEAST64
                                 " that has not been allocated. %s",
                                 reinterpret_cast<uint64_t &>(src_set), validation_error_map[VALIDATION_ERROR_00971]);
        } else if (!dst_node) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                 reinterpret_cast<uint64_t &>(dst_set), __LINE__, VALIDATION_ERROR_00972, "DS",
                                 "Cannot call vkUpdateDescriptorSets() to copy to descriptor set 0x%" PRIxLEAST64
                                 " that has not been allocated. %s",
                                 reinterpret_cast<uint64_t &>(dst_set), validation_error_map[VALIDATION_ERROR_00972]);
        } else {
            UNIQUE_VALIDATION_ERROR_CODE error_code;
            std::string error_str;
            if (!dst_node->ValidateCopyUpdate(report_data, &p_cds[i], src_node, &error_code, &error_str)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                     reinterpret_cast<uint64_t &>(dst_set), __LINE__, error_code, "DS",
                                     "vkUpdateDescriptorsSets() failed copy update from Descriptor Set 0x%" PRIx64
                                     " to Descriptor Set 0x%" PRIx64 " with error: %s. %s",
                                     reinterpret_cast<uint64_t &>(src_set), reinterpret_cast<uint64_t &>(dst_set),
                                     error_str.c_str(), validation_error_map[error_code]);
            }
        }
    }
    return skip_call;
}
// This is a helper function that iterates over a set of Write and Copy updates, pulls the DescriptorSet* for updated
//  sets, and then calls their respective Perform[Write|Copy]Update functions.
// Prerequisite : ValidateUpdateDescriptorSets() should be called and return "false" prior to calling PerformUpdateDescriptorSets()
//  with the same set of updates.
// This is split from the validate code to allow validation prior to calling down the chain, and then update after
//  calling down the chain.
void cvdescriptorset::PerformUpdateDescriptorSets(const core_validation::layer_data *dev_data, uint32_t write_count,
                                                  const VkWriteDescriptorSet *p_wds, uint32_t copy_count,
                                                  const VkCopyDescriptorSet *p_cds) {
    // Write updates first
    uint32_t i = 0;
    for (i = 0; i < write_count; ++i) {
        auto dest_set = p_wds[i].dstSet;
        auto set_node = core_validation::getSetNode(dev_data, dest_set);
        if (set_node) {
            set_node->PerformWriteUpdate(&p_wds[i]);
        }
    }
    // Now copy updates
    for (i = 0; i < copy_count; ++i) {
        auto dst_set = p_cds[i].dstSet;
        auto src_set = p_cds[i].srcSet;
        auto src_node = core_validation::getSetNode(dev_data, src_set);
        auto dst_node = core_validation::getSetNode(dev_data, dst_set);
        if (src_node && dst_node) {
            dst_node->PerformCopyUpdate(&p_cds[i], src_node);
        }
    }
}
// Validate the state for a given write update but don't actually perform the update
//  If an error would occur for this update, return false and fill in details in error_msg string
bool cvdescriptorset::DescriptorSet::ValidateWriteUpdate(const debug_report_data *report_data, const VkWriteDescriptorSet *update,
                                                         UNIQUE_VALIDATION_ERROR_CODE *error_code, std::string *error_msg) {
    // Verify idle ds
    if (in_use.load()) {
        // TODO : Re-using Allocate Idle error code, need write update idle error code
        *error_code = VALIDATION_ERROR_00919;
        std::stringstream error_str;
        error_str << "Cannot call vkUpdateDescriptorSets() to perform write update on descriptor set " << set_
                  << " that is in use by a command buffer.";
        *error_msg = error_str.str();
        return false;
    }
    // Verify dst binding exists
    if (!p_layout_->HasBinding(update->dstBinding)) {
        *error_code = VALIDATION_ERROR_00936;
        std::stringstream error_str;
        error_str << "DescriptorSet " << set_ << " does not have binding " << update->dstBinding << ".";
        *error_msg = error_str.str();
        return false;
    }
    // We know that binding is valid, verify update and do update on each descriptor
    auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding) + update->dstArrayElement;
    auto type = p_layout_->GetTypeFromBinding(update->dstBinding);
    if (type != update->descriptorType) {
        *error_code = VALIDATION_ERROR_00937;
        std::stringstream error_str;
        error_str << "Attempting write update to descriptor set " << set_ << " binding #" << update->dstBinding << " with type "
                  << string_VkDescriptorType(type) << " but update type is " << string_VkDescriptorType(update->descriptorType);
        *error_msg = error_str.str();
        return false;
    }
    if ((start_idx + update->descriptorCount) > p_layout_->GetTotalDescriptorCount()) {
        *error_code = VALIDATION_ERROR_00938;
        std::stringstream error_str;
        error_str << "Attempting write update to descriptor set " << set_ << " binding #" << update->dstBinding << " with "
                  << p_layout_->GetTotalDescriptorCount() << " total descriptors but update of " << update->descriptorCount
                  << " descriptors starting at binding offset of " << p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding)
                  << " combined with update array element offset of " << update->dstArrayElement
                  << " oversteps the size of this descriptor set.";
        *error_msg = error_str.str();
        return false;
    }
    // Verify consecutive bindings match (if needed)
    if (!p_layout_->VerifyUpdateConsistency(update->dstBinding, update->dstArrayElement, update->descriptorCount, "write update to",
                                            set_, error_msg)) {
        *error_code = VALIDATION_ERROR_00938;
        return false;
    }
    // Update is within bounds and consistent so last step is to validate update contents
    if (!VerifyWriteUpdateContents(update, start_idx, error_code, error_msg)) {
        std::stringstream error_str;
        error_str << "Write update to descriptor in set " << set_ << " binding #" << update->dstBinding
                  << " failed with error message: " << error_msg->c_str();
        *error_msg = error_str.str();
        return false;
    }
    // All checks passed, update is clean
    return true;
}
// For the given buffer, verify that its creation parameters are appropriate for the given type
//  If there's an error, update the error_msg string with details and return false, else return true
bool cvdescriptorset::DescriptorSet::ValidateBufferUsage(BUFFER_NODE const *buffer_node, VkDescriptorType type,
                                                         UNIQUE_VALIDATION_ERROR_CODE *error_code, std::string *error_msg) const {
    // Verify that usage bits set correctly for given type
    auto usage = buffer_node->createInfo.usage;
    std::string error_usage_bit;
    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        if (!(usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)) {
            *error_code = VALIDATION_ERROR_00950;
            error_usage_bit = "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT";
        }
        break;
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (!(usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)) {
            *error_code = VALIDATION_ERROR_00951;
            error_usage_bit = "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT";
        }
        break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        if (!(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
            *error_code = VALIDATION_ERROR_00946;
            error_usage_bit = "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT";
        }
        break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (!(usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
            *error_code = VALIDATION_ERROR_00947;
            error_usage_bit = "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT";
        }
        break;
    default:
        break;
    }
    if (!error_usage_bit.empty()) {
        std::stringstream error_str;
        error_str << "Buffer (" << buffer_node->buffer << ") with usage mask 0x" << usage
                  << " being used for a descriptor update of type " << string_VkDescriptorType(type) << " does not have "
                  << error_usage_bit << " set.";
        *error_msg = error_str.str();
        return false;
    }
    return true;
}
// For buffer descriptor updates, verify the buffer usage and VkDescriptorBufferInfo struct which includes:
//  1. buffer is valid
//  2. buffer was created with correct usage flags
//  3. offset is less than buffer size
//  4. range is either VK_WHOLE_SIZE or falls in (0, (buffer size - offset)]
// If there's an error, update the error_msg string with details and return false, else return true
bool cvdescriptorset::DescriptorSet::ValidateBufferUpdate(VkDescriptorBufferInfo const *buffer_info, VkDescriptorType type,
                                                          UNIQUE_VALIDATION_ERROR_CODE *error_code, std::string *error_msg) const {
    // TODO : Defaulting to 00962 for all cases here. Need to create new error codes for a few cases below.
    *error_code = VALIDATION_ERROR_00962;
    // First make sure that buffer is valid
    auto buffer_node = getBufferNode(device_data_, buffer_info->buffer);
    if (!buffer_node) {
        std::stringstream error_str;
        error_str << "Invalid VkBuffer: " << buffer_info->buffer;
        *error_msg = error_str.str();
        return false;
    }
    if (ValidateMemoryIsBoundToBuffer(device_data_, buffer_node, "vkUpdateDescriptorSets()")) {
        // TODO : This is a repeat code, need new code(s) for language in 11.6 Memory Association
        *error_msg = "No memory bound to buffer.";
        return false;
    }
    // Verify usage bits
    if (!ValidateBufferUsage(buffer_node, type, error_code, error_msg)) {
        // error_msg will have been updated by ValidateBufferUsage()
        return false;
    }
    // TODO : Need to also validate device limit offset requirements captured in VALIDATION_ERROR_00944,945
    // offset must be less than buffer size
    if (buffer_info->offset > buffer_node->createInfo.size) {
        *error_code = VALIDATION_ERROR_00959;
        std::stringstream error_str;
        error_str << "VkDescriptorBufferInfo offset of " << buffer_info->offset << " is greater than buffer " << buffer_node->buffer
                  << " size of " << buffer_node->createInfo.size;
        *error_msg = error_str.str();
        return false;
    }
    // TODO : Need to also validate device limit range requirements captured in VALIDATION_ERROR_00948,949
    if (buffer_info->range != VK_WHOLE_SIZE) {
        // Range must be VK_WHOLE_SIZE or > 0
        if (!buffer_info->range) {
            *error_code = VALIDATION_ERROR_00960;
            std::stringstream error_str;
            error_str << "VkDescriptorBufferInfo range is not VK_WHOLE_SIZE and is zero, which is not allowed.";
            *error_msg = error_str.str();
            return false;
        }
        // Range must be VK_WHOLE_SIZE or <= (buffer size - offset)
        if (buffer_info->range > (buffer_node->createInfo.size - buffer_info->offset)) {
            *error_code = VALIDATION_ERROR_00961;
            std::stringstream error_str;
            error_str << "VkDescriptorBufferInfo range is " << buffer_info->range << " which is greater than buffer size ("
                      << buffer_node->createInfo.size << ") minus requested offset of " << buffer_info->offset;
            *error_msg = error_str.str();
            return false;
        }
    }
    return true;
}

// Verify that the contents of the update are ok, but don't perform actual update
bool cvdescriptorset::DescriptorSet::VerifyWriteUpdateContents(const VkWriteDescriptorSet *update, const uint32_t index,
                                                               UNIQUE_VALIDATION_ERROR_CODE *error_code,
                                                               std::string *error_msg) const {
    switch (update->descriptorType) {
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            // Validate image
            auto image_view = update->pImageInfo[di].imageView;
            auto image_layout = update->pImageInfo[di].imageLayout;
            if (!ValidateImageUpdate(image_view, image_layout, update->descriptorType, device_data_, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted write update to combined image sampler descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        // Intentional fall-through to validate sampler
    }
    case VK_DESCRIPTOR_TYPE_SAMPLER: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            if (!descriptors_[index + di].get()->IsImmutableSampler()) {
                if (!ValidateSampler(update->pImageInfo[di].sampler, device_data_)) {
                    *error_code = VALIDATION_ERROR_00942;
                    std::stringstream error_str;
                    error_str << "Attempted write update to sampler descriptor with invalid sampler: "
                              << update->pImageInfo[di].sampler << ".";
                    *error_msg = error_str.str();
                    return false;
                }
            } else {
                // TODO : Warn here
            }
        }
        break;
    }
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            auto image_view = update->pImageInfo[di].imageView;
            auto image_layout = update->pImageInfo[di].imageLayout;
            if (!ValidateImageUpdate(image_view, image_layout, update->descriptorType, device_data_, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted write update to image descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        break;
    }
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            auto buffer_view = update->pTexelBufferView[di];
            auto bv_state = getBufferViewState(device_data_, buffer_view);
            if (!bv_state) {
                *error_code = VALIDATION_ERROR_00940;
                std::stringstream error_str;
                error_str << "Attempted write update to texel buffer descriptor with invalid buffer view: " << buffer_view;
                *error_msg = error_str.str();
                return false;
            }
            auto buffer = bv_state->create_info.buffer;
            if (!ValidateBufferUsage(getBufferNode(device_data_, buffer), update->descriptorType, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted write update to texel buffer descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        break;
    }
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            if (!ValidateBufferUpdate(update->pBufferInfo + di, update->descriptorType, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted write update to buffer descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        break;
    }
    default:
        assert(0); // We've already verified update type so should never get here
        break;
    }
    // All checks passed so update contents are good
    return true;
}
// Verify that the contents of the update are ok, but don't perform actual update
bool cvdescriptorset::DescriptorSet::VerifyCopyUpdateContents(const VkCopyDescriptorSet *update, const DescriptorSet *src_set,
                                                              VkDescriptorType type, uint32_t index,
                                                              UNIQUE_VALIDATION_ERROR_CODE *error_code,
                                                              std::string *error_msg) const {
    // Note : Repurposing some Write update error codes here as specific details aren't called out for copy updates like they are
    // for write updates
    switch (src_set->descriptors_[index]->descriptor_class) {
    case PlainSampler: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            if (!src_set->descriptors_[index + di]->IsImmutableSampler()) {
                auto update_sampler = static_cast<SamplerDescriptor *>(src_set->descriptors_[index + di].get())->GetSampler();
                if (!ValidateSampler(update_sampler, device_data_)) {
                    *error_code = VALIDATION_ERROR_00942;
                    std::stringstream error_str;
                    error_str << "Attempted copy update to sampler descriptor with invalid sampler: " << update_sampler << ".";
                    *error_msg = error_str.str();
                    return false;
                }
            } else {
                // TODO : Warn here
            }
        }
        break;
    }
    case ImageSampler: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            auto img_samp_desc = static_cast<const ImageSamplerDescriptor *>(src_set->descriptors_[index + di].get());
            // First validate sampler
            if (!img_samp_desc->IsImmutableSampler()) {
                auto update_sampler = img_samp_desc->GetSampler();
                if (!ValidateSampler(update_sampler, device_data_)) {
                    *error_code = VALIDATION_ERROR_00942;
                    std::stringstream error_str;
                    error_str << "Attempted copy update to sampler descriptor with invalid sampler: " << update_sampler << ".";
                    *error_msg = error_str.str();
                    return false;
                }
            } else {
                // TODO : Warn here
            }
            // Validate image
            auto image_view = img_samp_desc->GetImageView();
            auto image_layout = img_samp_desc->GetImageLayout();
            if (!ValidateImageUpdate(image_view, image_layout, type, device_data_, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted copy update to combined image sampler descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        break;
    }
    case Image: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            auto img_desc = static_cast<const ImageDescriptor *>(src_set->descriptors_[index + di].get());
            auto image_view = img_desc->GetImageView();
            auto image_layout = img_desc->GetImageLayout();
            if (!ValidateImageUpdate(image_view, image_layout, type, device_data_, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted copy update to image descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        break;
    }
    case TexelBuffer: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            auto buffer_view = static_cast<TexelDescriptor *>(src_set->descriptors_[index + di].get())->GetBufferView();
            auto bv_state = getBufferViewState(device_data_, buffer_view);
            if (!bv_state) {
                *error_code = VALIDATION_ERROR_00940;
                std::stringstream error_str;
                error_str << "Attempted copy update to texel buffer descriptor with invalid buffer view: " << buffer_view;
                *error_msg = error_str.str();
                return false;
            }
            auto buffer = bv_state->create_info.buffer;
            if (!ValidateBufferUsage(getBufferNode(device_data_, buffer), type, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted copy update to texel buffer descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        break;
    }
    case GeneralBuffer: {
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            auto buffer = static_cast<BufferDescriptor *>(src_set->descriptors_[index + di].get())->GetBuffer();
            if (!ValidateBufferUsage(getBufferNode(device_data_, buffer), type, error_code, error_msg)) {
                std::stringstream error_str;
                error_str << "Attempted copy update to buffer descriptor failed due to: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
        }
        break;
    }
    default:
        assert(0); // We've already verified update type so should never get here
        break;
    }
    // All checks passed so update contents are good
    return true;
}
// Verify that the state at allocate time is correct, but don't actually allocate the sets yet
bool cvdescriptorset::ValidateAllocateDescriptorSets(const debug_report_data *report_data,
                                                     const VkDescriptorSetAllocateInfo *p_alloc_info,
                                                     const core_validation::layer_data *dev_data,
                                                     AllocateDescriptorSetsData *ds_data) {
    bool skip_call = false;

    for (uint32_t i = 0; i < p_alloc_info->descriptorSetCount; i++) {
        auto layout = getDescriptorSetLayout(dev_data, p_alloc_info->pSetLayouts[i]);
        if (!layout) {
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT,
                        reinterpret_cast<const uint64_t &>(p_alloc_info->pSetLayouts[i]), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                        "Unable to find set layout node for layout 0x%" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call",
                        reinterpret_cast<const uint64_t &>(p_alloc_info->pSetLayouts[i]));
        } else {
            ds_data->layout_nodes[i] = layout;
            // Count total descriptors required per type
            for (uint32_t j = 0; j < layout->GetBindingCount(); ++j) {
                const auto &binding_layout = layout->GetDescriptorSetLayoutBindingPtrFromIndex(j);
                uint32_t typeIndex = static_cast<uint32_t>(binding_layout->descriptorType);
                ds_data->required_descriptors_by_type[typeIndex] += binding_layout->descriptorCount;
            }
        }
    }
    auto pool_state = getDescriptorPoolState(dev_data, p_alloc_info->descriptorPool);
    // Track number of descriptorSets allowable in this pool
    if (pool_state->availableSets < p_alloc_info->descriptorSetCount) {
        skip_call |= log_msg(
            report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
            reinterpret_cast<uint64_t &>(pool_state->pool), __LINE__, DRAWSTATE_DESCRIPTOR_POOL_EMPTY, "DS",
            "Unable to allocate %u descriptorSets from pool 0x%" PRIxLEAST64 ". This pool only has %d descriptorSets remaining.",
            p_alloc_info->descriptorSetCount, reinterpret_cast<uint64_t &>(pool_state->pool), pool_state->availableSets);
    }
    // Determine whether descriptor counts are satisfiable
    for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; i++) {
        if (ds_data->required_descriptors_by_type[i] > pool_state->availableDescriptorTypeCount[i]) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
                                 reinterpret_cast<const uint64_t &>(pool_state->pool), __LINE__, DRAWSTATE_DESCRIPTOR_POOL_EMPTY,
                                 "DS", "Unable to allocate %u descriptors of type %s from pool 0x%" PRIxLEAST64
                                       ". This pool only has %d descriptors of this type remaining.",
                                 ds_data->required_descriptors_by_type[i], string_VkDescriptorType(VkDescriptorType(i)),
                                 reinterpret_cast<uint64_t &>(pool_state->pool), pool_state->availableDescriptorTypeCount[i]);
        }
    }

    return skip_call;
}
// Decrement allocated sets from the pool and insert new sets into set_map
void cvdescriptorset::PerformAllocateDescriptorSets(const VkDescriptorSetAllocateInfo *p_alloc_info,
                                                    const VkDescriptorSet *descriptor_sets,
                                                    const AllocateDescriptorSetsData *ds_data,
                                                    std::unordered_map<VkDescriptorPool, DESCRIPTOR_POOL_STATE *> *pool_map,
                                                    std::unordered_map<VkDescriptorSet, cvdescriptorset::DescriptorSet *> *set_map,
                                                    const core_validation::layer_data *dev_data) {
    auto pool_state = (*pool_map)[p_alloc_info->descriptorPool];
    /* Account for sets and individual descriptors allocated from pool */
    pool_state->availableSets -= p_alloc_info->descriptorSetCount;
    for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; i++) {
        pool_state->availableDescriptorTypeCount[i] -= ds_data->required_descriptors_by_type[i];
    }
    /* Create tracking object for each descriptor set; insert into
     * global map and the pool's set.
     */
    for (uint32_t i = 0; i < p_alloc_info->descriptorSetCount; i++) {
        auto new_ds = new cvdescriptorset::DescriptorSet(descriptor_sets[i], p_alloc_info->descriptorPool, ds_data->layout_nodes[i],
                                                         dev_data);

        pool_state->sets.insert(new_ds);
        new_ds->in_use.store(0);
        (*set_map)[descriptor_sets[i]] = new_ds;
    }
}
