/*
 * Copyright (C) 2011 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <inttypes.h>
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_sampler.h"
#include "util/u_format.h"
#include "tgsi/tgsi_text.h"
#include "pipe-loader/pipe_loader.h"

#define MAX_RESOURCES 4

struct context {
        struct pipe_loader_device *dev;
        struct pipe_screen *screen;
        struct pipe_context *pipe;
        void *hwcs;
        void *hwsmp[MAX_RESOURCES];
        struct pipe_resource *tex[MAX_RESOURCES];
        bool tex_rw[MAX_RESOURCES];
        struct pipe_sampler_view *view[MAX_RESOURCES];
        struct pipe_surface *surf[MAX_RESOURCES];
};

#define DUMP_COMPUTE_PARAM(p, c) do {                                   \
                uint64_t __v[4];                                        \
                int __i, __n;                                           \
                                                                        \
                __n = ctx->screen->get_compute_param(ctx->screen, c, __v); \
                printf("%s: {", #c);                                    \
                                                                        \
                for (__i = 0; __i < __n / sizeof(*__v); ++__i)          \
                        printf(" %"PRIu64, __v[__i]);                   \
                                                                        \
                printf(" }\n");                                         \
        } while (0)

static void init_ctx(struct context *ctx)
{
        int ret;

        ret = pipe_loader_probe(&ctx->dev, 1);
        assert(ret);

        ctx->screen = pipe_loader_create_screen(ctx->dev, PIPE_SEARCH_DIR);
        assert(ctx->screen);

        ctx->pipe = ctx->screen->context_create(ctx->screen, NULL);
        assert(ctx->pipe);

        DUMP_COMPUTE_PARAM(p, PIPE_COMPUTE_CAP_GRID_DIMENSION);
        DUMP_COMPUTE_PARAM(p, PIPE_COMPUTE_CAP_MAX_GRID_SIZE);
        DUMP_COMPUTE_PARAM(p, PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE);
}

static void destroy_ctx(struct context *ctx)
{
        ctx->pipe->destroy(ctx->pipe);
        ctx->screen->destroy(ctx->screen);
        pipe_loader_release(&ctx->dev, 1);
        FREE(ctx);
}

static char *
preprocess_prog(struct context *ctx, const char *src, const char *defs)
{
        const char header[] =
                "#define RGLOBAL        RES[32767]\n"
                "#define RLOCAL         RES[32766]\n"
                "#define RPRIVATE       RES[32765]\n"
                "#define RINPUT         RES[32764]\n";
        char cmd[512];
        char tmp[] = "/tmp/test-compute.tgsi-XXXXXX";
        char *buf;
        int fd, ret;
        struct stat st;
        FILE *p;

        /* Open a temporary file */
        fd = mkstemp(tmp);
        assert(fd >= 0);
        snprintf(cmd, sizeof(cmd), "cpp -P -nostdinc -undef %s > %s",
                 defs ? defs : "", tmp);

        /* Preprocess */
        p = popen(cmd, "w");
        fwrite(header, strlen(header), 1, p);
        fwrite(src, strlen(src), 1, p);
        ret = pclose(p);
        assert(!ret);

        /* Read back */
        ret = fstat(fd, &st);
        assert(!ret);

        buf = malloc(st.st_size + 1);
        ret = read(fd, buf, st.st_size);
        assert(ret == st.st_size);
        buf[ret] = 0;

        /* Clean up */
        close(fd);
        unlink(tmp);

        return buf;
}

static void init_prog(struct context *ctx, unsigned local_sz,
                      unsigned private_sz, unsigned input_sz,
                      const char *src, const char *defs)
{
        struct pipe_context *pipe = ctx->pipe;
        struct tgsi_token prog[1024];
        struct pipe_compute_state cs = {
                .prog = prog,
                .req_local_mem = local_sz,
                .req_private_mem = private_sz,
                .req_input_mem = input_sz
        };
        char *psrc = preprocess_prog(ctx, src, defs);
        int ret;

        ret = tgsi_text_translate(psrc, prog, Elements(prog));
        assert(ret);
        free(psrc);

        ctx->hwcs = pipe->create_compute_state(pipe, &cs);
        assert(ctx->hwcs);

        pipe->bind_compute_state(pipe, ctx->hwcs);
}

static void destroy_prog(struct context *ctx)
{
        struct pipe_context *pipe = ctx->pipe;

        pipe->delete_compute_state(pipe, ctx->hwcs);
        ctx->hwcs = NULL;
}

static void init_tex(struct context *ctx, int slot,
                     enum pipe_texture_target target, bool rw,
                     enum pipe_format format, int w, int h,
                     void (*init)(void *, int, int, int))
{
        struct pipe_context *pipe = ctx->pipe;
        struct pipe_resource **tex = &ctx->tex[slot];
        struct pipe_resource ttex = {
                .target = target,
                .format = format,
                .width0 = w,
                .height0 = h,
                .depth0 = 1,
                .array_size = 1,
                .bind = (PIPE_BIND_SAMPLER_VIEW |
                         PIPE_BIND_COMPUTE_RESOURCE |
                         PIPE_BIND_GLOBAL)
        };
        int dx = util_format_get_blocksize(format);
        int dy = util_format_get_stride(format, w);
        int nx = (target == PIPE_BUFFER ? (w / dx) :
                  util_format_get_nblocksx(format, w));
        int ny = (target == PIPE_BUFFER ? 1 :
                  util_format_get_nblocksy(format, h));
        struct pipe_transfer *xfer;
        char *map;
        int x, y;

        *tex = ctx->screen->resource_create(ctx->screen, &ttex);
        assert(*tex);

        xfer = pipe->get_transfer(pipe, *tex, 0, PIPE_TRANSFER_WRITE,
                                  &(struct pipe_box) { .width = w,
                                                  .height = h,
                                                  .depth = 1 });
        assert(xfer);

        map = pipe->transfer_map(pipe, xfer);
        assert(map);

        for (y = 0; y < ny; ++y) {
                for (x = 0; x < nx; ++x) {
                        init(map + y * dy + x * dx, slot, x, y);
                }
        }

        pipe->transfer_unmap(pipe, xfer);
        pipe->transfer_destroy(pipe, xfer);

        ctx->tex_rw[slot] = rw;
}

static bool default_check(void *x, void *y, int sz) {
        return !memcmp(x, y, sz);
}

