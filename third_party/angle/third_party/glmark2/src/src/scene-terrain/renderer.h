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
#include <stdint.h>
#include <vector>

#include "mesh.h"
#include "vec.h"
#include "program.h"
#include "gl-headers.h"

/** 
 * Renderer interface.
 */
class IRenderer
{
public:
    /** 
     * Sets up the renderer's target.
     */
    virtual void setup(const LibMatrix::vec2 &size, bool onscreen, bool has_depth) = 0;
    /** 
     * Sets up the renderer's target texture (if any).
     */
    virtual void setup_texture(GLint min_filter, GLint mag_filter,
                               GLint wrap_s, GLint wrap_t) = 0;
    /** 
     * Sets the renderer's input texture.
     */
    virtual void input_texture(GLuint t) = 0;
    /** 
     * Gets the renderer's target texture (if any).
     */
    virtual GLuint texture() = 0;
    /** 
     * Gets the size of the renderer's target.
     */
    virtual LibMatrix::vec2 size() = 0;
    /** 
     * Makes the renderer current i.e. the rendering target.
     */
    virtual void make_current() = 0;
    /** 
     * Updates the mipmap of the texture backing the renderer (if any).
     */
    virtual void update_mipmap() = 0;
    /** 
     * Renders to the renderer's target.
     */
    virtual void render() = 0;

protected:
    virtual ~IRenderer() {}
};

/** 
 * A chain of renderers, which implements IRenderer
 */
class RendererChain : public IRenderer
{
public:
    RendererChain() {}
    virtual ~RendererChain() {}

    /* IRenderer methods */
    void setup(const LibMatrix::vec2 &size, bool onscreen, bool has_depth);
    void setup_texture(GLint min_filter, GLint mag_filter,
                       GLint wrap_s, GLint wrap_t);
    void input_texture(GLuint t);
    GLuint texture();
    LibMatrix::vec2 size();
    void make_current();
    void update_mipmap();
    void render();

    /** 
     * Appends a renderer to the chain.
     * 
     * @param renderer the renderer to append
     */
    void append(IRenderer &renderer);

private:
    std::vector<IRenderer *> renderers_;
};

/** 
 * A base implementation of the IRenderer interface.
 */
class BaseRenderer : public IRenderer
{
public:
    BaseRenderer(const LibMatrix::vec2 &size);
    virtual ~BaseRenderer();

    /* IRenderer methods */
    virtual void setup(const LibMatrix::vec2 &size, bool onscreen, bool has_depth);
    virtual void setup_texture(GLint min_filter, GLint mag_filter,
                               GLint wrap_s, GLint wrap_t);
    virtual void input_texture(GLuint t) { input_texture_ = t; }
    virtual GLuint texture() { return texture_; }
    virtual LibMatrix::vec2 size() { return size_; }
    virtual void make_current();
    virtual void update_mipmap();
    virtual void render() = 0;

protected:
    void recreate(bool onscreen, bool has_depth);
    void create_texture();
    void update_texture_parameters();
    void create_fbo(bool has_depth);

    LibMatrix::vec2 size_;
    GLuint texture_;
    GLuint input_texture_;
    GLuint fbo_;
    GLuint depth_renderbuffer_;
    GLint min_filter_;
    GLint mag_filter_;
    GLint wrap_s_;
    GLint wrap_t_;
};

/** 
 * A renderer that renders its input texture to its target,
 * according to the supplied GL Program.
 */
class TextureRenderer : public BaseRenderer
{
public:
    TextureRenderer(const LibMatrix::vec2 &size, Program &program);
    virtual ~TextureRenderer() { }

    /* IRenderer/BaseRenderer methods */
    virtual void render();

    /**
     * Gets the program associated with the renderer.
     */
    Program &program() { return program_; }

private:
    void create_mesh();

    Mesh mesh_;
    Program &program_;
};

/** 
 * A renderer that copies the input texture to its target.
 */
class CopyRenderer : public TextureRenderer
{
public:
    CopyRenderer(const LibMatrix::vec2 &size);

    virtual ~CopyRenderer() { delete copy_program_; }

private:
    static Program *copy_program(bool create_new);

    Program *copy_program_;
};

/** 
 * A renderer that renders simplex noise to its target.
 */
class SimplexNoiseRenderer : public TextureRenderer
{
public:
    SimplexNoiseRenderer(const LibMatrix::vec2 &size);
    virtual ~SimplexNoiseRenderer() { delete noise_program_; }

