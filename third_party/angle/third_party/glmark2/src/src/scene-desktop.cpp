/*
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
 *  Alexandros Frantzis (glmark2)
 *  Jesse Barker (glmark2)
 */
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "scene.h"
#include "mat.h"
#include "stack.h"
#include "vec.h"
#include "log.h"
#include "program.h"
#include "shader-source.h"
#include "util.h"
#include "texture.h"

enum BlurDirection {
    BlurDirectionHorizontal,
    BlurDirectionVertical,
    BlurDirectionBoth
};

static void
create_blur_shaders(ShaderSource& vtx_source, ShaderSource& frg_source,
                    unsigned int radius, float sigma, BlurDirection direction)
{
    vtx_source.append_file(GLMARK_DATA_PATH"/shaders/desktop.vert");
    frg_source.append_file(GLMARK_DATA_PATH"/shaders/desktop-blur.frag");

    /* Don't let the gaussian curve become too narrow */
    if (sigma < 1.0)
        sigma = 1.0;

    unsigned int side = 2 * radius + 1;

    for (unsigned int i = 0; i < radius + 1; i++) {
        float s2 = 2.0 * sigma * sigma;
        float k = 1.0 / std::sqrt(M_PI * s2) * std::exp( - (static_cast<float>(i) * i) / s2);
        std::stringstream ss_tmp;
        ss_tmp << "Kernel" << i;
        frg_source.add_const(ss_tmp.str(), k);
    }

    std::stringstream ss;
    ss << "result = " << std::endl;

    if (direction == BlurDirectionHorizontal) {
        for (unsigned int i = 0; i < side; i++) {
            int offset = static_cast<int>(i - radius);
            ss << "texture2D(Texture0, TextureCoord + vec2(" <<
                  offset << ".0 * TextureStepX, 0.0)) * Kernel" <<
                  std::abs(offset) << " +" << std::endl;
        }
        ss << "0.0 ;" << std::endl;
    }
    else if (direction == BlurDirectionVertical) {
        for (unsigned int i = 0; i < side; i++) {
            int offset = static_cast<int>(i - radius);
            ss << "texture2D(Texture0, TextureCoord + vec2(0.0, " <<
                  offset << ".0 * TextureStepY)) * Kernel" <<
                  std::abs(offset) << " +" << std::endl;
        }
        ss << "0.0 ;" << std::endl;
    }
    else if (direction == BlurDirectionBoth) {
        for (unsigned int i = 0; i < side; i++) {
            int ioffset = static_cast<int>(i - radius);
            for (unsigned int j = 0; j < side; j++) {
                int joffset = static_cast<int>(j - radius);
                ss << "texture2D(Texture0, TextureCoord + vec2(" <<
                      ioffset << ".0 * TextureStepX, " <<
                      joffset << ".0 * TextureStepY))" <<
                      " * Kernel" << std::abs(ioffset) <<
                      " * Kernel" << std::abs(joffset) << " +" << std::endl;
            }
        }
        ss << " 0.0;" << std::endl;
    }

    frg_source.replace("$CONVOLUTION$", ss.str());
}

/**
 * A RenderObject represents a source and target of rendering
 * operations.
 */
class RenderObject
{
public:
    RenderObject() :
        texture_(0), fbo_(0), rotation_rad_(0),
        texture_contents_invalid_(true) { }

    virtual ~RenderObject() {}

    virtual void init()
    {
        /* Create a texture to draw to */
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        /* Create a FBO */
        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        /*
         * Only create the texture image and attach it to the framebuffer
         * if the size has been set.  Framebuffer completeness depends
         * upon non-zero width and height images, and some implementations
         * are overly aggressive in checking at attachment time rather than
         * at draw time.
         */ 
        if (size_.x() != 0 && size_.y() != 0) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_.x(), size_.y(), 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texture_, 0);
            unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                Log::error("RenderObject::init: glCheckFramebufferStatus failed (0x%x)\n", status);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        /* Load the shader program when this class if first used */
        if (RenderObject::use_count == 0) {
            ShaderSource vtx_source(GLMARK_DATA_PATH"/shaders/desktop.vert");
            ShaderSource frg_source(GLMARK_DATA_PATH"/shaders/desktop.frag");
            Scene::load_shaders_from_strings(main_program, vtx_source.str(),
                                             frg_source.str());
        }

