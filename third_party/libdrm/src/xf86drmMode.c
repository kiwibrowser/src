/*
 * \file xf86drmMode.c
 * Header for DRM modesetting interface.
 *
 * \author Jakob Bornecrantz <wallbraker@gmail.com>
 *
 * \par Acknowledgements:
 * Feb 2007, Dave Airlie <airlied@linux.ie>
 */

/*
 * Copyright (c) 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2007-2008 Dave Airlie <airlied@linux.ie>
 * Copyright (c) 2007-2008 Jakob Bornecrantz <wallbraker@gmail.com>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

/*
 * TODO the types we are after are defined in different headers on different
 * platforms find which headers to include to get uint32_t
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#include <stdio.h>
#include <stdbool.h>

#include "xf86drmMode.h"
#include "xf86drm.h"
#include <drm.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#define memclear(s) memset(&s, 0, sizeof(s))

#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

static inline int DRM_IOCTL(int fd, unsigned long cmd, void *arg)
{
	int ret = drmIoctl(fd, cmd, arg);
	return ret < 0 ? -errno : ret;
}

/*
 * Util functions
 */

static void* drmAllocCpy(char *array, int count, int entry_size)
{
	char *r;
	int i;

	if (!count || !array || !entry_size)
		return 0;

	if (!(r = drmMalloc(count*entry_size)))
		return 0;

	for (i = 0; i < count; i++)
		memcpy(r+(entry_size*i), array+(entry_size*i), entry_size);

	return r;
}

/*
 * A couple of free functions.
 */

void drmModeFreeModeInfo(drmModeModeInfoPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);
}

void drmModeFreeResources(drmModeResPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->fbs);
	drmFree(ptr->crtcs);
	drmFree(ptr->connectors);
	drmFree(ptr->encoders);
	drmFree(ptr);
}

void drmModeFreeFB(drmModeFBPtr ptr)
{
	if (!ptr)
		return;

	/* we might add more frees later. */
	drmFree(ptr);
}

void drmModeFreeCrtc(drmModeCrtcPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);
}

void drmModeFreeConnector(drmModeConnectorPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->encoders);
	drmFree(ptr->prop_values);
	drmFree(ptr->props);
	drmFree(ptr->modes);
	drmFree(ptr);
}

void drmModeFreeEncoder(drmModeEncoderPtr ptr)
{
	drmFree(ptr);
}

/*
 * ModeSetting functions.
 */

