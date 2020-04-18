/* Header file for libgcov-*.c.
   Copyright (C) 1996-2014 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_LIBGCOV_KERNEL_H
#define GCC_LIBGCOV_KERNEL_H

/* work around the poisoned malloc/calloc in system.h.  */
#ifndef xmalloc
#define xmalloc vmalloc
#endif
#ifndef xcalloc
#define xcalloc vcalloc
#endif
#ifndef xrealloc
#define xrealloc vrealloc
#endif
#ifndef xfree
#define xfree vfree
#endif
#ifndef alloca
#define alloca __builtin_alloca
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

 /* Define MACROs to be used by kernel compilation.  */
# define L_gcov
# define L_gcov_interval_profiler
# define L_gcov_pow2_profiler
# define L_gcov_one_value_profiler
# define L_gcov_indirect_call_profiler_v2
# define L_gcov_direct_call_profiler
# define L_gcov_indirect_call_profiler
# define L_gcov_indirect_call_topn_profiler
# define L_gcov_time_profiler
# define L_gcov_average_profiler
# define L_gcov_ior_profiler
# define L_gcov_merge_add
# define L_gcov_merge_single
# define L_gcov_merge_delta
# define L_gcov_merge_ior
# define L_gcov_merge_time_profile
# define L_gcov_merge_icall_topn
# define L_gcov_merge_dc

# define IN_LIBGCOV 1
# define IN_GCOV 0
#define THREAD_PREFIX
#define GCOV_LINKAGE /* nothing */
#define BITS_PER_UNIT 8
#define LONG_LONG_TYPE_SIZE 64
#define MEMMODEL_RELAXED 0

#define ENABLE_ASSERT_CHECKING 1

/* gcc_assert() prints out a warning if the check fails. It
   will not abort.  */
#if ENABLE_ASSERT_CHECKING
# define gcc_assert(EXPR) \
    ((void)(!(EXPR) ? printk (KERN_WARNING \
      "GCOV assertion fails: func=%s line=%d\n", \
      __FUNCTION__, __LINE__), 0 : 0))
#else
# define gcc_assert(EXPR) ((void)(0 && (EXPR)))
#endif

/* In Linux kernel mode, a virtual file is used for file operations.  */
struct gcov_info;
typedef struct {
  long size; /* size of buf */
  long count; /* element written into buf */
  struct gcov_info *info;
  char *buf;
} gcov_kernel_vfile;

#define _GCOV_FILE gcov_kernel_vfile

/* Wrappers to the file operations.  */
#define _GCOV_fclose     kernel_file_fclose
#define _GCOV_ftell      kernel_file_ftell
#define _GCOV_fseek      kernel_file_fseek
#define _GCOV_ftruncate  kernel_file_ftruncate
#define _GCOV_fread      kernel_file_fread
#define _GCOV_fwrite     kernel_file_fwrite
#define _GCOV_fileno     kernel_file_fileno

/* Declarations for virtual files operations.  */
extern int kernel_file_fclose (gcov_kernel_vfile *);
extern long kernel_file_ftell (gcov_kernel_vfile *);
extern int kernel_file_fseek (gcov_kernel_vfile *, long, int);
extern int kernel_file_ftruncate (gcov_kernel_vfile *, off_t);
extern int kernel_file_fread (void *, size_t, size_t,
    gcov_kernel_vfile *);
extern int kernel_file_fwrite (const void *, size_t, size_t,
    gcov_kernel_vfile *);
extern int kernel_file_fileno (gcov_kernel_vfile *);

#endif /* GCC_LIBGCOV_KERNEL_H */
