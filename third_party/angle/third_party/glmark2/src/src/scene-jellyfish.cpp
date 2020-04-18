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
//  Aleksandar Rodic - Creator and WebGL implementation 
//  Jesse Barker - glmark2 port
//
#include <string>
#include <fstream>
#include <memory>
#include <iomanip>
#include "scene.h"
#include "scene-jellyfish.h"
#include "log.h"
#include "util.h"
#include "texture.h"
#include "shader-source.h"

SceneJellyfish::SceneJellyfish(Canvas& canvas) :
    Scene(canvas, "jellyfish"), priv_(0)
{

}

SceneJellyfish::~SceneJellyfish()
{
    delete priv_;
}

bool
SceneJellyfish::load()
{
    running_ = false;
    return true;
}

void
SceneJellyfish::unload()
{
}

bool
SceneJellyfish::setup()
{
    if (!Scene::setup())
        return false;

    // Set up our private object that does all of the lifting
    priv_ = new JellyfishPrivate();
    if (!priv_->initialize())
        return false;

    // Set core scene timing after actual initialization so we don't measure
    // set up time.
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;
    running_ = true;

    return true;
}

void
SceneJellyfish::teardown()
{
    priv_->cleanup();
    Scene::teardown();
}

void
SceneJellyfish::update()
{
    Scene::update();
    priv_->update_viewport(LibMatrix::vec2(canvas_.width(), canvas_.height()));
    priv_->update_time();
}

void
SceneJellyfish::draw()
{
    priv_->draw();
}

Scene::ValidationResult
SceneJellyfish::validate()
{
    return Scene::ValidationUnknown;
}


//
// JellyfishPrivate implementation
//
using LibMatrix::mat4;
using LibMatrix::vec3;
using LibMatrix::vec2;
using std::string;
using std::vector;

bool
GradientRenderer::init()
{
    // Program set up
    static const string vtx_shader_filename(GLMARK_DATA_PATH"/shaders/gradient.vert");
    static const string frg_shader_filename(GLMARK_DATA_PATH"/shaders/gradient.frag");
    ShaderSource vtx_source(vtx_shader_filename);
    ShaderSource frg_source(frg_shader_filename);
    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(),
        frg_source.str()))
    {
        return false;
    }
    positionLocation_ = program_["position"].location();
    uvLocation_ = program_["uvIn"].location();

    // Set up the position data for our "quad".
    vertices_.push_back(vec2(-1.0, -1.0));
    vertices_.push_back(vec2(1.0, -1.0));
    vertices_.push_back(vec2(-1.0, 1.0));
    vertices_.push_back(vec2(1.0, 1.0));
    uvs_.push_back(vec2(1.0, 1.0));
    uvs_.push_back(vec2(1.0, 1.0));
    uvs_.push_back(vec2(0.0, 0.0));
    uvs_.push_back(vec2(0.0, 0.0));
    uvOffset_ = vertices_.size() * sizeof(vec2);

    // Set up the VBO and stash our position data in it.
    glGenBuffers(1, &bufferObject_);
    glBindBuffer(GL_ARRAY_BUFFER, bufferObject_);
    glBufferData(GL_ARRAY_BUFFER, (vertices_.size() + uvs_.size()) * sizeof(vec2),
                 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_.size() * sizeof(vec2),
                    &vertices_.front());
    glBufferSubData(GL_ARRAY_BUFFER, uvOffset_, uvs_.size() * sizeof(vec2),
                    &uvs_.front());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void
GradientRenderer::cleanup()
{
    program_.stop();
    program_.release();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &bufferObject_);
}

