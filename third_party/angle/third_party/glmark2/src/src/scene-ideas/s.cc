/*
 * Vertex position data describing the letter 's'
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

LetterS::LetterS()
{
    // Vertex data...
    vertexData_.push_back(vec2(0.860393, 5.283798));
    vertexData_.push_back(vec2(0.529473, 3.550052));
    vertexData_.push_back(vec2(0.992761, 4.491228));
    vertexData_.push_back(vec2(0.910031, 3.368421));
    vertexData_.push_back(vec2(1.240951, 3.830753));
    vertexData_.push_back(vec2(1.456050, 3.104231));
    vertexData_.push_back(vec2(1.935884, 3.517028));
    vertexData_.push_back(vec2(2.002068, 2.988648));
    vertexData_.push_back(vec2(2.763185, 3.533540));
    vertexData_.push_back(vec2(3.061013, 3.120743));
    vertexData_.push_back(vec2(3.391934, 3.748194));
    vertexData_.push_back(vec2(4.053774, 3.632611));
    vertexData_.push_back(vec2(3.822130, 4.540764));
    vertexData_.push_back(vec2(4.550155, 4.590299));
    vertexData_.push_back(vec2(3.656670, 5.465428));
    vertexData_.push_back(vec2(4.517063, 5.713106));
    vertexData_.push_back(vec2(3.276112, 5.894737));
    vertexData_.push_back(vec2(3.921407, 6.538700));
    vertexData_.push_back(vec2(2.299896, 6.736842));
    vertexData_.push_back(vec2(3.044467, 7.430341));
    vertexData_.push_back(vec2(1.886246, 7.496388));
    vertexData_.push_back(vec2(2.581179, 8.222910));
    vertexData_.push_back(vec2(1.902792, 8.751290));
    vertexData_.push_back(vec2(2.680455, 8.883385));
    vertexData_.push_back(vec2(2.283350, 9.312694));
    vertexData_.push_back(vec2(3.358842, 9.609907));
    vertexData_.push_back(vec2(3.507756, 9.907121));
    vertexData_.push_back(vec2(4.285419, 9.758514));
    vertexData_.push_back(vec2(5.112720, 9.973168));
    vertexData_.push_back(vec2(4.748707, 9.593395));

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
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 30, curOffset));
    curOffset += (30 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 30, curOffset));
}
