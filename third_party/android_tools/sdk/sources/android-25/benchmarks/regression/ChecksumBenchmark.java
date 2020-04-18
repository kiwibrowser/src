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

import java.util.zip.Adler32;
import java.util.zip.CRC32;

public class ChecksumBenchmark {
    public void timeAdler_block(int reps) throws Exception {
        byte[] bytes = new byte[10000];
        Adler32 adler = new Adler32();
        for (int i = 0; i < reps; ++i) {
            adler.update(bytes);
        }
    }
    public void timeAdler_byte(int reps) throws Exception {
        Adler32 adler = new Adler32();
        for (int i = 0; i < reps; ++i) {
            adler.update(1);
        }
    }
    public void timeCrc_block(int reps) throws Exception {
        byte[] bytes = new byte[10000];
        CRC32 crc = new CRC32();
        for (int i = 0; i < reps; ++i) {
            crc.update(bytes);
        }
    }
    public void timeCrc_byte(int reps) throws Exception {
        CRC32 crc = new CRC32();
        for (int i = 0; i < reps; ++i) {
            crc.update(1);
        }
    }
}
