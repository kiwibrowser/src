///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2016 The Khronos Group Inc.
// Copyright (c) 2015-2016 Valve Corporation
// Copyright (c) 2015-2016 LunarG, Inc.
// Copyright (c) 2015-2016 Google, Inc.
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
///////////////////////////////////////////////////////////////////////////////

#include "vkjson.h"

#include <assert.h>
#include <string.h>

#include <cmath>
#include <cinttypes>
#include <cstdio>
#include <limits>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>

#include <cJSON.h>
#include <vulkan/vk_sdk_platform.h>

namespace {

inline bool IsIntegral(double value) {
  return std::trunc(value) == value;
}

template <typename T> struct EnumTraits;
template <> struct EnumTraits<VkPhysicalDeviceType> {
  static uint32_t min() { return VK_PHYSICAL_DEVICE_TYPE_BEGIN_RANGE; }
  static uint32_t max() { return VK_PHYSICAL_DEVICE_TYPE_END_RANGE; }
};

template <> struct EnumTraits<VkFormat> {
  static uint32_t min() { return VK_FORMAT_BEGIN_RANGE; }
  static uint32_t max() { return VK_FORMAT_END_RANGE; }
};


// VkSparseImageFormatProperties

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkExtent3D* extents) {
  return
    visitor->Visit("width", &extents->width) &&
    visitor->Visit("height", &extents->height) &&
    visitor->Visit("depth", &extents->depth);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkImageFormatProperties* properties) {
  return
    visitor->Visit("maxExtent", &properties->maxExtent) &&
    visitor->Visit("maxMipLevels", &properties->maxMipLevels) &&
    visitor->Visit("maxArrayLayers", &properties->maxArrayLayers) &&
    visitor->Visit("sampleCounts", &properties->sampleCounts) &&
    visitor->Visit("maxResourceSize", &properties->maxResourceSize);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkPhysicalDeviceLimits* limits) {
  return
    visitor->Visit("maxImageDimension1D", &limits->maxImageDimension1D) &&
    visitor->Visit("maxImageDimension2D", &limits->maxImageDimension2D) &&
    visitor->Visit("maxImageDimension3D", &limits->maxImageDimension3D) &&
    visitor->Visit("maxImageDimensionCube", &limits->maxImageDimensionCube) &&
    visitor->Visit("maxImageArrayLayers", &limits->maxImageArrayLayers) &&
    visitor->Visit("maxTexelBufferElements", &limits->maxTexelBufferElements) &&
    visitor->Visit("maxUniformBufferRange", &limits->maxUniformBufferRange) &&
    visitor->Visit("maxStorageBufferRange", &limits->maxStorageBufferRange) &&
    visitor->Visit("maxPushConstantsSize", &limits->maxPushConstantsSize) &&
    visitor->Visit("maxMemoryAllocationCount", &limits->maxMemoryAllocationCount) &&
    visitor->Visit("maxSamplerAllocationCount", &limits->maxSamplerAllocationCount) &&
    visitor->Visit("bufferImageGranularity", &limits->bufferImageGranularity) &&
    visitor->Visit("sparseAddressSpaceSize", &limits->sparseAddressSpaceSize) &&
    visitor->Visit("maxBoundDescriptorSets", &limits->maxBoundDescriptorSets) &&
    visitor->Visit("maxPerStageDescriptorSamplers", &limits->maxPerStageDescriptorSamplers) &&
    visitor->Visit("maxPerStageDescriptorUniformBuffers", &limits->maxPerStageDescriptorUniformBuffers) &&
    visitor->Visit("maxPerStageDescriptorStorageBuffers", &limits->maxPerStageDescriptorStorageBuffers) &&
    visitor->Visit("maxPerStageDescriptorSampledImages", &limits->maxPerStageDescriptorSampledImages) &&
    visitor->Visit("maxPerStageDescriptorStorageImages", &limits->maxPerStageDescriptorStorageImages) &&
    visitor->Visit("maxPerStageDescriptorInputAttachments", &limits->maxPerStageDescriptorInputAttachments) &&
    visitor->Visit("maxPerStageResources", &limits->maxPerStageResources) &&
    visitor->Visit("maxDescriptorSetSamplers", &limits->maxDescriptorSetSamplers) &&
    visitor->Visit("maxDescriptorSetUniformBuffers", &limits->maxDescriptorSetUniformBuffers) &&
    visitor->Visit("maxDescriptorSetUniformBuffersDynamic", &limits->maxDescriptorSetUniformBuffersDynamic) &&
    visitor->Visit("maxDescriptorSetStorageBuffers", &limits->maxDescriptorSetStorageBuffers) &&
    visitor->Visit("maxDescriptorSetStorageBuffersDynamic", &limits->maxDescriptorSetStorageBuffersDynamic) &&
    visitor->Visit("maxDescriptorSetSampledImages", &limits->maxDescriptorSetSampledImages) &&
    visitor->Visit("maxDescriptorSetStorageImages", &limits->maxDescriptorSetStorageImages) &&
    visitor->Visit("maxDescriptorSetInputAttachments", &limits->maxDescriptorSetInputAttachments) &&
    visitor->Visit("maxVertexInputAttributes", &limits->maxVertexInputAttributes) &&
    visitor->Visit("maxVertexInputBindings", &limits->maxVertexInputBindings) &&
    visitor->Visit("maxVertexInputAttributeOffset", &limits->maxVertexInputAttributeOffset) &&
    visitor->Visit("maxVertexInputBindingStride", &limits->maxVertexInputBindingStride) &&
    visitor->Visit("maxVertexOutputComponents", &limits->maxVertexOutputComponents) &&
    visitor->Visit("maxTessellationGenerationLevel", &limits->maxTessellationGenerationLevel) &&
    visitor->Visit("maxTessellationPatchSize", &limits->maxTessellationPatchSize) &&
    visitor->Visit("maxTessellationControlPerVertexInputComponents", &limits->maxTessellationControlPerVertexInputComponents) &&
    visitor->Visit("maxTessellationControlPerVertexOutputComponents", &limits->maxTessellationControlPerVertexOutputComponents) &&
    visitor->Visit("maxTessellationControlPerPatchOutputComponents", &limits->maxTessellationControlPerPatchOutputComponents) &&
    visitor->Visit("maxTessellationControlTotalOutputComponents", &limits->maxTessellationControlTotalOutputComponents) &&
    visitor->Visit("maxTessellationEvaluationInputComponents", &limits->maxTessellationEvaluationInputComponents) &&
    visitor->Visit("maxTessellationEvaluationOutputComponents", &limits->maxTessellationEvaluationOutputComponents) &&
    visitor->Visit("maxGeometryShaderInvocations", &limits->maxGeometryShaderInvocations) &&
    visitor->Visit("maxGeometryInputComponents", &limits->maxGeometryInputComponents) &&
    visitor->Visit("maxGeometryOutputComponents", &limits->maxGeometryOutputComponents) &&
    visitor->Visit("maxGeometryOutputVertices", &limits->maxGeometryOutputVertices) &&
    visitor->Visit("maxGeometryTotalOutputComponents", &limits->maxGeometryTotalOutputComponents) &&
    visitor->Visit("maxFragmentInputComponents", &limits->maxFragmentInputComponents) &&
    visitor->Visit("maxFragmentOutputAttachments", &limits->maxFragmentOutputAttachments) &&
    visitor->Visit("maxFragmentDualSrcAttachments", &limits->maxFragmentDualSrcAttachments) &&
    visitor->Visit("maxFragmentCombinedOutputResources", &limits->maxFragmentCombinedOutputResources) &&
    visitor->Visit("maxComputeSharedMemorySize", &limits->maxComputeSharedMemorySize) &&
    visitor->Visit("maxComputeWorkGroupCount", &limits->maxComputeWorkGroupCount) &&
    visitor->Visit("maxComputeWorkGroupInvocations", &limits->maxComputeWorkGroupInvocations) &&
    visitor->Visit("maxComputeWorkGroupSize", &limits->maxComputeWorkGroupSize) &&
    visitor->Visit("subPixelPrecisionBits", &limits->subPixelPrecisionBits) &&
    visitor->Visit("subTexelPrecisionBits", &limits->subTexelPrecisionBits) &&
    visitor->Visit("mipmapPrecisionBits", &limits->mipmapPrecisionBits) &&
    visitor->Visit("maxDrawIndexedIndexValue", &limits->maxDrawIndexedIndexValue) &&
    visitor->Visit("maxDrawIndirectCount", &limits->maxDrawIndirectCount) &&
    visitor->Visit("maxSamplerLodBias", &limits->maxSamplerLodBias) &&
    visitor->Visit("maxSamplerAnisotropy", &limits->maxSamplerAnisotropy) &&
    visitor->Visit("maxViewports", &limits->maxViewports) &&
    visitor->Visit("maxViewportDimensions", &limits->maxViewportDimensions) &&
    visitor->Visit("viewportBoundsRange", &limits->viewportBoundsRange) &&
    visitor->Visit("viewportSubPixelBits", &limits->viewportSubPixelBits) &&
    visitor->Visit("minMemoryMapAlignment", &limits->minMemoryMapAlignment) &&
    visitor->Visit("minTexelBufferOffsetAlignment", &limits->minTexelBufferOffsetAlignment) &&
    visitor->Visit("minUniformBufferOffsetAlignment", &limits->minUniformBufferOffsetAlignment) &&
    visitor->Visit("minStorageBufferOffsetAlignment", &limits->minStorageBufferOffsetAlignment) &&
    visitor->Visit("minTexelOffset", &limits->minTexelOffset) &&
    visitor->Visit("maxTexelOffset", &limits->maxTexelOffset) &&
    visitor->Visit("minTexelGatherOffset", &limits->minTexelGatherOffset) &&
    visitor->Visit("maxTexelGatherOffset", &limits->maxTexelGatherOffset) &&
    visitor->Visit("minInterpolationOffset", &limits->minInterpolationOffset) &&
    visitor->Visit("maxInterpolationOffset", &limits->maxInterpolationOffset) &&
    visitor->Visit("subPixelInterpolationOffsetBits", &limits->subPixelInterpolationOffsetBits) &&
    visitor->Visit("maxFramebufferWidth", &limits->maxFramebufferWidth) &&
    visitor->Visit("maxFramebufferHeight", &limits->maxFramebufferHeight) &&
    visitor->Visit("maxFramebufferLayers", &limits->maxFramebufferLayers) &&
    visitor->Visit("framebufferColorSampleCounts", &limits->framebufferColorSampleCounts) &&
    visitor->Visit("framebufferDepthSampleCounts", &limits->framebufferDepthSampleCounts) &&
    visitor->Visit("framebufferStencilSampleCounts", &limits->framebufferStencilSampleCounts) &&
    visitor->Visit("framebufferNoAttachmentsSampleCounts", &limits->framebufferNoAttachmentsSampleCounts) &&
    visitor->Visit("maxColorAttachments", &limits->maxColorAttachments) &&
    visitor->Visit("sampledImageColorSampleCounts", &limits->sampledImageColorSampleCounts) &&
    visitor->Visit("sampledImageIntegerSampleCounts", &limits->sampledImageIntegerSampleCounts) &&
    visitor->Visit("sampledImageDepthSampleCounts", &limits->sampledImageDepthSampleCounts) &&
    visitor->Visit("sampledImageStencilSampleCounts", &limits->sampledImageStencilSampleCounts) &&
    visitor->Visit("storageImageSampleCounts", &limits->storageImageSampleCounts) &&
    visitor->Visit("maxSampleMaskWords", &limits->maxSampleMaskWords) &&
    visitor->Visit("timestampComputeAndGraphics", &limits->timestampComputeAndGraphics) &&
    visitor->Visit("timestampPeriod", &limits->timestampPeriod) &&
    visitor->Visit("maxClipDistances", &limits->maxClipDistances) &&
    visitor->Visit("maxCullDistances", &limits->maxCullDistances) &&
    visitor->Visit("maxCombinedClipAndCullDistances", &limits->maxCombinedClipAndCullDistances) &&
    visitor->Visit("discreteQueuePriorities", &limits->discreteQueuePriorities) &&
    visitor->Visit("pointSizeRange", &limits->pointSizeRange) &&
    visitor->Visit("lineWidthRange", &limits->lineWidthRange) &&
    visitor->Visit("pointSizeGranularity", &limits->pointSizeGranularity) &&
    visitor->Visit("lineWidthGranularity", &limits->lineWidthGranularity) &&
    visitor->Visit("strictLines", &limits->strictLines) &&
    visitor->Visit("standardSampleLocations", &limits->standardSampleLocations) &&
    visitor->Visit("optimalBufferCopyOffsetAlignment", &limits->optimalBufferCopyOffsetAlignment) &&
    visitor->Visit("optimalBufferCopyRowPitchAlignment", &limits->optimalBufferCopyRowPitchAlignment) &&
    visitor->Visit("nonCoherentAtomSize", &limits->nonCoherentAtomSize);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor,
                    VkPhysicalDeviceSparseProperties* properties) {
  return
    visitor->Visit("residencyStandard2DBlockShape", &properties->residencyStandard2DBlockShape) &&
    visitor->Visit("residencyStandard2DMultisampleBlockShape", &properties->residencyStandard2DMultisampleBlockShape) &&
    visitor->Visit("residencyStandard3DBlockShape", &properties->residencyStandard3DBlockShape) &&
    visitor->Visit("residencyAlignedMipSize", &properties->residencyAlignedMipSize) &&
    visitor->Visit("residencyNonResidentStrict", &properties->residencyNonResidentStrict);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor,
                    VkPhysicalDeviceProperties* properties) {
  return
    visitor->Visit("apiVersion", &properties->apiVersion) &&
    visitor->Visit("driverVersion", &properties->driverVersion) &&
    visitor->Visit("vendorID", &properties->vendorID) &&
    visitor->Visit("deviceID", &properties->deviceID) &&
    visitor->Visit("deviceType", &properties->deviceType) &&
    visitor->Visit("deviceName", &properties->deviceName) &&
    visitor->Visit("pipelineCacheUUID", &properties->pipelineCacheUUID) &&
    visitor->Visit("limits", &properties->limits) &&
    visitor->Visit("sparseProperties", &properties->sparseProperties);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkPhysicalDeviceFeatures* features) {
  return
    visitor->Visit("robustBufferAccess", &features->robustBufferAccess) &&
    visitor->Visit("fullDrawIndexUint32", &features->fullDrawIndexUint32) &&
    visitor->Visit("imageCubeArray", &features->imageCubeArray) &&
    visitor->Visit("independentBlend", &features->independentBlend) &&
    visitor->Visit("geometryShader", &features->geometryShader) &&
    visitor->Visit("tessellationShader", &features->tessellationShader) &&
    visitor->Visit("sampleRateShading", &features->sampleRateShading) &&
    visitor->Visit("dualSrcBlend", &features->dualSrcBlend) &&
    visitor->Visit("logicOp", &features->logicOp) &&
    visitor->Visit("multiDrawIndirect", &features->multiDrawIndirect) &&
    visitor->Visit("drawIndirectFirstInstance", &features->drawIndirectFirstInstance) &&
    visitor->Visit("depthClamp", &features->depthClamp) &&
    visitor->Visit("depthBiasClamp", &features->depthBiasClamp) &&
    visitor->Visit("fillModeNonSolid", &features->fillModeNonSolid) &&
    visitor->Visit("depthBounds", &features->depthBounds) &&
    visitor->Visit("wideLines", &features->wideLines) &&
    visitor->Visit("largePoints", &features->largePoints) &&
    visitor->Visit("alphaToOne", &features->alphaToOne) &&
    visitor->Visit("multiViewport", &features->multiViewport) &&
    visitor->Visit("samplerAnisotropy", &features->samplerAnisotropy) &&
    visitor->Visit("textureCompressionETC2", &features->textureCompressionETC2) &&
    visitor->Visit("textureCompressionASTC_LDR", &features->textureCompressionASTC_LDR) &&
    visitor->Visit("textureCompressionBC", &features->textureCompressionBC) &&
    visitor->Visit("occlusionQueryPrecise", &features->occlusionQueryPrecise) &&
    visitor->Visit("pipelineStatisticsQuery", &features->pipelineStatisticsQuery) &&
    visitor->Visit("vertexPipelineStoresAndAtomics", &features->vertexPipelineStoresAndAtomics) &&
    visitor->Visit("fragmentStoresAndAtomics", &features->fragmentStoresAndAtomics) &&
    visitor->Visit("shaderTessellationAndGeometryPointSize", &features->shaderTessellationAndGeometryPointSize) &&
    visitor->Visit("shaderImageGatherExtended", &features->shaderImageGatherExtended) &&
    visitor->Visit("shaderStorageImageExtendedFormats", &features->shaderStorageImageExtendedFormats) &&
    visitor->Visit("shaderStorageImageMultisample", &features->shaderStorageImageMultisample) &&
    visitor->Visit("shaderStorageImageReadWithoutFormat", &features->shaderStorageImageReadWithoutFormat) &&
    visitor->Visit("shaderStorageImageWriteWithoutFormat", &features->shaderStorageImageWriteWithoutFormat) &&
    visitor->Visit("shaderUniformBufferArrayDynamicIndexing", &features->shaderUniformBufferArrayDynamicIndexing) &&
    visitor->Visit("shaderSampledImageArrayDynamicIndexing", &features->shaderSampledImageArrayDynamicIndexing) &&
    visitor->Visit("shaderStorageBufferArrayDynamicIndexing", &features->shaderStorageBufferArrayDynamicIndexing) &&
    visitor->Visit("shaderStorageImageArrayDynamicIndexing", &features->shaderStorageImageArrayDynamicIndexing) &&
    visitor->Visit("shaderClipDistance", &features->shaderClipDistance) &&
    visitor->Visit("shaderCullDistance", &features->shaderCullDistance) &&
    visitor->Visit("shaderFloat64", &features->shaderFloat64) &&
    visitor->Visit("shaderInt64", &features->shaderInt64) &&
    visitor->Visit("shaderInt16", &features->shaderInt16) &&
    visitor->Visit("shaderResourceResidency", &features->shaderResourceResidency) &&
    visitor->Visit("shaderResourceMinLod", &features->shaderResourceMinLod) &&
    visitor->Visit("sparseBinding", &features->sparseBinding) &&
    visitor->Visit("sparseResidencyBuffer", &features->sparseResidencyBuffer) &&
    visitor->Visit("sparseResidencyImage2D", &features->sparseResidencyImage2D) &&
    visitor->Visit("sparseResidencyImage3D", &features->sparseResidencyImage3D) &&
    visitor->Visit("sparseResidency2Samples", &features->sparseResidency2Samples) &&
    visitor->Visit("sparseResidency4Samples", &features->sparseResidency4Samples) &&
    visitor->Visit("sparseResidency8Samples", &features->sparseResidency8Samples) &&
    visitor->Visit("sparseResidency16Samples", &features->sparseResidency16Samples) &&
    visitor->Visit("sparseResidencyAliased", &features->sparseResidencyAliased) &&
    visitor->Visit("variableMultisampleRate", &features->variableMultisampleRate) &&
    visitor->Visit("inheritedQueries", &features->inheritedQueries);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkMemoryType* type) {
  return
    visitor->Visit("propertyFlags", &type->propertyFlags) &&
    visitor->Visit("heapIndex", &type->heapIndex);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkMemoryHeap* heap) {
  return
    visitor->Visit("size", &heap->size) &&
    visitor->Visit("flags", &heap->flags);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkPhysicalDeviceMemoryProperties* properties) {
  return
    visitor->Visit("memoryTypeCount", &properties->memoryTypeCount) &&
    visitor->VisitArray("memoryTypes", properties->memoryTypeCount, &properties->memoryTypes) &&
    visitor->Visit("memoryHeapCount", &properties->memoryHeapCount) &&
    visitor->VisitArray("memoryHeaps", properties->memoryHeapCount, &properties->memoryHeaps);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkQueueFamilyProperties* properties) {
  return
    visitor->Visit("queueFlags", &properties->queueFlags) &&
    visitor->Visit("queueCount", &properties->queueCount) &&
    visitor->Visit("timestampValidBits", &properties->timestampValidBits) &&
    visitor->Visit("minImageTransferGranularity", &properties->minImageTransferGranularity);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkExtensionProperties* properties) {
  return
    visitor->Visit("extensionName", &properties->extensionName) &&
    visitor->Visit("specVersion", &properties->specVersion);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkLayerProperties* properties) {
  return
    visitor->Visit("layerName", &properties->layerName) &&
    visitor->Visit("specVersion", &properties->specVersion) &&
    visitor->Visit("implementationVersion", &properties->implementationVersion) &&
    visitor->Visit("description", &properties->description);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkFormatProperties* properties) {
  return
    visitor->Visit("linearTilingFeatures", &properties->linearTilingFeatures) &&
    visitor->Visit("optimalTilingFeatures", &properties->optimalTilingFeatures) &&
    visitor->Visit("bufferFeatures", &properties->bufferFeatures);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkJsonLayer* layer) {
  return visitor->Visit("properties", &layer->properties) &&
         visitor->Visit("extensions", &layer->extensions);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkJsonDevice* device) {
  return visitor->Visit("properties", &device->properties) &&
         visitor->Visit("features", &device->features) &&
         visitor->Visit("memory", &device->memory) &&
         visitor->Visit("queues", &device->queues) &&
         visitor->Visit("extensions", &device->extensions) &&
         visitor->Visit("layers", &device->layers) &&
         visitor->Visit("formats", &device->formats);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkJsonInstance* instance) {
  return visitor->Visit("layers", &instance->layers) &&
         visitor->Visit("extensions", &instance->extensions) &&
         visitor->Visit("devices", &instance->devices);
}

