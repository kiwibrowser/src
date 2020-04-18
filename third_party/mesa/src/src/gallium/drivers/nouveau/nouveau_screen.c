#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_string.h"

#include "os/os_time.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <libdrm/nouveau_drm.h>

#include "nouveau_winsys.h"
#include "nouveau_screen.h"
#include "nouveau_fence.h"
#include "nouveau_mm.h"
#include "nouveau_buffer.h"

/* XXX this should go away */
#include "state_tracker/drm_driver.h"

int nouveau_mesa_debug = 0;

static const char *
nouveau_screen_get_name(struct pipe_screen *pscreen)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	static char buffer[128];

	util_snprintf(buffer, sizeof(buffer), "NV%02X", dev->chipset);
	return buffer;
}

static const char *
nouveau_screen_get_vendor(struct pipe_screen *pscreen)
{
	return "nouveau";
}

static uint64_t
nouveau_screen_get_timestamp(struct pipe_screen *pscreen)
{
	int64_t cpu_time = os_time_get() * 1000;

        /* getparam of PTIMER_TIME takes about x10 as long (several usecs) */

	return cpu_time + nouveau_screen(pscreen)->cpu_gpu_time_delta;
}

static void
nouveau_screen_fence_ref(struct pipe_screen *pscreen,
			 struct pipe_fence_handle **ptr,
			 struct pipe_fence_handle *pfence)
{
	nouveau_fence_ref(nouveau_fence(pfence), (struct nouveau_fence **)ptr);
}

static boolean
nouveau_screen_fence_signalled(struct pipe_screen *screen,
                               struct pipe_fence_handle *pfence)
{
        return nouveau_fence_signalled(nouveau_fence(pfence));
}

static boolean
nouveau_screen_fence_finish(struct pipe_screen *screen,
			    struct pipe_fence_handle *pfence,
                            uint64_t timeout)
{
	return nouveau_fence_wait(nouveau_fence(pfence));
}


struct nouveau_bo *
nouveau_screen_bo_from_handle(struct pipe_screen *pscreen,
			      struct winsys_handle *whandle,
			      unsigned *out_stride)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nouveau_bo *bo = 0;
	int ret;
 
	ret = nouveau_bo_name_ref(dev, whandle->handle, &bo);
	if (ret) {
		debug_printf("%s: ref name 0x%08x failed with %d\n",
			     __FUNCTION__, whandle->handle, ret);
		return NULL;
	}

	*out_stride = whandle->stride;
	return bo;
}


boolean
nouveau_screen_bo_get_handle(struct pipe_screen *pscreen,
			     struct nouveau_bo *bo,
			     unsigned stride,
			     struct winsys_handle *whandle)
{
	whandle->stride = stride;

	if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) { 
		return nouveau_bo_name_get(bo, &whandle->handle) == 0;
	} else if (whandle->type == DRM_API_HANDLE_TYPE_KMS) {
		whandle->handle = bo->handle;
		return TRUE;
	} else {
		return FALSE;
	}
}

int
nouveau_screen_init(struct nouveau_screen *screen, struct nouveau_device *dev)
{
	struct pipe_screen *pscreen = &screen->base;
	struct nv04_fifo nv04_data = { .vram = 0xbeef0201, .gart = 0xbeef0202 };
	struct nvc0_fifo nvc0_data = { };
	uint64_t time;
	int size, ret;
	void *data;
	union nouveau_bo_config mm_config;

	char *nv_dbg = getenv("NOUVEAU_MESA_DEBUG");
	if (nv_dbg)
	   nouveau_mesa_debug = atoi(nv_dbg);

	if (dev->chipset < 0xc0) {
		data = &nv04_data;
		size = sizeof(nv04_data);
	} else {
		data = &nvc0_data;
		size = sizeof(nvc0_data);
	}

	ret = nouveau_object_new(&dev->object, 0, NOUVEAU_FIFO_CHANNEL_CLASS,
				 data, size, &screen->channel);
	if (ret)
		return ret;
	screen->device = dev;

	ret = nouveau_client_new(screen->device, &screen->client);
	if (ret)
		return ret;
	ret = nouveau_pushbuf_new(screen->client, screen->channel,
				  4, 512 * 1024, 1,
				  &screen->pushbuf);
	if (ret)
		return ret;

        /* getting CPU time first appears to be more accurate */
        screen->cpu_gpu_time_delta = os_time_get();

        ret = nouveau_getparam(dev, NOUVEAU_GETPARAM_PTIMER_TIME, &time);
        if (!ret)
           screen->cpu_gpu_time_delta = time - screen->cpu_gpu_time_delta * 1000;

	pscreen->get_name = nouveau_screen_get_name;
	pscreen->get_vendor = nouveau_screen_get_vendor;

	pscreen->get_timestamp = nouveau_screen_get_timestamp;

	pscreen->fence_reference = nouveau_screen_fence_ref;
	pscreen->fence_signalled = nouveau_screen_fence_signalled;
	pscreen->fence_finish = nouveau_screen_fence_finish;

	util_format_s3tc_init();

	screen->lowmem_bindings = PIPE_BIND_GLOBAL; /* gallium limit */
	screen->vidmem_bindings =
		PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL |
		PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT | PIPE_BIND_CURSOR |
		PIPE_BIND_SAMPLER_VIEW |
		PIPE_BIND_SHADER_RESOURCE | PIPE_BIND_COMPUTE_RESOURCE |
		PIPE_BIND_GLOBAL;
	screen->sysmem_bindings =
		PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_STREAM_OUTPUT;

	memset(&mm_config, 0, sizeof(mm_config));

	screen->mm_GART = nouveau_mm_create(dev,
					    NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
					    &mm_config);
	screen->mm_VRAM = nouveau_mm_create(dev, NOUVEAU_BO_VRAM, &mm_config);
	return 0;
}

void
nouveau_screen_fini(struct nouveau_screen *screen)
{
	nouveau_mm_destroy(screen->mm_GART);
	nouveau_mm_destroy(screen->mm_VRAM);

	nouveau_pushbuf_del(&screen->pushbuf);

	nouveau_client_del(&screen->client);
	nouveau_object_del(&screen->channel);

	nouveau_device_del(&screen->device);
}
