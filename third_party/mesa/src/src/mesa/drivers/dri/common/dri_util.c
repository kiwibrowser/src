/*
 * (C) Copyright IBM Corporation 2002, 2004
 * All Rights Reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file dri_util.c
 * DRI utility functions.
 *
 * This module acts as glue between GLX and the actual hardware driver.  A DRI
 * driver doesn't really \e have to use any of this - it's optional.  But, some
 * useful stuff is done here that otherwise would have to be duplicated in most
 * drivers.
 * 
 * Basically, these utility functions take care of some of the dirty details of
 * screen initialization, context creation, context binding, DRM setup, etc.
 *
 * These functions are compiled into each DRI driver so libGL.so knows nothing
 * about them.
 */


#include <xf86drm.h>
#include "dri_util.h"
#include "utils.h"
#include "xmlpool.h"
#include "../glsl/glsl_parser_extras.h"

PUBLIC const char __dri2ConfigOptions[] =
   DRI_CONF_BEGIN
      DRI_CONF_SECTION_PERFORMANCE
         DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_1)
      DRI_CONF_SECTION_END
   DRI_CONF_END;

static const uint __dri2NConfigOptions = 1;

/*****************************************************************/
/** \name Screen handling functions                              */
/*****************************************************************/
/*@{*/

static void
setupLoaderExtensions(__DRIscreen *psp,
		      const __DRIextension **extensions)
{
    int i;

    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_DRI2_LOADER) == 0)
	    psp->dri2.loader = (__DRIdri2LoaderExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_IMAGE_LOOKUP) == 0)
	    psp->dri2.image = (__DRIimageLookupExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_USE_INVALIDATE) == 0)
	    psp->dri2.useInvalidate = (__DRIuseInvalidateExtension *) extensions[i];
    }
}

static __DRIscreen *
dri2CreateNewScreen(int scrn, int fd,
		    const __DRIextension **extensions,
		    const __DRIconfig ***driver_configs, void *data)
{
    static const __DRIextension *emptyExtensionList[] = { NULL };
    __DRIscreen *psp;
    drmVersionPtr version;

    psp = calloc(1, sizeof(*psp));
    if (!psp)
	return NULL;

    setupLoaderExtensions(psp, extensions);

    version = drmGetVersion(fd);
    if (version) {
	psp->drm_version.major = version->version_major;
	psp->drm_version.minor = version->version_minor;
	psp->drm_version.patch = version->version_patchlevel;
	drmFreeVersion(version);
    }

    psp->loaderPrivate = data;

    psp->extensions = emptyExtensionList;
    psp->fd = fd;
    psp->myNum = scrn;

    psp->api_mask = (1 << __DRI_API_OPENGL);

    *driver_configs = driDriverAPI.InitScreen(psp);
    if (*driver_configs == NULL) {
	free(psp);
	return NULL;
    }

    driParseOptionInfo(&psp->optionInfo, __dri2ConfigOptions, __dri2NConfigOptions);
    driParseConfigFiles(&psp->optionCache, &psp->optionInfo, psp->myNum, "dri2");

    return psp;
}

/**
 * Destroy the per-screen private information.
 * 
 * \internal
 * This function calls __DriverAPIRec::DestroyScreen on \p screenPrivate, calls
 * drmClose(), and finally frees \p screenPrivate.
 */
static void driDestroyScreen(__DRIscreen *psp)
{
    if (psp) {
	/* No interaction with the X-server is possible at this point.  This
	 * routine is called after XCloseDisplay, so there is no protocol
	 * stream open to the X-server anymore.
	 */

       _mesa_destroy_shader_compiler();

	driDriverAPI.DestroyScreen(psp);

	driDestroyOptionCache(&psp->optionCache);
	driDestroyOptionInfo(&psp->optionInfo);

	free(psp);
    }
}

static const __DRIextension **driGetExtensions(__DRIscreen *psp)
{
    return psp->extensions;
}

/*@}*/


/*****************************************************************/
/** \name Context handling functions                             */
/*****************************************************************/
/*@{*/

