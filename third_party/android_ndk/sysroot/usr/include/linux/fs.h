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
#ifndef _UAPI_LINUX_FS_H
#define _UAPI_LINUX_FS_H
#include <linux/limits.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#undef NR_OPEN
#define INR_OPEN_CUR 1024
#define INR_OPEN_MAX 4096
#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE (1 << BLOCK_SIZE_BITS)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_DATA 3
#define SEEK_HOLE 4
#define SEEK_MAX SEEK_HOLE
#define RENAME_NOREPLACE (1 << 0)
#define RENAME_EXCHANGE (1 << 1)
#define RENAME_WHITEOUT (1 << 2)
struct file_clone_range {
  __s64 src_fd;
  __u64 src_offset;
  __u64 src_length;
  __u64 dest_offset;
};
struct fstrim_range {
  __u64 start;
  __u64 len;
  __u64 minlen;
};
#define FILE_DEDUPE_RANGE_SAME 0
#define FILE_DEDUPE_RANGE_DIFFERS 1
struct file_dedupe_range_info {
  __s64 dest_fd;
  __u64 dest_offset;
  __u64 bytes_deduped;
  __s32 status;
  __u32 reserved;
};
struct file_dedupe_range {
  __u64 src_offset;
  __u64 src_length;
  __u16 dest_count;
  __u16 reserved1;
  __u32 reserved2;
  struct file_dedupe_range_info info[0];
};
struct files_stat_struct {
  unsigned long nr_files;
  unsigned long nr_free_files;
  unsigned long max_files;
};
struct inodes_stat_t {
  long nr_inodes;
  long nr_unused;
  long dummy[5];
};
#define NR_FILE 8192
#define MS_RDONLY 1
#define MS_NOSUID 2
#define MS_NODEV 4
#define MS_NOEXEC 8
#define MS_SYNCHRONOUS 16
#define MS_REMOUNT 32
#define MS_MANDLOCK 64
#define MS_DIRSYNC 128
#define MS_NOATIME 1024
#define MS_NODIRATIME 2048
#define MS_BIND 4096
#define MS_MOVE 8192
#define MS_REC 16384
#define MS_VERBOSE 32768
#define MS_SILENT 32768
#define MS_POSIXACL (1 << 16)
#define MS_UNBINDABLE (1 << 17)
#define MS_PRIVATE (1 << 18)
#define MS_SLAVE (1 << 19)
#define MS_SHARED (1 << 20)
#define MS_RELATIME (1 << 21)
#define MS_KERNMOUNT (1 << 22)
#define MS_I_VERSION (1 << 23)
#define MS_STRICTATIME (1 << 24)
#define MS_LAZYTIME (1 << 25)
#define MS_SUBMOUNT (1 << 26)
#define MS_NOREMOTELOCK (1 << 27)
#define MS_NOSEC (1 << 28)
#define MS_BORN (1 << 29)
#define MS_ACTIVE (1 << 30)
#define MS_NOUSER (1 << 31)
#define MS_RMT_MASK (MS_RDONLY | MS_SYNCHRONOUS | MS_MANDLOCK | MS_I_VERSION | MS_LAZYTIME)
#define MS_MGC_VAL 0xC0ED0000
#define MS_MGC_MSK 0xffff0000
struct fsxattr {
  __u32 fsx_xflags;
  __u32 fsx_extsize;
  __u32 fsx_nextents;
  __u32 fsx_projid;
  __u32 fsx_cowextsize;
  unsigned char fsx_pad[8];
};
#define FS_XFLAG_REALTIME 0x00000001
#define FS_XFLAG_PREALLOC 0x00000002
#define FS_XFLAG_IMMUTABLE 0x00000008
#define FS_XFLAG_APPEND 0x00000010
#define FS_XFLAG_SYNC 0x00000020
#define FS_XFLAG_NOATIME 0x00000040
#define FS_XFLAG_NODUMP 0x00000080
#define FS_XFLAG_RTINHERIT 0x00000100
#define FS_XFLAG_PROJINHERIT 0x00000200
#define FS_XFLAG_NOSYMLINKS 0x00000400
#define FS_XFLAG_EXTSIZE 0x00000800
#define FS_XFLAG_EXTSZINHERIT 0x00001000
#define FS_XFLAG_NODEFRAG 0x00002000
#define FS_XFLAG_FILESTREAM 0x00004000
#define FS_XFLAG_DAX 0x00008000
#define FS_XFLAG_COWEXTSIZE 0x00010000
#define FS_XFLAG_HASATTR 0x80000000
#define BLKROSET _IO(0x12, 93)
#define BLKROGET _IO(0x12, 94)
#define BLKRRPART _IO(0x12, 95)
#define BLKGETSIZE _IO(0x12, 96)
#define BLKFLSBUF _IO(0x12, 97)
#define BLKRASET _IO(0x12, 98)
#define BLKRAGET _IO(0x12, 99)
#define BLKFRASET _IO(0x12, 100)
#define BLKFRAGET _IO(0x12, 101)
#define BLKSECTSET _IO(0x12, 102)
#define BLKSECTGET _IO(0x12, 103)
#define BLKSSZGET _IO(0x12, 104)
#define BLKBSZGET _IOR(0x12, 112, size_t)
#define BLKBSZSET _IOW(0x12, 113, size_t)
#define BLKGETSIZE64 _IOR(0x12, 114, size_t)
#define BLKTRACESETUP _IOWR(0x12, 115, struct blk_user_trace_setup)
#define BLKTRACESTART _IO(0x12, 116)
#define BLKTRACESTOP _IO(0x12, 117)
#define BLKTRACETEARDOWN _IO(0x12, 118)
#define BLKDISCARD _IO(0x12, 119)
#define BLKIOMIN _IO(0x12, 120)
#define BLKIOOPT _IO(0x12, 121)
#define BLKALIGNOFF _IO(0x12, 122)
#define BLKPBSZGET _IO(0x12, 123)
#define BLKDISCARDZEROES _IO(0x12, 124)
#define BLKSECDISCARD _IO(0x12, 125)
#define BLKROTATIONAL _IO(0x12, 126)
#define BLKZEROOUT _IO(0x12, 127)
#define BMAP_IOCTL 1
#define FIBMAP _IO(0x00, 1)
#define FIGETBSZ _IO(0x00, 2)
#define FIFREEZE _IOWR('X', 119, int)
#define FITHAW _IOWR('X', 120, int)
#define FITRIM _IOWR('X', 121, struct fstrim_range)
#define FICLONE _IOW(0x94, 9, int)
#define FICLONERANGE _IOW(0x94, 13, struct file_clone_range)
#define FIDEDUPERANGE _IOWR(0x94, 54, struct file_dedupe_range)
#define FS_IOC_GETFLAGS _IOR('f', 1, long)
#define FS_IOC_SETFLAGS _IOW('f', 2, long)
#define FS_IOC_GETVERSION _IOR('v', 1, long)
#define FS_IOC_SETVERSION _IOW('v', 2, long)
#define FS_IOC_FIEMAP _IOWR('f', 11, struct fiemap)
#define FS_IOC32_GETFLAGS _IOR('f', 1, int)
#define FS_IOC32_SETFLAGS _IOW('f', 2, int)
#define FS_IOC32_GETVERSION _IOR('v', 1, int)
#define FS_IOC32_SETVERSION _IOW('v', 2, int)
#define FS_IOC_FSGETXATTR _IOR('X', 31, struct fsxattr)
#define FS_IOC_FSSETXATTR _IOW('X', 32, struct fsxattr)
#define FS_KEY_DESCRIPTOR_SIZE 8
#define FS_POLICY_FLAGS_PAD_4 0x00
#define FS_POLICY_FLAGS_PAD_8 0x01
#define FS_POLICY_FLAGS_PAD_16 0x02
#define FS_POLICY_FLAGS_PAD_32 0x03
#define FS_POLICY_FLAGS_PAD_MASK 0x03
#define FS_POLICY_FLAGS_VALID 0x03
#define FS_ENCRYPTION_MODE_INVALID 0
#define FS_ENCRYPTION_MODE_AES_256_XTS 1
#define FS_ENCRYPTION_MODE_AES_256_GCM 2
#define FS_ENCRYPTION_MODE_AES_256_CBC 3
#define FS_ENCRYPTION_MODE_AES_256_CTS 4
struct fscrypt_policy {
  __u8 version;
  __u8 contents_encryption_mode;
  __u8 filenames_encryption_mode;
  __u8 flags;
  __u8 master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
};
#define FS_IOC_SET_ENCRYPTION_POLICY _IOR('f', 19, struct fscrypt_policy)
#define FS_IOC_GET_ENCRYPTION_PWSALT _IOW('f', 20, __u8[16])
#define FS_IOC_GET_ENCRYPTION_POLICY _IOW('f', 21, struct fscrypt_policy)
#define FS_KEY_DESC_PREFIX "fscrypt:"
#define FS_KEY_DESC_PREFIX_SIZE 8
#define FS_MAX_KEY_SIZE 64
struct fscrypt_key {
  __u32 mode;
  __u8 raw[FS_MAX_KEY_SIZE];
  __u32 size;
};
#define FS_SECRM_FL 0x00000001
#define FS_UNRM_FL 0x00000002
#define FS_COMPR_FL 0x00000004
#define FS_SYNC_FL 0x00000008
#define FS_IMMUTABLE_FL 0x00000010
#define FS_APPEND_FL 0x00000020
#define FS_NODUMP_FL 0x00000040
#define FS_NOATIME_FL 0x00000080
#define FS_DIRTY_FL 0x00000100
#define FS_COMPRBLK_FL 0x00000200
#define FS_NOCOMP_FL 0x00000400
#define FS_ENCRYPT_FL 0x00000800
#define FS_BTREE_FL 0x00001000
#define FS_INDEX_FL 0x00001000
#define FS_IMAGIC_FL 0x00002000
#define FS_JOURNAL_DATA_FL 0x00004000
#define FS_NOTAIL_FL 0x00008000
#define FS_DIRSYNC_FL 0x00010000
#define FS_TOPDIR_FL 0x00020000
#define FS_HUGE_FILE_FL 0x00040000
#define FS_EXTENT_FL 0x00080000
#define FS_EA_INODE_FL 0x00200000
#define FS_EOFBLOCKS_FL 0x00400000
#define FS_NOCOW_FL 0x00800000
#define FS_INLINE_DATA_FL 0x10000000
#define FS_PROJINHERIT_FL 0x20000000
#define FS_RESERVED_FL 0x80000000
#define FS_FL_USER_VISIBLE 0x0003DFFF
#define FS_FL_USER_MODIFIABLE 0x000380FF
#define SYNC_FILE_RANGE_WAIT_BEFORE 1
#define SYNC_FILE_RANGE_WRITE 2
#define SYNC_FILE_RANGE_WAIT_AFTER 4
#define RWF_HIPRI 0x00000001
#define RWF_DSYNC 0x00000002
#define RWF_SYNC 0x00000004
#endif
