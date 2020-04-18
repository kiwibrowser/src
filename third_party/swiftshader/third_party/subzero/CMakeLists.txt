unset(PNACL_LLVM)
# Define PNACL_LLVM for LLVM_VERSION <= 3.7
if((NOT LLVM_VERSION_MAJOR GREATER 3) AND (NOT LLVM_VERSION_MINOR GREATER 7))
  set(PNACL_LLVM 1)
endif()

if(PNACL_LLVM)
  add_definitions(
    -DPNACL_LLVM
    )
  set(PNACL_EXTRA_COMPONENTS NaClBitReader NaClBitTestUtils)
else()
  include_directories(pnacl-llvm/include)
  file(GLOB pnacl_llvm_SRCS "pnacl-llvm/*.cpp")
endif()

set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Core
  IRReader
  Support
  ${PNACL_EXTRA_COMPONENTS}
  )

file(GLOB pnacl_sz_SRCS "src/*.cpp")
add_llvm_tool(pnacl-sz ${pnacl_sz_SRCS} ${pnacl_llvm_SRCS})

if(PNACL_BROWSER_TRANSLATOR)
  add_definitions(
  -DALLOW_DUMP=0
  -DALLOW_LLVM_CL=0
  -DALLOW_LLVM_IR=0
  -DALLOW_LLVM_IR_AS_INPUT=0
  -DALLOW_TIMERS=0
  -DALLOW_MINIMAL_BUILD=1
  -DALLOW_WASM=0
  -DPNACL_BROWSER_TRANSLATOR=1
  )
else()
  add_definitions(
  -DALLOW_DUMP=1
  -DALLOW_LLVM_CL=1
  -DALLOW_LLVM_IR=1
  -DALLOW_LLVM_IR_AS_INPUT=1
  -DALLOW_TIMERS=1
  -DALLOW_MINIMAL_BUILD=0
  -DALLOW_WASM=0
  -DPNACL_BROWSER_TRANSLATOR=0
  )
  add_compile_options(-Wno-undefined-var-template)
endif()

execute_process(COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE SZ_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
execute_process(COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE SZ_COMMIT_COUNT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
add_definitions(-DSUBZERO_REVISION=${SZ_COMMIT_COUNT}_${SZ_GIT_HASH})

target_link_libraries(pnacl-sz pthread)
