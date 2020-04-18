
#ifndef __NVC0_PROGRAM_H__
#define __NVC0_PROGRAM_H__

#include "pipe/p_state.h"

#define NVC0_CAP_MAX_PROGRAM_TEMPS 128


struct nvc0_transform_feedback_state {
   uint32_t stride[4];
   uint8_t varying_count[4];
   uint8_t varying_index[4][128];
};


#define NVC0_SHADER_HEADER_SIZE (20 * 4)

struct nvc0_program {
   struct pipe_shader_state pipe;

   ubyte type;
   boolean translated;
   boolean need_tls;
   uint8_t max_gpr;

   uint32_t *code;
   uint32_t *immd_data;
   unsigned code_base;
   unsigned code_size;
   unsigned immd_base;
   unsigned immd_size; /* size of immediate array data */
   unsigned parm_size; /* size of non-bindable uniforms (c0[]) */

   uint32_t hdr[20];
   uint32_t flags[2];

   struct {
      uint32_t clip_mode; /* clip/cull selection */
      uint8_t clip_enable; /* mask of defined clip planes */
      uint8_t num_ucps; /* also set to max if ClipDistance is used */
      uint8_t edgeflag; /* attribute index of edgeflag input */
      boolean need_vertex_id;
   } vp;
   struct {
      uint8_t early_z;
      uint8_t in_pos[PIPE_MAX_SHADER_INPUTS];
   } fp;
   struct {
      uint32_t tess_mode; /* ~0 if defined by the other stage */
      uint32_t input_patch_size;
   } tp;

   void *relocs;

   struct nvc0_transform_feedback_state *tfb;

   struct nouveau_heap *mem;
};

#endif
