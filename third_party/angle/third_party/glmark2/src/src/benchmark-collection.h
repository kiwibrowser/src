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
#ifndef GLMARK2_BENCHMARK_COLLECTION_H_
#define GLMARK2_BENCHMARK_COLLECTION_H_

#include <vector>
#include <string>
#include "benchmark.h"

class BenchmarkCollection
{
public:
    BenchmarkCollection() {}
    ~BenchmarkCollection();

    /*
     * Adds benchmarks to the collection.
     */
    void add(const std::vector<std::string> &benchmarks);

    /*
     * Populates the collection guided by the global options.
     */
    void populate_from_options();

    /*
     * Whether the benchmarks in this collection need decoration.
     */
    bool needs_decoration();

    const std::vector<Benchmark *>& benchmarks() { return benchmarks_; }

private:
    void add_benchmarks_from_files();
    bool benchmarks_contain_normal_scenes();

    std::vector<Benchmark *> benchmarks_;
};

#endif /* GLMARK2_BENCHMARK_COLLECTION_H_ */
