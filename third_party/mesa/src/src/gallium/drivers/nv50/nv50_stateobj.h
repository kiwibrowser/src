
#ifndef __NV50_STATEOBJ_H__
#define __NV50_STATEOBJ_H__

#include "pipe/p_state.h"

#define NV50_SCISSORS_CLIPPING

#define SB_BEGIN_3D(so, m, s) \
   (so)->state[(so)->size++] = NV50_FIFO_PKHDR(NV50_3D(m), s)

#define SB_BEGIN_3D_(so, m, s) \
   (so)->state[(so)->size++] = NV50_FIFO_PKHDR(SUBC_3D(m), s)

#define SB_DATA(so, u) (so)->state[(so)->size++] = (u)

#include "nv50_stateobj_tex.h"

struct nv50_blend_stateobj {
   struct pipe_blend_state pipe;
   int size;
   uint32_t state[84]; // TODO: allocate less if !independent_blend_enable
};

struct nv50_rasterizer_stateobj {
   struct pipe_rasterizer_state pipe;
   int size;
   uint32_t state[48];
};

struct nv50_zsa_stateobj {
   struct pipe_depth_stencil_alpha_state pipe;
   int size;
   uint32_t state[29];
};

struct nv50_constbuf {
   union {
      struct pipe_resource *buf;
      const uint8_t *data;
   } u;
   uint32_t size; /* max 65536 */
   uint16_t offset;
   boolean user; /* should only be TRUE if u.data is valid and non-NULL */
};

struct nv50_vertex_element {
   struct pipe_vertex_element pipe;
   uint32_t state;
};

struct nv50_vertex_stateobj {
   uint32_t min_instance_div[PIPE_MAX_ATTRIBS];
   uint16_t vb_access_size[PIPE_MAX_ATTRIBS];
   struct translate *translate;
   unsigned num_elements;
   uint32_t instance_elts;
   uint32_t instance_bufs;
   boolean need_conversion;
   unsigned vertex_size;
   unsigned packet_vertex_limit;
   struct nv50_vertex_element element[0];
};

struct nv50_so_target {
   struct pipe_stream_output_target pipe;
   struct pipe_query *pq;
   unsigned stride;
   boolean clean;
};

static INLINE struct nv50_so_target *
nv50_so_target(struct pipe_stream_output_target *ptarg)
{
   return (struct nv50_so_target *)ptarg;
}

#endif
