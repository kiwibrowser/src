/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_defines.h"

#include "nvc0_context.h"

#include "nv50/codegen/nv50_ir_driver.h"

/* If only they told use the actual semantic instead of just GENERIC ... */
static void
nvc0_mesa_varying_hack(struct nv50_ir_varying *var)
{
   unsigned c;

   if (var->sn != TGSI_SEMANTIC_GENERIC)
      return;

   if (var->si <= 7) /* gl_TexCoord */
      for (c = 0; c < 4; ++c)
         var->slot[c] = (0x300 + var->si * 0x10 + c * 0x4) / 4;
   else
   if (var->si == 9) /* gl_PointCoord */
      for (c = 0; c < 4; ++c)
         var->slot[c] = (0x2e0 + c * 0x4) / 4;
   else
      for (c = 0; c < 4; ++c) /* move down user varyings (first has index 8) */
         var->slot[c] -= 0x80 / 4;
}

static uint32_t
nvc0_shader_input_address(unsigned sn, unsigned si, unsigned ubase)
{
   switch (sn) {
   case NV50_SEMANTIC_TESSFACTOR:   return 0x000 + si * 0x4;
   case TGSI_SEMANTIC_PRIMID:       return 0x060;
   case TGSI_SEMANTIC_PSIZE:        return 0x06c;
   case TGSI_SEMANTIC_POSITION:     return 0x070;
   case TGSI_SEMANTIC_GENERIC:      return ubase + si * 0x10;
   case TGSI_SEMANTIC_FOG:          return 0x270;
   case TGSI_SEMANTIC_COLOR:        return 0x280 + si * 0x10;
   case TGSI_SEMANTIC_BCOLOR:       return 0x2a0 + si * 0x10;
   case NV50_SEMANTIC_CLIPDISTANCE: return 0x2c0 + si * 0x4;
   case TGSI_SEMANTIC_CLIPDIST:     return 0x2c0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPVERTEX:   return 0x260;
   case NV50_SEMANTIC_POINTCOORD:   return 0x2e0;
   case NV50_SEMANTIC_TESSCOORD:    return 0x2f0;
   case TGSI_SEMANTIC_INSTANCEID:   return 0x2f8;
   case TGSI_SEMANTIC_VERTEXID:     return 0x2fc;
   case NV50_SEMANTIC_TEXCOORD:     return 0x300 + si * 0x10;
   case TGSI_SEMANTIC_FACE:         return 0x3fc;
   case NV50_SEMANTIC_INVOCATIONID: return ~0;
   default:
      assert(!"invalid TGSI input semantic");
      return ~0;
   }
}

static uint32_t
nvc0_shader_output_address(unsigned sn, unsigned si, unsigned ubase)
{
   switch (sn) {
   case NV50_SEMANTIC_TESSFACTOR:    return 0x000 + si * 0x4;
   case TGSI_SEMANTIC_PRIMID:        return 0x060;
   case NV50_SEMANTIC_LAYER:         return 0x064;
   case NV50_SEMANTIC_VIEWPORTINDEX: return 0x068;
   case TGSI_SEMANTIC_PSIZE:         return 0x06c;
   case TGSI_SEMANTIC_POSITION:      return 0x070;
   case TGSI_SEMANTIC_GENERIC:       return ubase + si * 0x10;
   case TGSI_SEMANTIC_FOG:           return 0x270;
   case TGSI_SEMANTIC_COLOR:         return 0x280 + si * 0x10;
   case TGSI_SEMANTIC_BCOLOR:        return 0x2a0 + si * 0x10;
   case NV50_SEMANTIC_CLIPDISTANCE:  return 0x2c0 + si * 0x4;
   case TGSI_SEMANTIC_CLIPDIST:      return 0x2c0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPVERTEX:    return 0x260;
   case NV50_SEMANTIC_TEXCOORD:      return 0x300 + si * 0x10;
   case TGSI_SEMANTIC_EDGEFLAG:      return ~0;
   default:
      assert(!"invalid TGSI output semantic");
      return ~0;
   }
}

