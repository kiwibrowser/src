/*-
 * Written by J.T. Conklin <jtc@netbsd.org>
 * Public domain.
 *
 *	$NetBSD: search.h,v 1.12 1999/02/22 10:34:28 christos Exp $
 * $FreeBSD: release/9.0.0/include/search.h 105250 2002-10-16 14:29:23Z robert $
 */

#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <sys/cdefs.h>
#include <sys/types.h>

typedef enum {
  FIND,
  ENTER
} ACTION;

typedef struct entry {
  char* key;
  void* data;
} ENTRY;

typedef enum {
  preorder,
  postorder,
  endorder,
  leaf
} VISIT;

#if defined(__USE_BSD) || defined(__USE_GNU)
struct hsearch_data {
  struct __hsearch* __hsearch;
};
#endif

__BEGIN_DECLS


#if __ANDROID_API__ >= 21
void insque(void* __element, void* __previous) __INTRODUCED_IN(21);
void remque(void* __element) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */



#if __ANDROID_API__ >= __ANDROID_API_FUTURE__
int hcreate(size_t) __INTRODUCED_IN_FUTURE;
void hdestroy(void) __INTRODUCED_IN_FUTURE;
ENTRY* hsearch(ENTRY, ACTION) __INTRODUCED_IN_FUTURE;
#endif /* __ANDROID_API__ >= __ANDROID_API_FUTURE__ */


#if defined(__USE_BSD) || defined(__USE_GNU)

#if __ANDROID_API__ >= __ANDROID_API_FUTURE__
int hcreate_r(size_t, struct hsearch_data*) __INTRODUCED_IN_FUTURE;
void hdestroy_r(struct hsearch_data*) __INTRODUCED_IN_FUTURE;
int hsearch_r(ENTRY, ACTION, ENTRY**, struct hsearch_data*) __INTRODUCED_IN_FUTURE;
#endif /* __ANDROID_API__ >= __ANDROID_API_FUTURE__ */

#endif


#if __ANDROID_API__ >= 21
void* lfind(const void* __key, const void* __base, size_t* __count, size_t __size, int (*__comparator)(const void*, const void*))
  __INTRODUCED_IN(21);
void* lsearch(const void* __key, void* __base, size_t* __count, size_t __size, int (*__comparator)(const void*, const void*))
  __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */



#if __ANDROID_API__ >= 16
void* tdelete(const void* __key, void** __root_ptr, int (*__comparator)(const void*, const void*)) __INTRODUCED_IN(16);
void tdestroy(void* __root, void (*__free_fn)(void*)) __INTRODUCED_IN(16);
void* tfind(const void* __key, void* const* __root_ptr, int (*__comparator)(const void*, const void*)) __INTRODUCED_IN(16);
void* tsearch(const void* __key, void** __root_ptr, int (*__comparator)(const void*, const void*)) __INTRODUCED_IN(16);
#endif /* __ANDROID_API__ >= 16 */


#if __ANDROID_API__ >= 21
void twalk(const void* __root, void (*__visitor)(const void*, VISIT, int)) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


__END_DECLS

#endif
