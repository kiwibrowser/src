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
 *  Alexandros Frantzis
 */
package org.linaro.glmark2;

/** 
 * Class that holds a configuration of a GL visual.
 */
class GLVisualConfig {
    public GLVisualConfig() {}
    public GLVisualConfig(int r, int g, int b, int a, int d, int s, int buf) {
        red = r;
        green = g;
        blue = b;
        alpha = a;
        depth = d;
        stencil = s;
        buffer = buf;
    }

    public int red;
    public int green;
    public int blue;
    public int alpha;
    public int depth;
    public int stencil;
    public int buffer;
}