template <typename T>
using EnableForArithmetic =
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type;

template <typename T>
using EnableForStruct =
    typename std::enable_if<std::is_class<T>::value, void>::type;

template <typename T>
using EnableForEnum =
    typename std::enable_if<std::is_enum<T>::value, void>::type;

template <typename T, typename = EnableForStruct<T>, typename = void>
cJSON* ToJsonValue(const T& value);

template <typename T, typename = EnableForArithmetic<T>>
inline cJSON* ToJsonValue(const T& value) {
  return cJSON_CreateNumber(static_cast<double>(value));
}

inline cJSON* ToJsonValue(const uint64_t& value) {
  char string[19] = {0};  // "0x" + 16 digits + terminal \0
  snprintf(string, sizeof(string), "0x%016" PRIx64, value);
  return cJSON_CreateString(string);
}

template <typename T, typename = EnableForEnum<T>, typename = void,
          typename = void>
inline cJSON* ToJsonValue(const T& value) {
  return cJSON_CreateNumber(static_cast<double>(value));
}

template <typename T>
inline cJSON* ArrayToJsonValue(uint32_t count, const T* values) {
  cJSON* array = cJSON_CreateArray();
  for (unsigned int i = 0; i < count; ++i)
    cJSON_AddItemToArray(array, ToJsonValue(values[i]));
  return array;
}

