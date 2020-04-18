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
#ifndef _LINUX_NCP_FS_H
#define _LINUX_NCP_FS_H
#include <linux/fs.h>
#include <linux/in.h>
#include <linux/types.h>
#include <linux/magic.h>
#include <linux/ipx.h>
#include <linux/ncp_no.h>
struct ncp_ioctl_request {
  unsigned int function;
  unsigned int size;
  char __user * data;
};
struct ncp_fs_info {
  int version;
  struct sockaddr_ipx addr;
  __kernel_uid_t mounted_uid;
  int connection;
  int buffer_size;
  int volume_number;
  __le32 directory_id;
};
struct ncp_fs_info_v2 {
  int version;
  unsigned long mounted_uid;
  unsigned int connection;
  unsigned int buffer_size;
  unsigned int volume_number;
  __le32 directory_id;
  __u32 dummy1;
  __u32 dummy2;
  __u32 dummy3;
};
struct ncp_sign_init {
  char sign_root[8];
  char sign_last[16];
};
struct ncp_lock_ioctl {
#define NCP_LOCK_LOG 0
#define NCP_LOCK_SH 1
#define NCP_LOCK_EX 2
#define NCP_LOCK_CLEAR 256
  int cmd;
  int origin;
  unsigned int offset;
  unsigned int length;
#define NCP_LOCK_DEFAULT_TIMEOUT 18
#define NCP_LOCK_MAX_TIMEOUT 180
  int timeout;
};
struct ncp_setroot_ioctl {
  int volNumber;
  int namespace;
  __le32 dirEntNum;
};
struct ncp_objectname_ioctl {
#define NCP_AUTH_NONE 0x00
#define NCP_AUTH_BIND 0x31
#define NCP_AUTH_NDS 0x32
  int auth_type;
  size_t object_name_len;
  void __user * object_name;
};
struct ncp_privatedata_ioctl {
  size_t len;
  void __user * data;
};
#define NCP_IOCSNAME_LEN 20
struct ncp_nls_ioctl {
  unsigned char codepage[NCP_IOCSNAME_LEN + 1];
  unsigned char iocharset[NCP_IOCSNAME_LEN + 1];
};
#define NCP_IOC_NCPREQUEST _IOR('n', 1, struct ncp_ioctl_request)
#define NCP_IOC_GETMOUNTUID _IOW('n', 2, __kernel_old_uid_t)
#define NCP_IOC_GETMOUNTUID2 _IOW('n', 2, unsigned long)
#define NCP_IOC_CONN_LOGGED_IN _IO('n', 3)
#define NCP_GET_FS_INFO_VERSION (1)
#define NCP_IOC_GET_FS_INFO _IOWR('n', 4, struct ncp_fs_info)
#define NCP_GET_FS_INFO_VERSION_V2 (2)
#define NCP_IOC_GET_FS_INFO_V2 _IOWR('n', 4, struct ncp_fs_info_v2)
#define NCP_IOC_SIGN_INIT _IOR('n', 5, struct ncp_sign_init)
#define NCP_IOC_SIGN_WANTED _IOR('n', 6, int)
#define NCP_IOC_SET_SIGN_WANTED _IOW('n', 6, int)
#define NCP_IOC_LOCKUNLOCK _IOR('n', 7, struct ncp_lock_ioctl)
#define NCP_IOC_GETROOT _IOW('n', 8, struct ncp_setroot_ioctl)
#define NCP_IOC_SETROOT _IOR('n', 8, struct ncp_setroot_ioctl)
#define NCP_IOC_GETOBJECTNAME _IOWR('n', 9, struct ncp_objectname_ioctl)
#define NCP_IOC_SETOBJECTNAME _IOR('n', 9, struct ncp_objectname_ioctl)
#define NCP_IOC_GETPRIVATEDATA _IOWR('n', 10, struct ncp_privatedata_ioctl)
#define NCP_IOC_SETPRIVATEDATA _IOR('n', 10, struct ncp_privatedata_ioctl)
#define NCP_IOC_GETCHARSETS _IOWR('n', 11, struct ncp_nls_ioctl)
#define NCP_IOC_SETCHARSETS _IOR('n', 11, struct ncp_nls_ioctl)
#define NCP_IOC_GETDENTRYTTL _IOW('n', 12, __u32)
#define NCP_IOC_SETDENTRYTTL _IOR('n', 12, __u32)
#define NCP_PACKET_SIZE 4070
#define NCP_MAXPATHLEN 255
#define NCP_MAXNAMELEN 14
#endif
