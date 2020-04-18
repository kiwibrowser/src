// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.annotation.Nullable;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.ViewStub;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.modelutil.LazyViewBinderAdapter;
import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor;

/**
 * Creates and owns all elements which are part of the accessory sheet component.
 * It's part of the controller but will mainly forward events (like showing the sheet) and handle
 * communication with the {@link ManualFillingCoordinator} (e.g. add a tab to trigger the sheet).
 * to the {@link AccessorySheetMediator}.
 */
public class AccessorySheetCoordinator {
    private final AccessorySheetMediator mMediator;

    /**
     * Creates the sheet component by instantiating Model, View and Controller before wiring these
     * parts up.
     * @param viewStub The view stub that can be inflated into the accessory layout.
     */
    public AccessorySheetCoordinator(ViewStub viewStub) {
        LazyViewBinderAdapter.StubHolder<ViewPager> stubHolder =
                new LazyViewBinderAdapter.StubHolder<>(viewStub);
        AccessorySheetModel model = new AccessorySheetModel();
        model.addObserver(new PropertyModelChangeProcessor<>(
                model, stubHolder, new LazyViewBinderAdapter<>(new AccessorySheetViewBinder())));
        mMediator = new AccessorySheetMediator(model);
    }

    /**
     * Creates the {@link PagerAdapter} for the newly inflated {@link ViewPager}.
     * If any ListModelChangeProcessor<> is needed, it would be created here. Currently, connecting
     * the model.getTabList() to the tabViewBinder would have no effect as only the change of
     * ACTIVE_TAB affects the view.
     * @param model The model containing the list of tabs to be displayed.
     * @return A fully initialized {@link PagerAdapter}.
     */
    static PagerAdapter createTabViewAdapter(AccessorySheetModel model) {
        return new AccessoryPagerAdapter(model.getTabList());
    }

    /**
     * Adds the contents of a given {@link KeyboardAccessoryData.Tab} to the accessory sheet. If it
     * is the first Tab, it automatically becomes the active Tab.
     * Careful, if you want to show this tab as icon in the KeyboardAccessory, use the method
     * {@link ManualFillingCoordinator#addTab(KeyboardAccessoryData.Tab)} instead.
     * @param tab The tab which should be added to the AccessorySheet.
     */
    void addTab(KeyboardAccessoryData.Tab tab) {
        mMediator.addTab(tab);
    }

    /**
     * Returns a {@link KeyboardAccessoryData.Tab} object that is used to display this bottom sheet.
     * @return Returns a {@link KeyboardAccessoryData.Tab}.
     */
    @Nullable
    public KeyboardAccessoryData.Tab getTab() {
        return mMediator.getTab();
    }

    @VisibleForTesting
    AccessorySheetMediator getMediatorForTesting() {
        return mMediator;
    }
}