template <typename T, unsigned int N>
inline cJSON* ToJsonValue(const T (&value)[N]) {
  return ArrayToJsonValue(N, value);
}

template <size_t N>
inline cJSON* ToJsonValue(const char (&value)[N]) {
  assert(strlen(value) < N);
  return cJSON_CreateString(value);
}

template <typename T>
inline cJSON* ToJsonValue(const std::vector<T>& value) {
  assert(value.size() <= std::numeric_limits<uint32_t>::max());
  return ArrayToJsonValue(static_cast<uint32_t>(value.size()), value.data());
}

template <typename F, typename S>
inline cJSON* ToJsonValue(const std::pair<F, S>& value) {
  cJSON* array = cJSON_CreateArray();
  cJSON_AddItemToArray(array, ToJsonValue(value.first));
  cJSON_AddItemToArray(array, ToJsonValue(value.second));
  return array;
}

template <typename F, typename S>
inline cJSON* ToJsonValue(const std::map<F, S>& value) {
  cJSON* array = cJSON_CreateArray();
  for (auto& kv : value)
    cJSON_AddItemToArray(array, ToJsonValue(kv));
  return array;
}

class JsonWriterVisitor {
 public:
  JsonWriterVisitor() : object_(cJSON_CreateObject()) {}

  ~JsonWriterVisitor() {
    if (object_)
      cJSON_Delete(object_);
  }

