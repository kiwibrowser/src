
#ifndef __NVC0_STATEOBJ_H__
#define __NVC0_STATEOBJ_H__

#include "pipe/p_state.h"

#define SB_BEGIN_3D(so, m, s)                                                  \
   (so)->state[(so)->size++] = NVC0_FIFO_PKHDR_SQ(NVC0_3D(m), s)

#define SB_IMMED_3D(so, m, d)                                                  \
   (so)->state[(so)->size++] = NVC0_FIFO_PKHDR_IL(NVC0_3D(m), d)

#define SB_DATA(so, u) (so)->state[(so)->size++] = (u)

#include "nv50/nv50_stateobj_tex.h"

struct nvc0_blend_stateobj {
   struct pipe_blend_state pipe;
   int size;
   uint32_t state[72];
};

struct nvc0_rasterizer_stateobj {
   struct pipe_rasterizer_state pipe;
   int size;
   uint32_t state[43];
};

struct nvc0_zsa_stateobj {
   struct pipe_depth_stencil_alpha_state pipe;
   int size;
   uint32_t state[26];
};

struct nvc0_constbuf {
   union {
      struct pipe_resource *buf;
      const void *data;
   } u;
   uint32_t size;
   uint16_t offset;
   boolean user; /* should only be TRUE if u.data is valid and non-NULL */
};

struct nvc0_vertex_element {
   struct pipe_vertex_element pipe;
   uint32_t state;
   uint32_t state_alt; /* buffer 0 and with source offset (for translate) */
};

struct nvc0_vertex_stateobj {
   uint32_t min_instance_div[PIPE_MAX_ATTRIBS];
   uint16_t vb_access_size[PIPE_MAX_ATTRIBS];
   struct translate *translate;
   unsigned num_elements;
   uint32_t instance_elts;
   uint32_t instance_bufs;
   boolean shared_slots;
   boolean need_conversion; /* e.g. VFETCH cannot convert f64 to f32 */
   unsigned size; /* size of vertex in bytes (when packed) */
   struct nvc0_vertex_element element[0];
};

struct nvc0_so_target {
   struct pipe_stream_output_target pipe;
   struct pipe_query *pq;
   unsigned stride;
   boolean clean;
};

static INLINE struct nvc0_so_target *
nvc0_so_target(struct pipe_stream_output_target *ptarg)
{
   return (struct nvc0_so_target *)ptarg;
}

#endif
