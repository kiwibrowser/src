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
import java.util.Hashtable;
import java.util.LinkedHashMap;
import java.util.concurrent.ConcurrentHashMap;

/**
 * How do the various hash maps compare?
 */
public class HashedCollectionsBenchmark {
    public void timeHashMapGet(int reps) {
        HashMap<String, String> map = new HashMap<String, String>();
        map.put("hello", "world");
        for (int i = 0; i < reps; ++i) {
            map.get("hello");
        }
    }
    public void timeHashMapGet_Synchronized(int reps) {
        HashMap<String, String> map = new HashMap<String, String>();
        synchronized (map) {
            map.put("hello", "world");
        }
        for (int i = 0; i < reps; ++i) {
            synchronized (map) {
                map.get("hello");
            }
        }
    }
    public void timeHashtableGet(int reps) {
        Hashtable<String, String> map = new Hashtable<String, String>();
        map.put("hello", "world");
        for (int i = 0; i < reps; ++i) {
            map.get("hello");
        }
    }
    public void timeLinkedHashMapGet(int reps) {
        LinkedHashMap<String, String> map = new LinkedHashMap<String, String>();
        map.put("hello", "world");
        for (int i = 0; i < reps; ++i) {
            map.get("hello");
        }
    }
    public void timeConcurrentHashMapGet(int reps) {
        ConcurrentHashMap<String, String> map = new ConcurrentHashMap<String, String>();
        map.put("hello", "world");
        for (int i = 0; i < reps; ++i) {
            map.get("hello");
        }
    }
}
