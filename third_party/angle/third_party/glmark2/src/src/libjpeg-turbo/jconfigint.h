/* jconfigint.h.  Generated from jconfigint.h.in by configure.  */
/* libjpeg-turbo build number */
#define BUILD ""

/* How to obtain function inlining. */
#ifndef INLINE
#if defined(__GNUC__)
#define INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE
#endif
#endif

/* Define to the full name of this package. */
#define PACKAGE_NAME "libjpeg-turbo"

/* Version number of package */
#define VERSION "1.4.90"

/* The size of `size_t', as computed by sizeof. */
#if __WORDSIZE==64 || defined(_WIN64)
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_SIZE_T 4
#endif