drmModeResPtr drmModeGetResources(int fd)
{
	struct drm_mode_card_res res, counts;
	drmModeResPtr r = 0;

retry:
	memclear(res);
	if (drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
		return 0;

	counts = res;

	if (res.count_fbs) {
		res.fb_id_ptr = VOID2U64(drmMalloc(res.count_fbs*sizeof(uint32_t)));
		if (!res.fb_id_ptr)
			goto err_allocs;
	}
	if (res.count_crtcs) {
		res.crtc_id_ptr = VOID2U64(drmMalloc(res.count_crtcs*sizeof(uint32_t)));
		if (!res.crtc_id_ptr)
			goto err_allocs;
	}
	if (res.count_connectors) {
		res.connector_id_ptr = VOID2U64(drmMalloc(res.count_connectors*sizeof(uint32_t)));
		if (!res.connector_id_ptr)
			goto err_allocs;
	}
	if (res.count_encoders) {
		res.encoder_id_ptr = VOID2U64(drmMalloc(res.count_encoders*sizeof(uint32_t)));
		if (!res.encoder_id_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
		goto err_allocs;

	/* The number of available connectors and etc may have changed with a
	 * hotplug event in between the ioctls, in which case the field is
	 * silently ignored by the kernel.
	 */
	if (counts.count_fbs < res.count_fbs ||
	    counts.count_crtcs < res.count_crtcs ||
	    counts.count_connectors < res.count_connectors ||
	    counts.count_encoders < res.count_encoders)
	{
		drmFree(U642VOID(res.fb_id_ptr));
		drmFree(U642VOID(res.crtc_id_ptr));
		drmFree(U642VOID(res.connector_id_ptr));
		drmFree(U642VOID(res.encoder_id_ptr));

		goto retry;
	}

	/*
	 * return
	 */
	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->min_width     = res.min_width;
	r->max_width     = res.max_width;
	r->min_height    = res.min_height;
	r->max_height    = res.max_height;
	r->count_fbs     = res.count_fbs;
	r->count_crtcs   = res.count_crtcs;
	r->count_connectors = res.count_connectors;
	r->count_encoders = res.count_encoders;

	r->fbs        = drmAllocCpy(U642VOID(res.fb_id_ptr), res.count_fbs, sizeof(uint32_t));
	r->crtcs      = drmAllocCpy(U642VOID(res.crtc_id_ptr), res.count_crtcs, sizeof(uint32_t));
	r->connectors = drmAllocCpy(U642VOID(res.connector_id_ptr), res.count_connectors, sizeof(uint32_t));
	r->encoders   = drmAllocCpy(U642VOID(res.encoder_id_ptr), res.count_encoders, sizeof(uint32_t));
	if ((res.count_fbs && !r->fbs) ||
	    (res.count_crtcs && !r->crtcs) ||
	    (res.count_connectors && !r->connectors) ||
	    (res.count_encoders && !r->encoders))
	{
		drmFree(r->fbs);
		drmFree(r->crtcs);
		drmFree(r->connectors);
		drmFree(r->encoders);
		drmFree(r);
		r = 0;
	}

err_allocs:
	drmFree(U642VOID(res.fb_id_ptr));
	drmFree(U642VOID(res.crtc_id_ptr));
	drmFree(U642VOID(res.connector_id_ptr));
	drmFree(U642VOID(res.encoder_id_ptr));

	return r;
}

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
		 uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
		 uint32_t *buf_id)
{
	struct drm_mode_fb_cmd f;
	int ret;

	memclear(f);
	f.width  = width;
	f.height = height;
	f.pitch  = pitch;
	f.bpp    = bpp;
	f.depth  = depth;
	f.handle = bo_handle;

	if ((ret = DRM_IOCTL(fd, DRM_IOCTL_MODE_ADDFB, &f)))
		return ret;

	*buf_id = f.fb_id;
	return 0;
}

int drmModeAddFB2WithModifiers(int fd, uint32_t width, uint32_t height,
                               uint32_t pixel_format, const uint32_t bo_handles[4],
                               const uint32_t pitches[4], const uint32_t offsets[4],
                               const uint64_t modifier[4], uint32_t *buf_id, uint32_t flags)
{
	struct drm_mode_fb_cmd2 f;
	int ret;

	memclear(f);
	f.width  = width;
	f.height = height;
	f.pixel_format = pixel_format;
	f.flags = flags;
	memcpy(f.handles, bo_handles, 4 * sizeof(bo_handles[0]));
	memcpy(f.pitches, pitches, 4 * sizeof(pitches[0]));
	memcpy(f.offsets, offsets, 4 * sizeof(offsets[0]));
	if (modifier)
		memcpy(f.modifier, modifier, 4 * sizeof(modifier[0]));

	if ((ret = DRM_IOCTL(fd, DRM_IOCTL_MODE_ADDFB2, &f)))
		return ret;

	*buf_id = f.fb_id;
	return 0;
}

int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
                  uint32_t pixel_format, const uint32_t bo_handles[4],
                  const uint32_t pitches[4], const uint32_t offsets[4],
                  uint32_t *buf_id, uint32_t flags)
{
	return drmModeAddFB2WithModifiers(fd, width, height,
					  pixel_format, bo_handles,
					  pitches, offsets, NULL,
					  buf_id, flags);
}

int drmModeRmFB(int fd, uint32_t bufferId)
{
	return DRM_IOCTL(fd, DRM_IOCTL_MODE_RMFB, &bufferId);
}

drmModeFBPtr drmModeGetFB(int fd, uint32_t buf)
{
	struct drm_mode_fb_cmd info;
	drmModeFBPtr r;

	memclear(info);
	info.fb_id = buf;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETFB, &info))
		return NULL;

	if (!(r = drmMalloc(sizeof(*r))))
		return NULL;

	r->fb_id = info.fb_id;
	r->width = info.width;
	r->height = info.height;
	r->pitch = info.pitch;
	r->bpp = info.bpp;
	r->handle = info.handle;
	r->depth = info.depth;

	return r;
}

int drmModeDirtyFB(int fd, uint32_t bufferId,
		   drmModeClipPtr clips, uint32_t num_clips)
{
	struct drm_mode_fb_dirty_cmd dirty;

	memclear(dirty);
	dirty.fb_id = bufferId;
	dirty.clips_ptr = VOID2U64(clips);
	dirty.num_clips = num_clips;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_DIRTYFB, &dirty);
}

/*
 * Crtc functions
 */

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId)
{
	struct drm_mode_crtc crtc;
	drmModeCrtcPtr r;

	memclear(crtc);
	crtc.crtc_id = crtcId;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCRTC, &crtc))
		return 0;

	/*
	 * return
	 */

	if (!(r = drmMalloc(sizeof(*r))))
		return 0;

	r->crtc_id         = crtc.crtc_id;
	r->x               = crtc.x;
	r->y               = crtc.y;
	r->mode_valid      = crtc.mode_valid;
	if (r->mode_valid) {
		memcpy(&r->mode, &crtc.mode, sizeof(struct drm_mode_modeinfo));
		r->width = crtc.mode.hdisplay;
		r->height = crtc.mode.vdisplay;
	}
	r->buffer_id       = crtc.fb_id;
	r->gamma_size      = crtc.gamma_size;
	return r;
}

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId,
		   uint32_t x, uint32_t y, uint32_t *connectors, int count,
		   drmModeModeInfoPtr mode)
{
	struct drm_mode_crtc crtc;

	memclear(crtc);
	crtc.x             = x;
	crtc.y             = y;
	crtc.crtc_id       = crtcId;
	crtc.fb_id         = bufferId;
	crtc.set_connectors_ptr = VOID2U64(connectors);
	crtc.count_connectors = count;
	if (mode) {
	  memcpy(&crtc.mode, mode, sizeof(struct drm_mode_modeinfo));
	  crtc.mode_valid = 1;
	}

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_SETCRTC, &crtc);
}

