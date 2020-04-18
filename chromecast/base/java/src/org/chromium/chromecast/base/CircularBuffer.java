// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * An Iterable object that stores up to a fixed amount of objects, and overwrites the least-recently
 * inserted object if it exceeds its capacity. Appending, removing, and iterating are all constant
 * time.
 *
 * This class is intended as a fast Iterable Deque with a fixed capacity. LinkedList generates lint
 * warnings because every item requires a heap allocation for the Node, ArrayList requires rewriting
 * the list when the head is removed, and ArrayDeque is not fixed-size.
 *
 * Currently, the only supported use case is appending items to the buffer, and then iterating the
 * buffer. Concurrent modification while iterating, or appending items after iterating, is
 * undefined behavior as of now.
 *
 * @param <T> The type that elements of the buffer should be instances of.
 */
public class CircularBuffer<T> implements Iterable<T> {
    private final List<T> mData;
    private final int mSize;
    private boolean mAtCapacity;
    private int mHeadPosition;
    private int mTailPosition;

    public CircularBuffer(int size) {
        mData = new ArrayList<T>(size);
        mSize = size;
        mAtCapacity = false;
        mHeadPosition = 0;
        mTailPosition = 0;
    }

    public void add(T item) {
        if (mSize == 0) {
            return;
        }
        if (mAtCapacity) {
            mData.set(mHeadPosition, item);
            mHeadPosition = increment(mHeadPosition);
        } else {
            mData.add(item);
        }
        mTailPosition = increment(mTailPosition);
        if (mTailPosition == mHeadPosition) {
            mAtCapacity = true;
        }
    }

    @Override
    public Iterator<T> iterator() {
        return new Iterator<T>() {
            @Override
            public boolean hasNext() {
                return mAtCapacity || mHeadPosition != mTailPosition;
            }

            @Override
            public T next() {
                T result = mData.get(mHeadPosition);
                mHeadPosition = increment(mHeadPosition);
                mAtCapacity = false;
                return result;
            }
        };
    }

    private int increment(int position) {
        return (position + 1) % mSize;
    }
}