        texture_contents_invalid_ = true;
        RenderObject::use_count++;
    }

    virtual void release()
    {
        /* Release resources */
        if (texture_ != 0)
        {
            glDeleteTextures(1, &texture_);
            texture_ = 0;
        }
        if (fbo_ != 0)
        {
            glDeleteFramebuffers(1, &fbo_);
            fbo_ = 0;
        }

        /*
         * Release the shader program when object of this class
         * are no longer in use.
         */
        RenderObject::use_count--;
        if (RenderObject::use_count == 0)
            RenderObject::main_program.release();
    }

    void make_current()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, size_.x(), size_.y());
    }

    void position(const LibMatrix::vec2& pos) { pos_ = pos; }
    const LibMatrix::vec2& position() { return pos_; }


    virtual void size(const LibMatrix::vec2& size)
    {
        /* Recreate the backing texture with correct size */
        if (size_.x() != size.x() || size_.y() != size.y()) {
            size_ = size;
            /* If we're resizing the texture, we need to tell the framebuffer*/
            glBindTexture(GL_TEXTURE_2D, texture_);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_.x(), size_.y(), 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texture_, 0);
            unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                Log::error("RenderObject::size: glCheckFramebufferStatus failed (0x%x)\n", status);
            }
            texture_contents_invalid_ = true;
        }

        if (texture_contents_invalid_) {
            clear();
            texture_contents_invalid_ = false;
        }
    }

    const LibMatrix::vec2& size() { return size_; }

    const LibMatrix::vec2& speed() { return speed_; }
    void speed(const LibMatrix::vec2& speed) { speed_ = speed; }

    GLuint texture() { return texture_; }

    virtual void clear()
    {
        make_current();
        glClear(GL_COLOR_BUFFER_BIT);
    }

    virtual void render_to(RenderObject& target)
    {
        render_to(target, main_program);
    }

    virtual void render_to(RenderObject& target, Program& program)
    {
        LibMatrix::vec2 anchor(pos_);
        LibMatrix::vec2 ll(pos_ - anchor);
        LibMatrix::vec2 ur(pos_ + size_ - anchor);

        /* Calculate new position according to rotation value */
        GLfloat position[2 * 4] = {
            rotate_x(ll.x(), ll.y()) + anchor.x(), rotate_y(ll.x(), ll.y()) + anchor.y(),
            rotate_x(ur.x(), ll.y()) + anchor.x(), rotate_y(ur.x(), ll.y()) + anchor.y(),
            rotate_x(ll.x(), ur.y()) + anchor.x(), rotate_y(ll.x(), ur.y()) + anchor.y(),
            rotate_x(ur.x(), ur.y()) + anchor.x(), rotate_y(ur.x(), ur.y()) + anchor.y(),
        };

        /* Normalize position and write back to array */
        for (int i = 0; i < 4; i++) {
            const LibMatrix::vec2& v2(
                    target.normalize_position(
                        LibMatrix::vec2(position[2 * i], position[2 * i + 1])
                        )
                    );
            position[2 * i] = v2.x();
            position[2 * i + 1] = v2.y();
        }

        static const GLfloat texcoord[2 * 4] = {
            0.0, 0.0,
            1.0, 0.0,
            0.0, 1.0,
            1.0, 1.0,
        };

        target.make_current();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_);
        draw_quad_with_program(position, texcoord, program);
    }

    virtual void render_from(RenderObject& target, Program& program = main_program)
    {
        LibMatrix::vec2 final_pos(pos_ + size_);
        LibMatrix::vec2 ll_tex(target.normalize_texcoord(pos_));
        LibMatrix::vec2 ur_tex(target.normalize_texcoord(final_pos));

        static const GLfloat position_blur[2 * 4] = {
            -1.0, -1.0,
             1.0, -1.0,
            -1.0,  1.0,
             1.0,  1.0,
        };
        GLfloat texcoord_blur[2 * 4] = {
            ll_tex.x(), ll_tex.y(),
            ur_tex.x(), ll_tex.y(),
            ll_tex.x(), ur_tex.y(),
            ur_tex.x(), ur_tex.y(),
        };

        make_current();
        glBindTexture(GL_TEXTURE_2D, target.texture());
        draw_quad_with_program(position_blur, texcoord_blur, program);
    }

    /**
     * Normalizes a position from [0, size] to [-1.0, 1.0]
     */
    LibMatrix::vec2 normalize_position(const LibMatrix::vec2& pos)
    {
        return pos * 2.0 / size_ - 1.0;
    }

    /**
     * Normalizes a position from [0, size] to [0.0, 1.0]
     */
    LibMatrix::vec2 normalize_texcoord(const LibMatrix::vec2& pos)
    {
        return pos / size_;
    }

    void rotation(float degrees)
    {
        rotation_rad_ = (M_PI * degrees / 180.0);
    }

