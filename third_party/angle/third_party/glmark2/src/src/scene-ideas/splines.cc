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
#include "splines.h"

using LibMatrix::vec3;

void
Spline::calcParams()
{
    unsigned int numParams(controlData_.size() - 3);
    paramData_ = new param[numParams];
    for (unsigned int i = 0; i < numParams; i++)
    {
        float x3(controlData_[i + 1].x());
        float x2(controlData_[i + 2].x() - controlData_[i].x());
        float x1(2.0 * controlData_[i].x() +
	        -2.0 * controlData_[i+1].x() +
	         1.0 * controlData_[i+2].x() +
	        -1.0 * controlData_[i+3].x());
        float x0(-1.0 * controlData_[i].x() +
	          1.0 * controlData_[i+1].x() +
	         -1.0 * controlData_[i+2].x() +
	          1.0 * controlData_[i+3].x());
        float y3(controlData_[i + 1].y());
        float y2(controlData_[i + 2].y() - controlData_[i].y());
        float y1(2.0 * controlData_[i].y() +
	        -2.0 * controlData_[i+1].y() +
	         1.0 * controlData_[i+2].y() +
	        -1.0 * controlData_[i+3].y());
        float y0(-1.0 * controlData_[i].y() +
	          1.0 * controlData_[i+1].y() +
	         -1.0 * controlData_[i+2].y() +
	          1.0 * controlData_[i+3].y());
        float z3(controlData_[i + 1].z());
        float z2(controlData_[i + 2].z() - controlData_[i].z());
        float z1(2.0 * controlData_[i].z() +
	        -2.0 * controlData_[i+1].z() +
	         1.0 * controlData_[i+2].z() +
	        -1.0 * controlData_[i+3].z());
        float z0(-1.0 * controlData_[i].z() +
	          1.0 * controlData_[i+1].z() +
	         -1.0 * controlData_[i+2].z() +
	          1.0 * controlData_[i+3].z());
        paramData_[i][3].x(x3);
        paramData_[i][2].x(x2);
        paramData_[i][1].x(x1);
        paramData_[i][0].x(x0);
        paramData_[i][3].y(y3);
        paramData_[i][2].y(y2);
        paramData_[i][1].y(y1);
        paramData_[i][0].y(y0);
        paramData_[i][3].z(z3);
        paramData_[i][2].z(z2);
        paramData_[i][1].z(z1);
        paramData_[i][0].z(z0);
    }
}

void
Spline::getCurrentVec(float currentTime, vec3& v) const
{
    unsigned int integerTime(static_cast<unsigned int>(currentTime));
    float t(currentTime - static_cast<float>(integerTime));
    v.x(paramData_[integerTime][3].x() +
	paramData_[integerTime][2].x() * t +
	paramData_[integerTime][1].x() * t * t +
	paramData_[integerTime][0].x() * t * t * t);
    v.y(paramData_[integerTime][3].y() +
	paramData_[integerTime][2].y() * t +
	paramData_[integerTime][1].y() * t * t +
	paramData_[integerTime][0].y() * t * t * t);
    v.z(paramData_[integerTime][3].z() +
	paramData_[integerTime][2].z() * t +
	paramData_[integerTime][1].z() * t * t +
	paramData_[integerTime][0].z() * t * t * t);
}

ViewFromSpline::ViewFromSpline()
{
    addControlPoint(vec3(-1.0, 1.0, -4.0));
    addControlPoint(vec3(-1.0, -3.0, -4.0));
    addControlPoint(vec3(-3.0, 1.0, -3.0));
    addControlPoint(vec3(-1.8, 2.0, 5.4));
    addControlPoint(vec3(-0.4, 2.0, 1.2));
    addControlPoint(vec3(-0.2, 1.5, 0.6));
    addControlPoint(vec3(-0.2, 1.2, 0.6));
    addControlPoint(vec3(-0.8, 1.0, 2.4));
    addControlPoint(vec3(-1.0, 2.0, 3.0));
    addControlPoint(vec3(0.0, 4.0, 3.6));
    addControlPoint(vec3(-0.8, 4.0, 1.2));
    addControlPoint(vec3(-0.2, 3.0, 0.6));
    addControlPoint(vec3(-0.1, 2.0, 0.3));
    addControlPoint(vec3(-0.1, 2.0, 0.3));
    addControlPoint(vec3(-0.1, 2.0, 0.3));
    addControlPoint(vec3(-0.1, 2.0, 0.3));
    calcParams();
}

