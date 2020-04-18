// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.support.v7.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.keyboard_accessory.PasswordAccessorySheetViewBinder.TextViewHolder;
import org.chromium.chrome.browser.modelutil.RecyclerViewAdapter;
import org.chromium.chrome.browser.modelutil.RecyclerViewModelChangeProcessor;

/**
 * This component is a tab that can be added to the {@link ManualFillingCoordinator} which shows it
 * as bottom sheet below the keyboard accessory.
 */
public class PasswordAccessorySheetCoordinator implements KeyboardAccessoryData.Tab {
    private final Context mContext;
    private final PasswordAccessorySheetModel mModel = new PasswordAccessorySheetModel();

    /**
     * Creates the passwords tab.
     * @param context The {@link Context} containing resources like icons and layouts for this tab.
     */
    public PasswordAccessorySheetCoordinator(Context context) {
        mContext = context;
    }

    @Override
    public Drawable getIcon() {
        return mContext.getResources().getDrawable(R.drawable.ic_vpn_key_grey);
    }

    @Override
    public String getContentDescription() {
        // TODO(fhorschig): Load resource from strings or via mediator from native side.
        return null;
    }

    @Override
    public int getTabLayout() {
        return R.layout.password_accessory_sheet;
    }

    @Override
    public Listener getListener() {
        final PasswordAccessorySheetViewBinder binder = new PasswordAccessorySheetViewBinder();
        return view
                -> binder.initializeView((RecyclerView) view,
                        PasswordAccessorySheetCoordinator.createAdapter(mModel, binder));
    }

    /**
     * Creates an adapter to an {@link PasswordAccessorySheetViewBinder} that is wired
     * up to the model change processor which listens to the {@link PasswordAccessorySheetModel}.
     * @param model the {@link PasswordAccessorySheetModel} the adapter gets its data from.
     * @return Returns a fully initialized and wired adapter to a PasswordAccessorySheetViewBinder.
     */
    static RecyclerViewAdapter<PasswordAccessorySheetModel, TextViewHolder> createAdapter(
            PasswordAccessorySheetModel model, PasswordAccessorySheetViewBinder viewBinder) {
        RecyclerViewAdapter<PasswordAccessorySheetModel, TextViewHolder> adapter =
                new PasswordAccessorySheetViewAdapter<>(model, viewBinder);
        model.addObserver(new RecyclerViewModelChangeProcessor<>(adapter));
        return adapter;
    }
}