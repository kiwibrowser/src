/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
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
 *  Jesse Barker
 */
#ifndef LOGO_H_
#define LOGO_H_

#include <string>
#include <vector>
#include "vec.h"
#include "stack.h"
#include "gl-headers.h"
#include "program.h"

class SGILogo
{
public:
    SGILogo();
    ~SGILogo();

    // Initialize the logo
    void init();
    bool valid() const { return valid_; }
    void setPosition(const LibMatrix::vec3& position) { currentPosition_ = position; }
    // Tell the logo to draw itself. DrawStyle tells it how.
    // - LOGO_NORMAL renders the logo lit and shaded.
    // - LOGO_FLAT renders the logo as if flattened onto a surface.
    // - LOGO_SHADOW renders a stippled-looking shadow of the object.
    enum DrawStyle
    {
        LOGO_NORMAL, 
        LOGO_FLAT, 
        LOGO_SHADOW
    };
    void draw(LibMatrix::Stack4& modelview, LibMatrix::Stack4& projection, 
              const LibMatrix::vec4& lightPosition, DrawStyle style, 
              const LibMatrix::uvec3& color = LibMatrix::uvec3());

private:
    void drawElbow();
    void drawSingleCylinder();
    void drawDoubleCylinder();
    void bendLeft(LibMatrix::Stack4& ms);
    void bendRight(LibMatrix::Stack4& ms);
    void bendForward(LibMatrix::Stack4& ms);
    void updateXform(const LibMatrix::mat4& mv, Program& program);
    Program& getProgram();
    LibMatrix::vec3 currentPosition_;
    std::vector<LibMatrix::vec3> singleCylinderVertices_;
    std::vector<LibMatrix::vec3> singleCylinderNormals_;
    std::vector<LibMatrix::vec3> doubleCylinderVertices_;
    std::vector<LibMatrix::vec3> doubleCylinderNormals_;
    std::vector<LibMatrix::vec3> elbowVertices_;
    std::vector<LibMatrix::vec3> elbowNormals_;
    std::vector<LibMatrix::vec3> elbowShadowVertices_;
    std::vector<unsigned short> indexData_;
    // A simple map so we know where each section of our data starts within
    // our vertex buffer object.
    struct VertexDataMap
    {
        unsigned int scvOffset;
        unsigned int scvSize;
        unsigned int scnOffset;
        unsigned int scnSize;
        unsigned int dcvOffset;
        unsigned int dcvSize;
        unsigned int dcnOffset;
        unsigned int dcnSize;
        unsigned int evOffset;
        unsigned int evSize;
        unsigned int enOffset;
        unsigned int enSize;
        unsigned int esvOffset;
        unsigned int esvSize;
        unsigned int totalSize;
    } dataMap_;
    unsigned int bufferObjects_[2];
    Program normalProgram_;
    Program flatProgram_;
    Program shadowProgram_;
    std::string normalVertexShader_;
    std::string normalFragmentShader_;
    std::string flatVertexShader_;
    std::string flatFragmentShader_;
    std::string shadowVertexShader_;
    std::string shadowFragmentShader_;
    int normalNormalIndex_;
    int normalVertexIndex_;
    int flatVertexIndex_;
    int shadowVertexIndex_;
    int vertexIndex_;
    static const std::string modelviewName_;
    static const std::string projectionName_;
    static const std::string lightPositionName_;
    static const std::string logoColorName_;
    static const std::string vertexAttribName_;
    static const std::string normalAttribName_;
    static const std::string normalMatrixName_;
    // "Shadow" state
    GLuint textureName_;
    GLubyte textureImage_[32][32];
    // This is the size in each direction of our texture image
    static const unsigned int textureResolution_;
    bool valid_;
    DrawStyle drawStyle_;
};

#endif // LOGO_H_
