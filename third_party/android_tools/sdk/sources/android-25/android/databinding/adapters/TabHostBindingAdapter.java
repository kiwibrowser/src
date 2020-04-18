/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.databinding.adapters;

import android.databinding.BindingAdapter;
import android.databinding.InverseBindingAdapter;
import android.databinding.InverseBindingListener;
import android.widget.TabHost;
import android.widget.TabHost.OnTabChangeListener;

public class TabHostBindingAdapter {

    @InverseBindingAdapter(attribute = "android:currentTab")
    public static int getCurrentTab(TabHost view) {
        return view.getCurrentTab();
    }

    @InverseBindingAdapter(attribute = "android:currentTab")
    public static String getCurrentTabTag(TabHost view) {
        return view.getCurrentTabTag();
    }

    @BindingAdapter("android:currentTab")
    public static void setCurrentTab(TabHost view, int tab) {
        if (view.getCurrentTab() != tab) {
            view.setCurrentTab(tab);
        }
    }

    @BindingAdapter("android:currentTab")
    public static void setCurrentTabTag(TabHost view, String tabTag) {
        if (view.getCurrentTabTag() != tabTag) {
            view.setCurrentTabByTag(tabTag);
        }
    }

    @BindingAdapter(value = {"android:onTabChanged", "android:currentTabAttrChanged"},
            requireAll = false)
    public static void setListeners(TabHost view, final OnTabChangeListener listener,
            final InverseBindingListener attrChange) {
        if (attrChange == null) {
            view.setOnTabChangedListener(listener);
        } else {
            view.setOnTabChangedListener(new OnTabChangeListener() {
                @Override
                public void onTabChanged(String tabId) {
                    if (listener != null) {
                        listener.onTabChanged(tabId);
                    }
                    attrChange.onChange();
                }
            });
        }
    }
}
