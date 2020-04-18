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
#ifndef GLMARK2_MAIN_LOOP_H_
#define GLMARK2_MAIN_LOOP_H_

#include "canvas.h"
#include "benchmark.h"
#include "text-renderer.h"
#include "vec.h"
#include <vector>

/**
 * Main loop for benchmarking.
 */
class MainLoop
{
public:
    MainLoop(Canvas &canvas, const std::vector<Benchmark *> &benchmarks);

    virtual ~MainLoop() {}

    /**
     * Resets the main loop.
     *
     * You need to call reset() if the loop has finished and
     * you need to run it again.
     */
    void reset();

    /**
     * Gets the current total benchmarking score.
     */
    unsigned int score();

    /**
     * Perform the next main loop step.
     *
     * @returns whether the loop has finished
     */
    bool step();

    /**
     * Overridable method for drawing the canvas contents.
     */
    virtual void draw();

    /**
     * Overridable method for pre scene-setup customizations.
     */
    virtual void before_scene_setup() {}

    /**
     * Overridable method for post scene-setup customizations.
     */
    virtual void after_scene_setup() {}

    /**
     * Overridable method for logging scene info.
     */
    virtual void log_scene_info();

    /**
     * Overridable method for logging scene result.
     */
    virtual void log_scene_result();

protected:
    enum SceneSetupStatus {
        SceneSetupStatusUnknown,
        SceneSetupStatusSuccess,
        SceneSetupStatusFailure,
        SceneSetupStatusUnsupported
    };
    void next_benchmark();
    Canvas &canvas_;
    Scene *scene_;
    const std::vector<Benchmark *> &benchmarks_;
    unsigned int score_;
    unsigned int benchmarks_run_;
    SceneSetupStatus scene_setup_status_;

    std::vector<Benchmark *>::const_iterator bench_iter_;
};

/**
 * Main loop for benchmarking with decorations (eg FPS, demo)
 */
class MainLoopDecoration : public MainLoop
{
public:
    MainLoopDecoration(Canvas &canvas, const std::vector<Benchmark *> &benchmarks);
    virtual ~MainLoopDecoration();

    virtual void draw();
    virtual void before_scene_setup();
    virtual void after_scene_setup();

protected:
    void fps_renderer_update_text(unsigned int fps);
    LibMatrix::vec2 vec2_from_pos_string(const std::string &s);

    bool show_fps_;
    bool show_title_;
    TextRenderer *fps_renderer_;
    TextRenderer *title_renderer_;
    unsigned int last_fps_;
    uint64_t fps_timestamp_;
};

/**
 * Main loop for validation.
 */
class MainLoopValidation : public MainLoop
{
public:
    MainLoopValidation(Canvas &canvas, const std::vector<Benchmark *> &benchmarks);

    virtual void draw();
    virtual void log_scene_result();
};

#endif /* GLMARK2_MAIN_LOOP_H_ */
