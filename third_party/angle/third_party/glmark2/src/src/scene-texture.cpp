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
 *  Jesse Barker (glmark2)
 */
#include "scene.h"
#include "mat.h"
#include "stack.h"
#include "vec.h"
#include "log.h"
#include "program.h"
#include "shader-source.h"
#include "texture.h"
#include "model.h"
#include "util.h"
#include <cmath>

using LibMatrix::vec3;
using std::string;

SceneTexture::SceneTexture(Canvas &pCanvas) :
    Scene(pCanvas, "texture"), radius_(0.0),
    orientModel_(false), orientationAngle_(0.0)
{
    const ModelMap& modelMap = Model::find_models();
    string optionValues;
    for (ModelMap::const_iterator modelIt = modelMap.begin();
         modelIt != modelMap.end();
         modelIt++)
    {
        static bool doSeparator(false);
        if (doSeparator)
        {
            optionValues += ",";
        }
        const std::string& curName = modelIt->first;
        optionValues += curName;
        doSeparator = true;
    }
    options_["model"] = Scene::Option("model", "cube", "Which model to use",
                                      optionValues);
    options_["texture-filter"] = Scene::Option("texture-filter", "nearest",
                                               "The texture filter to use",
                                               "nearest,linear,linear-shader,mipmap");
    optionValues = "";
    const TextureMap& textureMap = Texture::find_textures();
    for (TextureMap::const_iterator textureIt = textureMap.begin();
         textureIt != textureMap.end();
         textureIt++)
    {
        static bool doSeparator(false);
        if (doSeparator)
        {
            optionValues += ",";
        }
        const std::string& curName = textureIt->first;
        optionValues += curName;
        doSeparator = true;
    }
    options_["texture"] = Scene::Option("texture", "crate-base", "Which texture to use",
                                        optionValues);
    options_["texgen"] = Scene::Option("texgen", "false",
                                       "Whether to generate texcoords in the shader",
                                       "false,true");
}

SceneTexture::~SceneTexture()
{
}

bool
SceneTexture::load()
{
    rotationSpeed_ = LibMatrix::vec3(36.0f, 36.0f, 36.0f);

    running_ = false;

    return true;
}

void
SceneTexture::unload()
{
    mesh_.reset();
}

bool
SceneTexture::setup()
{
    if (!Scene::setup())
        return false;

    static const std::string vtx_shader_filename(GLMARK_DATA_PATH"/shaders/light-basic.vert");
    static const std::string vtx_shader_texgen_filename(GLMARK_DATA_PATH"/shaders/light-basic-texgen.vert");
    static const std::string frg_shader_filename(GLMARK_DATA_PATH"/shaders/light-basic-tex.frag");
    static const std::string frg_shader_bilinear_filename(GLMARK_DATA_PATH"/shaders/light-basic-tex-bilinear.frag");
    static const LibMatrix::vec4 lightPosition(20.0f, 20.0f, 10.0f, 1.0f);
    static const LibMatrix::vec4 materialDiffuse(1.0f, 1.0f, 1.0f, 1.0f);

    // Create texture according to selected filtering
    GLint min_filter = GL_NONE;
    GLint mag_filter = GL_NONE;
    const std::string &filter = options_["texture-filter"].value;

    if (filter == "nearest") {
        min_filter = GL_NEAREST;
        mag_filter = GL_NEAREST;
    }
    else if (filter == "linear") {
        min_filter = GL_LINEAR;
        mag_filter = GL_LINEAR;
    }
    else if (filter == "linear-shader") {
        min_filter = GL_NEAREST;
        mag_filter = GL_NEAREST;
    }
    else if (filter == "mipmap") {
        min_filter = GL_LINEAR_MIPMAP_LINEAR;
        mag_filter = GL_LINEAR;
    }

    const string& whichTexture(options_["texture"].value);
    if (!Texture::load(whichTexture, &texture_, min_filter, mag_filter, 0))
        return false;

    // Load shaders
    bool doTexGen(options_["texgen"].value == "true");
    ShaderSource vtx_source;
    if (doTexGen) {
        vtx_source.append_file(vtx_shader_texgen_filename);
        vtx_source.add_const("PI", static_cast<float>(M_PI));
    }
    else {
        vtx_source.append_file(vtx_shader_filename);
    }
    ShaderSource frg_source;
    if (filter == "linear-shader") {
        frg_source.append_file(frg_shader_bilinear_filename);
        LibMatrix::vec2 texture_size(512, 512);
        frg_source.add_const("TextureSize", texture_size);
    }
    else {
        frg_source.append_file(frg_shader_filename);
    }

    // Add constants to shaders
    vtx_source.add_const("LightSourcePosition", lightPosition);
    vtx_source.add_const("MaterialDiffuse", materialDiffuse);

    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(),
                                          frg_source.str()))
    {
        return false;
    }

    Model model;
    const string& whichModel(options_["model"].value);
    bool modelLoaded = model.load(whichModel);
    if(!modelLoaded)
        return false;

    // Now that we're successfully loaded, there are a few quirks about
    // some of the known models that we need to account for.  The draw
    // logic for the scene wants to rotate the model around the Y axis.
    // Most of our models are described this way.  Some need adjustment
    // (an additional rotation that gets the model into the correct
    // orientation).
    //
    // Here's a summary:
    //
    // Angel rotates around the Y axis
    // Armadillo rotates around the Y axis
    // Buddha rotates around the X axis
    // Bunny rotates around the Y axis
    // Dragon rotates around the X axis
    // Horse rotates around the Y axis
    if (whichModel == "buddha" || whichModel == "dragon")
    {
        orientModel_ = true;
        orientationAngle_ = -90.0;
        orientationVec_ = vec3(1.0, 0.0, 0.0);
    }
    else if (whichModel == "armadillo")
    {
        orientModel_ = true;
        orientationAngle_ = 180.0; 
        orientationVec_ = vec3(0.0, 1.0, 0.0);
    }

    if (model.needTexcoords())
        model.calculate_texcoords();
    if (model.needNormals())
        model.calculate_normals();

    // Tell the converter which attributes we care about
    std::vector<std::pair<Model::AttribType, int> > attribs;
    attribs.push_back(std::pair<Model::AttribType, int>(Model::AttribTypePosition, 3));
    attribs.push_back(std::pair<Model::AttribType, int>(Model::AttribTypeNormal, 3));
    if (!doTexGen) {
        attribs.push_back(std::pair<Model::AttribType, int>(Model::AttribTypeTexcoord, 2));
    }
    model.convert_to_mesh(mesh_, attribs);
    mesh_.build_vbo();

    // Calculate a projection matrix that is a good fit for the model
    vec3 maxVec = model.maxVec();
    vec3 minVec = model.minVec();
    vec3 diffVec = maxVec - minVec;
    centerVec_ = maxVec + minVec;
    centerVec_ /= 2.0;
    float diameter = diffVec.length();
    radius_ = diameter / 2;
    float fovy = 2.0 * atanf(radius_ / (2.0 + radius_));
    fovy /= M_PI;
    fovy *= 180.0;
    float aspect(static_cast<float>(canvas_.width())/static_cast<float>(canvas_.height()));
    perspective_.setIdentity();
    perspective_ *= LibMatrix::Mat4::perspective(fovy, aspect, 2.0, 2.0 + diameter);

    program_.start();

    std::vector<GLint> attrib_locations;
    attrib_locations.push_back(program_["position"].location());
    attrib_locations.push_back(program_["normal"].location());
    if (doTexGen) {
        program_["CenterPoint"] = centerVec_;
    }
    else {
        attrib_locations.push_back(program_["texcoord"].location());
    }
    mesh_.set_attrib_locations(attrib_locations);

    currentFrame_ = 0;
    rotation_ = LibMatrix::vec3();
    running_ = true;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;

    return true;
}

