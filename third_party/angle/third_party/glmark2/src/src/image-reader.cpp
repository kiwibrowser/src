/*
 * Copyright © 2012 Linaro Limited
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Alexandros Frantzis
 */
#include <png.h>
#include <jpeglib.h>
#include <cstring>
#include <memory>

#include "image-reader.h"
#include "log.h"
#include "util.h"

/*******
 * PNG *
 *******/

struct PNGReaderPrivate
{
    PNGReaderPrivate() :
        png(0), info(0), rows(0), png_error(0),
        current_row(0), row_stride(0) {}

    static void png_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        std::istream *is = reinterpret_cast<std::istream*>(png_get_io_ptr(png_ptr));
        is->read(reinterpret_cast<char *>(data), length);
    }

    png_structp png;
    png_infop info;
    png_bytepp rows;
    bool png_error;
    unsigned int current_row;
    unsigned int row_stride;
};

PNGReader::PNGReader(const std::string& filename):
    priv_(new PNGReaderPrivate())
{
    priv_->png_error = !init(filename);
}

PNGReader::~PNGReader()
{
    finish();
    delete priv_;
}

bool
PNGReader::error()
{
    return priv_->png_error;
}

bool
PNGReader::nextRow(unsigned char *dst)
{
    bool ret;

    if (priv_->current_row < height()) {
        memcpy(dst, priv_->rows[priv_->current_row], priv_->row_stride);
        priv_->current_row++;
        ret = true;
    }
    else {
        ret = false;
    }

    return ret;
}

unsigned int
PNGReader::width() const
{ 
    return png_get_image_width(priv_->png, priv_->info);
}

unsigned int
PNGReader::height() const
{ 
    return png_get_image_height(priv_->png, priv_->info);
}

unsigned int
PNGReader::pixelBytes() const
{
    if (png_get_color_type(priv_->png, priv_->info) == PNG_COLOR_TYPE_RGB)
    {
        return 3;
    }
    return 4;
}


bool
PNGReader::init(const std::string& filename)
{
    static const int png_transforms = PNG_TRANSFORM_STRIP_16 |
                                      PNG_TRANSFORM_GRAY_TO_RGB |
                                      PNG_TRANSFORM_PACKING |
                                      PNG_TRANSFORM_EXPAND;

    Log::debug("Reading PNG file %s\n", filename.c_str());

    const std::unique_ptr<std::istream> is_ptr(Util::get_resource(filename));
    if (!(*is_ptr)) {
        Log::error("Cannot open file %s!\n", filename.c_str());
        return false;
    }

    /* Set up all the libpng structs we need */
    priv_->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!priv_->png) {
        Log::error("Couldn't create libpng read struct\n");
        return false;
    }

    priv_->info = png_create_info_struct(priv_->png);
    if (!priv_->info) {
        Log::error("Couldn't create libpng info struct\n");
        return false;
    }

    /* Set up libpng error handling */
    if (setjmp(png_jmpbuf(priv_->png))) {
        Log::error("libpng error while reading file %s\n", filename.c_str());
        return false;
    }

    /* Read the image information and data */
    png_set_read_fn(priv_->png, reinterpret_cast<void*>(is_ptr.get()),
                    PNGReaderPrivate::png_read_fn);

    png_read_png(priv_->png, priv_->info, png_transforms, 0);

    priv_->rows = png_get_rows(priv_->png, priv_->info);

    priv_->current_row = 0;
    priv_->row_stride = width() * pixelBytes();

    return true;
}

void
PNGReader::finish()
{
    if (priv_->png)
    {
        png_destroy_read_struct(&priv_->png, &priv_->info, 0);
    }
}


/********
 * JPEG *
 ********/

struct JPEGErrorMgr
{
    struct jpeg_error_mgr pub;
    jmp_buf jmp_buffer;

    JPEGErrorMgr()
    {
        jpeg_std_error(&pub);
        pub.error_exit = error_exit;
    }

    static void error_exit(j_common_ptr cinfo)
    {
        JPEGErrorMgr *err =
            reinterpret_cast<JPEGErrorMgr *>(cinfo->err);

        char buffer[JMSG_LENGTH_MAX];

        /* Create the message */
        (*cinfo->err->format_message)(cinfo, buffer);
        std::string msg(std::string(buffer) + "\n");
        Log::error(msg.c_str());

        longjmp(err->jmp_buffer, 1);
    }
};

struct JPEGIStreamSourceMgr
{
    static const int BUFFER_SIZE = 4096;
    struct jpeg_source_mgr pub;
    std::istream *is;
    JOCTET buffer[BUFFER_SIZE];

