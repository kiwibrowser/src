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
#include "benchmark.h"
#include "log.h"

bool
SceneDefaultOptions::setup()
{
    const std::map<std::string, Scene *> &scenes = Benchmark::scenes();

    for (std::list<std::pair<std::string, std::string> >::const_iterator iter = defaultOptions_.begin();
         iter != defaultOptions_.end();
         iter++)
    {
        for (std::map<std::string, Scene *>::const_iterator scene_iter = scenes.begin();
             scene_iter != scenes.end();
             scene_iter++)
        {
            Scene &scene(*(scene_iter->second));

            /* 
             * Display warning only if the option value is unsupported, not if
             * the scene doesn't support the option at all.
             */
            if (!scene.set_option_default(iter->first, iter->second) &&
                scene.options().find(iter->first) != scene.options().end())
            {
                Log::info("Warning: Scene '%s' doesn't accept default value '%s' for option '%s'\n",
                          scene.name().c_str(), iter->second.c_str(), iter->first.c_str());
            }
        }
    }

    return true;
}

bool
SceneDefaultOptions::set_option(const std::string &opt, const std::string &val)
{
    defaultOptions_.push_back(std::pair<std::string, std::string>(opt, val));
    return true;
}
