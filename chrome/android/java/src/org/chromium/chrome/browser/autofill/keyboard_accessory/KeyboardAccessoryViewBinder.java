// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.keyboard_accessory.KeyboardAccessoryData.Action;
import org.chromium.chrome.browser.autofill.keyboard_accessory.KeyboardAccessoryData.Tab;
import org.chromium.chrome.browser.autofill.keyboard_accessory.KeyboardAccessoryModel.PropertyKey;
import org.chromium.chrome.browser.modelutil.LazyViewBinderAdapter;
import org.chromium.chrome.browser.modelutil.ListModelChangeProcessor;
import org.chromium.chrome.browser.modelutil.RecyclerViewAdapter;
import org.chromium.chrome.browser.modelutil.SimpleListObservable;
import org.chromium.ui.widget.ButtonCompat;

/**
 * Observes {@link KeyboardAccessoryModel} changes (like a newly available tab) and triggers the
 * {@link KeyboardAccessoryViewBinder} which will modify the view accordingly.
 */
class KeyboardAccessoryViewBinder
        implements LazyViewBinderAdapter.SimpleViewBinder<KeyboardAccessoryModel,
                KeyboardAccessoryView, PropertyKey> {
    static class ActionViewBinder
            implements RecyclerViewAdapter.ViewBinder<SimpleListObservable<Action>,
                    ActionViewBinder.ViewHolder> {
        static class ViewHolder extends RecyclerView.ViewHolder {
            public ViewHolder(ButtonCompat actionView) {
                super(actionView);
            }

            ButtonCompat getActionView() {
                return (ButtonCompat) super.itemView;
            }
        }

        @Override
        public ActionViewBinder.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            return new ViewHolder(
                    (ButtonCompat) LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.keyboard_accessory_action, parent, false));
        }

        @Override
        public void onBindViewHolder(
                SimpleListObservable<Action> actions, ViewHolder holder, int position) {
            final Action action = actions.get(position);
            holder.getActionView().setText(action.getCaption());
            holder.getActionView().setOnClickListener(
                    view -> action.getDelegate().onActionTriggered(action));
        }
    }

    static class TabViewBinder
            implements ListModelChangeProcessor
                               .ViewBinder<SimpleListObservable<Tab>, KeyboardAccessoryView> {
        @Override
        public void onItemsInserted(
                SimpleListObservable<Tab> model, KeyboardAccessoryView view, int index, int count) {
            assert count > 0 : "Tried to insert invalid amount of tabs - must be at least one.";
            while (count-- > 0) {
                Tab tab = model.get(index);
                view.addTabAt(index, tab.getIcon(), tab.getContentDescription());
                ++index;
            }
        }

        @Override
        public void onItemsRemoved(
                SimpleListObservable<Tab> model, KeyboardAccessoryView view, int index, int count) {
            assert count > 0 : "Tried to remove invalid amount of tabs - must be at least one.";
            while (count-- > 0) {
                view.removeTabAt(index++);
            }
        }

        @Override
        public void onItemsChanged(
                SimpleListObservable<Tab> model, KeyboardAccessoryView view, int index, int count) {
            // TODO(fhorschig): Implement fine-grained, ranged changes should the need arise.
            updateAllTabs(view, model);
        }

        void updateAllTabs(KeyboardAccessoryView view, SimpleListObservable<Tab> model) {
            view.clearTabs();
            for (int i = 0; i < model.getItemCount(); ++i) {
                Tab tab = model.get(i);
                view.addTabAt(i, tab.getIcon(), tab.getContentDescription());
            }
        }
    }

    @Override
    public PropertyKey getVisibilityProperty() {
        return PropertyKey.VISIBLE;
    }

    @Override
    public boolean isVisible(KeyboardAccessoryModel model) {
        return model.isVisible();
    }

    @Override
    public void onInitialInflation(
            KeyboardAccessoryModel model, KeyboardAccessoryView inflatedView) {
        for (PropertyKey key : PropertyKey.ALL_PROPERTIES) {
            bind(model, inflatedView, key);
        }

        inflatedView.setActionsAdapter(KeyboardAccessoryCoordinator.createActionsAdapter(model));
        KeyboardAccessoryCoordinator.createTabViewBinder(model, inflatedView)
                .updateAllTabs(inflatedView, model.getTabList());
    }

    @Override
    public void bind(
            KeyboardAccessoryModel model, KeyboardAccessoryView view, PropertyKey propertyKey) {
        if (propertyKey == PropertyKey.VISIBLE) {
            view.setVisible(model.isVisible());
            return;
        }
        if (propertyKey == PropertyKey.SUGGESTIONS) {
            view.updateSuggestions(model.getAutofillSuggestions());
            return;
        }
        assert false : "Every possible property update needs to be handled!";
    }
}
