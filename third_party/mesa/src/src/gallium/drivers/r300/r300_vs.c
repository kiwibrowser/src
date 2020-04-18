/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r300_vs.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_tgsi_to_rc.h"
#include "r300_reg.h"

#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_ureg.h"

#include "compiler/radeon_compiler.h"

/* Convert info about VS output semantics into r300_shader_semantics. */
static void r300_shader_read_vs_outputs(
    struct r300_context *r300,
    struct tgsi_shader_info* info,
    struct r300_shader_semantics* vs_outputs)
{
    int i;
    unsigned index;

    r300_shader_semantics_reset(vs_outputs);

    for (i = 0; i < info->num_outputs; i++) {
        index = info->output_semantic_index[i];

        switch (info->output_semantic_name[i]) {
            case TGSI_SEMANTIC_POSITION:
                assert(index == 0);
                vs_outputs->pos = i;
                break;

            case TGSI_SEMANTIC_PSIZE:
                assert(index == 0);
                vs_outputs->psize = i;
                break;

            case TGSI_SEMANTIC_COLOR:
                assert(index < ATTR_COLOR_COUNT);
                vs_outputs->color[index] = i;
                break;

            case TGSI_SEMANTIC_BCOLOR:
                assert(index < ATTR_COLOR_COUNT);
                vs_outputs->bcolor[index] = i;
                break;

            case TGSI_SEMANTIC_GENERIC:
                assert(index < ATTR_GENERIC_COUNT);
                vs_outputs->generic[index] = i;
                break;

            case TGSI_SEMANTIC_FOG:
                assert(index == 0);
                vs_outputs->fog = i;
                break;

            case TGSI_SEMANTIC_EDGEFLAG:
                assert(index == 0);
                fprintf(stderr, "r300 VP: cannot handle edgeflag output.\n");
                break;

            case TGSI_SEMANTIC_CLIPVERTEX:
                assert(index == 0);
                /* Draw does clip vertex for us. */
                if (r300->screen->caps.has_tcl) {
                    fprintf(stderr, "r300 VP: cannot handle clip vertex output.\n");
                }
                break;

            default:
                fprintf(stderr, "r300 VP: unknown vertex output semantic: %i.\n",
                        info->output_semantic_name[i]);
        }
    }

    /* WPOS is a straight copy of POSITION and it's always emitted. */
    vs_outputs->wpos = i;
}

static void set_vertex_inputs_outputs(struct r300_vertex_program_compiler * c)
{
    struct r300_vertex_shader * vs = c->UserData;
    struct r300_shader_semantics* outputs = &vs->outputs;
    struct tgsi_shader_info* info = &vs->info;
    int i, reg = 0;
    boolean any_bcolor_used = outputs->bcolor[0] != ATTR_UNUSED ||
                              outputs->bcolor[1] != ATTR_UNUSED;

    /* Fill in the input mapping */
    for (i = 0; i < info->num_inputs; i++)
        c->code->inputs[i] = i;

    /* Position. */
    if (outputs->pos != ATTR_UNUSED) {
        c->code->outputs[outputs->pos] = reg++;
    } else {
        assert(0);
    }

    /* Point size. */
    if (outputs->psize != ATTR_UNUSED) {
        c->code->outputs[outputs->psize] = reg++;
    }

    /* If we're writing back facing colors we need to send
     * four colors to make front/back face colors selection work.
     * If the vertex program doesn't write all 4 colors, lets
     * pretend it does by skipping output index reg so the colors
     * get written into appropriate output vectors.
     */

    /* Colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (outputs->color[i] != ATTR_UNUSED) {
            c->code->outputs[outputs->color[i]] = reg++;
        } else if (any_bcolor_used ||
                   outputs->color[1] != ATTR_UNUSED) {
            reg++;
        }
    }

    /* Back-face colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (outputs->bcolor[i] != ATTR_UNUSED) {
            c->code->outputs[outputs->bcolor[i]] = reg++;
        } else if (any_bcolor_used) {
            reg++;
        }
    }

    /* Texture coordinates. */
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (outputs->generic[i] != ATTR_UNUSED) {
            c->code->outputs[outputs->generic[i]] = reg++;
        }
    }

    /* Fog coordinates. */
    if (outputs->fog != ATTR_UNUSED) {
        c->code->outputs[outputs->fog] = reg++;
    }

    /* WPOS. */
    c->code->outputs[outputs->wpos] = reg++;
}

