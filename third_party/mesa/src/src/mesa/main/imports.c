/**
 * \file imports.c
 * Standard C library function wrappers.
 * 
 * Imports are services which the device driver or window system or
 * operating system provides to the core renderer.  The core renderer (Mesa)
 * will call these functions in order to do memory allocation, simple I/O,
 * etc.
 *
 * Some drivers will want to override/replace this file with something
 * specialized, but that'll be rare.
 *
 * Eventually, I want to move roll the glheader.h file into this.
 *
 * \todo Functions still needed:
 * - scanf
 * - qsort
 * - rand and RAND_MAX
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#include "imports.h"
#include "context.h"
#include "mtypes.h"
#include "version.h"

#ifdef _GNU_SOURCE
#include <locale.h>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#endif


#ifdef WIN32
#define vsnprintf _vsnprintf
#elif defined(__IBMC__) || defined(__IBMCPP__) || ( defined(__VMS) && __CRTL_VER < 70312000 )
extern int vsnprintf(char *str, size_t count, const char *fmt, va_list arg);
#ifdef __VMS
#include "vsnprintf.c"
#endif
#endif

/**********************************************************************/
/** \name Memory */
/*@{*/

/**
 * Allocate aligned memory.
 *
 * \param bytes number of bytes to allocate.
 * \param alignment alignment (must be greater than zero).
 * 
 * Allocates extra memory to accommodate rounding up the address for
 * alignment and to record the real malloc address.
 *
 * \sa _mesa_align_free().
 */
