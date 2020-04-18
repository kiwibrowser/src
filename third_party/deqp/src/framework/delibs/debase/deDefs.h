#ifndef _DEDEFS_H
#define _DEDEFS_H
/*-------------------------------------------------------------------------
 * drawElements Base Portability Library
 * -------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Basic portability.
 *//*--------------------------------------------------------------------*/

/* Compilers. */
#define DE_COMPILER_VANILLA	0		/*!< Vanilla compiler. Used for disabling all platform-specific optimizations.	*/
#define DE_COMPILER_MSC		1		/*!< Microsoft Visual Studio.													*/
#define DE_COMPILER_GCC     2		/*!< Gnu C Compiler.															*/
#define DE_COMPILER_CLANG   3		/*!< LLVM Clang Compiler.														*/

/* Compiler detection. */
#if defined(_MSC_VER)
#	define DE_DETAIL_DETECTED_COMPILER DE_COMPILER_MSC
#elif defined(__clang__)
#	define DE_DETAIL_DETECTED_COMPILER DE_COMPILER_CLANG
#elif defined(__GNUC__)
#   define DE_DETAIL_DETECTED_COMPILER DE_COMPILER_GCC
#else
	/* DE_DETAIL_DETECTED_COMPILER not set */
#endif

/* Compiler setting. */
#if defined(DE_COMPILER)
	/* Allow definitions from outside, but fail early if it conflicts with our detection */
#	if defined(DE_DETAIL_DETECTED_COMPILER) && (DE_COMPILER != DE_DETAIL_DETECTED_COMPILER)
		/* conflict, print a nice error messages for the most common misconfigs,
		 * GCC and Clang, and a generic for other conflicts.
		 */
#		if (DE_DETAIL_DETECTED_COMPILER == DE_COMPILER_CLANG) && (DE_COMPILER == DE_COMPILER_GCC)
#			error Detected compiler is Clang, but got DE_COMPILER == DE_COMPILER_GCC
#		elif (DE_DETAIL_DETECTED_COMPILER == DE_COMPILER_GCC) && (DE_COMPILER == DE_COMPILER_CLANG)
#			error Detected compiler is GCC, but got DE_COMPILER == DE_COMPILER_CLANG
#		else
#			error Detected compiler does not match the supplied compiler.
#		endif
#	endif
	/* Clear autodetect vars. */
#	if defined(DE_DETAIL_DETECTED_COMPILER)
#		undef DE_DETAIL_DETECTED_COMPILER
#	endif
#else
	/* No definition given from outside, try to autodetect */
#	if defined(DE_DETAIL_DETECTED_COMPILER)
#		define DE_COMPILER DE_DETAIL_DETECTED_COMPILER /*!< Compiler identification (set to one of DE_COMPILER_*). */
#	else
#		error Unknown compiler.
#	endif
#endif

/* Operating systems. */
#define DE_OS_VANILLA	0			/*!< Vanilla OS.								*/
#define DE_OS_WIN32		1			/*!< Microsoft Windows desktop					*/
#define DE_OS_UNIX      2			/*!< Unix (or compatible)						*/
#define DE_OS_WINCE		3			/*!< Windows CE, Windows Mobile or Pocket PC	*/
#define DE_OS_OSX		4			/*!< Mac OS X									*/
#define DE_OS_ANDROID	5			/*!< Android									*/
#define DE_OS_SYMBIAN	6			/*!< Symbian OS									*/
#define DE_OS_IOS		7			/*!< iOS										*/
#define DE_OS_QNX       8           /*!< QNX                                        */

/* OS detection (set to one of DE_OS_*). */
#if defined(DE_OS)
	/* Allow definitions from outside. */
#elif defined(__ANDROID__)
#	define DE_OS DE_OS_ANDROID
#elif defined(_WIN32_WCE) || defined(UNDER_CE)
#	define DE_OS DE_OS_WINCE
#elif defined(_WIN32)
#	define DE_OS DE_OS_WIN32
#elif defined(__unix__) || defined(__linux) || defined(__linux__)
#   define DE_OS DE_OS_UNIX
#elif defined(__APPLE__)
#	define DE_OS DE_OS_OSX
#elif defined(__EPOC32__)
#	define DE_OS DE_OS_SYMBIAN
#elif defined(__QNX__)
#   define DE_OS DE_OS_QNX
#else
#	error Unknown operating system.
#endif

