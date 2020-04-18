/*
 * Vertex position data describing the letter 't'
 *
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
#include "characters.h"

using LibMatrix::vec2;

LetterT::LetterT()
{
    // Vertex data...
    vertexData_.push_back(vec2(2.986667, 14.034801));
    vertexData_.push_back(vec2(2.445128, 10.088024));
    vertexData_.push_back(vec2(1.788718, 9.236438));
    vertexData_.push_back(vec2(2.264615, 7.664279));
    vertexData_.push_back(vec2(1.165128, 5.666326));
    vertexData_.push_back(vec2(2.034872, 4.945752));
    vertexData_.push_back(vec2(1.132308, 3.766633));
    vertexData_.push_back(vec2(2.182564, 3.570113));
    vertexData_.push_back(vec2(1.411282, 2.309109));
    vertexData_.push_back(vec2(2.510769, 2.341863));
    vertexData_.push_back(vec2(2.149744, 1.048106));
    vertexData_.push_back(vec2(3.364103, 1.375640));
    vertexData_.push_back(vec2(3.167180, 0.327533));
    vertexData_.push_back(vec2(4.381538, 0.736950));
    vertexData_.push_back(vec2(5.005128, 0.032753));
    vertexData_.push_back(vec2(5.612308, 0.638690));
    vertexData_.push_back(vec2(6.235898, 0.540430));
    vertexData_.push_back(vec2(7.187692, 1.162743));
    vertexData_.push_back(vec2(1.985641, 9.039918));
    vertexData_.push_back(vec2(2.133333, 10.186285));
    vertexData_.push_back(vec2(1.509744, 9.023541));
    vertexData_.push_back(vec2(1.608205, 9.662231));
    vertexData_.push_back(vec2(1.050256, 9.023541));
    vertexData_.push_back(vec2(1.050256, 9.334698));
    vertexData_.push_back(vec2(0.196923, 9.007165));
    vertexData_.push_back(vec2(2.363077, 9.711361));
    vertexData_.push_back(vec2(2.264615, 9.023541));
    vertexData_.push_back(vec2(3.282051, 9.563972));
    vertexData_.push_back(vec2(3.446154, 9.023541));
    vertexData_.push_back(vec2(4.069744, 9.531218));
    vertexData_.push_back(vec2(4.299487, 9.236438));
    vertexData_.push_back(vec2(4.644103, 9.613101));
    vertexData_.push_back(vec2(5.251282, 9.875128));

    // Index data...
    indexData_.push_back(0);
    indexData_.push_back(1);
    indexData_.push_back(2);
    indexData_.push_back(3);
    indexData_.push_back(4);
    indexData_.push_back(5);
    indexData_.push_back(6);
    indexData_.push_back(7);
    indexData_.push_back(8);
    indexData_.push_back(9);
    indexData_.push_back(10);
    indexData_.push_back(11);
    indexData_.push_back(12);
    indexData_.push_back(13);
    indexData_.push_back(14);
    indexData_.push_back(15);
    indexData_.push_back(16);
    indexData_.push_back(17);
    indexData_.push_back(18);
    indexData_.push_back(19);
    indexData_.push_back(20);
    indexData_.push_back(21);
    indexData_.push_back(22);
    indexData_.push_back(23);
    indexData_.push_back(24);
    indexData_.push_back(25);
    indexData_.push_back(26);
    indexData_.push_back(27);
    indexData_.push_back(28);
    indexData_.push_back(29);
    indexData_.push_back(30);
    indexData_.push_back(31);
    indexData_.push_back(32);
    indexData_.push_back(0);
    indexData_.push_back(2);
    indexData_.push_back(4);
    indexData_.push_back(6);
    indexData_.push_back(8);
    indexData_.push_back(10);
    indexData_.push_back(12);
    indexData_.push_back(14);
    indexData_.push_back(16);
    indexData_.push_back(17);
    indexData_.push_back(15);
    indexData_.push_back(13);
    indexData_.push_back(11);
    indexData_.push_back(9);
    indexData_.push_back(7);
    indexData_.push_back(5);
    indexData_.push_back(3);
    indexData_.push_back(1);
    indexData_.push_back(18);
    indexData_.push_back(20);
    indexData_.push_back(22);
    indexData_.push_back(24);
    indexData_.push_back(23);
    indexData_.push_back(21);
    indexData_.push_back(19);
    indexData_.push_back(26);
    indexData_.push_back(28);
    indexData_.push_back(30);
    indexData_.push_back(32);
    indexData_.push_back(31);
    indexData_.push_back(29);
    indexData_.push_back(27);
    indexData_.push_back(25);

    // Primitive state so that the draw call can issue the primitives we want.
    unsigned int curOffset(0);
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 18, curOffset));
    curOffset += (18 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 7, curOffset));
    curOffset += (7 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 8, curOffset));
    curOffset += (8 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 18, curOffset));
    curOffset += (18 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 7, curOffset));
    curOffset += (7 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 8, curOffset));
}