/*
 * Cursor manipulation
 */

int drmModeSetCursor(int fd, uint32_t crtcId, uint32_t bo_handle, uint32_t width, uint32_t height)
{
	struct drm_mode_cursor arg;

	memclear(arg);
	arg.flags = DRM_MODE_CURSOR_BO;
	arg.crtc_id = crtcId;
	arg.width = width;
	arg.height = height;
	arg.handle = bo_handle;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_CURSOR, &arg);
}

int drmModeSetCursor2(int fd, uint32_t crtcId, uint32_t bo_handle, uint32_t width, uint32_t height, int32_t hot_x, int32_t hot_y)
{
	struct drm_mode_cursor2 arg;

	memclear(arg);
	arg.flags = DRM_MODE_CURSOR_BO;
	arg.crtc_id = crtcId;
	arg.width = width;
	arg.height = height;
	arg.handle = bo_handle;
	arg.hot_x = hot_x;
	arg.hot_y = hot_y;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_CURSOR2, &arg);
}

int drmModeMoveCursor(int fd, uint32_t crtcId, int x, int y)
{
	struct drm_mode_cursor arg;

	memclear(arg);
	arg.flags = DRM_MODE_CURSOR_MOVE;
	arg.crtc_id = crtcId;
	arg.x = x;
	arg.y = y;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_CURSOR, &arg);
}

/*
 * Encoder get
 */
drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id)
{
	struct drm_mode_get_encoder enc;
	drmModeEncoderPtr r = NULL;

	memclear(enc);
	enc.encoder_id = encoder_id;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETENCODER, &enc))
		return 0;

	if (!(r = drmMalloc(sizeof(*r))))
		return 0;

	r->encoder_id = enc.encoder_id;
	r->crtc_id = enc.crtc_id;
	r->encoder_type = enc.encoder_type;
	r->possible_crtcs = enc.possible_crtcs;
	r->possible_clones = enc.possible_clones;

	return r;
}

/*
 * Connector manipulation
 */
static drmModeConnectorPtr
_drmModeGetConnector(int fd, uint32_t connector_id, int probe)
{
	struct drm_mode_get_connector conn, counts;
	drmModeConnectorPtr r = NULL;
	struct drm_mode_modeinfo stack_mode;

	memclear(conn);
	conn.connector_id = connector_id;
	if (!probe) {
		conn.count_modes = 1;
		conn.modes_ptr = VOID2U64(&stack_mode);
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
		return 0;

retry:
	counts = conn;

	if (conn.count_props) {
		conn.props_ptr = VOID2U64(drmMalloc(conn.count_props*sizeof(uint32_t)));
		if (!conn.props_ptr)
			goto err_allocs;
		conn.prop_values_ptr = VOID2U64(drmMalloc(conn.count_props*sizeof(uint64_t)));
		if (!conn.prop_values_ptr)
			goto err_allocs;
	}

	if (conn.count_modes) {
		conn.modes_ptr = VOID2U64(drmMalloc(conn.count_modes*sizeof(struct drm_mode_modeinfo)));
		if (!conn.modes_ptr)
			goto err_allocs;
	} else {
		conn.count_modes = 1;
		conn.modes_ptr = VOID2U64(&stack_mode);
	}

	if (conn.count_encoders) {
		conn.encoders_ptr = VOID2U64(drmMalloc(conn.count_encoders*sizeof(uint32_t)));
		if (!conn.encoders_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
		goto err_allocs;

	/* The number of available connectors and etc may have changed with a
	 * hotplug event in between the ioctls, in which case the field is
	 * silently ignored by the kernel.
	 */
	if (counts.count_props < conn.count_props ||
	    counts.count_modes < conn.count_modes ||
	    counts.count_encoders < conn.count_encoders) {
		drmFree(U642VOID(conn.props_ptr));
		drmFree(U642VOID(conn.prop_values_ptr));
		if (U642VOID(conn.modes_ptr) != &stack_mode)
			drmFree(U642VOID(conn.modes_ptr));
		drmFree(U642VOID(conn.encoders_ptr));

		goto retry;
	}

	if(!(r = drmMalloc(sizeof(*r)))) {
		goto err_allocs;
	}

	r->connector_id = conn.connector_id;
	r->encoder_id = conn.encoder_id;
	r->connection   = conn.connection;
	r->mmWidth      = conn.mm_width;
	r->mmHeight     = conn.mm_height;
	/* convert subpixel from kernel to userspace */
	r->subpixel     = conn.subpixel + 1;
	r->count_modes  = conn.count_modes;
	r->count_props  = conn.count_props;
	r->props        = drmAllocCpy(U642VOID(conn.props_ptr), conn.count_props, sizeof(uint32_t));
	r->prop_values  = drmAllocCpy(U642VOID(conn.prop_values_ptr), conn.count_props, sizeof(uint64_t));
	r->modes        = drmAllocCpy(U642VOID(conn.modes_ptr), conn.count_modes, sizeof(struct drm_mode_modeinfo));
	r->count_encoders = conn.count_encoders;
	r->encoders     = drmAllocCpy(U642VOID(conn.encoders_ptr), conn.count_encoders, sizeof(uint32_t));
	r->connector_type  = conn.connector_type;
	r->connector_type_id = conn.connector_type_id;

	if ((r->count_props && !r->props) ||
	    (r->count_props && !r->prop_values) ||
	    (r->count_modes && !r->modes) ||
	    (r->count_encoders && !r->encoders)) {
		drmFree(r->props);
		drmFree(r->prop_values);
		drmFree(r->modes);
		drmFree(r->encoders);
		drmFree(r);
		r = 0;
	}

err_allocs:
	drmFree(U642VOID(conn.prop_values_ptr));
	drmFree(U642VOID(conn.props_ptr));
	if (U642VOID(conn.modes_ptr) != &stack_mode)
		drmFree(U642VOID(conn.modes_ptr));
	drmFree(U642VOID(conn.encoders_ptr));

	return r;
}

drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connector_id)
{
	return _drmModeGetConnector(fd, connector_id, 1);
}

drmModeConnectorPtr drmModeGetConnectorCurrent(int fd, uint32_t connector_id)
{
	return _drmModeGetConnector(fd, connector_id, 0);
}

int drmModeAttachMode(int fd, uint32_t connector_id, drmModeModeInfoPtr mode_info)
{
	struct drm_mode_mode_cmd res;

	memclear(res);
	memcpy(&res.mode, mode_info, sizeof(struct drm_mode_modeinfo));
	res.connector_id = connector_id;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_ATTACHMODE, &res);
}

int drmModeDetachMode(int fd, uint32_t connector_id, drmModeModeInfoPtr mode_info)
{
	struct drm_mode_mode_cmd res;

	memclear(res);
	memcpy(&res.mode, mode_info, sizeof(struct drm_mode_modeinfo));
	res.connector_id = connector_id;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_DETACHMODE, &res);
}

