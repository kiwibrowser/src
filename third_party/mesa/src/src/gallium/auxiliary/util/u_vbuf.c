/**************************************************************************
 *
 * Copyright 2011 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_vbuf.h"

#include "util/u_dump.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"
#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_hash.h"

struct u_vbuf_elements {
   unsigned count;
   struct pipe_vertex_element ve[PIPE_MAX_ATTRIBS];

   unsigned src_format_size[PIPE_MAX_ATTRIBS];

   /* If (velem[i].src_format != native_format[i]), the vertex buffer
    * referenced by the vertex element cannot be used for rendering and
    * its vertex data must be translated to native_format[i]. */
   enum pipe_format native_format[PIPE_MAX_ATTRIBS];
   unsigned native_format_size[PIPE_MAX_ATTRIBS];

   /* This might mean two things:
    * - src_format != native_format, as discussed above.
    * - src_offset % 4 != 0 (if the caps don't allow such an offset). */
   uint32_t incompatible_elem_mask; /* each bit describes a corresp. attrib  */
   /* Which buffer has at least one vertex element referencing it
    * incompatible. */
   uint32_t incompatible_vb_mask_any;
   /* Which buffer has all vertex elements referencing it incompatible. */
   uint32_t incompatible_vb_mask_all;
   /* Which buffer has at least one vertex element referencing it
    * compatible. */
   uint32_t compatible_vb_mask_any;
   /* Which buffer has all vertex elements referencing it compatible. */
   uint32_t compatible_vb_mask_all;

   /* Which buffer has at least one vertex element referencing it
    * non-instanced. */
   uint32_t noninstance_vb_mask_any;

   void *driver_cso;
};

enum {
   VB_VERTEX = 0,
   VB_INSTANCE = 1,
   VB_CONST = 2,
   VB_NUM = 3
};

struct u_vbuf {
   struct u_vbuf_caps caps;

   struct pipe_context *pipe;
   struct translate_cache *translate_cache;
   struct cso_cache *cso_cache;
   struct u_upload_mgr *uploader;

   /* This is what was set in set_vertex_buffers.
    * May contain user buffers. */
   struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
   unsigned nr_vertex_buffers;

   /* Saved vertex buffers. */
   struct pipe_vertex_buffer vertex_buffer_saved[PIPE_MAX_ATTRIBS];
   unsigned nr_vertex_buffers_saved;

   /* Vertex buffers for the driver.
    * There are no user buffers. */
   struct pipe_vertex_buffer real_vertex_buffer[PIPE_MAX_ATTRIBS];
   int nr_real_vertex_buffers;
   boolean vertex_buffers_dirty;

   /* The index buffer. */
   struct pipe_index_buffer index_buffer;

   /* Vertex elements. */
   struct u_vbuf_elements *ve, *ve_saved;

   /* Vertex elements used for the translate fallback. */
   struct pipe_vertex_element fallback_velems[PIPE_MAX_ATTRIBS];
   /* If non-NULL, this is a vertex element state used for the translate
    * fallback and therefore used for rendering too. */
   boolean using_translate;
   /* The vertex buffer slot index where translated vertices have been
    * stored in. */
   unsigned fallback_vbs[VB_NUM];

   /* Which buffer is a user buffer. */
   uint32_t user_vb_mask; /* each bit describes a corresp. buffer */
   /* Which buffer is incompatible (unaligned). */
   uint32_t incompatible_vb_mask; /* each bit describes a corresp. buffer */
   /* Which buffer has a non-zero stride. */
   uint32_t nonzero_stride_vb_mask; /* each bit describes a corresp. buffer */
};

static void *
u_vbuf_create_vertex_elements(struct u_vbuf *mgr, unsigned count,
                              const struct pipe_vertex_element *attribs);
static void u_vbuf_delete_vertex_elements(struct u_vbuf *mgr, void *cso);


