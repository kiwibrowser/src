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
#ifndef GIGASET_INTERFACE_H
#define GIGASET_INTERFACE_H
#include <linux/ioctl.h>
#define GIGASET_IOCTL 0x47
#define GIGASET_REDIR _IOWR(GIGASET_IOCTL, 0, int)
#define GIGASET_CONFIG _IOWR(GIGASET_IOCTL, 1, int)
#define GIGASET_BRKCHARS _IOW(GIGASET_IOCTL, 2, unsigned char[6])
#define GIGASET_VERSION _IOWR(GIGASET_IOCTL, 3, unsigned[4])
#define GIGVER_DRIVER 0
#define GIGVER_COMPAT 1
#define GIGVER_FWBASE 2
#endif