void
GradientRenderer::draw()
{
    static const vec3 lightBlue(0.360784314, 0.584313725, 1.0);
    static const vec3 darkBlue(0.074509804, 0.156862745, 0.619607843);
    glBindBuffer(GL_ARRAY_BUFFER, bufferObject_);
    program_.start();
    program_["color1"] = lightBlue;
    program_["color2"] = darkBlue;
    glEnableVertexAttribArray(positionLocation_);
    glEnableVertexAttribArray(uvLocation_);
    glVertexAttribPointer(positionLocation_, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(uvLocation_, 2, GL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<GLvoid*>(uvOffset_));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(positionLocation_);
    glDisableVertexAttribArray(uvLocation_);
    program_.stop();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//!
// Parse index values from an OBJ file.
//
// @param source the source line to parse
// @param idx the unsigned short to populate
//
static void
obj_get_index(const string& source, unsigned short& idx)
{
     // Skip the definition type...
    string::size_type endPos = source.find(" ");
    string::size_type startPos(0);
    if (endPos == string::npos)
    {
        Log::error("Bad element '%s'\n", source.c_str());
        return;
    }
    // Find the first value...
    startPos = endPos + 1;
    string is(source, startPos);
    idx = Util::fromString<unsigned short>(is);   
}

//!
// Parse vec3 values from an OBJ file.
//
// @param source the source line to parse
// @param v the vec3 to populate
//
static void
obj_get_values(const string& source, vec3& v)
{
    // Skip the definition type...
    string::size_type endPos = source.find(" ");
    string::size_type startPos(0);
    if (endPos == string::npos)
    {
        Log::error("Bad element '%s'\n", source.c_str());
        return;
    }
    // Find the first value...
    startPos = endPos + 1;
    endPos = source.find(" ", startPos);
    if (endPos == string::npos)
    {
        Log::error("Bad element '%s'\n", source.c_str());
        return;
    }
    string::size_type numChars(endPos - startPos);
    string xs(source, startPos, numChars);
    float x = Util::fromString<float>(xs);
    // Then the second value...
    startPos = endPos + 1;
    endPos = source.find(" ", startPos);
    if (endPos == string::npos)
    {
        Log::error("Bad element '%s'\n", source.c_str());
        return;
    }
    numChars = endPos - startPos;
    string ys(source, startPos, numChars);
    float y = Util::fromString<float>(ys);
    // And the third value (there might be a fourth, but we don't care)...
    startPos = endPos + 1;
    endPos = source.find(" ", startPos);
    if (endPos == string::npos)
    {
        numChars = endPos;
    }
    else
    {
        numChars = endPos - startPos;
    }
    string zs(source, startPos, endPos - startPos);
    float z = Util::fromString<float>(zs);
    v.x(x);
    v.y(y);
    v.z(z);
}

// Custom OBJ loader.
//
// To support the jellyfish model, some amendments to the OBJ format are
// necessary.  In particular, a vertex color attribute is required, and
// it contains an index list rather than a face list.
bool
JellyfishPrivate::load_obj(const std::string &filename)
{
    Log::debug("Loading model from file '%s'\n", filename.c_str());

    const std::unique_ptr<std::istream> input_file_ptr(Util::get_resource(filename));
    std::istream& inputFile(*input_file_ptr);
    if (!inputFile)
    {
        Log::error("Failed to open '%s'\n", filename.c_str());
        return false;
    }

    vector<string> sourceVec;
    string curLine;
    while (getline(inputFile, curLine))
    {
        sourceVec.push_back(curLine);
    }

    static const string vertex_definition("v");
    static const string normal_definition("vn");
    static const string texcoord_definition("vt");
    static const string color_definition("vc");
    static const string index_definition("i");
    for (vector<string>::const_iterator lineIt = sourceVec.begin();
         lineIt != sourceVec.end();
         lineIt++)
    {
        const string& curSrc = *lineIt;
        // Is it a vertex attribute, a face description, comment or other?
        // We only care about the first two, we ignore comments, object names,
        // group names, smoothing groups, etc.
        string::size_type startPos(0);
        string::size_type spacePos = curSrc.find(" ", startPos);
        string definitionType(curSrc, startPos, spacePos - startPos);
        if (definitionType == vertex_definition)
        {
            vec3 v;
            obj_get_values(curSrc, v);
            positions_.push_back(v);
        }
        else if (definitionType == normal_definition)
        {
            vec3 v;
            obj_get_values(curSrc, v);
            normals_.push_back(v);
        }
        else if (definitionType == color_definition)
        {
            vec3 v;
            obj_get_values(curSrc, v);
            colors_.push_back(v);
        }
        else if (definitionType == texcoord_definition)
        {
            vec3 v;
            obj_get_values(curSrc, v);
            texcoords_.push_back(v);
        }
        else if (definitionType == index_definition)
        {
            unsigned short idx(0);
            obj_get_index(curSrc, idx);
            indices_.push_back(idx);
        }
    }

    Log::debug("Object populated with %u vertices %u normals %u colors %u texcoords and %u indices.\n",
        positions_.size(), normals_.size(), colors_.size(), texcoords_.size(), indices_.size());
    return true;
}

JellyfishPrivate::JellyfishPrivate() :
    positionLocation_(0),
    normalLocation_(0),
    colorLocation_(0),
    texcoordLocation_(0),
    viewport_(512.0, 512.0),
    lightPosition_(10.0, 40.0, -60.0),
    lightColor_(0.8, 1.3, 1.1, 1.0),
    lightRadius_(200.0),
    ambientColor_(0.3, 0.2, 1.0, 1.0),
    fresnelColor_(0.8, 0.7, 0.6, 1.1),
    fresnelPower_(1.0),
    rotation_(0.0),
    currentTime_(0.0),
    lastUpdateTime_(0.0),
    cullFace_(0),
    depthTest_(0),
    blend_(0),
    blendFuncSrc_(0),
    blendFuncDst_(0)
{
}

JellyfishPrivate::~JellyfishPrivate()
{
    positions_.clear();
    normals_.clear();
    colors_.clear();
    texcoords_.clear();
    indices_.clear();
}

bool
JellyfishPrivate::initialize()
{
    static const string modelFilename(GLMARK_DATA_PATH"/models/jellyfish.jobj");
    if (!load_obj(modelFilename))
    {
        return false;
    }

    // Now that we've setup the vertex data, we can setup the map of how
    // that data will be laid out in the buffer object.
    static const unsigned int sv3(sizeof(vec3));
    dataMap_.positionOffset = 0;
    dataMap_.positionSize = positions_.size() * sv3;
    dataMap_.totalSize = dataMap_.positionSize;
    dataMap_.normalOffset = dataMap_.positionOffset + dataMap_.positionSize;
    dataMap_.normalSize = normals_.size() * sv3;
    dataMap_.totalSize += dataMap_.normalSize;
    dataMap_.colorOffset = dataMap_.normalOffset + dataMap_.normalSize;
    dataMap_.colorSize = colors_.size() * sv3;
    dataMap_.totalSize += dataMap_.colorSize;
    dataMap_.texcoordOffset = dataMap_.colorOffset + dataMap_.colorSize;
    dataMap_.texcoordSize = texcoords_.size() * sv3;
    dataMap_.totalSize += dataMap_.texcoordSize;

    lastUpdateTime_ = Util::get_timestamp_us() / 1000.0;
    currentTime_ = static_cast<uint64_t>(lastUpdateTime_) % 100000000 / 1000.0;
    whichCaustic_ = static_cast<uint64_t>(currentTime_ * 30) % 32 + 1;
    rotation_ = 0.0;

    if (!gradient_.init())
    {
        return false;
    }

    // Set up program first so we can store attribute and uniform locations
    // away for the 
    using std::string;
    static const string vtx_shader_filename(GLMARK_DATA_PATH"/shaders/jellyfish.vert");
    static const string frg_shader_filename(GLMARK_DATA_PATH"/shaders/jellyfish.frag");

    ShaderSource vtx_source(vtx_shader_filename);
    ShaderSource frg_source(frg_shader_filename);

    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(),
        frg_source.str()))
    {
        return false;
    }

    // Stash away attribute and uniform locations for handy use.
    positionLocation_ = program_["aVertexPosition"].location();
    normalLocation_ = program_["aVertexNormal"].location();
    colorLocation_ = program_["aVertexColor"].location();
    texcoordLocation_ = program_["aTextureCoord"].location();

    // We need 2 buffers for our work here.  One for the vertex data.
    // and one for the index data.
    glGenBuffers(2, &bufferObjects_[0]);

    // First, setup the vertex data by binding the first buffer object, 
    // allocating its data store, and filling it in with our vertex data.
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
    glBufferData(GL_ARRAY_BUFFER, dataMap_.totalSize, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.positionOffset,
                    dataMap_.positionSize, &positions_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.normalOffset,
                    dataMap_.normalSize, &normals_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.colorOffset,
                    dataMap_.colorSize, &colors_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.texcoordOffset,
                    dataMap_.texcoordSize, &texcoords_.front());

    // Now repeat for our index data.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned short),
                 &indices_.front(), GL_STATIC_DRAW);

    // "Unbind" our buffer objects to make sure the state is consistent.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Finally, set up our textures.
    //
    // First, the main jellyfish texture
    bool gotTex = Texture::load("jellyfish256", &textureObjects_[0], GL_LINEAR,
                                GL_LINEAR, 0);
    if (!gotTex || textureObjects_[0] == 0)
    {
        Log::error("Jellyfish texture set up failed!!!\n");
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // Then, the caustics textures
    static const string baseName("jellyfish-caustics-");
    for (unsigned int i = 1; i < 33; i++)
    {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << i;
        string curName(baseName);
        curName += ss.str();
        gotTex = Texture::load(curName, &textureObjects_[i], GL_LINEAR,
                               GL_LINEAR, 0);
        if (!gotTex || textureObjects_[i] == 0)
        {
            Log::error("Caustics texture[%u] set up failed!!!\n", i);
            return false;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Save the GL state we are changing so we can restore it later.
    cullFace_ = glIsEnabled(GL_CULL_FACE);
    depthTest_ = glIsEnabled(GL_DEPTH_TEST);
    blend_ = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendFuncSrc_);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendFuncDst_);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    return true;
}

