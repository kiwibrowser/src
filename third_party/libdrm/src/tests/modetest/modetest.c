/*
 * DRM based mode setting test program
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
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
 */

/*
 * This fairly simple test program dumps output in a similar format to the
 * "xrandr" tool everyone knows & loves.  It's necessarily slightly different
 * since the kernel separates outputs into encoder and connector structures,
 * each with their own unique ID.  The program also allows test testing of the
 * memory management and mode setting APIs by allowing the user to specify a
 * connector and mode to use for mode setting.  If all works as expected, a
 * blue background should be painted on the monitor attached to the specified
 * connector after the selected mode is set.
 *
 * TODO: use cairo to write the mode info on the selected output once
 *       the mode has been programmed, along with possible test patterns.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "xf86drm.h"
#include "xf86drmMode.h"
#include "drm_fourcc.h"

#include "util/common.h"
#include "util/format.h"
#include "util/kms.h"
#include "util/pattern.h"

#include "buffers.h"
#include "cursor.h"

struct crtc {
	drmModeCrtc *crtc;
	drmModeObjectProperties *props;
	drmModePropertyRes **props_info;
	drmModeModeInfo *mode;
};

struct encoder {
	drmModeEncoder *encoder;
};

struct connector {
	drmModeConnector *connector;
	drmModeObjectProperties *props;
	drmModePropertyRes **props_info;
	char *name;
};

struct fb {
	drmModeFB *fb;
};

struct plane {
	drmModePlane *plane;
	drmModeObjectProperties *props;
	drmModePropertyRes **props_info;
};

struct resources {
	drmModeRes *res;
	drmModePlaneRes *plane_res;

	struct crtc *crtcs;
	struct encoder *encoders;
	struct connector *connectors;
	struct fb *fbs;
	struct plane *planes;
};

struct device {
	int fd;

	struct resources *resources;

	struct {
		unsigned int width;
		unsigned int height;

		unsigned int fb_id;
		struct bo *bo;
		struct bo *cursor_bo;
	} mode;
};

static inline int64_t U642I64(uint64_t val)
{
	return (int64_t)*((int64_t *)&val);
}

#define bit_name_fn(res)					\
const char * res##_str(int type) {				\
	unsigned int i;						\
	const char *sep = "";					\
	for (i = 0; i < ARRAY_SIZE(res##_names); i++) {		\
		if (type & (1 << i)) {				\
			printf("%s%s", sep, res##_names[i]);	\
			sep = ", ";				\
		}						\
	}							\
	return NULL;						\
}

static const char *mode_type_names[] = {
	"builtin",
	"clock_c",
	"crtc_c",
	"preferred",
	"default",
	"userdef",
	"driver",
};

static bit_name_fn(mode_type)

static const char *mode_flag_names[] = {
	"phsync",
	"nhsync",
	"pvsync",
	"nvsync",
	"interlace",
	"dblscan",
	"csync",
	"pcsync",
	"ncsync",
	"hskew",
	"bcast",
	"pixmux",
	"dblclk",
	"clkdiv2"
};

static bit_name_fn(mode_flag)

static void dump_fourcc(uint32_t fourcc)
{
	printf(" %c%c%c%c",
		fourcc,
		fourcc >> 8,
		fourcc >> 16,
		fourcc >> 24);
}

static void dump_encoders(struct device *dev)
{
	drmModeEncoder *encoder;
	int i;

	printf("Encoders:\n");
	printf("id\tcrtc\ttype\tpossible crtcs\tpossible clones\t\n");
	for (i = 0; i < dev->resources->res->count_encoders; i++) {
		encoder = dev->resources->encoders[i].encoder;
		if (!encoder)
			continue;

		printf("%d\t%d\t%s\t0x%08x\t0x%08x\n",
		       encoder->encoder_id,
		       encoder->crtc_id,
		       util_lookup_encoder_type_name(encoder->encoder_type),
		       encoder->possible_crtcs,
		       encoder->possible_clones);
	}
	printf("\n");
}

static void dump_mode(drmModeModeInfo *mode)
{
	printf("  %s %d %d %d %d %d %d %d %d %d %d",
	       mode->name,
	       mode->vrefresh,
	       mode->hdisplay,
	       mode->hsync_start,
	       mode->hsync_end,
	       mode->htotal,
	       mode->vdisplay,
	       mode->vsync_start,
	       mode->vsync_end,
	       mode->vtotal,
	       mode->clock);

	printf(" flags: ");
	mode_flag_str(mode->flags);
	printf("; type: ");
	mode_type_str(mode->type);
	printf("\n");
}

static void dump_blob(struct device *dev, uint32_t blob_id)
{
	uint32_t i;
	unsigned char *blob_data;
	drmModePropertyBlobPtr blob;

	blob = drmModeGetPropertyBlob(dev->fd, blob_id);
	if (!blob) {
		printf("\n");
		return;
	}

	blob_data = blob->data;

	for (i = 0; i < blob->length; i++) {
		if (i % 16 == 0)
			printf("\n\t\t\t");
		printf("%.2hhx", blob_data[i]);
	}
	printf("\n");

	drmModeFreePropertyBlob(blob);
}

static const char *modifier_to_string(uint64_t modifier)
{
	switch (modifier) {
	case DRM_FORMAT_MOD_INVALID:
		return "INVALID";
	case DRM_FORMAT_MOD_LINEAR:
		return "LINEAR";
	case I915_FORMAT_MOD_X_TILED:
		return "X_TILED";
	case I915_FORMAT_MOD_Y_TILED:
		return "Y_TILED";
	case I915_FORMAT_MOD_Yf_TILED:
		return "Yf_TILED";
	case I915_FORMAT_MOD_Y_TILED_CCS:
		return "Y_TILED_CCS";
	case I915_FORMAT_MOD_Yf_TILED_CCS:
		return "Yf_TILED_CCS";
	case DRM_FORMAT_MOD_SAMSUNG_64_32_TILE:
		return "SAMSUNG_64_32_TILE";
	case DRM_FORMAT_MOD_VIVANTE_TILED:
		return "VIVANTE_TILED";
	case DRM_FORMAT_MOD_VIVANTE_SUPER_TILED:
		return "VIVANTE_SUPER_TILED";
	case DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED:
		return "VIVANTE_SPLIT_TILED";
	case DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED:
		return "VIVANTE_SPLIT_SUPER_TILED";
	case NV_FORMAT_MOD_TEGRA_TILED:
		return "MOD_TEGRA_TILED";
	case NV_FORMAT_MOD_TEGRA_16BX2_BLOCK(0):
		return "MOD_TEGRA_16BX2_BLOCK(0)";
	case NV_FORMAT_MOD_TEGRA_16BX2_BLOCK(1):
		return "MOD_TEGRA_16BX2_BLOCK(1)";
	case NV_FORMAT_MOD_TEGRA_16BX2_BLOCK(2):
		return "MOD_TEGRA_16BX2_BLOCK(2)";
	case NV_FORMAT_MOD_TEGRA_16BX2_BLOCK(3):
		return "MOD_TEGRA_16BX2_BLOCK(3)";
	case NV_FORMAT_MOD_TEGRA_16BX2_BLOCK(4):
		return "MOD_TEGRA_16BX2_BLOCK(4)";
	case NV_FORMAT_MOD_TEGRA_16BX2_BLOCK(5):
		return "MOD_TEGRA_16BX2_BLOCK(5)";
	case DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED:
		return "MOD_BROADCOM_VC4_T_TILED";
	case DRM_FORMAT_MOD_CHROMEOS_ROCKCHIP_AFBC:
		return "MOD_CHROMEOS_ROCKCHIP_AFBC";
	default:
		return "(UNKNOWN MODIFIER)";
	}
}

static void dump_in_formats(struct device *dev, uint32_t blob_id)
{
	uint32_t i, j;
	drmModePropertyBlobPtr blob;
	struct drm_format_modifier_blob *header;
	uint32_t *formats;
	struct drm_format_modifier *modifiers;

	printf("\t\tin_formats blob decoded:\n");
	blob = drmModeGetPropertyBlob(dev->fd, blob_id);
	if (!blob) {
		printf("\n");
		return;
	}

	header = blob->data;
	formats = (uint32_t *) ((char *) header + header->formats_offset);
	modifiers = (struct drm_format_modifier *)
		((char *) header + header->modifiers_offset);

	for (i = 0; i < header->count_formats; i++) {
		printf("\t\t\t");
		dump_fourcc(formats[i]);
		printf(": ");
		for (j = 0; j < header->count_modifiers; j++) {
			uint64_t mask = 1ULL << i;
			if (modifiers[j].formats & mask)
				printf(" %s", modifier_to_string(modifiers[j].modifier));
		}
		printf("\n");
	}

	drmModeFreePropertyBlob(blob);
}

static void dump_prop(struct device *dev, drmModePropertyPtr prop,
		      uint32_t prop_id, uint64_t value)
{
	int i;
	printf("\t%d", prop_id);
	if (!prop) {
		printf("\n");
		return;
	}

	printf(" %s:\n", prop->name);

	printf("\t\tflags:");
	if (prop->flags & DRM_MODE_PROP_PENDING)
		printf(" pending");
	if (prop->flags & DRM_MODE_PROP_IMMUTABLE)
		printf(" immutable");
	if (drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE))
		printf(" signed range");
	if (drm_property_type_is(prop, DRM_MODE_PROP_RANGE))
		printf(" range");
	if (drm_property_type_is(prop, DRM_MODE_PROP_ENUM))
		printf(" enum");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BITMASK))
		printf(" bitmask");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB))
		printf(" blob");
	if (drm_property_type_is(prop, DRM_MODE_PROP_OBJECT))
		printf(" object");
	printf("\n");

	if (drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE)) {
		printf("\t\tvalues:");
		for (i = 0; i < prop->count_values; i++)
			printf(" %"PRId64, U642I64(prop->values[i]));
		printf("\n");
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_RANGE)) {
		printf("\t\tvalues:");
		for (i = 0; i < prop->count_values; i++)
			printf(" %"PRIu64, prop->values[i]);
		printf("\n");
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_ENUM)) {
		printf("\t\tenums:");
		for (i = 0; i < prop->count_enums; i++)
			printf(" %s=%llu", prop->enums[i].name,
			       prop->enums[i].value);
		printf("\n");
	} else if (drm_property_type_is(prop, DRM_MODE_PROP_BITMASK)) {
		printf("\t\tvalues:");
		for (i = 0; i < prop->count_enums; i++)
			printf(" %s=0x%llx", prop->enums[i].name,
			       (1LL << prop->enums[i].value));
		printf("\n");
	} else {
		assert(prop->count_enums == 0);
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB)) {
		printf("\t\tblobs:\n");
		for (i = 0; i < prop->count_blobs; i++)
			dump_blob(dev, prop->blob_ids[i]);
		printf("\n");
	} else {
		assert(prop->count_blobs == 0);
	}

	printf("\t\tvalue:");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB))
		dump_blob(dev, value);
	else if (drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE))
		printf(" %"PRId64"\n", value);
	else
		printf(" %"PRIu64"\n", value);

	if (strcmp(prop->name, "IN_FORMATS") == 0)
		dump_in_formats(dev, value);
}

static void dump_connectors(struct device *dev)
{
	int i, j;

	printf("Connectors:\n");
	printf("id\tencoder\tstatus\t\tname\t\tsize (mm)\tmodes\tencoders\n");
	for (i = 0; i < dev->resources->res->count_connectors; i++) {
		struct connector *_connector = &dev->resources->connectors[i];
		drmModeConnector *connector = _connector->connector;
		if (!connector)
			continue;

		printf("%d\t%d\t%s\t%-15s\t%dx%d\t\t%d\t",
		       connector->connector_id,
		       connector->encoder_id,
		       util_lookup_connector_status_name(connector->connection),
		       _connector->name,
		       connector->mmWidth, connector->mmHeight,
		       connector->count_modes);

		for (j = 0; j < connector->count_encoders; j++)
			printf("%s%d", j > 0 ? ", " : "", connector->encoders[j]);
		printf("\n");

		if (connector->count_modes) {
			printf("  modes:\n");
			printf("\tname refresh (Hz) hdisp hss hse htot vdisp "
			       "vss vse vtot)\n");
			for (j = 0; j < connector->count_modes; j++)
				dump_mode(&connector->modes[j]);
		}

		if (_connector->props) {
			printf("  props:\n");
			for (j = 0; j < (int)_connector->props->count_props; j++)
				dump_prop(dev, _connector->props_info[j],
					  _connector->props->props[j],
					  _connector->props->prop_values[j]);
		}
	}
	printf("\n");
}

static void dump_crtcs(struct device *dev)
{
	int i;
	uint32_t j;

	printf("CRTCs:\n");
	printf("id\tfb\tpos\tsize\n");
	for (i = 0; i < dev->resources->res->count_crtcs; i++) {
		struct crtc *_crtc = &dev->resources->crtcs[i];
		drmModeCrtc *crtc = _crtc->crtc;
		if (!crtc)
			continue;

		printf("%d\t%d\t(%d,%d)\t(%dx%d)\n",
		       crtc->crtc_id,
		       crtc->buffer_id,
		       crtc->x, crtc->y,
		       crtc->width, crtc->height);
		dump_mode(&crtc->mode);

		if (_crtc->props) {
			printf("  props:\n");
			for (j = 0; j < _crtc->props->count_props; j++)
				dump_prop(dev, _crtc->props_info[j],
					  _crtc->props->props[j],
					  _crtc->props->prop_values[j]);
		} else {
			printf("  no properties found\n");
		}
	}
	printf("\n");
}

static void dump_framebuffers(struct device *dev)
{
	drmModeFB *fb;
	int i;

	printf("Frame buffers:\n");
	printf("id\tsize\tpitch\n");
	for (i = 0; i < dev->resources->res->count_fbs; i++) {
		fb = dev->resources->fbs[i].fb;
		if (!fb)
			continue;

		printf("%u\t(%ux%u)\t%u\n",
		       fb->fb_id,
		       fb->width, fb->height,
		       fb->pitch);
	}
	printf("\n");
}

static void dump_planes(struct device *dev)
{
	unsigned int i, j;

	printf("Planes:\n");
	printf("id\tcrtc\tfb\tCRTC x,y\tx,y\tgamma size\tpossible crtcs\n");

	if (!dev->resources->plane_res)
		return;

	for (i = 0; i < dev->resources->plane_res->count_planes; i++) {
		struct plane *plane = &dev->resources->planes[i];
		drmModePlane *ovr = plane->plane;
		if (!ovr)
			continue;

		printf("%d\t%d\t%d\t%d,%d\t\t%d,%d\t%-8d\t0x%08x\n",
		       ovr->plane_id, ovr->crtc_id, ovr->fb_id,
		       ovr->crtc_x, ovr->crtc_y, ovr->x, ovr->y,
		       ovr->gamma_size, ovr->possible_crtcs);

		if (!ovr->count_formats)
			continue;

		printf("  formats:");
		for (j = 0; j < ovr->count_formats; j++)
			dump_fourcc(ovr->formats[j]);
		printf("\n");

		if (plane->props) {
			printf("  props:\n");
			for (j = 0; j < plane->props->count_props; j++)
				dump_prop(dev, plane->props_info[j],
					  plane->props->props[j],
					  plane->props->prop_values[j]);
		} else {
			printf("  no properties found\n");
		}
	}
	printf("\n");

	return;
}

static void free_resources(struct resources *res)
{
	int i;

	if (!res)
		return;

#define free_resource(_res, __res, type, Type)					\
	do {									\
		if (!(_res)->type##s)						\
			break;							\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			if (!(_res)->type##s[i].type)				\
				break;						\
			drmModeFree##Type((_res)->type##s[i].type);		\
		}								\
		free((_res)->type##s);						\
	} while (0)

#define free_properties(_res, __res, type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			drmModeFreeObjectProperties(res->type##s[i].props);	\
			free(res->type##s[i].props_info);			\
		}								\
	} while (0)

	if (res->res) {
		free_properties(res, res, crtc);

		free_resource(res, res, crtc, Crtc);
		free_resource(res, res, encoder, Encoder);

		for (i = 0; i < res->res->count_connectors; i++)
			free(res->connectors[i].name);

		free_resource(res, res, connector, Connector);
		free_resource(res, res, fb, FB);

		drmModeFreeResources(res->res);
	}

	if (res->plane_res) {
		free_properties(res, plane_res, plane);

		free_resource(res, plane_res, plane, Plane);

		drmModeFreePlaneResources(res->plane_res);
	}

	free(res);
}

static struct resources *get_resources(struct device *dev)
{
	struct resources *res;
	int i;

	res = calloc(1, sizeof(*res));
	if (res == 0)
		return NULL;

	drmSetClientCap(dev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

	res->res = drmModeGetResources(dev->fd);
	if (!res->res) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
			strerror(errno));
		goto error;
	}

	res->crtcs = calloc(res->res->count_crtcs, sizeof(*res->crtcs));
	res->encoders = calloc(res->res->count_encoders, sizeof(*res->encoders));
	res->connectors = calloc(res->res->count_connectors, sizeof(*res->connectors));
	res->fbs = calloc(res->res->count_fbs, sizeof(*res->fbs));

	if (!res->crtcs || !res->encoders || !res->connectors || !res->fbs)
		goto error;

#define get_resource(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			(_res)->type##s[i].type =				\
				drmModeGet##Type(dev->fd, (_res)->__res->type##s[i]); \
			if (!(_res)->type##s[i].type)				\
				fprintf(stderr, "could not get %s %i: %s\n",	\
					#type, (_res)->__res->type##s[i],	\
					strerror(errno));			\
		}								\
	} while (0)

	get_resource(res, res, crtc, Crtc);
	get_resource(res, res, encoder, Encoder);
	get_resource(res, res, connector, Connector);
	get_resource(res, res, fb, FB);

	/* Set the name of all connectors based on the type name and the per-type ID. */
	for (i = 0; i < res->res->count_connectors; i++) {
		struct connector *connector = &res->connectors[i];
		drmModeConnector *conn = connector->connector;

		asprintf(&connector->name, "%s-%u",
			 util_lookup_connector_type_name(conn->connector_type),
			 conn->connector_type_id);
	}

