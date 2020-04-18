// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import junit.framework.TestCase;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Unit tests for MinAndroidSdkLevelSkipCheck. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 18)
public class MinAndroidSdkLevelSkipCheckTest {
    private static class UnannotatedBaseClass extends TestCase {
        public UnannotatedBaseClass(String name) {
            super(name);
        }
        @MinAndroidSdkLevel(17) public void min17Method() {}
        @MinAndroidSdkLevel(20) public void min20Method() {}
    }

    @MinAndroidSdkLevel(17)
    private static class Min17Class extends UnannotatedBaseClass {
        public Min17Class(String name) {
            super(name);
        }
        public void unannotatedMethod() {}
    }

    @MinAndroidSdkLevel(20)
    private static class Min20Class extends UnannotatedBaseClass {
        public Min20Class(String name) {
            super(name);
        }
        public void unannotatedMethod() {}
    }

    private static class ExtendsMin17Class extends Min17Class {
        public ExtendsMin17Class(String name) {
            super(name);
        }
        @Override
        public void unannotatedMethod() {}
    }

    private static class ExtendsMin20Class extends Min20Class {
        public ExtendsMin20Class(String name) {
            super(name);
        }
        @Override
        public void unannotatedMethod() {}
    }

    @Test
    public void testAnnotatedMethodAboveMin() {
        Assert.assertFalse(new MinAndroidSdkLevelSkipCheck().shouldSkip(
                new UnannotatedBaseClass("min17Method")));
    }

    @Test
    public void testAnnotatedMethodBelowMin() {
        Assert.assertTrue(new MinAndroidSdkLevelSkipCheck().shouldSkip(
                new UnannotatedBaseClass("min20Method")));
    }

    @Test
    public void testAnnotatedClassAboveMin() {
        Assert.assertFalse(new MinAndroidSdkLevelSkipCheck().shouldSkip(
                new Min17Class("unannotatedMethod")));
    }

    @Test
    public void testAnnotatedClassBelowMin() {
        Assert.assertTrue(new MinAndroidSdkLevelSkipCheck().shouldSkip(
                new Min20Class("unannotatedMethod")));
    }

    @Test
    public void testAnnotatedSuperclassAboveMin() {
        Assert.assertFalse(new MinAndroidSdkLevelSkipCheck().shouldSkip(
                new ExtendsMin17Class("unannotatedMethod")));
    }

    @Test
    public void testAnnotatedSuperclassBelowMin() {
        Assert.assertTrue(new MinAndroidSdkLevelSkipCheck().shouldSkip(
                new ExtendsMin20Class("unannotatedMethod")));
    }
}
