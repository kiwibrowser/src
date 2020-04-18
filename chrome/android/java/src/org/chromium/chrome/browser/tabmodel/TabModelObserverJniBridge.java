// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tabmodel;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;

import java.util.List;

/**
 * An implementation of TabModelObserver that forwards notifications over a JNI bridge
 * to a corresponding native implementation. Objects of this type are created and owned by
 * the native TabModelJniBridge implementation when native observers are added.
 */
class TabModelObserverJniBridge implements TabModelObserver {
    /** Native TabModelObserverJniBridge pointer, set by the constructor. */
    private long mNativeTabModelObserverJniBridge;

    /** TabModel to which this observer is attached, set by the constructor. */
    private TabModel mTabModel;

    /** Constructor. Only intended to be used by the static create factory function.
     *
     * @param nativeTabModelObserverJniBridge The address of the corresponding native object.
     * @param tabModel The tab model to which the observer bridge will be associated.
     */
    private TabModelObserverJniBridge(long nativeTabModelObserverJniBridge, TabModel tabModel) {
        mNativeTabModelObserverJniBridge = nativeTabModelObserverJniBridge;
        mTabModel = tabModel;
    }

    // TabModelObserver implementation.
    // These simply forward events to the corresponding native implementation.

    @Override
    public final void didSelectTab(Tab tab, TabSelectionType type, int lastId) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeDidSelectTab(mNativeTabModelObserverJniBridge, tab, type.ordinal(), lastId);
    }

    @Override
    public final void willCloseTab(Tab tab, boolean animate) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeWillCloseTab(mNativeTabModelObserverJniBridge, tab, animate);
    }

    @Override
    public final void didCloseTab(int tabId, boolean incognito) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeDidCloseTab(mNativeTabModelObserverJniBridge, tabId, incognito);
    }

    @Override
    public final void willAddTab(Tab tab, TabLaunchType type) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeWillAddTab(mNativeTabModelObserverJniBridge, tab, type.ordinal());
    }

    @Override
    public final void didAddTab(Tab tab, TabLaunchType type) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeDidAddTab(mNativeTabModelObserverJniBridge, tab, type.ordinal());
    }

    @Override
    public final void didMoveTab(Tab tab, int newIndex, int curIndex) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeDidMoveTab(mNativeTabModelObserverJniBridge, tab, newIndex, curIndex);
    }

    @Override
    public final void tabPendingClosure(Tab tab) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeTabPendingClosure(mNativeTabModelObserverJniBridge, tab);
    }

    @Override
    public final void tabClosureUndone(Tab tab) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeTabClosureUndone(mNativeTabModelObserverJniBridge, tab);
    }

    @Override
    public final void tabClosureCommitted(Tab tab) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeTabClosureCommitted(mNativeTabModelObserverJniBridge, tab);
    }

    @Override
    public final void allTabsPendingClosure(List<Tab> tabs) {
        // Convert the List to an array of objects. This makes the corresponding C++ code much
        // easier.
        assert mNativeTabModelObserverJniBridge != 0;
        nativeAllTabsPendingClosure(mNativeTabModelObserverJniBridge, tabs.toArray());
    }

    @Override
    public final void allTabsClosureCommitted() {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeAllTabsClosureCommitted(mNativeTabModelObserverJniBridge);
    }

    @Override
    public final void tabRemoved(Tab tab) {
        assert mNativeTabModelObserverJniBridge != 0;
        nativeTabRemoved(mNativeTabModelObserverJniBridge, tab);
    }

    /**
     * Creates an observer bridge for the given tab model. The native counterpart to this object
     * will hold a global reference to the Java endpoint and manage its lifetime. This is private as
     * it is only intended to be called from native code.
     *
     * @param nativeTabModelObserverJniBridge The address of the corresponding native object.
     * @param tabModel The tab model to which the observer bridge will be associated.
     */
    @CalledByNative
    static private TabModelObserverJniBridge create(
            long nativeTabModelObserverJniBridge, TabModel tabModel) {
        TabModelObserverJniBridge bridge =
                new TabModelObserverJniBridge(nativeTabModelObserverJniBridge, tabModel);
        tabModel.addObserver(bridge);
        return bridge;
    }

    /**
     * Causes the observer to be removed from the associated tab model. The native counterpart calls
     * this prior to cleaning up its last reference to the Java endpoint so that it can be correctly
     * torn down.
     */
    @CalledByNative
    private void detachFromTabModel() {
        assert mNativeTabModelObserverJniBridge != 0;
        assert mTabModel != null;
        mTabModel.removeObserver(this);
        mNativeTabModelObserverJniBridge = 0;
        mTabModel = null;
    }

    // Native functions that are implemented by
    // browser/ui/android/tab_model/tab_model_observer_jni_bridge.*.
    private native void nativeDidSelectTab(
            long nativeTabModelObserverJniBridge, Tab tab, int type, int lastId);
    private native void nativeWillCloseTab(
            long nativeTabModelObserverJniBridge, Tab tab, boolean animate);
    private native void nativeDidCloseTab(
            long nativeTabModelObserverJniBridge, int tabId, boolean incognito);
    private native void nativeWillAddTab(long nativeTabModelObserverJniBridge, Tab tab, int type);
    private native void nativeDidAddTab(long nativeTabModelObserverJniBridge, Tab tab, int type);
    private native void nativeDidMoveTab(
            long nativeTabModelObserverJniBridge, Tab tab, int newIndex, int curIndex);
    private native void nativeTabPendingClosure(long nativeTabModelObserverJniBridge, Tab tab);
    private native void nativeTabClosureUndone(long nativeTabModelObserverJniBridge, Tab tab);
    private native void nativeTabClosureCommitted(long nativeTabModelObserverJniBridge, Tab tab);
    private native void nativeAllTabsPendingClosure(
            long nativeTabModelObserverJniBridge, Object[] tabs);
    private native void nativeAllTabsClosureCommitted(long nativeTabModelObserverJniBridge);
    private native void nativeTabRemoved(long nativeTabModelObserverJniBridge, Tab tab);
}
