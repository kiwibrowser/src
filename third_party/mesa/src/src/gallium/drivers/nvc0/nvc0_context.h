#ifndef __NVC0_CONTEXT_H__
#define __NVC0_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_dynarray.h"

#ifdef NVC0_WITH_DRAW_MODULE
#include "draw/draw_vertex.h"
#endif

#include "nv50/nv50_debug.h"
#include "nvc0_winsys.h"
#include "nvc0_stateobj.h"
#include "nvc0_screen.h"
#include "nvc0_program.h"
#include "nvc0_resource.h"

#include "nv50/nv50_transfer.h"

#include "nouveau/nouveau_context.h"

#include "nvc0_3ddefs.xml.h"
#include "nvc0_3d.xml.h"
#include "nvc0_2d.xml.h"
#include "nvc0_m2mf.xml.h"
#include "nve4_p2mf.xml.h"

/* NOTE: must keep NVC0_NEW_...PROG in consecutive bits in this order */
#define NVC0_NEW_BLEND        (1 << 0)
#define NVC0_NEW_RASTERIZER   (1 << 1)
#define NVC0_NEW_ZSA          (1 << 2)
#define NVC0_NEW_VERTPROG     (1 << 3)
#define NVC0_NEW_TCTLPROG     (1 << 4)
#define NVC0_NEW_TEVLPROG     (1 << 5)
#define NVC0_NEW_GMTYPROG     (1 << 6)
#define NVC0_NEW_FRAGPROG     (1 << 7)
#define NVC0_NEW_BLEND_COLOUR (1 << 8)
#define NVC0_NEW_STENCIL_REF  (1 << 9)
#define NVC0_NEW_CLIP         (1 << 10)
#define NVC0_NEW_SAMPLE_MASK  (1 << 11)
#define NVC0_NEW_FRAMEBUFFER  (1 << 12)
#define NVC0_NEW_STIPPLE      (1 << 13)
#define NVC0_NEW_SCISSOR      (1 << 14)
#define NVC0_NEW_VIEWPORT     (1 << 15)
#define NVC0_NEW_ARRAYS       (1 << 16)
#define NVC0_NEW_VERTEX       (1 << 17)
#define NVC0_NEW_CONSTBUF     (1 << 18)
#define NVC0_NEW_TEXTURES     (1 << 19)
#define NVC0_NEW_SAMPLERS     (1 << 20)
#define NVC0_NEW_TFB_TARGETS  (1 << 21)
#define NVC0_NEW_IDXBUF       (1 << 22)

#define NVC0_BIND_FB            0
#define NVC0_BIND_VTX           1
#define NVC0_BIND_VTX_TMP       2
#define NVC0_BIND_IDX           3
#define NVC0_BIND_TEX(s, i)  (  4 + 32 * (s) + (i))
#define NVC0_BIND_CB(s, i)   (164 + 16 * (s) + (i))
#define NVC0_BIND_TFB         244
#define NVC0_BIND_SCREEN      245
#define NVC0_BIND_TLS         246
#define NVC0_BIND_COUNT       247

#define NVC0_BIND_2D            0
#define NVC0_BIND_M2MF          0
#define NVC0_BIND_FENCE         1

struct nvc0_context {
   struct nouveau_context base;

   struct nouveau_bufctx *bufctx_3d;
   struct nouveau_bufctx *bufctx;

   struct nvc0_screen *screen;

   void (*m2mf_copy_rect)(struct nvc0_context *,
                          const struct nv50_m2mf_rect *dst,
                          const struct nv50_m2mf_rect *src,
                          uint32_t nblocksx, uint32_t nblocksy);

   uint32_t dirty;

   struct {
      boolean flushed;
      boolean rasterizer_discard;
      boolean early_z_forced;
      boolean prim_restart;
      uint32_t instance_elts; /* bitmask of per-instance elements */
      uint32_t instance_base;
      uint32_t constant_vbos;
      uint32_t constant_elts;
      int32_t index_bias;
      uint16_t scissor;
      uint8_t vbo_mode; /* 0 = normal, 1 = translate, 3 = translate, forced */
      uint8_t num_vtxbufs;
      uint8_t num_vtxelts;
      uint8_t num_textures[5];
      uint8_t num_samplers[5];
      uint8_t tls_required; /* bitmask of shader types using l[] */
      uint8_t c14_bound; /* whether immediate array constbuf is bound */
      uint8_t clip_enable;
      uint32_t clip_mode;
      uint32_t uniform_buffer_bound[5];
      struct nvc0_transform_feedback_state *tfb;
   } state;

   struct nvc0_blend_stateobj *blend;
   struct nvc0_rasterizer_stateobj *rast;
   struct nvc0_zsa_stateobj *zsa;
   struct nvc0_vertex_stateobj *vertex;

   struct nvc0_program *vertprog;
   struct nvc0_program *tctlprog;
   struct nvc0_program *tevlprog;
   struct nvc0_program *gmtyprog;
   struct nvc0_program *fragprog;

   struct nvc0_constbuf constbuf[5][NVC0_MAX_PIPE_CONSTBUFS];
   uint16_t constbuf_dirty[5];

   struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
   unsigned num_vtxbufs;
   struct pipe_index_buffer idxbuf;
   uint32_t constant_vbos;
   uint32_t vbo_user; /* bitmask of vertex buffers pointing to user memory */
   uint32_t vb_elt_first; /* from pipe_draw_info, for vertex upload */
   uint32_t vb_elt_limit; /* max - min element (count - 1) */
   uint32_t instance_off; /* current base vertex for instanced arrays */
   uint32_t instance_max; /* last instance for current draw call */

