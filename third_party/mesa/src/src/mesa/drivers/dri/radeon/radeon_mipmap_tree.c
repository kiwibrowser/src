/*
 * Copyright (C) 2009 Maciej Cencora.
 * Copyright (C) 2008 Nicolai Haehnle.
 *
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

#include "radeon_mipmap_tree.h"

#include <errno.h>
#include <unistd.h>

#include "main/simple_list.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/enums.h"
#include "radeon_texture.h"
#include "radeon_tile.h"

static unsigned get_aligned_compressed_row_stride(
		gl_format format,
		unsigned width,
		unsigned minStride)
{
	const unsigned blockBytes = _mesa_get_format_bytes(format);
	unsigned blockWidth, blockHeight;
	unsigned stride;

	_mesa_get_format_block_size(format, &blockWidth, &blockHeight);

	/* Count number of blocks required to store the given width.
	 * And then multiple it with bytes required to store a block.
	 */
	stride = (width + blockWidth - 1) / blockWidth * blockBytes;

	/* Round the given minimum stride to the next full blocksize.
	 * (minStride + blockBytes - 1) / blockBytes * blockBytes
	 */
	if ( stride < minStride )
		stride = (minStride + blockBytes - 1) / blockBytes * blockBytes;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
			"%s width %u, minStride %u, block(bytes %u, width %u):"
			"stride %u\n",
			__func__, width, minStride,
			blockBytes, blockWidth,
			stride);

	return stride;
}

unsigned get_texture_image_size(
		gl_format format,
		unsigned rowStride,
		unsigned height,
		unsigned depth,
		unsigned tiling)
{
	if (_mesa_is_format_compressed(format)) {
		unsigned blockWidth, blockHeight;

		_mesa_get_format_block_size(format, &blockWidth, &blockHeight);

		return rowStride * ((height + blockHeight - 1) / blockHeight) * depth;
	} else if (tiling) {
		/* Need to align height to tile height */
		unsigned tileWidth, tileHeight;

		get_tile_size(format, &tileWidth, &tileHeight);
		tileHeight--;

		height = (height + tileHeight) & ~tileHeight;
	}

	return rowStride * height * depth;
}

unsigned get_texture_image_row_stride(radeonContextPtr rmesa, gl_format format, unsigned width, unsigned tiling, GLuint target)
{
	if (_mesa_is_format_compressed(format)) {
		return get_aligned_compressed_row_stride(format, width, rmesa->texture_compressed_row_align);
	} else {
		unsigned row_align;

		if (!_mesa_is_pow_two(width) || target == GL_TEXTURE_RECTANGLE) {
			row_align = rmesa->texture_rect_row_align - 1;
		} else if (tiling) {
			unsigned tileWidth, tileHeight;
			get_tile_size(format, &tileWidth, &tileHeight);
			row_align = tileWidth * _mesa_get_format_bytes(format) - 1;
		} else {
			row_align = rmesa->texture_row_align - 1;
		}

		return (_mesa_format_row_stride(format, width) + row_align) & ~row_align;
	}
}

/**
 * Compute sizes and fill in offset and blit information for the given
 * image (determined by \p face and \p level).
 *
 * \param curOffset points to the offset at which the image is to be stored
 * and is updated by this function according to the size of the image.
 */
static void compute_tex_image_offset(radeonContextPtr rmesa, radeon_mipmap_tree *mt,
	GLuint face, GLuint level, GLuint* curOffset)
{
	radeon_mipmap_level *lvl = &mt->levels[level];
	GLuint height;

	height = _mesa_next_pow_two_32(lvl->height);

	lvl->rowstride = get_texture_image_row_stride(rmesa, mt->mesaFormat, lvl->width, mt->tilebits, mt->target);
	lvl->size = get_texture_image_size(mt->mesaFormat, lvl->rowstride, height, lvl->depth, mt->tilebits);

	assert(lvl->size > 0);

	lvl->faces[face].offset = *curOffset;
	*curOffset += lvl->size;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
			"%s(%p) level %d, face %d: rs:%d %dx%d at %d\n",
			__func__, rmesa,
			level, face,
			lvl->rowstride, lvl->width, height, lvl->faces[face].offset);
}

