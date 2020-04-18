/*
 * Copyright (C) 2012 Google Inc.
 * Licensed to The Android Open Source Project.
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

package com.android.ex.photo.views;

import android.view.View;
import android.widget.ProgressBar;

/**
 * This class wraps around two progress bars and is solely designed to fix
 * a bug in the framework (b/6928449) that prevents a progress bar from
 * gracefully switching back and forth between indeterminate and determinate
 * modes.
 */
public class ProgressBarWrapper {
    private final ProgressBar mDeterminate;
    private final ProgressBar mIndeterminate;
    private boolean mIsIndeterminate;

    public ProgressBarWrapper(ProgressBar determinate,
            ProgressBar indeterminate, boolean isIndeterminate) {
        mDeterminate = determinate;
        mIndeterminate = indeterminate;
        setIndeterminate(isIndeterminate);
    }

    public void setIndeterminate(boolean isIndeterminate) {
        mIsIndeterminate = isIndeterminate;

        setVisibility(mIsIndeterminate);
    }

    public void setVisibility(int visibility) {
        if (visibility == View.INVISIBLE || visibility == View.GONE) {
            mIndeterminate.setVisibility(visibility);
            mDeterminate.setVisibility(visibility);
        } else {
            setVisibility(mIsIndeterminate);
        }
    }

    private void setVisibility(boolean isIndeterminate) {
        mIndeterminate.setVisibility(isIndeterminate ? View.VISIBLE : View.GONE);
        mDeterminate.setVisibility(isIndeterminate ? View.GONE : View.VISIBLE);
    }

    public void setMax(int max) {
        mDeterminate.setMax(max);
    }

    public void setProgress(int progress) {
        mDeterminate.setProgress(progress);
    }
}
