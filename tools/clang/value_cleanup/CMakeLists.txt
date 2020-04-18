set(LLVM_LINK_COMPONENTS
  BitReader
  MCParser
  Option
  X86AsmParser
  X86CodeGen
  )

add_llvm_executable(value_cleanup
  ValueCleanup.cpp
  ValueRewriter.cpp
  )

target_link_libraries(value_cleanup
  clangAST
  clangASTMatchers
  clangAnalysis
  clangBasic
  clangDriver
  clangEdit
  clangFrontend
  clangLex
  clangParse
  clangSema
  clangSerialization
  clangTooling
  )

cr_install(TARGETS value_cleanup RUNTIME DESTINATION bin)
