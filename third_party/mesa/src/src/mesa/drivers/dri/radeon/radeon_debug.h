/*
 * Copyright © 2009 Pauli Nieminen
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Pauli Nieminen <suokkos@gmail.com>
 */

#ifndef RADEON_DEBUG_H_INCLUDED
#define RADEON_DEBUG_H_INCLUDED

#include <stdlib.h>

typedef enum radeon_debug_levels {
	RADEON_CRITICAL  = 0, /* Only errors */
	RADEON_IMPORTANT = 1, /* Important warnings and messages */
	RADEON_NORMAL    = 2, /* Normal log messages usefull for debugging */
	RADEON_VERBOSE   = 3, /* Extra details to debugging */
	RADEON_TRACE     = 4  /* Log about everything that happens */
} radeon_debug_level_t;

/**
 * Compile time option to change level of debugging compiled to dri driver.
 * Selecting critical level is not recommended because perfromance gains are
 * going to minimal but you will lose a lot of important warnings in case of
 * errors.
 */
#ifndef RADEON_DEBUG_LEVEL
# ifdef DEBUG
#  define RADEON_DEBUG_LEVEL RADEON_TRACE
# else
#  define RADEON_DEBUG_LEVEL RADEON_VERBOSE
# endif
#endif

typedef enum radeon_debug_types {
	RADEON_TEXTURE   = 0x00001,
	RADEON_STATE     = 0x00002,
	RADEON_IOCTL     = 0x00004,
	RADEON_RENDER    = 0x00008,
	RADEON_SWRENDER  = 0x00010,
	RADEON_FALLBACKS = 0x00020,
	RADEON_VFMT      = 0x00040,
	RADEON_SHADER    = 0x00080,
	RADEON_CS        = 0x00100,
	RADEON_DRI       = 0x00200,
	RADEON_DMA       = 0x00400,
	RADEON_SANITY    = 0x00800,
	RADEON_SYNC      = 0x01000,
	RADEON_PIXEL     = 0x02000,
	RADEON_MEMORY    = 0x04000,
	RADEON_VERTS     = 0x08000,
	RADEON_GENERAL   = 0x10000   /* Used for errors and warnings */
} radeon_debug_type_t;

#define RADEON_MAX_INDENT 5

struct radeon_debug {
       size_t indent_depth;
       char indent[RADEON_MAX_INDENT];
};

extern radeon_debug_type_t radeon_enabled_debug_types;

/**
 * Compabibility layer for old debug code
 **/
#define RADEON_DEBUG radeon_enabled_debug_types

static inline int radeon_is_debug_enabled(const radeon_debug_type_t type,
	   const radeon_debug_level_t level)
{
       return RADEON_DEBUG_LEVEL >= level
		&& (type & radeon_enabled_debug_types);
}
/*
 * define macro for gcc specific __attribute__ if using alternative compiler
 */
#ifndef __GNUC__
#define  __attribute__(x)  /*empty*/
#endif


extern void _radeon_print(const radeon_debug_type_t type,
	   const radeon_debug_level_t level,
	   const char* message,
	   ...)  __attribute__((format(printf,3,4)));
/**
 * Print out debug message if channel specified by type is enabled
 * and compile time debugging level is at least as high as level parameter
 */
#define radeon_print(type, level, ...) do {			\
	const radeon_debug_level_t _debug_level = (level);	\
	const radeon_debug_type_t _debug_type = (type);		\
	/* Compile out if level of message is too high */	\
	if (radeon_is_debug_enabled(type, level)) {		\
		_radeon_print(_debug_type, _debug_level,	\
			__VA_ARGS__);				\
	}							\
} while(0)

/**
 * printf style function for writing error messages.
 */
#define radeon_error(...) do {					\
	radeon_print(RADEON_GENERAL, RADEON_CRITICAL,		\
		__VA_ARGS__);					\
} while(0)

/**
 * printf style function for writing warnings.
 */
#define radeon_warning(...) do {				\
	radeon_print(RADEON_GENERAL, RADEON_IMPORTANT,		\
		__VA_ARGS__);					\
} while(0)

extern void radeon_init_debug(void);
extern void _radeon_debug_add_indent(void);
extern void _radeon_debug_remove_indent(void);

static inline void radeon_debug_add_indent(void)
{
       if (RADEON_DEBUG_LEVEL >= RADEON_VERBOSE) {
	      _radeon_debug_add_indent();
       }
}
static inline void radeon_debug_remove_indent(void)
{
       if (RADEON_DEBUG_LEVEL >= RADEON_VERBOSE) {
	      _radeon_debug_remove_indent();
       }
}


/* From http://gcc. gnu.org/onlinedocs/gcc-3.2.3/gcc/Variadic-Macros.html .
   I suppose we could inline this and use macro to fetch out __LINE__ and stuff in case we run into trouble
   with other compilers ... GLUE!
*/
#define WARN_ONCE(...)      do { \
       static int __warn_once=1; \
       if(__warn_once){ \
               radeon_warning("*********************************WARN_ONCE*********************************\n"); \
               radeon_warning("File %s function %s line %d\n", \
                       __FILE__, __FUNCTION__, __LINE__); \
               radeon_warning(__VA_ARGS__);\
               radeon_warning("***************************************************************************\n"); \
               __warn_once=0;\
               } \
       } while(0)


#endif
