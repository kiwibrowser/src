/*
 * Copyright © 2010-2012 Linaro Limited
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
 *  Alexandros Frantzis (glmark2)
 *  Jesse Barker (glmark2)
 */
#include <cmath>
#include <climits>
#include <numeric>

#include "scene.h"
#include "mat.h"
#include "stack.h"
#include "vec.h"
#include "log.h"
#include "program.h"
#include "shader-source.h"
#include "util.h"
#include "texture.h"

SceneEffect2D::SceneEffect2D(Canvas &pCanvas) :
    Scene(pCanvas, "effect2d")
{
    options_["kernel"] = Scene::Option("kernel",
        "0,0,0;0,1,0;0,0,0",
        "The convolution kernel matrix to use [format: \"a,b,c...;d,e,f...\"");;
    options_["normalize"] = Scene::Option("normalize", "true",
        "Whether to normalize the supplied convolution kernel matrix",
        "false,true");
}

SceneEffect2D::~SceneEffect2D()
{
}

/*
 * Calculates the offset of the coefficient with index i
 * from the center of the kernel matrix. Note that we are
 * using the standard OpenGL texture coordinate system
 * (x grows rightwards, y grows upwards).
 */
static LibMatrix::vec2
calc_offset(unsigned int i, unsigned int width, unsigned int height)
{
    int x = i % width - (width - 1) / 2;
    int y = -(i / width - (height - 1) / 2);

    return LibMatrix::vec2(static_cast<float>(x),
                           static_cast<float>(y));

}

/**
 * Creates a fragment shader implementing 2D image convolution.
 *
 * In the mathematical definition of 2D convolution, the kernel/filter (2D
 * impulse response) is essentially mirrored in both directions (that is,
 * rotated 180 degrees) when being applied on a 2D block of data (eg pixels).
 *
 * Most image manipulation programs, however, use the term kernel/filter to
 * describe a 180 degree rotation of the 2D impulse response. This is more
 * intuitive from a human understanding perspective because this rotated matrix
 * can be regarded as a stencil that can be directly applied by just "placing"
 * it on the image.
 *
 * In order to be compatible with image manipulation programs, we will
 * use the same definition of kernel/filter (180 degree rotation of impulse
 * response). This also means that we don't need to perform the (implicit)
 * rotation of the kernel in our convolution implementation.
 *
 * @param canvas the destination Canvas for this shader
 * @param array the array holding the filter coefficients in row-major
 *              order
 * @param width the width of the filter
 * @param width the height of the filter
 *
 * @return a string containing the frament source code
 */
static std::string
create_convolution_fragment_shader(Canvas &canvas, std::vector<float> &array,
                                   unsigned int width, unsigned int height)
{
    static const std::string frg_shader_filename(GLMARK_DATA_PATH"/shaders/effect-2d-convolution.frag");
    ShaderSource source(frg_shader_filename);

    if (width * height != array.size()) {
        Log::error("Convolution filter size doesn't match supplied dimensions\n");
        return "";
    }

    /* Steps are needed to be able to access nearby pixels */
    source.add_const("TextureStepX", 1.0f/canvas.width());
    source.add_const("TextureStepY", 1.0f/canvas.height());

    std::stringstream ss_def;
    std::stringstream ss_convolution;

    /* Set up stringstream floating point options */
    ss_def << std::fixed;
    ss_convolution.precision(1);
    ss_convolution << std::fixed;

    ss_convolution << "result = ";

    for(std::vector<float>::const_iterator iter = array.begin();
        iter != array.end();
        iter++)
    {
        unsigned int i = iter - array.begin();

        /* Add Filter coefficient const definitions */
        ss_def << "const float Kernel" << i << " = "
               << *iter << ";" << std::endl;

        /* Add convolution term using the current filter coefficient */
        LibMatrix::vec2 offset(calc_offset(i, width, height));
        ss_convolution << "texture2D(Texture0, TextureCoord + vec2("
                       << offset.x() << " * TextureStepX, "
                       << offset.y() << " * TextureStepY)) * Kernel" << i;
        if (iter + 1 != array.end())
            ss_convolution << " +" << std::endl;
    }

    ss_convolution << ";" << std::endl;

    source.add(ss_def.str());
    source.replace("$CONVOLUTION$", ss_convolution.str());

    return source.str();
}

