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

package com.google.android.libraries.feed.piet.host;

import android.graphics.drawable.Drawable;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.search.now.ui.piet.ImagesProto.Image;

/** Provide Assets from the host */
public interface AssetProvider {
  /**
   * Given an {@link Image}, asynchronously load the {@link Drawable} and return via a {@link
   * Consumer}.
   */
  void getImage(Image image, Consumer<Drawable> consumer);

  /** Return a relative elapsed time string such as "8 minutes ago" or "1 day ago". */
  String getRelativeElapsedString(long elapsedTimeMillis);

  /** Returns the default corner rounding radius in pixels. */
  int getDefaultCornerRadius();
}
