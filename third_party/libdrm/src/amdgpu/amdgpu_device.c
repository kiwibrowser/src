/*
 * Copyright 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * \file amdgpu_device.c
 *
 *  Implementation of functions for AMD GPU device
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xf86drm.h"
#include "amdgpu_drm.h"
#include "amdgpu_internal.h"
#include "util_hash_table.h"
#include "util_math.h"

#define PTR_TO_UINT(x) ((unsigned)((intptr_t)(x)))
#define UINT_TO_PTR(x) ((void *)((intptr_t)(x)))

static pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct util_hash_table *fd_tab;

static unsigned handle_hash(void *key)
{
	return PTR_TO_UINT(key);
}

static int handle_compare(void *key1, void *key2)
{
	return PTR_TO_UINT(key1) != PTR_TO_UINT(key2);
}

static unsigned fd_hash(void *key)
{
	int fd = PTR_TO_UINT(key);
	char *name = drmGetPrimaryDeviceNameFromFd(fd);
	unsigned result = 0;
	char *c;

	if (name == NULL)
		return 0;

	for (c = name; *c; ++c)
		result += *c;

	free(name);

	return result;
}

static int fd_compare(void *key1, void *key2)
{
	int fd1 = PTR_TO_UINT(key1);
	int fd2 = PTR_TO_UINT(key2);
	char *name1 = drmGetPrimaryDeviceNameFromFd(fd1);
	char *name2 = drmGetPrimaryDeviceNameFromFd(fd2);
	int result;

	if (name1 == NULL || name2 == NULL) {
		free(name1);
		free(name2);
		return 0;
	}

	result = strcmp(name1, name2);
	free(name1);
	free(name2);

	return result;
}

/**
* Get the authenticated form fd,
*
* \param   fd   - \c [in]  File descriptor for AMD GPU device
* \param   auth - \c [out] Pointer to output the fd is authenticated or not
*                          A render node fd, output auth = 0
*                          A legacy fd, get the authenticated for compatibility root
*
* \return   0 on success\n
*          >0 - AMD specific error code\n
*          <0 - Negative POSIX Error code
*/
static int amdgpu_get_auth(int fd, int *auth)
{
	int r = 0;
	drm_client_t client = {};

	if (drmGetNodeTypeFromFd(fd) == DRM_NODE_RENDER)
		*auth = 0;
	else {
		client.idx = 0;
		r = drmIoctl(fd, DRM_IOCTL_GET_CLIENT, &client);
		if (!r)
			*auth = client.auth;
	}
	return r;
}

static void amdgpu_device_free_internal(amdgpu_device_handle dev)
{
	const struct amdgpu_asic_id *id;
	amdgpu_vamgr_deinit(&dev->vamgr_32);
	amdgpu_vamgr_deinit(&dev->vamgr);
	util_hash_table_destroy(dev->bo_flink_names);
	util_hash_table_destroy(dev->bo_handles);
	pthread_mutex_destroy(&dev->bo_table_mutex);
	util_hash_table_remove(fd_tab, UINT_TO_PTR(dev->fd));
	close(dev->fd);
	if ((dev->flink_fd >= 0) && (dev->fd != dev->flink_fd))
		close(dev->flink_fd);
	if (dev->asic_ids) {
		for (id = dev->asic_ids; id->did; id++)
			free(id->marketing_name);

		free(dev->asic_ids);
	}
	free(dev);
}

/**
 * Assignment between two amdgpu_device pointers with reference counting.
 *
 * Usage:
 *    struct amdgpu_device *dst = ... , *src = ...;
 *
 *    dst = src;
 *    // No reference counting. Only use this when you need to move
 *    // a reference from one pointer to another.
 *
 *    amdgpu_device_reference(&dst, src);
 *    // Reference counters are updated. dst is decremented and src is
 *    // incremented. dst is freed if its reference counter is 0.
 */
static void amdgpu_device_reference(struct amdgpu_device **dst,
			     struct amdgpu_device *src)
{
	if (update_references(&(*dst)->refcount, &src->refcount))
		amdgpu_device_free_internal(*dst);
	*dst = src;
}

