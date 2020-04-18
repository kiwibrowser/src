// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.v4.view.ViewPager;
import android.view.View;

import org.chromium.chrome.browser.autofill.keyboard_accessory.AccessorySheetModel.PropertyKey;
import org.chromium.chrome.browser.modelutil.LazyViewBinderAdapter;

/**
 * Observes {@link AccessorySheetModel} changes (like a newly available tab) and triggers the
 * {@link AccessorySheetViewBinder} which will modify the view accordingly.
 */
class AccessorySheetViewBinder
        implements LazyViewBinderAdapter
                           .SimpleViewBinder<AccessorySheetModel, ViewPager, PropertyKey> {
    @Override
    public PropertyKey getVisibilityProperty() {
        return PropertyKey.VISIBLE;
    }

    @Override
    public boolean isVisible(AccessorySheetModel model) {
        return model.isVisible();
    }

    @Override
    public void onInitialInflation(AccessorySheetModel model, ViewPager inflatedView) {
        if (model.getActiveTabIndex() != -1) inflatedView.setCurrentItem(model.getActiveTabIndex());
        inflatedView.setAdapter(AccessorySheetCoordinator.createTabViewAdapter(model));
    }

    @Override
    public void bind(AccessorySheetModel model, ViewPager inflatedView, PropertyKey propertyKey) {
        if (propertyKey == PropertyKey.VISIBLE) {
            inflatedView.setVisibility(model.isVisible() ? View.VISIBLE : View.GONE);
            return;
        }
        if (propertyKey == PropertyKey.ACTIVE_TAB_INDEX) {
            if (model.getActiveTabIndex() != AccessorySheetModel.NO_ACTIVE_TAB) {
                inflatedView.setCurrentItem(model.getActiveTabIndex());
            }
            return;
        }
        assert false : "Every possible property update needs to be handled!";
    }
}