void *
_mesa_align_malloc(size_t bytes, unsigned long alignment)
{
#if defined(HAVE_POSIX_MEMALIGN)
   void *mem;
   int err = posix_memalign(& mem, alignment, bytes);
   if (err)
      return NULL;
   return mem;
#elif defined(_WIN32) && defined(_MSC_VER)
   return _aligned_malloc(bytes, alignment);
#else
   uintptr_t ptr, buf;

   ASSERT( alignment > 0 );

   ptr = (uintptr_t) malloc(bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (ptr + alignment + sizeof(void *)) & ~(uintptr_t)(alignment - 1);
   *(uintptr_t *)(buf - sizeof(void *)) = ptr;

#ifdef DEBUG
   /* mark the non-aligned area */
   while ( ptr < buf - sizeof(void *) ) {
      *(unsigned long *)ptr = 0xcdcdcdcd;
      ptr += sizeof(unsigned long);
   }
#endif

   return (void *) buf;
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Same as _mesa_align_malloc(), but using calloc(1, ) instead of
 * malloc()
 */
void *
_mesa_align_calloc(size_t bytes, unsigned long alignment)
{
#if defined(HAVE_POSIX_MEMALIGN)
   void *mem;
   
   mem = _mesa_align_malloc(bytes, alignment);
   if (mem != NULL) {
      (void) memset(mem, 0, bytes);
   }

   return mem;
#elif defined(_WIN32) && defined(_MSC_VER)
   void *mem;

   mem = _aligned_malloc(bytes, alignment);
   if (mem != NULL) {
      (void) memset(mem, 0, bytes);
   }

   return mem;
#else
   uintptr_t ptr, buf;

   ASSERT( alignment > 0 );

   ptr = (uintptr_t) calloc(1, bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (ptr + alignment + sizeof(void *)) & ~(uintptr_t)(alignment - 1);
   *(uintptr_t *)(buf - sizeof(void *)) = ptr;

#ifdef DEBUG
   /* mark the non-aligned area */
   while ( ptr < buf - sizeof(void *) ) {
      *(unsigned long *)ptr = 0xcdcdcdcd;
      ptr += sizeof(unsigned long);
   }
#endif

   return (void *)buf;
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Free memory which was allocated with either _mesa_align_malloc()
 * or _mesa_align_calloc().
 * \param ptr pointer to the memory to be freed.
 * The actual address to free is stored in the word immediately before the
 * address the client sees.
 */
void
_mesa_align_free(void *ptr)
{
#if defined(HAVE_POSIX_MEMALIGN)
   free(ptr);
#elif defined(_WIN32) && defined(_MSC_VER)
   _aligned_free(ptr);
#else
   void **cubbyHole = (void **) ((char *) ptr - sizeof(void *));
   void *realAddr = *cubbyHole;
   free(realAddr);
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Reallocate memory, with alignment.
 */
void *
_mesa_align_realloc(void *oldBuffer, size_t oldSize, size_t newSize,
                    unsigned long alignment)
{
#if defined(_WIN32) && defined(_MSC_VER)
   (void) oldSize;
   return _aligned_realloc(oldBuffer, newSize, alignment);
#else
   const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
   void *newBuf = _mesa_align_malloc(newSize, alignment);
   if (newBuf && oldBuffer && copySize > 0) {
      memcpy(newBuf, oldBuffer, copySize);
   }
   if (oldBuffer)
      _mesa_align_free(oldBuffer);
   return newBuf;
#endif
}



/** Reallocate memory */
void *
_mesa_realloc(void *oldBuffer, size_t oldSize, size_t newSize)
{
   const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
   void *newBuffer = malloc(newSize);
   if (newBuffer && oldBuffer && copySize > 0)
      memcpy(newBuffer, oldBuffer, copySize);
   if (oldBuffer)
      free(oldBuffer);
   return newBuffer;
}

/*@}*/


/**********************************************************************/
/** \name Math */
/*@{*/


#ifndef __GNUC__
/**
 * Find the first bit set in a word.
 */
int
ffs(int i)
{
   register int bit = 0;
   if (i != 0) {
      if ((i & 0xffff) == 0) {
         bit += 16;
         i >>= 16;
      }
      if ((i & 0xff) == 0) {
         bit += 8;
         i >>= 8;
      }
      if ((i & 0xf) == 0) {
         bit += 4;
         i >>= 4;
      }
      while ((i & 1) == 0) {
         bit++;
         i >>= 1;
      }
      bit++;
   }
   return bit;
}


/**
 * Find position of first bit set in given value.
 * XXX Warning: this function can only be used on 64-bit systems!
 * \return  position of least-significant bit set, starting at 1, return zero
 *          if no bits set.
 */
int
ffsll(long long int val)
{
   int bit;

   assert(sizeof(val) == 8);

   bit = ffs((int) val);
   if (bit != 0)
      return bit;

   bit = ffs((int) (val >> 32));
   if (bit != 0)
      return 32 + bit;

   return 0;
}
#endif /* __GNUC__ */


#if !defined(__GNUC__) ||\
   ((__GNUC__ * 100 + __GNUC_MINOR__) < 304) /* Not gcc 3.4 or later */
/**
 * Return number of bits set in given GLuint.
 */
unsigned int
_mesa_bitcount(unsigned int n)
{
   unsigned int bits;
   for (bits = 0; n > 0; n = n >> 1) {
      bits += (n & 1);
   }
   return bits;
}

/**
 * Return number of bits set in given 64-bit uint.
 */
unsigned int
_mesa_bitcount_64(uint64_t n)
{
   unsigned int bits;
   for (bits = 0; n > 0; n = n >> 1) {
      bits += (n & 1);
   }
   return bits;
}
#endif


/**
 * Convert a 4-byte float to a 2-byte half float.
 * Based on code from:
 * http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/008786.html
 */
GLhalfARB
_mesa_float_to_half(float val)
{
   const fi_type fi = {val};
   const int flt_m = fi.i & 0x7fffff;
   const int flt_e = (fi.i >> 23) & 0xff;
   const int flt_s = (fi.i >> 31) & 0x1;
   int s, e, m = 0;
   GLhalfARB result;
   
   /* sign bit */
   s = flt_s;

   /* handle special cases */
   if ((flt_e == 0) && (flt_m == 0)) {
      /* zero */
      /* m = 0; - already set */
      e = 0;
   }
   else if ((flt_e == 0) && (flt_m != 0)) {
      /* denorm -- denorm float maps to 0 half */
      /* m = 0; - already set */
      e = 0;
   }
   else if ((flt_e == 0xff) && (flt_m == 0)) {
      /* infinity */
      /* m = 0; - already set */
      e = 31;
   }
   else if ((flt_e == 0xff) && (flt_m != 0)) {
      /* NaN */
      m = 1;
      e = 31;
   }
   else {
      /* regular number */
      const int new_exp = flt_e - 127;
      if (new_exp < -24) {
         /* this maps to 0 */
         /* m = 0; - already set */
         e = 0;
      }
      else if (new_exp < -14) {
         /* this maps to a denorm */
         unsigned int exp_val = (unsigned int) (-14 - new_exp); /* 2^-exp_val*/
         e = 0;
         switch (exp_val) {
            case 0:
               _mesa_warning(NULL,
                   "float_to_half: logical error in denorm creation!\n");
               /* m = 0; - already set */
               break;
            case 1: m = 512 + (flt_m >> 14); break;
            case 2: m = 256 + (flt_m >> 15); break;
            case 3: m = 128 + (flt_m >> 16); break;
            case 4: m = 64 + (flt_m >> 17); break;
            case 5: m = 32 + (flt_m >> 18); break;
            case 6: m = 16 + (flt_m >> 19); break;
            case 7: m = 8 + (flt_m >> 20); break;
            case 8: m = 4 + (flt_m >> 21); break;
            case 9: m = 2 + (flt_m >> 22); break;
            case 10: m = 1; break;
         }
      }
      else if (new_exp > 15) {
         /* map this value to infinity */
         /* m = 0; - already set */
         e = 31;
      }
      else {
         /* regular */
         e = new_exp + 15;
         m = flt_m >> 13;
      }
   }

   result = (s << 15) | (e << 10) | m;
   return result;
}


/**
 * Convert a 2-byte half float to a 4-byte float.
 * Based on code from:
 * http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/008786.html
 */
float
_mesa_half_to_float(GLhalfARB val)
{
   /* XXX could also use a 64K-entry lookup table */
   const int m = val & 0x3ff;
   const int e = (val >> 10) & 0x1f;
   const int s = (val >> 15) & 0x1;
   int flt_m, flt_e, flt_s;
   fi_type fi;
   float result;

   /* sign bit */
   flt_s = s;

   /* handle special cases */
   if ((e == 0) && (m == 0)) {
      /* zero */
      flt_m = 0;
      flt_e = 0;
   }
   else if ((e == 0) && (m != 0)) {
      /* denorm -- denorm half will fit in non-denorm single */
      const float half_denorm = 1.0f / 16384.0f; /* 2^-14 */
      float mantissa = ((float) (m)) / 1024.0f;
      float sign = s ? -1.0f : 1.0f;
      return sign * mantissa * half_denorm;
   }
   else if ((e == 31) && (m == 0)) {
      /* infinity */
      flt_e = 0xff;
      flt_m = 0;
   }
   else if ((e == 31) && (m != 0)) {
      /* NaN */
      flt_e = 0xff;
      flt_m = 1;
   }
   else {
      /* regular */
      flt_e = e + 112;
      flt_m = m << 13;
   }

   fi.i = (flt_s << 31) | (flt_e << 23) | flt_m;
   result = fi.f;
   return result;
}

/*@}*/


/**********************************************************************/
/** \name Sort & Search */
/*@{*/

/**
 * Wrapper for bsearch().
 */
void *
_mesa_bsearch( const void *key, const void *base, size_t nmemb, size_t size, 
               int (*compar)(const void *, const void *) )
{
#if defined(_WIN32_WCE)
   void *mid;
   int cmp;
   while (nmemb) {
      nmemb >>= 1;
      mid = (char *)base + nmemb * size;
      cmp = (*compar)(key, mid);
      if (cmp == 0)
	 return mid;
      if (cmp > 0) {
	 base = (char *)mid + size;
	 --nmemb;
      }
   }
   return NULL;
#else
   return bsearch(key, base, nmemb, size, compar);
#endif
}

/*@}*/


/**********************************************************************/
/** \name Environment vars */
/*@{*/

/**
 * Wrapper for getenv().
 */
char *
_mesa_getenv( const char *var )
{
#if defined(_XBOX) || defined(_WIN32_WCE)
   return NULL;
#else
   return getenv(var);
#endif
}

/*@}*/


/**********************************************************************/
/** \name String */
/*@{*/

/**
 * Implemented using malloc() and strcpy.
 * Note that NULL is handled accordingly.
 */
char *
_mesa_strdup( const char *s )
{
   if (s) {
      size_t l = strlen(s);
      char *s2 = (char *) malloc(l + 1);
      if (s2)
         strcpy(s2, s);
      return s2;
   }
   else {
      return NULL;
   }
}

/** Wrapper around strtof() */
float
_mesa_strtof( const char *s, char **end )
{
#if defined(_GNU_SOURCE) && !defined(__CYGWIN__) && !defined(__FreeBSD__) && \
   !defined(ANDROID) && !defined(__HAIKU__) && !defined(__UCLIBC__)
   static locale_t loc = NULL;
   if (!loc) {
      loc = newlocale(LC_CTYPE_MASK, "C", NULL);
   }
   return strtof_l(s, end, loc);
#elif defined(_ISOC99_SOURCE) || (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 600)
   return strtof(s, end);
#else
   return (float)strtod(s, end);
#endif
}

/** Compute simple checksum/hash for a string */
unsigned int
_mesa_str_checksum(const char *str)
{
   /* This could probably be much better */
   unsigned int sum, i;
   const char *c;
   sum = i = 1;
   for (c = str; *c; c++, i++)
      sum += *c * (i % 100);
   return sum + i;
}


/*@}*/


/** Needed due to #ifdef's, above. */
int
_mesa_vsnprintf(char *str, size_t size, const char *fmt, va_list args)
{
   return vsnprintf( str, size, fmt, args);
}

/** Wrapper around vsnprintf() */
int
_mesa_snprintf( char *str, size_t size, const char *fmt, ... )
{
   int r;
   va_list args;
   va_start( args, fmt );  
   r = vsnprintf( str, size, fmt, args );
   va_end( args );
   return r;
}