static __DRIcontext *
dri2CreateContextAttribs(__DRIscreen *screen, int api,
			 const __DRIconfig *config,
			 __DRIcontext *shared,
			 unsigned num_attribs,
			 const uint32_t *attribs,
			 unsigned *error,
			 void *data)
{
    __DRIcontext *context;
    const struct gl_config *modes = (config != NULL) ? &config->modes : NULL;
    void *shareCtx = (shared != NULL) ? shared->driverPrivate : NULL;
    gl_api mesa_api;
    unsigned major_version = 1;
    unsigned minor_version = 0;
    uint32_t flags = 0;

    assert((num_attribs == 0) || (attribs != NULL));

    if (!(screen->api_mask & (1 << api))) {
	*error = __DRI_CTX_ERROR_BAD_API;
	return NULL;
    }

    switch (api) {
    case __DRI_API_OPENGL:
	mesa_api = API_OPENGL;
	break;
    case __DRI_API_GLES:
	mesa_api = API_OPENGLES;
	break;
    case __DRI_API_GLES2:
	mesa_api = API_OPENGLES2;
	break;
    case __DRI_API_OPENGL_CORE:
        mesa_api = API_OPENGL_CORE;
        break;
    default:
	*error = __DRI_CTX_ERROR_BAD_API;
	return NULL;
    }

    for (unsigned i = 0; i < num_attribs; i++) {
	switch (attribs[i * 2]) {
	case __DRI_CTX_ATTRIB_MAJOR_VERSION:
	    major_version = attribs[i * 2 + 1];
	    break;
	case __DRI_CTX_ATTRIB_MINOR_VERSION:
	    minor_version = attribs[i * 2 + 1];
	    break;
	case __DRI_CTX_ATTRIB_FLAGS:
	    flags = attribs[i * 2 + 1];
	    break;
	default:
	    /* We can't create a context that satisfies the requirements of an
	     * attribute that we don't understand.  Return failure.
	     */
	    assert(!"Should not get here.");
	    *error = __DRI_CTX_ERROR_UNKNOWN_ATTRIBUTE;
	    return NULL;
	}
    }

    /* Mesa does not support the GL_ARB_compatibilty extension or the
     * compatibility profile.  This means that we treat a API_OPENGL 3.1 as
     * API_OPENGL_CORE and reject API_OPENGL 3.2+.
     */
    if (mesa_api == API_OPENGL && major_version == 3 && minor_version == 1)
       mesa_api = API_OPENGL_CORE;

    if (mesa_api == API_OPENGL
        && ((major_version > 3)
            || (major_version == 3 && minor_version >= 2))) {
       *error = __DRI_CTX_ERROR_BAD_API;
       return NULL;
    }

    /* The EGL_KHR_create_context spec says:
     *
     *     "Flags are only defined for OpenGL context creation, and specifying
     *     a flags value other than zero for other types of contexts,
     *     including OpenGL ES contexts, will generate an error."
     *
     * The GLX_EXT_create_context_es2_profile specification doesn't say
     * anything specific about this case.  However, none of the known flags
     * have any meaning in an ES context, so this seems safe.
     */
    if (mesa_api != API_OPENGL
        && mesa_api != API_OPENGL_CORE
        && flags != 0) {
	*error = __DRI_CTX_ERROR_BAD_FLAG;
	return NULL;
    }

    /* There are no forward-compatible contexts before OpenGL 3.0.  The
     * GLX_ARB_create_context spec says:
     *
     *     "Forward-compatible contexts are defined only for OpenGL versions
     *     3.0 and later."
     *
     * Forward-looking contexts are supported by silently converting the
     * requested API to API_OPENGL_CORE.
     *
     * In Mesa, a debug context is the same as a regular context.
     */
    if ((flags & __DRI_CTX_FLAG_FORWARD_COMPATIBLE) != 0) {
       mesa_api = API_OPENGL_CORE;
    }

    if ((flags & ~(__DRI_CTX_FLAG_DEBUG | __DRI_CTX_FLAG_FORWARD_COMPATIBLE))
        != 0) {
	*error = __DRI_CTX_ERROR_UNKNOWN_FLAG;
	return NULL;
    }

    context = calloc(1, sizeof *context);
    if (!context) {
	*error = __DRI_CTX_ERROR_NO_MEMORY;
	return NULL;
    }

    context->loaderPrivate = data;

    context->driScreenPriv = screen;
    context->driDrawablePriv = NULL;
    context->driReadablePriv = NULL;

    if (!driDriverAPI.CreateContext(mesa_api, modes, context,
				    major_version, minor_version,
				    flags, error, shareCtx) ) {
        free(context);
        return NULL;
    }

    *error = __DRI_CTX_ERROR_SUCCESS;
    return context;
}

