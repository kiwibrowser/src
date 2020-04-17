// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor;
import org.chromium.chrome.browser.toolbar.TabSwitcherButtonModel.PropertyKey;

/**
 * This class is responsible for pushing updates to the Android view of the tab switcher. These
 * updates are pulled from the {@link TabSwitcherModel} when a notification of an update is
 * received.
 */
public class TabSwitcherButtonViewBinder
        implements PropertyModelChangeProcessor.ViewBinder<TabSwitcherButtonModel,
                TabSwitcherButtonView, TabSwitcherButtonModel.PropertyKey> {
    /**
     * Build a binder that handles interaction between the model and the views that make up the
     * tab switcher.
     */
    public TabSwitcherButtonViewBinder() {}

    @Override
    public final void bind(
            TabSwitcherButtonModel model, TabSwitcherButtonView view, PropertyKey propertyKey) {
        if (PropertyKey.NUMBER_OF_TABS == propertyKey) {
            view.updateTabCountVisuals(model.getNumberOfTabs());
        } else if (PropertyKey.ON_CLICK_LISTENER == propertyKey) {
            view.setOnClickListener(model.getOnClickListener());
        } else if (PropertyKey.ON_LONG_CLICK_LISTENER == propertyKey) {
            view.setOnLongClickListener(model.getOnLongClickListener());
        } else {
            assert false : "Unhandled property detected in TabSwitcherViewBinder!";
        }
    }
}
