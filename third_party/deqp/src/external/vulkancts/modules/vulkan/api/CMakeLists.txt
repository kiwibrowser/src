# API layer tests

include_directories(..)

set(DEQP_VK_API_SRCS
	vktApiTests.cpp
	vktApiTests.hpp
	vktApiSmokeTests.cpp
	vktApiSmokeTests.hpp
	vktApiDeviceInitializationTests.cpp
	vktApiDeviceInitializationTests.hpp
	vktApiObjectManagementTests.cpp
	vktApiObjectManagementTests.hpp
	vktApiBufferTests.cpp
	vktApiBufferTests.hpp
	vktApiBufferViewCreateTests.cpp
	vktApiBufferViewCreateTests.hpp
	vktApiBufferViewAccessTests.cpp
	vktApiBufferViewAccessTests.hpp
	vktApiFeatureInfo.cpp
	vktApiFeatureInfo.hpp
	vktApiCommandBuffersTests.cpp
	vktApiCommandBuffersTests.hpp
	vktApiComputeInstanceResultBuffer.cpp
	vktApiComputeInstanceResultBuffer.hpp
	vktApiBufferComputeInstance.cpp
	vktApiBufferComputeInstance.hpp
	vktApiCopiesAndBlittingTests.cpp
	vktApiCopiesAndBlittingTests.hpp
	vktApiImageClearingTests.cpp
	vktApiImageClearingTests.hpp
	vktApiFillBufferTests.cpp
	vktApiFillBufferTests.hpp
	vktApiDescriptorPoolTests.cpp
	vktApiDescriptorPoolTests.hpp
	vktApiNullHandleTests.cpp
	vktApiNullHandleTests.hpp
	vktApiGranularityTests.cpp
	vktApiGranularityTests.hpp
	vktApiGetMemoryCommitment.cpp
	vktApiGetMemoryCommitment.hpp
	vktApiExternalMemoryTests.cpp
	vktApiExternalMemoryTests.hpp
	vktApiBufferAndImageAllocationUtil.hpp
	vktApiBufferAndImageAllocationUtil.cpp
	)

set(DEQP_VK_API_LIBS
	deqp-vk-package
	tcutil
	vkutil
	deqp-vk-package
	)

add_library(deqp-vk-api STATIC ${DEQP_VK_API_SRCS})
target_link_libraries(deqp-vk-api ${DEQP_VK_API_LIBS})