static int
nvc0_vp_assign_input_slots(struct nv50_ir_prog_info *info)
{
   unsigned i, c, n;

   for (n = 0, i = 0; i < info->numInputs; ++i) {
      switch (info->in[i].sn) {
      case TGSI_SEMANTIC_INSTANCEID: /* for SM4 only, in TGSI they're SVs */
      case TGSI_SEMANTIC_VERTEXID:
         info->in[i].mask = 0x1;
         info->in[i].slot[0] =
            nvc0_shader_input_address(info->in[i].sn, 0, 0) / 4;
         continue;
      default:
         break;
      }
      for (c = 0; c < 4; ++c)
         info->in[i].slot[c] = (0x80 + n * 0x10 + c * 0x4) / 4;
      ++n;
   }

   return 0;
}

static int
nvc0_sp_assign_input_slots(struct nv50_ir_prog_info *info)
{
   unsigned ubase = MAX2(0x80, 0x20 + info->numPatchConstants * 0x10);
   unsigned offset;
   unsigned i, c;

   for (i = 0; i < info->numInputs; ++i) {
      offset = nvc0_shader_input_address(info->in[i].sn,
                                         info->in[i].si, ubase);
      if (info->in[i].patch && offset >= 0x20)
         offset = 0x20 + info->in[i].si * 0x10;

      if (info->in[i].sn == NV50_SEMANTIC_TESSCOORD)
         info->in[i].mask &= 3;

      for (c = 0; c < 4; ++c)
         info->in[i].slot[c] = (offset + c * 0x4) / 4;

      nvc0_mesa_varying_hack(&info->in[i]);
   }

   return 0;
}

static int
nvc0_fp_assign_output_slots(struct nv50_ir_prog_info *info)
{
   unsigned count = info->prop.fp.numColourResults * 4;
   unsigned i, c;

   for (i = 0; i < info->numOutputs; ++i)
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
         for (c = 0; c < 4; ++c)
            info->out[i].slot[c] = info->out[i].si * 4 + c;

   if (info->io.sampleMask < PIPE_MAX_SHADER_OUTPUTS)
      info->out[info->io.sampleMask].slot[0] = count++;
   else
   if (info->target >= 0xe0)
      count++; /* on Kepler, depth is always last colour reg + 2 */

   if (info->io.fragDepth < PIPE_MAX_SHADER_OUTPUTS)
      info->out[info->io.fragDepth].slot[2] = count;

   return 0;
}

static int
nvc0_sp_assign_output_slots(struct nv50_ir_prog_info *info)
{
   unsigned ubase = MAX2(0x80, 0x20 + info->numPatchConstants * 0x10);
   unsigned offset;
   unsigned i, c;

   for (i = 0; i < info->numOutputs; ++i) {
      offset = nvc0_shader_output_address(info->out[i].sn,
                                          info->out[i].si, ubase);
      if (info->out[i].patch && offset >= 0x20)
         offset = 0x20 + info->out[i].si * 0x10;

      for (c = 0; c < 4; ++c)
         info->out[i].slot[c] = (offset + c * 0x4) / 4;

      nvc0_mesa_varying_hack(&info->out[i]);
   }

   return 0;
}

static int
nvc0_program_assign_varying_slots(struct nv50_ir_prog_info *info)
{
   int ret;

   if (info->type == PIPE_SHADER_VERTEX)
      ret = nvc0_vp_assign_input_slots(info);
   else
      ret = nvc0_sp_assign_input_slots(info);
   if (ret)
      return ret;

   if (info->type == PIPE_SHADER_FRAGMENT)
      ret = nvc0_fp_assign_output_slots(info);
   else
      ret = nvc0_sp_assign_output_slots(info);
   return ret;
}

static INLINE void
nvc0_vtgp_hdr_update_oread(struct nvc0_program *vp, uint8_t slot)
{
   uint8_t min = (vp->hdr[4] >> 12) & 0xff;
   uint8_t max = (vp->hdr[4] >> 24);

   min = MIN2(min, slot);
   max = MAX2(max, slot);

   vp->hdr[4] = (max << 24) | (min << 12);
}