  template <typename T> bool Visit(const char* key, const T* value) {
    cJSON_AddItemToObjectCS(object_, key, ToJsonValue(*value));
    return true;
  }

  template <typename T, uint32_t N>
  bool VisitArray(const char* key, uint32_t count, const T (*value)[N]) {
    assert(count <= N);
    cJSON_AddItemToObjectCS(object_, key, ArrayToJsonValue(count, *value));
    return true;
  }

  cJSON* get_object() const { return object_; }
  cJSON* take_object() {
    cJSON* object = object_;
    object_ = nullptr;
    return object;
  }

 private:
  cJSON* object_;
};

template <typename Visitor, typename T>
inline void VisitForWrite(Visitor* visitor, const T& t) {
  Iterate(visitor, const_cast<T*>(&t));
}

template <typename T, typename /*= EnableForStruct<T>*/, typename /*= void*/>
cJSON* ToJsonValue(const T& value) {
  JsonWriterVisitor visitor;
  VisitForWrite(&visitor, value);
  return visitor.take_object();
}

template <typename T, typename = EnableForStruct<T>>
bool AsValue(cJSON* json_value, T* t);

inline bool AsValue(cJSON* json_value, int32_t* value) {
  double d = json_value->valuedouble;
  if (json_value->type != cJSON_Number || !IsIntegral(d) ||
      d < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
      d > static_cast<double>(std::numeric_limits<int32_t>::max()))
    return false;
  *value = static_cast<int32_t>(d);
  return true;
}

