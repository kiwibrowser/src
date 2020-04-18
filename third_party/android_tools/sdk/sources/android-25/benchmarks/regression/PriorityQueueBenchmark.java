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

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.PriorityQueue;
import java.util.Random;

public class PriorityQueueBenchmark {
    @Param({"100", "1000", "10000"}) private int queueSize;
    @Param({"0", "25", "50", "75", "100"}) private int hitRate;

    private PriorityQueue<Integer> pq;
    private PriorityQueue<Integer> usepq;
    private List<Integer> seekElements;
    private Random random = new Random(189279387L);

    @BeforeExperiment
    protected void setUp() throws Exception {
        pq = new PriorityQueue<Integer>();
        usepq = new PriorityQueue<Integer>();
        seekElements = new ArrayList<Integer>();
        List<Integer> allElements = new ArrayList<Integer>();
        int numShared = (int)(queueSize * ((double)hitRate / 100));
        // the total number of elements we require to engineer a hit rate of hitRate%
        int totalElements = 2 * queueSize - numShared;
        for (int i = 0; i < totalElements; i++) {
            allElements.add(i);
        }
        // shuffle these elements so that we get a reasonable distribution of missed elements
        Collections.shuffle(allElements, random);
        // add shared elements
        for (int i = 0; i < numShared; i++) {
            pq.add(allElements.get(i));
            seekElements.add(allElements.get(i));
        }
        // add priority queue only elements (these won't be touched)
        for (int i = numShared; i < queueSize; i++) {
            pq.add(allElements.get(i));
        }
        // add non-priority queue elements (these will be misses)
        for (int i = queueSize; i < totalElements; i++) {
            seekElements.add(allElements.get(i));
        }
        usepq = new PriorityQueue<Integer>(pq);
        // shuffle again so that elements are accessed in a different pattern than they were
        // inserted
        Collections.shuffle(seekElements, random);
    }

    public boolean timeRemove(int reps) {
        boolean dummy = false;
        int elementsSize = seekElements.size();
        // At most allow the queue to empty 10%.
        int resizingThreshold = queueSize / 10;
        for (int i = 0; i < reps; i++) {
            // Reset queue every so often. This will be called more often for smaller
            // queueSizes, but since a copy is linear, it will also cost proportionally
            // less, and hopefully it will approximately balance out.
            if (i % resizingThreshold == 0) {
                usepq = new PriorityQueue<Integer>(pq);
            }
            dummy = usepq.remove(seekElements.get(i % elementsSize));
        }
        return dummy;
    }
}