void u_vbuf_get_caps(struct pipe_screen *screen, struct u_vbuf_caps *caps)
{
   caps->format_fixed32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_FIXED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   caps->format_float16 =
      screen->is_format_supported(screen, PIPE_FORMAT_R16_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   caps->format_float64 =
      screen->is_format_supported(screen, PIPE_FORMAT_R64_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   caps->format_norm32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_UNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   caps->format_scaled32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_USCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SSCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   caps->buffer_offset_unaligned =
      !screen->get_param(screen,
                        PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY);

   caps->buffer_stride_unaligned =
      !screen->get_param(screen,
                        PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY);

   caps->velem_src_offset_unaligned =
      !screen->get_param(screen,
                        PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY);

   caps->user_vertex_buffers =
      screen->get_param(screen, PIPE_CAP_USER_VERTEX_BUFFERS);
}

struct u_vbuf *
u_vbuf_create(struct pipe_context *pipe,
              struct u_vbuf_caps *caps)
{
   struct u_vbuf *mgr = CALLOC_STRUCT(u_vbuf);

   mgr->caps = *caps;
   mgr->pipe = pipe;
   mgr->cso_cache = cso_cache_create();
   mgr->translate_cache = translate_cache_create();
   memset(mgr->fallback_vbs, ~0, sizeof(mgr->fallback_vbs));

   mgr->uploader = u_upload_create(pipe, 1024 * 1024, 4,
                                   PIPE_BIND_VERTEX_BUFFER);

   return mgr;
}

/* u_vbuf uses its own caching for vertex elements, because it needs to keep
 * its own preprocessed state per vertex element CSO. */
static struct u_vbuf_elements *
u_vbuf_set_vertex_elements_internal(struct u_vbuf *mgr, unsigned count,
                                    const struct pipe_vertex_element *states)
{
   struct pipe_context *pipe = mgr->pipe;
   unsigned key_size, hash_key;
   struct cso_hash_iter iter;
   struct u_vbuf_elements *ve;
   struct cso_velems_state velems_state;

   /* need to include the count into the stored state data too. */
   key_size = sizeof(struct pipe_vertex_element) * count + sizeof(unsigned);
   velems_state.count = count;
   memcpy(velems_state.velems, states,
          sizeof(struct pipe_vertex_element) * count);
   hash_key = cso_construct_key((void*)&velems_state, key_size);
   iter = cso_find_state_template(mgr->cso_cache, hash_key, CSO_VELEMENTS,
                                  (void*)&velems_state, key_size);

   if (cso_hash_iter_is_null(iter)) {
      struct cso_velements *cso = MALLOC_STRUCT(cso_velements);
      memcpy(&cso->state, &velems_state, key_size);
      cso->data = u_vbuf_create_vertex_elements(mgr, count, states);
      cso->delete_state = (cso_state_callback)u_vbuf_delete_vertex_elements;
      cso->context = (void*)mgr;

      iter = cso_insert_state(mgr->cso_cache, hash_key, CSO_VELEMENTS, cso);
      ve = cso->data;
   } else {
      ve = ((struct cso_velements *)cso_hash_iter_data(iter))->data;
   }

   assert(ve);

   if (ve != mgr->ve)
	   pipe->bind_vertex_elements_state(pipe, ve->driver_cso);
   return ve;
}

void u_vbuf_set_vertex_elements(struct u_vbuf *mgr, unsigned count,
                               const struct pipe_vertex_element *states)
{
   mgr->ve = u_vbuf_set_vertex_elements_internal(mgr, count, states);
}

void u_vbuf_destroy(struct u_vbuf *mgr)
{
   unsigned i;

   mgr->pipe->set_vertex_buffers(mgr->pipe, 0, NULL);

   for (i = 0; i < mgr->nr_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->vertex_buffer[i].buffer, NULL);
   }
   for (i = 0; i < mgr->nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->real_vertex_buffer[i].buffer, NULL);
   }

   translate_cache_destroy(mgr->translate_cache);
   u_upload_destroy(mgr->uploader);
   cso_cache_delete(mgr->cso_cache);
   FREE(mgr);
}