static __DRIcontext *
dri2CreateNewContextForAPI(__DRIscreen *screen, int api,
			   const __DRIconfig *config,
			   __DRIcontext *shared, void *data)
{
    unsigned error;

    return dri2CreateContextAttribs(screen, api, config, shared, 0, NULL,
				    &error, data);
}

static __DRIcontext *
dri2CreateNewContext(__DRIscreen *screen, const __DRIconfig *config,
		      __DRIcontext *shared, void *data)
{
    return dri2CreateNewContextForAPI(screen, __DRI_API_OPENGL,
				      config, shared, data);
}

/**
 * Destroy the per-context private information.
 * 
 * \internal
 * This function calls __DriverAPIRec::DestroyContext on \p contextPrivate, calls
 * drmDestroyContext(), and finally frees \p contextPrivate.
 */
static void
driDestroyContext(__DRIcontext *pcp)
{
    if (pcp) {
	driDriverAPI.DestroyContext(pcp);
	free(pcp);
    }
}

static int
driCopyContext(__DRIcontext *dest, __DRIcontext *src, unsigned long mask)
{
    (void) dest;
    (void) src;
    (void) mask;
    return GL_FALSE;
}

/*@}*/


/*****************************************************************/
/** \name Context (un)binding functions                          */
/*****************************************************************/
/*@{*/

static void dri_get_drawable(__DRIdrawable *pdp);
static void dri_put_drawable(__DRIdrawable *pdp);

/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 */
static int driBindContext(__DRIcontext *pcp,
			  __DRIdrawable *pdp,
			  __DRIdrawable *prp)
{
    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driUnbindContext.
    */

    if (!pcp)
	return GL_FALSE;

    /* Bind the drawable to the context */
    pcp->driDrawablePriv = pdp;
    pcp->driReadablePriv = prp;
    if (pdp) {
	pdp->driContextPriv = pcp;
	dri_get_drawable(pdp);
    }
    if (prp && pdp != prp) {
	dri_get_drawable(prp);
    }

    return driDriverAPI.MakeCurrent(pcp, pdp, prp);
}

/**
 * Unbind context.
 * 
 * \param scrn the screen.
 * \param gc context.
 *
 * \return \c GL_TRUE on success, or \c GL_FALSE on failure.
 * 
 * \internal
 * This function calls __DriverAPIRec::UnbindContext, and then decrements
 * __DRIdrawableRec::refcount which must be non-zero for a successful
 * return.
 * 
 * While casting the opaque private pointers associated with the parameters
 * into their respective real types it also assures they are not \c NULL. 
 */
static int driUnbindContext(__DRIcontext *pcp)
{
    __DRIdrawable *pdp;
    __DRIdrawable *prp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driUnbindContext.
    */

    if (pcp == NULL)
	return GL_FALSE;

    pdp = pcp->driDrawablePriv;
    prp = pcp->driReadablePriv;

    /* already unbound */
    if (!pdp && !prp)
	return GL_TRUE;

    driDriverAPI.UnbindContext(pcp);

    assert(pdp);
    if (pdp->refcount == 0) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    dri_put_drawable(pdp);

    if (prp != pdp) {
	if (prp->refcount == 0) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

	dri_put_drawable(prp);
    }

    /* XXX this is disabled so that if we call SwapBuffers on an unbound
     * window we can determine the last context bound to the window and
     * use that context's lock. (BrianP, 2-Dec-2000)
     */
    pcp->driDrawablePriv = NULL;
    pcp->driReadablePriv = NULL;

    return GL_TRUE;
}

/*@}*/


static void dri_get_drawable(__DRIdrawable *pdp)
{
    pdp->refcount++;
}

static void dri_put_drawable(__DRIdrawable *pdp)
{
    if (pdp) {
	pdp->refcount--;
	if (pdp->refcount)
	    return;

	driDriverAPI.DestroyBuffer(pdp);
	free(pdp);
    }
}

