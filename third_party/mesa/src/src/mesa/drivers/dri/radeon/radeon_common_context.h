
#ifndef COMMON_CONTEXT_H
#define COMMON_CONTEXT_H

#include "main/mm.h"
#include "math/m_vector.h"
#include "tnl/t_context.h"
#include "main/colormac.h"

#include "radeon_debug.h"
#include "radeon_screen.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "tnl/t_vertex.h"
#include "swrast/s_context.h"

struct radeon_context;

#include "radeon_bo_gem.h"
#include "radeon_cs_gem.h"

/* This union is used to avoid warnings/miscompilation
   with float to uint32_t casts due to strict-aliasing */
typedef union { GLfloat f; uint32_t ui32; } float_ui32_type;

struct radeon_context;
typedef struct radeon_context radeonContextRec;
typedef struct radeon_context *radeonContextPtr;


#define TEX_0   0x1
#define TEX_1   0x2
#define TEX_2   0x4
#define TEX_3	0x8
#define TEX_4	0x10
#define TEX_5	0x20

/* Rasterizing fallbacks */
/* See correponding strings in r200_swtcl.c */
#define RADEON_FALLBACK_TEXTURE		0x0001
#define RADEON_FALLBACK_DRAW_BUFFER	0x0002
#define RADEON_FALLBACK_STENCIL		0x0004
#define RADEON_FALLBACK_RENDER_MODE	0x0008
#define RADEON_FALLBACK_BLEND_EQ	0x0010
#define RADEON_FALLBACK_BLEND_FUNC	0x0020
#define RADEON_FALLBACK_DISABLE 	0x0040
#define RADEON_FALLBACK_BORDER_MODE	0x0080
#define RADEON_FALLBACK_DEPTH_BUFFER	0x0100
#define RADEON_FALLBACK_STENCIL_BUFFER  0x0200

#define R200_FALLBACK_TEXTURE           0x01
#define R200_FALLBACK_DRAW_BUFFER       0x02
#define R200_FALLBACK_STENCIL           0x04
#define R200_FALLBACK_RENDER_MODE       0x08
#define R200_FALLBACK_DISABLE           0x10
#define R200_FALLBACK_BORDER_MODE       0x20

#define RADEON_TCL_FALLBACK_RASTER            0x1 /* rasterization */
#define RADEON_TCL_FALLBACK_UNFILLED          0x2 /* unfilled tris */
#define RADEON_TCL_FALLBACK_LIGHT_TWOSIDE     0x4 /* twoside tris */
#define RADEON_TCL_FALLBACK_MATERIAL          0x8 /* material in vb */
#define RADEON_TCL_FALLBACK_TEXGEN_0          0x10 /* texgen, unit 0 */
#define RADEON_TCL_FALLBACK_TEXGEN_1          0x20 /* texgen, unit 1 */
#define RADEON_TCL_FALLBACK_TEXGEN_2          0x40 /* texgen, unit 2 */
#define RADEON_TCL_FALLBACK_TCL_DISABLE       0x80 /* user disable */
#define RADEON_TCL_FALLBACK_FOGCOORDSPEC      0x100 /* fogcoord, sep. spec light */

/* The blit width for texture uploads
 */
#define BLIT_WIDTH_BYTES 1024

/* Use the templated vertex format:
 */
#define COLOR_IS_RGBA
#define TAG(x) radeon##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

#define RADEON_RB_CLASS 0xdeadbeef

struct radeon_renderbuffer
{
	struct swrast_renderbuffer base;

	struct radeon_bo *bo;
	unsigned int cpp;
	/* unsigned int offset; */
	unsigned int pitch;

	struct radeon_bo *map_bo;
	GLbitfield map_mode;
	int map_x, map_y, map_w, map_h;
	int map_pitch;
	void *map_buffer;

	uint32_t draw_offset; /* FBO */
	/* boo Xorg 6.8.2 compat */
	int has_surface;

	GLuint pf_pending;  /**< sequence number of pending flip */
	__DRIdrawable *dPriv;
};

struct radeon_framebuffer
{
	struct gl_framebuffer base;

	struct radeon_renderbuffer *color_rb[2];
};


struct radeon_colorbuffer_state {
	int roundEnable;
	struct gl_renderbuffer *rb;
	uint32_t draw_offset; /* offset into color renderbuffer - FBOs */
};

