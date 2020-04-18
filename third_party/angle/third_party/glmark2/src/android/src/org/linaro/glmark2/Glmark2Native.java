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

import android.content.res.AssetManager;

class Glmark2Native {
    public static native void init(AssetManager assetManager, String args,
                                   String logFilePath);
    public static native void resize(int w, int h);
    public static native boolean render();
    public static native void done();
    public static native int scoreConfig(GLVisualConfig vc, GLVisualConfig target);
    public static native SceneInfo[] getSceneInfo(AssetManager assetManager);
}
