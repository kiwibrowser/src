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
#include <string>

class ImageReader
{
public:
    virtual bool error() = 0;
    virtual bool nextRow(unsigned char *dst) = 0;
    virtual unsigned int width() const = 0;
    virtual unsigned int height() const = 0;
    virtual unsigned int pixelBytes() const = 0;
    virtual ~ImageReader() {}
};

struct PNGReaderPrivate;

class PNGReader : public ImageReader
{
public:
    PNGReader(const std::string& filename);

    virtual ~PNGReader();
    bool error();
    bool nextRow(unsigned char *dst);

    unsigned int width() const;
    unsigned int height() const;
    unsigned int pixelBytes() const;

private:
    bool init(const std::string& filename);
    void finish();

    PNGReaderPrivate *priv_;
};

struct JPEGReaderPrivate;

class JPEGReader : public ImageReader
{
public:
    JPEGReader(const std::string& filename);

    virtual ~JPEGReader();
    bool error();
    bool nextRow(unsigned char *dst);
    unsigned int width() const;
    unsigned int height() const;
    unsigned int pixelBytes() const;

private:
    bool init(const std::string& filename);
    void finish();

    JPEGReaderPrivate *priv_;
};

