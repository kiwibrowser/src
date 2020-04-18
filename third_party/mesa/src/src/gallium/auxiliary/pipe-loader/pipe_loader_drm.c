/**************************************************************************
 *
 * Copyright 2011 Intel Corporation
 * Copyright 2012 Francisco Jerez
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 *
 **************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <libudev.h>
#include <xf86drm.h>

#ifdef PIPE_LOADER_HAVE_XCB

#include <xcb/dri2.h>

#endif

#include "state_tracker/drm_driver.h"
#include "pipe_loader_priv.h"

#include "util/u_memory.h"
#include "util/u_dl.h"
#include "util/u_debug.h"

#define DRIVER_MAP_GALLIUM_ONLY
#include "pci_ids/pci_id_driver_map.h"

struct pipe_loader_drm_device {
   struct pipe_loader_device base;
   struct util_dl_library *lib;
   int fd;
};

#define pipe_loader_drm_device(dev) ((struct pipe_loader_drm_device *)dev)

static boolean
find_drm_pci_id(struct pipe_loader_drm_device *ddev)
{
   struct udev *udev = NULL;
   struct udev_device *parent, *device = NULL;
   struct stat stat;
   const char *pci_id;

   if (fstat(ddev->fd, &stat) < 0)
      goto fail;

   udev = udev_new();
   if (!udev)
      goto fail;

   device = udev_device_new_from_devnum(udev, 'c', stat.st_rdev);
   if (!device)
      goto fail;

   parent = udev_device_get_parent(device);
   if (!parent)
      goto fail;

   pci_id = udev_device_get_property_value(parent, "PCI_ID");
   if (!pci_id ||
       sscanf(pci_id, "%x:%x", &ddev->base.u.pci.vendor_id,
              &ddev->base.u.pci.chip_id) != 2)
      goto fail;

   return TRUE;

  fail:
   if (device)
      udev_device_unref(device);
   if (udev)
      udev_unref(udev);

   debug_printf("pci id for fd %d not found\n", ddev->fd);
   return FALSE;
}

static boolean
find_drm_driver_name(struct pipe_loader_drm_device *ddev)
{
   struct pipe_loader_device *dev = &ddev->base;
   int i, j;

   for (i = 0; driver_map[i].driver; i++) {
      if (dev->u.pci.vendor_id != driver_map[i].vendor_id)
         continue;

      if (driver_map[i].num_chips_ids == -1) {
         dev->driver_name = driver_map[i].driver;
         goto found;
      }

      for (j = 0; j < driver_map[i].num_chips_ids; j++) {
         if (dev->u.pci.chip_id == driver_map[i].chip_ids[j]) {
            dev->driver_name = driver_map[i].driver;
            goto found;
         }
      }
   }

   return FALSE;

  found:
   debug_printf("driver for %04x:%04x: %s\n", dev->u.pci.vendor_id,
                dev->u.pci.chip_id, dev->driver_name);
   return TRUE;
}

static struct pipe_loader_ops pipe_loader_drm_ops;

static void
pipe_loader_drm_x_auth(int fd)
{
#if PIPE_LOADER_HAVE_XCB
   /* Try authenticate with the X server to give us access to devices that X
    * is running on. */
   xcb_connection_t *xcb_conn;
   const xcb_setup_t *xcb_setup;
   xcb_screen_iterator_t s;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_dri2_connect_reply_t *connect;
   drm_magic_t magic;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_dri2_authenticate_reply_t *authenticate;

   xcb_conn = xcb_connect(NULL,  NULL);

   if(!xcb_conn)
      return;

   xcb_setup = xcb_get_setup(xcb_conn);

  if (!xcb_setup)
    goto disconnect;

   s = xcb_setup_roots_iterator(xcb_setup);
   connect_cookie = xcb_dri2_connect_unchecked(xcb_conn, s.data->root,
                                               XCB_DRI2_DRIVER_TYPE_DRI);
   connect = xcb_dri2_connect_reply(xcb_conn, connect_cookie, NULL);

   if (!connect || connect->driver_name_length
                   + connect->device_name_length == 0) {

      goto disconnect;
   }

   if (drmGetMagic(fd, &magic))
      goto disconnect;

   authenticate_cookie = xcb_dri2_authenticate_unchecked(xcb_conn,
                                                         s.data->root,
                                                         magic);
   authenticate = xcb_dri2_authenticate_reply(xcb_conn,
                                              authenticate_cookie,
                                              NULL);
   FREE(authenticate);

disconnect:
   xcb_disconnect(xcb_conn);

#endif
}

boolean
pipe_loader_drm_probe_fd(struct pipe_loader_device **dev, int fd)
{
   struct pipe_loader_drm_device *ddev = CALLOC_STRUCT(pipe_loader_drm_device);

   ddev->base.type = PIPE_LOADER_DEVICE_PCI;
   ddev->base.ops = &pipe_loader_drm_ops;
   ddev->fd = fd;

   pipe_loader_drm_x_auth(fd);

   if (!find_drm_pci_id(ddev))
      goto fail;

   if (!find_drm_driver_name(ddev))
      goto fail;

   *dev = &ddev->base;
   return TRUE;

  fail:
   FREE(ddev);
   return FALSE;
}

static int
open_drm_minor(int minor)
{
   char path[PATH_MAX];
   snprintf(path, sizeof(path), DRM_DEV_NAME, DRM_DIR_NAME, minor);
   return open(path, O_RDWR, 0);
}

int
pipe_loader_drm_probe(struct pipe_loader_device **devs, int ndev)
{
   int i, j, fd;

   for (i = 0, j = 0; i < DRM_MAX_MINOR; i++) {
      fd = open_drm_minor(i);
      if (fd < 0)
         continue;

      if (j >= ndev || !pipe_loader_drm_probe_fd(&devs[j], fd))
         close(fd);

      j++;
   }

   return j;
}

static void
pipe_loader_drm_release(struct pipe_loader_device **dev)
{
   struct pipe_loader_drm_device *ddev = pipe_loader_drm_device(*dev);

   if (ddev->lib)
      util_dl_close(ddev->lib);

   close(ddev->fd);
   FREE(ddev);
   *dev = NULL;
}

static struct pipe_screen *
pipe_loader_drm_create_screen(struct pipe_loader_device *dev,
                              const char *library_paths)
{
   struct pipe_loader_drm_device *ddev = pipe_loader_drm_device(dev);
   const struct drm_driver_descriptor *dd;

   if (!ddev->lib)
      ddev->lib = pipe_loader_find_module(dev, library_paths);
   if (!ddev->lib)
      return NULL;

   dd = (const struct drm_driver_descriptor *)
      util_dl_get_proc_address(ddev->lib, "driver_descriptor");

   /* sanity check on the name */
   if (!dd || strcmp(dd->name, ddev->base.driver_name) != 0)
      return NULL;

   return dd->create_screen(ddev->fd);
}

static struct pipe_loader_ops pipe_loader_drm_ops = {
   .create_screen = pipe_loader_drm_create_screen,
   .release = pipe_loader_drm_release
};
