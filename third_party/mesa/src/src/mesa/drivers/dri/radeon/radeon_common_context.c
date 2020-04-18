/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "radeon_common.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */
#include "utils.h"
#include "drivers/common/meta.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/fbobject.h"
#include "main/renderbuffer.h"
#include "main/state.h"
#include "main/simple_list.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"

#ifndef RADEON_DEBUG
int RADEON_DEBUG = (0);
#endif


static const char* get_chip_family_name(int chip_family)
{
	switch(chip_family) {
#if defined(RADEON_R100)
	case CHIP_FAMILY_R100: return "R100";
	case CHIP_FAMILY_RV100: return "RV100";
	case CHIP_FAMILY_RS100: return "RS100";
	case CHIP_FAMILY_RV200: return "RV200";
	case CHIP_FAMILY_RS200: return "RS200";
#elif defined(RADEON_R200)
	case CHIP_FAMILY_R200: return "R200";
	case CHIP_FAMILY_RV250: return "RV250";
	case CHIP_FAMILY_RS300: return "RS300";
	case CHIP_FAMILY_RV280: return "RV280";
#endif
	default: return "unknown";
	}
}


/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString(struct gl_context * ctx, GLenum name)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	static char buffer[128];

	switch (name) {
	case GL_VENDOR:
		return (GLubyte *) "Tungsten Graphics, Inc.";

	case GL_RENDERER:
	{
		unsigned offset;
		GLuint agp_mode = (radeon->radeonScreen->card_type==RADEON_CARD_PCI) ? 0 :
			radeon->radeonScreen->AGPMode;
		char hardwarename[32];

		sprintf(hardwarename, "%s (%s %04X)",
#if defined(RADEON_R100)
		        "R100",
#elif defined(RADEON_R200)
		        "R200",
#endif
		        get_chip_family_name(radeon->radeonScreen->chip_family),
		        radeon->radeonScreen->device_id);

		offset = driGetRendererString(buffer, hardwarename, agp_mode);

		sprintf(&buffer[offset], " %sTCL",
			!(radeon->TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
			? "" : "NO-");

		strcat(buffer, " DRI2");

		return (GLubyte *) buffer;
	}

	default:
		return NULL;
	}
}

/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs(struct dd_function_table *functions)
{
	functions->GetString = radeonGetString;
}

/**
 * Create and initialize all common fields of the context,
 * including the Mesa context itself.
 */
GLboolean radeonInitContext(radeonContextPtr radeon,
			    struct dd_function_table* functions,
			    const struct gl_config * glVisual,
			    __DRIcontext * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreen *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->driverPrivate);
	struct gl_context* ctx;
	struct gl_context* shareCtx;
	int fthrottle_mode;

	/* Fill in additional standard functions. */
	radeonInitDriverFuncs(functions);

	radeon->radeonScreen = screen;
	/* Allocate and initialize the Mesa context */
	if (sharedContextPrivate)
		shareCtx = ((radeonContextPtr)sharedContextPrivate)->glCtx;
	else
		shareCtx = NULL;
	radeon->glCtx = _mesa_create_context(API_OPENGL, glVisual, shareCtx,
					    functions, (void *)radeon);
	if (!radeon->glCtx)
		return GL_FALSE;

	ctx = radeon->glCtx;
	driContextPriv->driverPrivate = radeon;

	_mesa_meta_init(ctx);

	/* DRI fields */
	radeon->dri.context = driContextPriv;
	radeon->dri.screen = sPriv;
	radeon->dri.fd = sPriv->fd;
	radeon->dri.drmMinor = sPriv->drm_version.minor;

	/* Setup IRQs */
	fthrottle_mode = driQueryOptioni(&radeon->optionCache, "fthrottle_mode");
	radeon->iw.irq_seq = -1;
	radeon->irqsEmitted = 0;
	radeon->do_irqs = (fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS &&
			   radeon->radeonScreen->irq);

	radeon->do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

	if (!radeon->do_irqs)
		fprintf(stderr,
			"IRQ's not enabled, falling back to %s: %d %d\n",
			radeon->do_usleeps ? "usleeps" : "busy waits",
			fthrottle_mode, radeon->radeonScreen->irq);

        radeon->texture_depth = driQueryOptioni (&radeon->optionCache,
					        "texture_depth");
        if (radeon->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
                radeon->texture_depth = ( glVisual->rgbBits > 16 ) ?
	        DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

	radeon->texture_row_align = 32;
	radeon->texture_rect_row_align = 64;
	radeon->texture_compressed_row_align = 32;

	radeon_init_dma(radeon);

	return GL_TRUE;
}



