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
#ifndef LAMP_H_
#define LAMP_H_

#include <string>
#include <vector>
#include "vec.h"
#include "stack.h"
#include "gl-headers.h"
#include "program.h"

class Lamp
{
public:
    Lamp();
    ~Lamp();

    void init();
    bool valid() const { return valid_; }
    void draw(LibMatrix::Stack4& modelview, LibMatrix::Stack4& projection,
              const LibMatrix::vec4* lightPositions);
private:    
    Program litProgram_;
    Program unlitProgram_;
    std::string litVertexShader_;
    std::string litFragmentShader_;
    std::string unlitVertexShader_;
    std::string unlitFragmentShader_;
    static const std::string modelviewName_;
    static const std::string projectionName_;
    static const std::string light0PositionName_;
    static const std::string light1PositionName_;
    static const std::string light2PositionName_;
    static const std::string vertexAttribName_;
    static const std::string normalAttribName_;
    static const std::string normalMatrixName_;
    std::vector<LibMatrix::vec3> vertexData_;
    std::vector<unsigned short> indexData_;
    unsigned int bufferObjects_[2];
    bool valid_;
};

#endif // LAMP_H_