/* CPUs */
#define DE_CPU_VANILLA	0
#define DE_CPU_X86		1
#define DE_CPU_ARM		2
#define DE_CPU_X86_64	3
#define DE_CPU_ARM_64	4
#define DE_CPU_MIPS		5
#define DE_CPU_MIPS_64	6

/* CPU detection. */
#if defined(DE_CPU)
	/* Allow definitions from outside. */
#elif defined(__aarch64__)
#	define DE_CPU DE_CPU_ARM_64
#elif defined(__arm__) || defined(__ARM__) || defined(__ARM_NEON__) || defined(ARM_BUILD)
#	define DE_CPU DE_CPU_ARM
#elif defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
#	define DE_CPU DE_CPU_X86_64
#elif defined(__i386__) || defined(_M_X86) || defined(_M_IX86) || defined(X86_BUILD)
#	define DE_CPU DE_CPU_X86
#elif defined(__mips__) && ((__mips) == 32)
#	define DE_CPU DE_CPU_MIPS
#elif defined(__mips__) && ((__mips) == 64)
#	define DE_CPU DE_CPU_MIPS_64
#else
#	error Unknown CPU.
#endif

/* Endianness */
#define DE_BIG_ENDIAN		0
#define DE_LITTLE_ENDIAN	1

#if defined(DE_ENDIANNESS)
	/* Allow definitions from outside. */
#elif (DE_CPU == DE_CPU_X86) || (DE_CPU == DE_CPU_X86_64)
	/* "detect" x86(_64) endianness */
#	define DE_ENDIANNESS DE_LITTLE_ENDIAN
#elif ((DE_CPU == DE_CPU_MIPS) || (DE_CPU == DE_CPU_MIPS_64))
	/* detect mips endianness using platform specific macros */
#	if defined(__MIPSEB__) && !defined(__MIPSEL__)
#		define DE_ENDIANNESS DE_BIG_ENDIAN
#	elif !defined(__MIPSEB__) && defined(__MIPSEL__)
#		define DE_ENDIANNESS DE_LITTLE_ENDIAN
#	else
#		error Invalid MIPS endianness.
#	endif
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#	define DE_ENDIANNESS DE_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#	define DE_ENDIANNESS DE_BIG_ENDIAN
#else
#	error Unknown endianness.
#endif

/* Sanity */
#if ((DE_CPU == DE_CPU_X86) || (DE_CPU == DE_CPU_X86_64)) && (DE_ENDIANNESS == DE_BIG_ENDIAN)
#	error Invalid x86(_64) endianness.
#endif

/* Sized data types. */
typedef signed char			deInt8;
typedef signed short		deInt16;
typedef signed int			deInt32;
typedef unsigned char		deUint8;
typedef unsigned short		deUint16;
typedef unsigned int		deUint32;

#if (DE_COMPILER == DE_COMPILER_MSC)
	typedef signed __int64		deInt64;
	typedef unsigned __int64	deUint64;

#	if (DE_OS == DE_OS_WINCE)
#		include <basetsd.h>
		typedef INT_PTR			deIntptr;
		typedef UINT_PTR		deUintptr;
#	elif (DE_OS == DE_OS_WIN32)
#		include <crtdefs.h>
		typedef intptr_t		deIntptr;
		typedef uintptr_t		deUintptr;
#	else
#		error Define intptr types.
#	endif

#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	/* \note stddef.h is needed for size_t definition. */
#	include <stddef.h>
#	include <stdint.h>
	typedef int64_t				deInt64;
	typedef uint64_t			deUint64;
	typedef intptr_t			deIntptr;
	typedef uintptr_t			deUintptr;
#else
#	error Define 64-bit and intptr types.
#endif

/** Boolean type. */
typedef int deBool;
#define DE_TRUE		1		/*!< True value for deBool.		*/
#define DE_FALSE	0		/*!< False value for deBool.	*/

/* Null pointer. */
#if defined(__cplusplus)
#	define DE_NULL 0
#else
#	define DE_NULL ((void*)0)		/*!< Null pointer.				*/
#endif

/* Function pointer type. */
typedef void (*deFunctionPtr) (void);

/* Use DE_PTR_TYPE(T) instead of T* in macros to avoid clang-tidy warning. */
#define DE_PTR_TYPE(T) T*  /* NOLINT(T) */

/* Debug macro. */
#if defined(DE_DEBUG)
	/* Already defined from outside. */
