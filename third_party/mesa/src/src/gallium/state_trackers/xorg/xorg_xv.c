#include "xorg_tracker.h"

#include <xf86xv.h>
#include <X11/extensions/Xv.h>
#include <fourcc.h>

#include "xorg_exa.h"
#include "xorg_renderer.h"
#include "xorg_exa_tgsi.h"

#include "cso_cache/cso_context.h"
#include "util/u_sampler.h"

#include "pipe/p_screen.h"

/*XXX get these from pipe's texture limits */
#define IMAGE_MAX_WIDTH		2048
#define IMAGE_MAX_HEIGHT	2048

#define RES_720P_X 1280
#define RES_720P_Y 720


/* The ITU-R BT.601 conversion matrix for SDTV. */
/* original, matrix, but we transpose it to
 * make the shader easier
static const float bt_601[] = {
    1.0, 0.0, 1.4075,   ,
    1.0, -0.3455, -0.7169, 0,
    1.0, 1.7790, 0., 0,
};*/
static const float bt_601[] = {
    1.0, 1.0, 1.0,        0.5,
    0.0, -0.3455, 1.7790, 0,
    1.4075, -0.7169, 0.,  0,
};

/* The ITU-R BT.709 conversion matrix for HDTV. */
/* original, but we transpose to make the conversion
 * in the shader easier
static const float bt_709[] = {
    1.0, 0.0, 1.581, 0,
    1.0, -0.1881, -0.47, 0,
    1.0, 1.8629, 0., 0,
};*/
static const float bt_709[] = {
    1.0,   1.0,     1.0,     0.5,
    0.0,  -0.1881,  1.8629,  0,
    1.581,-0.47   , 0.0,     0,
};

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvBrightness, xvContrast;

#define NUM_TEXTURED_ATTRIBUTES 2
static XF86AttributeRec TexturedAttributes[NUM_TEXTURED_ATTRIBUTES] = {
   {XvSettable | XvGettable, -128, 127, "XV_BRIGHTNESS"},
   {XvSettable | XvGettable, 0, 255, "XV_CONTRAST"}
};

