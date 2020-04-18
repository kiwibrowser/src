/*
 * Vertex position data describing the letter 'o'
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

LetterN::LetterN()
{
    // Vertex data...
    vertexData_.push_back(vec2(1.009307, 9.444788));
    vertexData_.push_back(vec2(2.548087, 9.742002));
    vertexData_.push_back(vec2(1.737332, 9.213622));
    vertexData_.push_back(vec2(2.994829, 9.659443));
    vertexData_.push_back(vec2(1.985522, 8.751290));
    vertexData_.push_back(vec2(3.127198, 9.180598));
    vertexData_.push_back(vec2(1.935884, 7.975232));
    vertexData_.push_back(vec2(2.481903, 6.571723));
    vertexData_.push_back(vec2(1.472596, 5.019608));
    vertexData_.push_back(vec2(1.439504, 2.988648));
    vertexData_.push_back(vec2(1.025853, 2.988648));
    vertexData_.push_back(vec2(2.283350, 6.059855));
    vertexData_.push_back(vec2(2.035160, 5.366357));
    vertexData_.push_back(vec2(3.292658, 7.711042));
    vertexData_.push_back(vec2(3.540848, 7.744066));
    vertexData_.push_back(vec2(4.384695, 9.031992));
    vertexData_.push_back(vec2(4.699069, 8.916409));
    vertexData_.push_back(vec2(5.609100, 9.808049));
    vertexData_.push_back(vec2(5.145812, 8.982456));
    vertexData_.push_back(vec2(6.155119, 9.791537));
    vertexData_.push_back(vec2(5.410548, 8.635707));
    vertexData_.push_back(vec2(6.337125, 9.312694));
    vertexData_.push_back(vec2(5.360910, 7.991744));
    vertexData_.push_back(vec2(6.088935, 8.090816));
    vertexData_.push_back(vec2(4.947259, 5.977296));
    vertexData_.push_back(vec2(5.261634, 4.804954));
    vertexData_.push_back(vec2(4.616339, 4.028896));
    vertexData_.push_back(vec2(5.211996, 3.962848));
    vertexData_.push_back(vec2(4.732162, 3.318886));
    vertexData_.push_back(vec2(5.559462, 3.814241));
    vertexData_.push_back(vec2(5.228542, 3.038184));
    vertexData_.push_back(vec2(5.940021, 3.814241));
    vertexData_.push_back(vec2(5.906929, 3.335397));
    vertexData_.push_back(vec2(6.684591, 4.094943));

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
    indexData_.push_back(33);
    indexData_.push_back(0);
    indexData_.push_back(2);
    indexData_.push_back(4);
    indexData_.push_back(6);
    indexData_.push_back(8);
    indexData_.push_back(10);
    indexData_.push_back(9);
    indexData_.push_back(7);
    indexData_.push_back(5);
    indexData_.push_back(3);
    indexData_.push_back(1);
    indexData_.push_back(12);
    indexData_.push_back(14);
    indexData_.push_back(16);
    indexData_.push_back(18);
    indexData_.push_back(20);
    indexData_.push_back(22);
    indexData_.push_back(24);
    indexData_.push_back(26);
    indexData_.push_back(28);
    indexData_.push_back(30);
    indexData_.push_back(32);
    indexData_.push_back(33);
    indexData_.push_back(31);
    indexData_.push_back(29);
    indexData_.push_back(27);
    indexData_.push_back(25);
    indexData_.push_back(23);
    indexData_.push_back(21);
    indexData_.push_back(19);
    indexData_.push_back(17);
    indexData_.push_back(15);
    indexData_.push_back(13);
    indexData_.push_back(11);

    // Primitive state so that the draw call can issue the primitives we want.
    unsigned int curOffset(0);
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 11, curOffset));
    curOffset += (11 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 23, curOffset));
    curOffset += (23 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 11, curOffset));
    curOffset += (11 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 23, curOffset));
}