#define get_properties(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			struct type *obj = &res->type##s[i];			\
			unsigned int j;						\
			obj->props =						\
				drmModeObjectGetProperties(dev->fd, obj->type->type##_id, \
							   DRM_MODE_OBJECT_##Type); \
			if (!obj->props) {					\
				fprintf(stderr,					\
					"could not get %s %i properties: %s\n", \
					#type, obj->type->type##_id,		\
					strerror(errno));			\
				continue;					\
			}							\
			obj->props_info = calloc(obj->props->count_props,	\
						 sizeof(*obj->props_info));	\
			if (!obj->props_info)					\
				continue;					\
			for (j = 0; j < obj->props->count_props; ++j)		\
				obj->props_info[j] =				\
					drmModeGetProperty(dev->fd, obj->props->props[j]); \
		}								\
	} while (0)

	get_properties(res, res, crtc, CRTC);
	get_properties(res, res, connector, CONNECTOR);

	for (i = 0; i < res->res->count_crtcs; ++i)
		res->crtcs[i].mode = &res->crtcs[i].crtc->mode;

	res->plane_res = drmModeGetPlaneResources(dev->fd);
	if (!res->plane_res) {
		fprintf(stderr, "drmModeGetPlaneResources failed: %s\n",
			strerror(errno));
		return res;
	}

	res->planes = calloc(res->plane_res->count_planes, sizeof(*res->planes));
	if (!res->planes)
		goto error;

	get_resource(res, plane_res, plane, Plane);
	get_properties(res, plane_res, plane, PLANE);

	return res;

