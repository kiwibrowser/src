/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.testframework;

import junit.framework.Assert;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.io.IOException;
import java.io.Writer;
import java.util.Arrays;
import java.util.Random;

/**
 * Tests behaviour common to all implementations of {@link Writer}. This adapts
 * writers that collects untransformed chars so that they may be tested.
 */
public abstract class CharSinkTester {

    private boolean throwsExceptions = true;

    /**
     * Creates a new writer ready to receive an arbitrary number of chars. Each
     * time this method is invoked, any previously returned writers may be
     * discarded.
     */
    public abstract Writer create() throws Exception;

    /**
     * Returns the current set of chars written to the writer last returned by
     * {@link #create}, and releases any resources held by that writer.
     */
    public abstract char[] getChars() throws Exception;

    /**
     * Configures whether the writer is expected to throw exceptions when an
     * error is encountered. Classes like {@code PrintWriter} report errors via
     * an API method instead.
     */
    public CharSinkTester setThrowsExceptions(boolean throwsExceptions) {
        this.throwsExceptions = throwsExceptions;
        return this;
    }

    public final TestSuite createTests() {
        TestSuite result = new TestSuite();
        result.addTest(new SinkTestCase("sinkTestNoWriting"));
        result.addTest(new SinkTestCase("sinkTestWriteZeroChars"));
        result.addTest(new SinkTestCase("sinkTestWriteCharByChar"));
        result.addTest(new SinkTestCase("sinkTestWriteArray"));
        result.addTest(new SinkTestCase("sinkTestWriteOffset"));
        result.addTest(new SinkTestCase("sinkTestWriteLargeArray"));

        if (throwsExceptions) {
            result.addTest(new SinkTestCase("sinkTestWriteAfterClose"));
        } else {
            result.addTest(new SinkTestCase("sinkTestWriteAfterCloseSuppressed"));
        }

        return result;
    }

    @Override
    public String toString() {
        return getClass().getName();
    }

    private static void assertArrayEquals(char[] expected, char[] actual) {
        Assert.assertEquals(Arrays.toString(expected), Arrays.toString(actual));
    }

    public class SinkTestCase extends TestCase {

        private SinkTestCase(String name) {
            super(name);
        }

        public void sinkTestNoWriting() throws Exception {
            char[] expected = new char[] { };

            Writer out = create();
            out.close();
            assertArrayEquals(expected, getChars());
        }

        public void sinkTestWriteZeroChars() throws Exception {
            char[] expected = new char[] { };

            Writer out = create();
            char[] a = new char[1024];
            out.write(a, 1000, 0);
            out.write(a, 0, 0);
            out.write(new char[] { });

            out.close();
            assertArrayEquals(expected, getChars());
        }

        public void sinkTestWriteCharByChar() throws Exception {
            char[] expected = "EFGCDECBA".toCharArray();

            Writer out = create();
            for (char c : expected) {
                out.write(c);
            }

            out.close();
            assertArrayEquals(expected, getChars());
        }

        public void sinkTestWriteArray() throws Exception {
            char[] expected = "EFGCDECBA".toCharArray();

            Writer out = create();

            out.write("EF".toCharArray());
            out.write("GCDE".toCharArray());
            out.write("CBA".toCharArray());

            out.close();
            assertArrayEquals(expected, getChars());
        }

        public void sinkTestWriteOffset() throws Exception {
            char[] expected = "EFGCDECBA".toCharArray();
            Writer out = create();

            char[] a = new char[1024];
            a[1000] = 'E';
            a[1001] = 'F';
            out.write(a, 1000, 2);

            char[] b = new char[1024];
            b[1020] = 'G';
            b[1021] = 'C';
            b[1022] = 'D';
            b[1023] = 'E';
            out.write(b, 1020, 4);

            char[] c = new char[1024];
            c[0] = 'C';
            c[1] = 'B';
            c[2] = 'A';
            out.write(c, 0, 3);

            out.close();
            assertArrayEquals(expected, getChars());
        }

        public void sinkTestWriteLargeArray() throws Exception {
            Random dice = new Random();
            char[] expected = new char[(1024 * 1024) + 1]; // 2 MB + 1 char
            for (int c = 0; c < expected.length; c++) {
                expected[c] = (char) ('A' + dice.nextInt(26));
            }

            Writer out = create();
            out.write(expected);
            out.close();

            assertArrayEquals(expected, getChars());
        }

        public void sinkTestWriteAfterClose() throws Exception {
            char[] expectedChars = "EF".toCharArray();
            Writer out = create();

            out.write(expectedChars);
            out.close();

            try {
                out.write("GCDE".toCharArray());
                fail("expected already closed exception");
            } catch (IOException expected) {
            }

            assertArrayEquals(expectedChars, getChars());
        }

        public void sinkTestWriteAfterCloseSuppressed() throws Exception {
            Writer out = create();
            out.write("EF".toCharArray());
            out.close();
            out.write("GCDE".toCharArray()); // no exception expected!
        }

        // adding a new test? Don't forget to update createTests().

        @Override
        public String getName() {
            return CharSinkTester.this.toString() + ":" + super.getName();
        }
    }
}