static void check_tex(struct context *ctx, int slot,
                      void (*expect)(void *, int, int, int),
                      bool (*check)(void *, void *, int))
{
        struct pipe_context *pipe = ctx->pipe;
        struct pipe_resource *tex = ctx->tex[slot];
        int dx = util_format_get_blocksize(tex->format);
        int dy = util_format_get_stride(tex->format, tex->width0);
        int nx = (tex->target == PIPE_BUFFER ? (tex->width0 / dx) :
                  util_format_get_nblocksx(tex->format, tex->width0));
        int ny = (tex->target == PIPE_BUFFER ? 1 :
                  util_format_get_nblocksy(tex->format, tex->height0));
        struct pipe_transfer *xfer;
        char *map;
        int x, y, i;
        int err = 0;

        if (!check)
                check = default_check;

        xfer = pipe->get_transfer(pipe, tex, 0, PIPE_TRANSFER_READ,
                                  &(struct pipe_box) { .width = tex->width0,
                                        .height = tex->height0,
                                        .depth = 1 });
        assert(xfer);

        map = pipe->transfer_map(pipe, xfer);
        assert(map);

        for (y = 0; y < ny; ++y) {
                for (x = 0; x < nx; ++x) {
                        uint32_t exp[4];
                        uint32_t *res = (uint32_t *)(map + y * dy + x * dx);

                        expect(exp, slot, x, y);
                        if (check(res, exp, dx) || (++err) > 20)
                                continue;

                        if (dx < 4) {
                                uint32_t u = 0, v = 0;

                                for (i = 0; i < dx; i++) {
                                        u |= ((uint8_t *)exp)[i] << (8 * i);
                                        v |= ((uint8_t *)res)[i] << (8 * i);
                                }
                                printf("(%d, %d): got 0x%x, expected 0x%x\n",
                                       x, y, v, u);
                        } else {
                                for (i = 0; i < dx / 4; i++) {
                                        printf("(%d, %d)[%d]: got 0x%x/%f,"
                                               " expected 0x%x/%f\n", x, y, i,
                                               res[i], ((float *)res)[i],
                                               exp[i], ((float *)exp)[i]);
                                }
                        }
                }
        }

        pipe->transfer_unmap(pipe, xfer);
        pipe->transfer_destroy(pipe, xfer);

        if (err)
                printf("(%d, %d): \x1b[31mFAIL\x1b[0m (%d)\n", x, y, err);
        else
                printf("(%d, %d): \x1b[32mOK\x1b[0m\n", x, y);
}

static void destroy_tex(struct context *ctx)
{
        int i;

        for (i = 0; i < MAX_RESOURCES; ++i) {
                if (ctx->tex[i])
                        pipe_resource_reference(&ctx->tex[i], NULL);
        }
}

static void init_sampler_views(struct context *ctx, const int *slots)
{
        struct pipe_context *pipe = ctx->pipe;
        struct pipe_sampler_view tview;
        int i;

        for (i = 0; *slots >= 0; ++i, ++slots) {
                u_sampler_view_default_template(&tview, ctx->tex[*slots],
                                                ctx->tex[*slots]->format);

                ctx->view[i] = pipe->create_sampler_view(pipe, ctx->tex[*slots],
                                                         &tview);
                assert(ctx->view[i]);
        }

        pipe->set_compute_sampler_views(pipe, 0, i, ctx->view);
}

static void destroy_sampler_views(struct context *ctx)
{
        struct pipe_context *pipe = ctx->pipe;
        int i;

        pipe->set_compute_sampler_views(pipe, 0, MAX_RESOURCES, NULL);

        for (i = 0; i < MAX_RESOURCES; ++i) {
                if (ctx->view[i]) {
                        pipe->sampler_view_destroy(pipe, ctx->view[i]);
                        ctx->view[i] = NULL;
                }
        }
}

static void init_compute_resources(struct context *ctx, const int *slots)
{
        struct pipe_context *pipe = ctx->pipe;
        int i;

        for (i = 0; *slots >= 0; ++i, ++slots) {
                struct pipe_surface tsurf = {
                        .format = ctx->tex[*slots]->format,
                        .usage = ctx->tex[*slots]->bind,
                        .writable = ctx->tex_rw[*slots]
                };

                if (ctx->tex[*slots]->target == PIPE_BUFFER)
                        tsurf.u.buf.last_element = ctx->tex[*slots]->width0 - 1;

                ctx->surf[i] = pipe->create_surface(pipe, ctx->tex[*slots],
                                                    &tsurf);
                assert(ctx->surf[i]);
        }

        pipe->set_compute_resources(pipe, 0, i, ctx->surf);
}

static void destroy_compute_resources(struct context *ctx)
{
        struct pipe_context *pipe = ctx->pipe;
        int i;

        pipe->set_compute_resources(pipe, 0, MAX_RESOURCES, NULL);

        for (i = 0; i < MAX_RESOURCES; ++i) {
                if (ctx->surf[i]) {
                        pipe->surface_destroy(pipe, ctx->surf[i]);
                        ctx->surf[i] = NULL;
                }
        }
}

static void init_sampler_states(struct context *ctx, int n)
{
        struct pipe_context *pipe = ctx->pipe;
        struct pipe_sampler_state smp = {
                .normalized_coords = 1,
        };
        int i;

        for (i = 0; i < n; ++i) {
                ctx->hwsmp[i] = pipe->create_sampler_state(pipe, &smp);
                assert(ctx->hwsmp[i]);
        }

        pipe->bind_compute_sampler_states(pipe, 0, i, ctx->hwsmp);
}

static void destroy_sampler_states(struct context *ctx)
{
        struct pipe_context *pipe = ctx->pipe;
        int i;

        pipe->bind_compute_sampler_states(pipe, 0, MAX_RESOURCES, NULL);

        for (i = 0; i < MAX_RESOURCES; ++i) {
                if (ctx->hwsmp[i]) {
                        pipe->delete_sampler_state(pipe, ctx->hwsmp[i]);
                        ctx->hwsmp[i] = NULL;
                }
        }
}

static void init_globals(struct context *ctx, const int *slots,
                         uint32_t **handles)
{
        struct pipe_context *pipe = ctx->pipe;
        struct pipe_resource *res[MAX_RESOURCES];
        int i;

        for (i = 0; *slots >= 0; ++i, ++slots)
                res[i] = ctx->tex[*slots];

        pipe->set_global_binding(pipe, 0, i, res, handles);
}

static void destroy_globals(struct context *ctx)
{
        struct pipe_context *pipe = ctx->pipe;

        pipe->set_global_binding(pipe, 0, MAX_RESOURCES, NULL, NULL);
}

static void launch_grid(struct context *ctx, const uint *block_layout,
                        const uint *grid_layout, uint32_t pc,
                        const void *input)
{
        struct pipe_context *pipe = ctx->pipe;

        pipe->launch_grid(pipe, block_layout, grid_layout, pc, input);
}