/* Common part of header generation for VP, TCP, TEP and GP. */
static int
nvc0_vtgp_gen_header(struct nvc0_program *vp, struct nv50_ir_prog_info *info)
{
   unsigned i, c, a;

   for (i = 0; i < info->numInputs; ++i) {
      if (info->in[i].patch)
         continue;
      for (c = 0; c < 4; ++c) {
         a = info->in[i].slot[c];
         if (info->in[i].mask & (1 << c)) {
            if (info->in[i].sn != NV50_SEMANTIC_TESSCOORD)
               vp->hdr[5 + a / 32] |= 1 << (a % 32);
            else
               nvc0_vtgp_hdr_update_oread(vp, info->in[i].slot[c]);
         }
      }
   }

   for (i = 0; i < info->numOutputs; ++i) {
      if (info->out[i].patch)
         continue;
      for (c = 0; c < 4; ++c) {
         if (!(info->out[i].mask & (1 << c)))
            continue;
         assert(info->out[i].slot[c] >= 0x40 / 4);
         a = info->out[i].slot[c] - 0x40 / 4;
         vp->hdr[13 + a / 32] |= 1 << (a % 32);
         if (info->out[i].oread)
            nvc0_vtgp_hdr_update_oread(vp, info->out[i].slot[c]);
      }
   }

   for (i = 0; i < info->numSysVals; ++i) {
      switch (info->sv[i].sn) {
      case TGSI_SEMANTIC_PRIMID:
         vp->hdr[5] |= 1 << 24;
         break;
      case TGSI_SEMANTIC_INSTANCEID:
         vp->hdr[10] |= 1 << 30;
         break;
      case TGSI_SEMANTIC_VERTEXID:
         vp->hdr[10] |= 1 << 31;
         break;
      default:
         break;
      }
   }

   vp->vp.clip_enable = info->io.clipDistanceMask;
   for (i = 0; i < 8; ++i)
      if (info->io.cullDistanceMask & (1 << i))
         vp->vp.clip_mode |= 1 << (i * 4);

   if (info->io.genUserClip < 0)
      vp->vp.num_ucps = PIPE_MAX_CLIP_PLANES + 1; /* prevent rebuilding */

   return 0;
}

static int
nvc0_vp_gen_header(struct nvc0_program *vp, struct nv50_ir_prog_info *info)
{
   vp->hdr[0] = 0x20061 | (1 << 10);
   vp->hdr[4] = 0xff000;

   vp->hdr[18] = info->io.clipDistanceMask;

   return nvc0_vtgp_gen_header(vp, info);
}

#if defined(PIPE_SHADER_HULL) || defined(PIPE_SHADER_DOMAIN)
static void
nvc0_tp_get_tess_mode(struct nvc0_program *tp, struct nv50_ir_prog_info *info)
{
   if (info->prop.tp.outputPrim == PIPE_PRIM_MAX) {
      tp->tp.tess_mode = ~0;
      return;
   }
   switch (info->prop.tp.domain) {
   case PIPE_PRIM_LINES:
      tp->tp.tess_mode = NVC0_3D_TESS_MODE_PRIM_ISOLINES;
      break;
   case PIPE_PRIM_TRIANGLES:
      tp->tp.tess_mode = NVC0_3D_TESS_MODE_PRIM_TRIANGLES;
      if (info->prop.tp.winding > 0)
         tp->tp.tess_mode |= NVC0_3D_TESS_MODE_CW;
      break;
   case PIPE_PRIM_QUADS:
      tp->tp.tess_mode = NVC0_3D_TESS_MODE_PRIM_QUADS;
      break;
   default:
      tp->tp.tess_mode = ~0;
      return;
   }
   if (info->prop.tp.outputPrim != PIPE_PRIM_POINTS)
      tp->tp.tess_mode |= NVC0_3D_TESS_MODE_CONNECTED;

   switch (info->prop.tp.partitioning) {
   case PIPE_TESS_PART_INTEGER:
   case PIPE_TESS_PART_POW2:
      tp->tp.tess_mode |= NVC0_3D_TESS_MODE_SPACING_EQUAL;
      break;
   case PIPE_TESS_PART_FRACT_ODD:
      tp->tp.tess_mode |= NVC0_3D_TESS_MODE_SPACING_FRACTIONAL_ODD;
      break;
   case PIPE_TESS_PART_FRACT_EVEN:
      tp->tp.tess_mode |= NVC0_3D_TESS_MODE_SPACING_FRACTIONAL_EVEN;
      break;
   default:
      assert(!"invalid tessellator partitioning");
      break;
   }
}
#endif

