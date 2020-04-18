cmake_minimum_required(VERSION 2.8)

project(SwiftShader C CXX)

###########################################################
# Detect system
###########################################################

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX ON)
elseif(WIN32)
elseif(APPLE)
else()
    message(FATAL_ERROR "Platform is not supported")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH "aarch64")
    else()
        set(ARCH "arm")
    endif()
else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH "x86_64")
    else()
        set(ARCH "x86")
    endif()
endif()

set(CMAKE_MACOSX_RPATH ON)

###########################################################
# Options
###########################################################

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The type of build: Debug Release MinSizeRel RelWithDebInfo." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)

option(BUILD_EGL "Build the EGL library" 1)
if(WIN32)
    option(BUILD_GL32 "Build the OpenGL32 library" 1)
endif()
option(BUILD_GLESv2 "Build the OpenGL ES 2 library" 1)
option(BUILD_GLES_CM "Build the OpenGL ES 1.1 library" 1)

option(USE_GROUP_SOURCES "Group the source files in a folder tree for Visual Studio" 1)

option(BUILD_SAMPLES "Build sample programs" 1)
option(BUILD_TESTS "Build test programs" 1)

if(ARCH STREQUAL "arm")
    set(DEFAULT_REACTOR_BACKEND "Subzero")
else()
    set(DEFAULT_REACTOR_BACKEND "LLVM")
endif()

set(REACTOR_BACKEND DEFAULT_REACTOR_BACKEND CACHE STRING "JIT compiler back-end used by Reactor")
set_property(CACHE REACTOR_BACKEND PROPERTY STRINGS LLVM Subzero)

# LLVM disallows calling cmake . from the main LLVM dir, the reason is that
# it builds header files that could overwrite the orignal ones. Here we
# want to include LLVM as a subdirectory and even though it wouldn't cause
# the problem, if cmake . is called from the main dir, the condition that
# LLVM checkes, "CMAKE_SOURCE_DIR == CMAKE_BINARY_DIR" will be true. So we
# disallow it ourselves too to. In addition if there are remining CMakeFiles
# and CMakeCache in the directory, cmake .. from a subdirectory will still
# try to build from the main directory so we instruct users to delete these
# files when they get the error.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In source builds are not allowed by LLVM, please create a build/ directory and build from there. You may have to delete the CMakeCache.txt file and CMakeFiles directory that are next to the CMakeLists.txt.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###########################################################
# Convenience macros
###########################################################

