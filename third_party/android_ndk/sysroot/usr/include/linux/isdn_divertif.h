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
#ifndef _UAPI_LINUX_ISDN_DIVERTIF_H
#define _UAPI_LINUX_ISDN_DIVERTIF_H
#define DIVERT_IF_MAGIC 0x25873401
#define DIVERT_CMD_REG 0x00
#define DIVERT_CMD_REL 0x01
#define DIVERT_NO_ERR 0x00
#define DIVERT_CMD_ERR 0x01
#define DIVERT_VER_ERR 0x02
#define DIVERT_REG_ERR 0x03
#define DIVERT_REL_ERR 0x04
#define DIVERT_REG_NAME isdn_register_divert
#endif
