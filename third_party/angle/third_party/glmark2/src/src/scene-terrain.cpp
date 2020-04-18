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
#include "scene.h"
#include "mat.h"
#include "stack.h"
#include "vec.h"
#include "log.h"
#include "mesh.h"
#include "util.h"
#include "texture.h"
#include "shader-source.h"
#include "renderer.h"

using LibMatrix::vec2;
using LibMatrix::vec3;
using LibMatrix::vec4;
using LibMatrix::mat4;

class SceneTerrainPrivate
{
public:
    SceneTerrainPrivate(Canvas &canvas, const LibMatrix::vec2 &repeat_overlay,
                        bool use_bloom, bool use_tilt_shift) :
        canvas(canvas), repeat_overlay(repeat_overlay),
        use_bloom(use_bloom), use_tilt_shift(use_tilt_shift),
        terrain_renderer(0), bloom_v_renderer(0), bloom_h_renderer(0),
        overlay_renderer(0), tilt_v_renderer(0), tilt_h_renderer(0),
        copy_renderer(0), height_map_renderer(0), normal_map_renderer(0),
        specular_map_renderer(0),
        height_normal_chain(0), bloom_chain(0), tilt_chain(0), terrain_chain(0)
    {
        init_renderers();
    }

    ~SceneTerrainPrivate()
    {
        release_renderers();
    }

