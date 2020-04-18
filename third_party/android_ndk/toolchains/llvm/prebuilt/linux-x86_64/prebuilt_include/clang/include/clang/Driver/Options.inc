/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Option Parsing Definitions                                                 *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/////////
// Prefixes

#ifdef PREFIX
#define COMMA ,
PREFIX(prefix_0, {nullptr})
PREFIX(prefix_1, {"-" COMMA nullptr})
PREFIX(prefix_4, {"-" COMMA "--" COMMA nullptr})
PREFIX(prefix_3, {"--" COMMA nullptr})
PREFIX(prefix_2, {"/" COMMA "-" COMMA nullptr})
#undef COMMA
#endif // PREFIX

/////////
// Groups

#ifdef OPTION
OPTION(nullptr, "<action group>", Action_Group, Group, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<CompileOnly group>", CompileOnly_Group, Group, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<g group>", DebugInfo_Group, Group, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<W/R group>", Diag_Group, Group, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<I group>", I_Group, Group, IncludePath_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<I/i group>", IncludePath_Group, Group, Preprocessor_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<T/e/s/t/u group>", Link_Group, Group, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<M group>", M_Group, Group, Preprocessor_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<O group>", O_Group, Group, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<Preprocessor group>", Preprocessor_Group, Group, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<R group>", R_Group, Group, Diag_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<R (with value) group>", R_value_Group, Group, R_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<Static analyzer group>", StaticAnalyzer_Group, Group, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<T group>", T_Group, Group, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<W group>", W_Group, Group, Diag_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<W (with value) group>", W_value_Group, Group, W_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "</M group>", _SLASH_M_Group, Group, cl_compile_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "</volatile group>", _SLASH_volatile_Group, Group, cl_compile_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang-cl options>", cl_Group, Group, INVALID, INVALID, nullptr, 0, 0,
       "CL.EXE COMPATIBILITY OPTIONS", nullptr)
OPTION(nullptr, "<clang-cl compile-only options>", cl_compile_Group, Group, cl_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang-cl ignored options>", cl_ignored_Group, Group, cl_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang i group>", clang_i_Group, Group, i_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang ignored f group>", clang_ignored_f_Group, Group, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang_ignored_gcc_optimization_f_Group>", clang_ignored_gcc_optimization_f_Group, Group, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang ignored m group>", clang_ignored_m_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<d group>", d_Group, Group, Preprocessor_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<f group>", f_Group, Group, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<f (clang-only) group>", f_clang_Group, Group, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<gN group>", gN_Group, Group, g_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<gTune group>", gTune_Group, Group, g_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<g group>", g_Group, Group, DebugInfo_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<g flags group>", g_flags_Group, Group, DebugInfo_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<gfortran group>", gfortran_Group, Group, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<ggdbN group>", ggdbN_Group, Group, gN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<i group>", i_Group, Group, IncludePath_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang internal options>", internal_Group, Group, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<clang debug/development internal options>", internal_debug_Group, Group, internal_Group, INVALID, nullptr, 0, 0,
       "DEBUG/DEVELOPMENT OPTIONS", nullptr)
OPTION(nullptr, "<clang driver internal options>", internal_driver_Group, Group, internal_Group, INVALID, nullptr, 0, 0,
       "DRIVER OPTIONS", nullptr)
