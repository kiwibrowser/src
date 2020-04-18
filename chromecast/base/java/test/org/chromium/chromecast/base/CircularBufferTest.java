// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.emptyIterable;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

/**
 * Tests for CircularBuffer.
 *
 * Currently, the only supported use case is appending items to the buffer, and then iterating the
 * buffer. Concurrent modification while iterating, or appending items after iterating, is
 * undefined behavior as of now.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class CircularBufferTest {
    @Test
    public void testBufferWithNoCapacity() {
        CircularBuffer<String> buffer = new CircularBuffer<>(0);
        buffer.add("a");
        assertThat(buffer, emptyIterable());
    }

    @Test
    public void testBufferWithPartialCapacity() {
        CircularBuffer<String> buffer = new CircularBuffer<>(4);
        buffer.add("a");
        buffer.add("b");
        buffer.add("c");
        assertThat(buffer, contains("a", "b", "c"));
    }

    @Test
    public void testBufferWithFullCapacity() {
        CircularBuffer<String> buffer = new CircularBuffer<>(4);
        buffer.add("zero");
        buffer.add("one");
        buffer.add("two");
        buffer.add("three");
        assertThat(buffer, contains("zero", "one", "two", "three"));
    }

    @Test
    public void testBufferThatOverflowsCapacityWrapsAroundAndErasesOldestElements() {
        CircularBuffer<String> buffer = new CircularBuffer<>(4);
        buffer.add("1");
        buffer.add("2");
        buffer.add("3");
        buffer.add("4"); // Hits capacity; subsequent additions overwrite oldest elements.
        buffer.add("5"); // erases "1"
        buffer.add("6"); // erases "2"
        assertThat(buffer, contains("3", "4", "5", "6"));
    }

    @Test
    public void testBufferThatOverflowsTwice() {
        CircularBuffer<String> buffer = new CircularBuffer<>(2);
        buffer.add("a");
        buffer.add("b");
        buffer.add("c");
        buffer.add("d");
        buffer.add("e");
        // Since the capacity is 2, return the 2 most-recently added items.
        assertThat(buffer, contains("d", "e"));
    }
}