static GLuint minify(GLuint size, GLuint levels)
{
	size = size >> levels;
	if (size < 1)
		size = 1;
	return size;
}


static void calculate_miptree_layout(radeonContextPtr rmesa, radeon_mipmap_tree *mt)
{
	GLuint curOffset, i, face, level;

	assert(mt->numLevels <= rmesa->glCtx->Const.MaxTextureLevels);

	curOffset = 0;
	for(face = 0; face < mt->faces; face++) {

		for(i = 0, level = mt->baseLevel; i < mt->numLevels; i++, level++) {
			mt->levels[level].valid = 1;
			mt->levels[level].width = minify(mt->width0, i);
			mt->levels[level].height = minify(mt->height0, i);
			mt->levels[level].depth = minify(mt->depth0, i);
			compute_tex_image_offset(rmesa, mt, face, level, &curOffset);
		}
	}

	/* Note the required size in memory */
	mt->totalsize = (curOffset + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
			"%s(%p, %p) total size %d\n",
			__func__, rmesa, mt, mt->totalsize);
}

/**
 * Create a new mipmap tree, calculate its layout and allocate memory.
 */
radeon_mipmap_tree* radeon_miptree_create(radeonContextPtr rmesa,
					  GLenum target, gl_format mesaFormat, GLuint baseLevel, GLuint numLevels,
					  GLuint width0, GLuint height0, GLuint depth0, GLuint tilebits)
{
	radeon_mipmap_tree *mt = CALLOC_STRUCT(_radeon_mipmap_tree);

	radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
		"%s(%p) new tree is %p.\n",
		__func__, rmesa, mt);

	mt->mesaFormat = mesaFormat;
	mt->refcount = 1;
	mt->target = target;
	mt->faces = _mesa_num_tex_faces(target);
	mt->baseLevel = baseLevel;
	mt->numLevels = numLevels;
	mt->width0 = width0;
	mt->height0 = height0;
	mt->depth0 = depth0;
	mt->tilebits = tilebits;

	calculate_miptree_layout(rmesa, mt);

	mt->bo = radeon_bo_open(rmesa->radeonScreen->bom,
                            0, mt->totalsize, 1024,
                            RADEON_GEM_DOMAIN_VRAM,
                            0);

	return mt;
}

void radeon_miptree_reference(radeon_mipmap_tree *mt, radeon_mipmap_tree **ptr)
{
	assert(!*ptr);

	mt->refcount++;
	assert(mt->refcount > 0);

	*ptr = mt;
}

void radeon_miptree_unreference(radeon_mipmap_tree **ptr)
{
	radeon_mipmap_tree *mt = *ptr;
	if (!mt)
		return;

	assert(mt->refcount > 0);

	mt->refcount--;
	if (!mt->refcount) {
		radeon_bo_unref(mt->bo);
		free(mt);
	}

	*ptr = 0;
}

/**
 * Calculate min and max LOD for the given texture object.
 * @param[in] tObj texture object whose LOD values to calculate
 * @param[out] pminLod minimal LOD
 * @param[out] pmaxLod maximal LOD
 */
static void calculate_min_max_lod(struct gl_sampler_object *samp, struct gl_texture_object *tObj,
				       unsigned *pminLod, unsigned *pmaxLod)
{
	int minLod, maxLod;
	/* Yes, this looks overly complicated, but it's all needed.
	*/
	switch (tObj->Target) {
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_3D:
	case GL_TEXTURE_CUBE_MAP:
		if (samp->MinFilter == GL_NEAREST || samp->MinFilter == GL_LINEAR) {
			/* GL_NEAREST and GL_LINEAR only care about GL_TEXTURE_BASE_LEVEL.
			*/
			minLod = maxLod = tObj->BaseLevel;
		} else {
			minLod = tObj->BaseLevel + (GLint)(samp->MinLod);
			minLod = MAX2(minLod, tObj->BaseLevel);
			minLod = MIN2(minLod, tObj->MaxLevel);
			maxLod = tObj->BaseLevel + (GLint)(samp->MaxLod + 0.5);
			maxLod = MIN2(maxLod, tObj->MaxLevel);
			maxLod = MIN2(maxLod, tObj->Image[0][minLod]->MaxNumLevels - 1 + minLod);
			maxLod = MAX2(maxLod, minLod); /* need at least one level */
		}
		break;
	case GL_TEXTURE_RECTANGLE_NV:
	case GL_TEXTURE_4D_SGIS:
		minLod = maxLod = 0;
		break;
	default:
		return;
	}

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
			"%s(%p) target %s, min %d, max %d.\n",
			__func__, tObj,
			_mesa_lookup_enum_by_nr(tObj->Target),
			minLod, maxLod);

	/* save these values */
	*pminLod = minLod;
	*pmaxLod = maxLod;
}

