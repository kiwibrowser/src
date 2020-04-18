
#ifndef __NOUVEAU_FENCE_H__
#define __NOUVEAU_FENCE_H__

#include "util/u_inlines.h"
#include "util/u_double_list.h"

#define NOUVEAU_FENCE_STATE_AVAILABLE 0
#define NOUVEAU_FENCE_STATE_EMITTING  1
#define NOUVEAU_FENCE_STATE_EMITTED   2
#define NOUVEAU_FENCE_STATE_FLUSHED   3
#define NOUVEAU_FENCE_STATE_SIGNALLED 4

struct nouveau_fence_work {
   struct list_head list;
   void (*func)(void *);
   void *data;
};

struct nouveau_fence {
   struct nouveau_fence *next;
   struct nouveau_screen *screen;
   int state;
   int ref;
   uint32_t sequence;
   struct list_head work;
};

void nouveau_fence_emit(struct nouveau_fence *);
void nouveau_fence_del(struct nouveau_fence *);

boolean nouveau_fence_new(struct nouveau_screen *, struct nouveau_fence **,
                          boolean emit);
boolean nouveau_fence_work(struct nouveau_fence *, void (*)(void *), void *);
void    nouveau_fence_update(struct nouveau_screen *, boolean flushed);
void    nouveau_fence_next(struct nouveau_screen *);
boolean nouveau_fence_wait(struct nouveau_fence *);
boolean nouveau_fence_signalled(struct nouveau_fence *);

static INLINE void
nouveau_fence_ref(struct nouveau_fence *fence, struct nouveau_fence **ref)
{
   if (fence)
      ++fence->ref;

   if (*ref) {
      if (--(*ref)->ref == 0)
         nouveau_fence_del(*ref);
   }

   *ref = fence;
}

static INLINE struct nouveau_fence *
nouveau_fence(struct pipe_fence_handle *fence)
{
   return (struct nouveau_fence *)fence;
}

#endif // __NOUVEAU_FENCE_H__
