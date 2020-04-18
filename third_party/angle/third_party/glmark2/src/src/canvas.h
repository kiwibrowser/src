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
 *  Jesse Barker
 */
#ifndef GLMARK2_CANVAS_H_
#define GLMARK2_CANVAS_H_

#include "gl-headers.h"
#include "mat.h"
#include "gl-visual-config.h"

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <cmath>

/**
 * Abstraction for a GL rendering target.
 */
class Canvas
{
public:
    virtual ~Canvas() {}

    /**
     * A pixel value.
     */
    struct Pixel {
        Pixel():
            r(0), g(0), b(0), a(0) {}
        Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a):
            r(r), g(g), b(b), a(a) {}
        /**
         * Gets the pixel value as a 32-bit integer in 0xAABBGGRR format.
         *
         * @return the pixel value
         */
        uint32_t to_le32()
        {
            return static_cast<uint32_t>(r) +
                   (static_cast<uint32_t>(g) << 8) +
                   (static_cast<uint32_t>(b) << 16) +
                   (static_cast<uint32_t>(a) << 24);

        }

        /**
         * Gets the euclidian distance from this pixel in 3D RGB space.
         *
         * @param p the pixel to get the distance from
         *
         * @return the euclidian distance
         */
        double distance_rgb(const Canvas::Pixel &p)
        {
            // These work without casts because of integer promotion rules
            // (the uint8_ts are promoted to ints)
            double d = (r - p.r) * (r - p.r) +
                       (g - p.g) * (g - p.g) +
                       (b - p.b) * (b - p.b);
            return std::sqrt(d);
        }

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };


    /**
     * Initializes the canvas and makes it the target of GL operations.
     *
     * This method should be implemented in derived classes.
     *
     * @return whether initialization succeeded
     */
    virtual bool init() { return false; }

    /**
     * Resets the canvas, destroying and recreating resources to give each new
     * test scenario a fresh context for rendering.
     *
     * This method should be implemented in derived classes.
     *
     * @return whether reset succeeded
     */
    virtual bool reset() { return false; }

    /**
     * Changes the visibility of the canvas.
     *
     * The canvas is initially not visible.
     *
     * This method should be implemented in derived classes.
     *
     * @param visible true to make the Canvas visible, false otherwise
     */
    virtual void visible(bool visible) { static_cast<void>(visible); }

    /**
     * Clears the canvas.
     * This method should be implemented in derived classes.
     */
    virtual void clear() {}

    /**
     * Ensures that the canvas on-screen representation gets updated
     * with the latest canvas contents.
     *
     * This method should be implemented in derived classes.
     */
    virtual void update() {}

    /**
     * Prints information about the canvas.
     *
     * This method should be implemented in derived classes.
     */
    virtual void print_info() {}

    /**
     * Reads a pixel from the canvas.
     *
     * The (0, 0) point is the lower left corner. The X and Y coordinates
     * increase towards the right and top, respectively.
     *
     * This method should be implemented in derived classes.
     *
     * @param x the X coordinate
     * @param y the Y coordinate
     *
     * @return the pixel
     */
    virtual Pixel read_pixel(int x, int y)
    {
        static_cast<void>(x);
        static_cast<void>(y);
        return Pixel();
    }

    /**
     * Writes the canvas contents to a file.
     *
     * The pixel save order is  upper left to lower right. Each pixel value
     * is stored as four consecutive bytes R,G,B,A.
     *
     * This method should be implemented in derived classes.
     *
     * @param filename the name of the file to write to
     */
    virtual void write_to_file(std::string &filename) { static_cast<void>(filename); }

    /**
     * Whether we should quit the application.
     *
     * This method should be implemented in derived classes.
     *
     * @return true if we should quit, false otherwise
     */
    virtual bool should_quit() { return false; }

    /**
     * Resizes the canvas.
     *
     * This method should be implemented in derived classes.
     *
     * @param width the new width in pixels
     * @param height the new height in pixels
     *
     * @return true if we should quit, false otherwise
     */
    virtual void resize(int width, int height) { static_cast<void>(width); static_cast<void>(height); }

    /**
     * Gets the FBO associated with the canvas.
     *
     * @return the FBO
     */
    virtual unsigned int fbo() { return 0; }

    /**
     * Gets a dummy canvas object.
     *
     * @return the dummy canvas
     */
    static Canvas &dummy()
    {
        static Canvas dummy_canvas(0, 0);
        return dummy_canvas;
    }

    /**
     * Gets the width of the canvas.
     *
     * @return the width in pixels
     */
    int width() { return width_; }

    /**
     * Gets the height of the canvas.
     *
     * @return the height in pixels
     */
    int height() { return height_; }

    /**
     * Gets the projection matrix recommended for use with the canvas.
     *
     * It's not mandatory to use this projection matrix.
     *
     * @return the projection matrix
     */
    const LibMatrix::mat4 &projection() { return projection_; }

    /**
     * Sets whether the canvas should be backed by an off-screen surface.
     *
     * This takes effect after the next init()/reset().
     */
    void offscreen(bool offscreen) { offscreen_ = offscreen; }

    /**
     * Sets the preferred visual configuration.
     *
     * This takes effect after the next init()/reset().
     */
    void visual_config(GLVisualConfig &config) { visual_config_ = config; }

protected:
    Canvas(int width, int height) :
        width_(width), height_(height), offscreen_(false) {}

    int width_;
    int height_;
    LibMatrix::mat4 projection_;
    bool offscreen_;
    GLVisualConfig visual_config_;
};

#endif