protected:
    void draw_quad_with_program(const GLfloat *position, const GLfloat *texcoord,
                                Program &program)
    {
        int pos_index = program["position"].location();
        int tex_index = program["texcoord"].location();

        program.start();

        glEnableVertexAttribArray(pos_index);
        glEnableVertexAttribArray(tex_index);
        glVertexAttribPointer(pos_index, 2,
                              GL_FLOAT, GL_FALSE, 0, position);
        glVertexAttribPointer(tex_index, 2,
                              GL_FLOAT, GL_FALSE, 0, texcoord);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(tex_index);
        glDisableVertexAttribArray(pos_index);

        program.stop();
    }

    static Program main_program;

    LibMatrix::vec2 pos_;
    LibMatrix::vec2 size_;
    LibMatrix::vec2 speed_;
    GLuint texture_;
    GLuint fbo_;

private:
    float rotate_x(float x, float y)
    {
        return x * cos(rotation_rad_) - y * sin(rotation_rad_);
    }

    float rotate_y(float x, float y)
    {
        return x * sin(rotation_rad_) + y * cos(rotation_rad_);
    }

    float rotation_rad_;
    bool texture_contents_invalid_;
    static int use_count;

};

int RenderObject::use_count = 0;
Program RenderObject::main_program;

/**
 * A RenderObject representing the screen.
 *
 * Rendering to this objects renders to the screen framebuffer.
 */
class RenderScreen : public RenderObject
{
public:
    RenderScreen(Canvas &canvas) { fbo_ = canvas.fbo(); }
    virtual void init() {}
    virtual void release() {}
};

/**
 * A RenderObject with a background image.
 *
 * The image is drawn to the RenderObject automatically when the
 * object is cleared, resized etc
 */
class RenderClearImage : public RenderObject
{
public:
    RenderClearImage(const std::string& texture) :
        RenderObject(), background_texture_name(texture),
        background_texture_(0) {}

    virtual void init()
    {
        RenderObject::init();

        /* Load the image into a texture */
        Texture::load(background_texture_name,
                      &background_texture_, GL_LINEAR, GL_LINEAR, 0);

    }

    virtual void release()
    {
        glDeleteTextures(1, &background_texture_);
        background_texture_ = 0;

        RenderObject::release();
    }

    virtual void clear()
    {
        static const GLfloat position[2 * 4] = {
            -1.0, -1.0,
             1.0, -1.0,
            -1.0,  1.0,
             1.0,  1.0,
        };
        static const GLfloat texcoord[2 * 4] = {
            0.0, 0.0,
            1.0, 0.0,
            0.0, 1.0,
            1.0, 1.0,
        };

        make_current();
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, background_texture_);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        draw_quad_with_program(position, texcoord, main_program);
        glDisable(GL_BLEND);
    }

private:
    std::string background_texture_name;
    GLuint background_texture_;
};

/**
 * A RenderObject that blurs the target it is drawn to.
 */
class RenderWindowBlur : public RenderObject
{
public:
    RenderWindowBlur(unsigned int passes, unsigned int radius, bool separable,
                     bool draw_contents = true) :
        RenderObject(), passes_(passes), radius_(radius), separable_(separable),
        draw_contents_(draw_contents) {}