drmModePropertyPtr drmModeGetProperty(int fd, uint32_t property_id)
{
	struct drm_mode_get_property prop;
	drmModePropertyPtr r;

	memclear(prop);
	prop.prop_id = property_id;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPERTY, &prop))
		return 0;

	if (prop.count_values)
		prop.values_ptr = VOID2U64(drmMalloc(prop.count_values * sizeof(uint64_t)));

	if (prop.count_enum_blobs && (prop.flags & (DRM_MODE_PROP_ENUM | DRM_MODE_PROP_BITMASK)))
		prop.enum_blob_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(struct drm_mode_property_enum)));

	if (prop.count_enum_blobs && (prop.flags & DRM_MODE_PROP_BLOB)) {
		prop.values_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(uint32_t)));
		prop.enum_blob_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(uint32_t)));
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPERTY, &prop)) {
		r = NULL;
		goto err_allocs;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		return NULL;

	r->prop_id = prop.prop_id;
	r->count_values = prop.count_values;

	r->flags = prop.flags;
	if (prop.count_values)
		r->values = drmAllocCpy(U642VOID(prop.values_ptr), prop.count_values, sizeof(uint64_t));
	if (prop.flags & (DRM_MODE_PROP_ENUM | DRM_MODE_PROP_BITMASK)) {
		r->count_enums = prop.count_enum_blobs;
		r->enums = drmAllocCpy(U642VOID(prop.enum_blob_ptr), prop.count_enum_blobs, sizeof(struct drm_mode_property_enum));
	} else if (prop.flags & DRM_MODE_PROP_BLOB) {
		r->values = drmAllocCpy(U642VOID(prop.values_ptr), prop.count_enum_blobs, sizeof(uint32_t));
		r->blob_ids = drmAllocCpy(U642VOID(prop.enum_blob_ptr), prop.count_enum_blobs, sizeof(uint32_t));
		r->count_blobs = prop.count_enum_blobs;
	}
	strncpy(r->name, prop.name, DRM_PROP_NAME_LEN);
	r->name[DRM_PROP_NAME_LEN-1] = 0;

err_allocs:
	drmFree(U642VOID(prop.values_ptr));
	drmFree(U642VOID(prop.enum_blob_ptr));

	return r;
}

void drmModeFreeProperty(drmModePropertyPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->values);
	drmFree(ptr->enums);
	drmFree(ptr);
}

drmModePropertyBlobPtr drmModeGetPropertyBlob(int fd, uint32_t blob_id)
{
	struct drm_mode_get_blob blob;
	drmModePropertyBlobPtr r;

	memclear(blob);
	blob.blob_id = blob_id;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPBLOB, &blob))
		return NULL;

	if (blob.length)
		blob.data = VOID2U64(drmMalloc(blob.length));

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPBLOB, &blob)) {
		r = NULL;
		goto err_allocs;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->id = blob.blob_id;
	r->length = blob.length;
	r->data = drmAllocCpy(U642VOID(blob.data), 1, blob.length);

err_allocs:
	drmFree(U642VOID(blob.data));
	return r;
}

void drmModeFreePropertyBlob(drmModePropertyBlobPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->data);
	drmFree(ptr);
}