/**
 * Destroy the command buffer and state atoms.
 */
static void radeon_destroy_atom_list(radeonContextPtr radeon)
{
	struct radeon_state_atom *atom;

	foreach(atom, &radeon->hw.atomlist) {
		FREE(atom->cmd);
		if (atom->lastcmd)
			FREE(atom->lastcmd);
	}

}

/**
 * Cleanup common context fields.
 * Called by r200DestroyContext
 */
void radeonDestroyContext(__DRIcontext *driContextPriv )
{
#ifdef RADEON_BO_TRACK
	FILE *track;
#endif
	GET_CURRENT_CONTEXT(ctx);
	radeonContextPtr radeon = (radeonContextPtr) driContextPriv->driverPrivate;
	radeonContextPtr current = ctx ? RADEON_CONTEXT(ctx) : NULL;

	assert(radeon);

	_mesa_meta_free(radeon->glCtx);

	if (radeon == current) {
		_mesa_make_current(NULL, NULL, NULL);
	}

	radeon_firevertices(radeon);
	if (!is_empty_list(&radeon->dma.reserved)) {
		rcommonFlushCmdBuf( radeon, __FUNCTION__ );
	}

	radeonFreeDmaRegions(radeon);
	radeonReleaseArrays(radeon->glCtx, ~0);
	if (radeon->vtbl.free_context)
		radeon->vtbl.free_context(radeon->glCtx);
	_swsetup_DestroyContext( radeon->glCtx );
	_tnl_DestroyContext( radeon->glCtx );
	_vbo_DestroyContext( radeon->glCtx );
	_swrast_DestroyContext( radeon->glCtx );

	/* free atom list */
	/* free the Mesa context */
	_mesa_destroy_context(radeon->glCtx);

	/* _mesa_destroy_context() might result in calls to functions that
	 * depend on the DriverCtx, so don't set it to NULL before.
	 *
	 * radeon->glCtx->DriverCtx = NULL;
	 */
	/* free the option cache */
	driDestroyOptionCache(&radeon->optionCache);

	rcommonDestroyCmdBuf(radeon);

	radeon_destroy_atom_list(radeon);

#ifdef RADEON_BO_TRACK
	track = fopen("/tmp/tracklog", "w");
	if (track) {
		radeon_tracker_print(&radeon->radeonScreen->bom->tracker, track);
		fclose(track);
	}
#endif
	FREE(radeon);
}

/* Force the context `c' to be unbound from its buffer.
 */
GLboolean radeonUnbindContext(__DRIcontext * driContextPriv)
{
	radeonContextPtr radeon = (radeonContextPtr) driContextPriv->driverPrivate;

	if (RADEON_DEBUG & RADEON_DRI)
		fprintf(stderr, "%s ctx %p\n", __FUNCTION__,
			radeon->glCtx);

	/* Unset current context and dispath table */
	_mesa_make_current(NULL, NULL, NULL);

	return GL_TRUE;
}


static unsigned
radeon_bits_per_pixel(const struct radeon_renderbuffer *rb)
{
   return _mesa_get_format_bytes(rb->base.Base.Format) * 8; 
}