    virtual void init()
    {
        RenderObject::init();

        /* Only have one instance of the window contents data */
        if (draw_contents_ && RenderWindowBlur::use_count == 0)
            window_contents_.init();

        RenderWindowBlur::use_count++;
    }

    virtual void release()
    {
        RenderWindowBlur::use_count--;

        /* Only have one instance of the window contents data */
        if (draw_contents_ && RenderWindowBlur::use_count == 0)
            window_contents_.release();

        RenderObject::release();
    }

    virtual void size(const LibMatrix::vec2& size)
    {
        RenderObject::size(size);
        if (draw_contents_)
            window_contents_.size(size);
    }

    virtual void render_to(RenderObject& target)
    {
        if (separable_) {
            Program& blur_program_h1 = blur_program_h(target.size().x());
            Program& blur_program_v1 = blur_program_v(target.size().y());

            for (unsigned int i = 0; i < passes_; i++) {
                render_from(target, blur_program_h1);
                RenderObject::render_to(target, blur_program_v1);
            }
        }
        else {
            Program& blur_program1 = blur_program(target.size().x(), target.size().y());

            for (unsigned int i = 0; i < passes_; i++) {
                if (i % 2 == 0)
                    render_from(target, blur_program1);
                else
                    RenderObject::render_to(target, blur_program1);
            }

            if (passes_ % 2 == 1)
                RenderObject::render_to(target);
        }

        /*
         * Blend the window contents with the target texture.
         */
        if (draw_contents_) {
            glEnable(GL_BLEND);
            /*
             * Blend the colors normally, but don't change the
             * destination alpha value.
             */
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_ZERO, GL_ONE);
            window_contents_.position(position());
            window_contents_.render_to(target);
            glDisable(GL_BLEND);
        }
    }

private:
    Program& blur_program(unsigned int w, unsigned int h)
    {
        /*
         * If the size of the window has changed we must recreate
         * the shader to contain the correct texture step values.
         */
        if (blur_program_dim_.x() != w || blur_program_dim_.y() != h ||
            !blur_program_.ready())
        {
            blur_program_dim_.x(w);
            blur_program_dim_.y(h);

            blur_program_.release();

            ShaderSource vtx_source;
            ShaderSource frg_source;
            create_blur_shaders(vtx_source, frg_source, radius_,
                                radius_ / 3.0, BlurDirectionBoth);
            frg_source.add_const("TextureStepX", 1.0 / w);
            frg_source.add_const("TextureStepY", 1.0 / h);
            Scene::load_shaders_from_strings(blur_program_, vtx_source.str(),
                                             frg_source.str());
        }

        return blur_program_;
    }

    Program& blur_program_h(unsigned int w)
    {
        /*
         * If the size of the window has changed we must recreate
         * the shader to contain the correct texture step values.
         */
        if (blur_program_dim_.x() != w ||
            !blur_program_h_.ready())
        {
            blur_program_dim_.x(w);

            blur_program_h_.release();

            ShaderSource vtx_source;
            ShaderSource frg_source;
            create_blur_shaders(vtx_source, frg_source, radius_,
                                radius_ / 3.0, BlurDirectionHorizontal);
            frg_source.add_const("TextureStepX", 1.0 / w);
            Scene::load_shaders_from_strings(blur_program_h_, vtx_source.str(),
                                             frg_source.str());
        }

        return blur_program_h_;
    }

    Program& blur_program_v(unsigned int h)
    {
        /*
         * If the size of the window has changed we must recreate
         * the shader to contain the correct texture step values.
         */
        if (blur_program_dim_.y() != h ||
            !blur_program_v_.ready())
        {
            blur_program_dim_.y(h);

            blur_program_v_.release();

            ShaderSource vtx_source;
            ShaderSource frg_source;
            create_blur_shaders(vtx_source, frg_source, radius_,
                                radius_ / 3.0, BlurDirectionVertical);
            frg_source.add_const("TextureStepY", 1.0 / h);
            Scene::load_shaders_from_strings(blur_program_v_, vtx_source.str(),
                                             frg_source.str());
        }

        return blur_program_v_;
    }

    LibMatrix::uvec2 blur_program_dim_;
    Program blur_program_;
    Program blur_program_h_;
    Program blur_program_v_;
    unsigned int passes_;
    unsigned int radius_;
    bool separable_;
    bool draw_contents_;

    static int use_count;
    static RenderClearImage window_contents_;

};