    LibMatrix::vec2 uv_scale() { return uv_scale_; }
private:
    static Program *noise_program(bool create_new);
    static LibMatrix::vec2 uv_scale_;

    Program *noise_program_;
};

/** 
 * A renderer that renders a normal map to its target from a
 * height map in its input texture.
 */
class NormalFromHeightRenderer : public TextureRenderer
{
public:
    NormalFromHeightRenderer(const LibMatrix::vec2 &size);
    virtual ~NormalFromHeightRenderer() { delete normal_from_height_program_; }

private:
    static Program *normal_from_height_program(const LibMatrix::vec2 &size, 
                                               bool create_new);

    Program *normal_from_height_program_;
};

/** 
 * A renderer that renders the luminance of its input texture to its target.
 */
class LuminanceRenderer : public TextureRenderer
{
public:
    LuminanceRenderer(const LibMatrix::vec2 &size);
    virtual ~LuminanceRenderer() { delete luminance_program_; }

private:
    static Program *luminance_program(bool create_new);

    Program *luminance_program_;
};


/** 
 * A renderer that renders a blurred version of the input texture to its target.
 */
class BlurRenderer : public TextureRenderer
{
public:
    enum BlurDirection {
        BlurDirectionHorizontal,
        BlurDirectionVertical,
        BlurDirectionBoth
    };

    BlurRenderer(const LibMatrix::vec2 &size, int radius, float sigma,
                 BlurDirection dir, const LibMatrix::vec2 &step, float tilt_shift);
    virtual ~BlurRenderer() { delete blur_program_; }

private:
    static Program *blur_program(bool create_new, int radius, float sigma,
                                 BlurDirection dir, const LibMatrix::vec2 &step,
                                 float tilt_shift);

    Program *blur_program_;
};

/** 
 * A renderer that renders with opacity (overlays) it's input texture over
 * the target of another renderer.
 */
class OverlayRenderer : public IRenderer
{
public:
    OverlayRenderer(IRenderer &target, GLfloat opacity);
    virtual ~OverlayRenderer() { }

    /* IRenderable Methods */
    void setup(const LibMatrix::vec2 &size, bool onscreen, bool has_depth);
    void setup_texture(GLint min_filter, GLint mag_filter,
                       GLint wrap_s, GLint wrap_t);
    void input_texture(GLuint t) { input_texture_ = t; }
    virtual GLuint texture() { return target_renderer_.texture(); }
    virtual LibMatrix::vec2 size() { return target_renderer_.size(); }
    virtual void render();
    void make_current();
    void update_mipmap();

private:
    void create_mesh();
    void create_program();

    Mesh mesh_;
    Program program_;
    IRenderer &target_renderer_;
    GLfloat opacity_;
    GLuint input_texture_;
};

/** 
 * A renderer that renders a dynamic terrain as per the WebGL
 * dynamic terrain demo.
 */
class TerrainRenderer : public BaseRenderer
{
public:
    TerrainRenderer(const LibMatrix::vec2 &size, const LibMatrix::vec2 &repeat_overlay);
    virtual ~TerrainRenderer();

    /* IRenderable Methods */
    virtual void render();

    /**
     * Gets the program associated with the renderer.
     */
    Program &program() { return program_; }
    /**
     * Sets the height map texture to use.
     */
    void height_map_texture(GLuint tex) { height_map_tex_ = tex; }
    /**
     * Sets the normal map texture to use.
     */
    void normal_map_texture(GLuint tex) { normal_map_tex_ = tex; }
    /**
     * Sets the specular map texture to use.
     */
    void specular_map_texture(GLuint tex) { specular_map_tex_ = tex; }
    /**
     * Returns the main diffuse texture.
     */
    GLuint diffuse1_texture() { return diffuse1_tex_; }
    LibMatrix::vec2 repeat_overlay() { return repeat_overlay_; }

private:
    void create_mesh();
    void init_textures();
    void init_program();
    void bind_textures();
    void deinit_textures();

    LibMatrix::vec3 color_to_vec3(uint32_t c)
    {
        return LibMatrix::vec3(((c >> 0) & 0xff) / 255.0,
                               ((c >> 8) & 0xff) / 255.0,
                               ((c >> 16) & 0xff) / 255.0);
    }

    Mesh mesh_;
    Program program_;

    GLuint height_map_tex_;
    GLuint normal_map_tex_;
    GLuint specular_map_tex_;
    GLuint diffuse1_tex_;
    GLuint diffuse2_tex_;
    GLuint detail_tex_;
    LibMatrix::vec2 repeat_overlay_;
};
