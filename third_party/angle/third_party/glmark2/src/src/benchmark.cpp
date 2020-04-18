/*
 * Copyright © 2011 Linaro Limited
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

#include "benchmark.h"
#include "log.h"
#include "util.h"

using std::string;
using std::vector;
using std::map;

std::map<string, Scene *> Benchmark::sceneMap_;

static Scene &
get_scene_from_description(const string &s)
{
    vector<string> elems;

    Util::split(s, ':', elems, Util::SplitModeNormal);

    const string &name = !elems.empty() ? elems[0] : "";

    return Benchmark::get_scene_by_name(name);
}

static vector<Benchmark::OptionPair>
get_options_from_description(const string &s)
{
    vector<Benchmark::OptionPair> options;
    vector<string> elems;

    Util::split(s, ':', elems, Util::SplitModeNormal);

    for (vector<string>::const_iterator iter = elems.begin() + 1;
         iter != elems.end();
         iter++)
    {
        vector<string> opt;

        Util::split(*iter, '=', opt, Util::SplitModeNormal);
        if (opt.size() == 2)
            options.push_back(Benchmark::OptionPair(opt[0], opt[1]));
        else
            Log::info("Warning: ignoring invalid option string '%s' "
                      "in benchmark description\n",
                      iter->c_str());
    }

    return options;
}

void
Benchmark::register_scene(Scene &scene)
{
    sceneMap_[scene.name()] = &scene;
}

Scene &
Benchmark::get_scene_by_name(const string &name)
{
    map<string, Scene *>::const_iterator iter;

    if ((iter = sceneMap_.find(name)) != sceneMap_.end())
        return *(iter->second);
    else
        return Scene::dummy();
}

Benchmark::Benchmark(Scene &scene, const vector<OptionPair> &options) :
    scene_(scene), options_(options)
{
}

Benchmark::Benchmark(const string &name, const vector<OptionPair> &options) :
    scene_(Benchmark::get_scene_by_name(name)), options_(options)
{
}

Benchmark::Benchmark(const string &s) :
    scene_(get_scene_from_description(s)),
    options_(get_options_from_description(s))
{
}

Scene &
Benchmark::setup_scene()
{
    scene_.reset_options();
    load_options();

    scene_.load();
    scene_.setup();

    return scene_;
}

void
Benchmark::teardown_scene()
{
    scene_.teardown();
    scene_.unload();
}

bool
Benchmark::needs_decoration() const
{
    for (vector<OptionPair>::const_iterator iter = options_.begin();
         iter != options_.end();
         iter++)
    {
        if ((iter->first == "show-fps" && iter->second == "true") ||
            (iter->first == "title" && !iter->second.empty()))
        {
            return true;
        }
    }

    return false;
}

void
Benchmark::load_options()
{
    for (vector<OptionPair>::iterator iter = options_.begin();
         iter != options_.end();
         iter++)
    {
        if (!scene_.set_option(iter->first, iter->second)) {
            map<string, Scene::Option>::const_iterator opt_iter = scene_.options().find(iter->first);

            if (opt_iter == scene_.options().end()) {
                Log::info("Warning: Scene '%s' doesn't accept option '%s'\n",
                          scene_.name().c_str(), iter->first.c_str());
            }
            else {
                Log::info("Warning: Scene '%s' doesn't accept value '%s' for option '%s'\n",
                          scene_.name().c_str(), iter->second.c_str(), iter->first.c_str());
            }
        }
    }
}

