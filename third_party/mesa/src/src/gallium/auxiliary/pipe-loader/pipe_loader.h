/**************************************************************************
 *
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
 **************************************************************************/

/**
 * \file Library that provides device enumeration and creation of
 * winsys/pipe_screen instances.
 */

#ifndef PIPE_LOADER_H
#define PIPE_LOADER_H

#include "pipe/p_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pipe_screen;

enum pipe_loader_device_type {
   PIPE_LOADER_DEVICE_SOFTWARE,
   PIPE_LOADER_DEVICE_PCI,
   NUM_PIPE_LOADER_DEVICE_TYPES
};

/**
 * A device known to the pipe loader.
 */
struct pipe_loader_device {
   enum pipe_loader_device_type type;

   union {
      struct {
         int vendor_id;
         int chip_id;
      } pci;
   } u; /**< Discriminated by \a type */

   const char *driver_name;
   const struct pipe_loader_ops *ops;
};

/**
 * Get a list of known devices.
 *
 * \param devs Array that will be filled with pointers to the devices
 *             available in the system.
 * \param ndev Maximum number of devices to return.
 * \return Number of devices available in the system.
 */
int
pipe_loader_probe(struct pipe_loader_device **devs, int ndev);

/**
 * Create a pipe_screen for the specified device.
 *
 * \param dev Device the screen will be created for.
 * \param library_paths Colon-separated list of filesystem paths that
 *                      will be used to look for the pipe driver
 *                      module that handles this device.
 */
struct pipe_screen *
pipe_loader_create_screen(struct pipe_loader_device *dev,
                          const char *library_paths);

/**
 * Release resources allocated for a list of devices.
 *
 * Should be called when the specified devices are no longer in use to
 * release any resources allocated by pipe_loader_probe.
 *
 * \param devs Devices to release.
 * \param ndev Number of devices to release.
 */
void
pipe_loader_release(struct pipe_loader_device **devs, int ndev);

#ifdef HAVE_PIPE_LOADER_SW

/**
 * Get a list of known software devices.
 *
 * This function is platform-specific.
 *
 * \sa pipe_loader_probe
 */
int
pipe_loader_sw_probe(struct pipe_loader_device **devs, int ndev);

#endif

#ifdef HAVE_PIPE_LOADER_DRM

/**
 * Get a list of known DRM devices.
 *
 * This function is platform-specific.
 *
 * \sa pipe_loader_probe
 */
int
pipe_loader_drm_probe(struct pipe_loader_device **devs, int ndev);

/**
 * Initialize a DRM device in an already opened fd.
 *
 * This function is platform-specific.
 *
 * \sa pipe_loader_probe
 */
boolean
pipe_loader_drm_probe_fd(struct pipe_loader_device **dev, int fd);

#endif

#ifdef __cplusplus
}
#endif

#endif /* PIPE_LOADER_H */
