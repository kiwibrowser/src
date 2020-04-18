//
// Copyright © 2012 Linaro Limited
//
// This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
//
// glmark2 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// glmark2.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
//  Jesse Barker
//
#include "scene-refract.h"
#include "model.h"
#include "texture.h"
#include "util.h"
#include "log.h"
#include "shader-source.h"

using std::string;
using std::vector;
using std::map;
using LibMatrix::mat4;
using LibMatrix::vec4;
using LibMatrix::vec3;

static const vec4 lightPosition(1.0f, 1.0f, 2.0f, 1.0f);

//
// Public interfaces
//

SceneRefract::SceneRefract(Canvas& canvas) :
    Scene(canvas, "refract"),
    priv_(0)
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
        const string& curName = modelIt->first;
        optionValues += curName;
        doSeparator = true;
    }
    options_["model"] = Scene::Option("model", "bunny", "Which model to use",
                                      optionValues);
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
        const string& curName = textureIt->first;
        optionValues += curName;
        doSeparator = true;
    }
    options_["texture"] = Scene::Option("texture", "nasa1", "Which texture to use",
                                        optionValues);
    options_["index"] = Scene::Option("index", "1.2",
                                      "Index of refraction of the medium to simulate");
    options_["use-vbo"] = Scene::Option("use-vbo", "true",
                                        "Whether to use VBOs for rendering",
                                        "false,true");
    options_["interleave"] = Scene::Option("interleave", "false",
                                           "Whether to interleave vertex attribute data",
                                           "false,true");
}

bool
SceneRefract::supported(bool show_errors)
{
    static const string oes_depth_texture("GL_OES_depth_texture");
    static const string arb_depth_texture("GL_ARB_depth_texture");
    if (!GLExtensions::support(oes_depth_texture) &&
        !GLExtensions::support(arb_depth_texture)) {
        if (show_errors) {
            Log::error("We do not have the depth texture extension!!!\n");
        }

        return false;
    }
    return true;
}

bool
SceneRefract::load()
{
    running_ = false;
    return true;
}

void
SceneRefract::unload()
{
}

bool
SceneRefract::setup()
{
    // If the scene isn't supported, don't bother to go through setup.
    if (!supported(false) || !Scene::setup())
    {
        return false;
    }

    priv_ = new RefractPrivate(canvas_);
    if (!priv_->setup(options_)) {
        delete priv_;
        priv_ = 0;
        return false;
    }

    // Set core scene timing after actual initialization so we don't measure
    // set up time.
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;
    running_ = true;

    return true;
}

void
SceneRefract::teardown()
{
    // Add scene-specific teardown here
    if (priv_) {
        priv_->teardown();
        delete priv_;
    }
    Scene::teardown();
}

void
SceneRefract::update()
{
    Scene::update();
    // Add scene-specific update here
    priv_->update(lastUpdateTime_ - startTime_);
}

void
SceneRefract::draw()
{
    priv_->draw();
}

Scene::ValidationResult
SceneRefract::validate()
{
    return Scene::ValidationUnknown;
}

//
// Private interfaces
//

bool
DistanceRenderTarget::setup(unsigned int width, unsigned int height)
{
    static const string vtx_shader_filename(GLMARK_DATA_PATH"/shaders/depth.vert");
    static const string frg_shader_filename(GLMARK_DATA_PATH"/shaders/depth.frag");

    ShaderSource vtx_source(vtx_shader_filename);
    ShaderSource frg_source(frg_shader_filename);

    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(), frg_source.str())) {
        return false;
    }

    canvas_width_ = width;
    canvas_height_ = height;
    width_ = canvas_width_ * 2;
    height_ = canvas_height_ * 2;

    // If the texture will be too large for the implemnetation, we need to
    // clamp the dimensions but maintain the aspect ratio.
    GLint tex_size(0);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tex_size);
    unsigned int max_size = static_cast<unsigned int>(tex_size);
    if (max_size < width_ || max_size < height_) {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        width_ = max_size;
        height_ = width_ / aspect;
        Log::debug("DistanceRenderTarget::setup: original texture size (%u x %u), clamped to (%u x %u)\n",
            canvas_width_ * 2, canvas_height_ * 2, width_, height_);
    }

    glGenTextures(2, &tex_[0]);
    glBindTexture(GL_TEXTURE_2D, tex_[DEPTH]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width_, height_, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, tex_[COLOR]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           tex_[DEPTH], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tex_[COLOR], 0);
    unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        Log::error("DistanceRenderTarget::setup: glCheckFramebufferStatus failed (0x%x)\n", status);
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void
DistanceRenderTarget::teardown()
{
    program_.stop();
    program_.release();
    if (tex_[0]) {
        glDeleteTextures(2, &tex_[0]);
        tex_[DEPTH] = tex_[COLOR] = 0;
    }
    if (fbo_) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
}

void
DistanceRenderTarget::enable(const mat4& mvp)
{
    program_.start();
    program_["ModelViewProjectionMatrix"] = mvp;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           tex_[DEPTH], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tex_[COLOR], 0);
    glViewport(0, 0, width_, height_);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glCullFace(GL_FRONT);
}

void DistanceRenderTarget::disable()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, canvas_width_, canvas_height_);
    glCullFace(GL_BACK);
}

