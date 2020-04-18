/*	$OpenBSD: stdio.h,v 1.35 2006/01/13 18:10:09 miod Exp $	*/
/*	$NetBSD: stdio.h,v 1.18 1996/04/25 18:29:21 jtc Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stdio.h	5.17 (Berkeley) 6/3/91
 */

#ifndef	_STDIO_H_
#define	_STDIO_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stdarg.h>
#include <stddef.h>

#include <bits/seek_constants.h>

#if __ANDROID_API__ < __ANDROID_API_N__
#include <bits/struct_file.h>
#endif

__BEGIN_DECLS

typedef off_t fpos_t;
typedef off64_t fpos64_t;

struct __sFILE;
typedef struct __sFILE FILE;

#if __ANDROID_API__ >= __ANDROID_API_M__
extern FILE* stdin __INTRODUCED_IN(23);
extern FILE* stdout __INTRODUCED_IN(23);
extern FILE* stderr __INTRODUCED_IN(23);

/* C99 and earlier plus current C++ standards say these must be macros. */
#define stdin stdin
#define stdout stdout
#define stderr stderr
#else
/* Before M the actual symbols for stdin and friends had different names. */
extern FILE __sF[] __REMOVED_IN(23);

#define stdin (&__sF[0])
#define stdout (&__sF[1])
#define stderr (&__sF[2])
#endif

/*
 * The following three definitions are for ANSI C, which took them
 * from System V, which brilliantly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
 *
 * Although numbered as their counterparts above, the implementation
 * does not rely on this.
 */
#define	_IOFBF	0		/* setvbuf should set fully buffered */
#define	_IOLBF	1		/* setvbuf should set line buffered */
#define	_IONBF	2		/* setvbuf should set unbuffered */

#define	BUFSIZ	1024		/* size of buffer used by setbuf */
#define	EOF	(-1)

/*
 * FOPEN_MAX is a minimum maximum, and is the number of streams that
 * stdio can provide without attempting to allocate further resources
 * (which could fail).  Do not use this for anything.
 */
#define FOPEN_MAX 20
#define FILENAME_MAX 4096

#define L_tmpnam 4096
#define TMP_MAX 308915776

void clearerr(FILE* __fp);
int fclose(FILE* __fp);
int feof(FILE* __fp);
int ferror(FILE* __fp);
int fflush(FILE* __fp);
int fgetc(FILE* __fp);
char* fgets(char* __buf, int __size, FILE* __fp) __overloadable __RENAME_CLANG(fgets);
int fprintf(FILE* __fp , const char* __fmt, ...) __printflike(2, 3);
int fputc(int __ch, FILE* __fp);
int fputs(const char* __s, FILE* __fp);
size_t fread(void* __buf, size_t __size, size_t __count, FILE* __fp) __overloadable __RENAME_CLANG(fread);
int fscanf(FILE* __fp, const char* __fmt, ...) __scanflike(2, 3);
size_t fwrite(const void* __buf, size_t __size, size_t __count, FILE* __fp) __overloadable __RENAME_CLANG(fwrite);
int getc(FILE* __fp);
int getchar(void);

#if __ANDROID_API__ >= 18
ssize_t getdelim(char** __line_ptr, size_t* __line_length_ptr, int __delimiter, FILE* __fp) __INTRODUCED_IN(18);
ssize_t getline(char** __line_ptr, size_t* __line_length_ptr, FILE* __fp) __INTRODUCED_IN(18);
#endif /* __ANDROID_API__ >= 18 */


void perror(const char* __msg);
int printf(const char* __fmt, ...) __printflike(1, 2);
int putc(int __ch, FILE* __fp);
int putchar(int __ch);
int puts(const char* __s);
int remove(const char* __path);
void rewind(FILE* __fp);
int scanf(const char* __fmt, ...) __scanflike(1, 2);
void setbuf(FILE* __fp, char* __buf);
int setvbuf(FILE* __fp, char* __buf, int __mode, size_t __size);
int sscanf(const char* __s, const char* __fmt, ...) __scanflike(2, 3);
int ungetc(int __ch, FILE* __fp);
int vfprintf(FILE* __fp, const char* __fmt, va_list __args) __printflike(2, 0);
int vprintf(const char* __fp, va_list __args) __printflike(1, 0);