static void test_system_values(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL SV[1], BLOCK_SIZE[0]\n"
                "DCL SV[2], GRID_SIZE[0]\n"
                "DCL SV[3], THREAD_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 64, 0, 0, 0 }\n"
                "IMM UINT32 { 16, 0, 0, 0 }\n"
                "IMM UINT32 { 0, 0, 0, 0 }\n"
                "\n"
                "BGNSUB"
                "  UMUL TEMP[0], SV[0], SV[1]\n"
                "  UADD TEMP[0], TEMP[0], SV[3]\n"
                "  UMUL TEMP[1], SV[1], SV[2]\n"
                "  UMUL TEMP[0].w, TEMP[0], TEMP[1].zzzz\n"
                "  UMUL TEMP[0].zw, TEMP[0], TEMP[1].yyyy\n"
                "  UMUL TEMP[0].yzw, TEMP[0], TEMP[1].xxxx\n"
                "  UADD TEMP[0].xy, TEMP[0].xyxy, TEMP[0].zwzw\n"
                "  UADD TEMP[0].x, TEMP[0].xxxx, TEMP[0].yyyy\n"
                "  UMUL TEMP[0].x, TEMP[0], IMM[0]\n"
                "  STORE RES[0].xyzw, TEMP[0], SV[0]\n"
                "  UADD TEMP[0].x, TEMP[0], IMM[1]\n"
                "  STORE RES[0].xyzw, TEMP[0], SV[1]\n"
                "  UADD TEMP[0].x, TEMP[0], IMM[1]\n"
                "  STORE RES[0].xyzw, TEMP[0], SV[2]\n"
                "  UADD TEMP[0].x, TEMP[0], IMM[1]\n"
                "  STORE RES[0].xyzw, TEMP[0], SV[3]\n"
                "  RET\n"
                "ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                int id = x / 16, sv = (x % 16) / 4, c = x % 4;
                int tid[] = { id % 20, (id % 240) / 20, id / 240, 0 };
                int bsz[] = { 4, 3, 5, 1};
                int gsz[] = { 5, 4, 1, 1};

                switch (sv) {
                case 0:
                        *(uint32_t *)p = tid[c] / bsz[c];
                        break;
                case 1:
                        *(uint32_t *)p = bsz[c];
                        break;
                case 2:
                        *(uint32_t *)p = gsz[c];
                        break;
                case 3:
                        *(uint32_t *)p = tid[c] % bsz[c];
                        break;
                }
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 76800, 0, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){4, 3, 5}, (uint []){5, 4, 1}, 0, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_resource_access(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW, WR\n"
                "DCL RES[1], 2D, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 15, 0, 0, 0 }\n"
                "IMM UINT32 { 16, 1, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UADD TEMP[0].x, SV[0].xxxx, SV[0].yyyy\n"
                "       AND TEMP[0].x, TEMP[0], IMM[0]\n"
                "       UMUL TEMP[0].x, TEMP[0], IMM[1]\n"
                "       LOAD TEMP[0].xyzw, RES[0], TEMP[0]\n"
                "       UMUL TEMP[1], SV[0], IMM[1]\n"
                "       STORE RES[1].xyzw, TEMP[1], TEMP[0]\n"
                "       RET\n"
                "    ENDSUB\n";
        void init0(void *p, int s, int x, int y) {
                *(float *)p = 8.0 - (float)x;
        }
        void init1(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                *(float *)p = 8.0 - (float)((x + 4*y) & 0x3f);
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 256, 0, init0);
        init_tex(ctx, 1, PIPE_TEXTURE_2D, true, PIPE_FORMAT_R32_FLOAT,
                 60, 12, init1);
        init_compute_resources(ctx, (int []) { 0, 1, -1 });
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){15, 12, 1}, 0, NULL);
        check_tex(ctx, 1, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_function_calls(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], 2D, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL SV[1], BLOCK_SIZE[0]\n"
                "DCL SV[2], GRID_SIZE[0]\n"
                "DCL SV[3], THREAD_ID[0]\n"
                "DCL TEMP[0]\n"
                "DCL TEMP[1]\n"
                "DCL TEMP[2], LOCAL\n"
                "IMM UINT32 { 0, 11, 22, 33 }\n"
                "IMM FLT32 { 11, 33, 55, 99 }\n"
                "IMM UINT32 { 4, 1, 0, 0 }\n"
                "IMM UINT32 { 12, 0, 0, 0 }\n"
                "\n"
                "00: BGNSUB\n"
                "01:  UMUL TEMP[0].x, TEMP[0], TEMP[0]\n"
                "02:  UADD TEMP[1].x, TEMP[1], IMM[2].yyyy\n"
                "03:  USLT TEMP[0].x, TEMP[0], IMM[0]\n"
                "04:  RET\n"
                "05: ENDSUB\n"
                "06: BGNSUB\n"
                "07:  UMUL TEMP[0].x, TEMP[0], TEMP[0]\n"
                "08:  UADD TEMP[1].x, TEMP[1], IMM[2].yyyy\n"
                "09:  USLT TEMP[0].x, TEMP[0], IMM[0].yyyy\n"
                "10:  IF TEMP[0].xxxx\n"
                "11:   CAL :0\n"
                "12:  ENDIF\n"
                "13:  RET\n"
                "14: ENDSUB\n"
                "15: BGNSUB\n"
                "16:  UMUL TEMP[2], SV[0], SV[1]\n"
                "17:  UADD TEMP[2], TEMP[2], SV[3]\n"
                "18:  UMUL TEMP[2], TEMP[2], IMM[2]\n"
                "00:  MOV TEMP[1].x, IMM[2].wwww\n"
                "19:  LOAD TEMP[0].x, RES[0].xxxx, TEMP[2]\n"
                "20:  CAL :6\n"
                "21:  STORE RES[0].x, TEMP[2], TEMP[1].xxxx\n"
                "22:  RET\n"
                "23: ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 15 * y + x;
        }
        void expect(void *p, int s, int x, int y) {
                *(uint32_t *)p = (15 * y + x) < 4 ? 2 : 1 ;
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_TEXTURE_2D, true, PIPE_FORMAT_R32_FLOAT,
                 15, 12, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){3, 3, 3}, (uint []){5, 4, 1}, 15, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_input_global(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL SV[0], THREAD_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 8, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0], SV[0], IMM[0]\n"
                "       LOAD TEMP[1].xy, RINPUT, TEMP[0]\n"
                "       LOAD TEMP[0].x, RGLOBAL, TEMP[1].yyyy\n"
                "       UADD TEMP[1].x, TEMP[0], -TEMP[1]\n"
                "       STORE RGLOBAL.x, TEMP[1].yyyy, TEMP[1]\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef - (x == 0 ? 0x10001 + 2 * s : 0);
        }
        uint32_t input[8] = { 0x10001, 0x10002, 0x10003, 0x10004,
                              0x10005, 0x10006, 0x10007, 0x10008 };

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 32, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT, 32, 0, init);
        init_tex(ctx, 1, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT, 32, 0, init);
        init_tex(ctx, 2, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT, 32, 0, init);
        init_tex(ctx, 3, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT, 32, 0, init);
        init_globals(ctx, (int []){ 0, 1, 2, 3, -1 },
                     (uint32_t *[]){ &input[1], &input[3],
                                     &input[5], &input[7] });
        launch_grid(ctx, (uint []){4, 1, 1}, (uint []){1, 1, 1}, 0, input);
        check_tex(ctx, 0, expect, NULL);
        check_tex(ctx, 1, expect, NULL);
        check_tex(ctx, 2, expect, NULL);
        check_tex(ctx, 3, expect, NULL);
        destroy_globals(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_private(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL SV[1], BLOCK_SIZE[0]\n"
                "DCL SV[2], THREAD_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "DCL TEMP[2], LOCAL\n"
                "IMM UINT32 { 128, 0, 0, 0 }\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, SV[0], SV[1]\n"
                "       UADD TEMP[0].x, TEMP[0], SV[2]\n"
                "       MOV TEMP[1].x, IMM[0].wwww\n"
                "       BGNLOOP\n"
                "               USEQ TEMP[2].x, TEMP[1], IMM[0]\n"
                "               IF TEMP[2]\n"
                "                       BRK\n"
                "               ENDIF\n"
                "               UDIV TEMP[2].x, TEMP[1], IMM[1]\n"
                "               UADD TEMP[2].x, TEMP[2], TEMP[0]\n"
                "               STORE RPRIVATE.x, TEMP[1], TEMP[2]\n"
                "               UADD TEMP[1].x, TEMP[1], IMM[1]\n"
                "       ENDLOOP\n"
                "       MOV TEMP[1].x, IMM[0].wwww\n"
                "       UMUL TEMP[0].x, TEMP[0], IMM[0]\n"
                "       BGNLOOP\n"
                "               USEQ TEMP[2].x, TEMP[1], IMM[0]\n"
                "               IF TEMP[2]\n"
                "                       BRK\n"
                "               ENDIF\n"
                "               LOAD TEMP[2].x, RPRIVATE, TEMP[1]\n"
                "               STORE RES[0].x, TEMP[0], TEMP[2]\n"
                "               UADD TEMP[0].x, TEMP[0], IMM[1]\n"
                "               UADD TEMP[1].x, TEMP[1], IMM[1]\n"
                "       ENDLOOP\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                *(uint32_t *)p = (x / 32) + x % 32;
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 128, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 32768, 0, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){16, 1, 1}, (uint []){16, 1, 1}, 0, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_local(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL SV[1], BLOCK_SIZE[0]\n"
                "DCL SV[2], THREAD_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "DCL TEMP[2], LOCAL\n"
                "IMM UINT32 { 1, 0, 0, 0 }\n"
                "IMM UINT32 { 2, 0, 0, 0 }\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "IMM UINT32 { 32, 0, 0, 0 }\n"
                "IMM UINT32 { 128, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, SV[2], IMM[2]\n"
                "       STORE RLOCAL.x, TEMP[0], IMM[0].wwww\n"
                "       MFENCE RLOCAL\n"
                "       USLT TEMP[1].x, SV[2], IMM[3]\n"
                "       IF TEMP[1]\n"
                "               UADD TEMP[1].x, TEMP[0], IMM[4]\n"
                "               BGNLOOP\n"
                "                       LOAD TEMP[2].x, RLOCAL, TEMP[1]\n"
                "                       USEQ TEMP[2].x, TEMP[2], IMM[0]\n"
                "                       IF TEMP[2]\n"
                "                               BRK\n"
                "                       ENDIF\n"
                "               ENDLOOP\n"
                "               STORE RLOCAL.x, TEMP[0], IMM[0]\n"
                "               MFENCE RLOCAL\n"
                "               BGNLOOP\n"
                "                       LOAD TEMP[2].x, RLOCAL, TEMP[1]\n"
                "                       USEQ TEMP[2].x, TEMP[2], IMM[1]\n"
                "                       IF TEMP[2]\n"
                "                               BRK\n"
                "                       ENDIF\n"
                "               ENDLOOP\n"
                "       ELSE\n"
                "               UADD TEMP[1].x, TEMP[0], -IMM[4]\n"
                "               BGNLOOP\n"
                "                       LOAD TEMP[2].x, RLOCAL, TEMP[1]\n"
                "                       USEQ TEMP[2].x, TEMP[2], IMM[0].wwww\n"
                "                       IF TEMP[2]\n"
                "                               BRK\n"
                "                       ENDIF\n"
                "               ENDLOOP\n"
                "               STORE RLOCAL.x, TEMP[0], IMM[0]\n"
                "               MFENCE RLOCAL\n"
                "               BGNLOOP\n"
                "                       LOAD TEMP[2].x, RLOCAL, TEMP[1]\n"
                "                       USEQ TEMP[2].x, TEMP[2], IMM[0]\n"
                "                       IF TEMP[2]\n"
                "                               BRK\n"
                "                       ENDIF\n"
                "               ENDLOOP\n"
                "               STORE RLOCAL.x, TEMP[0], IMM[1]\n"
                "               MFENCE RLOCAL\n"
                "       ENDIF\n"
                "       UMUL TEMP[1].x, SV[0], SV[1]\n"
                "       UMUL TEMP[1].x, TEMP[1], IMM[2]\n"
                "       UADD TEMP[1].x, TEMP[1], TEMP[0]\n"
                "       LOAD TEMP[0].x, RLOCAL, TEMP[0]\n"
                "       STORE RES[0].x, TEMP[1], TEMP[0]\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                *(uint32_t *)p = x & 0x20 ? 2 : 1;
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 256, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 4096, 0, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){64, 1, 1}, (uint []){16, 1, 1}, 0, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_sample(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL SVIEW[0], 2D, FLOAT\n"
                "DCL RES[0], 2D, RAW, WR\n"
                "DCL SAMP[0]\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 16, 1, 0, 0 }\n"
                "IMM FLT32 { 128, 32, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       I2F TEMP[1], SV[0]\n"
                "       DIV TEMP[1], TEMP[1], IMM[1]\n"
                "       SAMPLE TEMP[1], TEMP[1], SVIEW[0], SAMP[0]\n"
                "       UMUL TEMP[0], SV[0], IMM[0]\n"
                "       STORE RES[0].xyzw, TEMP[0], TEMP[1]\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(float *)p = s ? 1 : x * y;
        }
        void expect(void *p, int s, int x, int y) {
                switch (x % 4) {
                case 0:
                        *(float *)p = x / 4 * y;
                        break;
                case 1:
                case 2:
                        *(float *)p = 0;
                        break;
                case 3:
                        *(float *)p = 1;
                        break;
                }
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_TEXTURE_2D, true, PIPE_FORMAT_R32_FLOAT,
                 128, 32, init);
        init_tex(ctx, 1, PIPE_TEXTURE_2D, true, PIPE_FORMAT_R32_FLOAT,
                 512, 32, init);
        init_compute_resources(ctx, (int []) { 1, -1 });
        init_sampler_views(ctx, (int []) { 0, -1 });
        init_sampler_states(ctx, 2);
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){128, 32, 1}, 0, NULL);
        check_tex(ctx, 1, expect, NULL);
        destroy_sampler_states(ctx);
        destroy_sampler_views(ctx);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_many_kern(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW, WR\n"
                "DCL TEMP[0], LOCAL\n"
                "IMM UINT32 { 0, 1, 2, 3 }\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, IMM[0].xxxx, IMM[1].xxxx\n"
                "       STORE RES[0].x, TEMP[0], IMM[0].xxxx\n"
                "       RET\n"
                "    ENDSUB\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, IMM[0].yyyy, IMM[1].xxxx\n"
                "       STORE RES[0].x, TEMP[0], IMM[0].yyyy\n"
                "       RET\n"
                "    ENDSUB\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, IMM[0].zzzz, IMM[1].xxxx\n"
                "       STORE RES[0].x, TEMP[0], IMM[0].zzzz\n"
                "       RET\n"
                "    ENDSUB\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, IMM[0].wwww, IMM[1].xxxx\n"
                "       STORE RES[0].x, TEMP[0], IMM[0].wwww\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                *(uint32_t *)p = x;
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 16, 0, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){1, 1, 1}, 0, NULL);
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){1, 1, 1}, 5, NULL);
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){1, 1, 1}, 10, NULL);
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){1, 1, 1}, 15, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_constant(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW\n"
                "DCL RES[1], BUFFER, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, SV[0], IMM[0]\n"
                "       LOAD TEMP[1].x, RES[0], TEMP[0]\n"
                "       STORE RES[1].x, TEMP[0], TEMP[1]\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(float *)p = s ? 0xdeadbeef : 8.0 - (float)x;
        }
        void expect(void *p, int s, int x, int y) {
                *(float *)p = 8.0 - (float)x;
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, false, PIPE_FORMAT_R32_FLOAT,
                 256, 0, init);
        init_tex(ctx, 1, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 256, 0, init);
        init_compute_resources(ctx, (int []) { 0, 1, -1 });
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){64, 1, 1}, 0, NULL);
        check_tex(ctx, 1, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_resource_indirect(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW, WR\n"
                "DCL RES[1..3], BUFFER, RAW\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, SV[0], IMM[0]\n"
                "       LOAD TEMP[1].x, RES[1], TEMP[0]\n"
                "       LOAD TEMP[1].x, RES[TEMP[1].x+2], TEMP[0]\n"
                "       STORE RES[0].x, TEMP[0], TEMP[1]\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = s == 0 ? 0xdeadbeef :
                   s == 1 ? x % 2 :
                   s == 2 ? 2 * x :
                   2 * x + 1;
        }
        void expect(void *p, int s, int x, int y) {
           *(uint32_t *)p = 2 * x + (x % 2 ? 1 : 0);
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 256, 0, init);
        init_tex(ctx, 1, PIPE_BUFFER, false, PIPE_FORMAT_R32_FLOAT,
                 256, 0, init);
        init_tex(ctx, 2, PIPE_BUFFER, false, PIPE_FORMAT_R32_FLOAT,
                 256, 0, init);
        init_tex(ctx, 3, PIPE_BUFFER, false, PIPE_FORMAT_R32_FLOAT,
                 256, 0, init);
        init_compute_resources(ctx, (int []) { 0, 1, 2, 3, -1 });
        launch_grid(ctx, (uint []){1, 1, 1}, (uint []){64, 1, 1}, 0, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

enum pipe_format surface_fmts[] = {
        PIPE_FORMAT_B8G8R8A8_UNORM,
        PIPE_FORMAT_B8G8R8X8_UNORM,
        PIPE_FORMAT_A8R8G8B8_UNORM,
        PIPE_FORMAT_X8R8G8B8_UNORM,
        PIPE_FORMAT_X8R8G8B8_UNORM,
        PIPE_FORMAT_L8_UNORM,
        PIPE_FORMAT_A8_UNORM,
        PIPE_FORMAT_I8_UNORM,
        PIPE_FORMAT_L8A8_UNORM,
        PIPE_FORMAT_R32_FLOAT,
        PIPE_FORMAT_R32G32_FLOAT,
        PIPE_FORMAT_R32G32B32A32_FLOAT,
        PIPE_FORMAT_R32_UNORM,
        PIPE_FORMAT_R32G32_UNORM,
        PIPE_FORMAT_R32G32B32A32_UNORM,
        PIPE_FORMAT_R32_SNORM,
        PIPE_FORMAT_R32G32_SNORM,
        PIPE_FORMAT_R32G32B32A32_SNORM,
        PIPE_FORMAT_R8_UINT,
        PIPE_FORMAT_R8G8_UINT,
        PIPE_FORMAT_R8G8B8A8_UINT,
        PIPE_FORMAT_R8_SINT,
        PIPE_FORMAT_R8G8_SINT,
        PIPE_FORMAT_R8G8B8A8_SINT,
        PIPE_FORMAT_R32_UINT,
        PIPE_FORMAT_R32G32_UINT,
        PIPE_FORMAT_R32G32B32A32_UINT,
        PIPE_FORMAT_R32_SINT,
        PIPE_FORMAT_R32G32_SINT,
        PIPE_FORMAT_R32G32B32A32_SINT
};

static void test_surface_ld(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], 2D\n"
                "DCL RES[1], 2D, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 16, 1, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       LOAD TEMP[1], RES[0], SV[0]\n"
                "       UMUL TEMP[0], SV[0], IMM[0]\n"
                "       STORE RES[1].xyzw, TEMP[0], TEMP[1]\n"
                "       RET\n"
                "    ENDSUB\n";
        int i = 0;
        void init0f(void *p, int s, int x, int y) {
                float v[] = { 1.0, -.75, .50, -.25 };
                util_format_write_4f(surface_fmts[i], v, 0,
                                     p, 0, 0, 0, 1, 1);
        }
        void init0i(void *p, int s, int x, int y) {
                int v[] = { 0xffffffff, 0xffff, 0xff, 0xf };
                util_format_write_4i(surface_fmts[i], v, 0,
                                     p, 0, 0, 0, 1, 1);
        }
        void init1(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expectf(void *p, int s, int x, int y) {
                float v[4], w[4];
                init0f(v, s, x / 4, y);
                util_format_read_4f(surface_fmts[i], w, 0,
                                    v, 0, 0, 0, 1, 1);
                *(float *)p = w[x % 4];
        }
        void expecti(void *p, int s, int x, int y) {
                int32_t v[4], w[4];
                init0i(v, s, x / 4, y);
                util_format_read_4i(surface_fmts[i], w, 0,
                                    v, 0, 0, 0, 1, 1);
                *(uint32_t *)p = w[x % 4];
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);

        for (i = 0; i < Elements(surface_fmts); i++) {
                bool is_int = util_format_is_pure_integer(surface_fmts[i]);

                printf("   - %s\n", util_format_name(surface_fmts[i]));

                init_tex(ctx, 0, PIPE_TEXTURE_2D, true, surface_fmts[i],
                         128, 32, (is_int ? init0i : init0f));
                init_tex(ctx, 1, PIPE_TEXTURE_2D, true, PIPE_FORMAT_R32_FLOAT,
                         512, 32, init1);
                init_compute_resources(ctx, (int []) { 0, 1, -1 });
                init_sampler_states(ctx, 2);
                launch_grid(ctx, (uint []){1, 1, 1}, (uint []){128, 32, 1}, 0,
                            NULL);
                check_tex(ctx, 1, (is_int ? expecti : expectf), NULL);
                destroy_sampler_states(ctx);
                destroy_compute_resources(ctx);
                destroy_tex(ctx);
        }

        destroy_prog(ctx);
}

static void test_surface_st(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], 2D, RAW\n"
                "DCL RES[1], 2D, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "IMM UINT32 { 16, 1, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0], SV[0], IMM[0]\n"
                "       LOAD TEMP[1], RES[0], TEMP[0]\n"
                "       STORE RES[1], SV[0], TEMP[1]\n"
                "       RET\n"
                "    ENDSUB\n";
        int i = 0;
        void init0f(void *p, int s, int x, int y) {
                float v[] = { 1.0, -.75, 0.5, -.25 };
                *(float *)p = v[x % 4];
        }
        void init0i(void *p, int s, int x, int y) {
                int v[] = { 0xffffffff, 0xffff, 0xff, 0xf };
                *(int32_t *)p = v[x % 4];
        }
        void init1(void *p, int s, int x, int y) {
                memset(p, 1, util_format_get_blocksize(surface_fmts[i]));
        }
        void expectf(void *p, int s, int x, int y) {
                float vf[4];
                int j;

                for (j = 0; j < 4; j++)
                        init0f(&vf[j], s, 4 * x + j, y);
                util_format_write_4f(surface_fmts[i], vf, 0,
                                     p, 0, 0, 0, 1, 1);
        }
        void expects(void *p, int s, int x, int y) {
                int32_t v[4];
                int j;

                for (j = 0; j < 4; j++)
                        init0i(&v[j], s, 4 * x + j, y);
                util_format_write_4i(surface_fmts[i], v, 0,
                                     p, 0, 0, 0, 1, 1);
        }
        void expectu(void *p, int s, int x, int y) {
                uint32_t v[4];
                int j;

                for (j = 0; j < 4; j++)
                        init0i(&v[j], s, 4 * x + j, y);
                util_format_write_4ui(surface_fmts[i], v, 0,
                                      p, 0, 0, 0, 1, 1);
        }
        bool check(void *x, void *y, int sz) {
                int j;

                if (util_format_is_float(surface_fmts[i])) {
                        return fabs(*(float *)x - *(float *)y) < 3.92156863e-3;

                } else if ((sz % 4) == 0) {
                        for (j = 0; j < sz / 4; j++)
                                if (abs(((uint32_t *)x)[j] -
                                        ((uint32_t *)y)[j]) > 1)
                                        return false;
                        return true;
                } else {
                        return !memcmp(x, y, sz);
                }
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 0, 0, 0, src, NULL);

        for (i = 0; i < Elements(surface_fmts); i++) {
                bool is_signed = (util_format_description(surface_fmts[i])
                                  ->channel[0].type == UTIL_FORMAT_TYPE_SIGNED);
                bool is_int = util_format_is_pure_integer(surface_fmts[i]);

                printf("   - %s\n", util_format_name(surface_fmts[i]));

                init_tex(ctx, 0, PIPE_TEXTURE_2D, true, PIPE_FORMAT_R32_FLOAT,
                         512, 32, (is_int ? init0i : init0f));
                init_tex(ctx, 1, PIPE_TEXTURE_2D, true, surface_fmts[i],
                         128, 32, init1);
                init_compute_resources(ctx, (int []) { 0, 1, -1 });
                init_sampler_states(ctx, 2);
                launch_grid(ctx, (uint []){1, 1, 1}, (uint []){128, 32, 1}, 0,
                            NULL);
                check_tex(ctx, 1, (is_int && is_signed ? expects :
                                   is_int && !is_signed ? expectu :
                                   expectf), check);
                destroy_sampler_states(ctx);
                destroy_compute_resources(ctx);
                destroy_tex(ctx);
        }

        destroy_prog(ctx);
}

static void test_barrier(struct context *ctx)
{
        const char *src = "COMP\n"
                "DCL RES[0], BUFFER, RAW, WR\n"
                "DCL SV[0], BLOCK_ID[0]\n"
                "DCL SV[1], BLOCK_SIZE[0]\n"
                "DCL SV[2], THREAD_ID[0]\n"
                "DCL TEMP[0], LOCAL\n"
                "DCL TEMP[1], LOCAL\n"
                "DCL TEMP[2], LOCAL\n"
                "DCL TEMP[3], LOCAL\n"
                "IMM UINT32 { 1, 0, 0, 0 }\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "IMM UINT32 { 32, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL TEMP[0].x, SV[2], IMM[1]\n"
                "       MOV TEMP[1].x, IMM[0].wwww\n"
                "       BGNLOOP\n"
                "               BARRIER\n"
                "               STORE RLOCAL.x, TEMP[0], TEMP[1]\n"
                "               BARRIER\n"
                "               MOV TEMP[2].x, IMM[0].wwww\n"
                "               BGNLOOP\n"
                "                       UMUL TEMP[3].x, TEMP[2], IMM[1]\n"
                "                       LOAD TEMP[3].x, RLOCAL, TEMP[3]\n"
                "                       USNE TEMP[3].x, TEMP[3], TEMP[1]\n"
                "                       IF TEMP[3]\n"
                "                               END\n"
                "                       ENDIF\n"
                "                       UADD TEMP[2].x, TEMP[2], IMM[0]\n"
                "                       USEQ TEMP[3].x, TEMP[2], SV[1]\n"
                "                       IF TEMP[3]\n"
                "                               BRK\n"
                "                       ENDIF\n"
                "               ENDLOOP\n"
                "               UADD TEMP[1].x, TEMP[1], IMM[0]\n"
                "               USEQ TEMP[2].x, TEMP[1], IMM[2]\n"
                "               IF TEMP[2]\n"
                "                       BRK\n"
                "               ENDIF\n"
                "       ENDLOOP\n"
                "       UMUL TEMP[1].x, SV[0], SV[1]\n"
                "       UMUL TEMP[1].x, TEMP[1], IMM[1]\n"
                "       UADD TEMP[1].x, TEMP[1], TEMP[0]\n"
                "       LOAD TEMP[0].x, RLOCAL, TEMP[0]\n"
                "       STORE RES[0].x, TEMP[1], TEMP[0]\n"
                "       RET\n"
                "    ENDSUB\n";
        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                *(uint32_t *)p = 31;
        }

        printf("- %s\n", __func__);

        init_prog(ctx, 256, 0, 0, src, NULL);
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 4096, 0, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){64, 1, 1}, (uint []){16, 1, 1}, 0, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_atom_ops(struct context *ctx, bool global)
{
        const char *src = "COMP\n"
                "#ifdef TARGET_GLOBAL\n"
                "#define target RES[0]\n"
                "#else\n"
                "#define target RLOCAL\n"
                "#endif\n"
                ""
                "DCL RES[0], BUFFER, RAW, WR\n"
                "#define threadid SV[0]\n"
                "DCL threadid, THREAD_ID[0]\n"
                ""
                "#define offset TEMP[0]\n"
                "DCL offset, LOCAL\n"
                "#define tmp TEMP[1]\n"
                "DCL tmp, LOCAL\n"
                ""
                "#define k0 IMM[0]\n"
                "IMM UINT32 { 0, 0, 0, 0 }\n"
                "#define k1 IMM[1]\n"
                "IMM UINT32 { 1, 0, 0, 0 }\n"
                "#define k2 IMM[2]\n"
                "IMM UINT32 { 2, 0, 0, 0 }\n"
                "#define k3 IMM[3]\n"
                "IMM UINT32 { 3, 0, 0, 0 }\n"
                "#define k4 IMM[4]\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "#define k5 IMM[5]\n"
                "IMM UINT32 { 5, 0, 0, 0 }\n"
                "#define k6 IMM[6]\n"
                "IMM UINT32 { 6, 0, 0, 0 }\n"
                "#define k7 IMM[7]\n"
                "IMM UINT32 { 7, 0, 0, 0 }\n"
                "#define k8 IMM[8]\n"
                "IMM UINT32 { 8, 0, 0, 0 }\n"
                "#define k9 IMM[9]\n"
                "IMM UINT32 { 9, 0, 0, 0 }\n"
                "#define korig IMM[10].xxxx\n"
                "#define karg IMM[10].yyyy\n"
                "IMM UINT32 { 3735928559, 286331153, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       UMUL offset.x, threadid, k4\n"
                "       STORE target.x, offset, korig\n"
                "       USEQ tmp.x, threadid, k0\n"
                "       IF tmp\n"
                "               ATOMUADD tmp.x, target, offset, karg\n"
                "               ATOMUADD tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k1\n"
                "       IF tmp\n"
                "               ATOMXCHG tmp.x, target, offset, karg\n"
                "               ATOMXCHG tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k2\n"
                "       IF tmp\n"
                "               ATOMCAS tmp.x, target, offset, korig, karg\n"
                "               ATOMCAS tmp.x, target, offset, tmp, k0\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k3\n"
                "       IF tmp\n"
                "               ATOMAND tmp.x, target, offset, karg\n"
                "               ATOMAND tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k4\n"
                "       IF tmp\n"
                "               ATOMOR tmp.x, target, offset, karg\n"
                "               ATOMOR tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k5\n"
                "       IF tmp\n"
                "               ATOMXOR tmp.x, target, offset, karg\n"
                "               ATOMXOR tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k6\n"
                "       IF tmp\n"
                "               ATOMUMIN tmp.x, target, offset, karg\n"
                "               ATOMUMIN tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k7\n"
                "       IF tmp\n"
                "               ATOMUMAX tmp.x, target, offset, karg\n"
                "               ATOMUMAX tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k8\n"
                "       IF tmp\n"
                "               ATOMIMIN tmp.x, target, offset, karg\n"
                "               ATOMIMIN tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "       USEQ tmp.x, threadid, k9\n"
                "       IF tmp\n"
                "               ATOMIMAX tmp.x, target, offset, karg\n"
                "               ATOMIMAX tmp.x, target, offset, tmp\n"
                "       ENDIF\n"
                "#ifdef TARGET_LOCAL\n"
                "       LOAD tmp.x, RLOCAL, offset\n"
                "       STORE RES[0].x, offset, tmp\n"
                "#endif\n"
                "       RET\n"
                "    ENDSUB\n";

        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xbad;
        }
        void expect(void *p, int s, int x, int y) {
                switch (x) {
                case 0:
                        *(uint32_t *)p = 0xce6c8eef;
                        break;
                case 1:
                        *(uint32_t *)p = 0xdeadbeef;
                        break;
                case 2:
                        *(uint32_t *)p = 0x11111111;
                        break;
                case 3:
                        *(uint32_t *)p = 0x10011001;
                        break;
                case 4:
                        *(uint32_t *)p = 0xdfbdbfff;
                        break;
                case 5:
                        *(uint32_t *)p = 0x11111111;
                        break;
                case 6:
                        *(uint32_t *)p = 0x11111111;
                        break;
                case 7:
                        *(uint32_t *)p = 0xdeadbeef;
                        break;
                case 8:
                        *(uint32_t *)p = 0xdeadbeef;
                        break;
                case 9:
                        *(uint32_t *)p = 0x11111111;
                        break;
                }
        }

        printf("- %s (%s)\n", __func__, global ? "global" : "local");

        init_prog(ctx, 40, 0, 0, src,
                  (global ? "-DTARGET_GLOBAL" : "-DTARGET_LOCAL"));
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 40, 0, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){10, 1, 1}, (uint []){1, 1, 1}, 0, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