int drmModeConnectorSetProperty(int fd, uint32_t connector_id, uint32_t property_id,
			     uint64_t value)
{
	struct drm_mode_connector_set_property osp;

	memclear(osp);
	osp.connector_id = connector_id;
	osp.prop_id = property_id;
	osp.value = value;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_SETPROPERTY, &osp);
}

/*
 * checks if a modesetting capable driver has attached to the pci id
 * returns 0 if modesetting supported.
 *  -EINVAL or invalid bus id
 *  -ENOSYS if no modesetting support
*/
int drmCheckModesettingSupported(const char *busid)
{
#if defined (__linux__)
	char pci_dev_dir[1024];
	int domain, bus, dev, func;
	DIR *sysdir;
	struct dirent *dent;
	int found = 0, ret;

	ret = sscanf(busid, "pci:%04x:%02x:%02x.%d", &domain, &bus, &dev, &func);
	if (ret != 4)
		return -EINVAL;

	sprintf(pci_dev_dir, "/sys/bus/pci/devices/%04x:%02x:%02x.%d/drm",
		domain, bus, dev, func);

	sysdir = opendir(pci_dev_dir);
	if (sysdir) {
		dent = readdir(sysdir);
		while (dent) {
			if (!strncmp(dent->d_name, "controlD", 8)) {
				found = 1;
				break;
			}

			dent = readdir(sysdir);
		}
		closedir(sysdir);
		if (found)
			return 0;
	}

	sprintf(pci_dev_dir, "/sys/bus/pci/devices/%04x:%02x:%02x.%d/",
		domain, bus, dev, func);

	sysdir = opendir(pci_dev_dir);
	if (!sysdir)
		return -EINVAL;

	dent = readdir(sysdir);
	while (dent) {
		if (!strncmp(dent->d_name, "drm:controlD", 12)) {
			found = 1;
			break;
		}

		dent = readdir(sysdir);
	}

	closedir(sysdir);
	if (found)
		return 0;
#elif defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
	char kbusid[1024], sbusid[1024];
	char oid[128];
	int domain, bus, dev, func;
	int i, modesetting, ret;
	size_t len;

	ret = sscanf(busid, "pci:%04x:%02x:%02x.%d", &domain, &bus, &dev,
	    &func);
	if (ret != 4)
		return -EINVAL;
	snprintf(kbusid, sizeof(kbusid), "pci:%04x:%02x:%02x.%d", domain, bus,
	    dev, func);

	/* How many GPUs do we expect in the machine ? */
	for (i = 0; i < 16; i++) {
		snprintf(oid, sizeof(oid), "hw.dri.%d.busid", i);
		len = sizeof(sbusid);
		ret = sysctlbyname(oid, sbusid, &len, NULL, 0);
		if (ret == -1) {
			if (errno == ENOENT)
				continue;
			return -EINVAL;
		}
		if (strcmp(sbusid, kbusid) != 0)
			continue;
		snprintf(oid, sizeof(oid), "hw.dri.%d.modesetting", i);
		len = sizeof(modesetting);
		ret = sysctlbyname(oid, &modesetting, &len, NULL, 0);
		if (ret == -1 || len != sizeof(modesetting))
			return -EINVAL;
		return (modesetting ? 0 : -ENOSYS);
	}
#elif defined(__DragonFly__)
	return 0;
#endif
#ifdef __OpenBSD__
	int	fd;
	struct drm_mode_card_res res;
	drmModeResPtr r = 0;

	if ((fd = drmOpen(NULL, busid)) < 0)
		return -EINVAL;

	memset(&res, 0, sizeof(struct drm_mode_card_res));

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res)) {
		drmClose(fd);
		return -errno;
	}

	drmClose(fd);
	return 0;
#endif
	return -ENOSYS;
}

int drmModeCrtcGetGamma(int fd, uint32_t crtc_id, uint32_t size,
			uint16_t *red, uint16_t *green, uint16_t *blue)
{
	struct drm_mode_crtc_lut l;

	memclear(l);
	l.crtc_id = crtc_id;
	l.gamma_size = size;
	l.red = VOID2U64(red);
	l.green = VOID2U64(green);
	l.blue = VOID2U64(blue);

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_GETGAMMA, &l);
}

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size,
			uint16_t *red, uint16_t *green, uint16_t *blue)
{
	struct drm_mode_crtc_lut l;

	memclear(l);
	l.crtc_id = crtc_id;
	l.gamma_size = size;
	l.red = VOID2U64(red);
	l.green = VOID2U64(green);
	l.blue = VOID2U64(blue);

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_SETGAMMA, &l);
}