bool
RefractPrivate::setup(map<string, Scene::Option>& options)
{
    // Program object setup
    static const string vtx_shader_filename(GLMARK_DATA_PATH"/shaders/light-refract.vert");
    static const string frg_shader_filename(GLMARK_DATA_PATH"/shaders/light-refract.frag");
    static const vec4 lightColor(0.4, 0.4, 0.4, 1.0);

    ShaderSource vtx_source(vtx_shader_filename);
    ShaderSource frg_source(frg_shader_filename);

    frg_source.add_const("LightColor", lightColor);
    frg_source.add_const("LightSourcePosition", lightPosition);
    float refractive_index(Util::fromString<float>(options["index"].value));
    frg_source.add_const("RefractiveIndex", refractive_index);

    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(), frg_source.str())) {
        return false;
    }

    const string& whichTexture(options["texture"].value);
    if (!Texture::load(whichTexture, &texture_, GL_LINEAR, GL_LINEAR, 0))
        return false;

    // Model setup
    Model model;
    const string& whichModel(options["model"].value);
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

    // Mesh setup
    vector<std::pair<Model::AttribType, int> > attribs;
    attribs.push_back(std::pair<Model::AttribType, int>(Model::AttribTypePosition, 3));
    attribs.push_back(std::pair<Model::AttribType, int>(Model::AttribTypeNormal, 3));
    model.convert_to_mesh(mesh_, attribs);

    useVbo_ = (options["use-vbo"].value == "true");
    bool interleave = (options["interleave"].value == "true");
    mesh_.vbo_update_method(Mesh::VBOUpdateMethodMap);
    mesh_.interleave(interleave);

    if (useVbo_) {
        mesh_.build_vbo();
    }
    else {
        mesh_.build_array();
    }

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
    projection_.perspective(fovy, aspect, 2.0, 2.0 + diameter);

    // Set up the light matrix with a bias that will convert values
    // in the range of [-1, 1] to [0, 1)], then add in the projection
    // and the "look at" matrix from the light position.
    light_ *= LibMatrix::Mat4::translate(0.5, 0.5, 0.5);
    light_ *= LibMatrix::Mat4::scale(0.5, 0.5, 0.5);
    light_ *= projection_.getCurrent();
    light_ *= LibMatrix::Mat4::lookAt(lightPosition.x(), lightPosition.y(), lightPosition.z(),
                                      0.0, 0.0, 0.0,
                                      0.0, 1.0, 0.0);

    if (!depthTarget_.setup(canvas_.width(), canvas_.height())) {
        Log::error("Failed to set up the render target for the depth pass\n");
        return false;
    }

    return true;
}
void
RefractPrivate::teardown()
{
    depthTarget_.teardown();
    program_.stop();
    program_.release();
    mesh_.reset();
}

void
RefractPrivate::update(double elapsedTime)
{
    rotation_ = rotationSpeed_ * elapsedTime;
}

void
RefractPrivate::draw()
{
    // To perform the depth pass, set up the model-view transformation so
    // that we're looking at the horse from the light position.  That will
    // give us the appropriate view for the shadow.
    modelview_.push();
    modelview_.loadIdentity();
    modelview_.lookAt(lightPosition.x(), lightPosition.y(), lightPosition.z(),
                      0.0, 0.0, 0.0,
                      0.0, 1.0, 0.0);
    modelview_.rotate(rotation_, 0.0f, 1.0f, 0.0f);
    if (orientModel_)
    {
        modelview_.rotate(orientationAngle_, orientationVec_.x(), orientationVec_.y(), orientationVec_.z());
    }
    mat4 mvp(projection_.getCurrent());
    mvp *= modelview_.getCurrent();
    modelview_.pop();

    // Enable the depth render target with our transformation and render.
    depthTarget_.enable(mvp);
    vector<GLint> attrib_locations;
    attrib_locations.push_back(depthTarget_.program()["position"].location());
    attrib_locations.push_back(depthTarget_.program()["normal"].location());
    mesh_.set_attrib_locations(attrib_locations);
    if (useVbo_) {
        mesh_.render_vbo();
    }
    else {
        mesh_.render_array();
    }
    depthTarget_.disable();

    // Draw the "normal" view of the horse
    modelview_.push();
    modelview_.translate(-centerVec_.x(), -centerVec_.y(), -(centerVec_.z() + 2.0 + radius_));
    modelview_.rotate(rotation_, 0.0f, 1.0f, 0.0f);
    if (orientModel_)
    {
        modelview_.rotate(orientationAngle_, orientationVec_.x(), orientationVec_.y(), orientationVec_.z());
    }
    mvp = projection_.getCurrent();
    mvp *= modelview_.getCurrent();

    program_.start();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTarget_.depthTexture());
    program_["DistanceMap"] = 0;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTarget_.colorTexture());
    program_["NormalMap"] = 1;
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_);
    program_["ImageMap"] = 2;
    // Load both the modelview*projection as well as the modelview matrix itself
    program_["ModelViewProjectionMatrix"] = mvp;
    program_["ModelViewMatrix"] = modelview_.getCurrent();
    // Load the NormalMatrix uniform in the shader. The NormalMatrix is the
    // inverse transpose of the model view matrix.
    mat4 normal_matrix(modelview_.getCurrent());
    normal_matrix.inverse().transpose();
    program_["NormalMatrix"] = normal_matrix;
    program_["LightMatrix"] = light_;
    attrib_locations.clear();
    attrib_locations.push_back(program_["position"].location());
    attrib_locations.push_back(program_["normal"].location());
    mesh_.set_attrib_locations(attrib_locations);
    if (useVbo_) {
        mesh_.render_vbo();
    }
    else {
        mesh_.render_array();
    }

    // Per-frame cleanup
    modelview_.pop();
}

