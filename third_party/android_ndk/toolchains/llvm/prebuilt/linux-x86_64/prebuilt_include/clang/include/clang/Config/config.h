/* tools/clang/include/clang/Config/config.h.  Generated from config.h.in by configure.  */
/* include/clang/Config/config.h.in. */

#ifdef CLANG_CONFIG_H
#error config.h can only be included once
#else
#define CLANG_CONFIG_H

#define CLANG_VENDOR "Android "

/* Bug report URL. */
#define BUG_REPORT_URL "http://llvm.org/bugs/"

/* Default linker to use. */
#define CLANG_DEFAULT_LINKER ""

/* Default C++ stdlib to use. */
#define CLANG_DEFAULT_CXX_STDLIB ""

/* Default runtime library to use (\"libgcc\" or \"compiler-rt\", empty for platform default) */
#define CLANG_DEFAULT_RTLIB ""

/* Default OpenMP runtime used by -fopenmp. */
#define CLANG_DEFAULT_OPENMP_RUNTIME "libomp"

/* Multilib suffix for libdir. */
#define CLANG_LIBDIR_SUFFIX "64"

/* Relative directory for resource files */
#define CLANG_RESOURCE_DIR ""

/* Directories clang will search for headers */
#define C_INCLUDE_DIRS ""

/* Default <path> to all compiler invocations for --sysroot=<path>. */
#define DEFAULT_SYSROOT ""

/* Directory where gcc is installed. */
#define GCC_INSTALL_PREFIX ""

/* Define if we have libxml2 */
/* #undef CLANG_HAVE_LIBXML */

#define PACKAGE_STRING "LLVM 5.0.300080"

/* The LLVM product name and version */
#define BACKEND_PACKAGE_STRING PACKAGE_STRING

/* Linker version detected at compile time. */
#define HOST_LINK_VERSION "2.24"

/* enable x86 relax relocations by default */
#define ENABLE_X86_RELAX_RELOCATIONS 0

#endif
