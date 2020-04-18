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

/**
 * How do the various schemes for iterating through a string compare?
 */
public class StringIterationBenchmark {
    public void timeStringIteration0(int reps) {
        String s = "hello, world!";
        for (int rep = 0; rep < reps; ++rep) {
            char ch;
            for (int i = 0; i < s.length(); ++i) {
                ch = s.charAt(i);
            }
        }
    }
    public void timeStringIteration1(int reps) {
        String s = "hello, world!";
        for (int rep = 0; rep < reps; ++rep) {
            char ch;
            for (int i = 0, length = s.length(); i < length; ++i) {
                ch = s.charAt(i);
            }
        }
    }
    public void timeStringIteration2(int reps) {
        String s = "hello, world!";
        for (int rep = 0; rep < reps; ++rep) {
            char ch;
            char[] chars = s.toCharArray();
            for (int i = 0, length = chars.length; i < length; ++i) {
                ch = chars[i];
            }
        }
    }
    public void timeStringToCharArray(int reps) {
        String s = "hello, world!";
        for (int rep = 0; rep < reps; ++rep) {
            char[] chars = s.toCharArray();
        }
    }
}