void
SceneTexture::teardown()
{
    program_.stop();
    program_.release();

    glDeleteTextures(1, &texture_);

    Scene::teardown();
}

void
SceneTexture::update()
{
    Scene::update();

    double elapsed_time = lastUpdateTime_ - startTime_;

    rotation_ = rotationSpeed_ * elapsed_time;
}

void
SceneTexture::draw()
{
    // Load the ModelViewProjectionMatrix uniform in the shader
    LibMatrix::Stack4 model_view;
    model_view.translate(-centerVec_.x(), -centerVec_.y(), -(centerVec_.z() + 2.5 + radius_));
    model_view.rotate(rotation_.x(), 1.0f, 0.0f, 0.0f);
    model_view.rotate(rotation_.y(), 0.0f, 1.0f, 0.0f);
    model_view.rotate(rotation_.z(), 0.0f, 0.0f, 1.0f);
    if (orientModel_)
    {
        model_view.rotate(orientationAngle_, orientationVec_.x(), orientationVec_.y(), orientationVec_.z());
    }
    LibMatrix::mat4 model_view_proj(perspective_);
    model_view_proj *= model_view.getCurrent();

    program_["ModelViewProjectionMatrix"] = model_view_proj;

    // Load the NormalMatrix uniform in the shader. The NormalMatrix is the
    // inverse transpose of the model view matrix.
    LibMatrix::mat4 normal_matrix(model_view.getCurrent());
    normal_matrix.inverse().transpose();
    program_["NormalMatrix"] = normal_matrix;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);

    mesh_.render_vbo();
}

Scene::ValidationResult
SceneTexture::validate()
{
    static const double radius_3d(std::sqrt(3 * 2.0 * 2.0));

    if (rotation_.x() != 0 || rotation_.y() != 0 || rotation_.z() != 0)
        return Scene::ValidationUnknown;

    Canvas::Pixel ref;

    Canvas::Pixel pixel = canvas_.read_pixel(canvas_.width() / 2 + 3,
                                             canvas_.height() / 2 + 3);

    const std::string &filter = options_["texture-filter"].value;

    if (filter == "nearest")
        ref = Canvas::Pixel(0x2b, 0x2a, 0x28, 0xff);
    else if (filter == "linear")
        ref = Canvas::Pixel(0x2b, 0x2a, 0x28, 0xff);
    else if (filter == "linear-shader")
        ref = Canvas::Pixel(0x2d, 0x2c, 0x2a, 0xff);
    else if (filter == "mipmap")
        ref = Canvas::Pixel(0x2c, 0x2d, 0x2a, 0xff);
    else
        return Scene::ValidationUnknown;

    double dist = pixel.distance_rgb(ref);

    if (dist < radius_3d + 0.01) {
        return Scene::ValidationSuccess;
    }
    else {
        Log::debug("Validation failed! Expected: 0x%x Actual: 0x%x Distance: %f\n",
                    ref.to_le32(), pixel.to_le32(), dist);
        return Scene::ValidationFailure;
    }
}