/**
 * A RenderObject that draws a drop shadow around the window.
 */
class RenderWindowShadow : public RenderObject
{
public:
    using RenderObject::size;

    RenderWindowShadow(unsigned int shadow_size, bool draw_contents = true) :
        RenderObject(), shadow_size_(shadow_size), draw_contents_(draw_contents) {}

    virtual void init()
    {
        RenderObject::init();

        /*
         * Only have one instance of the resources.
         * This works only if all windows have the same size, which
         * is currently the case for this scene. If this condition
         * ceases to be true we will need to create the resources per
         * object.
         */
        if (RenderWindowShadow::use_count == 0) {
            shadow_h_.init();
            shadow_v_.init();
            shadow_corner_.init();
            if (draw_contents_)
                window_contents_.init();
        }

        RenderWindowShadow::use_count++;
    }

    virtual void release()
    {
        RenderWindowShadow::use_count--;

        /* Only have one instance of the data */
        if (RenderWindowShadow::use_count == 0) {
            shadow_h_.release();
            shadow_v_.release();
            shadow_corner_.release();
            if (draw_contents_)
                window_contents_.release();
        }

        RenderObject::release();
    }

    virtual void size(const LibMatrix::vec2& size)
    {
        RenderObject::size(size);
        shadow_h_.size(LibMatrix::vec2(size.x() - shadow_size_,
                                       static_cast<double>(shadow_size_)));
        shadow_v_.size(LibMatrix::vec2(size.y() - shadow_size_,
                                       static_cast<double>(shadow_size_)));
        shadow_corner_.size(LibMatrix::vec2(static_cast<double>(shadow_size_),
                                            static_cast<double>(shadow_size_)));
        if (draw_contents_)
            window_contents_.size(size);
    }

    virtual void render_to(RenderObject& target)
    {
        glEnable(GL_BLEND);
        /*
         * Blend the colors normally, but don't change the
         * destination alpha value.
         */
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                            GL_ZERO, GL_ONE);

        /* Bottom shadow */
        shadow_h_.rotation(0.0);
        shadow_h_.position(position() +
                           LibMatrix::vec2(shadow_size_,
                                           -shadow_h_.size().y()));
        shadow_h_.render_to(target);

        /* Right shadow */
        shadow_v_.rotation(90.0);
        shadow_v_.position(position() +
                           LibMatrix::vec2(size().x() + shadow_v_.size().y(), 0.0));
        shadow_v_.render_to(target);

        /* Bottom right shadow */
        shadow_corner_.rotation(0.0);
        shadow_corner_.position(position() +
                                LibMatrix::vec2(size().x(),
                                                -shadow_corner_.size().y()));
        shadow_corner_.render_to(target);

        /* Top right shadow */
        shadow_corner_.rotation(90.0);
        shadow_corner_.position(position() + size() +
                                LibMatrix::vec2(shadow_corner_.size().x(),
                                                -shadow_corner_.size().y()));
        shadow_corner_.render_to(target);

        /* Bottom left shadow */
        shadow_corner_.rotation(-90.0);
        shadow_corner_.position(position());
        shadow_corner_.render_to(target);

        /*
         * Blend the window contents with the target texture.
         */
        if (draw_contents_) {
            window_contents_.position(position());
            window_contents_.render_to(target);
        }

        glDisable(GL_BLEND);
    }

private:
    unsigned int shadow_size_;
    bool draw_contents_;

    static int use_count;
    static RenderClearImage window_contents_;
    static RenderClearImage shadow_h_;
    static RenderClearImage shadow_v_;
    static RenderClearImage shadow_corner_;

};

int RenderWindowBlur::use_count = 0;
RenderClearImage RenderWindowBlur::window_contents_("desktop-window");
int RenderWindowShadow::use_count = 0;
RenderClearImage RenderWindowShadow::window_contents_("desktop-window");
RenderClearImage RenderWindowShadow::shadow_h_("desktop-shadow");
RenderClearImage RenderWindowShadow::shadow_v_("desktop-shadow");
RenderClearImage RenderWindowShadow::shadow_corner_("desktop-shadow-corner");