    JPEGIStreamSourceMgr(const std::string& filename) : is(0)
    {
        is = Util::get_resource(filename);

        /* Fill in jpeg_source_mgr pub struct */
        pub.init_source = init_source;
        pub.fill_input_buffer = fill_input_buffer;
        pub.skip_input_data = skip_input_data;
        pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
        pub.term_source = term_source;
        pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
        pub.next_input_byte = NULL; /* until buffer loaded */
    }

    ~JPEGIStreamSourceMgr()
    {
        delete is;
    }

    bool error()
    {
        return !is || (is->fail() && !is->eof());
    }

    static void init_source(j_decompress_ptr cinfo)
    {
        static_cast<void>(cinfo);
    }

    static boolean fill_input_buffer(j_decompress_ptr cinfo)
    {
        JPEGIStreamSourceMgr *src =
            reinterpret_cast<JPEGIStreamSourceMgr *>(cinfo->src);

        src->is->read(reinterpret_cast<char *>(src->buffer), BUFFER_SIZE);

        src->pub.next_input_byte = src->buffer;
        src->pub.bytes_in_buffer = src->is->gcount();

        /* 
         * If the decoder needs more data, but we have no more bytes left to
         * read mark the end of input.
         */
        if (src->pub.bytes_in_buffer == 0) {
            src->pub.bytes_in_buffer = 2;
            src->buffer[0] = 0xFF;
            src->buffer[0] = JPEG_EOI;
        }

        return TRUE;
    }

    static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
    {
        JPEGIStreamSourceMgr *src =
            reinterpret_cast<JPEGIStreamSourceMgr *>(cinfo->src);

        if (num_bytes > 0) {
            size_t n = static_cast<size_t>(num_bytes);
            while (n > src->pub.bytes_in_buffer) {
                n -= src->pub.bytes_in_buffer;
                src->fill_input_buffer(cinfo);
            }
            src->pub.next_input_byte += n;
            src->pub.bytes_in_buffer -= n;
        }
    }

    static void term_source(j_decompress_ptr cinfo)
    {
        static_cast<void>(cinfo);
    }
};

struct JPEGReaderPrivate
{
    JPEGReaderPrivate(const std::string& filename) :
        source_mgr(filename), jpeg_error(false) {}

    struct jpeg_decompress_struct cinfo;
    JPEGErrorMgr error_mgr;
    JPEGIStreamSourceMgr source_mgr;
    bool jpeg_error;
};


JPEGReader::JPEGReader(const std::string& filename) :
    priv_(new JPEGReaderPrivate(filename))
{
    priv_->jpeg_error = !init(filename);
}

JPEGReader::~JPEGReader()
{
    finish();
    delete priv_;
}

bool
JPEGReader::error()
{
    return priv_->jpeg_error || priv_->source_mgr.error();
}

bool
JPEGReader::nextRow(unsigned char *dst)
{
    bool ret = true;
    unsigned char *buffer[1];
    buffer[0] = dst;

    /* Set up error handling */
    if (setjmp(priv_->error_mgr.jmp_buffer)) {
        return false;
    }

    /* While there are lines left, read next line */
    if (priv_->cinfo.output_scanline < priv_->cinfo.output_height) {
        jpeg_read_scanlines(&priv_->cinfo, buffer, 1);
    }
    else {
        jpeg_finish_decompress(&priv_->cinfo);
        ret = false;
    }

    return ret;
}

unsigned int
JPEGReader::width() const
{ 
    return priv_->cinfo.output_width;
}

unsigned int
JPEGReader::height() const
{ 
    return priv_->cinfo.output_height;
}

unsigned int
JPEGReader::pixelBytes() const
{ 
    return priv_->cinfo.output_components;
}

bool
JPEGReader::init(const std::string& filename)
{
    Log::debug("Reading JPEG file %s\n", filename.c_str());

    /* Initialize error manager */
    priv_->cinfo.err = reinterpret_cast<jpeg_error_mgr*>(&priv_->error_mgr);

    if (setjmp(priv_->error_mgr.jmp_buffer)) {
        return false;
    }

    jpeg_create_decompress(&priv_->cinfo);
    priv_->cinfo.src = reinterpret_cast<jpeg_source_mgr*>(&priv_->source_mgr);

    /* Read header */
    jpeg_read_header(&priv_->cinfo, TRUE);

    jpeg_start_decompress(&priv_->cinfo);

    return true;
}

void
JPEGReader::finish()
{
    jpeg_destroy_decompress(&priv_->cinfo);
}



