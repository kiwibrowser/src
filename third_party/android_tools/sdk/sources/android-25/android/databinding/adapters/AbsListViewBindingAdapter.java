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
import android.databinding.BindingMethod;
import android.databinding.BindingMethods;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;

@BindingMethods({
        @BindingMethod(type = AbsListView.class, attribute = "android:listSelector", method = "setSelector"),
        @BindingMethod(type = AbsListView.class, attribute = "android:scrollingCache", method = "setScrollingCacheEnabled"),
        @BindingMethod(type = AbsListView.class, attribute = "android:smoothScrollbar", method = "setSmoothScrollbarEnabled"),
        @BindingMethod(type = AbsListView.class, attribute = "android:onMovedToScrapHeap", method = "setRecyclerListener"),
})
public class AbsListViewBindingAdapter {

    @BindingAdapter(value = {"android:onScroll", "android:onScrollStateChanged"},
            requireAll = false)
    public static void setOnScroll(AbsListView view, final OnScroll scrollListener,
            final OnScrollStateChanged scrollStateListener) {
        view.setOnScrollListener(new OnScrollListener() {
            @Override
            public void onScrollStateChanged(AbsListView view, int scrollState) {
                if (scrollStateListener != null) {
                    scrollStateListener.onScrollStateChanged(view, scrollState);
                }
            }

            @Override
            public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount,
                    int totalItemCount) {
                if (scrollListener != null) {
                    scrollListener
                            .onScroll(view, firstVisibleItem, visibleItemCount, totalItemCount);
                }
            }
        });
    }

    public interface OnScroll {
        void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount,
                int totalItemCount);
    }

    public interface OnScrollStateChanged {
        void onScrollStateChanged(AbsListView view, int scrollState);
    }
}
