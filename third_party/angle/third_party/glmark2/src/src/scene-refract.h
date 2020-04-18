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
#ifndef SCENE_REFRACT_
#define SCENE_REFRACT_

#include "scene.h"
#include "stack.h"

//
// To create a shadow map, we need a framebuffer object set up for a 
// depth-only pass.  The render target can then be bound as a texture,
// and the depth values sampled from that texture can be used in the
// distance-from-light computations when rendering the shadow on the
// ground below the rendered object.
//
class DistanceRenderTarget
{
    enum
    {
        DEPTH = 0,
        COLOR
    };
    Program program_;
    unsigned int canvas_width_;
    unsigned int canvas_height_;
    unsigned int width_;
    unsigned int height_;
    unsigned int tex_[2];
    unsigned int fbo_;
public:
    DistanceRenderTarget() :
        canvas_width_(0),
        canvas_height_(0),
        width_(0),
        height_(0),
        fbo_(0)
    {
        tex_[DEPTH] = tex_[COLOR] = 0;
    }
    ~DistanceRenderTarget() {}
    bool setup(unsigned int width, unsigned int height);
    void teardown();
    void enable(const LibMatrix::mat4& mvp);
    void disable();
    unsigned int depthTexture() { return tex_[DEPTH]; }
    unsigned int colorTexture() { return tex_[COLOR]; }
    Program& program() { return program_; }
};

class RefractPrivate
{
    Canvas& canvas_;
    DistanceRenderTarget depthTarget_;
    Program program_;
    LibMatrix::Stack4 modelview_;
    LibMatrix::Stack4 projection_;
    LibMatrix::mat4 light_;
    Mesh mesh_;
    LibMatrix::vec3 centerVec_;
    bool orientModel_;
    float orientationAngle_;
    LibMatrix::vec3 orientationVec_;
    float radius_;
    float rotation_;
    float rotationSpeed_;
    unsigned int texture_;
    bool useVbo_;
    
public:
    RefractPrivate(Canvas& canvas) :
        canvas_(canvas),
        orientModel_(false),
        orientationAngle_(0.0),
        radius_(0.0),
        rotation_(0.0),
        rotationSpeed_(36.0),
        texture_(0),
        useVbo_(true) {}
    ~RefractPrivate() {}

    bool setup(std::map<std::string, Scene::Option>& options);
    void teardown();
    void update(double elapsedTime);
    void draw();
};

#endif // SCENE_REFRACT_
