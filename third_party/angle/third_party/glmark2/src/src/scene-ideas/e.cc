/*
 * Vertex position data describing the letter 'e'
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

LetterE::LetterE()
{
    // Vertex data...
    vertexData_.push_back(vec2(1.095436, 6.190871));
    vertexData_.push_back(vec2(2.107884, 6.970954));
    vertexData_.push_back(vec2(2.556017, 7.020747));
    vertexData_.push_back(vec2(3.020747, 7.867220));
    vertexData_.push_back(vec2(3.518672, 8.033195));
    vertexData_.push_back(vec2(3.269710, 8.531120));
    vertexData_.push_back(vec2(4.165975, 8.929461));
    vertexData_.push_back(vec2(3.302905, 9.062241));
    vertexData_.push_back(vec2(4.331950, 9.626556));
    vertexData_.push_back(vec2(3.286307, 9.344398));
    vertexData_.push_back(vec2(4.116183, 9.958507));
    vertexData_.push_back(vec2(3.004149, 9.510373));
    vertexData_.push_back(vec2(3.518672, 9.991701));
    vertexData_.push_back(vec2(2.705394, 9.493776));
    vertexData_.push_back(vec2(2.091286, 9.311203));
    vertexData_.push_back(vec2(2.041494, 9.062241));
    vertexData_.push_back(vec2(1.178423, 8.514523));
    vertexData_.push_back(vec2(1.443983, 8.165976));
    vertexData_.push_back(vec2(0.481328, 7.535270));
    vertexData_.push_back(vec2(1.045643, 6.904564));
    vertexData_.push_back(vec2(0.149378, 6.091286));
    vertexData_.push_back(vec2(1.095436, 5.410789));
    vertexData_.push_back(vec2(0.464730, 4.232365));
    vertexData_.push_back(vec2(1.377593, 4.497925));
    vertexData_.push_back(vec2(1.261411, 3.136930));
    vertexData_.push_back(vec2(1.925311, 3.950207));
    vertexData_.push_back(vec2(2.240664, 3.037344));
    vertexData_.push_back(vec2(2.589212, 3.834025));
    vertexData_.push_back(vec2(3.087137, 3.269710));
    vertexData_.push_back(vec2(3.236515, 3.867220));
    vertexData_.push_back(vec2(3.684647, 3.867220));
    vertexData_.push_back(vec2(3.867220, 4.448133));
    vertexData_.push_back(vec2(4.398340, 5.128631));

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
