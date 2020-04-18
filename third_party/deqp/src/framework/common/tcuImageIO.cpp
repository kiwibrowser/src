/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Image IO.
 *//*--------------------------------------------------------------------*/

#include "tcuImageIO.hpp"
#include "tcuResource.hpp"
#include "tcuSurface.hpp"
#include "tcuCompressedTexture.hpp"
#include "deFilePath.hpp"
#include "deUniquePtr.hpp"

#include <string>
#include <vector>
#include <cstdio>

#include "png.h"

namespace tcu
{
namespace ImageIO
{

using std::string;
using std::vector;

/*--------------------------------------------------------------------*//*!
 * \brief Load image from resource
 *
 * TextureLevel storage is set to match image data. Only PNG format is
 * currently supported.
 *
 * \param dst		Destination pixel container
 * \param archive	Resource archive
 * \param fileName	Resource file name
 *//*--------------------------------------------------------------------*/
void loadImage (TextureLevel& dst, const tcu::Archive& archive, const char* fileName)
{
	string ext = de::FilePath(fileName).getFileExtension();

	if (ext == "png" || ext == "PNG")
		loadPNG(dst, archive, fileName);
	else
		throw InternalError("Unrecognized image file extension", fileName, __FILE__, __LINE__);
}

DE_BEGIN_EXTERN_C
static void pngReadResource (png_structp png_ptr, png_bytep data, png_size_t length)
{
	tcu::Resource* resource = (tcu::Resource*)png_get_io_ptr(png_ptr);
	resource->read(data, (int)length);
}
DE_END_EXTERN_C

/*--------------------------------------------------------------------*//*!
 * \brief Load PNG image from resource
 *
 * TextureLevel storage is set to match image data.
 *
 * \param dst		Destination pixel container
 * \param archive	Resource archive
 * \param fileName	Resource file name
 *//*--------------------------------------------------------------------*/
void loadPNG (TextureLevel& dst, const tcu::Archive& archive, const char* fileName)
{
	de::UniquePtr<Resource> resource(archive.getResource(fileName));

	// Verify header.
	deUint8 header[8];
	resource->read(header, sizeof(header));
	TCU_CHECK(png_sig_cmp((png_bytep)&header[0], 0, DE_LENGTH_OF_ARRAY(header)) == 0);

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, DE_NULL, DE_NULL, DE_NULL);
	TCU_CHECK(png_ptr);

	png_infop info_ptr = png_create_info_struct(png_ptr);
	TCU_CHECK(info_ptr);

	if (setjmp(png_jmpbuf(png_ptr)))
		throw InternalError("An error occured when loading PNG", fileName, __FILE__, __LINE__);

	png_set_read_fn(png_ptr, resource.get(), pngReadResource);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	const deUint32	width			= (deUint32)png_get_image_width(png_ptr, info_ptr);
	const deUint32	height			= (deUint32)png_get_image_height(png_ptr, info_ptr);
	TextureFormat	textureFormat;

	{
		const png_byte	colorType	= png_get_color_type(png_ptr, info_ptr);
		const png_byte	bitDepth	= png_get_bit_depth(png_ptr, info_ptr);

		if (colorType == PNG_COLOR_TYPE_RGB && bitDepth == 8)
			textureFormat = TextureFormat(TextureFormat::RGB, TextureFormat::UNORM_INT8);
		else if (colorType == PNG_COLOR_TYPE_RGBA && bitDepth == 8)
			textureFormat = TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8);
		else
			throw InternalError("Unsupported PNG depth or color type", fileName, __FILE__, __LINE__);
	}

	// Resize destination texture.
	dst.setStorage(textureFormat, width, height);

	std::vector<png_bytep> row_pointers;
	row_pointers.resize(height);
	for (deUint32 y = 0; y < height; y++)
		row_pointers[y] = (deUint8*)dst.getAccess().getDataPtr() + y*dst.getAccess().getRowPitch();

	png_read_image(png_ptr, &row_pointers[0]);

	png_destroy_info_struct(png_ptr, &info_ptr);
	png_destroy_read_struct(&png_ptr, DE_NULL, DE_NULL);
}