int drmHandleEvent(int fd, drmEventContextPtr evctx)
{
	char buffer[1024];
	int len, i;
	struct drm_event *e;
	struct drm_event_vblank *vblank;
	void *user_data;

	/* The DRM read semantics guarantees that we always get only
	 * complete events. */

	len = read(fd, buffer, sizeof buffer);
	if (len == 0)
		return 0;
	if (len < (int)sizeof *e)
		return -1;

	i = 0;
	while (i < len) {
		e = (struct drm_event *)(buffer + i);
		switch (e->type) {
		case DRM_EVENT_VBLANK:
			if (evctx->version < 1 ||
			    evctx->vblank_handler == NULL)
				break;
			vblank = (struct drm_event_vblank *) e;
			evctx->vblank_handler(fd,
					      vblank->sequence,
					      vblank->tv_sec,
					      vblank->tv_usec,
					      U642VOID (vblank->user_data));
			break;
		case DRM_EVENT_FLIP_COMPLETE:
			vblank = (struct drm_event_vblank *) e;
			user_data = U642VOID (vblank->user_data);

			if (evctx->version >= 3 && evctx->page_flip_handler2)
				evctx->page_flip_handler2(fd,
							 vblank->sequence,
							 vblank->tv_sec,
							 vblank->tv_usec,
							 vblank->crtc_id,
							 user_data);
			else if (evctx->version >= 2 && evctx->page_flip_handler)
				evctx->page_flip_handler(fd,
							 vblank->sequence,
							 vblank->tv_sec,
							 vblank->tv_usec,
							 user_data);
			break;
		default:
			break;
		}
		i += e->length;
	}

	return 0;
}

int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id,
		    uint32_t flags, void *user_data)
{
	struct drm_mode_crtc_page_flip flip;

	memclear(flip);
	flip.fb_id = fb_id;
	flip.crtc_id = crtc_id;
	flip.user_data = VOID2U64(user_data);
	flip.flags = flags;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_PAGE_FLIP, &flip);
}

int drmModePageFlipTarget(int fd, uint32_t crtc_id, uint32_t fb_id,
			  uint32_t flags, void *user_data,
			  uint32_t target_vblank)
{
	struct drm_mode_crtc_page_flip_target flip_target;

	memclear(flip_target);
	flip_target.fb_id = fb_id;
	flip_target.crtc_id = crtc_id;
	flip_target.user_data = VOID2U64(user_data);
	flip_target.flags = flags;
	flip_target.sequence = target_vblank;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_PAGE_FLIP, &flip_target);
}

int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id,
		    uint32_t fb_id, uint32_t flags,
		    int32_t crtc_x, int32_t crtc_y,
		    uint32_t crtc_w, uint32_t crtc_h,
		    uint32_t src_x, uint32_t src_y,
		    uint32_t src_w, uint32_t src_h)
{
	struct drm_mode_set_plane s;

	memclear(s);
	s.plane_id = plane_id;
	s.crtc_id = crtc_id;
	s.fb_id = fb_id;
	s.flags = flags;
	s.crtc_x = crtc_x;
	s.crtc_y = crtc_y;
	s.crtc_w = crtc_w;
	s.crtc_h = crtc_h;
	s.src_x = src_x;
	s.src_y = src_y;
	s.src_w = src_w;
	s.src_h = src_h;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_SETPLANE, &s);
}

drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id)
{
	struct drm_mode_get_plane ovr, counts;
	drmModePlanePtr r = 0;

retry:
	memclear(ovr);
	ovr.plane_id = plane_id;
	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANE, &ovr))
		return 0;

	counts = ovr;

	if (ovr.count_format_types) {
		ovr.format_type_ptr = VOID2U64(drmMalloc(ovr.count_format_types *
							 sizeof(uint32_t)));
		if (!ovr.format_type_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANE, &ovr))
		goto err_allocs;

	if (counts.count_format_types < ovr.count_format_types) {
		drmFree(U642VOID(ovr.format_type_ptr));
		goto retry;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->count_formats = ovr.count_format_types;
	r->plane_id = ovr.plane_id;
	r->crtc_id = ovr.crtc_id;
	r->fb_id = ovr.fb_id;
	r->possible_crtcs = ovr.possible_crtcs;
	r->gamma_size = ovr.gamma_size;
	r->formats = drmAllocCpy(U642VOID(ovr.format_type_ptr),
				 ovr.count_format_types, sizeof(uint32_t));
	if (ovr.count_format_types && !r->formats) {
		drmFree(r->formats);
		drmFree(r);
		r = 0;
	}

err_allocs:
	drmFree(U642VOID(ovr.format_type_ptr));

	return r;
}

void drmModeFreePlane(drmModePlanePtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->formats);
	drmFree(ptr);
}

drmModePlaneResPtr drmModeGetPlaneResources(int fd)
{
	struct drm_mode_get_plane_res res, counts;
	drmModePlaneResPtr r = 0;

retry:
	memclear(res);
	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANERESOURCES, &res))
		return 0;

	counts = res;

	if (res.count_planes) {
		res.plane_id_ptr = VOID2U64(drmMalloc(res.count_planes *
							sizeof(uint32_t)));
		if (!res.plane_id_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANERESOURCES, &res))
		goto err_allocs;

	if (counts.count_planes < res.count_planes) {
		drmFree(U642VOID(res.plane_id_ptr));
		goto retry;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->count_planes = res.count_planes;
	r->planes = drmAllocCpy(U642VOID(res.plane_id_ptr),
				  res.count_planes, sizeof(uint32_t));
	if (res.count_planes && !r->planes) {
		drmFree(r->planes);
		drmFree(r);
		r = 0;
	}

err_allocs:
	drmFree(U642VOID(res.plane_id_ptr));

	return r;
}

