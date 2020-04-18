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

LetterO::LetterO()
{
    // Vertex data...
    vertexData_.push_back(vec2(2.975610, 9.603255));
    vertexData_.push_back(vec2(2.878049, 9.342828));
    vertexData_.push_back(vec2(2.292683, 9.131231));
    vertexData_.push_back(vec2(2.048780, 8.691760));
    vertexData_.push_back(vec2(1.707317, 8.528993));
    vertexData_.push_back(vec2(1.658537, 7.731434));
    vertexData_.push_back(vec2(0.878049, 7.047813));
    vertexData_.push_back(vec2(1.349594, 5.550356));
    vertexData_.push_back(vec2(0.569106, 5.029501));
    vertexData_.push_back(vec2(1.528455, 4.443540));
    vertexData_.push_back(vec2(0.991870, 3.434385));
    vertexData_.push_back(vec2(1.967480, 3.955239));
    vertexData_.push_back(vec2(1.772358, 2.994914));
    vertexData_.push_back(vec2(2.422764, 3.825025));
    vertexData_.push_back(vec2(2.829268, 3.092574));
    vertexData_.push_back(vec2(3.154472, 3.971516));
    vertexData_.push_back(vec2(3.512195, 3.727365));
    vertexData_.push_back(vec2(3.772358, 4.264496));
    vertexData_.push_back(vec2(4.130081, 4.524924));
    vertexData_.push_back(vec2(4.162601, 4.996948));
    vertexData_.push_back(vec2(4.699187, 5.403866));
    vertexData_.push_back(vec2(4.471545, 6.461852));
    vertexData_.push_back(vec2(5.219512, 7.243133));
    vertexData_.push_back(vec2(4.439024, 8.105799));
    vertexData_.push_back(vec2(5.235772, 8.756866));
    vertexData_.push_back(vec2(4.065041, 8.870804));
    vertexData_.push_back(vec2(4.991870, 9.391658));
    vertexData_.push_back(vec2(3.853658, 9.228891));
    vertexData_.push_back(vec2(4.390244, 9.912513));
    vertexData_.push_back(vec2(3.463415, 9.407935));
    vertexData_.push_back(vec2(3.674797, 9.912513));
    vertexData_.push_back(vec2(2.829268, 9.342828));
    vertexData_.push_back(vec2(2.959350, 9.603255));

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
    indexData_.push_back(18);
    indexData_.push_back(20);
    indexData_.push_back(22);
    indexData_.push_back(24);
    indexData_.push_back(26);
    indexData_.push_back(28);
    indexData_.push_back(30);
    indexData_.push_back(32);
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
    indexData_.push_back(9);
    indexData_.push_back(7);
    indexData_.push_back(5);
    indexData_.push_back(3);
    indexData_.push_back(1);

    // Primitive state so that the draw call can issue the primitives we want.
    unsigned int curOffset(0);
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 33, curOffset));
    curOffset += (33 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 33, curOffset));
}