#define NUM_FORMATS 3
static XF86VideoFormatRec Formats[NUM_FORMATS] = {
   {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

static XF86VideoEncodingRec DummyEncoding[1] = {
   {
      0,
      "XV_IMAGE",
      IMAGE_MAX_WIDTH, IMAGE_MAX_HEIGHT,
      {1, 1}
   }
};

#define NUM_IMAGES 3
static XF86ImageRec Images[NUM_IMAGES] = {
   XVIMAGE_UYVY,
   XVIMAGE_YUY2,
   XVIMAGE_YV12,
};

struct xorg_xv_port_priv {
   struct xorg_renderer *r;

   RegionRec clip;

   int brightness;
   int contrast;

   int current_set;
   /* juggle two sets of seperate Y, U and V
    * textures */
   struct pipe_resource *yuv[2][3];
   struct pipe_sampler_view *yuv_views[2][3];
};


static void
stop_video(ScrnInfoPtr pScrn, pointer data, Bool shutdown)
{
   struct xorg_xv_port_priv *priv = (struct xorg_xv_port_priv *)data;

   REGION_EMPTY(pScrn->pScreen, &priv->clip);
}

static int
set_port_attribute(ScrnInfoPtr pScrn,
                   Atom attribute, INT32 value, pointer data)
{
   struct xorg_xv_port_priv *priv = (struct xorg_xv_port_priv *)data;

   if (attribute == xvBrightness) {
      if ((value < -128) || (value > 127))
         return BadValue;
      priv->brightness = value;
   } else if (attribute == xvContrast) {
      if ((value < 0) || (value > 255))
         return BadValue;
      priv->contrast = value;
   } else
      return BadMatch;

   return Success;
}

static int
get_port_attribute(ScrnInfoPtr pScrn,
                   Atom attribute, INT32 * value, pointer data)
{
   struct xorg_xv_port_priv *priv = (struct xorg_xv_port_priv *)data;

   if (attribute == xvBrightness)
      *value = priv->brightness;
   else if (attribute == xvContrast)
      *value = priv->contrast;
   else
      return BadMatch;

   return Success;
}

static void
query_best_size(ScrnInfoPtr pScrn,
                Bool motion,
                short vid_w, short vid_h,
                short drw_w, short drw_h,
                unsigned int *p_w, unsigned int *p_h, pointer data)
{
   if (vid_w > (drw_w << 1))
      drw_w = vid_w >> 1;
   if (vid_h > (drw_h << 1))
      drw_h = vid_h >> 1;

   *p_w = drw_w;
   *p_h = drw_h;
}

static INLINE struct pipe_resource *
create_component_texture(struct pipe_context *pipe,
                         int width, int height)
{
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource *tex = 0;
   struct pipe_resource templ;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = PIPE_FORMAT_L8_UNORM;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW;

   tex = screen->resource_create(screen, &templ);

   return tex;
}

static int
check_yuv_textures(struct xorg_xv_port_priv *priv,  int width, int height)
{
   struct pipe_resource **dst = priv->yuv[priv->current_set];
   struct pipe_sampler_view **dst_view = priv->yuv_views[priv->current_set];
   struct pipe_sampler_view view_templ;
   struct pipe_context *pipe = priv->r->pipe;

   if (!dst[0] ||
       dst[0]->width0 != width ||
       dst[0]->height0 != height) {
      pipe_resource_reference(&dst[0], NULL);
      pipe_sampler_view_reference(&dst_view[0], NULL);
   }
   if (!dst[1] ||
       dst[1]->width0 != width ||
       dst[1]->height0 != height) {
      pipe_resource_reference(&dst[1], NULL);
      pipe_sampler_view_reference(&dst_view[1], NULL);
   }
   if (!dst[2] ||
       dst[2]->width0 != width ||
       dst[2]->height0 != height) {
      pipe_resource_reference(&dst[2], NULL);
      pipe_sampler_view_reference(&dst_view[2], NULL);
   }

   if (!dst[0]) {
      dst[0] = create_component_texture(priv->r->pipe, width, height);
      if (dst[0]) {
         u_sampler_view_default_template(&view_templ,
                                         dst[0],
                                         dst[0]->format);
         dst_view[0] = pipe->create_sampler_view(pipe, dst[0], &view_templ);
      }
   }

   if (!dst[1]) {
      dst[1] = create_component_texture(priv->r->pipe, width, height);
      if (dst[1]) {
         u_sampler_view_default_template(&view_templ,
                                         dst[1],
                                         dst[1]->format);
         dst_view[1] = pipe->create_sampler_view(pipe, dst[1], &view_templ);
      }
   }

   if (!dst[2]) {
      dst[2] = create_component_texture(priv->r->pipe, width, height);
      if (dst[2]) {
         u_sampler_view_default_template(&view_templ,
                                      dst[2],
                                      dst[2]->format);
         dst_view[2] = pipe->create_sampler_view(pipe, dst[2], &view_templ);
      }
   }

   if (!dst[0] || !dst[1] || !dst[2] || !dst_view[0] || !dst_view[1] || !dst_view[2] )
      return BadAlloc;

   return Success;
}

static int
query_image_attributes(ScrnInfoPtr pScrn,
                       int id,
                       unsigned short *w, unsigned short *h,
                       int *pitches, int *offsets)
{
   int size, tmp;

   if (*w > IMAGE_MAX_WIDTH)
      *w = IMAGE_MAX_WIDTH;
   if (*h > IMAGE_MAX_HEIGHT)
      *h = IMAGE_MAX_HEIGHT;

   *w = (*w + 1) & ~1;
   if (offsets)
      offsets[0] = 0;

   switch (id) {
   case FOURCC_YV12:
      *h = (*h + 1) & ~1;
      size = (*w + 3) & ~3;
      if (pitches) {
         pitches[0] = size;
      }
      size *= *h;
      if (offsets) {
         offsets[1] = size;
      }
      tmp = ((*w >> 1) + 3) & ~3;
      if (pitches) {
         pitches[1] = pitches[2] = tmp;
      }
      tmp *= (*h >> 1);
      size += tmp;
      if (offsets) {
         offsets[2] = size;
      }
      size += tmp;
      break;
   case FOURCC_UYVY:
   case FOURCC_YUY2:
   default:
      size = *w << 1;
      if (pitches)
	 pitches[0] = size;
      size *= *h;
      break;
   }

   return size;
}

static void
copy_packed_data(ScrnInfoPtr pScrn,
                 struct xorg_xv_port_priv *port,
                 int id,
                 unsigned char *buf,
                 int left,
                 int top,
                 unsigned short w, unsigned short h)
{
   int i, j;
   struct pipe_resource **dst = port->yuv[port->current_set];
   struct pipe_transfer *ytrans, *utrans, *vtrans;
   struct pipe_context *pipe = port->r->pipe;
   char *ymap, *vmap, *umap;
   unsigned char y1, y2, u, v;
   int yidx, uidx, vidx;
   int y_array_size = w * h;

   ytrans = pipe_get_transfer(pipe, dst[0],
                              0, 0,
                              PIPE_TRANSFER_WRITE,
                              left, top, w, h);
   utrans = pipe_get_transfer(pipe, dst[1],
                              0, 0,
                              PIPE_TRANSFER_WRITE,
                              left, top, w, h);
   vtrans = pipe_get_transfer(pipe, dst[2],
                              0, 0,
                              PIPE_TRANSFER_WRITE,
                              left, top, w, h);

   ymap = (char*)pipe->transfer_map(pipe, ytrans);
   umap = (char*)pipe->transfer_map(pipe, utrans);
   vmap = (char*)pipe->transfer_map(pipe, vtrans);

   yidx = uidx = vidx = 0;

   switch (id) {
   case FOURCC_YV12: {
      int pitches[3], offsets[3];
      unsigned char *y, *u, *v;
      query_image_attributes(pScrn, FOURCC_YV12,
                             &w, &h, pitches, offsets);

      y = buf + offsets[0];
      v = buf + offsets[1];
      u = buf + offsets[2];
      for (i = 0; i < h; ++i) {
         for (j = 0; j < w; ++j) {
            int yoffset = (w*i+j);
            int ii = (i|1), jj = (j|1);
            int vuoffset = (w/2)*(ii/2) + (jj/2);
            ymap[yidx++] = y[yoffset];
            umap[uidx++] = u[vuoffset];
            vmap[vidx++] = v[vuoffset];
         }
      }
   }
      break;
   case FOURCC_UYVY:
      for (i = 0; i < y_array_size; i +=2 ) {
         /* extracting two pixels */
         u  = buf[0];
         y1 = buf[1];
         v  = buf[2];
         y2 = buf[3];
         buf += 4;

         ymap[yidx++] = y1;
         ymap[yidx++] = y2;
         umap[uidx++] = u;
         umap[uidx++] = u;
         vmap[vidx++] = v;
         vmap[vidx++] = v;
      }
      break;
   case FOURCC_YUY2:
      for (i = 0; i < y_array_size; i +=2 ) {
         /* extracting two pixels */
         y1 = buf[0];
         u  = buf[1];
         y2 = buf[2];
         v  = buf[3];

         buf += 4;

         ymap[yidx++] = y1;
         ymap[yidx++] = y2;
         umap[uidx++] = u;
         umap[uidx++] = u;
         vmap[vidx++] = v;
         vmap[vidx++] = v;
      }
      break;
   default:
      debug_assert(!"Unsupported yuv format!");
      break;
   }

   pipe->transfer_unmap(pipe, ytrans);
   pipe->transfer_unmap(pipe, utrans);
   pipe->transfer_unmap(pipe, vtrans);
   pipe->transfer_destroy(pipe, ytrans);
   pipe->transfer_destroy(pipe, utrans);
   pipe->transfer_destroy(pipe, vtrans);
}


static void
setup_fs_video_constants(struct xorg_renderer *r, boolean hdtv)
{
   const int param_bytes = 12 * sizeof(float);
   const float *video_constants = (hdtv) ? bt_709 : bt_601;

   renderer_set_constants(r, PIPE_SHADER_FRAGMENT,
                          video_constants, param_bytes);
}

static void
draw_yuv(struct xorg_xv_port_priv *port,
         float src_x, float src_y, float src_w, float src_h,
         int dst_x, int dst_y, int dst_w, int dst_h)
{
   struct pipe_resource **textures = port->yuv[port->current_set];

   /*debug_printf("  draw_yuv([%d, %d, %d ,%d], [%d, %d, %d, %d])\n",
                src_x, src_y, src_w, src_h,
                dst_x, dst_y, dst_w, dst_h);*/
   renderer_draw_yuv(port->r,
                     src_x, src_y, src_w, src_h,
                     dst_x, dst_y, dst_w, dst_h,
                     textures);
}

static void
bind_blend_state(struct xorg_xv_port_priv *port)
{
   struct pipe_blend_state blend;

   memset(&blend, 0, sizeof(struct pipe_blend_state));
   blend.rt[0].blend_enable = 0;
   blend.rt[0].colormask = PIPE_MASK_RGBA;

   /* porter&duff src */
   blend.rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;

   cso_set_blend(port->r->cso, &blend);
}


static void
bind_shaders(struct xorg_xv_port_priv *port)
{
   unsigned vs_traits = 0, fs_traits = 0;
   struct xorg_shader shader;

   vs_traits |= VS_YUV;
   fs_traits |= FS_YUV;

   shader = xorg_shaders_get(port->r->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(port->r->cso, shader.vs);
   cso_set_fragment_shader_handle(port->r->cso, shader.fs);
}

static void
bind_samplers(struct xorg_xv_port_priv *port)
{
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_state sampler;
   struct pipe_sampler_view **dst_views = port->yuv_views[port->current_set];

   memset(&sampler, 0, sizeof(struct pipe_sampler_state));

   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   sampler.normalized_coords = 1;

   samplers[0] = &sampler;
   samplers[1] = &sampler;
   samplers[2] = &sampler;


   cso_set_samplers(port->r->cso, PIPE_SHADER_FRAGMENT, 3,
                    (const struct pipe_sampler_state **)samplers);
   cso_set_sampler_views(port->r->cso, PIPE_SHADER_FRAGMENT, 3, dst_views);
}

static int
display_video(ScrnInfoPtr pScrn, struct xorg_xv_port_priv *pPriv, int id,
              RegionPtr dstRegion,
              int src_x, int src_y, int src_w, int src_h,
              int dstX, int dstY, int dst_w, int dst_h,
              PixmapPtr pPixmap)
{
   modesettingPtr ms = modesettingPTR(pScrn);
   BoxPtr pbox;
   int nbox;
   int dxo, dyo;
   Bool hdtv;
   int x, y, w, h;
   struct exa_pixmap_priv *dst;
   struct pipe_surface *dst_surf = NULL;

   exaMoveInPixmap(pPixmap);
   dst = exaGetPixmapDriverPrivate(pPixmap);

   /*debug_printf("display_video([%d, %d, %d, %d], [%d, %d, %d, %d])\n",
     src_x, src_y, src_w, src_h, dstX, dstY, dst_w, dst_h);*/

   if (dst && !dst->tex) {
	xorg_exa_set_shared_usage(pPixmap);
	pScrn->pScreen->ModifyPixmapHeader(pPixmap, 0, 0, 0, 0, 0, NULL);
   }

   if (!dst || !dst->tex)
      XORG_FALLBACK("Xv destination %s", !dst ? "!dst" : "!dst->tex");

   dst_surf = xorg_gpu_surface(pPriv->r->pipe, dst);
   hdtv = ((src_w >= RES_720P_X) && (src_h >= RES_720P_Y));

#ifdef COMPOSITE
   REGION_TRANSLATE(pScrn->pScreen, dstRegion, -pPixmap->screen_x,
                    -pPixmap->screen_y);
#endif

   dxo = dstRegion->extents.x1;
   dyo = dstRegion->extents.y1;

   pbox = REGION_RECTS(dstRegion);
   nbox = REGION_NUM_RECTS(dstRegion);

   renderer_bind_destination(pPriv->r, dst_surf, 
                             dst_surf->width, dst_surf->height);

   bind_blend_state(pPriv);
   bind_shaders(pPriv);
   bind_samplers(pPriv);
   setup_fs_video_constants(pPriv->r, hdtv);

   DamageDamageRegion(&pPixmap->drawable, dstRegion);

   while (nbox--) {
      int box_x1 = pbox->x1;
      int box_y1 = pbox->y1;
      int box_x2 = pbox->x2;
      int box_y2 = pbox->y2;
      float diff_x = (float)src_w / (float)dst_w;
      float diff_y = (float)src_h / (float)dst_h;
      float offset_x = box_x1 - dstX;
      float offset_y = box_y1 - dstY;
      float offset_w;
      float offset_h;

#ifdef COMPOSITE
      offset_x += pPixmap->screen_x;
      offset_y += pPixmap->screen_y;
#endif

      x = box_x1;
      y = box_y1;
      w = box_x2 - box_x1;
      h = box_y2 - box_y1;

      offset_w = dst_w - w;
      offset_h = dst_h - h;

      draw_yuv(pPriv,
               (float) src_x + offset_x*diff_x, (float) src_y + offset_y*diff_y,
               (float) src_w - offset_w*diff_x, (float) src_h - offset_h*diff_y,
               x, y, w, h);

      pbox++;
   }
   DamageRegionProcessPending(&pPixmap->drawable);

   pipe_surface_reference(&dst_surf, NULL);

   return TRUE;
}

static int
put_image(ScrnInfoPtr pScrn,
          short src_x, short src_y,
          short drw_x, short drw_y,
          short src_w, short src_h,
          short drw_w, short drw_h,
          int id, unsigned char *buf,
          short width, short height,
          Bool sync, RegionPtr clipBoxes, pointer data,
          DrawablePtr pDraw)
{
   struct xorg_xv_port_priv *pPriv = (struct xorg_xv_port_priv *) data;
   ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
   PixmapPtr pPixmap;
   INT32 x1, x2, y1, y2;
   BoxRec dstBox;
   int ret;

   /* Clip */
   x1 = src_x;
   x2 = src_x + src_w;
   y1 = src_y;
   y2 = src_y + src_h;

   dstBox.x1 = drw_x;
   dstBox.x2 = drw_x + drw_w;
   dstBox.y1 = drw_y;
   dstBox.y2 = drw_y + drw_h;

   if (!xf86XVClipVideoHelper(&dstBox, &x1, &x2, &y1, &y2, clipBoxes,
			      width, height))
      return Success;

   ret = check_yuv_textures(pPriv, width, height);

   if (ret)
      return ret;

   copy_packed_data(pScrn, pPriv, id, buf,
                    src_x, src_y, width, height);

   if (pDraw->type == DRAWABLE_WINDOW) {
      pPixmap = (*pScreen->GetWindowPixmap)((WindowPtr)pDraw);
   } else {
      pPixmap = (PixmapPtr)pDraw;
   }

   display_video(pScrn, pPriv, id, clipBoxes,
                 src_x, src_y, src_w, src_h,
                 drw_x, drw_y,
                 drw_w, drw_h, pPixmap);

   pPriv->current_set = (pPriv->current_set + 1) & 1;
   return Success;
}

static struct xorg_xv_port_priv *
port_priv_create(struct xorg_renderer *r)
{
   struct xorg_xv_port_priv *priv = NULL;

   priv = calloc(1, sizeof(struct xorg_xv_port_priv));

   if (!priv)
      return NULL;

   priv->r = r;

   REGION_NULL(pScreen, &priv->clip);

   debug_assert(priv && priv->r);

   return priv;
}

static XF86VideoAdaptorPtr
xorg_setup_textured_adapter(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   modesettingPtr ms = modesettingPTR(pScrn);
   XF86VideoAdaptorPtr adapt;
   XF86AttributePtr attrs;
   DevUnion *dev_unions;
   int nports = 16, i;
   int nattributes;

   nattributes = NUM_TEXTURED_ATTRIBUTES;

   debug_assert(ms->exa);
   debug_assert(ms->exa->renderer);

   adapt = calloc(1, sizeof(XF86VideoAdaptorRec));
   dev_unions = calloc(nports, sizeof(DevUnion));
   attrs = calloc(nattributes, sizeof(XF86AttributeRec));
   if (adapt == NULL || dev_unions == NULL || attrs == NULL) {
      free(adapt);
      free(dev_unions);
      free(attrs);
      return NULL;
   }

   adapt->type = XvWindowMask | XvInputMask | XvImageMask;
   adapt->flags = 0;
   adapt->name = "Gallium3D Textured Video";
   adapt->nEncodings = 1;
   adapt->pEncodings = DummyEncoding;
   adapt->nFormats = NUM_FORMATS;
   adapt->pFormats = Formats;
   adapt->nPorts = 0;
   adapt->pPortPrivates = dev_unions;
   adapt->nAttributes = nattributes;
   adapt->pAttributes = attrs;
   memcpy(attrs, TexturedAttributes, nattributes * sizeof(XF86AttributeRec));
   adapt->nImages = NUM_IMAGES;
   adapt->pImages = Images;
   adapt->PutVideo = NULL;
   adapt->PutStill = NULL;
   adapt->GetVideo = NULL;
   adapt->GetStill = NULL;
   adapt->StopVideo = stop_video;
   adapt->SetPortAttribute = set_port_attribute;
   adapt->GetPortAttribute = get_port_attribute;
   adapt->QueryBestSize = query_best_size;
   adapt->PutImage = put_image;
   adapt->QueryImageAttributes = query_image_attributes;

   for (i = 0; i < nports; i++) {
      struct xorg_xv_port_priv *priv =
         port_priv_create(ms->exa->renderer);

      adapt->pPortPrivates[i].ptr = (pointer) (priv);
      adapt->nPorts++;
   }

   return adapt;
}

void
xorg_xv_init(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   /*modesettingPtr ms = modesettingPTR(pScrn);*/
   XF86VideoAdaptorPtr *adaptors, *new_adaptors = NULL;
   XF86VideoAdaptorPtr textured_adapter;
   int num_adaptors;

   num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);
   new_adaptors = malloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr *));
   if (new_adaptors == NULL)
      return;

   memcpy(new_adaptors, adaptors, num_adaptors * sizeof(XF86VideoAdaptorPtr));
   adaptors = new_adaptors;

   /* Add the adaptors supported by our hardware.  First, set up the atoms
    * that will be used by both output adaptors.
    */
   xvBrightness = MAKE_ATOM("XV_BRIGHTNESS");
   xvContrast = MAKE_ATOM("XV_CONTRAST");

   textured_adapter = xorg_setup_textured_adapter(pScreen);

   debug_assert(textured_adapter);

   if (textured_adapter) {
      adaptors[num_adaptors++] = textured_adapter;
   }

   if (num_adaptors) {
      xf86XVScreenInit(pScreen, adaptors, num_adaptors);
      if (textured_adapter)
         xorg_xvmc_init(pScreen, textured_adapter->name);
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                 "Disabling Xv because no adaptors could be initialized.\n");
   }

   free(adaptors);
}
