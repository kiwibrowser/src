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

package benchmarks;

import java.util.HashMap;
import java.util.Map;

/**
 * Is there a performance reason to "Prefer virtual over interface", as the
 * Android documentation once claimed?
 */
public class VirtualVersusInterfaceBenchmark {
    public void timeMapPut(int reps) {
        Map<String, String> map = new HashMap<String, String>();
        for (int i = 0; i < reps; ++i) {
            map.put("hello", "world");
        }
    }
    public void timeHashMapPut(int reps) {
        HashMap<String, String> map = new HashMap<String, String>();
        for (int i = 0; i < reps; ++i) {
            map.put("hello", "world");
        }
    }
}