/*
 * Check if drawable has been invalidated by dri2InvalidateDrawable().
 * Update renderbuffers if so. This prevents a client from accessing
 * a backbuffer that has a swap pending but not yet completed.
 *
 * See intel_prepare_render for equivalent code in intel driver.
 *
 */
void radeon_prepare_render(radeonContextPtr radeon)
{
    __DRIcontext *driContext = radeon->dri.context;
    __DRIdrawable *drawable;
    __DRIscreen *screen;

    screen = driContext->driScreenPriv;
    if (!screen->dri2.loader)
        return;

    drawable = driContext->driDrawablePriv;
    if (drawable->dri2.stamp != driContext->dri2.draw_stamp) {
	if (drawable->lastStamp != drawable->dri2.stamp)
	    radeon_update_renderbuffers(driContext, drawable, GL_FALSE);

	/* Intel driver does the equivalent of this, no clue if it is needed:*/
	radeon_draw_buffer(radeon->glCtx, radeon->glCtx->DrawBuffer);

	driContext->dri2.draw_stamp = drawable->dri2.stamp;
    }

    drawable = driContext->driReadablePriv;
    if (drawable->dri2.stamp != driContext->dri2.read_stamp) {
	if (drawable->lastStamp != drawable->dri2.stamp)
	    radeon_update_renderbuffers(driContext, drawable, GL_FALSE);
	driContext->dri2.read_stamp = drawable->dri2.stamp;
    }

    /* If we're currently rendering to the front buffer, the rendering
     * that will happen next will probably dirty the front buffer.  So
     * mark it as dirty here.
     */
    if (radeon->is_front_buffer_rendering)
	radeon->front_buffer_dirty = GL_TRUE;
}

