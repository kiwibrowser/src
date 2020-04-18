#ifndef COMMON_MISC_H
#define COMMON_MISC_H

#include "radeon_common_context.h"
#include "radeon_dma.h"
#include "radeon_texture.h"

void radeonUserClear(struct gl_context *ctx, GLuint mask);
void radeonSetCliprects(radeonContextPtr radeon);
void radeonUpdateScissor( struct gl_context *ctx );
void radeonScissor(struct gl_context* ctx, GLint x, GLint y, GLsizei w, GLsizei h);

extern uint32_t radeonGetAge(radeonContextPtr radeon);

void radeonFlush(struct gl_context *ctx);
void radeonFinish(struct gl_context * ctx);
void radeonEmitState(radeonContextPtr radeon);
GLuint radeonCountStateEmitSize(radeonContextPtr radeon);

void radeon_clear_tris(struct gl_context *ctx, GLbitfield mask);

void radeon_window_moved(radeonContextPtr radeon);
void radeon_draw_buffer(struct gl_context *ctx, struct gl_framebuffer *fb);
void radeonDrawBuffer( struct gl_context *ctx, GLenum mode );
void radeonReadBuffer( struct gl_context *ctx, GLenum mode );
void radeon_viewport(struct gl_context *ctx, GLint x, GLint y, GLsizei width, GLsizei height);
void radeon_fbo_init(struct radeon_context *radeon);
void
radeon_renderbuffer_set_bo(struct radeon_renderbuffer *rb,
			   struct radeon_bo *bo);
struct radeon_renderbuffer *
radeon_create_renderbuffer(gl_format format, __DRIdrawable *driDrawPriv);

void
radeonReadPixels(struct gl_context * ctx,
				GLint x, GLint y, GLsizei width, GLsizei height,
				GLenum format, GLenum type,
				const struct gl_pixelstore_attrib *pack, GLvoid * pixels);

void radeon_check_front_buffer_rendering(struct gl_context *ctx);
static inline struct radeon_renderbuffer *radeon_renderbuffer(struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = (struct radeon_renderbuffer *)rb;
	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s(rb %p)\n",
		__func__, (void *) rb);
	if (rrb && rrb->base.Base.ClassID == RADEON_RB_CLASS)
		return rrb;
	else
		return NULL;
}

static inline struct radeon_renderbuffer *radeon_get_renderbuffer(struct gl_framebuffer *fb, int att_index)
{
	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s(fb %p, index %d)\n",
		__func__, (void *) fb, att_index);

	if (att_index >= 0)
		return radeon_renderbuffer(fb->Attachment[att_index].Renderbuffer);
	else
		return NULL;
}

static inline struct radeon_renderbuffer *radeon_get_depthbuffer(radeonContextPtr rmesa)
{
	struct radeon_renderbuffer *rrb;
	rrb = radeon_renderbuffer(rmesa->state.depth.rb);
	if (!rrb)
		return NULL;

	return rrb;
}

static inline struct radeon_renderbuffer *radeon_get_colorbuffer(radeonContextPtr rmesa)
{
	struct radeon_renderbuffer *rrb;

	rrb = radeon_renderbuffer(rmesa->state.color.rb);
	if (!rrb)
		return NULL;
	return rrb;
}

#include "radeon_cmdbuf.h"


#endif
