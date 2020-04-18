/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package benchmarks.regression;

import com.google.caliper.Param;
import java.io.File;
import java.util.jar.JarFile;
import java.util.jar.Manifest;

public class JarFileBenchmark {
    @Param({
        "/system/framework/core-oj.jar",
        "/system/priv-app/Phonesky/Phonesky.apk"
    })
    private String filename;

    public void time(int reps) throws Exception {
        File f = new File(filename);
        for (int i = 0; i < reps; ++i) {
            JarFile jf = new JarFile(f);
            Manifest m = jf.getManifest();
            jf.close();
        }
    }
}