#ifdef PIPE_SHADER_HULL
static int
nvc0_tcp_gen_header(struct nvc0_program *tcp, struct nv50_ir_prog_info *info)
{
   unsigned opcs = 6; /* output patch constants (at least the TessFactors) */

   tcp->tp.input_patch_size = info->prop.tp.inputPatchSize;

   if (info->numPatchConstants)
      opcs = 8 + info->numPatchConstants * 4;

   tcp->hdr[0] = 0x20061 | (2 << 10);

   tcp->hdr[1] = opcs << 24;
   tcp->hdr[2] = info->prop.tp.outputPatchSize << 24;

   tcp->hdr[4] = 0xff000; /* initial min/max parallel output read address */

   nvc0_vtgp_gen_header(tcp, info);

   nvc0_tp_get_tess_mode(tcp, info);

   return 0;
}
#endif

#ifdef PIPE_SHADER_DOMAIN
static int
nvc0_tep_gen_header(struct nvc0_program *tep, struct nv50_ir_prog_info *info)
{
   tep->tp.input_patch_size = ~0;

   tep->hdr[0] = 0x20061 | (3 << 10);
   tep->hdr[4] = 0xff000;

   nvc0_vtgp_gen_header(tep, info);

   nvc0_tp_get_tess_mode(tep, info);

   tep->hdr[18] |= 0x3 << 12; /* ? */

   return 0;
}
#endif

static int
nvc0_gp_gen_header(struct nvc0_program *gp, struct nv50_ir_prog_info *info)
{
   gp->hdr[0] = 0x20061 | (4 << 10);

   gp->hdr[2] = MIN2(info->prop.gp.instanceCount, 32) << 24;

   switch (info->prop.gp.outputPrim) {
   case PIPE_PRIM_POINTS:
      gp->hdr[3] = 0x01000000;
      gp->hdr[0] |= 0xf0000000;
      break;
   case PIPE_PRIM_LINE_STRIP:
      gp->hdr[3] = 0x06000000;
      gp->hdr[0] |= 0x10000000;
      break;
   case PIPE_PRIM_TRIANGLE_STRIP:
      gp->hdr[3] = 0x07000000;
      gp->hdr[0] |= 0x10000000;
      break;
   default:
      assert(0);
      break;
   }

   gp->hdr[4] = info->prop.gp.maxVertices & 0x1ff;

   return nvc0_vtgp_gen_header(gp, info);
}

#define NVC0_INTERP_FLAT          (1 << 0)
#define NVC0_INTERP_PERSPECTIVE   (2 << 0)
#define NVC0_INTERP_LINEAR        (3 << 0)
#define NVC0_INTERP_CENTROID      (1 << 2)

static uint8_t
nvc0_hdr_interp_mode(const struct nv50_ir_varying *var)
{
   if (var->linear)
      return NVC0_INTERP_LINEAR;
   if (var->flat)
      return NVC0_INTERP_FLAT;
   return NVC0_INTERP_PERSPECTIVE;
}

