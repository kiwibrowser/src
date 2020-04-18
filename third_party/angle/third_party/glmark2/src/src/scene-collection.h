//
// Copyright © 2013 Linaro Limited
//
// This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
//
// glmark2 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// glmark2.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
//  Jesse Barker
//  Alexandros Frantzis
//
#ifndef GLMARK2_SCENE_COLLECTION_H_
#define GLMARK2_SCENE_COLLECTION_H_

#include <vector>
#include "scene.h"


class SceneCollection
{
public:
    SceneCollection(Canvas& canvas) 
    {
        add_scenes(canvas);
    }
    ~SceneCollection() { Util::dispose_pointer_vector(scenes_); }
    void register_scenes()
    {
        for (std::vector<Scene*>::const_iterator iter = scenes_.begin();
             iter != scenes_.end();
             iter++)
        {
            Benchmark::register_scene(**iter);
        }
    }
    const std::vector<Scene*>& get() { return scenes_; }

private:
    std::vector<Scene*> scenes_;

    //
    // Creates all the available scenes and adds them to the supplied vector.
    // 
    // @param scenes the vector to add the scenes to
    // @param canvas the canvas to create the scenes with
    //
    void add_scenes(Canvas& canvas)
    {
        scenes_.push_back(new SceneDefaultOptions(canvas));
        scenes_.push_back(new SceneBuild(canvas));
        scenes_.push_back(new SceneTexture(canvas));
        scenes_.push_back(new SceneShading(canvas));
        scenes_.push_back(new SceneConditionals(canvas));
        scenes_.push_back(new SceneFunction(canvas));
        scenes_.push_back(new SceneLoop(canvas));
        scenes_.push_back(new SceneBump(canvas));
        scenes_.push_back(new SceneEffect2D(canvas));
        scenes_.push_back(new ScenePulsar(canvas));
        scenes_.push_back(new SceneDesktop(canvas));
        scenes_.push_back(new SceneBuffer(canvas));
        scenes_.push_back(new SceneIdeas(canvas));
        scenes_.push_back(new SceneTerrain(canvas));
        scenes_.push_back(new SceneJellyfish(canvas));
        scenes_.push_back(new SceneShadow(canvas));
        scenes_.push_back(new SceneRefract(canvas));
        scenes_.push_back(new SceneClear(canvas));

    }
};
#endif // GLMARK2_SCENE_COLLECTION_H_