#if __ANDROID_API__ >= 21
int dprintf(int __fd, const char* __fmt, ...) __printflike(2, 3) __INTRODUCED_IN(21);
int vdprintf(int __fd, const char* __fmt, va_list __args) __printflike(2, 0) __INTRODUCED_IN(21);
#else
/*
 * Old versions of Android called these fdprintf and vfdprintf out of fears that the glibc names
 * would collide with user debug printfs.
 *
 * Allow users to just use dprintf and vfdprintf on any version by renaming those calls to their
 * legacy equivalents if needed.
 */
int dprintf(int __fd, const char* __fmt, ...) __RENAME(fdprintf) __printflike(2, 3);
int vdprintf(int __fd, const char* __fmt, va_list __args) __RENAME(vfdprintf) __printflike(2, 0);
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ < 201112L) || \
    (defined(__cplusplus) && __cplusplus <= 201103L)
char* gets(char* __buf) __attribute__((deprecated("gets is unsafe, use fgets instead")));
#endif
int sprintf(char* __s, const char* __fmt, ...)
    __printflike(2, 3) __warnattr_strict("sprintf is often misused; please use snprintf")
    __overloadable __RENAME_CLANG(sprintf);
int vsprintf(char* __s, const char* __fmt, va_list __args)
    __overloadable __printflike(2, 0) __RENAME_CLANG(vsprintf)
    __warnattr_strict("vsprintf is often misused; please use vsnprintf");
char* tmpnam(char* __s)
    __warnattr("tempnam is unsafe, use mkstemp or tmpfile instead");
#define P_tmpdir "/tmp/" /* deprecated */
char* tempnam(const char* __dir, const char* __prefix)
    __warnattr("tempnam is unsafe, use mkstemp or tmpfile instead");

int rename(const char* __old_path, const char* __new_path);
int renameat(int __old_dir_fd, const char* __old_path, int __new_dir_fd, const char* __new_path);

int fseek(FILE* __fp, long __offset, int __whence);
long ftell(FILE* __fp);

#if defined(__USE_FILE_OFFSET64)

#if __ANDROID_API__ >= 24
int fgetpos(FILE* __fp, fpos_t* __pos) __RENAME(fgetpos64) __INTRODUCED_IN(24);
int fsetpos(FILE* __fp, const fpos_t* __pos) __RENAME(fsetpos64) __INTRODUCED_IN(24);
int fseeko(FILE* __fp, off_t __offset, int __whence) __RENAME(fseeko64) __INTRODUCED_IN(24);
off_t ftello(FILE* __fp) __RENAME(ftello64) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

#  if defined(__USE_BSD)

#if __ANDROID_API__ >= 24
FILE* funopen(const void* __cookie,
              int (*__read_fn)(void*, char*, int),
              int (*__write_fn)(void*, const char*, int),
              fpos_t (*__seek_fn)(void*, fpos_t, int),
              int (*__close_fn)(void*)) __RENAME(funopen64) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

#  endif
#else
int fgetpos(FILE* __fp, fpos_t* __pos);
int fsetpos(FILE* __fp, const fpos_t* __pos);
int fseeko(FILE* __fp, off_t __offset, int __whence);
off_t ftello(FILE* __fp);
#  if defined(__USE_BSD)
FILE* funopen(const void* __cookie,
              int (*__read_fn)(void*, char*, int),
              int (*__write_fn)(void*, const char*, int),
              fpos_t (*__seek_fn)(void*, fpos_t, int),
              int (*__close_fn)(void*));
#  endif
#endif

