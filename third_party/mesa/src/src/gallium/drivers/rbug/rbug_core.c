/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "os/os_thread.h"
#include "util/u_format.h"
#include "util/u_string.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "util/u_network.h"
#include "os/os_time.h"

#include "tgsi/tgsi_parse.h"

#include "rbug_context.h"
#include "rbug_objects.h"

#include "rbug/rbug.h"

#include <errno.h>

#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

#define container_of(ptr, type, field) \
   (type*)((char*)ptr - offsetof(type, field))

struct rbug_rbug
{
   struct rbug_screen *rb_screen;
   struct rbug_connection *con;
   pipe_thread thread;
   boolean running;
};

PIPE_THREAD_ROUTINE(rbug_thread, void_rbug);


/**********************************************************
 * Helper functions
 */


static struct rbug_context *
rbug_get_context_locked(struct rbug_screen *rb_screen, rbug_context_t ctx)
{
   struct rbug_context *rb_context = NULL;
   struct rbug_list *ptr;

   foreach(ptr, &rb_screen->contexts) {
      rb_context = container_of(ptr, struct rbug_context, list);
      if (ctx == VOID2U64(rb_context))
         break;
      rb_context = NULL;
   }

   return rb_context;
}

static struct rbug_shader *
rbug_get_shader_locked(struct rbug_context *rb_context, rbug_shader_t shdr)
{
   struct rbug_shader *tr_shdr = NULL;
   struct rbug_list *ptr;

   foreach(ptr, &rb_context->shaders) {
      tr_shdr = container_of(ptr, struct rbug_shader, list);
      if (shdr == VOID2U64(tr_shdr))
         break;
      tr_shdr = NULL;
   }

   return tr_shdr;
}

static void *
rbug_shader_create_locked(struct pipe_context *pipe,
                          struct rbug_shader *rb_shader,
                          struct tgsi_token *tokens)
{
   void *state = NULL;
   struct pipe_shader_state pss;
   memset(&pss, 0, sizeof(pss));
   pss.tokens = tokens;

   switch(rb_shader->type) {
   case RBUG_SHADER_FRAGMENT:
      state = pipe->create_fs_state(pipe, &pss);
      break;
   case RBUG_SHADER_VERTEX:
      state = pipe->create_vs_state(pipe, &pss);
      break;
   case RBUG_SHADER_GEOM:
      state = pipe->create_gs_state(pipe, &pss);
      break;
   default:
      assert(0);
      break;
   }

   return state;
}

static void
rbug_shader_bind_locked(struct pipe_context *pipe,
                        struct rbug_shader *rb_shader,
                        void *state)
{
   switch(rb_shader->type) {
   case RBUG_SHADER_FRAGMENT:
      pipe->bind_fs_state(pipe, state);
      break;
   case RBUG_SHADER_VERTEX:
      pipe->bind_vs_state(pipe, state);
      break;
   case RBUG_SHADER_GEOM:
      pipe->bind_gs_state(pipe, state);
      break;
   default:
      assert(0);
      break;
   }
}

static void
rbug_shader_delete_locked(struct pipe_context *pipe,
                          struct rbug_shader *rb_shader,
                          void *state)
{
   switch(rb_shader->type) {
   case RBUG_SHADER_FRAGMENT:
      pipe->delete_fs_state(pipe, state);
      break;
   case RBUG_SHADER_VERTEX:
      pipe->delete_vs_state(pipe, state);
      break;
   case RBUG_SHADER_GEOM:
      pipe->delete_gs_state(pipe, state);
      break;
   default:
      assert(0);
      break;
   }
}

/************************************************
 * Request handler functions
 */


static int
rbug_texture_list(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_resource *tr_tex = NULL;
   struct rbug_list *ptr;
   rbug_texture_t *texs;
   int i = 0;

   pipe_mutex_lock(rb_screen->list_mutex);
   texs = MALLOC(rb_screen->num_resources * sizeof(rbug_texture_t));
   foreach(ptr, &rb_screen->resources) {
      tr_tex = container_of(ptr, struct rbug_resource, list);
      texs[i++] = VOID2U64(tr_tex);
   }
   pipe_mutex_unlock(rb_screen->list_mutex);

   rbug_send_texture_list_reply(tr_rbug->con, serial, texs, i, NULL);
   FREE(texs);

   return 0;
}