inline bool AsValue(cJSON* json_value, uint64_t* value) {
  if (json_value->type != cJSON_String)
    return false;
  int result = std::sscanf(json_value->valuestring, "0x%016" PRIx64, value);
  return result == 1;
}

inline bool AsValue(cJSON* json_value, uint32_t* value) {
  double d = json_value->valuedouble;
  if (json_value->type != cJSON_Number || !IsIntegral(d) ||
      d < 0.0 || d > static_cast<double>(std::numeric_limits<uint32_t>::max()))
    return false;
  *value = static_cast<uint32_t>(d);
  return true;
}

inline bool AsValue(cJSON* json_value, uint8_t* value) {
  uint32_t value32 = 0;
  AsValue(json_value, &value32);
  if (value32 > std::numeric_limits<uint8_t>::max())
    return false;
  *value = static_cast<uint8_t>(value32);
  return true;
}

inline bool AsValue(cJSON* json_value, float* value) {
  if (json_value->type != cJSON_Number)
    return false;
  *value = static_cast<float>(json_value->valuedouble);
  return true;
}

template <typename T>
inline bool AsArray(cJSON* json_value, uint32_t count, T* values) {
  if (json_value->type != cJSON_Array ||
      cJSON_GetArraySize(json_value) != count)
    return false;
  for (uint32_t i = 0; i < count; ++i) {
    if (!AsValue(cJSON_GetArrayItem(json_value, i), values + i))
      return false;
  }
  return true;
}