static enum pipe_error
u_vbuf_translate_buffers(struct u_vbuf *mgr, struct translate_key *key,
                         unsigned vb_mask, unsigned out_vb,
                         int start_vertex, unsigned num_vertices,
                         int start_index, unsigned num_indices, int min_index,
                         boolean unroll_indices)
{
   struct translate *tr;
   struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0};
   struct pipe_resource *out_buffer = NULL;
   uint8_t *out_map;
   unsigned out_offset, i;
   enum pipe_error err;

   /* Get a translate object. */
   tr = translate_cache_find(mgr->translate_cache, key);

   /* Map buffers we want to translate. */
   for (i = 0; i < mgr->nr_vertex_buffers; i++) {
      if (vb_mask & (1 << i)) {
         struct pipe_vertex_buffer *vb = &mgr->vertex_buffer[i];
         unsigned offset = vb->buffer_offset + vb->stride * start_vertex;
         uint8_t *map;

         if (vb->user_buffer) {
            map = (uint8_t*)vb->user_buffer + offset;
         } else {
            unsigned size = vb->stride ? num_vertices * vb->stride
                                       : sizeof(double)*4;

            if (offset+size > vb->buffer->width0) {
               size = vb->buffer->width0 - offset;
            }

            map = pipe_buffer_map_range(mgr->pipe, vb->buffer, offset, size,
                                        PIPE_TRANSFER_READ, &vb_transfer[i]);
         }

         /* Subtract min_index so that indexing with the index buffer works. */
         if (unroll_indices) {
            map -= vb->stride * min_index;
         }

         tr->set_buffer(tr, i, map, vb->stride, ~0);
      }
   }

   /* Translate. */
   if (unroll_indices) {
      struct pipe_index_buffer *ib = &mgr->index_buffer;
      struct pipe_transfer *transfer = NULL;
      unsigned offset = ib->offset + start_index * ib->index_size;
      uint8_t *map;

      assert((ib->buffer || ib->user_buffer) && ib->index_size);

      /* Create and map the output buffer. */
      err = u_upload_alloc(mgr->uploader, 0,
                           key->output_stride * num_indices,
                           &out_offset, &out_buffer,
                           (void**)&out_map);
      if (err != PIPE_OK)
         return err;

      if (ib->user_buffer) {
         map = (uint8_t*)ib->user_buffer + offset;
      } else {
         map = pipe_buffer_map_range(mgr->pipe, ib->buffer, offset,
                                     num_indices * ib->index_size,
                                     PIPE_TRANSFER_READ, &transfer);
      }

      switch (ib->index_size) {
      case 4:
         tr->run_elts(tr, (unsigned*)map, num_indices, 0, out_map);
         break;
      case 2:
         tr->run_elts16(tr, (uint16_t*)map, num_indices, 0, out_map);
         break;
      case 1:
         tr->run_elts8(tr, map, num_indices, 0, out_map);
         break;
      }

      if (transfer) {
         pipe_buffer_unmap(mgr->pipe, transfer);
      }
   } else {
      /* Create and map the output buffer. */
      err = u_upload_alloc(mgr->uploader,
                           key->output_stride * start_vertex,
                           key->output_stride * num_vertices,
                           &out_offset, &out_buffer,
                           (void**)&out_map);
      if (err != PIPE_OK)
         return err;

      out_offset -= key->output_stride * start_vertex;

      tr->run(tr, 0, num_vertices, 0, out_map);
   }

   /* Unmap all buffers. */
   for (i = 0; i < mgr->nr_vertex_buffers; i++) {
      if (vb_transfer[i]) {
         pipe_buffer_unmap(mgr->pipe, vb_transfer[i]);
      }
   }

   /* Setup the new vertex buffer. */
   mgr->real_vertex_buffer[out_vb].buffer_offset = out_offset;
   mgr->real_vertex_buffer[out_vb].stride = key->output_stride;

   /* Move the buffer reference. */
   pipe_resource_reference(
      &mgr->real_vertex_buffer[out_vb].buffer, NULL);
   mgr->real_vertex_buffer[out_vb].buffer = out_buffer;

   return PIPE_OK;
}

static boolean
u_vbuf_translate_find_free_vb_slots(struct u_vbuf *mgr,
                                    unsigned mask[VB_NUM])
{
   unsigned type;
   unsigned fallback_vbs[VB_NUM];
   /* Set the bit for each buffer which is incompatible, or isn't set. */
   uint32_t unused_vb_mask =
      mgr->ve->incompatible_vb_mask_all | mgr->incompatible_vb_mask |
      ~((1 << mgr->nr_vertex_buffers) - 1);

   memset(fallback_vbs, ~0, sizeof(fallback_vbs));

   /* Find free slots for each type if needed. */
   for (type = 0; type < VB_NUM; type++) {
      if (mask[type]) {
         uint32_t index;

         if (!unused_vb_mask) {
            /* fail, reset the number to its original value */
            mgr->nr_real_vertex_buffers = mgr->nr_vertex_buffers;
            return FALSE;
         }

         index = ffs(unused_vb_mask) - 1;
         fallback_vbs[type] = index;
         if (index >= mgr->nr_real_vertex_buffers) {
            mgr->nr_real_vertex_buffers = index + 1;
         }
         /*printf("found slot=%i for type=%i\n", index, type);*/
      }
   }

   memcpy(mgr->fallback_vbs, fallback_vbs, sizeof(fallback_vbs));
   return TRUE;
}

