// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.api.stream;

import android.support.annotation.IntDef;

/** Interface users can implement to be told about changes to scrolling in the Stream. */
public interface ScrollListener {
  void onScrollStateChanged(@ScrollState int state);

  void onScrolled(int dx, int dy);

  /** Possible scroll states. */
  @IntDef({ScrollState.IDLE, ScrollState.DRAGGING, ScrollState.SETTLING})
  @interface ScrollState {
    /** Stream is not scrolling */
    int IDLE = 0;

    /** Stream is currently scrolling through external means such as user input. */
    int DRAGGING = 1;

    /** Stream is animating to a final position. */
    int SETTLING = 2;
  }
}