void
JellyfishPrivate::update_viewport(const vec2& vp)
{
    if (viewport_.x() == vp.x() && viewport_.y() == vp.y())
    {
        return;
    }
    viewport_ = vp;
    projection_.loadIdentity();
    projection_.perspective(30.0, viewport_.x()/viewport_.y(), 20.0, 120.0);    
}

void
JellyfishPrivate::update_time()
{
    double now = Util::get_timestamp_us() / 1000.0;
    double elapsedTime = now - lastUpdateTime_;
    rotation_ += (2.0 * elapsedTime) / 1000.0;
    currentTime_ = static_cast<uint64_t>(now) % 100000000 / 1000.0;
    whichCaustic_ = static_cast<uint64_t>(currentTime_ * 30) % 32 + 1;
    lastUpdateTime_ = now;
}

void
JellyfishPrivate::cleanup()
{
    // Restore the GL state we changed for the scene.
    glBlendFunc(blendFuncSrc_, blendFuncDst_);
    if (GL_FALSE == blend_)
    {
        glDisable(GL_BLEND);
    }
    if (GL_TRUE == cullFace_)
    {
        glEnable(GL_CULL_FACE);
    }
    if (GL_TRUE == depthTest_)
    {
        glEnable(GL_DEPTH_TEST);
    }

    program_.stop();
    program_.release();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDeleteTextures(33, &textureObjects_[0]);
    glDeleteBuffers(2, &bufferObjects_[0]);

    gradient_.cleanup();
}