error:
	free_resources(res);
	return NULL;
}

static int get_crtc_index(struct device *dev, uint32_t id)
{
	int i;

	for (i = 0; i < dev->resources->res->count_crtcs; ++i) {
		drmModeCrtc *crtc = dev->resources->crtcs[i].crtc;
		if (crtc && crtc->crtc_id == id)
			return i;
	}

	return -1;
}

static drmModeConnector *get_connector_by_name(struct device *dev, const char *name)
{
	struct connector *connector;
	int i;

	for (i = 0; i < dev->resources->res->count_connectors; i++) {
		connector = &dev->resources->connectors[i];

		if (strcmp(connector->name, name) == 0)
			return connector->connector;
	}

	return NULL;
}

static drmModeConnector *get_connector_by_id(struct device *dev, uint32_t id)
{
	drmModeConnector *connector;
	int i;

	for (i = 0; i < dev->resources->res->count_connectors; i++) {
		connector = dev->resources->connectors[i].connector;
		if (connector && connector->connector_id == id)
			return connector;
	}

	return NULL;
}

static drmModeEncoder *get_encoder_by_id(struct device *dev, uint32_t id)
{
	drmModeEncoder *encoder;
	int i;

	for (i = 0; i < dev->resources->res->count_encoders; i++) {
		encoder = dev->resources->encoders[i].encoder;
		if (encoder && encoder->encoder_id == id)
			return encoder;
	}

	return NULL;
}