static __DRIdrawable *
dri2CreateNewDrawable(__DRIscreen *screen,
		      const __DRIconfig *config,
		      void *data)
{
    __DRIdrawable *pdraw;

    pdraw = malloc(sizeof *pdraw);
    if (!pdraw)
	return NULL;

    pdraw->loaderPrivate = data;

    pdraw->driScreenPriv = screen;
    pdraw->driContextPriv = NULL;
    pdraw->refcount = 0;
    pdraw->lastStamp = 0;
    pdraw->w = 0;
    pdraw->h = 0;

    dri_get_drawable(pdraw);

    if (!driDriverAPI.CreateBuffer(screen, pdraw, &config->modes, GL_FALSE)) {
       free(pdraw);
       return NULL;
    }

    pdraw->dri2.stamp = pdraw->lastStamp + 1;

    return pdraw;
}

static void
driDestroyDrawable(__DRIdrawable *pdp)
{
    dri_put_drawable(pdp);
}

static __DRIbuffer *
dri2AllocateBuffer(__DRIscreen *screen,
		   unsigned int attachment, unsigned int format,
		   int width, int height)
{
    return driDriverAPI.AllocateBuffer(screen, attachment, format,
				       width, height);
}

static void
dri2ReleaseBuffer(__DRIscreen *screen, __DRIbuffer *buffer)
{
    driDriverAPI.ReleaseBuffer(screen, buffer);
}


static int
dri2ConfigQueryb(__DRIscreen *screen, const char *var, GLboolean *val)
{
   if (!driCheckOption(&screen->optionCache, var, DRI_BOOL))
      return -1;

   *val = driQueryOptionb(&screen->optionCache, var);

   return 0;
}

static int
dri2ConfigQueryi(__DRIscreen *screen, const char *var, GLint *val)
{
   if (!driCheckOption(&screen->optionCache, var, DRI_INT) &&
       !driCheckOption(&screen->optionCache, var, DRI_ENUM))
      return -1;

    *val = driQueryOptioni(&screen->optionCache, var);

    return 0;
}

static int
dri2ConfigQueryf(__DRIscreen *screen, const char *var, GLfloat *val)
{
   if (!driCheckOption(&screen->optionCache, var, DRI_FLOAT))
      return -1;

    *val = driQueryOptionf(&screen->optionCache, var);

    return 0;
}

static unsigned int
dri2GetAPIMask(__DRIscreen *screen)
{
    return screen->api_mask;
}


/** Core interface */
const __DRIcoreExtension driCoreExtension = {
    { __DRI_CORE, __DRI_CORE_VERSION },
    NULL,
    driDestroyScreen,
    driGetExtensions,
    driGetConfigAttrib,
    driIndexConfigAttrib,
    NULL,
    driDestroyDrawable,
    NULL,
    NULL,
    driCopyContext,
    driDestroyContext,
    driBindContext,
    driUnbindContext
};

/** DRI2 interface */
const __DRIdri2Extension driDRI2Extension = {
    { __DRI_DRI2, 3 },
    dri2CreateNewScreen,
    dri2CreateNewDrawable,
    dri2CreateNewContext,
    dri2GetAPIMask,
    dri2CreateNewContextForAPI,
    dri2AllocateBuffer,
    dri2ReleaseBuffer,
    dri2CreateContextAttribs
};

const __DRI2configQueryExtension dri2ConfigQueryExtension = {
   { __DRI2_CONFIG_QUERY, __DRI2_CONFIG_QUERY_VERSION },
   dri2ConfigQueryb,
   dri2ConfigQueryi,
   dri2ConfigQueryf,
};

void
dri2InvalidateDrawable(__DRIdrawable *drawable)
{
    drawable->dri2.stamp++;
}

/**
 * Check that the gl_framebuffer associated with dPriv is the right size.
 * Resize the gl_framebuffer if needed.
 * It's expected that the dPriv->driverPrivate member points to a
 * gl_framebuffer object.
 */
void
driUpdateFramebufferSize(struct gl_context *ctx, const __DRIdrawable *dPriv)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) dPriv->driverPrivate;
   if (fb && (dPriv->w != fb->Width || dPriv->h != fb->Height)) {
      ctx->Driver.ResizeBuffers(ctx, fb, dPriv->w, dPriv->h);
      /* if the driver needs the hw lock for ResizeBuffers, the drawable
         might have changed again by now */
      assert(fb->Width == dPriv->w);
      assert(fb->Height == dPriv->h);
   }
}