template <typename T, unsigned int N>
inline bool AsValue(cJSON* json_value, T (*value)[N]) {
  return AsArray(json_value, N, *value);
}

template <size_t N>
inline bool AsValue(cJSON* json_value, char (*value)[N]) {
  if (json_value->type != cJSON_String)
    return false;
  size_t len = strlen(json_value->valuestring);
  if (len >= N)
    return false;
  memcpy(*value, json_value->valuestring, len);
  memset(*value + len, 0, N-len);
  return true;
}

template <typename T, typename = EnableForEnum<T>, typename = void>
inline bool AsValue(cJSON* json_value, T* t) {
  // TODO(piman): to/from strings instead?
  uint32_t value = 0;
  if (!AsValue(json_value, &value))
      return false;
  if (value < EnumTraits<T>::min() || value > EnumTraits<T>::max())
    return false;
  *t = static_cast<T>(value);
  return true;
}

template <typename T>
inline bool AsValue(cJSON* json_value, std::vector<T>* value) {
  if (json_value->type != cJSON_Array)
    return false;
  int size = cJSON_GetArraySize(json_value);
  value->resize(size);
  return AsArray(json_value, size, value->data());
}

template <typename F, typename S>
inline bool AsValue(cJSON* json_value, std::pair<F, S>* value) {
  if (json_value->type != cJSON_Array || cJSON_GetArraySize(json_value) != 2)
    return false;
  return AsValue(cJSON_GetArrayItem(json_value, 0), &value->first) &&
         AsValue(cJSON_GetArrayItem(json_value, 1), &value->second);
}

