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
#include "renderer.h"
#include "texture.h"
#include "shader-source.h"

TerrainRenderer::TerrainRenderer(const LibMatrix::vec2 &size,
                                 const LibMatrix::vec2 &repeat_overlay) :
    BaseRenderer(size), height_map_tex_(0), normal_map_tex_(0),
    specular_map_tex_(0), repeat_overlay_(repeat_overlay)
{
    create_mesh();
    init_textures();
    init_program();
}

TerrainRenderer::~TerrainRenderer()
{
    deinit_textures();
}

void
TerrainRenderer::render()
{
    make_current();
    glClearColor(0.825f, 0.7425f, 0.61875f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    program_.start();

    bind_textures();
    mesh_.render_vbo();

    program_.stop();

    update_mipmap();
}

void
TerrainRenderer::init_textures()
{
    /* Create textures */
    Texture::load("terrain-grasslight-512", &diffuse1_tex_,
            GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
    Texture::load("terrain-backgrounddetailed6", &diffuse2_tex_,
            GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
    Texture::load("terrain-grasslight-512-nm", &detail_tex_,
            GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);

    /* Set REPEAT wrap mode */
    glBindTexture(GL_TEXTURE_2D, diffuse1_tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, diffuse2_tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, detail_tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}


void
TerrainRenderer::init_program()
{
    ShaderSource vtx_shader(GLMARK_DATA_PATH"/shaders/terrain.vert");
    ShaderSource frg_shader(GLMARK_DATA_PATH"/shaders/terrain.frag");

    if (!Scene::load_shaders_from_strings(program_, vtx_shader.str(), frg_shader.str()))
        return;

    program_.start();
    /* Fog */
    program_["fogDensity"] = 0.00025f;
    program_["fogNear"] = 1.0f;
    program_["fogFar"] = 2000.0f;
    program_["fogColor"] = LibMatrix::vec3(0.825, 0.7425, 0.61875);

    /* Lights */
    program_["ambientLightColor"] = color_to_vec3(0xffffff);

    program_["pointLightColor[0]"] = color_to_vec3(0xffffff);
    program_["pointLightPosition[0]"] = LibMatrix::vec3(0.0, 0.0, 0.0);
    program_["pointLightDistance[0]"] = 0.0f;

    /* Textures */
    program_["tDiffuse1"] = 0;
    program_["tDiffuse2"] = 1;
    program_["tDetail"] = 2;
    program_["tNormal"] = 3;
    program_["tSpecular"] = 4;
    program_["tDisplacement"] = 5;

    program_["uNormalScale"] = 3.5f;

    program_["uDisplacementBias"] = 0.0f;
    program_["uDisplacementScale"] = 375.0f;

    program_["uDiffuseColor"] = color_to_vec3(0xffffff);
    program_["uSpecularColor"] = color_to_vec3(0xffffff);
    program_["uAmbientColor"] = color_to_vec3(0x888888);
    program_["uShininess"] = 30.0f;
    program_["uOpacity"] = 1.0f;

    program_["uRepeatOverlay"] = repeat_overlay_;

    program_["uOffset"] = LibMatrix::vec2(0.0, 0.0);

    std::vector<GLint> attrib_locations;
    attrib_locations.push_back(program_["position"].location());
    attrib_locations.push_back(program_["normal"].location());
    attrib_locations.push_back(program_["tangent"].location());
    attrib_locations.push_back(program_["uv"].location());
    mesh_.set_attrib_locations(attrib_locations);

    program_.stop();
}

void
TerrainRenderer::bind_textures()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuse1_tex_);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, diffuse2_tex_);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, detail_tex_);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, normal_map_tex_);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, specular_map_tex_);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, height_map_tex_);
}

void
TerrainRenderer::deinit_textures()
{
    glDeleteTextures(1, &diffuse1_tex_);
    glDeleteTextures(1, &diffuse2_tex_);
    glDeleteTextures(1, &detail_tex_);
}

static void
grid_conf(Mesh &mesh, int x, int y, int n_x, int n_y,
          LibMatrix::vec3 &ul,
          LibMatrix::vec3 &ll,
          LibMatrix::vec3 &ur,
          LibMatrix::vec3 &lr)
{
    struct PlaneMeshVertex {
        LibMatrix::vec3 position;
        LibMatrix::vec2 texcoord;
        LibMatrix::vec3 normal;
        LibMatrix::vec3 tangent;
    };

    LibMatrix::vec2 uv_ul(static_cast<float>(x) / n_x,
            1.0 - static_cast<float>(y) / n_y);
    LibMatrix::vec2 uv_lr(static_cast<float>(x + 1) / n_x,
            1.0 - static_cast<float>(y + 1) / n_y);

    LibMatrix::vec3 normal(0.0, 0.0, 1.0);
    LibMatrix::vec3 tangent(1.0, 0.0, 0.0);

    PlaneMeshVertex cell_vertices[] = {
        {
            ll,
            LibMatrix::vec2(uv_ul.x(), uv_lr.y()),
            normal,
            tangent
        },
        {
            lr,
            LibMatrix::vec2(uv_lr.x(), uv_lr.y()),
            normal,
            tangent
        },
        {
            ur,
            LibMatrix::vec2(uv_lr.x(), uv_ul.y()),
            normal,
            tangent
        },
        {
            ul,
            LibMatrix::vec2(uv_ul.x(), uv_ul.y()),
            normal,
            tangent
        }
    };

    unsigned int vertex_index[] = {0, 1, 2, 0, 2, 3};

    for (size_t i = 0; i < sizeof(vertex_index) / sizeof(*vertex_index); i++) {
        PlaneMeshVertex& vertex = cell_vertices[vertex_index[i]];

        mesh.next_vertex();
        mesh.set_attrib(0, vertex.position);
        mesh.set_attrib(1, vertex.normal);
        mesh.set_attrib(2, vertex.tangent);
        mesh.set_attrib(3, vertex.texcoord);
    }
}

void
TerrainRenderer::create_mesh()
{
    std::vector<int> vertex_format;
    vertex_format.push_back(3);
    vertex_format.push_back(3);
    vertex_format.push_back(3);
    vertex_format.push_back(2);
    mesh_.set_vertex_format(vertex_format);

    mesh_.make_grid(256, 256, 6000, 6000, 0, grid_conf);
    mesh_.build_vbo();
}

