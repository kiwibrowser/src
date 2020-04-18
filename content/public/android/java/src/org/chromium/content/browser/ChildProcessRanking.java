// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import org.chromium.base.process_launcher.ChildProcessConnection;
import org.chromium.content_public.browser.ChildProcessImportance;

import java.util.Arrays;
import java.util.Comparator;

/**
 * Ranking of ChildProcessConnections for a particular ChildConnectionAllocator.
 */
public class ChildProcessRanking {
    private static class ConnectionWithRank {
        public final ChildProcessConnection connection;

        // Info for ranking a connection.
        public boolean foreground;
        public long frameDepth;
        @ChildProcessImportance
        public int importance;

        public ConnectionWithRank(ChildProcessConnection connection, boolean foreground,
                long frameDepth, @ChildProcessImportance int importance) {
            this.connection = connection;
            this.foreground = foreground;
            this.frameDepth = frameDepth;
            this.importance = importance;
        }
    }

    private static class RankComparator implements Comparator<ConnectionWithRank> {
        @Override
        public int compare(ConnectionWithRank o1, ConnectionWithRank o2) {
            // Sort null to the end.
            if (o1 == null && o2 == null) {
                return 0;
            } else if (o1 == null && o2 != null) {
                return 1;
            } else if (o1 != null && o2 == null) {
                return -1;
            }

            assert o1 != null;
            assert o2 != null;

            // Ranking order:
            // * foreground or ChildProcessImportance.IMPORTANT
            // * ChildProcessImportance.MODERATE
            // * frameDepth (lower value is higher rank)
            // Note boostForPendingViews is not used for ranking.

            boolean o1IsForeground =
                    o1.foreground || o1.importance == ChildProcessImportance.IMPORTANT;
            boolean o2IsForeground =
                    o2.foreground || o2.importance == ChildProcessImportance.IMPORTANT;

            if (o1IsForeground && !o2IsForeground) {
                return -1;
            } else if (!o1IsForeground && o2IsForeground) {
                return 1;
            }

            boolean o1IsModerate = o1.importance == ChildProcessImportance.MODERATE;
            boolean o2IsModerate = o2.importance == ChildProcessImportance.MODERATE;
            if (o1IsModerate && !o2IsModerate) {
                return -1;
            } else if (!o1IsModerate && o2IsModerate) {
                return 1;
            }

            return (int) (o1.frameDepth - o2.frameDepth);
        }
    }

    private static final RankComparator COMPARATOR = new RankComparator();

    // Not the most theoretically efficient data structure, but is good enough
    // for sizes in production and more memory efficient than linked data structures.
    private final ConnectionWithRank mRankings[];
    private int mSize;

    public ChildProcessRanking(int maxSize) {
        mRankings = new ConnectionWithRank[maxSize];
    }

    public void addConnection(ChildProcessConnection connection, boolean foreground,
            long frameDepth, @ChildProcessImportance int importance) {
        assert connection != null;
        assert indexOf(connection) == -1;
        assert mSize < mRankings.length;
        mRankings[mSize] = new ConnectionWithRank(connection, foreground, frameDepth, importance);
        mSize++;
        sort();
    }

    public void removeConnection(ChildProcessConnection connection) {
        assert connection != null;
        assert mSize > 0;
        int i = indexOf(connection);
        assert i != -1;

        // Null is sorted to the end.
        mRankings[i] = null;
        sort();
        mSize--;
    }

    public void updateConnection(ChildProcessConnection connection, boolean foreground,
            long frameDepth, @ChildProcessImportance int importance) {
        assert connection != null;
        assert mSize > 0;
        int i = indexOf(connection);
        assert i != -1;

        mRankings[i].foreground = foreground;
        mRankings[i].frameDepth = frameDepth;
        mRankings[i].importance = importance;
        sort();
    }

    public ChildProcessConnection getLowestRankedConnection() {
        if (mSize < 1) return null;
        return mRankings[mSize - 1].connection;
    }

    private int indexOf(ChildProcessConnection connection) {
        for (int i = 0; i < mSize; ++i) {
            if (mRankings[i].connection == connection) return i;
        }
        return -1;
    }

    private void sort() {
        // Sort is stable and linear when array is mostly sorted.
        Arrays.sort(mRankings, 0, mSize, COMPARATOR);
    }
}
