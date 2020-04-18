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
#ifndef _UAPI_LINUX_THERMAL_H
#define _UAPI_LINUX_THERMAL_H
#define THERMAL_NAME_LENGTH 20
#define THERMAL_GENL_FAMILY_NAME "thermal_event"
#define THERMAL_GENL_VERSION 0x01
#define THERMAL_GENL_MCAST_GROUP_NAME "thermal_mc_grp"
enum events {
  THERMAL_AUX0,
  THERMAL_AUX1,
  THERMAL_CRITICAL,
  THERMAL_DEV_FAULT,
};
enum {
  THERMAL_GENL_ATTR_UNSPEC,
  THERMAL_GENL_ATTR_EVENT,
  __THERMAL_GENL_ATTR_MAX,
};
#define THERMAL_GENL_ATTR_MAX (__THERMAL_GENL_ATTR_MAX - 1)
enum {
  THERMAL_GENL_CMD_UNSPEC,
  THERMAL_GENL_CMD_EVENT,
  __THERMAL_GENL_CMD_MAX,
};
#define THERMAL_GENL_CMD_MAX (__THERMAL_GENL_CMD_MAX - 1)
#endif
