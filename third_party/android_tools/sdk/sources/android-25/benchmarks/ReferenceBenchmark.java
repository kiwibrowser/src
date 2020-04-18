/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package benchmarks;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Benchmark to evaluate the performance of References.
 */
public class ReferenceBenchmark {

    private Object object;

    // How fast can references can be allocated?
    public void timeAlloc(int reps) {
        ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
        for (int i = 0; i < reps; i++) {
            new PhantomReference(object, queue);
        }
    }

    // How fast can references can be allocated and manually enqueued?
    public void timeAllocAndEnqueue(int reps) {
        ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
        for (int i = 0; i < reps; i++) {
            (new PhantomReference<Object>(object, queue)).enqueue();
        }
    }

    // How fast can references can be allocated, enqueued, and polled?
    public void timeAllocEnqueueAndPoll(int reps) {
        ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
        for (int i = 0; i < reps; i++) {
            (new PhantomReference<Object>(object, queue)).enqueue();
        }
        for (int i = 0; i < reps; i++) {
            queue.poll();
        }
    }

    // How fast can references can be allocated, enqueued, and removed?
    public void timeAllocEnqueueAndRemove(int reps) {
        ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
        for (int i = 0; i < reps; i++) {
            (new PhantomReference<Object>(object, queue)).enqueue();
        }
        for (int i = 0; i < reps; i++) {
            try {
                queue.remove();
            } catch (InterruptedException ie) {
                i--;
            }
        }
    }

    // How fast can references can be implicitly allocated, enqueued, and
    // removed?
    public void timeAllocImplicitEnqueueAndRemove(int reps) {
        ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
        List<Object> refs = new ArrayList<Object>();
        for (int i = 0; i < reps; i++) {
            refs.add(new PhantomReference<Object>(new Object(), queue));
        }
        Runtime.getRuntime().gc();
        for (int i = 0; i < reps; i++) {
            try {
                queue.remove();
            } catch (InterruptedException ie) {
                i--;
            }
        }
    }

    static private class FinalizableObject {
        AtomicInteger count;

        public FinalizableObject(AtomicInteger count) {
            this.count = count;
        }

        @Override
        protected void finalize() {
            count.incrementAndGet();
        }
    }

    // How fast does finalization run?
    public void timeFinalization(int reps) {
        // Allocate a bunch of finalizable objects.
        int n = reps;
        AtomicInteger count = new AtomicInteger(0);
        for (int i = 0; i < n; i++) {
            new FinalizableObject(count);
        }

        // Run GC so the objects will be collected for finalization.
        Runtime.getRuntime().gc();

        // Wait for finalization.
        Runtime.getRuntime().runFinalization();

        // Double check all the objects were finalized.
        int got = count.get();
        if (n != got) {
            throw new IllegalStateException(
                    String.format("Only %i of %i objects finalized?", got, n));
        }
    }
}