/* -----------------------------------------------------------------------------
 * Pipes and planes
 */

/*
 * Mode setting with the kernel interfaces is a bit of a chore.
 * First you have to find the connector in question and make sure the
 * requested mode is available.
 * Then you need to find the encoder attached to that connector so you
 * can bind it with a free crtc.
 */
struct pipe_arg {
	const char **cons;
	uint32_t *con_ids;
	unsigned int num_cons;
	uint32_t crtc_id;
	char mode_str[64];
	char format_str[5];
	unsigned int vrefresh;
	unsigned int fourcc;
	drmModeModeInfo *mode;
	struct crtc *crtc;
	unsigned int fb_id[2], current_fb_id;
	struct timeval start;

	int swap_count;
};

struct plane_arg {
	uint32_t plane_id;  /* the id of plane to use */
	uint32_t crtc_id;  /* the id of CRTC to bind to */
	bool has_position;
	int32_t x, y;
	uint32_t w, h;
	double scale;
	unsigned int fb_id;
	struct bo *bo;
	char format_str[5]; /* need to leave room for terminating \0 */
	unsigned int fourcc;
};

static drmModeModeInfo *
connector_find_mode(struct device *dev, uint32_t con_id, const char *mode_str,
        const unsigned int vrefresh)
{
	drmModeConnector *connector;
	drmModeModeInfo *mode;
	int i;

	connector = get_connector_by_id(dev, con_id);
	if (!connector || !connector->count_modes)
		return NULL;

	for (i = 0; i < connector->count_modes; i++) {
		mode = &connector->modes[i];
		if (!strcmp(mode->name, mode_str)) {
			/* If the vertical refresh frequency is not specified then return the
			 * first mode that match with the name. Else, return the mode that match
			 * the name and the specified vertical refresh frequency.
			 */
			if (vrefresh == 0)
				return mode;
			else if (mode->vrefresh == vrefresh)
				return mode;
		}
	}

	return NULL;
}

static struct crtc *pipe_find_crtc(struct device *dev, struct pipe_arg *pipe)
{
	uint32_t possible_crtcs = ~0;
	uint32_t active_crtcs = 0;
	unsigned int crtc_idx;
	unsigned int i;
	int j;

	for (i = 0; i < pipe->num_cons; ++i) {
		uint32_t crtcs_for_connector = 0;
		drmModeConnector *connector;
		drmModeEncoder *encoder;
		int idx;

		connector = get_connector_by_id(dev, pipe->con_ids[i]);
		if (!connector)
			return NULL;

		for (j = 0; j < connector->count_encoders; ++j) {
			encoder = get_encoder_by_id(dev, connector->encoders[j]);
			if (!encoder)
				continue;

			crtcs_for_connector |= encoder->possible_crtcs;

			idx = get_crtc_index(dev, encoder->crtc_id);
			if (idx >= 0)
				active_crtcs |= 1 << idx;
		}

		possible_crtcs &= crtcs_for_connector;
	}

	if (!possible_crtcs)
		return NULL;

	/* Return the first possible and active CRTC if one exists, or the first
	 * possible CRTC otherwise.
	 */
	if (possible_crtcs & active_crtcs)
		crtc_idx = ffs(possible_crtcs & active_crtcs);
	else
		crtc_idx = ffs(possible_crtcs);

	return &dev->resources->crtcs[crtc_idx - 1];
}

static int pipe_find_crtc_and_mode(struct device *dev, struct pipe_arg *pipe)
{
	drmModeModeInfo *mode = NULL;
	int i;

	pipe->mode = NULL;

	for (i = 0; i < (int)pipe->num_cons; i++) {
		mode = connector_find_mode(dev, pipe->con_ids[i],
					   pipe->mode_str, pipe->vrefresh);
		if (mode == NULL) {
			fprintf(stderr,
				"failed to find mode \"%s\" for connector %s\n",
				pipe->mode_str, pipe->cons[i]);
			return -EINVAL;
		}
	}

	/* If the CRTC ID was specified, get the corresponding CRTC. Otherwise
	 * locate a CRTC that can be attached to all the connectors.
	 */
	if (pipe->crtc_id != (uint32_t)-1) {
		for (i = 0; i < dev->resources->res->count_crtcs; i++) {
			struct crtc *crtc = &dev->resources->crtcs[i];

			if (pipe->crtc_id == crtc->crtc->crtc_id) {
				pipe->crtc = crtc;
				break;
			}
		}
	} else {
		pipe->crtc = pipe_find_crtc(dev, pipe);
	}

	if (!pipe->crtc) {
		fprintf(stderr, "failed to find CRTC for pipe\n");
		return -EINVAL;
	}

	pipe->mode = mode;
	pipe->crtc->mode = mode;

	return 0;
}