/**
 * Checks whether the given miptree can hold the given texture image at the
 * given face and level.
 */
GLboolean radeon_miptree_matches_image(radeon_mipmap_tree *mt,
				       struct gl_texture_image *texImage)
{
	radeon_mipmap_level *lvl;
	GLuint level = texImage->Level;
	if (texImage->TexFormat != mt->mesaFormat)
		return GL_FALSE;

	lvl = &mt->levels[level];
	if (!lvl->valid ||
	    lvl->width != texImage->Width ||
	    lvl->height != texImage->Height ||
	    lvl->depth != texImage->Depth)
		return GL_FALSE;

	return GL_TRUE;
}

/**
 * Checks whether the given miptree has the right format to store the given texture object.
 */
static GLboolean radeon_miptree_matches_texture(radeon_mipmap_tree *mt, struct gl_texture_object *texObj)
{
	struct gl_texture_image *firstImage;
	unsigned numLevels;
	radeon_mipmap_level *mtBaseLevel;

	if (texObj->BaseLevel < mt->baseLevel)
		return GL_FALSE;

	mtBaseLevel = &mt->levels[texObj->BaseLevel - mt->baseLevel];
	firstImage = texObj->Image[0][texObj->BaseLevel];
	numLevels = MIN2(texObj->_MaxLevel - texObj->BaseLevel + 1, firstImage->MaxNumLevels);

	if (radeon_is_debug_enabled(RADEON_TEXTURE,RADEON_TRACE)) {
		fprintf(stderr, "Checking if miptree %p matches texObj %p\n", mt, texObj);
		fprintf(stderr, "target %d vs %d\n", mt->target, texObj->Target);
		fprintf(stderr, "format %d vs %d\n", mt->mesaFormat, firstImage->TexFormat);
		fprintf(stderr, "numLevels %d vs %d\n", mt->numLevels, numLevels);
		fprintf(stderr, "width0 %d vs %d\n", mtBaseLevel->width, firstImage->Width);
		fprintf(stderr, "height0 %d vs %d\n", mtBaseLevel->height, firstImage->Height);
		fprintf(stderr, "depth0 %d vs %d\n", mtBaseLevel->depth, firstImage->Depth);
		if (mt->target == texObj->Target &&
	        mt->mesaFormat == firstImage->TexFormat &&
	        mt->numLevels >= numLevels &&
	        mtBaseLevel->width == firstImage->Width &&
	        mtBaseLevel->height == firstImage->Height &&
	        mtBaseLevel->depth == firstImage->Depth) {
			fprintf(stderr, "MATCHED\n");
		} else {
			fprintf(stderr, "NOT MATCHED\n");
		}
	}

	return (mt->target == texObj->Target &&
	        mt->mesaFormat == firstImage->TexFormat &&
	        mt->numLevels >= numLevels &&
	        mtBaseLevel->width == firstImage->Width &&
	        mtBaseLevel->height == firstImage->Height &&
	        mtBaseLevel->depth == firstImage->Depth);
}

/**
 * Try to allocate a mipmap tree for the given texture object.
 * @param[in] rmesa radeon context
 * @param[in] t radeon texture object
 */
