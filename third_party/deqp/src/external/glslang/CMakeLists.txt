# cmake file for glslang

if (NOT DE_DEFS)
	message(FATAL_ERROR "Include Defs.cmake")
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/SPIRV/GlslangToSpv.cpp")
	set(DEFAULT_GLSLANG_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
else ()
	set(DEFAULT_GLSLANG_SRC_PATH "../glslang")
endif ()

set(GLSLANG_SRC_PATH ${DEFAULT_GLSLANG_SRC_PATH} CACHE STRING "Path to glslang source tree")

if (IS_ABSOLUTE ${GLSLANG_SRC_PATH})
	set(GLSLANG_ABS_PATH ${GLSLANG_SRC_PATH})
else ()
	set(GLSLANG_ABS_PATH "${CMAKE_SOURCE_DIR}/${GLSLANG_SRC_PATH}")
endif ()

if (EXISTS ${GLSLANG_ABS_PATH}/glslang/GenericCodeGen/CodeGen.cpp)
	message(STATUS "glslang found; building with DEQP_SUPPORT_GLSLANG")

	include_directories(
		${GLSLANG_ABS_PATH}
		${GLSLANG_ABS_PATH}/glslang
		${GLSLANG_ABS_PATH}/glslang/Include
		${GLSLANG_ABS_PATH}/glslang/Public
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent
		${GLSLANG_ABS_PATH}/glslang/GenericCodeGen
		${GLSLANG_ABS_PATH}/glslang/OSDependent
		${GLSLANG_ABS_PATH}/hlsl
		${GLSLANG_ABS_PATH}/OGLCompilersDLL
		${GLSLANG_ABS_PATH}/SPIRV
		)

	set(GLSLANG_SRCS
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/Constant.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/glslang_tab.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/InfoSink.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/Initialize.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/Intermediate.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/intermOut.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/iomapper.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/IntermTraverse.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/limits.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/linkValidate.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/parseConst.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/ParseContextBase.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/ParseHelper.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/PoolAlloc.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/propagateNoContraction.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/reflection.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/RemoveTree.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/Scan.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/ShaderLang.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/SymbolTable.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/Versions.cpp

		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/preprocessor/PpAtom.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/preprocessor/PpContext.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/preprocessor/Pp.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/preprocessor/PpScanner.cpp
		${GLSLANG_ABS_PATH}/glslang/MachineIndependent/preprocessor/PpTokens.cpp

		${GLSLANG_ABS_PATH}/glslang/GenericCodeGen/CodeGen.cpp
		${GLSLANG_ABS_PATH}/glslang/GenericCodeGen/Link.cpp

		${GLSLANG_ABS_PATH}/OGLCompilersDLL/InitializeDll.cpp

		${GLSLANG_ABS_PATH}/hlsl/hlslAttributes.cpp
		${GLSLANG_ABS_PATH}/hlsl/hlslGrammar.cpp
		${GLSLANG_ABS_PATH}/hlsl/hlslOpMap.cpp
		${GLSLANG_ABS_PATH}/hlsl/hlslParseables.cpp
		${GLSLANG_ABS_PATH}/hlsl/hlslParseHelper.cpp
		${GLSLANG_ABS_PATH}/hlsl/hlslScanContext.cpp
		${GLSLANG_ABS_PATH}/hlsl/hlslTokenStream.cpp

		${GLSLANG_ABS_PATH}/SPIRV/disassemble.cpp
		${GLSLANG_ABS_PATH}/SPIRV/doc.cpp
		${GLSLANG_ABS_PATH}/SPIRV/GlslangToSpv.cpp
		${GLSLANG_ABS_PATH}/SPIRV/InReadableOrder.cpp
		${GLSLANG_ABS_PATH}/SPIRV/Logger.cpp
		${GLSLANG_ABS_PATH}/SPIRV/SpvBuilder.cpp
		${GLSLANG_ABS_PATH}/SPIRV/SPVRemapper.cpp

		osinclude.cpp
		)

	set(CMAKE_C_FLAGS	${DE_3RD_PARTY_C_FLAGS})
	set(CMAKE_CXX_FLAGS	${DE_3RD_PARTY_CXX_FLAGS})

	if (DE_COMPILER_IS_GCC OR DE_COMPILER_IS_CLANG)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif ()

	add_library(glslang STATIC ${GLSLANG_SRCS})
	target_link_libraries(glslang dethread ${ZLIB_LIBRARY})

	add_definitions(-DENABLE_HLSL)

	set(GLSLANG_INCLUDE_PATH	${GLSLANG_ABS_PATH}	PARENT_SCOPE)
	set(GLSLANG_LIBRARY			glslang				PARENT_SCOPE)
	set(DEQP_HAVE_GLSLANG		ON					PARENT_SCOPE)

else ()
	message(STATUS "glslang not found; GLSL to SPIR-V compilation not available")

	set(DEQP_HAVE_GLSLANG		OFF					PARENT_SCOPE)

endif ()