/* -----------------------------------------------------------------------------
 * Properties
 */

struct property_arg {
	uint32_t obj_id;
	uint32_t obj_type;
	char name[DRM_PROP_NAME_LEN+1];
	uint32_t prop_id;
	uint64_t value;
};

static void set_property(struct device *dev, struct property_arg *p)
{
	drmModeObjectProperties *props = NULL;
	drmModePropertyRes **props_info = NULL;
	const char *obj_type;
	int ret;
	int i;

	p->obj_type = 0;
	p->prop_id = 0;

#define find_object(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			struct type *obj = &(_res)->type##s[i];			\
			if (obj->type->type##_id != p->obj_id)			\
				continue;					\
			p->obj_type = DRM_MODE_OBJECT_##Type;			\
			obj_type = #Type;					\
			props = obj->props;					\
			props_info = obj->props_info;				\
		}								\
	} while(0)								\

	find_object(dev->resources, res, crtc, CRTC);
	if (p->obj_type == 0)
		find_object(dev->resources, res, connector, CONNECTOR);
	if (p->obj_type == 0)
		find_object(dev->resources, plane_res, plane, PLANE);
	if (p->obj_type == 0) {
		fprintf(stderr, "Object %i not found, can't set property\n",
			p->obj_id);
			return;
	}

	if (!props) {
		fprintf(stderr, "%s %i has no properties\n",
			obj_type, p->obj_id);
		return;
	}

	for (i = 0; i < (int)props->count_props; ++i) {
		if (!props_info[i])
			continue;
		if (strcmp(props_info[i]->name, p->name) == 0)
			break;
	}

	if (i == (int)props->count_props) {
		fprintf(stderr, "%s %i has no %s property\n",
			obj_type, p->obj_id, p->name);
		return;
	}

	p->prop_id = props->props[i];

	ret = drmModeObjectSetProperty(dev->fd, p->obj_id, p->obj_type,
				       p->prop_id, p->value);
	if (ret < 0)
		fprintf(stderr, "failed to set %s %i property %s to %" PRIu64 ": %s\n",
			obj_type, p->obj_id, p->name, p->value, strerror(errno));
}

/* -------------------------------------------------------------------------- */

static void
page_flip_handler(int fd, unsigned int frame,
		  unsigned int sec, unsigned int usec, void *data)
{
	struct pipe_arg *pipe;
	unsigned int new_fb_id;
	struct timeval end;
	double t;

	pipe = data;
	if (pipe->current_fb_id == pipe->fb_id[0])
		new_fb_id = pipe->fb_id[1];
	else
		new_fb_id = pipe->fb_id[0];

	drmModePageFlip(fd, pipe->crtc->crtc->crtc_id, new_fb_id,
			DRM_MODE_PAGE_FLIP_EVENT, pipe);
	pipe->current_fb_id = new_fb_id;
	pipe->swap_count++;
	if (pipe->swap_count == 60) {
		gettimeofday(&end, NULL);
		t = end.tv_sec + end.tv_usec * 1e-6 -
			(pipe->start.tv_sec + pipe->start.tv_usec * 1e-6);
		fprintf(stderr, "freq: %.02fHz\n", pipe->swap_count / t);
		pipe->swap_count = 0;
		pipe->start = end;
	}
}

static bool format_support(const drmModePlanePtr ovr, uint32_t fmt)
{
	unsigned int i;

	for (i = 0; i < ovr->count_formats; ++i) {
		if (ovr->formats[i] == fmt)
			return true;
	}

	return false;
}

static int set_plane(struct device *dev, struct plane_arg *p)
{
	drmModePlane *ovr;
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	uint32_t plane_id;
	struct bo *plane_bo;
	uint32_t plane_flags = 0;
	int crtc_x, crtc_y, crtc_w, crtc_h;
	struct crtc *crtc = NULL;
	unsigned int pipe;
	unsigned int i;

	/* Find an unused plane which can be connected to our CRTC. Find the
	 * CRTC index first, then iterate over available planes.
	 */
	for (i = 0; i < (unsigned int)dev->resources->res->count_crtcs; i++) {
		if (p->crtc_id == dev->resources->res->crtcs[i]) {
			crtc = &dev->resources->crtcs[i];
			pipe = i;
			break;
		}
	}

	if (!crtc) {
		fprintf(stderr, "CRTC %u not found\n", p->crtc_id);
		return -1;
	}

	plane_id = p->plane_id;

	for (i = 0; i < dev->resources->plane_res->count_planes; i++) {
		ovr = dev->resources->planes[i].plane;
		if (!ovr)
			continue;

		if (plane_id && plane_id != ovr->plane_id)
			continue;

		if (!format_support(ovr, p->fourcc))
			continue;

		if ((ovr->possible_crtcs & (1 << pipe)) &&
		    (ovr->crtc_id == 0 || ovr->crtc_id == p->crtc_id)) {
			plane_id = ovr->plane_id;
			break;
		}
	}

	if (i == dev->resources->plane_res->count_planes) {
		fprintf(stderr, "no unused plane available for CRTC %u\n",
			crtc->crtc->crtc_id);
		return -1;
	}

	fprintf(stderr, "testing %dx%d@%s overlay plane %u\n",
		p->w, p->h, p->format_str, plane_id);

	plane_bo = bo_create(dev->fd, p->fourcc, p->w, p->h, handles,
			     pitches, offsets, UTIL_PATTERN_TILES);
	if (plane_bo == NULL)
		return -1;

	p->bo = plane_bo;

	/* just use single plane format for now.. */
	if (drmModeAddFB2(dev->fd, p->w, p->h, p->fourcc,
			handles, pitches, offsets, &p->fb_id, plane_flags)) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		return -1;
	}

	crtc_w = p->w * p->scale;
	crtc_h = p->h * p->scale;
	if (!p->has_position) {
		/* Default to the middle of the screen */
		crtc_x = (crtc->mode->hdisplay - crtc_w) / 2;
		crtc_y = (crtc->mode->vdisplay - crtc_h) / 2;
	} else {
		crtc_x = p->x;
		crtc_y = p->y;
	}

	/* note src coords (last 4 args) are in Q16 format */
	if (drmModeSetPlane(dev->fd, plane_id, crtc->crtc->crtc_id, p->fb_id,
			    plane_flags, crtc_x, crtc_y, crtc_w, crtc_h,
			    0, 0, p->w << 16, p->h << 16)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));
		return -1;
	}

	ovr->crtc_id = crtc->crtc->crtc_id;

	return 0;
}