void radeon_try_alloc_miptree(radeonContextPtr rmesa, radeonTexObj *t)
{
	struct gl_texture_object *texObj = &t->base;
	struct gl_texture_image *texImg = texObj->Image[0][texObj->BaseLevel];
	GLuint numLevels;
	assert(!t->mt);

	if (!texImg) {
		radeon_warning("%s(%p) No image in given texture object(%p).\n",
				__func__, rmesa, t);
		return;
	}


	numLevels = MIN2(texObj->MaxLevel - texObj->BaseLevel + 1, texImg->MaxNumLevels);

	t->mt = radeon_miptree_create(rmesa, t->base.Target,
		texImg->TexFormat, texObj->BaseLevel,
		numLevels, texImg->Width, texImg->Height,
		texImg->Depth, t->tile_bits);
}

GLuint
radeon_miptree_image_offset(radeon_mipmap_tree *mt,
			    GLuint face, GLuint level)
{
	if (mt->target == GL_TEXTURE_CUBE_MAP_ARB)
		return (mt->levels[level].faces[face].offset);
	else
		return mt->levels[level].faces[0].offset;
}

/**
 * Ensure that the given image is stored in the given miptree from now on.
 */
static void migrate_image_to_miptree(radeon_mipmap_tree *mt,
									 radeon_texture_image *image,
									 int face, int level)
{
	radeon_mipmap_level *dstlvl = &mt->levels[level];
	unsigned char *dest;

	assert(image->mt != mt);
	assert(dstlvl->valid);
	assert(dstlvl->width == image->base.Base.Width);
	assert(dstlvl->height == image->base.Base.Height);
	assert(dstlvl->depth == image->base.Base.Depth);

	radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
			"%s miptree %p, image %p, face %d, level %d.\n",
			__func__, mt, image, face, level);

	radeon_bo_map(mt->bo, GL_TRUE);
	dest = mt->bo->ptr + dstlvl->faces[face].offset;

	if (image->mt) {
		/* Format etc. should match, so we really just need a memcpy().
		 * In fact, that memcpy() could be done by the hardware in many
		 * cases, provided that we have a proper memory manager.
		 */
		assert(mt->mesaFormat == image->base.Base.TexFormat);

		radeon_mipmap_level *srclvl = &image->mt->levels[image->base.Base.Level];

		assert(image->base.Base.Level == level);
		assert(srclvl->size == dstlvl->size);
		assert(srclvl->rowstride == dstlvl->rowstride);

		radeon_bo_map(image->mt->bo, GL_FALSE);

		memcpy(dest,
			image->mt->bo->ptr + srclvl->faces[face].offset,
			dstlvl->size);
		radeon_bo_unmap(image->mt->bo);

		radeon_miptree_unreference(&image->mt);
	} else if (image->base.Map) {
		/* This condition should be removed, it's here to workaround
		 * a segfault when mapping textures during software fallbacks.
		 */
		radeon_print(RADEON_FALLBACKS, RADEON_IMPORTANT,
				"%s Trying to map texture in software fallback.\n",
				__func__);
		const uint32_t srcrowstride = _mesa_format_row_stride(image->base.Base.TexFormat, image->base.Base.Width);
		uint32_t rows = image->base.Base.Height * image->base.Base.Depth;

		if (_mesa_is_format_compressed(image->base.Base.TexFormat)) {
			uint32_t blockWidth, blockHeight;
			_mesa_get_format_block_size(image->base.Base.TexFormat, &blockWidth, &blockHeight);
			rows = (rows + blockHeight - 1) / blockHeight;
		}

		copy_rows(dest, dstlvl->rowstride, image->base.Map, srcrowstride,
				  rows, srcrowstride);

		_mesa_align_free(image->base.Map);
		image->base.Map = 0;
	}

	radeon_bo_unmap(mt->bo);

	radeon_miptree_reference(mt, &image->mt);
}

/**
 * Filter matching miptrees, and select one with the most of data.
 * @param[in] texObj radeon texture object
 * @param[in] firstLevel first texture level to check
 * @param[in] lastLevel last texture level to check
 */
