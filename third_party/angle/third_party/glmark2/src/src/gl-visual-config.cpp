/*
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
 *  Alexandros Frantzis <alexandros.frantzis@linaro.org>
 */
#include "gl-visual-config.h"
#include "util.h"
#include "log.h"

#include <vector>

GLVisualConfig::GLVisualConfig(const std::string &s) :
    red(1), green(1), blue(1), alpha(1), depth(1), stencil(0), buffer(1)
{
    std::vector<std::string> elems;

    Util::split(s, ':', elems, Util::SplitModeNormal);

    for (std::vector<std::string>::const_iterator iter = elems.begin();
         iter != elems.end();
         iter++)
    {
        std::vector<std::string> opt;

        Util::split(*iter, '=', opt, Util::SplitModeNormal);
        if (opt.size() == 2) {
            if (opt[0] == "r" || opt[0] == "red")
                red = Util::fromString<int>(opt[1]);
            else if (opt[0] == "g" || opt[0] == "green")
                green = Util::fromString<int>(opt[1]);
            else if (opt[0] == "b" || opt[0] == "blue")
                blue = Util::fromString<int>(opt[1]);
            else if (opt[0] == "a" || opt[0] == "alpha")
                alpha = Util::fromString<int>(opt[1]);
            else if (opt[0] == "d" || opt[0] == "depth")
                depth = Util::fromString<int>(opt[1]);
            else if (opt[0] == "s" || opt[0] == "stencil")
                stencil = Util::fromString<int>(opt[1]);
            else if (opt[0] == "buf" || opt[0] == "buffer")
                buffer = Util::fromString<int>(opt[1]);
        }
        else
            Log::info("Warning: ignoring invalid option string '%s' "
                      "in GLVisualConfig description\n",
                      iter->c_str());
    }
}

int
GLVisualConfig::match_score(const GLVisualConfig &target) const
{
    int score(0);

    /* 
     * R,G,B,A integer values are at most 8 bits wide (for current widespread
     * hardware), so we need to scale them by 4 to get them in the [0,32] range.
     */
    score += score_component(red, target.red, 4);
    score += score_component(green, target.green, 4);
    score += score_component(blue, target.blue, 4);
    score += score_component(alpha, target.alpha, 4);
    score += score_component(depth, target.depth, 1);
    score += score_component(stencil, target.stencil, 0);
    score += score_component(buffer, target.buffer, 1);

    return score;
}

int
GLVisualConfig::score_component(int component, int target, int scale) const
{
    /* 
     * The maximum (positive) score that can be returned is based
     * on the maximum bit width of the components. We assume this to
     * be 32 bits, which is a reasonable assumption for current platforms.
     */
    static const int MAXIMUM_COMPONENT_SCORE = 32;
    static const int UNACCEPTABLE_COMPONENT_PENALTY = -1000;
    int score(0);

    if ((component > 0 && target == 0) ||
        (component == 0 && target > 0))
    {
        /* 
         * Penalize components that are not present but have been requested,
         * and components that have been excluded but are present.
         */
        score = UNACCEPTABLE_COMPONENT_PENALTY;
    }
    else if (component == target)
    {
        /* Reward exact matches with the maximum per component score */
        score = MAXIMUM_COMPONENT_SCORE;
    }
    else
    {
        /* 
         * Reward deeper than requested component values, penalize shallower
         * than requested component values. Because the ranges of component
         * values vary we use a scaling factor to even them out, so that the
         * score for all components ranges from [0,MAXIMUM_COMPONENT_SCORE).
         */
        score = scale * (component - target);
    }

    return score;
}