static void test_atom_race(struct context *ctx, bool global)
{
        const char *src = "COMP\n"
                "#ifdef TARGET_GLOBAL\n"
                "#define target RES[0]\n"
                "#else\n"
                "#define target RLOCAL\n"
                "#endif\n"
                ""
                "DCL RES[0], BUFFER, RAW, WR\n"
                ""
                "#define blockid SV[0]\n"
                "DCL blockid, BLOCK_ID[0]\n"
                "#define blocksz SV[1]\n"
                "DCL blocksz, BLOCK_SIZE[0]\n"
                "#define threadid SV[2]\n"
                "DCL threadid, THREAD_ID[0]\n"
                ""
                "#define offset TEMP[0]\n"
                "DCL offset, LOCAL\n"
                "#define arg TEMP[1]\n"
                "DCL arg, LOCAL\n"
                "#define count TEMP[2]\n"
                "DCL count, LOCAL\n"
                "#define vlocal TEMP[3]\n"
                "DCL vlocal, LOCAL\n"
                "#define vshared TEMP[4]\n"
                "DCL vshared, LOCAL\n"
                "#define last TEMP[5]\n"
                "DCL last, LOCAL\n"
                "#define tmp0 TEMP[6]\n"
                "DCL tmp0, LOCAL\n"
                "#define tmp1 TEMP[7]\n"
                "DCL tmp1, LOCAL\n"
                ""
                "#define k0 IMM[0]\n"
                "IMM UINT32 { 0, 0, 0, 0 }\n"
                "#define k1 IMM[1]\n"
                "IMM UINT32 { 1, 0, 0, 0 }\n"
                "#define k4 IMM[2]\n"
                "IMM UINT32 { 4, 0, 0, 0 }\n"
                "#define k32 IMM[3]\n"
                "IMM UINT32 { 32, 0, 0, 0 }\n"
                "#define k128 IMM[4]\n"
                "IMM UINT32 { 128, 0, 0, 0 }\n"
                "#define kdeadcafe IMM[5]\n"
                "IMM UINT32 { 3735931646, 0, 0, 0 }\n"
                "#define kallowed_set IMM[6]\n"
                "IMM UINT32 { 559035650, 0, 0, 0 }\n"
                "#define k11111111 IMM[7]\n"
                "IMM UINT32 { 286331153, 0, 0, 0 }\n"
                "\n"
                "    BGNSUB\n"
                "       MOV offset.x, threadid\n"
                "#ifdef TARGET_GLOBAL\n"
                "       UMUL tmp0.x, blockid, blocksz\n"
                "       UADD offset.x, offset, tmp0\n"
                "#endif\n"
                "       UMUL offset.x, offset, k4\n"
                "       USLT tmp0.x, threadid, k32\n"
                "       STORE target.x, offset, k0\n"
                "       BARRIER\n"
                "       IF tmp0\n"
                "               MOV vlocal.x, k0\n"
                "               MOV arg.x, kdeadcafe\n"
                "               BGNLOOP\n"
                "                       INEG arg.x, arg\n"
                "                       ATOMUADD vshared.x, target, offset, arg\n"
                "                       SFENCE target\n"
                "                       USNE tmp0.x, vshared, vlocal\n"
                "                       IF tmp0\n"
                "                               BRK\n"
                "                       ENDIF\n"
                "                       UADD vlocal.x, vlocal, arg\n"
                "               ENDLOOP\n"
                "               UADD vlocal.x, vshared, arg\n"
                "               LOAD vshared.x, target, offset\n"
                "               USEQ tmp0.x, vshared, vlocal\n"
                "               STORE target.x, offset, tmp0\n"
                "       ELSE\n"
                "               UADD offset.x, offset, -k128\n"
                "               MOV count.x, k0\n"
                "               MOV last.x, k0\n"
                "               BGNLOOP\n"
                "                       LOAD vshared.x, target, offset\n"
                "                       USEQ tmp0.x, vshared, kallowed_set.xxxx\n"
                "                       USEQ tmp1.x, vshared, kallowed_set.yyyy\n"
                "                       OR tmp0.x, tmp0, tmp1\n"
                "                       IF tmp0\n"
                "                               USEQ tmp0.x, vshared, last\n"
                "                               IF tmp0\n"
                "                                       CONT\n"
                "                               ENDIF\n"
                "                               MOV last.x, vshared\n"
                "                       ELSE\n"
                "                               END\n"
                "                       ENDIF\n"
                "                       UADD count.x, count, k1\n"
                "                       USEQ tmp0.x, count, k128\n"
                "                       IF tmp0\n"
                "                               BRK\n"
                "                       ENDIF\n"
                "               ENDLOOP\n"
                "               ATOMXCHG tmp0.x, target, offset, k11111111\n"
                "               UADD offset.x, offset, k128\n"
                "               ATOMXCHG tmp0.x, target, offset, k11111111\n"
                "               SFENCE target\n"
                "       ENDIF\n"
                "#ifdef TARGET_LOCAL\n"
                "       LOAD tmp0.x, RLOCAL, offset\n"
                "       UMUL tmp1.x, blockid, blocksz\n"
                "       UMUL tmp1.x, tmp1, k4\n"
                "       UADD offset.x, offset, tmp1\n"
                "       STORE RES[0].x, offset, tmp0\n"
                "#endif\n"
                "       RET\n"
                "    ENDSUB\n";

        void init(void *p, int s, int x, int y) {
                *(uint32_t *)p = 0xdeadbeef;
        }
        void expect(void *p, int s, int x, int y) {
                *(uint32_t *)p = x & 0x20 ? 0x11111111 : 0xffffffff;
        }

        printf("- %s (%s)\n", __func__, global ? "global" : "local");

        init_prog(ctx, 256, 0, 0, src,
                  (global ? "-DTARGET_GLOBAL" : "-DTARGET_LOCAL"));
        init_tex(ctx, 0, PIPE_BUFFER, true, PIPE_FORMAT_R32_FLOAT,
                 4096, 0, init);
        init_compute_resources(ctx, (int []) { 0, -1 });
        launch_grid(ctx, (uint []){64, 1, 1}, (uint []){16, 1, 1}, 0, NULL);
        check_tex(ctx, 0, expect, NULL);
        destroy_compute_resources(ctx);
        destroy_tex(ctx);
        destroy_prog(ctx);
}

int main(int argc, char *argv[])
{
        struct context *ctx = CALLOC_STRUCT(context);

        init_ctx(ctx);
        test_system_values(ctx);
        test_resource_access(ctx);
        test_function_calls(ctx);
        test_input_global(ctx);
        test_private(ctx);
        test_local(ctx);
        test_sample(ctx);
        test_many_kern(ctx);
        test_constant(ctx);
        test_resource_indirect(ctx);
        test_surface_ld(ctx);
        test_surface_st(ctx);
        test_barrier(ctx);
        test_atom_ops(ctx, true);
        test_atom_race(ctx, true);
        test_atom_ops(ctx, false);
        test_atom_race(ctx, false);
        destroy_ctx(ctx);

        return 0;
}