/**
 * Creates a string containing a printout of a kernel matrix.
 *
 * @param filter the vector containing the filter coefficients
 * @param width the width of the filter
 *
 * @return the printout
 */
static std::string
kernel_printout(const std::vector<float> &kernel,
                unsigned int width)
{
    std::stringstream ss;
    ss << std::fixed;

    for (std::vector<float>::const_iterator iter = kernel.begin();
         iter != kernel.end();
         iter++)
    {
        ss << *iter << " ";
        if ((iter - kernel.begin()) % width == width - 1)
            ss << std::endl;
    }

    return ss.str();
}

/**
 * Parses a string representation of a matrix and returns it
 * in row-major format.
 *
 * In the string representation, elements are delimited using
 * commas (',') and rows are delimited using semi-colons (';').
 * eg 0,0,0;0,1.0,0;0,0,0
 *
 * @param str the matrix string representation to parse
 * @param matrix the float vector to populate
 * @param[out] width the width of the matrix
 * @param[out] height the height of the matrix
 *
 * @return whether parsing succeeded
 */
static bool
parse_matrix(const std::string &str, std::vector<float> &matrix,
             unsigned int &width, unsigned int &height)
{
    std::vector<std::string> rows;
    unsigned int w = UINT_MAX;

    Util::split(str, ';', rows, Util::SplitModeNormal);

    Log::debug("Parsing kernel matrix:\n");
    static const std::string format("%f ");
    static const std::string format_cont(Log::continuation_prefix + format);
    static const std::string newline(Log::continuation_prefix + "\n");

    for (std::vector<std::string>::const_iterator iter = rows.begin();
         iter != rows.end();
         iter++)
    {
        std::vector<std::string> elems;
        Util::split(*iter, ',', elems, Util::SplitModeNormal);

        if (w != UINT_MAX && elems.size() != w) {
            Log::error("Matrix row %u contains %u elements, whereas previous"
                       " rows had %u\n",
                       iter - rows.begin(), elems.size(), w);
            return false;
        }

        w = elems.size();

        for (std::vector<std::string>::const_iterator iter_el = elems.begin();
             iter_el != elems.end();
             iter_el++)
        {
            float f(Util::fromString<float>(*iter_el));
            matrix.push_back(f);
            if (iter_el == elems.begin())
                Log::debug(format.c_str(), f);
            else
                Log::debug(format_cont.c_str(), f);
        }

        Log::debug(newline.c_str());
    }

    width = w;
    height = rows.size();

    return true;
}

/**
 * Normalizes a convolution kernel matrix.
 *
 * @param filter the filter to normalize
 */
static void
normalize(std::vector<float> &kernel)
{
    float sum = std::accumulate(kernel.begin(), kernel.end(), 0.0);

    /*
     * If sum is essentially zero, perform a zero-sum normalization.
     * This normalizes positive and negative values separately,
     */
    if (fabs(sum) < 0.00000001) {
        sum = 0.0;
        for (std::vector<float>::iterator iter = kernel.begin();
             iter != kernel.end();
             iter++)
        {
            if (*iter > 0.0)
                sum += *iter;
        }
    }

    /*
     * We can simply compare with 0.0f here, because we just care about
     * avoiding division-by-zero.
     */
    if (sum == 0.0)
        return;

    for (std::vector<float>::iterator iter = kernel.begin();
         iter != kernel.end();
         iter++)
    {
        *iter /= sum;
    }

}

