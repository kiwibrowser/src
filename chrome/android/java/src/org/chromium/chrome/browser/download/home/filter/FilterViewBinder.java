// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter;

import org.chromium.chrome.browser.download.home.filter.FilterModel.PropertyKey;
import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor.ViewBinder;

/**
 * A helper {@link ViewBinder} responsible for gluing {@link FilterModel} to
 * {@link FilterView}.
 */
class FilterViewBinder implements ViewBinder<FilterModel, FilterView, FilterModel.PropertyKey> {
    @Override
    public void bind(FilterModel model, FilterView view, PropertyKey propertyKey) {
        if (propertyKey == FilterModel.PropertyKey.CONTENT_VIEW) {
            view.setContentView(model.getContentView());
        } else if (propertyKey == FilterModel.PropertyKey.SELECTED_TAB) {
            view.setTabSelected(model.getSelectedTab());
        } else if (propertyKey == FilterModel.PropertyKey.CHANGE_LISTENER) {
            view.setTabSelectedCallback(model.getChangeListener());
        }
    }
}