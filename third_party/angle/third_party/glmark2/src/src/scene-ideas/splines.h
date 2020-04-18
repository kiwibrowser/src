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
#ifndef SPLINES_H_
#define SPLINES_H_

#include <vector>
#include "vec.h"

class Spline
{
public:
    Spline() :
        paramData_(0) {}
    ~Spline()
    {
        delete [] paramData_;
    }
    void addControlPoint(const LibMatrix::vec3& point) { controlData_.push_back(point); }
    void getCurrentVec(float currentTime, LibMatrix::vec3& v) const;
    void calcParams();

private:
    std::vector<LibMatrix::vec3> controlData_;
    typedef LibMatrix::vec3 param[4];
    param* paramData_;
};

class ViewFromSpline : public Spline
{
public:
    ViewFromSpline();
    ~ViewFromSpline() {}
};

class ViewToSpline : public Spline
{
public:
    ViewToSpline();
    ~ViewToSpline() {}
};

class LightPositionSpline : public Spline
{
public:
    LightPositionSpline();
    ~LightPositionSpline() {}
};

class LogoPositionSpline : public Spline
{
public:
    LogoPositionSpline();
    ~LogoPositionSpline() {}
};

class LogoRotationSpline : public Spline
{
public:
    LogoRotationSpline();
    ~LogoRotationSpline() {}
};

#endif // SPLINES_H_
