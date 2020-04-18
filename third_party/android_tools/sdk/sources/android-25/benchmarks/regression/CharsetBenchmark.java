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

public class CharsetBenchmark {
    @Param({ "1", "10", "100", "1000", "10000" })
    private int length;

    // canonical    => canonical charset name
    // built-in     => guaranteed-present charset
    // special-case => libcore treats this charset specially for performance
    @Param({
        "UTF-16",     //     canonical,     built-in, non-special-case
        "UTF-8",      //     canonical,     built-in,     special-case
        "UTF8",       // non-canonical,     built-in,     special-case
        "ISO-8859-1", //     canonical,     built-in,     special-case
        "8859_1",     // non-canonical,     built-in,     special-case
        "ISO-8859-2", //     canonical, non-built-in, non-special-case
        "8859_2",     // non-canonical, non-built-in, non-special-case
        "US-ASCII",   //     canonical,     built-in,     special-case
        "ASCII"       // non-canonical,     built-in,     special-case
    })
    private String name;

    public void time_new_String_BString(int reps) throws Exception {
        byte[] bytes = makeBytes(makeString(length));
        for (int i = 0; i < reps; ++i) {
            new String(bytes, name);
        }
    }

    public void time_new_String_BII(int reps) throws Exception {
      byte[] bytes = makeBytes(makeString(length));
      for (int i = 0; i < reps; ++i) {
        new String(bytes, 0, bytes.length);
      }
    }

    public void time_new_String_BIIString(int reps) throws Exception {
      byte[] bytes = makeBytes(makeString(length));
      for (int i = 0; i < reps; ++i) {
        new String(bytes, 0, bytes.length, name);
      }
    }

    public void time_String_getBytes(int reps) throws Exception {
        String string = makeString(length);
        for (int i = 0; i < reps; ++i) {
            string.getBytes(name);
        }
    }

    private static String makeString(int length) {
        StringBuilder result = new StringBuilder(length);
        for (int i = 0; i < length; ++i) {
            result.append('A' + (i % 26));
        }
        return result.toString();
    }

    private static byte[] makeBytes(String s) {
        try {
            return s.getBytes("US-ASCII");
        } catch (Exception ex) {
            throw new RuntimeException(ex);
        }
    }
}
