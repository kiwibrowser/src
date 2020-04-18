/*
 * Copyright © 2011 Linaro Limited
 *
 * This file is part of glcompbench.
 *
 * glcompbench is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glcompbench is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glcompbench.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Alexandros Frantzis <alexandros.frantzis@linaro.org>
 *  Jesse Barker <jesse.barker@linaro.org>
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <string>
#include <vector>
#include "gl-visual-config.h"

struct Options {
    enum FrameEnd {
        FrameEndDefault,
        FrameEndNone,
        FrameEndSwap,
        FrameEndFinish,
        FrameEndReadPixels
    };

    static bool parse_args(int argc, char **argv);
    static void print_help();

    static std::vector<std::string> benchmarks;
    static std::vector<std::string> benchmark_files;
    static bool validate;
    static FrameEnd frame_end;
    static std::pair<int,int> size;
    static bool list_scenes;
    static bool show_all_options;
    static bool show_debug;
    static bool show_help;
    static bool reuse_context;
    static bool run_forever;
    static bool annotate;
    static bool offscreen;
    static GLVisualConfig visual_config;
};

#endif /* OPTIONS_H_ */
