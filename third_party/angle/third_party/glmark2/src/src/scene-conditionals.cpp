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
#include "scene.h"
#include "mat.h"
#include "stack.h"
#include "vec.h"
#include "log.h"
#include "shader-source.h"
#include "util.h"

#include <cmath>

static const std::string shader_file_base(GLMARK_DATA_PATH"/shaders/conditionals");

static const std::string vtx_file(shader_file_base + ".vert");
static const std::string frg_file(shader_file_base + ".frag");
static const std::string step_conditional_file(shader_file_base + "-step-conditional.all");
static const std::string step_simple_file(shader_file_base + "-step-simple.all");

SceneConditionals::SceneConditionals(Canvas &pCanvas) :
    SceneGrid(pCanvas, "conditionals")
{
    options_["fragment-steps"] = Scene::Option("fragment-steps", "1",
            "The number of computational steps in the fragment shader");
    options_["fragment-conditionals"] = Scene::Option("fragment-conditionals", "true",
            "Whether each computational step includes an if-else clause", "false,true");
    options_["vertex-steps"] = Scene::Option("vertex-steps", "1",
            "The number of computational steps in the vertex shader");
    options_["vertex-conditionals"] = Scene::Option("vertex-conditionals", "true",
            "Whether each computational step includes an if-else clause", "false,true");
}

SceneConditionals::~SceneConditionals()
{
}

static std::string
get_vertex_shader_source(int steps, bool conditionals)
{
    ShaderSource source(vtx_file);
    ShaderSource source_main;

    for (int i = 0; i < steps; i++) {
        if (conditionals)
            source_main.append_file(step_conditional_file);
        else
            source_main.append_file(step_simple_file);
    }

    source.replace("$MAIN$", source_main.str());

    return source.str();
}

static std::string
get_fragment_shader_source(int steps, bool conditionals)
{
    ShaderSource source(frg_file);
    ShaderSource source_main;

    for (int i = 0; i < steps; i++) {
        if (conditionals)
            source_main.append_file(step_conditional_file);
        else
            source_main.append_file(step_simple_file);
    }

    source.replace("$MAIN$", source_main.str());

    return source.str();
}

bool
SceneConditionals::setup()
{
    if (!SceneGrid::setup())
        return false;

    /* Parse options */
    bool vtx_conditionals = options_["vertex-conditionals"].value == "true";
    bool frg_conditionals = options_["fragment-conditionals"].value == "true";
    int vtx_steps(Util::fromString<int>(options_["vertex-steps"].value));
    int frg_steps(Util::fromString<int>(options_["fragment-steps"].value));
    /* Load shaders */
    std::string vtx_shader(get_vertex_shader_source(vtx_steps, vtx_conditionals));
    std::string frg_shader(get_fragment_shader_source(frg_steps, frg_conditionals));

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
SceneConditionals::validate()
{
    static const double radius_3d(std::sqrt(3.0 * 5.0 * 5.0));

    bool frg_conditionals = options_["fragment-conditionals"].value == "true";
    int frg_steps(Util::fromString<int>(options_["fragment-steps"].value));

    if (!frg_conditionals)
        return Scene::ValidationUnknown;

    Canvas::Pixel ref;

    if (frg_steps == 0)
        ref = Canvas::Pixel(0xa0, 0xa0, 0xa0, 0xff);
    else if (frg_steps == 5)
        ref = Canvas::Pixel(0x25, 0x25, 0x25, 0xff);
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
