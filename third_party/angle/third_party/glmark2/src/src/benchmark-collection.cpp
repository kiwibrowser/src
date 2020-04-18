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
#include <fstream>
#include "benchmark-collection.h"
#include "default-benchmarks.h"
#include "options.h"
#include "log.h"
#include "util.h"

BenchmarkCollection::~BenchmarkCollection()
{
    Util::dispose_pointer_vector(benchmarks_);
}

void
BenchmarkCollection::add(const std::vector<std::string> &benchmarks)
{
    for (std::vector<std::string>::const_iterator iter = benchmarks.begin();
         iter != benchmarks.end();
         iter++)
    {
        benchmarks_.push_back(new Benchmark(*iter));
    }
}

void
BenchmarkCollection::populate_from_options()
{
    if (Options::annotate) {
        std::vector<std::string> annotate;
        annotate.push_back(":show-fps=true:title=#info#");
        add(annotate);
    }

    if (!Options::benchmarks.empty())
        add(Options::benchmarks);

    if (!Options::benchmark_files.empty())
        add_benchmarks_from_files();

    if (!benchmarks_contain_normal_scenes())
        add(DefaultBenchmarks::get());
}

bool
BenchmarkCollection::needs_decoration()
{
    for (std::vector<Benchmark *>::const_iterator bench_iter = benchmarks_.begin();
         bench_iter != benchmarks_.end();
         bench_iter++)
    {
        const Benchmark *bench = *bench_iter;
        if (bench->needs_decoration())
            return true;
    }

    return false;
}


void
BenchmarkCollection::add_benchmarks_from_files()
{
    for (std::vector<std::string>::const_iterator iter = Options::benchmark_files.begin();
         iter != Options::benchmark_files.end();
         iter++)
    {
        std::ifstream ifs(iter->c_str());

        if (!ifs.fail()) {
            std::string line;

            while (getline(ifs, line)) {
                if (!line.empty())
                    benchmarks_.push_back(new Benchmark(line));
            }
        }
        else {
            Log::error("Cannot open benchmark file %s\n",
                       iter->c_str());
        }

    }
}

bool
BenchmarkCollection::benchmarks_contain_normal_scenes()
{
    for (std::vector<Benchmark *>::const_iterator bench_iter = benchmarks_.begin();
         bench_iter != benchmarks_.end();
         bench_iter++)
    {
        const Benchmark *bench = *bench_iter;
        if (!bench->scene().name().empty())
            return true;
    }

    return false;
}