static radeon_mipmap_tree * get_biggest_matching_miptree(radeonTexObj *texObj,
														 unsigned firstLevel,
														 unsigned lastLevel)
{
	const unsigned numLevels = lastLevel - firstLevel + 1;
	unsigned *mtSizes = calloc(numLevels, sizeof(unsigned));
	radeon_mipmap_tree **mts = calloc(numLevels, sizeof(radeon_mipmap_tree *));
	unsigned mtCount = 0;
	unsigned maxMtIndex = 0;
	radeon_mipmap_tree *tmp;
	unsigned int level;
	int i;

	for (level = firstLevel; level <= lastLevel; ++level) {
		radeon_texture_image *img = get_radeon_texture_image(texObj->base.Image[0][level]);
		unsigned found = 0;
		// TODO: why this hack??
		if (!img)
			break;

		if (!img->mt)
			continue;

		for (i = 0; i < mtCount; ++i) {
			if (mts[i] == img->mt) {
				found = 1;
				mtSizes[i] += img->mt->levels[img->base.Base.Level].size;
				break;
			}
		}

		if (!found && radeon_miptree_matches_texture(img->mt, &texObj->base)) {
			mtSizes[mtCount] = img->mt->levels[img->base.Base.Level].size;
			mts[mtCount] = img->mt;
			mtCount++;
		}
	}

	if (mtCount == 0) {
		free(mtSizes);
		free(mts);
		return NULL;
	}

	for (i = 1; i < mtCount; ++i) {
		if (mtSizes[i] > mtSizes[maxMtIndex]) {
			maxMtIndex = i;
		}
	}

	tmp = mts[maxMtIndex];
	free(mtSizes);
	free(mts);

	return tmp;
}

/**
 * Validate texture mipmap tree.
 * If individual images are stored in different mipmap trees
 * use the mipmap tree that has the most of the correct data.
 */
int radeon_validate_texture_miptree(struct gl_context * ctx,
				    struct gl_sampler_object *samp,
				    struct gl_texture_object *texObj)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	radeonTexObj *t = radeon_tex_obj(texObj);
	radeon_mipmap_tree *dst_miptree;

	if (samp == &texObj->Sampler && (t->validated || t->image_override)) {
		return GL_TRUE;
	}

	calculate_min_max_lod(samp, &t->base, &t->minLod, &t->maxLod);

	radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s: Validating texture %p now, minLod = %d, maxLod = %d\n",
			__FUNCTION__, texObj ,t->minLod, t->maxLod);

	dst_miptree = get_biggest_matching_miptree(t, t->base.BaseLevel, t->base._MaxLevel);

	radeon_miptree_unreference(&t->mt);
	if (!dst_miptree) {
		radeon_try_alloc_miptree(rmesa, t);
		radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s: No matching miptree found, allocated new one %p\n",
			__FUNCTION__, t->mt);

	} else {
		radeon_miptree_reference(dst_miptree, &t->mt);
		radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
			"%s: Using miptree %p\n", __FUNCTION__, t->mt);
	}

	const unsigned faces = _mesa_num_tex_faces(texObj->Target);
	unsigned face, level;
	radeon_texture_image *img;
	/* Validate only the levels that will actually be used during rendering */
	for (face = 0; face < faces; ++face) {
		for (level = t->minLod; level <= t->maxLod; ++level) {
			img = get_radeon_texture_image(texObj->Image[face][level]);

			radeon_print(RADEON_TEXTURE, RADEON_TRACE,
				"Checking image level %d, face %d, mt %p ... ",
				level, face, img->mt);
			
			if (img->mt != t->mt && !img->used_as_render_target) {
				radeon_print(RADEON_TEXTURE, RADEON_TRACE,
					"MIGRATING\n");

				struct radeon_bo *src_bo = (img->mt) ? img->mt->bo : img->bo;
				if (src_bo && radeon_bo_is_referenced_by_cs(src_bo, rmesa->cmdbuf.cs)) {
					radeon_firevertices(rmesa);
				}
				migrate_image_to_miptree(t->mt, img, face, level);
			} else
				radeon_print(RADEON_TEXTURE, RADEON_TRACE, "OK\n");
		}
	}

	t->validated = GL_TRUE;

	return GL_TRUE;
}

uint32_t get_base_teximage_offset(radeonTexObj *texObj)
{
	if (!texObj->mt) {
		return 0;
	} else {
		return radeon_miptree_image_offset(texObj->mt, 0, texObj->minLod);
	}
}