struct radeon_depthbuffer_state {
	struct gl_renderbuffer *rb;
};

struct radeon_scissor_state {
	drm_clip_rect_t rect;
	GLboolean enabled;
};

struct radeon_state_atom {
	struct radeon_state_atom *next, *prev;
	const char *name;	/* for debug */
	int cmd_size;		/* size in bytes */
        GLuint idx;
	GLuint is_tcl;
        GLuint *cmd;		/* one or more cmd's */
	GLuint *lastcmd;		/* one or more cmd's */
	GLboolean dirty;	/* dirty-mark in emit_state_list */
        int (*check) (struct gl_context *, struct radeon_state_atom *atom); /* is this state active? */
        void (*emit) (struct gl_context *, struct radeon_state_atom *atom);
};

struct radeon_hw_state {
  	/* Head of the linked list of state atoms. */
	struct radeon_state_atom atomlist;
	int max_state_size;	/* Number of bytes necessary for a full state emit. */
	int max_post_flush_size; /* Number of bytes necessary for post flushing emits */
	GLboolean is_dirty, all_dirty;
};


/* Texture related */
typedef struct _radeon_texture_image radeon_texture_image;


/**
 * This is a subclass of swrast_texture_image since we use swrast
 * for software fallback rendering.
 */
struct _radeon_texture_image {
	struct swrast_texture_image base;

	/**
	 * If mt != 0, the image is stored in hardware format in the
	 * given mipmap tree. In this case, base.Data may point into the
	 * mapping of the buffer object that contains the mipmap tree.
	 *
	 * If mt == 0, the image is stored in normal memory pointed to
	 * by base.Data.
	 */
	struct _radeon_mipmap_tree *mt;
	struct radeon_bo *bo;
	GLboolean used_as_render_target;
};


static INLINE radeon_texture_image *get_radeon_texture_image(struct gl_texture_image *image)
{
	return (radeon_texture_image*)image;
}


typedef struct radeon_tex_obj radeonTexObj, *radeonTexObjPtr;

#define RADEON_TXO_MICRO_TILE               (1 << 3)

/* Texture object in locally shared texture space.
 */
struct radeon_tex_obj {
	struct gl_texture_object base;
	struct _radeon_mipmap_tree *mt;

	/**
	 * This is true if we've verified that the mipmap tree above is complete
	 * and so on.
	 */
	GLboolean validated;
	/* Minimum LOD to be used during rendering */
	unsigned minLod;
	/* Miximum LOD to be used during rendering */
	unsigned maxLod;

	GLuint override_offset;
	GLboolean image_override; /* Image overridden by GLX_EXT_tfp */
	GLuint tile_bits;	/* hw texture tile bits used on this texture */
        struct radeon_bo *bo;

	GLuint pp_txfilter;	/* hardware register values */
	GLuint pp_txformat;
	GLuint pp_txformat_x;
	GLuint pp_txsize;	/* npot only */
	GLuint pp_txpitch;	/* npot only */
	GLuint pp_border_color;
	GLuint pp_cubic_faces;	/* cube face 1,2,3,4 log2 sizes */

	GLboolean border_fallback;
};

static INLINE radeonTexObj* radeon_tex_obj(struct gl_texture_object *texObj)
{
	return (radeonTexObj*)texObj;
}

/* occlusion query */
struct radeon_query_object {
	struct gl_query_object Base;
	struct radeon_bo *bo;
	int curr_offset;
	GLboolean emitted_begin;

	/* Double linked list of not flushed query objects */
	struct radeon_query_object *prev, *next;
};

/* Need refcounting on dma buffers:
 */
struct radeon_dma_buffer {
	int refcount;		/* the number of retained regions in buf */
	drmBufPtr buf;
};

struct radeon_aos {
	struct radeon_bo *bo; /** Buffer object where vertex data is stored */
	int offset; /** Offset into buffer object, in bytes */
	int components; /** Number of components per vertex */
	int stride; /** Stride in dwords (may be 0 for repeating) */
	int count; /** Number of vertices */
};

#define DMA_BO_FREE_TIME 100

struct radeon_dma_bo {
  struct radeon_dma_bo *next, *prev;
  struct radeon_bo *bo;
  int expire_counter;
};

