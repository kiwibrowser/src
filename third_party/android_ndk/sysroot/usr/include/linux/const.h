/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _LINUX_CONST_H
#define _LINUX_CONST_H
#ifdef __ASSEMBLY__
#define _AC(X,Y) X
#define _AT(T,X) X
#else
#define __AC(X,Y) (X ##Y)
#define _AC(X,Y) __AC(X, Y)
#define _AT(T,X) ((T) (X))
#endif
#define _BITUL(x) (_AC(1, UL) << (x))
#define _BITULL(x) (_AC(1, ULL) << (x))
#endif
