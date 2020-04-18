/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.bitmap.util;

import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;

/**
 * Wrapper for {@link InputStream} that allows you to read bytes from it like a byte[]. An
 * internal buffer is kept as small as possible to avoid large unnecessary allocations.
 *
 * <p/>
 * Care must be taken so that the internal buffer is kept small. The best practice is to
 * precalculate the maximum buffer size that you will need. For example,
 * say you have a loop that reads bytes from index <code>0</code> to <code>10</code>,
 * skips to index <code>N</code>, reads from index <code>N</code> to <code>N+10</code>, etc. Then
 * you know that the internal buffer can have a maximum size of <code>10</code>,
 * and you should set the <code>bufferSize</code> parameter to <code>10</code> in the constructor.
 *
 * <p/>
 * Use {@link #advanceTo(int)} to declare that you will not need to access lesser indexes. This
 * helps to keep the internal buffer small. In the above example, after reading bytes from index
 * <code>0</code> to <code>10</code>, you should call <code>advanceTo(N)</code> so that internal
 * buffer becomes filled with bytes from index <code>N</code> to <code>N+10</code>.
 *
 * <p/>
 * If you know that you are reading bytes from a <strong>strictly</strong> increasing or equal
 * index, then you should set the <code>autoAdvance</code> parameter to <code>true</code> in the
 * constructor. For complicated access patterns, or when you prefer to control the internal
 * buffer yourself, set <code>autoAdvance</code> to <code>false</code>. When
 * <code>autoAdvance</code> is enabled, every time an index is beyond the buffer length,
 * the buffer will be shifted forward such that the index requested becomes the first element in
 * the buffer.
 *
 * <p/>
 * All public methods with parameter <code>index</code> are absolute indexed. The index is from
 * the beginning of the wrapped input stream.
 */
public class InputStreamBuffer {

    private static final boolean DEBUG = false;
    private static final int DEBUG_MAX_BUFFER_SIZE = 80;
    private static final String TAG = InputStreamBuffer.class.getSimpleName();

    private InputStream mInputStream;
    private byte[] mBuffer;
    private boolean mAutoAdvance;
    /** Byte count the buffer is offset by. */
    private int mOffset = 0;
    /** Number of bytes filled in the buffer. */
    private int mFilled = 0;

    /**
     * Construct a new wrapper for an InputStream.
     *
     * <p/>
     * If <code>autoAdvance</code> is true, behavior is undefined if you call {@link #get(int)}
     * or {@link #has(int)} with an index N, then some arbitrary time later call {@link #get(int)}
     * or {@link #has(int)} with an index M < N. The wrapper may return the right value,
     * if the buffer happens to still contain index M, but more likely it will throw an
     * {@link IllegalStateException}.
     *
     * <p/>
     * If <code>autoAdvance</code> is false, you must be diligent and call {@link #advanceTo(int)}
     * at the appropriate times to ensure that the internal buffer is not unnecessarily resized
     * and reallocated.
     *
     * @param inputStream The input stream to wrap. The input stream will not be closed by the
     *                    wrapper.
     * @param bufferSize  The initial size for the internal buffer. The buffer size should be
     *                    carefully chosen to avoid resizing and reallocating the internal buffer.
     *                    The internal buffer size used will be the least power of two greater
     *                    than this parameter.
     * @param autoAdvance Determines the behavior when you need to read an index that is beyond
     *                    the internal buffer size. If true, the internal buffer will shift so
     *                    that the requested index becomes the first element. If false,
     *                    the internal buffer size will grow to the smallest power of 2 which is
     *                    greater than the requested index.
     */
    public InputStreamBuffer(final InputStream inputStream, int bufferSize,
            final boolean autoAdvance) {
        mInputStream = inputStream;
        if (bufferSize <= 0) {
            throw new IllegalArgumentException(
                    String.format("Buffer size %d must be positive.", bufferSize));
        }
        bufferSize = leastPowerOf2(bufferSize);
        mBuffer = new byte[bufferSize];
        mAutoAdvance = autoAdvance;
    }

