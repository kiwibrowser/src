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
#ifndef GLMARK2_BENCHMARK_H_
#define GLMARK2_BENCHMARK_H_

#include <vector>
#include <string>
#include <map>

#include "scene.h"

/**
 * A glmark2 benchmark.
 *
 * A benchmark is a Scene configured with a set of option values.
 */
class Benchmark
{
public:
    typedef std::pair<std::string, std::string> OptionPair;

    /**
     * Creates a benchmark using a scene object reference.
     *
     * @param scene the scene to use
     * @param options the options to use
     */
    Benchmark(Scene &scene, const std::vector<OptionPair> &options);

    /**
     * Creates a benchmark using a scene name.
     *
     * To use a scene by name, that scene must have been previously registered
     * using ::register_scene().
     *
     * @param name the name of the scene to use
     * @param options the options to use
     */
    Benchmark(const std::string &name, const std::vector<OptionPair> &options);

    /**
     * Creates a benchmark from a description string.
     *
     * The description string is of the form scene[:opt1=val1:opt2=val2...].
     * The specified scene must have been previously registered using
     * ::register_scene().
     *
     * @param s a description string
     */
    Benchmark(const std::string &s);

    /**
     * Gets the Scene associated with the benchmark.
     *
     * This method doesn't prepare the scene for a run.
     * (See ::setup_scene())
     *
     * @return the Scene
     */
    Scene &scene() const { return scene_; }

    /**
     * Sets up the Scene associated with the benchmark.
     *
     * @return the Scene
     */
    Scene &setup_scene();

    /**
     * Tears down the Scene associated with the benchmark.
     */
    void teardown_scene();

    /**
     * Whether the benchmark needs extra decoration.
     */
    bool needs_decoration() const;

    /**
     * Registers a Scene, so that it becomes accessible by name.
     */
    static void register_scene(Scene &scene);

    /**
     * Gets a registered scene by its name.
     *
     * @return the Scene
     */
    static Scene &get_scene_by_name(const std::string &name);

    /**
     * Gets the registered scenes.
     *
     * @return the Scene
     */
    static const std::map<std::string, Scene *> &scenes() { return sceneMap_; }

private:
    Scene &scene_;
    std::vector<OptionPair> options_;

    void load_options();

    static std::map<std::string, Scene *> sceneMap_;
};

#endif
