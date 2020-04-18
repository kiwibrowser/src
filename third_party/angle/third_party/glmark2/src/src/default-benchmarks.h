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
#ifndef GLMARK2_DEFAULT_BENCHMARKS_H_
#define GLMARK2_DEFAULT_BENCHMARKS_H_

#include <string>
#include <vector>

class DefaultBenchmarks
{
public:
    static const std::vector<std::string>& get()
    {
        static std::vector<std::string> default_benchmarks;

        if (default_benchmarks.empty())
            populate(default_benchmarks);

        return default_benchmarks;
    }

private:
    static void populate(std::vector<std::string>& benchmarks)
    {
        benchmarks.push_back("build:use-vbo=false");
        benchmarks.push_back("build:use-vbo=true");
        benchmarks.push_back("texture:texture-filter=nearest");
        benchmarks.push_back("texture:texture-filter=linear");
        benchmarks.push_back("texture:texture-filter=mipmap");
        benchmarks.push_back("shading:shading=gouraud");
        benchmarks.push_back("shading:shading=blinn-phong-inf");
        benchmarks.push_back("shading:shading=phong");
        benchmarks.push_back("shading:shading=cel");
        benchmarks.push_back("bump:bump-render=high-poly");
        benchmarks.push_back("bump:bump-render=normals");
        benchmarks.push_back("bump:bump-render=height");
        benchmarks.push_back("effect2d:kernel=0,1,0;1,-4,1;0,1,0;");
        benchmarks.push_back("effect2d:kernel=1,1,1,1,1;1,1,1,1,1;1,1,1,1,1;");
        benchmarks.push_back("pulsar:quads=5:texture=false:light=false");
        benchmarks.push_back("desktop:windows=4:effect=blur:blur-radius=5:passes=1:separable=true");
        benchmarks.push_back("desktop:windows=4:effect=shadow");
        benchmarks.push_back("buffer:update-fraction=0.5:update-dispersion=0.9:columns=200:update-method=map:interleave=false");
        benchmarks.push_back("buffer:update-fraction=0.5:update-dispersion=0.9:columns=200:update-method=subdata:interleave=false");
        benchmarks.push_back("buffer:update-fraction=0.5:update-dispersion=0.9:columns=200:update-method=map:interleave=true");
        benchmarks.push_back("ideas:speed=duration");
        benchmarks.push_back("jellyfish");
        benchmarks.push_back("terrain");
        benchmarks.push_back("shadow");
        benchmarks.push_back("refract");
        benchmarks.push_back("conditionals:vertex-steps=0:fragment-steps=0");
        benchmarks.push_back("conditionals:vertex-steps=0:fragment-steps=5");
        benchmarks.push_back("conditionals:vertex-steps=5:fragment-steps=0");
        benchmarks.push_back("function:fragment-steps=5:fragment-complexity=low");
        benchmarks.push_back("function:fragment-steps=5:fragment-complexity=medium");
        benchmarks.push_back("loop:vertex-steps=5:fragment-steps=5:fragment-loop=false");
        benchmarks.push_back("loop:vertex-steps=5:fragment-steps=5:fragment-uniform=false");
        benchmarks.push_back("loop:vertex-steps=5:fragment-steps=5:fragment-uniform=true");
    }
};

#endif