static int
nvc0_fp_gen_header(struct nvc0_program *fp, struct nv50_ir_prog_info *info)
{
   unsigned i, c, a, m;

   /* just 00062 on Kepler */
   fp->hdr[0] = 0x20062 | (5 << 10);
   fp->hdr[5] = 0x80000000; /* getting a trap if FRAG_COORD_UMASK.w = 0 */

   if (info->prop.fp.usesDiscard)
      fp->hdr[0] |= 0x8000;
   if (info->prop.fp.numColourResults > 1)
      fp->hdr[0] |= 0x4000;
   if (info->io.sampleMask < PIPE_MAX_SHADER_OUTPUTS)
      fp->hdr[19] |= 0x1;
   if (info->prop.fp.writesDepth) {
      fp->hdr[19] |= 0x2;
      fp->flags[0] = 0x11; /* deactivate ZCULL */
   }

   for (i = 0; i < info->numInputs; ++i) {
      m = nvc0_hdr_interp_mode(&info->in[i]);
      for (c = 0; c < 4; ++c) {
         if (!(info->in[i].mask & (1 << c)))
            continue;
         a = info->in[i].slot[c];
         if (info->in[i].slot[0] >= (0x060 / 4) &&
             info->in[i].slot[0] <= (0x07c / 4)) {
            fp->hdr[5] |= 1 << (24 + (a - 0x060 / 4));
         } else
         if (info->in[i].slot[0] >= (0x2c0 / 4) &&
             info->in[i].slot[0] <= (0x2fc / 4)) {
            fp->hdr[14] |= (1 << (a - 0x280 / 4)) & 0x03ff0000;
         } else {
            if (info->in[i].slot[c] < (0x040 / 4) ||
                info->in[i].slot[c] > (0x380 / 4))
               continue;
            a *= 2;
            if (info->in[i].slot[0] >= (0x300 / 4))
               a -= 32;
            fp->hdr[4 + a / 32] |= m << (a % 32);
         }
      }
   }

   for (i = 0; i < info->numOutputs; ++i) {
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
         fp->hdr[18] |= info->out[i].mask << info->out[i].slot[0];
   }

   fp->fp.early_z = info->prop.fp.earlyFragTests;

   return 0;
}

static struct nvc0_transform_feedback_state *
nvc0_program_create_tfb_state(const struct nv50_ir_prog_info *info,
                              const struct pipe_stream_output_info *pso)
{
   struct nvc0_transform_feedback_state *tfb;
   unsigned b, i, c;

   tfb = MALLOC_STRUCT(nvc0_transform_feedback_state);
   if (!tfb)
      return NULL;
   for (b = 0; b < 4; ++b) {
      tfb->stride[b] = pso->stride[b] * 4;
      tfb->varying_count[b] = 0;
   }
   memset(tfb->varying_index, 0xff, sizeof(tfb->varying_index)); /* = skip */

   for (i = 0; i < pso->num_outputs; ++i) {
      unsigned s = pso->output[i].start_component;
      unsigned p = pso->output[i].dst_offset;
      b = pso->output[i].output_buffer;

      for (c = 0; c < pso->output[i].num_components; ++c)
         tfb->varying_index[b][p++] =
            info->out[pso->output[i].register_index].slot[s + c];

      tfb->varying_count[b] = MAX2(tfb->varying_count[b], p);
   }
   for (b = 0; b < 4; ++b) // zero unused indices (looks nicer)
      for (c = tfb->varying_count[b]; c & 3; ++c)
         tfb->varying_index[b][c] = 0;

   return tfb;
}

#ifdef DEBUG
static void
nvc0_program_dump(struct nvc0_program *prog)
{
   unsigned pos;

   for (pos = 0; pos < sizeof(prog->hdr) / sizeof(prog->hdr[0]); ++pos)
      debug_printf("HDR[%02lx] = 0x%08x\n",
                   pos * sizeof(prog->hdr[0]), prog->hdr[pos]);

   debug_printf("shader binary code (0x%x bytes):", prog->code_size);
   for (pos = 0; pos < prog->code_size / 4; ++pos) {
      if ((pos % 8) == 0)
         debug_printf("\n");
      debug_printf("%08x ", prog->code[pos]);
   }
   debug_printf("\n");
}
#endif