# Recursively calls source_group on the files of the directory
# so that Visual Studio has the files in a folder tree
macro(group_all_sources directory)
    file(GLOB files RELATIVE ${CMAKE_SOURCE_DIR}/${directory} ${CMAKE_SOURCE_DIR}/${directory}/*)
    foreach(file ${files})
        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${directory}/${file})
            group_all_sources(${directory}/${file})
        else()
            string(REPLACE "/" "\\" groupname ${directory})
            source_group(${groupname} FILES ${CMAKE_SOURCE_DIR}/${directory}/${file})
        endif()
    endforeach()
endmacro()

# Takes target library and a directory where the export map is
# and add the linker options so that only the API symbols are
# exported.
macro(set_target_export_map TARGET DIR)
    get_target_property(LINKFLAGS ${TARGET} LINK_FLAGS)
    if(LINKFLAGS MATCHES "NOTFOUND")
        set(LINKFLAGS "")
    endif()
    if(MSVC)
        set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "/DEF:\"${DIR}/${TARGET}.def\"")
    elseif(LINUX)
        # Both hash-style are needed, because we want both gold and
        # GNU ld to be able to read our libraries (maybe?).
        # The version script only exports the API functions and
        # hides all the others. Gc sections is used in combination
        # with each functions being in its section, to reduce the
        # binary size.
        set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "${LINKFLAGS} -Wl,--hash-style=both,--version-script=${DIR}/${TARGET}.lds,--gc-sections,--no-undefined")
        set_target_properties(${TARGET} PROPERTIES LINK_DEPENDS "${DIR}/${TARGET}.lds")
    endif()
endmacro()

if(USE_GROUP_SOURCES)
    group_all_sources(src)
endif()

###########################################################
# Directories
###########################################################

set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(OPENGL_DIR ${SOURCE_DIR}/OpenGL)
set(OPENGL_COMPILER_DIR ${OPENGL_DIR}/compiler)
set(LLVM_DIR ${CMAKE_SOURCE_DIR}/third_party/LLVM)
set(SUBZERO_DIR ${CMAKE_SOURCE_DIR}/third_party/subzero)
set(SUBZERO_LLVM_DIR ${CMAKE_SOURCE_DIR}/third_party/llvm-subzero)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)
set(HELLO2_DIR ${CMAKE_SOURCE_DIR}/third_party/PowerVR_SDK/Examples/Beginner/01_HelloAPI/OGLES2)

###########################################################
# Compile flags
###########################################################

macro(set_cpp_flag FLAG)
    if(${ARGC} GREATER 1)
        set(CMAKE_CXX_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${FLAG}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
    endif()
endmacro()

if(MSVC)
    set_cpp_flag("/MP")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set_cpp_flag("--std=c++11")
    set_cpp_flag("-Wall")
    set_cpp_flag("-Werror=reorder")
    set_cpp_flag("-Werror=sign-compare")
    set_cpp_flag("-fno-exceptions")

    # Remove xor, and, or and friends from the list of keywords, they are used
    # by Reactor
    set_cpp_flag("-fno-operator-names")

    # LLVM headers requires these flags to be set
    set_cpp_flag("-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS")

    if(ARCH EQUAL "x86")
        set_cpp_flag("-m32")
        set_cpp_flag("-msse2")
        set_cpp_flag("-mfpmath=sse")
        set_cpp_flag("-march=pentium4")
        set_cpp_flag("-mtune=generic")
    endif()
    if(ARCH EQUAL "x86_64")
        set_cpp_flag("-m64")
        set_cpp_flag("-fPIC")
        set_cpp_flag("-march=x86-64")
        set_cpp_flag("-mtune=generic")
    endif()

    # Use -g3 to have even more debug info
    set_cpp_flag("-g -g3" DEBUG)
    set_cpp_flag("-g -g3" RELWITHDEBINFO)
    set_cpp_flag("-s" RELEASE)

    # For distribution it is more important to be slim than super optimized
    set_cpp_flag("-Os" RELEASE)
    set_cpp_flag("-Os" RELWITHDEBINFO)

    set_cpp_flag("-DNDEBUG" RELEASE)
    set_cpp_flag("-DNDEBUG" RELWITHDEBINFO)
    set_cpp_flag("-DANGLE_DISABLE_TRACE" RELEASE)
    set_cpp_flag("-DANGLE_DISABLE_TRACE" RELWITHDEBINFO)

    # Put each variable and function in its own section so that when linking
    # with -gc-sections unused functions and variables are removed.
    set_cpp_flag("-ffunction-sections" RELEASE)
    set_cpp_flag("-fdata-sections" RELEASE)
    set_cpp_flag("-fomit-frame-pointer" RELEASE)
endif()

if( WIN32 )
    add_definitions(-DWINVER=0x501 -DNOMINMAX -DSTRICT)
    set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "" "lib")
endif()


###########################################################
# LLVM
###########################################################

set(LLVM_LIST
    ${LLVM_DIR}/lib/Analysis/AliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/AliasSetTracker.cpp
    ${LLVM_DIR}/lib/Analysis/BasicAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/BranchProbabilityInfo.cpp
    ${LLVM_DIR}/lib/Analysis/CaptureTracking.cpp
    ${LLVM_DIR}/lib/Analysis/ConstantFolding.cpp
    ${LLVM_DIR}/lib/Analysis/DIBuilder.cpp
    ${LLVM_DIR}/lib/Analysis/DebugInfo.cpp
    ${LLVM_DIR}/lib/Analysis/IVUsers.cpp
    ${LLVM_DIR}/lib/Analysis/InstructionSimplify.cpp
    ${LLVM_DIR}/lib/Analysis/Loads.cpp
    ${LLVM_DIR}/lib/Analysis/LoopInfo.cpp
    ${LLVM_DIR}/lib/Analysis/LoopPass.cpp
    ${LLVM_DIR}/lib/Analysis/MemoryBuiltins.cpp
    ${LLVM_DIR}/lib/Analysis/MemoryDependenceAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/NoAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/PHITransAddr.cpp
    ${LLVM_DIR}/lib/Analysis/PathNumbering.cpp
    ${LLVM_DIR}/lib/Analysis/ProfileInfo.cpp
    ${LLVM_DIR}/lib/Analysis/ScalarEvolution.cpp
    ${LLVM_DIR}/lib/Analysis/ScalarEvolutionExpander.cpp
    ${LLVM_DIR}/lib/Analysis/ScalarEvolutionNormalization.cpp
    ${LLVM_DIR}/lib/Analysis/TypeBasedAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/ValueTracking.cpp
    ${LLVM_DIR}/lib/CodeGen/AggressiveAntiDepBreaker.cpp
    ${LLVM_DIR}/lib/CodeGen/AllocationOrder.cpp
    ${LLVM_DIR}/lib/CodeGen/Analysis.cpp
    ${LLVM_DIR}/lib/CodeGen/BranchFolding.cpp
    ${LLVM_DIR}/lib/CodeGen/CalcSpillWeights.cpp
    ${LLVM_DIR}/lib/CodeGen/CallingConvLower.cpp
    ${LLVM_DIR}/lib/CodeGen/CodePlacementOpt.cpp
    ${LLVM_DIR}/lib/CodeGen/CriticalAntiDepBreaker.cpp
    ${LLVM_DIR}/lib/CodeGen/DeadMachineInstructionElim.cpp
    ${LLVM_DIR}/lib/CodeGen/DwarfEHPrepare.cpp
    ${LLVM_DIR}/lib/CodeGen/ELFCodeEmitter.cpp
    ${LLVM_DIR}/lib/CodeGen/ELFWriter.cpp
    ${LLVM_DIR}/lib/CodeGen/EdgeBundles.cpp
    ${LLVM_DIR}/lib/CodeGen/ExecutionDepsFix.cpp
    ${LLVM_DIR}/lib/CodeGen/ExpandISelPseudos.cpp
    ${LLVM_DIR}/lib/CodeGen/ExpandPostRAPseudos.cpp
    ${LLVM_DIR}/lib/CodeGen/GCMetadata.cpp
    ${LLVM_DIR}/lib/CodeGen/GCStrategy.cpp
    ${LLVM_DIR}/lib/CodeGen/InlineSpiller.cpp
    ${LLVM_DIR}/lib/CodeGen/InterferenceCache.cpp
    ${LLVM_DIR}/lib/CodeGen/IntrinsicLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/LLVMTargetMachine.cpp
    ${LLVM_DIR}/lib/CodeGen/LatencyPriorityQueue.cpp
    ${LLVM_DIR}/lib/CodeGen/LexicalScopes.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveDebugVariables.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveInterval.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveIntervalAnalysis.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveIntervalUnion.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveRangeCalc.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveRangeEdit.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveStackAnalysis.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveVariables.cpp
    ${LLVM_DIR}/lib/CodeGen/LocalStackSlotAllocation.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineBasicBlock.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineCSE.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineDominators.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFunction.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFunctionAnalysis.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFunctionPass.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFunctionPrinterPass.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineInstr.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineLICM.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineLoopInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineModuleInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineModuleInfoImpls.cpp
    ${LLVM_DIR}/lib/CodeGen/MachinePassRegistry.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineRegisterInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineSSAUpdater.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineSink.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineVerifier.cpp
    ${LLVM_DIR}/lib/CodeGen/ObjectCodeEmitter.cpp
    ${LLVM_DIR}/lib/CodeGen/OptimizePHIs.cpp
    ${LLVM_DIR}/lib/CodeGen/PHIElimination.cpp
    ${LLVM_DIR}/lib/CodeGen/PHIEliminationUtils.cpp
    ${LLVM_DIR}/lib/CodeGen/Passes.cpp
    ${LLVM_DIR}/lib/CodeGen/PeepholeOptimizer.cpp
    ${LLVM_DIR}/lib/CodeGen/PostRASchedulerList.cpp
    ${LLVM_DIR}/lib/CodeGen/ProcessImplicitDefs.cpp
    ${LLVM_DIR}/lib/CodeGen/PrologEpilogInserter.cpp
    ${LLVM_DIR}/lib/CodeGen/PseudoSourceValue.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocBasic.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocFast.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocGreedy.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocLinearScan.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterClassInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterCoalescer.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterScavenging.cpp
    ${LLVM_DIR}/lib/CodeGen/RenderMachineFunction.cpp
    ${LLVM_DIR}/lib/CodeGen/ScheduleDAG.cpp
    ${LLVM_DIR}/lib/CodeGen/ScheduleDAGEmit.cpp
    ${LLVM_DIR}/lib/CodeGen/ScheduleDAGInstrs.cpp
    ${LLVM_DIR}/lib/CodeGen/ScheduleDAGPrinter.cpp
    ${LLVM_DIR}/lib/CodeGen/ScoreboardHazardRecognizer.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/DAGCombiner.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/FastISel.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/FunctionLoweringInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/InstrEmitter.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeDAG.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeFloatTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeIntegerTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeTypesGeneric.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeVectorOps.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeVectorTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ScheduleDAGFast.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ScheduleDAGList.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ScheduleDAGRRList.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ScheduleDAGSDNodes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAG.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGBuilder.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGISel.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGPrinter.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/TargetLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/TargetSelectionDAGInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/ShrinkWrapping.cpp
    ${LLVM_DIR}/lib/CodeGen/SjLjEHPrepare.cpp
    ${LLVM_DIR}/lib/CodeGen/SlotIndexes.cpp
    ${LLVM_DIR}/lib/CodeGen/SpillPlacement.cpp
    ${LLVM_DIR}/lib/CodeGen/Spiller.cpp
    ${LLVM_DIR}/lib/CodeGen/SplitKit.cpp
    ${LLVM_DIR}/lib/CodeGen/StackProtector.cpp
    ${LLVM_DIR}/lib/CodeGen/StackSlotColoring.cpp
    ${LLVM_DIR}/lib/CodeGen/StrongPHIElimination.cpp
    ${LLVM_DIR}/lib/CodeGen/TailDuplication.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetInstrInfoImpl.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetLoweringObjectFileImpl.cpp
    ${LLVM_DIR}/lib/CodeGen/TwoAddressInstructionPass.cpp
    ${LLVM_DIR}/lib/CodeGen/UnreachableBlockElim.cpp
    ${LLVM_DIR}/lib/CodeGen/VirtRegMap.cpp
    ${LLVM_DIR}/lib/CodeGen/VirtRegRewriter.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/ExecutionEngine.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/JIT/Intercept.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/JIT/JIT.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/JIT/JITDebugRegisterer.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/JIT/JITDwarfEmitter.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/JIT/JITEmitter.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/JIT/JITMemoryManager.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/JIT/OProfileJITEventListener.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/TargetSelect.cpp
    ${LLVM_DIR}/lib/MC/ELFObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/MCAsmBackend.cpp
    ${LLVM_DIR}/lib/MC/MCAsmInfo.cpp
    ${LLVM_DIR}/lib/MC/MCAsmInfoCOFF.cpp
    ${LLVM_DIR}/lib/MC/MCAsmInfoDarwin.cpp
    ${LLVM_DIR}/lib/MC/MCAsmStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCAssembler.cpp
    ${LLVM_DIR}/lib/MC/MCCodeEmitter.cpp
    ${LLVM_DIR}/lib/MC/MCCodeGenInfo.cpp
    ${LLVM_DIR}/lib/MC/MCContext.cpp
    ${LLVM_DIR}/lib/MC/MCDwarf.cpp
    ${LLVM_DIR}/lib/MC/MCELF.cpp
    ${LLVM_DIR}/lib/MC/MCELFObjectTargetWriter.cpp
    ${LLVM_DIR}/lib/MC/MCELFStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCExpr.cpp
    ${LLVM_DIR}/lib/MC/MCInst.cpp
    ${LLVM_DIR}/lib/MC/MCInstPrinter.cpp
    ${LLVM_DIR}/lib/MC/MCInstrAnalysis.cpp
    ${LLVM_DIR}/lib/MC/MCLoggingStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCMachOStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCMachObjectTargetWriter.cpp
    ${LLVM_DIR}/lib/MC/MCNullStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCObjectFileInfo.cpp
    ${LLVM_DIR}/lib/MC/MCObjectStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/MCSection.cpp
    ${LLVM_DIR}/lib/MC/MCSectionCOFF.cpp
    ${LLVM_DIR}/lib/MC/MCSectionELF.cpp
    ${LLVM_DIR}/lib/MC/MCSectionMachO.cpp
    ${LLVM_DIR}/lib/MC/MCStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCSubtargetInfo.cpp
    ${LLVM_DIR}/lib/MC/MCSymbol.cpp
    ${LLVM_DIR}/lib/MC/MCWin64EH.cpp
    ${LLVM_DIR}/lib/MC/MachObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/SubtargetFeature.cpp
    ${LLVM_DIR}/lib/MC/WinCOFFObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/WinCOFFStreamer.cpp
    ${LLVM_DIR}/lib/Support/APFloat.cpp
    ${LLVM_DIR}/lib/Support/APInt.cpp
    ${LLVM_DIR}/lib/Support/Allocator.cpp
    ${LLVM_DIR}/lib/Support/Atomic.cpp
    ${LLVM_DIR}/lib/Support/BlockFrequency.cpp
    ${LLVM_DIR}/lib/Support/BranchProbability.cpp
    ${LLVM_DIR}/lib/Support/CommandLine.cpp
    ${LLVM_DIR}/lib/Support/ConstantRange.cpp
    ${LLVM_DIR}/lib/Support/CrashRecoveryContext.cpp
    ${LLVM_DIR}/lib/Support/DAGDeltaAlgorithm.cpp
    ${LLVM_DIR}/lib/Support/DataExtractor.cpp
    ${LLVM_DIR}/lib/Support/Debug.cpp
    ${LLVM_DIR}/lib/Support/DeltaAlgorithm.cpp
    ${LLVM_DIR}/lib/Support/Disassembler.cpp
    ${LLVM_DIR}/lib/Support/Dwarf.cpp
    ${LLVM_DIR}/lib/Support/DynamicLibrary.cpp
    ${LLVM_DIR}/lib/Support/Errno.cpp
    ${LLVM_DIR}/lib/Support/ErrorHandling.cpp
    ${LLVM_DIR}/lib/Support/FileUtilities.cpp
    ${LLVM_DIR}/lib/Support/FoldingSet.cpp
    ${LLVM_DIR}/lib/Support/FormattedStream.cpp
    ${LLVM_DIR}/lib/Support/GraphWriter.cpp
    ${LLVM_DIR}/lib/Support/Host.cpp
    ${LLVM_DIR}/lib/Support/IncludeFile.cpp
    ${LLVM_DIR}/lib/Support/IntEqClasses.cpp
    ${LLVM_DIR}/lib/Support/IntervalMap.cpp
    ${LLVM_DIR}/lib/Support/IsInf.cpp
    ${LLVM_DIR}/lib/Support/IsNAN.cpp
    ${LLVM_DIR}/lib/Support/ManagedStatic.cpp
    ${LLVM_DIR}/lib/Support/Memory.cpp
    ${LLVM_DIR}/lib/Support/MemoryBuffer.cpp
    ${LLVM_DIR}/lib/Support/MemoryObject.cpp
    ${LLVM_DIR}/lib/Support/Mutex.cpp
    ${LLVM_DIR}/lib/Support/Path.cpp
    ${LLVM_DIR}/lib/Support/PathV2.cpp
    ${LLVM_DIR}/lib/Support/PrettyStackTrace.cpp
    ${LLVM_DIR}/lib/Support/Process.cpp
    ${LLVM_DIR}/lib/Support/Program.cpp
    ${LLVM_DIR}/lib/Support/RWMutex.cpp
    ${LLVM_DIR}/lib/Support/SearchForAddressOfSpecialSymbol.cpp
    ${LLVM_DIR}/lib/Support/Signals.cpp
    ${LLVM_DIR}/lib/Support/SmallPtrSet.cpp
    ${LLVM_DIR}/lib/Support/SmallVector.cpp
    ${LLVM_DIR}/lib/Support/SourceMgr.cpp
    ${LLVM_DIR}/lib/Support/Statistic.cpp
    ${LLVM_DIR}/lib/Support/StringExtras.cpp
    ${LLVM_DIR}/lib/Support/StringMap.cpp
    ${LLVM_DIR}/lib/Support/StringPool.cpp
    ${LLVM_DIR}/lib/Support/StringRef.cpp
    ${LLVM_DIR}/lib/Support/SystemUtils.cpp
    ${LLVM_DIR}/lib/Support/TargetRegistry.cpp
    ${LLVM_DIR}/lib/Support/ThreadLocal.cpp
    ${LLVM_DIR}/lib/Support/Threading.cpp
    ${LLVM_DIR}/lib/Support/TimeValue.cpp
    ${LLVM_DIR}/lib/Support/Timer.cpp
    ${LLVM_DIR}/lib/Support/ToolOutputFile.cpp
    ${LLVM_DIR}/lib/Support/Triple.cpp
    ${LLVM_DIR}/lib/Support/Twine.cpp
    ${LLVM_DIR}/lib/Support/Valgrind.cpp
    ${LLVM_DIR}/lib/Support/circular_raw_ostream.cpp
    ${LLVM_DIR}/lib/Support/raw_os_ostream.cpp
    ${LLVM_DIR}/lib/Support/raw_ostream.cpp
    ${LLVM_DIR}/lib/Support/system_error.cpp
    ${LLVM_DIR}/lib/Target/Mangler.cpp
    ${LLVM_DIR}/lib/Target/TargetData.cpp
    ${LLVM_DIR}/lib/Target/TargetELFWriterInfo.cpp
    ${LLVM_DIR}/lib/Target/TargetFrameLowering.cpp
    ${LLVM_DIR}/lib/Target/TargetInstrInfo.cpp
    ${LLVM_DIR}/lib/Target/TargetLibraryInfo.cpp
    ${LLVM_DIR}/lib/Target/TargetLoweringObjectFile.cpp
    ${LLVM_DIR}/lib/Target/TargetMachine.cpp
    ${LLVM_DIR}/lib/Target/TargetRegisterInfo.cpp
    ${LLVM_DIR}/lib/Target/TargetSubtargetInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/InstPrinter/X86ATTInstPrinter.cpp
    ${LLVM_DIR}/lib/Target/X86/InstPrinter/X86InstComments.cpp
    ${LLVM_DIR}/lib/Target/X86/InstPrinter/X86IntelInstPrinter.cpp
    ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86AsmBackend.cpp
    ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MCAsmInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MCCodeEmitter.cpp
    ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MCTargetDesc.cpp
    ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MachObjectWriter.cpp
    ${LLVM_DIR}/lib/Target/X86/TargetInfo/X86TargetInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/Utils/X86ShuffleDecode.cpp
    ${LLVM_DIR}/lib/Target/X86/X86CodeEmitter.cpp
    ${LLVM_DIR}/lib/Target/X86/X86ELFWriterInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/X86FastISel.cpp
    ${LLVM_DIR}/lib/Target/X86/X86FloatingPoint.cpp
    ${LLVM_DIR}/lib/Target/X86/X86FrameLowering.cpp
    ${LLVM_DIR}/lib/Target/X86/X86ISelDAGToDAG.cpp
    ${LLVM_DIR}/lib/Target/X86/X86ISelLowering.cpp
    ${LLVM_DIR}/lib/Target/X86/X86InstrInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/X86JITInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/X86RegisterInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/X86SelectionDAGInfo.cpp
    ${LLVM_DIR}/lib/Target/X86/X86Subtarget.cpp
    ${LLVM_DIR}/lib/Target/X86/X86TargetMachine.cpp
    ${LLVM_DIR}/lib/Target/X86/X86TargetObjectFile.cpp
    ${LLVM_DIR}/lib/Target/X86/X86VZeroUpper.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineAddSub.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineAndOrXor.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineCalls.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineCasts.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineCompares.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineLoadStoreAlloca.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineMulDivRem.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombinePHI.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineSelect.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineShifts.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineSimplifyDemanded.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineVectorOps.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstructionCombining.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/ADCE.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/CodeGenPrepare.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/DeadStoreElimination.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/GVN.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/LICM.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/LoopStrengthReduce.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/Reassociate.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/Reg2Mem.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/SCCP.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/ScalarReplAggregates.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/SimplifyCFGPass.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/AddrModeMatcher.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/BasicBlockUtils.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/BreakCriticalEdges.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/BuildLibCalls.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/DemoteRegToStack.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/InstructionNamer.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LCSSA.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/Local.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LoopSimplify.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LowerInvoke.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LowerSwitch.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/PromoteMemoryToRegister.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/SSAUpdater.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/SimplifyCFG.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/UnifyFunctionExitNodes.cpp
    ${LLVM_DIR}/lib/VMCore/AsmWriter.cpp
    ${LLVM_DIR}/lib/VMCore/Attributes.cpp
    ${LLVM_DIR}/lib/VMCore/BasicBlock.cpp
    ${LLVM_DIR}/lib/VMCore/ConstantFold.cpp
    ${LLVM_DIR}/lib/VMCore/Constants.cpp
    ${LLVM_DIR}/lib/VMCore/DebugInfoProbe.cpp
    ${LLVM_DIR}/lib/VMCore/DebugLoc.cpp
    ${LLVM_DIR}/lib/VMCore/Dominators.cpp
    ${LLVM_DIR}/lib/VMCore/Function.cpp
    ${LLVM_DIR}/lib/VMCore/Globals.cpp
    ${LLVM_DIR}/lib/VMCore/IRBuilder.cpp
    ${LLVM_DIR}/lib/VMCore/InlineAsm.cpp
    ${LLVM_DIR}/lib/VMCore/Instruction.cpp
    ${LLVM_DIR}/lib/VMCore/Instructions.cpp
    ${LLVM_DIR}/lib/VMCore/IntrinsicInst.cpp
    ${LLVM_DIR}/lib/VMCore/LLVMContext.cpp
    ${LLVM_DIR}/lib/VMCore/LLVMContextImpl.cpp
    ${LLVM_DIR}/lib/VMCore/LeakDetector.cpp
    ${LLVM_DIR}/lib/VMCore/Metadata.cpp
    ${LLVM_DIR}/lib/VMCore/Module.cpp
    ${LLVM_DIR}/lib/VMCore/Pass.cpp
    ${LLVM_DIR}/lib/VMCore/PassManager.cpp
    ${LLVM_DIR}/lib/VMCore/PassRegistry.cpp
    ${LLVM_DIR}/lib/VMCore/PrintModulePass.cpp
    ${LLVM_DIR}/lib/VMCore/Type.cpp
    ${LLVM_DIR}/lib/VMCore/Use.cpp
    ${LLVM_DIR}/lib/VMCore/User.cpp
    ${LLVM_DIR}/lib/VMCore/Value.cpp
    ${LLVM_DIR}/lib/VMCore/ValueSymbolTable.cpp
    ${LLVM_DIR}/lib/VMCore/ValueTypes.cpp
    ${LLVM_DIR}/lib/VMCore/Verifier.cpp
)

if(WIN32)
    set(LLVM_INCLUDE_DIR ${LLVM_DIR}/include)
elseif(LINUX)
    set(LLVM_INCLUDE_DIR ${LLVM_DIR}/include-linux)
elseif(APPLE)
    set(LLVM_INCLUDE_DIR ${LLVM_DIR}/include-osx)
endif()

list(APPEND LLVM_INCLUDE_DIR
    ${LLVM_DIR}/include
    ${LLVM_DIR}/lib/Target/X86
)
add_library(llvm STATIC ${LLVM_LIST})
set_target_properties(llvm PROPERTIES
    POSITION_INDEPENDENT_CODE 1
    INCLUDE_DIRECTORIES "${LLVM_INCLUDE_DIR}"
    COMPILE_DEFINITIONS "__STDC_CONSTANT_MACROS; __STDC_LIMIT_MACROS;"
    FOLDER "LLVM"
)

###########################################################
# Subzero
###########################################################

if(${REACTOR_BACKEND} STREQUAL "Subzero")
    set(SUBZERO_LIST
        ${SUBZERO_DIR}/src/IceAssembler.cpp
        ${SUBZERO_DIR}/src/IceCfg.cpp
        ${SUBZERO_DIR}/src/IceCfgNode.cpp
        ${SUBZERO_DIR}/src/IceClFlags.cpp
        ${SUBZERO_DIR}/src/IceELFObjectWriter.cpp
        ${SUBZERO_DIR}/src/IceELFSection.cpp
        ${SUBZERO_DIR}/src/IceFixups.cpp
        ${SUBZERO_DIR}/src/IceGlobalContext.cpp
        ${SUBZERO_DIR}/src/IceGlobalInits.cpp
        ${SUBZERO_DIR}/src/IceInst.cpp
        ${SUBZERO_DIR}/src/IceInstrumentation.cpp
        ${SUBZERO_DIR}/src/IceIntrinsics.cpp
        ${SUBZERO_DIR}/src/IceLiveness.cpp
        ${SUBZERO_DIR}/src/IceLoopAnalyzer.cpp
        ${SUBZERO_DIR}/src/IceMangling.cpp
        ${SUBZERO_DIR}/src/IceMemory.cpp
        ${SUBZERO_DIR}/src/IceOperand.cpp
        ${SUBZERO_DIR}/src/IceRangeSpec.cpp
        ${SUBZERO_DIR}/src/IceRegAlloc.cpp
        ${SUBZERO_DIR}/src/IceRevision.cpp
        ${SUBZERO_DIR}/src/IceRNG.cpp
        ${SUBZERO_DIR}/src/IceSwitchLowering.cpp
        ${SUBZERO_DIR}/src/IceTargetLowering.cpp
        ${SUBZERO_DIR}/src/IceThreading.cpp
        ${SUBZERO_DIR}/src/IceTimerTree.cpp
        ${SUBZERO_DIR}/src/IceTypes.cpp
        ${SUBZERO_DIR}/src/IceVariableSplitting.cpp
    )

    if(ARCH STREQUAL "x86_64")
        list(APPEND SUBZERO_LIST
            ${SUBZERO_DIR}/src/IceTargetLoweringX86.cpp
            ${SUBZERO_DIR}/src/IceInstX8664.cpp
            ${SUBZERO_DIR}/src/IceTargetLoweringX8664.cpp
        )
        set(SUBZERO_TARGET X8664)
    elseif(ARCH STREQUAL "x86")
        list(APPEND SUBZERO_LIST
            ${SUBZERO_DIR}/src/IceTargetLoweringX86.cpp
            ${SUBZERO_DIR}/src/IceInstX8632.cpp
            ${SUBZERO_DIR}/src/IceTargetLoweringX8632.cpp
        )
        set(SUBZERO_TARGET X8632)
    elseif(ARCH STREQUAL "arm")
        list(APPEND SUBZERO_LIST
            ${SUBZERO_DIR}/src/IceAssemblerARM32.cpp
            ${SUBZERO_DIR}/src/IceInstARM32.cpp
            ${SUBZERO_DIR}/src/IceTargetLoweringARM32.cpp
        )
        set(SUBZERO_TARGET ARM32)
    else()
        message(FATAL_ERROR "Architecture '${ARCH}' not supported by Subzero")
    endif()

    file(GLOB_RECURSE SUBZERO_DEPENDENCIES_LIST
        ${SUBZERO_LLVM_DIR}/*.cpp
        ${SUBZERO_LLVM_DIR}/*.c
        ${SUBZERO_LLVM_DIR}/*.h
    )

    set(SUBZERO_REACTOR_LIST
        ${SOURCE_DIR}/Reactor/SubzeroReactor.cpp
        ${SOURCE_DIR}/Reactor/Routine.cpp
        ${SOURCE_DIR}/Reactor/Optimizer.cpp
        ${SOURCE_DIR}/Reactor/Nucleus.hpp
        ${SOURCE_DIR}/Reactor/Routine.hpp
    )

    set(SUBZERO_INCLUDE_DIR
        ${SUBZERO_DIR}/
        ${SUBZERO_LLVM_DIR}/include/
        ${SUBZERO_DIR}/pnacl-llvm/include/
     )

    if(WIN32)
        list(APPEND SUBZERO_INCLUDE_DIR ${SUBZERO_LLVM_DIR}/build/Windows/include/)
    elseif(LINUX)
        list(APPEND SUBZERO_INCLUDE_DIR ${SUBZERO_LLVM_DIR}/build/Linux/include/)
    elseif(APPLE)
        list(APPEND SUBZERO_INCLUDE_DIR ${SUBZERO_LLVM_DIR}/build/MacOS/include/)
    endif()

    add_library(ReactorSubzero STATIC
        ${SUBZERO_LIST}
        ${SUBZERO_DEPENDENCIES_LIST}
        ${SUBZERO_REACTOR_LIST}
    )

    set_target_properties(ReactorSubzero PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        INCLUDE_DIRECTORIES "${SUBZERO_INCLUDE_DIR}"
        COMPILE_DEFINITIONS "SZTARGET=${SUBZERO_TARGET}; ALLOW_DUMP=0; ALLOW_TIMERS=0; ALLOW_LLVM_CL=0; ALLOW_LLVM_IR=0; ALLOW_LLVM_IR_AS_INPUT=0; ALLOW_MINIMAL_BUILD=0; ALLOW_WASM=0; ICE_THREAD_LOCAL_HACK=0;"
        FOLDER "Subzero"
    )

    if(WIN32)
        target_compile_definitions(ReactorSubzero PRIVATE SUBZERO_USE_MICROSOFT_ABI)
    endif()
endif()

###########################################################
# Include Directories
###########################################################

set(COMMON_INCLUDE_DIR
    ${SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIR}
)
set(OPENGL_INCLUDE_DIR
    ${OPENGL_DIR}
    ${COMMON_INCLUDE_DIR}
)

###########################################################
# File Lists
###########################################################

file(GLOB_RECURSE SWIFTSHADER_LIST
    ${SOURCE_DIR}/Common/*.cpp
    ${SOURCE_DIR}/Common/*.h
    ${SOURCE_DIR}/Common/*.hpp
    ${SOURCE_DIR}/Renderer/*.cpp
    ${SOURCE_DIR}/Renderer/*.h
    ${SOURCE_DIR}/Renderer/*.hpp
    ${SOURCE_DIR}/Shader/*.cpp
    ${SOURCE_DIR}/Shader/*.h
    ${SOURCE_DIR}/Shader/*.hpp
)
list(APPEND SWIFTSHADER_LIST
    ${SOURCE_DIR}/Main/Config.cpp
    ${SOURCE_DIR}/Main/Config.hpp
    ${SOURCE_DIR}/Main/FrameBuffer.cpp
    ${SOURCE_DIR}/Main/FrameBuffer.hpp
    ${SOURCE_DIR}/Main/SwiftConfig.cpp
    ${SOURCE_DIR}/Main/SwiftConfig.hpp
)
list(REMOVE_ITEM SWIFTSHADER_LIST
    ${SOURCE_DIR}/Common/DebugAndroid.cpp
    ${SOURCE_DIR}/Common/DebugAndroid.hpp
    ${SOURCE_DIR}/Common/GrallocAndroid.cpp
    ${SOURCE_DIR}/Common/GrallocAndroid.hpp
)

set(REACTOR_LLVM_LIST
    ${SOURCE_DIR}/Reactor/LLVMReactor.cpp
    ${SOURCE_DIR}/Reactor/Nucleus.hpp
    ${SOURCE_DIR}/Reactor/Routine.cpp
    ${SOURCE_DIR}/Reactor/Routine.hpp
    ${SOURCE_DIR}/Reactor/LLVMRoutine.cpp
    ${SOURCE_DIR}/Reactor/LLVMRoutine.hpp
    ${SOURCE_DIR}/Reactor/LLVMRoutineManager.cpp
    ${SOURCE_DIR}/Reactor/LLVMRoutineManager.hpp
)

file(GLOB_RECURSE EGL_LIST
    ${OPENGL_DIR}/libEGL/*.cpp
    ${OPENGL_DIR}/libEGL/*.h
    ${OPENGL_DIR}/libEGL/*.hpp
    ${OPENGL_DIR}/common/Object.cpp
    ${OPENGL_DIR}/common/Object.hpp
    ${OPENGL_DIR}/common/debug.cpp
    ${OPENGL_DIR}/common/debug.h
    ${CMAKE_SOURCE_DIR}/include/*.h
)

file(GLOB_RECURSE GL32_LIST
    ${OPENGL_DIR}/libGL/*.cpp
    ${OPENGL_DIR}/libGL/*.h
    ${OPENGL_DIR}/libGL/*.hpp
    ${OPENGL_DIR}/common/*.cpp
    ${OPENGL_DIR}/common/*.h
    ${OPENGL_DIR}/common/*.hpp
    ${CMAKE_SOURCE_DIR}/include/GL/*.h
)

file(GLOB_RECURSE GLES2_LIST
    ${OPENGL_DIR}/libGLESv2/*.cpp
    ${OPENGL_DIR}/libGLESv2/*.h
    ${OPENGL_DIR}/libGLESv2/*.hpp
    ${OPENGL_DIR}/common/*.cpp
    ${OPENGL_DIR}/common/*.h
    ${OPENGL_DIR}/common/*.hpp
    ${CMAKE_SOURCE_DIR}/include/KHR/*.h
    ${CMAKE_SOURCE_DIR}/include/GLES2/*.h
    ${CMAKE_SOURCE_DIR}/include/GLES3/*.h
)

file(GLOB_RECURSE GLES_CM_LIST
    ${OPENGL_DIR}/libGLES_CM/*.cpp
    ${OPENGL_DIR}/libGLES_CM/*.h
    ${OPENGL_DIR}/libGLES_CM/*.hpp
    ${OPENGL_DIR}/common/*.cpp
    ${OPENGL_DIR}/common/*.h
    ${OPENGL_DIR}/common/*.hpp
    ${CMAKE_SOURCE_DIR}/include/KHR/*.h
    ${CMAKE_SOURCE_DIR}/include/GLES/*.h
)

file(GLOB_RECURSE OPENGL_COMPILER_LIST
    ${OPENGL_COMPILER_DIR}/*.cpp
    ${OPENGL_COMPILER_DIR}/*.h
    ${OPENGL_COMPILER_DIR}/*.hpp
)
file(GLOB_RECURSE OPENGL_COMPILER_OS_SPECIFIC_LIST
    ${OPENGL_COMPILER_DIR}/ossource*.cpp
    ${OPENGL_COMPILER_DIR}/ossource*.h
    ${OPENGL_COMPILER_DIR}/ossource*.hpp
)
list(REMOVE_ITEM OPENGL_COMPILER_LIST ${OPENGL_COMPILER_OS_SPECIFIC_LIST})

###########################################################
# Append OS specific files to lists
###########################################################

if(WIN32)
    list(APPEND SWIFTSHADER_LIST
        ${SOURCE_DIR}/Main/FrameBufferDD.cpp
        ${SOURCE_DIR}/Main/FrameBufferDD.hpp
        ${SOURCE_DIR}/Main/FrameBufferGDI.cpp
        ${SOURCE_DIR}/Main/FrameBufferGDI.hpp
        ${SOURCE_DIR}/Main/FrameBufferWin.cpp
        ${SOURCE_DIR}/Main/FrameBufferWin.hpp
    )
    list(APPEND OPENGL_COMPILER_LIST ${OPENGL_COMPILER_DIR}/ossource_win.cpp)
    list(APPEND EGL_LIST ${OPENGL_DIR}/libEGL/libEGL.rc)
    list(APPEND GL32_LIST ${OPENGL_DIR}/libGL/libGL.rc)
    list(APPEND GLES2_LIST ${OPENGL_DIR}/libGLESv2/libGLESv2.rc)
    list(APPEND GLES_CM_LIST ${OPENGL_DIR}/libGLES_CM/libGLES_CM.rc)
elseif(LINUX)
    list(APPEND SWIFTSHADER_LIST
        ${SOURCE_DIR}/Main/FrameBufferX11.cpp
        ${SOURCE_DIR}/Main/FrameBufferX11.hpp
        ${SOURCE_DIR}/Common/SharedLibrary.hpp
        ${SOURCE_DIR}/Main/libX11.cpp
        ${SOURCE_DIR}/Main/libX11.hpp
    )
    list(APPEND EGL_LIST
        ${SOURCE_DIR}/Common/SharedLibrary.hpp
        ${SOURCE_DIR}/Main/libX11.cpp
        ${SOURCE_DIR}/Main/libX11.hpp
    )
    list(APPEND OPENGL_COMPILER_LIST
        ${OPENGL_COMPILER_DIR}/ossource_posix.cpp
    )
elseif(APPLE)
    list(APPEND SWIFTSHADER_LIST
        ${SOURCE_DIR}/Main/FrameBufferOSX.mm
        ${SOURCE_DIR}/Main/FrameBufferOSX.hpp
    )
    list(APPEND EGL_LIST
        ${OPENGL_DIR}/libEGL/OSXUtils.mm
        ${OPENGL_DIR}/libEGL/OSXUtils.hpp
    )
    list(APPEND OPENGL_COMPILER_LIST
        ${OPENGL_COMPILER_DIR}/ossource_posix.cpp
    )
endif()

if(WIN32)
    set(OS_LIBS odbc32 odbccp32 WS2_32 dxguid)
elseif(LINUX)
    set(OS_LIBS dl pthread)
elseif(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(QUARTZ_FRAMEWORK Quartz)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(IOSURFACE_FRAMEWORK IOSurface)
    set(OS_LIBS "${COCOA_FRAMEWORK}" "${QUARTZ_FRAMEWORK}" "${CORE_FOUNDATION_FRAMEWORK}" "${IOSURFACE_FRAMEWORK}")
endif()

###########################################################
# SwiftShader Targets
###########################################################

add_library(SwiftShader STATIC ${SWIFTSHADER_LIST})
set_target_properties(SwiftShader PROPERTIES
    INCLUDE_DIRECTORIES "${COMMON_INCLUDE_DIR}"
    POSITION_INDEPENDENT_CODE 1
    FOLDER "Core"
    COMPILE_DEFINITIONS "NO_SANITIZE_FUNCTION=;"
)
target_link_libraries(SwiftShader ${OS_LIBS})

add_library(ReactorLLVM STATIC ${REACTOR_LLVM_LIST})
set_target_properties(ReactorLLVM PROPERTIES
    INCLUDE_DIRECTORIES "${COMMON_INCLUDE_DIR}"
    POSITION_INDEPENDENT_CODE 1
    FOLDER "Core"
)
target_link_libraries(ReactorLLVM llvm ${OS_LIBS})

if(${REACTOR_BACKEND} STREQUAL "LLVM")
    set(Reactor ReactorLLVM)
elseif(${REACTOR_BACKEND} STREQUAL "Subzero")
    set(Reactor ReactorSubzero)
else()
    message(FATAL_ERROR "REACTOR_BACKEND must be 'LLVM' or 'Subzero'")
endif()

add_library(GLCompiler STATIC ${OPENGL_COMPILER_LIST})
set_target_properties(GLCompiler PROPERTIES
    INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
    POSITION_INDEPENDENT_CODE 1
    FOLDER "OpenGL"
)
target_link_libraries(GLCompiler ${OS_LIBS})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIB_PREFIX "lib64")
else()
    set(LIB_PREFIX "lib")
endif()

if(BUILD_EGL)
    add_library(libEGL SHARED ${EGL_LIST})
    set_target_properties(libEGL PROPERTIES
        INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
        FOLDER "OpenGL"
        COMPILE_DEFINITIONS "EGL_EGLEXT_PROTOTYPES; EGLAPI=; NO_SANITIZE_FUNCTION=;"
        PREFIX ""
    )
    set_target_export_map(libEGL ${SOURCE_DIR}/OpenGL/libEGL)
    target_link_libraries(libEGL ${OS_LIBS})
    add_custom_command(
        TARGET libEGL
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:libEGL>/translator
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libEGL> $<TARGET_FILE_DIR:libEGL>/translator/${LIB_PREFIX}EGL_translator${CMAKE_SHARED_LIBRARY_SUFFIX}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/out/${CMAKE_SYSTEM_NAME}/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libEGL> ${CMAKE_SOURCE_DIR}/out/${CMAKE_SYSTEM_NAME}/
    )
endif()

if(BUILD_GL32)
    add_library(libGL SHARED ${GL32_LIST})
    set_target_properties(libGL PROPERTIES
        INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
        FOLDER "OpenGL"
        COMPILE_DEFINITIONS "GL_GLEXT_PROTOTYPES"
        PREFIX ""
    )
    set_target_export_map(libGL ${SOURCE_DIR}/OpenGL/libGL)
    target_link_libraries(libGL SwiftShader ${Reactor} GLCompiler ${OS_LIBS})
endif()

if(BUILD_GLESv2)
    add_library(libGLESv2 SHARED ${GLES2_LIST})
    set_target_properties(libGLESv2 PROPERTIES
        INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
        FOLDER "OpenGL"
        COMPILE_DEFINITIONS "GL_GLEXT_PROTOTYPES; GL_API=; GL_APICALL=; NO_SANITIZE_FUNCTION=;"
        PREFIX ""
    )
    set_target_export_map(libGLESv2 ${SOURCE_DIR}/OpenGL/libGLESv2)
    target_link_libraries(libGLESv2 SwiftShader ${Reactor} GLCompiler ${OS_LIBS})
    add_custom_command(
        TARGET libGLESv2
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:libGLESv2>/translator
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libGLESv2> $<TARGET_FILE_DIR:libGLESv2>/translator/${LIB_PREFIX}GLES_V2_translator${CMAKE_SHARED_LIBRARY_SUFFIX}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/out/${CMAKE_SYSTEM_NAME}/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libGLESv2> ${CMAKE_SOURCE_DIR}/out/${CMAKE_SYSTEM_NAME}/
    )
endif()

if(BUILD_GLES_CM)
    add_library(libGLES_CM SHARED ${GLES_CM_LIST})
    set_target_properties(libGLES_CM PROPERTIES
        INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
        FOLDER "OpenGL"
        COMPILE_DEFINITIONS "GL_GLEXT_PROTOTYPES; EGLAPI=; GL_API=; GL_APICALL=;"
        PREFIX ""
    )
    set_target_export_map(libGLES_CM ${SOURCE_DIR}/OpenGL/libGLES_CM)
    target_link_libraries(libGLES_CM SwiftShader ${Reactor} GLCompiler ${OS_LIBS})
    add_custom_command(
        TARGET libGLES_CM
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:libGLES_CM>/translator
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libGLES_CM> $<TARGET_FILE_DIR:libGLES_CM>/translator/${LIB_PREFIX}GLES_CM_translator${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
endif()

###########################################################
# Sample programs
###########################################################

if(BUILD_SAMPLES)
    if(WIN32)
        add_executable(OGLES2HelloAPI ${HELLO2_DIR}/OGLES2HelloAPI_Windows.cpp)
        set_target_properties(OGLES2HelloAPI PROPERTIES
            INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
            COMPILE_DEFINITIONS "GL_GLEXT_PROTOTYPES"
            FOLDER "Samples"
        )
        target_link_libraries(OGLES2HelloAPI libEGL libGLESv2)
        set_property(TARGET OGLES2HelloAPI APPEND_STRING PROPERTY LINK_FLAGS "/SUBSYSTEM:WINDOWS")
        set(CMAKE_DEFAULT_STARTUP_PROJECT OGLES2HelloAPI)
    elseif(LINUX)
        add_executable(OGLES2HelloAPI ${HELLO2_DIR}/OGLES2HelloAPI_LinuxX11.cpp)
        set_target_properties(OGLES2HelloAPI PROPERTIES
            INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
            COMPILE_DEFINITIONS "GL_GLEXT_PROTOTYPES"
        )
        target_link_libraries(OGLES2HelloAPI dl X11 libEGL libGLESv2)   # Explicitly link our "lib*" targets, not the platform provided "EGL" and "GLESv2"
    elseif(APPLE)
        add_executable(OGLES2HelloAPI MACOSX_BUNDLE
            ${HELLO2_DIR}/OGLES2HelloAPI_OSX.mm
            ${HELLO2_DIR}/Build/OSX/en.lproj/MainMenu.xib
        )
        set_target_properties(OGLES2HelloAPI PROPERTIES
            INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
            COMPILE_DEFINITIONS "GL_GLEXT_PROTOTYPES"
            MACOSX_BUNDLE_INFO_PLIST "${HELLO2_DIR}/Build/OSX/Info.plist"
        )
        target_link_libraries(OGLES2HelloAPI libEGL libGLESv2 ${OS_LIBS})
        set_source_files_properties(${HELLO2_DIR}/Build/OSX/en.lproj/MainMenu.xib PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
    endif()
endif()

if(BUILD_TESTS AND ${REACTOR_BACKEND} STREQUAL "Subzero")
    set(SUBZERO_TEST_LIST
        ${SOURCE_DIR}/Reactor/Main.cpp
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/src/gtest-all.cc
    )

    set(SUBZERO_TEST_INCLUDE_DIR
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/
    )

    add_executable(SubzeroTest ${SUBZERO_TEST_LIST})
    set_target_properties(SubzeroTest PROPERTIES
        INCLUDE_DIRECTORIES "${SUBZERO_TEST_INCLUDE_DIR}"
        FOLDER "Tests"
    )
    if(WIN32)
        target_link_libraries(SubzeroTest ReactorSubzero)
    else()
        target_link_libraries(SubzeroTest ReactorSubzero pthread dl)
    endif()
endif()