#else
#	if (DE_COMPILER != DE_COMPILER_GCC)
#		if defined(_DEBUG)
#			define DE_DEBUG					/*!< Debug build enabled? Usage: #if defined(DE_DEBUG).	*/
#		endif
#	elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
#		if !defined(NDEBUG)
#			define DE_DEBUG
#		endif
#	endif
#endif

/* Debug code macro. */
#if defined(DE_DEBUG)
#	define DE_DEBUG_CODE(X) X
#else
#	define DE_DEBUG_CODE(X)
#endif

/* Inline. */
#if (DE_COMPILER == DE_COMPILER_MSC)
#	define DE_INLINE __forceinline
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
#   define DE_INLINE static __inline__
#else
#	define DE_INLINE inline			/*!< Function inline.		*/
#endif

/* DE_DEV_BUILD -- only define when building on a development machine. */
#if !defined(DE_DEV_BUILD)
#	if (DE_COMPILER == DE_COMPILER_MSC)
#		define DE_DEV_BUILD
#	endif
#endif

/* DE_VALGRIND_BUILD -- define this in makefile if support for Valgrind is wanted. */
/*#define DE_VALGRIND_BUILD*/

/** Length of array. C++ version does compile time check that passed value is an array reference. */
#if defined(__cplusplus) && (DE_COMPILER == DE_COMPILER_MSC)
	template <typename T, size_t N> char (&deArraySizeHelper(T (&array)[N]))[N];
#	define DE_LENGTH_OF_ARRAY(ARRAY) ((int)(sizeof(deArraySizeHelper(ARRAY))))
#else
#	define DE_LENGTH_OF_ARRAY(ARRAY) ((int)(sizeof(ARRAY) / sizeof((ARRAY)[0])))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Assertion macro family. */
void deAssertFail(const char* reason, const char* file, int line);

/* Assertion failure callback. Requires DE_ASSERT_FAILURE_CALLBACK to be defined or otherwise has no effect. */
typedef void (*deAssertFailureCallbackFunc) (const char* reason, const char* file, int line);
void deSetAssertFailureCallback (deAssertFailureCallbackFunc callback);

DE_INLINE deBool deGetFalse (void) { return DE_FALSE; }
DE_INLINE deBool deGetTrue (void) { return DE_TRUE; }

/* Assertion macro. */
#if defined(DE_DEBUG) && !defined(DE_COVERAGE_BUILD)
#	define DE_ASSERT(X) do { if ((!deGetFalse() && (X)) ? DE_FALSE : DE_TRUE) deAssertFail(#X, __FILE__, __LINE__); } while(deGetFalse())
#else
#	define DE_ASSERT(X) /*@ -noeffect*/ ((void)0)	/*!< Assertion macro. */
#endif

/* Verify macro. Behaves like assert in debug build, but executes statement in release build. */
#if defined(DE_DEBUG)
#	define DE_VERIFY(X) do { if ((!deGetFalse() && (X)) ? DE_FALSE : DE_TRUE) deAssertFail(#X, __FILE__, __LINE__); } while(deGetFalse())
#else
#	define DE_VERIFY(X) X
#endif

/* Fatal macro. */
#if defined(DE_DEBUG) && !defined(DE_COVERAGE_BUILD)
#	define DE_FATAL(MSG) do { deAssertFail("" /* force to string literal */ MSG, __FILE__, __LINE__); } while(deGetFalse())
#else
#	define DE_FATAL(MSG) /*@ -noeffect*/ ((void)0)	/*!< Fatal macro. */
#endif

/** Test assert macro for use in testers (same as DE_ASSERT, but always enabled). */
#define DE_TEST_ASSERT(X) do { if ((!deGetFalse() && (X)) ? DE_FALSE : DE_TRUE) deAssertFail(#X, __FILE__, __LINE__); } while(deGetFalse())

#if (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	/* GCC 4.8 and newer warns about unused typedefs. */
#	define DE_UNUSED_TYPEDEF_ATTR __attribute__((unused))
#else
#	define DE_UNUSED_TYPEDEF_ATTR
#endif

/** Compile-time assertion macro. */
#define DE_STATIC_ASSERT(X)						typedef char DE_UNIQUE_NAME[(X) ? 1 : -1] DE_UNUSED_TYPEDEF_ATTR
#define DE_HEADER_STATIC_ASSERT(HEADERTOKEN, X)	typedef char DE_HEADER_UNIQUE_NAME(HEADERTOKEN)[(X) ? 1 : -1] DE_UNUSED_TYPEDEF_ATTR

