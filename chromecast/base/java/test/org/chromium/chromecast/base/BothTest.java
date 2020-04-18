// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for Both.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class BothTest {
    @Test
    public void testAccessMembersOfBoth() {
        Both<Integer, Boolean> x = Both.both(10, true);
        assertEquals((int) x.first, 10);
        assertEquals((boolean) x.second, true);
    }

    @Test
    public void testDeeplyNestedBothType() {
        // Yes you can do this.
        Both<Both<Both<String, String>, String>, String> x =
                Both.both(Both.both(Both.both("A", "B"), "C"), "D");
        assertEquals(x.first.first.first, "A");
        assertEquals(x.first.first.second, "B");
        assertEquals(x.first.second, "C");
        assertEquals(x.second, "D");
    }

    @Test
    public void testBothToString() {
        Both<String, String> x = Both.both("a", "b");
        assertEquals(x.toString(), "a, b");
    }

    @Test
    public void testUseGetFirstAsMethodReference() {
        Both<Integer, String> x = Both.both(1, "one");
        Function<Both<Integer, String>, Integer> getFirst = Both::getFirst;
        assertEquals((int) getFirst.apply(x), 1);
    }

    @Test
    public void testUseGetSecondAsMethodReference() {
        Both<Integer, String> x = Both.both(2, "two");
        Function<Both<Integer, String>, String> getSecond = Both::getSecond;
        assertEquals(getSecond.apply(x), "two");
    }

    @Test
    public void testAdaptBiFunction() {
        String result = Both.adapt((String a, String b) -> a + b).apply(Both.both("a", "b"));
        assertEquals(result, "ab");
    }

    @Test
    public void testAdaptBiConsumer() {
        List<String> result = new ArrayList<>();
        Both.adapt((String a, String b) -> { result.add(a + b); }).accept(Both.both("A", "B"));
        assertThat(result, contains("AB"));
    }

    @Test
    public void testAdaptBiPredicate() {
        Predicate<Both<String, String>> p = Both.adapt(String::equals);
        assertTrue(p.test(Both.both("a", "a")));
        assertFalse(p.test(Both.both("a", "b")));
    }
}
