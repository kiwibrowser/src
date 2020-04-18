/*
 * Copyright (C) 2015 Google Inc.
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
import junit.framework.Assert;

/**
 * Benchmarks to measure the performance of String.equals for Strings of varying lengths.
 * Each benchmarks makes 5 measurements, aiming at covering cases like strings of equal length
 * that are not equal, identical strings with different references, strings with different endings,
 * interned strings, and strings of different lengths.
 */
public class StringEqualsBenchmark {
    private final String long1 = "Ahead-of-time compilation is possible as the compiler may just"
        + "convert an instruction thus: dex code: add-int v1000, v2000, v3000 C code: setIntRegter"
        + "(1000, call_dex_add_int(getIntRegister(2000), getIntRegister(3000)) This means even lid"
        + "instructions may have code generated, however, it is not expected that code generate in"
        + "this way will perform well. The job of AOT verification is to tell the compiler that"
        + "instructions are sound and provide tests to detect unsound sequences so slow path code"
        + "may be generated. Other than for totally invalid code, the verification may fail at AOr"
        + "run-time. At AOT time it can be because of incomplete information, at run-time it can e"
        + "that code in a different apk that the application depends upon has changed. The Dalvik"
        + "verifier would return a bool to state whether a Class were good or bad. In ART the fail"
        + "case becomes either a soft or hard failure. Classes have new states to represent that a"
        + "soft failure occurred at compile time and should be re-verified at run-time.";

    private final String veryLong = "Garbage collection has two phases. The first distinguishes"
        + "live objects from garbage objects.  The second is reclaiming the rage of garbage object"
        + "In the mark-sweep algorithm used by Dalvik, the first phase is achievd by computing the"
        + "closure of all reachable objects in a process known as tracing from theoots.  After the"
        + "trace has completed, garbage objects are reclaimed.  Each of these operations can be"
        + "parallelized and can be interleaved with the operation of the applicationTraditionally,"
        + "the tracing phase dominates the time spent in garbage collection.  The greatreduction i"
        + "pause time can be achieved by interleaving as much of this phase as possible with the"
        + "application. If we simply ran the GC in a separate thread with no other changes, normal"
        + "operation of an application would confound the trace.  Abstractly, the GC walks the h o"
        + "all reachable objects.  When the application is paused, the object graph cannot change."
        + "The GC can therefore walk this structure and assume that all reachable objects live."
        + "When the application is running, this graph may be altered. New nodes may be addnd edge"
        + "may be changed.  These changes may cause live objects to be hidden and falsely recla by"
        + "the GC.  To avoid this problem a write barrier is used to intercept and record modifion"
        + "to objects in a separate structure.  After performing its walk, the GC will revisit the"
        + "updated objects and re-validate its assumptions.  Without a card table, the garbage"
        + "collector would have to visit all objects reached during the trace looking for dirtied"
        + "objects.  The cost of this operation would be proportional to the amount of live data."
        + "With a card table, the cost of this operation is proportional to the amount of updateat"
        + "The write barrier in Dalvik is a card marking write barrier.  Card marking is the proce"
        + "of noting the location of object connectivity changes on a sub-page granularity.  A car"
        + "is merely a colorful term for a contiguous extent of memory smaller than a page, common"
        + "somewhere between 128- and 512-bytes.  Card marking is implemented by instrumenting all"
        + "locations in the virtual machine which can assign a pointer to an object.  After themal"
        + "pointer assignment has occurred, a byte is written to a byte-map spanning the heap whic"
        + "corresponds to the location of the updated object.  This byte map is known as a card ta"
        + "The garbage collector visits this card table and looks for written bytes to reckon the"
        + "location of updated objects.  It then rescans all objects located on the dirty card,"
        + "correcting liveness assumptions that were invalidated by the application.  While card"
        + "marking imposes a small burden on the application outside of a garbage collection, the"
        + "overhead of maintaining the card table is paid for by the reduced time spent inside"
        + "garbage collection. With the concurrent garbage collection thread and a write barrier"
        + "supported by the interpreter, JIT, and Runtime we modify garbage collection";

    private final String[][] shortStrings = new String[][] {
        // Equal, constant comparison
        { "a", "a" },
        // Different constants, first character different
        { ":", " :"},
        // Different constants, last character different, same length
        { "ja M", "ja N"},
        // Different constants, different lengths
        {"$$$", "$$"},
        // Force execution of code beyond reference equality check
        {"hi", new String("hi")}
    };

    private final String[][] mediumStrings = new String[][] {
        // Equal, constant comparison
        { "Hello my name is ", "Hello my name is " },
        // Different constants, different lengths
        { "What's your name?", "Whats your name?" },
        // Force execution of code beyond reference equality check
        { "Android Runtime", new String("Android Runtime") },
        // Different constants, last character different, same length
        { "v3ry Cre@tiVe?****", "v3ry Cre@tiVe?***." },
        // Different constants, first character different, same length
        { "!@#$%^&*()_++*^$#@", "0@#$%^&*()_++*^$#@" }
    };

