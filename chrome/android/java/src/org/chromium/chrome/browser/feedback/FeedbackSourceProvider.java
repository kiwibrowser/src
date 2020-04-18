// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feedback;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Used for an external component to be able to provide extra feedback sources to the
 * FeedbackCollector.  In general it is best to add the sources directly to FeedbackCollector, but
 * if there are DEPS or repository reasons for not doing that, they can be provided through here.
 */
public interface FeedbackSourceProvider {
    // clang-format off
    // TODO(crbug.com/781015): Clang isn't formatting this correctly.
    /** @return A list of {@link FeedbackSource}s to add to a {@link FeedbackCollector}. */
    default Collection<FeedbackSource> getSynchronousSources() { return new ArrayList<>(); }

    /** @return A list of {@link AsyncFeedbackSource}s to add to a {@link FeedbackCollector}. */
    default Collection<AsyncFeedbackSource> getAsynchronousSources() { return new ArrayList<>(); }
        // clang-format on
}
