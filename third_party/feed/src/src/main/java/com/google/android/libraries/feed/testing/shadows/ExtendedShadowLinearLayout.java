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

package com.google.android.libraries.feed.testing.shadows;

import android.widget.LinearLayout;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowLinearLayout;

/**
 * Robolectric Shadow for {@link LinearLayout}.
 *
 * <p>Provides functionality to test view position.
 */
@Implements(LinearLayout.class)
public class ExtendedShadowLinearLayout extends ShadowLinearLayout {
  private int xLocation = 0;
  private int yLocation = 0;
  private int height = 0;
  private int width = 0;
  private boolean attached = false;

  @Implementation
  public void getLocationOnScreen(int[] pos) {
    pos[0] = xLocation;
    pos[1] = yLocation;
  }

  public void setLocationOnScreen(int x, int y) {
    xLocation = x;
    yLocation = y;
  }

  @Implementation
  public void getLocationInWindow(int[] pos) {
    pos[0] = xLocation;
    pos[1] = yLocation;
  }

  public void setLocationInWindow(int x, int y) {
    xLocation = x;
    yLocation = y;
  }

  @Override
  @Implementation
  public boolean isAttachedToWindow() {
    return attached;
  }

  public void setAttachedToWindow(boolean attached) {
    this.attached = attached;
  }

  @Implementation
  public int getHeight() {
    return height;
  }

  public void setHeight(int height) {
    this.height = height;
  }

  @Implementation
  public int getWidth() {
    return width;
  }

  public void setWidth(int width) {
    this.width = width;
  }
}
