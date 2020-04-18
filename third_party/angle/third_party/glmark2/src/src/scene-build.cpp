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
#include "log.h"
#include "mat.h"
#include "stack.h"
#include "shader-source.h"
#include "model.h"
#include "util.h"
#include <cmath>

SceneBuild::SceneBuild(Canvas &pCanvas) :
    Scene(pCanvas, "build"),
    orientModel_(false)
{
    const ModelMap& modelMap = Model::find_models();
    std::string optionValues;
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
    options_["use-vbo"] = Scene::Option("use-vbo", "true",
                                        "Whether to use VBOs for rendering",
                                        "false,true");
    options_["interleave"] = Scene::Option("interleave", "false",
                                           "Whether to interleave vertex attribute data",
                                           "false,true");
    options_["model"] = Scene::Option("model", "horse", "Which model to use",
                                      optionValues);
}

SceneBuild::~SceneBuild()
{
}

bool
SceneBuild::load()
{
    rotationSpeed_ = 36.0f;

    running_ = false;

    return true;
}

void
SceneBuild::unload()
{
    mesh_.reset();
}

bool
SceneBuild::setup()
{
    using LibMatrix::vec3;

    if (!Scene::setup())
        return false;

    /* Set up shaders */
    static const std::string vtx_shader_filename(GLMARK_DATA_PATH"/shaders/light-basic.vert");
    static const std::string frg_shader_filename(GLMARK_DATA_PATH"/shaders/light-basic.frag");
    static const LibMatrix::vec4 lightPosition(20.0f, 20.0f, 10.0f, 1.0f);
    static const LibMatrix::vec4 materialDiffuse(1.0f, 1.0f, 1.0f, 1.0f);

    ShaderSource vtx_source(vtx_shader_filename);
    ShaderSource frg_source(frg_shader_filename);

    vtx_source.add_const("LightSourcePosition", lightPosition);
    vtx_source.add_const("MaterialDiffuse", materialDiffuse);

    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(),
                                          frg_source.str()))
    {
        return false;
    }

    Model model;
    const std::string& whichModel(options_["model"].value);
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

    if (model.needNormals())
        model.calculate_normals();

    /* Tell the converter that we only care about position and normal attributes */
    std::vector<std::pair<Model::AttribType, int> > attribs;
    attribs.push_back(std::pair<Model::AttribType, int>(Model::AttribTypePosition, 3));
    attribs.push_back(std::pair<Model::AttribType, int>(Model::AttribTypeNormal, 3));

    model.convert_to_mesh(mesh_, attribs);

    std::vector<GLint> attrib_locations;
    attrib_locations.push_back(program_["position"].location());
    attrib_locations.push_back(program_["normal"].location());
    mesh_.set_attrib_locations(attrib_locations);

    useVbo_ = (options_["use-vbo"].value == "true");
    bool interleave = (options_["interleave"].value == "true");

    mesh_.vbo_update_method(Mesh::VBOUpdateMethodMap);
    mesh_.interleave(interleave);

    if (useVbo_)
        mesh_.build_vbo();
    else
        mesh_.build_array();

    /* Calculate a projection matrix that is a good fit for the model */
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

    currentFrame_ = 0;
    rotation_ = 0.0;
    running_ = true;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;

    return true;
}

void
SceneBuild::teardown()
{
    program_.stop();
    program_.release();

    mesh_.reset();

    Scene::teardown();
}

void
SceneBuild::update()
{
    Scene::update();

    double elapsed_time = lastUpdateTime_ - startTime_;

    rotation_ = rotationSpeed_ * elapsed_time;
}

void
SceneBuild::draw()
{
    LibMatrix::Stack4 model_view;

    // Load the ModelViewProjectionMatrix uniform in the shader
    LibMatrix::mat4 model_view_proj(perspective_);
    model_view.translate(-centerVec_.x(), -centerVec_.y(), -(centerVec_.z() + 2.0 + radius_));
    model_view.rotate(rotation_, 0.0f, 1.0f, 0.0f);
    if (orientModel_)
    {
        model_view.rotate(orientationAngle_, orientationVec_.x(), orientationVec_.y(), orientationVec_.z());
    }
    model_view_proj *= model_view.getCurrent();

    program_["ModelViewProjectionMatrix"] = model_view_proj;

    // Load the NormalMatrix uniform in the shader. The NormalMatrix is the
    // inverse transpose of the model view matrix.
    LibMatrix::mat4 normal_matrix(model_view.getCurrent());
    normal_matrix.inverse().transpose();
    program_["NormalMatrix"] = normal_matrix;

    if (useVbo_) {
        mesh_.render_vbo();
    }
    else {
        mesh_.render_array();
    }
}

Scene::ValidationResult
SceneBuild::validate()
{
    static const double radius_3d(std::sqrt(3.0));

    if (rotation_ != 0)
        return Scene::ValidationUnknown;

    Canvas::Pixel ref(0xa7, 0xa7, 0xa7, 0xff);
    Canvas::Pixel pixel = canvas_.read_pixel(canvas_.width() / 2,
                                             canvas_.height() / 2);

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