void
radeon_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable,
			    GLboolean front_only)
{
	unsigned int attachments[10];
	__DRIbuffer *buffers = NULL;
	__DRIscreen *screen;
	struct radeon_renderbuffer *rb;
	int i, count;
	struct radeon_framebuffer *draw;
	radeonContextPtr radeon;
	char *regname;
	struct radeon_bo *depth_bo = NULL, *bo;

	if (RADEON_DEBUG & RADEON_DRI)
	    fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

	draw = drawable->driverPrivate;
	screen = context->driScreenPriv;
	radeon = (radeonContextPtr) context->driverPrivate;

	/* Set this up front, so that in case our buffers get invalidated
	 * while we're getting new buffers, we don't clobber the stamp and
	 * thus ignore the invalidate. */
	drawable->lastStamp = drawable->dri2.stamp;

	if (screen->dri2.loader
	   && (screen->dri2.loader->base.version > 2)
	   && (screen->dri2.loader->getBuffersWithFormat != NULL)) {
		struct radeon_renderbuffer *depth_rb;
		struct radeon_renderbuffer *stencil_rb;

		i = 0;
		if ((front_only || radeon->is_front_buffer_rendering ||
		     radeon->is_front_buffer_reading ||
		     !draw->color_rb[1])
		    && draw->color_rb[0]) {
			attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
			attachments[i++] = radeon_bits_per_pixel(draw->color_rb[0]);
		}

		if (!front_only) {
			if (draw->color_rb[1]) {
				attachments[i++] = __DRI_BUFFER_BACK_LEFT;
				attachments[i++] = radeon_bits_per_pixel(draw->color_rb[1]);
			}

			depth_rb = radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH);
			stencil_rb = radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL);

			if ((depth_rb != NULL) && (stencil_rb != NULL)) {
				attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
				attachments[i++] = radeon_bits_per_pixel(depth_rb);
			} else if (depth_rb != NULL) {
				attachments[i++] = __DRI_BUFFER_DEPTH;
				attachments[i++] = radeon_bits_per_pixel(depth_rb);
			} else if (stencil_rb != NULL) {
				attachments[i++] = __DRI_BUFFER_STENCIL;
				attachments[i++] = radeon_bits_per_pixel(stencil_rb);
			}
		}

		buffers = (*screen->dri2.loader->getBuffersWithFormat)(drawable,
								&drawable->w,
								&drawable->h,
								attachments, i / 2,
								&count,
								drawable->loaderPrivate);
	} else if (screen->dri2.loader) {
		i = 0;
		if (draw->color_rb[0])
			attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
		if (!front_only) {
			if (draw->color_rb[1])
				attachments[i++] = __DRI_BUFFER_BACK_LEFT;
			if (radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH))
				attachments[i++] = __DRI_BUFFER_DEPTH;
			if (radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL))
				attachments[i++] = __DRI_BUFFER_STENCIL;
		}

		buffers = (*screen->dri2.loader->getBuffers)(drawable,
								 &drawable->w,
								 &drawable->h,
								 attachments, i,
								 &count,
								 drawable->loaderPrivate);
	}

	if (buffers == NULL)
		return;

	for (i = 0; i < count; i++) {
		switch (buffers[i].attachment) {
		case __DRI_BUFFER_FRONT_LEFT:
			rb = draw->color_rb[0];
			regname = "dri2 front buffer";
			break;
		case __DRI_BUFFER_FAKE_FRONT_LEFT:
			rb = draw->color_rb[0];
			regname = "dri2 fake front buffer";
			break;
		case __DRI_BUFFER_BACK_LEFT:
			rb = draw->color_rb[1];
			regname = "dri2 back buffer";
			break;
		case __DRI_BUFFER_DEPTH:
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH);
			regname = "dri2 depth buffer";
			break;
		case __DRI_BUFFER_DEPTH_STENCIL:
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH);
			regname = "dri2 depth / stencil buffer";
			break;
		case __DRI_BUFFER_STENCIL:
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL);
			regname = "dri2 stencil buffer";
			break;
		case __DRI_BUFFER_ACCUM:
		default:
			fprintf(stderr,
				"unhandled buffer attach event, attacment type %d\n",
				buffers[i].attachment);
			return;
		}

		if (rb == NULL)
			continue;

		if (rb->bo) {
			uint32_t name = radeon_gem_name_bo(rb->bo);
			if (name == buffers[i].name)
				continue;
		}

		if (RADEON_DEBUG & RADEON_DRI)
			fprintf(stderr,
				"attaching buffer %s, %d, at %d, cpp %d, pitch %d\n",
				regname, buffers[i].name, buffers[i].attachment,
				buffers[i].cpp, buffers[i].pitch);

		rb->cpp = buffers[i].cpp;
		rb->pitch = buffers[i].pitch;
		rb->base.Base.Width = drawable->w;
		rb->base.Base.Height = drawable->h;
		rb->has_surface = 0;

		if (buffers[i].attachment == __DRI_BUFFER_STENCIL && depth_bo) {
			if (RADEON_DEBUG & RADEON_DRI)
				fprintf(stderr, "(reusing depth buffer as stencil)\n");
			bo = depth_bo;
			radeon_bo_ref(bo);
		} else {
			uint32_t tiling_flags = 0, pitch = 0;
			int ret;

			bo = radeon_bo_open(radeon->radeonScreen->bom,
						buffers[i].name,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						buffers[i].flags);

			if (bo == NULL) {
				fprintf(stderr, "failed to attach %s %d\n",
					regname, buffers[i].name);
				continue;
			}

			ret = radeon_bo_get_tiling(bo, &tiling_flags, &pitch);
			if (ret) {
				fprintf(stderr,
					"failed to get tiling for %s %d\n",
					regname, buffers[i].name);
				radeon_bo_unref(bo);
				bo = NULL;
				continue;
			} else {
				if (tiling_flags & RADEON_TILING_MACRO)
					bo->flags |= RADEON_BO_FLAGS_MACRO_TILE;
				if (tiling_flags & RADEON_TILING_MICRO)
					bo->flags |= RADEON_BO_FLAGS_MICRO_TILE;
			}
		}

		if (buffers[i].attachment == __DRI_BUFFER_DEPTH) {
			if (draw->base.Visual.depthBits == 16)
				rb->cpp = 2;
			depth_bo = bo;
		}

		radeon_renderbuffer_set_bo(rb, bo);
		radeon_bo_unref(bo);

		if (buffers[i].attachment == __DRI_BUFFER_DEPTH_STENCIL) {
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL);
			if (rb != NULL) {
				struct radeon_bo *stencil_bo = NULL;

				if (rb->bo) {
					uint32_t name = radeon_gem_name_bo(rb->bo);
					if (name == buffers[i].name)
						continue;
				}

				stencil_bo = bo;
				radeon_bo_ref(stencil_bo);
				radeon_renderbuffer_set_bo(rb, stencil_bo);
				radeon_bo_unref(stencil_bo);
			}
		}
	}

	driUpdateFramebufferSize(radeon->glCtx, drawable);
}