static void clear_planes(struct device *dev, struct plane_arg *p, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		if (p[i].fb_id)
			drmModeRmFB(dev->fd, p[i].fb_id);
		if (p[i].bo)
			bo_destroy(p[i].bo);
	}
}


static void set_mode(struct device *dev, struct pipe_arg *pipes, unsigned int count)
{
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	unsigned int fb_id;
	struct bo *bo;
	unsigned int i;
	unsigned int j;
	int ret, x;

	dev->mode.width = 0;
	dev->mode.height = 0;
	dev->mode.fb_id = 0;

	for (i = 0; i < count; i++) {
		struct pipe_arg *pipe = &pipes[i];

		ret = pipe_find_crtc_and_mode(dev, pipe);
		if (ret < 0)
			continue;

		dev->mode.width += pipe->mode->hdisplay;
		if (dev->mode.height < pipe->mode->vdisplay)
			dev->mode.height = pipe->mode->vdisplay;
	}

	bo = bo_create(dev->fd, pipes[0].fourcc, dev->mode.width,
		       dev->mode.height, handles, pitches, offsets,
		       UTIL_PATTERN_SMPTE);
	if (bo == NULL)
		return;

	dev->mode.bo = bo;

	ret = drmModeAddFB2(dev->fd, dev->mode.width, dev->mode.height,
			    pipes[0].fourcc, handles, pitches, offsets, &fb_id, 0);
	if (ret) {
		fprintf(stderr, "failed to add fb (%ux%u): %s\n",
			dev->mode.width, dev->mode.height, strerror(errno));
		return;
	}

	dev->mode.fb_id = fb_id;

	x = 0;
	for (i = 0; i < count; i++) {
		struct pipe_arg *pipe = &pipes[i];

		if (pipe->mode == NULL)
			continue;

		printf("setting mode %s-%dHz@%s on connectors ",
		       pipe->mode_str, pipe->mode->vrefresh, pipe->format_str);
		for (j = 0; j < pipe->num_cons; ++j)
			printf("%s, ", pipe->cons[j]);
		printf("crtc %d\n", pipe->crtc->crtc->crtc_id);

		ret = drmModeSetCrtc(dev->fd, pipe->crtc->crtc->crtc_id, fb_id,
				     x, 0, pipe->con_ids, pipe->num_cons,
				     pipe->mode);

		/* XXX: Actually check if this is needed */
		drmModeDirtyFB(dev->fd, fb_id, NULL, 0);

		x += pipe->mode->hdisplay;

		if (ret) {
			fprintf(stderr, "failed to set mode: %s\n", strerror(errno));
			return;
		}
	}
}

static void clear_mode(struct device *dev)
{
	if (dev->mode.fb_id)
		drmModeRmFB(dev->fd, dev->mode.fb_id);
	if (dev->mode.bo)
		bo_destroy(dev->mode.bo);
}

static void set_planes(struct device *dev, struct plane_arg *p, unsigned int count)
{
	unsigned int i;

	/* set up planes/overlays */
	for (i = 0; i < count; i++)
		if (set_plane(dev, &p[i]))
			return;
}

static void set_cursors(struct device *dev, struct pipe_arg *pipes, unsigned int count)
{
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	struct bo *bo;
	unsigned int i;
	int ret;

	/* maybe make cursor width/height configurable some day */
	uint32_t cw = 64;
	uint32_t ch = 64;

	/* create cursor bo.. just using PATTERN_PLAIN as it has
	 * translucent alpha
	 */
	bo = bo_create(dev->fd, DRM_FORMAT_ARGB8888, cw, ch, handles, pitches,
		       offsets, UTIL_PATTERN_PLAIN);
	if (bo == NULL)
		return;

	dev->mode.cursor_bo = bo;

	for (i = 0; i < count; i++) {
		struct pipe_arg *pipe = &pipes[i];
		ret = cursor_init(dev->fd, handles[0],
				pipe->crtc->crtc->crtc_id,
				pipe->mode->hdisplay, pipe->mode->vdisplay,
				cw, ch);
		if (ret) {
			fprintf(stderr, "failed to init cursor for CRTC[%u]\n",
					pipe->crtc_id);
			return;
		}
	}

	cursor_start();
}

static void clear_cursors(struct device *dev)
{
	cursor_stop();

	if (dev->mode.cursor_bo)
		bo_destroy(dev->mode.cursor_bo);
}

static void test_page_flip(struct device *dev, struct pipe_arg *pipes, unsigned int count)
{
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	unsigned int other_fb_id;
	struct bo *other_bo;
	drmEventContext evctx;
	unsigned int i;
	int ret;

	other_bo = bo_create(dev->fd, pipes[0].fourcc, dev->mode.width,
			     dev->mode.height, handles, pitches, offsets,
			     UTIL_PATTERN_PLAIN);
	if (other_bo == NULL)
		return;

	ret = drmModeAddFB2(dev->fd, dev->mode.width, dev->mode.height,
			    pipes[0].fourcc, handles, pitches, offsets,
			    &other_fb_id, 0);
	if (ret) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		goto err;
	}

	for (i = 0; i < count; i++) {
		struct pipe_arg *pipe = &pipes[i];

		if (pipe->mode == NULL)
			continue;

		ret = drmModePageFlip(dev->fd, pipe->crtc->crtc->crtc_id,
				      other_fb_id, DRM_MODE_PAGE_FLIP_EVENT,
				      pipe);
		if (ret) {
			fprintf(stderr, "failed to page flip: %s\n", strerror(errno));
			goto err_rmfb;
		}
		gettimeofday(&pipe->start, NULL);
		pipe->swap_count = 0;
		pipe->fb_id[0] = dev->mode.fb_id;
		pipe->fb_id[1] = other_fb_id;
		pipe->current_fb_id = other_fb_id;
	}

	memset(&evctx, 0, sizeof evctx);
	evctx.version = DRM_EVENT_CONTEXT_VERSION;
	evctx.vblank_handler = NULL;
	evctx.page_flip_handler = page_flip_handler;

	while (1) {
#if 0
		struct pollfd pfd[2];

		pfd[0].fd = 0;
		pfd[0].events = POLLIN;
		pfd[1].fd = fd;
		pfd[1].events = POLLIN;

		if (poll(pfd, 2, -1) < 0) {
			fprintf(stderr, "poll error\n");
			break;
		}

		if (pfd[0].revents)
			break;
#else
		struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(dev->fd, &fds);
		ret = select(dev->fd + 1, &fds, NULL, NULL, &timeout);

		if (ret <= 0) {
			fprintf(stderr, "select timed out or error (ret %d)\n",
				ret);
			continue;
		} else if (FD_ISSET(0, &fds)) {
			break;
		}
#endif

		drmHandleEvent(dev->fd, &evctx);
	}