void drmModeFreePlaneResources(drmModePlaneResPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->planes);
	drmFree(ptr);
}

drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd,
						      uint32_t object_id,
						      uint32_t object_type)
{
	struct drm_mode_obj_get_properties properties;
	drmModeObjectPropertiesPtr ret = NULL;
	uint32_t count;

retry:
	memclear(properties);
	properties.obj_id = object_id;
	properties.obj_type = object_type;

	if (drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &properties))
		return 0;

	count = properties.count_props;

	if (count) {
		properties.props_ptr = VOID2U64(drmMalloc(count *
							  sizeof(uint32_t)));
		if (!properties.props_ptr)
			goto err_allocs;
		properties.prop_values_ptr = VOID2U64(drmMalloc(count *
						      sizeof(uint64_t)));
		if (!properties.prop_values_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &properties))
		goto err_allocs;

	if (count < properties.count_props) {
		drmFree(U642VOID(properties.props_ptr));
		drmFree(U642VOID(properties.prop_values_ptr));
		goto retry;
	}
	count = properties.count_props;

	ret = drmMalloc(sizeof(*ret));
	if (!ret)
		goto err_allocs;

	ret->count_props = count;
	ret->props = drmAllocCpy(U642VOID(properties.props_ptr),
				 count, sizeof(uint32_t));
	ret->prop_values = drmAllocCpy(U642VOID(properties.prop_values_ptr),
				       count, sizeof(uint64_t));
	if (ret->count_props && (!ret->props || !ret->prop_values)) {
		drmFree(ret->props);
		drmFree(ret->prop_values);
		drmFree(ret);
		ret = NULL;
	}

err_allocs:
	drmFree(U642VOID(properties.props_ptr));
	drmFree(U642VOID(properties.prop_values_ptr));
	return ret;
}

void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr)
{
	if (!ptr)
		return;
	drmFree(ptr->props);
	drmFree(ptr->prop_values);
	drmFree(ptr);
}

int drmModeObjectSetProperty(int fd, uint32_t object_id, uint32_t object_type,
			     uint32_t property_id, uint64_t value)
{
	struct drm_mode_obj_set_property prop;

	memclear(prop);
	prop.value = value;
	prop.prop_id = property_id;
	prop.obj_id = object_id;
	prop.obj_type = object_type;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_OBJ_SETPROPERTY, &prop);
}

typedef struct _drmModeAtomicReqItem drmModeAtomicReqItem, *drmModeAtomicReqItemPtr;

struct _drmModeAtomicReqItem {
	uint32_t object_id;
	uint32_t property_id;
	uint64_t value;
};

struct _drmModeAtomicReq {
	uint32_t cursor;
	uint32_t size_items;
	drmModeAtomicReqItemPtr items;
};

drmModeAtomicReqPtr drmModeAtomicAlloc(void)
{
	drmModeAtomicReqPtr req;

	req = drmMalloc(sizeof *req);
	if (!req)
		return NULL;

	req->items = NULL;
	req->cursor = 0;
	req->size_items = 0;

	return req;
}

drmModeAtomicReqPtr drmModeAtomicDuplicate(drmModeAtomicReqPtr old)
{
	drmModeAtomicReqPtr new;

	if (!old)
		return NULL;

	new = drmMalloc(sizeof *new);
	if (!new)
		return NULL;

	new->cursor = old->cursor;
	new->size_items = old->size_items;

	if (old->size_items) {
		new->items = drmMalloc(old->size_items * sizeof(*new->items));
		if (!new->items) {
			free(new);
			return NULL;
		}
		memcpy(new->items, old->items,
		       old->size_items * sizeof(*new->items));
	} else {
		new->items = NULL;
	}

	return new;
}

int drmModeAtomicMerge(drmModeAtomicReqPtr base, drmModeAtomicReqPtr augment)
{
	if (!base)
		return -EINVAL;

	if (!augment || augment->cursor == 0)
		return 0;

	if (base->cursor + augment->cursor >= base->size_items) {
		drmModeAtomicReqItemPtr new;
		int saved_size = base->size_items;

		base->size_items = base->cursor + augment->cursor;
		new = realloc(base->items,
			      base->size_items * sizeof(*base->items));
		if (!new) {
			base->size_items = saved_size;
			return -ENOMEM;
		}
		base->items = new;
	}

	memcpy(&base->items[base->cursor], augment->items,
	       augment->cursor * sizeof(*augment->items));
	base->cursor += augment->cursor;

	return 0;
}

int drmModeAtomicGetCursor(drmModeAtomicReqPtr req)
{
	if (!req)
		return -EINVAL;
	return req->cursor;
}

void drmModeAtomicSetCursor(drmModeAtomicReqPtr req, int cursor)
{
	if (req)
		req->cursor = cursor;
}

int drmModeAtomicAddProperty(drmModeAtomicReqPtr req,
			     uint32_t object_id,
			     uint32_t property_id,
			     uint64_t value)
{
	if (!req)
		return -EINVAL;

	if (req->cursor >= req->size_items) {
		drmModeAtomicReqItemPtr new;

		req->size_items += 16;
		new = realloc(req->items, req->size_items * sizeof(*req->items));
		if (!new) {
			req->size_items -= 16;
			return -ENOMEM;
		}
		req->items = new;
	}

	req->items[req->cursor].object_id = object_id;
	req->items[req->cursor].property_id = property_id;
	req->items[req->cursor].value = value;
	req->cursor++;

	return req->cursor;
}

