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

import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.io.IOException;
import java.io.Writer;

/**
 * Tests behaviour common to wrapping and filtering implementations of {@link
 * Writer}.
 */
public abstract class CharWrapperTester {

    private boolean throwsExceptions = true;

    /**
     * Creates a new output stream that receives one stream of chars, optionally
     * transforms it, and emits another stream of chars to {@code delegate}.
     */
    public abstract Writer create(Writer delegate) throws Exception;

    /**
     * Decodes the chars received by the delegate into their original form: the
     * chars originally received by this wrapper.
     */
    public abstract char[] decode(char[] delegateChars) throws Exception;

    /**
     * Configures whether the writer is expected to throw exceptions when an
     * error is encountered. Classes like {@code PrintWriter} report errors via
     * an API method instead.
     */
    public CharWrapperTester setThrowsExceptions(boolean throwsExceptions) {
        this.throwsExceptions = throwsExceptions;
        return this;
    }

    public final TestSuite createTests() {
        TestSuite result = new TestSuite();
        result.addTest(new WrapperSinkTester()
                .setThrowsExceptions(throwsExceptions)
                .createTests());

        if (throwsExceptions) {
            result.addTest(new WrapperTestCase("wrapperTestFlushThrowsViaFlush"));
            result.addTest(new WrapperTestCase("wrapperTestFlushThrowsViaClose"));
            result.addTest(new WrapperTestCase("wrapperTestCloseThrows"));
        } else {
            result.addTest(new WrapperTestCase("wrapperTestFlushThrowsViaFlushSuppressed"));
            result.addTest(new WrapperTestCase("wrapperTestFlushThrowsViaCloseSuppressed"));
            result.addTest(new WrapperTestCase("wrapperTestCloseThrowsSuppressed"));
        }

        return result;
    }

    @Override
    public String toString() {
        return getClass().getName();
    }

    private class WrapperSinkTester extends CharSinkTester {
        private ClosableStringWriter delegate;

        @Override
        public Writer create() throws Exception {
            delegate = new ClosableStringWriter();
            return CharWrapperTester.this.create(delegate);
        }

        @Override
        public char[] getChars() throws Exception {
            return decode(delegate.buffer.toString().toCharArray());
        }

        @Override
        public String toString() {
            return CharWrapperTester.this.toString();
        }
    }

    public class WrapperTestCase extends TestCase {

        private WrapperTestCase(String name) {
            super(name);
        }

        @Override
        public String getName() {
            return CharWrapperTester.this.toString() + ":" + super.getName();
        }

        public void wrapperTestFlushThrowsViaFlushSuppressed() throws Exception {
            FailOnFlushWriter delegate = new FailOnFlushWriter();
            Writer o = create(delegate);
            o.write("BUT");
            o.write("TERS");
            o.flush();
            assertTrue(delegate.flushed);
        }

        public void wrapperTestFlushThrowsViaCloseSuppressed() throws Exception {
            FailOnFlushWriter delegate = new FailOnFlushWriter();
            Writer o = create(delegate);
            o.write("BUT");
            o.write("TERS");
            o.close();
            assertTrue(delegate.flushed);
        }

        public void wrapperTestFlushThrowsViaFlush() throws Exception {
            FailOnFlushWriter delegate = new FailOnFlushWriter();

            Writer o = create(delegate);
            try {
                // any of these is permitted to flush
                o.write("BUT");
                o.write("TERS");
                o.flush();
                assertTrue(delegate.flushed);
                fail("flush exception ignored");
            } catch (IOException expected) {
                assertEquals("Flush failed", expected.getMessage());
            }
        }

        public void wrapperTestFlushThrowsViaClose() throws Exception {
            FailOnFlushWriter delegate = new FailOnFlushWriter();

            Writer o = create(delegate);
            try {
                // any of these is permitted to flush
                o.write("BUT");
                o.write("TERS");
                o.close();
                assertTrue(delegate.flushed);
                fail("flush exception ignored");
            } catch (IOException expected) {
                assertEquals("Flush failed", expected.getMessage());
            }

            try {
                o.write("BARK");
                fail("expected already closed exception");
            } catch (IOException expected) {
            }
        }

        public void wrapperTestCloseThrows() throws Exception {
            FailOnCloseWriter delegate = new FailOnCloseWriter();
            Writer o = create(delegate);
            try {
                o.close();
                assertTrue(delegate.closed);
                fail("close exception ignored");
            } catch (IOException expected) {
                assertEquals("Close failed", expected.getMessage());
            }
        }

        public void wrapperTestCloseThrowsSuppressed() throws Exception {
            FailOnCloseWriter delegate = new FailOnCloseWriter();
            Writer o = create(delegate);
            o.close();
            assertTrue(delegate.closed);
        }

        // adding a new test? Don't forget to update createTests().
    }

    /**
     * A custom Writer that respects the closed state. The built-in StringWriter
     * doesn't respect close(), which makes testing wrapped streams difficult.
     */
    private static class ClosableStringWriter extends Writer {
        private final StringBuilder buffer = new StringBuilder();
        private boolean closed = false;

        @Override
        public void close() throws IOException {
            closed = true;
        }

        @Override
        public void flush() throws IOException {
        }

        @Override
        public void write(char[] buf, int offset, int count) throws IOException {
            if (closed) {
                throw new IOException();
            }
            buffer.append(buf, offset, count);
        }
    }

    private static class FailOnFlushWriter extends Writer {
        boolean flushed = false;
        boolean closed = false;

        @Override
        public void write(char[] buf, int offset, int count) throws IOException {
            if (closed) {
                throw new IOException("Already closed");
            }
        }

        @Override
        public void close() throws IOException {
            closed = true;
            flush();
        }

        @Override
        public void flush() throws IOException {
            if (!flushed) {
                flushed = true;
                throw new IOException("Flush failed");
            }
        }
    }

    private static class FailOnCloseWriter extends Writer {
        boolean closed = false;

        @Override
        public void flush() throws IOException {
        }

        @Override
        public void write(char[] buf, int offset, int count) throws IOException {
        }

        @Override
        public void close() throws IOException {
            closed = true;
            throw new IOException("Close failed");
        }
    }
}