template <typename F, typename S>
inline bool AsValue(cJSON* json_value, std::map<F, S>* value) {
  if (json_value->type != cJSON_Array)
    return false;
  int size = cJSON_GetArraySize(json_value);
  for (int i = 0; i < size; ++i) {
    std::pair<F, S> elem;
    if (!AsValue(cJSON_GetArrayItem(json_value, i), &elem))
      return false;
    if (!value->insert(elem).second)
      return false;
  }
  return true;
}

template <typename T>
bool ReadValue(cJSON* object, const char* key, T* value,
               std::string* errors) {
  cJSON* json_value = cJSON_GetObjectItem(object, key);
  if (!json_value) {
    if (errors)
      *errors = std::string(key) + " missing.";
    return false;
  }
  if (AsValue(json_value, value))
    return true;
  if (errors)
    *errors = std::string("Wrong type for ") + std::string(key) + ".";
  return false;
}

template <typename Visitor, typename T>
inline bool VisitForRead(Visitor* visitor, T* t) {
  return Iterate(visitor, t);
}

class JsonReaderVisitor {
 public:
  JsonReaderVisitor(cJSON* object, std::string* errors)
      : object_(object), errors_(errors) {}

  template <typename T> bool Visit(const char* key, T* value) const {
    return ReadValue(object_, key, value, errors_);
  }