void r300_init_vs_outputs(struct r300_context *r300,
                          struct r300_vertex_shader *vs)
{
    tgsi_scan_shader(vs->state.tokens, &vs->info);
    r300_shader_read_vs_outputs(r300, &vs->info, &vs->outputs);
}

static void r300_dummy_vertex_shader(
    struct r300_context* r300,
    struct r300_vertex_shader* shader)
{
    struct ureg_program *ureg;
    struct ureg_dst dst;
    struct ureg_src imm;

    /* Make a simple vertex shader which outputs (0, 0, 0, 1),
     * effectively rendering nothing. */
    ureg = ureg_create(TGSI_PROCESSOR_VERTEX);
    dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
    imm = ureg_imm4f(ureg, 0, 0, 0, 1);

    ureg_MOV(ureg, dst, imm);
    ureg_END(ureg);

    shader->state.tokens = tgsi_dup_tokens(ureg_finalize(ureg));
    ureg_destroy(ureg);

    shader->dummy = TRUE;
    r300_init_vs_outputs(r300, shader);
    r300_translate_vertex_shader(r300, shader);
}

void r300_translate_vertex_shader(struct r300_context *r300,
                                  struct r300_vertex_shader *vs)
{
    struct r300_vertex_program_compiler compiler;
    struct tgsi_to_rc ttr;
    unsigned i;

    /* Setup the compiler */
    memset(&compiler, 0, sizeof(compiler));
    rc_init(&compiler.Base);

    DBG_ON(r300, DBG_VP) ? compiler.Base.Debug |= RC_DBG_LOG : 0;
    DBG_ON(r300, DBG_P_STAT) ? compiler.Base.Debug |= RC_DBG_STATS : 0;
    compiler.code = &vs->code;
    compiler.UserData = vs;
    compiler.Base.is_r500 = r300->screen->caps.is_r500;
    compiler.Base.disable_optimizations = DBG_ON(r300, DBG_NO_OPT);
    compiler.Base.has_half_swizzles = FALSE;
    compiler.Base.has_presub = FALSE;
    compiler.Base.has_omod = FALSE;
    compiler.Base.max_temp_regs = 32;
    compiler.Base.max_constants = 256;
    compiler.Base.max_alu_insts = r300->screen->caps.is_r500 ? 1024 : 256;

    if (compiler.Base.Debug & RC_DBG_LOG) {
        DBG(r300, DBG_VP, "r300: Initial vertex program\n");
        tgsi_dump(vs->state.tokens, 0);
    }

    /* Translate TGSI to our internal representation */
    ttr.compiler = &compiler.Base;
    ttr.info = &vs->info;
    ttr.use_half_swizzles = FALSE;

    r300_tgsi_to_rc(&ttr, vs->state.tokens);

    if (ttr.error) {
        fprintf(stderr, "r300 VP: Cannot translate a shader. "
                "Using a dummy shader instead.\n");
        r300_dummy_vertex_shader(r300, vs);
        return;
    }

    if (compiler.Base.Program.Constants.Count > 200) {
        compiler.Base.remove_unused_constants = TRUE;
    }

    compiler.RequiredOutputs = ~(~0 << (vs->info.num_outputs + 1));
    compiler.SetHwInputOutput = &set_vertex_inputs_outputs;

    /* Insert the WPOS output. */
    rc_copy_output(&compiler.Base, 0, vs->outputs.wpos);

    /* Invoke the compiler */
    r3xx_compile_vertex_program(&compiler);
    if (compiler.Base.Error) {
        fprintf(stderr, "r300 VP: Compiler error:\n%sUsing a dummy shader"
                " instead.\n", compiler.Base.ErrorMsg);

        if (vs->dummy) {
            fprintf(stderr, "r300 VP: Cannot compile the dummy shader! "
                    "Giving up...\n");
            abort();
        }

        rc_destroy(&compiler.Base);
        r300_dummy_vertex_shader(r300, vs);
        return;
    }

    /* Initialize numbers of constants for each type. */
    vs->externals_count = 0;
    for (i = 0;
         i < vs->code.constants.Count &&
         vs->code.constants.Constants[i].Type == RC_CONSTANT_EXTERNAL; i++) {
        vs->externals_count = i+1;
    }
    for (; i < vs->code.constants.Count; i++) {
        assert(vs->code.constants.Constants[i].Type == RC_CONSTANT_IMMEDIATE);
    }
    vs->immediates_count = vs->code.constants.Count - vs->externals_count;

    /* And, finally... */
    rc_destroy(&compiler.Base);
}