struct radeon_dma {
        /* Active dma region.  Allocations for vertices and retained
         * regions come from here.  Also used for emitting random vertices,
         * these may be flushed by calling flush_current();
         */
	struct radeon_dma_bo free;
	struct radeon_dma_bo wait;
	struct radeon_dma_bo reserved;
        size_t current_used; /** Number of bytes allocated and forgotten about */
        size_t current_vertexptr; /** End of active vertex region */
        size_t minimum_size;

        /**
         * If current_vertexptr != current_used then flush must be non-zero.
         * flush must be called before non-active vertex allocations can be
         * performed.
         */
        void (*flush) (struct gl_context *);
};

/* radeon_swtcl.c
 */
struct radeon_swtcl_info {

	GLuint RenderIndex;
	GLuint vertex_size;
	GLubyte *verts;

	/* Fallback rasterization functions
	 */
	GLuint hw_primitive;
	GLenum render_primitive;
	GLuint numverts;

	struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
	GLuint vertex_attr_count;

	GLuint emit_prediction;
        struct radeon_bo *bo;
};

#define RADEON_MAX_AOS_ARRAYS		16
struct radeon_tcl_info {
	struct radeon_aos aos[RADEON_MAX_AOS_ARRAYS];
	GLuint aos_count;
	struct radeon_bo *elt_dma_bo; /** Buffer object that contains element indices */
	int elt_dma_offset; /** Offset into this buffer object, in bytes */
};

struct radeon_ioctl {
	GLuint vertex_offset;
	GLuint vertex_max;
	struct radeon_bo *bo;
	GLuint vertex_size;
};

#define RADEON_MAX_PRIMS 64

struct radeon_prim {
	GLuint start;
	GLuint end;
	GLuint prim;
};

static INLINE GLuint radeonPackColor(GLuint cpp,
                                     GLubyte r, GLubyte g,
                                     GLubyte b, GLubyte a)
{
	switch (cpp) {
	case 2:
		return PACK_COLOR_565(r, g, b);
	case 4:
		return PACK_COLOR_8888(a, r, g, b);
	default:
		return 0;
	}
}

#define MAX_CMD_BUF_SZ (16*1024)

#define MAX_DMA_BUF_SZ (64*1024)

struct radeon_store {
	GLuint statenr;
	GLuint primnr;
	char cmd_buf[MAX_CMD_BUF_SZ];
	int cmd_used;
	int elts_start;
};

struct radeon_dri_mirror {
	__DRIcontext *context;	/* DRI context */
	__DRIscreen *screen;	/* DRI screen */

	drm_context_t hwContext;
	drm_hw_lock_t *hwLock;
	int hwLockCount;
	int fd;
	int drmMinor;
};

typedef void (*radeon_tri_func) (radeonContextPtr,
				 radeonVertex *,
				 radeonVertex *, radeonVertex *);

typedef void (*radeon_line_func) (radeonContextPtr,
				  radeonVertex *, radeonVertex *);

typedef void (*radeon_point_func) (radeonContextPtr, radeonVertex *);

#define RADEON_MAX_BOS 32
struct radeon_state {
	struct radeon_colorbuffer_state color;
	struct radeon_depthbuffer_state depth;
	struct radeon_scissor_state scissor;
};

/**
 * This structure holds the command buffer while it is being constructed.
 *
 * The first batch of commands in the buffer is always the state that needs
 * to be re-emitted when the context is lost. This batch can be skipped
 * otherwise.
 */
struct radeon_cmdbuf {
	struct radeon_cs_manager    *csm;
	struct radeon_cs            *cs;
	int size; /** # of dwords total */
	unsigned int flushing:1; /** whether we're currently in FlushCmdBufLocked */
};

struct radeon_context {
   struct gl_context *glCtx;
   radeonScreenPtr radeonScreen;	/* Screen private DRI data */

   /* Texture object bookkeeping
    */
   int                   texture_depth;
   float                 initialMaxAnisotropy;
   uint32_t              texture_row_align;
   uint32_t              texture_rect_row_align;
   uint32_t              texture_compressed_row_align;

  struct radeon_dma dma;
  struct radeon_hw_state hw;
   /* Rasterization and vertex state:
    */
   GLuint TclFallback;
   GLuint Fallback;
   GLuint NewGLState;
   GLbitfield64 tnl_index_bitset;	/* index of bits for last tnl_install_attrs */