err_rmfb:
	drmModeRmFB(dev->fd, other_fb_id);
err:
	bo_destroy(other_bo);
}

#define min(a, b)	((a) < (b) ? (a) : (b))

static int parse_connector(struct pipe_arg *pipe, const char *arg)
{
	unsigned int len;
	unsigned int i;
	const char *p;
	char *endp;

	pipe->vrefresh = 0;
	pipe->crtc_id = (uint32_t)-1;
	strcpy(pipe->format_str, "XR24");

	/* Count the number of connectors and allocate them. */
	pipe->num_cons = 1;
	for (p = arg; *p && *p != ':' && *p != '@'; ++p) {
		if (*p == ',')
			pipe->num_cons++;
	}

	pipe->con_ids = calloc(pipe->num_cons, sizeof(*pipe->con_ids));
	pipe->cons = calloc(pipe->num_cons, sizeof(*pipe->cons));
	if (pipe->con_ids == NULL || pipe->cons == NULL)
		return -1;

	/* Parse the connectors. */
	for (i = 0, p = arg; i < pipe->num_cons; ++i, p = endp + 1) {
		endp = strpbrk(p, ",@:");
		if (!endp)
			break;

		pipe->cons[i] = strndup(p, endp - p);

		if (*endp != ',')
			break;
	}

	if (i != pipe->num_cons - 1)
		return -1;

	/* Parse the remaining parameters. */
	if (*endp == '@') {
		arg = endp + 1;
		pipe->crtc_id = strtoul(arg, &endp, 10);
	}
	if (*endp != ':')
		return -1;

	arg = endp + 1;

	/* Search for the vertical refresh or the format. */
	p = strpbrk(arg, "-@");
	if (p == NULL)
		p = arg + strlen(arg);
	len = min(sizeof pipe->mode_str - 1, (unsigned int)(p - arg));
	strncpy(pipe->mode_str, arg, len);
	pipe->mode_str[len] = '\0';

	if (*p == '-') {
		pipe->vrefresh = strtoul(p + 1, &endp, 10);
		p = endp;
	}

	if (*p == '@') {
		strncpy(pipe->format_str, p + 1, 4);
		pipe->format_str[4] = '\0';
	}

	pipe->fourcc = util_format_fourcc(pipe->format_str);
	if (pipe->fourcc == 0)  {
		fprintf(stderr, "unknown format %s\n", pipe->format_str);
		return -1;
	}

	return 0;
}

static int parse_plane(struct plane_arg *plane, const char *p)
{
	char *end;

	plane->plane_id = strtoul(p, &end, 10);
	if (*end != '@')
		return -EINVAL;

	p = end + 1;
	plane->crtc_id = strtoul(p, &end, 10);
	if (*end != ':')
		return -EINVAL;

	p = end + 1;
	plane->w = strtoul(p, &end, 10);
	if (*end != 'x')
		return -EINVAL;

	p = end + 1;
	plane->h = strtoul(p, &end, 10);

	if (*end == '+' || *end == '-') {
		plane->x = strtol(end, &end, 10);
		if (*end != '+' && *end != '-')
			return -EINVAL;
		plane->y = strtol(end, &end, 10);

		plane->has_position = true;
	}

	if (*end == '*') {
		p = end + 1;
		plane->scale = strtod(p, &end);
		if (plane->scale <= 0.0)
			return -EINVAL;
	} else {
		plane->scale = 1.0;
	}

	if (*end == '@') {
		p = end + 1;
		if (strlen(p) != 4)
			return -EINVAL;

		strcpy(plane->format_str, p);
	} else {
		strcpy(plane->format_str, "XR24");
	}

	plane->fourcc = util_format_fourcc(plane->format_str);
	if (plane->fourcc == 0) {
		fprintf(stderr, "unknown format %s\n", plane->format_str);
		return -EINVAL;
	}

	return 0;
}

static int parse_property(struct property_arg *p, const char *arg)
{
	if (sscanf(arg, "%d:%32[^:]:%" SCNu64, &p->obj_id, p->name, &p->value) != 3)
		return -1;

	p->obj_type = 0;
	p->name[DRM_PROP_NAME_LEN] = '\0';

	return 0;
}

static void usage(char *name)
{
	fprintf(stderr, "usage: %s [-cDdefMPpsCvw]\n", name);

	fprintf(stderr, "\n Query options:\n\n");
	fprintf(stderr, "\t-c\tlist connectors\n");
	fprintf(stderr, "\t-e\tlist encoders\n");
	fprintf(stderr, "\t-f\tlist framebuffers\n");
	fprintf(stderr, "\t-p\tlist CRTCs and planes (pipes)\n");

	fprintf(stderr, "\n Test options:\n\n");
	fprintf(stderr, "\t-P <plane_id>@<crtc_id>:<w>x<h>[+<x>+<y>][*<scale>][@<format>]\tset a plane\n");
	fprintf(stderr, "\t-s <connector_id>[,<connector_id>][@<crtc_id>]:<mode>[-<vrefresh>][@<format>]\tset a mode\n");
	fprintf(stderr, "\t-C\ttest hw cursor\n");
	fprintf(stderr, "\t-v\ttest vsynced page flipping\n");
	fprintf(stderr, "\t-w <obj_id>:<prop_name>:<value>\tset property\n");

	fprintf(stderr, "\n Generic options:\n\n");
	fprintf(stderr, "\t-d\tdrop master after mode set\n");
	fprintf(stderr, "\t-M module\tuse the given driver\n");
	fprintf(stderr, "\t-D device\tuse the given device\n");

	fprintf(stderr, "\n\tDefault is to dump all info.\n");
	exit(0);
}

