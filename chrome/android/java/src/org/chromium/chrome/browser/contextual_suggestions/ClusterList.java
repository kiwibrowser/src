// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import org.chromium.chrome.browser.ntp.cards.InnerNode;

import java.util.List;

/**
 * A node in a tree containing a list of {@link ContextualSuggestionsCluster}s.
 */
class ClusterList extends InnerNode {
    private boolean mIsDestroyed;

    /**
     * Construct a new {@link ClusterList}.
     * @param clusters The list of clusters held by this ClusterList.
     */
    ClusterList(List<ContextualSuggestionsCluster> clusters) {
        for (ContextualSuggestionsCluster cluster : clusters) {
            addChild(cluster);
        }
    }

    /** Remove all clusters and detach itself from its parent. */
    void destroy() {
        assert !mIsDestroyed;
        mIsDestroyed = true;
        removeChildren();
        detach();
    }
}
