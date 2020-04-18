/*
 * Copyright © 2011-2012 Linaro Limited
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

#include <cstring>
#include <cstdio>
#include <getopt.h>

#include "options.h"
#include "util.h"

std::vector<std::string> Options::benchmarks;
std::vector<std::string> Options::benchmark_files;
bool Options::validate = false;
Options::FrameEnd Options::frame_end = Options::FrameEndDefault;
std::pair<int,int> Options::size(800, 600);
bool Options::list_scenes = false;
bool Options::show_all_options = false;
bool Options::show_debug = false;
bool Options::show_help = false;
bool Options::reuse_context = false;
bool Options::run_forever = false;
bool Options::annotate = false;
bool Options::offscreen = false;
GLVisualConfig Options::visual_config;

static struct option long_options[] = {
    {"annotate", 0, 0, 0},
    {"benchmark", 1, 0, 0},
    {"benchmark-file", 1, 0, 0},
    {"validate", 0, 0, 0},
    {"frame-end", 1, 0, 0},
    {"off-screen", 0, 0, 0},
    {"visual-config", 1, 0, 0},
    {"reuse-context", 0, 0, 0},
    {"run-forever", 0, 0, 0},
    {"size", 1, 0, 0},
    {"fullscreen", 0, 0, 0},
    {"list-scenes", 0, 0, 0},
    {"show-all-options", 0, 0, 0},
    {"debug", 0, 0, 0},
    {"help", 0, 0, 0},
    {0, 0, 0, 0}
};

/**
 * Parses a size string of the form WxH
 *
 * @param str the string to parse
 * @param size the parsed size (width, height)
 */
static void
parse_size(const std::string &str, std::pair<int,int> &size)
{
    std::vector<std::string> d;
    Util::split(str, 'x', d, Util::SplitModeNormal);

    size.first = Util::fromString<int>(d[0]);

    /*
     * Parse the second element (height). If there is none, use the value
     * of the first element for the second (width = height)
     */
    if (d.size() > 1)
        size.second = Util::fromString<int>(d[1]);
    else
        size.second = size.first;
}

/**
 * Parses a frame-end method string
 *
 * @param str the string to parse
 *
 * @return the parsed frame end method
 */
static Options::FrameEnd 
frame_end_from_str(const std::string &str)
{
    Options::FrameEnd m = Options::FrameEndDefault;

    if (str == "swap")
        m = Options::FrameEndSwap;
    else if (str == "finish")
        m = Options::FrameEndFinish;
    else if (str == "readpixels")
        m = Options::FrameEndReadPixels;
    else if (str == "none")
        m = Options::FrameEndNone;

    return m;
}

void
Options::print_help()
{
    printf("A benchmark for Open GL (ES) 2.0\n"
           "\n"
           "Options:\n"
           "  -b, --benchmark BENCH  A benchmark to run: 'scene(:opt1=val1)*'\n"
           "                         (the option can be used multiple times)\n"
           "  -f, --benchmark-file F Load benchmarks to run from a file containing a\n"
           "                         list of benchmark descriptions (one per line)\n"
           "                         (the option can be used multiple times)\n"
           "      --validate         Run a quick output validation test instead of \n"
           "                         running the benchmarks\n"
           "      --frame-end METHOD How to end a frame [default,none,swap,finish,readpixels]\n"
           "      --off-screen       Render to an off-screen surface\n"
           "      --visual-config C  The visual configuration to use for the rendering\n"
           "                         target: 'red=R:green=G:blue=B:alpha=A:buffer=BUF'.\n"
           "                         The parameters may be defined in any order, and any\n"
           "                         omitted parameters assume a default value of '1'\n"
           "      --reuse-context    Use a single context for all scenes\n"
           "                         (by default, each scene gets its own context)\n"
           "  -s, --size WxH         Size of the output window (default: 800x600)\n"
           "      --fullscreen       Run in fullscreen mode (equivalent to --size -1x-1)\n"
           "  -l, --list-scenes      Display information about the available scenes\n"
           "                         and their options\n"
           "      --show-all-options Show all scene option values used for benchmarks\n"
           "                         (only explicitly set options are shown by default)\n"
           "      --run-forever      Run indefinitely, looping from the last benchmark\n"
           "                         back to the first\n"
           "      --annotate         Annotate the benchmarks with on-screen information\n"
           "                         (same as -b :show-fps=true:title=#info#)\n"
           "  -d, --debug            Display debug messages\n"
           "  -h, --help             Display help\n");
}

bool
Options::parse_args(int argc, char **argv)
{
    while (1) {
        int option_index = -1;
        int c;
        const char *optname = "";

        c = getopt_long(argc, argv, "b:f:s:ldh",
                        long_options, &option_index);
        if (c == -1)
            break;
        if (c == ':' || c == '?')
            return false;

        if (option_index != -1)
            optname = long_options[option_index].name;

        if (!strcmp(optname, "annotate"))
            Options::annotate = true;
        if (c == 'b' || !strcmp(optname, "benchmark"))
            Options::benchmarks.push_back(optarg);
        else if (c == 'f' || !strcmp(optname, "benchmark-file"))
            Options::benchmark_files.push_back(optarg);
        else if (!strcmp(optname, "validate"))
            Options::validate = true;
        else if (!strcmp(optname, "frame-end"))
            Options::frame_end = frame_end_from_str(optarg);
        else if (!strcmp(optname, "off-screen"))
            Options::offscreen = true;
        else if (!strcmp(optname, "visual-config"))
            Options::visual_config = GLVisualConfig(optarg);
        else if (!strcmp(optname, "reuse-context"))
            Options::reuse_context = true;
        else if (c == 's' || !strcmp(optname, "size"))
            parse_size(optarg, Options::size);
        else if (!strcmp(optname, "fullscreen"))
            Options::size = std::pair<int,int>(-1, -1);
        else if (c == 'l' || !strcmp(optname, "list-scenes"))
            Options::list_scenes = true;
        else if (!strcmp(optname, "show-all-options"))
            Options::show_all_options = true;
        else if (!strcmp(optname, "run-forever"))
            Options::run_forever = true;
        else if (c == 'd' || !strcmp(optname, "debug"))
            Options::show_debug = true;
        else if (c == 'h' || !strcmp(optname, "help"))
            Options::show_help = true;
    }

    return true;
}
