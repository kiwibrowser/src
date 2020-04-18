//
// Copyright © 2013 Linaro Limited
//
// This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
//
// glmark2 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// glmark2.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
//  Jesse Barker
//
#include "scene.h"
#include "util.h"

SceneClear::SceneClear(Canvas& canvas) :
    Scene(canvas, "clear")
{
}

bool
SceneClear::load()
{
    running_ = false;
    return true;
}

void
SceneClear::unload()
{
}

bool
SceneClear::setup()
{
    if (!Scene::setup())
        return false;

    // Add scene-specific setup code here

    // Set up the frame timing values and
    // indicate that the scene is ready to run.
    running_ = true;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;
    return true;
}

void
SceneClear::teardown()
{
    // Add scene-specific teardown code here
    Scene::teardown();
}

void
SceneClear::update()
{
    Scene::update();
    // Add scene-specific update code here
}

void
SceneClear::draw()
{
}

Scene::ValidationResult
SceneClear::validate()
{
    return Scene::ValidationUnknown;
}
