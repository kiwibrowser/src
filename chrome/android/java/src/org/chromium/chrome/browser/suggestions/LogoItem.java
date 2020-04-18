// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.res.Resources;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.LogoView;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.cards.Leaf;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.cards.NodeVisitor;

/**
 * A logo for the search engine provider.
 */
public class LogoItem extends Leaf {
    private boolean mVisible;

    @Override
    @ItemViewType
    protected int getItemViewType() {
        return ItemViewType.LOGO;
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        ((ViewHolder) holder).setVisible(mVisible);
    }

    @Override
    public void visitItems(NodeVisitor visitor) {
        visitor.visitLogo();
    }

    public void setVisible(boolean visible) {
        mVisible = visible;
        notifyItemChanged(0, holder -> ((ViewHolder) holder).setVisible(visible));
    }

    /**
     * The {@code ViewHolder} for the {@link LogoItem}.
     */
    public static class ViewHolder extends NewTabPageViewHolder {
        public ViewHolder(LogoView logoView) {
            super(logoView);
        }

        @Override
        public void recycle() {
            if (itemView.getParent() != null) {
                ((ViewGroup) itemView.getParent()).removeView(itemView);
            }
            super.recycle();
        }

        public void setVisible(boolean visible) {
            itemView.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
            Resources resources = itemView.getResources();
            MarginLayoutParams params = (MarginLayoutParams) itemView.getLayoutParams();
            params.height = visible
                    ? resources.getDimensionPixelSize(R.dimen.ntp_logo_height)
                    : resources.getDimensionPixelSize(R.dimen.chrome_home_logo_minimum_height);
            int top = visible ? resources.getDimensionPixelSize(R.dimen.ntp_logo_margin_top_modern)
                              : 0;
            int bottom = visible
                    ? resources.getDimensionPixelSize(R.dimen.ntp_logo_margin_bottom_modern)
                    : 0;
            params.setMargins(0, top, 0, bottom);
            itemView.setLayoutParams(params);
        }
    }
}