int amdgpu_device_initialize(int fd,
			     uint32_t *major_version,
			     uint32_t *minor_version,
			     amdgpu_device_handle *device_handle)
{
	struct amdgpu_device *dev;
	drmVersionPtr version;
	int r;
	int flag_auth = 0;
	int flag_authexist=0;
	uint32_t accel_working = 0;
	uint64_t start, max;

	*device_handle = NULL;

	pthread_mutex_lock(&fd_mutex);
	if (!fd_tab)
		fd_tab = util_hash_table_create(fd_hash, fd_compare);
	r = amdgpu_get_auth(fd, &flag_auth);
	if (r) {
		fprintf(stderr, "%s: amdgpu_get_auth (1) failed (%i)\n",
			__func__, r);
		pthread_mutex_unlock(&fd_mutex);
		return r;
	}
	dev = util_hash_table_get(fd_tab, UINT_TO_PTR(fd));
	if (dev) {
		r = amdgpu_get_auth(dev->fd, &flag_authexist);
		if (r) {
			fprintf(stderr, "%s: amdgpu_get_auth (2) failed (%i)\n",
				__func__, r);
			pthread_mutex_unlock(&fd_mutex);
			return r;
		}
		if ((flag_auth) && (!flag_authexist)) {
			dev->flink_fd = dup(fd);
		}
		*major_version = dev->major_version;
		*minor_version = dev->minor_version;
		amdgpu_device_reference(device_handle, dev);
		pthread_mutex_unlock(&fd_mutex);
		return 0;
	}

	dev = calloc(1, sizeof(struct amdgpu_device));
	if (!dev) {
		fprintf(stderr, "%s: calloc failed\n", __func__);
		pthread_mutex_unlock(&fd_mutex);
		return -ENOMEM;
	}

	dev->fd = -1;
	dev->flink_fd = -1;

	atomic_set(&dev->refcount, 1);

	version = drmGetVersion(fd);
	if (version->version_major != 3) {
		fprintf(stderr, "%s: DRM version is %d.%d.%d but this driver is "
			"only compatible with 3.x.x.\n",
			__func__,
			version->version_major,
			version->version_minor,
			version->version_patchlevel);
		drmFreeVersion(version);
		r = -EBADF;
		goto cleanup;
	}

	dev->fd = dup(fd);
	dev->flink_fd = dev->fd;
	dev->major_version = version->version_major;
	dev->minor_version = version->version_minor;
	drmFreeVersion(version);

	dev->bo_flink_names = util_hash_table_create(handle_hash,
						     handle_compare);
	dev->bo_handles = util_hash_table_create(handle_hash, handle_compare);
	pthread_mutex_init(&dev->bo_table_mutex, NULL);

	/* Check if acceleration is working. */
	r = amdgpu_query_info(dev, AMDGPU_INFO_ACCEL_WORKING, 4, &accel_working);
	if (r) {
		fprintf(stderr, "%s: amdgpu_query_info(ACCEL_WORKING) failed (%i)\n",
			__func__, r);
		goto cleanup;
	}
	if (!accel_working) {
		fprintf(stderr, "%s: AMDGPU_INFO_ACCEL_WORKING = 0\n", __func__);
		r = -EBADF;
		goto cleanup;
	}

	r = amdgpu_query_gpu_info_init(dev);
	if (r) {
		fprintf(stderr, "%s: amdgpu_query_gpu_info_init failed\n", __func__);
		goto cleanup;
	}

	amdgpu_vamgr_init(&dev->vamgr, dev->dev_info.virtual_address_offset,
			  dev->dev_info.virtual_address_max,
			  dev->dev_info.virtual_address_alignment);

	max = MIN2(dev->dev_info.virtual_address_max, 0xffffffff);
	start = amdgpu_vamgr_find_va(&dev->vamgr,
				     max - dev->dev_info.virtual_address_offset,
				     dev->dev_info.virtual_address_alignment, 0);
	if (start > 0xffffffff) {
		fprintf(stderr, "%s: amdgpu_vamgr_find_va failed\n", __func__);
		goto free_va; /* shouldn't get here */
	}

	amdgpu_vamgr_init(&dev->vamgr_32, start, max,
			  dev->dev_info.virtual_address_alignment);

	r = amdgpu_parse_asic_ids(&dev->asic_ids);
	if (r) {
		fprintf(stderr, "%s: Cannot parse ASIC IDs, 0x%x.",
			__func__, r);
	}

	*major_version = dev->major_version;
	*minor_version = dev->minor_version;
	*device_handle = dev;
	util_hash_table_set(fd_tab, UINT_TO_PTR(dev->fd), dev);
	pthread_mutex_unlock(&fd_mutex);

	return 0;

free_va:
	r = -ENOMEM;
	amdgpu_vamgr_free_va(&dev->vamgr, start,
			     max - dev->dev_info.virtual_address_offset);
	amdgpu_vamgr_deinit(&dev->vamgr);

cleanup:
	if (dev->fd >= 0)
		close(dev->fd);
	free(dev);
	pthread_mutex_unlock(&fd_mutex);
	return r;
}

int amdgpu_device_deinitialize(amdgpu_device_handle dev)
{
	amdgpu_device_reference(&dev, NULL);
	return 0;
}

const char *amdgpu_get_marketing_name(amdgpu_device_handle dev)
{
	const struct amdgpu_asic_id *id;

	if (!dev->asic_ids)
		return NULL;

	for (id = dev->asic_ids; id->did; id++) {
		if ((id->did == dev->info.asic_id) &&
		    (id->rid == dev->info.pci_rev_id))
			return id->marketing_name;
	}

	return NULL;
}
