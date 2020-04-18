#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "nouveau_drm_public.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_screen.h"

struct pipe_screen *
nouveau_drm_screen_create(int fd)
{
	struct nouveau_device *dev = NULL;
	struct pipe_screen *(*init)(struct nouveau_device *);
	int ret;

	ret = nouveau_device_wrap(fd, 0, &dev);
	if (ret)
		return NULL;

	switch (dev->chipset & 0xf0) {
	case 0x30:
	case 0x40:
	case 0x60:
		init = nv30_screen_create;
		break;
	case 0x50:
	case 0x80:
	case 0x90:
	case 0xa0:
		init = nv50_screen_create;
		break;
	case 0xc0:
	case 0xd0:
	case 0xe0:
		init = nvc0_screen_create;
		break;
	default:
		debug_printf("%s: unknown chipset nv%02x\n", __func__,
			     dev->chipset);
		return NULL;
	}

	return init(dev);
}
