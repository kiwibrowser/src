#ifndef __NV30_SCREEN_H__
#define __NV30_SCREEN_H__

#include <stdio.h>

#define NOUVEAU_ERR(fmt, args...) \
   fprintf(stderr, "%s:%d -  "fmt, __FUNCTION__, __LINE__, ##args);

#include "util/u_double_list.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_fence.h"
#include "nouveau/nouveau_heap.h"
#include "nv30_winsys.h"
#include "nv30_resource.h"

struct nv30_context;

struct nv30_screen {
   struct nouveau_screen base;

   struct nv30_context *cur_ctx;

   struct nouveau_bo *notify;

   struct nouveau_object *ntfy;
   struct nouveau_object *fence;

   struct nouveau_object *query;
   struct nouveau_heap *query_heap;
   struct list_head queries;

   struct nouveau_object *null;
   struct nouveau_object *eng3d;
   struct nouveau_object *m2mf;
   struct nouveau_object *surf2d;
   struct nouveau_object *swzsurf;
   struct nouveau_object *sifm;

   /*XXX: nvfx state */
   struct nouveau_heap *vp_exec_heap;
   struct nouveau_heap *vp_data_heap;
};

static INLINE struct nv30_screen *
nv30_screen(struct pipe_screen *pscreen)
{
   return (struct nv30_screen *)pscreen;
}

#endif