void drmModeAtomicFree(drmModeAtomicReqPtr req)
{
	if (!req)
		return;

	if (req->items)
		drmFree(req->items);
	drmFree(req);
}

static int sort_req_list(const void *misc, const void *other)
{
	const drmModeAtomicReqItem *first = misc;
	const drmModeAtomicReqItem *second = other;

	if (first->object_id < second->object_id)
		return -1;
	else if (first->object_id > second->object_id)
		return 1;
	else
		return second->property_id - first->property_id;
}

int drmModeAtomicCommit(int fd, drmModeAtomicReqPtr req, uint32_t flags,
			void *user_data)
{
	drmModeAtomicReqPtr sorted;
	struct drm_mode_atomic atomic;
	uint32_t *objs_ptr = NULL;
	uint32_t *count_props_ptr = NULL;
	uint32_t *props_ptr = NULL;
	uint64_t *prop_values_ptr = NULL;
	uint32_t last_obj_id = 0;
	uint32_t i;
	int obj_idx = -1;
	int ret = -1;

	if (!req)
		return -EINVAL;

	if (req->cursor == 0)
		return 0;

	sorted = drmModeAtomicDuplicate(req);
	if (sorted == NULL)
		return -ENOMEM;

	memclear(atomic);

	/* Sort the list by object ID, then by property ID. */
	qsort(sorted->items, sorted->cursor, sizeof(*sorted->items),
	      sort_req_list);

	/* Now the list is sorted, eliminate duplicate property sets. */
	for (i = 0; i < sorted->cursor; i++) {
		if (sorted->items[i].object_id != last_obj_id) {
			atomic.count_objs++;
			last_obj_id = sorted->items[i].object_id;
		}

		if (i == sorted->cursor - 1)
			continue;

		if (sorted->items[i].object_id != sorted->items[i + 1].object_id ||
		    sorted->items[i].property_id != sorted->items[i + 1].property_id)
			continue;

		memmove(&sorted->items[i], &sorted->items[i + 1],
			(sorted->cursor - i - 1) * sizeof(*sorted->items));
		sorted->cursor--;
	}

	objs_ptr = drmMalloc(atomic.count_objs * sizeof objs_ptr[0]);
	if (!objs_ptr) {
		errno = ENOMEM;
		goto out;
	}

	count_props_ptr = drmMalloc(atomic.count_objs * sizeof count_props_ptr[0]);
	if (!count_props_ptr) {
		errno = ENOMEM;
		goto out;
	}

	props_ptr = drmMalloc(sorted->cursor * sizeof props_ptr[0]);
	if (!props_ptr) {
		errno = ENOMEM;
		goto out;
	}

	prop_values_ptr = drmMalloc(sorted->cursor * sizeof prop_values_ptr[0]);
	if (!prop_values_ptr) {
		errno = ENOMEM;
		goto out;
	}

	for (i = 0, last_obj_id = 0; i < sorted->cursor; i++) {
		if (sorted->items[i].object_id != last_obj_id) {
			obj_idx++;
			objs_ptr[obj_idx] = sorted->items[i].object_id;
			last_obj_id = objs_ptr[obj_idx];
		}

		count_props_ptr[obj_idx]++;
		props_ptr[i] = sorted->items[i].property_id;
		prop_values_ptr[i] = sorted->items[i].value;

	}

	atomic.flags = flags;
	atomic.objs_ptr = VOID2U64(objs_ptr);
	atomic.count_props_ptr = VOID2U64(count_props_ptr);
	atomic.props_ptr = VOID2U64(props_ptr);
	atomic.prop_values_ptr = VOID2U64(prop_values_ptr);
	atomic.user_data = VOID2U64(user_data);

	ret = DRM_IOCTL(fd, DRM_IOCTL_MODE_ATOMIC, &atomic);

out:
	drmFree(objs_ptr);
	drmFree(count_props_ptr);
	drmFree(props_ptr);
	drmFree(prop_values_ptr);
	drmModeAtomicFree(sorted);

	return ret;
}

int
drmModeCreatePropertyBlob(int fd, const void *data, size_t length, uint32_t *id)
{
	struct drm_mode_create_blob create;
	int ret;

	if (length >= 0xffffffff)
		return -ERANGE;

	memclear(create);

	create.length = length;
	create.data = (uintptr_t) data;
	create.blob_id = 0;
	*id = 0;

	ret = DRM_IOCTL(fd, DRM_IOCTL_MODE_CREATEPROPBLOB, &create);
	if (ret != 0)
		return ret;

	*id = create.blob_id;
	return 0;
}

int
drmModeDestroyPropertyBlob(int fd, uint32_t id)
{
	struct drm_mode_destroy_blob destroy;

	memclear(destroy);
	destroy.blob_id = id;
	return DRM_IOCTL(fd, DRM_IOCTL_MODE_DESTROYPROPBLOB, &destroy);
}
