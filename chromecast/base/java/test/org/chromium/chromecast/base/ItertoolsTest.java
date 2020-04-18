// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.emptyIterable;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import org.chromium.chromecast.base.Inheritance.Base;
import org.chromium.chromecast.base.Inheritance.Derived;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for Itertools.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class ItertoolsTest {
    @Test
    public void testReverse() {
        List<String> emptyList = new ArrayList<String>();
        assertThat(Itertools.reverse(emptyList), emptyIterable());
        List<String> singleItem = new ArrayList<>();
        singleItem.add("a");
        assertThat(Itertools.reverse(singleItem), contains("a"));
        List<String> threeItems = new ArrayList<>();
        threeItems.add("a");
        threeItems.add("b");
        threeItems.add("c");
        assertThat(Itertools.reverse(threeItems), contains("c", "b", "a"));
    }

    @Test
    public void testAssignReversedToIterableOfSuperclass() {
        // Compile error if the generics are wrong.
        Iterable<Base> reversed = Itertools.reverse(new ArrayList<Derived>());
    }
}