/*******************************
 * SceneDesktop implementation *
 *******************************/

/**
 * Private structure used to avoid contaminating scene.h with all of the
 * SceneDesktop internal classes.
 */
struct SceneDesktopPrivate
{
    RenderScreen screen;
    RenderClearImage desktop;
    std::vector<RenderObject *> windows;

    SceneDesktopPrivate(Canvas &canvas) :
        screen(canvas), desktop("effect-2d") {}

    ~SceneDesktopPrivate() { Util::dispose_pointer_vector(windows); }

};


SceneDesktop::SceneDesktop(Canvas &canvas) :
    Scene(canvas, "desktop")
{
    priv_ = new SceneDesktopPrivate(canvas);
    options_["effect"] = Scene::Option("effect", "blur", "The effect to use",
                                       "blur,shadow");
    options_["windows"] = Scene::Option("windows", "4",
                                        "the number of windows");
    options_["window-size"] = Scene::Option("window-size", "0.35",
                                            "the window size as a percentage of the minimum screen dimension [0.0 - 0.5]");
    options_["passes"] = Scene::Option("passes", "1",
                                       "the number of effect passes (effect dependent)");
    options_["blur-radius"] = Scene::Option("blur-radius", "5",
                                            "the blur effect radius (in pixels)");
    options_["separable"] = Scene::Option("separable", "true",
                                          "use separable convolution for the blur effect",
                                          "false,true");
    options_["shadow-size"] = Scene::Option("shadow-size", "20",
                                            "the size of the shadow (in pixels)");
}

SceneDesktop::~SceneDesktop()
{
    delete priv_;
}

bool
SceneDesktop::load()
{
    return true;
}

void
SceneDesktop::unload()
{
}

bool
SceneDesktop::setup()
{
    if (!Scene::setup())
        return false;

    /* Parse the options */
    unsigned int windows(0);
    unsigned int passes(0);
    unsigned int blur_radius(0);
    float window_size_factor(0.0);
    unsigned int shadow_size(0);
    bool separable(options_["separable"].value == "true");

    windows = Util::fromString<unsigned int>(options_["windows"].value);
    window_size_factor = Util::fromString<float>(options_["window-size"].value);
    passes = Util::fromString<unsigned int>(options_["passes"].value);
    blur_radius = Util::fromString<unsigned int>(options_["blur-radius"].value);
    shadow_size = Util::fromString<unsigned int>(options_["shadow-size"].value);

    // Make sure the Texture object knows where to find our images.
    Texture::find_textures();

    /* Ensure we get a transparent clear color for all following operations */
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    /* Set up the screen and desktop RenderObjects */
    priv_->screen.init();
    priv_->desktop.init();
    priv_->screen.size(LibMatrix::vec2(canvas_.width(), canvas_.height()));
    priv_->desktop.size(LibMatrix::vec2(canvas_.width(), canvas_.height()));

    /* Create the windows */
    const float angular_step(2.0 * M_PI / windows);
    unsigned int min_dimension = std::min(canvas_.width(), canvas_.height());
    float window_size(min_dimension * window_size_factor);
    static const LibMatrix::vec2 corner_offset(window_size / 2.0,
                                               window_size / 2.0);

    for (unsigned int i = 0; i < windows; i++) {
        LibMatrix::vec2 center(canvas_.width() * (0.5 + 0.25 * cos(i * angular_step)),
                               canvas_.height() * (0.5 + 0.25 * sin(i * angular_step)));
        RenderObject* win;
        if (options_["effect"].value == "shadow")
            win = new RenderWindowShadow(shadow_size);
        else
            win = new RenderWindowBlur(passes, blur_radius, separable);

        win->init();
        win->position(center - corner_offset);
        win->size(LibMatrix::vec2(window_size, window_size));
        /*
         * Set the speed in increments of about 30 degrees (but not exactly,
         * so we don't get windows moving just on the X axis or Y axis).
         */
        win->speed(LibMatrix::vec2(cos(0.1 + i * M_PI / 6.0) * canvas_.width() / 3,
                                   sin(0.1 + i * M_PI / 6.0) * canvas_.height() / 3));
        /*
         * Perform a dummy rendering to ensure internal shaders are initialized
         * now, in order not to affect the benchmarking.
         */
        win->render_to(priv_->desktop);
        priv_->windows.push_back(win);
    }

    /*
     * Ensure the screen is the current rendering target (it might have changed
     * to a FBO in the previous steps).
     */
    priv_->screen.make_current();

    currentFrame_ = 0;
    running_ = true;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;

    return true;
}