  template <typename T, uint32_t N>
  bool VisitArray(const char* key, uint32_t count, T (*value)[N]) {
    if (count > N)
      return false;
    cJSON* json_value = cJSON_GetObjectItem(object_, key);
    if (!json_value) {
      if (errors_)
        *errors_ = std::string(key) + " missing.";
      return false;
    }
    if (AsArray(json_value, count, *value))
      return true;
    if (errors_)
      *errors_ = std::string("Wrong type for ") + std::string(key) + ".";
    return false;
  }


 private:
  cJSON* object_;
  std::string* errors_;
};

template <typename T, typename /*= EnableForStruct<T>*/>
bool AsValue(cJSON* json_value, T* t) {
  if (json_value->type != cJSON_Object)
    return false;
  JsonReaderVisitor visitor(json_value, nullptr);
  return VisitForRead(&visitor, t);
}


template <typename T> std::string VkTypeToJson(const T& t) {
  JsonWriterVisitor visitor;
  VisitForWrite(&visitor, t);

  char* output = cJSON_Print(visitor.get_object());
  std::string result(output);
  free(output);
  return result;
}

template <typename T> bool VkTypeFromJson(const std::string& json,
                                          T* t,
                                          std::string* errors) {
  *t = T();
  cJSON* object = cJSON_Parse(json.c_str());
  if (!object) {
    if (errors)
      errors->assign(cJSON_GetErrorPtr());
    return false;
  }
  bool result = AsValue(object, t);
  cJSON_Delete(object);
  return result;
}

}  // anonymous namespace

std::string VkJsonInstanceToJson(const VkJsonInstance& instance) {
  return VkTypeToJson(instance);
}

bool VkJsonInstanceFromJson(const std::string& json,
                            VkJsonInstance* instance,
                            std::string* errors) {
  return VkTypeFromJson(json, instance, errors);
}

std::string VkJsonDeviceToJson(const VkJsonDevice& device) {
  return VkTypeToJson(device);
}

bool VkJsonDeviceFromJson(const std::string& json,
                          VkJsonDevice* device,
                          std::string* errors) {
  return VkTypeFromJson(json, device, errors);
};

std::string VkJsonImageFormatPropertiesToJson(
    const VkImageFormatProperties& properties) {
  return VkTypeToJson(properties);
}

bool VkJsonImageFormatPropertiesFromJson(const std::string& json,
                                         VkImageFormatProperties* properties,
                                         std::string* errors) {
  return VkTypeFromJson(json, properties, errors);
};