    /**
     * Attempt to get byte at the requested index from the wrapped input stream. If the internal
     * buffer contains the requested index, return immediately. If the index is less than the
     * head of the buffer, or the index is greater or equal to the size of the wrapped input stream,
     * a runtime exception is thrown.
     *
     * <p/>
     * If the index is not in the internal buffer, but it can be requested from the input stream,
     * {@link #fill(int)} will be called first, and the byte at the index returned.
     *
     * <p/>
     * You should always call {@link #has(int)} with the same index, unless you are sure that no
     * exceptions will be thrown as described above.
     *
     * <p/>
     * Consider calling {@link #advanceTo(int)} if you know that you will never request a lesser
     * index in the future.
     * @param index The requested index.
     * @return The byte at that index.
     */
    public byte get(final int index) throws IllegalStateException, IndexOutOfBoundsException {
        Trace.beginSection("get");
        if (has(index)) {
            final int i = index - mOffset;
            Trace.endSection();
            return mBuffer[i];
        } else {
            Trace.endSection();
            throw new IndexOutOfBoundsException(
                    String.format("Index %d beyond length.", index));
        }
    }

    /**
     * Attempt to return whether the requested index is within the size of the wrapped input
     * stream. One side effect is {@link #fill(int)} will be called.
     *
     * <p/>
     * If this method returns true, it is guaranteed that {@link #get(int)} with the same index
     * will not fail. That means that if the requested index is within the size of the wrapped
     * input stream, but the index is less than the head of the internal buffer,
     * a runtime exception is thrown.
     *
     * <p/>
     * See {@link #get(int)} for caveats. A lot of the same warnings about exceptions and
     * <code>advanceTo()</code> apply.
     * @param index The requested index.
     * @return True if requested index is within the size of the wrapped input stream. False if
     * the index is beyond the size.
     */
    public boolean has(final int index) throws IllegalStateException, IndexOutOfBoundsException {
        Trace.beginSection("has");
        if (index < mOffset) {
            Trace.endSection();
            throw new IllegalStateException(
                    String.format("Index %d is before buffer %d", index, mOffset));
        }

        final int i = index - mOffset;

        // Requested index not in internal buffer.
        if (i >= mFilled || i >= mBuffer.length) {
            Trace.endSection();
            return fill(index);
        }

        Trace.endSection();
        return true;
    }

