# cmake file for drawElements C++ base library

if (NOT DE_DEFS)
	message(FATAL_ERROR "Include Defs.cmake")
endif ()

set(DECPP_SRCS
	deAppendList.cpp
	deAppendList.hpp
	deArrayBuffer.cpp
	deArrayBuffer.hpp
	deArrayUtil.cpp
	deArrayUtil.hpp
	deBlockBuffer.cpp
	deBlockBuffer.hpp
	deCommandLine.cpp
	deCommandLine.hpp
	deDefs.cpp
	deDefs.hpp
	deDirectoryIterator.cpp
	deDirectoryIterator.hpp
	deDynamicLibrary.cpp
	deDynamicLibrary.hpp
	deFilePath.cpp
	deFilePath.hpp
	deMemPool.cpp
	deMemPool.hpp
	deMeta.cpp
	deMeta.hpp
	deMutex.cpp
	deMutex.hpp
	dePoolArray.cpp
	dePoolArray.hpp
	dePoolString.cpp
	dePoolString.hpp
	deProcess.cpp
	deProcess.hpp
	deRandom.cpp
	deRandom.hpp
	deRingBuffer.cpp
	deRingBuffer.hpp
	deSemaphore.cpp
	deSemaphore.hpp
	deSTLUtil.hpp
	deSTLUtil.cpp
	deSharedPtr.cpp
	deSharedPtr.hpp
	deSocket.cpp
	deSocket.hpp
	deStringUtil.cpp
	deStringUtil.hpp
	deThread.cpp
	deThread.hpp
	deThreadLocal.cpp
	deThreadLocal.hpp
	deThreadSafeRingBuffer.cpp
	deThreadSafeRingBuffer.hpp
	deUniquePtr.cpp
	deUniquePtr.hpp
	deSpinBarrier.cpp
	deSpinBarrier.hpp
	deSha1.cpp
	deSha1.hpp
	)

set(DECPP_LIBS
	debase
	depool
	dethread
	deutil
	)

# \note [pyry] Somewhat kludgy that toolchain file doesn't set it, but there is no easy way
if (DE_OS_IS_ANDROID)
	if (DEFINED ANDROID_CXX_LIBRARY)
		set(DECPP_LIBS ${DECPP_LIBS} ${ANDROID_CXX_LIBRARY})
	else ()
		message(STATUS "Warning: ANDROID_CXX_LIBRARY not defined")
	endif ()
endif ()

add_library(decpp STATIC ${DECPP_SRCS})
target_link_libraries(decpp ${DECPP_LIBS})
