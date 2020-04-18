/* WARNING: This is auto-generated file. Do not modify, since changes will
 * be lost! Modify the generating script instead.
 */

void InstanceDriver::destroyInstance (VkInstance instance, const VkAllocationCallbacks* pAllocator) const
{
	m_vk.destroyInstance(instance, pAllocator);
}

VkResult InstanceDriver::enumeratePhysicalDevices (VkInstance instance, deUint32* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) const
{
	return m_vk.enumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

void InstanceDriver::getPhysicalDeviceFeatures (VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) const
{
	m_vk.getPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void InstanceDriver::getPhysicalDeviceFormatProperties (VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties) const
{
	m_vk.getPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

VkResult InstanceDriver::getPhysicalDeviceImageFormatProperties (VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) const
{
	return m_vk.getPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

void InstanceDriver::getPhysicalDeviceProperties (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) const
{
	m_vk.getPhysicalDeviceProperties(physicalDevice, pProperties);
}

void InstanceDriver::getPhysicalDeviceQueueFamilyProperties (VkPhysicalDevice physicalDevice, deUint32* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) const
{
	m_vk.getPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void InstanceDriver::getPhysicalDeviceMemoryProperties (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) const
{
	m_vk.getPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

PFN_vkVoidFunction InstanceDriver::getDeviceProcAddr (VkDevice device, const char* pName) const
{
	return m_vk.getDeviceProcAddr(device, pName);
}

VkResult InstanceDriver::createDevice (VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) const
{
	return m_vk.createDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VkResult InstanceDriver::enumerateDeviceExtensionProperties (VkPhysicalDevice physicalDevice, const char* pLayerName, deUint32* pPropertyCount, VkExtensionProperties* pProperties) const
{
	return m_vk.enumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult InstanceDriver::enumerateDeviceLayerProperties (VkPhysicalDevice physicalDevice, deUint32* pPropertyCount, VkLayerProperties* pProperties) const
{
	return m_vk.enumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
}

void InstanceDriver::getPhysicalDeviceSparseImageFormatProperties (VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, deUint32* pPropertyCount, VkSparseImageFormatProperties* pProperties) const
{
	m_vk.getPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

void InstanceDriver::destroySurfaceKHR (VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator) const
{
	m_vk.destroySurfaceKHR(instance, surface, pAllocator);
}

VkResult InstanceDriver::getPhysicalDeviceSurfaceSupportKHR (VkPhysicalDevice physicalDevice, deUint32 queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported) const
{
	return m_vk.getPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult InstanceDriver::getPhysicalDeviceSurfaceCapabilitiesKHR (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) const
{
	return m_vk.getPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}

VkResult InstanceDriver::getPhysicalDeviceSurfaceFormatsKHR (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, deUint32* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) const
{
	return m_vk.getPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult InstanceDriver::getPhysicalDeviceSurfacePresentModesKHR (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, deUint32* pPresentModeCount, VkPresentModeKHR* pPresentModes) const
{
	return m_vk.getPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}

VkResult InstanceDriver::getPhysicalDeviceDisplayPropertiesKHR (VkPhysicalDevice physicalDevice, deUint32* pPropertyCount, VkDisplayPropertiesKHR* pProperties) const
{
	return m_vk.getPhysicalDeviceDisplayPropertiesKHR(physicalDevice, pPropertyCount, pProperties);
}

VkResult InstanceDriver::getPhysicalDeviceDisplayPlanePropertiesKHR (VkPhysicalDevice physicalDevice, deUint32* pPropertyCount, VkDisplayPlanePropertiesKHR* pProperties) const
{
	return m_vk.getPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, pPropertyCount, pProperties);
}

VkResult InstanceDriver::getDisplayPlaneSupportedDisplaysKHR (VkPhysicalDevice physicalDevice, deUint32 planeIndex, deUint32* pDisplayCount, VkDisplayKHR* pDisplays) const
{
	return m_vk.getDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, pDisplayCount, pDisplays);
}

VkResult InstanceDriver::getDisplayModePropertiesKHR (VkPhysicalDevice physicalDevice, VkDisplayKHR display, deUint32* pPropertyCount, VkDisplayModePropertiesKHR* pProperties) const
{
	return m_vk.getDisplayModePropertiesKHR(physicalDevice, display, pPropertyCount, pProperties);
}

VkResult InstanceDriver::createDisplayModeKHR (VkPhysicalDevice physicalDevice, VkDisplayKHR display, const VkDisplayModeCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDisplayModeKHR* pMode) const
{
	return m_vk.createDisplayModeKHR(physicalDevice, display, pCreateInfo, pAllocator, pMode);
}

VkResult InstanceDriver::getDisplayPlaneCapabilitiesKHR (VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode, deUint32 planeIndex, VkDisplayPlaneCapabilitiesKHR* pCapabilities) const
{
	return m_vk.getDisplayPlaneCapabilitiesKHR(physicalDevice, mode, planeIndex, pCapabilities);
}

VkResult InstanceDriver::createDisplayPlaneSurfaceKHR (VkInstance instance, const VkDisplaySurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const
{
	return m_vk.createDisplayPlaneSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkResult InstanceDriver::createXlibSurfaceKHR (VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const
{
	return m_vk.createXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 InstanceDriver::getPhysicalDeviceXlibPresentationSupportKHR (VkPhysicalDevice physicalDevice, deUint32 queueFamilyIndex, pt::XlibDisplayPtr dpy, pt::XlibVisualID visualID) const
{
	return m_vk.getPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID);
}

VkResult InstanceDriver::createXcbSurfaceKHR (VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const
{
	return m_vk.createXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 InstanceDriver::getPhysicalDeviceXcbPresentationSupportKHR (VkPhysicalDevice physicalDevice, deUint32 queueFamilyIndex, pt::XcbConnectionPtr connection, pt::XcbVisualid visual_id) const
{
	return m_vk.getPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
}

VkResult InstanceDriver::createWaylandSurfaceKHR (VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const
{
	return m_vk.createWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 InstanceDriver::getPhysicalDeviceWaylandPresentationSupportKHR (VkPhysicalDevice physicalDevice, deUint32 queueFamilyIndex, pt::WaylandDisplayPtr display) const
{
	return m_vk.getPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex, display);
}

VkResult InstanceDriver::createMirSurfaceKHR (VkInstance instance, const VkMirSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const
{
	return m_vk.createMirSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 InstanceDriver::getPhysicalDeviceMirPresentationSupportKHR (VkPhysicalDevice physicalDevice, deUint32 queueFamilyIndex, pt::MirConnectionPtr connection) const
{
	return m_vk.getPhysicalDeviceMirPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection);
}

VkResult InstanceDriver::createAndroidSurfaceKHR (VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const
{
	return m_vk.createAndroidSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkResult InstanceDriver::createWin32SurfaceKHR (VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const
{
	return m_vk.createWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 InstanceDriver::getPhysicalDeviceWin32PresentationSupportKHR (VkPhysicalDevice physicalDevice, deUint32 queueFamilyIndex) const
{
	return m_vk.getPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
}

void InstanceDriver::getPhysicalDeviceFeatures2KHR (VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR* pFeatures) const
{
	m_vk.getPhysicalDeviceFeatures2KHR(physicalDevice, pFeatures);
}

void InstanceDriver::getPhysicalDeviceProperties2KHR (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2KHR* pProperties) const
{
	m_vk.getPhysicalDeviceProperties2KHR(physicalDevice, pProperties);
}

void InstanceDriver::getPhysicalDeviceFormatProperties2KHR (VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2KHR* pFormatProperties) const
{
	m_vk.getPhysicalDeviceFormatProperties2KHR(physicalDevice, format, pFormatProperties);
}

VkResult InstanceDriver::getPhysicalDeviceImageFormatProperties2KHR (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2KHR* pImageFormatInfo, VkImageFormatProperties2KHR* pImageFormatProperties) const
{
	return m_vk.getPhysicalDeviceImageFormatProperties2KHR(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void InstanceDriver::getPhysicalDeviceQueueFamilyProperties2KHR (VkPhysicalDevice physicalDevice, deUint32* pQueueFamilyPropertyCount, VkQueueFamilyProperties2KHR* pQueueFamilyProperties) const
{
	m_vk.getPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void InstanceDriver::getPhysicalDeviceMemoryProperties2KHR (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2KHR* pMemoryProperties) const
{
	m_vk.getPhysicalDeviceMemoryProperties2KHR(physicalDevice, pMemoryProperties);
}

void InstanceDriver::getPhysicalDeviceSparseImageFormatProperties2KHR (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2KHR* pFormatInfo, deUint32* pPropertyCount, VkSparseImageFormatProperties2KHR* pProperties) const
{
	m_vk.getPhysicalDeviceSparseImageFormatProperties2KHR(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

VkResult InstanceDriver::getPhysicalDeviceSurfaceCapabilities2KHR (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkSurfaceCapabilities2KHR* pSurfaceCapabilities) const
{
	return m_vk.getPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
}

VkResult InstanceDriver::getPhysicalDeviceSurfaceFormats2KHR (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, deUint32* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats) const
{
	return m_vk.getPhysicalDeviceSurfaceFormats2KHR(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
}

void InstanceDriver::getPhysicalDeviceExternalFencePropertiesKHR (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfoKHR* pExternalFenceInfo, VkExternalFencePropertiesKHR* pExternalFenceProperties) const
{
	m_vk.getPhysicalDeviceExternalFencePropertiesKHR(physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

VkResult InstanceDriver::createDebugReportCallbackEXT (VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) const
{
	return m_vk.createDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
}

void InstanceDriver::destroyDebugReportCallbackEXT (VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) const
{
	m_vk.destroyDebugReportCallbackEXT(instance, callback, pAllocator);
}

void InstanceDriver::debugReportMessageEXT (VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, deUint64 object, deUintptr location, deInt32 messageCode, const char* pLayerPrefix, const char* pMessage) const
{
	m_vk.debugReportMessageEXT(instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
}

void InstanceDriver::getPhysicalDeviceExternalBufferPropertiesKHR (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfoKHR* pExternalBufferInfo, VkExternalBufferPropertiesKHR* pExternalBufferProperties) const
{
	m_vk.getPhysicalDeviceExternalBufferPropertiesKHR(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

void InstanceDriver::getPhysicalDeviceExternalSemaphorePropertiesKHR (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfoKHR* pExternalSemaphoreInfo, VkExternalSemaphorePropertiesKHR* pExternalSemaphoreProperties) const
{
	m_vk.getPhysicalDeviceExternalSemaphorePropertiesKHR(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}