OPTION(nullptr, "<m group>", m_Group, Group, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<aarch64 features group>", m_aarch64_Features_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<amdgpu features group>", m_amdgpu_Features_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<arm features group>", m_arm_Features_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<hexagon features group>", m_hexagon_Features_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<m libc group>", m_libc_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<ppc features group>", m_ppc_Features_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<wasm features group>", m_wasm_Features_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<x86 features group>", m_x86_Features_Group, Group, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<opencl group>", opencl_Group, Group, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<pedantic group>", pedantic_Group, Group, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<reserved libs group>", reserved_lib_Group, Group, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(nullptr, "<u group>", u_Group, Group, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)

//////////
// Options

OPTION(prefix_0, "<input>", INPUT, Input, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_0, "<unknown>", UNKNOWN, Unknown, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "###", _HASH_HASH_HASH, Flag, INVALID, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Print (but do not run) the commands to run for this compilation", nullptr)
OPTION(prefix_2, "?", _SLASH_QUESTION, Flag, cl_Group, help, nullptr, CLOption | DriverOption | CLOption, 0,
       "Display available options", nullptr)
OPTION(prefix_1, "A-", A_DASH, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "add-plugin", add_plugin, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use the named plugin action in addition to the default action", "<name>")
OPTION(prefix_2, "AI", _SLASH_AI, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_3, "all-warnings", _all_warnings, Flag, INVALID, Wall, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "all_load", all__load, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "allowable_client", allowable__client, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "analyze-auto", _analyze_auto, Flag, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyze-function=", analyze_function_EQ, Joined, INVALID, analyze_function, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyze-function", analyze_function, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Run analysis on specific function", nullptr)
OPTION(prefix_2, "analyze-", _SLASH_analyze_, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-checker-help", analyzer_checker_help, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Display the list of analyzer checkers that are available", nullptr)
OPTION(prefix_1, "analyzer-checker=", analyzer_checker_EQ, Joined, INVALID, analyzer_checker, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-checker", analyzer_checker, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Choose analyzer checkers to enable", nullptr)
OPTION(prefix_1, "analyzer-config", analyzer_config, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Choose analyzer options to enable", nullptr)
OPTION(prefix_1, "analyzer-constraints=", analyzer_constraints_EQ, Joined, INVALID, analyzer_constraints, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-constraints", analyzer_constraints, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Source Code Analysis - Symbolic Constraint Engines", nullptr)
OPTION(prefix_1, "analyzer-disable-all-checks", analyzer_disable_all_checks, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable all static analyzer checks", nullptr)
OPTION(prefix_1, "analyzer-disable-checker=", analyzer_disable_checker_EQ, Joined, INVALID, analyzer_disable_checker, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-disable-checker", analyzer_disable_checker, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Choose analyzer checkers to disable", nullptr)
OPTION(prefix_1, "analyzer-disable-retry-exhausted", analyzer_disable_retry_exhausted, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not re-analyze paths leading to exhausted nodes with a different strategy (may decrease code coverage)", nullptr)
OPTION(prefix_1, "analyzer-display-progress", analyzer_display_progress, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit verbose output about the analyzer's progress", nullptr)
OPTION(prefix_1, "analyzer-eagerly-assume", analyzer_eagerly_assume, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Eagerly assume the truth/falseness of some symbolic constraints", nullptr)
OPTION(prefix_1, "analyzer-inline-max-stack-depth=", analyzer_inline_max_stack_depth_EQ, Joined, INVALID, analyzer_inline_max_stack_depth, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-inline-max-stack-depth", analyzer_inline_max_stack_depth, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Bound on stack depth while inlining (4 by default)", nullptr)
OPTION(prefix_1, "analyzer-inlining-mode=", analyzer_inlining_mode_EQ, Joined, INVALID, analyzer_inlining_mode, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-inlining-mode", analyzer_inlining_mode, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Specify the function selection heuristic used during inlining", nullptr)
OPTION(prefix_1, "analyzer-list-enabled-checkers", analyzer_list_enabled_checkers, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Display the list of enabled analyzer checkers", nullptr)
OPTION(prefix_1, "analyzer-max-loop", analyzer_max_loop, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "The maximum number of times the analyzer will go through a loop", nullptr)
OPTION(prefix_3, "analyzer-no-default-checks", _analyzer_no_default_checks, Flag, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-opt-analyze-headers", analyzer_opt_analyze_headers, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Force the static analyzer to analyze functions defined in header files", nullptr)
OPTION(prefix_1, "analyzer-opt-analyze-nested-blocks", analyzer_opt_analyze_nested_blocks, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Analyze the definitions of blocks in addition to functions", nullptr)
OPTION(prefix_1, "analyzer-output=", analyzer_output_EQ, Joined, INVALID, analyzer_output, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_3, "analyzer-output", _analyzer_output, JoinedOrSeparate, INVALID, INVALID, nullptr, DriverOption, 0,
       "Static analyzer report output format (html|plist|plist-multi-file|plist-html|text).", nullptr)
OPTION(prefix_1, "analyzer-output", analyzer_output, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Source Code Analysis - Output Options", nullptr)
OPTION(prefix_1, "analyzer-purge=", analyzer_purge_EQ, Joined, INVALID, analyzer_purge, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-purge", analyzer_purge, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Source Code Analysis - Dead Symbol Removal Frequency", nullptr)
OPTION(prefix_1, "analyzer-stats", analyzer_stats, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Print internal analyzer statistics.", nullptr)
OPTION(prefix_1, "analyzer-store=", analyzer_store_EQ, Joined, INVALID, analyzer_store, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "analyzer-store", analyzer_store, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Source Code Analysis - Abstract Memory Store Models", nullptr)
OPTION(prefix_1, "analyzer-viz-egraph-graphviz", analyzer_viz_egraph_graphviz, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Display exploded graph using GraphViz", nullptr)
OPTION(prefix_1, "analyzer-viz-egraph-ubigraph", analyzer_viz_egraph_ubigraph, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Display exploded graph using Ubigraph", nullptr)
OPTION(prefix_3, "analyze", _analyze, Flag, INVALID, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Run the static analyzer", nullptr)
OPTION(prefix_1, "analyze", analyze, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Run static analysis engine", nullptr)
OPTION(prefix_4, "ansi", ansi, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "arange_sections", arange_sections, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit DWARF .debug_arange sections", nullptr)
OPTION(prefix_2, "arch:", _SLASH_arch, Joined, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Set architecture for code generation", nullptr)
OPTION(prefix_1, "arch_errors_fatal", arch__errors__fatal, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "arch_only", arch__only, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "arch", arch, Separate, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "arcmt-check", arcmt_check, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Check for ARC migration issues that need manual handling", nullptr)
OPTION(prefix_1, "arcmt-migrate-emit-errors", arcmt_migrate_emit_arc_errors, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Emit ARC errors even if the migrator can fix them", nullptr)
OPTION(prefix_1, "arcmt-migrate-report-output", arcmt_migrate_report_output, Separate, INVALID, INVALID, nullptr, CC1Option, 0,
       "Output path for the plist report", nullptr)
OPTION(prefix_1, "arcmt-migrate", arcmt_migrate, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply modifications and produces temporary files that conform to ARC", nullptr)
OPTION(prefix_1, "arcmt-modify", arcmt_modify, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply modifications to files to conform to ARC", nullptr)
OPTION(prefix_3, "assemble", _assemble, Flag, INVALID, S, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "assert=", _assert_EQ, Joined, INVALID, A, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "assert", _assert, Separate, INVALID, A, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ast-dump-all", ast_dump_all, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs and then debug dump them, forcing deserialization", nullptr)
OPTION(prefix_1, "ast-dump-filter", ast_dump_filter, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use with -ast-dump or -ast-print to dump/print only AST declaration nodes having a certain substring in a qualified name. Use -ast-list to list all filterable declaration node names.", "<dump_filter>")
OPTION(prefix_1, "ast-dump-lookups", ast_dump_lookups, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs and then debug dump their name lookup tables", nullptr)
OPTION(prefix_1, "ast-dump", ast_dump, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs and then debug dump them", nullptr)
OPTION(prefix_1, "ast-list", ast_list, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs and print the list of declaration node qualified names", nullptr)
OPTION(prefix_1, "ast-merge", ast_merge, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Merge the given AST file into the translation unit being compiled.", "<ast file>")
OPTION(prefix_1, "ast-print", ast_print, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs and then pretty-print them", nullptr)
OPTION(prefix_1, "ast-view", ast_view, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs and view them with GraphViz", nullptr)
OPTION(prefix_1, "aux-triple", aux_triple, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Auxiliary target triple.", nullptr)
OPTION(prefix_1, "A", A, JoinedOrSeparate, gfortran_Group, INVALID, nullptr, RenderJoined, 0, nullptr, nullptr)
OPTION(prefix_1, "a", a, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "backend-option", backend_option, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Additional arguments to forward to LLVM backend (during code gen)", nullptr)
OPTION(prefix_2, "bigobj", _SLASH_bigobj, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "bind_at_load", bind__at__load, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "bootclasspath=", _bootclasspath_EQ, Joined, INVALID, fbootclasspath_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "bootclasspath", _bootclasspath, Separate, INVALID, fbootclasspath_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "Brepro-", _SLASH_Brepro_, Flag, cl_Group, mincremental_linker_compatible, nullptr, CLOption | DriverOption | CLOption, 0,
       "Emit an object file which cannot be reproduced over time", nullptr)
OPTION(prefix_2, "Brepro", _SLASH_Brepro, Flag, cl_Group, mno_incremental_linker_compatible, nullptr, CLOption | DriverOption | CLOption, 0,
       "Emit an object file which can be reproduced over time", nullptr)
OPTION(prefix_1, "bundle_loader", bundle__loader, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "bundle", bundle, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "B", B, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0,
       "Add <dir> to search path for binaries and object files used implicitly", "<dir>")
OPTION(prefix_1, "b", b, JoinedOrSeparate, INVALID, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "c-isystem", c_isystem, JoinedOrSeparate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add directory to the C SYSTEM include search path", "<directory>")
OPTION(prefix_1, "cc1as", cc1as, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "cc1", cc1, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "ccc-arcmt-check", ccc_arcmt_check, Flag, internal_driver_Group, INVALID, nullptr, DriverOption | HelpHidden, 0,
       "Check for ARC migration issues that need manual handling", nullptr)
OPTION(prefix_1, "ccc-arcmt-migrate", ccc_arcmt_migrate, Separate, internal_driver_Group, INVALID, nullptr, DriverOption | HelpHidden, 0,
       "Apply modifications and produces temporary files that conform to ARC", nullptr)
OPTION(prefix_1, "ccc-arcmt-modify", ccc_arcmt_modify, Flag, internal_driver_Group, INVALID, nullptr, DriverOption | HelpHidden, 0,
       "Apply modifications to files to conform to ARC", nullptr)
OPTION(prefix_1, "ccc-gcc-name", ccc_gcc_name, Separate, internal_driver_Group, INVALID, nullptr, DriverOption | HelpHidden, 0,
       "Name for native GCC compiler", "<gcc-path>")
OPTION(prefix_1, "ccc-install-dir", ccc_install_dir, Separate, internal_debug_Group, INVALID, nullptr, DriverOption | HelpHidden | CoreOption, 0,
       "Simulate installation in the given directory", nullptr)
OPTION(prefix_1, "ccc-objcmt-migrate", ccc_objcmt_migrate, Separate, internal_driver_Group, INVALID, nullptr, DriverOption | HelpHidden, 0,
       "Apply modifications and produces temporary files to migrate to modern ObjC syntax", nullptr)
OPTION(prefix_1, "ccc-pch-is-pch", ccc_pch_is_pch, Flag, internal_driver_Group, INVALID, nullptr, DriverOption | HelpHidden, 0,
       "Use lazy PCH for precompiled headers", nullptr)
OPTION(prefix_1, "ccc-pch-is-pth", ccc_pch_is_pth, Flag, internal_driver_Group, INVALID, nullptr, DriverOption | HelpHidden, 0,
       "Use pretokenized headers for precompiled headers", nullptr)
OPTION(prefix_1, "ccc-print-bindings", ccc_print_bindings, Flag, internal_debug_Group, INVALID, nullptr, DriverOption | HelpHidden | CoreOption, 0,
       "Show bindings of tools to actions", nullptr)
OPTION(prefix_1, "ccc-print-phases", ccc_print_phases, Flag, internal_debug_Group, INVALID, nullptr, DriverOption | HelpHidden | CoreOption, 0,
       "Dump list of actions to perform", nullptr)
OPTION(prefix_1, "ccc-", ccc_, Joined, internal_Group, INVALID, nullptr, Unsupported | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "CC", CC, Flag, Preprocessor_Group, INVALID, nullptr, CC1Option, 0,
       "Include comments from within macros in preprocessed output", nullptr)
OPTION(prefix_1, "cfg-add-implicit-dtors", analysis_CFGAddImplicitDtors, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add C++ implicit destructors to CFGs for all analyses", nullptr)
OPTION(prefix_2, "cgthreads", _SLASH_cgthreads, Joined, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "chain-include", chain_include, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include and chain a header file after turning it into PCH", "<file>")
OPTION(prefix_1, "cl-denorms-are-zero", cl_denorms_are_zero, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Allow denormals to be flushed to zero.", nullptr)
OPTION(prefix_1, "cl-ext=", cl_ext_EQ, CommaJoined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "OpenCL only. Enable or disable OpenCL extensions. The argument is a comma-separated sequence of one or more extension names, each prefixed by '+' or '-'.", nullptr)
OPTION(prefix_1, "cl-fast-relaxed-math", cl_fast_relaxed_math, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Sets -cl-finite-math-only and -cl-unsafe-math-optimizations, and defines __FAST_RELAXED_MATH__.", nullptr)
OPTION(prefix_1, "cl-finite-math-only", cl_finite_math_only, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Allow floating-point optimizations that assume arguments and results are not NaNs or +-Inf.", nullptr)
OPTION(prefix_1, "cl-fp32-correctly-rounded-divide-sqrt", cl_fp32_correctly_rounded_divide_sqrt, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Specify that single precision floating-point divide and sqrt used in the program source are correctly rounded.", nullptr)
OPTION(prefix_1, "cl-kernel-arg-info", cl_kernel_arg_info, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Generate kernel argument metadata.", nullptr)
OPTION(prefix_1, "cl-mad-enable", cl_mad_enable, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Allow use of less precise MAD computations in the generated binary.", nullptr)
OPTION(prefix_1, "cl-no-signed-zeros", cl_no_signed_zeros, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Allow use of less precise no signed zeros computations in the generated binary.", nullptr)
OPTION(prefix_1, "cl-opt-disable", cl_opt_disable, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. This option disables all optimizations. By default optimizations are enabled.", nullptr)
OPTION(prefix_1, "cl-single-precision-constant", cl_single_precision_constant, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Treat double precision floating-point constant as single precision constant.", nullptr)
OPTION(prefix_1, "cl-std=", cl_std_EQ, Joined, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL language standard to compile for.", nullptr)
OPTION(prefix_1, "cl-strict-aliasing", cl_strict_aliasing, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. This option is added for compatibility with OpenCL 1.0.", nullptr)
OPTION(prefix_1, "cl-unsafe-math-optimizations", cl_unsafe_math_optimizations, Flag, opencl_Group, INVALID, nullptr, CC1Option, 0,
       "OpenCL only. Allow unsafe floating-point optimizations.  Also implies -cl-no-signed-zeros and -cl-mad-enable.", nullptr)
OPTION(prefix_3, "CLASSPATH=", _CLASSPATH_EQ, Joined, INVALID, fclasspath_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "classpath=", _classpath_EQ, Joined, INVALID, fclasspath_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "CLASSPATH", _CLASSPATH, Separate, INVALID, fclasspath_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "classpath", _classpath, Separate, INVALID, fclasspath_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "client_name", client__name, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "clr", _SLASH_clr, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "code-completion-at=", code_completion_at_EQ, Joined, INVALID, code_completion_at, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "code-completion-at", code_completion_at, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Dump code-completion information at a location", "<file>:<line>:<column>")
OPTION(prefix_1, "code-completion-brief-comments", code_completion_brief_comments, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include brief documentation comments in code-completion results.", nullptr)
OPTION(prefix_1, "code-completion-macros", code_completion_macros, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include macros in code-completion results", nullptr)
OPTION(prefix_1, "code-completion-patterns", code_completion_patterns, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include code patterns in code-completion results", nullptr)
OPTION(prefix_4, "combine", combine, Flag, INVALID, INVALID, nullptr, DriverOption | Unsupported, 0, nullptr, nullptr)
OPTION(prefix_3, "comments-in-macros", _comments_in_macros, Flag, INVALID, CC, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "comments", _comments, Flag, INVALID, C, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "compatibility_version", compatibility__version, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "compile", _compile, Flag, INVALID, c, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "compress-debug-sections", compress_debug_sections, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Compress DWARF debug sections using zlib", nullptr)
OPTION(prefix_3, "constant-cfstrings", _constant_cfstrings, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "coverage-cfg-checksum", coverage_cfg_checksum, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit CFG checksum for functions in .gcno files.", nullptr)
OPTION(prefix_1, "coverage-data-file=", coverage_data_file_EQ, Joined, INVALID, coverage_data_file, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "coverage-data-file", coverage_data_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit coverage data to this filename.", nullptr)
OPTION(prefix_1, "coverage-exit-block-before-body", coverage_exit_block_before_body, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit the exit block before the body blocks in .gcno files.", nullptr)
OPTION(prefix_1, "coverage-no-function-names-in-data", coverage_no_function_names_in_data, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit function names in .gcda files.", nullptr)
OPTION(prefix_1, "coverage-notes-file=", coverage_notes_file_EQ, Joined, INVALID, coverage_notes_file, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "coverage-notes-file", coverage_notes_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit coverage notes to this filename.", nullptr)
OPTION(prefix_1, "coverage-version=", coverage_version_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Four-byte version string for gcov files.", nullptr)
OPTION(prefix_4, "coverage", coverage, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "cpp-precomp", cpp_precomp, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "cpp", cpp, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "cuda-compile-host-device", cuda_compile_host_device, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Compile CUDA code for both host and device (default).  Has no effect on non-CUDA compilations.", nullptr)
OPTION(prefix_3, "cuda-device-only", cuda_device_only, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Compile CUDA code for device only", nullptr)
OPTION(prefix_3, "cuda-gpu-arch=", cuda_gpu_arch_EQ, Joined, INVALID, INVALID, nullptr, DriverOption, 0,
       "CUDA GPU architecture (e.g. sm_35).  May be specified more than once.", nullptr)
OPTION(prefix_3, "cuda-host-only", cuda_host_only, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Compile CUDA code for host only.  Has no effect on non-CUDA compilations.", nullptr)
OPTION(prefix_3, "cuda-noopt-device-debug", cuda_noopt_device_debug, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Enable device-side debug info generation. Disables ptxas optimizations.", nullptr)
OPTION(prefix_3, "cuda-path=", cuda_path_EQ, Joined, i_Group, INVALID, nullptr, 0, 0,
       "CUDA installation path", nullptr)
OPTION(prefix_1, "current_version", current__version, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "cxx-isystem", cxx_isystem, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Add directory to the C++ SYSTEM include search path", "<directory>")
OPTION(prefix_1, "C", C, Flag, Preprocessor_Group, INVALID, nullptr, CC1Option, 0,
       "Include comments in preprocessed output", nullptr)
OPTION(prefix_2, "C", _SLASH_C, Flag, cl_Group, C, nullptr, CLOption | DriverOption | CLOption, 0,
       "Don't discard comments when preprocessing", nullptr)
OPTION(prefix_1, "c", c, Flag, Action_Group, INVALID, nullptr, DriverOption, 0,
       "Only run preprocess, compile, and assemble steps", nullptr)
OPTION(prefix_2, "c", _SLASH_c, Flag, cl_Group, c, nullptr, CLOption | DriverOption | CLOption, 0,
       "Compile only", nullptr)
OPTION(prefix_2, "d2FastFail", _SLASH_d2FastFail, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "d2Zi+", _SLASH_d2Zi_PLUS, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "dA", dA, Flag, d_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dD", dD, Flag, d_Group, INVALID, nullptr, CC1Option, 0,
       "Print macro definitions in -E mode in addition to normal output", nullptr)
OPTION(prefix_1, "dead_strip", dead__strip, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "debug-info-kind=", debug_info_kind_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "debug-info-macro", debug_info_macro, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Emit macro debug information", nullptr)
OPTION(prefix_3, "debug=", _debug_EQ, Joined, INVALID, g_Flag, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "debugger-tuning=", debugger_tuning_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_3, "debug", _debug, Flag, INVALID, g_Flag, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "define-macro=", _define_macro_EQ, Joined, INVALID, D, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "define-macro", _define_macro, Separate, INVALID, D, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "defsym", defsym, Separate, INVALID, INVALID, nullptr, CC1AsOption | NoDriverOption, 0,
       "Define a value for a symbol", nullptr)
OPTION(prefix_3, "dependencies", _dependencies, Flag, INVALID, M, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dependency-dot", dependency_dot, Separate, INVALID, INVALID, nullptr, CC1Option, 0,
       "Filename to write DOT-formatted header dependencies to", nullptr)
OPTION(prefix_1, "dependency-file", dependency_file, Separate, INVALID, INVALID, nullptr, CC1Option, 0,
       "Filename (or -) to write dependency output to", nullptr)
OPTION(prefix_3, "dependent-lib=", dependent_lib, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add dependent library", nullptr)
OPTION(prefix_1, "detailed-preprocessing-record", detailed_preprocessing_record, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "include a detailed record of preprocessing actions", nullptr)
OPTION(prefix_1, "diagnostic-log-file", diagnostic_log_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Filename (or -) to log diagnostics to", nullptr)
OPTION(prefix_1, "disable-free", disable_free, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable freeing of memory on exit", nullptr)
OPTION(prefix_1, "disable-lifetime-markers", disable_lifetimemarkers, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable lifetime-markers emission even when optimizations are enabled", nullptr)
OPTION(prefix_1, "disable-llvm-optzns", disable_llvm_optzns, Flag, INVALID, disable_llvm_passes, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "disable-llvm-passes", disable_llvm_passes, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use together with -emit-llvm to get pristine LLVM IR from the frontend by not running any LLVM passes at all", nullptr)
OPTION(prefix_1, "disable-llvm-verifier", disable_llvm_verifier, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Don't run the LLVM IR verifier pass", nullptr)
OPTION(prefix_1, "disable-objc-default-synthesize-properties", disable_objc_default_synthesize_properties, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "disable the default synthesis of Objective-C properties", nullptr)
OPTION(prefix_1, "disable-red-zone", disable_red_zone, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not emit code that uses the red zone.", nullptr)
OPTION(prefix_1, "discard-value-names", discard_value_names, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Discard value names in LLVM IR", nullptr)
OPTION(prefix_1, "dI", dI, Flag, d_Group, INVALID, nullptr, CC1Option, 0,
       "Print include directives in -E mode in addition to normal output", nullptr)
OPTION(prefix_1, "dM", dM, Flag, d_Group, INVALID, nullptr, CC1Option, 0,
       "Print macro definitions in -E mode instead of normal output", nullptr)
OPTION(prefix_2, "doc", _SLASH_doc, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_3, "driver-mode=", driver_mode, Joined, internal_driver_Group, INVALID, nullptr, CoreOption | DriverOption | HelpHidden, 0,
       "Set the driver mode to either 'gcc', 'g++', 'cpp', or 'cl'", nullptr)
OPTION(prefix_1, "dump-coverage-mapping", dump_coverage_mapping, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Dump the coverage mapping records, for testing", nullptr)
OPTION(prefix_1, "dump-deserialized-decls", dump_deserialized_pch_decls, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Dump declarations that are deserialized from PCH, for testing", nullptr)
OPTION(prefix_1, "dump-raw-tokens", dump_raw_tokens, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Lex file in raw mode and dump raw tokens", nullptr)
OPTION(prefix_1, "dump-tokens", dump_tokens, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Run preprocessor, dump internal rep of tokens", nullptr)
OPTION(prefix_1, "dumpmachine", dumpmachine, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dumpspecs", dumpspecs, Flag, INVALID, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "dumpversion", dumpversion, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dwarf-column-info", dwarf_column_info, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Turn on column location information.", nullptr)
OPTION(prefix_1, "dwarf-debug-flags", dwarf_debug_flags, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "The string to embed in the Dwarf debug flags record.", nullptr)
OPTION(prefix_1, "dwarf-debug-producer", dwarf_debug_producer, Separate, INVALID, INVALID, nullptr, CC1AsOption | NoDriverOption, 0,
       "The string to embed in the Dwarf debug AT_producer record.", nullptr)
OPTION(prefix_1, "dwarf-ext-refs", dwarf_ext_refs, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate debug info with external references to clang modules or precompiled headers", nullptr)
OPTION(prefix_1, "dwarf-version=", dwarf_version_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_3, "dyld-prefix=", _dyld_prefix_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "dyld-prefix", _dyld_prefix, Separate, INVALID, _dyld_prefix_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dylib_file", dylib__file, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dylinker_install_name", dylinker__install__name, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dylinker", dylinker, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dynamiclib", dynamiclib, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "dynamic", dynamic, Flag, INVALID, INVALID, nullptr, NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "D", D, JoinedOrSeparate, Preprocessor_Group, INVALID, nullptr, CC1Option, 0,
       "Define <macro> to <value> (or 1 if <value> omitted)", "<macro>=<value>")
OPTION(prefix_2, "D", _SLASH_D, JoinedOrSeparate, cl_Group, D, nullptr, CLOption | DriverOption | CLOption, 0,
       "Define macro", "<macro[=value]>")
OPTION(prefix_1, "d", d_Flag, Flag, d_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "d", d_Joined, Joined, d_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "EB", EB, Flag, INVALID, mbig_endian, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "EH", _SLASH_EH, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Exception handling model", nullptr)
OPTION(prefix_1, "EL", EL, Flag, INVALID, mlittle_endian, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "emit-ast", emit_ast, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Emit Clang AST files for source inputs", nullptr)
OPTION(prefix_1, "emit-codegen-only", emit_codegen_only, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate machine code, but discard output", nullptr)
OPTION(prefix_1, "emit-html", emit_html, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Output input source as HTML", nullptr)
OPTION(prefix_1, "emit-llvm-bc", emit_llvm_bc, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs then convert to LLVM, emit .bc file", nullptr)
OPTION(prefix_1, "emit-llvm-only", emit_llvm_only, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Build ASTs and convert to LLVM, discarding output", nullptr)
OPTION(prefix_1, "emit-llvm-uselists", emit_llvm_uselists, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Preserve order of LLVM use-lists when serializing", nullptr)
OPTION(prefix_1, "emit-llvm", emit_llvm, Flag, Action_Group, INVALID, nullptr, CC1Option, 0,
       "Use the LLVM representation for assembler and object files", nullptr)
OPTION(prefix_1, "emit-module-interface", emit_module_interface, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate pre-compiled module file from a C++ module interface", nullptr)
OPTION(prefix_1, "emit-module", emit_module, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate pre-compiled module file from a module map", nullptr)
OPTION(prefix_1, "emit-obj", emit_obj, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit native object files", nullptr)
OPTION(prefix_1, "emit-pch", emit_pch, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate pre-compiled header file", nullptr)
OPTION(prefix_1, "emit-pth", emit_pth, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate pre-tokenized header file", nullptr)
OPTION(prefix_3, "encoding=", _encoding_EQ, Joined, INVALID, fencoding_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "encoding", _encoding, Separate, INVALID, fencoding_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "entry", _entry, Flag, INVALID, e, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Eonly", Eonly, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Just run preprocessor, no output (for timings)", nullptr)
OPTION(prefix_2, "EP", _SLASH_EP, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable linemarker output and preprocess to stdout", nullptr)
OPTION(prefix_1, "error-on-deserialized-decl=", error_on_deserialized_pch_decl_EQ, Joined, INVALID, error_on_deserialized_pch_decl, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "error-on-deserialized-decl", error_on_deserialized_pch_decl, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit error if a specific declaration is deserialized from PCH, for testing", nullptr)
OPTION(prefix_2, "errorReport", _SLASH_errorReport, Joined, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "execution-charset:", _SLASH_execution_charset, Joined, cl_compile_Group, fexec_charset_EQ, nullptr, CLOption | DriverOption, 0,
       "Runtime encoding, supports only UTF-8", nullptr)
OPTION(prefix_1, "exported_symbols_list", exported__symbols__list, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "extdirs=", _extdirs_EQ, Joined, INVALID, fextdirs_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "extdirs", _extdirs, Separate, INVALID, fextdirs_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "extra-warnings", _extra_warnings, Flag, INVALID, W_Joined, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "E", E, Flag, Action_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Only run the preprocessor", nullptr)
OPTION(prefix_2, "E", _SLASH_E, Flag, cl_Group, E, nullptr, CLOption | DriverOption | CLOption, 0,
       "Preprocess to stdout", nullptr)
OPTION(prefix_1, "e", e, JoinedOrSeparate, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "faccess-control", faccess_control, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "faddress-space-map-mangling=", faddress_space_map_mangling_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set the mode for address space map based mangling; OpenCL testing purposes only", "<yes|no|target>")
OPTION(prefix_1, "faggressive-function-elimination", aggressive_function_elimination_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-commons", align_commons_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-functions=", falign_functions_EQ, Joined, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-functions", align_functions_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-jumps=", falign_jumps_EQ, Joined, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-jumps", align_jumps_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-labels=", falign_labels_EQ, Joined, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-labels", align_labels_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-loops=", falign_loops_EQ, Joined, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "falign-loops", align_loops_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "faligned-allocation", faligned_allocation, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable C++17 aligned allocation functions", nullptr)
OPTION(prefix_1, "faligned-new=", faligned_new_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "faligned-new", anonymous_11, Flag, INVALID, faligned_allocation, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fall-intrinsics", all_intrinsics_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "fallback", _SLASH_fallback, Flag, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Fall back to cl.exe if clang-cl fails to compile", nullptr)
OPTION(prefix_1, "fallow-half-arguments-and-returns", fallow_half_arguments_and_returns, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Allow function arguments and returns of type half", nullptr)
OPTION(prefix_1, "fallow-pch-with-compiler-errors", fallow_pch_with_errors, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Accept a PCH file that was created with compiler errors", nullptr)
OPTION(prefix_1, "fallow-unsupported", fallow_unsupported, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "faltivec", faltivec, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fansi-escape-codes", fansi_escape_codes, Flag, f_Group, INVALID, nullptr, CoreOption | CC1Option, 0,
       "Use ANSI escape codes for diagnostics", nullptr)
OPTION(prefix_1, "fapple-kext", fapple_kext, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use Apple's kernel extensions ABI", nullptr)
OPTION(prefix_1, "fapple-pragma-pack", fapple_pragma_pack, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable Apple gcc-compatible #pragma pack handling", nullptr)
OPTION(prefix_1, "fapplication-extension", fapplication_extension, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Restrict code to those available for App Extensions", nullptr)
OPTION(prefix_1, "fasm-blocks", fasm_blocks, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fasm", fasm, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fassociative-math", fassociative_math, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fassume-sane-operator-new", fassume_sane_operator_new, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fastcp", fastcp, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fastf", fastf, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fast", fast, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fasynchronous-unwind-tables", fasynchronous_unwind_tables, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fauto-profile=", fauto_profile_EQ, Joined, INVALID, fprofile_sample_use_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fauto-profile", fauto_profile, Flag, f_Group, fprofile_sample_use, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fautolink", fautolink, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fautomatic", automatic_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "favor", _SLASH_favor, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "FA", _SLASH_FA, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Output assembly code file during compilation", nullptr)
OPTION(prefix_2, "FA", _SLASH_FA_joined, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Fa", _SLASH_Fa, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Output assembly code to this file during compilation (with /FA)", "<file or directory>")
OPTION(prefix_1, "fbackslash", backslash_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fbacktrace", backtrace_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fblas-matmul-limit=", fblas_matmul_limit_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fblocks-runtime-optional", fblocks_runtime_optional, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Weakly link in the blocks runtime", nullptr)
OPTION(prefix_1, "fblocks", fblocks, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable the 'blocks' language feature", nullptr)
OPTION(prefix_1, "fbootclasspath=", fbootclasspath_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fborland-extensions", fborland_extensions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Accept non-standard constructs supported by the Borland compiler", nullptr)
OPTION(prefix_1, "fbounds-check", bounds_check_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fbracket-depth=", fbracket_depth_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fbracket-depth", fbracket_depth, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Maximum nesting level for parentheses, brackets, and braces", nullptr)
OPTION(prefix_1, "fbranch-count-reg", branch_count_reg_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fbuild-session-file=", fbuild_session_file, Joined, i_Group, INVALID, nullptr, 0, 0,
       "Use the last modification time of <file> as the build session timestamp", "<file>")
OPTION(prefix_1, "fbuild-session-timestamp=", fbuild_session_timestamp, Joined, i_Group, INVALID, nullptr, CC1Option, 0,
       "Time when the current build session started", "<time since Epoch in seconds>")
OPTION(prefix_1, "fbuiltin-module-map", fbuiltin_module_map, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Load the clang builtins module map file.", nullptr)
OPTION(prefix_1, "fbuiltin", fbuiltin, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcaller-saves", caller_saves_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fcaret-diagnostics", fcaret_diagnostics, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcheck-array-temporaries", check_array_temporaries_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcheck-new", fcheck_new_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fcheck=", fcheck_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fclasspath=", fclasspath_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcoarray=", fcoarray_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcolor-diagnostics", fcolor_diagnostics, Flag, f_Group, INVALID, nullptr, CoreOption | CC1Option, 0,
       "Use colors in diagnostics", nullptr)
OPTION(prefix_1, "fcomment-block-commands=", fcomment_block_commands, CommaJoined, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Treat each comma separated argument in <arg> as a documentation comment block command", "<arg>")
OPTION(prefix_1, "fcommon", fcommon, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcompile-resource=", fcompile_resource_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fconcepts-ts", fconcepts_ts, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable C++ Extensions for Concepts.", nullptr)
OPTION(prefix_1, "fconst-strings", fconst_strings, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use a const qualified type for string literals in C and ObjC", nullptr)
OPTION(prefix_1, "fconstant-cfstrings", fconstant_cfstrings, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fconstant-string-class=", fconstant_string_class_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fconstant-string-class", fconstant_string_class, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Specify the class to use for constant Objective-C string objects.", "<class name>")
OPTION(prefix_1, "fconstexpr-backtrace-limit=", fconstexpr_backtrace_limit_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fconstexpr-backtrace-limit", fconstexpr_backtrace_limit, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set the maximum number of entries to print in a constexpr evaluation backtrace (0 = no limit).", "<N>")
OPTION(prefix_1, "fconstexpr-depth=", fconstexpr_depth_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fconstexpr-depth", fconstexpr_depth, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Maximum depth of recursive constexpr function calls", nullptr)
OPTION(prefix_1, "fconstexpr-steps=", fconstexpr_steps_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fconstexpr-steps", fconstexpr_steps, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Maximum number of steps in constexpr function evaluation", nullptr)
OPTION(prefix_1, "fconvert=", fconvert_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcoroutines-ts", fcoroutines_ts, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Enable support for the C++ Coroutines TS", nullptr)
OPTION(prefix_1, "fcoverage-mapping", fcoverage_mapping, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Generate coverage mapping to enable code coverage analysis", nullptr)
OPTION(prefix_1, "fcray-pointer", cray_pointer_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcreate-profile", fcreate_profile, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fcuda-allow-variadic-functions", fcuda_allow_variadic_functions, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Allow variadic functions in CUDA device code.", nullptr)
OPTION(prefix_1, "fcuda-approx-transcendentals", fcuda_approx_transcendentals, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Use approximate transcendental functions", nullptr)
OPTION(prefix_1, "fcuda-flush-denormals-to-zero", fcuda_flush_denormals_to_zero, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Flush denormal floating point values to zero in CUDA device mode.", nullptr)
OPTION(prefix_1, "fcuda-include-gpubinary", fcuda_include_gpubinary, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Incorporate CUDA device-side binary into host object file.", nullptr)
OPTION(prefix_1, "fcuda-is-device", fcuda_is_device, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate code for CUDA device", nullptr)
OPTION(prefix_1, "fcxx-exceptions", fcxx_exceptions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable C++ exceptions", nullptr)
OPTION(prefix_1, "fcxx-modules", fcxx_modules, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "FC", _SLASH_FC, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fd-lines-as-code", d_lines_as_code_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fd-lines-as-comments", d_lines_as_comments_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdata-sections", fdata_sections, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Place each data in its own section (ELF Only)", nullptr)
OPTION(prefix_1, "fdebug-compilation-dir", fdebug_compilation_dir, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "The compilation directory to embed in the debug info.", nullptr)
OPTION(prefix_1, "fdebug-info-for-profiling", fdebug_info_for_profiling, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Emit extra debug info to make sample profile more accurate.", nullptr)
OPTION(prefix_1, "fdebug-macro", fdebug_macro, Flag, f_Group, INVALID, nullptr, CoreOption, 0,
       "Emit macro debug information", nullptr)
OPTION(prefix_1, "fdebug-pass-arguments", fdebug_pass_arguments, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdebug-pass-structure", fdebug_pass_structure, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdebug-prefix-map=", fdebug_prefix_map_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "remap file source paths in debug info", nullptr)
OPTION(prefix_1, "fdebug-types-section", fdebug_types_section, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Place debug types in their own section (ELF Only)", nullptr)
OPTION(prefix_1, "fdebugger-cast-result-to-id", fdebugger_cast_result_to_id, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable casting unknown expression results to id", nullptr)
OPTION(prefix_1, "fdebugger-objc-literal", fdebugger_objc_literal, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable special debugger support for Objective-C subscripting and literals", nullptr)
OPTION(prefix_1, "fdebugger-support", fdebugger_support, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable special debugger support behavior", nullptr)
OPTION(prefix_1, "fdeclspec", fdeclspec, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Allow __declspec as a keyword", nullptr)
OPTION(prefix_1, "fdefault-calling-conv=", fdefault_calling_conv_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set default MS calling convention", nullptr)
OPTION(prefix_1, "fdefault-double-8", default_double_8_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdefault-inline", default_inline_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fdefault-integer-8", default_integer_8_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdefault-real-8", default_real_8_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdefer-pop", anonymous_3, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fdelayed-template-parsing", fdelayed_template_parsing, Flag, f_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Parse templated function definitions at the end of the translation unit", nullptr)
OPTION(prefix_1, "fdelete-null-pointer-checks", delete_null_pointer_checks_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fdenormal-fp-math=", fdenormal_fp_math_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fdepfile-entry=", fdepfile_entry, Joined, f_clang_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fdeprecated-macro", fdeprecated_macro, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Defines the __DEPRECATED macro", nullptr)
OPTION(prefix_1, "fdevirtualize-speculatively", devirtualize_speculatively_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fdevirtualize", devirtualize_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fdiagnostics-absolute-paths", fdiagnostics_absolute_paths, Flag, f_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Print absolute paths in diagnostics", nullptr)
OPTION(prefix_1, "fdiagnostics-color=", fdiagnostics_color_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdiagnostics-color", fdiagnostics_color, Flag, f_Group, INVALID, nullptr, CoreOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fdiagnostics-fixit-info", fdiagnostics_fixit_info, Flag, f_clang_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdiagnostics-format=", fdiagnostics_format_EQ, Joined, f_clang_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdiagnostics-format", fdiagnostics_format, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Change diagnostic formatting to match IDE and command line tools", nullptr)
OPTION(prefix_1, "fdiagnostics-parseable-fixits", fdiagnostics_parseable_fixits, Flag, f_clang_Group, INVALID, nullptr, CoreOption | CC1Option, 0,
       "Print fix-its in machine parseable form", nullptr)
OPTION(prefix_1, "fdiagnostics-print-source-range-info", fdiagnostics_print_source_range_info, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Print source range spans in numeric form", nullptr)
OPTION(prefix_1, "fdiagnostics-show-category=", fdiagnostics_show_category_EQ, Joined, f_clang_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdiagnostics-show-category", fdiagnostics_show_category, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Print diagnostic category", nullptr)
OPTION(prefix_1, "fdiagnostics-show-hotness", fdiagnostics_show_hotness, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable profile hotness information in diagnostic line", nullptr)
OPTION(prefix_1, "fdiagnostics-show-location=", fdiagnostics_show_location_EQ, Joined, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fdiagnostics-show-note-include-stack", fdiagnostics_show_note_include_stack, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Display include stacks for diagnostic notes", nullptr)
OPTION(prefix_1, "fdiagnostics-show-option", fdiagnostics_show_option, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Print option name with mappable diagnostics", nullptr)
OPTION(prefix_1, "fdiagnostics-show-template-tree", fdiagnostics_show_template_tree, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Print a template comparison tree for differing templates", nullptr)
OPTION(prefix_1, "fdisable-module-hash", fdisable_module_hash, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable the module hash", nullptr)
OPTION(prefix_1, "fdollar-ok", dollar_ok_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdollars-in-identifiers", fdollars_in_identifiers, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Allow '$' in identifiers", nullptr)
OPTION(prefix_1, "fdump-fortran-optimized", dump_fortran_optimized_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdump-fortran-original", dump_fortran_original_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdump-parse-tree", dump_parse_tree_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdump-record-layouts-simple", fdump_record_layouts_simple, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Dump record layout information in a simple form used for testing", nullptr)
OPTION(prefix_1, "fdump-record-layouts", fdump_record_layouts, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Dump record layout information", nullptr)
OPTION(prefix_1, "fdump-vtable-layouts", fdump_vtable_layouts, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Dump the layouts of all vtables that will be emitted in a translation unit", nullptr)
OPTION(prefix_1, "fdwarf-directory-asm", fdwarf_directory_asm, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fdwarf2-cfi-asm", fdwarf2_cfi_asm, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_2, "Fd", _SLASH_Fd, Joined, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "felide-constructors", felide_constructors, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "feliminate-unused-debug-symbols", feliminate_unused_debug_symbols, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "feliminate-unused-debug-types", eliminate_unused_debug_types_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fembed-bitcode-marker", fembed_bitcode_marker, Flag, INVALID, fembed_bitcode_EQ, "marker\0", 0, 0,
       "Embed placeholder LLVM IR data as a marker", nullptr)
OPTION(prefix_1, "fembed-bitcode=", fembed_bitcode_EQ, Joined, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Embed LLVM bitcode (option: off, all, bitcode, marker)", "<option>")
OPTION(prefix_1, "fembed-bitcode", fembed_bitcode, Flag, f_Group, fembed_bitcode_EQ, "all\0", 0, 0,
       "Embed LLVM IR bitcode as data", nullptr)
OPTION(prefix_1, "femit-all-decls", femit_all_decls, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Emit all declarations, even if unused", nullptr)
OPTION(prefix_1, "femit-coverage-data", femit_coverage_data, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Instrument the program to emit gcov coverage data when run.", nullptr)
OPTION(prefix_1, "femit-coverage-notes", femit_coverage_notes, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit a gcov coverage notes file when compiling.", nullptr)
OPTION(prefix_1, "femulated-tls", femulated_tls, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use emutls functions to access thread_local variables", nullptr)
OPTION(prefix_1, "fencode-extended-block-signature", fencode_extended_block_signature, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "enable extended encoding of block type signature", nullptr)
OPTION(prefix_1, "fencoding=", fencoding_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ferror-limit=", ferror_limit_EQ, Joined, f_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "ferror-limit", ferror_limit, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set the maximum number of errors to emit before stopping (0 = no limit).", "<N>")
OPTION(prefix_1, "fexceptions", fexceptions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable support for exception handling", nullptr)
OPTION(prefix_1, "fexcess-precision=", fexcess_precision_EQ, Joined, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fexec-charset=", fexec_charset_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fexpensive-optimizations", anonymous_1, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fexperimental-new-pass-manager", fexperimental_new_pass_manager, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Enables an experimental new pass manager in LLVM.", nullptr)
OPTION(prefix_1, "fextdirs=", fextdirs_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fextended-identifiers", anonymous_5, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fexternal-blas", external_blas_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fexternc-nounwind", fexternc_nounwind, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Assume all functions with C linkage do not unwind", nullptr)
OPTION(prefix_2, "Fe", _SLASH_Fe, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set output executable file or directory (ends in / or \\)", "<file or directory>")
OPTION(prefix_1, "ff2c", f2c_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ffake-address-space-map", ffake_address_space_map, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use a fake address space map; OpenCL testing purposes only", nullptr)
OPTION(prefix_1, "ffast-math", ffast_math, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Allow aggressive, lossy floating-point optimizations", nullptr)
OPTION(prefix_1, "ffat-lto-objects", fat_lto_objects_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ffinite-math-only", ffinite_math_only, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "ffixed-form", fixed_form_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ffixed-line-length-", ffixed_line_length_VALUE, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ffixed-r9", ffixed_r9, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Reserve the r9 register (ARM only)", nullptr)
OPTION(prefix_1, "ffixed-x18", ffixed_x18, Flag, m_aarch64_Features_Group, INVALID, nullptr, 0, 0,
       "Reserve the x18 register (AArch64 only)", nullptr)
OPTION(prefix_1, "ffloat-store", float_store_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ffor-scope", ffor_scope, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fforbid-guard-variables", fforbid_guard_variables, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit an error if a C++ static local initializer would need a guard variable", nullptr)
OPTION(prefix_1, "fforce-addr", force_addr, Joined, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ffp-contract=", ffp_contract, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Form fused FP ops (e.g. FMAs): fast (everywhere) | on (according to FP_CONTRACT pragma, default) | off (never fuse)", nullptr)
OPTION(prefix_1, "ffpe-trap=", ffpe_trap_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ffree-form", free_form_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ffree-line-length-", ffree_line_length_VALUE, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ffreestanding", ffreestanding, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Assert that the compilation takes place in a freestanding environment", nullptr)
OPTION(prefix_1, "ffriend-injection", friend_injection_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ffrontend-optimize", frontend_optimize_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ffunction-attribute-list", function_attribute_list_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ffunction-sections", ffunction_sections, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Place each function in its own section (ELF Only)", nullptr)
OPTION(prefix_1, "fgcse-after-reload", gcse_after_reload_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fgcse-las", gcse_las_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fgcse-sm", gcse_sm_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fgcse", gcse_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fgnu-inline-asm", fgnu_inline_asm, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fgnu-keywords", fgnu_keywords, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Allow GNU-extension keywords regardless of language standard", nullptr)
OPTION(prefix_1, "fgnu-runtime", fgnu_runtime, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Generate output compatible with the standard GNU Objective-C runtime", nullptr)
OPTION(prefix_1, "fgnu89-inline", fgnu89_inline, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use the gnu89 inline semantics", nullptr)
OPTION(prefix_1, "fgnu", gnu_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fheinous-gnu-extensions", fheinous_gnu_extensions, Flag, INVALID, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fhonor-infinites", anonymous_7, Flag, INVALID, fhonor_infinities, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fhonor-infinities", fhonor_infinities, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fhonor-nans", fhonor_nans, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fhosted", fhosted, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fident", ident_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "filelist", filelist, Separate, Link_Group, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_1, "filetype", filetype, Separate, INVALID, INVALID, nullptr, CC1AsOption | NoDriverOption, 0,
       "Specify the output file type ('asm', 'null', or 'obj')", nullptr)
OPTION(prefix_1, "fimplement-inlines", implement_inlines_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fimplicit-module-maps", fimplicit_module_maps, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Implicitly search the file system for module map files.", nullptr)
OPTION(prefix_1, "fimplicit-modules", fimplicit_modules, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fimplicit-none", implicit_none_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fimplicit-templates", implicit_templates_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "finclude-default-header", finclude_default_header, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include the default header file for OpenCL", nullptr)
OPTION(prefix_1, "find-pch-source=", find_pch_source_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "When building a pch, try to find the input file in include directories, as if it had been included by the argument passed to this flag.", nullptr)
OPTION(prefix_1, "findirect-virtual-calls", anonymous_9, Flag, INVALID, fapple_kext, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "finit-character=", finit_character_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "finit-integer=", finit_integer_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "finit-local-zero", init_local_zero_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "finit-logical=", finit_logical_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "finit-real=", finit_real_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "finline-functions-called-once", inline_functions_called_once_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "finline-functions", finline_functions, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Inline suitable functions", nullptr)
OPTION(prefix_1, "finline-hint-functions", finline_hint_functions, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Inline functions which are (explicitly or implicitly) marked inline", nullptr)
OPTION(prefix_1, "finline-limit=", finline_limit_EQ, Joined, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "finline-limit", finline_limit_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "finline-small-functions", inline_small_functions_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "finline", finline, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "finput-charset=", finput_charset_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "finstrument-functions", finstrument_functions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Generate calls to instrument function entry and exit", nullptr)
OPTION(prefix_1, "finteger-4-integer-8", integer_4_integer_8_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fintegrated-as", fintegrated_as, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Enable the integrated assembler", nullptr)
OPTION(prefix_1, "fintrinsic-modules-path", intrinsic_modules_path_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fipa-cp", ipa_cp_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fivopts", ivopts_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fix-only-warnings", fix_only_warnings, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply fix-it advice only for warnings, not errors", nullptr)
OPTION(prefix_1, "fix-what-you-can", fix_what_you_can, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply fix-it advice even in the presence of unfixable errors", nullptr)
OPTION(prefix_1, "fixit-recompile", fixit_recompile, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply fix-it changes and recompile", nullptr)
OPTION(prefix_1, "fixit-to-temporary", fixit_to_temp, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply fix-it changes to temporary files", nullptr)
OPTION(prefix_1, "fixit=", fixit_EQ, Joined, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply fix-it advice creating a file with the given suffix", nullptr)
OPTION(prefix_1, "fixit", fixit, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Apply fix-it advice to the input source", nullptr)
OPTION(prefix_2, "FI", _SLASH_FI, JoinedOrSeparate, cl_Group, include, nullptr, CLOption | DriverOption | CLOption, 0,
       "Include file before parsing", nullptr)
OPTION(prefix_2, "Fi", _SLASH_Fi, Joined, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Set preprocess output file name (with /P)", "<file>")
OPTION(prefix_1, "fjump-tables", fjump_tables, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fkeep-inline-functions", anonymous_23_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "flat_namespace", flat__namespace, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "flax-vector-conversions", flax_vector_conversions, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "flimit-debug-info", flimit_debug_info, Flag, INVALID, fno_standalone_debug, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "flimited-precision=", flimited_precision_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "flto-jobs=", flto_jobs_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Controls the backend parallelism of -flto=thin (default of 0 means the number of threads will be derived from the number of CPUs detected)", nullptr)
OPTION(prefix_1, "flto-unit", flto_unit, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit IR to support LTO unit features (CFI, whole program vtable opt)", nullptr)
OPTION(prefix_1, "flto-visibility-public-std", flto_visibility_public_std, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use public LTO visibility for classes in std and stdext namespaces", nullptr)
OPTION(prefix_1, "flto=", flto_EQ, Joined, f_Group, INVALID, nullptr, CoreOption | CC1Option, 0,
       "Set LTO mode to either 'full' or 'thin'", nullptr)
OPTION(prefix_1, "flto", flto, Flag, f_Group, INVALID, nullptr, CoreOption | CC1Option, 0,
       "Enable LTO in 'full' mode", nullptr)
OPTION(prefix_1, "fmacro-backtrace-limit=", fmacro_backtrace_limit_EQ, Joined, f_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fmacro-backtrace-limit", fmacro_backtrace_limit, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set the maximum number of entries to print in a macro expansion backtrace (0 = no limit).", "<N>")
OPTION(prefix_1, "fmath-errno", fmath_errno, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Require math functions to indicate errors by setting errno", nullptr)
OPTION(prefix_1, "fmax-array-constructor=", fmax_array_constructor_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmax-errors=", fmax_errors_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmax-identifier-length", max_identifier_length_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmax-stack-var-size=", fmax_stack_var_size_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmax-subrecord-length=", fmax_subrecord_length_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmax-type-align=", fmax_type_align_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Specify the maximum alignment to enforce on pointers lacking an explicit alignment", nullptr)
OPTION(prefix_1, "fmerge-all-constants", fmerge_all_constants, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmerge-constants", merge_constants_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fmerge-functions", fmerge_functions, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Permit merging of identical functions when optimizing.", nullptr)
OPTION(prefix_1, "fmessage-length=", fmessage_length_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmessage-length", fmessage_length, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Format message diagnostics so that they fit within N columns or fewer, when possible.", "<N>")
OPTION(prefix_1, "fmodule-feature", fmodule_feature, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable <feature> in module map requires declarations", "<feature>")
OPTION(prefix_1, "fmodule-file-deps", fmodule_file_deps, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fmodule-file=", fmodule_file, Joined, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Load this precompiled module file", "<file>")
OPTION(prefix_1, "fmodule-format=", fmodule_format_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Select the container format for clang modules and PCH. Supported options are 'raw' and 'obj'.", nullptr)
OPTION(prefix_1, "fmodule-implementation-of", fmodule_implementation_of, Separate, INVALID, fmodule_name_EQ, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fmodule-map-file-home-is-cwd", fmodule_map_file_home_is_cwd, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use the current working directory as the home directory of module maps specified by -fmodule-map-file=<FILE>", nullptr)
OPTION(prefix_1, "fmodule-map-file=", fmodule_map_file, Joined, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Load this module map file", "<file>")
OPTION(prefix_1, "fmodule-maps", fmodule_maps, Flag, INVALID, fimplicit_module_maps, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmodule-name=", fmodule_name_EQ, Joined, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Specify the name of the module to build", "<name>")
OPTION(prefix_1, "fmodule-name", fmodule_name, Separate, INVALID, fmodule_name_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmodule-private", module_private_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmodules-cache-path=", fmodules_cache_path, Joined, i_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Specify the module cache path", "<directory>")
OPTION(prefix_1, "fmodules-codegen", fmodule_codegen, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate code for uses of this module that assumes an explicit object file will be built for the module", nullptr)
OPTION(prefix_1, "fmodules-decluse", fmodules_decluse, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Require declaration of modules used within a module", nullptr)
OPTION(prefix_1, "fmodules-disable-diagnostic-validation", fmodules_disable_diagnostic_validation, Flag, i_Group, INVALID, nullptr, CC1Option, 0,
       "Disable validation of the diagnostic options when loading the module", nullptr)
OPTION(prefix_1, "fmodules-embed-all-files", fmodules_embed_all_files, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Embed the contents of all files read by this compilation into the produced module file.", nullptr)
OPTION(prefix_1, "fmodules-embed-file=", fmodules_embed_file_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Embed the contents of the specified file into the module file being compiled.", "<file>")
OPTION(prefix_1, "fmodules-hash-content", fmodules_hash_content, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable hashing the content of a module file", nullptr)
OPTION(prefix_1, "fmodules-ignore-macro=", fmodules_ignore_macro, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Ignore the definition of the given macro when building and loading modules", nullptr)
OPTION(prefix_1, "fmodules-local-submodule-visibility", fmodules_local_submodule_visibility, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enforce name visibility rules across submodules of the same top-level module.", nullptr)
OPTION(prefix_1, "fmodules-prune-after=", fmodules_prune_after, Joined, i_Group, INVALID, nullptr, CC1Option, 0,
       "Specify the interval (in seconds) after which a module file will be considered unused", "<seconds>")
OPTION(prefix_1, "fmodules-prune-interval=", fmodules_prune_interval, Joined, i_Group, INVALID, nullptr, CC1Option, 0,
       "Specify the interval (in seconds) between attempts to prune the module cache", "<seconds>")
OPTION(prefix_1, "fmodules-search-all", fmodules_search_all, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Search even non-imported modules to resolve references", nullptr)
OPTION(prefix_1, "fmodules-strict-decluse", fmodules_strict_decluse, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Like -fmodules-decluse but requires all headers to be in modules", nullptr)
OPTION(prefix_1, "fmodules-ts", fmodules_ts, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable support for the C++ Modules TS", nullptr)
OPTION(prefix_1, "fmodules-user-build-path", fmodules_user_build_path, Separate, i_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Specify the module user build path", "<directory>")
OPTION(prefix_1, "fmodules-validate-once-per-build-session", fmodules_validate_once_per_build_session, Flag, i_Group, INVALID, nullptr, CC1Option, 0,
       "Don't verify input files for the modules if the module has been successfully validated or loaded during this build session", nullptr)
OPTION(prefix_1, "fmodules-validate-system-headers", fmodules_validate_system_headers, Flag, i_Group, INVALID, nullptr, CC1Option, 0,
       "Validate the system headers that a module depends on when loading the module", nullptr)
OPTION(prefix_1, "fmodules", fmodules, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Enable the 'modules' language feature", nullptr)
OPTION(prefix_1, "fmodulo-sched-allow-regmoves", modulo_sched_allow_regmoves_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fmodulo-sched", modulo_sched_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fms-compatibility-version=", fms_compatibility_version, Joined, f_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Dot-separated value representing the Microsoft compiler version number to report in _MSC_VER (0 = don't define it (default))", nullptr)
OPTION(prefix_1, "fms-compatibility", fms_compatibility, Flag, f_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Enable full Microsoft Visual C++ compatibility", nullptr)
OPTION(prefix_1, "fms-extensions", fms_extensions, Flag, f_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Accept some non-standard constructs supported by the Microsoft compiler", nullptr)
OPTION(prefix_1, "fms-memptr-rep=", fms_memptr_rep_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fms-volatile", fms_volatile, Joined, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fmsc-version=", fmsc_version, Joined, f_Group, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Microsoft compiler version number to report in _MSC_VER (0 = don't define it (default))", nullptr)
OPTION(prefix_1, "fmudflapth", fmudflapth, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fmudflap", fmudflap, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "Fm", _SLASH_Fm, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fnative-half-arguments-and-returns", fnative_half_arguments_and_returns, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use the native __fp16 type for arguments and returns (and skip ABI-specific lowering)", nullptr)
OPTION(prefix_1, "fnative-half-type", fnative_half_type, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use the native half type for __fp16 instead of promoting to float", nullptr)
OPTION(prefix_1, "fnested-functions", fnested_functions, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fnew-alignment=", fnew_alignment_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Specifies the largest alignment guaranteed by '::operator new(size_t)'", "<align>")
OPTION(prefix_1, "fnew-alignment", anonymous_10, Separate, INVALID, fnew_alignment_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fnext-runtime", fnext_runtime, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-access-control", fno_access_control, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disable C++ access control", nullptr)
OPTION(prefix_1, "fno-aggressive-function-elimination", aggressive_function_elimination_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-align-commons", align_commons_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-align-functions", align_functions_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-align-jumps", align_jumps_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-align-labels", align_labels_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-align-loops", align_loops_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-aligned-allocation", fno_aligned_allocation, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-aligned-new", anonymous_12, Flag, INVALID, fno_aligned_allocation, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-all-intrinsics", all_intrinsics_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-altivec", fno_altivec, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-apple-pragma-pack", fno_apple_pragma_pack, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-application-extension", fno_application_extension, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-asm-blocks", fno_asm_blocks, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-asm", fno_asm, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-associative-math", fno_associative_math, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-assume-sane-operator-new", fno_assume_sane_operator_new, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Don't assume that C++'s global operator new can't alias any pointer", nullptr)
OPTION(prefix_1, "fno-asynchronous-unwind-tables", fno_asynchronous_unwind_tables, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-auto-profile", fno_auto_profile, Flag, f_Group, fno_profile_sample_use, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-autolink", fno_autolink, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Disable generation of linker directives for automatic library linking", nullptr)
OPTION(prefix_1, "fno-automatic", automatic_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-backslash", backslash_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-backtrace", backtrace_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-bitfield-type-align", fno_bitfield_type_align, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Ignore bit-field types when aligning structures", nullptr)
OPTION(prefix_1, "fno-blocks", fno_blocks, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-borland-extensions", fno_borland_extensions, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-bounds-check", bounds_check_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-branch-count-reg", branch_count_reg_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-builtin-", fno_builtin_, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disable implicit builtin knowledge of a specific function", nullptr)
OPTION(prefix_1, "fno-builtin", fno_builtin, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disable implicit builtin knowledge of functions", nullptr)
OPTION(prefix_1, "fno-caller-saves", caller_saves_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-caret-diagnostics", fno_caret_diagnostics, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-check-array-temporaries", check_array_temporaries_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-check-new", fcheck_new_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-color-diagnostics", fno_color_diagnostics, Flag, f_Group, INVALID, nullptr, CoreOption | CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-common", fno_common, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Compile common globals like normal definitions", nullptr)
OPTION(prefix_1, "fno-const-strings", fno_const_strings, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Don't use a const qualified type for string literals in C and ObjC", nullptr)
OPTION(prefix_1, "fno-constant-cfstrings", fno_constant_cfstrings, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disable creation of CodeFoundation-type constant strings", nullptr)
OPTION(prefix_1, "fno-coroutines-ts", fno_coroutines_ts, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-coverage-mapping", fno_coverage_mapping, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Disable code coverage analysis", nullptr)
OPTION(prefix_1, "fno-crash-diagnostics", fno_crash_diagnostics, Flag, f_clang_Group, INVALID, nullptr, NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-cray-pointer", cray_pointer_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-cuda-approx-transcendentals", fno_cuda_approx_transcendentals, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-cuda-flush-denormals-to-zero", fno_cuda_flush_denormals_to_zero, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-cuda-host-device-constexpr", fno_cuda_host_device_constexpr, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Don't treat unattributed constexpr functions as __host__ __device__.", nullptr)
OPTION(prefix_1, "fno-cxx-exceptions", fno_cxx_exceptions, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-cxx-modules", fno_cxx_modules, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-d-lines-as-code", d_lines_as_code_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-d-lines-as-comments", d_lines_as_comments_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-data-sections", fno_data_sections, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-debug-info-for-profiling", fno_debug_info_for_profiling, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Do not emit extra debug info for sample profiler.", nullptr)
OPTION(prefix_1, "fno-debug-macro", fno_debug_macro, Flag, f_Group, INVALID, nullptr, CoreOption, 0,
       "Do not emit macro debug information", nullptr)
OPTION(prefix_1, "fno-debug-types-section", fno_debug_types_section, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-declspec", fno_declspec, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Disallow __declspec as a keyword", nullptr)
OPTION(prefix_1, "fno-default-double-8", default_double_8_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-default-inline", default_inline_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-default-integer-8", default_integer_8_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-default-real-8", default_real_8_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-defer-pop", anonymous_4, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-delayed-template-parsing", fno_delayed_template_parsing, Flag, f_Group, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Disable delayed template parsing", nullptr)
OPTION(prefix_1, "fno-delete-null-pointer-checks", delete_null_pointer_checks_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-deprecated-macro", fno_deprecated_macro, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Undefines the __DEPRECATED macro", nullptr)
OPTION(prefix_1, "fno-devirtualize-speculatively", devirtualize_speculatively_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-devirtualize", devirtualize_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-diagnostics-color", fno_diagnostics_color, Flag, f_Group, INVALID, nullptr, CoreOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-diagnostics-fixit-info", fno_diagnostics_fixit_info, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not include fixit information in diagnostics", nullptr)
OPTION(prefix_1, "fno-diagnostics-show-hotness", fno_diagnostics_show_hotness, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-diagnostics-show-note-include-stack", fno_diagnostics_show_note_include_stack, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-diagnostics-show-option", fno_diagnostics_show_option, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-diagnostics-use-presumed-location", fno_diagnostics_use_presumed_location, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Ignore #line directives when displaying diagnostic locations", nullptr)
OPTION(prefix_1, "fno-dollar-ok", dollar_ok_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-dollars-in-identifiers", fno_dollars_in_identifiers, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disallow '$' in identifiers", nullptr)
OPTION(prefix_1, "fno-dump-fortran-optimized", dump_fortran_optimized_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-dump-fortran-original", dump_fortran_original_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-dump-parse-tree", dump_parse_tree_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-dwarf-directory-asm", fno_dwarf_directory_asm, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-dwarf2-cfi-asm", fno_dwarf2_cfi_asm, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-elide-constructors", fno_elide_constructors, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disable C++ copy constructor elision", nullptr)
OPTION(prefix_1, "fno-elide-type", fno_elide_type, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not elide types when printing diagnostics", nullptr)
OPTION(prefix_1, "fno-eliminate-unused-debug-symbols", fno_eliminate_unused_debug_symbols, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-eliminate-unused-debug-types", eliminate_unused_debug_types_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-emulated-tls", fno_emulated_tls, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-exceptions", fno_exceptions, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-expensive-optimizations", anonymous_2, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-experimental-new-pass-manager", fno_experimental_new_pass_manager, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Disables an experimental new pass manager in LLVM.", nullptr)
OPTION(prefix_1, "fno-extended-identifiers", anonymous_6, Flag, f_Group, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-external-blas", external_blas_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-f2c", f2c_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-fast-math", fno_fast_math, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-fat-lto-objects", fat_lto_objects_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-finite-math-only", fno_finite_math_only, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-fixed-form", fixed_form_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-float-store", float_store_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-for-scope", fno_for_scope, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-free-form", free_form_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-friend-injection", friend_injection_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-frontend-optimize", frontend_optimize_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-function-attribute-list", function_attribute_list_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-function-sections", fno_function_sections, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-gcse-after-reload", gcse_after_reload_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-gcse-las", gcse_las_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-gcse-sm", gcse_sm_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-gcse", gcse_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-gnu-inline-asm", fno_gnu_inline_asm, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Disable GNU style inline asm", nullptr)
OPTION(prefix_1, "fno-gnu-keywords", fno_gnu_keywords, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-gnu89-inline", fno_gnu89_inline, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-gnu", gnu_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-honor-infinites", anonymous_8, Flag, INVALID, fno_honor_infinities, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-honor-infinities", fno_honor_infinities, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-honor-nans", fno_honor_nans, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-ident", ident_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-implement-inlines", implement_inlines_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-implicit-module-maps", fno_implicit_module_maps, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-implicit-modules", fno_implicit_modules, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-implicit-none", implicit_none_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-implicit-templates", implicit_templates_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-init-local-zero", init_local_zero_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-inline-functions-called-once", inline_functions_called_once_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-inline-functions", fno_inline_functions, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-inline-limit", finline_limit_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-inline-small-functions", inline_small_functions_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-inline", fno_inline, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-integer-4-integer-8", integer_4_integer_8_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-integrated-as", fno_integrated_as, Flag, f_Group, INVALID, nullptr, CC1Option | DriverOption, 0,
       "Disable the integrated assembler", nullptr)
OPTION(prefix_1, "fno-intrinsic-modules-path", intrinsic_modules_path_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-ipa-cp", ipa_cp_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-ivopts", ivopts_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-jump-tables", fno_jump_tables, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not use jump tables for lowering switches", nullptr)
OPTION(prefix_1, "fno-keep-inline-functions", anonymous_23_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-lax-vector-conversions", fno_lax_vector_conversions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disallow implicit conversions between vectors with a different number of elements or different element types", nullptr)
OPTION(prefix_1, "fno-limit-debug-info", fno_limit_debug_info, Flag, INVALID, fstandalone_debug, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-lto-unit", fno_lto_unit, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-lto", fno_lto, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Disable LTO mode (default)", nullptr)
OPTION(prefix_1, "fno-math-builtin", fno_math_builtin, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Disable implicit builtin knowledge of math functions", nullptr)
OPTION(prefix_1, "fno-math-errno", fno_math_errno, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-max-identifier-length", max_identifier_length_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-max-type-align", fno_max_type_align, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-merge-all-constants", fno_merge_all_constants, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disallow merging of constants", nullptr)
OPTION(prefix_1, "fno-merge-constants", merge_constants_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-module-file-deps", fno_module_file_deps, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-module-maps", fno_module_maps, Flag, INVALID, fno_implicit_module_maps, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-module-private", module_private_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-modules-decluse", fno_modules_decluse, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-modules-error-recovery", fno_modules_error_recovery, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not automatically import modules for error recovery", nullptr)
OPTION(prefix_1, "fno-modules-global-index", fno_modules_global_index, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not automatically generate or update the global module index", nullptr)
OPTION(prefix_1, "fno-modules-search-all", fno_modules_search_all, Flag, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-modules", fno_modules, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-modulo-sched-allow-regmoves", modulo_sched_allow_regmoves_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-modulo-sched", modulo_sched_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-ms-compatibility", fno_ms_compatibility, Flag, f_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-ms-extensions", fno_ms_extensions, Flag, f_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-non-call-exceptions", non_call_exceptions_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-objc-arc-exceptions", fno_objc_arc_exceptions, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-objc-arc", fno_objc_arc, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-objc-exceptions", fno_objc_exceptions, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-objc-infer-related-result-type", fno_objc_infer_related_result_type, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "do not infer Objective-C related result type based on method family", nullptr)
OPTION(prefix_1, "fno-objc-legacy-dispatch", fno_objc_legacy_dispatch, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-objc-nonfragile-abi", fno_objc_nonfragile_abi, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-objc-weak", fno_objc_weak, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-omit-frame-pointer", fno_omit_frame_pointer, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-openmp", fno_openmp, Flag, f_Group, INVALID, nullptr, NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-operator-names", fno_operator_names, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not treat C++ operator name keywords as synonyms for operators", nullptr)
OPTION(prefix_1, "fno-optimize-sibling-calls", fno_optimize_sibling_calls, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-pack-derived", pack_derived_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-pack-struct", fno_pack_struct, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-pascal-strings", fno_pascal_strings, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-pch-timestamp", fno_pch_timestamp, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable inclusion of timestamp in precompiled headers", nullptr)
OPTION(prefix_1, "fno-peel-loops", peel_loops_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-permissive", permissive_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-PIC", fno_PIC, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-pic", fno_pic, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-PIE", fno_PIE, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-pie", fno_pie, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-prefetch-loop-arrays", prefetch_loop_arrays_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-preserve-as-comments", fno_preserve_as_comments, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not preserve comments in inline assembly", nullptr)
OPTION(prefix_1, "fno-printf", printf_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile-arcs", fno_profile_arcs, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile-correction", profile_correction_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile-generate-sampling", profile_generate_sampling_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile-generate", fno_profile_generate, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Disable generation of profile instrumentation.", nullptr)
OPTION(prefix_1, "fno-profile-instr-generate", fno_profile_instr_generate, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Disable generation of profile instrumentation.", nullptr)
OPTION(prefix_1, "fno-profile-instr-use", fno_profile_instr_use, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Disable using instrumentation data for profile-guided optimization", nullptr)
OPTION(prefix_1, "fno-profile-reusedist", profile_reusedist_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile-sample-use", fno_profile_sample_use, Flag, f_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile-use", fno_profile_use, Flag, INVALID, fno_profile_instr_use, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile-values", profile_values_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-profile", profile_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-protect-parens", protect_parens_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-range-check", range_check_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-real-4-real-10", real_4_real_10_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-real-4-real-16", real_4_real_16_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-real-4-real-8", real_4_real_8_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-real-8-real-10", real_8_real_10_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-real-8-real-16", real_8_real_16_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-real-8-real-4", real_8_real_4_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-realloc-lhs", realloc_lhs_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-reciprocal-math", fno_reciprocal_math, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-recursive", recursive_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-regs-graph", regs_graph_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-relaxed-template-template-args", fno_relaxed_template_template_args, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-rename-registers", rename_registers_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-reorder-blocks", reorder_blocks_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-repack-arrays", repack_arrays_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-reroll-loops", fno_reroll_loops, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Turn off loop reroller", nullptr)
OPTION(prefix_1, "fno-rewrite-includes", fno_rewrite_includes, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-ripa", ripa_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-ropi", fno_ropi, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-rounding-math", rounding_math_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-rtlib-add-rpath", fno_rtlib_add_rpath, Flag, INVALID, INVALID, nullptr, NoArgumentUnused, 0,
       "Do not add -rpath with architecture-specific resource directory to the linker flags", nullptr)
OPTION(prefix_1, "fno-rtti-data", fno_rtti_data, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Control emission of RTTI data", nullptr)
OPTION(prefix_1, "fno-rtti", fno_rtti, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disable generation of rtti information", nullptr)
OPTION(prefix_1, "fno-rwpi", fno_rwpi, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-sanitize-address-use-after-scope", fno_sanitize_address_use_after_scope, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Disable use-after-scope detection in AddressSanitizer", nullptr)
OPTION(prefix_1, "fno-sanitize-blacklist", fno_sanitize_blacklist, Flag, f_clang_Group, INVALID, nullptr, 0, 0,
       "Don't use blacklist file for sanitizers", nullptr)
OPTION(prefix_1, "fno-sanitize-cfi-cross-dso", fno_sanitize_cfi_cross_dso, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Disable control flow integrity (CFI) checks for cross-DSO calls.", nullptr)
OPTION(prefix_1, "fno-sanitize-coverage=", fno_sanitize_coverage, CommaJoined, f_clang_Group, INVALID, nullptr, CoreOption, 0,
       "Disable specified features of coverage instrumentation for Sanitizers", nullptr)
OPTION(prefix_1, "fno-sanitize-memory-track-origins", fno_sanitize_memory_track_origins, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Disable origins tracking in MemorySanitizer", nullptr)
OPTION(prefix_1, "fno-sanitize-recover=", fno_sanitize_recover_EQ, CommaJoined, f_clang_Group, INVALID, nullptr, CoreOption, 0,
       "Disable recovery for specified sanitizers", nullptr)
OPTION(prefix_1, "fno-sanitize-recover", fno_sanitize_recover, Flag, f_clang_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-sanitize-stats", fno_sanitize_stats, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Disable sanitizer statistics gathering.", nullptr)
OPTION(prefix_1, "fno-sanitize-thread-atomics", fno_sanitize_thread_atomics, Flag, f_clang_Group, INVALID, nullptr, 0, 0,
       "Disable atomic operations instrumentation in ThreadSanitizer", nullptr)
OPTION(prefix_1, "fno-sanitize-thread-func-entry-exit", fno_sanitize_thread_func_entry_exit, Flag, f_clang_Group, INVALID, nullptr, 0, 0,
       "Disable function entry/exit instrumentation in ThreadSanitizer", nullptr)
OPTION(prefix_1, "fno-sanitize-thread-memory-access", fno_sanitize_thread_memory_access, Flag, f_clang_Group, INVALID, nullptr, 0, 0,
       "Disable memory access instrumentation in ThreadSanitizer", nullptr)
OPTION(prefix_1, "fno-sanitize-trap=", fno_sanitize_trap_EQ, CommaJoined, f_clang_Group, INVALID, nullptr, CoreOption, 0,
       "Disable trapping for specified sanitizers", nullptr)
OPTION(prefix_1, "fno-sanitize-undefined-trap-on-error", fno_sanitize_undefined_trap_on_error, Flag, f_clang_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-sanitize=", fno_sanitize_EQ, CommaJoined, f_clang_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-save-optimization-record", fno_save_optimization_record, Flag, f_Group, INVALID, nullptr, NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-schedule-insns2", schedule_insns2_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-schedule-insns", schedule_insns_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-second-underscore", second_underscore_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-see", see_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-short-enums", fno_short_enums, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-short-wchar", fno_short_wchar, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Force wchar_t to be an unsigned int", nullptr)
OPTION(prefix_1, "fno-show-column", fno_show_column, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not include column number on diagnostics", nullptr)
OPTION(prefix_1, "fno-show-source-location", fno_show_source_location, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not include source location information with diagnostics", nullptr)
OPTION(prefix_1, "fno-sign-zero", sign_zero_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-signaling-math", fno_signaling_math, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-signaling-nans", signaling_nans_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-signed-char", fno_signed_char, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Char is unsigned", nullptr)
OPTION(prefix_1, "fno-signed-zeros", fno_signed_zeros, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Allow optimizations that ignore the sign of floating point zeros", nullptr)
OPTION(prefix_1, "fno-single-precision-constant", single_precision_constant_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-sized-deallocation", fno_sized_deallocation, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-slp-vectorize-aggressive", fno_slp_vectorize_aggressive, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-slp-vectorize", fno_slp_vectorize, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-spec-constr-count", spec_constr_count_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-spell-checking", fno_spell_checking, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Disable spell-checking", nullptr)
OPTION(prefix_1, "fno-split-dwarf-inlining", fno_split_dwarf_inlining, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-stack-arrays", stack_arrays_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-stack-check", stack_check_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-stack-protector", fno_stack_protector, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Disable the use of stack protectors", nullptr)
OPTION(prefix_1, "fno-standalone-debug", fno_standalone_debug, Flag, f_Group, INVALID, nullptr, CoreOption, 0,
       "Limit debug information produced to reduce size of debug binary", nullptr)
OPTION(prefix_1, "fno-strength-reduce", strength_reduce_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-strict-aliasing", fno_strict_aliasing, Flag, f_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-strict-enums", fno_strict_enums, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-strict-modules-decluse", fno_modules_strict_decluse, Flag, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-strict-overflow", fno_strict_overflow, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-strict-return", fno_strict_return, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-strict-vtable-pointers", fno_strict_vtable_pointers, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-struct-path-tbaa", fno_struct_path_tbaa, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-threadsafe-statics", fno_threadsafe_statics, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not emit code to make initialization of local statics thread safe", nullptr)
OPTION(prefix_1, "fno-tls-model", tls_model_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tracer", tracer_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-trapping-math", fno_trapping_math, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree-dce", tree_dce_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree-salias", tree_salias_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree-slp-vectorize", anonymous_17, Flag, INVALID, fno_slp_vectorize, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree-ter", tree_ter_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree-vectorizer-verbose", tree_vectorizer_verbose_fno, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree-vectorize", anonymous_15, Flag, INVALID, fno_vectorize, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree-vrp", tree_vrp_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree_loop_im", tree_loop_im_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree_loop_ivcanon", tree_loop_ivcanon_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-tree_loop_linear", tree_loop_linear_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-trigraphs", fno_trigraphs, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Do not process trigraph sequences", nullptr)
OPTION(prefix_1, "fno-underscoring", underscoring_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unique-section-names", fno_unique_section_names, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unit-at-a-time", fno_unit_at_a_time, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unroll-all-loops", unroll_all_loops_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unroll-loops", fno_unroll_loops, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Turn off loop unroller", nullptr)
OPTION(prefix_1, "fno-unsafe-loop-optimizations", unsafe_loop_optimizations_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unsafe-math-optimizations", fno_unsafe_math_optimizations, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unsigned-char", fno_unsigned_char, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unswitch-loops", unswitch_loops_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-unwind-tables", fno_unwind_tables, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-use-cxa-atexit", fno_use_cxa_atexit, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Don't use __cxa_atexit for calling destructors", nullptr)
OPTION(prefix_1, "fno-use-init-array", fno_use_init_array, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Don't use .init_array instead of .ctors", nullptr)
OPTION(prefix_1, "fno-use-line-directives", fno_use_line_directives, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-use-linker-plugin", use_linker_plugin_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-validate-pch", fno_validate_pch, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable validation of precompiled headers", nullptr)
OPTION(prefix_1, "fno-var-tracking", fno_var_tracking, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-variable-expansion-in-unroller", variable_expansion_in_unroller_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-vect-cost-model", vect_cost_model_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-vectorize", fno_vectorize, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-verbose-asm", fno_verbose_asm, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-wchar", fno_wchar, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable C++ builtin type wchar_t", nullptr)
OPTION(prefix_1, "fno-web", web_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-whole-file", whole_file_fno, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-whole-program-vtables", fno_whole_program_vtables, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-whole-program", whole_program_fno, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-working-directory", fno_working_directory, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-wrapv", fno_wrapv, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-xray-instrument", fnoxray_instrument, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-zero-initialized-in-bss", fno_zero_initialized_in_bss, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fno-zvector", fno_zvector, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fnon-call-exceptions", non_call_exceptions_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fnoopenmp-use-tls", fnoopenmp_use_tls, Flag, f_Group, INVALID, nullptr, CC1Option | NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-abi-version=", fobjc_abi_version_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-arc-cxxlib=", fobjc_arc_cxxlib_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Objective-C++ Automatic Reference Counting standard library kind", nullptr)
OPTION(prefix_1, "fobjc-arc-exceptions", fobjc_arc_exceptions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use EH-safe code when synthesizing retains and releases in -fobjc-arc", nullptr)
OPTION(prefix_1, "fobjc-arc", fobjc_arc, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Synthesize retain and release calls for Objective-C pointers", nullptr)
OPTION(prefix_1, "fobjc-atdefs", fobjc_atdefs, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-call-cxx-cdtors", fobjc_call_cxx_cdtors, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-dispatch-method=", fobjc_dispatch_method_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Objective-C dispatch method to use", nullptr)
OPTION(prefix_1, "fobjc-exceptions", fobjc_exceptions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable Objective-C exceptions", nullptr)
OPTION(prefix_1, "fobjc-gc-only", fobjc_gc_only, Flag, f_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use GC exclusively for Objective-C related memory management", nullptr)
OPTION(prefix_1, "fobjc-gc", fobjc_gc, Flag, f_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable Objective-C garbage collection", nullptr)
OPTION(prefix_1, "fobjc-infer-related-result-type", fobjc_infer_related_result_type, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-legacy-dispatch", fobjc_legacy_dispatch, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-link-runtime", fobjc_link_runtime, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-new-property", fobjc_new_property, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-nonfragile-abi-version=", fobjc_nonfragile_abi_version_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-nonfragile-abi", fobjc_nonfragile_abi, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-runtime-has-weak", fobjc_runtime_has_weak, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "The target Objective-C runtime supports ARC weak operations", nullptr)
OPTION(prefix_1, "fobjc-runtime=", fobjc_runtime_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Specify the target Objective-C runtime kind and version", nullptr)
OPTION(prefix_1, "fobjc-sender-dependent-dispatch", fobjc_sender_dependent_dispatch, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fobjc-subscripting-legacy-runtime", fobjc_subscripting_legacy_runtime, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Allow Objective-C array and dictionary subscripting in legacy runtime", nullptr)
OPTION(prefix_1, "fobjc-weak", fobjc_weak, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable ARC-style weak references in Objective-C", nullptr)
OPTION(prefix_1, "fomit-frame-pointer", fomit_frame_pointer, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fopenmp-dump-offload-linker-script", fopenmp_dump_offload_linker_script, Flag, f_Group, INVALID, nullptr, NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "fopenmp-host-ir-file-path", fopenmp_host_ir_file_path, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Path to the IR file produced by the frontend for the host.", nullptr)
OPTION(prefix_1, "fopenmp-is-device", fopenmp_is_device, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate code only for an OpenMP target device.", nullptr)
OPTION(prefix_1, "fopenmp-targets=", fopenmp_targets_EQ, CommaJoined, INVALID, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Specify comma-separated list of triples OpenMP offloading targets to be supported", nullptr)
OPTION(prefix_1, "fopenmp-use-tls", fopenmp_use_tls, Flag, f_Group, INVALID, nullptr, NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "fopenmp-version=", fopenmp_version_EQ, Joined, f_Group, INVALID, nullptr, CC1Option | NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "fopenmp=", fopenmp_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fopenmp", fopenmp, Flag, f_Group, INVALID, nullptr, CC1Option | NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "foperator-arrow-depth=", foperator_arrow_depth_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "foperator-arrow-depth", foperator_arrow_depth, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Maximum number of 'operator->'s to call for a member access", nullptr)
OPTION(prefix_1, "foptimization-record-file=", foptimization_record_file_EQ, Joined, f_Group, INVALID, nullptr, 0, 0,
       "Specify the file name of any generated YAML optimization record", nullptr)
OPTION(prefix_1, "foptimize-sibling-calls", foptimize_sibling_calls, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "for-linker=", _for_linker_EQ, Joined, INVALID, Xlinker, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "for-linker", _for_linker, Separate, INVALID, Xlinker, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "force-link=", _force_link_EQ, Joined, INVALID, u, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "force-link", _force_link, Separate, INVALID, u, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "force_cpusubtype_ALL", force__cpusubtype__ALL, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "force_flat_namespace", force__flat__namespace, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "force_load", force__load, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "foutput-class-dir=", foutput_class_dir_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "foverride-record-layout=", foverride_record_layout_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Override record layouts with those in the given file", nullptr)
OPTION(prefix_2, "Fo", _SLASH_Fo, Joined, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Set output object file, or directory (ends in / or \\) (with /c)", "<file or directory>")
OPTION(prefix_2, "fp:except-", _SLASH_fp_except_, Flag, cl_Group, fno_trapping_math, nullptr, CLOption | DriverOption | CLOption, 0,
       "", nullptr)
OPTION(prefix_2, "fp:except", _SLASH_fp_except, Flag, cl_Group, ftrapping_math, nullptr, CLOption | DriverOption | CLOption, 0,
       "", nullptr)
OPTION(prefix_2, "fp:fast", _SLASH_fp_fast, Flag, cl_Group, ffast_math, nullptr, CLOption | DriverOption | CLOption, 0,
       "", nullptr)
OPTION(prefix_2, "fp:precise", _SLASH_fp_precise, Flag, cl_Group, fno_fast_math, nullptr, CLOption | DriverOption | CLOption, 0,
       "", nullptr)
OPTION(prefix_2, "fp:strict", _SLASH_fp_strict, Flag, cl_Group, fno_fast_math, nullptr, CLOption | DriverOption | CLOption, 0,
       "", nullptr)
OPTION(prefix_1, "fpack-derived", pack_derived_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fpack-struct=", fpack_struct_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Specify the default maximum struct packing alignment", nullptr)
OPTION(prefix_1, "fpack-struct", fpack_struct, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fparse-all-comments", fparse_all_comments, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fpascal-strings", fpascal_strings, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Recognize and construct Pascal-style string literals", nullptr)
OPTION(prefix_1, "fpcc-struct-return", fpcc_struct_return, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Override the default ABI to return all structs on the stack", nullptr)
OPTION(prefix_1, "fpch-preprocess", fpch_preprocess, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fpeel-loops", peel_loops_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fpermissive", permissive_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fPIC", fPIC, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fpic", fpic, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fPIE", fPIE, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fpie", fpie, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fplugin=", fplugin_EQ, Joined, f_Group, INVALID, nullptr, DriverOption, 0,
       "Load the named plugin (dynamic shared object)", "<dsopath>")
OPTION(prefix_1, "fprebuilt-module-path=", fprebuilt_module_path, Joined, i_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Specify the prebuilt module path", "<directory>")
OPTION(prefix_1, "fprefetch-loop-arrays", prefetch_loop_arrays_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fpreserve-as-comments", fpreserve_as_comments, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fpreserve-vec3-type", fpreserve_vec3_type, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Preserve 3-component vector type", nullptr)
OPTION(prefix_1, "fprintf", printf_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-arcs", fprofile_arcs, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-correction", profile_correction_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-dir=", fprofile_dir, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-generate-sampling", profile_generate_sampling_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-generate=", fprofile_generate_EQ, Joined, f_Group, INVALID, nullptr, DriverOption, 0,
       "Generate instrumented code to collect execution counts into <directory>/default.profraw (overridden by LLVM_PROFILE_FILE env var)", "<directory>")
OPTION(prefix_1, "fprofile-generate", fprofile_generate, Flag, f_Group, INVALID, nullptr, DriverOption, 0,
       "Generate instrumented code to collect execution counts into default.profraw (overridden by LLVM_PROFILE_FILE env var)", nullptr)
OPTION(prefix_1, "fprofile-instr-generate=", fprofile_instr_generate_EQ, Joined, f_Group, INVALID, nullptr, CoreOption, 0,
       "Generate instrumented code to collect execution counts into <file> (overridden by LLVM_PROFILE_FILE env var)", "<file>")
OPTION(prefix_1, "fprofile-instr-generate", fprofile_instr_generate, Flag, f_Group, INVALID, nullptr, CoreOption, 0,
       "Generate instrumented code to collect execution counts into default.profraw file (overridden by '=' form of option or LLVM_PROFILE_FILE env var)", nullptr)
OPTION(prefix_1, "fprofile-instr-use=", fprofile_instr_use_EQ, Joined, f_Group, INVALID, nullptr, CoreOption, 0,
       "Use instrumentation data for profile-guided optimization", nullptr)
OPTION(prefix_1, "fprofile-instr-use", fprofile_instr_use, Flag, f_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-instrument-path=", fprofile_instrument_path_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate instrumented code to collect execution counts into <file> (overridden by LLVM_PROFILE_FILE env var)", nullptr)
OPTION(prefix_1, "fprofile-instrument-use-path=", fprofile_instrument_use_path_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Specify the profile path in PGO use compilation", nullptr)
OPTION(prefix_1, "fprofile-instrument=", fprofile_instrument_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable PGO instrumentation. The accepted value is clang, llvm, or none", nullptr)
OPTION(prefix_1, "fprofile-reusedist", profile_reusedist_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-sample-use=", fprofile_sample_use_EQ, Joined, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Enable sample-based profile guided optimizations", nullptr)
OPTION(prefix_1, "fprofile-sample-use", fprofile_sample_use, Flag, f_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-use=", fprofile_use_EQ, Joined, f_Group, INVALID, nullptr, DriverOption, 0,
       "Use instrumentation data for profile-guided optimization. If pathname is a directory, it reads from <pathname>/default.profdata. Otherwise, it reads from file <pathname>.", "<pathname>")
OPTION(prefix_1, "fprofile-use", fprofile_use, Flag, f_Group, fprofile_instr_use, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile-values", profile_values_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fprofile", profile_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fprotect-parens", protect_parens_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "Fp", _SLASH_Fp, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set pch filename (with /Yc and /Yu)", "<filename>")
OPTION(prefix_1, "framework", framework, Separate, INVALID, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_1, "frandom-seed=", frandom_seed_EQ, Joined, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "frange-check", range_check_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freal-4-real-10", real_4_real_10_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freal-4-real-16", real_4_real_16_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freal-4-real-8", real_4_real_8_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freal-8-real-10", real_8_real_10_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freal-8-real-16", real_8_real_16_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freal-8-real-4", real_8_real_4_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "frealloc-lhs", realloc_lhs_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freciprocal-math", freciprocal_math, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Allow division operations to be reassociated", nullptr)
OPTION(prefix_1, "frecord-marker=", frecord_marker_EQ, Joined, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "frecursive", recursive_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freg-struct-return", freg_struct_return, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Override the default ABI to return small structs in registers", nullptr)
OPTION(prefix_1, "fregs-graph", regs_graph_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "frelaxed-template-template-args", frelaxed_template_template_args, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable C++17 relaxed template template argument matching", nullptr)
OPTION(prefix_1, "frename-registers", rename_registers_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "freorder-blocks", reorder_blocks_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "frepack-arrays", repack_arrays_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "freroll-loops", freroll_loops, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Turn on loop reroller", nullptr)
OPTION(prefix_1, "fretain-comments-from-system-headers", fretain_comments_from_system_headers, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "frewrite-includes", frewrite_includes, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "frewrite-map-file=", frewrite_map_file_EQ, Joined, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "frewrite-map-file", frewrite_map_file, Separate, f_Group, INVALID, nullptr, DriverOption | CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fripa", ripa_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fropi", fropi, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "frounding-math", rounding_math_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "frtlib-add-rpath", frtlib_add_rpath, Flag, INVALID, INVALID, nullptr, NoArgumentUnused, 0,
       "Add -rpath with architecture-specific resource directory to the linker flags", nullptr)
OPTION(prefix_1, "frtti", frtti, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "frwpi", frwpi, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "FR", _SLASH_FR, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Fr", _SLASH_Fr, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fsanitize-address-field-padding=", fsanitize_address_field_padding, Joined, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Level of field padding for AddressSanitizer", nullptr)
OPTION(prefix_1, "fsanitize-address-use-after-scope", fsanitize_address_use_after_scope, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Enable use-after-scope detection in AddressSanitizer", nullptr)
OPTION(prefix_1, "fsanitize-blacklist=", fsanitize_blacklist, Joined, f_clang_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Path to blacklist file for sanitizers", nullptr)
OPTION(prefix_1, "fsanitize-cfi-cross-dso", fsanitize_cfi_cross_dso, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Enable control flow integrity (CFI) checks for cross-DSO calls.", nullptr)
OPTION(prefix_1, "fsanitize-coverage-8bit-counters", fsanitize_coverage_8bit_counters, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable frequency counters in sanitizer coverage", nullptr)
OPTION(prefix_1, "fsanitize-coverage-indirect-calls", fsanitize_coverage_indirect_calls, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable sanitizer coverage for indirect calls", nullptr)
OPTION(prefix_1, "fsanitize-coverage-trace-bb", fsanitize_coverage_trace_bb, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable basic block tracing in sanitizer coverage", nullptr)
OPTION(prefix_1, "fsanitize-coverage-trace-cmp", fsanitize_coverage_trace_cmp, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable cmp instruction tracing in sanitizer coverage", nullptr)
OPTION(prefix_1, "fsanitize-coverage-trace-div", fsanitize_coverage_trace_div, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable div instruction tracing in sanitizer coverage", nullptr)
OPTION(prefix_1, "fsanitize-coverage-trace-gep", fsanitize_coverage_trace_gep, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable gep instruction tracing in sanitizer coverage", nullptr)
OPTION(prefix_1, "fsanitize-coverage-trace-pc-guard", fsanitize_coverage_trace_pc_guard, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable PC tracing with guard in sanitizer coverage", nullptr)
OPTION(prefix_1, "fsanitize-coverage-trace-pc", fsanitize_coverage_trace_pc, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable PC tracing in sanitizer coverage", nullptr)
OPTION(prefix_1, "fsanitize-coverage-type=", fsanitize_coverage_type, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Sanitizer coverage type", nullptr)
OPTION(prefix_1, "fsanitize-coverage=", fsanitize_coverage, CommaJoined, f_clang_Group, INVALID, nullptr, CoreOption, 0,
       "Specify the type of coverage instrumentation for Sanitizers", nullptr)
OPTION(prefix_1, "fsanitize-link-c++-runtime", fsanitize_link_cxx_runtime, Flag, f_clang_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsanitize-memory-track-origins=", fsanitize_memory_track_origins_EQ, Joined, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Enable origins tracking in MemorySanitizer", nullptr)
OPTION(prefix_1, "fsanitize-memory-track-origins", fsanitize_memory_track_origins, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Enable origins tracking in MemorySanitizer", nullptr)
OPTION(prefix_1, "fsanitize-memory-use-after-dtor", fsanitize_memory_use_after_dtor, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Enable use-after-destroy detection in MemorySanitizer", nullptr)
OPTION(prefix_1, "fsanitize-recover=", fsanitize_recover_EQ, CommaJoined, f_clang_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Enable recovery for specified sanitizers", nullptr)
OPTION(prefix_1, "fsanitize-recover", fsanitize_recover, Flag, f_clang_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fsanitize-stats", fsanitize_stats, Flag, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Enable sanitizer statistics gathering.", nullptr)
OPTION(prefix_1, "fsanitize-thread-atomics", fsanitize_thread_atomics, Flag, f_clang_Group, INVALID, nullptr, 0, 0,
       "Enable atomic operations instrumentation in ThreadSanitizer (default)", nullptr)
OPTION(prefix_1, "fsanitize-thread-func-entry-exit", fsanitize_thread_func_entry_exit, Flag, f_clang_Group, INVALID, nullptr, 0, 0,
       "Enable function entry/exit instrumentation in ThreadSanitizer (default)", nullptr)
OPTION(prefix_1, "fsanitize-thread-memory-access", fsanitize_thread_memory_access, Flag, f_clang_Group, INVALID, nullptr, 0, 0,
       "Enable memory access instrumentation in ThreadSanitizer (default)", nullptr)
OPTION(prefix_1, "fsanitize-trap=", fsanitize_trap_EQ, CommaJoined, f_clang_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Enable trapping for specified sanitizers", nullptr)
OPTION(prefix_1, "fsanitize-undefined-strip-path-components=", fsanitize_undefined_strip_path_components_EQ, Joined, f_clang_Group, INVALID, nullptr, CC1Option, 0,
       "Strip (or keep only, if negative) a given number of path components when emitting check metadata.", "<number>")
OPTION(prefix_1, "fsanitize-undefined-trap-on-error", fsanitize_undefined_trap_on_error, Flag, f_clang_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsanitize=", fsanitize_EQ, CommaJoined, f_clang_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Turn on runtime checks for various forms of undefined or suspicious behavior. See user manual for available checks", "<check>")
OPTION(prefix_1, "fsave-optimization-record", fsave_optimization_record, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Generate a YAML optimization record file", nullptr)
OPTION(prefix_1, "fsched-interblock", anonymous_13, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fschedule-insns2", schedule_insns2_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fschedule-insns", schedule_insns_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fsecond-underscore", second_underscore_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsee", see_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fshort-enums", fshort_enums, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Allocate to an enum type only as many bytes as it needs for the declared range of possible values", nullptr)
OPTION(prefix_1, "fshort-wchar", fshort_wchar, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Force wchar_t to be a short unsigned int", nullptr)
OPTION(prefix_1, "fshow-column", fshow_column, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fshow-overloads=", fshow_overloads_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Which overload candidates to show when overload resolution fails: best|all; defaults to all", nullptr)
OPTION(prefix_1, "fshow-source-location", fshow_source_location, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsign-zero", sign_zero_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsignaling-math", fsignaling_math, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsignaling-nans", signaling_nans_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fsigned-bitfields", fsigned_bitfields, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsigned-char", fsigned_char, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsigned-zeros", fsigned_zeros, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsingle-precision-constant", single_precision_constant_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fsized-deallocation", fsized_deallocation, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable C++14 sized global deallocation functions", nullptr)
OPTION(prefix_1, "fsjlj-exceptions", fsjlj_exceptions, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use SjLj style exceptions", nullptr)
OPTION(prefix_1, "fslp-vectorize-aggressive", fslp_vectorize_aggressive, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Enable the BB vectorization passes", nullptr)
OPTION(prefix_1, "fslp-vectorize", fslp_vectorize, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Enable the superword-level parallelism vectorization passes", nullptr)
OPTION(prefix_1, "fspec-constr-count", spec_constr_count_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fspell-checking-limit=", fspell_checking_limit_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fspell-checking-limit", fspell_checking_limit, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set the maximum number of times to perform spell checking on unrecognized identifiers (0 = no limit).", "<N>")
OPTION(prefix_1, "fspell-checking", fspell_checking, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsplit-dwarf-inlining", fsplit_dwarf_inlining, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Place debug types in their own section (ELF Only)", nullptr)
OPTION(prefix_1, "fsplit-stack", fsplit_stack, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fstack-arrays", stack_arrays_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fstack-check", stack_check_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fstack-protector-all", fstack_protector_all, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Force the usage of stack protectors for all functions", nullptr)
OPTION(prefix_1, "fstack-protector-strong", fstack_protector_strong, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Use a strong heuristic to apply stack protectors to functions", nullptr)
OPTION(prefix_1, "fstack-protector", fstack_protector, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Enable stack protectors for functions potentially vulnerable to stack smashing", nullptr)
OPTION(prefix_1, "fstandalone-debug", fstandalone_debug, Flag, f_Group, INVALID, nullptr, CoreOption, 0,
       "Emit full debug info for all types used by the program", nullptr)
OPTION(prefix_1, "fstrength-reduce", strength_reduce_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fstrict-aliasing", fstrict_aliasing, Flag, f_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fstrict-enums", fstrict_enums, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable optimizations based on the strict definition of an enum's value range", nullptr)
OPTION(prefix_1, "fstrict-overflow", fstrict_overflow, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fstrict-return", fstrict_return, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Always treat control flow paths that fall off the end of a non-void function as unreachable", nullptr)
OPTION(prefix_1, "fstrict-vtable-pointers", fstrict_vtable_pointers, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable optimizations based on the strict rules for overwriting polymorphic C++ objects", nullptr)
OPTION(prefix_1, "fstruct-path-tbaa", fstruct_path_tbaa, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fsyntax-only", fsyntax_only, Flag, Action_Group, INVALID, nullptr, DriverOption | CoreOption | CC1Option, 0, nullptr, nullptr)
OPTION(prefix_2, "FS", _SLASH_FS, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "ftabstop=", ftabstop_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftabstop", ftabstop, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set the tab stop distance.", "<N>")
OPTION(prefix_1, "ftemplate-backtrace-limit=", ftemplate_backtrace_limit_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftemplate-backtrace-limit", ftemplate_backtrace_limit, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Set the maximum number of entries to print in a template instantiation backtrace (0 = no limit).", "<N>")
OPTION(prefix_1, "ftemplate-depth-", ftemplate_depth_, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftemplate-depth=", ftemplate_depth_EQ, Joined, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftemplate-depth", ftemplate_depth, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Maximum depth of recursive template instantiation", nullptr)
OPTION(prefix_1, "fterminated-vtables", anonymous_18, Flag, INVALID, fapple_kext, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftest-coverage", ftest_coverage, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftest-module-file-extension=", ftest_module_file_extension_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "introduce a module file extension for testing purposes. The argument is parsed as blockname:major:minor:hashed:user info", nullptr)
OPTION(prefix_1, "fthin-link-bitcode=", fthin_link_bitcode_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Write minimized bitcode to <file> for the ThinLTO thin link only", nullptr)
OPTION(prefix_1, "fthinlto-index=", fthinlto_index_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Perform ThinLTO importing using provided function summary index", nullptr)
OPTION(prefix_1, "fthreadsafe-statics", fthreadsafe_statics, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftime-report", ftime_report, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "ftls-model=", ftlsmodel_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "ftls-model", tls_model_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftracer", tracer_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftrap-function=", ftrap_function_EQ, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Issue call to specified function rather than a trap instruction", nullptr)
OPTION(prefix_1, "ftrapping-math", ftrapping_math, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "ftrapv-handler=", ftrapv_handler_EQ, Joined, f_Group, INVALID, nullptr, 0, 0,
       "Specify the function to be called on overflow", "<function name>")
OPTION(prefix_1, "ftrapv-handler", ftrapv_handler, Separate, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "ftrapv", ftrapv, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Trap on integer overflow", nullptr)
OPTION(prefix_1, "ftree-dce", tree_dce_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree-salias", tree_salias_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree-slp-vectorize", anonymous_16, Flag, INVALID, fslp_vectorize, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree-ter", tree_ter_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree-vectorizer-verbose", tree_vectorizer_verbose_f, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree-vectorize", anonymous_14, Flag, INVALID, fvectorize, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree-vrp", tree_vrp_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree_loop_im", tree_loop_im_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree_loop_ivcanon", tree_loop_ivcanon_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftree_loop_linear", tree_loop_linear_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "ftrigraphs", ftrigraphs, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Process trigraph sequences", nullptr)
OPTION(prefix_1, "ftype-visibility", ftype_visibility, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Default type visibility", nullptr)
OPTION(prefix_1, "funderscoring", underscoring_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "funique-section-names", funique_section_names, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use unique names for text and data sections (ELF Only)", nullptr)
OPTION(prefix_1, "funit-at-a-time", funit_at_a_time, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "funknown-anytype", funknown_anytype, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable parser support for the __unknown_anytype type; for testing purposes only", nullptr)
OPTION(prefix_1, "funroll-all-loops", unroll_all_loops_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "funroll-loops", funroll_loops, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Turn on loop unroller", nullptr)
OPTION(prefix_1, "funsafe-loop-optimizations", unsafe_loop_optimizations_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "funsafe-math-optimizations", funsafe_math_optimizations, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "funsigned-bitfields", funsigned_bitfields, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "funsigned-char", funsigned_char, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "funswitch-loops", unswitch_loops_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "funwind-tables", funwind_tables, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fuse-cxa-atexit", fuse_cxa_atexit, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fuse-init-array", fuse_init_array, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use .init_array instead of .ctors", nullptr)
OPTION(prefix_1, "fuse-ld=", fuse_ld_EQ, Joined, f_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fuse-line-directives", fuse_line_directives, Flag, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fuse-linker-plugin", use_linker_plugin_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fuse-register-sized-bitfield-access", fuse_register_sized_bitfield_access, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use register sized accesses to bit-fields, when possible.", nullptr)
OPTION(prefix_2, "FU", _SLASH_FU, JoinedOrSeparate, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fvariable-expansion-in-unroller", variable_expansion_in_unroller_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fveclib=", fveclib, Joined, f_Group, INVALID, nullptr, CC1Option, 0,
       "Use the given vector functions library", nullptr)
OPTION(prefix_1, "fvect-cost-model", vect_cost_model_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fvectorize", fvectorize, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Enable the loop vectorization passes", nullptr)
OPTION(prefix_1, "fverbose-asm", fverbose_asm, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fvisibility-inlines-hidden", fvisibility_inlines_hidden, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Give inline C++ member functions default visibility by default", nullptr)
OPTION(prefix_1, "fvisibility-ms-compat", fvisibility_ms_compat, Flag, f_Group, INVALID, nullptr, 0, 0,
       "Give global types 'default' visibility and global functions and variables 'hidden' visibility by default", nullptr)
OPTION(prefix_1, "fvisibility=", fvisibility_EQ, Joined, f_Group, INVALID, nullptr, 0, 0,
       "Set the default symbol visibility for all global declarations", nullptr)
OPTION(prefix_1, "fvisibility", fvisibility, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Default type and symbol visibility", nullptr)
OPTION(prefix_1, "fweb", web_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fwhole-file", whole_file_f, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fwhole-program-vtables", fwhole_program_vtables, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enables whole-program vtable optimization. Requires -flto", nullptr)
OPTION(prefix_1, "fwhole-program", whole_program_f, Flag, clang_ignored_gcc_optimization_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "fwrapv", fwrapv, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Treat signed integer overflow as two's complement", nullptr)
OPTION(prefix_1, "fwritable-strings", fwritable_strings, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Store string literals as writable data", nullptr)
OPTION(prefix_1, "fxray-always-instrument=", fxray_always_instrument, JoinedOrSeparate, f_Group, INVALID, nullptr, CC1Option, 0,
       "Filename defining the whitelist for imbuing the 'always instrument' XRay attribute.", nullptr)
OPTION(prefix_1, "fxray-instruction-threshold=", fxray_instruction_threshold_EQ, JoinedOrSeparate, f_Group, INVALID, nullptr, CC1Option, 0,
       "Sets the minimum function size to instrument with XRay", nullptr)
OPTION(prefix_1, "fxray-instruction-threshold", fxray_instruction_threshold_, JoinedOrSeparate, f_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "fxray-instrument", fxray_instrument, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Generate XRay instrumentation sleds on function entry and exit", nullptr)
OPTION(prefix_1, "fxray-never-instrument=", fxray_never_instrument, JoinedOrSeparate, f_Group, INVALID, nullptr, CC1Option, 0,
       "Filename defining the whitelist for imbuing the 'never instrument' XRay attribute.", nullptr)
OPTION(prefix_2, "Fx", _SLASH_Fx, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "fzero-initialized-in-bss", fzero_initialized_in_bss, Flag, f_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "fzvector", fzvector, Flag, f_Group, INVALID, nullptr, CC1Option, 0,
       "Enable System z vector language extension", nullptr)
OPTION(prefix_1, "F", F, JoinedOrSeparate, INVALID, INVALID, nullptr, RenderJoined | CC1Option, 0,
       "Add directory to framework include search path", nullptr)
OPTION(prefix_2, "F", _SLASH_F, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "g0", g0, Flag, gN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "G1", _SLASH_G1, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "g1", g1, Flag, gN_Group, gline_tables_only, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "G2", _SLASH_G2, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "g2", g2, Flag, gN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "g3", g3, Flag, gN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "G=", G_EQ, Joined, m_Group, G, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "GA", _SLASH_GA, Flag, cl_Group, ftlsmodel_EQ, "local-exec\0", CLOption | DriverOption | CLOption, 0,
       "Assume thread-local variables are defined in the executable", nullptr)
OPTION(prefix_3, "gcc-toolchain=", gcc_toolchain, Joined, INVALID, INVALID, nullptr, DriverOption, 0,
       "Use the gcc toolchain at the given directory", nullptr)
OPTION(prefix_1, "gcc-toolchain", gcc_toolchain_legacy_spelling, Separate, INVALID, gcc_toolchain, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gcodeview", gcodeview, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | CoreOption, 0,
       "Generate CodeView debug information", nullptr)
OPTION(prefix_1, "gcoff", gcoff, Joined, g_Group, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "gcolumn-info", gcolumn_info, Flag, g_flags_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "gdwarf-2", gdwarf_2, Flag, g_Group, INVALID, nullptr, 0, 0,
       "Generate source-level debug information with dwarf version 2", nullptr)
OPTION(prefix_1, "gdwarf-3", gdwarf_3, Flag, g_Group, INVALID, nullptr, 0, 0,
       "Generate source-level debug information with dwarf version 3", nullptr)
OPTION(prefix_1, "gdwarf-4", gdwarf_4, Flag, g_Group, INVALID, nullptr, 0, 0,
       "Generate source-level debug information with dwarf version 4", nullptr)
OPTION(prefix_1, "gdwarf-5", gdwarf_5, Flag, g_Group, INVALID, nullptr, 0, 0,
       "Generate source-level debug information with dwarf version 5", nullptr)
OPTION(prefix_1, "gdwarf-aranges", gdwarf_aranges, Flag, g_flags_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gdwarf", gdwarf, Flag, INVALID, gdwarf_4, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Gd", _SLASH_Gd, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set __cdecl as a default calling convention", nullptr)
OPTION(prefix_2, "Ge", _SLASH_Ge, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "GF-", _SLASH_GF_, Flag, cl_Group, fwritable_strings, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable string pooling", nullptr)
OPTION(prefix_1, "gfull", gfull, Flag, g_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "GF", _SLASH_GF, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "ggdb0", ggdb0, Flag, ggdbN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ggdb1", ggdb1, Flag, ggdbN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ggdb2", ggdb2, Flag, ggdbN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ggdb3", ggdb3, Flag, ggdbN_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ggdb", ggdb, Flag, gTune_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "ggnu-pubnames", ggnu_pubnames, Flag, g_flags_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "GH", _SLASH_GH, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Gh", _SLASH_Gh, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "GL-", _SLASH_GL_, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "gline-tables-only", gline_tables_only, Flag, gN_Group, INVALID, nullptr, CoreOption, 0,
       "Emit debug line number tables only", nullptr)
OPTION(prefix_1, "glldb", glldb, Flag, gTune_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "GL", _SLASH_GL, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Gm-", _SLASH_Gm_, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "gmlt", gmlt, Flag, INVALID, gline_tables_only, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gmodules", gmodules, Flag, gN_Group, INVALID, nullptr, 0, 0,
       "Generate debug info with external references to clang modules or precompiled headers", nullptr)
OPTION(prefix_2, "Gm", _SLASH_Gm, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "gno-column-info", gno_column_info, Flag, g_flags_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "gno-record-gcc-switches", gno_record_gcc_switches, Flag, g_flags_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gno-strict-dwarf", gno_strict_dwarf, Flag, g_flags_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gnu-pubnames", gnu_pubnames, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit newer GNU style pubnames", nullptr)
OPTION(prefix_2, "GR-", _SLASH_GR_, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable emission of RTTI data", nullptr)
OPTION(prefix_1, "grecord-gcc-switches", grecord_gcc_switches, Flag, g_flags_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "GR", _SLASH_GR, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable emission of RTTI data", nullptr)
OPTION(prefix_2, "Gr", _SLASH_Gr, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set __fastcall as a default calling convention", nullptr)
OPTION(prefix_2, "GS-", _SLASH_GS_, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable buffer security check", nullptr)
OPTION(prefix_1, "gsce", gsce, Flag, gTune_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gsplit-dwarf", gsplit_dwarf, Flag, g_flags_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gstabs", gstabs, Joined, g_Group, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "gstrict-dwarf", gstrict_dwarf, Flag, g_flags_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "GS", _SLASH_GS, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable buffer security check", nullptr)
OPTION(prefix_2, "Gs", _SLASH_Gs, Joined, cl_Group, mstack_probe_size, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set stack probe size", nullptr)
OPTION(prefix_1, "gtoggle", gtoggle, Flag, g_flags_Group, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_2, "GT", _SLASH_GT, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "guard:", _SLASH_Guard, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "gused", gused, Flag, g_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "gvms", gvms, Joined, g_Group, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_2, "Gv", _SLASH_Gv, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set __vectorcall as a default calling convention", nullptr)
OPTION(prefix_2, "Gw-", _SLASH_Gw_, Flag, cl_Group, fno_data_sections, nullptr, CLOption | DriverOption | CLOption, 0,
       "Don't put each data item in its own section", nullptr)
OPTION(prefix_2, "Gw", _SLASH_Gw, Flag, cl_Group, fdata_sections, nullptr, CLOption | DriverOption | CLOption, 0,
       "Put each data item in its own section", nullptr)
OPTION(prefix_2, "GX-", _SLASH_GX_, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable exception handling", nullptr)
OPTION(prefix_1, "gxcoff", gxcoff, Joined, g_Group, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_2, "GX", _SLASH_GX, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable exception handling", nullptr)
OPTION(prefix_2, "Gy-", _SLASH_Gy_, Flag, cl_Group, fno_function_sections, nullptr, CLOption | DriverOption | CLOption, 0,
       "Don't put each function in its own section", nullptr)
OPTION(prefix_2, "Gy", _SLASH_Gy, Flag, cl_Group, ffunction_sections, nullptr, CLOption | DriverOption | CLOption, 0,
       "Put each function in its own section", nullptr)
OPTION(prefix_2, "GZ", _SLASH_GZ, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Gz", _SLASH_Gz, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set __stdcall as a default calling convention", nullptr)
OPTION(prefix_1, "G", G, JoinedOrSeparate, m_Group, INVALID, nullptr, DriverOption, 0,
       "Put objects of at most <size> bytes into small data section (MIPS / Hexagon)", "<size>")
OPTION(prefix_1, "g", g_Flag, Flag, g_Group, INVALID, nullptr, 0, 0,
       "Generate source-level debug information", nullptr)
OPTION(prefix_1, "header-include-file", header_include_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Filename (or -) to write header include output to", nullptr)
OPTION(prefix_1, "headerpad_max_install_names", headerpad__max__install__names, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "help-hidden", _help_hidden, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "HELP", _SLASH_HELP, Flag, cl_Group, help, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_4, "help", help, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption, 0,
       "Display available options", nullptr)
OPTION(prefix_2, "help", _SLASH_help, Flag, cl_Group, help, nullptr, CLOption | DriverOption | CLOption, 0,
       "Display available options", nullptr)
OPTION(prefix_2, "homeparams", _SLASH_homeparams, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "hotpatch", _SLASH_hotpatch, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "H", H, Flag, Preprocessor_Group, INVALID, nullptr, CC1Option, 0,
       "Show header includes and nesting depth", nullptr)
OPTION(prefix_2, "H", _SLASH_H, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "I-", I_, Flag, I_Group, INVALID, nullptr, 0, 0,
       "Restrict all prior -I flags to double-quoted inclusion and remove current directory from include path", nullptr)
OPTION(prefix_1, "idirafter", idirafter, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Add directory to AFTER include search path", nullptr)
OPTION(prefix_1, "iframeworkwithsysroot", iframeworkwithsysroot, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Add directory to SYSTEM framework search path, absolute paths are relative to -isysroot", "<directory>")
OPTION(prefix_1, "iframework", iframework, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Add directory to SYSTEM framework search path", nullptr)
OPTION(prefix_3, "imacros=", _imacros_EQ, Joined, INVALID, imacros, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "imacros", imacros, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Include macros from file before parsing", "<file>")
OPTION(prefix_1, "image_base", image__base, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "imsvc", _SLASH_imsvc, JoinedOrSeparate, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Add directory to system include search path, as if part of %INCLUDE%", "<dir>")
OPTION(prefix_3, "include-barrier", _include_barrier, Flag, INVALID, I_, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-directory-after=", _include_directory_after_EQ, Joined, INVALID, idirafter, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-directory-after", _include_directory_after, Separate, INVALID, idirafter, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-directory=", _include_directory_EQ, Joined, INVALID, I, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-directory", _include_directory, Separate, INVALID, I, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "include-pch", include_pch, Separate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Include precompiled header file", "<file>")
OPTION(prefix_3, "include-prefix=", _include_prefix_EQ, Joined, INVALID, iprefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-prefix", _include_prefix, Separate, INVALID, iprefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "include-pth", include_pth, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include file before parsing", "<file>")
OPTION(prefix_3, "include-with-prefix-after=", _include_with_prefix_after_EQ, Joined, INVALID, iwithprefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-with-prefix-after", _include_with_prefix_after, Separate, INVALID, iwithprefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-with-prefix-before=", _include_with_prefix_before_EQ, Joined, INVALID, iwithprefixbefore, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-with-prefix-before", _include_with_prefix_before, Separate, INVALID, iwithprefixbefore, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-with-prefix=", _include_with_prefix_EQ, Joined, INVALID, iwithprefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include-with-prefix", _include_with_prefix, Separate, INVALID, iwithprefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "include=", _include_EQ, Joined, INVALID, include, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "include", include, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Include file before parsing", "<file>")
OPTION(prefix_1, "index-header-map", index_header_map, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Make the next included directory (-I or -F) an indexer header map", nullptr)
OPTION(prefix_1, "init-only", init_only, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Only execute frontend initialization", nullptr)
OPTION(prefix_1, "init", init, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "install_name", install__name, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "integrated-as", anonymous_21, Flag, INVALID, fintegrated_as, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "internal-externc-isystem", internal_externc_isystem, JoinedOrSeparate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add directory to the internal system include search path with implicit extern \"C\" semantics; these are assumed to not be user-provided and are used to model system and standard headers' paths.", "<directory>")
OPTION(prefix_1, "internal-isystem", internal_isystem, JoinedOrSeparate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add directory to the internal system include search path; these are assumed to not be user-provided and are used to model system and standard headers' paths.", "<directory>")
OPTION(prefix_1, "iprefix", iprefix, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Set the -iwithprefix/-iwithprefixbefore prefix", "<dir>")
OPTION(prefix_1, "iquote", iquote, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Add directory to QUOTE include search path", "<directory>")
OPTION(prefix_1, "isysroot", isysroot, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Set the system root directory (usually /)", "<dir>")
OPTION(prefix_1, "isystem-after", isystem_after, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, DriverOption, 0,
       "Add directory to end of the SYSTEM include search path", "<directory>")
OPTION(prefix_1, "isystem", isystem, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Add directory to SYSTEM include search path", "<directory>")
OPTION(prefix_1, "ivfsoverlay", ivfsoverlay, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Overlay the virtual filesystem described by file over the real file system", nullptr)
OPTION(prefix_1, "iwithprefixbefore", iwithprefixbefore, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Set directory to include search path with prefix", "<dir>")
OPTION(prefix_1, "iwithprefix", iwithprefix, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Set directory to SYSTEM include search path with prefix", "<dir>")
OPTION(prefix_1, "iwithsysroot", iwithsysroot, JoinedOrSeparate, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Add directory to SYSTEM include search path, absolute paths are relative to -isysroot", "<directory>")
OPTION(prefix_1, "I", I, JoinedOrSeparate, I_Group, INVALID, nullptr, CC1Option | CC1AsOption, 0,
       "Add directory to include search path", "<dir>")
OPTION(prefix_2, "I", _SLASH_I, JoinedOrSeparate, cl_Group, I, nullptr, CLOption | DriverOption | CLOption, 0,
       "Add directory to include search path", "<dir>")
OPTION(prefix_1, "i", i, Joined, i_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "J", J, JoinedOrSeparate, gfortran_Group, INVALID, nullptr, RenderJoined, 0, nullptr, nullptr)
OPTION(prefix_2, "J", _SLASH_J, Flag, cl_Group, funsigned_char, nullptr, CLOption | DriverOption | CLOption, 0,
       "Make char type unsigned", nullptr)
OPTION(prefix_1, "keep_private_externs", keep__private__externs, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "kernel-", _SLASH_kernel_, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "kernel", _SLASH_kernel, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_3, "language=", _language_EQ, Joined, INVALID, x, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "language", _language, Separate, INVALID, x, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "lazy_framework", lazy__framework, Separate, INVALID, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_1, "lazy_library", lazy__library, Separate, INVALID, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_2, "LDd", _SLASH_LDd, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Create debug DLL", nullptr)
OPTION(prefix_2, "LD", _SLASH_LD, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Create DLL", nullptr)
OPTION(prefix_3, "library-directory=", _library_directory_EQ, Joined, INVALID, L, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "library-directory", _library_directory, Separate, INVALID, L, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "linker-option=", linker_option, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add linker option", nullptr)
OPTION(prefix_2, "link", _SLASH_link, RemainingArgsJoined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Forward options to the linker", "<options>")
OPTION(prefix_2, "LN", _SLASH_LN, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "load", load, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Load the named plugin (dynamic shared object)", "<dsopath>")
OPTION(prefix_1, "L", L, JoinedOrSeparate, Link_Group, INVALID, nullptr, RenderJoined, 0,
       "Add directory to library search path", "<dir>")
OPTION(prefix_1, "l", l, JoinedOrSeparate, Link_Group, INVALID, nullptr, LinkerInput | RenderJoined, 0, nullptr, nullptr)
OPTION(prefix_1, "m16", m16, Flag, m_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "m32", m32, Flag, m_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "m3dnowa", m3dnowa, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "m3dnow", m3dnow, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "m64", m64, Flag, m_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "m80387", m80387, Flag, INVALID, mx87, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mabi=", mabi_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mabicalls", mabicalls, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Enable SVR4-style position-independent code (Mips only)", nullptr)
OPTION(prefix_1, "Mach", Mach, Flag, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "madx", madx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "maes", maes, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "main-file-name", main_file_name, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Main file name to use for debug info", nullptr)
OPTION(prefix_1, "malign-double", malign_double, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Align doubles to two words in structs (x86 only)", nullptr)
OPTION(prefix_1, "malign-functions=", malign_functions_EQ, Joined, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "malign-jumps=", malign_jumps_EQ, Joined, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "malign-loops=", malign_loops_EQ, Joined, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "maltivec", maltivec, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mamdgpu-debugger-abi=", mamdgpu_debugger_abi, Joined, m_Group, INVALID, nullptr, HelpHidden, 0,
       "Generate additional code for specified <version> of debugger ABI (AMDGPU only)", "<version>")
OPTION(prefix_1, "mappletvos-version-min=", mappletvos_version_min_EQ, Joined, INVALID, mtvos_version_min_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mappletvsimulator-version-min=", mappletvsimulator_version_min_EQ, Joined, INVALID, mtvos_simulator_version_min_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "march=", march_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "marm", marm, Flag, INVALID, mno_thumb, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "masm-verbose", masm_verbose, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate verbose assembly output", nullptr)
OPTION(prefix_1, "masm=", masm_EQ, Joined, m_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "massembler-fatal-warnings", massembler_fatal_warnings, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Make assembler warnings fatal", nullptr)
OPTION(prefix_1, "mavx2", mavx2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512bw", mavx512bw, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512cd", mavx512cd, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512dq", mavx512dq, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512er", mavx512er, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512f", mavx512f, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512ifma", mavx512ifma, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512pf", mavx512pf, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512vbmi", mavx512vbmi, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx512vl", mavx512vl, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mavx", mavx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mbackchain", mbackchain, Flag, m_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Link stack frames through backchain on System Z", nullptr)
OPTION(prefix_1, "mbig-endian", mbig_endian, Flag, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mbmi2", mbmi2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mbmi", mbmi, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mcheck-zero-division", mcheck_zero_division, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mclflushopt", mclflushopt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mclwb", mclwb, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mclzero", mclzero, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mcmodel=", mcmodel_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mcmpb", mcmpb, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mcode-model", mcode_model, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "The code model to use", nullptr)
OPTION(prefix_1, "mcompact-branches=", mcompact_branches_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mconsole", mconsole, Joined, m_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mconstant-cfstrings", mconstant_cfstrings, Flag, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "mconstructor-aliases", mconstructor_aliases, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Emit complete constructors and destructors as aliases when possible", nullptr)
OPTION(prefix_1, "mcpu=", mcpu_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mcrbits", mcrbits, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mcrc", mcrc, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Allow use of CRC instructions (ARM only)", nullptr)
OPTION(prefix_1, "mcrypto", mpower8_crypto, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mcx16", mcx16, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_2, "MDd", _SLASH_MDd, Flag, _SLASH_M_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Use DLL debug run-time", nullptr)
OPTION(prefix_1, "mdebug-pass", mdebug_pass, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable additional debug output", nullptr)
OPTION(prefix_1, "mdirect-move", mdirect_move, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mdisable-fp-elim", mdisable_fp_elim, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable frame pointer elimination optimization", nullptr)
OPTION(prefix_1, "mdisable-tail-calls", mdisable_tail_calls, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable tail call optimization, keeping the call stack accurate", nullptr)
OPTION(prefix_1, "mdll", mdll, Joined, m_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mdouble-float", mdouble_float, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mdspr2", mdspr2, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mdsp", mdsp, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mdynamic-no-pic", mdynamic_no_pic, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "MD", MD, Flag, M_Group, INVALID, nullptr, 0, 0,
       "Write a depfile containing user and system headers", nullptr)
OPTION(prefix_2, "MD", _SLASH_MD, Flag, _SLASH_M_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Use DLL run-time", nullptr)
OPTION(prefix_1, "meabi", meabi, Separate, m_Group, INVALID, nullptr, CC1Option, 0,
       "Set EABI type, e.g. 4, 5 or gnu (default depends on triple)", nullptr)
OPTION(prefix_1, "menable-no-infs", menable_no_infinities, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Allow optimization to assume there are no infinities.", nullptr)
OPTION(prefix_1, "menable-no-nans", menable_no_nans, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Allow optimization to assume there are no NaNs.", nullptr)
OPTION(prefix_1, "menable-unsafe-fp-math", menable_unsafe_fp_math, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Allow unsafe floating-point math optimizations which may decrease precision", nullptr)
OPTION(prefix_1, "mexecute-only", mexecute_only, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Disallow generation of data access to code sections (ARM only)", nullptr)
OPTION(prefix_1, "mf16c", mf16c, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mfancy-math-387", mfancy_math_387, Flag, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "mfentry", mfentry, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "insert calls to fentry at function entry (x86 only)", nullptr)
OPTION(prefix_1, "mfix-and-continue", mfix_and_continue, Flag, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "mfix-cortex-a53-835769", mfix_cortex_a53_835769, Flag, m_aarch64_Features_Group, INVALID, nullptr, 0, 0,
       "Workaround Cortex-A53 erratum 835769 (AArch64 only)", nullptr)
OPTION(prefix_1, "mfloat-abi=", mfloat_abi_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mfloat-abi", mfloat_abi, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "The float ABI to use", nullptr)
OPTION(prefix_1, "mfloat128", mfloat128, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mfma4", mfma4, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mfma", mfma, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mfp32", mfp32, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Use 32-bit floating point registers (MIPS only)", nullptr)
OPTION(prefix_1, "mfp64", mfp64, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Use 64-bit floating point registers (MIPS only)", nullptr)
OPTION(prefix_1, "mfpmath=", mfpmath_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mfpmath", mfpmath, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Which unit to use for fp math", nullptr)
OPTION(prefix_1, "mfprnd", mfprnd, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mfpu=", mfpu_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mfpxx", mfpxx, Flag, m_Group, INVALID, nullptr, HelpHidden, 0,
       "Avoid FPU mode dependent operations when used with the O32 ABI", nullptr)
OPTION(prefix_1, "mfsgsbase", mfsgsbase, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mfxsr", mfxsr, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "MF", MF, JoinedOrSeparate, M_Group, INVALID, nullptr, 0, 0,
       "Write depfile output from -MMD, -MD, -MM, or -M to <file>", "<file>")
OPTION(prefix_1, "mgeneral-regs-only", mgeneral_regs_only, Flag, m_aarch64_Features_Group, INVALID, nullptr, 0, 0,
       "Generate code which only uses the general purpose registers (AArch64 only)", nullptr)
OPTION(prefix_1, "mglibc", mglibc, Flag, m_libc_Group, INVALID, nullptr, HelpHidden | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "mglobal-merge", mglobal_merge, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Enable merging of globals", nullptr)
OPTION(prefix_1, "MG", MG, Flag, M_Group, INVALID, nullptr, CC1Option, 0,
       "Add missing headers to depfile", nullptr)
OPTION(prefix_1, "mhard-float", mhard_float, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mhtm", mhtm, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mhvx-double", mhexagon_hvx_double, Flag, m_hexagon_Features_Group, INVALID, nullptr, CC1Option, 0,
       "Enable Hexagon Double Vector eXtensions", nullptr)
OPTION(prefix_1, "mhvx", mhexagon_hvx, Flag, m_hexagon_Features_Group, INVALID, nullptr, CC1Option, 0,
       "Enable Hexagon Vector eXtensions", nullptr)
OPTION(prefix_3, "mhwdiv=", _mhwdiv_EQ, Joined, INVALID, mhwdiv_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mhwdiv=", mhwdiv_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "mhwdiv", _mhwdiv, Separate, INVALID, mhwdiv_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "miamcu", miamcu, Flag, m_Group, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Use Intel MCU ABI", nullptr)
OPTION(prefix_1, "mieee-fp", mieee_fp, Flag, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "mieee-rnd-near", mieee_rnd_near, Flag, m_hexagon_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "migrate", _migrate, Flag, INVALID, INVALID, nullptr, DriverOption, 0,
       "Run the migrator", nullptr)
OPTION(prefix_1, "migrate", migrate, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Migrate source code", nullptr)
OPTION(prefix_1, "mimplicit-float", mimplicit_float, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mimplicit-it=", mimplicit_it_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mincremental-linker-compatible", mincremental_linker_compatible, Flag, m_Group, INVALID, nullptr, CC1Option | CC1AsOption, 0,
       "(integrated-as) Emit an object file which can be used with an incremental linker", nullptr)
OPTION(prefix_1, "minline-all-stringops", minline_all_stringops, Flag, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "minvariant-function-descriptors", minvariant_function_descriptors, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mios-simulator-version-min=", mios_simulator_version_min_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mios-version-min=", mios_version_min_EQ, Joined, INVALID, miphoneos_version_min_EQ, nullptr, 0, 0,
       "Set iOS deployment target", nullptr)
OPTION(prefix_1, "miphoneos-version-min=", miphoneos_version_min_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "miphonesimulator-version-min=", miphonesimulator_version_min_EQ, Joined, INVALID, mios_simulator_version_min_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mips16", mips16, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mips1", mips1, Flag, INVALID, march_EQ, "mips1\0", HelpHidden, 0,
       "Equivalent to -march=mips1", nullptr)
OPTION(prefix_1, "mips2", mips2, Flag, INVALID, march_EQ, "mips2\0", HelpHidden, 0,
       "Equivalent to -march=mips2", nullptr)
OPTION(prefix_1, "mips32r2", mips32r2, Flag, INVALID, march_EQ, "mips32r2\0", HelpHidden, 0,
       "Equivalent to -march=mips32r2", nullptr)
OPTION(prefix_1, "mips32r3", mips32r3, Flag, INVALID, march_EQ, "mips32r3\0", HelpHidden, 0,
       "Equivalent to -march=mips32r3", nullptr)
OPTION(prefix_1, "mips32r5", mips32r5, Flag, INVALID, march_EQ, "mips32r5\0", HelpHidden, 0,
       "Equivalent to -march=mips32r5", nullptr)
OPTION(prefix_1, "mips32r6", mips32r6, Flag, INVALID, march_EQ, "mips32r6\0", HelpHidden, 0,
       "Equivalent to -march=mips32r6", nullptr)
OPTION(prefix_1, "mips32", mips32, Flag, INVALID, march_EQ, "mips32\0", HelpHidden, 0,
       "Equivalent to -march=mips32", nullptr)
OPTION(prefix_1, "mips3", mips3, Flag, INVALID, march_EQ, "mips3\0", HelpHidden, 0,
       "Equivalent to -march=mips3", nullptr)
OPTION(prefix_1, "mips4", mips4, Flag, INVALID, march_EQ, "mips4\0", HelpHidden, 0,
       "Equivalent to -march=mips4", nullptr)
OPTION(prefix_1, "mips5", mips5, Flag, INVALID, march_EQ, "mips5\0", HelpHidden, 0,
       "Equivalent to -march=mips5", nullptr)
OPTION(prefix_1, "mips64r2", mips64r2, Flag, INVALID, march_EQ, "mips64r2\0", HelpHidden, 0,
       "Equivalent to -march=mips64r2", nullptr)
OPTION(prefix_1, "mips64r3", mips64r3, Flag, INVALID, march_EQ, "mips64r3\0", HelpHidden, 0,
       "Equivalent to -march=mips64r3", nullptr)
OPTION(prefix_1, "mips64r5", mips64r5, Flag, INVALID, march_EQ, "mips64r5\0", HelpHidden, 0,
       "Equivalent to -march=mips64r5", nullptr)
OPTION(prefix_1, "mips64r6", mips64r6, Flag, INVALID, march_EQ, "mips64r6\0", HelpHidden, 0,
       "Equivalent to -march=mips64r6", nullptr)
OPTION(prefix_1, "mips64", mips64, Flag, INVALID, march_EQ, "mips64\0", HelpHidden, 0,
       "Equivalent to -march=mips64", nullptr)
OPTION(prefix_1, "misel", misel, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "MJ", MJ, JoinedOrSeparate, M_Group, INVALID, nullptr, 0, 0,
       "Write a compilation database entry per input", nullptr)
OPTION(prefix_1, "mkernel", mkernel, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mldc1-sdc1", mldc1_sdc1, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mlimit-float-precision", mlimit_float_precision, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Limit float precision to the given value", nullptr)
OPTION(prefix_1, "mlink-bitcode-file", mlink_bitcode_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Link the given bitcode file before performing optimizations.", nullptr)
OPTION(prefix_1, "mlink-cuda-bitcode", mlink_cuda_bitcode, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Link and internalize needed symbols from the given bitcode file before performing optimizations.", nullptr)
OPTION(prefix_1, "mlinker-version=", mlinker_version_EQ, Joined, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mlittle-endian", mlittle_endian, Flag, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mllvm", mllvm, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | CoreOption, 0,
       "Additional arguments to forward to LLVM's option processing", nullptr)
OPTION(prefix_1, "mlong-calls", mlong_calls, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Generate branches with extended addressability, usually via indirect jumps.", nullptr)
OPTION(prefix_1, "mlongcall", mlongcall, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mlzcnt", mlzcnt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mmacosx-version-min=", mmacosx_version_min_EQ, Joined, m_Group, INVALID, nullptr, 0, 0,
       "Set Mac OS X deployment target", nullptr)
OPTION(prefix_1, "MMD", MMD, Flag, M_Group, INVALID, nullptr, 0, 0,
       "Write a depfile containing user headers", nullptr)
OPTION(prefix_1, "mmfcrf", mmfcrf, Flag, INVALID, mmfocrf, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mmfocrf", mmfocrf, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mmicromips", mmicromips, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mmmx", mmmx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mmovbe", mmovbe, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mmpx", mmpx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mms-bitfields", mms_bitfields, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Set the default structure layout to be compatible with the Microsoft compiler standard", nullptr)
OPTION(prefix_1, "mmsa", mmsa, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Enable MSA ASE (MIPS only)", nullptr)
OPTION(prefix_1, "mmwaitx", mmwaitx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "MM", MM, Flag, M_Group, INVALID, nullptr, 0, 0,
       "Like -MMD, but also implies -E and writes to stdout by default", nullptr)
OPTION(prefix_1, "mnan=", mnan_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-3dnowa", mno_3dnowa, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-3dnow", mno_3dnow, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-80387", mno_80387, Flag, INVALID, mno_x87, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-abicalls", mno_abicalls, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Disable SVR4-style position-independent code (Mips only)", nullptr)
OPTION(prefix_1, "mno-adx", mno_adx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-aes", mno_aes, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-altivec", mno_altivec, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx2", mno_avx2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512bw", mno_avx512bw, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512cd", mno_avx512cd, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512dq", mno_avx512dq, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512er", mno_avx512er, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512f", mno_avx512f, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512ifma", mno_avx512ifma, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512pf", mno_avx512pf, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512vbmi", mno_avx512vbmi, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx512vl", mno_avx512vl, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-avx", mno_avx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-backchain", mno_backchain, Flag, m_Group, INVALID, nullptr, DriverOption | CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-bmi2", mno_bmi2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-bmi", mno_bmi, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-check-zero-division", mno_check_zero_division, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-clflushopt", mno_clflushopt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-clwb", mno_clwb, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-clzero", mno_clzero, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-cmpb", mno_cmpb, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-constant-cfstrings", mno_constant_cfstrings, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-crbits", mno_crbits, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-crypto", mnopower8_crypto, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-cx16", mno_cx16, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-direct-move", mnodirect_move, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-dspr2", mno_dspr2, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-dsp", mno_dsp, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-execute-only", mno_execute_only, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Allow generation of data access to code sections (ARM only)", nullptr)
OPTION(prefix_1, "mno-f16c", mno_f16c, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-fix-cortex-a53-835769", mno_fix_cortex_a53_835769, Flag, m_aarch64_Features_Group, INVALID, nullptr, 0, 0,
       "Don't workaround Cortex-A53 erratum 835769 (AArch64 only)", nullptr)
OPTION(prefix_1, "mno-float128", mno_float128, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-fma4", mno_fma4, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-fma", mno_fma, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-fprnd", mno_fprnd, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-fsgsbase", mno_fsgsbase, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-fxsr", mno_fxsr, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-global-merge", mno_global_merge, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Disable merging of globals", nullptr)
OPTION(prefix_1, "mno-htm", mno_htm, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-hvx-double", mno_hexagon_hvx_double, Flag, m_hexagon_Features_Group, INVALID, nullptr, CC1Option, 0,
       "Disable Hexagon Double Vector eXtensions", nullptr)
OPTION(prefix_1, "mno-hvx", mno_hexagon_hvx, Flag, m_hexagon_Features_Group, INVALID, nullptr, CC1Option, 0,
       "Disable Hexagon Vector eXtensions", nullptr)
OPTION(prefix_1, "mno-iamcu", mno_iamcu, Flag, m_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-implicit-float", mno_implicit_float, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Don't generate implicit floating point instructions", nullptr)
OPTION(prefix_1, "mno-incremental-linker-compatible", mno_incremental_linker_compatible, Flag, m_Group, INVALID, nullptr, 0, 0,
       "(integrated-as) Emit an object file which cannot be used with an incremental linker", nullptr)
OPTION(prefix_1, "mno-inline-all-stringops", mno_inline_all_stringops, Flag, clang_ignored_m_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-invariant-function-descriptors", mno_invariant_function_descriptors, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-isel", mno_isel, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-ldc1-sdc1", mno_ldc1_sdc1, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-long-calls", mno_long_calls, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Restore the default behaviour of not generating long calls", nullptr)
OPTION(prefix_1, "mno-longcall", mno_longcall, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-lzcnt", mno_lzcnt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-mfcrf", mno_mfcrf, Flag, INVALID, mno_mfocrf, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-mfocrf", mno_mfocrf, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-micromips", mno_micromips, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-mips16", mno_mips16, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-mmx", mno_mmx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-movbe", mno_movbe, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-movt", mno_movt, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Disallow use of movt/movw pairs (ARM only)", nullptr)
OPTION(prefix_1, "mno-mpx", mno_mpx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-ms-bitfields", mno_ms_bitfields, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Do not set the default structure layout to be compatible with the Microsoft compiler standard", nullptr)
OPTION(prefix_1, "mno-msa", mno_msa, Flag, m_Group, INVALID, nullptr, 0, 0,
       "Disable MSA ASE (MIPS only)", nullptr)
OPTION(prefix_1, "mno-mwaitx", mno_mwaitx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-neg-immediates", mno_neg_immediates, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Disallow converting instructions with negative immediates to their negation or inversion.", nullptr)
OPTION(prefix_1, "mno-odd-spreg", mno_odd_spreg, Flag, m_Group, INVALID, nullptr, HelpHidden, 0,
       "Disable odd single-precision floating point registers", nullptr)
OPTION(prefix_1, "mno-omit-leaf-frame-pointer", mno_omit_leaf_frame_pointer, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-pascal-strings", mno_pascal_strings, Flag, INVALID, fno_pascal_strings, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-pclmul", mno_pclmul, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-pie-copy-relocations", mno_pie_copy_relocations, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-pku", mno_pku, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-popcntd", mno_popcntd, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-popcnt", mno_popcnt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-power8-vector", mno_power8_vector, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-power9-vector", mno_power9_vector, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-prefetchwt1", mno_prefetchwt1, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-prfchw", mno_prfchw, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-pure-code", mno_pure_code, Flag, INVALID, mno_execute_only, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-qpx", mno_qpx, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-rdrnd", mno_rdrnd, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-rdseed", mno_rdseed, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-red-zone", mno_red_zone, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-relax-all", mno_relax_all, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-restrict-it", mno_restrict_it, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Allow generation of deprecated IT blocks for ARMv8. It is off by default for ARMv8 Thumb mode", nullptr)
OPTION(prefix_1, "mno-rtd", mno_rtd, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-rtm", mno_rtm, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sgx", mno_sgx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sha", mno_sha, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-simd128", mno_simd128, Flag, m_wasm_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-soft-float", mno_soft_float, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sse2", mno_sse2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sse3", mno_sse3, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sse4.1", mno_sse4_1, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sse4.2", mno_sse4_2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sse4a", mno_sse4a, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sse4", mno_sse4, Flag, INVALID, mno_sse4_1, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-sse", mno_sse, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-ssse3", mno_ssse3, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-stackrealign", mno_stackrealign, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-tbm", mno_tbm, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-thumb", mno_thumb, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-unaligned-access", mno_unaligned_access, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Force all memory accesses to be aligned (AArch32/AArch64 only)", nullptr)
OPTION(prefix_1, "mno-vsx", mno_vsx, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-vx", mno_vx, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-warn-nonportable-cfstrings", mno_warn_nonportable_cfstrings, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-x87", mno_x87, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-xgot", mno_xgot, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-xop", mno_xop, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-xsavec", mno_xsavec, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-xsaveopt", mno_xsaveopt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-xsaves", mno_xsaves, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-xsave", mno_xsave, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mno-zero-initialized-in-bss", mno_zero_initialized_in_bss, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not put zero initialized data in the BSS", nullptr)
OPTION(prefix_1, "mno-zvector", mno_zvector, Flag, INVALID, fno_zvector, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mnocrc", mnocrc, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Disallow use of CRC instructions (ARM only)", nullptr)
OPTION(prefix_1, "mnoexecstack", mno_exec_stack, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Mark the file as not needing an executable stack", nullptr)
OPTION(prefix_1, "modd-spreg", modd_spreg, Flag, m_Group, INVALID, nullptr, HelpHidden, 0,
       "Enable odd single-precision floating point registers", nullptr)
OPTION(prefix_1, "module-dependency-dir", module_dependency_dir, Separate, INVALID, INVALID, nullptr, CC1Option, 0,
       "Directory to dump module dependencies to", nullptr)
OPTION(prefix_1, "module-file-deps", module_file_deps, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include module files in dependency output", nullptr)
OPTION(prefix_1, "module-file-info", module_file_info, Flag, Action_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Provide information about a particular module file", nullptr)
OPTION(prefix_1, "momit-leaf-frame-pointer", momit_leaf_frame_pointer, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Omit frame pointer setup for leaf functions", nullptr)
OPTION(prefix_1, "moslib=", moslib_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mpascal-strings", mpascal_strings, Flag, INVALID, fpascal_strings, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mpclmul", mpclmul, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mpie-copy-relocations", mpie_copy_relocations, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Use copy relocations support for PIE builds", nullptr)
OPTION(prefix_1, "mpku", mpku, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mpopcntd", mpopcntd, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mpopcnt", mpopcnt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mpower8-vector", mpower8_vector, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mpower9-vector", mpower9_vector, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mprefetchwt1", mprefetchwt1, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mprfchw", mprfchw, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mpure-code", mpure_code, Flag, INVALID, mexecute_only, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "MP", MP, Flag, M_Group, INVALID, nullptr, CC1Option, 0,
       "Create phony target for each dependency (other than main file)", nullptr)
OPTION(prefix_2, "MP", _SLASH_MP, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mqdsp6-compat", mqdsp6_compat, Flag, m_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Enable hexagon-qdsp6 backward compatibility", nullptr)
OPTION(prefix_1, "mqpx", mqpx, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "MQ", MQ, JoinedOrSeparate, M_Group, INVALID, nullptr, CC1Option, 0,
       "Specify name of main file output to quote in depfile", nullptr)
OPTION(prefix_1, "mrdrnd", mrdrnd, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mrdseed", mrdseed, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mrecip=", mrecip_EQ, CommaJoined, m_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "mrecip", mrecip, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mred-zone", mred_zone, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mregparm=", mregparm_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mregparm", mregparm, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Limit the number of registers available for integer arguments", nullptr)
OPTION(prefix_1, "mrelax-all", mrelax_all, Flag, m_Group, INVALID, nullptr, CC1Option | CC1AsOption, 0,
       "(integrated-as) Relax all machine instructions", nullptr)
OPTION(prefix_3, "mrelax-relocations", mrelax_relocations, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Use relaxable elf relocations", nullptr)
OPTION(prefix_1, "mrelocation-model", mrelocation_model, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "The relocation model to use", nullptr)
OPTION(prefix_1, "mrestrict-it", mrestrict_it, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Disallow generation of deprecated IT blocks for ARMv8. It is on by default for ARMv8 Thumb mode.", nullptr)
OPTION(prefix_1, "mrtd", mrtd, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Make StdCall calling convention the default", nullptr)
OPTION(prefix_1, "mrtm", mrtm, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msave-temp-labels", msave_temp_labels, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Save temporary labels in the symbol table. Note this may change .s semantics and shouldn't generally be used on compiler-generated code.", nullptr)
OPTION(prefix_1, "msgx", msgx, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msha", msha, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msimd128", msimd128, Flag, m_wasm_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "msingle-float", msingle_float, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "msmall-data-threshold=", msmall_data_threshold_EQ, Joined, m_Group, G, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "msoft-float", msoft_float, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Use software floating point", nullptr)
OPTION(prefix_1, "msse2", msse2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msse3", msse3, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msse4.1", msse4_1, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msse4.2", msse4_2, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msse4a", msse4a, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "msse4", msse4, Flag, INVALID, msse4_2, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "msse", msse, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mssse3", mssse3, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mstack-alignment=", mstack_alignment, Joined, m_Group, INVALID, nullptr, CC1Option, 0,
       "Set the stack alignment", nullptr)
OPTION(prefix_1, "mstack-probe-size=", mstack_probe_size, Joined, m_Group, INVALID, nullptr, CC1Option, 0,
       "Set the stack probe size", nullptr)
OPTION(prefix_1, "mstackrealign", mstackrealign, Flag, m_Group, INVALID, nullptr, CC1Option, 0,
       "Force realign the stack at entry to every function", nullptr)
OPTION(prefix_1, "mstrict-align", mstrict_align, Flag, INVALID, mno_unaligned_access, nullptr, CC1Option | HelpHidden, 0,
       "Force all memory accesses to be aligned (same as mno-unaligned-access)", nullptr)
OPTION(prefix_1, "mt-migrate-directory", mt_migrate_directory, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Directory for temporary files produced during ARC or ObjC migration", nullptr)
OPTION(prefix_1, "mtbm", mtbm, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_2, "MTd", _SLASH_MTd, Flag, _SLASH_M_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Use static debug run-time", nullptr)
OPTION(prefix_1, "mthread-model", mthread_model, Separate, m_Group, INVALID, nullptr, CC1Option, 0,
       "The thread model to use, e.g. posix, single (posix by default)", nullptr)
OPTION(prefix_1, "mthreads", mthreads, Joined, m_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mthumb", mthumb, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mtune=", mtune_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mtvos-simulator-version-min=", mtvos_simulator_version_min_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mtvos-version-min=", mtvos_version_min_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "MT", MT, JoinedOrSeparate, M_Group, INVALID, nullptr, CC1Option, 0,
       "Specify name of main file output in depfile", nullptr)
OPTION(prefix_2, "MT", _SLASH_MT, Flag, _SLASH_M_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Use static run-time", nullptr)
OPTION(prefix_1, "muclibc", muclibc, Flag, m_libc_Group, INVALID, nullptr, HelpHidden | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "multi_module", multi__module, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "multiply_defined_unused", multiply__defined__unused, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "multiply_defined", multiply__defined, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "munaligned-access", munaligned_access, Flag, m_arm_Features_Group, INVALID, nullptr, 0, 0,
       "Allow memory accesses to be unaligned (AArch32/AArch64 only)", nullptr)
OPTION(prefix_1, "municode", municode, Joined, m_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "munwind-tables", munwind_tables, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate unwinding tables for all functions", nullptr)
OPTION(prefix_1, "mv4", mv4, Flag, m_hexagon_Features_Group, mcpu_EQ, "hexagonv4\0", 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mv55", mv55, Flag, m_hexagon_Features_Group, mcpu_EQ, "hexagonv55\0", 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mv5", mv5, Flag, m_hexagon_Features_Group, mcpu_EQ, "hexagonv5\0", 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mv60", mv60, Flag, m_hexagon_Features_Group, mcpu_EQ, "hexagonv60\0", 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mv62", mv62, Flag, m_hexagon_Features_Group, mcpu_EQ, "hexagonv62\0", 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mvsx", mvsx, Flag, m_ppc_Features_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mvx", mvx, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "MV", MV, Flag, M_Group, INVALID, nullptr, CC1Option, 0,
       "Use NMake/Jom format for the depfile", nullptr)
OPTION(prefix_1, "mwarn-nonportable-cfstrings", mwarn_nonportable_cfstrings, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mwatchos-simulator-version-min=", mwatchos_simulator_version_min_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mwatchos-version-min=", mwatchos_version_min_EQ, Joined, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mwatchsimulator-version-min=", mwatchsimulator_version_min_EQ, Joined, INVALID, mwatchos_simulator_version_min_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mwindows", mwindows, Joined, m_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mx32", mx32, Flag, m_Group, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mx87", mx87, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mxgot", mxgot, Flag, m_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "mxop", mxop, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mxsavec", mxsavec, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mxsaveopt", mxsaveopt, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mxsaves", mxsaves, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mxsave", mxsave, Flag, m_x86_Features_Group, INVALID, nullptr, CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "mzvector", mzvector, Flag, INVALID, fzvector, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "M", M, Flag, M_Group, INVALID, nullptr, 0, 0,
       "Like -MD, but also implies -E and writes to stdout by default", nullptr)
OPTION(prefix_1, "no-canonical-prefixes", no_canonical_prefixes, Flag, INVALID, INVALID, nullptr, HelpHidden, 0,
       "Use relative instead of canonical paths", nullptr)
OPTION(prefix_1, "no-code-completion-globals", no_code_completion_globals, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not include global declarations in code-completion results.", nullptr)
OPTION(prefix_1, "no-cpp-precomp", no_cpp_precomp, Flag, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_3, "no-cuda-gpu-arch=", no_cuda_gpu_arch_EQ, Joined, INVALID, INVALID, nullptr, DriverOption, 0,
       "Remove GPU architecture (e.g. sm_35) from the list of GPUs to compile for. 'all' resets the list to its default value.", nullptr)
OPTION(prefix_3, "no-cuda-noopt-device-debug", no_cuda_noopt_device_debug, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "no-cuda-version-check", no_cuda_version_check, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Don't error out if the detected version of the CUDA install is too low for the requested CUDA gpu architecture.", nullptr)
OPTION(prefix_1, "no-emit-llvm-uselists", no_emit_llvm_uselists, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Don't preserve order of LLVM use-lists when serializing", nullptr)
OPTION(prefix_1, "no-finalize-removal", migrator_no_finalize_removal, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not remove finalize method in gc mode", nullptr)
OPTION(prefix_1, "no-implicit-float", no_implicit_float, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Don't generate implicit floating point instructions", nullptr)
OPTION(prefix_1, "no-integrated-as", anonymous_22, Flag, INVALID, fno_integrated_as, nullptr, CC1Option | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_4, "no-integrated-cpp", no_integrated_cpp, Flag, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_3, "no-line-commands", _no_line_commands, Flag, INVALID, P, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "no-ns-alloc-error", migrator_no_nsalloc_error, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not error on use of NSAllocateCollectable/NSReallocateCollectable", nullptr)
OPTION(prefix_4, "no-pedantic", no_pedantic, Flag, pedantic_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "no-pthread", no_pthread, Flag, INVALID, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_3, "no-standard-includes", _no_standard_includes, Flag, INVALID, nostdinc, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "no-standard-libraries", _no_standard_libraries, Flag, INVALID, nostdlib, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "no-struct-path-tbaa", no_struct_path_tbaa, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Turn off struct-path aware Type Based Alias Analysis", nullptr)
OPTION(prefix_3, "no-system-header-prefix=", no_system_header_prefix, Joined, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Treat all #include paths starting with <prefix> as not including a system header.", "<prefix>")
OPTION(prefix_3, "no-system-header-prefix", anonymous_20, Separate, INVALID, no_system_header_prefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "no-undefined", _no_undefined, Flag, INVALID, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_3, "no-warnings", _no_warnings, Flag, INVALID, w, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "no_dead_strip_inits_and_terms", no__dead__strip__inits__and__terms, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nobuiltininc", nobuiltininc, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Disable builtin #include directories", nullptr)
OPTION(prefix_1, "nocpp", nocpp, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nocudainc", nocudainc, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nocudalib", nocudalib, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nodefaultlibs", nodefaultlibs, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nofixprebinding", nofixprebinding, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nolibc", nolibc, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "nologo", _SLASH_nologo, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "nomultidefs", nomultidefs, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nopie", nopie, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "noprebind", noprebind, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "noseglinkedit", noseglinkedit, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nostartfiles", nostartfiles, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nostdinc++", nostdincxx, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Disable standard #include directories for the C++ standard library", nullptr)
OPTION(prefix_1, "nostdinc", nostdinc, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nostdlibinc", nostdlibinc, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nostdlib", nostdlib, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "nostdsysteminc", nostdsysteminc, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Disable standard system #include directories", nullptr)
OPTION(prefix_1, "n", n, Flag, INVALID, INVALID, nullptr, CC1AsOption | NoDriverOption, 0,
       "Don't automatically start assembly file with a text section", nullptr)
OPTION(prefix_1, "O0", O0, Flag, O_Group, INVALID, nullptr, CC1Option | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "O0", _SLASH_O0, Flag, cl_Group, O0, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "O4", O4, Flag, O_Group, INVALID, nullptr, CC1Option | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "ObjC++", ObjCXX, Flag, INVALID, INVALID, nullptr, DriverOption, 0,
       "Treat source input files as Objective-C++ inputs", nullptr)
OPTION(prefix_1, "objc-isystem", objc_isystem, JoinedOrSeparate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add directory to the ObjC SYSTEM include search path", "<directory>")
OPTION(prefix_1, "objcmt-atomic-property", objcmt_atomic_property, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Make migration to 'atomic' properties", nullptr)
OPTION(prefix_1, "objcmt-migrate-all", objcmt_migrate_all, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to modern ObjC", nullptr)
OPTION(prefix_1, "objcmt-migrate-annotation", objcmt_migrate_annotation, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to property and method annotations", nullptr)
OPTION(prefix_1, "objcmt-migrate-designated-init", objcmt_migrate_designated_init, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to infer NS_DESIGNATED_INITIALIZER for initializer methods", nullptr)
OPTION(prefix_1, "objcmt-migrate-instancetype", objcmt_migrate_instancetype, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to infer instancetype for method result type", nullptr)
OPTION(prefix_1, "objcmt-migrate-literals", objcmt_migrate_literals, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to modern ObjC literals", nullptr)
OPTION(prefix_1, "objcmt-migrate-ns-macros", objcmt_migrate_nsmacros, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to NS_ENUM/NS_OPTIONS macros", nullptr)
OPTION(prefix_1, "objcmt-migrate-property-dot-syntax", objcmt_migrate_property_dot_syntax, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration of setter/getter messages to property-dot syntax", nullptr)
OPTION(prefix_1, "objcmt-migrate-property", objcmt_migrate_property, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to modern ObjC property", nullptr)
OPTION(prefix_1, "objcmt-migrate-protocol-conformance", objcmt_migrate_protocol_conformance, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to add protocol conformance on classes", nullptr)
OPTION(prefix_1, "objcmt-migrate-readonly-property", objcmt_migrate_readonly_property, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to modern ObjC readonly property", nullptr)
OPTION(prefix_1, "objcmt-migrate-readwrite-property", objcmt_migrate_readwrite_property, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to modern ObjC readwrite property", nullptr)
OPTION(prefix_1, "objcmt-migrate-subscripting", objcmt_migrate_subscripting, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to modern ObjC subscripting", nullptr)
OPTION(prefix_1, "objcmt-ns-nonatomic-iosonly", objcmt_ns_nonatomic_iosonly, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to use NS_NONATOMIC_IOSONLY macro for setting property's 'atomic' attribute", nullptr)
OPTION(prefix_1, "objcmt-returns-innerpointer-property", objcmt_returns_innerpointer_property, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable migration to annotate property with NS_RETURNS_INNER_POINTER", nullptr)
OPTION(prefix_1, "objcmt-white-list-dir-path=", anonymous_0, Joined, INVALID, objcmt_whitelist_dir_path, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "objcmt-whitelist-dir-path=", objcmt_whitelist_dir_path, Joined, INVALID, INVALID, nullptr, CC1Option, 0,
       "Only modify files with a filename contained in the provided directory path", nullptr)
OPTION(prefix_1, "objcxx-isystem", objcxx_isystem, JoinedOrSeparate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Add directory to the ObjC++ SYSTEM include search path", "<directory>")
OPTION(prefix_1, "ObjC", ObjC, Flag, INVALID, INVALID, nullptr, DriverOption, 0,
       "Treat source input files as Objective-C inputs", nullptr)
OPTION(prefix_1, "object", object, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "Od", _SLASH_Od, Flag, cl_Group, O0, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable optimization", nullptr)
OPTION(prefix_1, "Ofast", Ofast, Joined, O_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_2, "Og", _SLASH_Og, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Oi-", _SLASH_Oi_, Flag, cl_Group, fno_builtin, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable use of builtin functions", nullptr)
OPTION(prefix_2, "Oi", _SLASH_Oi, Flag, cl_Group, fbuiltin, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable use of builtin functions", nullptr)
OPTION(prefix_2, "openmp-", _SLASH_openmp_, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "openmp", _SLASH_openmp, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "opt-record-file", opt_record_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "File name to use for YAML optimization record output", nullptr)
OPTION(prefix_3, "optimize=", _optimize_EQ, Joined, INVALID, O, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "optimize", _optimize, Flag, INVALID, O, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "Os", _SLASH_Os, Flag, cl_Group, O, "s\0", CLOption | DriverOption | CLOption, 0,
       "Optimize for size", nullptr)
OPTION(prefix_2, "Ot", _SLASH_Ot, Flag, cl_Group, O, "2\0", CLOption | DriverOption | CLOption, 0,
       "Optimize for speed", nullptr)
OPTION(prefix_1, "output-asm-variant", output_asm_variant, Separate, INVALID, INVALID, nullptr, CC1AsOption | NoDriverOption, 0,
       "Select the asm variant index to use for output", nullptr)
OPTION(prefix_3, "output-class-directory=", _output_class_directory_EQ, Joined, INVALID, foutput_class_dir_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "output-class-directory", _output_class_directory, Separate, INVALID, foutput_class_dir_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "output=", _output_EQ, Joined, INVALID, o, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "output", _output, Separate, INVALID, o, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "O", O_flag, Flag, INVALID, O, "2\0", CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "O", O, Joined, O_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_2, "O", _SLASH_O, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Optimization level", nullptr)
OPTION(prefix_1, "o", o, JoinedOrSeparate, INVALID, INVALID, nullptr, DriverOption | RenderAsInput | CC1Option | CC1AsOption, 0,
       "Write output to <file>", "<file>")
OPTION(prefix_2, "o", _SLASH_o, JoinedOrSeparate, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set output file or directory (ends in / or \\)", "<file or directory>")
OPTION(prefix_1, "pagezero_size", pagezero__size, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "param=", _param_EQ, Joined, INVALID, _param, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "param", _param, Separate, CompileOnly_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "pass-exit-codes", pass_exit_codes, Flag, INVALID, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_4, "pedantic-errors", pedantic_errors, Flag, pedantic_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_4, "pedantic", pedantic, Flag, pedantic_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "pg", pg, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable mcount instrumentation", nullptr)
OPTION(prefix_1, "pic-is-pie", pic_is_pie, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "File is for a position independent executable", nullptr)
OPTION(prefix_1, "pic-level", pic_level, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Value for __PIC__", nullptr)
OPTION(prefix_1, "pie", pie, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "pipe", pipe, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Use pipes between commands, when possible", nullptr)
OPTION(prefix_1, "plugin-arg-", plugin_arg, JoinedAndSeparate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Pass <arg> to plugin <name>", "<name> <arg>")
OPTION(prefix_1, "plugin", plugin, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use the named plugin action instead of the default action (use \"help\" to list available options)", "<name>")
OPTION(prefix_1, "preamble-bytes=", preamble_bytes_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Assume that the precompiled header is a precompiled preamble covering the first N bytes of the main file", nullptr)
OPTION(prefix_1, "prebind_all_twolevel_modules", prebind__all__twolevel__modules, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "prebind", prebind, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "precompile", _precompile, Flag, Action_Group, INVALID, nullptr, DriverOption, 0,
       "Only precompile the input", nullptr)
OPTION(prefix_3, "prefix=", _prefix_EQ, Joined, INVALID, B, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "prefix", _prefix, Separate, INVALID, B, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "preload", preload, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "preprocess", _preprocess, Flag, INVALID, E, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "print-decl-contexts", print_decl_contexts, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Print DeclContexts and their Decls", nullptr)
OPTION(prefix_3, "print-diagnostic-categories", _print_diagnostic_categories, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "print-file-name=", print_file_name_EQ, Joined, INVALID, INVALID, nullptr, 0, 0,
       "Print the full library path of <file>", "<file>")
OPTION(prefix_3, "print-file-name", _print_file_name, Separate, INVALID, print_file_name_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "print-ivar-layout", print_ivar_layout, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable Objective-C Ivar layout bitmap print trace", nullptr)
OPTION(prefix_4, "print-libgcc-file-name", print_libgcc_file_name, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Print the library path for the currently used compiler runtime library (\"libgcc.a\" or \"libclang_rt.builtins.*.a\")", nullptr)
OPTION(prefix_3, "print-missing-file-dependencies", _print_missing_file_dependencies, Flag, INVALID, MG, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "print-multi-directory", print_multi_directory, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "print-multi-lib", print_multi_lib, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "print-multi-os-directory", print_multi_os_directory, Flag, INVALID, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "print-preamble", print_preamble, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Print the \"preamble\" of a file, which is a candidate for implicit precompiled headers.", nullptr)
OPTION(prefix_4, "print-prog-name=", print_prog_name_EQ, Joined, INVALID, INVALID, nullptr, 0, 0,
       "Print the full program path of <name>", "<name>")
OPTION(prefix_3, "print-prog-name", _print_prog_name, Separate, INVALID, print_prog_name_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "print-resource-dir", print_resource_dir, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Print the resource directory pathname", nullptr)
OPTION(prefix_4, "print-search-dirs", print_search_dirs, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Print the paths used for finding libraries and programs", nullptr)
OPTION(prefix_1, "print-stats", print_stats, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Print performance metrics and statistics", nullptr)
OPTION(prefix_1, "private_bundle", private__bundle, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "profile-blocks", _profile_blocks, Flag, INVALID, a, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "profile", _profile, Flag, INVALID, p, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "pthreads", pthreads, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "pthread", pthread, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Support POSIX threads in generated code", nullptr)
OPTION(prefix_3, "ptxas-path=", ptxas_path_EQ, Joined, i_Group, INVALID, nullptr, 0, 0,
       "Path to ptxas (used for compiling CUDA code)", nullptr)
OPTION(prefix_1, "P", P, Flag, Preprocessor_Group, INVALID, nullptr, CC1Option, 0,
       "Disable linemarker output in -E mode", nullptr)
OPTION(prefix_2, "P", _SLASH_P, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Preprocess to file", nullptr)
OPTION(prefix_1, "p", p, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "Qfast_transcendentals", _SLASH_Qfast_transcendentals, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "QIfist", _SLASH_QIfist, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Qimprecise_fwaits", _SLASH_Qimprecise_fwaits, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "Qn", Qn, Flag, INVALID, INVALID, nullptr, HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "Qpar", _SLASH_Qpar, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "Qunused-arguments", Qunused_arguments, Flag, INVALID, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Don't emit warning for unused driver arguments", nullptr)
OPTION(prefix_2, "Qvec-report", _SLASH_Qvec_report, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Qvec-", _SLASH_Qvec_, Flag, cl_Group, fno_vectorize, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable the loop vectorization passes", nullptr)
OPTION(prefix_2, "Qvec", _SLASH_Qvec, Flag, cl_Group, fvectorize, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable the loop vectorization passes", nullptr)
OPTION(prefix_1, "Q", Q, Flag, INVALID, INVALID, nullptr, HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "rdynamic", rdynamic, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "read_only_relocs", read__only__relocs, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "relaxed-aliasing", relaxed_aliasing, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Turn off Type Based Alias Analysis", nullptr)
OPTION(prefix_4, "relocatable-pch", relocatable_pch, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Whether to build a relocatable precompiled header", nullptr)
OPTION(prefix_1, "remap-file", remap_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Replace the contents of the <from> file with the contents of the <to> file", "<from>;<to>")
OPTION(prefix_1, "remap", remap, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "resource-dir=", resource_dir_EQ, Joined, INVALID, resource_dir, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_1, "resource-dir", resource_dir, Separate, INVALID, INVALID, nullptr, DriverOption | CC1Option | CoreOption | HelpHidden, 0,
       "The directory which holds the compiler resource files", nullptr)
OPTION(prefix_3, "resource=", _resource_EQ, Joined, INVALID, fcompile_resource_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "resource", _resource, Separate, INVALID, fcompile_resource_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "rewrite-legacy-objc", rewrite_legacy_objc, Flag, INVALID, INVALID, nullptr, DriverOption, 0,
       "Rewrite Legacy Objective-C source to C++", nullptr)
OPTION(prefix_1, "rewrite-macros", rewrite_macros, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Expand macros without full preprocessing", nullptr)
OPTION(prefix_1, "rewrite-objc", rewrite_objc, Flag, Action_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Rewrite Objective-C source to C++", nullptr)
OPTION(prefix_1, "rewrite-test", rewrite_test, Flag, Action_Group, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Rewriter playground", nullptr)
OPTION(prefix_1, "Rpass-analysis=", Rpass_analysis_EQ, Joined, R_value_Group, INVALID, nullptr, CC1Option, 0,
       "Report transformation analysis from optimization passes whose name matches the given POSIX regular expression", nullptr)
OPTION(prefix_1, "Rpass-missed=", Rpass_missed_EQ, Joined, R_value_Group, INVALID, nullptr, CC1Option, 0,
       "Report missed transformations by optimization passes whose name matches the given POSIX regular expression", nullptr)
OPTION(prefix_1, "Rpass=", Rpass_EQ, Joined, R_value_Group, INVALID, nullptr, CC1Option, 0,
       "Report transformations performed by optimization passes whose name matches the given POSIX regular expression", nullptr)
OPTION(prefix_1, "rpath", rpath, Separate, Link_Group, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_3, "rsp-quoting=", rsp_quoting, Joined, internal_driver_Group, INVALID, nullptr, CoreOption | DriverOption | HelpHidden, 0,
       "Set the rsp quoting to either 'posix', or 'windows'", nullptr)
OPTION(prefix_2, "RTC", _SLASH_RTC, Joined, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_4, "rtlib=", rtlib_EQ, Joined, INVALID, INVALID, nullptr, 0, 0,
       "Compiler runtime library to use", nullptr)
OPTION(prefix_3, "rtlib", _rtlib, Separate, INVALID, rtlib_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "R", R_Joined, Joined, R_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Enable the specified remark", "<remark>")
OPTION(prefix_1, "r", r, Flag, Link_Group, INVALID, nullptr, LinkerInput | NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_4, "save-stats=", save_stats_EQ, Joined, INVALID, INVALID, nullptr, DriverOption, 0,
       "Save llvm statistics.", nullptr)
OPTION(prefix_4, "save-stats", save_stats, Flag, INVALID, save_stats_EQ, "cwd\0", DriverOption, 0,
       "Save llvm statistics.", nullptr)
OPTION(prefix_4, "save-temps=", save_temps_EQ, Joined, INVALID, INVALID, nullptr, DriverOption, 0,
       "Save intermediate compilation results.", nullptr)
OPTION(prefix_4, "save-temps", save_temps, Flag, INVALID, save_temps_EQ, "cwd\0", DriverOption, 0,
       "Save intermediate compilation results", nullptr)
OPTION(prefix_2, "sdl-", _SLASH_sdl_, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "sdl", _SLASH_sdl, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "sectalign", sectalign, MultiArg, INVALID, INVALID, nullptr, 0, 3, nullptr, nullptr)
OPTION(prefix_1, "sectcreate", sectcreate, MultiArg, INVALID, INVALID, nullptr, 0, 3, nullptr, nullptr)
OPTION(prefix_1, "sectobjectsymbols", sectobjectsymbols, MultiArg, INVALID, INVALID, nullptr, 0, 2, nullptr, nullptr)
OPTION(prefix_1, "sectorder", sectorder, MultiArg, INVALID, INVALID, nullptr, 0, 3, nullptr, nullptr)
OPTION(prefix_1, "seg1addr", seg1addr, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "seg_addr_table_filename", seg__addr__table__filename, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "seg_addr_table", seg__addr__table, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "segaddr", segaddr, MultiArg, INVALID, INVALID, nullptr, 0, 2, nullptr, nullptr)
OPTION(prefix_1, "segcreate", segcreate, MultiArg, INVALID, INVALID, nullptr, 0, 3, nullptr, nullptr)
OPTION(prefix_1, "seglinkedit", seglinkedit, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "segprot", segprot, MultiArg, INVALID, INVALID, nullptr, 0, 3, nullptr, nullptr)
OPTION(prefix_1, "segs_read_only_addr", segs__read__only__addr, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "segs_read_write_addr", segs__read__write__addr, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "segs_read_", segs__read__, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "serialize-diagnostic-file", diagnostic_serialized_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "File for serializing diagnostics in a binary format", "<filename>")
OPTION(prefix_4, "serialize-diagnostics", _serialize_diags, Separate, INVALID, INVALID, nullptr, DriverOption, 0,
       "Serialize compiler diagnostics to a file", nullptr)
OPTION(prefix_1, "shared-libasan", shared_libasan, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "shared-libgcc", shared_libgcc, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "shared", shared, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "show-encoding", show_encoding, Flag, INVALID, INVALID, nullptr, CC1AsOption | NoDriverOption, 0,
       "Show instruction encoding information in transliterate mode", nullptr)
OPTION(prefix_3, "show-includes", show_includes, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Print cl.exe style /showIncludes to stdout", nullptr)
OPTION(prefix_1, "show-inst", show_inst, Flag, INVALID, INVALID, nullptr, CC1AsOption | NoDriverOption, 0,
       "Show internal instruction representation in transliterate mode", nullptr)
OPTION(prefix_2, "showIncludes", _SLASH_showIncludes, Flag, cl_Group, show_includes, nullptr, CLOption | DriverOption | CLOption, 0,
       "Print info about included files to stderr", nullptr)
OPTION(prefix_3, "signed-char", _signed_char, Flag, INVALID, fsigned_char, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "single_module", single__module, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "source-charset:", _SLASH_source_charset, Joined, cl_compile_Group, finput_charset_EQ, nullptr, CLOption | DriverOption, 0,
       "Source encoding, supports only UTF-8", nullptr)
OPTION(prefix_4, "specs=", specs_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "specs", specs, Separate, INVALID, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "split-dwarf-file", split_dwarf_file, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "File name to use for split dwarf debug info output", nullptr)
OPTION(prefix_1, "split-dwarf", split_dwarf, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Split out the dwarf .dwo sections", nullptr)
OPTION(prefix_1, "split-stacks", split_stacks, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Try to use a split stack if possible.", nullptr)
OPTION(prefix_1, "stack-protector-buffer-size", stack_protector_buffer_size, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Lower bound for a buffer to be considered for stack protection", nullptr)
OPTION(prefix_1, "stack-protector", stack_protector, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Enable stack protectors", nullptr)
OPTION(prefix_1, "static-define", static_define, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Should __STATIC__ be defined", nullptr)
OPTION(prefix_1, "static-libgcc", static_libgcc, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "static-libgfortran", static_libgfortran, Flag, gfortran_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "static-libstdc++", static_libstdcxx, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "static", static, Flag, INVALID, INVALID, nullptr, NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_1, "stats-file=", stats_file, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Filename to write statistics to", nullptr)
OPTION(prefix_1, "std-default=", std_default_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "std:", _SLASH_std, Joined, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Language standard to compile for", nullptr)
OPTION(prefix_4, "std=", std_EQ, Joined, CompileOnly_Group, INVALID, nullptr, CC1Option, 0,
       "Language standard to compile for", nullptr)
OPTION(prefix_4, "stdlib=", stdlib_EQ, Joined, INVALID, INVALID, nullptr, CC1Option, 0,
       "C++ standard library to use", nullptr)
OPTION(prefix_3, "stdlib", _stdlib, Separate, INVALID, stdlib_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "std", _std, Separate, INVALID, std_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "sub_library", sub__library, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "sub_umbrella", sub__umbrella, JoinedOrSeparate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "sys-header-deps", sys_header_deps, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Include system headers in dependency output", nullptr)
OPTION(prefix_3, "sysroot=", _sysroot_EQ, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "sysroot", _sysroot, Separate, INVALID, _sysroot_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "system-header-prefix=", system_header_prefix, Joined, clang_i_Group, INVALID, nullptr, CC1Option, 0,
       "Treat all #include paths starting with <prefix> as including a system header.", "<prefix>")
OPTION(prefix_3, "system-header-prefix", anonymous_19, Separate, INVALID, system_header_prefix, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "S", S, Flag, Action_Group, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Only run preprocess and compilation steps", nullptr)
OPTION(prefix_1, "s", s, Flag, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "target-abi", target_abi, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Target a particular ABI type", nullptr)
OPTION(prefix_1, "target-cpu", target_cpu, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Target a specific cpu type", nullptr)
OPTION(prefix_1, "target-feature", target_feature, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Target specific attributes", nullptr)
OPTION(prefix_3, "target-help", _target_help, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "target-linker-version", target_linker_version, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Target linker version", nullptr)
OPTION(prefix_3, "target=", target, Joined, INVALID, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Generate code for the given target", nullptr)
OPTION(prefix_1, "target", target_legacy_spelling, Separate, INVALID, target, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Tbss", Tbss, JoinedOrSeparate, T_Group, INVALID, nullptr, 0, 0,
       "Set starting address of BSS to <addr>", "<addr")
OPTION(prefix_2, "TC", _SLASH_TC, Flag, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Treat all source files as C", nullptr)
OPTION(prefix_2, "Tc", _SLASH_Tc, JoinedOrSeparate, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Specify a C source file", "<filename>")
OPTION(prefix_1, "Tdata", Tdata, JoinedOrSeparate, T_Group, INVALID, nullptr, 0, 0,
       "Set starting address of BSS to <addr>", "<addr")
OPTION(prefix_1, "test-coverage", test_coverage, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Do not generate coverage files or remove coverage changes from IR", nullptr)
OPTION(prefix_1, "time", time, Flag, INVALID, INVALID, nullptr, 0, 0,
       "Time individual commands", nullptr)
OPTION(prefix_1, "token-cache", token_cache, Separate, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Use specified token cache file", "<path>")
OPTION(prefix_2, "TP", _SLASH_TP, Flag, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Treat all source files as C++", nullptr)
OPTION(prefix_2, "Tp", _SLASH_Tp, JoinedOrSeparate, cl_compile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Specify a C++ source file", "<filename>")
OPTION(prefix_3, "trace-includes", _trace_includes, Flag, INVALID, H, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "traditional-cpp", traditional_cpp, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Enable some traditional CPP emulation", nullptr)
OPTION(prefix_4, "traditional", traditional, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_4, "trigraphs", trigraphs, Flag, INVALID, ftrigraphs, nullptr, 0, 0,
       "Process trigraph sequences", nullptr)
OPTION(prefix_1, "trim-egraph", trim_egraph, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Only show error-related paths in the analysis graph", nullptr)
OPTION(prefix_1, "triple=", triple_EQ, Joined, INVALID, triple, nullptr, CC1Option | NoDriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "triple", triple, Separate, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Specify target triple (e.g. i686-apple-darwin9)", nullptr)
OPTION(prefix_1, "Ttext", Ttext, JoinedOrSeparate, T_Group, INVALID, nullptr, 0, 0,
       "Set starting address of BSS to <addr>", "<addr")
OPTION(prefix_1, "twolevel_namespace_hints", twolevel__namespace__hints, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "twolevel_namespace", twolevel__namespace, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "T", T, JoinedOrSeparate, T_Group, INVALID, nullptr, 0, 0,
       "Specify <script> as linker script", "<script>")
OPTION(prefix_1, "t", t, Flag, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "umbrella", umbrella, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "undefine-macro=", _undefine_macro_EQ, Joined, INVALID, U, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "undefine-macro", _undefine_macro, Separate, INVALID, U, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "undefined", undefined, JoinedOrSeparate, u_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "undef", undef, Flag, u_Group, INVALID, nullptr, CC1Option, 0,
       "undef all system defines", nullptr)
OPTION(prefix_1, "unexported_symbols_list", unexported__symbols__list, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "unoptimized-cfg", analysis_UnoptimizedCFG, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Generate unoptimized CFGs for all analyses", nullptr)
OPTION(prefix_3, "unsigned-char", _unsigned_char, Flag, INVALID, funsigned_char, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "user-dependencies", _user_dependencies, Flag, INVALID, MM, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "utf-8", _SLASH_utf8, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Set source and runtime encoding to UTF-8 (default)", nullptr)
OPTION(prefix_1, "U", U, JoinedOrSeparate, Preprocessor_Group, INVALID, nullptr, CC1Option, 0,
       "Undefine macro <macro>", "<macro>")
OPTION(prefix_2, "U", _SLASH_U, JoinedOrSeparate, cl_Group, U, nullptr, CLOption | DriverOption | CLOption, 0,
       "Undefine macro", "<macro>")
OPTION(prefix_1, "u", u, JoinedOrSeparate, u_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_2, "u", _SLASH_u, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "validate-charset-", _SLASH_validate_charset_, Flag, cl_Group, W_Joined, "no-invalid-source-encoding\0", CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "validate-charset", _SLASH_validate_charset, Flag, cl_Group, W_Joined, "invalid-source-encoding\0", CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "vd", _SLASH_vd, Joined, cl_Group, vtordisp_mode_EQ, nullptr, CLOption | DriverOption | CLOption, 0,
       "Control vtordisp placement", nullptr)
OPTION(prefix_1, "vectorize-loops", vectorize_loops, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Run the Loop vectorization passes", nullptr)
OPTION(prefix_1, "vectorize-slp-aggressive", vectorize_slp_aggressive, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Run the BB vectorization passes", nullptr)
OPTION(prefix_1, "vectorize-slp", vectorize_slp, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Run the SLP vectorization passes", nullptr)
OPTION(prefix_3, "verbose", _verbose, Flag, INVALID, v, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "verify-debug-info", verify_debug_info, Flag, INVALID, INVALID, nullptr, DriverOption, 0,
       "Verify the binary representation of debug output", nullptr)
OPTION(prefix_1, "verify-ignore-unexpected=", verify_ignore_unexpected_EQ, CommaJoined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Ignore unexpected diagnostic messages", nullptr)
OPTION(prefix_1, "verify-ignore-unexpected", verify_ignore_unexpected, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Ignore unexpected diagnostic messages", nullptr)
OPTION(prefix_1, "verify-pch", verify_pch, Flag, Action_Group, INVALID, nullptr, CC1Option, 0,
       "Load and verify that a pre-compiled header file is not stale", nullptr)
OPTION(prefix_1, "verify", verify, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Verify diagnostic output using comment directives", nullptr)
OPTION(prefix_3, "version", _version, Flag, INVALID, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "version", version, Flag, INVALID, INVALID, nullptr, CC1Option | CC1AsOption | NoDriverOption, 0,
       "Print the compiler version", nullptr)
OPTION(prefix_4, "via-file-asm", via_file_asm, Flag, internal_debug_Group, INVALID, nullptr, DriverOption | HelpHidden | CoreOption, 0,
       "Write assembly to file for input to assemble jobs", nullptr)
OPTION(prefix_2, "vmb", _SLASH_vmb, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Use a best-case representation method for member pointers", nullptr)
OPTION(prefix_2, "vmg", _SLASH_vmg, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Use a most-general representation for member pointers", nullptr)
OPTION(prefix_2, "vmm", _SLASH_vmm, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set the default most-general representation to multiple inheritance", nullptr)
OPTION(prefix_2, "vms", _SLASH_vms, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set the default most-general representation to single inheritance", nullptr)
OPTION(prefix_2, "vmv", _SLASH_vmv, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Set the default most-general representation to virtual inheritance", nullptr)
OPTION(prefix_2, "volatile:iso", _SLASH_volatile_iso, Flag, _SLASH_volatile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Volatile loads and stores have standard semantics", nullptr)
OPTION(prefix_2, "volatile:ms", _SLASH_volatile_ms, Flag, _SLASH_volatile_Group, INVALID, nullptr, CLOption | DriverOption, 0,
       "Volatile loads and stores have acquire and release semantics", nullptr)
OPTION(prefix_1, "vtordisp-mode=", vtordisp_mode_EQ, Joined, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Control vtordisp placement on win32 targets", nullptr)
OPTION(prefix_1, "V", V, JoinedOrSeparate, INVALID, INVALID, nullptr, DriverOption | Unsupported, 0, nullptr, nullptr)
OPTION(prefix_2, "V", _SLASH_V, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "v", v, Flag, INVALID, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Show commands to run and use verbose output", nullptr)
OPTION(prefix_2, "W0", _SLASH_W0, Flag, cl_Group, w, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable all warnings", nullptr)
OPTION(prefix_2, "W1", _SLASH_W1, Flag, cl_Group, Wall, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable -Wall", nullptr)
OPTION(prefix_2, "W2", _SLASH_W2, Flag, cl_Group, Wall, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable -Wall", nullptr)
OPTION(prefix_2, "W3", _SLASH_W3, Flag, cl_Group, Wall, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable -Wall", nullptr)
OPTION(prefix_2, "W4", _SLASH_W4, Flag, cl_Group, WCL4, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable -Wall and -Wextra", nullptr)
OPTION(prefix_1, "Wa,", Wa_COMMA, CommaJoined, INVALID, INVALID, nullptr, 0, 0,
       "Pass the comma separated arguments in <arg> to the assembler", "<arg>")
OPTION(prefix_1, "Wall", Wall, Flag, W_Group, INVALID, nullptr, CC1Option | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "Wall", _SLASH_Wall, Flag, cl_Group, WCL4, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable -Wall and -Wextra", nullptr)
OPTION(prefix_3, "warn-=", _warn__EQ, Joined, INVALID, W_Joined, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "warn-", _warn_, Joined, INVALID, W_Joined, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "WCL4", WCL4, Flag, W_Group, INVALID, nullptr, CC1Option | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "wd4005", _SLASH_wd4005, Flag, cl_Group, W_Joined, "no-macro-redefined\0", CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "wd4100", _SLASH_wd4100, Flag, cl_Group, W_Joined, "no-unused-parameter\0", CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "wd4910", _SLASH_wd4910, Flag, cl_Group, W_Joined, "no-dllexport-explicit-instantiation-decl\0", CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "wd4996", _SLASH_wd4996, Flag, cl_Group, W_Joined, "no-deprecated-declarations\0", CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "Wdeprecated", Wdeprecated, Flag, W_Group, INVALID, nullptr, CC1Option, 0,
       "Enable warnings for deprecated constructs and define __DEPRECATED", nullptr)
OPTION(prefix_1, "weak-l", weak_l, Joined, INVALID, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_1, "weak_framework", weak__framework, Separate, INVALID, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_1, "weak_library", weak__library, Separate, INVALID, INVALID, nullptr, LinkerInput, 0, nullptr, nullptr)
OPTION(prefix_1, "weak_reference_mismatches", weak__reference__mismatches, Separate, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Wframe-larger-than=", Wframe_larger_than_EQ, Joined, f_Group, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "whatsloaded", whatsloaded, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "whyload", whyload, Flag, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Wl,", Wl_COMMA, CommaJoined, Link_Group, INVALID, nullptr, LinkerInput | RenderAsInput, 0,
       "Pass the comma separated arguments in <arg> to the linker", "<arg>")
OPTION(prefix_1, "Wlarge-by-value-copy=", Wlarge_by_value_copy_EQ, Joined, INVALID, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "Wlarge-by-value-copy", Wlarge_by_value_copy_def, Flag, INVALID, INVALID, nullptr, HelpHidden, 0,
       "Warn if a function definition returns or accepts an object larger in bytes than a given value", nullptr)
OPTION(prefix_1, "Wlarger-than-", Wlarger_than_, Joined, INVALID, Wlarger_than_EQ, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Wlarger-than=", Wlarger_than_EQ, Joined, clang_ignored_f_Group, INVALID, nullptr, Ignored, 0, nullptr, nullptr)
OPTION(prefix_2, "WL", _SLASH_WL, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "Wno-deprecated", Wno_deprecated, Flag, W_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "Wno-nonportable-cfstrings", Wno_nonportable_cfstrings, Joined, W_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "Wno-rewrite-macros", Wno_rewrite_macros, Flag, INVALID, INVALID, nullptr, CC1Option | NoDriverOption, 0,
       "Silence ObjC rewriting warnings", nullptr)
OPTION(prefix_1, "Wno-write-strings", Wno_write_strings, Flag, W_Group, INVALID, nullptr, CC1Option | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "Wnonportable-cfstrings", Wnonportable_cfstrings, Joined, W_Group, INVALID, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "working-directory=", working_directory_EQ, Joined, INVALID, working_directory, nullptr, CC1Option, 0, nullptr, nullptr)
OPTION(prefix_1, "working-directory", working_directory, JoinedOrSeparate, INVALID, INVALID, nullptr, CC1Option, 0,
       "Resolve file paths relative to the specified directory", nullptr)
OPTION(prefix_1, "Wp,", Wp_COMMA, CommaJoined, Preprocessor_Group, INVALID, nullptr, 0, 0,
       "Pass the comma separated arguments in <arg> to the preprocessor", "<arg>")
OPTION(prefix_2, "Wp64", _SLASH_Wp64, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_3, "write-dependencies", _write_dependencies, Flag, INVALID, MD, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_3, "write-user-dependencies", _write_user_dependencies, Flag, INVALID, MMD, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Wwrite-strings", Wwrite_strings, Flag, W_Group, INVALID, nullptr, CC1Option | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "WX-", _SLASH_WX_, Flag, cl_Group, W_Joined, "no-error\0", CLOption | DriverOption | CLOption, 0,
       "Do not treat warnings as errors", nullptr)
OPTION(prefix_2, "WX", _SLASH_WX, Flag, cl_Group, W_Joined, "error\0", CLOption | DriverOption | CLOption, 0,
       "Treat warnings as errors", nullptr)
OPTION(prefix_1, "W", W_Joined, Joined, W_Group, INVALID, nullptr, CC1Option | CoreOption, 0,
       "Enable the specified warning", "<warning>")
OPTION(prefix_1, "w", w, Flag, INVALID, INVALID, nullptr, CC1Option, 0,
       "Suppress all warnings", nullptr)
OPTION(prefix_2, "w", _SLASH_w_flag, Flag, cl_Group, w, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable all warnings", nullptr)
OPTION(prefix_2, "w", _SLASH_w, Joined, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_1, "Xanalyzer", Xanalyzer, Separate, StaticAnalyzer_Group, INVALID, nullptr, 0, 0,
       "Pass <arg> to the static analyzer", "<arg>")
OPTION(prefix_1, "Xarch_", Xarch__, JoinedAndSeparate, INVALID, INVALID, nullptr, DriverOption, 0, nullptr, nullptr)
OPTION(prefix_1, "Xassembler", Xassembler, Separate, CompileOnly_Group, INVALID, nullptr, 0, 0,
       "Pass <arg> to the assembler", "<arg>")
OPTION(prefix_1, "Xclang", Xclang, Separate, CompileOnly_Group, INVALID, nullptr, DriverOption | CoreOption, 0,
       "Pass <arg> to the clang compiler", "<arg>")
OPTION(prefix_1, "Xcuda-fatbinary", Xcuda_fatbinary, Separate, INVALID, INVALID, nullptr, 0, 0,
       "Pass <arg> to fatbinary invocation", "<arg>")
OPTION(prefix_1, "Xcuda-ptxas", Xcuda_ptxas, Separate, INVALID, INVALID, nullptr, 0, 0,
       "Pass <arg> to the ptxas assembler", "<arg>")
OPTION(prefix_1, "Xlinker", Xlinker, Separate, Link_Group, INVALID, nullptr, LinkerInput | RenderAsInput, 0,
       "Pass <arg> to the linker", "<arg>")
OPTION(prefix_1, "Xpreprocessor", Xpreprocessor, Separate, Preprocessor_Group, INVALID, nullptr, 0, 0,
       "Pass <arg> to the preprocessor", "<arg>")
OPTION(prefix_1, "X", X_Flag, Flag, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "X", X_Joined, Joined, INVALID, INVALID, nullptr, HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "X", _SLASH_X, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "x", x, JoinedOrSeparate, INVALID, INVALID, nullptr, DriverOption | CC1Option, 0,
       "Treat subsequent input files as having type <language>", "<language>")
OPTION(prefix_2, "Y-", _SLASH_Y_, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable precompiled headers, overrides /Yc and /Yu", nullptr)
OPTION(prefix_2, "Yc", _SLASH_Yc, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Generate a pch file for all code up to and including <filename>", "<filename>")
OPTION(prefix_2, "Yd", _SLASH_Yd, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Yl", _SLASH_Yl, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Yu", _SLASH_Yu, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Load a pch file and use it instead of all code up to and including <filename>", "<filename>")
OPTION(prefix_1, "y", y, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Z-reserved-lib-cckext", Z_reserved_lib_cckext, Flag, reserved_lib_Group, INVALID, nullptr, LinkerInput | NoArgumentUnused | Unsupported | Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "Z-reserved-lib-stdc++", Z_reserved_lib_stdcxx, Flag, reserved_lib_Group, INVALID, nullptr, LinkerInput | NoArgumentUnused | Unsupported | Unsupported, 0, nullptr, nullptr)
OPTION(prefix_1, "Z-Xlinker-no-demangle", Z_Xlinker__no_demangle, Flag, INVALID, INVALID, nullptr, Unsupported | NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_2, "Z7", _SLASH_Z7, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable CodeView debug information in object files", nullptr)
OPTION(prefix_2, "Za", _SLASH_Za, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zc:auto", _SLASH_Zc_auto, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zc:forScope", _SLASH_Zc_forScope, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zc:inline", _SLASH_Zc_inline, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zc:rvalueCast", _SLASH_Zc_rvalueCast, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zc:sizedDealloc-", _SLASH_Zc_sizedDealloc_, Flag, cl_Group, fno_sized_deallocation, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable C++14 sized global deallocation functions", nullptr)
OPTION(prefix_2, "Zc:sizedDealloc", _SLASH_Zc_sizedDealloc, Flag, cl_Group, fsized_deallocation, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable C++14 sized global deallocation functions", nullptr)
OPTION(prefix_2, "Zc:strictStrings", _SLASH_Zc_strictStrings, Flag, cl_Group, W_Joined, "error=c++11-compat-deprecated-writable-strings\0", CLOption | DriverOption | CLOption, 0,
       "Treat string literals as const", nullptr)
OPTION(prefix_2, "Zc:threadSafeInit-", _SLASH_Zc_threadSafeInit_, Flag, cl_Group, fno_threadsafe_statics, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable thread-safe initialization of static variables", nullptr)
OPTION(prefix_2, "Zc:threadSafeInit", _SLASH_Zc_threadSafeInit, Flag, cl_Group, fthreadsafe_statics, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable thread-safe initialization of static variables", nullptr)
OPTION(prefix_2, "Zc:trigraphs-", _SLASH_Zc_trigraphs_off, Flag, cl_Group, fno_trigraphs, nullptr, CLOption | DriverOption | CLOption, 0,
       "Disable trigraphs (default)", nullptr)
OPTION(prefix_2, "Zc:trigraphs", _SLASH_Zc_trigraphs, Flag, cl_Group, ftrigraphs, nullptr, CLOption | DriverOption | CLOption, 0,
       "Enable trigraphs", nullptr)
OPTION(prefix_2, "Zc:wchar_t", _SLASH_Zc_wchar_t, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zc:", _SLASH_Zc, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zd", _SLASH_Zd, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Emit debug line number tables only", nullptr)
OPTION(prefix_2, "Ze", _SLASH_Ze, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zg", _SLASH_Zg, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "ZI", _SLASH_ZI, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zi", _SLASH_Zi, Flag, cl_Group, _SLASH_Z7, nullptr, CLOption | DriverOption | CLOption, 0,
       "Alias for /Z7. Does not produce PDBs.", nullptr)
OPTION(prefix_1, "Zlinker-input", Zlinker_input, Separate, INVALID, INVALID, nullptr, Unsupported | NoArgumentUnused, 0, nullptr, nullptr)
OPTION(prefix_2, "Zl", _SLASH_Zl, Flag, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0,
       "Don't mention any default libraries in the object file", nullptr)
OPTION(prefix_2, "Zm", _SLASH_Zm, Joined, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption | HelpHidden, 0, nullptr, nullptr)
OPTION(prefix_2, "Zo-", _SLASH_Zo_, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zo", _SLASH_Zo, Flag, cl_ignored_Group, INVALID, nullptr, CLOption | DriverOption, 0, nullptr, nullptr)
OPTION(prefix_2, "Zp", _SLASH_Zp_flag, Flag, cl_Group, fpack_struct_EQ, "1\0", CLOption | DriverOption | CLOption, 0,
       "Set the default maximum struct packing alignment to 1", nullptr)
OPTION(prefix_2, "Zp", _SLASH_Zp, Joined, cl_Group, fpack_struct_EQ, nullptr, CLOption | DriverOption | CLOption, 0,
       "Specify the default maximum struct packing alignment", nullptr)
OPTION(prefix_2, "Zs", _SLASH_Zs, Flag, cl_Group, fsyntax_only, nullptr, CLOption | DriverOption | CLOption, 0,
       "Syntax-check only", nullptr)
OPTION(prefix_2, "ZW", _SLASH_ZW, Joined, cl_Group, INVALID, nullptr, CLOption | DriverOption | CLOption, 0, nullptr, nullptr)
OPTION(prefix_1, "Z", Z_Flag, Flag, Link_Group, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "Z", Z_Joined, Joined, INVALID, INVALID, nullptr, 0, 0, nullptr, nullptr)
OPTION(prefix_1, "z", z, Separate, Link_Group, INVALID, nullptr, LinkerInput | RenderAsInput, 0,
       "Pass -z <arg> to the linker", "<arg>")
OPTION(prefix_3, "", _DASH_DASH, RemainingArgs, INVALID, INVALID, nullptr, DriverOption | CoreOption, 0, nullptr, nullptr)
OPTION(prefix_3, "", _, Joined, INVALID, INVALID, nullptr, Unsupported, 0, nullptr, nullptr)
#endif // OPTION