   struct pipe_sampler_view *textures[5][PIPE_MAX_SAMPLERS];
   unsigned num_textures[5];
   uint32_t textures_dirty[5];
   struct nv50_tsc_entry *samplers[5][PIPE_MAX_SAMPLERS];
   unsigned num_samplers[5];
   uint16_t samplers_dirty[5];

   uint32_t tex_handles[5][PIPE_MAX_SAMPLERS]; /* for nve4 */

   struct pipe_framebuffer_state framebuffer;
   struct pipe_blend_color blend_colour;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_poly_stipple stipple;
   struct pipe_scissor_state scissor;
   struct pipe_viewport_state viewport;
   struct pipe_clip_state clip;

   unsigned sample_mask;

   boolean vbo_push_hint;

   uint8_t tfbbuf_dirty;
   struct pipe_stream_output_target *tfbbuf[4];
   unsigned num_tfbbufs;

#ifdef NVC0_WITH_DRAW_MODULE
   struct draw_context *draw;
#endif
};

static INLINE struct nvc0_context *
nvc0_context(struct pipe_context *pipe)
{
   return (struct nvc0_context *)pipe;
}

static INLINE unsigned
nvc0_shader_stage(unsigned pipe)
{
   switch (pipe) {
   case PIPE_SHADER_VERTEX: return 0;
/* case PIPE_SHADER_TESSELLATION_CONTROL: return 1; */
/* case PIPE_SHADER_TESSELLATION_EVALUATION: return 2; */
   case PIPE_SHADER_GEOMETRY: return 3;
   case PIPE_SHADER_FRAGMENT: return 4;
   case PIPE_SHADER_COMPUTE: return 5;
   default:
      assert(!"invalid PIPE_SHADER type");
      return 0;
   }
}


/* nvc0_context.c */
struct pipe_context *nvc0_create(struct pipe_screen *, void *);
void nvc0_bufctx_fence(struct nvc0_context *, struct nouveau_bufctx *,
                       boolean on_flush);
void nvc0_default_kick_notify(struct nouveau_pushbuf *);

/* nvc0_draw.c */
extern struct draw_stage *nvc0_draw_render_stage(struct nvc0_context *);

/* nvc0_program.c */
boolean nvc0_program_translate(struct nvc0_program *, uint16_t chipset);
boolean nvc0_program_upload_code(struct nvc0_context *, struct nvc0_program *);
void nvc0_program_destroy(struct nvc0_context *, struct nvc0_program *);
void nvc0_program_library_upload(struct nvc0_context *);

/* nvc0_query.c */
void nvc0_init_query_functions(struct nvc0_context *);
void nvc0_query_pushbuf_submit(struct nouveau_pushbuf *,
                               struct pipe_query *, unsigned result_offset);
void nvc0_query_fifo_wait(struct nouveau_pushbuf *, struct pipe_query *);
void nvc0_so_target_save_offset(struct pipe_context *,
                                struct pipe_stream_output_target *, unsigned i,
                                boolean *serialize);

#define NVC0_QUERY_TFB_BUFFER_OFFSET (PIPE_QUERY_TYPES + 0)

/* nvc0_shader_state.c */
void nvc0_vertprog_validate(struct nvc0_context *);
void nvc0_tctlprog_validate(struct nvc0_context *);
void nvc0_tevlprog_validate(struct nvc0_context *);
void nvc0_gmtyprog_validate(struct nvc0_context *);
void nvc0_fragprog_validate(struct nvc0_context *);

void nvc0_tfb_validate(struct nvc0_context *);

/* nvc0_state.c */
extern void nvc0_init_state_functions(struct nvc0_context *);

/* nvc0_state_validate.c */
extern boolean nvc0_state_validate(struct nvc0_context *, uint32_t state_mask,
                                   unsigned space_words);

/* nvc0_surface.c */
extern void nvc0_clear(struct pipe_context *, unsigned buffers,
                       const union pipe_color_union *color,
                       double depth, unsigned stencil);
extern void nvc0_init_surface_functions(struct nvc0_context *);

/* nvc0_tex.c */
void nvc0_validate_textures(struct nvc0_context *);
void nvc0_validate_samplers(struct nvc0_context *);
void nve4_set_tex_handles(struct nvc0_context *);

struct pipe_sampler_view *
nvc0_create_sampler_view(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_sampler_view *);

/* nvc0_transfer.c */
void
nvc0_init_transfer_functions(struct nvc0_context *);

void
nvc0_m2mf_push_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned offset, unsigned domain,
                      unsigned size, const void *data);
void
nve4_p2mf_push_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned offset, unsigned domain,
                      unsigned size, const void *data);
void
nvc0_cb_push(struct nouveau_context *,
             struct nouveau_bo *bo, unsigned domain,
             unsigned base, unsigned size,
             unsigned offset, unsigned words, const uint32_t *data);

/* nvc0_vbo.c */
void nvc0_draw_vbo(struct pipe_context *, const struct pipe_draw_info *);

void *
nvc0_vertex_state_create(struct pipe_context *pipe,
                         unsigned num_elements,
                         const struct pipe_vertex_element *elements);
void
nvc0_vertex_state_delete(struct pipe_context *pipe, void *hwcso);

void nvc0_vertex_arrays_validate(struct nvc0_context *);

void nvc0_idxbuf_validate(struct nvc0_context *);

/* nvc0_push.c */
void nvc0_push_vbo(struct nvc0_context *, const struct pipe_draw_info *);

#endif
