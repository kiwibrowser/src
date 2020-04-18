/*
 * Copyright © 2008 Ben Smith
 * Copyright © 2010-2011 Linaro Limited
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
 *  Ben Smith (original glmark benchmark)
 *  Alexandros Frantzis (glmark2)
 */
#include "texture.h"
#include "log.h"
#include "util.h"
#include "image-reader.h"

#include <algorithm>
#include <cstdarg>
#include <vector>

class ImageData {
    void resize(unsigned int w, unsigned int h, unsigned int b)
    {
        width = w;
        height = h;
        bpp = b;
        delete [] pixels;
        pixels = new unsigned char[bpp * width * height];
    }

public:
    ImageData() : pixels(0), width(0), height(0), bpp(0) {}
    ~ImageData() { delete [] pixels; }
    bool load(ImageReader &reader);

    unsigned char *pixels;
    unsigned int width;
    unsigned int height;
    unsigned int bpp;
};

bool
ImageData::load(ImageReader &reader)
{
    if (reader.error())
        return false;

    resize(reader.width(), reader.height(), reader.pixelBytes());

    Log::debug("    Height: %d Width: %d Bpp: %d\n", width, height, bpp);

    /* 
     * Copy the row data to the image buffer in reverse Y order, suitable
     * for texture upload.
     */
    unsigned char *ptr = &pixels[bpp * width * (height - 1)];

    while (reader.nextRow(ptr))
        ptr -= bpp * width;

    return !reader.error();
}

static void
setup_texture(GLuint *tex, ImageData &image, GLint min_filter, GLint mag_filter)
{
    GLenum format = image.bpp == 3 ? GL_RGB : GL_RGBA;

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0,
                 format, GL_UNSIGNED_BYTE, image.pixels);

    if ((min_filter != GL_NEAREST && min_filter != GL_LINEAR) ||
        (mag_filter != GL_NEAREST && mag_filter != GL_LINEAR))
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

namespace TexturePrivate
{
TextureMap textureMap;
}

bool
Texture::load(const std::string &textureName, GLuint *pTexture, ...)
{
    // Make sure the named texture is in the map.
    TextureMap::const_iterator textureIt = TexturePrivate::textureMap.find(textureName);
    if (textureIt == TexturePrivate::textureMap.end())
    {
        return false;
    }

    // Pull the pathname out of the descriptor and use it for the PNG load.
    TextureDescriptor* desc = textureIt->second;
    const std::string& filename = desc->pathname();
    ImageData image;

    if (desc->filetype() == TextureDescriptor::FileTypePNG) {
        PNGReader reader(filename);
        if (!image.load(reader))
            return false;
    }
    else if (desc->filetype() == TextureDescriptor::FileTypeJPEG) {
        JPEGReader reader(filename);
        if (!image.load(reader))
            return false;
    }

    va_list ap;
    va_start(ap, pTexture);
    GLint arg;

    while ((arg = va_arg(ap, GLint)) != 0) {
        GLint arg2 = va_arg(ap, GLint);
        setup_texture(pTexture, image, arg, arg2);
        pTexture++;
    }

    va_end(ap);

    return true;
}

const TextureMap&
Texture::find_textures()
{
    using std::vector;
    using std::string;
    if (!TexturePrivate::textureMap.empty())
    {
        return TexturePrivate::textureMap;
    }
    vector<string> pathVec;
    string dataDir(GLMARK_DATA_PATH"/textures");
    Util::list_files(dataDir, pathVec);
    // Now that we have a list of all of the image files available to us,
    // let's go through and pull out the names and what format they're in
    // so the scene can decide which ones to use.
    for(vector<string>::const_iterator pathIt = pathVec.begin();
        pathIt != pathVec.end();
        pathIt++)
    {
        const string& curPath = *pathIt;
        string::size_type namePos(0);
        string::size_type slashPos = curPath.rfind("/");
        if (slashPos != string::npos)
        {
            // Advance to the first character after the last slash
            namePos = slashPos + 1;
        }

        // Find the position of the extension
        string::size_type pngExtPos = curPath.rfind(".png");
        string::size_type jpgExtPos = curPath.rfind(".jpg");
        string::size_type extPos(string::npos);

        // Select the extension that's closer to the end of the file name
        if (pngExtPos == string::npos)
        {
            extPos = jpgExtPos;
        }
        else if (jpgExtPos == string::npos)
        {
            extPos = pngExtPos;
        }
        else
        {
            extPos = std::max(pngExtPos, jpgExtPos);
        }

        if (extPos == string::npos)
        {
            // We can't trivially determine it's an image file so skip it...
            continue;
        }

        // Set the file type based on the extension
        TextureDescriptor::FileType type(TextureDescriptor::FileTypeUnknown);
        if (extPos == pngExtPos)
        {
            type = TextureDescriptor::FileTypePNG;
        }
        else if (extPos == jpgExtPos)
        {
            type = TextureDescriptor::FileTypeJPEG;
        }

        string name(curPath, namePos, extPos - namePos);
        TextureDescriptor* desc = new TextureDescriptor(name, curPath, type);
        TexturePrivate::textureMap.insert(std::make_pair(name, desc));
    }

    return TexturePrivate::textureMap;
}