    void init_renderers()
    {
        /* Create and set up renderers */
        const vec2 map_res(256.0f, 256.0f);
        const vec2 screen_res(canvas.width(), canvas.height());
        const vec2 bloom_res(256.0f, 256.0f);
        const vec2 grass_res(512.0f, 512.0f);

        height_map_renderer = new SimplexNoiseRenderer(map_res);
        height_map_renderer->setup(height_map_renderer->size(), false, false);

        normal_map_renderer = new NormalFromHeightRenderer(map_res);
        normal_map_renderer->setup(normal_map_renderer->size(), false, false);

        specular_map_renderer = new LuminanceRenderer(grass_res);
        specular_map_renderer->setup_texture(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                             GL_REPEAT, GL_REPEAT);
        specular_map_renderer->setup(specular_map_renderer->size(), false, false);

        terrain_renderer = new TerrainRenderer(screen_res, repeat_overlay);
        terrain_renderer->setup(terrain_renderer->size(),
                                !use_bloom && !use_tilt_shift,
                                true);
        terrain_renderer->setup_texture(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                        GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        /* Bloom */
        if (use_bloom) {
            bloom_h_renderer = new BlurRenderer(bloom_res, 2, 4.0,
                                                BlurRenderer::BlurDirectionHorizontal,
                                                vec2(1.0, 1.0) / screen_res,
                                                0.0);
            bloom_h_renderer->setup_texture(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            bloom_h_renderer->setup(bloom_h_renderer->size(), false, false);

            bloom_v_renderer = new BlurRenderer(bloom_res, 2, 4.0,
                                                BlurRenderer::BlurDirectionVertical,
                                                vec2(1.0, 1.0) / bloom_res,
                                                0.0);
            bloom_v_renderer->setup_texture(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            bloom_v_renderer->setup(bloom_v_renderer->size(), false, false);
            overlay_renderer = new OverlayRenderer(*terrain_renderer, 0.6);
        }

        /* Tilt-shift */
        if (use_tilt_shift) {
            tilt_h_renderer = new BlurRenderer(screen_res, 4, 2.7,
                                               BlurRenderer::BlurDirectionHorizontal,
                                               vec2(1.0, 1.0) / screen_res,
                                               0.5);
            tilt_h_renderer->setup_texture(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                           GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            tilt_h_renderer->setup(tilt_h_renderer->size(), false, false);

            tilt_v_renderer = new BlurRenderer(screen_res, 4, 2.7,
                                               BlurRenderer::BlurDirectionVertical,
                                               vec2(1.0, 1.0) / screen_res,
                                               0.5);
        }

        /* Copy renderer */
        if (use_bloom && !use_tilt_shift)
            copy_renderer = new CopyRenderer(screen_res);

        /* Height normal chain */
        height_normal_chain = new RendererChain();
        height_normal_chain->append(*height_map_renderer);
        height_normal_chain->append(*normal_map_renderer);

        /* Bloom effect chain */
        if (use_bloom) {
            bloom_chain = new RendererChain();
            bloom_chain->append(*bloom_h_renderer);
            bloom_chain->append(*bloom_v_renderer);
            bloom_chain->append(*overlay_renderer);
        }

        /* Tilt-shift effect chain */
        if (use_tilt_shift) {
            tilt_chain = new RendererChain();
            tilt_chain->append(*tilt_h_renderer);
            tilt_chain->append(*tilt_v_renderer);
        }

        /* Terrain chain */
        terrain_chain = new RendererChain();
        terrain_chain->append(*terrain_renderer);
        if (use_bloom)
            terrain_chain->append(*bloom_chain);
        if (use_tilt_shift)
            terrain_chain->append(*tilt_chain);

        /*
         * If are just using bloom, the terrain is rendered to a texture and
         * bloom applied on that texture. We need to "copy" that texture's
         * contents to the screen to make the scene visible.
         */
        if (use_bloom && !use_tilt_shift)
            terrain_chain->append(*copy_renderer);

        /*
         * Set up renderer textures.
         */
        terrain_renderer->height_map_texture(height_map_renderer->texture());
        terrain_renderer->normal_map_texture(normal_map_renderer->texture());
        terrain_renderer->specular_map_texture(specular_map_renderer->texture());

        specular_map_renderer->input_texture(terrain_renderer->diffuse1_texture());
    }

    void release_renderers()
    {
        delete terrain_chain;
        delete bloom_chain;
        delete tilt_chain;
        delete height_normal_chain;

        delete height_map_renderer;
        delete normal_map_renderer;
        delete specular_map_renderer;
        delete terrain_renderer;
        delete bloom_v_renderer;
        delete bloom_h_renderer;
        delete overlay_renderer;
        delete tilt_v_renderer;
        delete tilt_h_renderer;
        delete copy_renderer;
    }

    Canvas &canvas;
    LibMatrix::vec2 repeat_overlay;
    bool use_bloom;
    bool use_tilt_shift;

    /* Renderers */
    TerrainRenderer *terrain_renderer;
    BlurRenderer *bloom_v_renderer;
    BlurRenderer *bloom_h_renderer;
    OverlayRenderer *overlay_renderer;
    BlurRenderer *tilt_v_renderer;
    BlurRenderer *tilt_h_renderer;
    CopyRenderer *copy_renderer;

    SimplexNoiseRenderer *height_map_renderer;
    NormalFromHeightRenderer *normal_map_renderer;
    LuminanceRenderer *specular_map_renderer;

    /* Chains */
    RendererChain *height_normal_chain;
    RendererChain *bloom_chain;
    RendererChain *tilt_chain;
    RendererChain *terrain_chain;
};

SceneTerrain::SceneTerrain(Canvas &pCanvas) :
    Scene(pCanvas, "terrain"), priv_(0)
{
    options_["repeat-overlay"] = Scene::Option("repeat-overlay", "6.0",
            "How many times to repeat the terrain texture on the terrain plane (per side)");
    options_["bloom"] = Scene::Option("bloom", "true",
                                      "Use bloom post-processing effect",
                                      "false,true");
    options_["tilt-shift"] = Scene::Option("tilt-shift", "true",
                                           "Use tilt-shift post-processing effect",
                                           "false,true");
}

SceneTerrain::~SceneTerrain()
{
    delete priv_;
}

bool
SceneTerrain::supported(bool show_errors)
{
    GLint vertex_textures;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &vertex_textures);

    if (show_errors && vertex_textures <= 0) {
        Log::error("SceneTerrain requires Vertex Texture Fetch support, "
                   "but GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS is %d\n",
                   vertex_textures);
    }
    
    return vertex_textures > 0;
}

bool
SceneTerrain::load()
{
    Scene::load();

    running_ = false;

    return true;
}

void
SceneTerrain::unload()
{
    Scene::unload();
}

bool
SceneTerrain::setup()
{
    if (!Scene::setup())
        return false;

    /* Parse options */
    float repeat = Util::fromString<double>(options_["repeat-overlay"].value);
    LibMatrix::vec2 repeat_overlay(repeat, repeat);
    bool use_bloom = options_["bloom"].value == "true";
    bool use_tilt_shift = options_["tilt-shift"].value == "true";

    priv_ = new SceneTerrainPrivate(canvas_, repeat_overlay,
                                    use_bloom, use_tilt_shift);

    /* Set up terrain rendering program */
    LibMatrix::Stack4 model;
    LibMatrix::Stack4 camera;
    LibMatrix::mat4 projection = LibMatrix::Mat4::perspective(
            40.0, canvas_.width() / static_cast<float>(canvas_.height()),
            2.0, 4000.0);

    /* Place camera */
    camera.lookAt(-1200.0f, 800.0f, 1200.0f,
                  0.0, 0.0, 0.0,
                  0.0, 1.0, 0.0);
    
    /* Move and rotate plane */
    model.translate(0.0f, -125.0f, 0.0f);
    model.rotate(-90.0, 1.0f, 0.0f, 0.0f);

    LibMatrix::mat4 view_matrix(camera.getCurrent());
    LibMatrix::mat4 model_matrix(model.getCurrent());

    LibMatrix::mat4 model_view_matrix(view_matrix * model_matrix);

    LibMatrix::mat4 normal_matrix(model_view_matrix);
    normal_matrix.inverse().transpose();

    /* Set up terrain renderer program */
    priv_->terrain_renderer->program().start();
    priv_->terrain_renderer->program()["viewMatrix"] = view_matrix;
    priv_->terrain_renderer->program()["modelViewMatrix"] = model_view_matrix;
    priv_->terrain_renderer->program()["normalMatrix"] = normal_matrix;
    priv_->terrain_renderer->program()["projectionMatrix"] = projection;

    /* Create the specular map */
    priv_->specular_map_renderer->render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, canvas_.width(), canvas_.height());

    currentFrame_ = 0;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    running_ = true;

    return true;
}

void
SceneTerrain::teardown()
{
    delete priv_;
    priv_ = 0;
    Scene::teardown();
}

void
SceneTerrain::update()
{
    Scene::update();

    double now = Util::get_timestamp_us() / 1000000.0;
    float diff = now - startTime_;
    float scale = priv_->terrain_renderer->repeat_overlay().x() /
                  priv_->height_map_renderer->uv_scale().x();

    /* Update height map */
    priv_->height_map_renderer->program().start();
    priv_->height_map_renderer->program()["uvOffset"] = vec2(diff * 0.05f, 0.0f);
    priv_->terrain_renderer->program().start();
    priv_->terrain_renderer->program()["uOffset"] = vec2(scale * diff * 0.05f, 0.0f);
}

void
SceneTerrain::draw()
{
    /* Render the height and normal maps used by the terrain */
    priv_->height_normal_chain->render();

    /* Render the terrain plus any post-processing effects */
    priv_->terrain_chain->render();
}

Scene::ValidationResult
SceneTerrain::validate()
{
    return Scene::ValidationUnknown;
}
