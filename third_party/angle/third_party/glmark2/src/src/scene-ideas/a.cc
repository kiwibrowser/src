/*
 * Vertex position data describing the letter 'a'
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

LetterA::LetterA()
{
    // Vertex data...
    vertexData_.push_back(vec2(5.618949, 10.261048));
    vertexData_.push_back(vec2(5.322348, 9.438848));
    vertexData_.push_back(vec2(5.124614, 10.030832));
    vertexData_.push_back(vec2(4.860968, 9.488181));
    vertexData_.push_back(vec2(4.811534, 9.932169));
    vertexData_.push_back(vec2(3.938208, 9.438848));
    vertexData_.push_back(vec2(3.658084, 9.685509));
    vertexData_.push_back(vec2(2.784758, 8.994862));
    vertexData_.push_back(vec2(2.801236, 9.175745));
    vertexData_.push_back(vec2(1.960865, 8.172662));
    vertexData_.push_back(vec2(1.186406, 7.761562));
    vertexData_.push_back(vec2(1.252317, 6.561151));
    vertexData_.push_back(vec2(0.576725, 6.610483));
    vertexData_.push_back(vec2(0.939238, 5.525180));
    vertexData_.push_back(vec2(0.164779, 4.883864));
    vertexData_.push_back(vec2(0.840371, 4.818089));
    vertexData_.push_back(vec2(0.230690, 3.963001));
    vertexData_.push_back(vec2(0.939238, 4.242549));
    vertexData_.push_back(vec2(0.609681, 3.255909));
    vertexData_.push_back(vec2(1.268795, 3.963001));
    vertexData_.push_back(vec2(1.021627, 3.075026));
    vertexData_.push_back(vec2(1.861998, 4.045221));
    vertexData_.push_back(vec2(1.829042, 3.535457));
    vertexData_.push_back(vec2(2.817714, 4.818089));
    vertexData_.push_back(vec2(3.163749, 4.998972));
    vertexData_.push_back(vec2(3.971164, 6.643371));
    vertexData_.push_back(vec2(4.267765, 6.725591));
    vertexData_.push_back(vec2(4.663234, 7.630010));
    vertexData_.push_back(vec2(5.404737, 9.734840));
    vertexData_.push_back(vec2(4.646756, 9.669065));
    vertexData_.push_back(vec2(5.108136, 8.731757));
    vertexData_.push_back(vec2(4.679712, 8.600205));
    vertexData_.push_back(vec2(4.926879, 7.564234));
    vertexData_.push_back(vec2(4.366632, 6.692703));
    vertexData_.push_back(vec2(4.663234, 5.344296));
    vertexData_.push_back(vec2(3.888774, 4.850976));
    vertexData_.push_back(vec2(4.630278, 4.094553));
    vertexData_.push_back(vec2(3.954686, 3.963001));
    vertexData_.push_back(vec2(4.828012, 3.798561));
    vertexData_.push_back(vec2(4.168898, 3.321686));
    vertexData_.push_back(vec2(5.157569, 3.864337));
    vertexData_.push_back(vec2(4.514933, 3.091470));
    vertexData_.push_back(vec2(5.553038, 4.045221));
    vertexData_.push_back(vec2(5.305870, 3.634121));
    vertexData_.push_back(vec2(5.932029, 4.176773));

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
    indexData_.push_back(34);
    indexData_.push_back(35);
    indexData_.push_back(36);
    indexData_.push_back(37);
    indexData_.push_back(38);
    indexData_.push_back(39);
    indexData_.push_back(40);
    indexData_.push_back(41);
    indexData_.push_back(42);
    indexData_.push_back(43);
    indexData_.push_back(44);
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
    indexData_.push_back(28);
    indexData_.push_back(30);
    indexData_.push_back(32);
    indexData_.push_back(34);
    indexData_.push_back(36);
    indexData_.push_back(38);
    indexData_.push_back(40);
    indexData_.push_back(42);
    indexData_.push_back(44);
    indexData_.push_back(43);
    indexData_.push_back(41);
    indexData_.push_back(39);
    indexData_.push_back(37);
    indexData_.push_back(35);
    indexData_.push_back(33);
    indexData_.push_back(31);
    indexData_.push_back(29);

    // Primitive state so that the draw call can issue the primitives we want.
    unsigned int curOffset(0);
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 28, curOffset));
    curOffset += (28 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 17, curOffset));
    curOffset += (17 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 28, curOffset));
    curOffset += (28 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 17, curOffset));
}