boolean
nvc0_program_translate(struct nvc0_program *prog, uint16_t chipset)
{
   struct nv50_ir_prog_info *info;
   int ret;

   info = CALLOC_STRUCT(nv50_ir_prog_info);
   if (!info)
      return FALSE;

   info->type = prog->type;
   info->target = chipset;
   info->bin.sourceRep = NV50_PROGRAM_IR_TGSI;
   info->bin.source = (void *)prog->pipe.tokens;

   info->io.genUserClip = prog->vp.num_ucps;
   info->io.ucpBase = 256;
   info->io.ucpBinding = 15;

   info->assignSlots = nvc0_program_assign_varying_slots;

#ifdef DEBUG
   info->optLevel = debug_get_num_option("NV50_PROG_OPTIMIZE", 3);
   info->dbgFlags = debug_get_num_option("NV50_PROG_DEBUG", 0);
#else
   info->optLevel = 3;
#endif

   ret = nv50_ir_generate_code(info);
   if (ret) {
      NOUVEAU_ERR("shader translation failed: %i\n", ret);
      goto out;
   }
   if (info->bin.syms) /* we don't need them yet */
      FREE(info->bin.syms);

   prog->code = info->bin.code;
   prog->code_size = info->bin.codeSize;
   prog->immd_data = info->immd.buf;
   prog->immd_size = info->immd.bufSize;
   prog->relocs = info->bin.relocData;
   prog->max_gpr = MAX2(4, (info->bin.maxGPR + 1));

   prog->vp.need_vertex_id = info->io.vertexId < PIPE_MAX_SHADER_INPUTS;

   if (info->io.edgeFlagOut < PIPE_MAX_ATTRIBS)
      info->out[info->io.edgeFlagOut].mask = 0; /* for headergen */
   prog->vp.edgeflag = info->io.edgeFlagIn;

   switch (prog->type) {
   case PIPE_SHADER_VERTEX:
      ret = nvc0_vp_gen_header(prog, info);
      break;
#ifdef PIPE_SHADER_HULL
   case PIPE_SHADER_HULL:
      ret = nvc0_tcp_gen_header(prog, info);
      break;
#endif
#ifdef PIPE_SHADER_DOMAIN
   case PIPE_SHADER_DOMAIN:
      ret = nvc0_tep_gen_header(prog, info);
      break;
#endif
   case PIPE_SHADER_GEOMETRY:
      ret = nvc0_gp_gen_header(prog, info);
      break;
   case PIPE_SHADER_FRAGMENT:
      ret = nvc0_fp_gen_header(prog, info);
      break;
   default:
      ret = -1;
      NOUVEAU_ERR("unknown program type: %u\n", prog->type);
      break;
   }
   if (ret)
      goto out;

   if (info->bin.tlsSpace) {
      assert(info->bin.tlsSpace < (1 << 24));
      prog->hdr[0] |= 1 << 26;
      prog->hdr[1] |= info->bin.tlsSpace; /* l[] size */
      prog->need_tls = TRUE;
   }
   /* TODO: factor 2 only needed where joinat/precont is used,
    *       and we only have to count non-uniform branches
    */
   /*
   if ((info->maxCFDepth * 2) > 16) {
      prog->hdr[2] |= (((info->maxCFDepth * 2) + 47) / 48) * 0x200;
      prog->need_tls = TRUE;
   }
   */
   if (info->io.globalAccess)
      prog->hdr[0] |= 1 << 16;

   if (prog->pipe.stream_output.num_outputs)
      prog->tfb = nvc0_program_create_tfb_state(info,
                                                &prog->pipe.stream_output);

out:
   FREE(info);
   return !ret;
}

