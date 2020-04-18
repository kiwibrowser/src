#include "swrast/swrast.h"
#include "main/renderbuffer.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/mipmap.h"
#include "drivers/common/meta.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static struct gl_texture_image *
intelNewTextureImage(struct gl_context * ctx)
{
   DBG("%s\n", __FUNCTION__);
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(intel_texture_image);
}

static void
intelDeleteTextureImage(struct gl_context * ctx, struct gl_texture_image *img)
{
   /* nothing special (yet) for intel_texture_image */
   _mesa_delete_texture_image(ctx, img);
}


static struct gl_texture_object *
intelNewTextureObject(struct gl_context * ctx, GLuint name, GLenum target)
{
   struct intel_texture_object *obj = CALLOC_STRUCT(intel_texture_object);

   (void) ctx;

   DBG("%s\n", __FUNCTION__);
   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

static void 
intelDeleteTextureObject(struct gl_context *ctx,
			 struct gl_texture_object *texObj)
{
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   intel_miptree_release(&intelObj->mt);
   _mesa_delete_texture_object(ctx, texObj);
}

static GLboolean
intel_alloc_texture_image_buffer(struct gl_context *ctx,
				 struct gl_texture_image *image)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);
   GLuint slices;

   assert(image->Border == 0);

   /* Because the driver uses AllocTextureImageBuffer() internally, it may end
    * up mismatched with FreeTextureImageBuffer(), but that is safe to call
    * multiple times.
    */
   ctx->Driver.FreeTextureImageBuffer(ctx, image);

   /* Allocate the swrast_texture_image::ImageOffsets array now */
   switch (texobj->Target) {
   case GL_TEXTURE_3D:
   case GL_TEXTURE_2D_ARRAY:
      slices = image->Depth;
      break;
   case GL_TEXTURE_1D_ARRAY:
      slices = image->Height;
      break;
   default:
      slices = 1;
   }
   assert(!intel_image->base.ImageOffsets);
   intel_image->base.ImageOffsets = malloc(slices * sizeof(GLuint));

   _swrast_init_texture_image(image);

   if (intel_texobj->mt &&
       intel_miptree_match_image(intel_texobj->mt, image)) {
      intel_miptree_reference(&intel_image->mt, intel_texobj->mt);
      DBG("%s: alloc obj %p level %d %dx%dx%d using object's miptree %p\n",
          __FUNCTION__, texobj, image->Level,
          image->Width, image->Height, image->Depth, intel_texobj->mt);
   } else {
      intel_image->mt = intel_miptree_create_for_teximage(intel, intel_texobj,
                                                          intel_image,
                                                          false);

      /* Even if the object currently has a mipmap tree associated
       * with it, this one is a more likely candidate to represent the
       * whole object since our level didn't fit what was there
       * before, and any lower levels would fit into our miptree.
       */
      intel_miptree_reference(&intel_texobj->mt, intel_image->mt);

      DBG("%s: alloc obj %p level %d %dx%dx%d using new miptree %p\n",
          __FUNCTION__, texobj, image->Level,
          image->Width, image->Height, image->Depth, intel_image->mt);
   }

   return true;
}

/**
 * Called via ctx->Driver.AllocTextureStorage()
 * Just have to allocate memory for the texture images.
 */
static GLboolean
intel_alloc_texture_storage(struct gl_context *ctx,
                            struct gl_texture_object *texObj,
                            GLsizei levels, GLsizei width,
                            GLsizei height, GLsizei depth)
{
   const int numFaces = _mesa_num_tex_faces(texObj->Target);
   int face;
   int level;

   for (face = 0; face < numFaces; face++) {
      for (level = 0; level < levels; level++) {
         struct gl_texture_image *const texImage = texObj->Image[face][level];
         if (!intel_alloc_texture_image_buffer(ctx, texImage))
            return false;
      }
   }

   return true;
}

static void
intel_free_texture_image_buffer(struct gl_context * ctx,
				struct gl_texture_image *texImage)
{
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   intel_miptree_release(&intelImage->mt);

   if (intelImage->base.Buffer) {
      _mesa_align_free(intelImage->base.Buffer);
      intelImage->base.Buffer = NULL;
   }

   if (intelImage->base.ImageOffsets) {
      free(intelImage->base.ImageOffsets);
      intelImage->base.ImageOffsets = NULL;
   }
}

/**
 * Map texture memory/buffer into user space.
 * Note: the region of interest parameters are ignored here.
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 * \param mapOut  returns start of mapping of region of interest
 * \param rowStrideOut  returns row stride in bytes
 */
static void
intel_map_texture_image(struct gl_context *ctx,
			struct gl_texture_image *tex_image,
			GLuint slice,
			GLuint x, GLuint y, GLuint w, GLuint h,
			GLbitfield mode,
			GLubyte **map,
			GLint *stride)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(tex_image);
   struct intel_mipmap_tree *mt = intel_image->mt;

   /* Our texture data is always stored in a miptree. */
   assert(mt);

   /* Check that our caller wasn't confused about how to map a 1D texture. */
   assert(tex_image->TexObject->Target != GL_TEXTURE_1D_ARRAY ||
	  h == 1);

   /* intel_miptree_map operates on a unified "slice" number that references the
    * cube face, since it's all just slices to the miptree code.
    */
   if (tex_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      slice = tex_image->Face;

   intel_miptree_map(intel, mt, tex_image->Level, slice, x, y, w, h, mode,
		     (void **)map, stride);
}

static void
intel_unmap_texture_image(struct gl_context *ctx,
			  struct gl_texture_image *tex_image, GLuint slice)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(tex_image);
   struct intel_mipmap_tree *mt = intel_image->mt;

   if (tex_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      slice = tex_image->Face;

   intel_miptree_unmap(intel, mt, tex_image->Level, slice);
}

void
intelInitTextureFuncs(struct dd_function_table *functions)
{
   functions->NewTextureObject = intelNewTextureObject;
   functions->NewTextureImage = intelNewTextureImage;
   functions->DeleteTextureImage = intelDeleteTextureImage;
   functions->DeleteTexture = intelDeleteTextureObject;
   functions->AllocTextureImageBuffer = intel_alloc_texture_image_buffer;
   functions->FreeTextureImageBuffer = intel_free_texture_image_buffer;
   functions->AllocTextureStorage = intel_alloc_texture_storage;
   functions->MapTextureImage = intel_map_texture_image;
   functions->UnmapTextureImage = intel_unmap_texture_image;
}
