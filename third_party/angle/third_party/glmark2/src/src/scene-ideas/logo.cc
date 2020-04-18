/*
 * Vertex position data describing the old Silicon Graphics logo
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
#include "logo.h"
#include "scene.h"
#include "shader-source.h"
#include "log.h"

using std::string;
using LibMatrix::vec3;
using LibMatrix::uvec3;
using LibMatrix::vec4;
using LibMatrix::Stack4;
using LibMatrix::mat4;

const unsigned int SGILogo::textureResolution_(32);
const string SGILogo::modelviewName_("modelview");
const string SGILogo::projectionName_("projection");
const string SGILogo::lightPositionName_("light0Position");
const string SGILogo::logoColorName_("logoColor");
const string SGILogo::vertexAttribName_("vertex");
const string SGILogo::normalAttribName_("normal");
const string SGILogo::normalMatrixName_("normalMatrix");

SGILogo::SGILogo(void) :
    normalNormalIndex_(0),
    normalVertexIndex_(0),
    flatVertexIndex_(0),
    shadowVertexIndex_(0),
    vertexIndex_(0),
    valid_(false),
    drawStyle_(LOGO_NORMAL)
{
    // Single cylinder data...
    singleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    singleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 5.000000));
    singleCylinderVertices_.push_back(vec3(0.707107, 0.707107, 0.000000));
    singleCylinderVertices_.push_back(vec3(0.707107, 0.707107, 5.000000));
    singleCylinderVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    singleCylinderVertices_.push_back(vec3(0.000000, 1.000000, 5.000000));
    singleCylinderVertices_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    singleCylinderVertices_.push_back(vec3(-0.707107, 0.707107, 5.000000));
    singleCylinderVertices_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    singleCylinderVertices_.push_back(vec3(-1.000000, 0.000000, 5.000000));
    singleCylinderVertices_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    singleCylinderVertices_.push_back(vec3(-0.707107, -0.707107, 5.000000));
    singleCylinderVertices_.push_back(vec3(0.000000, -1.000000, 0.000000));
    singleCylinderVertices_.push_back(vec3(0.000000, -1.000000, 5.000000));
    singleCylinderVertices_.push_back(vec3(0.707107, -0.707107, 0.000000));
    singleCylinderVertices_.push_back(vec3(0.707107, -0.707107, 5.000000));
    singleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    singleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 5.000000));

    singleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.707107, 0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.707107, 0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.000000, 1.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.000000, 1.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.000000, -1.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.000000, -1.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.707107, -0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(0.707107, -0.707107, 0.000000));
    singleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    singleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));

    // Double cylinder data...
    doubleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    doubleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 7.000000));
    doubleCylinderVertices_.push_back(vec3(0.707107, 0.707107, 0.000000));
    doubleCylinderVertices_.push_back(vec3(0.707107, 0.707107, 7.000000));
    doubleCylinderVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    doubleCylinderVertices_.push_back(vec3(0.000000, 1.000000, 7.000000));
    doubleCylinderVertices_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    doubleCylinderVertices_.push_back(vec3(-0.707107, 0.707107, 7.000000));
    doubleCylinderVertices_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    doubleCylinderVertices_.push_back(vec3(-1.000000, 0.000000, 7.000000));
    doubleCylinderVertices_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    doubleCylinderVertices_.push_back(vec3(-0.707107, -0.707107, 7.000000));
    doubleCylinderVertices_.push_back(vec3(0.000000, -1.000000, 0.000000));
    doubleCylinderVertices_.push_back(vec3(0.000000, -1.000000, 7.000000));
    doubleCylinderVertices_.push_back(vec3(0.707107, -0.707107, 0.000000));
    doubleCylinderVertices_.push_back(vec3(0.707107, -0.707107, 7.000000));
    doubleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    doubleCylinderVertices_.push_back(vec3(1.000000, 0.000000, 7.000000));

    doubleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.707107, 0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.707107, 0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.000000, 1.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.000000, 1.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.000000, -1.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.000000, -1.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.707107, -0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(0.707107, -0.707107, 0.000000));
    doubleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    doubleCylinderNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));

    // Elbow data...
    elbowVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowVertices_.push_back(vec3(0.707107, 0.707107, 0.000000));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    elbowVertices_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    elbowVertices_.push_back(vec3(0.000000, -1.000000, 0.000000));
    elbowVertices_.push_back(vec3(0.707107, -0.707107, 0.000000));
    elbowVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowVertices_.push_back(vec3(1.000000, 0.034074, 0.258819));
    elbowVertices_.push_back(vec3(0.707107, 0.717087, 0.075806));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, 0.717087, 0.075806));
    elbowVertices_.push_back(vec3(-1.000000, 0.034074, 0.258819));
    elbowVertices_.push_back(vec3(-0.707107, -0.648939, 0.441832));
    elbowVertices_.push_back(vec3(0.000000, -0.931852, 0.517638));
    elbowVertices_.push_back(vec3(0.707107, -0.648939, 0.441832));
    elbowVertices_.push_back(vec3(1.000000, 0.034074, 0.258819));
    elbowVertices_.push_back(vec3(1.000000, 0.133975, 0.500000));
    elbowVertices_.push_back(vec3(0.707107, 0.746347, 0.146447));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, 0.746347, 0.146447));
    elbowVertices_.push_back(vec3(-1.000000, 0.133975, 0.500000));
    elbowVertices_.push_back(vec3(-0.707107, -0.478398, 0.853553));
    elbowVertices_.push_back(vec3(0.000000, -0.732051, 1.000000));
    elbowVertices_.push_back(vec3(0.707107, -0.478398, 0.853553));
    elbowVertices_.push_back(vec3(1.000000, 0.133975, 0.500000));
    elbowVertices_.push_back(vec3(1.000000, 0.292893, 0.707107));
    elbowVertices_.push_back(vec3(0.707107, 0.792893, 0.207107));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, 0.792893, 0.207107));
    elbowVertices_.push_back(vec3(-1.000000, 0.292893, 0.707107));
    elbowVertices_.push_back(vec3(-0.707107, -0.207107, 1.207107));
    elbowVertices_.push_back(vec3(0.000000, -0.414214, 1.414214));
    elbowVertices_.push_back(vec3(0.707107, -0.207107, 1.207107));
    elbowVertices_.push_back(vec3(1.000000, 0.292893, 0.707107));
    elbowVertices_.push_back(vec3(1.000000, 0.500000, 0.866025));
    elbowVertices_.push_back(vec3(0.707107, 0.853553, 0.253653));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, 0.853553, 0.253653));
    elbowVertices_.push_back(vec3(-1.000000, 0.500000, 0.866025));
    elbowVertices_.push_back(vec3(-0.707107, 0.146447, 1.478398));
    elbowVertices_.push_back(vec3(0.000000, 0.000000, 1.732051));
    elbowVertices_.push_back(vec3(0.707107, 0.146447, 1.478398));
    elbowVertices_.push_back(vec3(1.000000, 0.500000, 0.866025));
    elbowVertices_.push_back(vec3(1.000000, 0.741181, 0.965926));
    elbowVertices_.push_back(vec3(0.707107, 0.924194, 0.282913));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, 0.924194, 0.282913));
    elbowVertices_.push_back(vec3(-1.000000, 0.741181, 0.965926));
    elbowVertices_.push_back(vec3(-0.707107, 0.558168, 1.648939));
    elbowVertices_.push_back(vec3(0.000000, 0.482362, 1.931852));
    elbowVertices_.push_back(vec3(0.707107, 0.558168, 1.648939));
    elbowVertices_.push_back(vec3(1.000000, 0.741181, 0.965926));
    elbowVertices_.push_back(vec3(1.000000, 1.000000, 1.000000));
    elbowVertices_.push_back(vec3(0.707107, 1.000000, 0.292893));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowVertices_.push_back(vec3(-0.707107, 1.000000, 0.292893));
    elbowVertices_.push_back(vec3(-1.000000, 1.000000, 1.000000));
    elbowVertices_.push_back(vec3(-0.707107, 1.000000, 1.707107));
    elbowVertices_.push_back(vec3(0.000000, 1.000000, 2.000000));
    elbowVertices_.push_back(vec3(0.707107, 1.000000, 1.707107));
    elbowVertices_.push_back(vec3(1.000000, 1.000000, 1.000000));

    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.707107, 0.000000));
    elbowNormals_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    elbowNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    elbowNormals_.push_back(vec3(0.000000, -1.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, -0.707107, 0.000000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.683013, -0.183013));
    elbowNormals_.push_back(vec3(0.000000, 0.965926, -0.258819));
    elbowNormals_.push_back(vec3(-0.707107, 0.683013, -0.183013));
    elbowNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, -0.683013, 0.183013));
    elbowNormals_.push_back(vec3(0.000000, -0.965926, 0.258819));
    elbowNormals_.push_back(vec3(0.707107, -0.683013, 0.183013));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.612372, -0.353553));
    elbowNormals_.push_back(vec3(0.000000, 0.866025, -0.500000));
    elbowNormals_.push_back(vec3(-0.707107, 0.612372, -0.353553));
    elbowNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, -0.612372, 0.353553));
    elbowNormals_.push_back(vec3(0.000000, -0.866025, 0.500000));
    elbowNormals_.push_back(vec3(0.707107, -0.612372, 0.353553));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.500000, -0.500000));
    elbowNormals_.push_back(vec3(0.000000, 0.707107, -0.707107));
    elbowNormals_.push_back(vec3(-0.707107, 0.500000, -0.500000));
    elbowNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, -0.500000, 0.500000));
    elbowNormals_.push_back(vec3(0.000000, -0.707107, 0.707107));
    elbowNormals_.push_back(vec3(0.707107, -0.500000, 0.500000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.353553, -0.612372));
    elbowNormals_.push_back(vec3(0.000000, 0.500000, -0.866025));
    elbowNormals_.push_back(vec3(-0.707107, 0.353553, -0.612372));
    elbowNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, -0.353553, 0.612372));
    elbowNormals_.push_back(vec3(0.000000, -0.500000, 0.866025));
    elbowNormals_.push_back(vec3(0.707107, -0.353553, 0.612372));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.183013, -0.683013));
    elbowNormals_.push_back(vec3(0.000000, 0.258819, -0.965926));
    elbowNormals_.push_back(vec3(-0.707107, 0.183013, -0.683013));
    elbowNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, -0.183013, 0.683013));
    elbowNormals_.push_back(vec3(0.000000, -0.258819, 0.965926));
    elbowNormals_.push_back(vec3(0.707107, -0.183013, 0.683013));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.000000, -0.707107));
    elbowNormals_.push_back(vec3(0.000000, 0.000000, -1.000000));
    elbowNormals_.push_back(vec3(-0.707107, 0.000000, -0.707107));
    elbowNormals_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowNormals_.push_back(vec3(-0.707107, 0.000000, 0.707107));
    elbowNormals_.push_back(vec3(0.000000, 0.000000, 1.000000));
    elbowNormals_.push_back(vec3(0.707107, 0.000000, 0.707107));
    elbowNormals_.push_back(vec3(1.000000, 0.000000, 0.000000));

    elbowShadowVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.707107, 0.000000));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.707107, 0.000000));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, -0.707107, 0.000000));
    elbowShadowVertices_.push_back(vec3(0.000000, -1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(0.707107, -0.707107, 0.000000));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.019215, 0.195090));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.712735, 0.057141));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.712735, 0.057141));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.019215, 0.195090));
    elbowShadowVertices_.push_back(vec3(-0.707107, -0.674305, 0.333040));
    elbowShadowVertices_.push_back(vec3(0.000000, -0.961571, 0.390181));
    elbowShadowVertices_.push_back(vec3(0.707107, -0.674305, 0.333040));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.019215, 0.195090));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.076120, 0.382683));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.729402, 0.112085));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.729402, 0.112085));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.076120, 0.382683));
    elbowShadowVertices_.push_back(vec3(-0.707107, -0.577161, 0.653282));
    elbowShadowVertices_.push_back(vec3(0.000000, -0.847759, 0.765367));
    elbowShadowVertices_.push_back(vec3(0.707107, -0.577161, 0.653282));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.076120, 0.382683));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.168530, 0.555570));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.756468, 0.162723));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.756468, 0.162723));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.168530, 0.555570));
    elbowShadowVertices_.push_back(vec3(-0.707107, -0.419407, 0.948418));
    elbowShadowVertices_.push_back(vec3(0.000000, -0.662939, 1.111140));
    elbowShadowVertices_.push_back(vec3(0.707107, -0.419407, 0.948418));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.168530, 0.555570));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.292893, 0.707107));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.792893, 0.207107));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.792893, 0.207107));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.292893, 0.707107));
    elbowShadowVertices_.push_back(vec3(-0.707107, -0.207107, 1.207107));
    elbowShadowVertices_.push_back(vec3(0.000000, -0.414214, 1.414214));
    elbowShadowVertices_.push_back(vec3(0.707107, -0.207107, 1.207107));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.292893, 0.707107));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.444430, 0.831470));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.837277, 0.243532));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.837277, 0.243532));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.444430, 0.831470));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.051582, 1.419407));
    elbowShadowVertices_.push_back(vec3(0.000000, -0.111140, 1.662939));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.051582, 1.419407));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.444430, 0.831470));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.617317, 0.923880));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.887915, 0.270598));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.887915, 0.270598));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.617317, 0.923880));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.346719, 1.577161));
    elbowShadowVertices_.push_back(vec3(0.000000, 0.234633, 1.847759));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.346719, 1.577161));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.617317, 0.923880));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.804910, 0.980785));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.942859, 0.287265));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.942859, 0.287265));
    elbowShadowVertices_.push_back(vec3(-1.000000, 0.804910, 0.980785));
    elbowShadowVertices_.push_back(vec3(-0.707107, 0.666960, 1.674305));
    elbowShadowVertices_.push_back(vec3(0.000000, 0.609819, 1.961571));
    elbowShadowVertices_.push_back(vec3(0.707107, 0.666960, 1.674305));
    elbowShadowVertices_.push_back(vec3(1.000000, 0.804910, 0.980785));
    elbowShadowVertices_.push_back(vec3(1.000000, 1.000000, 1.000000));
    elbowShadowVertices_.push_back(vec3(0.707107, 1.000000, 0.292893));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 0.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 1.000000, 0.292893));
    elbowShadowVertices_.push_back(vec3(-1.000000, 1.000000, 1.000000));
    elbowShadowVertices_.push_back(vec3(-0.707107, 1.000000, 1.707107));
    elbowShadowVertices_.push_back(vec3(0.000000, 1.000000, 2.000000));
    elbowShadowVertices_.push_back(vec3(0.707107, 1.000000, 1.707107));
    elbowShadowVertices_.push_back(vec3(1.000000, 1.000000, 1.000000));

    // Now that we've setup the vertex data, we can setup the map of how
    // that data will be laid out in the buffer object.
    static const unsigned int sv3(sizeof(vec3));
    dataMap_.scvOffset = 0;
    dataMap_.scvSize = singleCylinderVertices_.size() * sv3;
    dataMap_.totalSize = dataMap_.scvSize;
    dataMap_.scnOffset = dataMap_.scvOffset + dataMap_.scvSize;
    dataMap_.scnSize = singleCylinderNormals_.size() * sv3;
    dataMap_.totalSize += dataMap_.scnSize;
    dataMap_.dcvOffset = dataMap_.scnOffset + dataMap_.scnSize;
    dataMap_.dcvSize = doubleCylinderVertices_.size() * sv3;
    dataMap_.totalSize += dataMap_.dcvSize;
    dataMap_.dcnOffset = dataMap_.dcvOffset + dataMap_.dcvSize;
    dataMap_.dcnSize = doubleCylinderNormals_.size() * sv3;
    dataMap_.totalSize += dataMap_.dcnSize;
    dataMap_.evOffset = dataMap_.dcnOffset + dataMap_.dcnSize;
    dataMap_.evSize = elbowVertices_.size() * sv3;
    dataMap_.totalSize += dataMap_.evSize;
    dataMap_.enOffset = dataMap_.evOffset + dataMap_.evSize;
    dataMap_.enSize = elbowNormals_.size() * sv3;
    dataMap_.totalSize += dataMap_.enSize;
    dataMap_.esvOffset = dataMap_.enOffset + dataMap_.enSize;
    dataMap_.esvSize = elbowShadowVertices_.size() * sv3;
    dataMap_.totalSize += dataMap_.esvSize;

    //
    // The original implementation of both the logo and the lamp represented
    // the vertex and normal data in a triply-dimensioned array of floats and
    // all of the calls referenced double-indexed arrays of vector data.
    // To my mind, this made the code look clunky and overly verbose.
    // Representing the data as a STL vector of vec3 (itself a 3-float vector
    // quantity) provides both an efficient container and allows for more
    // concise looking code.  The slightly goofy loops (using the original 2
    // dimensional indices to compute a single offset into the STL vector) are 
    // a compromise to avoid rearranging the original data.
    //
    // - jesse 2010/10/04
    //
    for (unsigned int i = 0; i < 8; i++)
    {
        for (unsigned int j = 0; j < 9; j++)
        {
            unsigned int index1(i * 9 + j);
            unsigned int index2((i + 1) * 9 + j);
            indexData_.push_back(index1);
            indexData_.push_back(index2);
        }
    }

    // Initialize the stipple pattern
    static const unsigned int patterns[] = { 0xaaaaaaaa, 0x55555555 };
    for (unsigned int i = 0; i < textureResolution_; i++)
    {
        for (unsigned int j = 0; j < textureResolution_; j++)
        {
            // Alternate the pattern every other line.
            unsigned int curMask(1 << j);
            unsigned int curPattern(patterns[i % 2]);
            textureImage_[i][j] = ((curPattern & curMask) >> j) * 255;
        }
    }
}

SGILogo::~SGILogo()
{
    if (valid_)
    {
        glDeleteBuffers(2, &bufferObjects_[0]);
    }
}

void
SGILogo::init()
{
    // Make sure we don't re-initialize...
    if (valid_)
    {
        return;
    }

    // Initialize shader sources from input files and create programs from them
    // The program for handling the main object with lighting...
    string logo_vtx_filename(GLMARK_DATA_PATH"/shaders/ideas-logo.vert");
    string logo_frg_filename(GLMARK_DATA_PATH"/shaders/ideas-logo.frag");
    ShaderSource logo_vtx_source(logo_vtx_filename);
    ShaderSource logo_frg_source(logo_frg_filename);
    if (!Scene::load_shaders_from_strings(normalProgram_, logo_vtx_source.str(),
                                          logo_frg_source.str()))
    {
        Log::error("No valid program for normal logo rendering\n");
        return;
    }
    normalVertexIndex_ = normalProgram_[vertexAttribName_].location();
    normalNormalIndex_ = normalProgram_[normalAttribName_].location();

    // The program for handling the flat object...
    string logo_flat_vtx_filename(GLMARK_DATA_PATH"/shaders/ideas-logo-flat.vert");
    string logo_flat_frg_filename(GLMARK_DATA_PATH"/shaders/ideas-logo-flat.frag");
    ShaderSource logo_flat_vtx_source(logo_flat_vtx_filename);
    ShaderSource logo_flat_frg_source(logo_flat_frg_filename);
    if (!Scene::load_shaders_from_strings(flatProgram_, logo_flat_vtx_source.str(),
                                          logo_flat_frg_source.str()))
    {
        Log::error("No valid program for flat logo rendering\n");
        return;
    }
    flatVertexIndex_ = flatProgram_[vertexAttribName_].location();

    // The program for handling the shadow object with texturing...
    string logo_shadow_vtx_filename(GLMARK_DATA_PATH"/shaders/ideas-logo-shadow.vert");
    string logo_shadow_frg_filename(GLMARK_DATA_PATH"/shaders/ideas-logo-shadow.frag");
    ShaderSource logo_shadow_vtx_source(logo_shadow_vtx_filename);
    ShaderSource logo_shadow_frg_source(logo_shadow_frg_filename);
    if (!Scene::load_shaders_from_strings(shadowProgram_, logo_shadow_vtx_source.str(),
                                          logo_shadow_frg_source.str()))
    {
        Log::error("No valid program for shadow logo rendering\n");
        return;
    }
    shadowVertexIndex_ = shadowProgram_[vertexAttribName_].location();

    // We need 2 buffers for our work here.  One for the vertex data.
    // and one for the index data.
    glGenBuffers(2, &bufferObjects_[0]);

    // First, setup the vertex data by binding the first buffer object, 
    // allocating its data store, and filling it in with our vertex data.
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
    glBufferData(GL_ARRAY_BUFFER, dataMap_.totalSize, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.scvOffset, dataMap_.scvSize,
                    &singleCylinderVertices_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.scnOffset, dataMap_.scnSize,
                    &singleCylinderNormals_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.dcvOffset, dataMap_.dcvSize,
                    &doubleCylinderVertices_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.dcnOffset, dataMap_.dcnSize,
                    &doubleCylinderNormals_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.evOffset, dataMap_.evSize,
                    &elbowVertices_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.enOffset, dataMap_.enSize, 
                    &elbowNormals_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.esvOffset, dataMap_.esvSize, 
                    &elbowShadowVertices_.front());

    // Now repeat for our index data.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData_.size() * sizeof(unsigned short),
                 &indexData_.front(), GL_STATIC_DRAW);

    // Setup our the texture that the shadow program will use...
    glGenTextures(1, &textureName_);
    glBindTexture(GL_TEXTURE_2D, textureName_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
                 textureResolution_, textureResolution_,
                 0, GL_ALPHA, GL_UNSIGNED_BYTE, textureImage_);

    // We're ready to go.
    valid_ = true;
}

void
SGILogo::bendForward(Stack4& ms)
{
    ms.translate(0.0, 1.0, 0.0);
    ms.rotate(90.0, 1.0, 0.0, 0.0);
    ms.translate(0.0, -1.0, 0.0);
}

void
SGILogo::bendLeft(Stack4& ms)
{
    ms.rotate(-90.0, 0.0, 0.0, 1.0);
    ms.translate(0.0, 1.0, 0.0);
    ms.rotate(90.0, 1.0, 0.0, 0.0);
    ms.translate(0.0, -1.0, 0.0);
}

void
SGILogo::bendRight(Stack4& ms) 
{
    ms.rotate(90.0, 0.0, 0.0, 1.0);
    ms.translate(0.0, 1.0, 0.0);
    ms.rotate(90.0, 1.0, 0.0, 0.0);
    ms.translate(0.0, -1.0, 0.0);
}

void
SGILogo::drawDoubleCylinder(void)
{
    glVertexAttribPointer(vertexIndex_, 3, GL_FLOAT, GL_FALSE, 0, 
        reinterpret_cast<const GLvoid*>(dataMap_.dcvOffset));
    if (drawStyle_ == LOGO_NORMAL)
    {
        glVertexAttribPointer(normalNormalIndex_, 3, GL_FLOAT, GL_FALSE, 0, 
            reinterpret_cast<const GLvoid*>(dataMap_.dcnOffset));
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 18);
}

void
SGILogo::drawSingleCylinder(void)
{
    glVertexAttribPointer(vertexIndex_, 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.scvOffset));
    if (drawStyle_ == LOGO_NORMAL)
    {
        glVertexAttribPointer(normalNormalIndex_, 3, GL_FLOAT, GL_FALSE, 0, 
            reinterpret_cast<const GLvoid*>(dataMap_.scnOffset));
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 18);
}

void
SGILogo::drawElbow(void)
{
    unsigned int startIdx(0);
    unsigned int endIdx(6);
    if (drawStyle_ == LOGO_NORMAL)
    {
        glVertexAttribPointer(vertexIndex_, 3, GL_FLOAT, GL_FALSE, 0,
            reinterpret_cast<const GLvoid*>(dataMap_.evOffset));
        glVertexAttribPointer(normalNormalIndex_, 3, GL_FLOAT, GL_FALSE, 0,
            reinterpret_cast<const GLvoid*>(dataMap_.enOffset));
    }
    else
    {
        endIdx = 8;
        glVertexAttribPointer(vertexIndex_, 3, GL_FLOAT, GL_FALSE, 0,
            reinterpret_cast<const GLvoid*>(dataMap_.esvOffset));
    }

    for (unsigned int i = startIdx; i < endIdx; i++)
    {
        unsigned int curOffset(i * 18 * sizeof(unsigned short));
        glDrawElements(GL_TRIANGLE_STRIP, 18, GL_UNSIGNED_SHORT, 
             reinterpret_cast<const GLvoid*>(curOffset));
    }
}

// Generate a normal matrix from a modelview matrix
//
// Since we can't universally handle the normal matrix inside the
// vertex shader (inverse() and transpose() built-ins not supported by
// GLSL ES, for example), we'll generate it here, and load it as a
// uniform.
void
SGILogo::updateXform(const mat4& mv, Program& program)
{
    if (drawStyle_ == LOGO_NORMAL)
    {
        LibMatrix::mat3 normalMatrix(mv[0][0], mv[1][0], mv[2][0],
                                     mv[0][1], mv[1][1], mv[2][1],
                                     mv[0][2], mv[1][2], mv[2][2]);
        normalMatrix.transpose().inverse();
        program[normalMatrixName_] = normalMatrix;
    }
    program[modelviewName_] = mv;
}

Program&
SGILogo::getProgram()
{
    switch (drawStyle_)
    {
        case LOGO_NORMAL:
            return normalProgram_;
            break;
        case LOGO_FLAT:
            return flatProgram_;
            break;
        case LOGO_SHADOW:
            return shadowProgram_;
            break;            
    }

    return normalProgram_;
}

void
SGILogo::draw(Stack4& modelview, 
    Stack4& projection, 
    const vec4& lightPosition,
    DrawStyle style,
    const uvec3& currentColor)
{
    if (!valid_)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);

    // Setup the program to use based upon draw style and set it running.
    drawStyle_ = style;
    vec4 logoColor(currentColor.x() / 255.0, currentColor.y() / 255.0, currentColor.z() / 255.0, 1.0);
    Program& curProgram = getProgram();
    curProgram.start();
    switch (drawStyle_)
    {
        case LOGO_NORMAL:
            curProgram[lightPositionName_] = lightPosition;
            vertexIndex_ = normalVertexIndex_;
            glEnableVertexAttribArray(normalNormalIndex_);
            break;
        case LOGO_FLAT:
            curProgram[logoColorName_] = logoColor;
            vertexIndex_ = flatVertexIndex_;
            break;
        case LOGO_SHADOW:
            vertexIndex_ = shadowVertexIndex_;
            break;            
    }

    glEnableVertexAttribArray(vertexIndex_);
    curProgram[projectionName_] = projection.getCurrent();
    modelview.translate(5.500000, -3.500000, 4.500000);
    modelview.translate(0.0,  0.0,  -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -5.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawSingleCylinder();
    bendRight(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -5.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawSingleCylinder();
    bendLeft(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -5.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawSingleCylinder();
    bendRight(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -5.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawSingleCylinder();
    bendLeft(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -5.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawSingleCylinder();
    bendRight(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -7.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawDoubleCylinder();
    bendForward(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    modelview.translate(0.0, 0.0, -5.000000);
    updateXform(modelview.getCurrent(), curProgram);
    drawSingleCylinder();
    bendLeft(modelview);
    updateXform(modelview.getCurrent(), curProgram);
    drawElbow();
    glDisableVertexAttribArray(vertexIndex_);
    switch (drawStyle_)
    {
        case LOGO_NORMAL:
            glDisableVertexAttribArray(normalNormalIndex_);
            break;
        case LOGO_FLAT:
            break;
        case LOGO_SHADOW:
            break;            
    }
    curProgram.stop();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
