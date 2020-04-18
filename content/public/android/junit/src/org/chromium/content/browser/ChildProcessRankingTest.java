// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.ComponentName;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.process_launcher.ChildProcessConnection;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.TestChildProcessConnection;
import org.chromium.content_public.browser.ChildProcessImportance;

/** Unit tests for ChildProessRanking */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ChildProcessRankingTest {
    private ChildProcessConnection createConnection() {
        return new TestChildProcessConnection(new ComponentName("pkg", "cls"),
                false /* bindToCallerCheck */, false /* bindAsExternalService */,
                null /* serviceBundle */);
    }

    private void assertRankingAndRemoveAll(
            ChildProcessRanking ranking, ChildProcessConnection[] connections) {
        for (int i = connections.length - 1; i >= 0; i--) {
            Assert.assertEquals(connections[i], ranking.getLowestRankedConnection());
            ranking.removeConnection(connections[i]);
        }
        Assert.assertNull(ranking.getLowestRankedConnection());
    }

    @Test
    public void testRanking() {
        ChildProcessConnection c1 = createConnection();
        ChildProcessConnection c2 = createConnection();
        ChildProcessConnection c3 = createConnection();
        ChildProcessConnection c4 = createConnection();

        ChildProcessRanking ranking = new ChildProcessRanking(4);

        // Insert in lowest ranked to highest ranked order.
        ranking.addConnection(
                c1, false /* foreground */, 1 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c2, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c3, true /* foreground */, 1 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c4, true /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);

        assertRankingAndRemoveAll(ranking, new ChildProcessConnection[] {c4, c3, c2, c1});
    }

    @Test
    public void testRankingWithImportance() {
        ChildProcessConnection c1 = createConnection();
        ChildProcessConnection c2 = createConnection();
        ChildProcessConnection c3 = createConnection();
        ChildProcessConnection c4 = createConnection();

        ChildProcessRanking ranking = new ChildProcessRanking(4);

        // Insert in lowest ranked to highest ranked order.
        ranking.addConnection(
                c1, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c2, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.MODERATE);
        ranking.addConnection(
                c3, false /* foreground */, 1 /* frameDepth */, ChildProcessImportance.IMPORTANT);
        ranking.addConnection(
                c4, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.IMPORTANT);

        assertRankingAndRemoveAll(ranking, new ChildProcessConnection[] {c4, c3, c2, c1});
    }

    @Test
    public void testUpdate() {
        ChildProcessConnection c1 = createConnection();
        ChildProcessConnection c2 = createConnection();
        ChildProcessConnection c3 = createConnection();
        ChildProcessConnection c4 = createConnection();

        ChildProcessRanking ranking = new ChildProcessRanking(4);

        // c1,2 are in one tab, and c3,4 are in second tab.
        ranking.addConnection(
                c1, true /* foreground */, 1 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c2, true /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c3, false /* foreground */, 1 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c4, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        Assert.assertEquals(c3, ranking.getLowestRankedConnection());

        // Switch from tab c1,2 to tab c3,c4.
        ranking.updateConnection(
                c1, false /* foreground */, 1 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.updateConnection(
                c2, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.updateConnection(
                c3, true /* foreground */, 1 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.updateConnection(
                c4, true /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);

        assertRankingAndRemoveAll(ranking, new ChildProcessConnection[] {c4, c3, c2, c1});
    }

    @Test
    public void testStability() {
        ChildProcessConnection c1 = createConnection();
        ChildProcessConnection c2 = createConnection();
        ChildProcessConnection c3 = createConnection();
        ChildProcessConnection c4 = createConnection();

        ChildProcessRanking ranking = new ChildProcessRanking(4);

        // Each connection is its own tab.
        ranking.addConnection(
                c1, true /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c2, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c3, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.addConnection(
                c4, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);

        // Tab through each connection.
        ranking.updateConnection(
                c2, true /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.updateConnection(
                c1, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);

        ranking.updateConnection(
                c3, true /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.updateConnection(
                c2, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);

        ranking.updateConnection(
                c4, true /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);
        ranking.updateConnection(
                c3, false /* foreground */, 0 /* frameDepth */, ChildProcessImportance.NORMAL);

        assertRankingAndRemoveAll(ranking, new ChildProcessConnection[] {c4, c3, c2, c1});
    }
}