bool
SceneEffect2D::load()
{
    Texture::load("effect-2d", &texture_,
                  GL_NEAREST, GL_NEAREST, 0);
    running_ = false;

    return true;
}

void
SceneEffect2D::unload()
{
    glDeleteTextures(1, &texture_);
}

bool
SceneEffect2D::setup()
{
    if (!Scene::setup())
        return false;

    Texture::find_textures();

    static const std::string vtx_shader_filename(GLMARK_DATA_PATH"/shaders/effect-2d.vert");

    std::vector<float> kernel;
    unsigned int kernel_width = 0;
    unsigned int kernel_height = 0;

    /* Parse the kernel matrix from the options */
    if (!parse_matrix(options_["kernel"].value, kernel,
                      kernel_width, kernel_height))
    {
        return false;
    }

    /* Normalize the kernel matrix if needed */
    if (options_["normalize"].value == "true") {
        normalize(kernel);
        Log::debug("Normalized kernel matrix:\n%s",
                   kernel_printout(kernel, kernel_width).c_str());
    }

    /* Create and load the shaders */
    ShaderSource vtx_source(vtx_shader_filename);
    ShaderSource frg_source;
    frg_source.append(create_convolution_fragment_shader(canvas_, kernel,
                                                         kernel_width,
                                                         kernel_height));

    if (frg_source.str().empty())
        return false;

    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(),
                                          frg_source.str()))
    {
        return false;
    }

    std::vector<int> vertex_format;
    vertex_format.push_back(3);
    mesh_.set_vertex_format(vertex_format);

    mesh_.make_grid(1, 1, 2.0, 2.0, 0.0);
    mesh_.build_vbo();

    std::vector<GLint> attrib_locations;
    attrib_locations.push_back(program_["position"].location());
    mesh_.set_attrib_locations(attrib_locations);

    program_.start();

    // Load texture sampler value
    program_["Texture0"] = 0;

    currentFrame_ = 0;
    running_ = true;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;

    return true;
}

void
SceneEffect2D::teardown()
{
    mesh_.reset();

    program_.stop();
    program_.release();

    Scene::teardown();
}

void
SceneEffect2D::update()
{
    Scene::update();
}

void
SceneEffect2D::draw()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);

    mesh_.render_vbo();
}

Scene::ValidationResult
SceneEffect2D::validate()
{
    static const double radius_3d(std::sqrt(3.0));

    std::vector<float> kernel;
    std::vector<float> kernel_edge;
    std::vector<float> kernel_blur;
    unsigned int kernel_width = 0;
    unsigned int kernel_height = 0;

    if (!parse_matrix("0,1,0;1,-4,1;0,1,0;", kernel_edge,
                      kernel_width, kernel_height))
    {
        return Scene::ValidationUnknown;
    }

    if (!parse_matrix("1,1,1,1,1;1,1,1,1,1;1,1,1,1,1;",
                      kernel_blur,
                      kernel_width, kernel_height))
    {
        return Scene::ValidationUnknown;
    }

    if (!parse_matrix(options_["kernel"].value, kernel,
                      kernel_width, kernel_height))
    {
        return Scene::ValidationUnknown;
    }

    Canvas::Pixel ref;

    if (kernel == kernel_edge)
        ref = Canvas::Pixel(0x17, 0x0c, 0x2f, 0xff);
    else if (kernel == kernel_blur)
        ref = Canvas::Pixel(0xc7, 0xe1, 0x8d, 0xff);
    else
        return Scene::ValidationUnknown;

    Canvas::Pixel pixel = canvas_.read_pixel(452, 237);

    double dist = pixel.distance_rgb(ref);
    if (dist < radius_3d + 0.01) {
        return Scene::ValidationSuccess;
    }
    else {
        Log::debug("Validation failed! Expected: 0x%x Actual: 0x%x Distance: %f\n",
                   ref.to_le32(), pixel.to_le32(), dist);
        return Scene::ValidationFailure;
    }

    return Scene::ValidationUnknown;
}