boolean
nvc0_program_upload_code(struct nvc0_context *nvc0, struct nvc0_program *prog)
{
   struct nvc0_screen *screen = nvc0->screen;
   int ret;
   uint32_t size = prog->code_size + NVC0_SHADER_HEADER_SIZE;
   uint32_t lib_pos = screen->lib_code->start;
   uint32_t code_pos;

   /* c[] bindings need to be aligned to 0x100, but we could use relocations
    * to save space. */
   if (prog->immd_size) {
      prog->immd_base = size;
      size = align(size, 0x40);
      size += prog->immd_size + 0xc0; /* add 0xc0 for align 0x40 -> 0x100 */
   }
   /* On Fermi, SP_START_ID must be aligned to 0x40.
    * On Kepler, the first instruction must be aligned to 0x80 because
    * latency information is expected only at certain positions.
    */
   if (screen->base.class_3d >= NVE4_3D_CLASS)
      size = size + 0x70;
   size = align(size, 0x40);

   ret = nouveau_heap_alloc(screen->text_heap, size, prog, &prog->mem);
   if (ret) {
      NOUVEAU_ERR("out of code space\n");
      return FALSE;
   }
   prog->code_base = prog->mem->start;
   prog->immd_base = align(prog->mem->start + prog->immd_base, 0x100);
   assert((prog->immd_size == 0) || (prog->immd_base + prog->immd_size <=
                                     prog->mem->start + prog->mem->size));

   if (screen->base.class_3d >= NVE4_3D_CLASS) {
      switch (prog->mem->start & 0xff) {
      case 0x40: prog->code_base += 0x70; break;
      case 0x80: prog->code_base += 0x30; break;
      case 0xc0: prog->code_base += 0x70; break;
      default:
         prog->code_base += 0x30;
         assert((prog->mem->start & 0xff) == 0x00);
         break;
      }
   }
   code_pos = prog->code_base + NVC0_SHADER_HEADER_SIZE;

   if (prog->relocs)
      nv50_ir_relocate_code(prog->relocs, prog->code, code_pos, lib_pos, 0);

#ifdef DEBUG
   if (debug_get_bool_option("NV50_PROG_DEBUG", FALSE))
      nvc0_program_dump(prog);
#endif

   nvc0->base.push_data(&nvc0->base, screen->text, prog->code_base,
                        NOUVEAU_BO_VRAM, NVC0_SHADER_HEADER_SIZE, prog->hdr);
   nvc0->base.push_data(&nvc0->base, screen->text,
                        prog->code_base + NVC0_SHADER_HEADER_SIZE,
                        NOUVEAU_BO_VRAM, prog->code_size, prog->code);
   if (prog->immd_size)
      nvc0->base.push_data(&nvc0->base,
                           screen->text, prog->immd_base, NOUVEAU_BO_VRAM,
                           prog->immd_size, prog->immd_data);

   BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(MEM_BARRIER), 1);
   PUSH_DATA (nvc0->base.pushbuf, 0x1011);

   return TRUE;
}

/* Upload code for builtin functions like integer division emulation. */
void
nvc0_program_library_upload(struct nvc0_context *nvc0)
{
   struct nvc0_screen *screen = nvc0->screen;
   int ret;
   uint32_t size;
   const uint32_t *code;

   if (screen->lib_code)
      return;

   nv50_ir_get_target_library(screen->base.device->chipset, &code, &size);
   if (!size)
      return;

   ret = nouveau_heap_alloc(screen->text_heap, align(size, 0x100), NULL,
                            &screen->lib_code);
   if (ret)
      return;

   nvc0->base.push_data(&nvc0->base,
                        screen->text, screen->lib_code->start, NOUVEAU_BO_VRAM,
                        size, code);
   /* no need for a memory barrier, will be emitted with first program */
}

void
nvc0_program_destroy(struct nvc0_context *nvc0, struct nvc0_program *prog)
{
   const struct pipe_shader_state pipe = prog->pipe;
   const ubyte type = prog->type;

   if (prog->mem)
      nouveau_heap_free(&prog->mem);

   if (prog->code)
      FREE(prog->code);
   if (prog->immd_data)
      FREE(prog->immd_data);
   if (prog->relocs)
      FREE(prog->relocs);
   if (prog->tfb) {
      if (nvc0->state.tfb == prog->tfb)
         nvc0->state.tfb = NULL;
      FREE(prog->tfb);
   }

   memset(prog, 0, sizeof(*prog));

   prog->pipe = pipe;
   prog->type = type;
}