static int
rbug_texture_info(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_resource *tr_tex = NULL;
   struct rbug_proto_texture_info *gpti = (struct rbug_proto_texture_info *)header;
   struct rbug_list *ptr;
   struct pipe_resource *t;

   pipe_mutex_lock(rb_screen->list_mutex);
   foreach(ptr, &rb_screen->resources) {
      tr_tex = container_of(ptr, struct rbug_resource, list);
      if (gpti->texture == VOID2U64(tr_tex))
         break;
      tr_tex = NULL;
   }

   if (!tr_tex) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   t = tr_tex->resource;
   rbug_send_texture_info_reply(tr_rbug->con, serial,
                               t->target, t->format,
                               &t->width0, 1,
                               &t->height0, 1,
                               &t->depth0, 1,
                               util_format_get_blockwidth(t->format),
                               util_format_get_blockheight(t->format),
                               util_format_get_blocksize(t->format),
                               t->last_level,
                               t->nr_samples,
                               t->bind,
                               NULL);

   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_texture_read(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_texture_read *gptr = (struct rbug_proto_texture_read *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_resource *tr_tex = NULL;
   struct rbug_list *ptr;

   struct pipe_context *context = rb_screen->private_context;
   struct pipe_resource *tex;
   struct pipe_transfer *t;

   void *map;

   pipe_mutex_lock(rb_screen->list_mutex);
   foreach(ptr, &rb_screen->resources) {
      tr_tex = container_of(ptr, struct rbug_resource, list);
      if (gptr->texture == VOID2U64(tr_tex))
         break;
      tr_tex = NULL;
   }

   if (!tr_tex) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   tex = tr_tex->resource;
   t = pipe_get_transfer(context, tex,
                         gptr->level, gptr->face + gptr->zslice,
                         PIPE_TRANSFER_READ,
                         gptr->x, gptr->y, gptr->w, gptr->h);

   map = context->transfer_map(context, t);

   rbug_send_texture_read_reply(tr_rbug->con, serial,
                                t->resource->format,
                                util_format_get_blockwidth(t->resource->format),
                                util_format_get_blockheight(t->resource->format),
                                util_format_get_blocksize(t->resource->format),
                                (uint8_t*)map,
                                t->stride * util_format_get_nblocksy(t->resource->format,
                                                                     t->box.height),
                                t->stride,
                                NULL);

   context->transfer_unmap(context, t);
   context->transfer_destroy(context, t);

   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_context_list(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_list *ptr;
   struct rbug_context *rb_context = NULL;
   rbug_context_t *ctxs;
   int i = 0;

   pipe_mutex_lock(rb_screen->list_mutex);
   ctxs = MALLOC(rb_screen->num_contexts * sizeof(rbug_context_t));
   foreach(ptr, &rb_screen->contexts) {
      rb_context = container_of(ptr, struct rbug_context, list);
      ctxs[i++] = VOID2U64(rb_context);
   }
   pipe_mutex_unlock(rb_screen->list_mutex);

   rbug_send_context_list_reply(tr_rbug->con, serial, ctxs, i, NULL);
   FREE(ctxs);

   return 0;
}

static int
rbug_context_info(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_info *info = (struct rbug_proto_context_info *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;
   rbug_texture_t cbufs[PIPE_MAX_COLOR_BUFS];
   rbug_texture_t texs[PIPE_MAX_SAMPLERS];
   int i;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, info->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   /* protect the pipe context */
   pipe_mutex_lock(rb_context->draw_mutex);
   pipe_mutex_lock(rb_context->call_mutex);

   for (i = 0; i < rb_context->curr.nr_cbufs; i++)
      cbufs[i] = VOID2U64(rb_context->curr.cbufs[i]);

   /* XXX what about vertex/geometry shader texture views? */
   for (i = 0; i < rb_context->curr.num_views[PIPE_SHADER_FRAGMENT]; i++)
      texs[i] = VOID2U64(rb_context->curr.texs[PIPE_SHADER_FRAGMENT][i]);

   rbug_send_context_info_reply(tr_rbug->con, serial,
                                VOID2U64(rb_context->curr.shader[PIPE_SHADER_VERTEX]), VOID2U64(rb_context->curr.shader[PIPE_SHADER_FRAGMENT]),
                                texs, rb_context->curr.num_views[PIPE_SHADER_FRAGMENT],
                                cbufs, rb_context->curr.nr_cbufs,
                                VOID2U64(rb_context->curr.zsbuf),
                                rb_context->draw_blocker, rb_context->draw_blocked, NULL);

   pipe_mutex_unlock(rb_context->call_mutex);
   pipe_mutex_unlock(rb_context->draw_mutex);
   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_context_draw_block(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_block *block = (struct rbug_proto_context_draw_block *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, block->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->draw_mutex);
   rb_context->draw_blocker |= block->block;
   pipe_mutex_unlock(rb_context->draw_mutex);

   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_context_draw_step(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_step *step = (struct rbug_proto_context_draw_step *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, step->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->draw_mutex);
   if (rb_context->draw_blocked & RBUG_BLOCK_RULE) {
      if (step->step & RBUG_BLOCK_RULE)
         rb_context->draw_blocked &= ~RBUG_BLOCK_MASK;
   } else {
      rb_context->draw_blocked &= ~step->step;
   }
   pipe_mutex_unlock(rb_context->draw_mutex);

   pipe_condvar_broadcast(rb_context->draw_cond);

   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_context_draw_unblock(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_unblock *unblock = (struct rbug_proto_context_draw_unblock *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, unblock->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->draw_mutex);
   if (rb_context->draw_blocked & RBUG_BLOCK_RULE) {
      if (unblock->unblock & RBUG_BLOCK_RULE)
         rb_context->draw_blocked &= ~RBUG_BLOCK_MASK;
   } else {
      rb_context->draw_blocked &= ~unblock->unblock;
   }
   rb_context->draw_blocker &= ~unblock->unblock;
   pipe_mutex_unlock(rb_context->draw_mutex);

   pipe_condvar_broadcast(rb_context->draw_cond);

   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_context_draw_rule(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_rule *rule = (struct rbug_proto_context_draw_rule *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, rule->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->draw_mutex);
   rb_context->draw_rule.shader[PIPE_SHADER_VERTEX] = U642VOID(rule->vertex);
   rb_context->draw_rule.shader[PIPE_SHADER_FRAGMENT] = U642VOID(rule->fragment);
   rb_context->draw_rule.texture = U642VOID(rule->texture);
   rb_context->draw_rule.surf = U642VOID(rule->surface);
   rb_context->draw_rule.blocker = rule->block;
   rb_context->draw_blocker |= RBUG_BLOCK_RULE;
   pipe_mutex_unlock(rb_context->draw_mutex);

   pipe_condvar_broadcast(rb_context->draw_cond);

   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_context_flush(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_flush *flush = (struct rbug_proto_context_flush *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, flush->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   /* protect the pipe context */
   pipe_mutex_lock(rb_context->call_mutex);

   rb_context->pipe->flush(rb_context->pipe, NULL);

   pipe_mutex_unlock(rb_context->call_mutex);
   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_shader_list(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_shader_list *list = (struct rbug_proto_shader_list *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;
   struct rbug_shader *tr_shdr = NULL;
   struct rbug_list *ptr;
   rbug_shader_t *shdrs;
   int i = 0;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, list->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->list_mutex);
   shdrs = MALLOC(rb_context->num_shaders * sizeof(rbug_shader_t));
   foreach(ptr, &rb_context->shaders) {
      tr_shdr = container_of(ptr, struct rbug_shader, list);
      shdrs[i++] = VOID2U64(tr_shdr);
   }

   pipe_mutex_unlock(rb_context->list_mutex);
   pipe_mutex_unlock(rb_screen->list_mutex);

   rbug_send_shader_list_reply(tr_rbug->con, serial, shdrs, i, NULL);
   FREE(shdrs);

   return 0;
}

static int
rbug_shader_info(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_shader_info *info = (struct rbug_proto_shader_info *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;
   struct rbug_shader *tr_shdr = NULL;
   unsigned original_len;
   unsigned replaced_len;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, info->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->list_mutex);

   tr_shdr = rbug_get_shader_locked(rb_context, info->shader);

   if (!tr_shdr) {
      pipe_mutex_unlock(rb_context->list_mutex);
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   /* just in case */
   assert(sizeof(struct tgsi_token) == 4);

   original_len = tgsi_num_tokens(tr_shdr->tokens);
   if (tr_shdr->replaced_tokens)
      replaced_len = tgsi_num_tokens(tr_shdr->replaced_tokens);
   else
      replaced_len = 0;

   rbug_send_shader_info_reply(tr_rbug->con, serial,
                               (uint32_t*)tr_shdr->tokens, original_len,
                               (uint32_t*)tr_shdr->replaced_tokens, replaced_len,
                               tr_shdr->disabled,
                               NULL);

   pipe_mutex_unlock(rb_context->list_mutex);
   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_shader_disable(struct rbug_rbug *tr_rbug, struct rbug_header *header)
{
   struct rbug_proto_shader_disable *dis = (struct rbug_proto_shader_disable *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;
   struct rbug_shader *tr_shdr = NULL;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, dis->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->list_mutex);

   tr_shdr = rbug_get_shader_locked(rb_context, dis->shader);

   if (!tr_shdr) {
      pipe_mutex_unlock(rb_context->list_mutex);
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   tr_shdr->disabled = dis->disable;

   pipe_mutex_unlock(rb_context->list_mutex);
   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;
}

static int
rbug_shader_replace(struct rbug_rbug *tr_rbug, struct rbug_header *header)
{
   struct rbug_proto_shader_replace *rep = (struct rbug_proto_shader_replace *)header;

   struct rbug_screen *rb_screen = tr_rbug->rb_screen;
   struct rbug_context *rb_context = NULL;
   struct rbug_shader *tr_shdr = NULL;
   struct pipe_context *pipe = NULL;
   void *state;

   pipe_mutex_lock(rb_screen->list_mutex);
   rb_context = rbug_get_context_locked(rb_screen, rep->context);

   if (!rb_context) {
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(rb_context->list_mutex);

   tr_shdr = rbug_get_shader_locked(rb_context, rep->shader);

   if (!tr_shdr) {
      pipe_mutex_unlock(rb_context->list_mutex);
      pipe_mutex_unlock(rb_screen->list_mutex);
      return -ESRCH;
   }

   /* protect the pipe context */
   pipe_mutex_lock(rb_context->call_mutex);

   pipe = rb_context->pipe;

   /* remove old replaced shader */
   if (tr_shdr->replaced_shader) {
      /* if this shader is bound rebind the original shader */
      if (rb_context->curr.shader[PIPE_SHADER_FRAGMENT] == tr_shdr || rb_context->curr.shader[PIPE_SHADER_VERTEX] == tr_shdr)
         rbug_shader_bind_locked(pipe, tr_shdr, tr_shdr->shader);

      FREE(tr_shdr->replaced_tokens);
      rbug_shader_delete_locked(pipe, tr_shdr, tr_shdr->replaced_shader);
      tr_shdr->replaced_shader = NULL;
      tr_shdr->replaced_tokens = NULL;
   }

   /* empty inputs means restore old which we did above */
   if (rep->tokens_len == 0)
      goto out;

   tr_shdr->replaced_tokens = tgsi_dup_tokens((struct tgsi_token *)rep->tokens);
   if (!tr_shdr->replaced_tokens)
      goto err;

   state = rbug_shader_create_locked(pipe, tr_shdr, tr_shdr->replaced_tokens);
   if (!state)
      goto err;

   /* bind new shader if the shader is currently a bound */
   if (rb_context->curr.shader[PIPE_SHADER_FRAGMENT] == tr_shdr || rb_context->curr.shader[PIPE_SHADER_VERTEX] == tr_shdr)
      rbug_shader_bind_locked(pipe, tr_shdr, state);

   /* save state */
   tr_shdr->replaced_shader = state;

out:
   pipe_mutex_unlock(rb_context->call_mutex);
   pipe_mutex_unlock(rb_context->list_mutex);
   pipe_mutex_unlock(rb_screen->list_mutex);

   return 0;

err:
   FREE(tr_shdr->replaced_tokens);
   tr_shdr->replaced_shader = NULL;
   tr_shdr->replaced_tokens = NULL;

   pipe_mutex_unlock(rb_context->call_mutex);
   pipe_mutex_unlock(rb_context->list_mutex);
   pipe_mutex_unlock(rb_screen->list_mutex);
   return -EINVAL;
}

static boolean
rbug_header(struct rbug_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   int ret = 0;

   switch(header->opcode) {
      case RBUG_OP_PING:
         rbug_send_ping_reply(tr_rbug->con, serial, NULL);
         break;
      case RBUG_OP_TEXTURE_LIST:
         ret = rbug_texture_list(tr_rbug, header, serial);
         break;
      case RBUG_OP_TEXTURE_INFO:
         ret = rbug_texture_info(tr_rbug, header, serial);
         break;
      case RBUG_OP_TEXTURE_READ:
         ret = rbug_texture_read(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_LIST:
         ret = rbug_context_list(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_INFO:
         ret = rbug_context_info(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_BLOCK:
         ret = rbug_context_draw_block(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_STEP:
         ret = rbug_context_draw_step(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_UNBLOCK:
         ret = rbug_context_draw_unblock(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_RULE:
         ret = rbug_context_draw_rule(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_FLUSH:
         ret = rbug_context_flush(tr_rbug, header, serial);
         break;
      case RBUG_OP_SHADER_LIST:
         ret = rbug_shader_list(tr_rbug, header, serial);
         break;
      case RBUG_OP_SHADER_INFO:
         ret = rbug_shader_info(tr_rbug, header, serial);
         break;
      case RBUG_OP_SHADER_DISABLE:
         ret = rbug_shader_disable(tr_rbug, header);
         break;
      case RBUG_OP_SHADER_REPLACE:
         ret = rbug_shader_replace(tr_rbug, header);
         break;
      default:
         debug_printf("%s - unsupported opcode %u\n", __FUNCTION__, header->opcode);
         ret = -ENOSYS;
         break;
   }
   rbug_free_header(header);

   if (ret)
      rbug_send_error_reply(tr_rbug->con, serial, ret, NULL);

   return TRUE;
}

static void
rbug_con(struct rbug_rbug *tr_rbug)
{
   struct rbug_header *header;
   uint32_t serial;

   debug_printf("%s - connection received\n", __FUNCTION__);

   while(tr_rbug->running) {
      header = rbug_get_message(tr_rbug->con, &serial);
      if (!header)
         break;

      if (!rbug_header(tr_rbug, header, serial))
         break;
   }

   debug_printf("%s - connection closed\n", __FUNCTION__);

   rbug_disconnect(tr_rbug->con);
   tr_rbug->con = NULL;
}

PIPE_THREAD_ROUTINE(rbug_thread, void_tr_rbug)
{
   struct rbug_rbug *tr_rbug = void_tr_rbug;
   uint16_t port = 13370;
   int s = -1;
   int c;

   u_socket_init();

   for (;port <= 13379 && s < 0; port++)
      s = u_socket_listen_on_port(port);

   if (s < 0) {
      debug_printf("rbug_rbug - failed to listen\n");
      return NULL;
   }

   u_socket_block(s, false);

   debug_printf("rbug_rbug - remote debugging listening on port %u\n", --port);

   while(tr_rbug->running) {
      os_time_sleep(1);

      c = u_socket_accept(s);
      if (c < 0)
         continue;

      u_socket_block(c, true);
      tr_rbug->con = rbug_from_socket(c);

      rbug_con(tr_rbug);

      u_socket_close(c);
   }

   u_socket_close(s);

   u_socket_stop();

   return NULL;
}

/**********************************************************
 *
 */

struct rbug_rbug *
rbug_start(struct rbug_screen *rb_screen)
{
   struct rbug_rbug *tr_rbug = CALLOC_STRUCT(rbug_rbug);
   if (!tr_rbug)
      return NULL;

   tr_rbug->rb_screen = rb_screen;
   tr_rbug->running = TRUE;
   tr_rbug->thread = pipe_thread_create(rbug_thread, tr_rbug);

   return tr_rbug;
}

void
rbug_stop(struct rbug_rbug *tr_rbug)
{
   if (!tr_rbug)
      return;

   tr_rbug->running = false;
   pipe_thread_wait(tr_rbug->thread);

   FREE(tr_rbug);

   return;
}

void
rbug_notify_draw_blocked(struct rbug_context *rb_context)
{
   struct rbug_screen *rb_screen = rbug_screen(rb_context->base.screen);
   struct rbug_rbug *tr_rbug = rb_screen->rbug;

   if (tr_rbug && tr_rbug->con)
      rbug_send_context_draw_blocked(tr_rbug->con,
                                     VOID2U64(rb_context), rb_context->draw_blocked, NULL);
}
