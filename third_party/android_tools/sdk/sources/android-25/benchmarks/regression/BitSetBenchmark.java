/*
 * Copyright (C) 2011 Google Inc.
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

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.util.BitSet;

public class BitSetBenchmark {
    @Param({ "1000", "10000" })
    private int size;

    private BitSet bs;

    @BeforeExperiment
    protected void setUp() throws Exception {
        bs = new BitSet(size);
    }

    public void timeIsEmptyTrue(int reps) {
        for (int i = 0; i < reps; ++i) {
            if (!bs.isEmpty()) throw new RuntimeException();
        }
    }

    public void timeIsEmptyFalse(int reps) {
        bs.set(bs.size() - 1);
        for (int i = 0; i < reps; ++i) {
            if (bs.isEmpty()) throw new RuntimeException();
        }
    }

    public void timeGet(int reps) {
        for (int i = 0; i < reps; ++i) {
            bs.get(i % size);
        }
    }

    public void timeClear(int reps) {
        for (int i = 0; i < reps; ++i) {
            bs.clear(i % size);
        }
    }

    public void timeSet(int reps) {
        for (int i = 0; i < reps; ++i) {
            bs.set(i % size);
        }
    }

    public void timeSetOn(int reps) {
        for (int i = 0; i < reps; ++i) {
            bs.set(i % size, true);
        }
    }

    public void timeSetOff(int reps) {
        for (int i = 0; i < reps; ++i) {
            bs.set(i % size, false);
        }
    }
}
