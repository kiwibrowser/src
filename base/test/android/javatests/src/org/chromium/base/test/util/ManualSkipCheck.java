// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import org.junit.runners.model.FrameworkMethod;

/**
 * Skips any test methods annotated with {@code @}{@link Manual}.
 */
public class ManualSkipCheck extends SkipCheck {
    @Override
    public boolean shouldSkip(FrameworkMethod testMethod) {
        return !AnnotationProcessingUtils.getAnnotations(testMethod.getMethod(), Manual.class)
                        .isEmpty();
    }
}
