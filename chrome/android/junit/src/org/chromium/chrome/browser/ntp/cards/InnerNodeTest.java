// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;

import android.support.annotation.Nullable;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder.PartialBindCallback;

import java.util.ArrayList;
import java.util.List;

/**
 * JUnit tests for {@link InnerNode}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class InnerNodeTest {
    private static final int[] ITEM_COUNTS = {1, 2, 3, 0, 3, 2, 1};
    private final List<TreeNode> mChildren = new ArrayList<>();
    @Mock private NodeParent mParent;
    private InnerNode mInnerNode;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mInnerNode = spy(new InnerNode());

        for (int childItemCount : ITEM_COUNTS) {
            TreeNode child = makeDummyNode(childItemCount);
            mChildren.add(child);
            mInnerNode.addChild(child);
        }

        mInnerNode.setParent(mParent);
    }

    @Test
    public void testItemCount() {
        assertThat(mInnerNode.getItemCount(), is(12));
    }

    @Test
    public void testGetItemViewType() {
        when(mChildren.get(0).getItemViewType(0)).thenReturn(42);
        when(mChildren.get(2).getItemViewType(2)).thenReturn(23);
        when(mChildren.get(4).getItemViewType(0)).thenReturn(4711);
        when(mChildren.get(6).getItemViewType(0)).thenReturn(1729);

        assertThat(mInnerNode.getItemViewType(0), is(42));
        assertThat(mInnerNode.getItemViewType(5), is(23));
        assertThat(mInnerNode.getItemViewType(6), is(4711));
        assertThat(mInnerNode.getItemViewType(11), is(1729));
    }

    @Test
    public void testBindViewHolder() {
        NewTabPageViewHolder holder = mock(NewTabPageViewHolder.class);
        mInnerNode.onBindViewHolder(holder, 0);
        mInnerNode.onBindViewHolder(holder, 5);
        mInnerNode.onBindViewHolder(holder, 6);
        mInnerNode.onBindViewHolder(holder, 11);

        verify(mChildren.get(0)).onBindViewHolder(holder, 0);
        verify(mChildren.get(2)).onBindViewHolder(holder, 2);
        verify(mChildren.get(4)).onBindViewHolder(holder, 0);
        verify(mChildren.get(6)).onBindViewHolder(holder, 0);
    }

    @Test
    public void testAddChild() {
        final int itemCountBefore = mInnerNode.getItemCount();

        TreeNode child = makeDummyNode(23);
        mInnerNode.addChild(child);

        // The child should have been initialized and the parent hierarchy notified about it.
        verify(child).setParent(eq(mInnerNode));
        verify(mParent).onItemRangeInserted(mInnerNode, itemCountBefore, 23);

        TreeNode child2 = makeDummyNode(0);
        mInnerNode.addChild(child2);

        // The empty child should have been initialized, but there should be no change
        // notifications.
        verify(child2).setParent(eq(mInnerNode));
        verifyNoMoreInteractions(mParent);
    }

    @Test
    public void testRemoveChild() {
        TreeNode child = mChildren.get(4);
        mInnerNode.removeChild(child);

        // The parent should have been notified about the removed items.
        verify(mParent).onItemRangeRemoved(mInnerNode, 6, 3);
        verify(child).detach();

        reset(mParent); // Prepare for the #verifyNoMoreInteractions() call below.
        TreeNode child2 = mChildren.get(3);
        mInnerNode.removeChild(child2);
        verify(child2).detach();

        // There should be no change notifications about the empty child.
        verifyNoMoreInteractions(mParent);
    }

    @Test
    public void testRemoveChildren() {
        mInnerNode.removeChildren();

        // The parent should have been notified about the removed items.
        verify(mParent).onItemRangeRemoved(mInnerNode, 0, 12);
        for (TreeNode child : mChildren) verify(child).detach();
    }

    @Test
    public void testNotifications() {
        mInnerNode.onItemRangeChanged(mChildren.get(0), 0, 1, null);
        when(mChildren.get(2).getItemCount()).thenReturn(5);
        mInnerNode.onItemRangeInserted(mChildren.get(2), 2, 2);
        when(mChildren.get(4).getItemCount()).thenReturn(0);
        mInnerNode.onItemRangeRemoved(mChildren.get(4), 0, 3);
        mInnerNode.onItemRangeChanged(mChildren.get(6), 0, 1, null);

        verify(mParent).onItemRangeChanged(mInnerNode, 0, 1, null);
        verify(mParent).onItemRangeInserted(mInnerNode, 5, 2);
        verify(mParent).onItemRangeRemoved(mInnerNode, 8, 3);
        verify(mParent).onItemRangeChanged(mInnerNode, 10, 1, null);

        assertThat(mInnerNode.getItemCount(), is(11));
    }

    /**
     * Tests that {@link ChildNode} sends the change notifications AFTER its child list is modified.
     */
    @Test
    public void testChangeNotificationsTiming() {
        // The MockModeParent will enforce a given number of items in the child when notified.
        MockNodeParent parent = spy(new MockNodeParent());
        InnerNode rootNode = new InnerNode();

        TreeNode[] children = {makeDummyNode(3), makeDummyNode(5)};
        rootNode.addChildren(children);
        rootNode.setParent(parent);

        assertThat(rootNode.getItemCount(), is(8));
        verifyZeroInteractions(parent); // Before the parent is set, no notifications.

        parent.expectItemCount(24);
        rootNode.addChildren(makeDummyNode(7), makeDummyNode(9)); // Should bundle the insertions.
        verify(parent).onItemRangeInserted(eq(rootNode), eq(8), eq(16));

        parent.expectItemCount(28);
        rootNode.addChild(makeDummyNode(4));
        verify(parent).onItemRangeInserted(eq(rootNode), eq(24), eq(4));

        parent.expectItemCount(23);
        rootNode.removeChild(children[1]);
        verify(parent).onItemRangeRemoved(eq(rootNode), eq(3), eq(5));

        parent.expectItemCount(0);
        rootNode.removeChildren(); // Bundles the removals in a single change notification
        verify(parent).onItemRangeRemoved(eq(rootNode), eq(0), eq(23));
    }

    /**
     * Implementation of {@link NodeParent} that checks the item count from the node that
     * sends notifications against defined expectations. Fails on unexpected calls.
     */
    private static class MockNodeParent implements NodeParent {
        @Nullable
        private Integer mNextExpectedItemCount;

        public void expectItemCount(int count) {
            mNextExpectedItemCount = count;
        }

        @Override
        public void onItemRangeChanged(
                TreeNode child, int index, int count, PartialBindCallback callback) {
            checkCount(child);
        }

        @Override
        public void onItemRangeInserted(TreeNode child, int index, int count) {
            checkCount(child);
        }

        @Override
        public void onItemRangeRemoved(TreeNode child, int index, int count) {
            checkCount(child);
        }

        private void checkCount(TreeNode child) {
            if (mNextExpectedItemCount == null) fail("Unexpected call");
            assertThat(child.getItemCount(), is(mNextExpectedItemCount));
            mNextExpectedItemCount = null;
        }
    }

    private static TreeNode makeDummyNode(int itemCount) {
        TreeNode node = mock(TreeNode.class);
        when(node.getItemCount()).thenReturn(itemCount);
        return node;
    }
}