   /* Drawable information */
   unsigned int lastStamp;
   drm_radeon_sarea_t *sarea;	/* Private SAREA data */

   /* Mirrors of some DRI state */
   struct radeon_dri_mirror dri;

   /* Busy waiting */
   GLuint do_usleeps;
   GLuint do_irqs;
   GLuint irqsEmitted;
   drm_radeon_irq_wait_t iw;

   /* Derived state - for r300 only */
   struct radeon_state state;

   struct radeon_swtcl_info swtcl;
   struct radeon_tcl_info tcl;
   /* Configuration cache
    */
   driOptionCache optionCache;

   struct radeon_cmdbuf cmdbuf;

   struct radeon_debug debug;

  drm_clip_rect_t fboRect;
  GLboolean front_cliprects;

   /**
    * Set if rendering has occured to the drawable's front buffer.
    *
    * This is used in the DRI2 case to detect that glFlush should also copy
    * the contents of the fake front buffer to the real front buffer.
    */
   GLboolean front_buffer_dirty;

   /**
    * Track whether front-buffer rendering is currently enabled
    *
    * A separate flag is used to track this in order to support MRT more
    * easily.
    */
   GLboolean is_front_buffer_rendering;

   /**
    * Track whether front-buffer is the current read target.
    *
    * This is closely associated with is_front_buffer_rendering, but may
    * be set separately.  The DRI2 fake front buffer must be referenced
    * either way.
    */
   GLboolean is_front_buffer_reading;

   struct {
	struct radeon_query_object *current;
	struct radeon_state_atom queryobj;
   } query;

   struct {
	   void (*get_lock)(radeonContextPtr radeon);
	   void (*update_viewport_offset)(struct gl_context *ctx);
	   void (*emit_cs_header)(struct radeon_cs *cs, radeonContextPtr rmesa);
	   void (*swtcl_flush)(struct gl_context *ctx, uint32_t offset);
	   void (*pre_emit_atoms)(radeonContextPtr rmesa);
	   void (*pre_emit_state)(radeonContextPtr rmesa);
	   void (*fallback)(struct gl_context *ctx, GLuint bit, GLboolean mode);
	   void (*free_context)(struct gl_context *ctx);
	   void (*emit_query_finish)(radeonContextPtr radeon);
	   void (*update_scissor)(struct gl_context *ctx);
	   unsigned (*check_blit)(gl_format mesa_format, uint32_t dst_pitch);
	   unsigned (*blit)(struct gl_context *ctx,
                        struct radeon_bo *src_bo,
                        intptr_t src_offset,
                        gl_format src_mesaformat,
                        unsigned src_pitch,
                        unsigned src_width,
                        unsigned src_height,
                        unsigned src_x_offset,
                        unsigned src_y_offset,
                        struct radeon_bo *dst_bo,
                        intptr_t dst_offset,
                        gl_format dst_mesaformat,
                        unsigned dst_pitch,
                        unsigned dst_width,
                        unsigned dst_height,
                        unsigned dst_x_offset,
                        unsigned dst_y_offset,
                        unsigned reg_width,
                        unsigned reg_height,
                        unsigned flip_y);
	   unsigned (*is_format_renderable)(gl_format mesa_format);
   } vtbl;
};

#define RADEON_CONTEXT(glctx) ((radeonContextPtr)(ctx->DriverCtx))

static inline __DRIdrawable* radeon_get_drawable(radeonContextPtr radeon)
{
	return radeon->dri.context->driDrawablePriv;
}

static inline __DRIdrawable* radeon_get_readable(radeonContextPtr radeon)
{
	return radeon->dri.context->driReadablePriv;
}

GLboolean radeonInitContext(radeonContextPtr radeon,
			    struct dd_function_table* functions,
			    const struct gl_config * glVisual,
			    __DRIcontext * driContextPriv,
			    void *sharedContextPrivate);

void radeonCleanupContext(radeonContextPtr radeon);
GLboolean radeonUnbindContext(__DRIcontext * driContextPriv);
void radeon_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable,
				 GLboolean front_only);
GLboolean radeonMakeCurrent(__DRIcontext * driContextPriv,
			    __DRIdrawable * driDrawPriv,
			    __DRIdrawable * driReadPriv);
extern void radeonDestroyContext(__DRIcontext * driContextPriv);
void radeon_prepare_render(radeonContextPtr radeon);

#endif
