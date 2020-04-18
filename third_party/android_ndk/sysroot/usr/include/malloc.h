/*
 * Copyright (C) 2012 The Android Open Source Project
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
 */

#ifndef LIBC_INCLUDE_MALLOC_H_
#define LIBC_INCLUDE_MALLOC_H_

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdio.h>

__BEGIN_DECLS

// Remove the workaround once b/37423073 is fixed.
#if defined(__clang__) && !__has_attribute(alloc_size)
#define __BIONIC_ALLOC_SIZE(...)
#else
#define __BIONIC_ALLOC_SIZE(...) __attribute__((__alloc_size__(__VA_ARGS__)))
#endif

void* malloc(size_t __byte_count) __mallocfunc __BIONIC_ALLOC_SIZE(1) __wur;
void* calloc(size_t __item_count, size_t __item_size) __mallocfunc __BIONIC_ALLOC_SIZE(1,2) __wur;
void* realloc(void* __ptr, size_t __byte_count) __BIONIC_ALLOC_SIZE(2) __wur;
void free(void* __ptr);

void* memalign(size_t __alignment, size_t __byte_count) __mallocfunc __BIONIC_ALLOC_SIZE(2) __wur;

#if __ANDROID_API__ >= 17
size_t malloc_usable_size(const void* __ptr) __INTRODUCED_IN(17);
#endif /* __ANDROID_API__ >= 17 */


#ifndef STRUCT_MALLINFO_DECLARED
#define STRUCT_MALLINFO_DECLARED 1
struct mallinfo {
  size_t arena;    /* Total number of non-mmapped bytes currently allocated from OS. */
  size_t ordblks;  /* Number of free chunks. */
  size_t smblks;   /* (Unused.) */
  size_t hblks;    /* (Unused.) */
  size_t hblkhd;   /* Total number of bytes in mmapped regions. */
  size_t usmblks;  /* Maximum total allocated space; greater than total if trimming has occurred. */
  size_t fsmblks;  /* (Unused.) */
  size_t uordblks; /* Total allocated space (normal or mmapped.) */
  size_t fordblks; /* Total free space. */
  size_t keepcost; /* Upper bound on number of bytes releasable by malloc_trim. */
};
#endif  /* STRUCT_MALLINFO_DECLARED */

struct mallinfo mallinfo(void);

/*
 * XML structure for malloc_info(3) is in the following format:
 *
 * <malloc version="jemalloc-1">
 *   <heap nr="INT">
 *     <allocated-large>INT</allocated-large>
 *     <allocated-huge>INT</allocated-huge>
 *     <allocated-bins>INT</allocated-bins>
 *     <bins-total>INT</bins-total>
 *     <bin nr="INT">
 *       <allocated>INT</allocated>
 *       <nmalloc>INT</nmalloc>
 *       <ndalloc>INT</ndalloc>
 *     </bin>
 *     <!-- more bins -->
 *   </heap>
 *   <!-- more heaps -->
 * </malloc>
 */

#if __ANDROID_API__ >= 23
int malloc_info(int __must_be_zero, FILE* __fp) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


/* mallopt options */
#define M_DECAY_TIME -100

#if __ANDROID_API__ >= 26
int mallopt(int __option, int __value) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */


__END_DECLS

#endif  /* LIBC_INCLUDE_MALLOC_H_ */
