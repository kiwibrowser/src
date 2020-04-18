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
#ifndef _UAPI_GPIO_H_
#define _UAPI_GPIO_H_
#include <linux/ioctl.h>
#include <linux/types.h>
struct gpiochip_info {
  char name[32];
  char label[32];
  __u32 lines;
};
#define GPIOLINE_FLAG_KERNEL (1UL << 0)
#define GPIOLINE_FLAG_IS_OUT (1UL << 1)
#define GPIOLINE_FLAG_ACTIVE_LOW (1UL << 2)
#define GPIOLINE_FLAG_OPEN_DRAIN (1UL << 3)
#define GPIOLINE_FLAG_OPEN_SOURCE (1UL << 4)
struct gpioline_info {
  __u32 line_offset;
  __u32 flags;
  char name[32];
  char consumer[32];
};
#define GPIOHANDLES_MAX 64
#define GPIOHANDLE_REQUEST_INPUT (1UL << 0)
#define GPIOHANDLE_REQUEST_OUTPUT (1UL << 1)
#define GPIOHANDLE_REQUEST_ACTIVE_LOW (1UL << 2)
#define GPIOHANDLE_REQUEST_OPEN_DRAIN (1UL << 3)
#define GPIOHANDLE_REQUEST_OPEN_SOURCE (1UL << 4)
struct gpiohandle_request {
  __u32 lineoffsets[GPIOHANDLES_MAX];
  __u32 flags;
  __u8 default_values[GPIOHANDLES_MAX];
  char consumer_label[32];
  __u32 lines;
  int fd;
};
struct gpiohandle_data {
  __u8 values[GPIOHANDLES_MAX];
};
#define GPIOHANDLE_GET_LINE_VALUES_IOCTL _IOWR(0xB4, 0x08, struct gpiohandle_data)
#define GPIOHANDLE_SET_LINE_VALUES_IOCTL _IOWR(0xB4, 0x09, struct gpiohandle_data)
#define GPIOEVENT_REQUEST_RISING_EDGE (1UL << 0)
#define GPIOEVENT_REQUEST_FALLING_EDGE (1UL << 1)
#define GPIOEVENT_REQUEST_BOTH_EDGES ((1UL << 0) | (1UL << 1))
struct gpioevent_request {
  __u32 lineoffset;
  __u32 handleflags;
  __u32 eventflags;
  char consumer_label[32];
  int fd;
};
#define GPIOEVENT_EVENT_RISING_EDGE 0x01
#define GPIOEVENT_EVENT_FALLING_EDGE 0x02
struct gpioevent_data {
  __u64 timestamp;
  __u32 id;
};
#define GPIO_GET_CHIPINFO_IOCTL _IOR(0xB4, 0x01, struct gpiochip_info)
#define GPIO_GET_LINEINFO_IOCTL _IOWR(0xB4, 0x02, struct gpioline_info)
#define GPIO_GET_LINEHANDLE_IOCTL _IOWR(0xB4, 0x03, struct gpiohandle_request)
#define GPIO_GET_LINEEVENT_IOCTL _IOWR(0xB4, 0x04, struct gpioevent_request)
#endif
