/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdio.h>
#include <string.h>

#include <libudev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "gbmint.h"

GBM_EXPORT struct udev_device *
_gbm_udev_device_new_from_fd(struct udev *udev, int fd)
{
   struct udev_device *device;
   struct stat buf;

   if (fstat(fd, &buf) < 0) {
      fprintf(stderr, "gbm: failed to stat fd %d", fd);
      return NULL;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      fprintf(stderr,
              "gbm: could not create udev device for fd %d", fd);
      return NULL;
   }

   return device;
}

GBM_EXPORT char *
_gbm_fd_get_device_name(int fd)
{
   struct udev *udev;
   struct udev_device *device;
   const char *const_device_name;
   char *device_name = NULL;

   udev = udev_new();
   device = _gbm_udev_device_new_from_fd(udev, fd);
   if (device == NULL)
      return NULL;

   const_device_name = udev_device_get_devnode(device);
   if (!const_device_name)
      goto out;
   device_name = strdup(const_device_name);

out:
   udev_device_unref(device);
   udev_unref(udev);

   return device_name;
}

GBM_EXPORT void
_gbm_log(const char *fmt_str, ...)
{
}