static boolean
u_vbuf_translate_begin(struct u_vbuf *mgr,
                       int start_vertex, unsigned num_vertices,
                       int start_instance, unsigned num_instances,
                       int start_index, unsigned num_indices, int min_index,
                       boolean unroll_indices)
{
   unsigned mask[VB_NUM] = {0};
   struct translate_key key[VB_NUM];
   unsigned elem_index[VB_NUM][PIPE_MAX_ATTRIBS]; /* ... into key.elements */
   unsigned i, type;

   int start[VB_NUM] = {
      start_vertex,     /* VERTEX */
      start_instance,   /* INSTANCE */
      0                 /* CONST */
   };

   unsigned num[VB_NUM] = {
      num_vertices,     /* VERTEX */
      num_instances,    /* INSTANCE */
      1                 /* CONST */
   };

   memset(key, 0, sizeof(key));
   memset(elem_index, ~0, sizeof(elem_index));

   /* See if there are vertex attribs of each type to translate and
    * which ones. */
   for (i = 0; i < mgr->ve->count; i++) {
      unsigned vb_index = mgr->ve->ve[i].vertex_buffer_index;

      if (!mgr->vertex_buffer[vb_index].stride) {
         if (!(mgr->ve->incompatible_elem_mask & (1 << i)) &&
             !(mgr->incompatible_vb_mask & (1 << vb_index))) {
            continue;
         }
         mask[VB_CONST] |= 1 << vb_index;
      } else if (mgr->ve->ve[i].instance_divisor) {
         if (!(mgr->ve->incompatible_elem_mask & (1 << i)) &&
             !(mgr->incompatible_vb_mask & (1 << vb_index))) {
            continue;
         }
         mask[VB_INSTANCE] |= 1 << vb_index;
      } else {
         if (!unroll_indices &&
             !(mgr->ve->incompatible_elem_mask & (1 << i)) &&
             !(mgr->incompatible_vb_mask & (1 << vb_index))) {
            continue;
         }
         mask[VB_VERTEX] |= 1 << vb_index;
      }
   }

   assert(mask[VB_VERTEX] || mask[VB_INSTANCE] || mask[VB_CONST]);

   /* Find free vertex buffer slots. */
   if (!u_vbuf_translate_find_free_vb_slots(mgr, mask)) {
      return FALSE;
   }

   /* Initialize the translate keys. */
   for (i = 0; i < mgr->ve->count; i++) {
      struct translate_key *k;
      struct translate_element *te;
      unsigned bit, vb_index = mgr->ve->ve[i].vertex_buffer_index;
      bit = 1 << vb_index;

      if (!(mgr->ve->incompatible_elem_mask & (1 << i)) &&
          !(mgr->incompatible_vb_mask & (1 << vb_index)) &&
          (!unroll_indices || !(mask[VB_VERTEX] & bit))) {
         continue;
      }

      /* Set type to what we will translate.
       * Whether vertex, instance, or constant attribs. */
      for (type = 0; type < VB_NUM; type++) {
         if (mask[type] & bit) {
            break;
         }
      }
      assert(type < VB_NUM);
      assert(translate_is_output_format_supported(mgr->ve->native_format[i]));
      /*printf("velem=%i type=%i\n", i, type);*/

      /* Add the vertex element. */
      k = &key[type];
      elem_index[type][i] = k->nr_elements;

      te = &k->element[k->nr_elements];
      te->type = TRANSLATE_ELEMENT_NORMAL;
      te->instance_divisor = 0;
      te->input_buffer = vb_index;
      te->input_format = mgr->ve->ve[i].src_format;
      te->input_offset = mgr->ve->ve[i].src_offset;
      te->output_format = mgr->ve->native_format[i];
      te->output_offset = k->output_stride;

      k->output_stride += mgr->ve->native_format_size[i];
      k->nr_elements++;
   }

   /* Translate buffers. */
   for (type = 0; type < VB_NUM; type++) {
      if (key[type].nr_elements) {
         enum pipe_error err;
         err = u_vbuf_translate_buffers(mgr, &key[type], mask[type],
                                        mgr->fallback_vbs[type],
                                        start[type], num[type],
                                        start_index, num_indices, min_index,
                                        unroll_indices && type == VB_VERTEX);
         if (err != PIPE_OK)
            return FALSE;

         /* Fixup the stride for constant attribs. */
         if (type == VB_CONST) {
            mgr->real_vertex_buffer[mgr->fallback_vbs[VB_CONST]].stride = 0;
         }
      }
   }

   /* Setup new vertex elements. */
   for (i = 0; i < mgr->ve->count; i++) {
      for (type = 0; type < VB_NUM; type++) {
         if (elem_index[type][i] < key[type].nr_elements) {
            struct translate_element *te = &key[type].element[elem_index[type][i]];
            mgr->fallback_velems[i].instance_divisor = mgr->ve->ve[i].instance_divisor;
            mgr->fallback_velems[i].src_format = te->output_format;
            mgr->fallback_velems[i].src_offset = te->output_offset;
            mgr->fallback_velems[i].vertex_buffer_index = mgr->fallback_vbs[type];

            /* elem_index[type][i] can only be set for one type. */
            assert(type > VB_INSTANCE || elem_index[type+1][i] == ~0);
            assert(type > VB_VERTEX   || elem_index[type+2][i] == ~0);
            break;
         }
      }
      /* No translating, just copy the original vertex element over. */
      if (type == VB_NUM) {
         memcpy(&mgr->fallback_velems[i], &mgr->ve->ve[i],
                sizeof(struct pipe_vertex_element));
      }
   }

   u_vbuf_set_vertex_elements_internal(mgr, mgr->ve->count,
                                       mgr->fallback_velems);
   mgr->using_translate = TRUE;
   return TRUE;
}

static void u_vbuf_translate_end(struct u_vbuf *mgr)
{
   unsigned i;

   /* Restore vertex elements. */
   mgr->pipe->bind_vertex_elements_state(mgr->pipe, mgr->ve->driver_cso);
   mgr->using_translate = FALSE;

   /* Unreference the now-unused VBOs. */
   for (i = 0; i < VB_NUM; i++) {
      unsigned vb = mgr->fallback_vbs[i];
      if (vb != ~0) {
         pipe_resource_reference(&mgr->real_vertex_buffer[vb].buffer, NULL);
         mgr->fallback_vbs[i] = ~0;
      }
   }
   mgr->nr_real_vertex_buffers = mgr->nr_vertex_buffers;
}

