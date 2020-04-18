#ifndef _VKTEXTERNALMEMORYUTIL_HPP
#define _VKTEXTERNALMEMORYUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \brief Vulkan external memory utilities
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"

#include "vkPlatform.hpp"
#include "vkRefUtil.hpp"

namespace vkt
{
namespace ExternalMemoryUtil
{

class NativeHandle
{
public:
	enum Win32HandleType
	{
		WIN32HANDLETYPE_NT = 0,
		WIN32HANDLETYPE_KMT,

		WIN32HANDLETYPE_LAST
	};

						NativeHandle	(void);
						NativeHandle	(const NativeHandle& other);
						NativeHandle	(int fd);
						NativeHandle	(Win32HandleType type, vk::pt::Win32Handle handle);
						~NativeHandle	(void);

	NativeHandle&		operator=		(int fd);

	void				setWin32Handle	(Win32HandleType type, vk::pt::Win32Handle handle);

	vk::pt::Win32Handle	getWin32Handle	(void) const;
	int					getFd			(void) const;
	void				disown			(void);
	void				reset			(void);

private:
	int					m_fd;
	Win32HandleType		m_win32HandleType;
	vk::pt::Win32Handle	m_win32Handle;

	// Disabled
	NativeHandle&		operator=		(const NativeHandle&);
};

const char*						externalSemaphoreTypeToName	(vk::VkExternalSemaphoreHandleTypeFlagBitsKHR type);
const char*						externalFenceTypeToName		(vk::VkExternalFenceHandleTypeFlagBitsKHR type);
const char*						externalMemoryTypeToName	(vk::VkExternalMemoryHandleTypeFlagBitsKHR type);

enum Permanence
{
	PERMANENCE_PERMANENT = 0,
	PERMANENCE_TEMPORARY
};

enum Transference
{
	TRANSFERENCE_COPY = 0,
	TRANSFERENCE_REFERENCE
};

bool							isSupportedPermanence				(vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	type,
																	 Permanence										permanence);
Transference					getHandelTypeTransferences			(vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	type);

bool							isSupportedPermanence				(vk::VkExternalFenceHandleTypeFlagBitsKHR		type,
																	 Permanence										permanence);
Transference					getHandelTypeTransferences			(vk::VkExternalFenceHandleTypeFlagBitsKHR		type);

int								getMemoryFd							(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 vk::VkDeviceMemory							memory,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType);

void							getMemoryNative						(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 vk::VkDeviceMemory							memory,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 NativeHandle&								nativeHandle);

vk::Move<vk::VkSemaphore>		createExportableSemaphore			(const vk::DeviceInterface&						vkd,
																	 vk::VkDevice									device,
																	 vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	externalType);

int								getSemaphoreFd						(const vk::DeviceInterface&						vkd,
																	 vk::VkDevice									device,
																	 vk::VkSemaphore								semaphore,
																	 vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	externalType);

void							getSemaphoreNative					(const vk::DeviceInterface&						vkd,
																	 vk::VkDevice									device,
																	 vk::VkSemaphore								semaphore,
																	 vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	externalType,
																	 NativeHandle&									nativeHandle);

void							importSemaphore						(const vk::DeviceInterface&						vkd,
																	 const vk::VkDevice								device,
																	 const vk::VkSemaphore							semaphore,
																	 vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	externalType,
																	 NativeHandle&									handle,
																	 vk::VkSemaphoreImportFlagsKHR					flags);

vk::Move<vk::VkSemaphore>		createAndImportSemaphore			(const vk::DeviceInterface&						vkd,
																	 const vk::VkDevice								device,
																	 vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	externalType,
																	 NativeHandle&									handle,
																	 vk::VkSemaphoreImportFlagsKHR					flags);

vk::Move<vk::VkFence>			createExportableFence				(const vk::DeviceInterface&						vkd,
																	 vk::VkDevice									device,
																	 vk::VkExternalFenceHandleTypeFlagBitsKHR		externalType);

int								getFenceFd							(const vk::DeviceInterface&						vkd,
																	 vk::VkDevice									device,
																	 vk::VkFence									fence,
																	 vk::VkExternalFenceHandleTypeFlagBitsKHR		externalType);

void							getFenceNative						(const vk::DeviceInterface&						vkd,
																	 vk::VkDevice									device,
																	 vk::VkFence									fence,
																	 vk::VkExternalFenceHandleTypeFlagBitsKHR		externalType,
																	 NativeHandle&									nativeHandle);

void							importFence							(const vk::DeviceInterface&						vkd,
																	 const vk::VkDevice								device,
																	 const vk::VkFence								fence,
																	 vk::VkExternalFenceHandleTypeFlagBitsKHR		externalType,
																	 NativeHandle&									handle,
																	 vk::VkFenceImportFlagsKHR						flags);

vk::Move<vk::VkFence>			createAndImportFence				(const vk::DeviceInterface&						vkd,
																	 const vk::VkDevice								device,
																	 vk::VkExternalFenceHandleTypeFlagBitsKHR		externalType,
																	 NativeHandle&									handle,
																	 vk::VkFenceImportFlagsKHR						flags);

vk::Move<vk::VkDeviceMemory>	allocateExportableMemory			(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 const vk::VkMemoryRequirements&			requirements,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 deUint32&									exportedMemoryTypeIndex);

// If buffer is not null use dedicated allocation
vk::Move<vk::VkDeviceMemory>	allocateExportableMemory			(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 const vk::VkMemoryRequirements&			requirements,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 vk::VkBuffer								buffer,
																	 deUint32&									exportedMemoryTypeIndex);

// If image is not null use dedicated allocation
vk::Move<vk::VkDeviceMemory>	allocateExportableMemory			(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 const vk::VkMemoryRequirements&			requirements,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 vk::VkImage								image,
																	 deUint32&									exportedMemoryTypeIndex);

// \note hostVisible argument is strict. Setting it to false will cause NotSupportedError to be thrown if non-host visible memory doesn't exist.
// If buffer is not null use dedicated allocation
vk::Move<vk::VkDeviceMemory>	allocateExportableMemory			(const vk::InstanceInterface&				vki,
																	 vk::VkPhysicalDevice						physicalDevice,
																	 const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 const vk::VkMemoryRequirements&			requirements,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 bool										hostVisible,
																	 vk::VkBuffer								buffer,
																	 deUint32&									exportedMemoryTypeIndex);

vk::Move<vk::VkDeviceMemory>	importMemory						(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 const vk::VkMemoryRequirements&			requirements,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 deUint32									memoryTypeIndex,
																	 NativeHandle&								handle);

vk::Move<vk::VkDeviceMemory>	importDedicatedMemory				(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 vk::VkBuffer								buffer,
																	 const vk::VkMemoryRequirements&			requirements,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 deUint32									memoryTypeIndex,
																	 NativeHandle&								handle);

vk::Move<vk::VkDeviceMemory>	importDedicatedMemory				(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 vk::VkImage								image,
																	 const vk::VkMemoryRequirements&			requirements,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 deUint32									memoryTypeIndex,
																	 NativeHandle&								handle);

vk::Move<vk::VkBuffer>			createExternalBuffer				(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 deUint32									queueFamilyIndex,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 vk::VkDeviceSize							size,
																	 vk::VkBufferCreateFlags					createFlags,
																	 vk::VkBufferUsageFlags						usageFlags);

vk::Move<vk::VkImage>			createExternalImage					(const vk::DeviceInterface&					vkd,
																	 vk::VkDevice								device,
																	 deUint32									queueFamilyIndex,
																	 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
																	 vk::VkFormat								format,
																	 deUint32									width,
																	 deUint32									height,
																	 vk::VkImageTiling							tiling,
																	 vk::VkImageCreateFlags						createFlags,
																	 vk::VkImageUsageFlags						usageFlags);

} // ExternalMemoryUtil
} // vkt

#endif // _VKTEXTERNALMEMORYUTIL_HPP