/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean radeonMakeCurrent(__DRIcontext * driContextPriv,
			    __DRIdrawable * driDrawPriv,
			    __DRIdrawable * driReadPriv)
{
	radeonContextPtr radeon;
	GET_CURRENT_CONTEXT(curCtx);
	struct gl_framebuffer *drfb, *readfb;

	if (driContextPriv)
		radeon = (radeonContextPtr)driContextPriv->driverPrivate;
	else
		radeon = NULL;
	/* According to the glXMakeCurrent() man page: "Pending commands to
	 * the previous context, if any, are flushed before it is released."
	 * But only flush if we're actually changing contexts.
	 */

	if ((radeonContextPtr)curCtx && (radeonContextPtr)curCtx != radeon) {
		_mesa_flush(curCtx);
	}

	if (!driContextPriv) {
		if (RADEON_DEBUG & RADEON_DRI)
			fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
		_mesa_make_current(NULL, NULL, NULL);
		return GL_TRUE;
	}

	if(driDrawPriv == NULL && driReadPriv == NULL) {
		drfb = _mesa_create_framebuffer(&radeon->glCtx->Visual);
		readfb = drfb;
	}
	else {
		drfb = driDrawPriv->driverPrivate;
		readfb = driReadPriv->driverPrivate;
	}

	if(driDrawPriv)
	   radeon_update_renderbuffers(driContextPriv, driDrawPriv, GL_FALSE);
	if (driDrawPriv != driReadPriv)
	   radeon_update_renderbuffers(driContextPriv, driReadPriv, GL_FALSE);
	_mesa_reference_renderbuffer(&radeon->state.color.rb,
		&(radeon_get_renderbuffer(drfb, BUFFER_BACK_LEFT)->base.Base));
	_mesa_reference_renderbuffer(&radeon->state.depth.rb,
		&(radeon_get_renderbuffer(drfb, BUFFER_DEPTH)->base.Base));

	if (RADEON_DEBUG & RADEON_DRI)
	     fprintf(stderr, "%s ctx %p dfb %p rfb %p\n", __FUNCTION__, radeon->glCtx, drfb, readfb);

	if(driDrawPriv)
		driUpdateFramebufferSize(radeon->glCtx, driDrawPriv);
	if (driReadPriv != driDrawPriv)
		driUpdateFramebufferSize(radeon->glCtx, driReadPriv);

	_mesa_make_current(radeon->glCtx, drfb, readfb);
	if (driDrawPriv == NULL && driReadPriv == NULL)
		_mesa_reference_framebuffer(&drfb, NULL);

	_mesa_update_state(radeon->glCtx);

	if (radeon->glCtx->DrawBuffer == drfb) {
		if(driDrawPriv != NULL) {
			radeon_window_moved(radeon);
		}

		radeon_draw_buffer(radeon->glCtx, drfb);
	}


	if (RADEON_DEBUG & RADEON_DRI)
		fprintf(stderr, "End %s\n", __FUNCTION__);

	return GL_TRUE;
}

