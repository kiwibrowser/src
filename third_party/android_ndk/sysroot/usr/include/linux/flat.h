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
#ifndef _UAPI_LINUX_FLAT_H
#define _UAPI_LINUX_FLAT_H
#define FLAT_VERSION 0x00000004L
#define MAX_SHARED_LIBS (1)
struct flat_hdr {
  char magic[4];
  unsigned long rev;
  unsigned long entry;
  unsigned long data_start;
  unsigned long data_end;
  unsigned long bss_end;
  unsigned long stack_size;
  unsigned long reloc_start;
  unsigned long reloc_count;
  unsigned long flags;
  unsigned long build_date;
  unsigned long filler[5];
};
#define FLAT_FLAG_RAM 0x0001
#define FLAT_FLAG_GOTPIC 0x0002
#define FLAT_FLAG_GZIP 0x0004
#define FLAT_FLAG_GZDATA 0x0008
#define FLAT_FLAG_KTRACE 0x0010
#endif