void
SceneDesktop::teardown()
{
    for (std::vector<RenderObject*>::iterator winIt = priv_->windows.begin();
         winIt != priv_->windows.end();
         winIt++)
    {
        RenderObject* curObj = *winIt;
        curObj->release();
        delete curObj;
    }
    priv_->windows.clear();
    priv_->screen.make_current();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    priv_->desktop.release();
    priv_->screen.release();

    Scene::teardown();
}

void
SceneDesktop::update()
{
    double current_time = Util::get_timestamp_us() / 1000000.0;
    double dt = current_time - lastUpdateTime_;

    Scene::update();

    std::vector<RenderObject *>& windows(priv_->windows);

    /*
     * Move the windows around the screen, bouncing them back when
     * they reach the edge.
     */
    for (std::vector<RenderObject *>::const_iterator iter = windows.begin();
         iter != windows.end();
         iter++)
    {
        bool should_update = true;
        RenderObject *win = *iter;
        LibMatrix::vec2 new_pos(
                win->position().x() + win->speed().x() * dt,
                win->position().y() + win->speed().y() * dt);

        if (new_pos.x() < 0.0 ||
            new_pos.x() + win->size().x() > static_cast<float>(canvas_.width()))
        {
            win->speed(LibMatrix::vec2(-win->speed().x(), win->speed().y()));
            should_update = false;
        }

        if (new_pos.y() < 0.0 ||
            new_pos.y() + win->size().y() > static_cast<float>(canvas_.height()))
        {
            win->speed(LibMatrix::vec2(win->speed().x(), -win->speed().y()));
            should_update = false;
        }

        if (should_update)
            win->position(new_pos);
    }
}

void
SceneDesktop::draw()
{
    std::vector<RenderObject *>& windows(priv_->windows);

    /* Ensure we get a transparent clear color for all following operations */
    glClearColor(0.0, 0.0, 0.0, 0.0);

    priv_->desktop.clear();

    for (std::vector<RenderObject *>::const_iterator iter = windows.begin();
         iter != windows.end();
         iter++)
    {
        RenderObject *win = *iter;
        win->render_to(priv_->desktop);
    }

    priv_->desktop.render_to(priv_->screen);

}

Scene::ValidationResult
SceneDesktop::validate()
{
    static const double radius_3d(std::sqrt(3.0 * 2.0 * 2.0));

    Canvas::Pixel ref;

    /* Parse the options */
    unsigned int windows(0);
    unsigned int passes(0);
    unsigned int blur_radius(0);
    float window_size_factor(0.0);
    unsigned int shadow_size(0);

    windows = Util::fromString<unsigned int>(options_["windows"].value);
    window_size_factor = Util::fromString<float>(options_["window-size"].value);
    passes = Util::fromString<unsigned int>(options_["passes"].value);
    blur_radius = Util::fromString<unsigned int>(options_["blur-radius"].value);
    shadow_size = Util::fromString<unsigned int>(options_["shadow-size"].value);

    if (options_["effect"].value == "blur")
    {
        if (windows == 4 && passes == 1 && blur_radius == 5)
            ref = Canvas::Pixel(0x89, 0xa3, 0x53, 0xff);
        else
            return Scene::ValidationUnknown;
    }
    else if (options_["effect"].value == "shadow")
    {
        if (windows == 4 && fabs(window_size_factor - 0.35) < 0.0001 &&
            shadow_size == 20)
        {
            ref = Canvas::Pixel(0x1f, 0x27, 0x0d, 0xff);
        }
        else
        {
            return Scene::ValidationUnknown;
        }
    }

    Canvas::Pixel pixel = canvas_.read_pixel(512, 209);

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