static int textureFormatToPNGFormat (const TextureFormat& format)
{
	if (format == TextureFormat(TextureFormat::RGB, TextureFormat::UNORM_INT8))
		return PNG_COLOR_TYPE_RGB;
	else if (format == TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8))
		return PNG_COLOR_TYPE_RGBA;
	else
		throw InternalError("Unsupported texture format", DE_NULL, __FILE__, __LINE__);
}

/*--------------------------------------------------------------------*//*!
 * \brief Write image to file in PNG format
 *
 * This is provided for debugging and development purposes. Test code must
 * not write to any files except the test log by default.
 *
 * \note Only RGB/RGBA, UNORM_INT8 formats are supported
 * \param src		Source pixel data
 * \param fileName	File name
 *//*--------------------------------------------------------------------*/
void savePNG (const ConstPixelBufferAccess& src, const char* fileName)
{
	FILE*	fp			= fopen(fileName, "wb");
	TCU_CHECK(fp);

	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!pngPtr)
	{
		fclose(fp);
		TCU_CHECK(pngPtr);
	}

	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
	{
		png_destroy_write_struct(&pngPtr, NULL);
		TCU_CHECK(infoPtr);
	}

	if (setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_write_struct(&pngPtr, &infoPtr);
		fclose(fp);
		throw tcu::InternalError("PNG compression failed");
	}
	else
	{
		int pngFormat = textureFormatToPNGFormat(src.getFormat());

		png_init_io(pngPtr, fp);

		// Header
		png_set_IHDR(pngPtr, infoPtr, src.getWidth(), src.getHeight(), 8,
					 pngFormat, PNG_INTERLACE_NONE,
					 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(pngPtr, infoPtr);

		std::vector<png_bytep> rowPointers(src.getHeight());
		for (int y = 0; y < src.getHeight(); y++)
			rowPointers[y] = (deUint8*)src.getDataPtr() + y*src.getRowPitch();

		png_write_image(pngPtr, &rowPointers[0]);
		png_write_end(pngPtr, NULL);

		png_destroy_write_struct(&pngPtr, &infoPtr);
		fclose(fp);
	}
}

enum PkmImageFormat
{
	ETC1_RGB_NO_MIPMAPS		= 0,
	ETC1_RGBA_NO_MIPMAPS	= 1,
	ETC1_RGB_MIPMAPS		= 2,
	ETC1_RGBA_MIPMAPS		= 3
};

static inline deUint16 readBigEndianShort (tcu::Resource* resource)
{
	deUint16 val;
	resource->read((deUint8*)&val, sizeof(val));
	return (deUint16)(((val >> 8) & 0xFF) | ((val << 8) & 0xFF00));
}

/*--------------------------------------------------------------------*//*!
 * \brief Load compressed image data from PKM file
 *
 * \note			Only ETC1_RGB8_NO_MIPMAPS format is supported
 * \param dst		Destination pixel container
 * \param archive	Resource archive
 * \param fileName	Resource file name
 *//*--------------------------------------------------------------------*/
void loadPKM (CompressedTexture& dst, const tcu::Archive& archive, const char* fileName)
{
	de::UniquePtr<Resource> resource(archive.getResource(fileName));

	// Check magic and version.
	deUint8 refMagic[] = {'P', 'K', 'M', ' ', '1', '0'};
	deUint8 magic[6];
	resource->read(magic, DE_LENGTH_OF_ARRAY(magic));

	if (memcmp(refMagic, magic, sizeof(magic)) != 0)
		throw InternalError("Signature doesn't match PKM signature", resource->getName().c_str(), __FILE__, __LINE__);

	deUint16 type = readBigEndianShort(resource.get());
	if (type != ETC1_RGB_NO_MIPMAPS)
		throw InternalError("Unsupported PKM type", resource->getName().c_str(), __FILE__, __LINE__);

	deUint16	width			= readBigEndianShort(resource.get());
	deUint16	height			= readBigEndianShort(resource.get());
	deUint16	activeWidth		= readBigEndianShort(resource.get());
	deUint16	activeHeight	= readBigEndianShort(resource.get());

    DE_UNREF(width && height);

	dst.setStorage(COMPRESSEDTEXFORMAT_ETC1_RGB8, (int)activeWidth, (int)activeHeight);
	resource->read((deUint8*)dst.getData(), dst.getDataSize());
}

} // ImageIO
} // tcu
