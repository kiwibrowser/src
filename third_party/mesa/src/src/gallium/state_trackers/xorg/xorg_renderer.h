#ifndef XORG_RENDERER_H
#define XORG_RENDERER_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct xorg_shaders;
struct exa_pixmap_priv;

/* max number of vertices *
 * max number of attributes per vertex *
 * max number of components per attribute
 *
 * currently the max is 100 quads
 */
#define BUF_SIZE (100 * 4 * 3 * 4)

struct xorg_renderer {
   struct pipe_context *pipe;

   struct cso_context *cso;
   struct xorg_shaders *shaders;

   int fb_width;
   int fb_height;
   struct pipe_resource *vs_const_buffer;
   struct pipe_resource *fs_const_buffer;

   float buffer[BUF_SIZE];
   int buffer_size;
   struct pipe_vertex_element velems[3];

   /* number of attributes per vertex for the current
    * draw operation */
   int attrs_per_vertex;
};

struct xorg_renderer *renderer_create(struct pipe_context *pipe);
void renderer_destroy(struct xorg_renderer *renderer);

void renderer_bind_destination(struct xorg_renderer *r,
                               struct pipe_surface *surface,
                               int width,
                               int height );

void renderer_bind_framebuffer(struct xorg_renderer *r,
                               struct exa_pixmap_priv *priv);
void renderer_bind_viewport(struct xorg_renderer *r,
                            struct exa_pixmap_priv *dst);
void renderer_set_constants(struct xorg_renderer *r,
                            int shader_type,
                            const float *buffer,
                            int size);


void renderer_draw_yuv(struct xorg_renderer *r,
                       float src_x, float src_y, float src_w, float src_h,
                       int dst_x, int dst_y, int dst_w, int dst_h,
                       struct pipe_resource **textures);

void renderer_begin_solid(struct xorg_renderer *r);
void renderer_solid(struct xorg_renderer *r,
                    int x0, int y0,
                    int x1, int y1,
                    float *color);

void renderer_begin_textures(struct xorg_renderer *r,
                             int num_textures);

void renderer_texture(struct xorg_renderer *r,
                      int *pos,
                      int width, int height,
                      struct pipe_sampler_view **textures,
                      int num_textures,
                      float *src_matrix,
                      float *mask_matrix);

void renderer_draw_flush(struct xorg_renderer *r);


#endif