#if __ANDROID_API__ >= 24
int fgetpos64(FILE* __fp, fpos64_t* __pos) __INTRODUCED_IN(24);
int fsetpos64(FILE* __fp, const fpos64_t* __pos) __INTRODUCED_IN(24);
int fseeko64(FILE* __fp, off64_t __offset, int __whence) __INTRODUCED_IN(24);
off64_t ftello64(FILE* __fp) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

#if defined(__USE_BSD)

#if __ANDROID_API__ >= 24
FILE* funopen64(const void* __cookie,
                int (*__read_fn)(void*, char*, int),
                int (*__write_fn)(void*, const char*, int),
                fpos64_t (*__seek_fn)(void*, fpos64_t, int),
                int (*__close_fn)(void*)) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

#endif

FILE* fopen(const char* __path, const char* __mode);

#if __ANDROID_API__ >= 24
FILE* fopen64(const char* __path, const char* __mode) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

FILE* freopen(const char* __path, const char* __mode, FILE* __fp);

#if __ANDROID_API__ >= 24
FILE* freopen64(const char* __path, const char* __mode, FILE* __fp) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

FILE* tmpfile(void);

#if __ANDROID_API__ >= 24
FILE* tmpfile64(void) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */


int snprintf(char* __buf, size_t __size, const char* __fmt, ...)
    __printflike(3, 4) __overloadable __RENAME_CLANG(snprintf);
int vfscanf(FILE* __fp, const char* __fmt, va_list __args) __scanflike(2, 0);
int vscanf(const char* __fmt , va_list __args) __scanflike(1, 0);
int vsnprintf(char* __buf, size_t __size, const char* __fmt, va_list __args)
    __printflike(3, 0) __overloadable __RENAME_CLANG(vsnprintf);
int vsscanf(const char* __s, const char* __fmt, va_list __args) __scanflike(2, 0);

#define L_ctermid 1024 /* size for ctermid() */

#if __ANDROID_API__ >= 26
char* ctermid(char* __buf) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */


FILE* fdopen(int __fd, const char* __mode);
int fileno(FILE* __fp);
int pclose(FILE* __fp);
FILE* popen(const char* __command, const char* __mode);
void flockfile(FILE* __fp);
int ftrylockfile(FILE* __fp);
void funlockfile(FILE* __fp);
int getc_unlocked(FILE* __fp);
int getchar_unlocked(void);
int putc_unlocked(int __ch, FILE* __fp);
int putchar_unlocked(int __ch);


#if __ANDROID_API__ >= 23
FILE* fmemopen(void* __buf, size_t __size, const char* __mode) __INTRODUCED_IN(23);
FILE* open_memstream(char** __ptr, size_t* __size_ptr) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


#if defined(__USE_BSD) || defined(__BIONIC__) /* Historically bionic exposed these. */
int  asprintf(char** __s_ptr, const char* __fmt, ...) __printflike(2, 3);
char* fgetln(FILE* __fp, size_t* __length_ptr);
int fpurge(FILE* __fp);
void setbuffer(FILE* __fp, char* __buf, int __size);
int setlinebuf(FILE* __fp);
int vasprintf(char** __s_ptr, const char* __fmt, va_list __args) __printflike(2, 0);

#if __ANDROID_API__ >= 23
void clearerr_unlocked(FILE* __fp) __INTRODUCED_IN(23);
int feof_unlocked(FILE* __fp) __INTRODUCED_IN(23);
int ferror_unlocked(FILE* __fp) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


#if __ANDROID_API__ >= 24
int fileno_unlocked(FILE* __fp) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

#define fropen(cookie, fn) funopen(cookie, fn, 0, 0, 0)
#define fwopen(cookie, fn) funopen(cookie, 0, fn, 0, 0)
#endif /* __USE_BSD */

#if defined(__BIONIC_INCLUDE_FORTIFY_HEADERS)
#include <bits/fortify/stdio.h>
#endif

__END_DECLS

#endif