ViewToSpline::ViewToSpline()
{
    addControlPoint(vec3(-1.0, 1.0, 0.0));
    addControlPoint(vec3(-1.0, -3.0, 0.0));
    addControlPoint(vec3(-1.0, 1.0, 0.0));
    addControlPoint(vec3(0.1, 0.0, -0.3));
    addControlPoint(vec3(0.1, 0.0, -0.3));
    addControlPoint(vec3(0.1, 0.0, -0.3));
    addControlPoint(vec3(0.0, 0.2, 0.0));
    addControlPoint(vec3(0.0, 0.6, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    calcParams();
}

LightPositionSpline::LightPositionSpline()
{
    addControlPoint(vec3(0.0, 1.8, 0.0));
    addControlPoint(vec3(0.0, 1.8, 0.0));
    addControlPoint(vec3(0.0, 1.6, 0.0));
    addControlPoint(vec3(0.0, 1.6, 0.0));
    addControlPoint(vec3(0.0, 1.6, 0.0));
    addControlPoint(vec3(0.0, 1.6, 0.0));
    addControlPoint(vec3(0.0, 1.4, 0.0));
    addControlPoint(vec3(0.0, 1.3, 0.0));
    addControlPoint(vec3(-0.2, 1.5, 2.0));
    addControlPoint(vec3(0.8, 1.5, -0.4));
    addControlPoint(vec3(-0.8, 1.5, -0.4));
    addControlPoint(vec3(0.8, 2.0, 1.0));
    addControlPoint(vec3(1.8, 5.0, -1.8));
    addControlPoint(vec3(8.0, 10.0, -4.0));
    addControlPoint(vec3(8.0, 10.0, -4.0));
    addControlPoint(vec3(8.0, 10.0, -4.0));
    calcParams();
}

LogoPositionSpline::LogoPositionSpline()
{
    addControlPoint(vec3(0.0, -0.5, 0.0));
    addControlPoint(vec3(0.0, -0.5, 0.0));
    addControlPoint(vec3(0.0, -0.5, 0.0));
    addControlPoint(vec3(0.0, -0.5, 0.0));
    addControlPoint(vec3(0.0, -0.5, 0.0));
    addControlPoint(vec3(0.0, -0.5, 0.0));
    addControlPoint(vec3(0.0, 0.0, 0.0));
    addControlPoint(vec3(0.0, 0.6, 0.0));
    addControlPoint(vec3(0.0, 0.75, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.8, 0.0));
    addControlPoint(vec3(0.0, 0.5, 0.0));
    addControlPoint(vec3(0.0, 0.5, 0.0));
    addControlPoint(vec3(0.0, 0.5, 0.0));
    addControlPoint(vec3(0.0, 0.5, 0.0));
    addControlPoint(vec3(0.0, 0.5, 0.0));
    calcParams();
}

LogoRotationSpline::LogoRotationSpline()
{
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(0.0, 0.0, -18.4));
    addControlPoint(vec3(240.0, 360.0, 180.0));
    addControlPoint(vec3(90.0, 180.0, 90.0));
    addControlPoint(vec3(11.9, 0.0, -18.4));
    addControlPoint(vec3(11.9, 0.0, -18.4));
    addControlPoint(vec3(11.9, 0.0, -18.4));
    addControlPoint(vec3(11.9, 0.0, -18.4));
    addControlPoint(vec3(11.9, 0.0, -18.4));
    calcParams();
}
