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
#include "util.h"
#include "shader-source.h"
#include "model.h"

#include <cmath>
#include <sstream>

using LibMatrix::vec3;
using std::string;
using std::endl;

SceneShading::SceneShading(Canvas &pCanvas) :
    Scene(pCanvas, "shading"),
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
    options_["shading"] = Scene::Option("shading", "gouraud",
                                        "Which shading method to use",
                                        "gouraud,blinn-phong-inf,phong,cel");
    options_["num-lights"] = Scene::Option("num-lights", "1",
            "The number of lights applied to the scene (phong only)");
    options_["model"] = Scene::Option("model", "cat", "Which model to use",
                                      optionValues);
}

SceneShading::~SceneShading()
{
}

bool
SceneShading::load()
{
    rotationSpeed_ = 36.0f;

    running_ = false;

    return true;
}

void
SceneShading::unload()
{
    mesh_.reset();
}

static string
get_fragment_shader_source(const string& frg_file, unsigned int lights)
{
    ShaderSource source(frg_file);

    static const string lightPositionName("LightSourcePosition");
    static const string lightColorName("LightColor");
    static const string callCompute("    gl_FragColor += compute_color(");
    static const string commaString(", ");
    static const string rParenString(");");
    std::stringstream doLightSS;
    doLightSS << string("    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);");
    doLightSS << endl;
    float theta(2.0 * M_PI / lights);
    float phi(theta / 2.0);
    float intensity(0.8 / lights);
    LibMatrix::vec4 lightCol(intensity, intensity, intensity, 1.0);
    for (unsigned int l = 0; l < lights; l++)
    {
        // Construct constant names for the light position and color and add it
        // to the list of constants for the shader.
        string indexString(Util::toString(l));
        string curLightPosition(lightPositionName + indexString);
        string curLightColor(lightColorName + indexString);
        float sin_theta(sin(theta * l));
        float cos_theta(cos(theta * l));
        float sin_phi(sin(phi * l));
        float cos_phi(cos(phi * l));
        LibMatrix::vec4 lightPos(cos_phi * sin_theta, cos_phi * cos_theta, sin_phi, 1.0);
        source.add_const(curLightPosition, lightPos);
        source.add_const(curLightColor, lightCol);

        // Add the section of source to the substantive...
        doLightSS << callCompute;
        doLightSS << curLightPosition;
        doLightSS << commaString;
        doLightSS << curLightColor;
        doLightSS << rParenString;
        doLightSS << endl;
    }

    source.replace("$DO_LIGHTS$", doLightSS.str());

    return source.str();
}

bool
SceneShading::setup()
{
    if (!Scene::setup())
        return false;

    static const LibMatrix::vec4 lightPosition(20.0f, 20.0f, 10.0f, 1.0f);
    static const LibMatrix::vec4 materialDiffuse(0.0f, 0.0f, 1.0f, 1.0f);

    // Calculate half vector for blinn-phong shading model
    LibMatrix::vec3 halfVector(lightPosition[0], lightPosition[1], lightPosition[2]);
    halfVector.normalize();
    halfVector += LibMatrix::vec3(0.0, 0.0, 1.0);
    halfVector.normalize();

    // Load and add constants to shaders
    std::string vtx_shader_filename;
    std::string frg_shader_filename;
    const std::string &shading = options_["shading"].value;
    ShaderSource vtx_source;
    ShaderSource frg_source;
    if (shading == "gouraud") {
        vtx_shader_filename = GLMARK_DATA_PATH"/shaders/light-basic.vert";
        frg_shader_filename = GLMARK_DATA_PATH"/shaders/light-basic.frag";
        frg_source.append_file(frg_shader_filename);
        vtx_source.append_file(vtx_shader_filename);
        vtx_source.add_const("LightSourcePosition", lightPosition);
        vtx_source.add_const("MaterialDiffuse", materialDiffuse);
    }
    else if (shading == "blinn-phong-inf") {
        vtx_shader_filename = GLMARK_DATA_PATH"/shaders/light-advanced.vert";
        frg_shader_filename = GLMARK_DATA_PATH"/shaders/light-advanced.frag";
        frg_source.append_file(frg_shader_filename);
        frg_source.add_const("LightSourcePosition", lightPosition);
        frg_source.add_const("LightSourceHalfVector", halfVector);
        vtx_source.append_file(vtx_shader_filename);
    }
    else if (shading == "phong") {
        vtx_shader_filename = GLMARK_DATA_PATH"/shaders/light-phong.vert";
        frg_shader_filename = GLMARK_DATA_PATH"/shaders/light-phong.frag";
        unsigned int num_lights = Util::fromString<unsigned int>(options_["num-lights"].value);
        string fragsource = get_fragment_shader_source(frg_shader_filename, num_lights);
        frg_source.append(fragsource);
        frg_source.add_const("MaterialDiffuse", materialDiffuse);
        vtx_source.append_file(vtx_shader_filename);
    }
    else if (shading == "cel") {
        vtx_shader_filename = GLMARK_DATA_PATH"/shaders/light-phong.vert";
        frg_shader_filename = GLMARK_DATA_PATH"/shaders/light-cel.frag";
        vtx_source.append_file(vtx_shader_filename);
        frg_source.append_file(frg_shader_filename);
    }

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

    mesh_.build_vbo();

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

    std::vector<GLint> attrib_locations;
    attrib_locations.push_back(program_["position"].location());
    attrib_locations.push_back(program_["normal"].location());
    mesh_.set_attrib_locations(attrib_locations);

    currentFrame_ = 0;
    rotation_ = 0.0f;
    running_ = true;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;

    return true;
}

void
SceneShading::teardown()
{
    program_.stop();
    program_.release();

    Scene::teardown();
}

void
SceneShading::update()
{
    Scene::update();

    double elapsed_time = lastUpdateTime_ - startTime_;

    rotation_ = rotationSpeed_ * elapsed_time;
}

void
SceneShading::draw()
{
    // Load the ModelViewProjectionMatrix uniform in the shader
    LibMatrix::Stack4 model_view;
    model_view.translate(-centerVec_.x(), -centerVec_.y(), -(centerVec_.z() + 2.0 + radius_));
    model_view.rotate(rotation_, 0.0f, 1.0f, 0.0f);
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

    // Load the modelview matrix itself
    program_["ModelViewMatrix"] = model_view.getCurrent();

    mesh_.render_vbo();
}

Scene::ValidationResult
SceneShading::validate()
{
    static const double radius_3d(std::sqrt(3.0));

    if (rotation_ != 0)
        return Scene::ValidationUnknown;

    Canvas::Pixel ref;

    Canvas::Pixel pixel = canvas_.read_pixel(canvas_.width() / 3,
                                             canvas_.height() / 3);

    const std::string &filter = options_["shading"].value;

    if (filter == "gouraud")
        ref = Canvas::Pixel(0x00, 0x00, 0x2d, 0xff);
    else if (filter == "blinn-phong-inf")
        ref = Canvas::Pixel(0x1a, 0x1a, 0x3e, 0xff);
    else if (filter == "phong" && options_["num-lights"].value == "1")
        ref = Canvas::Pixel(0x05, 0x05, 0xad, 0xff);
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
