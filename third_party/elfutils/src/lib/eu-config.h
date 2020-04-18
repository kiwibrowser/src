/* Configuration definitions.
   Copyright (C) 2008, 2009 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifndef EU_CONFIG_H
#define EU_CONFIG_H	1

#ifdef USE_LOCKS
# include <pthread.h>
# include <assert.h>
# define rwlock_define(class,name)	class pthread_rwlock_t name
# define RWLOCK_CALL(call)		\
  ({ int _err = pthread_rwlock_ ## call; assert_perror (_err); })
# define rwlock_init(lock)		RWLOCK_CALL (init (&lock, NULL))
# define rwlock_fini(lock)		RWLOCK_CALL (destroy (&lock))
# define rwlock_rdlock(lock)		RWLOCK_CALL (rdlock (&lock))
# define rwlock_wrlock(lock)		RWLOCK_CALL (wrlock (&lock))
# define rwlock_unlock(lock)		RWLOCK_CALL (unlock (&lock))
#else
/* Eventually we will allow multi-threaded applications to use the
   libraries.  Therefore we will add the necessary locking although
   the macros used expand to nothing for now.  */
# define rwlock_define(class,name) class int name
# define rwlock_init(lock) ((void) (lock))
# define rwlock_fini(lock) ((void) (lock))
# define rwlock_rdlock(lock) ((void) (lock))
# define rwlock_wrlock(lock) ((void) (lock))
# define rwlock_unlock(lock) ((void) (lock))
#endif	/* USE_LOCKS */

/* gettext helper macro.  */
#define N_(Str) Str

/* Compiler-specific definitions.  */
#define strong_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((alias (#name)));

#ifdef __i386__
# define internal_function __attribute__ ((regparm (3), stdcall))
#else
# define internal_function /* nothing */
#endif

#define internal_strong_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((alias (#name))) internal_function;

#define attribute_hidden \
  __attribute__ ((visibility ("hidden")))

/* Define ALLOW_UNALIGNED if the architecture allows operations on
   unaligned memory locations.  */
#if defined __i386__ || defined __x86_64__
# define ALLOW_UNALIGNED	1
#else
# define ALLOW_UNALIGNED	0
#endif

#if DEBUGPRED
# ifdef __x86_64__
asm (".section predict_data, \"aw\"; .previous\n"
     ".section predict_line, \"a\"; .previous\n"
     ".section predict_file, \"a\"; .previous");
#  ifndef PIC
#   define debugpred__(e, E) \
  ({ long int _e = !!(e); \
     asm volatile (".pushsection predict_data; ..predictcnt%=: .quad 0; .quad 0\n" \
                   ".section predict_line; .quad %c1\n" \
                   ".section predict_file; .quad %c2; .popsection\n" \
                   "addq $1,..predictcnt%=(,%0,8)" \
                   : : "r" (_e == E), "i" (__LINE__), "i" (__FILE__)); \
    __builtin_expect (_e, E); \
  })
#  endif
# elif defined __i386__
asm (".section predict_data, \"aw\"; .previous\n"
     ".section predict_line, \"a\"; .previous\n"
     ".section predict_file, \"a\"; .previous");
#  ifndef PIC
#   define debugpred__(e, E) \
  ({ long int _e = !!(e); \
     asm volatile (".pushsection predict_data; ..predictcnt%=: .long 0; .long 0\n" \
                   ".section predict_line; .long %c1\n" \
                   ".section predict_file; .long %c2; .popsection\n" \
                   "incl ..predictcnt%=(,%0,8)" \
                   : : "r" (_e == E), "i" (__LINE__), "i" (__FILE__)); \
    __builtin_expect (_e, E); \
  })
#  endif
# endif
# ifdef debugpred__
#  define unlikely(e) debugpred__ (e,0)
#  define likely(e) debugpred__ (e,1)
# endif
#endif
#ifndef likely
# define unlikely(expr) __builtin_expect (!!(expr), 0)
# define likely(expr) __builtin_expect (!!(expr), 1)
#endif

#define obstack_calloc(ob, size) \
  ({ size_t _s = (size); memset (obstack_alloc (ob, _s), '\0', _s); })
#define obstack_strdup(ob, str) \
  ({ const char *_s = (str); obstack_copy0 (ob, _s, strlen (_s)); })
#define obstack_strndup(ob, str, n) \
  ({ const char *_s = (str); obstack_copy0 (ob, _s, strnlen (_s, n)); })

#if __STDC_VERSION__ >= 199901L
# define flexarr_size /* empty */
#else
# define flexarr_size 0
#endif

/* Calling conventions.  */
#ifdef __i386__
# define CALLING_CONVENTION regparm (3), stdcall
# define AND_CALLING_CONVENTION , regparm (3), stdcall
#else
# define CALLING_CONVENTION
# define AND_CALLING_CONVENTION
#endif

/* Avoid PLT entries.  */
#ifdef PIC
# define INTUSE(name) _INTUSE(name)
# define _INTUSE(name) __##name##_internal
# define INTDEF(name) _INTDEF(name)
# define _INTDEF(name) \
  extern __typeof__ (name) __##name##_internal __attribute__ ((alias (#name)));
# define INTDECL(name) _INTDECL(name)
# define _INTDECL(name) \
  extern __typeof__ (name) __##name##_internal attribute_hidden;
#else
# define INTUSE(name) name
# define INTDEF(name) /* empty */
# define INTDECL(name) /* empty */
#endif

/* This macro is used by the tests conditionalize for standalone building.  */
#define ELFUTILS_HEADER(name) <lib##name.h>


#ifdef SHARED
# define OLD_VERSION(name, version) \
  asm (".globl _compat." #version "." #name "\n" \
       "_compat." #version "." #name " = " #name "\n" \
       ".symver _compat." #version "." #name "," #name "@" #version);
# define NEW_VERSION(name, version) \
  asm (".symver " #name "," #name "@@@" #version);
# define COMPAT_VERSION_NEWPROTO(name, version, prefix) \
  asm (".symver _compat." #version "." #name "," #name "@" #version); \
  __typeof (_compat_##prefix##_##name) _compat_##prefix##_##name \
    asm ("_compat." #version "." #name);
# define COMPAT_VERSION(name, version, prefix) \
  asm (".symver _compat." #version "." #name "," #name "@" #version); \
  __typeof (name) _compat_##prefix##_##name asm ("_compat." #version "." #name);
#else
# define OLD_VERSION(name, version) /* Nothing for static linking.  */
# define NEW_VERSION(name, version) /* Nothing for static linking.  */
# define COMPAT_VERSION_NEWPROTO(name, version, prefix) \
  error "should use #ifdef SHARED"
# define COMPAT_VERSION(name, version, prefix) error "should use #ifdef SHARED"
#endif


#endif	/* eu-config.h */
