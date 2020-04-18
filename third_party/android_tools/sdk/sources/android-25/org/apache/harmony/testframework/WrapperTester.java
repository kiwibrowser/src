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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;

/**
 * Tests behaviour common to wrapping and filtering implementations of {@link
 * OutputStream}.
 */
public abstract class WrapperTester {

    private boolean throwsExceptions = true;

    /**
     * Creates a new output stream that receives one stream of bytes, optionally
     * transforms it, and emits another stream of bytes to {@code delegate}.
     */
    public abstract OutputStream create(OutputStream delegate) throws Exception;

    /**
     * Decodes the bytes received by the delegate into their original form: the
     * bytes originally received by this wrapper.
     */
    public abstract byte[] decode(byte[] delegateBytes) throws Exception;

    /**
     * Configures whether the stream is expected to throw exceptions when an
     * error is encountered. Classes like {@code PrintStream} report errors via
     * an API method instead.
     */
    public WrapperTester setThrowsExceptions(boolean throwsExceptions) {
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

    private class WrapperSinkTester extends SinkTester {
        private ClosableByteArrayOutputStream delegate;

        @Override
        public OutputStream create() throws Exception {
            delegate = new ClosableByteArrayOutputStream();
            return WrapperTester.this.create(delegate);
        }

        @Override
        public byte[] getBytes() throws Exception {
            return WrapperTester.this.decode(delegate.bytesOut.toByteArray());
        }

        @Override
        public String toString() {
            return WrapperTester.this.toString();
        }
    }

    public class WrapperTestCase extends TestCase {

        private WrapperTestCase(String name) {
            super(name);
        }

        @Override
        public String getName() {
            return WrapperTester.this.toString() + ":" + super.getName();
        }

        public void wrapperTestFlushThrowsViaFlushSuppressed() throws Exception {
            FailOnFlushOutputStream delegate = new FailOnFlushOutputStream();
            OutputStream o = create(delegate);
            o.write(new byte[] { 8, 6, 7, 5 });
            o.write(new byte[] { 3, 0, 9 });
            o.flush();
            assertTrue(delegate.flushed);
        }

        public void wrapperTestFlushThrowsViaCloseSuppressed() throws Exception {
            FailOnFlushOutputStream delegate = new FailOnFlushOutputStream();
            OutputStream o = create(delegate);
            o.write(new byte[] { 8, 6, 7, 5 });
            o.write(new byte[] { 3, 0, 9 });
            o.close();
            assertTrue(delegate.flushed);
        }

        public void wrapperTestFlushThrowsViaFlush() throws Exception {
            FailOnFlushOutputStream delegate = new FailOnFlushOutputStream();

            OutputStream o = create(delegate);
            try {
                // any of these is permitted to flush
                o.write(new byte[] { 8, 6, 7, 5 });
                o.write(new byte[] { 3, 0, 9 });
                o.flush();
                assertTrue(delegate.flushed);
                fail("flush exception ignored");
            } catch (IOException expected) {
                assertEquals("Flush failed", expected.getMessage());
            }
        }

        public void wrapperTestFlushThrowsViaClose() throws Exception {
            FailOnFlushOutputStream delegate = new FailOnFlushOutputStream();

            OutputStream o = create(delegate);
            try {
                // any of these is permitted to flush
                o.write(new byte[] { 8, 6, 7, 5 });
                o.write(new byte[] { 3, 0, 9 });
                o.close();
                assertTrue(delegate.flushed);
                fail("flush exception ignored");
            } catch (IOException expected) {
                assertEquals("Flush failed", expected.getMessage());
            }

            try {
                o.write(new byte[] { 4, 4, 5 });
                fail("expected already closed exception");
            } catch (IOException expected) {
            }
        }

        public void wrapperTestCloseThrows() throws Exception {
            FailOnCloseOutputStream delegate = new FailOnCloseOutputStream();
            OutputStream o = create(delegate);
            try {
                o.close();
                assertTrue(delegate.closed);
                fail("close exception ignored");
            } catch (IOException expected) {
                assertEquals("Close failed", expected.getMessage());
            }
        }

        public void wrapperTestCloseThrowsSuppressed() throws Exception {
            FailOnCloseOutputStream delegate = new FailOnCloseOutputStream();
            OutputStream o = create(delegate);
            o.close();
            assertTrue(delegate.closed);
        }

        // adding a new test? Don't forget to update createTests().
    }

    private static class ClosableByteArrayOutputStream extends OutputStream {
        private final ByteArrayOutputStream bytesOut = new ByteArrayOutputStream();
        private boolean closed = false;

        @Override
        public void close() throws IOException {
            closed = true;
        }

        @Override
        public void write(int oneByte) throws IOException {
            if (closed) {
                throw new IOException();
            }
            bytesOut.write(oneByte);
        }
    }

    private static class FailOnFlushOutputStream extends OutputStream {
        boolean flushed = false;
        boolean closed = false;

        @Override
        public void write(int oneByte) throws IOException {
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

    private static class FailOnCloseOutputStream extends ByteArrayOutputStream {
        boolean closed = false;

        @Override
        public void close() throws IOException {
            closed = true;
            throw new IOException("Close failed");
        }
    }
}