    private final String[][] longStrings = new String[][] {
        // Force execution of code beyond reference equality check
        { long1, new String(long1) },
        // Different constants, last character different, same length
        { long1 + "fun!", long1 + "----" },
        // Equal, constant comparison
        { long1 + long1, long1 + long1 },
        // Different constants, different lengths
        { long1 + "123456789", long1 + "12345678" },
        // Different constants, first character different, same length
        { "Android Runtime" + long1, "android Runtime" + long1 }
    };

    private final String[][] veryLongStrings = new String[][] {
        // Force execution of code beyond reference equality check
        { veryLong, new String(veryLong) },
        // Different constants, different lengths
        { veryLong + veryLong, veryLong + " " + veryLong },
        // Equal, constant comparison
        { veryLong + veryLong + veryLong, veryLong + veryLong + veryLong },
        // Different constants, last character different, same length
        { veryLong + "77777", veryLong + "99999" },
        // Different constants, first character different
        { "Android Runtime" + veryLong, "android Runtime" + veryLong }
    };

    private final String[][] endStrings = new String[][] {
        // Different constants, medium but different lengths
        { "Hello", "Hello " },
        // Different constants, long but different lengths
        { long1, long1 + "x"},
        // Different constants, very long but different lengths
        { veryLong, veryLong + "?"},
        // Different constants, same medium lengths
        { "How are you doing today?", "How are you doing today " },
        // Different constants, short but different lengths
        { "1", "1." }
    };

    private final String tmpStr1 = "012345678901234567890"
        + "0123456789012345678901234567890123456789"
        + "0123456789012345678901234567890123456789"
        + "0123456789012345678901234567890123456789"
        + "0123456789012345678901234567890123456789";

    private final String tmpStr2 = "z012345678901234567890"
        + "0123456789012345678901234567890123456789"
        + "0123456789012345678901234567890123456789"
        + "0123456789012345678901234567890123456789"
        + "012345678901234567890123456789012345678x";

    private final String[][] nonalignedStrings = new String[][] {
        // Different non-word aligned medium length strings
        { tmpStr1, tmpStr1.substring(1) },
        // Different differently non-word aligned medium length strings
        { tmpStr2, tmpStr2.substring(2) },
        // Different non-word aligned long length strings
        { long1, long1.substring(3) },
        // Different non-word aligned very long length strings
        { veryLong, veryLong.substring(1) },
        // Equal non-word aligned constant strings
        { "hello", "hello".substring(1) }
    };

    private final Object[] objects = new Object[] {
        // Compare to Double object
        new Double(1.5),
        // Compare to Integer object
        new Integer(9999999),
        // Compare to String array
        new String[] {"h", "i"},
        // Compare to int array
        new int[] {1, 2, 3},
        // Compare to Character object
        new Character('a')
    };

    // Check assumptions about how the compiler, new String(String), and String.intern() work.
    // Any failures here would invalidate these benchmarks.
    @BeforeExperiment
    protected void setUp() throws Exception {
        // String constants are the same object
        Assert.assertSame("abc", "abc");
        // new String(String) makes a copy
        Assert.assertNotSame("abc" , new String("abc"));
        // Interned strings are treated like constants, so it is not necessary to
        // separately benchmark interned strings.
        Assert.assertSame("abc", "abc".intern());
        Assert.assertSame("abc", new String("abc").intern());
        // Compiler folds constant strings into new constants
        Assert.assertSame(long1 + long1, long1 + long1);
    }

    // Benchmark cases of String.equals(null)
    public void timeEqualsNull(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < mediumStrings.length; i++) {
                mediumStrings[i][0].equals(null);
            }
        }
    }

    // Benchmark cases with very short (<5 character) Strings
    public void timeEqualsShort(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < shortStrings.length; i++) {
                shortStrings[i][0].equals(shortStrings[i][1]);
            }
        }
    }

    // Benchmark cases with medium length (10-15 character) Strings
    public void timeEqualsMedium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < mediumStrings.length; i++) {
                mediumStrings[i][0].equals(mediumStrings[i][1]);
            }
        }
    }

    // Benchmark cases with long (>100 character) Strings
    public void timeEqualsLong(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < longStrings.length; i++) {
                longStrings[i][0].equals(longStrings[i][1]);
            }
        }
    }

    // Benchmark cases with very long (>1000 character) Strings
    public void timeEqualsVeryLong(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < veryLongStrings.length; i++) {
                veryLongStrings[i][0].equals(veryLongStrings[i][1]);
            }
        }
    }

    // Benchmark cases with non-word aligned Strings
    public void timeEqualsNonWordAligned(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < nonalignedStrings.length; i++) {
                nonalignedStrings[i][0].equals(nonalignedStrings[i][1]);
            }
        }
    }

    // Benchmark cases with slight differences in the endings
    public void timeEqualsEnd(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < endStrings.length; i++) {
                endStrings[i][0].equals(endStrings[i][1]);
            }
        }
    }

    // Benchmark cases of comparing a string to a non-string object
    public void timeEqualsNonString(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < mediumStrings.length; i++) {
                mediumStrings[i][0].equals(objects[i]);
            }
        }
    }
}
