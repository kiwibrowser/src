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

import android.annotation.TargetApi;
import android.databinding.BindingAdapter;
import android.databinding.BindingMethod;
import android.databinding.BindingMethods;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.widget.SearchView;
import android.widget.SearchView.OnQueryTextListener;
import android.widget.SearchView.OnSuggestionListener;

@BindingMethods({
        @BindingMethod(type = SearchView.class, attribute = "android:onQueryTextFocusChange", method = "setOnQueryTextFocusChangeListener"),
        @BindingMethod(type = SearchView.class, attribute = "android:onSearchClick", method = "setOnSearchClickListener"),
        @BindingMethod(type = SearchView.class, attribute = "android:onClose", method = "setOnCloseListener"),
})
public class SearchViewBindingAdapter {
    @BindingAdapter(value = {"android:onQueryTextSubmit", "android:onQueryTextChange"},
            requireAll = false)
    public static void setOnQueryTextListener(SearchView view, final OnQueryTextSubmit submit,
            final OnQueryTextChange change) {
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            if (submit == null && change == null){
                view.setOnQueryTextListener(null);
            } else {
                view.setOnQueryTextListener(new OnQueryTextListener() {
                    @Override
                    public boolean onQueryTextSubmit(String query) {
                        if (submit != null) {
                            return submit.onQueryTextSubmit(query);
                        } else {
                            return false;
                        }
                    }

                    @Override
                    public boolean onQueryTextChange(String newText) {
                        if (change != null) {
                            return change.onQueryTextChange(newText);
                        } else {
                            return false;
                        }
                    }
                });
            }
        }
    }

    @BindingAdapter(value = {"android:onSuggestionSelect", "android:onSuggestionClick"},
            requireAll = false)
    public static void setOnSuggestListener(SearchView view, final OnSuggestionSelect submit,
            final OnSuggestionClick change) {
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            if (submit == null && change == null) {
                view.setOnSuggestionListener(null);
            } else {
                view.setOnSuggestionListener(new OnSuggestionListener() {
                    @Override
                    public boolean onSuggestionSelect(int position) {
                        if (submit != null) {
                            return submit.onSuggestionSelect(position);
                        } else {
                            return false;
                        }
                    }

                    @Override
                    public boolean onSuggestionClick(int position) {
                        if (change != null) {
                            return change.onSuggestionClick(position);
                        } else {
                            return false;
                        }
                    }
                });
            }
        }
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public interface OnQueryTextSubmit {
        boolean onQueryTextSubmit(String query);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public interface OnQueryTextChange {
        boolean onQueryTextChange(String newText);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public interface OnSuggestionSelect {
        boolean onSuggestionSelect(int position);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public interface OnSuggestionClick {
        boolean onSuggestionClick(int position);
    }
}