static int page_flipping_supported(void)
{
	/*FIXME: generic ioctl needed? */
	return 1;
#if 0
	int ret, value;
	struct drm_i915_getparam gp;

	gp.param = I915_PARAM_HAS_PAGEFLIPPING;
	gp.value = &value;

	ret = drmCommandWriteRead(fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
	if (ret) {
		fprintf(stderr, "drm_i915_getparam: %m\n");
		return 0;
	}

	return *gp.value;
#endif
}

static int cursor_supported(void)
{
	/*FIXME: generic ioctl needed? */
	return 1;
}

static int pipe_resolve_connectors(struct device *dev, struct pipe_arg *pipe)
{
	drmModeConnector *connector;
	unsigned int i;
	uint32_t id;
	char *endp;

	for (i = 0; i < pipe->num_cons; i++) {
		id = strtoul(pipe->cons[i], &endp, 10);
		if (endp == pipe->cons[i]) {
			connector = get_connector_by_name(dev, pipe->cons[i]);
			if (!connector) {
				fprintf(stderr, "no connector named '%s'\n",
					pipe->cons[i]);
				return -ENODEV;
			}

			id = connector->connector_id;
		}

		pipe->con_ids[i] = id;
	}

	return 0;
}

static char optstr[] = "cdD:efM:P:ps:Cvw:";

int main(int argc, char **argv)
{
	struct device dev;

	int c;
	int encoders = 0, connectors = 0, crtcs = 0, planes = 0, framebuffers = 0;
	int drop_master = 0;
	int test_vsync = 0;
	int test_cursor = 0;
	char *device = NULL;
	char *module = NULL;
	unsigned int i;
	unsigned int count = 0, plane_count = 0;
	unsigned int prop_count = 0;
	struct pipe_arg *pipe_args = NULL;
	struct plane_arg *plane_args = NULL;
	struct property_arg *prop_args = NULL;
	unsigned int args = 0;
	int ret;

	memset(&dev, 0, sizeof dev);

	opterr = 0;
	while ((c = getopt(argc, argv, optstr)) != -1) {
		args++;

		switch (c) {
		case 'c':
			connectors = 1;
			break;
		case 'D':
			device = optarg;
			args--;
			break;
		case 'd':
			drop_master = 1;
			break;
		case 'e':
			encoders = 1;
			break;
		case 'f':
			framebuffers = 1;
			break;
		case 'M':
			module = optarg;
			/* Preserve the default behaviour of dumping all information. */
			args--;
			break;
		case 'P':
			plane_args = realloc(plane_args,
					     (plane_count + 1) * sizeof *plane_args);
			if (plane_args == NULL) {
				fprintf(stderr, "memory allocation failed\n");
				return 1;
			}
			memset(&plane_args[plane_count], 0, sizeof(*plane_args));

			if (parse_plane(&plane_args[plane_count], optarg) < 0)
				usage(argv[0]);

			plane_count++;
			break;
		case 'p':
			crtcs = 1;
			planes = 1;
			break;
		case 's':
			pipe_args = realloc(pipe_args,
					    (count + 1) * sizeof *pipe_args);
			if (pipe_args == NULL) {
				fprintf(stderr, "memory allocation failed\n");
				return 1;
			}
			memset(&pipe_args[count], 0, sizeof(*pipe_args));

			if (parse_connector(&pipe_args[count], optarg) < 0)
				usage(argv[0]);

			count++;
			break;
		case 'C':
			test_cursor = 1;
			break;
		case 'v':
			test_vsync = 1;
			break;
		case 'w':
			prop_args = realloc(prop_args,
					   (prop_count + 1) * sizeof *prop_args);
			if (prop_args == NULL) {
				fprintf(stderr, "memory allocation failed\n");
				return 1;
			}
			memset(&prop_args[prop_count], 0, sizeof(*prop_args));

			if (parse_property(&prop_args[prop_count], optarg) < 0)
				usage(argv[0]);

			prop_count++;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if (!args)
		encoders = connectors = crtcs = planes = framebuffers = 1;

	dev.fd = util_open(device, module);
	if (dev.fd < 0)
		return -1;

	if (test_vsync && !page_flipping_supported()) {
		fprintf(stderr, "page flipping not supported by drm.\n");
		return -1;
	}

	if (test_vsync && !count) {
		fprintf(stderr, "page flipping requires at least one -s option.\n");
		return -1;
	}

	if (test_cursor && !cursor_supported()) {
		fprintf(stderr, "hw cursor not supported by drm.\n");
		return -1;
	}

	dev.resources = get_resources(&dev);
	if (!dev.resources) {
		drmClose(dev.fd);
		return 1;
	}

	for (i = 0; i < count; i++) {
		if (pipe_resolve_connectors(&dev, &pipe_args[i]) < 0) {
			free_resources(dev.resources);
			drmClose(dev.fd);
			return 1;
		}
	}

#define dump_resource(dev, res) if (res) dump_##res(dev)

	dump_resource(&dev, encoders);
	dump_resource(&dev, connectors);
	dump_resource(&dev, crtcs);
	dump_resource(&dev, planes);
	dump_resource(&dev, framebuffers);

	for (i = 0; i < prop_count; ++i)
		set_property(&dev, &prop_args[i]);

	if (count || plane_count) {
		uint64_t cap = 0;

		ret = drmGetCap(dev.fd, DRM_CAP_DUMB_BUFFER, &cap);
		if (ret || cap == 0) {
			fprintf(stderr, "driver doesn't support the dumb buffer API\n");
			return 1;
		}

		if (count)
			set_mode(&dev, pipe_args, count);

		if (plane_count)
			set_planes(&dev, plane_args, plane_count);

		if (test_cursor)
			set_cursors(&dev, pipe_args, count);

		if (test_vsync)
			test_page_flip(&dev, pipe_args, count);

		if (drop_master)
			drmDropMaster(dev.fd);

		getchar();

		if (test_cursor)
			clear_cursors(&dev);

		if (plane_count)
			clear_planes(&dev, plane_args, plane_count);

		if (count)
			clear_mode(&dev);
	}

	free_resources(dev.resources);

	return 0;
}