#define DE_UNIQUE_NAME						DE_MAKE_NAME(__LINE__, hoax)
#define DE_HEADER_UNIQUE_NAME(HEADERTOKEN)	DE_MAKE_NAME(__LINE__, HEADERTOKEN)
#define DE_MAKE_NAME(line, token) DE_MAKE_NAME2(line, token)
#define DE_MAKE_NAME2(line, token) _static_assert_##line##_##token

/** Software breakpoint. */
#if (DE_CPU == DE_CPU_X86) && (DE_COMPILER == DE_COMPILER_MSC)
#	define DE_BREAKPOINT() do { printf("Software breakpoint encountered in %s, line %d\n", __FILE__, __LINE__); __asm { int 3 } } while (deGetFalse())
#elif (DE_CPU == DE_CPU_X86_64) && (DE_COMPILER == DE_COMPILER_MSC)
#	define DE_BREAKPOINT() do { printf("Software breakpoint encountered in %s, line %d\n", __FILE__, __LINE__); __debugbreak(); } while (deGetFalse())
#elif (DE_CPU == DE_CPU_ARM) && (DE_COMPILER == DE_COMPILER_GCC)
#	define DE_BREAKPOINT() do { printf("Software breakpoint encountered in %s, line %d\n", __FILE__, __LINE__); __asm__ __volatile__ ( "bkpt #3" ); } while (deGetFalse())
#elif (DE_CPU == DE_CPU_ARM_64) && (DE_COMPILER == DE_COMPILER_GCC)
#	define DE_BREAKPOINT() do { printf("Software breakpoint encountered in %s, line %d\n", __FILE__, __LINE__); __asm__ __volatile__ ( "brk #3" ); } while (deGetFalse())
#elif ((DE_CPU == DE_CPU_ARM) || (DE_CPU == DE_CPU_ARM_64)) && (DE_COMPILER == DE_COMPILER_MSC)
#	define DE_BREAKPOINT() do { printf("Software breakpoint encountered in %s, line %d\n", __FILE__, __LINE__); DebugBreak(); } while (deGetFalse())
#else
#	define DE_BREAKPOINT() DE_FATAL("Software breakpoint encountered!")
#endif

/** Swap two values. */
#define DE_SWAP(TYPE, A, B) do { TYPE _tmp_ = (A); (A) = (B); (B) = _tmp_; } while(deGetFalse())

/** Offset of a struct member. */
#define DE_OFFSET_OF(STRUCT, MEMBER) ((deUint32)(deUintptr)(deUint8*)&(((STRUCT*)0)->MEMBER))

/* Pointer size. */
#if defined(DE_PTR_SIZE)
	/* nada */
#elif defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) || defined(__aarch64__) || (defined(__mips) && ((__mips) == 64)) || defined(_LP64) || defined(__LP64__)
#	define DE_PTR_SIZE 8
#else
#	define DE_PTR_SIZE 4	/* default to 32-bit */
#endif

/** Unreferenced variable silencing. */
#define DE_UNREF(VAR) ((void)(VAR))

/** DE_BEGIN_EXTERN_C and DE_END_EXTERN_C. */
#if defined(__cplusplus)
#	define DE_BEGIN_EXTERN_C extern "C" {
#	define DE_END_EXTERN_C }
#else
#	define DE_BEGIN_EXTERN_C
#	define DE_END_EXTERN_C
#endif

/** DE_NULL_STATEMENT */
#if defined(DE_DEBUG)
#	define DE_NULL_STATEMENT do {} while (deGetFalse())
#else
#	define DE_NULL_STATEMENT (void)0
#endif

/** GCC format string attributes */
#if (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
#	define DE_PRINTF_FUNC_ATTR(FORMAT_STRING, FIRST_ARG) __attribute__ ((format(printf, FORMAT_STRING, FIRST_ARG)))
#else
#	define DE_PRINTF_FUNC_ATTR(FORMAT_STRING, FIRST_ARG)
#endif

/** Potentially unused func attribute to silence warnings from C templates. */
#if (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
#	define DE_UNUSED_FUNCTION __attribute__((unused))
#else
#	define DE_UNUSED_FUNCTION
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DEDEFS_H */
