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
#ifndef SCENE_JELLYFISH_
#define SCENE_JELLYFISH_
#include <vector>
#include "vec.h"
#include "stack.h"
#include "program.h"

class GradientRenderer
{
    Program program_;
    int positionLocation_;
    int uvLocation_;
    unsigned int uvOffset_;
    unsigned int bufferObject_;
    std::vector<LibMatrix::vec2> vertices_;
    std::vector<LibMatrix::vec2> uvs_;
    
public:
    GradientRenderer() :
        positionLocation_(0),
        uvLocation_(0),
        uvOffset_(0),
        bufferObject_(0) {}
    ~GradientRenderer()
    {
        vertices_.clear();
        uvs_.clear();
    }
    bool init();
    void cleanup();
    void draw();
};

class JellyfishPrivate
{
    bool load_obj(const std::string& filename);

    // For the background gradient.
    GradientRenderer gradient_;

    // Vertex data.
    std::vector<LibMatrix::vec3> positions_;
    std::vector<LibMatrix::vec3> normals_;
    std::vector<LibMatrix::vec3> colors_;
    std::vector<LibMatrix::vec3> texcoords_;
    std::vector<unsigned short> indices_;
    // A simple map so we know where each section of our data starts within
    // our vertex buffer object.
    struct VertexDataMap
    {
        unsigned int positionOffset;
        unsigned int positionSize;
        unsigned int normalOffset;
        unsigned int normalSize;
        unsigned int colorOffset;
        unsigned int colorSize;
        unsigned int texcoordOffset;
        unsigned int texcoordSize;
        unsigned int totalSize;
    } dataMap_;
    // Object handles
    unsigned int bufferObjects_[2];
    unsigned int textureObjects_[33];
    unsigned int whichCaustic_;
    std::map<std::string, unsigned int> causticMap_;

    // Program state, including attributes, uniforms, and, locations.
    Program program_;
    int positionLocation_;
    int normalLocation_;
    int colorLocation_;
    int texcoordLocation_;
    LibMatrix::vec2 viewport_;
    LibMatrix::Stack4 world_;
    LibMatrix::Stack4 projection_;
    LibMatrix::vec3 lightPosition_;
    LibMatrix::vec4 lightColor_;
    float lightRadius_;
    LibMatrix::vec4 ambientColor_;
    LibMatrix::vec4 fresnelColor_;
    float fresnelPower_;
    float rotation_;
    float currentTime_;
    double lastUpdateTime_;
    // GL state we plan to override, so we can restore it cleanly.
    unsigned int cullFace_;
    unsigned int depthTest_;
    unsigned int blend_;
    int blendFuncSrc_;
    int blendFuncDst_;

public:
    JellyfishPrivate();
    ~JellyfishPrivate();
    bool initialize();
    void update_viewport(const LibMatrix::vec2& viewport);
    void update_time();
    void cleanup();
    void draw();
};

#endif // SCENE_JELLYFISH_
