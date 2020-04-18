/*
 * Vertex position data describing the letter 'm'
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

LetterM::LetterM()
{
    // Vertex data...
    vertexData_.push_back(vec2(0.590769, 9.449335));
    vertexData_.push_back(vec2(2.116923, 9.842375));
    vertexData_.push_back(vec2(1.362051, 9.383828));
    vertexData_.push_back(vec2(2.527179, 9.825998));
    vertexData_.push_back(vec2(1.591795, 9.072672));
    vertexData_.push_back(vec2(2.789744, 9.514841));
    vertexData_.push_back(vec2(1.690256, 8.663255));
    vertexData_.push_back(vec2(2.658462, 8.335722));
    vertexData_.push_back(vec2(1.575385, 7.222108));
    vertexData_.push_back(vec2(2.067692, 6.255886));
    vertexData_.push_back(vec2(0.918974, 4.028659));
    vertexData_.push_back(vec2(1.050256, 3.013306));
    vertexData_.push_back(vec2(0.705641, 3.013306));
    vertexData_.push_back(vec2(2.018461, 6.386899));
    vertexData_.push_back(vec2(1.788718, 5.617196));
    vertexData_.push_back(vec2(2.921026, 7.991812));
    vertexData_.push_back(vec2(3.167180, 8.008188));
    vertexData_.push_back(vec2(3.544615, 8.827022));
    vertexData_.push_back(vec2(3.872821, 8.843398));
    vertexData_.push_back(vec2(4.414359, 9.547595));
    vertexData_.push_back(vec2(4.447179, 9.056294));
    vertexData_.push_back(vec2(5.120000, 9.891504));
    vertexData_.push_back(vec2(4.841026, 8.843398));
    vertexData_.push_back(vec2(5.825641, 9.809621));
    vertexData_.push_back(vec2(5.005128, 8.040941));
    vertexData_.push_back(vec2(5.989744, 8.761515));
    vertexData_.push_back(vec2(4.906667, 6.714432));
    vertexData_.push_back(vec2(5.595897, 7.123848));
    vertexData_.push_back(vec2(3.987692, 2.996929));
    vertexData_.push_back(vec2(4.348718, 2.996929));
    vertexData_.push_back(vec2(5.218462, 5.977482));
    vertexData_.push_back(vec2(5.251282, 6.354146));
    vertexData_.push_back(vec2(6.449231, 7.893552));
    vertexData_.push_back(vec2(6.400000, 8.221085));
    vertexData_.push_back(vec2(7.302564, 8.843398));
    vertexData_.push_back(vec2(7.351795, 9.334698));
    vertexData_.push_back(vec2(7.827693, 9.154554));
    vertexData_.push_back(vec2(8.008205, 9.842375));
    vertexData_.push_back(vec2(8.139487, 9.121801));
    vertexData_.push_back(vec2(8.795897, 9.973388));
    vertexData_.push_back(vec2(8.402051, 8.728762));
    vertexData_.push_back(vec2(9.337436, 9.531218));
    vertexData_.push_back(vec2(8.402051, 8.040941));
    vertexData_.push_back(vec2(9.288205, 8.433982));
    vertexData_.push_back(vec2(7.745641, 5.813715));
    vertexData_.push_back(vec2(8.320000, 5.928352));
    vertexData_.push_back(vec2(7.286154, 4.012282));
    vertexData_.push_back(vec2(7.991795, 4.126919));
    vertexData_.push_back(vec2(7.499487, 3.357216));
    vertexData_.push_back(vec2(8.533334, 3.766633));
    vertexData_.push_back(vec2(8.123077, 3.062436));
    vertexData_.push_back(vec2(8.927179, 3.832139));
    vertexData_.push_back(vec2(8.910769, 3.340839));
    vertexData_.push_back(vec2(9.550769, 4.126919));

    // Index data...
    indexData_.push_back(0);
    indexData_.push_back(2);
    indexData_.push_back(4);
    indexData_.push_back(6);
    indexData_.push_back(8);
    indexData_.push_back(10);
    indexData_.push_back(12);
    indexData_.push_back(11);
    indexData_.push_back(9);
    indexData_.push_back(7);
    indexData_.push_back(5);
    indexData_.push_back(3);
    indexData_.push_back(1);
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
    indexData_.push_back(30);
    indexData_.push_back(32);
    indexData_.push_back(34);
    indexData_.push_back(36);
    indexData_.push_back(38);
    indexData_.push_back(40);
    indexData_.push_back(42);
    indexData_.push_back(44);
    indexData_.push_back(46);
    indexData_.push_back(48);
    indexData_.push_back(50);
    indexData_.push_back(52);
    indexData_.push_back(53);
    indexData_.push_back(51);
    indexData_.push_back(49);
    indexData_.push_back(47);
    indexData_.push_back(45);
    indexData_.push_back(43);
    indexData_.push_back(41);
    indexData_.push_back(39);
    indexData_.push_back(37);
    indexData_.push_back(35);
    indexData_.push_back(33);
    indexData_.push_back(31);
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
    indexData_.push_back(45);
    indexData_.push_back(46);
    indexData_.push_back(47);
    indexData_.push_back(48);
    indexData_.push_back(49);
    indexData_.push_back(50);
    indexData_.push_back(51);
    indexData_.push_back(52);
    indexData_.push_back(53);

    // Primitive state so that the draw call can issue the primitives we want.
    unsigned int curOffset(0);
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 13, curOffset));
    curOffset += (13 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 17, curOffset));
    curOffset += (17 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_LINE_STRIP, 24, curOffset));
    curOffset += (24 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 13, curOffset));
    curOffset += (13 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 17, curOffset));
    curOffset += (17 * sizeof(unsigned short));
    primVec_.push_back(PrimitiveState(GL_TRIANGLE_STRIP, 24, curOffset));
}