void
JellyfishPrivate::draw()
{
    // "Clear" the background to the desired gradient.
    gradient_.draw();

    // We need "world", "world view projection", and "world inverse transpose"
    // matrix uniforms for the current shader.
    //
    // NOTE: Some of this seems a bit of a no-op (e.g., multipying by and
    //       inverting identity matrices), but leave it like the original
    //       WebGL files for the time being.  Worth revisiting not doing all
    //       of that math every draw call (might even be good to be doing
    //       some of it in the shader as well).
    world_.push();
    world_.translate(0.0, 5.0, -75.0);
    world_.rotate(sin(rotation_ / 10.0) * 30.0, 0.0, 1.0, 0.0);
    world_.rotate(sin(rotation_ / 20.0) * 30.0, 1.0, 0.0, 0.0);
    world_.scale(5.0, 5.0, 5.0);
    world_.translate(0.0, sin(rotation_ / 10.0) * 2.5, 0.0);
    mat4 worldViewProjection(projection_.getCurrent());
    worldViewProjection *= world_.getCurrent();;
    mat4 worldInverseTranspose(world_.getCurrent());
    worldInverseTranspose.inverse().transpose();

    // Load up the uniforms
    program_.start();
    program_["uWorld"] = world_.getCurrent();
    program_["uWorldViewProj"] = worldViewProjection;
    program_["uWorldInvTranspose"] = worldInverseTranspose;
    program_["uCurrentTime"] = currentTime_;
    // Revisit making these constants rather than uniforms as they appear never
    // to change
    program_["uLightPos"] = lightPosition_;
    program_["uLightRadius"] = lightRadius_;
    program_["uLightCol"] = lightColor_;
    program_["uAmbientCol"] = ambientColor_;
    program_["uFresnelCol"] = fresnelColor_;
    program_["uFresnelPower"] = fresnelPower_;
    // Set up textures for this frame.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureObjects_[0]);
    program_["uSampler"] = 0;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureObjects_[whichCaustic_]);
    program_["uSampler1"] = 1;

    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);

    glEnableVertexAttribArray(positionLocation_);
    glEnableVertexAttribArray(normalLocation_);
    glEnableVertexAttribArray(colorLocation_);
    glEnableVertexAttribArray(texcoordLocation_);
    glVertexAttribPointer(positionLocation_ , 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.positionOffset));
    glVertexAttribPointer(normalLocation_ , 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.normalOffset));
    glVertexAttribPointer(colorLocation_ , 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.colorOffset));
    glVertexAttribPointer(texcoordLocation_ , 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.texcoordOffset));

    glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(positionLocation_);
    glDisableVertexAttribArray(normalLocation_);
    glDisableVertexAttribArray(colorLocation_);
    glDisableVertexAttribArray(texcoordLocation_);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    program_.stop();
    world_.pop();
}
