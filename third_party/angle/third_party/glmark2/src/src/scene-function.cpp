/*
 * Copyright © 2010-2011 Linaro Limited
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
 *  Alexandros Frantzis (glmark2)
 */
#include <cmath>

#include "scene.h"
#include "mat.h"
#include "stack.h"
#include "vec.h"
#include "log.h"
#include "shader-source.h"
#include "util.h"

static const std::string shader_file_base(GLMARK_DATA_PATH"/shaders/function");

static const std::string vtx_file(shader_file_base + ".vert");
static const std::string frg_file(shader_file_base + ".frag");
static const std::string call_file(shader_file_base + "-call.all");
static const std::string step_low_file(shader_file_base + "-step-low.all");
static const std::string step_medium_file(shader_file_base + "-step-medium.all");

SceneFunction::SceneFunction(Canvas &pCanvas) :
    SceneGrid(pCanvas, "function")
{
    options_["fragment-steps"] = Scene::Option("fragment-steps", "1",
            "The number of computational steps in the fragment shader");
    options_["fragment-function"] = Scene::Option("fragment-function", "true",
            "Whether each computational step includes a function call", "false,true");
    options_["vertex-steps"] = Scene::Option("vertex-steps", "1",
            "The number of computational steps in the vertex shader");
    options_["vertex-function"] = Scene::Option("vertex-function", "true",
            "Whether each computational step includes an if-else clause", "false,true");
    options_["vertex-complexity"] = Scene::Option("vertex-complexity", "low",
            "The complexity of each computational step in the vertex shader", "low,medium");
    options_["fragment-complexity"] = Scene::Option("fragment-complexity", "low",
            "The complexity of each computational step in the fragment shader", "low,medium");
}

SceneFunction::~SceneFunction()
{
}

static std::string
get_vertex_shader_source(int steps, bool function, std::string &complexity)
{
    ShaderSource source(vtx_file);
    ShaderSource source_main;
    std::string step_file;

    if (complexity == "low")
        step_file = step_low_file;
    else if (complexity == "medium")
        step_file = step_medium_file;

    for (int i = 0; i < steps; i++) {
        if (function)
            source_main.append_file(call_file);
        else
            source_main.append_file(step_file);
    }

    if (function)
        source.replace_with_file("$PROCESS$", step_file);
    else
        source.replace("$PROCESS$", "");

    source.replace("$MAIN$", source_main.str());

    return source.str();
}

static std::string
get_fragment_shader_source(int steps, bool function, std::string &complexity)
{
    ShaderSource source(frg_file);
    ShaderSource source_main;
    std::string step_file;

    if (complexity == "low")
        step_file = step_low_file;
    else if (complexity == "medium")
        step_file = step_medium_file;

    for (int i = 0; i < steps; i++) {
        if (function)
            source_main.append_file(call_file);
        else
            source_main.append_file(step_file);
    }

    if (function)
        source.replace_with_file("$PROCESS$", step_file);
    else
        source.replace("$PROCESS$", "");

    source.replace("$MAIN$", source_main.str());

    return source.str();
}

bool
SceneFunction::setup()
{
    if (!SceneGrid::setup())
        return false;

    /* Parse options */
    bool vtx_function = options_["vertex-function"].value == "true";
    bool frg_function = options_["fragment-function"].value == "true";
    std::string vtx_complexity = options_["vertex-complexity"].value;
    std::string frg_complexity = options_["fragment-complexity"].value;
    int vtx_steps = Util::fromString<int>(options_["vertex-steps"].value);
    int frg_steps = Util::fromString<int>(options_["fragment-steps"].value);

    /* Load shaders */
    std::string vtx_shader(get_vertex_shader_source(vtx_steps, vtx_function,
                                                    vtx_complexity));
    std::string frg_shader(get_fragment_shader_source(frg_steps, frg_function,
                                                      frg_complexity));

    if (!Scene::load_shaders_from_strings(program_, vtx_shader, frg_shader))
        return false;

    program_.start();

    std::vector<GLint> attrib_locations;
    attrib_locations.push_back(program_["position"].location());
    mesh_.set_attrib_locations(attrib_locations);

    running_ = true;
    startTime_ = Util::get_timestamp_us() / 1000000.0;
    lastUpdateTime_ = startTime_;

    return true;
}

Scene::ValidationResult
SceneFunction::validate()
{
    static const double radius_3d(std::sqrt(3.0 * 15.0 * 15.0));

    int frg_steps = Util::fromString<int>(options_["fragment-steps"].value);

    Canvas::Pixel ref;

    if (frg_steps == 5)
        ref = Canvas::Pixel(0x5e, 0x5e, 0x5e, 0xff);
    else
        return Scene::ValidationUnknown;

    Canvas::Pixel pixel = canvas_.read_pixel(293, 89);

    double dist = pixel.distance_rgb(ref);
    if (dist < radius_3d + 0.01) {
        return Scene::ValidationSuccess;
    }
    else {
        Log::debug("Validation failed! Expected: 0x%x Actual: 0x%x Distance: %f\n",
                    ref.to_le32(), pixel.to_le32(), dist);
        return Scene::ValidationFailure;
    }

    return Scene::ValidationUnknown;
}
