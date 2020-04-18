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
#ifndef GLMARK2_GL_VISUAL_CONFIG_H_
#define GLMARK2_GL_VISUAL_CONFIG_H_

#include <string>

/**
 * Configuration parameters for a GL visual
 */
class GLVisualConfig
{
public:
    GLVisualConfig():
        red(1), green(1), blue(1), alpha(1), depth(1), stencil(0), buffer(1) {}
    GLVisualConfig(int r, int g, int b, int a, int d, int s, int buf):
        red(r), green(g), blue(b), alpha(a), depth(d), stencil(s), buffer(buf) {}
    GLVisualConfig(const std::string &s);

    /**
     * How well a GLVisualConfig matches another target config.
     *
     * The returned score has no meaning on its own. Its only purpose is
     * to allow comparison of how well different configs match a target
     * config, with a higher scores denoting a better match.
     *
     * Also note that this operation is not commutative:
     * a.match_score(b) != b.match_score(a)
     *
     * @return the match score
     */
    int match_score(const GLVisualConfig &target) const;

    int red;
    int green;
    int blue;
    int alpha;
    int depth;
    int stencil;
    int buffer;

private:
    int score_component(int component, int target, int scale) const;
};

#endif