#define FORMAT_REPLACE(what, withwhat) \
    case PIPE_FORMAT_##what: format = PIPE_FORMAT_##withwhat; break

static void *
u_vbuf_create_vertex_elements(struct u_vbuf *mgr, unsigned count,
                              const struct pipe_vertex_element *attribs)
{
   struct pipe_context *pipe = mgr->pipe;
   unsigned i;
   struct pipe_vertex_element driver_attribs[PIPE_MAX_ATTRIBS];
   struct u_vbuf_elements *ve = CALLOC_STRUCT(u_vbuf_elements);
   uint32_t used_buffers = 0;

   ve->count = count;

   memcpy(ve->ve, attribs, sizeof(struct pipe_vertex_element) * count);
   memcpy(driver_attribs, attribs, sizeof(struct pipe_vertex_element) * count);

   /* Set the best native format in case the original format is not
    * supported. */
   for (i = 0; i < count; i++) {
      enum pipe_format format = ve->ve[i].src_format;

      ve->src_format_size[i] = util_format_get_blocksize(format);

      used_buffers |= 1 << ve->ve[i].vertex_buffer_index;

      if (!ve->ve[i].instance_divisor) {
         ve->noninstance_vb_mask_any |= 1 << ve->ve[i].vertex_buffer_index;
      }

      /* Choose a native format.
       * For now we don't care about the alignment, that's going to
       * be sorted out later. */
      if (!mgr->caps.format_fixed32) {
         switch (format) {
            FORMAT_REPLACE(R32_FIXED,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_FIXED,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_FIXED,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_FIXED,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->caps.format_float16) {
         switch (format) {
            FORMAT_REPLACE(R16_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R16G16_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R16G16B16_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R16G16B16A16_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->caps.format_float64) {
         switch (format) {
            FORMAT_REPLACE(R64_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R64G64_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R64G64B64_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R64G64B64A64_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->caps.format_norm32) {
         switch (format) {
            FORMAT_REPLACE(R32_UNORM,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_UNORM,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_UNORM,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_UNORM,  R32G32B32A32_FLOAT);
            FORMAT_REPLACE(R32_SNORM,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_SNORM,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_SNORM,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_SNORM,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->caps.format_scaled32) {
         switch (format) {
            FORMAT_REPLACE(R32_USCALED,         R32_FLOAT);
            FORMAT_REPLACE(R32G32_USCALED,      R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_USCALED,   R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_USCALED,R32G32B32A32_FLOAT);
            FORMAT_REPLACE(R32_SSCALED,         R32_FLOAT);
            FORMAT_REPLACE(R32G32_SSCALED,      R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_SSCALED,   R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_SSCALED,R32G32B32A32_FLOAT);
            default:;
         }
      }

      driver_attribs[i].src_format = format;
      ve->native_format[i] = format;
      ve->native_format_size[i] =
            util_format_get_blocksize(ve->native_format[i]);

      if (ve->ve[i].src_format != format ||
          (!mgr->caps.velem_src_offset_unaligned &&
           ve->ve[i].src_offset % 4 != 0)) {
         ve->incompatible_elem_mask |= 1 << i;
         ve->incompatible_vb_mask_any |= 1 << ve->ve[i].vertex_buffer_index;
      } else {
         ve->compatible_vb_mask_any |= 1 << ve->ve[i].vertex_buffer_index;
      }
   }

   ve->compatible_vb_mask_all = ~ve->incompatible_vb_mask_any & used_buffers;
   ve->incompatible_vb_mask_all = ~ve->compatible_vb_mask_any & used_buffers;

   /* Align the formats to the size of DWORD if needed. */
   if (!mgr->caps.velem_src_offset_unaligned) {
      for (i = 0; i < count; i++) {
         ve->native_format_size[i] = align(ve->native_format_size[i], 4);
      }
   }

   ve->driver_cso =
      pipe->create_vertex_elements_state(pipe, count, driver_attribs);
   return ve;
}

static void u_vbuf_delete_vertex_elements(struct u_vbuf *mgr, void *cso)
{
   struct pipe_context *pipe = mgr->pipe;
   struct u_vbuf_elements *ve = cso;

   pipe->delete_vertex_elements_state(pipe, ve->driver_cso);
   FREE(ve);
}

void u_vbuf_set_vertex_buffers(struct u_vbuf *mgr, unsigned count,
                               const struct pipe_vertex_buffer *bufs)
{
   unsigned i;

   mgr->user_vb_mask = 0;
   mgr->incompatible_vb_mask = 0;
   mgr->nonzero_stride_vb_mask = 0;

   for (i = 0; i < count; i++) {
      const struct pipe_vertex_buffer *vb = &bufs[i];
      struct pipe_vertex_buffer *orig_vb = &mgr->vertex_buffer[i];
      struct pipe_vertex_buffer *real_vb = &mgr->real_vertex_buffer[i];

      pipe_resource_reference(&orig_vb->buffer, vb->buffer);
      orig_vb->user_buffer = vb->user_buffer;

      real_vb->buffer_offset = orig_vb->buffer_offset = vb->buffer_offset;
      real_vb->stride = orig_vb->stride = vb->stride;
      real_vb->user_buffer = NULL;

      if (vb->stride) {
         mgr->nonzero_stride_vb_mask |= 1 << i;
      }

      if (!vb->buffer && !vb->user_buffer) {
         pipe_resource_reference(&real_vb->buffer, NULL);
         continue;
      }

      if ((!mgr->caps.buffer_offset_unaligned && vb->buffer_offset % 4 != 0) ||
          (!mgr->caps.buffer_stride_unaligned && vb->stride % 4 != 0)) {
         mgr->incompatible_vb_mask |= 1 << i;
         pipe_resource_reference(&real_vb->buffer, NULL);
         continue;
      }

      if (!mgr->caps.user_vertex_buffers && vb->user_buffer) {
         mgr->user_vb_mask |= 1 << i;
         pipe_resource_reference(&real_vb->buffer, NULL);
         continue;
      }

      pipe_resource_reference(&real_vb->buffer, vb->buffer);
      real_vb->user_buffer = vb->user_buffer;
   }

   for (i = count; i < mgr->nr_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->vertex_buffer[i].buffer, NULL);
   }
   for (i = count; i < mgr->nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->real_vertex_buffer[i].buffer, NULL);
   }

   mgr->nr_vertex_buffers = count;
   mgr->nr_real_vertex_buffers = count;
   mgr->vertex_buffers_dirty = TRUE;
}

void u_vbuf_set_index_buffer(struct u_vbuf *mgr,
                             const struct pipe_index_buffer *ib)
{
   struct pipe_context *pipe = mgr->pipe;

   if (ib) {
      assert(ib->offset % ib->index_size == 0);
      pipe_resource_reference(&mgr->index_buffer.buffer, ib->buffer);
      memcpy(&mgr->index_buffer, ib, sizeof(*ib));
   } else {
      pipe_resource_reference(&mgr->index_buffer.buffer, NULL);
   }

   pipe->set_index_buffer(pipe, ib);
}

static enum pipe_error
u_vbuf_upload_buffers(struct u_vbuf *mgr,
                      int start_vertex, unsigned num_vertices,
                      int start_instance, unsigned num_instances)
{
   unsigned i;
   unsigned nr_velems = mgr->ve->count;
   unsigned nr_vbufs = mgr->nr_vertex_buffers;
   struct pipe_vertex_element *velems =
         mgr->using_translate ? mgr->fallback_velems : mgr->ve->ve;
   unsigned start_offset[PIPE_MAX_ATTRIBS];
   unsigned end_offset[PIPE_MAX_ATTRIBS] = {0};

   /* Determine how much data needs to be uploaded. */
   for (i = 0; i < nr_velems; i++) {
      struct pipe_vertex_element *velem = &velems[i];
      unsigned index = velem->vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &mgr->vertex_buffer[index];
      unsigned instance_div, first, size;

      /* Skip the buffers generated by translate. */
      if (index == mgr->fallback_vbs[VB_VERTEX] ||
          index == mgr->fallback_vbs[VB_INSTANCE] ||
          index == mgr->fallback_vbs[VB_CONST]) {
         continue;
      }

      if (!vb->user_buffer) {
         continue;
      }

      instance_div = velem->instance_divisor;
      first = vb->buffer_offset + velem->src_offset;

      if (!vb->stride) {
         /* Constant attrib. */
         size = mgr->ve->src_format_size[i];
      } else if (instance_div) {
         /* Per-instance attrib. */
         unsigned count = (num_instances + instance_div - 1) / instance_div;
         first += vb->stride * start_instance;
         size = vb->stride * (count - 1) + mgr->ve->src_format_size[i];
      } else {
         /* Per-vertex attrib. */
         first += vb->stride * start_vertex;
         size = vb->stride * (num_vertices - 1) + mgr->ve->src_format_size[i];
      }

      /* Update offsets. */
      if (!end_offset[index]) {
         start_offset[index] = first;
         end_offset[index] = first + size;
      } else {
         if (first < start_offset[index])
            start_offset[index] = first;
         if (first + size > end_offset[index])
            end_offset[index] = first + size;
      }
   }

   /* Upload buffers. */
   for (i = 0; i < nr_vbufs; i++) {
      unsigned start, end = end_offset[i];
      struct pipe_vertex_buffer *real_vb;
      const uint8_t *ptr;
      enum pipe_error err;

      if (!end) {
         continue;
      }

      start = start_offset[i];
      assert(start < end);

      real_vb = &mgr->real_vertex_buffer[i];
      ptr = mgr->vertex_buffer[i].user_buffer;

      err = u_upload_data(mgr->uploader, start, end - start, ptr + start,
                          &real_vb->buffer_offset, &real_vb->buffer);
      if (err != PIPE_OK)
         return err;

      real_vb->buffer_offset -= start;
   }

   return PIPE_OK;
}

static boolean u_vbuf_need_minmax_index(struct u_vbuf *mgr)
{
   /* See if there are any per-vertex attribs which will be uploaded or
    * translated. Use bitmasks to get the info instead of looping over vertex
    * elements. */
   return ((mgr->user_vb_mask | mgr->incompatible_vb_mask |
            mgr->ve->incompatible_vb_mask_any) &
           mgr->ve->noninstance_vb_mask_any & mgr->nonzero_stride_vb_mask) != 0;
}

static boolean u_vbuf_mapping_vertex_buffer_blocks(struct u_vbuf *mgr)
{
   /* Return true if there are hw buffers which don't need to be translated.
    *
    * We could query whether each buffer is busy, but that would
    * be way more costly than this. */
   return (~mgr->user_vb_mask & ~mgr->incompatible_vb_mask &
           mgr->ve->compatible_vb_mask_all & mgr->ve->noninstance_vb_mask_any &
           mgr->nonzero_stride_vb_mask) != 0;
}

static void u_vbuf_get_minmax_index(struct pipe_context *pipe,
                                    struct pipe_index_buffer *ib,
                                    const struct pipe_draw_info *info,
                                    int *out_min_index,
                                    int *out_max_index)
{
   struct pipe_transfer *transfer = NULL;
   const void *indices;
   unsigned i;
   unsigned restart_index = info->restart_index;

   if (ib->user_buffer) {
      indices = (uint8_t*)ib->user_buffer +
                ib->offset + info->start * ib->index_size;
   } else {
      indices = pipe_buffer_map_range(pipe, ib->buffer,
                                      ib->offset + info->start * ib->index_size,
                                      info->count * ib->index_size,
                                      PIPE_TRANSFER_READ, &transfer);
   }

   switch (ib->index_size) {
   case 4: {
      const unsigned *ui_indices = (const unsigned*)indices;
      unsigned max_ui = 0;
      unsigned min_ui = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (ui_indices[i] != restart_index) {
               if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
               if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
            if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
         }
      }
      *out_min_index = min_ui;
      *out_max_index = max_ui;
      break;
   }
   case 2: {
      const unsigned short *us_indices = (const unsigned short*)indices;
      unsigned max_us = 0;
      unsigned min_us = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (us_indices[i] != restart_index) {
               if (us_indices[i] > max_us) max_us = us_indices[i];
               if (us_indices[i] < min_us) min_us = us_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (us_indices[i] > max_us) max_us = us_indices[i];
            if (us_indices[i] < min_us) min_us = us_indices[i];
         }
      }
      *out_min_index = min_us;
      *out_max_index = max_us;
      break;
   }
   case 1: {
      const unsigned char *ub_indices = (const unsigned char*)indices;
      unsigned max_ub = 0;
      unsigned min_ub = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (ub_indices[i] != restart_index) {
               if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
               if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
            if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
         }
      }
      *out_min_index = min_ub;
      *out_max_index = max_ub;
      break;
   }
   default:
      assert(0);
      *out_min_index = 0;
      *out_max_index = 0;
   }

   if (transfer) {
      pipe_buffer_unmap(pipe, transfer);
   }
}

void u_vbuf_draw_vbo(struct u_vbuf *mgr, const struct pipe_draw_info *info)
{
   struct pipe_context *pipe = mgr->pipe;
   int start_vertex, min_index;
   unsigned num_vertices;
   boolean unroll_indices = FALSE;
   uint32_t user_vb_mask = mgr->user_vb_mask;

   /* Normal draw. No fallback and no user buffers. */
   if (!mgr->incompatible_vb_mask &&
       !mgr->ve->incompatible_elem_mask &&
       !user_vb_mask) {
      /* Set vertex buffers if needed. */
      if (mgr->vertex_buffers_dirty) {
         pipe->set_vertex_buffers(pipe, mgr->nr_real_vertex_buffers,
                                  mgr->real_vertex_buffer);
         mgr->vertex_buffers_dirty = FALSE;
      }

      pipe->draw_vbo(pipe, info);
      return;
   }

   if (info->indexed) {
      /* See if anything needs to be done for per-vertex attribs. */
      if (u_vbuf_need_minmax_index(mgr)) {
         int max_index;

         if (info->max_index != ~0) {
            min_index = info->min_index;
            max_index = info->max_index;
         } else {
            u_vbuf_get_minmax_index(mgr->pipe, &mgr->index_buffer, info,
                                    &min_index, &max_index);
         }

         assert(min_index <= max_index);

         start_vertex = min_index + info->index_bias;
         num_vertices = max_index + 1 - min_index;

         /* Primitive restart doesn't work when unrolling indices.
          * We would have to break this drawing operation into several ones. */
         /* Use some heuristic to see if unrolling indices improves
          * performance. */
         if (!info->primitive_restart &&
             num_vertices > info->count*2 &&
             num_vertices-info->count > 32 &&
             !u_vbuf_mapping_vertex_buffer_blocks(mgr)) {
            /*printf("num_vertices=%i count=%i\n", num_vertices, info->count);*/
            unroll_indices = TRUE;
            user_vb_mask &= ~(mgr->nonzero_stride_vb_mask &
                              mgr->ve->noninstance_vb_mask_any);
         }
      } else {
         /* Nothing to do for per-vertex attribs. */
         start_vertex = 0;
         num_vertices = 0;
         min_index = 0;
      }
   } else {
      start_vertex = info->start;
      num_vertices = info->count;
      min_index = 0;
   }

   /* Translate vertices with non-native layouts or formats. */
   if (unroll_indices ||
       mgr->incompatible_vb_mask ||
       mgr->ve->incompatible_elem_mask) {
      if (!u_vbuf_translate_begin(mgr, start_vertex, num_vertices,
                                  info->start_instance, info->instance_count,
                                  info->start, info->count, min_index,
                                  unroll_indices)) {
         debug_warn_once("u_vbuf_translate_begin() failed");
         return;
      }

      user_vb_mask &= ~(mgr->incompatible_vb_mask |
                        mgr->ve->incompatible_vb_mask_all);
   }

   /* Upload user buffers. */
   if (user_vb_mask) {
      if (u_vbuf_upload_buffers(mgr, start_vertex, num_vertices,
                                info->start_instance,
                                info->instance_count) != PIPE_OK) {
         debug_warn_once("u_vbuf_upload_buffers() failed");
         return;
      }
   }

   /*
   if (unroll_indices) {
      printf("unrolling indices: start_vertex = %i, num_vertices = %i\n",
             start_vertex, num_vertices);
      util_dump_draw_info(stdout, info);
      printf("\n");
   }

   unsigned i;
   for (i = 0; i < mgr->nr_vertex_buffers; i++) {
      printf("input %i: ", i);
      util_dump_vertex_buffer(stdout, mgr->vertex_buffer+i);
      printf("\n");
   }
   for (i = 0; i < mgr->nr_real_vertex_buffers; i++) {
      printf("real %i: ", i);
      util_dump_vertex_buffer(stdout, mgr->real_vertex_buffer+i);
      printf("\n");
   }
   */

   u_upload_unmap(mgr->uploader);
   pipe->set_vertex_buffers(pipe, mgr->nr_real_vertex_buffers,
                            mgr->real_vertex_buffer);

   if (unlikely(unroll_indices)) {
      struct pipe_draw_info new_info = *info;
      new_info.indexed = FALSE;
      new_info.index_bias = 0;
      new_info.min_index = 0;
      new_info.max_index = info->count - 1;
      new_info.start = 0;

      pipe->draw_vbo(pipe, &new_info);
   } else {
      pipe->draw_vbo(pipe, info);
   }

   if (mgr->using_translate) {
      u_vbuf_translate_end(mgr);
   }
   mgr->vertex_buffers_dirty = TRUE;
}

void u_vbuf_save_vertex_elements(struct u_vbuf *mgr)
{
   assert(!mgr->ve_saved);
   mgr->ve_saved = mgr->ve;
}

void u_vbuf_restore_vertex_elements(struct u_vbuf *mgr)
{
   if (mgr->ve != mgr->ve_saved) {
      struct pipe_context *pipe = mgr->pipe;

      mgr->ve = mgr->ve_saved;
      pipe->bind_vertex_elements_state(pipe,
                                       mgr->ve ? mgr->ve->driver_cso : NULL);
   }
   mgr->ve_saved = NULL;
}

void u_vbuf_save_vertex_buffers(struct u_vbuf *mgr)
{
   util_copy_vertex_buffers(mgr->vertex_buffer_saved,
                            &mgr->nr_vertex_buffers_saved,
                            mgr->vertex_buffer,
                            mgr->nr_vertex_buffers);
}

void u_vbuf_restore_vertex_buffers(struct u_vbuf *mgr)
{
   unsigned i;

   u_vbuf_set_vertex_buffers(mgr, mgr->nr_vertex_buffers_saved,
                             mgr->vertex_buffer_saved);
   for (i = 0; i < mgr->nr_vertex_buffers_saved; i++) {
      pipe_resource_reference(&mgr->vertex_buffer_saved[i].buffer, NULL);
   }
   mgr->nr_vertex_buffers_saved = 0;
}