    /**
     * Attempts to advance the head of the buffer to the requested index. If the index is less
     * than the head of the buffer, the internal state will not be changed.
     *
     * <p/>
     * Advancing does not fill the internal buffer. The next {@link #get(int)} or
     * {@link #has(int)} call will fill the buffer.
     */
    public void advanceTo(final int index) throws IllegalStateException, IndexOutOfBoundsException {
        Trace.beginSection("advance to");
        final int i = index - mOffset;
        if (i <= 0) {
            // noop
            Trace.endSection();
            return;
        } else if (i < mFilled) {
            // Shift elements starting at i to position 0.
            shiftToBeginning(i);
            mOffset = index;
            mFilled = mFilled - i;
        } else if (mInputStream != null) {
            // Burn some bytes from the input stream to match the new index.
            int burn = i - mFilled;
            boolean empty = false;
            int fails = 0;
            try {
                while (burn > 0) {
                    final long burned = mInputStream.skip(burn);
                    if (burned <= 0) {
                        fails++;
                    } else {
                        burn -= burned;
                    }

                    if (fails >= 5) {
                        empty = true;
                        break;
                    }
                }
            } catch (IOException ignored) {
                empty = true;
            }

            if (empty) {
                //Mark input stream as consumed.
                mInputStream = null;
            }

            mOffset = index - burn;
            mFilled = 0;
        } else {
            // Advancing beyond the input stream.
            mOffset = index;
            mFilled = 0;
        }

        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, String.format("advanceTo %d buffer: %s", i, this));
        }
        Trace.endSection();
    }

    /**
     * Attempt to fill the internal buffer fully. The buffer will be modified such that the
     * requested index will always be in the buffer. If the index is less
     * than the head of the buffer, a runtime exception is thrown.
     *
     * <p/>
     * If the requested index is already in bounds of the buffer, then the buffer will just be
     * filled.
     *
     * <p/>
     * Otherwise, if <code>autoAdvance</code> was set to true in the constructor,
     * {@link #advanceTo(int)} will be called with the requested index,
     * and then the buffer filled. If <code>autoAdvance</code> was set to false,
     * we allocate a single larger buffer of a least multiple-of-two size that can contain the
     * requested index. The elements in the old buffer are copied over to the head of the new
     * buffer. Then the entire buffer is filled.
     * @param index The requested index.
     * @return True if the byte at the requested index has been filled. False if the wrapped
     * input stream ends before we reach the index.
     */
    private boolean fill(final int index) {
        Trace.beginSection("fill");
        if (index < mOffset) {
            Trace.endSection();
            throw new IllegalStateException(
                    String.format("Index %d is before buffer %d", index, mOffset));
        }

        int i = index - mOffset;
        // Can't fill buffer anymore if input stream is consumed.
        if (mInputStream == null) {
            Trace.endSection();
            return false;
        }

        // Increase buffer size if necessary.
        int length = i + 1;
        if (length > mBuffer.length) {
            if (mAutoAdvance) {
                advanceTo(index);
                i = index - mOffset;
            } else {
                length = leastPowerOf2(length);
                Log.w(TAG, String.format(
                        "Increasing buffer length from %d to %d. Bad buffer size chosen, "
                                + "or advanceTo() not called.",
                        mBuffer.length, length));
                mBuffer = Arrays.copyOf(mBuffer, length);
            }
        }

        // Read from input stream to fill buffer.
        int read = -1;
        try {
            read = mInputStream.read(mBuffer, mFilled, mBuffer.length - mFilled);
        } catch (IOException ignored) {
        }

        if (read != -1) {
            mFilled = mFilled + read;
        } else {
            // Mark input stream as consumed.
            mInputStream = null;
        }

        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, String.format("fill %d      buffer: %s", i, this));
        }

        Trace.endSection();
        return i < mFilled;
    }

    /**
     * Modify the internal buffer so that all the bytes are shifted towards the head by
     * <code>i</code>. In other words, the byte at index <code>i</code> will now be at index
     * <code>0</code>. Bytes from a lesser index are tossed.
     * @param i How much to shift left.
     */
    private void shiftToBeginning(final int i) {
        if (i >= mBuffer.length) {
            throw new IndexOutOfBoundsException(
                    String.format("Index %d out of bounds. Length %d", i, mBuffer.length));
        }
        for (int j = 0; j + i < mFilled; j++) {
            mBuffer[j] = mBuffer[j + i];
        }
    }

    @Override
    public String toString() {
        if (DEBUG) {
            return toDebugString();
        }
        return String.format("+%d+%d [%d]", mOffset, mBuffer.length, mFilled);
    }

    public String toDebugString() {
        Trace.beginSection("to debug string");
        final StringBuilder sb = new StringBuilder();
        sb.append("+").append(mOffset);
        sb.append("+").append(mBuffer.length);
        sb.append(" [");
        for (int i = 0; i < mBuffer.length && i < DEBUG_MAX_BUFFER_SIZE; i++) {
            if (i > 0) {
                sb.append(",");
            }
            if (i < mFilled) {
                sb.append(String.format("%02X", mBuffer[i]));
            } else {
                sb.append("__");
            }
        }
        if (mInputStream != null) {
            sb.append("...");
        }
        sb.append("]");

        Trace.endSection();
        return sb.toString();
    }

    /**
     * Calculate the least power of two greater than or equal to the input.
     */
    private static int leastPowerOf2(int n) {
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;
        return n;
    }
}