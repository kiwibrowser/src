// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.contextual_suggestions.ContextualSuggestionsModel.PropertyKey;
import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor;

/**
 * Coordinator for the toolbar sub-component. Responsible for communication with the parent
 * {@link ContextualSuggestionsCoordinator} and lifecycle of sub-component objects.
 */
class ToolbarCoordinator {
    private final ContextualSuggestionsModel mModel;
    private ToolbarView mToolbarView;
    private PropertyModelChangeProcessor<ContextualSuggestionsModel, ToolbarView, PropertyKey>
            mModelChangeProcessor;

    /**
     * Construct a new {@link ToolbarCoordinator}.
     * @param context The {@link Context} used to retrieve resources.
     * @param parentView The parent {@link View} to which the content will eventually be attached.
     * @param model The {@link ContextualSuggestionsModel} for the component.
     */
    ToolbarCoordinator(Context context, ViewGroup parentView, ContextualSuggestionsModel model) {
        mModel = model;

        mToolbarView = (ToolbarView) LayoutInflater.from(context).inflate(
                R.layout.contextual_suggestions_toolbar, parentView, false);

        mModelChangeProcessor =
                new PropertyModelChangeProcessor<>(mModel, mToolbarView, new ToolbarViewBinder());
        mModel.addObserver(mModelChangeProcessor);

        // The ToolbarCoordinator is created dynamically as needed, so the initial model state
        // needs to be bound on creation.
        mModelChangeProcessor.onPropertyChanged(mModel, PropertyKey.CLOSE_BUTTON_ON_CLICK_LISTENER);
        mModelChangeProcessor.onPropertyChanged(mModel, PropertyKey.MENU_BUTTON_VISIBILITY);
        mModelChangeProcessor.onPropertyChanged(mModel, PropertyKey.MENU_BUTTON_DELEGATE);
        mModelChangeProcessor.onPropertyChanged(mModel, PropertyKey.TITLE);
        mModelChangeProcessor.onPropertyChanged(
                mModel, PropertyKey.DEFAULT_TOOLBAR_ON_CLICK_LISTENER);
        mModelChangeProcessor.onPropertyChanged(mModel, PropertyKey.SLIM_PEEK_ENABLED);
    }

    /** @return The content {@link View}. */
    View getView() {
        return mToolbarView;
    }

    /** Destroy the toolbar component. */
    void destroy() {
        // The model outlives the toolbar sub-component. Remove the observer so that this object
        // can be garbage collected.
        mModel.removeObserver(mModelChangeProcessor);
    }
}
